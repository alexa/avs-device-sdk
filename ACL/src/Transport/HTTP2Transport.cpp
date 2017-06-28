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
#include <random>
#include <functional>

#include "AVSCommon/Utils/Logger/DeprecatedLogger.h"

namespace alexaClientSDK {
namespace acl {

using namespace avsCommon::sdkInterfaces;
using namespace alexaClientSDK::avsCommon::utils;

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

/**
 * This function logs a warning if the version of curl is not recent enough for use with the ACL.
 */
static void printCurlDiagnostics() {
#ifdef DEBUG
    // TODO: ACSDK-79 add libcurl initialization to the ACL
    curl_version_info_data* data = curl_version_info(CURLVERSION_NOW);
    if (!(data->features & CURL_VERSION_HTTP2)) {
        logger::deprecated::Logger::log("WARNING: This libcurl does not have HTTP/2 support built!");
    }
#endif
}

HTTP2Transport::HTTP2Transport(std::shared_ptr<AuthDelegateInterface> authDelegate, const std::string& avsEndpoint,
        MessageConsumerInterface* messageConsumerInterface,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> attachmentManager,
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
        logger::deprecated::Logger::log("HTTP2Transport::connect() - connection already being attempted.  Returning false.");
        return false;
    }

    m_multi.reset(new MultiHandle());
    m_multi->handle = curl_multi_init();
    if (!m_multi->handle) {
        m_multi.reset();
        logger::deprecated::Logger::log("Could not create curl multi handle");
        return false;
    }
    if (curl_multi_setopt(m_multi->handle, CURLMOPT_PIPELINING, 2L) != CURLM_OK) {
        m_multi.reset();
        logger::deprecated::Logger::log("Could not enable HTTP2 pipelining");
        return false;
    }

