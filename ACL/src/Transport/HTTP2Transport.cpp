/*
 * HTTP2Transport.cpp
 *
 * Copyright 2016-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "ACL/Transport/HTTP2Transport.h"

#include <chrono>
#include <functional>
#include <random>
#include <stdio.h>

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace acl {

using namespace alexaClientSDK::avsCommon::utils;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;

/// String to identify log entries originating from this file.
static const std::string TAG("HTTP2Transport");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * The maximum number of streams we can have active at once.  Please see here for more information:
 * https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/docs/managing-an-http-2-connection
 */
const static int MAX_STREAMS = 10;
/// Downchannel URL
const static std::string AVS_DOWNCHANNEL_URL_PATH_EXTENSION = "/v20160207/directives";
/// URL to send events to
const static std::string AVS_EVENT_URL_PATH_EXTENSION = "/v20160207/events";
/// URL to send pings to
const static std::string AVS_PING_URL_PATH_EXTENSION = "/ping";
/// Timeout for curl_multi_wait
const static int WAIT_FOR_ACTIVITY_TIMEOUT_MS = 100;
/// Timeout for curl_multi_wait when there's a paused HTTP/2 stream.
const static int WAIT_FOR_ACTIVITY_WHILE_PAUSED_STREAM_TIMEOUT_MS = 10;
/// 1 minute in milliseconds
const static int MS_PER_MIN = 60000;
/// Timeout before we send a ping
const static int PING_TIMEOUT_MS = MS_PER_MIN * 5;
/// Number of times we timeout waiting for activity before sending a ping
const static int NUM_TIMEOUTS_BEFORE_PING = PING_TIMEOUT_MS / WAIT_FOR_ACTIVITY_TIMEOUT_MS;
/// The maximum time a ping should take in seconds
const static long PING_RESPONSE_TIMEOUT_SEC = 30;
/// Connection timeout
static const std::chrono::seconds ESTABLISH_CONNECTION_TIMEOUT = std::chrono::seconds{60};
/// Timeout for transmission of data on a given stream
static const std::chrono::seconds STREAM_PROGRESS_TIMEOUT = std::chrono::seconds{30};

/**
 * Calculates the time, in milliseconds, to wait before attempting to reconnect.
 *
 * @param retryCount The number of times we've retried already
 * @returns The amount of time to wait, in milliseconds
 */
static std::chrono::milliseconds calculateTimeToRetry(int retryCount) {
    static const double RETRY_RANDOMIZATION_FACTOR = 0.5;
    static const double RETRY_DECREASE_FACTOR = 1 / (RETRY_RANDOMIZATION_FACTOR + 1);
    static const double RETRY_INCREASE_FACTOR = (RETRY_RANDOMIZATION_FACTOR + 1);
    /**
     * We use this schedule to ensure that we don't continuously attempt to retry
     * a connection to AVS (which would cause a DOS).
     * Randomization further prevents multiple devices from attempting connections
     * at the same time (which would also cause a DOS at each step).
     */
    static const int RETRY_TABLE[] = {
        250,   // Retry 1:  0.25s, range with 0.5 randomization: [ 0.167,  0.375]
        1000,  // Retry 2:  1.00s, range with 0.5 randomization: [ 0.667,  1.500]
        3000,  // Retry 3:  3.00s, range with 0.5 randomization: [ 2.000,  4.500]
        5000,  // Retry 4:  5.00s, range with 0.5 randomization: [ 3.333,  7.500]
        10000, // Retry 5: 10.00s, range with 0.5 randomization: [ 6.667, 15.000]
        20000, // Retry 6: 20.00s, range with 0.5 randomization: [13.333, 30.000]
        30000, // Retry 7: 30.00s, range with 0.5 randomization: [20.000, 45.000]
        60000, // Retry 8: 60.00s, range with 0.5 randomization: [40.000, 90.000]
    };

    static const int RETRY_TABLE_SIZE = (sizeof(RETRY_TABLE) / sizeof(RETRY_TABLE[0]));
    if (retryCount < 0) {
        retryCount = 0;
    } else if (retryCount >= RETRY_TABLE_SIZE) {
        retryCount = RETRY_TABLE_SIZE - 1;
    }

    std::mt19937 generator(static_cast<unsigned>(std::time(nullptr)));
    std::uniform_int_distribution<int> distribution(static_cast<int>(RETRY_TABLE[retryCount] * RETRY_DECREASE_FACTOR),
                                            static_cast<int>(RETRY_TABLE[retryCount] * RETRY_INCREASE_FACTOR));
    auto delayMs = std::chrono::milliseconds(distribution(generator));
    return delayMs;
}

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
        sscanf(data->ssl_version,
            "OpenSSL/%u.%u.%u",
            &versionUsed[0], &versionUsed[1], &versionUsed[2]);

    // parse minimum OpenSSL version required
    int matchedVersionRequired =
        sscanf(ACSDK_STRINGIFY(ACSDK_OPENSSL_MIN_VER_REQUIRED),
            "%u.%u.%u",
            &minVersionRequired[0], &minVersionRequired[1], &minVersionRequired[2]);

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
 */
