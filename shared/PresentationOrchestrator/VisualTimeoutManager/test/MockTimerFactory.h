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

#ifndef ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_VISUALTIMEOUTMANAGER_TEST_MOCKTIMERFACTORY_H_
#define ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_VISUALTIMEOUTMANAGER_TEST_MOCKTIMERFACTORY_H_

#include <chrono>
#include <functional>

#include <AVSCommon/SDKInterfaces/Timing/TimerDelegateFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/Timing/TimerDelegateInterface.h>

namespace alexaClientSDK {
namespace visualTimeoutManager {
namespace test {

/**
 * A basic timer that enables jumping forward to prevent real-time waiting
 * This timer does not support periodic delays
 */
class WarpTimer : public alexaClientSDK::avsCommon::sdkInterfaces::timing::TimerDelegateInterface {
public:
    /// @name TimerDelegateInterface Functions
    /// @{
    void start(
        std::chrono::nanoseconds delay,
        std::chrono::nanoseconds period,
        PeriodType periodType,
        size_t maxCount,
        std::function<void()> task) override {
        m_task = task;
        m_delay = delay;
    }
    void stop() override {
        m_active = false;
    }
    bool activate() override {
        assert(!m_active);
        m_active = true;
        return m_active;
    }
    bool isActive() const override {
        return m_active;
    }
    /// @}

    /**
     * Simulate time moving forward by the given period, if step is greater than the configured delay the timer will be
     * fired. Step time is absolute, time is not added to any previous calls to this function.
     * @param step Time period to step forward.
     * @return True if task was dispatched
     */
    bool warpForward(std::chrono::nanoseconds step) {
        bool dispatched = (step >= m_delay);
        if (dispatched) {
            m_task();
        }

        return dispatched;
    }

    /**
     * Get the delay which has been configured for this timer.
     *
     * @return The delay period
     */
    std::chrono::nanoseconds getDelay() {
        return m_delay;
    }

protected:
    std::function<void()> m_task;
    std::chrono::nanoseconds m_delay;
    bool m_active{false};
};

/**
 * MockTimerFactory to return a single instance of WarpTimer
 */
class MockTimerFactory : public alexaClientSDK::avsCommon::sdkInterfaces::timing::TimerDelegateFactoryInterface {
public:
    /// @name TimerDelegateFactoryInterface Functions
    /// @{
    bool supportsLowPowerMode() override {
        return true;
    }
    std::unique_ptr<alexaClientSDK::avsCommon::sdkInterfaces::timing::TimerDelegateInterface> getTimerDelegate()
        override {
        if (m_timer) {
            assert(false);  // does not support multiple instances
        }
        m_timer = new WarpTimer();
        std::unique_ptr<alexaClientSDK::avsCommon::sdkInterfaces::timing::TimerDelegateInterface> ptr{m_timer};

        return ptr;
    }
    /// @}

    /**
     * Gets the timer which was created by the last call to @c getTimerDelegate()
     * @note This function must be used with great care, the lifetime of the timer is controlled by the unique_ptr
     * which was returned by @c getTimerDelegate()
     *
     * @returns pointer to the timer instance
     */
    WarpTimer* getTimer() {
        return m_timer;
    }

protected:
    WarpTimer* m_timer{nullptr};
};

}  // namespace test
}  // namespace visualTimeoutManager
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_VISUALTIMEOUTMANAGER_TEST_MOCKTIMERFACTORY_H_