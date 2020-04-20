/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "AVSCommon/AVS/Initialization/AlexaClientSDKInit.h"
#include "AVSCommon/Utils/Logger/Logger.h"
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

std::shared_ptr<AVSConnectionManager> AVSConnectionManager::create(
    std::shared_ptr<MessageRouterInterface> messageRouter,
    bool isEnabled,
    std::unordered_set<std::shared_ptr<ConnectionStatusObserverInterface>> connectionStatusObservers,
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface>> messageObservers,
    std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionMonitorInterface> internetConnectionMonitor) {
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

    for (auto observer : messageObservers) {
        if (!observer) {
            ACSDK_ERROR(LX("createFailed").d("reason", "nullMessageObserver").d("return", "nullptr"));
            return nullptr;
        }
    }

    auto connectionManager = std::shared_ptr<AVSConnectionManager>(new AVSConnectionManager(
        messageRouter, connectionStatusObservers, messageObservers, internetConnectionMonitor));

    messageRouter->setObserver(connectionManager);

    if (isEnabled) {
        connectionManager->enable();
    }

    if (internetConnectionMonitor) {
        ACSDK_DEBUG5(LX(__func__).m("Subscribing to InternetConnectionMonitor Callbacks"));
        internetConnectionMonitor->addInternetConnectionObserver(connectionManager);
    }

    return connectionManager;
}

AVSConnectionManager::AVSConnectionManager(
    std::shared_ptr<MessageRouterInterface> messageRouter,
    std::unordered_set<std::shared_ptr<ConnectionStatusObserverInterface>> connectionStatusObservers,
    std::unordered_set<std::shared_ptr<MessageObserverInterface>> messageObservers,
    std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionMonitorInterface> internetConnectionMonitor) :
        AbstractAVSConnectionManager{connectionStatusObservers},
        RequiresShutdown{"AVSConnectionManager"},
        m_isEnabled{false},
        m_messageObservers{messageObservers},
        m_messageRouter{messageRouter},
        m_internetConnectionMonitor{internetConnectionMonitor} {
}

void AVSConnectionManager::doShutdown() {
    if (m_internetConnectionMonitor) {
        m_internetConnectionMonitor->removeInternetConnectionObserver(shared_from_this());
    }

    disable();
    clearObservers();
    {
        std::lock_guard<std::mutex> lock{m_messageObserverMutex};
        m_messageObservers.clear();
    }
    m_messageRouter.reset();
}

void AVSConnectionManager::enable() {
    std::lock_guard<std::mutex> lock(m_isEnabledMutex);
    ACSDK_DEBUG5(LX(__func__));
    m_isEnabled = true;
    m_messageRouter->enable();
}

void AVSConnectionManager::disable() {
    std::lock_guard<std::mutex> lock(m_isEnabledMutex);
    ACSDK_DEBUG5(LX(__func__));
    m_isEnabled = false;
    m_messageRouter->disable();
}

bool AVSConnectionManager::isEnabled() {
    std::lock_guard<std::mutex> lock(m_isEnabledMutex);
    return m_isEnabled;
}

void AVSConnectionManager::reconnect() {
    std::lock_guard<std::mutex> lock(m_isEnabledMutex);
    ACSDK_DEBUG5(LX(__func__).d("isEnabled", m_isEnabled));
    if (m_isEnabled) {
        m_messageRouter->disable();
        m_messageRouter->enable();
    }
}

void AVSConnectionManager::sendMessage(std::shared_ptr<avsCommon::avs::MessageRequest> request) {
    m_messageRouter->sendMessage(request);
}

bool AVSConnectionManager::isConnected() const {
    return m_messageRouter->getConnectionStatus().first == ConnectionStatusObserverInterface::Status::CONNECTED;
}

void AVSConnectionManager::onWakeConnectionRetry() {
    ACSDK_DEBUG9(LX(__func__));
    m_messageRouter->onWakeConnectionRetry();
}

void AVSConnectionManager::setAVSGateway(const std::string& avsGateway) {
    m_messageRouter->setAVSGateway(avsGateway);
}

std::string AVSConnectionManager::getAVSGateway() {
    return m_messageRouter->getAVSGateway();
}

void AVSConnectionManager::onConnectionStatusChanged(bool connected) {
    ACSDK_DEBUG5(LX(__func__).d("connected", connected).d("isEnabled", m_isEnabled));
    if (m_isEnabled) {
        if (connected) {
            m_messageRouter->onWakeConnectionRetry();
        } else {
            m_messageRouter->onWakeVerifyConnectivity();
        }
    }
}

void AVSConnectionManager::addMessageObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("addObserverFailed").d("reason", "nullObserver"));
        return;
    }

    std::lock_guard<std::mutex> lock{m_messageObserverMutex};
    m_messageObservers.insert(observer);
}

void AVSConnectionManager::removeMessageObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("removeObserverFailed").d("reason", "nullObserver"));
        return;
    }

    std::lock_guard<std::mutex> lock{m_messageObserverMutex};
    m_messageObservers.erase(observer);
}

void AVSConnectionManager::onConnectionStatusChanged(
    const ConnectionStatusObserverInterface::Status status,
    const ConnectionStatusObserverInterface::ChangedReason reason) {
    updateConnectionStatus(status, reason);
}

void AVSConnectionManager::receive(const std::string& contextId, const std::string& message) {
    std::unique_lock<std::mutex> lock{m_messageObserverMutex};
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface>> observers{
        m_messageObservers};
    lock.unlock();

    for (auto observer : observers) {
        if (observer) {
            observer->receive(contextId, message);
        }
    }
}

}  // namespace acl
}  // namespace alexaClientSDK
