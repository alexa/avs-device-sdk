/*
 * Copyright 2016-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <chrono>
#include <functional>
#include <random>

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpResponseCodes.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Timing/TimeUtils.h>
#include <ACL/Transport/PostConnectInterface.h>

#include "ACL/Transport/HTTP2Transport.h"
#include "ACL/Transport/TransportDefines.h"

namespace alexaClientSDK {
namespace acl {

using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::utils::libcurlUtils;
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
/// Default @c AVS endpoint to connect to.
const static std::string DEFAULT_AVS_ENDPOINT = "https://avs-alexa-na.amazon.com";
/// Downchannel URL
const static std::string AVS_DOWNCHANNEL_URL_PATH_EXTENSION = "/v20160207/directives";
/// URL to send events to
const static std::string AVS_EVENT_URL_PATH_EXTENSION = "/v20160207/events";
/// URL to send pings to
const static std::string AVS_PING_URL_PATH_EXTENSION = "/ping";
/// Timeout for curl_multi_wait
const static std::chrono::milliseconds WAIT_FOR_ACTIVITY_TIMEOUT(100);
/// Timeout for curl_multi_wait while all HTTP/2 event streams are paused.
const static std::chrono::milliseconds WAIT_FOR_ACTIVITY_WHILE_STREAMS_PAUSED_TIMEOUT(10);
/// Inactivity timeout before we send a ping
const static std::chrono::minutes INACTIVITY_TIMEOUT = std::chrono::minutes(5);
/// The maximum time a ping should take in seconds
const static long PING_RESPONSE_TIMEOUT_SEC = 30;
/// Connection timeout
static const std::chrono::seconds ESTABLISH_CONNECTION_TIMEOUT = std::chrono::seconds(60);
/// Timeout for transmission of data on a given stream
static const std::chrono::seconds STREAM_PROGRESS_TIMEOUT = std::chrono::seconds(30);
/// Key for the root node value containing configuration values for ACL.
static const std::string ACL_CONFIG_KEY = "acl";
/// Key for the 'endpoint' value under the @c ACL_CONFIG_KEY configuration node.
static const std::string ENDPOINT_KEY = "endpoint";

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

std::shared_ptr<HTTP2Transport> HTTP2Transport::create(
    std::shared_ptr<AuthDelegateInterface> authDelegate,
    const std::string& avsEndpoint,
    std::shared_ptr<MessageConsumerInterface> messageConsumerInterface,
    std::shared_ptr<AttachmentManager> attachmentManager,
    std::shared_ptr<TransportObserverInterface> transportObserver,
    std::shared_ptr<PostConnectFactoryInterface> postConnectFactory) {
    if (!authDelegate) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullAuthDelegate"));
        return nullptr;
    }

    if (!messageConsumerInterface) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMessageConsumerInterface"));
        return nullptr;
    }

    if (!attachmentManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullAttachmentManager"));
        return nullptr;
    }

    if (!postConnectFactory) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullPostConnectFactory"));
        return nullptr;
    }

    auto transport = std::shared_ptr<HTTP2Transport>(new HTTP2Transport(
        authDelegate, avsEndpoint, messageConsumerInterface, attachmentManager, postConnectFactory, transportObserver));

    authDelegate->addAuthObserver(transport);

    return transport;
}

HTTP2Transport::HTTP2Transport(
    std::shared_ptr<AuthDelegateInterface> authDelegate,
    const std::string& avsEndpoint,
    std::shared_ptr<MessageConsumerInterface> messageConsumerInterface,
    std::shared_ptr<AttachmentManager> attachmentManager,
    std::shared_ptr<PostConnectFactoryInterface> postConnectFactory,
    std::shared_ptr<TransportObserverInterface> observer) :
        m_messageConsumer{messageConsumerInterface},
        m_authDelegate{authDelegate},
        m_avsEndpoint{avsEndpoint},
        m_streamPool{MAX_STREAMS, attachmentManager},
        m_disconnectReason{ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR},
        m_isNetworkThreadRunning{false},
        m_isAuthRefreshed{false},
        m_isConnected{false},
        m_isStopping{false},
        m_disconnectedSent{false},
        m_postConnectFactory{postConnectFactory} {
    m_observers.insert(observer);

    printCurlDiagnostics();

    if (m_avsEndpoint.empty()) {
        alexaClientSDK::avsCommon::utils::configuration::ConfigurationNode::getRoot()[ACL_CONFIG_KEY].getString(
            ENDPOINT_KEY, &m_avsEndpoint, DEFAULT_AVS_ENDPOINT);
    }
}

void HTTP2Transport::doShutdown() {
    m_authDelegate->removeAuthObserver(shared_from_this());
    disconnect();
    m_postConnectFactory.reset();
    m_postConnect.reset();
}

bool HTTP2Transport::connect() {
    std::lock_guard<std::mutex> lock(m_mutex);

    /*
     * To handle cases were shutdown was called before the transport is connected. In this case we
     * do not want to spawn a thread and create a post-connect object.
     */
    if (m_isStopping) {
        return false;
    }

    // This function spawns a worker thread, so let's ensure this function may only be called when
    // the worker thread is not running.
    if (m_isNetworkThreadRunning) {
        ACSDK_ERROR(LX("connectFailed").d("reason", "networkThreadAlreadyRunning"));
        return false;
    }

    m_multi = avsCommon::utils::libcurlUtils::CurlMultiHandleWrapper::create();
    if (!m_multi) {
        ACSDK_ERROR(LX("connectFailed").d("reason", "curlMultiHandleWrapperCreateFailed"));
        return false;
    }
    if (curl_multi_setopt(m_multi->getCurlHandle(), CURLMOPT_PIPELINING, 2L) != CURLM_OK) {
        m_multi.reset();
        ACSDK_ERROR(LX("connectFailed").d("reason", "enableHTTP2PipeliningFailed"));
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

    if (localNetworkThread.joinable()) {
        localNetworkThread.join();
    }
    {
        std::lock_guard<std::mutex> lock(m_observerMutex);
        m_observers.clear();
    }
}