static void printCurlDiagnostics() {
    curl_version_info_data* data = curl_version_info(CURLVERSION_NOW);
    if (data && !(data->features & CURL_VERSION_HTTP2)) {
        ACSDK_CRITICAL(LX("libcurl not built with HTTP/2 support!"));
    }

#ifdef ACSDK_OPENSSL_MIN_VER_REQUIRED
    verifyOpenSslVersion(data);
#endif
}

HTTP2Transport::HTTP2Transport(
        std::shared_ptr<AuthDelegateInterface> authDelegate,
        const std::string& avsEndpoint,
        MessageConsumerInterface* messageConsumerInterface,
        std::shared_ptr<AttachmentManager> attachmentManager,
        TransportObserverInterface* observer)
        :
        m_observer{observer},
        m_messageConsumer{messageConsumerInterface},
        m_authDelegate{authDelegate},
        m_avsEndpoint{avsEndpoint},
        m_streamPool{MAX_STREAMS, attachmentManager},
        m_disconnectReason{ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR},
        m_isNetworkThreadRunning{false},
        m_isConnected{false},
        m_isStopping{false} {
    printCurlDiagnostics();
}

HTTP2Transport::~HTTP2Transport() {
    disconnect();
}

bool HTTP2Transport::connect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    // This function spawns a worker thread, so let's ensure this function may only be called when
    // the worker thread is not running.
    if (m_isNetworkThreadRunning) {
        ACSDK_ERROR(LX("connectFailed").d("reason", "networkThreadAlreadyRunning"));
        return false;
    }

    m_multi.reset(new MultiHandle());
    m_multi->handle = curl_multi_init();
    if (!m_multi->handle) {
        m_multi.reset();
        ACSDK_ERROR(LX("connectFailed").d("reason", "createCurlMultiHandleFailed"));
        return false;
    }
    if (curl_multi_setopt(m_multi->handle, CURLMOPT_PIPELINING, 2L) != CURLM_OK) {
        m_multi.reset();
        ACSDK_ERROR(LX("connectFailed").d("reason", "enableHTTP2PipeliningFailed"));
        return false;
    }

    ConnectionStatusObserverInterface::ChangedReason reason =
            ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR;
    if (!setupDownchannelStream(&reason)) {
        m_multi.reset();
        ACSDK_ERROR(LX("connectFailed").d("reason", "setupDownchannelStreamFailed").d("error", reason));
        return false;
    }

    m_isNetworkThreadRunning = true;
    m_isStopping = false;
    m_networkThread = std::thread(&HTTP2Transport::networkLoop, this);
    return true;
}

void HTTP2Transport::disconnect() {
    std::thread localNetworkThread;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        setIsStoppingLocked(ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);
        std::swap(m_networkThread, localNetworkThread);
    }
    if (localNetworkThread.joinable()){
        localNetworkThread.join();
    }
}

bool HTTP2Transport::isConnected() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return isConnectedLocked();
}

