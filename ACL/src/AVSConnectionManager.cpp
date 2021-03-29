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

std::shared_ptr<MessageSenderInterface> AVSConnectionManager::createMessageSenderInterface(
    const std::shared_ptr<AVSConnectionManagerInterface>& connectionManager) {
    return connectionManager;
}

std::shared_ptr<AVSConnectionManagerInterface> AVSConnectionManager::createAVSConnectionManagerInterface(
    const std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>& shutdownNotifier,
    const std::shared_ptr<MessageRouterInterface>& messageRouter,
    const std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionMonitorInterface>& internetConnectionMonitor) {
    auto avsConnectionManager = create(messageRouter, false, {}, {}, internetConnectionMonitor);
    if (avsConnectionManager) {
        shutdownNotifier->addObserver(avsConnectionManager);
    }
    return avsConnectionManager;
}

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
    std::unique_lock<std::mutex> lock{m_messageRouterMutex};
    /* There is still a potential deadlock if the reset of m_messageRouter triggers a delete of the message router,
    and that delete blocks on code that could call back into AVSConnectionManager and try to acquire that new mutex.
    We can get around this by having doShutdown() swap m_messageRouter with a local variable while holding the lock,
    and then resetting the local variable after new mutex is released.
     */
    auto messageRouter = m_messageRouter;
    m_messageRouter.reset();
    lock.unlock();

    messageRouter.reset();
}

void AVSConnectionManager::enable() {
    std::lock_guard<std::mutex> lock(m_isEnabledMutex);
    ACSDK_DEBUG5(LX(__func__));
    m_isEnabled = true;
    auto messageRouter = getMessageRouter();
    if (messageRouter) {
        messageRouter->enable();
    }
}

void AVSConnectionManager::disable() {
    std::lock_guard<std::mutex> lock(m_isEnabledMutex);
    ACSDK_DEBUG5(LX(__func__));
    m_isEnabled = false;
    auto messageRouter = getMessageRouter();
    if (messageRouter) {
        messageRouter->disable();
    }
}

bool AVSConnectionManager::isEnabled() {
    std::lock_guard<std::mutex> lock(m_isEnabledMutex);
    return m_isEnabled;
}

void AVSConnectionManager::reconnect() {
    std::lock_guard<std::mutex> lock(m_isEnabledMutex);
    ACSDK_DEBUG5(LX(__func__).d("isEnabled", m_isEnabled));
    if (m_isEnabled) {
        auto messageRouter = getMessageRouter();
        if (messageRouter) {
            messageRouter->disable();
            messageRouter->enable();
        }
    }
}

void AVSConnectionManager::sendMessage(std::shared_ptr<avsCommon::avs::MessageRequest> request) {
    auto messageRouter = getMessageRouter();
    if (messageRouter) {
        messageRouter->sendMessage(request);
    } else {
        ACSDK_WARN(LX("sendMessageFailed")
                       .d("reason", "nullMessageRouter")
                       .m("setting status for request to NOT_CONNECTED")
                       .d("request", request->getJsonContent()));
        request->sendCompleted(MessageRequestObserverInterface::Status::NOT_CONNECTED);
    }
}

bool AVSConnectionManager::isConnected() const {
    auto messageRouter = getMessageRouter();
    if (messageRouter) {
        return messageRouter->getConnectionStatus().first == ConnectionStatusObserverInterface::Status::CONNECTED;
    }
    return false;
}

void AVSConnectionManager::onWakeConnectionRetry() {
    ACSDK_DEBUG9(LX(__func__));
    auto messageRouter = getMessageRouter();
    if (messageRouter) {
        messageRouter->onWakeConnectionRetry();
    } else {
        ACSDK_WARN(LX("onWakeConnectionRetryFailed").d("reason", "nullMessageRouter"));
    }
}

void AVSConnectionManager::setAVSGateway(const std::string& avsGateway) {
    auto messageRouter = getMessageRouter();
    if (messageRouter) {
        messageRouter->setAVSGateway(avsGateway);
    } else {
        ACSDK_WARN(LX("setAVSGatewayFailed").d("reason", "nullMessageRouter"));
    }
}

std::string AVSConnectionManager::getAVSGateway() const {
    auto messageRouter = getMessageRouter();
    if (messageRouter) {
        return messageRouter->getAVSGateway();
    } else {
        ACSDK_WARN(LX("getAVSGatewayFailed").d("reason", "nullMessageRouter"));
    }
    return "";
}

void AVSConnectionManager::onConnectionStatusChanged(bool connected) {
    ACSDK_DEBUG5(LX(__func__).d("connected", connected).d("isEnabled", m_isEnabled));
    if (m_isEnabled) {
        auto messageRouter = getMessageRouter();
        if (messageRouter) {
            if (connected) {
                messageRouter->onWakeConnectionRetry();
            } else {
                messageRouter->onWakeVerifyConnectivity();
            }
        } else {
            ACSDK_WARN(LX("onConnectionStatusChangedFailed").d("reason", "nullMessageRouter"));
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
    const std::vector<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::EngineConnectionStatus>&
        engineConnectionStatuses) {
    ACSDK_DEBUG(LX(__func__).d("status", status).d("engine_count", engineConnectionStatuses.size()));
    updateConnectionStatus(status, engineConnectionStatuses);
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

std::shared_ptr<MessageRouterInterface> AVSConnectionManager::getMessageRouter() const {
    std::lock_guard<std::mutex> lock{m_messageRouterMutex};
    return m_messageRouter;
}

}  // namespace acl
}  // namespace alexaClientSDK
