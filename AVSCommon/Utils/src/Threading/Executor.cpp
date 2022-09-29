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

#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/Threading/private/SharedExecutor.h>
#include <AVSCommon/Utils/Logger/Logger.h>

/// String to identify log entries originating from this file.
#define TAG "ExecutorWrapper"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace threading {

Executor::Executor() noexcept : m_executor{std::make_shared<SharedExecutor>()} {
    if (!m_executor) {
        ACSDK_ERROR(LX("initError"));
    }
}

Executor::~Executor() noexcept {
    m_executor.reset();
}

bool Executor::execute(std::function<void()>&& function) noexcept {
    return execute(std::move(function), QueuePosition::Back);
}

bool Executor::execute(std::function<void()>&& function, QueuePosition queuePosition) noexcept {
    if (m_executor) {
        auto error = m_executor->execute(std::move(function), queuePosition);
        return !error;
    }
    return false;
}

bool Executor::execute(const std::function<void()>& function) noexcept {
    return execute(std::function<void()>(function), QueuePosition::Back);
}

void Executor::waitForSubmittedTasks() noexcept {
    if (m_executor) {
        m_executor->waitForSubmittedTasks();
    }
}

void Executor::shutdown() noexcept {
    if (m_executor) {
        m_executor->shutdown();
    }
}

bool Executor::isShutdown() noexcept {
    if (m_executor) {
        return m_executor->isShutdown();
    }
    return true;
}

Executor::operator std::shared_ptr<ExecutorInterface>() const noexcept {
    return m_executor;
}

}  // namespace threading
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
