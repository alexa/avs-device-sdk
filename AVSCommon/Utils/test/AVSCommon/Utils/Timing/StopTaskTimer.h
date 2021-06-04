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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_TIMING_STOPTASKTIMER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_TIMING_STOPTASKTIMER_H_

#include <memory>
#include <functional>

#include <AVSCommon/SDKInterfaces/Timing/TimerDelegateFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/Timing/TimerDelegateInterface.h>
#include <AVSCommon/Utils/Timing/TimerDelegate.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {
namespace test {

/**
 * A timer that will call the task when stop() is called. This simulates the limitation that's inherent in the Timer.h
 * API.
 *
 * Since the task() may be executing naturally when stop() is called, we synchronize the task with a lock so only one
 * instance of it can execute at a given time.
 *
 * WARNING: The task is executed during stop(), it's possible that the task will fire NOT at the scheduled intervals of
 * the original call to start(). This does not fully conform with the Timer.h API, and should be used for very specific
 * test cases. This should not be used outside of testing.
 */
class StopTaskTimer : public avsCommon::sdkInterfaces::timing::TimerDelegateInterface {
public:
    /// @name TimerDelegateInterface Functions
    /// @{
    void start(
        std::chrono::nanoseconds delay,
        std::chrono::nanoseconds period,
        PeriodType periodType,
        size_t maxCount,
        std::function<void()> task) override;
    void stop() override;
    bool activate() override;
    bool isActive() const override;
    /// @}

    /// Constructor.
    StopTaskTimer();

    /// Destructor.
    ~StopTaskTimer();

private:
    /// Used to prevent multiple instances of the task executing.
    std::mutex m_taskMutex;

    /// Used to synchronize calls to StopTaskTimer
    std::mutex m_mutex;

    /// The task.
    std::function<void()> m_task;

    /// The underlying timer.
    std::unique_ptr<avsCommon::sdkInterfaces::timing::TimerDelegateInterface> m_delegate;
};

class StopTaskTimerDelegateFactory : public avsCommon::sdkInterfaces::timing::TimerDelegateFactoryInterface {
public:
    /// @name TimerDelegateFactoryInterface Functions
    /// @{
    bool supportsLowPowerMode() override;
    std::unique_ptr<avsCommon::sdkInterfaces::timing::TimerDelegateInterface> getTimerDelegate() override;
    /// @}
};

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_TIMING_STOPTASKTIMER_H_

}  // namespace test
}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
