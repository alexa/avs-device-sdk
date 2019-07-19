/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <algorithm>
#include <chrono>

#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterface.h>
#include <AVSCommon/Utils/HTTP/HttpResponseCode.h>
#include <AVSCommon/Utils/LibcurlUtils/CurlEasyHandleWrapper.h>
#include "AVSCommon/Utils/LibcurlUtils/CurlMultiHandleWrapper.h"
#include <AVSCommon/Utils/LibcurlUtils/LibCurlHttpContentFetcher.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/SDS/InProcessSDS.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace libcurlUtils {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::http;
using namespace avsCommon::utils::libcurlUtils;

/// String to identify log entries originating from this file.
static const std::string TAG("LibCurlHttpContentFetcher");

/**
 * The timeout for a blocking write call to an @c AttachmentWriter. This value may be increased to decrease wakeups but
 * may also increase latency.
 */
static const std::chrono::milliseconds TIMEOUT_FOR_BLOCKING_WRITE = std::chrono::milliseconds(100);
/// Timeout for polling loops that check activities running on separate threads.
static const std::chrono::milliseconds WAIT_FOR_ACTIVITY_TIMEOUT{100};
/// Timeout for curl connection.
static const auto TIMEOUT_CONNECTION = std::chrono::seconds(30);
/// Timeout to wait for a call to get body after getContent was called
static const std::chrono::minutes MAX_GET_BODY_WAIT{1};
/// Timeout to wait for get header to complete
static const std::chrono::minutes MAX_GET_HEADER_WAIT{5};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

size_t LibCurlHttpContentFetcher::headerCallback(char* data, size_t size, size_t nmemb, void* userData) {
    if (!userData) {
        ACSDK_ERROR(LX("headerCallback").d("reason", "nullUserDataPointer"));
        return 0;
    }
    auto fetcher = static_cast<LibCurlHttpContentFetcher*>(userData);

    ACSDK_DEBUG9(LX(__func__).sensitive("url", fetcher->m_url).m("CALLED"));

    fetcher->stateTransition(State::FETCHING_HEADER, true);

    std::string line(static_cast<const char*>(data), size * nmemb);
    std::transform(line.begin(), line.end(), line.begin(), ::tolower);
    if (line.find("http") == 0) {
        // To find lines like: "HTTP/1.1 200 OK"
        std::istringstream iss(line);
        std::string httpVersion;
        int statusCode = 0;
        iss >> httpVersion >> statusCode;
        fetcher->m_header.responseCode = intToHTTPResponseCode(statusCode);
    } else if (line.find("content-type") == 0) {
        // To find lines like: "Content-Type: audio/x-mpegurl; charset=utf-8"
        std::istringstream iss(line);
        std::string contentTypeBeginning;
        std::string contentType;
        iss >> contentTypeBeginning >> contentType;

        size_t separator = contentType.find(";");
        if (separator != std::string::npos) {
            // Remove characters after the separator ;
            contentType.erase(separator);
        }
        fetcher->m_header.contentType = contentType;
    } else if (line.find("content-length") == 0) {
        // To find lines like: "Content-Length: 12345"
        std::istringstream iss(line);
        std::string contentLengthBeginning;
        iss >> contentLengthBeginning >> fetcher->m_header.contentLength;
        ACSDK_DEBUG9(LX(__func__).d("type", "content-length").d("length", fetcher->m_header.contentLength));
    } else if (line.find("content-range") == 0) {
        // To find lines like: "Content-Range: bytes 1000-3979/3980"
        std::istringstream iss(line);
        std::string contentRangeBeginning;
        std::string contentUnit;
        std::string range;
        iss >> contentRangeBeginning >> contentUnit >> range;
        ACSDK_DEBUG9(LX(__func__).d("type", "content-range").d("unit", contentUnit).d("range", range));
    }
    return size * nmemb;
}

