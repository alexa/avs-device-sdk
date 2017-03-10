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

#include "ACL/Values.h"
#include "ACL/Transport/MessageRouter.h"
#include "AVSUtils/Logging/Logger.h"
#include "AVSUtils/Memory/Memory.h"
#include "AVSUtils/Threading/Executor.h"

namespace alexaClientSDK {
namespace acl {

using namespace alexaClientSDK::avsUtils;

MessageRouter::MessageRouter(
        std::shared_ptr<AuthDelegateInterface> authDelegate,
        const std::string& avsEndpoint,
        std::shared_ptr<avsUtils::threading::Executor> sendExecutor,
        std::shared_ptr<avsUtils::threading::Executor> receiveExecutor):
    m_avsEndpoint{avsEndpoint},
    m_authDelegate{authDelegate},
    m_connectionStatus{ConnectionStatus::DISCONNECTED},
    m_isEnabled{false},
    m_sendExecutor{sendExecutor},
    m_receiveExecutor{receiveExecutor} {
}

MessageRouter::~MessageRouter() {
    disable();
}

ConnectionStatus MessageRouter::getConnectionStatus() {
    std::lock_guard<std::recursive_mutex> lock{m_connectionMutex};
    return m_connectionStatus;
}

void MessageRouter::enable() {
    std::lock_guard<std::recursive_mutex> lock{m_connectionMutex};

    m_isEnabled = true;

    if (m_activeTransport && m_activeTransport->isConnected()) {
        return;
    } else {
        m_connectionStatus = ConnectionStatus::PENDING;
        notifyObserverOnConnectionStatusChanged(ConnectionStatus::PENDING, ConnectionChangedReason::ACL_CLIENT_REQUEST);
        createActiveTransportLocked();
    }
}

void MessageRouter::disable() {
    std::lock_guard<std::recursive_mutex> lock{m_connectionMutex};
    m_isEnabled = false;

    disconnectAllTransportsLocked(ConnectionChangedReason::ACL_CLIENT_REQUEST);
}

void MessageRouter::send(std::shared_ptr<MessageRequest> request) {
    auto task = [this, request]() {
        if (m_activeTransport && m_activeTransport->isConnected()) {
            m_activeTransport->send(request);
        }
    };
    m_sendExecutor->submit(task);
}

void MessageRouter::setAVSEndpoint(const std::string &avsEndpoint) {
    std::lock_guard<std::recursive_mutex> lock{m_connectionMutex};
    if (avsEndpoint != m_avsEndpoint) {
        m_avsEndpoint = avsEndpoint;

        if(m_isEnabled) {
            disconnectAllTransportsLocked(ConnectionChangedReason::SERVER_ENDPOINT_CHANGED);

            createActiveTransportLocked();
        }
    }
}

void MessageRouter::onConnected() {
    std::lock_guard<std::recursive_mutex> lock{m_connectionMutex};
    if (ConnectionStatus::CONNECTED != m_connectionStatus) {
        m_connectionStatus = ConnectionStatus::CONNECTED;
        notifyObserverOnConnectionStatusChanged(m_connectionStatus, ConnectionChangedReason::ACL_CLIENT_REQUEST);
    }
}

void MessageRouter::onDisconnected(ConnectionChangedReason reason) {
    std::lock_guard<std::recursive_mutex> lock{m_connectionMutex};
    if (ConnectionStatus::CONNECTED == m_connectionStatus) {
        auto isDisconnected = [](std::shared_ptr<TransportInterface> transport) { return !transport->isConnected(); };

        m_transports.erase(
                std::remove_if(m_transports.begin(), m_transports.end(), isDisconnected), m_transports.end());

        if (m_transports.empty()) {
            disconnectAllTransportsLocked(reason);
        }
    }
}

void MessageRouter::onServerSideDisconnect() {
    std::lock_guard<std::recursive_mutex> lock{m_connectionMutex};
    if (m_isEnabled) {
        m_connectionStatus = ConnectionStatus::PENDING;
        notifyObserverOnConnectionStatusChanged(
                m_connectionStatus,
                ConnectionChangedReason::SERVER_SIDE_DISCONNECT);
        // TODO: ACSDK-100 There is a race condition to fix with sending a message on the new transport when the old one
        // is still waiting for responses for sent events.
        createActiveTransportLocked();
    }
}

void MessageRouter::consumeMessage(std::shared_ptr<Message> message) {
    notifyObserverOnReceive(message);
}

void MessageRouter::setObserver(std::shared_ptr<MessageRouterObserverInterface> observer) {
    m_observer = observer;
}

void MessageRouter::notifyObserverOnConnectionStatusChanged(
        const ConnectionStatus status,
        const ConnectionChangedReason reason) {
    auto task = [this, status, reason]() {
        if (m_observer) {
            m_observer->onConnectionStatusChanged(status, reason);
        }
    };

    m_receiveExecutor->submit(task);
}

void MessageRouter::notifyObserverOnReceive(std::shared_ptr<Message> msg) {
    auto task = [this, msg]() {
        if (m_observer) {
            m_observer->receive(msg);
        }
    };

    m_receiveExecutor->submit(task);
}

void MessageRouter::createActiveTransportLocked() {
    auto transport = createTransport(m_authDelegate, m_avsEndpoint, this, this);
    if (transport->connect()) {
        m_transports.push_back(transport);
        m_activeTransport = transport;
    } else {
        Logger::log("Could not create connection to AVS backend");
    }
}

void MessageRouter::disconnectAllTransportsLocked(const ConnectionChangedReason reason) {
    for (auto transport : m_transports) {
        transport->disconnect();
    }

    m_activeTransport.reset();

    m_transports.clear();

    m_connectionStatus = ConnectionStatus::DISCONNECTED;
    notifyObserverOnConnectionStatusChanged(m_connectionStatus, reason);
}

} // acl
} // alexaClientSDK
