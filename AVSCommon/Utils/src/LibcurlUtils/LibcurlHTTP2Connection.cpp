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

#include <curl/multi.h>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/LibcurlUtils/CurlEasyHandleWrapper.h>

#include "AVSCommon/Utils/LibcurlUtils/LibcurlHTTP2Connection.h"
#include "AVSCommon/Utils/LibcurlUtils/LibcurlHTTP2Request.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace libcurlUtils {

using namespace avsCommon::utils::http2;
using namespace avsCommon::utils::libcurlUtils;

/// String to identify log entries originating from this file.
static const std::string TAG("LibcurlHTTP2Connection");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Timeout for curl_multi_wait
const static std::chrono::milliseconds WAIT_FOR_ACTIVITY_TIMEOUT(100);
/// Timeout for curl_multi_wait while all non-intermittent HTTP/2 streams are paused.
const static std::chrono::milliseconds WAIT_FOR_ACTIVITY_WHILE_STREAMS_PAUSED_TIMEOUT(10);

#ifdef ACSDK_OPENSSL_MIN_VER_REQUIRED
/**
 * This function checks the minimum version of OpenSSL required and prints a warning if the version is too old or
 * the version string parsing failed.
 */
static void verifyOpenSslVersion(curl_version_info_data* data) {
    // There are three numbers in OpenSSL version: major.minor.patch
    static const int NUM_OF_VERSION_NUMBER = 3;

    unsigned int versionUsed[NUM_OF_VERSION_NUMBER];
    unsigned int minVersionRequired[NUM_OF_VERSION_NUMBER];

    if (!data) {
        ACSDK_ERROR(LX("verifyOpenSslVersionFailed").d("reason", "nullData"));
        return;
    } else if (!data->ssl_version) {
        ACSDK_ERROR(LX("verifyOpenSslVersionFailed").d("reason", "nullSslVersion"));
        return;
    }

    // parse ssl_version
    int matchedVersionUsed =
        sscanf(data->ssl_version, "OpenSSL/%u.%u.%u", &versionUsed[0], &versionUsed[1], &versionUsed[2]);

    // parse minimum OpenSSL version required
    int matchedVersionRequired = sscanf(
        ACSDK_STRINGIFY(ACSDK_OPENSSL_MIN_VER_REQUIRED),
        "%u.%u.%u",
        &minVersionRequired[0],
        &minVersionRequired[1],
        &minVersionRequired[2]);

    if (matchedVersionUsed == matchedVersionRequired && matchedVersionUsed == NUM_OF_VERSION_NUMBER) {
        bool versionRequirementFailed = false;

        for (int i = 0; i < NUM_OF_VERSION_NUMBER; i++) {
            if (versionUsed[i] < minVersionRequired[i]) {
                versionRequirementFailed = true;
                break;
            } else if (versionUsed[i] > minVersionRequired[i]) {
                break;
            }
        }
        if (versionRequirementFailed) {
            ACSDK_WARN(LX("OpenSSL minimum version requirement failed!")
                           .d("version", data->ssl_version)
                           .d("required", ACSDK_STRINGIFY(ACSDK_OPENSSL_MIN_VER_REQUIRED)));
        }
    } else {
        ACSDK_WARN(LX("Unable to parse OpenSSL version!")
                       .d("version", data->ssl_version)
                       .d("required", ACSDK_STRINGIFY(ACSDK_OPENSSL_MIN_VER_REQUIRED)));
    }
}
#endif

/**
 * This function logs a warning if the version of curl is not recent enough for use with the ACL.
 * @return true if check passes, false if it fails
 */
static bool performCurlChecks() {
    curl_version_info_data* data = curl_version_info(CURLVERSION_NOW);
    if (data && !(data->features & CURL_VERSION_HTTP2)) {
        ACSDK_CRITICAL(LX("libcurl not built with HTTP/2 support!"));
        return false;
    }
#ifdef ACSDK_OPENSSL_MIN_VER_REQUIRED
    verifyOpenSslVersion(data);
#endif
    return true;
}

LibcurlHTTP2Connection::LibcurlHTTP2Connection() : m_isStopping{false} {
    m_networkThread = std::thread(&LibcurlHTTP2Connection::networkLoop, this);
}

bool LibcurlHTTP2Connection::createMultiHandle() {
    m_multi = CurlMultiHandleWrapper::create();
    if (!m_multi) {
        ACSDK_ERROR(LX("initFailed").d("reason", "curlMultiHandleWrapperCreateFailed"));
        return false;
    }
    if (curl_multi_setopt(m_multi->getCurlHandle(), CURLMOPT_PIPELINING, 2L) != CURLM_OK) {
        m_multi.reset();
        ACSDK_ERROR(LX("initFailed").d("reason", "enableHTTP2PipeliningFailed"));
        return false;
    }

    return true;
}

