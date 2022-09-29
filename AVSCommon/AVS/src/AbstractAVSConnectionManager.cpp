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

#include "AVSCommon/AVS/AbstractAVSConnectionManager.h"
#include "AVSCommon/Utils/Logger/Logger.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

using namespace avsCommon::sdkInterfaces;

/// String to identify log entries originating from this file.
#define TAG "AbstractAVSConnectionManager"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * Create a LogEntry using this file's TAG, the specified event string, and pointer to disambiguate the instance.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX_P(event) LX(event).p("this", this)

AbstractAVSConnectionManager::AbstractAVSConnectionManager(
    std::unordered_set<std::shared_ptr<ConnectionStatusObserverInterface>> observers) :
        m_connectionStatus{ConnectionStatusObserverInterface::Status::DISCONNECTED},
        m_avsConnectionStatus{ConnectionStatusObserverInterface::Status::DISCONNECTED},
        m_connectionStatusObservers{observers} {
    m_engineConnectionStatuses.emplace_back(
        avsCommon::sdkInterfaces::ENGINE_TYPE_ALEXA_VOICE_SERVICES,
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST,
        ConnectionStatusObserverInterface::Status::DISCONNECTED);
}

void AbstractAVSConnectionManager::addConnectionStatusObserver(
    std::shared_ptr<ConnectionStatusObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX_P("addConnectionStatusObserverFailed").d("reason", "nullObserver"));
        return;
    }

    std::unique_lock<std::mutex> lock{m_mutex};
    auto localStatus = m_connectionStatus;
    auto localEngineConnectionStatuses = m_engineConnectionStatuses;
    bool addedOk = m_connectionStatusObservers.insert(observer).second;
    lock.unlock();

    if (addedOk) {
        // call new onConnectionStatusChanged API
        for (auto engineStatus : localEngineConnectionStatuses) {
            (void)engineStatus;
            ACSDK_DEBUG9(LX_P("addConnectionStatusObserver")
                             .d("engineType", engineStatus.engineType)
                             .d("status", engineStatus.status)
                             .d("reason", engineStatus.reason));
        }
        observer->onConnectionStatusChanged(localStatus, localEngineConnectionStatuses);
        // call old onConnectionStatusChanged API on AVS connection
        for (const auto& status : localEngineConnectionStatuses) {
            if (status.engineType == avsCommon::sdkInterfaces::ENGINE_TYPE_ALEXA_VOICE_SERVICES) {
                observer->onConnectionStatusChanged(status.status, status.reason);
                break;
            }
        }
    }
}

void AbstractAVSConnectionManager::removeConnectionStatusObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX_P("removeConnectionStatusObserverFailed").d("reason", "nullObserver"));
        return;
    }

    std::lock_guard<std::mutex> lock{m_mutex};
    m_connectionStatusObservers.erase(observer);
}

void AbstractAVSConnectionManager::updateConnectionStatus(
    alexaClientSDK::avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status status,
    const std::vector<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::EngineConnectionStatus>&
        engineConnectionStatuses) {
    std::unique_lock<std::mutex> lock{m_mutex};
    m_connectionStatus = status;
    bool avsConnectionStatusChanged = false;
    for (const auto& engineStatus : engineConnectionStatuses) {
        if (engineStatus.engineType == avsCommon::sdkInterfaces::ENGINE_TYPE_ALEXA_VOICE_SERVICES) {
            avsConnectionStatusChanged = m_avsConnectionStatus != engineStatus.status;
            m_avsConnectionStatus = engineStatus.status;
            break;
        }
    }
    m_engineConnectionStatuses = engineConnectionStatuses;
    lock.unlock();

    notifyObservers(avsConnectionStatusChanged);
}

void AbstractAVSConnectionManager::notifyObservers(bool avsConnectionStatusChanged) {
    std::unique_lock<std::mutex> lock{m_mutex};
    auto observers = m_connectionStatusObservers;
    auto localStatus = m_connectionStatus;
    auto localEngineConnectionStatuses = m_engineConnectionStatuses;
    lock.unlock();

    for (auto engineStatus : localEngineConnectionStatuses) {
        (void)engineStatus;
        ACSDK_DEBUG5(LX_P("notifyObservers")
                         .m("EngineConnectionStatusDetail")
                         .d("engineType", engineStatus.engineType)
                         .d("status", engineStatus.status)
                         .d("reason", engineStatus.reason));
    }
    for (const auto& observer : observers) {
        // call new onConnectionStatusChanged API
        observer->onConnectionStatusChanged(localStatus, localEngineConnectionStatuses);
        // call old onConnectionStatusChanged API on AVS connection
        if (avsConnectionStatusChanged) {
            for (const auto& status : localEngineConnectionStatuses) {
                if (status.engineType == avsCommon::sdkInterfaces::ENGINE_TYPE_ALEXA_VOICE_SERVICES) {
                    observer->onConnectionStatusChanged(status.status, status.reason);
                    break;
                }
            }
        }
    }
}

void AbstractAVSConnectionManager::clearObservers() {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_connectionStatusObservers.clear();
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