bool HTTP2Transport::isConnected() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return isConnectedLocked();
}

void HTTP2Transport::send(std::shared_ptr<MessageRequest> request) {
    if (!request) {
        ACSDK_ERROR(LX("sendFailed").d("reason", "nullRequest"));
    } else if (!enqueueRequest(request, false)) {
        request->sendCompleted(MessageRequestObserverInterface::Status::NOT_CONNECTED);
    }
}

void HTTP2Transport::onAuthStateChange(State newState, Error error) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isAuthRefreshed = (AuthObserverInterface::State::REFRESHED == newState);
    if (m_isAuthRefreshed) {
        m_wakeRetryTrigger.notify_all();
    }
}

void HTTP2Transport::sendPostConnectMessage(std::shared_ptr<MessageRequest> request) {
    if (!request) {
        ACSDK_ERROR(LX("sendFailed").d("reason", "nullRequest"));
    }
    enqueueRequest(request, true);
}

void HTTP2Transport::networkLoop() {
    // Call the post-connect object to do the post-connect operations.
    m_postConnect = m_postConnectFactory->createPostConnect();
    if (!m_postConnect || !m_postConnect->doPostConnect(shared_from_this())) {
        ACSDK_ERROR(LX("networkLoopFailed").d("reason", "failedToCreateAPostConnectObject"));
        setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
    }

    // Establish a connection.
    int retryCount = 0;
    while (true) {
        // Wait for auth state to reach REFRESHED.
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_wakeRetryTrigger.wait(lock, [this] { return m_isAuthRefreshed || m_isStopping; });
        }
        // If Stopping or connected, break out of this loop.
        if (isStopping() || establishConnection()) {
            break;
        }
        // Connect failed.  Schedule a retry.
        std::chrono::milliseconds retryBackoff = TransportDefines::RETRY_TIMER.calculateTimeToRetry(retryCount);
        ACSDK_ERROR(LX("networkLoopRetryingToConnect")
                        .d("reason", "establishConnectionFailed")
                        .d("retryCount", retryCount)
                        .d("retryBackoff", retryBackoff.count()));
        retryCount++;
        std::unique_lock<std::mutex> lock(m_mutex);
        m_wakeRetryTrigger.wait_for(lock, retryBackoff, [this] { return m_isStopping; });
    }

    /*
     * Call perform repeatedly to transfer data on active streams. If all the event streams have HTTP2
     * response codes service the next outgoing message (if any).  While the connection is alive we should have
     * at least 1 transfer active (the downchannel).
     */
    int numTransfersLeft = 1;
    auto inactivityTimerStart = std::chrono::steady_clock::now();
    while (numTransfersLeft && !isStopping()) {
        auto result = m_multi->perform(&numTransfersLeft);
        if (CURLM_CALL_MULTI_PERFORM == result) {
            continue;
        } else if (result != CURLM_OK) {
            ACSDK_ERROR(LX("networkLoopStopping").d("reason", "performFailed"));
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

        auto multiWaitTimeout = WAIT_FOR_ACTIVITY_TIMEOUT;

        size_t numberEventStreams = 0;
        size_t numberPausedStreams = 0;
        for (auto entry : m_activeStreams) {
            auto stream = entry.second;
            if (isEventStream(stream)) {
                numberEventStreams++;
                if (entry.second->isPaused()) {
                    numberPausedStreams++;
                }
            }
        }
        bool paused = numberPausedStreams > 0 && (numberPausedStreams == numberEventStreams);

        auto before = std::chrono::time_point<std::chrono::steady_clock>::max();
        if (paused) {
            multiWaitTimeout = WAIT_FOR_ACTIVITY_WHILE_STREAMS_PAUSED_TIMEOUT;
            before = std::chrono::steady_clock::now();
        }

        // TODO: ACSDK-69 replace timeout with signal fd

        int numTransfersUpdated = 0;
        result = m_multi->wait(multiWaitTimeout, &numTransfersUpdated);
        if (result != CURLM_OK) {
            ACSDK_ERROR(
                LX("networkLoopStopping").d("reason", "multiWaitFailed").d("error", curl_multi_strerror(result)));
            setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
            break;
        }

        // TODO: ACSDK-69 replace this logic with a signal fd

        // @note curl_multi_wait will return immediately even if all streams are paused, because HTTP/2 streams
        // are full-duplex - so activity may have occurred on the other side. Therefore, if our intent is
        // to pause ACL to give attachment readers time to catch up with written data, we must perform a local
        // sleep of our own.
        if (paused) {
            auto after = std::chrono::steady_clock::now();
            auto elapsed = after - before;
            auto remaining = multiWaitTimeout - elapsed;

            // sanity check that remainingMs is valid before performing a sleep.
            if (remaining.count() > 0 && remaining <= WAIT_FOR_ACTIVITY_WHILE_STREAMS_PAUSED_TIMEOUT) {
                std::this_thread::sleep_for(remaining);
            }
        }

        /**
         * If some transfers were updated then reset the start of the inactivity timer to now.  Otherwise,
         * if the INACTIVITY_TIMEOUT has been reached send a ping to AVS so verify connectivity.
         */
        auto now = std::chrono::steady_clock::now();
        if (numTransfersUpdated != 0) {
            inactivityTimerStart = now;
        } else {
            auto elapsed = now - inactivityTimerStart;
            if (elapsed >= INACTIVITY_TIMEOUT) {
                if (!sendPing()) {
                    ACSDK_ERROR(LX("networkLoopStopping").d("reason", "sendPingFailed"));
                    setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
                    break;
                }
                inactivityTimerStart = now;
            }
        }

        // un-pause the streams so that progress may be made in the next invocation of @c m_multi->perform().
        for (auto stream : m_activeStreams) {
            stream.second->unPause();
        }
    }

    // Catch-all. Reaching this point implies stopping.
    setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);

    releaseAllEventStreams();
    releasePingStream();
    releaseDownchannelStream(true);
    m_multi.reset();
    clearQueuedRequests();
    setIsConnectedFalse();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_isNetworkThreadRunning = false;
    }
}

