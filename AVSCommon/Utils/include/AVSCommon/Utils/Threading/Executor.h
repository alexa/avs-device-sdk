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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_THREADING_EXECUTOR_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_THREADING_EXECUTOR_H_

#include <atomic>
#include <condition_variable>
#include <chrono>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <utility>

#include <AVSCommon/Utils/Threading/ExecutorInterface.h>
#include <AVSCommon/Utils/Threading/TaskThread.h>
#include <AVSCommon/Utils/Power/PowerResource.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace threading {

/**
 * @brief Single-thread executor implementation.
 *
 * An Executor is used to run callable types asynchronously.
 *
 * This type is a wrapper around ExecutorInterface implementation.
 */
class Executor {
public:
    /**
     * Constructs an Executor.
     */
    Executor() noexcept;

    /**
     * Constructs an Executor.
     *
     * @param unused Unused parameter.
     *
     * @deprecated This method is kept for backwards compatibility.
     */
    Executor(const std::chrono::milliseconds& unused) noexcept;

    /**
     * Destructs an Executor.
     */
    ~Executor() noexcept;

    /**
     * @brief Schedules a function for execution.
     *
     * Submits a function to be executed on an Executor thread.
     *
     * @param[in] function Function to execute. Function must not be empty.
     * @return True if @a function is accepted for execution, false if @a function is empty or executor is shutdown.
     */
    bool execute(std::function<void()>&& function) noexcept;

    /**
     * @brief Schedules a function for execution.
     *
     * Submits a function to be executed on an Executor thread.
     *
     * @param[in] function Function to execute.
     * @return True if @a function is accepted for execution, false if @a function is empty or executor is shutdown.
     */
    bool execute(const std::function<void()>& function) noexcept;

    /**
     * Submits a callable type (function, lambda expression, bind expression, or another function object) to be executed
     * on an Executor thread. The future must be checked for validity before waiting on it.
     *
     * @tparam Task Callable type.
     * @tparam Args Argument types.
     *
     * @param task A callable type representing a task.
     * @param args The arguments to call the task with.
     * @return A @c std::future for the return value of the task.
     *
     * @note This method is less memory and speed efficient then #execute() and should not be used unless std::future
     *       result is required.
     */
    template <typename Task, typename... Args>
    auto submit(Task task, Args&&... args) noexcept -> std::future<decltype(task(args...))>;

    /**
     * Submits a callable type (function, lambda expression, bind expression, or another function object) to the front
     * of the internal queue to be executed on an Executor thread. The future must be checked for validity before
     * waiting on it.
     *
     * @tparam Task Callable type.
     * @tparam Args Argument types.
     *
     * @param task A callable type representing a task.
     * @param args The arguments to call the task with.
     * @return A @c std::future for the return value of the task.
     *
     * @note This method is less memory and speed efficient then #execute() and should not be used unless std::future
     *       result is required.
     */
    template <typename Task, typename... Args>
    auto submitToFront(Task task, Args&&... args) noexcept -> std::future<decltype(task(args...))>;

    /**
     * Waits for any previously submitted tasks to complete.
     */
    void waitForSubmittedTasks() noexcept;

    /// Clears the executor of outstanding tasks and refuses any additional tasks to be submitted.
    void shutdown() noexcept;

    /// Returns whether or not the executor is shutdown.
    bool isShutdown() noexcept;

    /**
     * @brief Provides access to ExecutorInterface reference.
     *
     * @return Reference to internal ExecutorInterface.
     */
    operator std::shared_ptr<ExecutorInterface>() const noexcept;

private:
    // Friend declaration.
    friend class SharedExecutor;

    /**
     * @brief Ordering hint when submitting a new task to executor.
     */
    enum class QueuePosition {
        /// Add task to front of task queue.
        Front = 1,
        /// Add task to back of task queue.
        Back
    };

    /**
     * @brief Schedules a function for execution.
     *
     * Submits a function to be executed on an Executor thread.
     *
     * @param[in] function Function to execute.
     * @param[in] queuePosition Position in the queue for the new task.
     * @return True on success, false on error: if @a function is empty, @a queuePosition is not valid, or executor is
     *         in shutdown.
     */
    bool execute(std::function<void()>&& function, QueuePosition queuePosition) noexcept;

    /**
     * Pushes a task on the the queue. If the queue is shutdown, the task will be dropped, and an invalid
     * future will be returned.
     *
     * @param queuePosition Position in the queue for the new task.
     * @param task A task to push to the front or back of the queue.
     * @param args The arguments to call the task with.
     * @returns A @c std::future to access the return value of the task. If the queue is shutdown, the task will be
     *     dropped, and an invalid future will be returned.
     */
    template <typename Task, typename... Args>
    auto pushTo(QueuePosition queuePosition, Task&& task, Args&&... args) noexcept
        -> std::future<decltype(task(args...))>;

    /**
     * Pushes a function on the the queue. If the queue is shutdown, the function will be dropped, and an invalid
     * future will be returned.
     *
     * @tparam T Return type for @a function and resulting future value type.
     * @param queuePosition Position in the queue for the new task.
     * @param function A function to push.
     * @returns A @c std::future to access the return value of the task. If the queue is shutdown, the task will be
     *     dropped, and an invalid future will be returned.
     */
    template <typename T>
    std::future<T> pushFunction(QueuePosition queuePosition, std::function<T()>&& function) noexcept;