void HTTP2Transport::send(std::shared_ptr<MessageRequest> request) {
    if (!request) {
        ACSDK_ERROR(LX("sendFailed").d("reason", "nullRequest"));
    } else if (!enqueueRequest(request)) {
        request->onSendCompleted(MessageRequest::Status::NOT_CONNECTED);
    }
}

bool HTTP2Transport::setupDownchannelStream(ConnectionStatusObserverInterface::ChangedReason* reason) {
    if (!reason) {
        ACSDK_CRITICAL(LX("setupDownchannelStreamFailed").d("reason", "nullReason"));
        return false;
    }
    if (m_downchannelStream) {
        CURLMcode ret = curl_multi_remove_handle(m_multi->handle, m_downchannelStream->getCurlHandle());
        if (ret != CURLM_OK) {
            ACSDK_ERROR(LX("setupDownchannelStreamFailed")
                    .d("reason", "curlFailure")
                    .d("method", "curl_multi_remove_handle")
                    .d("error", curl_multi_strerror(ret)));
            *reason = ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR;
            return false;
        }
        m_streamPool.releaseStream(m_downchannelStream);
        m_downchannelStream.reset();
    }

    std::string authToken = m_authDelegate->getAuthToken();
    if (authToken.empty()) {
        ACSDK_ERROR(LX("setupDownchannelStreamFailed").d("reason", "getAuthTokenFailed"));
        *reason = ConnectionStatusObserverInterface::ChangedReason::INVALID_AUTH;
        return false;
    }

    std::string url = m_avsEndpoint + AVS_DOWNCHANNEL_URL_PATH_EXTENSION;
    m_downchannelStream = m_streamPool.createGetStream(url, authToken, m_messageConsumer);
    if (!m_downchannelStream) {
        ACSDK_ERROR(LX("setupDownchannelStreamFailed").d("reason", "createGetStreamFailed"));
        *reason = ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR;
        return false;
    }
    // Since the downchannel is the first stream to be established, make sure it times out if
    // a connection can't be established.
    if (!m_downchannelStream->setConnectionTimeout(ESTABLISH_CONNECTION_TIMEOUT)) {
        m_streamPool.releaseStream(m_downchannelStream);
        m_downchannelStream.reset();
        ACSDK_ERROR(LX("setupDownchannelStreamFailed").d("reason", "setConnectionTimeoutFailed"));
        *reason = ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR;
        return false;
    }

    auto result = curl_multi_add_handle(m_multi->handle, m_downchannelStream->getCurlHandle());
    if (result != CURLM_OK) {
        m_streamPool.releaseStream(m_downchannelStream);
        m_downchannelStream.reset();
        ACSDK_ERROR(LX("setupDownchannelStreamFailed")
                .d("reason", "curlFailure")
                .d("method", "curl_multi_add_handle")
                .d("error", curl_multi_strerror(result)));
        *reason = ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR;
        return false;
    }

    return true;
}