std::shared_ptr<LibcurlHTTP2Connection> LibcurlHTTP2Connection::create() {
    if (!performCurlChecks()) {
        return nullptr;
    }
    return std::shared_ptr<LibcurlHTTP2Connection>(new LibcurlHTTP2Connection());
}

LibcurlHTTP2Connection::~LibcurlHTTP2Connection() {
    ACSDK_DEBUG5(LX(__func__));
    disconnect();
}

bool LibcurlHTTP2Connection::isStopping() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_isStopping;
}

void LibcurlHTTP2Connection::setIsStopping() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isStopping = true;
    m_cv.notify_one();
}

std::shared_ptr<LibcurlHTTP2Request> LibcurlHTTP2Connection::dequeueRequest() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isStopping || m_requestQueue.empty()) {
        return nullptr;
    }
    auto result = m_requestQueue.front();
    m_requestQueue.pop_front();
    return result;
}

void LibcurlHTTP2Connection::processNextRequest() {
    auto stream = dequeueRequest();
    if (!stream) {
        return;
    }
    stream->setTimeOfLastTransfer();
    auto result = m_multi->addHandle(stream->getCurlHandle());
    if (CURLM_OK == result) {
        auto handle = stream->getCurlHandle();
        ACSDK_DEBUG9(LX("insertActiveStream").d("handle", handle).d("streamId", stream->getId()));
        m_activeStreams[handle] = stream;
    } else {
        ACSDK_ERROR(LX("processNextRequest").d("reason", "addHandleFailed").d("error", curl_multi_strerror(result)));
        stream->reportCompletion(HTTP2ResponseFinishedStatus::INTERNAL_ERROR);
    }
}

void LibcurlHTTP2Connection::networkLoop() {
    ACSDK_DEBUG5(LX(__func__));

    while (!isStopping()) {
        if (!createMultiHandle()) {
            setIsStopping();
            break;
        }
        {
            // wait until we have at least one request
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] { return m_isStopping || !m_requestQueue.empty(); });
            if (m_isStopping) {
                break;
            }
        }

        processNextRequest();

        int numTransfersLeft = 1;  // just dequeued the first request.
        // Call perform repeatedly to transfer data on active streams.
        while (numTransfersLeft > 0 && !isStopping()) {
            auto result = m_multi->perform(&numTransfersLeft);
            if (result == CURLM_CALL_MULTI_PERFORM) {
                continue;
            }
            if (result != CURLM_OK) {
                ACSDK_ERROR(LX("networkLoopStopping").d("reason", "performFailed"));
                setIsStopping();
                break;
            }

            cleanupFinishedStreams();
            cleanupCancelledAndStalledStreams();

            if (isStopping()) {
                break;
            }

            processNextRequest();

            auto before = std::chrono::time_point<std::chrono::steady_clock>::max();
            auto multiWaitTimeout = WAIT_FOR_ACTIVITY_TIMEOUT;
            bool paused = areStreamsPaused();
            if (paused) {
                multiWaitTimeout = WAIT_FOR_ACTIVITY_WHILE_STREAMS_PAUSED_TIMEOUT;
                before = std::chrono::steady_clock::now();
            }

            int numTransfersUpdated = 0;
            result = m_multi->wait(multiWaitTimeout, &numTransfersUpdated);
            if (result != CURLM_OK) {
                ACSDK_ERROR(
                    LX("networkLoopStopping").d("reason", "multiWaitFailed").d("error", curl_multi_strerror(result)));
                setIsStopping();
                break;
            }

            // @note curl_multi_wait will return immediately even if all streams are paused, because HTTP/2 streams
            // are full-duplex - so activity may have occurred on the other side. Therefore, if our intent is to pause
            // transfers to give the readers / writers time to catch up, we must perform a local sleep of our own.
            if (paused) {
                auto after = std::chrono::steady_clock::now();
                auto elapsed = after - before;
                auto remaining = multiWaitTimeout - elapsed;

                // sanity check that remainingMs is valid before performing a sleep.
                if (remaining.count() > 0 && remaining <= WAIT_FOR_ACTIVITY_WHILE_STREAMS_PAUSED_TIMEOUT) {
                    std::this_thread::sleep_for(remaining);
                }
            }
            unPauseActiveStreams();
        }
        cancelAllStreams();
        m_multi.reset();
    }

    ACSDK_DEBUG5(LX("networkLoopExiting"));
}

