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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_THREADING_WORKERTHREAD_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_THREADING_WORKERTHREAD_H_

#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include <string>
#include <thread>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace threading {

/**
 * Executes work on a single thread. Stays alive sleeping while there is no work to be done.
 */
class WorkerThread {
public:
    /**
     * Construct a worker thread.
     */
    WorkerThread();

    /**
     * Destruct a worker thread.
     */
    ~WorkerThread();

    /**
     * Perform work on this worker thread until the work is complete or cancel is called.
     *
     * @param workFunc the work function, which shall be called repeatedly while the workFunc returns true or until
     *     cancel() is called.
     */
    void run(std::function<bool()> workFunc);

    /**
     * Cancel currently running work. If work is not running, the call has no effect.
     */
    void cancel();

    /**
     * Return thread id.
     *
     * @return Thread id for the allocated thread.
     */
    std::thread::id getThreadId() const;

private:
    /**
     * method for running thread.
     */
    void runInternal();

    /// Flag indicating the thread is stopping.
    std::atomic<bool> m_stop;

    /// Flag indicating the work should be cancelled
    std::atomic<bool> m_cancel;

    /// The thread on which to perform work.
    std::thread m_thread;

    /// The worker function.
    std::function<bool()> m_workerFunc;

    /// Mutex for protecting the condition.
    std::mutex m_mutex;

    /// Condition variable for waking the thread.
    std::condition_variable m_workReady;

    /// Platform-specific thread identifier.
    std::thread::id m_threadId;
};

}  // namespace threading
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_THREADING_WORKERTHREAD_H_
