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

#include "AVSCommon/Utils/Threading/TaskThread.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace threading {

TaskThread::TaskThread(std::shared_ptr<TaskQueue> taskQueue) : m_taskQueue{taskQueue}, m_shutdown{false} {
}

TaskThread::~TaskThread() {
    m_shutdown = true;

    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void TaskThread::start() {
    m_thread = std::thread{std::bind(&TaskThread::processTasksLoop, this)};
}

bool TaskThread::isShutdown() {
    return m_shutdown;
}

void TaskThread::processTasksLoop() {
    while (!m_shutdown) {
        auto m_actualTaskQueue = m_taskQueue.lock();

        if (m_actualTaskQueue && !m_actualTaskQueue->isShutdown()) {
            auto task = m_actualTaskQueue->pop();

            if (task) {
                task->operator()();
            }
        } else {
            // Since we could not get a shared pointer to the the TaskQueue, it must have been destroyed.
            // The thread must shut down.
            m_shutdown = true;
        }
    }
}

}  // namespace threading
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
