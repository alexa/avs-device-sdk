/*
 * MessageRouter.cpp
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include <curl/curl.h>

#include <AVSUtils/Logger/LogEntry.h>
#include <AVSUtils/Logging/Logger.h>
#include <AVSUtils/Memory/Memory.h>
#include <AVSUtils/Threading/Executor.h>

#include "ACL/EnumUtils.h"
#include "ACL/Transport/MessageRouter.h"

namespace alexaClientSDK {
namespace acl {

using namespace alexaClientSDK::avsUtils;

/// String to identify log entries originating from this file.
static const std::string TAG("MessageRouter");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsUtils::logger::LogEntry(TAG, event)

MessageRouter::MessageRouter(
        std::shared_ptr<AuthDelegateInterface> authDelegate,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> attachmentManager,
        const std::string& avsEndpoint,
        std::shared_ptr<avsUtils::threading::Executor> sendExecutor,
        std::shared_ptr<avsUtils::threading::Executor> receiveExecutor):
    m_avsEndpoint{avsEndpoint},
    m_authDelegate{authDelegate},
    m_connectionStatus{ConnectionStatusObserverInterface::Status::DISCONNECTED},
    m_isEnabled{false},
    m_sendExecutor{sendExecutor},
    m_receiveExecutor{receiveExecutor},
    m_attachmentManager{attachmentManager} {
}

MessageRouter::~MessageRouter() {
    disable();
    m_receiveExecutor->waitForSubmittedTasks();
    m_sendExecutor->waitForSubmittedTasks();
}

ConnectionStatusObserverInterface::Status MessageRouter::getConnectionStatus() {
    std::lock_guard<std::mutex> lock{m_connectionMutex};
    return m_connectionStatus;
}

void MessageRouter::enable() {
    std::lock_guard<std::mutex> lock{m_connectionMutex};
    m_isEnabled = true;
    if (!m_activeTransport || !m_activeTransport->isConnected()) {
        setConnectionStatusLocked(ConnectionStatusObserverInterface::Status::PENDING, 
                                  ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);
        createActiveTransportLocked();
    }
}

void MessageRouter::disable() {
    std::unique_lock<std::mutex> lock{m_connectionMutex};
    m_isEnabled = false;
    disconnectAllTransportsLocked(lock, ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);
}

void MessageRouter::send(std::shared_ptr<avsCommon::avs::MessageRequest> request) {
    auto task = [this, request]() {
        std::shared_ptr<TransportInterface> transport;
        {
            std::lock_guard<std::mutex> lock{m_connectionMutex};
            transport = m_activeTransport;
        }
        if (transport && transport->isConnected()) {
            transport->send(request);
        } else {
            request->onSendCompleted(avsCommon::avs::MessageRequest::Status::NOT_CONNECTED);
        }
    };
    m_sendExecutor->submit(task);
}

void MessageRouter::setAVSEndpoint(const std::string &avsEndpoint) {
    std::unique_lock<std::mutex> lock{m_connectionMutex};
    if (avsEndpoint != m_avsEndpoint) {
        m_avsEndpoint = avsEndpoint;
        if (m_isEnabled) {
            disconnectAllTransportsLocked(lock, 
                ConnectionStatusObserverInterface::ChangedReason::SERVER_ENDPOINT_CHANGED);
        }
        // disconnectAllTransportLocked releases the lock temporarily, so re-check m_isEnabled.
        if (m_isEnabled) {
            createActiveTransportLocked();
        }
    }
}

void MessageRouter::onConnected() {
    std::unique_lock<std::mutex> lock{m_connectionMutex};
    if (m_isEnabled) {
        setConnectionStatusLocked(ConnectionStatusObserverInterface::Status::CONNECTED, 
                                  ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);
    }
}

void MessageRouter::onDisconnected(ConnectionStatusObserverInterface::ChangedReason reason) {
    std::lock_guard<std::mutex> lock{m_connectionMutex};
    if (ConnectionStatusObserverInterface::Status::CONNECTED == m_connectionStatus) {
        if (m_activeTransport && !m_activeTransport->isConnected()) {
            m_activeTransport.reset();
        }
        auto isDisconnected = [](std::shared_ptr<TransportInterface> transport) { return !transport->isConnected(); };
        m_transports.erase(
                std::remove_if(m_transports.begin(), m_transports.end(), isDisconnected), m_transports.end());
        if (m_transports.empty()) {
            setConnectionStatusLocked(ConnectionStatusObserverInterface::Status::DISCONNECTED, reason);
        }
    }
}

void MessageRouter::onServerSideDisconnect() {
    std::unique_lock<std::mutex> lock{m_connectionMutex};
    if (m_isEnabled) {
        setConnectionStatusLocked(ConnectionStatusObserverInterface::Status::PENDING, 
                                  ConnectionStatusObserverInterface::ChangedReason::SERVER_SIDE_DISCONNECT);
        // For server side disconnects leave the old transport alive to receive any further data, but send
        // new messages through a new transport.
        // @see: https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/docs/managing-an-http-2-connection#disconnects
        createActiveTransportLocked();
    }
}

void MessageRouter::consumeMessage(const std::string & contextId, const std::string & message) {
    notifyObserverOnReceive(contextId, message);
}

void MessageRouter::setObserver(std::shared_ptr<MessageRouterObserverInterface> observer) {
    std::lock_guard<std::mutex> lock{m_connectionMutex};
    m_observer = observer;
}

void MessageRouter::setConnectionStatusLocked(const ConnectionStatusObserverInterface::Status status, 
                                              const ConnectionStatusObserverInterface::ChangedReason reason) {
    if (status != m_connectionStatus) {
        m_connectionStatus = status;
        ACSDK_DEBUG(LX("connectionStatusChanged").d("reason", reason).d("newStatus", m_connectionStatus));
        notifyObserverOnConnectionStatusChanged(m_connectionStatus, reason);
    }
}

void MessageRouter::notifyObserverOnConnectionStatusChanged(
        const ConnectionStatusObserverInterface::Status status,
        const ConnectionStatusObserverInterface::ChangedReason reason) {
    auto task = [this, status, reason]() {
        auto observer = getObserver();
        if (observer) {
            observer->onConnectionStatusChanged(status, reason);
        }
    };
    m_receiveExecutor->submit(task);
}

void MessageRouter::notifyObserverOnReceive(const std::string & contextId, const std::string & message) {
    auto task = [this, contextId, message]() {
        auto temp = getObserver();
        if (temp) {
            temp->receive(contextId, message);
        }
    };
    m_receiveExecutor->submit(task);
}

void MessageRouter::createActiveTransportLocked() {
    auto transport = createTransport(m_authDelegate, m_attachmentManager, m_avsEndpoint, this, this);
    if (transport && transport->connect()) {
        m_transports.push_back(transport);
        m_activeTransport = transport;
    } else {
        m_activeTransport.reset();
        setConnectionStatusLocked(ConnectionStatusObserverInterface::Status::DISCONNECTED, 
                                  ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR);
        ACSDK_ERROR(LX("createActiveTransportLockedFailed")
                .d("reason", transport ? "internalError" : "createTransportFailed"));
    }
}

void MessageRouter::disconnectAllTransportsLocked(
        std::unique_lock<std::mutex>& lock, const ConnectionStatusObserverInterface::ChangedReason reason) {

    // Use std::move() to optimize copy. Use clear() otherwise contents of m_transports becomes undefined.
    auto movedTransports = std::move(m_transports);
    m_transports.clear();
    m_activeTransport.reset();
    setConnectionStatusLocked(ConnectionStatusObserverInterface::Status::DISCONNECTED, reason);

    lock.unlock();
    for (auto transport : movedTransports) {
        transport->disconnect();
    }
    lock.lock();
}

std::shared_ptr<MessageRouterObserverInterface> MessageRouter::getObserver() {
    std::lock_guard<std::mutex> lock{m_connectionMutex};
    return m_observer;
}

} // acl
} // alexaClientSDK