size_t LibCurlHttpContentFetcher::bodyCallback(char* data, size_t size, size_t nmemb, void* userData) {
    if (!userData) {
        ACSDK_ERROR(LX("bodyCallback").d("reason", "nullUserDataPointer"));
        return 0;
    }
    auto fetcher = static_cast<LibCurlHttpContentFetcher*>(userData);
    if (fetcher->m_done) {
        // In order to properly quit when downloading live content, which block forever when performing a GET request
        return 0;
    }

    if (State::FETCHING_HEADER == fetcher->getState()) {
        ACSDK_DEBUG9(LX(__func__).sensitive("url", fetcher->m_url).m("End of header found."));
        fetcher->stateTransition(State::HEADER_DONE, true);
    }

    // Waits until the content fetcher is shutting down or the @c getBody method gets called.
    auto startTime = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::steady_clock::now() - startTime;
    while ((MAX_GET_BODY_WAIT > elapsedTime) && !fetcher->m_isShutdown && fetcher->waitingForBodyRequest()) {
        std::this_thread::sleep_for(WAIT_FOR_ACTIVITY_TIMEOUT);
        elapsedTime = std::chrono::steady_clock::now() - startTime;
    }
    if (MAX_GET_BODY_WAIT <= elapsedTime) {
        ACSDK_ERROR(LX(__func__).d("reason", "getBodyCallWaitTimeout"));
        fetcher->stateTransition(State::ERROR, false);
        return 0;
    }
    if (fetcher->m_isShutdown) {
        return 0;
    }

    fetcher->stateTransition(State::FETCHING_BODY, true);

    if (!fetcher->m_streamWriter) {
        ACSDK_DEBUG9(LX(__func__).m("No writer received. Creating a new one."));
        // Using the url as the identifier for the attachment
        auto stream = std::make_shared<avsCommon::avs::attachment::InProcessAttachment>(fetcher->m_url);
        fetcher->m_streamWriter = stream->createWriter(sds::WriterPolicy::BLOCKING);
    }

    auto streamWriter = fetcher->m_streamWriter;
    size_t totalBytesWritten = 0;

    if (streamWriter) {
        size_t targetNumBytes = size * nmemb;

        while ((totalBytesWritten < targetNumBytes) && !fetcher->m_done) {
            auto writeStatus = avsCommon::avs::attachment::AttachmentWriter::WriteStatus::OK;

            size_t numBytesWritten = streamWriter->write(
                data,
                targetNumBytes - totalBytesWritten,
                &writeStatus,
                std::chrono::milliseconds(TIMEOUT_FOR_BLOCKING_WRITE));
            totalBytesWritten += numBytesWritten;
            data += numBytesWritten;

            switch (writeStatus) {
                case avsCommon::avs::attachment::AttachmentWriter::WriteStatus::CLOSED:
                case avsCommon::avs::attachment::AttachmentWriter::WriteStatus::ERROR_BYTES_LESS_THAN_WORD_SIZE:
                case avsCommon::avs::attachment::AttachmentWriter::WriteStatus::ERROR_INTERNAL:
                    return totalBytesWritten;
                case avsCommon::avs::attachment::AttachmentWriter::WriteStatus::TIMEDOUT:
                case avsCommon::avs::attachment::AttachmentWriter::WriteStatus::OK:
                    // might still have bytes to write
                    continue;
                case avsCommon::avs::attachment::AttachmentWriter::WriteStatus::OK_BUFFER_FULL:
                    ACSDK_ERROR(LX(__func__).d("unexpected return code", "OK_BUFFER_FULL"));
                    return 0;
            }
            ACSDK_ERROR(LX("UnexpectedWriteStatus").d("writeStatus", static_cast<int>(writeStatus)));
            return 0;
        }
    }

    fetcher->m_totalContentReceivedLength += totalBytesWritten;
    fetcher->m_currentContentReceivedLength += totalBytesWritten;

    ACSDK_DEBUG9(LX(__func__)
                     .d("totalContentReceived", fetcher->m_totalContentReceivedLength)
                     .d("contentLength", fetcher->m_header.contentLength)
                     .d("currentContentReceived", fetcher->m_currentContentReceivedLength)
                     .d("remaining", fetcher->m_header.contentLength - fetcher->m_currentContentReceivedLength)
                     .d("totalBytesWritten", totalBytesWritten));

    return totalBytesWritten;
}