bool HTTP2Transport::establishConnection() {
    if (!setupDownchannelStream()) {
        ACSDK_ERROR(LX("establishConnectionFailed").d("reason", "setupDownchannelStreamFailed"));
        return false;
    }

    // Set numTransferLeft to 1 because the downchannel stream has been added already.
    int numTransfersLeft = 1;

    // Zero indicates no response yet.
    long downchannelResponseCode = 0;

    // Start of time interval used to limit log spam when waiting for activity.
    auto lastActivityLogTime = std::chrono::steady_clock::now();

    /*
     * Calls curl_multi_perform until down channel stream receives an HTTP2 response code. If the down channel stream
     * Call perform() to further networking transfers until the down channel stream receives an HTTP2 response
     * ends before receiving a response code (numTransfersLeft == 0), then there was an error and we must try again.
     * code indicating success or failure to establish a connection. If the down channel stream ends before
     * If we're told to shutdown the network loop (isStopping()) then return false since no connection was established.
     * receiving a response code (numTransfersLeft == 0), then set up a new down channel stream in preparation
     * for being called again to retry establishing a connection.
     */
    while (numTransfersLeft && !isStopping()) {
        auto result = m_multi->perform(&numTransfersLeft);
        // curl asked us to call multiperform again immediately
        if (CURLM_CALL_MULTI_PERFORM == result) {
            continue;
        } else if (result != CURLM_OK) {
            ACSDK_ERROR(LX("establishConnectionFailed").d("reason", "performFailed"));
            setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
        }
        downchannelResponseCode = m_downchannelStream->getResponseCode();
        /* if the downchannel response code is > 0 then we have some response from the backend
         * if its < 0 there was a problem getting the response code from the easy handle
         * if its == 0 then keep looping since we have not yet received a response
         */
        if (downchannelResponseCode > 0) {
            /*
             * Only break the loop if we are successful. If we aren't keep looping so that we download
             * the full error message (for logging purposes) and then return false when we're done
             */
            if (HTTPResponseCode::SUCCESS_OK == downchannelResponseCode) {
                return true;
            }
        } else if (downchannelResponseCode < 0) {
            ACSDK_ERROR(LX("establishConnectionFailed")
                            .d("reason", "negativeResponseCode")
                            .d("responseCode", downchannelResponseCode));
            setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
        } else {
            auto now = std::chrono::steady_clock::now();
            if (now - lastActivityLogTime > WAIT_FOR_ACTIVITY_TIMEOUT) {
                lastActivityLogTime = now;
                ACSDK_DEBUG9(LX("establishConnectionWaitingForActivity"));
            }
        }
        // wait for activity on the downchannel stream, kinda like poll()
        int numTransfersUpdated = 0;
        result = m_multi->wait(WAIT_FOR_ACTIVITY_TIMEOUT, &numTransfersUpdated);
        if (result != CURLM_OK) {
            ACSDK_ERROR(
                LX("establishConnectionFailed").d("reason", "waitFailed").d("error", curl_multi_strerror(result)));
            setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
        }
    }
    ACSDK_ERROR(LX("establishConnectionFailed")
                    .d("code", downchannelResponseCode)
                    .d("body", m_downchannelStream->getFailureBody()));
    return false;
}

