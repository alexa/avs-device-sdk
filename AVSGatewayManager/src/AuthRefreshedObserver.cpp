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

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSGatewayManager/AuthRefreshedObserver.h>

using namespace std;
using namespace alexaClientSDK::avsCommon::sdkInterfaces;
using namespace alexaClientSDK::avsGatewayManager;

static const string TAG("AuthRefreshedObserver");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * Create a LogEntry using this file's TAG and the specified event string.  With the first entry being the instance
 * memory location.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX_P(event) LX(event).p("this", this)

AuthRefreshedObserver::AuthRefreshedObserver(function<void()> afterAuthRefreshedCallback) :
        m_state{State::UNINITIALIZED},
        m_afterAuthRefreshedCallback{move(afterAuthRefreshedCallback)} {
}

shared_ptr<AuthRefreshedObserver> alexaClientSDK::avsGatewayManager::AuthRefreshedObserver::create(
    function<void()> callback) {
    if (!callback) {
        ACSDK_ERROR(LX("createFailed").d("reason", "invalid callback"));
        return nullptr;
    }
    return shared_ptr<AuthRefreshedObserver>(new AuthRefreshedObserver(callback));
}

void AuthRefreshedObserver::onAuthStateChange(State newState, Error error) {
    ACSDK_INFO(LX_P(__func__).d("newState", newState).d("error", error));

    lock_guard<std::mutex> lock(m_mutex);
    switch (newState) {
        case AuthObserverInterface::State::REFRESHED:
            if (newState != m_state) {
                m_afterAuthRefreshedCallback();
            }
            break;
        default:
            break;
    }
    m_state = newState;
};