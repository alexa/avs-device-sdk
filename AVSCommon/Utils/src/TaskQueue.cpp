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

#include "AVSCommon/Utils/Threading/TaskQueue.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace threading {

TaskQueue::TaskQueue() : m_shutdown{false} {
}

std::unique_ptr<std::function<void()>> TaskQueue::pop() {
    std::unique_lock<std::mutex> queueLock{m_queueMutex};

    auto shouldNotWait = [this]() { return m_shutdown || !m_queue.empty(); };

    if (!shouldNotWait()) {
        m_queueChanged.wait(queueLock, shouldNotWait);
    }

    if (!m_queue.empty()) {
        auto task = std::move(m_queue.front());

        m_queue.pop_front();
        return task;
    }

    return nullptr;
}

void TaskQueue::shutdown() {
    std::lock_guard<std::mutex> queueLock{m_queueMutex};
    m_queue.clear();
    m_shutdown = true;
    m_queueChanged.notify_all();
}

bool TaskQueue::isShutdown() {
    return m_shutdown;
}

}  // namespace threading
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