void HTTP2Transport::networkLoop() {

    int retryCount = 0;
    while (!establishConnection() && !isStopping()) {
        retryCount++;
        ACSDK_ERROR(LX("networkLoopRetryingToConnect")
                .d("reason", "establishConnectionFailed")
                .d("retryCount", retryCount));
        auto retryBackoff = calculateTimeToRetry(retryCount);
        std::unique_lock<std::mutex> lock(m_mutex);
        m_wakeRetryTrigger.wait_for(lock, retryBackoff, [this]{ return m_isStopping; });
    }

    setIsConnectedTrueUnlessStopping();

    /*
     * Call curl_multi_perform repeatedly to receive data on active streams. If all the currently
     * active streams have HTTP2 response codes service the next outgoing message (if any).
     * While the connection is alive we should have at least 1 transfer active (the downchannel).
     */
    int numTransfersLeft = 1;
    int timeouts = 0;
    while (numTransfersLeft && !isStopping()) {

        CURLMcode ret = curl_multi_perform(m_multi->handle, &numTransfersLeft);
        if (CURLM_CALL_MULTI_PERFORM == ret) {
            continue;
        } else if (ret != CURLM_OK) {
            ACSDK_ERROR(LX("networkLoopStopping")
                    .d("reason", "curlFailure")
                    .d("method", "curl_multi_perform")
                    .d("error", curl_multi_strerror(ret)));
            setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
            break;
        }

        cleanupFinishedStreams();
        cleanupStalledStreams();
        if (isStopping()) {
            break;
        }

        if (canProcessOutgoingMessage()) {
            processNextOutgoingMessage();
        }

        int multiWaitTimeoutMs = WAIT_FOR_ACTIVITY_TIMEOUT_MS;

        size_t numberPausedStreams = 0;
        for (auto stream : m_activeStreams) {
            if (stream.second->isPaused()) {
                numberPausedStreams++;
                multiWaitTimeoutMs = WAIT_FOR_ACTIVITY_WHILE_PAUSED_STREAM_TIMEOUT_MS;
            }
        }

        auto msBefore = std::chrono::milliseconds::max();
        if (numberPausedStreams > 0) {
            msBefore = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch());
        }

        //TODO: ACSDK-69 replace timeout with signal fd
        //TODO: ACSDK-281 - investigate the timeout values and performance consequences for curl_multi_wait.
        int numTransfersUpdated = 0;
        ret = curl_multi_wait(m_multi->handle, NULL, 0, multiWaitTimeoutMs, &numTransfersUpdated);
        if (ret != CURLM_OK) {
            ACSDK_ERROR(LX("networkLoopStopping")
                    .d("reason", "curlFailure")
                    .d("method", "curl_multi_wait")
                    .d("error", curl_multi_strerror(ret)));
            setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
            break;
        }

        // @note - curl_multi_wait will return immediately even if all streams are paused, because HTTP/2 streams
        // are full-duplex - so activity may have occurred on the other side. Therefore, if our intent is
        // to pause ACL to give attachment readers time to catch up with written data, we must perform a local
        // sleep of our own.
        if ((numberPausedStreams > 0) && (m_activeStreams.size() == numberPausedStreams)) {
            std::chrono::milliseconds msAfter = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch());
            auto elapsedMs = (msAfter - msBefore).count();
            auto remainingMs = multiWaitTimeoutMs - elapsedMs;

            // sanity check that remainingMs is valid before performing a sleep.
            if (remainingMs > 0 && remainingMs < WAIT_FOR_ACTIVITY_WHILE_PAUSED_STREAM_TIMEOUT_MS) {
                std::this_thread::sleep_for(std::chrono::milliseconds(remainingMs));
            }

            // un-pause the streams so that in the next invocation of curl_multi_perform progress may be made.
            for (auto stream : m_activeStreams) {
                if (stream.second->isPaused()) {
                    stream.second->setPaused(false);
                }
            }
        }

        //TODO: ACSDK-69 replace this logic with a signal fd
        /**
         * If no transfers were updated then curl_multi_wait would have waited WAIT_FOR_ACTIVITY_TIMEOUT_MS milliseconds.
         * Increment a counter everytime this happens, when the counter reaches:
         * MINUTES_TO_MILLISECONDS * 5 / WAIT_FOR_ACTIVITY_TIMEOUT_MS
         * we have waited 5 minutes with an idle connection. In this case send a ping.
         * We clear the counter once there is an activity on any transfer.
         */
        if (0 == numTransfersUpdated) {
            timeouts++;
            if (timeouts >= NUM_TIMEOUTS_BEFORE_PING) {
                if (!sendPing()) {
                    ACSDK_ERROR(LX("networkLoopStopping").d("reason", "sendPingFailed"));
                    setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
                    break;
                }
                timeouts = 0;
            }
        } else {
            timeouts = 0;
        }
    }

    // Catch-all. Reaching this point implies stopping.
    setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);

    // Remove active event handles from multi handle and release them back into the event pool.
    auto it = m_activeStreams.begin();
    while (it != m_activeStreams.end()) {
        (*it).second->notifyRequestObserver(MessageRequest::Status::NOT_CONNECTED);
        CURLMcode ret = curl_multi_remove_handle(m_multi->handle, (*it).first);
        if (ret != CURLM_OK) {
            ACSDK_ERROR(LX("networkLoopCleanupFailed")
                    .d("reason", "curlFailure")
                    .d("method", "curl_multi_remove_handle")
                    .d("error", curl_multi_strerror(ret)));
            (*it).second.reset(); // Force stream to be deleted, also don't put back in the pool
            it = m_activeStreams.erase(it);
            continue;
        }
        m_streamPool.releaseStream((*it).second);
        it = m_activeStreams.erase(it);
    }
    CURLMcode ret = curl_multi_remove_handle(m_multi->handle, m_downchannelStream->getCurlHandle());
    if (ret != CURLM_OK) {
        ACSDK_ERROR(LX("networkLoopCleanupFailed")
                    .d("reason", "curlFailure")
                    .d("method", "curl_multi_remove_handle")
                    .d("error", curl_multi_strerror(ret)));
        //Don't do anything here since we should to cleanup the downchannel stream anyway
    }
    m_streamPool.releaseStream(m_downchannelStream);
    m_downchannelStream.reset();
    m_multi.reset();
    clearQueuedRequests();
    setIsConnectedFalse();
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_isNetworkThreadRunning = false;
    }
}

