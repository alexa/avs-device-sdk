/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKFOCUSMANAGEROBSERVER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKFOCUSMANAGEROBSERVER_H_

#include "AVSCommon/SDKInterfaces/FocusManagerObserverInterface.h"

#include <gmock/gmock.h>

#include <chrono>
#include <mutex>
#include <condition_variable>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace test {

/// Mock class that implements the FocusManagerObserver.
class MockFocusManagerObserver : public FocusManagerObserverInterface {
public:
    MockFocusManagerObserver();

    MOCK_METHOD2(onFocusChanged, void(const std::string& channelName, avs::FocusState newFocus));

    /**
     * EXPECT_CALL wrapper which tracks the number of onFocusChanged calls we are expecting.
     *
     * @param channelName The name of the channel which changed @c FocusState.
     * @param newFocus The new @c FocusState of @c channelName.
     */
    void expectFocusChange(const std::string& channelName, avs::FocusState newFocus);

    /**
     * Waits for @c expectFocusChange() calls to complete.
     *
     * @param timeout Amount of time to wait for all calls to complete.
     * @return @c true if all calls completed, else @c false.
     */
    bool waitForFocusChanges(std::chrono::milliseconds timeout = std::chrono::milliseconds::zero());

private:
    size_t m_expects;
    std::mutex m_mutex;
    std::condition_variable m_conditionVariable;
};

inline MockFocusManagerObserver::MockFocusManagerObserver() : m_expects{0} {
}

inline void MockFocusManagerObserver::expectFocusChange(const std::string& channelName, avs::FocusState newFocus) {
    std::lock_guard<std::mutex> lock(m_mutex);
    EXPECT_CALL(*this, onFocusChanged(testing::StrEq(channelName), newFocus))
        .WillOnce(testing::InvokeWithoutArgs([this] {
            std::lock_guard<std::mutex> lock(m_mutex);
            --m_expects;
            m_conditionVariable.notify_all();
        }));
    ++m_expects;
}

inline bool MockFocusManagerObserver::waitForFocusChanges(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_conditionVariable.wait_for(lock, timeout, [this] { return !m_expects; });
}

}  // namespace test
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKFOCUSMANAGEROBSERVER_H_
