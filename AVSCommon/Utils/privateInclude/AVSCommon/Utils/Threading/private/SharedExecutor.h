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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_PRIVATEINCLUDE_AVSCOMMON_UTILS_THREADING_PRIVATE_SHAREDEXECUTOR_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_PRIVATEINCLUDE_AVSCOMMON_UTILS_THREADING_PRIVATE_SHAREDEXECUTOR_H_

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
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/Threading/TaskThread.h>
#include <AVSCommon/Utils/Power/PowerResource.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace threading {

/**
 * @brief Shared executor implementation.
 *
 * This implementation is managed by std::shared_ptr<ExecutorInterface>.
 */
class SharedExecutor : public virtual ExecutorInterface {
public:
    /**
     * @brief Constructs an object.
     */
    SharedExecutor() noexcept;

    /**
     * @brief Destructs an Executor.
     *
     * This method awaits till all running tasks are completed, and drops all enqueued tasks that haven't started
     * execution.
     *
     * @see #shutdown()
     * @see #waitForSubmittedTasks()
     */
    ~SharedExecutor() noexcept;

    /// @name Methods from ExecutorInterface.
    /// @{
    std::error_condition execute(std::function<void()>&& function) noexcept override;
    std::error_condition execute(const std::function<void()>& function) noexcept override;
    /// @}

    /**
     * Waits for any previously submitted tasks to complete.
     */
    void waitForSubmittedTasks() noexcept;

    /// Clears the executor of outstanding tasks and refuses any additional tasks to be submitted.
    void shutdown() noexcept;

    /**
     * @brief Returns whether or not the executor is shutdown.
     *
     * @return True if Executor::shutdown() has been called.
     */
    bool isShutdown() noexcept;

private:
    // Friend declaration.
    friend class Executor;

    /// Alias for queue position.
    using QueuePosition = Executor::QueuePosition;

    /// The queue type to use for holding tasks.
    using Queue = std::deque<std::function<void()>>;

    /**
     * Executes the next job in the queue.
     *
     * @return @c true if there's a next job; @c false if the job queue is empty.
     */
    bool runNext() noexcept;

    /**
     * Checks if the job queue is empty and that no job is added in the grace period determined by @c m_timeout.
     *
     * @return @c true if there's at least one job left in the queue; @c false if the job queue is empty.
     */
    bool hasNext() noexcept;

    /**
     * Returns and removes the task at the front of the queue. If there are no tasks, this call will return an empty
     * function.
     *
     * @returns A function that represents a new task. The function will be empty if the queue has no job.
     */
    std::function<void()> pop() noexcept;

    /**
     * @brief Schedules a function for execution.
     *
     * Submits a function to be executed on an Executor thread.
     *
     * @param[in] function Function to execute.
     * @param[in] queuePosition Position in the queue for the new task.
     *
     * @return Portable error code. If successful, value is zero.
     */
    std::error_condition execute(std::function<void()>&& function, QueuePosition queuePosition) noexcept;

    /// Moniker for executed tasks.
    /// Whenever executor runs a task, it guarantees that task's thread has this value set with @c ThreadMoniker.
    const std::string m_executorMoniker;

    /// The queue of tasks
    Queue m_queue;

    /// Flag to indicate if the taskThread already have an executing job.
    bool m_threadRunning;

    /// A mutex to protect access to the tasks in m_queue.
    std::mutex m_queueMutex;

    /// A flag for whether or not the queue is expecting more tasks.
    std::atomic_bool m_shutdown;

    /// A @c PowerResource.
    std::shared_ptr<power::PowerResource> m_powerResource;

    /// The thread to execute tasks on. The thread must be declared last to be destructed first.
    TaskThread m_taskThread;
};

}  // namespace threading
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_PRIVATEINCLUDE_AVSCOMMON_UTILS_THREADING_PRIVATE_SHAREDEXECUTOR_H_
