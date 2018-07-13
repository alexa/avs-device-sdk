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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_TIMER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_TIMER_H_

#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <thread>

#include "AVSCommon/Utils/Logger/LoggerUtils.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {

/**
 * A @c Timer is used to schedule a callable type to run in the future.
 */
class Timer {
public:
    /**
     * Value for @c start()'s @c maxCount parameter which indicates that the @c Timer should continue firing
     * indefinitely.
     */
    static const size_t FOREVER = 0;

    /// Specifies different ways to apply the period of a recurring task.
    enum class PeriodType {
        /**
         * The period specifies the time from the start of one task call to the start of the next task call.  This
         * period type ensures task calls occur on a predictable cadence.
         *
         * @note A timer makes one task call at a time, so if a task call takes more than one period to execute,
         *     the subsequent calls which would have occured while the task was still executing will be skipped, and
         *     the next call will not occur until the next period-multiple after the original task call completes.
         */
        ABSOLUTE,

        /**
         * The period specifies the time from the end of one task call to the start of the next task call.  This period
         * type ensures a specific amount of idle time between task calls.
         */
        RELATIVE
    };

    /**
     * Contructs a @c Timer.
     */
    Timer();

    /**
     * Destructs a @c Timer.
     */
    ~Timer();

    /**
     * Submits a callable type (function, lambda expression, bind expression, or another function object) to be
     * executed after an initial delay, and then called repeatedly on a fixed time schedule.  A @c Timer instance
     * manages only one running @c Timer at a time; calling @c start() on an already-running @c Timer will fail.
     * Note that the template parameters are typically inferred from the function paramters when calling @c start().
     *
     * @tparam Rep A type for measuring 'ticks' in a generic @c std::chrono::duration.
     * @tparam Period A type for representing the number of ticks per second in a generic @c std::chrono::duration.
     * @tparam Task The type of task to execute.
     * @tparam Args The argument types for the task to execute.
     *
     * @param delay The non-negative time to wait before making the first task call.  Negative values will cause this
     *    function to return false.
     * @param period The non-negative time to wait between subsequent task calls.  Negative values will cause this
     *    function to return false.
     * @param periodType The type of period to use when making subsequent task calls.
     * @param maxCount The desired number of times to call task.  @c Timer::FOREVER means to call forever until
     *     @c stop() is called.  Note that fewer than @c maxCount calls may occur if @c periodType is
     *     @c PeriodType::ABSOLUTE and the task runtime exceeds @c period.
     * @param task A callable type representing a task.
     * @param args The arguments to call the task with.
     * @returns @c true if the timer started, else @c false.
     */
    template <typename Rep, typename Period, typename Task, typename... Args>
    bool start(
        const std::chrono::duration<Rep, Period>& delay,
        const std::chrono::duration<Rep, Period>& period,
        PeriodType periodType,
        size_t maxCount,
        Task task,
        Args&&... args);

    /**
     * Submits a callable type (function, lambda expression, bind expression, or another function object) to be
     * executed repeatedly on a fixed time schedule.  A @c Timer instance manages only one running @c Timer at a time;
     * calling @c start() on an already-running @c Timer will fail.  Note that the template parameters are typically
     * inferred from the function paramters when calling @c start().
     *
     * @tparam Rep A type for measuring 'ticks' in a generic @c std::chrono::duration.
     * @tparam Period A type for representing the number of ticks per second in a generic @c std::chrono::duration.
     * @tparam Task The type of task to execute.
     * @tparam Args The argument types for the task to execute.
     *
     * @param period The non-negative time to wait before each @c task call.  Negative values will cause this
     *    function to return false.
     * @param periodType The type of period to use when making subsequent task calls.
     * @param maxCount The desired number of times to call task.  @c Timer::FOREVER means to call forever until
     *     @c stop() is called.  Note that fewer than @c maxCount calls may occur if @c periodType is
     *     @c PeriodType::ABSOLUTE and the task runtime exceeds @c period.
     * @param task A callable type representing a task.
     * @param args The arguments to call @c task with.
     * @returns @c true if the timer started, else @c false.
     */
    template <typename Rep, typename Period, typename Task, typename... Args>
    bool start(
        const std::chrono::duration<Rep, Period>& period,
        PeriodType periodType,
        size_t maxCount,
        Task task,
        Args&&... args);

