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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_MULTITIMER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_MULTITIMER_H_

#include <cstdint>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>

#include "AVSCommon/Utils/Threading/Executor.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {

/**
 * A @c MultiTimer is used to schedule multiple callable types to run in the future.
 *
 * @note The executed function should not block since this may cause delays to trigger other tasks in the queue.
 */
class MultiTimer {
public:
    /// Alias for the token used to identify a task. This can be used to cancel a task execution.
    using Token = uint64_t;

    /**
     * Constructor.
     */
    MultiTimer();

    /**
     * Destructor.
     */
    ~MultiTimer();

    /**
     * Submits a task to be executed after a given delay.
     *
     * This function take longer than the delay due to scheduling or resource contention.
     *
     * @param delay The non-negative time to wait before calling the given task.
     * @param task The task to be executed.
     * @return A unique token that can be used to cancel this task.
     */
    Token submitTask(const std::chrono::milliseconds& delay, std::function<void()> task);

    /**
     * Removes a task from the queue.
     *
     * @param token The token used to identify the task to be canceled.
     */
    void cancelTask(Token token);

private:
    /**
     * Execute the timer inside its own thread.
     *
     * @return @c true if there are more timers scheduled; @c false otherwise.
     */
    bool executeTimer();

    /**
     * Checks if there are any pending tasks within a grace period determined by an internal timeout.
     *
     * @param lock The lock being held during the check.
     * @return @c true if there's at least one task left; @c false if there is no pending task.
     */
    bool hasNextLocked(std::unique_lock<std::mutex>& lock);

    /// Alias for the time point used in this class.
    using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

    /// The condition variable used to wait for the next task.
    std::condition_variable m_waitCondition;

    /// The mutex for @c m_waitCondition.
    std::mutex m_waitMutex;

    /// The executor used to trigger tasks.
    threading::TaskThread m_timerThread;

    /// A map of timers and the token used to identify the task to be run.
    std::multimap<TimePoint, Token> m_timers;

    /// A map of tasks to be run.
    std::map<Token, std::pair<TimePoint, std::function<void()>>> m_tasks;

    /// Flag indicating whether there is an ongoing timer.
    bool m_isRunning;

    /// Flag indicating whether object is being destructed.
    bool m_isBeingDestroyed;

    /// The next token available.
    Token m_nextToken;
};

}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_MULTITIMER_H_