    /// Internal shared executor reference.
    std::shared_ptr<class SharedExecutor> m_executor;
};

inline Executor::Executor(const std::chrono::milliseconds&) noexcept : Executor() {
}

template <typename Task, typename... Args>
auto Executor::submit(Task task, Args&&... args) noexcept -> std::future<decltype(task(args...))> {
    return pushTo(QueuePosition::Back, std::forward<Task>(task), std::forward<Args>(args)...);
}

template <typename Task, typename... Args>
auto Executor::submitToFront(Task task, Args&&... args) noexcept -> std::future<decltype(task(args...))> {
    return pushTo(QueuePosition::Front, std::forward<Task>(task), std::forward<Args>(args)...);
}

/**
 * Utility function which waits for a @c std::future to be fulfilled and forward the result to a @c std::promise.
 *
 * @param promise The @c std::promise to fulfill when @c future is fulfilled.
 * @param future The @c std::future on which to wait for a result to forward to @c promise.
 */
template <typename T>
inline static void forwardPromise(std::promise<T>& promise, std::future<T>& future) noexcept {
#if __cpp_exceptions || defined(__EXCEPTIONS)
    try {
#endif
        promise.set_value(future.get());
#if __cpp_exceptions || defined(__EXCEPTIONS)
    } catch (...) {
        promise.set_exception(std::current_exception());
    }
#endif
}

/**
 * Specialization of @c forwardPromise() for @c void types.
 *
 * @param promise The @c std::promise to fulfill when @c future is fulfilled.
 * @param future The @c std::future on which to wait before fulfilling @c promise.
 */
template <>
inline void forwardPromise<void>(std::promise<void>& promise, std::future<void>& future) noexcept {
#if __cpp_exceptions || defined(__EXCEPTIONS)
    try {
#endif
        future.get();
        promise.set_value();
#if __cpp_exceptions || defined(__EXCEPTIONS)
    } catch (...) {
        promise.set_exception(std::current_exception());
    }
#endif
}

template <typename Task, typename... Args>
inline auto Executor::pushTo(QueuePosition queuePosition, Task&& task, Args&&... args) noexcept
    -> std::future<decltype(task(args...))> {
    using ValueType = decltype(task(args...));
    // Remove arguments from the tasks type by binding the arguments to the task.
    std::function<ValueType()> fn{std::bind(std::forward<Task>(task), std::forward<Args>(args)...)};
    return pushFunction(queuePosition, std::move(fn));
}

template <typename T>
std::future<T> Executor::pushFunction(QueuePosition queuePosition, std::function<T()>&& function) noexcept {
    /*
     * Create a std::packaged_task with the correct return type.
     *
     * Note: A std::packaged_task fulfills its future *during* the call to operator().  If the user of a
     * std::packaged_task hands it off to another thread to execute, and then waits on the future, they will be able to
     * retrieve the return value from the task and know that the task has executed, but they do not know exactly when
     * the task object has been deleted.  This distinction can be significant if the packaged task is holding onto
     * resources that need to be freed (through a std::shared_ptr for example).  If the user needs to wait for those
     * resources to be freed they have no way of knowing how long to wait.  The translated_task lambda below is a
     * workaround for this limitation.  It executes the packaged task, then disposes of it before passing the task's
     * return value back to the future that the user is waiting on.
     */

    /**
     * @brief Structure to carry parameters into lambda through shared pointer.
     * @private
     */
    struct CallCtx {
        /**
         * @brief Construct object and assigned function to packaged task.
         *
         * @param function Function to wrap into packaged task.
         */
        inline CallCtx(std::function<T()>&& function) : packagedTask{std::move(function)} {
        }

        /// Packaged task.
        std::packaged_task<T()> packagedTask;
        /// Promise for result forwarding.
        std::promise<T> cleanupPromise;
    };

    auto callCtx = std::make_shared<CallCtx>(std::move(function));

    // Remove the return type from the task by wrapping it in a lambda with no return value.
    auto translated_task = [callCtx]() mutable {
        // Execute the task.
        callCtx->packagedTask();
        // Note the future for the task's result.
        auto taskFuture = callCtx->packagedTask.get_future();
        // Clean up the task.
        callCtx->packagedTask.reset();
        auto cleanupPromise = std::move(callCtx->cleanupPromise);
        // Release parameters.
        callCtx.reset();
        // Forward the task's result to our cleanup promise/future.
        forwardPromise(cleanupPromise, taskFuture);
    };

    // Create a promise/future that we will fulfill when we have cleaned up the task.
    auto cleanupFuture = callCtx->cleanupPromise.get_future();

    // Release our local reference to packaged task so that the only remaining reference is inside the lambda.
    callCtx.reset();

    if (!execute(std::move(translated_task), queuePosition)) {
        return std::future<T>();
    }

    return cleanupFuture;
}

/// @name Externalize Executor::pushFunction() for common types.
/// @{
extern template std::future<void> Executor::pushFunction(
    QueuePosition queuePosition,
    std::function<void()>&& function) noexcept;
extern template std::future<bool> Executor::pushFunction(
    QueuePosition queuePosition,
    std::function<bool()>&& function) noexcept;
extern template std::future<std::string> Executor::pushFunction(
    QueuePosition queuePosition,
    std::function<std::string()>&& function) noexcept;
/// @}

}  // namespace threading
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_THREADING_EXECUTOR_H_
