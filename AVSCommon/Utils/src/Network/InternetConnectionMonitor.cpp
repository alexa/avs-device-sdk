/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include "AVSCommon/Utils/Network/InternetConnectionMonitor.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace network {

using namespace avsCommon::sdkInterfaces;
using namespace utils::timing;

/// String to identify log entries originating from this file.
static const std::string TAG("InternetConnectionMonitor");

/**
 * Create a @c LogEntry using this file's @c TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The number of bytes read from the attachment with each read in the read loop.
static const size_t CHUNK_SIZE(1024);

/// The amount of time to wait before retesting for internet connection availability.
static const std::chrono::minutes DEFAULT_TEST_PERIOD{5};

/// The URL to fetch content from.
static const std::string S3_TEST_URL = "http://spectrum.s3.amazonaws.com/kindle-wifi/wifistub.html";

/// The string that will serve as validation that the HTTP content was received correctly.
static const std::string VALIDATION_STRING = "81ce4465-7167-4dcb-835b-dcc9e44c112a";

std::unique_ptr<InternetConnectionMonitor> InternetConnectionMonitor::create(
    std::shared_ptr<sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory) {
    if (!contentFetcherFactory) {
        ACSDK_ERROR(LX("createFailed").d("reason", "contentFetcherFactory was nullptr"));
        return nullptr;
    }

    return std::unique_ptr<InternetConnectionMonitor>(new InternetConnectionMonitor(contentFetcherFactory));
}

InternetConnectionMonitor::InternetConnectionMonitor(
    std::shared_ptr<sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory) :
        m_connected{false},
        m_period{DEFAULT_TEST_PERIOD},
        m_contentFetcherFactory{contentFetcherFactory} {
    /// Using the test URL as the stream id.
    m_stream = std::make_shared<avs::attachment::InProcessAttachment>(S3_TEST_URL);
    startMonitoring();
}

void InternetConnectionMonitor::addInternetConnectionObserver(
    std::shared_ptr<InternetConnectionObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("addObserverFailed").d("reason", "nullObserver"));
        return;
    }

    bool connectedCopy = false;
    bool observerAddedSuccessfully = false;

    {
        std::unique_lock<std::mutex> lock{m_mutex};
        connectedCopy = m_connected;
        observerAddedSuccessfully = m_observers.insert(observer).second;
    }

    // Inform the new observer of the current connection status.
    if (observerAddedSuccessfully) {
        observer->onConnectionStatusChanged(connectedCopy);
    }
}

void InternetConnectionMonitor::removeInternetConnectionObserver(
    std::shared_ptr<InternetConnectionObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("removeObserverFailed").d("reason", "nullObserver"));
        return;
    }

    std::lock_guard<std::mutex> lock{m_mutex};
    m_observers.erase(observer);
}

void InternetConnectionMonitor::startMonitoring() {
    ACSDK_DEBUG5(LX(__func__));

    // Start a timer with an initial delay of 0 to kick off the first test, then
    // allow the subsequent tests to start after m_period.
    m_connectionTestTimer.start(
        std::chrono::seconds(0),
        m_period,
        Timer::PeriodType::RELATIVE,
        Timer::FOREVER,
        std::bind(&InternetConnectionMonitor::testConnection, this));
}

void InternetConnectionMonitor::stopMonitoring() {
    ACSDK_DEBUG5(LX(__func__));
    m_connectionTestTimer.stop();
}

void InternetConnectionMonitor::testConnection() {
    ACSDK_DEBUG5(LX(__func__));

    auto contentFetcher = m_contentFetcherFactory->create(S3_TEST_URL);
    auto httpContent = contentFetcher->getContent(HTTPContentFetcherInterface::FetchOptions::ENTIRE_BODY);
    if (!httpContent) {
        ACSDK_ERROR(LX("testConnectionFailed").d("reason", "nullHTTPContentReceived"));
        updateConnectionStatus(false);
        return;
    }
    if (!httpContent->isReady(std::chrono::duration_cast<std::chrono::milliseconds>(m_period))) {
        ACSDK_ERROR(LX("testConnectionFailed").d("reason", "getHttpContentTimeout"));
        updateConnectionStatus(false);
        return;
    }
    if (!(*httpContent)) {
        ACSDK_ERROR(LX("testConnectionFailed").d("reason", "badHTTPContentReceived"));
        updateConnectionStatus(false);
        return;
    }

    auto reader = httpContent->dataStream->createReader(sds::ReaderPolicy::BLOCKING);
    if (!reader) {
        ACSDK_ERROR(LX("testConnectionFailed").d("reason", "failedToCreateStreamReader"));
        return;
    }
    auto readStatus = avs::attachment::AttachmentReader::ReadStatus::OK;
    std::string testContent;
    std::vector<char> buffer(CHUNK_SIZE, 0);
    bool streamClosed = false;
    while (!streamClosed) {
        auto bytesRead = reader->read(buffer.data(), buffer.size(), &readStatus);
        switch (readStatus) {
            case avs::attachment::AttachmentReader::ReadStatus::CLOSED:
                streamClosed = true;
                if (bytesRead == 0) {
                    break;
                }
            /* FALL THROUGH - to add any data received even if closed */
            case avs::attachment::AttachmentReader::ReadStatus::OK:
            case avs::attachment::AttachmentReader::ReadStatus::OK_WOULDBLOCK:
            case avs::attachment::AttachmentReader::ReadStatus::OK_TIMEDOUT:
                testContent.append(buffer.data(), bytesRead);
                break;
            case avs::attachment::AttachmentReader::ReadStatus::ERROR_OVERRUN:
            case avs::attachment::AttachmentReader::ReadStatus::ERROR_BYTES_LESS_THAN_WORD_SIZE:
            case avs::attachment::AttachmentReader::ReadStatus::ERROR_INTERNAL:
                ACSDK_ERROR(LX("testConnectionFailed").d("reason", "readError"));
                updateConnectionStatus(false);
                return;
        }
    }

    // Check that the HTTP content received is what we expected.
    bool found = (testContent.find(VALIDATION_STRING) != std::string::npos);
    updateConnectionStatus(found);
}

void InternetConnectionMonitor::notifyObserversLocked() {
    ACSDK_DEBUG5(LX(__func__));
    for (auto& observer : m_observers) {
        observer->onConnectionStatusChanged(m_connected);
    }
}

void InternetConnectionMonitor::updateConnectionStatus(bool connected) {
    ACSDK_DEBUG5(LX(__func__).d("connected", connected));

    std::lock_guard<std::mutex> lock{m_mutex};
    if (m_connected != connected) {
        m_connected = connected;
        notifyObserversLocked();
    }
}

InternetConnectionMonitor::~InternetConnectionMonitor() {
    stopMonitoring();
}

}  // namespace network
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
