/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "UrlRequester.h"

#include <AVSCommon/Utils/Error/FinallyGuard.h>
#include <AVSCommon/Utils/FileSystem/FileSystemUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Power/WakeGuard.h>

#include <regex>

#include "acsdkAssetsCommon/AmdMetricWrapper.h"
#include "acsdkAssetsCommon/CurlWrapper.h"
#include "acsdkAssetsCommon/JitterUtil.h"
#include "acsdkAssetsInterfaces/UrlRequest.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace manager {

using namespace std;
using namespace chrono;
using namespace client;
using namespace common;
using namespace commonInterfaces;
using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::utils::error;
using namespace alexaClientSDK::avsCommon::utils::power;

#if UNIT_TEST == 1
// For tests, because we don't want to wait hours for it to finish...
static constexpr auto BASE_BACKOFF_VALUE = milliseconds(10);
static constexpr auto MAX_DOWNLOAD_RETRY = 2;
#else
static constexpr auto BASE_BACKOFF_VALUE = milliseconds(200);
static constexpr auto MAX_DOWNLOAD_RETRY = 10;
#endif

static const auto s_metrics = AmdMetricsWrapper::creator("urlRequester");

/// String to identify log entries originating from this file.
static const std::string TAG{"UrlRequester"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

static constexpr size_t DEFAULT_EXPECTED_URL_SIZE = 1 * 1024 * 1024;

static string getValueFromHeaders(const string& headers, const string& key) {
    regex rgx(key + " ?: ?(.*)");
    smatch matches;
    if (regex_search(headers, matches, rgx) && matches.size() == 2) {
        return matches[1].str();
    }

    return "";
}

UrlRequester::UrlRequester(
        std::shared_ptr<StorageManager> storageManager,
        std::shared_ptr<AmdCommunicationInterface> communicationHandler,
        std::shared_ptr<RequesterMetadata> metadata,
        std::string metadataFilePath,
        std::string workingDirectory,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
        std::shared_ptr<alexaClientSDK::avsCommon::utils::power::PowerResource> powerResource,
        std::shared_ptr<UrlAllowListWrapper> allowUrlList) :
        Requester(
                std::move(storageManager),
                std::move(communicationHandler),
                std::move(metadata),
                std::move(metadataFilePath)),
        m_workingDirectory(move(workingDirectory)),
        m_downloadProgressTrigger(std::make_shared<CurlProgressCallback>()),
        m_authDelegate(move(authDelegate)),
        m_powerResource(move(powerResource)),
        m_allowUrlList(move(allowUrlList)) {
}

UrlRequester::~UrlRequester() {
    m_downloadProgressTrigger->cancel();
    if (m_downloadFuture.valid()) {
        m_downloadFuture.wait();
    }
}

size_t UrlRequester::deleteAndCleanupLocked(unique_lock<mutex>& lock) {
    m_downloadProgressTrigger->cancel();
    return Requester::deleteAndCleanupLocked(lock);
}

bool UrlRequester::download() {
    ACSDK_INFO(LX("download").m("Requesting download").d("request", name()));
    unique_lock<mutex> lock(m_eventMutex);
    auto state = getState();
    if (State::INVALID != state && State::INIT != state) {
        ACSDK_INFO(LX("download").m("Download is unnecessary").d("request", name()).d("state", state));
        return true;
    }

    if (!registerCommunicationHandlerPropsLocked()) {
        ACSDK_ERROR(LX("download").m("Could not register Communication Handler properties").d("request", name()));
        handleDownloadFailureLocked(lock);
        return false;
    }

    if (!m_allowUrlList->isUrlAllowed(static_pointer_cast<UrlRequest>(m_metadata->getRequest())->getUrl())) {
        ACSDK_ERROR(LX("download").m("Requested URL is NOT approved").d("request", name()));
        handleDownloadFailureLocked(lock);
        return false;
    }

    setStateLocked(State::DOWNLOADING);
    m_storageReservationToken.reset();
    m_downloadFuture = async(launch::async, &UrlRequester::downloadWorker, this);

    ACSDK_INFO(LX("download").m("Creating a request").d("request", name()));
    return true;
}

void UrlRequester::downloadWorker() {
    // Will create and acquire PowerResource. Will be released when the variable goes out of scope
    WakeGuard guard(m_powerResource);
    auto request = static_pointer_cast<UrlRequest>(m_metadata->getRequest());
    auto unpack = request->needsUnpacking();
    // each URL download should have its own unique tmp dir name
    auto path = m_workingDirectory + "/" + (unpack ? request->getSummary() : request->getFilename());
    FinallyGuard deleteTmpPath([&] { filesystem::removeAll(path); });
    auto waitTime = milliseconds(0);

    auto curl = CurlWrapper::create(false, m_authDelegate, request->getCertPath());
    if (nullptr == curl) {
        ACSDK_ERROR(LX("downloadWorker").m("Could not create curl wrapper"));
        unique_lock<mutex> lock(m_eventMutex);
        handleDownloadFailureLocked(lock);
        return;
    }

    auto headerResult = curl->getHeaders(request->getUrl());
    string contentLength = getValueFromHeaders(headerResult.value(), "Content-Length");

    size_t expectedSize = atoi(contentLength.c_str());

    if (0 == expectedSize) {
        ACSDK_INFO(LX("downloadWorker")
                           .m("ContentLength was invalid or missing")
                           .d("Defaulting to size", DEFAULT_EXPECTED_URL_SIZE));
        expectedSize = DEFAULT_EXPECTED_URL_SIZE;
    }

    auto reservation = m_storageManager->reserveSpace(expectedSize);
    if (reservation == nullptr) {
        ACSDK_ERROR(LX("downloadWorker").m("Could not free up enough space").d("request", name()));
        unique_lock<mutex> lock(m_eventMutex);
        handleDownloadFailureLocked(lock);
        return;
    }

    unique_lock<mutex> lock(m_eventMutex);
    m_storageReservationToken = move(reservation);
    for (auto i = 0; i < MAX_DOWNLOAD_RETRY; ++i) {
        m_stateTrigger.wait_for(lock, waitTime, [this] { return State::DOWNLOADING != getState(); });
        if (State::DOWNLOADING != getState()) {
            ACSDK_INFO(LX("downloadWorker").m("Cancelling download").d("request", name()));
            return;
        }
        waitTime = jitterUtil::expJitter(max(waitTime, BASE_BACKOFF_VALUE));

        m_downloadProgressTrigger->enable(expectedSize);
        lock.unlock();
        if (request->needsUnpacking()) {
            // Creating the unpacked subdirectory explicitly with default 750 permission
            filesystem::makeDirectory(path);
        }
        auto result = curl->download(request->getUrl(), path, m_downloadProgressTrigger, request->needsUnpacking());
        lock.lock();

        if (State::DOWNLOADING != getState()) {
            ACSDK_ERROR(LX("downloadWorker").m("Cancelling download").d("request", name()));
            handleDownloadFailureLocked(lock);
            return;
        }

        if (ResultCode::SUCCESS == result) {
            auto newResource = m_storageManager->registerAndAcquireResource(
                    move(m_storageReservationToken), request->getSummary(), path);
            if (nullptr == newResource) {
                ACSDK_ERROR(LX("downloadWorker").m("Failed to register and acquire the resource").d("request", name()));
                handleDownloadFailureLocked(lock);
                return;
            }

            handleAcquiredResourceLocked(lock, newResource);
            return;
        }

        if (i < MAX_DOWNLOAD_RETRY - 1) {
            ACSDK_INFO(LX("downloadWorker").m("Download attempt failed. Retrying...").d("attempt", i));
        }
    }

    ACSDK_ERROR(LX("downloadWorker").m("Failed to download").d("After attempt", MAX_DOWNLOAD_RETRY));
    handleDownloadFailureLocked(lock);
}

}  // namespace manager
}  // namespace acsdkAssets
}  // namespace alexaClientSDK