bool HTTP2Transport::setupDownchannelStream() {
    if (m_downchannelStream && !releaseDownchannelStream(true)) {
        ACSDK_ERROR(LX("setupDownchannelStreamFiled").d("reason", "releaseDownchannelStreamFailed"));
        setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
        return false;
    }

    const std::string authToken = m_authDelegate->getAuthToken();

    if (authToken.empty()) {
        ACSDK_ERROR(LX("setupDownchannelStreamFiled").d("reason", "getAuthTokenFailed"));
        return false;
    }

    const std::string url = m_avsEndpoint + AVS_DOWNCHANNEL_URL_PATH_EXTENSION;
    ACSDK_DEBUG3(LX("setupDownchannelStream").d("url", url));

    m_downchannelStream = m_streamPool.createGetStream(url, authToken, m_messageConsumer);
    if (!m_downchannelStream) {
        ACSDK_ERROR(LX("setupDownchannelStreamFiled").d("reason", "createGetStreamFailed"));
        setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
        return false;
    }
    // Since the downchannel is the first stream to be established, make sure it times out if
    // a connection can't be established.
    if (!m_downchannelStream->setConnectionTimeout(ESTABLISH_CONNECTION_TIMEOUT)) {
        releaseDownchannelStream(false);
        ACSDK_ERROR(LX("setupDownchannelStreamFiled").d("reason", "setConnectionTimeoutFailed"));
        setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
        return false;
    }

    auto result = m_multi->addHandle(m_downchannelStream->getCurlHandle());
    if (result != CURLM_OK) {
        releaseDownchannelStream(false);
        ACSDK_ERROR(LX("setupDownchannelStreamFiled").d("reason", "addHandleFailed"));
        setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
        return false;
    }

    m_activeStreams.insert(ActiveTransferEntry(m_downchannelStream->getCurlHandle(), m_downchannelStream));
    return true;
}