size_t LibCurlHttpContentFetcher::noopCallback(char* data, size_t size, size_t nmemb, void* userData) {
    return 0;
}

LibCurlHttpContentFetcher::LibCurlHttpContentFetcher(const std::string& url) :
        m_state{HTTPContentFetcherInterface::State::INITIALIZED},
        m_url{url},
        m_currentContentReceivedLength{0},
        m_totalContentReceivedLength{0},
        m_done{false},
        m_isShutdown{false} {
    m_hasObjectBeenUsed.clear();
    m_headerFuture = m_headerPromise.get_future();
}

HTTPContentFetcherInterface::State LibCurlHttpContentFetcher::getState() {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    return m_state;
}

std::string LibCurlHttpContentFetcher::getUrl() const {
    return m_url;
}

HTTPContentFetcherInterface::Header LibCurlHttpContentFetcher::getHeader(std::atomic<bool>* shouldShutdown) {
    // Creates a copy of the future for the current thread
    auto localFuture = m_headerFuture;
    auto startTime = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::steady_clock::now() - startTime;
    while ((MAX_GET_HEADER_WAIT > elapsedTime) && !m_isShutdown && (!shouldShutdown || !(*shouldShutdown))) {
        if (State::ERROR == getState()) {
            ACSDK_ERROR(LX(__func__).sensitive("URL", m_url).d("reason", "Invalid state").d("state", "ERROR"));
            m_header.successful = false;
            return m_header;
        }
        std::future_status status = localFuture.wait_for(std::chrono::milliseconds(WAIT_FOR_ACTIVITY_TIMEOUT));
        if (std::future_status::ready == status) {
            break;
        }
        elapsedTime = std::chrono::steady_clock::now() - startTime;
    }
    if (MAX_GET_HEADER_WAIT <= elapsedTime) {
        ACSDK_ERROR(LX("getHeaderFailed").d("reason", "waitTimeout"));
        m_header.successful = false;
    }
    return m_header;
}

bool LibCurlHttpContentFetcher::getBody(std::shared_ptr<avsCommon::avs::attachment::AttachmentWriter> writer) {
    std::lock_guard<std::mutex> lock(m_getBodyMutex);
    if (State::ERROR == getState()) {
        return false;
    }
    if (!waitingForBodyRequest()) {
        ACSDK_ERROR(LX(__func__).d("reason", "functionAlreadyCalled"));
        return false;
    }
    m_streamWriter = writer;
    stateTransition(State::FETCHING_BODY, true);
    return true;
}

void LibCurlHttpContentFetcher::shutdown() {
    ACSDK_DEBUG9(LX(__func__).m("Shutting down"));
    m_isShutdown = true;
    stateTransition(State::BODY_DONE, true);
}

