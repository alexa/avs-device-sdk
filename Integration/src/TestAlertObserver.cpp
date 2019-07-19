/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <iostream>

#include "Integration/TestAlertObserver.h"

namespace alexaClientSDK {
namespace integration {
namespace test {

void TestAlertObserver::onAlertStateChange(
    const std::string& alertToken,
    const std::string& alertType,
    State state,
    const std::string& reason) {
    std::unique_lock<std::mutex> lock(m_mutex);
    TestAlertObserver::changedAlert ca;
    ca.state = state;
    m_queue.push_back(ca);
    m_wakeTrigger.notify_all();
}

TestAlertObserver::changedAlert TestAlertObserver::waitForNext(const std::chrono::seconds duration) {
    TestAlertObserver::changedAlert ret;
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakeTrigger.wait_for(lock, duration, [this]() { return !m_queue.empty(); })) {
        ret.state = currentState;
        return ret;
    }
    ret = m_queue.front();
    m_queue.pop_front();
    return ret;
}

}  // namespace test
}  // namespace integration
}  // namespace alexaClientSDK