void HTTP2Transport::cleanupFinishedStreams() {
    CURLMsg* message = nullptr;
    do {
        int messagesLeft = 0;
        message = m_multi->infoRead(&messagesLeft);
        if (message && CURLMSG_DONE == message->msg) {
            if (m_downchannelStream && m_downchannelStream->getCurlHandle() == message->easy_handle) {
                if (!isStopping()) {
                    notifyObserversOnServerSideDisconnect();
                }
                releaseDownchannelStream(true);
                continue;
            }

            if (m_pingStream && m_pingStream->getCurlHandle() == message->easy_handle) {
                handlePingResponse();
                continue;
            }

            auto it = m_activeStreams.find(message->easy_handle);
            if (it != m_activeStreams.end()) {
                it->second->notifyRequestObserver();
                ACSDK_DEBUG0(LX("cleanupFinishedStream")
                                 .d("streamId", it->second->getLogicalStreamId())
                                 .d("result", it->second->getResponseCode()));
                releaseEventStream(it->second);
            } else {
                ACSDK_ERROR(
                    LX("cleanupFinishedStreamError").d("reason", "streamNotFound").d("handle", message->easy_handle));
            }
        }
    } while (message);
}

void HTTP2Transport::cleanupStalledStreams() {
    auto it = m_activeStreams.begin();
    bool hasStalledStream = false;
    while (it != m_activeStreams.end()) {
        auto stream = (it++)->second;
        if (isEventStream(stream) && stream->hasProgressTimedOut()) {
            ACSDK_INFO(LX("streamProgressTimedOut").d("streamId", stream->getLogicalStreamId()));
            stream->notifyRequestObserver(MessageRequestObserverInterface::Status::TIMEDOUT);
            releaseEventStream(stream);
            hasStalledStream = true;
        }
    }
    if (hasStalledStream) {
        // Send a ping if a stream has not progressed for the duration of the stream timeout to make sure we're still
        // connected
        if (!sendPing()) {
            ACSDK_INFO(LX("networkLoopStopping").d("reason", "sendPingFailed"));
            setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
        }
    }
}

