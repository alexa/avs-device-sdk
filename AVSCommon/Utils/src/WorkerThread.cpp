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

#include "AVSCommon/Utils/Logger/ThreadMoniker.h"
#include "AVSCommon/Utils/Logger/Logger.h"
#include "AVSCommon/Utils/Threading/ThreadPool.h"
#include "AVSCommon/Utils/Threading/WorkerThread.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace threading {

WorkerThread::WorkerThread() : m_stop{false}, m_cancel{false} {
    m_thread = std::thread{std::bind(&WorkerThread::runInternal, this)};
}

WorkerThread::~WorkerThread() {
    // We set purposely set m_stop before locking to break the loop that is running work while holding the mutex.
    m_stop = true;
    std::unique_lock<std::mutex> lock(m_mutex);
    // If we had a spurious wake at the time of setting m_stop = true above, by locking here, we ensure that the
    // m_workReady condition variable has to be waiting again (since it will have released the lock for us to acquire
    // it here. Therefore by locking and unlocking the mutex here we protect form a spurious wake, and ensure the
    // worker thread will exit allowing the thread to be joined.
    lock.unlock();
    m_workReady.notify_one();
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void WorkerThread::cancel() {
    m_cancel = true;
}

std::thread::id WorkerThread::getThreadId() const {
    return m_thread.get_id();
}

void WorkerThread::run(std::function<bool()> workFunc) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cancel = false;
    m_workerFunc = move(workFunc);
    m_workReady.notify_one();
}

void WorkerThread::runInternal() {
    std::unique_lock<std::mutex> lock(m_mutex);
    do {
        // If run is called before the thread starts, it will notify before we wait, so we guard against that by
        // checking if the worker function has been set before waiting.
        while (!m_workerFunc && !m_stop) {
            m_workReady.wait(lock);
        }
        while (!m_stop && !m_cancel && m_workerFunc())
            ;
        m_workerFunc = nullptr;
    } while (!m_stop);
}

}  // namespace threading
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