bool HTTP2Transport::establishConnection() {
    // Set numTransferLeft to 1 because the downchannel stream has been added already.
    int numTransfersLeft = 1;

    /*
     * Calls curl_multi_perform until downchannel stream receives an HTTP2 response code. If the downchannel stream
     * ends before receiving a response code (numTransfersLeft == 0), then there was an error and we must try again.
     * If we're told to shutdown the network loop (isStopping()) then return false since no connection was established.
     */
    while(numTransfersLeft && !isStopping()) {
        CURLMcode ret = curl_multi_perform(m_multi->handle, &numTransfersLeft);
        // curl asked us to call multiperform again immediately
        if (CURLM_CALL_MULTI_PERFORM == ret) {
            continue;
        } else if (ret != CURLM_OK) {
            ACSDK_ERROR(LX("establishConnectionFailed")
                    .d("reason", "curlFailure")
                    .d("method", "curl_multi_perform")
                    .d("error", curl_multi_strerror(ret)));
            setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
        }
        long downchannelResponseCode = m_downchannelStream->getResponseCode();
        /* if the downchannel response code is > 0 then we have some response from the backend
         * if its < 0 there was a problem getting the response code from the easy handle
         * if its == 0 then keep looping since we have not yet received a response
         */
        if (downchannelResponseCode > 0) {
            /*
             * Only break the loop if we are successful. If we aren't keep looping so that we download
             * the full error message (for logging purposes) and then return false when we're done
             */
            if (HTTP2Stream::HTTPResponseCodes::SUCCESS_OK == downchannelResponseCode) {
                return true;
            }
        } else if (downchannelResponseCode < 0) {
            ACSDK_ERROR(LX("establishConnectionFailed")
                    .d("reason", "negativeResponseCode")
                    .d("responseCode", downchannelResponseCode));
            setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
        }
        // wait for activity on the downchannel stream, kinda like poll()
        int numTransfersUpdated = 0;
        ret = curl_multi_wait(m_multi->handle, NULL, 0 , WAIT_FOR_ACTIVITY_TIMEOUT_MS, &numTransfersUpdated);
        if (ret != CURLM_OK) {
            ACSDK_ERROR(LX("establishConnectionFailed")
                    .d("reason", "curlFailure")
                    .d("method", "curl_multi_wait")
                    .d("error", curl_multi_strerror(ret)));
            setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
        }
    }

    ConnectionStatusObserverInterface::ChangedReason reason =
            ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR;
    if (!setupDownchannelStream(&reason)) {
        ACSDK_ERROR(LX("establishConnectionFailed").d("reason", "setupDownchannelStreamFailed").d("error", reason));
        setIsStopping(reason);
    }
    return false;
}

