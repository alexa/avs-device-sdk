/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
static const std::string TAG("AbstractAVSConnectionManager");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

AbstractAVSConnectionManager::AbstractAVSConnectionManager(
    std::unordered_set<std::shared_ptr<ConnectionStatusObserverInterface>> observers) :
        m_connectionStatus{ConnectionStatusObserverInterface::Status::DISCONNECTED},
        m_connectionChangedReason{ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST},
        m_connectionStatusObservers{observers} {
}

void AbstractAVSConnectionManager::addConnectionStatusObserver(
    std::shared_ptr<ConnectionStatusObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("addConnectionStatusObserverFailed").d("reason", "nullObserver"));
        return;
    }

    std::unique_lock<std::mutex> lock{m_mutex};
    auto localStatus = m_connectionStatus;
    auto localReason = m_connectionChangedReason;
    bool addedOk = m_connectionStatusObservers.insert(observer).second;
    lock.unlock();

    if (addedOk) {
        observer->onConnectionStatusChanged(localStatus, localReason);
    }
}

void AbstractAVSConnectionManager::removeConnectionStatusObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("removeConnectionStatusObserverFailed").d("reason", "nullObserver"));
        return;
    }

    std::lock_guard<std::mutex> lock{m_mutex};
    m_connectionStatusObservers.erase(observer);
}

void AbstractAVSConnectionManager::updateConnectionStatus(
    ConnectionStatusObserverInterface::Status status,
    ConnectionStatusObserverInterface::ChangedReason reason) {
    std::unique_lock<std::mutex> lock{m_mutex};
    m_connectionStatus = status;
    m_connectionChangedReason = reason;
    lock.unlock();

    notifyObservers();
}

void AbstractAVSConnectionManager::notifyObservers() {
    std::unique_lock<std::mutex> lock{m_mutex};
    auto observers = m_connectionStatusObservers;
    auto localStatus = m_connectionStatus;
    auto localReason = m_connectionChangedReason;
    lock.unlock();

    for (auto observer : observers) {
        observer->onConnectionStatusChanged(localStatus, localReason);
    }
}

void AbstractAVSConnectionManager::clearObservers() {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_connectionStatusObservers.clear();
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
