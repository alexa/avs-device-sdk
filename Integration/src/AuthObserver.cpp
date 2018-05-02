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

#include "Integration/AuthObserver.h"
#include <iostream>

namespace alexaClientSDK {
namespace integration {

using avsCommon::sdkInterfaces::AuthObserverInterface;

AuthObserver::AuthObserver() :
        m_authState(AuthObserverInterface::State::UNINITIALIZED),
        m_authError(AuthObserverInterface::Error::SUCCESS) {
}

void AuthObserver::onAuthStateChange(
    const AuthObserverInterface::State authState,
    const AuthObserverInterface::Error authError) {
    m_authState = authState;
    m_authError = authError;
    m_wakeTrigger.notify_all();
}

AuthObserverInterface::State AuthObserver::getAuthState() const {
    return m_authState;
}

bool AuthObserver::waitFor(const AuthObserverInterface::State authState, const std::chrono::seconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_wakeTrigger.wait_for(lock, duration, [this, authState]() { return m_authState == authState; });
}

}  // namespace integration
}  // namespace alexaClientSDK
