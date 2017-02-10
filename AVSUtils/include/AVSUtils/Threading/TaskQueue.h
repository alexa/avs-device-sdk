/**
 * TaskQueue.h
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSUTILS_INCLUDE_AVSUTILS_THREADING_TASK_QUEUE_H_
#define ALEXA_CLIENT_SDK_AVSUTILS_INCLUDE_AVSUTILS_THREADING_TASK_QUEUE_H_

#include <condition_variable>
#include <deque>
#include <future>
#include <mutex>

namespace alexaClientSDK {
namespace avsUtils {
namespace threading {

/**
 * A TaskQueue contains a queue of tasks to run
 */
class TaskQueue {
public:
    /**
     * Constructs an empty TaskQueue.
     */
    TaskQueue();

    /**
     * Pushes a task on the back of the queue. If the queue is shutdown, the task will be a dropped, and an invalid
     * future will be returned.
     *
     * @param task A task to push to the back of the queue.
     * @param args The arguments to call the task with.
     * @returns A @c std::future to access the return value of the task. If the queue is shutdown, the task will be a
     *     dropped, and an invalid future will be returned.
     */
    template<typename Task, typename... Args>
    auto push(Task task, Args &&... args) -> std::future<decltype(task(args...))>;

    /**
     * Returns and removes the task at the front of the queue. If there are no tasks, this call will block until there
     * is one. A @c nullptr will be returned if there are no more tasks expected.
     *
     * @returns A task which the caller assumes ownership of, or @c nullptr if the TaskQueue expects no more tasks.
     */
    std::unique_ptr<std::function<void()>> pop();

    /**
     * Clears the queue of outstanding tasks and refuses any additional tasks to be pushed onto the queue.
     *
     * Must be called by task enqueuers when no more tasks will be enqueued.
     */
    void shutdown();

    /**
     * Returns whether or not the queue is shutdown.
     *
     * @returns Whether or not the queue is shutdown.
     */
    bool isShutdown();

private:
    /// The queue of tasks
    std::deque<std::unique_ptr<std::function<void()>>> m_queue;

    /// A condition variable to wait for new tasks to be placed on the queue.
    std::condition_variable m_queueChanged;

    /// A mutex to protect access to the tasks in m_queue.
    std::mutex m_queueMutex;

    /// A flag for whether or not the queue is expecting more tasks.
    std::atomic_bool m_shutdown;
};

template<typename Task, typename... Args>
auto TaskQueue::push(Task task, Args &&... args) -> std::future<decltype(task(args...))> {
    // Remove arguments from the tasks type by binding the arguments to the task.
    auto boundTask = std::bind(std::forward<Task>(task), std::forward<Args>(args)...);

    /*
     * Create a std::packaged_task with the correct return type. The decltype only returns the return value of the
     * boundTask. The following parentheses make it a function call with the boundTask return type. The package task
     * will then return a future of the correct type.
     */
    using PackagedTaskType = std::packaged_task<decltype(boundTask())()>;
    auto packaged_task = std::make_shared<PackagedTaskType>(boundTask);

    // Remove the return type from the task by wrapping it in a lambda with no return value.
    auto translated_task = [packaged_task]() { packaged_task->operator()(); };

    {
        std::lock_guard<std::mutex> queueLock{m_queueMutex};
        if (!m_shutdown) {
            m_queue.emplace_back(new std::function<void()>(translated_task));
        } else {
            using FutureType = decltype(task(args...));
            return std::future<FutureType>();
        }
    }

    m_queueChanged.notify_all();
    return packaged_task->get_future();
}

} // namespace threading
} // namespace avsUtils
} // namespace alexaClientSDK

#endif //ALEXA_CLIENT_SDK_AVSUTILS_INCLUDE_AVSUTILS_THREADING_TASK_QUEUE_H_