void HTTP2Transport::cleanupFinishedStreams() {
    CURLMsg *message = nullptr;
    do {
        int messagesLeft = 0;
        message = curl_multi_info_read(m_multi->handle, &messagesLeft);
        if (message && CURLMSG_DONE == message->msg) {
            bool isPingStream = m_pingStream && m_pingStream->getCurlHandle() == message->easy_handle;
            bool isDownchannelStream = m_downchannelStream->getCurlHandle() == message->easy_handle;
            if (isPingStream) {
                handlePingResponse();
                continue;
            } else if (isDownchannelStream) {
                if (!isStopping()) {
                    m_observer->onServerSideDisconnect();
                }
                setIsStopping(ConnectionStatusObserverInterface::ChangedReason::SERVER_SIDE_DISCONNECT);
                continue;
            } else {
                auto it = m_activeStreams.find(message->easy_handle);
                if (it != m_activeStreams.end()) {
                    it->second->notifyRequestObserver();
                    cleanupStream(it->second);
                } else {
                    ACSDK_ERROR(LX("cleanupFinishedStreamError").d("reason", "streamNotFound"));
                }
            }
        }
    } while (message);
}

void HTTP2Transport::cleanupStalledStreams() {
    auto it = m_activeStreams.begin();
    while (it != m_activeStreams.end()) {
        auto handle = it->first;
        if (m_pingStream && m_pingStream->getCurlHandle() == handle) {
            ++it;
            continue;
        }
        auto stream = it->second;
        if (stream->hasProgressTimedOut()) {
            ACSDK_INFO(LX("streamProgressTimedOut").d("streamId", stream->getLogicalStreamId()));
            stream->notifyRequestObserver(avsCommon::avs::MessageRequest::Status::TIMEDOUT);
            cleanupStream((it++)->second);
        } else {
            ++it;
        }
    }
}

void HTTP2Transport::cleanupStream(std::shared_ptr<HTTP2Stream> stream) {
    if (!stream) {
        ACSDK_ERROR(LX("cleanupStreamFailed").d("reason", "nullStream"));
        return;
    }
    auto result = curl_multi_remove_handle(m_multi->handle, stream->getCurlHandle());
    if (result != CURLM_OK) {
        ACSDK_ERROR(LX("cleanupStreamFailed")
                .d("reason", "curlFailure")
                .d("method", "curl_multi_remove_handle")
                .d("streamId", stream->getLogicalStreamId())
                .d("result", "stoppingNetworkLoop"));
        setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
    }
    m_activeStreams.erase(stream->getCurlHandle());
    m_streamPool.releaseStream(stream);
}

bool HTTP2Transport::canProcessOutgoingMessage() {
    for (auto stream : m_activeStreams) {
        long responseCode = stream.second->getResponseCode();
        // If we have an event that still hasn't received a response code then we cannot send another outgoing message.
        if (!responseCode) {
            return false;
        }
    }
    // All outstanding streams (if any) have received a response, the next message can now be sent.
    return true;
}

void HTTP2Transport::processNextOutgoingMessage() {
    auto request = dequeueRequest();
    if (!request) {
        return;
    }
    auto authToken = m_authDelegate->getAuthToken();
    if (authToken.empty()) {
        request->onSendCompleted(MessageRequest::Status::INVALID_AUTH);
        return;
    }
    auto url =  m_avsEndpoint + AVS_EVENT_URL_PATH_EXTENSION;
    std::shared_ptr<HTTP2Stream> stream = m_streamPool.createPostStream(url, authToken, request, m_messageConsumer);
    // note : if the stream is nullptr, the streampool already called onSendCompleted on the MessageRequest.
    if (stream) {
        stream->setProgressTimeout(STREAM_PROGRESS_TIMEOUT);
        if (curl_multi_add_handle(m_multi->handle, stream->getCurlHandle()) != CURLM_OK) {
            stream->notifyRequestObserver(MessageRequest::Status::INTERNAL_ERROR);
        } else {
            m_activeStreams.insert(ActiveTransferEntry(stream->getCurlHandle(), stream));
        }
    }
}

