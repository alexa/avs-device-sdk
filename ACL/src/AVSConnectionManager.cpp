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

#include "AVSCommon/Utils/Logger/Logger.h"
#include "AVSCommon/AVS/Initialization/AlexaClientSDKInit.h"
#include "AVSCommon/Utils/Memory/Memory.h"

#include "ACL/AVSConnectionManager.h"
#include "ACL/Transport/MessageRouterInterface.h"

namespace alexaClientSDK {
namespace acl {

using namespace alexaClientSDK::avsCommon::sdkInterfaces;

/// String to identify log entries originating from this file.
static const std::string TAG("AVSConnectionManager");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<AVSConnectionManager>
AVSConnectionManager::create(std::shared_ptr<MessageRouterInterface> messageRouter,
        bool isEnabled,
        std::unordered_set<std::shared_ptr<ConnectionStatusObserverInterface>> connectionStatusObservers,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface>> messageObservers) {
    if (!avsCommon::avs::initialization::AlexaClientSDKInit::isInitialized()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "uninitialziedAlexaClientSdk").d("return", "nullptr"));
        return nullptr;
    }

    if (!messageRouter) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMessageRouter").d("return", "nullptr"));
        return nullptr;
    }

    for (auto observer : connectionStatusObservers) {
        if (!observer) {
            ACSDK_ERROR(LX("createFailed").d("reason", "nullConnectionStatusObserver").d("return", "nullptr"));
            return nullptr;
        }
    }

    for(auto observer : messageObservers) {
        if (!observer) {
            ACSDK_ERROR(LX("createFailed").d("reason", "nullMessageObserver").d("return", "nullptr"));
            return nullptr;
        }
    }

    auto connectionManager = std::shared_ptr<AVSConnectionManager>(
            new AVSConnectionManager(messageRouter, connectionStatusObservers, messageObservers));

    messageRouter->setObserver(connectionManager);

    if (isEnabled) {
        connectionManager->enable();
    }

    return connectionManager;
}

AVSConnectionManager::AVSConnectionManager(
        std::shared_ptr<MessageRouterInterface> messageRouter,
        std::unordered_set<std::shared_ptr<ConnectionStatusObserverInterface>> connectionStatusObservers,
        std::unordered_set<std::shared_ptr<MessageObserverInterface>> messageObservers)
        : m_isEnabled{false},
          m_isSynchronized{false},
          m_connectionStatusObservers{connectionStatusObservers},
          m_messageObservers{messageObservers},
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
    if (m_isEnabled) {
        m_messageRouter->disable();
        m_messageRouter->enable();
    }
}

void AVSConnectionManager::sendMessage(std::shared_ptr<avsCommon::avs::MessageRequest> request) {
    // TODO: ACSDK-421: Implement synchronized state check at a lower level.
    if (m_isSynchronized) {
        m_messageRouter->sendMessage(request);
    } else {
        ACSDK_DEBUG(LX("sendMessageNotSuccessful").d("reason", "notSynchronized"));
        request->onSendCompleted(avsCommon::avs::MessageRequest::Status::NOT_SYNCHRONIZED);
    }
}

bool AVSConnectionManager::isConnected() const {
    return m_messageRouter->getConnectionStatus().first == ConnectionStatusObserverInterface::Status::CONNECTED;
}

void AVSConnectionManager::setAVSEndpoint(const std::string& avsEndpoint) {
    m_messageRouter->setAVSEndpoint(avsEndpoint);
}

void AVSConnectionManager::addConnectionStatusObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("addConnectionStatusObserverFailed").d("reason", "nullObserver"));
        return;
    }

    {
        std::lock_guard<std::mutex> lock{m_connectionStatusObserverMutex};
        if (!m_connectionStatusObservers.insert(observer).second) {
            // The observer was already part of the set, no need to notify it of a status change.
            return;
        }
    }

    auto connectionStatus = m_messageRouter->getConnectionStatus();
    observer->onConnectionStatusChanged(connectionStatus.first, connectionStatus.second);
}

void AVSConnectionManager::removeConnectionStatusObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("removeConnectionStatusObserverFailed").d("reason", "nullObserver"));
        return;
    }
    std::lock_guard<std::mutex> lock{m_connectionStatusObserverMutex};
    m_connectionStatusObservers.erase(observer);
}

void AVSConnectionManager::addMessageObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("addObserverFailed").d("reason", "nullObserver"));
        return;
    }

    std::lock_guard<std::mutex> lock{m_messageOberverMutex};
    m_messageObservers.insert(observer);
}

void AVSConnectionManager::removeMessageObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("removeObserverFailed").d("reason", "nullObserver"));
        return;
    }

    std::lock_guard<std::mutex> lock{m_messageOberverMutex};
    m_messageObservers.erase(observer);
}

void AVSConnectionManager::onConnectionStatusChanged(
  const ConnectionStatusObserverInterface::Status status,
        const ConnectionStatusObserverInterface::ChangedReason reason) {
    std::unique_lock<std::mutex> lock{m_connectionStatusObserverMutex};
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>> observers{m_connectionStatusObservers};
    lock.unlock();

    for (auto observer : observers) {
        observer->onConnectionStatusChanged(status, reason);
    }
}

void AVSConnectionManager::onStateChanged(StateSynchronizerObserverInterface::State newState) {
    m_isSynchronized = (StateSynchronizerObserverInterface::State::SYNCHRONIZED == newState);
}

void AVSConnectionManager::receive(const std::string & contextId, const std::string & message) {
    std::unique_lock<std::mutex> lock{m_messageOberverMutex};
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface>> observers{m_messageObservers};
    lock.unlock();

    for (auto observer : observers) {
        if (observer) {
            observer->receive(contextId, message);
        }
    }
}

} // namespace acl
} // namespace alexaClientSDK