bool HTTP2Transport::canProcessOutgoingMessage() {
    for (auto entry : m_activeStreams) {
        auto stream = entry.second;
        // If we have an event that still hasn't received a response code then we cannot send another outgoing message.
        if (isEventStream(stream) && (stream->getResponseCode() == 0)) {
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
        ACSDK_DEBUG0(LX("processNextOutgoingMessageFailed")
                         .d("reason", "invalidAuth")
                         .sensitive("jsonContext", request->getJsonContent()));
        request->sendCompleted(MessageRequestObserverInterface::Status::INVALID_AUTH);
        return;
    }
    ACSDK_DEBUG0(LX("processNextOutgoingMessage")
                     .sensitive("jsonContent", request->getJsonContent())
                     .sensitive("uriPathExtension", request->getUriPathExtension()));

    // Allow custom path extension, if provided by the sender of the MessageRequest
    std::string pathExtension = AVS_EVENT_URL_PATH_EXTENSION;
    if (!request->getUriPathExtension().empty()) {
        pathExtension = request->getUriPathExtension();
    }

    auto url = m_avsEndpoint + pathExtension;
    std::shared_ptr<HTTP2Stream> stream = m_streamPool.createPostStream(url, authToken, request, m_messageConsumer);
    // note : if the stream is nullptr, the stream pool already called sendCompleted on the MessageRequest.
    if (stream) {
        stream->setProgressTimeout(STREAM_PROGRESS_TIMEOUT);
        auto result = m_multi->addHandle(stream->getCurlHandle());
        if (result != CURLM_OK) {
            ACSDK_ERROR(LX("processNextOutgoingMessageFailed")
                            .d("reason", "addHandleFailed")
                            .d("error", curl_multi_strerror(result))
                            .d("streamId", stream->getLogicalStreamId()));
            m_streamPool.releaseStream(stream);
            stream->notifyRequestObserver(MessageRequestObserverInterface::Status::INTERNAL_ERROR);
        } else {
            ACSDK_DEBUG9(LX("insertActiveStream").d("handle", stream->getCurlHandle()));
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
        releasePingStream(false);
        setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
        return false;
    }

    auto result = m_multi->addHandle(m_pingStream->getCurlHandle());
    if (result != CURLM_OK) {
        ACSDK_ERROR(LX("sendPingFailed").d("reason", "addHandleFailed"));
        releasePingStream(false);
        setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
        return false;
    }

    m_activeStreams.insert(ActiveTransferEntry(m_pingStream->getCurlHandle(), m_pingStream));
    return true;
}

void HTTP2Transport::handlePingResponse() {
    ACSDK_DEBUG(LX("handlePingResponse"));
    if (HTTPResponseCode::SUCCESS_NO_CONTENT != m_pingStream->getResponseCode()) {
        ACSDK_ERROR(LX("pingFailed").d("responseCode", m_pingStream->getResponseCode()));
        setIsStopping(ConnectionStatusObserverInterface::ChangedReason::SERVER_SIDE_DISCONNECT);
    }
    releasePingStream();
}

void HTTP2Transport::onPostConnected() {
    m_postConnect.reset();
    setIsConnectedTrueUnlessStopping();
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

    notifyObserversOnConnected();
}

void HTTP2Transport::setIsConnectedFalse() {
    auto disconnectReason = ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_disconnectedSent) {
            return;
        }
        m_disconnectedSent = true;
        m_isConnected = false;
        disconnectReason = m_disconnectReason;
    }

    notifyObserversOnDisconnect(disconnectReason);
}

bool HTTP2Transport::enqueueRequest(std::shared_ptr<MessageRequest> request, bool ignoreConnectState) {
    if (!request) {
        ACSDK_ERROR(LX("enqueueRequestFailed").d("reason", "nullRequest"));
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_isStopping) {
        if (ignoreConnectState || m_isConnected) {
            ACSDK_DEBUG9(LX("enqueueRequest").sensitive("jsonContent", request->getJsonContent()));
            m_requestQueue.push_back(request);
            return true;
        } else {
            ACSDK_ERROR(LX("enqueueRequestFailed").d("reason", "isNotConnected"));
        }
    } else {
        ACSDK_ERROR(LX("enqueueRequestFailed").d("reason", "isStopping"));
    }
    return false;
}

std::shared_ptr<MessageRequest> HTTP2Transport::dequeueRequest() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isStopping || m_requestQueue.empty()) {
        return nullptr;
    }
    auto result = m_requestQueue.front();
    m_requestQueue.pop_front();
    return result;
}

void HTTP2Transport::clearQueuedRequests() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto request : m_requestQueue) {
        request->sendCompleted(MessageRequestObserverInterface::Status::NOT_CONNECTED);
    }
    m_requestQueue.clear();
}

