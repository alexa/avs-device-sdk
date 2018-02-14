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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_THREADING_TASKQUEUE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_THREADING_TASKQUEUE_H_

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <utility>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
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
     * Pushes a task on the back of the queue. If the queue is shutdown, the task will be dropped, and an invalid
     * future will be returned.
     *
     * @param task A task to push to the back of the queue.
     * @param args The arguments to call the task with.
     * @returns A @c std::future to access the return value of the task. If the queue is shutdown, the task will be
     *     dropped, and an invalid future will be returned.
     */
    template <typename Task, typename... Args>
    auto push(Task task, Args&&... args) -> std::future<decltype(task(args...))>;

    /**
     * Pushes a task on the front of the queue. If the queue is shutdown, the task will be dropped, and an invalid
     * future will be returned.
     *
     * @param task A task to push to the back of the queue.
     * @param args The arguments to call the task with.
     * @returns A @c std::future to access the return value of the task. If the queue is shutdown, the task will be
     *     dropped, and an invalid future will be returned.
     */
    template <typename Task, typename... Args>
    auto pushToFront(Task task, Args&&... args) -> std::future<decltype(task(args...))>;

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
    /// The queue type to use for holding tasks.
    using Queue = std::deque<std::unique_ptr<std::function<void()>>>;

    /**
     * Pushes a task on the the queue. If the queue is shutdown, the task will be dropped, and an invalid
     * future will be returned.
     *
     * @param front If @c true, push to the front of the queue, else push to the back.
     * @param task A task to push to the front or back of the queue.
     * @param args The arguments to call the task with.
     * @returns A @c std::future to access the return value of the task. If the queue is shutdown, the task will be
     *     dropped, and an invalid future will be returned.
     */
    template <typename Task, typename... Args>
    auto pushTo(bool front, Task task, Args&&... args) -> std::future<decltype(task(args...))>;

    /// The queue of tasks
    Queue m_queue;

    /// A condition variable to wait for new tasks to be placed on the queue.
    std::condition_variable m_queueChanged;

    /// A mutex to protect access to the tasks in m_queue.
    std::mutex m_queueMutex;

    /// A flag for whether or not the queue is expecting more tasks.
    std::atomic_bool m_shutdown;
};

template <typename Task, typename... Args>
auto TaskQueue::push(Task task, Args&&... args) -> std::future<decltype(task(args...))> {
    bool front = true;
    return pushTo(!front, std::forward<Task>(task), std::forward<Args>(args)...);
}

template <typename Task, typename... Args>
auto TaskQueue::pushToFront(Task task, Args&&... args) -> std::future<decltype(task(args...))> {
    bool front = true;
    return pushTo(front, std::forward<Task>(task), std::forward<Args>(args)...);
}

/**
 * Utility function which waits for a @c std::future to be fulfilled and forward the result to a @c std::promise.
 *
 * @param promise The @c std::promise to fulfill when @c future is fulfilled.
 * @param future The @c std::future on which to wait for a result to forward to @c promise.
 */
template <typename T>
inline static void forwardPromise(std::shared_ptr<std::promise<T>> promise, std::future<T>* future) {
    promise->set_value(future->get());
}

/**
 * Specialization of @c forwardPromise() for @c void types.
 *
 * @param promise The @c std::promise to fulfill when @c future is fulfilled.
 * @param future The @c std::future on which to wait before fulfilling @c promise.
 */
template <>
inline void forwardPromise<void>(std::shared_ptr<std::promise<void>> promise, std::future<void>* future) {
    future->get();
    promise->set_value();
}

template <typename Task, typename... Args>
auto TaskQueue::pushTo(bool front, Task task, Args&&... args) -> std::future<decltype(task(args...))> {
    // Remove arguments from the tasks type by binding the arguments to the task.
    auto boundTask = std::bind(std::forward<Task>(task), std::forward<Args>(args)...);

    /*
     * Create a std::packaged_task with the correct return type. The decltype only returns the return value of the
     * boundTask. The following parentheses make it a function call with the boundTask return type. The package task
     * will then return a future of the correct type.
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
    using PackagedTaskType = std::packaged_task<decltype(boundTask())()>;
    auto packaged_task = std::make_shared<PackagedTaskType>(boundTask);

    // Create a promise/future that we will fulfill when we have cleaned up the task.
    auto cleanupPromise = std::make_shared<std::promise<decltype(task(args...))>>();
    auto cleanupFuture = cleanupPromise->get_future();

    // Remove the return type from the task by wrapping it in a lambda with no return value.
    auto translated_task = [packaged_task, cleanupPromise]() mutable {
        // Execute the task.
        packaged_task->operator()();
        // Note the future for the task's result.
        auto taskFuture = packaged_task->get_future();
        // Clean up the task.
        packaged_task.reset();
        // Forward the task's result to our cleanup promise/future.
        forwardPromise(cleanupPromise, &taskFuture);
    };

    // Release our local reference to packaged task so that the only remaining reference is inside the lambda.
    packaged_task.reset();

    {
        std::lock_guard<std::mutex> queueLock{m_queueMutex};
        if (!m_shutdown) {
            m_queue.emplace(front ? m_queue.begin() : m_queue.end(), new std::function<void()>(translated_task));
        } else {
            using FutureType = decltype(task(args...));
            return std::future<FutureType>();
        }
    }

    m_queueChanged.notify_all();
    return cleanupFuture;
}

}  // namespace threading
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_THREADING_TASKQUEUE_H_