std::shared_ptr<HTTP2RequestInterface> LibcurlHTTP2Connection::createAndSendRequest(const HTTP2RequestConfig& config) {
    auto req = std::make_shared<LibcurlHTTP2Request>(config, config.getId());
    addStream(req);
    return req;
}

void LibcurlHTTP2Connection::disconnect() {
    ACSDK_DEBUG5(LX(__func__));
    setIsStopping();
    if (m_networkThread.joinable()) {
        m_networkThread.join();
    }
}

bool LibcurlHTTP2Connection::addStream(std::shared_ptr<LibcurlHTTP2Request> stream) {
    if (!stream) {
        ACSDK_ERROR(LX("addStream").d("failed", "null stream"));
        return false;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isStopping) {
        ACSDK_ERROR(LX("addStream").d("failed", "network loop stopping"));
        return false;
    }
    m_requestQueue.push_back(std::move(stream));
    m_cv.notify_one();
    return true;
}

void LibcurlHTTP2Connection::cleanupFinishedStreams() {
    CURLMsg* message = nullptr;
    do {
        int messagesLeft = 0;
        message = m_multi->infoRead(&messagesLeft);
        if (message && message->msg == CURLMSG_DONE) {
            auto it = m_activeStreams.find(message->easy_handle);
            if (it != m_activeStreams.end()) {
                it->second->reportResponseCode();

                if (CURLE_OPERATION_TIMEDOUT == message->data.result) {
                    it->second->reportCompletion(HTTP2ResponseFinishedStatus::TIMEOUT);
                } else {
                    it->second->reportCompletion(HTTP2ResponseFinishedStatus::COMPLETE);
                }
                ACSDK_DEBUG(LX("streamFinished")
                                .d("streamId", it->second->getId())
                                .d("result", curl_easy_strerror(message->data.result))
                                .d("CURLcode", message->data.result));
                releaseStream(*(it->second));
            } else {
                ACSDK_ERROR(
                    LX("cleanupFinishedStreamError").d("reason", "streamNotFound").d("handle", message->easy_handle));
            }
        }
    } while (message);
}

void LibcurlHTTP2Connection::cleanupCancelledAndStalledStreams() {
    auto it = m_activeStreams.begin();
    while (it != m_activeStreams.end()) {
        auto stream = (it++)->second;
        if (stream->isCancelled()) {
            cancelStream(*stream);
        } else if (stream->hasProgressTimedOut()) {
            ACSDK_WARN(LX("streamProgressTimedOut").d("streamId", stream->getId()));
            stream->reportCompletion(HTTP2ResponseFinishedStatus::TIMEOUT);
            releaseStream(*stream);
        }
    }
}

bool LibcurlHTTP2Connection::areStreamsPaused() {
    size_t numberNonIntermittentStreams = 0;
    size_t numberPausedStreams = 0;
    for (const auto& entry : m_activeStreams) {
        const auto& stream = entry.second;
        if (!stream->isIntermittentTransferExpected()) {
            numberNonIntermittentStreams++;
            if (entry.second->isPaused()) {
                numberPausedStreams++;
            }
        }
    }
    return numberPausedStreams > 0 && (numberPausedStreams == numberNonIntermittentStreams);
}

void LibcurlHTTP2Connection::unPauseActiveStreams() {
    for (auto& stream : m_activeStreams) {
        stream.second->unPause();
    }
}

bool LibcurlHTTP2Connection::cancelStream(LibcurlHTTP2Request& stream) {
    ACSDK_INFO(LX("cancelStream").d("streamId", stream.getId()));
    stream.reportCompletion(HTTP2ResponseFinishedStatus::CANCELLED);
    return releaseStream(stream);
}

void LibcurlHTTP2Connection::cancelAllStreams() {
    auto it = m_activeStreams.begin();
    while (it != m_activeStreams.end()) {
        cancelStream(*(it++)->second);
    }
}

bool LibcurlHTTP2Connection::releaseStream(LibcurlHTTP2Request& stream) {
    auto handle = stream.getCurlHandle();
    ACSDK_DEBUG9(LX("releaseStream").d("streamId", stream.getId()));
    auto result = m_multi->removeHandle(handle);
    m_activeStreams.erase(handle);
    if (result != CURLM_OK) {
        ACSDK_ERROR(LX("releaseStreamFailed").d("reason", "removeHandleFailed").d("streamId", stream.getId()));
        return false;
    }
    return true;
}

}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
