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

#include <memory>
#include <functional>

#include "AVSCommon/Utils/Logger/Logger.h"
#include "AVSCommon/Utils/Logger/ThreadMoniker.h"
#include "AVSCommon/Utils/Memory/Memory.h"
#include "AVSCommon/Utils/Threading/ThreadPool.h"

/// String to identify log entries originating from this file.
static const std::string TAG("ThreadPool");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace threading {

using namespace std;
using namespace logger;

static ThreadPool SINGLETON_THREAD_POOL{};

ThreadPool::ThreadPool(size_t maxThreads) :
        m_maxPoolThreads{maxThreads},
        m_created{0},
        m_obtained{0},
        m_releasedToPool{0},
        m_releasedFromPool{0} {
}

ThreadPool::~ThreadPool() {
    std::lock_guard<mutex> lock(m_queueMutex);
    // Ensure all threads are stopped and joined
    m_workerQueue.clear();
}

unique_ptr<WorkerThread> ThreadPool::obtainWorker(string optionalMoniker) {
    std::lock_guard<mutex> lock(m_queueMutex);
    ACSDK_DEBUG9(LX("obtainWorker")
                     .d("created", m_created)
                     .d("obtained", m_obtained)
                     .d("releasedToPool", m_releasedToPool)
                     .d("releasedFromPool", m_releasedFromPool)
                     .d("outstanding", m_obtained - (m_releasedToPool + m_releasedFromPool)));
    m_obtained++;
    unique_ptr<WorkerThread> ret;
    if (m_workerQueue.empty()) {
        m_created++;
        ret = memory::make_unique<WorkerThread>();
    } else {
#ifdef THREAD_AFFINITY
        bool containsMoniker = false;
        if (!optionalMoniker.empty()) {
            containsMoniker = m_workerMap.count(optionalMoniker) > 0;
        }
        auto workerIterator = containsMoniker ? m_workerMap[optionalMoniker] : m_workerQueue.begin();
#else
        auto workerIterator = m_workerQueue.begin();
#endif
        ret = std::move(*workerIterator);
        m_workerQueue.erase(workerIterator);
#ifdef THREAD_AFFINITY
        std::string moniker = (*workerIterator)->getMoniker();
        m_workerMap.erase(moniker);
#endif
    }

    return ret;
}

void ThreadPool::releaseWorker(std::unique_ptr<WorkerThread> workerThread) {
    std::lock_guard<mutex> lock(m_queueMutex);

    if (m_workerQueue.size() >= m_maxPoolThreads) {
        // In order to allow this to be called from the thread being released,
        // we release the first thread in the queue when we want to stop growing.
        m_workerQueue.pop_front();
        m_releasedFromPool++;
    } else {
        m_releasedToPool++;
    }
    m_workerQueue.push_back(std::move(workerThread));
#ifdef THREAD_AFFINITY
    std::string moniker = workerThread->getMoniker();
    m_workerMap[moniker] = --m_workerQueue.end();
#endif
}

void ThreadPool::setMaxThreads(size_t maxThreads) {
    std::lock_guard<mutex> lock(m_queueMutex);
    m_maxPoolThreads = maxThreads <= 0 ? 1 : maxThreads;
    while (m_workerQueue.size() > m_maxPoolThreads) {
        m_workerQueue.pop_front();
    }
}

uint32_t ThreadPool::getMaxThreads() {
    std::lock_guard<mutex> lock(m_queueMutex);
    return m_maxPoolThreads;
}

void ThreadPool::getStats(
    uint64_t& threadsCreated,
    uint64_t& threadsObtained,
    uint64_t& threadsReleasedToPool,
    uint64_t& threadsReleasedFromPool) {
    std::lock_guard<mutex> lock(m_queueMutex);
    threadsCreated = m_created;
    threadsObtained = m_obtained;
    threadsReleasedToPool = m_releasedToPool;
    threadsReleasedFromPool = m_releasedFromPool;
}

std::shared_ptr<ThreadPool> ThreadPool::getDefaultThreadPool() {
    static std::mutex singletonMutex;
    static std::weak_ptr<ThreadPool> weakTPRef;

    auto sharedTPRef = weakTPRef.lock();
    if (!sharedTPRef) {
        std::lock_guard<mutex> lock(singletonMutex);
        // Ensure we don't unnecessarily create twice if there was contention on the lock by attempting to lock again.
        sharedTPRef = weakTPRef.lock();
        if (!sharedTPRef) {
            sharedTPRef = std::make_shared<ThreadPool>();
            weakTPRef = sharedTPRef;
        }
    }
    return sharedTPRef;
}

}  // namespace threading
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