    /**
     * Submits a callable type (function, lambda expression, bind expression, or another function object) to be
     * executed once, after the specified duration.  A @c Timer instance manages only one running @c Timer at a time;
     * calling @c start() on an already-running @c Timer will fail.  Note that the template parameters are typically
     * inferred from the function paramters when calling @c start().
     *
     * @tparam Rep A type for measuring 'ticks' in a generic @c std::chrono::duration.
     * @tparam Period A type for representing the number of ticks per second in a generic @c std::chrono::duration.
     * @tparam Task The type of task to execute.
     * @tparam Args The argument types for the task to execute.
     *
     * @param delay The non-negative time to wait before calling @c task.  Negative values will cause this
     *    function to return an invalid future.
     * @param task A callable type representing a task.
     * @param args The arguments to call @c task with.
     * @returns A valid @c std::future for the return value of @c task if the timer started, else an invalid
     *     @c std::future.  Note that the promise will be broken if @c stop() is called before @c task is called.
     */
    template <typename Rep, typename Period, typename Task, typename... Args>
    auto start(const std::chrono::duration<Rep, Period>& delay, Task task, Args&&... args)
        -> std::future<decltype(task(args...))>;

    /**
     * Stops the @c Timer (if running).  This will not interrupt an active call to the task, but will prevent any
     * subequent calls to the task.  If @c stop() is called while the task is executing, this function will block until
     * the task completes.
     *
     * @note In the special case that @c stop() is called from inside the task function, @c stop() will still prevent
     *     any subsequent calls to the task, but will *not* block as described above.
     */
    void stop();

    /**
     * Reports whether the @c Timer is active.  A timer is considered active if it is waiting to start a call to the
     * task, or if a call to the task is in progress.  A timer is only considered inactive if it has not been started,
     * if all requested/scheduled calls to the task have completed, or after a call to @c stop().
     *
     * @returns @c true if the @c Timer is active, else @c false.
     */
    bool isActive() const;

private:
    /**
     * Atomically activates this @c Timer (by setting @c m_running).
     *
     * @returns @c true if the @c Timer was previously inactive, else @c false.
     */
    bool activate();

    /**
     * Waits for the @c Timer @c period, then calls @c task.
     *
     * @tparam Rep A type for measuring 'ticks' in a generic @c std::chrono::duration.
     * @tparam Period A type for representing the number of ticks per second in a generic @c std::chrono::duration.
     *
     * @param delay The non-negative time to wait before making the first @c task call.
     * @param period The non-negative time to wait between subsequent @c task calls.
     * @param periodType The type of period to use when making subsequent task calls.
     * @param maxCount The desired number of times to call task.  @c Timer::FOREVER means to call forever until
     *     @c stop() is called.  Note that fewer than @c maxCount calls may occur if @c periodType is
     *     @c PeriodType::ABSOLUTE and the task runtime exceeds @c period.
     * @param task A callable type representing a task.
     */
    template <typename Rep, typename Period>
    void callTask(
        std::chrono::duration<Rep, Period> delay,
        std::chrono::duration<Rep, Period> period,
        PeriodType periodType,
        size_t maxCount,
        std::function<void()> task);

    /**
     * The tag associated with log entries from this class.
     */
    static const std::string TAG;

    /// The condition variable used to wait for @c stop() or period timeouts.
    std::condition_variable m_waitCondition;

    /// The mutex for @c m_waitCondition.
    std::mutex m_waitMutex;

    /// The thread to execute tasks on.
    std::thread m_thread;

    /// Flag which indicates that a @c Timer is active.
    std::atomic<bool> m_running;

    /**
     * Flag which requests that the active @c Timer be stopped.  @c m_waitMutex must be locked when modifying this
     * variable.
     */
    bool m_stopping;
};