void HTTP2Transport::addObserver(std::shared_ptr<TransportObserverInterface> transportObserver) {
    if (!transportObserver) {
        ACSDK_ERROR(LX("addObserverFailed").d("reason", "nullObserver"));
        return;
    }

    std::lock_guard<std::mutex> lock{m_observerMutex};
    m_observers.insert(transportObserver);
}

void HTTP2Transport::removeObserver(std::shared_ptr<TransportObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("removeObserverFailed").d("reason", "nullObserver"));
        return;
    }

    std::lock_guard<std::mutex> lock{m_observerMutex};
    m_observers.erase(observer);
}

void HTTP2Transport::notifyObserversOnServerSideDisconnect() {
    if (m_postConnect) {
        m_postConnect->onDisconnect();
        m_postConnect.reset();
    }

    std::unique_lock<std::mutex> lock{m_observerMutex};
    auto observers = m_observers;
    lock.unlock();

    for (auto observer : observers) {
        observer->onServerSideDisconnect(shared_from_this());
    }
}

void HTTP2Transport::notifyObserversOnDisconnect(ConnectionStatusObserverInterface::ChangedReason reason) {
    if (m_postConnect) {
        m_postConnect->onDisconnect();
        m_postConnect.reset();
    }

    std::unique_lock<std::mutex> lock{m_observerMutex};
    auto observers = m_observers;
    lock.unlock();

    for (auto observer : observers) {
        observer->onDisconnected(shared_from_this(), reason);
    }
}

void HTTP2Transport::notifyObserversOnConnected() {
    std::unique_lock<std::mutex> lock{m_observerMutex};
    auto observers = m_observers;
    lock.unlock();

    for (auto observer : observers) {
        observer->onConnected(shared_from_this());
    }
}

bool HTTP2Transport::releaseDownchannelStream(bool removeFromMulti) {
    if (m_downchannelStream) {
        if (!releaseStream(m_downchannelStream, removeFromMulti, "downchannel")) {
            return false;
        }
        m_downchannelStream.reset();
    }
    return true;
}

bool HTTP2Transport::releasePingStream(bool removeFromMulti) {
    if (m_pingStream) {
        if (!releaseStream(m_pingStream, removeFromMulti, "ping")) {
            setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
            return false;
        }
        m_pingStream.reset();
    }
    return true;
}

void HTTP2Transport::releaseAllEventStreams() {
    // Get a copy of the event streams to release.
    std::vector<std::shared_ptr<HTTP2Stream>> eventStreams;
    for (auto entry : m_activeStreams) {
        auto stream = entry.second;
        if (isEventStream(stream)) {
            eventStreams.push_back(stream);
        }
    }
    for (auto stream : eventStreams) {
        releaseEventStream(stream);
    }
}

bool HTTP2Transport::releaseEventStream(std::shared_ptr<HTTP2Stream> stream, bool removeFromMulti) {
    if (stream) {
        if (!releaseStream(stream, removeFromMulti, "event")) {
            setIsStopping(ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
            return false;
        }
    }
    return true;
}

bool HTTP2Transport::releaseStream(std::shared_ptr<HTTP2Stream> stream, bool removeFromMulti, const std::string& name) {
    auto handle = stream->getCurlHandle();
    m_activeStreams.erase(handle);
    if (removeFromMulti) {
        auto result = m_multi->removeHandle(handle);
        if (result != CURLM_OK) {
            ACSDK_ERROR(LX("releaseStreamFailed")
                            .d("reason", "removeHandleFailed")
                            .d("streamId", stream->getLogicalStreamId())
                            .d("name", name));
            return false;
        }
    }
    m_streamPool.releaseStream(stream);
    return true;
}

bool HTTP2Transport::isEventStream(std::shared_ptr<HTTP2Stream> stream) {
    return stream != m_downchannelStream && stream != m_pingStream;
}

}  // namespace acl
}  // namespace alexaClientSDK