std::unique_ptr<avsCommon::utils::HTTPContent> LibCurlHttpContentFetcher::getContent(
    FetchOptions fetchOption,
    std::unique_ptr<avsCommon::avs::attachment::AttachmentWriter> attachmentWriter,
    const std::vector<std::string>& customHeaders) {
    if (m_hasObjectBeenUsed.test_and_set()) {
        ACSDK_ERROR(LX("getContentFailed").d("reason", "Object has already been used"));
        stateTransition(State::ERROR, false);
        return nullptr;
    }
    if (!m_curlWrapper.setURL(m_url)) {
        ACSDK_ERROR(LX("getContentFailed").d("reason", "failedToSetUrl"));
        stateTransition(State::ERROR, false);
        return nullptr;
    }
    auto curlReturnValue = curl_easy_setopt(m_curlWrapper.getCurlHandle(), CURLOPT_FOLLOWLOCATION, 1L);
    if (curlReturnValue != CURLE_OK) {
        ACSDK_ERROR(LX("getContentFailed").d("reason", "enableFollowRedirectsFailed").d("error", curlReturnValue));
        stateTransition(State::ERROR, false);
        return nullptr;
    }
    curlReturnValue = curl_easy_setopt(m_curlWrapper.getCurlHandle(), CURLOPT_AUTOREFERER, 1L);
    if (curlReturnValue != CURLE_OK) {
        ACSDK_ERROR(LX("getContentFailed")
                        .d("reason", "enableAutoReferralSettingToRedirectsFailed")
                        .d("error", curlReturnValue));
        stateTransition(State::ERROR, false);
        return nullptr;
    }
    // This enables the libcurl cookie engine, allowing it to send cookies
    curlReturnValue = curl_easy_setopt(m_curlWrapper.getCurlHandle(), CURLOPT_COOKIEFILE, "");
    if (curlReturnValue != CURLE_OK) {
        ACSDK_ERROR(LX("getContentFailed").d("reason", "enableLibCurlCookieEngineFailed").d("error", curlReturnValue));
        stateTransition(State::ERROR, false);
        return nullptr;
    }
    if (!m_curlWrapper.setConnectionTimeout(TIMEOUT_CONNECTION)) {
        ACSDK_ERROR(LX("getContentFailed").d("reason", "setConnectionTimeoutFailed"));
        stateTransition(State::ERROR, false);
        return nullptr;
    }

    curl_slist* headerList = getCustomHeaderList(customHeaders);
    if (headerList && curl_easy_setopt(m_curlWrapper.getCurlHandle(), CURLOPT_HTTPHEADER, headerList) != CURLE_OK) {
        ACSDK_ERROR(LX("getContentFailed").d("reason", "setCustomHeadersFailed"));
        stateTransition(State::ERROR, false);
        return nullptr;
    }

    curlReturnValue = curl_easy_setopt(
        m_curlWrapper.getCurlHandle(),
        CURLOPT_USERAGENT,
        sdkInterfaces::HTTPContentFetcherInterface::getUserAgent().c_str());
    if (curlReturnValue != CURLE_OK) {
        ACSDK_ERROR(LX("getContentFailed").d("reason", "setCurlOptRangeFailed").d("error", curlReturnValue));
        stateTransition(State::ERROR, false);
        return nullptr;
    }

    std::shared_ptr<avsCommon::avs::attachment::InProcessAttachment> stream = nullptr;

    // This flag will remain false if the caller of getContent() passed in their own writer.
    bool writerWasCreatedLocally = false;

    switch (fetchOption) {
        case FetchOptions::CONTENT_TYPE:
            /*
             * Since this option only wants the content-type, I set a noop callback for parsing the body of the HTTP
             * response. For some webpages, it is required to set a body callback in order for the full webpage data
             * to render.
             */
            curlReturnValue = curl_easy_setopt(m_curlWrapper.getCurlHandle(), CURLOPT_WRITEFUNCTION, noopCallback);
            if (curlReturnValue != CURLE_OK) {
                ACSDK_ERROR(LX("getContentFailed").d("reason", "failedToSetCurlCallback"));
                return nullptr;
            }
            stateTransition(State::FETCHING_HEADER, true);
            m_thread = std::thread([this, headerList]() {
                auto curlMultiHandle = CurlMultiHandleWrapper::create();
                if (!curlMultiHandle) {
                    ACSDK_ERROR(LX("getContentFailed").d("reason", "curlMultiHandleWrapperCreateFailed"));
                    // Set the promises because of errors.
                    m_header.responseCode = HTTPResponseCode::HTTP_RESPONSE_CODE_UNDEFINED;
                    m_header.contentType = "";
                    return;
                }
                curlMultiHandle->addHandle(m_curlWrapper.getCurlHandle());

                int numTransfersLeft = 1;
                HTTPResponseCode finalResponseCode = HTTPResponseCode::HTTP_RESPONSE_CODE_UNDEFINED;
                char* contentType = nullptr;

                while (numTransfersLeft && !m_isShutdown) {
                    auto result = curlMultiHandle->perform(&numTransfersLeft);
                    if (CURLM_CALL_MULTI_PERFORM == result) {
                        continue;
                    } else if (CURLM_OK != result) {
                        ACSDK_ERROR(LX("getContentFailed").d("reason", "performFailed"));
                        break;
                    }

                    int finalResponseCodeId = 0;
                    auto curlReturnValue =
                        curl_easy_getinfo(m_curlWrapper.getCurlHandle(), CURLINFO_RESPONSE_CODE, &finalResponseCodeId);
                    finalResponseCode = intToHTTPResponseCode(finalResponseCode);
                    if (curlReturnValue != CURLE_OK) {
                        ACSDK_ERROR(LX("curlEasyGetInfoFailed").d("error", curl_easy_strerror(curlReturnValue)));
                        break;
                    }
                    if (HTTPResponseCode::HTTP_RESPONSE_CODE_UNDEFINED != finalResponseCode &&
                        !isRedirect(finalResponseCode)) {
                        ACSDK_DEBUG9(LX("getContent").d("responseCode", finalResponseCode).sensitive("url", m_url));
                        curlReturnValue =
                            curl_easy_getinfo(m_curlWrapper.getCurlHandle(), CURLINFO_CONTENT_TYPE, &contentType);
                        if ((curlReturnValue == CURLE_OK) && contentType) {
                            ACSDK_DEBUG9(LX("getContent").d("contentType", contentType).sensitive("url", m_url));
                        } else {
                            ACSDK_ERROR(LX("curlEasyGetInfoFailed").d("error", curl_easy_strerror(curlReturnValue)));
                            ACSDK_ERROR(
                                LX("getContent").d("contentType", "failedToGetContentType").sensitive("url", m_url));
                        }
                        break;
                    }

                    int numTransfersUpdated = 0;
                    result = curlMultiHandle->wait(WAIT_FOR_ACTIVITY_TIMEOUT, &numTransfersUpdated);
                    if (result != CURLM_OK) {
                        ACSDK_ERROR(LX("getContentFailed")
                                        .d("reason", "multiWaitFailed")
                                        .d("error", curl_multi_strerror(result)));
                        break;
                    }
                }

                // Free custom headers.
                curl_slist_free_all(headerList);

                // Abort any curl operation by removing the curl handle.
                curlMultiHandle->removeHandle(m_curlWrapper.getCurlHandle());
            });
            break;
        case FetchOptions::ENTIRE_BODY:

            stateTransition(State::FETCHING_HEADER, true);
            if (!m_curlWrapper.setWriteCallback(bodyCallback, this)) {
                ACSDK_ERROR(LX("getContentFailed").d("reason", "failedToSetCurlBodyCallback"));
                stateTransition(State::ERROR, false);
                return nullptr;
            }
            if (!m_curlWrapper.setHeaderCallback(headerCallback, this)) {
                ACSDK_ERROR(LX("getContentFailed").d("reason", "failedToSetCurlHeaderCallback"));
                stateTransition(State::ERROR, false);
                return nullptr;
            }

            m_thread = std::thread([this, writerWasCreatedLocally, headerList]() {
                ACSDK_DEBUG9(LX("transferThread").sensitive("URL", m_url).m("start"));
                auto curlMultiHandle = avsCommon::utils::libcurlUtils::CurlMultiHandleWrapper::create();
                if (!curlMultiHandle) {
                    ACSDK_ERROR(LX("getContentFailed").d("reason", "curlMultiHandleWrapperCreateFailed"));
                    // Set the promises because of errors.
                    m_header.responseCode = HTTPResponseCode::HTTP_RESPONSE_CODE_UNDEFINED;
                    m_header.contentType = "";
                    stateTransition(State::ERROR, false);
                    return;
                }
                curlMultiHandle->addHandle(m_curlWrapper.getCurlHandle());

                int numTransfersLeft = 1;
                while (numTransfersLeft && !m_isShutdown) {
                    ACSDK_DEBUG9(LX("transferThread").sensitive("URL", m_url).d("numTransfersLeft", numTransfersLeft));
                    auto result = curlMultiHandle->perform(&numTransfersLeft);
                    if (CURLM_CALL_MULTI_PERFORM == result) {
                        continue;
                    } else if (CURLM_OK != result) {
                        ACSDK_ERROR(LX("getContentFailed").sensitive("URL", m_url).d("reason", "performFailed"));
                        stateTransition(State::ERROR, false);
                        return;
                    }

                    int numTransfersUpdated = 0;
                    result = curlMultiHandle->wait(WAIT_FOR_ACTIVITY_TIMEOUT, &numTransfersUpdated);
                    if (result != CURLM_OK) {
                        ACSDK_ERROR(LX("getContentFailed")
                                        .d("reason", "multiWaitFailed")
                                        .d("error", curl_multi_strerror(result)));
                        stateTransition(State::ERROR, false);
                        return;
                    }

                    auto bytesRemaining = m_header.contentLength - m_currentContentReceivedLength;
                    if (bytesRemaining < 0) {
                        bytesRemaining = 0;
                    }

                    // There's still byte remaining to download, let's try to get the rest of the data by using range.
                    if ((numTransfersLeft == 0) && bytesRemaining > 0) {
                        // Remove the current curlHandle from the multiHandle first.
                        curlMultiHandle->removeHandle(m_curlWrapper.getCurlHandle());

                        // Reset the current content counters.
                        m_header.contentLength = 0;
                        m_currentContentReceivedLength = 0;

                        // Set the range to start with total content that's been received so far to the end.
                        std::ostringstream ss;
                        ss << (m_totalContentReceivedLength) << "-";
                        auto curlReturnValue =
                            curl_easy_setopt(m_curlWrapper.getCurlHandle(), CURLOPT_RANGE, ss.str().c_str());
                        if (curlReturnValue != CURLE_OK) {
                            ACSDK_ERROR(
                                LX("getContentFailed").d("reason", "setUserAgentFailed").d("error", curlReturnValue));
                            stateTransition(State::ERROR, false);
                            return;
                        }

                        // Add the curlHandle back to the mutliHandle.
                        curlMultiHandle->addHandle(m_curlWrapper.getCurlHandle());

                        // Set this to 1 so that we will try to perform() again.
                        numTransfersLeft = 1;

                        ACSDK_DEBUG9(LX(__func__)
                                         .d("bytesRemaining", bytesRemaining)
                                         .d("totalContentReceived", m_totalContentReceivedLength)
                                         .d("restartingWithRange", ss.str()));
                    }
                }

                /*
                 * If the writer was created locally, its job is done and can be safely closed.
                 */
                if (writerWasCreatedLocally) {
                    ACSDK_DEBUG9(LX(__func__).m("Closing the writer"));
                    m_streamWriter->close();
                }

                /*
                 * Note: If the writer was not created locally, its owner must ensure that it closes when
                 * necessary. In the case of a livestream, if the writer is not closed the
                 * LibCurlHttpContentFetcher will continue to download data indefinitely.
                 */
                m_done = true;

                // Free custom headers.
                curl_slist_free_all(headerList);

                // Abort any curl operation by removing the curl handle.
                curlMultiHandle->removeHandle(m_curlWrapper.getCurlHandle());

                if (State::INITIALIZED == getState() || State::ERROR == getState()) {
                    ACSDK_DEBUG9(LX("transferThread").sensitive("URL", m_url).m("end with error"));
                    stateTransition(State::ERROR, false);
                } else {
                    ACSDK_DEBUG9(LX("transferThread").sensitive("URL", m_url).m("end"));
                    stateTransition(State::BODY_DONE, true);
                }
            });
            break;
        default:
            stateTransition(State::ERROR, false);
            return nullptr;
    }
    return nullptr;
}

