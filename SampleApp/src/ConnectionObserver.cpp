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

#include "SampleApp/ConnectionObserver.h"

namespace alexaClientSDK {
namespace sampleApp {

using namespace avsCommon::sdkInterfaces;

ConnectionObserver::ConnectionObserver() :
        m_authState(AuthObserverInterface::State::UNINITIALIZED),
        m_connectionStatus(ConnectionStatusObserverInterface::Status::DISCONNECTED) {
}

void ConnectionObserver::onAuthStateChange(AuthObserverInterface::State newState, AuthObserverInterface::Error error) {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_authState = newState;
    m_trigger.notify_all();
}

void ConnectionObserver::onConnectionStatusChanged(
    const ConnectionStatusObserverInterface::Status status,
    const ConnectionStatusObserverInterface::ChangedReason reason) {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_connectionStatus = status;
    m_trigger.notify_all();
}

bool ConnectionObserver::waitFor(const AuthObserverInterface::State authState, const std::chrono::seconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_trigger.wait_for(lock, duration, [this, authState]() { return authState == m_authState; });
}

bool ConnectionObserver::waitFor(
    const ConnectionStatusObserverInterface::Status connectionStatus,
    const std::chrono::seconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_trigger.wait_for(
        lock, duration, [this, connectionStatus]() { return connectionStatus == m_connectionStatus; });
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
