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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_TIMER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_TIMER_H_

#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <thread>

#include <AVSCommon/AVS/Initialization/SDKPrimitivesProvider.h>
#include <AVSCommon/Utils/Timing/TimerDelegate.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {

/**
 * @brief Timer to schedule task for delayed and periodic execution.
 *
 * A @c Timer is used to schedule a callable type to run in the future.
 */
class Timer {
public:
    /**
     * Value for @c start()'s @c maxCount parameter which indicates that the @c Timer should continue firing
     * indefinitely.
     */
    static constexpr size_t FOREVER = 0;

    /// Specifies different ways to apply the period of a recurring task.
    enum class PeriodType {
        /**
         * The period specifies the time from the start of one task call to the start of the next task call.  This
         * period type ensures task calls occur on a predictable cadence.
         *
         * @note A timer makes one task call at a time, so if a task call takes more than one period to execute,
         *       the subsequent calls which would have occured while the task was still executing will be skipped, and
         *       the next call will not occur until the next period-multiple after the original task call completes.
         */
        ABSOLUTE,

        /**
         * The period specifies the time from the end of one task call to the start of the next task call.  This period
         * type ensures a specific amount of idle time between task calls.
         */
        RELATIVE
    };

    /**
     * @brief Constructs a @c Timer.
     *
     * @param[in] timerDelegateFactory A factory to create the @c TimerDelegate. By if the parameter is nullptr,
     *                                 implementation will use TimerDelegateFactory.
     */
    Timer(
        const std::shared_ptr<sdkInterfaces::timing::TimerDelegateFactoryInterface>& timerDelegateFactory =
            avs::initialization::SDKPrimitivesProvider::getInstance()->getTimerDelegateFactory()) noexcept;

    /**
     * @brief Destructs a @c Timer.
     */
    ~Timer() noexcept;

    /// Static member function to get FOREVER.
    static inline size_t getForever() noexcept {
        return FOREVER;
    }

    /**
     * @brief Submit a callable type for periodic execution.
     *
     * Submits a callable type (function, lambda expression, bind expression, or another function object) to be
     * executed after an initial delay, and then called repeatedly on a fixed time schedule.  A @c Timer instance
     * manages only one running @c Timer at a time; calling @c start() on an already-running @c Timer will fail.
     * Note that the template parameters are typically inferred from the function paramters when calling @c start().
     *
     * @tparam Delay    Initial delay type. This can be any specialization of std::chrono::duration convertible to
     *                  std::chrono::nanoseconds.
     * @tparam Period   Period type. This can be any specialization of std::chrono::duration convertible to
     *                  std::chrono::nanoseconds.
     * @tparam Task     The type of task to execute.
     * @tparam Args     The argument types for the task to execute.
     *
     * @param[in] delay      The non-negative time to wait before making the first task call. Negative values will cause
     *                       this function to return false.
     * @param[in] period     The non-negative time to wait between subsequent task calls. Negative values will cause
     *                       this function to return false.
     * @param[in] periodType The type of period to use when making subsequent task calls.
     * @param[in] maxCount   The desired number of times to call task.  Timer::getForever() means to call forever until
     *                       Timer::stop() is called. Note that fewer than @a maxCount calls may occur if @a periodType
     *                       is PeriodType::ABSOLUTE and the task runtime exceeds @a period.
     * @param[in] task       A callable type representing a task.
     * @param[in] args       The arguments to call the task with.
     *
     * @returns @c true if the timer started, else @c false.
     */
    template <typename Delay, typename Period, typename Task, typename... Args>
    bool start(
        Delay&& delay,
        Period&& period,
        PeriodType periodType,
        size_t maxCount,
        Task&& task,
        Args&&... args) noexcept;