bool HTTP2Transport::sendPing() {
    ACSDK_DEBUG(LX("sendPing").d("pingStream", m_pingStream.get()));

    if (m_pingStream) {
        return true;
    }

    std::string authToken = m_authDelegate->getAuthToken();
    if (authToken.empty()) {
        ACSDK_ERROR(LX("sendPingFailed").d("reason", "getAuthTokenFailed"));
        setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INVALID_AUTH);
        return false;
    }

    std::string url = m_avsEndpoint + AVS_PING_URL_PATH_EXTENSION;

    m_pingStream = m_streamPool.createGetStream(url, authToken, m_messageConsumer);
    if (!m_pingStream) {
        ACSDK_ERROR(LX("sendPingFailed").d("reason", "createPingStreamFailed"));
        setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
        return false;
    }

    if (!m_pingStream->setStreamTimeout(PING_RESPONSE_TIMEOUT_SEC)) {
        setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
        return false;
    }

    CURLMcode ret = curl_multi_add_handle(m_multi->handle, m_pingStream->getCurlHandle());
    if (ret != CURLM_OK) {
        ACSDK_ERROR(LX("sendPingFailed")
                .d("reason", "curlFailure")
                .d("method", "curl_multi_add_handle")
                .d("error", curl_multi_strerror(ret)));
        setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
        return false;
    }

    return true;
}

void HTTP2Transport::handlePingResponse() {
    ACSDK_DEBUG(LX("handlePingResponse"));
    if (HTTP2Stream::HTTPResponseCodes::SUCCESS_NO_CONTENT != m_pingStream->getResponseCode()) {
        ACSDK_ERROR(LX("pingFailed")
                .d("responseCode", m_pingStream->getResponseCode()));
        setIsStopping(ConnectionStatusObserverInterface::ChangedReason::SERVER_SIDE_DISCONNECT);
    }
    curl_multi_remove_handle(m_multi->handle, m_pingStream->getCurlHandle());
    m_streamPool.releaseStream(m_pingStream);
    m_pingStream.reset();
}

void HTTP2Transport::setIsStopping(ConnectionStatusObserverInterface::ChangedReason reason) {
    std::lock_guard<std::mutex> lock(m_mutex);
    setIsStoppingLocked(reason);
}

void HTTP2Transport::setIsStoppingLocked(ConnectionStatusObserverInterface::ChangedReason reason) {
    if (m_isStopping) {
        return;
    }
    m_disconnectReason = reason;
    m_isStopping = true;
    m_wakeRetryTrigger.notify_one();
}

bool HTTP2Transport::isStopping() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_isStopping;
}

bool HTTP2Transport::isConnectedLocked() const {
    return m_isConnected && !m_isStopping;
}

void HTTP2Transport::setIsConnectedTrueUnlessStopping() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_isConnected || m_isStopping) {
            return;
        }
        m_isConnected = true;
    }
    m_observer->onConnected();
}

void HTTP2Transport::setIsConnectedFalse() {
    auto disconnectReason = ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_isConnected) {
            return;
        }
        m_isConnected = false;
        disconnectReason = m_disconnectReason;
    }
    m_observer->onDisconnected(disconnectReason);
}

bool HTTP2Transport::enqueueRequest(std::shared_ptr<MessageRequest> request) {
    if (!request) {
        ACSDK_ERROR(LX("enqueueRequestFailed").d("reason", "nullRequest"));
        return false;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isConnected && !m_isStopping) {
        m_requestQueue.push_back(request);
        return true;
    }
    return false;
}

std::shared_ptr<MessageRequest> HTTP2Transport::dequeueRequest() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_requestQueue.empty()) {
        return nullptr;
    }
    auto result = m_requestQueue.front();
    m_requestQueue.pop_front();
    return result;
}


void HTTP2Transport::clearQueuedRequests(){
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto request : m_requestQueue) {
        request->onSendCompleted(MessageRequest::Status::NOT_CONNECTED);
    }
    m_requestQueue.clear();
}

} // acl
} // alexaClientSDK