curl_slist* LibCurlHttpContentFetcher::getCustomHeaderList(std::vector<std::string> customHeaders) {
    struct curl_slist* headers = nullptr;
    for (const auto& header : customHeaders) {
        ACSDK_DEBUG9(LX("getCustomHeaderList").d("header", header.c_str()));
        headers = curl_slist_append(headers, header.c_str());
    }
    return headers;
}

LibCurlHttpContentFetcher::~LibCurlHttpContentFetcher() {
    ACSDK_DEBUG9(LX("~LibCurlHttpContentFetcher").sensitive("URL", m_url));
    if (m_thread.joinable()) {
        m_done = true;
        m_isShutdown = true;
        stateTransition(State::BODY_DONE, true);
        m_thread.join();
    }
}

void LibCurlHttpContentFetcher::reportInvalidStateTransitionAttempt(State currentState, State newState) {
    ACSDK_ERROR(LX(__func__)
                    .d("currentState", currentState)
                    .d("newState", newState)
                    .m("An attempt was made to perform an invalid state transition."));
}

void LibCurlHttpContentFetcher::stateTransition(State newState, bool value) {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    switch (m_state) {
        case State::INITIALIZED:
            switch (newState) {
                case State::INITIALIZED:
                    return;
                case State::FETCHING_HEADER:
                    break;
                case State::HEADER_DONE:
                    m_header.successful = value;
                    m_headerPromise.set_value(value);
                    break;
                case State::FETCHING_BODY:
                    m_header.successful = value;
                    m_headerPromise.set_value(value);
                    break;
                case State::BODY_DONE:
                    m_header.successful = value;
                    m_headerPromise.set_value(value);
                    break;
                case State::ERROR:
                    m_header.successful = false;
                    m_headerPromise.set_value(false);
                    break;
            }
            break;
        case State::FETCHING_HEADER:
            switch (newState) {
                case State::INITIALIZED:
                    reportInvalidStateTransitionAttempt(m_state, newState);
                    return;
                case State::FETCHING_HEADER:
                    return;
                case State::HEADER_DONE:
                    m_header.successful = value;
                    m_headerPromise.set_value(value);
                    break;
                case State::FETCHING_BODY:
                    m_header.successful = value;
                    m_headerPromise.set_value(value);
                    break;
                case State::BODY_DONE:
                    m_header.successful = value;
                    m_headerPromise.set_value(value);
                    break;
                case State::ERROR:
                    m_header.successful = false;
                    m_headerPromise.set_value(false);
                    break;
            }
            break;
        case State::HEADER_DONE:
            switch (newState) {
                case State::INITIALIZED:
                    reportInvalidStateTransitionAttempt(m_state, newState);
                    return;
                case State::FETCHING_HEADER:
                    reportInvalidStateTransitionAttempt(m_state, newState);
                    return;
                case State::HEADER_DONE:
                    return;
                case State::FETCHING_BODY:
                    break;
                case State::BODY_DONE:
                    break;
                case State::ERROR:
                    break;
            }
            break;
        case State::FETCHING_BODY:
            switch (newState) {
                case State::INITIALIZED:
                    reportInvalidStateTransitionAttempt(m_state, newState);
                    return;
                case State::FETCHING_HEADER:
                    reportInvalidStateTransitionAttempt(m_state, newState);
                    return;
                case State::HEADER_DONE:
                    reportInvalidStateTransitionAttempt(m_state, newState);
                    return;
                case State::FETCHING_BODY:
                    return;
                case State::BODY_DONE:
                    break;
                case State::ERROR:
                    break;
            }
            break;
        case State::BODY_DONE:
            switch (newState) {
                case State::INITIALIZED:
                    reportInvalidStateTransitionAttempt(m_state, newState);
                    return;
                case State::FETCHING_HEADER:
                    reportInvalidStateTransitionAttempt(m_state, newState);
                    return;
                case State::HEADER_DONE:
                    reportInvalidStateTransitionAttempt(m_state, newState);
                    return;
                case State::FETCHING_BODY:
                    reportInvalidStateTransitionAttempt(m_state, newState);
                    return;
                case State::BODY_DONE:
                    return;
                case State::ERROR:
                    break;
            }
            break;
        case State::ERROR:
            return;
    }
    if (State::ERROR == newState) {
        ACSDK_ERROR(LX(__func__).sensitive("URL", m_url).d("oldState", m_state).m("State transition to ERROR"));
    } else {
        ACSDK_DEBUG9(
            LX(__func__).sensitive("URL", m_url).d("oldState", m_state).d("newState", newState).m("State transition"));
    }
    m_state = newState;
}

bool LibCurlHttpContentFetcher::waitingForBodyRequest() {
    State state = getState();
    return (State::INITIALIZED == state) || (State::FETCHING_HEADER == state) || (State::HEADER_DONE == state);
}

}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