    /**
     * @brief Submit a callable type for periodic execution.
     *
     * Submits a callable type (function, lambda expression, bind expression, or another function object) to be
     * executed repeatedly on a fixed time schedule.  A @c Timer instance manages only one running @c Timer at a time;
     * calling @c start() on an already-running @c Timer will fail.  Note that the template parameters are typically
     * inferred from the function paramters when calling @c start().
     *
     * @tparam Period   Period and initial delay type. This can be any specialization of std::chrono::duration
     *                  convertible to std::chrono::nanoseconds.
     * @tparam Task     The type of task to execute. This must be callable with @a args parameters.
     * @tparam Args     The argument types for the task to execute.
     *
     * @param[in] period     The non-negative time to wait before each @a task call.  Negative values will cause this
     *                       function to return false.
     * @param[in] periodType The type of period to use when making subsequent task calls.
     * @param[in] maxCount   The desired number of times to call task.  Timer::getForever() means to call forever until
     *                       Timer::stop() is called.  Note that fewer than @a maxCount calls may occur if @a periodType
     *                       is PeriodType::ABSOLUTE and the task runtime exceeds @a period.
     * @param[in] task       A callable type representing a task.
     * @param[in] args       The arguments to call @a task with.
     *
     * @returns @c true if the timer started, else @c false.
     */
    template <typename Period, typename Task, typename... Args>
    bool start(Period&& period, PeriodType periodType, size_t maxCount, Task&& task, Args&&... args) noexcept;

    /**
     * @brief Submit a callable type for single execution with future result.
     *
     * Submits a callable type (function, lambda expression, bind expression, or another function object) to be
     * executed once, after the specified duration.  A @c Timer instance manages only one running @c Timer at a time;
     * calling @c start() on an already-running @c Timer will fail.  Note that the template parameters are typically
     * inferred from the function parameters when calling @c start().
     *
     * @tparam Delay    Delay type. This can be any specialization of std::chrono::duration convertible to
     *                  std::chrono::nanoseconds.
     * @tparam Task     The type of task to execute.
     * @tparam Args     The argument types for the task to execute.
     *
     * @param[in] delay The non-negative time to wait before calling @a task. Negative values will cause this function
     *                  to return an invalid future.
     * @param[in] task  A callable type representing a task.
     * @param[in] args  The arguments to call @c task with.
     *
     * @returns A valid @c std::future for the return value of @c task if the timer started, else an invalid @c
     *          std::future.  Note that the promise will be broken if Timer::stop() is called before @a task is called.
     */
    template <typename Delay, typename Task, typename... Args>
    auto start(Delay&& delay, Task&& task, Args&&... args) noexcept -> std::future<decltype(task(args...))>;

    /**
     * @brief Stop the timer.
     *
     * Stops the @c Timer (if running).  This will not interrupt an active call to the task, but will prevent any
     * subsequent calls to the task.  If @c stop() is called while the task is executing, this function will block until
     * the task completes.
     *
     * @note In the special case that @c stop() is called from inside the task function, @c stop() will still prevent
     *       any subsequent calls to the task, but will *not* block as described above.
     */
    void stop() noexcept;

    /**
     * @brief Check if timer is active.
     *
     * Reports whether the @c Timer is active.  A timer is considered active if it is waiting to start a call to the
     * task, or if a call to the task is in progress.  A timer is only considered inactive if it has not been started,
     * if all requested/scheduled calls to the task have completed, or after a call to @c stop().
     *
     * @returns @c true if the @c Timer is active, else @c false.
     */
    bool isActive() const noexcept;

private:
    /**
     * Value for Timer::start()'s @a maxCount parameter which indicates that the @c Timer should fire once.
     */
    static constexpr size_t ONCE = 1u;

    /**
     * Atomically activates this @c Timer (by setting @c m_running).
     *
     * @returns @c true if the @c Timer was previously inactive, else @c false.
     */
    bool activate() noexcept;

    /**
     * Waits for the @c Timer @c period, then calls @c task.
     *
     * @param[in] delayNano  The non-negative time to wait before making the first @a task call.
     * @param[in] periodNano The non-negative time to wait between subsequent @a task calls.
     * @param[in] periodType The type of period to use when making subsequent task calls.
     * @param[in] maxCount   The desired number of times to call task. Timer::getForever() means to call forever until
     *                       Timer::stop() is called. Note that fewer than @a maxCount calls may occur if @a periodType
     *                       is PeriodType::ABSOLUTE and the task runtime exceeds @a period.
     * @param[in] task       Function object to call. It must be not empty.
     */
    bool callTask(
        const std::chrono::nanoseconds& delayNano,
        const std::chrono::nanoseconds& periodNano,
        PeriodType periodType,
        size_t maxCount,
        std::function<void()>&& task) noexcept;