    if (!setupDownchannelStreamLocked()) {
        m_multi.reset();
        logger::deprecated::Logger::log("Could not setup Downchannel stream");
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

void HTTP2Transport::send(std::shared_ptr<avsCommon::avs::MessageRequest> request) {
    if (!request) {
        logger::deprecated::Logger::log("sendFailed:nullRequest");
    } else if (!enqueueRequest(request)) {
        request->onSendCompleted(avsCommon::avs::MessageRequest::Status::NOT_CONNECTED);
    }
}

bool HTTP2Transport::setupDownchannelStreamLocked() {
    if (m_downchannelStream) {
        CURLMcode ret = curl_multi_remove_handle(m_multi->handle, m_downchannelStream->getCurlHandle());
        if (ret != CURLM_OK) {
            logger::deprecated::Logger::log(
                    std::string("Could not remove downchannel stream from multi handle. error=") +
                            curl_multi_strerror(ret));
            setIsStoppingLocked(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
            return false;
        }
        m_streamPool.releaseStream(m_downchannelStream);
        m_downchannelStream.reset();
    }

    std::string authToken = m_authDelegate->getAuthToken();
    if (authToken.empty()) {
        logger::deprecated::Logger::log("Could not get auth token.");
        setIsStoppingLocked(ConnectionStatusObserverInterface::ChangedReason::INVALID_AUTH);
        return false;
    }

    std::string url = m_avsEndpoint + AVS_DOWNCHANNEL_URL_PATH_EXTENSION;
    m_downchannelStream = m_streamPool.createGetStream(url, authToken, m_messageConsumer);
    if (!m_downchannelStream) {
        logger::deprecated::Logger::log("Could not setup downchannel stream");
        setIsStoppingLocked(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
        return false;
    }
    // Since the downchannel is the first stream to be established, make sure it times out if
    // a connection can't be established.
    if (!m_downchannelStream->setConnectionTimeout(ESTABLISH_CONNECTION_TIMEOUT)) {
        m_streamPool.releaseStream(m_downchannelStream);
        m_downchannelStream.reset();
        setIsStoppingLocked(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
        return false;
    }

    auto result = curl_multi_add_handle(m_multi->handle, m_downchannelStream->getCurlHandle());
    if (result != CURLM_OK) {
        m_streamPool.releaseStream(m_downchannelStream);
        m_downchannelStream.reset();
        logger::deprecated::Logger::log(
                std::string("Could not add downchannel stream to multi handle. error=") +
                        curl_multi_strerror(result));
        setIsStoppingLocked(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
        return false;
    }

    return true;
}

void HTTP2Transport::networkLoop() {

    int retryCount = 0;
    while (!establishConnection() && !isStopping()) {
        retryCount++;
        logger::deprecated::Logger::log("Could not setup downchannel, retry count: " + std::to_string(retryCount));
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
    while (numTransfersLeft && !isStopping()) {

        int numTransfersUpdated = 0;
        int timeouts = 0;

        CURLMcode ret = curl_multi_perform(m_multi->handle, &numTransfersLeft);
        if (CURLM_CALL_MULTI_PERFORM == ret) {
            continue;
        } else if (ret != CURLM_OK) {
            std::string error("CURL multi perform failed: ");
            error.append(curl_multi_strerror(ret));
            logger::deprecated::Logger::log(error);
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
        ret = curl_multi_wait(m_multi->handle, NULL, 0, multiWaitTimeoutMs, &numTransfersUpdated);
        if (ret != CURLM_OK) {
            std::string error("CURL multi wait failed: ");
            error.append(curl_multi_strerror(ret));
            logger::deprecated::Logger::log(error);
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
                    logger::deprecated::Logger::log("could not send ping!");
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
        (*it).second->notifyRequestObserver(avsCommon::avs::MessageRequest::Status::NOT_CONNECTED);
        if (curl_multi_remove_handle(m_multi->handle, (*it).first) != CURLM_OK) {
            logger::deprecated::Logger::log("Could not remove easy handle from multi handle");
            (*it).second.reset(); // Force stream to be deleted, also don't put back in the pool
            it = m_activeStreams.erase(it);
            continue;
        }
        m_streamPool.releaseStream((*it).second);
        it = m_activeStreams.erase(it);
    }
    if (curl_multi_remove_handle(m_multi->handle, m_downchannelStream->getCurlHandle()) != CURLM_OK) {
        logger::deprecated::Logger::log("Could not remove downchannel handle from multi handle");
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
    int numTransfersUpdated = 0;

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
            std::string error("CURL multi perform failed: ");
            error.append(curl_multi_strerror(ret));
            logger::deprecated::Logger::log(error);
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
            logger::deprecated::Logger::log("Getting downchannel response code failed!");
            setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
        }
        // wait for activity on the downchannel stream, kinda like poll()
        ret = curl_multi_wait(m_multi->handle, NULL, 0 , WAIT_FOR_ACTIVITY_TIMEOUT_MS, &numTransfersUpdated);
        if (ret != CURLM_OK) {
            std::string error("CURL multi wait failed: ");
            error.append(curl_multi_strerror(ret));
            logger::deprecated::Logger::log(error);
            setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
        }
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!setupDownchannelStreamLocked()) {
        logger::deprecated::Logger::log("establishConnectionFailed:reason=setupDownchannelStreamFailed.");
        setIsStoppingLocked(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
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
                std::shared_ptr<HTTP2Stream> stream = m_activeStreams[message->easy_handle];
                stream->notifyRequestObserver();
                curl_multi_remove_handle(m_multi->handle, message->easy_handle);
                m_activeStreams.erase(message->easy_handle);
                m_streamPool.releaseStream(stream);
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
            std::stringstream message;
            message << "streamProgressTimedOut:streamId=" << stream->getLogicalStreamId();
            logger::deprecated::Logger::log(message.str());
            stream->notifyRequestObserver(avsCommon::avs::MessageRequest::Status::TIMEDOUT);
            auto result = curl_multi_remove_handle(m_multi->handle, handle);
            if (result != CURLM_OK) {
                message.clear();
                message
                        << "cleanupStalledStreamsError:reason=curl_multi_remove_handleFailed"
                        << ",error=" << curl_multi_strerror(result)
                        << ",streamId=" << stream->getLogicalStreamId()
                        << ",result=stoppingNetworkLoop";
                logger::deprecated::Logger::log(message.str());
                setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
            }
            ++it;
            m_activeStreams.erase(handle);
            m_streamPool.releaseStream(stream);
        } else {
            ++it;
        }
    }
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
        request->onSendCompleted(avsCommon::avs::MessageRequest::Status::INVALID_AUTH);
        return;
    }
    auto url =  m_avsEndpoint + AVS_EVENT_URL_PATH_EXTENSION;
    std::shared_ptr<HTTP2Stream> stream = m_streamPool.createPostStream(url, authToken, request, m_messageConsumer);
    // note : if the stream is nullptr, the streampool already called onSendCompleted on the MessageRequest.
    if (stream) {
        stream->setProgressTimeout(STREAM_PROGRESS_TIMEOUT);
        if (curl_multi_add_handle(m_multi->handle, stream->getCurlHandle()) != CURLM_OK) {
            stream->notifyRequestObserver(avsCommon::avs::MessageRequest::Status::INTERNAL_ERROR);
        } else {
            m_activeStreams.insert(ActiveTransferEntry(stream->getCurlHandle(), stream));
        }
    }
}

bool HTTP2Transport::sendPing() {
    if (m_pingStream) {
        return true;
    }

    std::string authToken = m_authDelegate->getAuthToken();
    if (authToken.empty()) {
        logger::deprecated::Logger::log("Could not get auth token.");
        setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INVALID_AUTH);
        return false;
    }

    std::string url = m_avsEndpoint + AVS_PING_URL_PATH_EXTENSION;

    m_pingStream = m_streamPool.createGetStream(url, authToken, m_messageConsumer);
    if (!m_pingStream) {
        logger::deprecated::Logger::log("Could not create ping stream");
        setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
        return false;
    }

    if (!m_pingStream->setStreamTimeout(PING_RESPONSE_TIMEOUT_SEC)) {
        setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
        return false;
    }

    CURLMcode ret = curl_multi_add_handle(m_multi->handle, m_pingStream->getCurlHandle());
    if (ret != CURLM_OK) {
        std::string err = "Could not add ping stream to curl multi handle returned: ";
        logger::deprecated::Logger::log(err + curl_multi_strerror(ret));
        setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
        return false;
    }

    return true;
}

void HTTP2Transport::handlePingResponse() {
    if (HTTP2Stream::HTTPResponseCodes::SUCCESS_NO_CONTENT != m_pingStream->getResponseCode()) {
        logger::deprecated::Logger::log("Ping failed returned: " + std::to_string(m_pingStream->getResponseCode()));
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

bool HTTP2Transport::enqueueRequest(std::shared_ptr<avsCommon::avs::MessageRequest> request) {
    if (!request) {
        logger::deprecated::Logger::log("enqueueRequestFailed:nullRequest");
        return false;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isConnected && !m_isStopping) {
        m_requestQueue.push_back(request);
        return true;
    }
    return false;
}

std::shared_ptr<avsCommon::avs::MessageRequest> HTTP2Transport::dequeueRequest() {
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
        request->onSendCompleted(avsCommon::avs::MessageRequest::Status::NOT_CONNECTED);
    }
    m_requestQueue.clear();
}

} // acl
} // alexaClientSDK
