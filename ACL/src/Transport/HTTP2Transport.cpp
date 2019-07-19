/*
 * Copyright 2016-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <AVSCommon/Utils/HTTP/HttpResponseCode.h>
#include <AVSCommon/Utils/HTTP2/HTTP2MimeRequestEncoder.h>
#include <AVSCommon/Utils/HTTP2/HTTP2MimeResponseDecoder.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Timing/TimeUtils.h>
#include <ACL/Transport/PostConnectInterface.h>

#include "ACL/Transport/DownchannelHandler.h"
#include "ACL/Transport/HTTP2Transport.h"
#include "ACL/Transport/MessageRequestHandler.h"
#include "ACL/Transport/PingHandler.h"
#include "ACL/Transport/TransportDefines.h"

namespace alexaClientSDK {
namespace acl {

using namespace alexaClientSDK::avsCommon::utils;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::utils::http2;

/// String to identify log entries originating from this file.
static const std::string TAG("HTTP2Transport");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The maximum number of streams we can have active at once.  Please see here for more information:
/// https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/docs/managing-an-http-2-connection
static const int MAX_STREAMS = 10;

/// Max number of message requests MAX_STREAMS - 2 (for the downchannel stream and the ping stream)
static const int MAX_MESSAGE_HANDLERS = MAX_STREAMS - 2;

/// Timeout to send a ping to AVS if there has not been any other acitivity on the connection.
static std::chrono::minutes INACTIVITY_TIMEOUT{5};

/**
 * Write a @c HTTP2Transport::State value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param state The state to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
std::ostream& operator<<(std::ostream& stream, HTTP2Transport::State state) {
    switch (state) {
        case HTTP2Transport::State::INIT:
            return stream << "INIT";
        case HTTP2Transport::State::AUTHORIZING:
            return stream << "AUTHORIZING";
        case HTTP2Transport::State::CONNECTING:
            return stream << "CONNECTING";
        case HTTP2Transport::State::WAITING_TO_RETRY_CONNECTING:
            return stream << "WAITING_TO_RETRY_CONNECTING";
        case HTTP2Transport::State::POST_CONNECTING:
            return stream << "POST_CONNECTING";
        case HTTP2Transport::State::CONNECTED:
            return stream << "CONNECTED";
        case HTTP2Transport::State::SERVER_SIDE_DISCONNECT:
            return stream << "SERVER_SIDE_DISCONNECT";
        case HTTP2Transport::State::DISCONNECTING:
            return stream << "DISCONNECTING";
        case HTTP2Transport::State::SHUTDOWN:
            return stream << "SHUTDOWN";
    }
    return stream << "";
}

HTTP2Transport::Configuration::Configuration() : inactivityTimeout{INACTIVITY_TIMEOUT} {
}

std::shared_ptr<HTTP2Transport> HTTP2Transport::create(
    std::shared_ptr<AuthDelegateInterface> authDelegate,
    const std::string& avsEndpoint,
    std::shared_ptr<HTTP2ConnectionInterface> http2Connection,
    std::shared_ptr<MessageConsumerInterface> messageConsumer,
    std::shared_ptr<AttachmentManager> attachmentManager,
    std::shared_ptr<TransportObserverInterface> transportObserver,
    std::shared_ptr<PostConnectFactoryInterface> postConnectFactory,
    Configuration configuration) {
    ACSDK_DEBUG5(LX(__func__)
                     .d("authDelegate", authDelegate.get())
                     .d("avsEndpoint", avsEndpoint)
                     .d("http2Connection", http2Connection.get())
                     .d("messageConsumer", messageConsumer.get())
                     .d("attachmentManager", attachmentManager.get())
                     .d("transportObserver", transportObserver.get())
                     .d("postConnectFactory", postConnectFactory.get()));

    if (!authDelegate) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullAuthDelegate"));
        return nullptr;
    }

    if (avsEndpoint.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "emptyEndpoint"));
        return nullptr;
    }

    if (!http2Connection) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullHTTP2ConnectionInterface"));
        return nullptr;
    }

    if (!messageConsumer) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMessageConsumer"));
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
        authDelegate,
        avsEndpoint,
        http2Connection,
        messageConsumer,
        attachmentManager,
        transportObserver,
        postConnectFactory,
        configuration));

    return transport;
}

HTTP2Transport::HTTP2Transport(
    std::shared_ptr<AuthDelegateInterface> authDelegate,
    const std::string& avsEndpoint,
    std::shared_ptr<HTTP2ConnectionInterface> http2Connection,
    std::shared_ptr<MessageConsumerInterface> messageConsumer,
    std::shared_ptr<AttachmentManager> attachmentManager,
    std::shared_ptr<TransportObserverInterface> transportObserver,
    std::shared_ptr<PostConnectFactoryInterface> postConnectFactory,
    Configuration configuration) :
        m_state{State::INIT},
        m_authDelegate{authDelegate},
        m_avsEndpoint{avsEndpoint},
        m_http2Connection{http2Connection},
        m_messageConsumer{messageConsumer},
        m_attachmentManager{attachmentManager},
        m_postConnectFactory{postConnectFactory},
        m_connectRetryCount{0},
        m_isMessageHandlerAwaitingResponse{false},
        m_countOfUnfinishedMessageHandlers{0},
        m_postConnected{false},
        m_configuration{configuration},
        m_disconnectReason{ConnectionStatusObserverInterface::ChangedReason::NONE} {
    ACSDK_DEBUG5(LX(__func__)
                     .d("authDelegate", authDelegate.get())
                     .d("avsEndpoint", avsEndpoint)
                     .d("http2Connection", http2Connection.get())
                     .d("messageConsumer", messageConsumer.get())
                     .d("attachmentManager", attachmentManager.get())
                     .d("transportObserver", transportObserver.get())
                     .d("postConnectFactory", postConnectFactory.get()));
    m_observers.insert(transportObserver);
}

void HTTP2Transport::addObserver(std::shared_ptr<TransportObserverInterface> transportObserver) {
    ACSDK_DEBUG5(LX(__func__).d("transportObserver", transportObserver.get()));

    if (!transportObserver) {
        ACSDK_ERROR(LX("addObserverFailed").d("reason", "nullObserver"));
        return;
    }

    std::lock_guard<std::mutex> lock{m_observerMutex};
    m_observers.insert(transportObserver);
}

void HTTP2Transport::removeObserver(std::shared_ptr<TransportObserverInterface> transportObserver) {
    ACSDK_DEBUG5(LX(__func__).d("transportObserver", transportObserver.get()));

    if (!transportObserver) {
        ACSDK_ERROR(LX("removeObserverFailed").d("reason", "nullObserver"));
        return;
    }

    std::lock_guard<std::mutex> lock{m_observerMutex};
    m_observers.erase(transportObserver);
}

std::shared_ptr<HTTP2ConnectionInterface> HTTP2Transport::getHTTP2Connection() {
    return m_http2Connection;
}

bool HTTP2Transport::connect() {
    ACSDK_DEBUG5(LX(__func__));

    if (!setState(State::AUTHORIZING, ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST)) {
        ACSDK_ERROR(LX("connectFailed").d("reason", "setStateFailed"));
        return false;
    }

    m_thread = std::thread(&HTTP2Transport::mainLoop, this);

    return true;
}

void HTTP2Transport::disconnect() {
    ACSDK_DEBUG5(LX(__func__));

    std::thread localThread;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (State::SHUTDOWN != m_state) {
            setStateLocked(State::DISCONNECTING, ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);
        }
        std::swap(m_thread, localThread);
    }

    if (localThread.joinable()) {
        localThread.join();
    }
}

bool HTTP2Transport::isConnected() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return State::CONNECTED == m_state;
}

void HTTP2Transport::send(std::shared_ptr<MessageRequest> request) {
    ACSDK_DEBUG5(LX(__func__));
    enqueueRequest(request, false);
}

void HTTP2Transport::sendPostConnectMessage(std::shared_ptr<MessageRequest> request) {
    ACSDK_DEBUG5(LX(__func__));
    enqueueRequest(request, true);
}

void HTTP2Transport::onPostConnected() {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);

    m_postConnect.reset();
    switch (m_state) {
        case State::INIT:
        case State::AUTHORIZING:
        case State::CONNECTING:
        case State::WAITING_TO_RETRY_CONNECTING:
            m_postConnected = true;
            break;
        case State::CONNECTED:
            ACSDK_ERROR(LX("onPostConnectFailed").d("reason", "unexpectedState"));
            break;
        case State::POST_CONNECTING:
            m_postConnected = true;
            if (!setStateLocked(State::CONNECTED, ConnectionStatusObserverInterface::ChangedReason::SUCCESS)) {
                ACSDK_ERROR(LX("onPostConnectFailed").d("reason", "setState(CONNECTED)Failed"));
            }
            break;
        case State::SERVER_SIDE_DISCONNECT:
        case State::DISCONNECTING:
        case State::SHUTDOWN:
            break;
    }
}

void HTTP2Transport::onAuthStateChange(
    avsCommon::sdkInterfaces::AuthObserverInterface::State newState,
    avsCommon::sdkInterfaces::AuthObserverInterface::Error error) {
    ACSDK_DEBUG5(LX(__func__).d("newState", newState).d("error", error));

    std::lock_guard<std::mutex> lock(m_mutex);

    switch (newState) {
        case AuthObserverInterface::State::UNINITIALIZED:
        case AuthObserverInterface::State::EXPIRED:
            if (State::WAITING_TO_RETRY_CONNECTING == m_state) {
                ACSDK_DEBUG0(LX("revertToAuthorizing").d("reason", "authorizationExpiredBeforeConnected"));
                setStateLocked(State::AUTHORIZING, ConnectionStatusObserverInterface::ChangedReason::INVALID_AUTH);
            }
            return;

        case AuthObserverInterface::State::REFRESHED:
            if (State::AUTHORIZING == m_state) {
                setStateLocked(State::CONNECTING, ConnectionStatusObserverInterface::ChangedReason::SUCCESS);
            }
            return;

        case AuthObserverInterface::State::UNRECOVERABLE_ERROR:
            ACSDK_ERROR(LX("shuttingDown").d("reason", "unrecoverableAuthError"));
            setStateLocked(State::SHUTDOWN, ConnectionStatusObserverInterface::ChangedReason::UNRECOVERABLE_ERROR);
            return;
    }

    ACSDK_ERROR(LX("shuttingDown").d("reason", "unknownAuthStatus").d("newState", static_cast<int>(newState)));
    setStateLocked(State::SHUTDOWN, ConnectionStatusObserverInterface::ChangedReason::UNRECOVERABLE_ERROR);
}

void HTTP2Transport::doShutdown() {
    ACSDK_DEBUG5(LX(__func__));
    setState(State::SHUTDOWN, ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);
    disconnect();
    m_authDelegate->removeAuthObserver(shared_from_this());
    m_pingHandler.reset();
    m_http2Connection.reset();
    m_messageConsumer.reset();
    m_attachmentManager.reset();
    m_postConnectFactory.reset();
    m_postConnect.reset();
    m_observers.clear();
}

void HTTP2Transport::onDownchannelConnected() {
    ACSDK_DEBUG5(LX(__func__));
    setState(State::POST_CONNECTING, ConnectionStatusObserverInterface::ChangedReason::SUCCESS);
}

void HTTP2Transport::onDownchannelFinished() {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);

    switch (m_state) {
        case State::INIT:
        case State::AUTHORIZING:
        case State::WAITING_TO_RETRY_CONNECTING:
            ACSDK_ERROR(LX("onDownchannelFinishedFailed").d("reason", "unexpectedState"));
            break;
        case State::CONNECTING:
            setStateLocked(State::WAITING_TO_RETRY_CONNECTING, ConnectionStatusObserverInterface::ChangedReason::NONE);
            break;
        case State::POST_CONNECTING:
        case State::CONNECTED:
            setStateLocked(
                State::SERVER_SIDE_DISCONNECT,
                ConnectionStatusObserverInterface::ChangedReason::SERVER_SIDE_DISCONNECT);
            break;
        case State::SERVER_SIDE_DISCONNECT:
        case State::DISCONNECTING:
        case State::SHUTDOWN:
            break;
    }
}

void HTTP2Transport::onMessageRequestSent() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isMessageHandlerAwaitingResponse = true;
    m_countOfUnfinishedMessageHandlers++;
    ACSDK_DEBUG5(LX(__func__).d("countOfUnfinishedMessageHandlers", m_countOfUnfinishedMessageHandlers));
}

void HTTP2Transport::onMessageRequestTimeout() {
    // If a message request times out, trigger a ping to test connectivity to AVS.
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_pingHandler) {
        m_timeOfLastActivity = m_timeOfLastActivity.min();
        m_wakeEvent.notify_all();
    }
}

void HTTP2Transport::onMessageRequestAcknowledged() {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isMessageHandlerAwaitingResponse = false;
    m_wakeEvent.notify_all();
}

void HTTP2Transport::onMessageRequestFinished() {
    std::lock_guard<std::mutex> lock(m_mutex);
    --m_countOfUnfinishedMessageHandlers;
    ACSDK_DEBUG5(LX(__func__).d("countOfUnfinishedMessageHandlers", m_countOfUnfinishedMessageHandlers));
    m_wakeEvent.notify_all();
}

void HTTP2Transport::onPingRequestAcknowledged(bool success) {
    ACSDK_DEBUG5(LX(__func__).d("success", success));
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pingHandler.reset();
    if (!success) {
        setStateLocked(
            State::SERVER_SIDE_DISCONNECT, ConnectionStatusObserverInterface::ChangedReason::SERVER_SIDE_DISCONNECT);
    }
    m_wakeEvent.notify_all();
}

void HTTP2Transport::onPingTimeout() {
    ACSDK_WARN(LX(__func__));
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pingHandler.reset();
    setStateLocked(State::SHUTDOWN, ConnectionStatusObserverInterface::ChangedReason::PING_TIMEDOUT);
    m_wakeEvent.notify_all();
}

void HTTP2Transport::onActivity() {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock(m_mutex);
    m_timeOfLastActivity = std::chrono::steady_clock::now();
}

void HTTP2Transport::onForbidden(const std::string& authToken) {
    ACSDK_DEBUG0(LX(__func__));
    m_authDelegate->onAuthFailure(authToken);
}

std::shared_ptr<HTTP2RequestInterface> HTTP2Transport::createAndSendRequest(const HTTP2RequestConfig& cfg) {
    ACSDK_DEBUG5(LX(__func__).d("type", cfg.getRequestType()).sensitive("url", cfg.getUrl()));
    return m_http2Connection->createAndSendRequest(cfg);
}

std::string HTTP2Transport::getEndpoint() {
    return m_avsEndpoint;
}

void HTTP2Transport::mainLoop() {
    ACSDK_DEBUG5(LX(__func__));

    m_postConnect = m_postConnectFactory->createPostConnect();
    if (!m_postConnect || !m_postConnect->doPostConnect(shared_from_this())) {
        ACSDK_ERROR(LX("mainLoopFailed").d("reason", "createPostConnectFailed"));
        std::lock_guard<std::mutex> lock(m_mutex);
        setStateLocked(State::SHUTDOWN, ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
    }

    m_timeOfLastActivity = std::chrono::steady_clock::now();
    State nextState = getState();

    while (nextState != State::SHUTDOWN) {
        switch (nextState) {
            case State::INIT:
                nextState = handleInit();
                break;
            case State::AUTHORIZING:
                nextState = handleAuthorizing();
                break;
            case State::CONNECTING:
                nextState = handleConnecting();
                break;
            case State::WAITING_TO_RETRY_CONNECTING:
                nextState = handleWaitingToRetryConnecting();
                break;
            case State::POST_CONNECTING:
                nextState = handlePostConnecting();
                break;
            case State::CONNECTED:
                nextState = handleConnected();
                break;
            case State::SERVER_SIDE_DISCONNECT:
                nextState = handleServerSideDisconnect();
            case State::DISCONNECTING:
                nextState = handleDisconnecting();
                break;
            case State::SHUTDOWN:
                break;
        }
    }

    handleShutdown();

    ACSDK_DEBUG5(LX("mainLoopExiting"));
}

HTTP2Transport::State HTTP2Transport::handleInit() {
    ACSDK_CRITICAL(LX(__func__).d("reason", "unexpectedState"));
    std::lock_guard<std::mutex> lock(m_mutex);
    setStateLocked(State::SHUTDOWN, ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
    return m_state;
}

HTTP2Transport::State HTTP2Transport::handleAuthorizing() {
    ACSDK_DEBUG5(LX(__func__));

    m_authDelegate->addAuthObserver(shared_from_this());

    std::unique_lock<std::mutex> lock(m_mutex);
    m_wakeEvent.wait(lock, [this]() { return m_state != State::AUTHORIZING; });
    return m_state;
}

HTTP2Transport::State HTTP2Transport::handleConnecting() {
    ACSDK_DEBUG5(LX(__func__));

    std::unique_lock<std::mutex> lock(m_mutex);

    while (State::CONNECTING == m_state) {
        lock.unlock();

        auto authToken = m_authDelegate->getAuthToken();

        if (authToken.empty()) {
            setState(
                State::WAITING_TO_RETRY_CONNECTING, ConnectionStatusObserverInterface::ChangedReason::INVALID_AUTH);
            break;
        }

        auto downchannelHandler =
            DownchannelHandler::create(shared_from_this(), authToken, m_messageConsumer, m_attachmentManager);
        lock.lock();

        if (!downchannelHandler) {
            ACSDK_ERROR(LX("handleConnectingFailed").d("reason", "createDownchannelHandlerFailed"));
            setStateLocked(
                State::WAITING_TO_RETRY_CONNECTING, ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
            return m_state;
        }

        while (State::CONNECTING == m_state) {
            m_wakeEvent.wait(lock);
        }
    }

    return m_state;
}

HTTP2Transport::State HTTP2Transport::handleWaitingToRetryConnecting() {
    ACSDK_DEBUG5(LX(__func__));

    std::chrono::milliseconds timeout = TransportDefines::RETRY_TIMER.calculateTimeToRetry(m_connectRetryCount);
    ACSDK_DEBUG5(
        LX("handleConnectingWaitingToRetry").d("connectRetryCount", m_connectRetryCount).d("timeout", timeout.count()));
    m_connectRetryCount++;
    std::unique_lock<std::mutex> lock(m_mutex);
    m_wakeEvent.wait_for(lock, timeout, [this] { return m_state != State::WAITING_TO_RETRY_CONNECTING; });
    if (State::WAITING_TO_RETRY_CONNECTING == m_state) {
        setStateLocked(State::CONNECTING, ConnectionStatusObserverInterface::ChangedReason::NONE);
    }
    return m_state;
}

HTTP2Transport::State HTTP2Transport::handlePostConnecting() {
    ACSDK_DEBUG5(LX(__func__));
    if (m_postConnected) {
        setState(State::CONNECTED, ConnectionStatusObserverInterface::ChangedReason::SUCCESS);
        return State::CONNECTED;
    }
    return sendMessagesAndPings(State::POST_CONNECTING);
}

HTTP2Transport::State HTTP2Transport::handleConnected() {
    ACSDK_DEBUG5(LX(__func__));
    notifyObserversOnConnected();
    return sendMessagesAndPings(State::CONNECTED);
}

HTTP2Transport::State HTTP2Transport::handleServerSideDisconnect() {
    ACSDK_DEBUG5(LX(__func__));
    notifyObserversOnServerSideDisconnect();
    return State::DISCONNECTING;
}

HTTP2Transport::State HTTP2Transport::handleDisconnecting() {
    ACSDK_DEBUG5(LX(__func__));

    std::unique_lock<std::mutex> lock(m_mutex);
    while (State::DISCONNECTING == m_state && m_countOfUnfinishedMessageHandlers > 0) {
        m_wakeEvent.wait(lock);
    }
    setStateLocked(State::SHUTDOWN, ConnectionStatusObserverInterface::ChangedReason::SUCCESS);
    return m_state;
}

HTTP2Transport::State HTTP2Transport::handleShutdown() {
    ACSDK_DEBUG5(LX(__func__));

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto request : m_requestQueue) {
            request->sendCompleted(MessageRequestObserverInterface::Status::NOT_CONNECTED);
        }
        m_requestQueue.clear();
    }

    m_http2Connection->disconnect();

    notifyObserversOnDisconnect(m_disconnectReason);

    return State::SHUTDOWN;
}

void HTTP2Transport::enqueueRequest(std::shared_ptr<avsCommon::avs::MessageRequest> request, bool beforeConnected) {
    ACSDK_DEBUG5(LX(__func__).d("beforeConnected", beforeConnected));

    if (!request) {
        ACSDK_ERROR(LX("enqueueRequestFailed").d("reason", "nullRequest"));
    }
    std::unique_lock<std::mutex> lock(m_mutex);
    bool allowed = false;
    switch (m_state) {
        case State::INIT:
        case State::AUTHORIZING:
        case State::CONNECTING:
        case State::WAITING_TO_RETRY_CONNECTING:
        case State::POST_CONNECTING:
            allowed = beforeConnected;
            break;
        case State::CONNECTED:
            allowed = !beforeConnected;
            break;
        case State::SERVER_SIDE_DISCONNECT:
        case State::DISCONNECTING:
        case State::SHUTDOWN:
            allowed = false;
            break;
    }

    if (allowed) {
        m_requestQueue.push_back(request);
        m_wakeEvent.notify_all();
    } else {
        lock.unlock();
        ACSDK_ERROR(LX("enqueueRequestFailed").d("reason", "notInAllowedState").d("m_state", m_state));
        request->sendCompleted(MessageRequestObserverInterface::Status::NOT_CONNECTED);
    }
}

HTTP2Transport::State HTTP2Transport::sendMessagesAndPings(alexaClientSDK::acl::HTTP2Transport::State whileState) {
    ACSDK_DEBUG5(LX(__func__).d("whileState", whileState));

    std::unique_lock<std::mutex> lock(m_mutex);

    auto canSendMessage = [this] {
        return (
            !m_isMessageHandlerAwaitingResponse && !m_requestQueue.empty() &&
            (m_countOfUnfinishedMessageHandlers < MAX_MESSAGE_HANDLERS));
    };

    auto wakePredicate = [this, whileState, canSendMessage] {
        return whileState != m_state || canSendMessage() ||
               (std::chrono::steady_clock::now() > m_timeOfLastActivity + m_configuration.inactivityTimeout);
    };

    auto pingWakePredicate = [this, whileState, canSendMessage] {
        return whileState != m_state || !m_pingHandler || canSendMessage();
    };

    while (true) {
        if (m_pingHandler) {
            m_wakeEvent.wait(lock, pingWakePredicate);
        } else {
            m_wakeEvent.wait_until(lock, m_timeOfLastActivity + m_configuration.inactivityTimeout, wakePredicate);
        }

        if (m_state != whileState) {
            break;
        }

        if (canSendMessage()) {
            auto messageRequest = m_requestQueue.front();
            m_requestQueue.pop_front();

            lock.unlock();

            auto authToken = m_authDelegate->getAuthToken();
            if (!authToken.empty()) {
                auto handler = MessageRequestHandler::create(
                    shared_from_this(), authToken, messageRequest, m_messageConsumer, m_attachmentManager);
                if (!handler) {
                    messageRequest->sendCompleted(MessageRequestObserverInterface::Status::INTERNAL_ERROR);
                }
            } else {
                ACSDK_ERROR(LX("failedToCreateMessageHandler").d("reason", "invalidAuth"));
                messageRequest->sendCompleted(MessageRequestObserverInterface::Status::INVALID_AUTH);
            }

            lock.lock();

        } else if (std::chrono::steady_clock::now() > m_timeOfLastActivity + m_configuration.inactivityTimeout) {
            if (!m_pingHandler) {
                lock.unlock();

                auto authToken = m_authDelegate->getAuthToken();
                if (!authToken.empty()) {
                    m_pingHandler = PingHandler::create(shared_from_this(), authToken);
                } else {
                    ACSDK_ERROR(LX("failedToCreatePingHandler").d("reason", "invalidAuth"));
                }
                if (!m_pingHandler) {
                    ACSDK_ERROR(LX("shutDown").d("reason", "failedToCreatePingHandler"));
                    setState(State::SHUTDOWN, ConnectionStatusObserverInterface::ChangedReason::PING_TIMEDOUT);
                }

                lock.lock();
            } else {
                ACSDK_DEBUG5(LX("m_pingHandler != nullptr"));
            }
        }
    }

    return m_state;
}

bool HTTP2Transport::setState(State newState, ConnectionStatusObserverInterface::ChangedReason changedReason) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return setStateLocked(newState, changedReason);
}

bool HTTP2Transport::setStateLocked(State newState, ConnectionStatusObserverInterface::ChangedReason changedReason) {
    ACSDK_DEBUG5(LX(__func__).d("newState", newState).d("changedReason", changedReason));

    if (newState == m_state) {
        ACSDK_DEBUG5(LX("alreadyInNewState"));
        return true;
    }

    bool allowed = false;
    switch (newState) {
        case State::INIT:
            allowed = false;
            break;
        case State::AUTHORIZING:
            allowed = State::INIT == m_state || State::WAITING_TO_RETRY_CONNECTING == m_state;
            break;
        case State::CONNECTING:
            allowed = State::AUTHORIZING == m_state || State::WAITING_TO_RETRY_CONNECTING == m_state;
            break;
        case State::WAITING_TO_RETRY_CONNECTING:
            allowed = State::CONNECTING == m_state;
            break;
        case State::POST_CONNECTING:
            allowed = State::CONNECTING == m_state;
            break;
        case State::CONNECTED:
            allowed = State::POST_CONNECTING == m_state;
            break;
        case State::SERVER_SIDE_DISCONNECT:
            allowed = m_state != State::DISCONNECTING && m_state != State::SHUTDOWN;
            break;
        case State::DISCONNECTING:
            allowed = m_state != State::SHUTDOWN;
            break;
        case State::SHUTDOWN:
            allowed = true;
            break;
    }

    if (!allowed) {
        ACSDK_ERROR(LX("stateChangeNotAllowed").d("oldState", m_state).d("newState", newState));
        return false;
    }

    switch (newState) {
        case State::INIT:
        case State::AUTHORIZING:
        case State::CONNECTING:
        case State::WAITING_TO_RETRY_CONNECTING:
        case State::POST_CONNECTING:
        case State::CONNECTED:
            break;

        case State::SERVER_SIDE_DISCONNECT:
        case State::DISCONNECTING:
        case State::SHUTDOWN:
            if (ConnectionStatusObserverInterface::ChangedReason::NONE == m_disconnectReason) {
                m_disconnectReason = changedReason;
            }
            break;
    }

    m_state = newState;
    m_wakeEvent.notify_all();

    return true;
}

void HTTP2Transport::notifyObserversOnConnected() {
    ACSDK_DEBUG5(LX(__func__));

    std::unique_lock<std::mutex> lock{m_observerMutex};
    auto observers = m_observers;
    lock.unlock();

    for (const auto& observer : observers) {
        observer->onConnected(shared_from_this());
    }
}

void HTTP2Transport::notifyObserversOnDisconnect(ConnectionStatusObserverInterface::ChangedReason reason) {
    ACSDK_DEBUG5(LX(__func__));

    if (m_postConnect) {
        m_postConnect->onDisconnect();
        m_postConnect.reset();
    }

    std::unique_lock<std::mutex> lock{m_observerMutex};
    auto observers = m_observers;
    lock.unlock();

    for (const auto& observer : observers) {
        observer->onDisconnected(shared_from_this(), reason);
    }
}

void HTTP2Transport::notifyObserversOnServerSideDisconnect() {
    ACSDK_DEBUG5(LX(__func__));

    if (m_postConnect) {
        m_postConnect->onDisconnect();
        m_postConnect.reset();
    }

    std::unique_lock<std::mutex> lock{m_observerMutex};
    auto observers = m_observers;
    lock.unlock();

    for (const auto& observer : observers) {
        observer->onServerSideDisconnect(shared_from_this());
    }
}

HTTP2Transport::State HTTP2Transport::getState() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_state;
}

}  // namespace acl
}  // namespace alexaClientSDK