    /**
     * @brief Proxy method for convering delay and period to nanoseconds.
     *
     * @tparam Delay    Initial delay type. This can be any specialization of std::chrono::duration convertible to
     *                  std::chrono::nanoseconds.
     * @tparam Period   Period type. This can be any specialization of std::chrono::duration convertible to
     *                  std::chrono::nanoseconds.
     * @param delayNano
     * @param periodNano
     * @param periodType
     * @param maxCount
     * @param task
     * @return
     */
    template <typename Delay, typename Period>
    bool adaptTypesAndCallTask(
        const Delay& delayNano,
        const Period& periodNano,
        PeriodType periodType,
        size_t maxCount,
        std::function<void()>&& task) noexcept;

    /// The @c TimerDelegateInterface which contains timer related logic.
    std::unique_ptr<sdkInterfaces::timing::TimerDelegateInterface> m_timer;

    /// The thread to execute tasks on.
    std::thread m_thread;
};

template <typename Delay, typename Period, typename Task, typename... Args>
bool Timer::start(
    Delay&& delay,
    Period&& period,
    PeriodType periodType,
    size_t maxCount,
    Task&& task,
    Args&&... args) noexcept {
    // convert arguments to lambda
    auto boundTask = std::bind(std::forward<Task>(task), std::forward<Args>(args)...);

    // Kick off the new timer thread.
    return adaptTypesAndCallTask(
        std::forward<Delay>(delay), std::forward<Period>(period), periodType, maxCount, std::move(boundTask));
}

template <typename Period, typename Task, typename... Args>
bool Timer::start(Period&& period, PeriodType periodType, size_t maxCount, Task&& task, Args&&... args) noexcept {
    // convert arguments to lambda
    auto boundTask = std::bind(std::forward<Task>(task), std::forward<Args>(args)...);

    // Kick off the new timer thread.
    return adaptTypesAndCallTask(
        std::forward<Period>(period), std::forward<Period>(period), periodType, maxCount, std::move(boundTask));
}

template <typename Delay, typename Task, typename... Args>
auto Timer::start(Delay&& delay, Task&& task, Args&&... args) noexcept -> std::future<decltype(task(args...))> {
    // Task result value type.
    using Value = decltype(task(args...));

    // Remove arguments from the task's type by binding the arguments to the task.
    auto boundTask = std::bind(std::forward<Task>(task), std::forward<Args>(args)...);

    /*
     * Create a std::packaged_task with the correct return type. The reference will be passed into execution context
     * and will not be shared between threads.
     */
    auto packagedTask = std::make_shared<std::packaged_task<Value()>>(std::move(boundTask));
    // Get future object reference for function result.
    auto packagedTaskFuture = packagedTask->get_future();

    // Remove the return type from the task by wrapping it in a lambda with no return value.
    auto translatedTask = std::bind(&std::packaged_task<Value()>::operator(), std::move(packagedTask));

    // Kick off the new timer thread.
    if (!adaptTypesAndCallTask(
            std::forward<Delay>(delay),
            std::forward<Delay>(delay),
            PeriodType::ABSOLUTE,
            ONCE,
            std::move(translatedTask))) {
        return std::future<Value>();
    }

    return packagedTaskFuture;
}

template <typename Delay, typename Period>
bool Timer::adaptTypesAndCallTask(
    const Delay& delay,
    const Period& period,
    PeriodType periodType,
    size_t maxCount,
    std::function<void()>&& task) noexcept {
    return callTask(delay, period, periodType, maxCount, std::move(task));
}

// Externalize templates for common parameter types.
extern template bool Timer::adaptTypesAndCallTask(
    const std::chrono::milliseconds&,
    const std::chrono::milliseconds&,
    PeriodType,
    size_t,
    std::function<void()>&&) noexcept;
extern template bool Timer::adaptTypesAndCallTask(
    const std::chrono::nanoseconds&,
    const std::chrono::nanoseconds&,
    PeriodType,
    size_t,
    std::function<void()>&&) noexcept;

}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_TIMER_H_
