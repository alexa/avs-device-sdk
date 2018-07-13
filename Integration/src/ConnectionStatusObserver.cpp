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

#include "Integration/ConnectionStatusObserver.h"

#include <iostream>

namespace alexaClientSDK {
namespace integration {

using alexaClientSDK::avsCommon::sdkInterfaces::ConnectionStatusObserverInterface;

ConnectionStatusObserver::ConnectionStatusObserver() :
        m_statusChanges({std::make_pair(Status::DISCONNECTED, ChangedReason::ACL_CLIENT_REQUEST)}) {
}

void ConnectionStatusObserver::onConnectionStatusChanged(const Status connectionStatus, const ChangedReason reason) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_statusChanges.push_back(std::make_pair(connectionStatus, reason));
    m_wakeTrigger.notify_all();
}

bool ConnectionStatusObserver::checkForServerSideDisconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto pairValue : m_statusChanges) {
        if (pairValue.first == Status::PENDING && pairValue.second == ChangedReason::SERVER_SIDE_DISCONNECT) {
            return true;
        }
    }
    return false;
}

ConnectionStatusObserverInterface::Status ConnectionStatusObserver::getConnectionStatus() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_statusChanges.back().first;
}

bool ConnectionStatusObserver::waitFor(const Status connectionStatus, const std::chrono::seconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_wakeTrigger.wait_for(
        lock, duration, [this, connectionStatus]() { return m_statusChanges.back().first == connectionStatus; });
}

}  // namespace integration
}  // namespace alexaClientSDK