template <typename Rep, typename Period, typename Task, typename... Args>
bool Timer::start(
    const std::chrono::duration<Rep, Period>& delay,
    const std::chrono::duration<Rep, Period>& period,
    PeriodType periodType,
    size_t maxCount,
    Task task,
    Args&&... args) {
    if (delay < std::chrono::duration<Rep, Period>::zero()) {
        logger::acsdkError(logger::LogEntry(TAG, "startFailed").d("reason", "negativeDelay"));
        return false;
    }
    if (period < std::chrono::duration<Rep, Period>::zero()) {
        logger::acsdkError(logger::LogEntry(TAG, "startFailed").d("reason", "negativePeriod"));
        return false;
    }

    // Can't start if already running.
    if (!activate()) {
        logger::acsdkError(logger::LogEntry(TAG, "startFailed").d("reason", "timerAlreadyActive"));
        return false;
    }

    // Join old timer thread (if any).
    if (m_thread.joinable()) {
        m_thread.join();
    }

    // Remove arguments from the task's type by binding the arguments to the task.
    using BoundTaskType = decltype(std::bind(std::forward<Task>(task), std::forward<Args>(args)...));
    auto boundTask = std::make_shared<BoundTaskType>(std::bind(std::forward<Task>(task), std::forward<Args>(args)...));

    // Remove the return type from the task by wrapping it in a lambda with no return value.
    auto translatedTask = [boundTask]() { boundTask->operator()(); };

    // Kick off the new timer thread.
    m_thread = std::thread{
        std::bind(&Timer::callTask<Rep, Period>, this, delay, period, periodType, maxCount, translatedTask)};

    return true;
}

template <typename Rep, typename Period, typename Task, typename... Args>
bool Timer::start(
    const std::chrono::duration<Rep, Period>& period,
    PeriodType periodType,
    size_t maxCount,
    Task task,
    Args&&... args) {
    return start(period, period, periodType, maxCount, std::forward<Task>(task), std::forward<Args>(args)...);
}

template <typename Rep, typename Period, typename Task, typename... Args>
auto Timer::start(const std::chrono::duration<Rep, Period>& delay, Task task, Args&&... args)
    -> std::future<decltype(task(args...))> {
    // Can't start if already running.
    if (!activate()) {
        logger::acsdkError(logger::LogEntry(TAG, "startFailed").d("reason", "timerAlreadyActive"));
        using FutureType = decltype(task(args...));
        return std::future<FutureType>();
    }

    // Join old timer thread (if any).
    if (m_thread.joinable()) {
        m_thread.join();
    }

    // Remove arguments from the task's type by binding the arguments to the task.
    auto boundTask = std::bind(std::forward<Task>(task), std::forward<Args>(args)...);

    /*
     * Create a std::packaged_task with the correct return type. The decltype only returns the return value of the
     * boundTask. The following parentheses make it a function call with the boundTask return type. The package task
     * will then return a future of the correct type.
     */
    using PackagedTaskType = std::packaged_task<decltype(boundTask())()>;
    auto packagedTask = std::make_shared<PackagedTaskType>(boundTask);

    // Remove the return type from the task by wrapping it in a lambda with no return value.
    auto translatedTask = [packagedTask]() { packagedTask->operator()(); };

    // Kick off the new timer thread.
    static const size_t once = 1;
    m_thread = std::thread{
        std::bind(&Timer::callTask<Rep, Period>, this, delay, delay, PeriodType::ABSOLUTE, once, translatedTask)};

    return packagedTask->get_future();
}

template <typename Rep, typename Period>
void Timer::callTask(
    std::chrono::duration<Rep, Period> delay,
    std::chrono::duration<Rep, Period> period,
    PeriodType periodType,
    size_t maxCount,
    std::function<void()> task) {
    // Timepoint to measure delay/period against.
    auto now = std::chrono::steady_clock::now();

    // Flag indicating whether we've drifted off schedule.
    bool offSchedule = false;

    for (size_t count = 0; maxCount == FOREVER || count < maxCount; ++count) {
        auto waitTime = (0 == count) ? delay : period;
        {
            std::unique_lock<std::mutex> lock(m_waitMutex);

            // Wait for stop() or a delay/period to elapse.
            if (m_waitCondition.wait_until(lock, now + waitTime, [this]() { return m_stopping; })) {
                m_stopping = false;
                m_running = false;
                return;
            }
        }

        switch (periodType) {
            case PeriodType::ABSOLUTE:
                // Update our estimate of where we should be after the delay.
                now += waitTime;

                // Run the task if we're still on schedule.
                if (!offSchedule) {
                    task();
                }

                // If the task runtime put us off schedule, skip the next task run.
                if (now + period < std::chrono::steady_clock::now()) {
                    offSchedule = true;
                } else {
                    offSchedule = false;
                }
                break;

            case PeriodType::RELATIVE:
                task();
                now = std::chrono::steady_clock::now();
                break;
        }
    }
    m_stopping = false;
    m_running = false;
}

}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_TIMER_H_
