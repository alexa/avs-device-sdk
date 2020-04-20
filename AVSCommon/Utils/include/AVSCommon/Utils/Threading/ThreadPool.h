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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_THREADING_THREADPOOL_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_THREADING_THREADPOOL_H_

#include <condition_variable>
#include <list>
#ifdef THREAD_AFFINITY
#include <map>
#endif
#include <string>

#include "WorkerThread.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace threading {

static const size_t DEFAULT_MAX_THREAD_POOL_THREADS = 20;

/**
 * The @c ThreadPool holds a pool of worker threads that each represent an OS thread. The pool begins empty. When
 * @c ThreadPool::obtainWorker is called when the pool is empty, a new worker thread will be created and returned to the
 * caller; otherwise, when the pool is not empty, one of the existing worker threads will be removed from the pool and
 * returned to the caller. The @c ThreadPool holds a configurable maximum number of worker threads. Threads are released
 * back into the pool when @c ThreadPool:releaseWorker is called. Once the maximum worker threads are present in the
 * pool, releasing a worker thread will cause one of the existing pool's worker threads to be destructed and released so
 * that the pool does not grow beyond the configured maximum. The ThreadPool also maintains and provides statistics for
 * the number of threads threads created, obtained, released into the pool, and released from the pool.
 */
class ThreadPool {
public:
    /**
     * Constructs a thread pool that keeps track of the threads it maintains by moniker.
     */
    ThreadPool(size_t maxThreads = DEFAULT_MAX_THREAD_POOL_THREADS);

    /**
     * Destructs the thread pool.
     */
    ~ThreadPool();

    /**
     * Obtain a worker thread to operate on.
     *
     * @param optionalMoniker the moniker of the worker desired.
     * @return the worker thread obtained.
     */
    std::unique_ptr<WorkerThread> obtainWorker(std::string optionalMoniker = "");

    /**
     * Release a worker
     *
     * @param workerThread the worker thread to release.
     */
    void releaseWorker(std::unique_ptr<WorkerThread> workerThread);

    /**
     * Sets the maximum threads the pool should hold.
     *
     * @param maxThreads the max threads the pool should hold. If 0 is passed, the maximum threads will be set to 1,
     *     since the ThreadPool implementation does not support less than a single thread.
     */
    void setMaxThreads(size_t maxThreads);

    /**
     * Obtain the current maximum threads for the thread pool.
     * @return the max threads the thread pool is currently set to hold.
     */
    uint32_t getMaxThreads();

    /**
     * Obtain statics for the thread pool
     * @param threadsCreated the total number of threads the pool has created.
     * @param threadsObtained the number of threads obtained from the pool.
     * @param threadsReleasedToPool the number of threads released from the caller into the pool.
     * @param threadsReleasedFromPool the number of threads released from the pool.
     */
    void getStats(
        uint64_t& threadsCreated,
        uint64_t& threadsObtained,
        uint64_t& threadsReleasedToPool,
        uint64_t& threadsReleasedFromPool);

    /**
     * Obtain a shared pointer to the default singleton thread pool.
     * @return a shared pointer to the thread pool.
     */
    static std::shared_ptr<ThreadPool> getDefaultThreadPool();

private:
#ifdef THREAD_AFFINITY
    /// Map of Worker thread monikers to worker thread iterator in the worker queue.
    std::map<std::string, std::list<std::unique_ptr<WorkerThread>>::iterator> m_workerMap;
#endif
    /// Queue of worker threads to vend.
    std::list<std::unique_ptr<WorkerThread>> m_workerQueue;

    /// Max threads to store.
    size_t m_maxPoolThreads;

    /// Metrics for created threads.
    uint64_t m_created;

    /// Metrics for obtained threads.
    uint64_t m_obtained;

    /// Metrics for threads released into the pool.
    uint64_t m_releasedToPool;

    /// Metrics for threads released from the pool.
    uint64_t m_releasedFromPool;

    /// A mutex to protect access to the worker threads in the thread pool.
    std::mutex m_queueMutex;
};

}  // namespace threading
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_THREADING_THREADPOOL_H_
