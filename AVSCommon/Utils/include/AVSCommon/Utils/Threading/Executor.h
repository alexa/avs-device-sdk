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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_THREADING_EXECUTOR_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_THREADING_EXECUTOR_H_

#include <future>
#include <utility>

#include "AVSCommon/Utils/Threading/TaskThread.h"
#include "AVSCommon/Utils/Threading/TaskQueue.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace threading {

/**
 * An Executor is used to run callable types asynchronously.
 */
class Executor {
public:
    /**
     * Constructs an Executor.
     */
    Executor();

    /**
     * Destructs an Executor.
     */
    ~Executor();

    /**
     * Submits a callable type (function, lambda expression, bind expression, or another function object) to be executed
     * on an Executor thread. The future must be checked for validity before waiting on it.
     *
     * @param task A callable type representing a task.
     * @param args The arguments to call the task with.
     * @returns A @c std::future for the return value of the task.
     */
    template <typename Task, typename... Args>
    auto submit(Task task, Args&&... args) -> std::future<decltype(task(args...))>;

    /**
     * Submits a callable type (function, lambda expression, bind expression, or another function object) to the front
     * of the internal queue to be executed on an Executor thread. The future must be checked for validity before
     * waiting on it.
     *
     * @param task A callable type representing a task.
     * @param args The arguments to call the task with.
     * @returns A @c std::future for the return value of the task.
     */
    template <typename Task, typename... Args>
    auto submitToFront(Task task, Args&&... args) -> std::future<decltype(task(args...))>;

    /**
     * Wait for any previously submitted tasks to complete.
     */
    void waitForSubmittedTasks();

    /// Clears the executor of outstanding tasks and refuses any additional tasks to be submitted.
    void shutdown();

    /// Returns whether or not the executor is shutdown.
    bool isShutdown();

private:
    /// The queue of tasks to execute.
    std::shared_ptr<TaskQueue> m_taskQueue;

    /// The thread to execute tasks on. The thread must be declared last to be destructed first.
    std::unique_ptr<TaskThread> m_taskThread;
};

template <typename Task, typename... Args>
auto Executor::submit(Task task, Args&&... args) -> std::future<decltype(task(args...))> {
    return m_taskQueue->push(task, std::forward<Args>(args)...);
}

template <typename Task, typename... Args>
auto Executor::submitToFront(Task task, Args&&... args) -> std::future<decltype(task(args...))> {
    return m_taskQueue->pushToFront(task, std::forward<Args>(args)...);
}

}  // namespace threading
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_THREADING_EXECUTOR_H_
