/*
* AVSConnectionManager.cpp
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

#include "AVSUtils/Logging/Logger.h"
#include "AVSUtils/Initialization/AlexaClientSDKInit.h"
#include "AVSUtils/Memory/Memory.h"

#include "ACL/AVSConnectionManager.h"
#include "ACL/Transport/MessageRouterInterface.h"

namespace alexaClientSDK {
namespace acl {

std::shared_ptr<AVSConnectionManager>

AVSConnectionManager::create(std::shared_ptr<MessageRouterInterface> messageRouter,
                             bool isEnabled,
                             std::shared_ptr<ConnectionStatusObserverInterface> connectionStatusObserver,
                             std::shared_ptr<MessageObserverInterface> messageObserver) {
    if (!avsUtils::initialization::AlexaClientSDKInit::isInitialized()) {
        avsUtils::Logger::log("Alexa Client SDK not initialized, returning nullptr");
        return nullptr;
    }

    auto connectionManager = std::shared_ptr<AVSConnectionManager>(
            new AVSConnectionManager(messageRouter, connectionStatusObserver, messageObserver));

    if(messageRouter) {
        messageRouter->setObserver(connectionManager);
    }

    if(isEnabled) {
        connectionManager->enable();
    }

    return connectionManager;
}

AVSConnectionManager::AVSConnectionManager(std::shared_ptr<MessageRouterInterface> messageRouter,
                                           std::shared_ptr<ConnectionStatusObserverInterface> connectionStatusObserver,
                                           std::shared_ptr<MessageObserverInterface> messageObserver)
        : m_isEnabled{false},
          m_connectionStatusObserver{connectionStatusObserver},
          m_messageObserver{messageObserver},
          m_messageRouter{messageRouter} {
}

void AVSConnectionManager::enable() {
    m_isEnabled = true;
    m_messageRouter->enable();
}

void AVSConnectionManager::disable() {
    m_isEnabled = false;
    m_messageRouter->disable();
}

bool AVSConnectionManager::isEnabled() {
    return m_isEnabled;
}

void AVSConnectionManager::reconnect() {
    if(m_isEnabled) {
        m_messageRouter->disable();
        m_messageRouter->enable();
    }
}

void AVSConnectionManager::sendMessage(std::shared_ptr<avsCommon::avs::MessageRequest> request) {
    m_messageRouter->send(request);
}

void AVSConnectionManager::setAVSEndpoint(const std::string& avsEndpoint) {
    m_messageRouter->setAVSEndpoint(avsEndpoint);
}

void AVSConnectionManager::onConnectionStatusChanged(const ConnectionStatus status,
                                                     const ConnectionChangedReason reason) {
    if(m_connectionStatusObserver) {
        m_connectionStatusObserver->onConnectionStatusChanged(status, reason);
    }
}

void AVSConnectionManager::receive(std::shared_ptr<avsCommon::avs::Message> msg) {
    if(m_messageObserver) {
        m_messageObserver->receive(msg);
    }
}

} // namespace acl
} // namespace alexaClientSDK
