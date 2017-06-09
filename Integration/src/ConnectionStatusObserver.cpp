/*
 * ConnectionStatusObserver.cpp
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

#include "Integration/ConnectionStatusObserver.h"

#include <iostream>

namespace alexaClientSDK {
namespace integration {

ConnectionStatusObserver::ConnectionStatusObserver(): 
        m_connectionStatus(ConnectionStatusObserverInterface::Status::DISCONNECTED) {
}

void ConnectionStatusObserver::onConnectionStatusChanged(
        const ConnectionStatusObserverInterface::Status connectionStatus,
        const ConnectionStatusObserverInterface::ChangedReason reason) {
            m_connectionStatus = connectionStatus;
            m_wakeTrigger.notify_all();
}
	
ConnectionStatusObserverInterface::Status ConnectionStatusObserver::getConnectionStatus() const {
    return m_connectionStatus;
}

bool ConnectionStatusObserver::waitFor(
        const ConnectionStatusObserverInterface::Status connectionStatus, const std::chrono::seconds duration) {
            std::unique_lock<std::mutex> lock(m_mutex);
            return m_wakeTrigger.wait_for(lock, duration, [this, connectionStatus]() {
                return m_connectionStatus == connectionStatus;
    });
}

} // namespace integration
} // namespace alexaClientSDK
