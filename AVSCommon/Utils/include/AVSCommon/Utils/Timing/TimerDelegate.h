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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_TIMERDELEGATE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_TIMERDELEGATE_H_

#include <atomic>
#include <condition_variable>
#include <thread>

#include <AVSCommon/SDKInterfaces/Timing/TimerDelegateInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {

class TimerDelegate : public sdkInterfaces::timing::TimerDelegateInterface {
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
    TimerDelegate();

    /// Destructor.
    ~TimerDelegate() override;

private:
    /**
     * Main timer loop.
     *
     * @param delay The non-negative time to wait before making the first @c task call.
     * @param period The non-negative time to wait between subsequent @c task calls.
     * @param periodType The type of period to use when making subsequent task calls.
     * @param maxCount The desired number of times to call task.
     * @param task A callable type representing a task.
     * @param moniker Moniker value to use for a new thread.
     */
    void timerLoop(
        std::chrono::nanoseconds delay,
        std::chrono::nanoseconds period,
        PeriodType periodType,
        size_t maxCount,
        std::function<void()> task,
        std::string moniker);

    /// Cleanup logic which waits for the thread to join if possible. @c m_callMutex must be held before calling.
    void cleanupLocked();

    /// Internal logic that activates this @c TimerDelegate instance.
    bool activateLocked();

    /// The condition variable used to wait for @c stop() or period timeouts.
    std::condition_variable m_waitCondition;

    /// The mutex for synchronization with the std::condition_variable.
    mutable std::mutex m_waitMutex;

    /// The mutex for synchronizing calls into TimerDelegate.
    mutable std::mutex m_callMutex;

    /// Flag which indicates that a @c Timer is active.
    std::atomic<bool> m_running;

    /**
     * Flag which requests that the active @c Timer be stopped.  @c m_waitMutex must be locked when modifying this
     * variable.
     */
    bool m_stopping;

    /**
     * The thread which this runs on.
     */
    std::thread m_thread;
};

}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_TIMERDELEGATE_H_
