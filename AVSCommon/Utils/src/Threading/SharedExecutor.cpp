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

#include <algorithm>
#include <exception>
#include <cctype>

#include <AVSCommon/Utils/Logger/ThreadMoniker.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/Power/PowerMonitor.h>
#include <AVSCommon/Utils/Threading/private/SharedExecutor.h>
#include <AVSCommon/Utils/Logger/Logger.h>

/// String to identify log entries originating from this file.
#define TAG "Executor"

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

template std::future<void> Executor::pushFunction(
    QueuePosition queuePosition,
    std::function<void()>&& function) noexcept;
template std::future<bool> Executor::pushFunction(
    QueuePosition queuePosition,
    std::function<bool()>&& function) noexcept;
template std::future<std::string> Executor::pushFunction(
    QueuePosition queuePosition,
    std::function<std::string()>&& function) noexcept;

/// Prefix for power resource owned by @c Executor instance.
/// @private
static constexpr auto POWER_RESOURCE_PREFIX = "Executor:";

/**
 * Helper method to create power resource id from a moniker.
 *
 * This method concatenates @a POWER_RESOURCE_PREFIX with @a moniker without trailing spaces.
 *
 * @param moniker Moniker value to use.
 * @return Power resource identifier.
 * @private
 */
static std::string createPowerResourceName(const std::string& moniker) noexcept {
    std::string::const_iterator it = moniker.begin();
    while (it != moniker.end() && std::isspace(*it)) {
        ++it;
    }
    std::string result;
    constexpr auto powerResourcePrefixLen = sizeof(POWER_RESOURCE_PREFIX) - 1;
    result.reserve(std::distance(it, moniker.end()) + powerResourcePrefixLen);
    result.append(POWER_RESOURCE_PREFIX, powerResourcePrefixLen);
    std::copy(it, moniker.end(), back_inserter(result));
    return result;
}

SharedExecutor::SharedExecutor() noexcept :
        m_executorMoniker{utils::logger::ThreadMoniker::generateMoniker(utils::logger::ThreadMoniker::PREFIX_EXECUTOR)},
        m_threadRunning{false},
        m_shutdown{false} {
    ACSDK_DEBUG5(LX("created").d("moniker", m_executorMoniker));
    m_powerResource =
        power::PowerMonitor::getInstance()->createLocalPowerResource(createPowerResourceName(m_executorMoniker));
}

SharedExecutor::~SharedExecutor() noexcept {
    shutdown();
    ACSDK_DEBUG5(LX("destroyed").d("moniker", m_executorMoniker));
}

std::error_condition SharedExecutor::execute(std::function<void()>&& function) noexcept {
    return execute(std::move(function), QueuePosition::Back);
}

std::error_condition SharedExecutor::execute(const std::function<void()>& function) noexcept {
    // Forward copy of function reference.
    return execute(std::function<void()>(function), QueuePosition::Back);
}

void SharedExecutor::waitForSubmittedTasks() noexcept {
    std::unique_lock<std::mutex> lock{m_queueMutex};
    if (m_threadRunning) {
        // wait for thread to exit.
        std::promise<void> flushedPromise;
        auto flushedFuture = flushedPromise.get_future();
        m_queue.emplace_back([&flushedPromise]() { flushedPromise.set_value(); });

        lock.unlock();
        flushedFuture.wait();
    }
}

std::error_condition SharedExecutor::execute(std::function<void()>&& function, QueuePosition queuePosition) noexcept {
    if (!function) {
        ACSDK_ERROR(LX(__func__).d("reason", "emptyFunction"));
        return std::errc::invalid_argument;
    }
    if (QueuePosition::Back != queuePosition && QueuePosition::Front != queuePosition) {
        ACSDK_ERROR(LX(__func__).d("reason", "badQueuePosition"));
        return std::errc::invalid_argument;
    }

    std::lock_guard<std::mutex> queueLock{m_queueMutex};
    if (m_shutdown) {
        ACSDK_WARN(LX(__func__).d("reason", "shutdownState"));
        return std::errc::operation_not_permitted;
    }

    if (m_powerResource) {
        m_powerResource->acquire();
    }
    m_queue.emplace(QueuePosition::Front == queuePosition ? m_queue.begin() : m_queue.end(), std::move(function));

    if (!m_threadRunning) {
        m_threadRunning = true;
        // Restart task thread.
        m_taskThread.start(std::bind(&SharedExecutor::runNext, this), m_executorMoniker);
    }

    return std::error_condition();
}

std::function<void()> SharedExecutor::pop() noexcept {
    std::lock_guard<std::mutex> lock{m_queueMutex};
    if (!m_queue.empty()) {
        auto task = std::move(m_queue.front());
        m_queue.pop_front();
        return task;
    }
    return std::function<void()>();
}

bool SharedExecutor::hasNext() noexcept {
    std::unique_lock<std::mutex> lock{m_queueMutex};
    m_threadRunning = !m_queue.empty();
    return m_threadRunning;
}

bool SharedExecutor::runNext() noexcept {
    auto task = pop();
    if (task) {
#if __cpp_exceptions || defined(__EXCEPTIONS)
        try {
#endif
            task();
            // Ensure the task is released after executed.
            task = nullptr;

#if __cpp_exceptions || defined(__EXCEPTIONS)
        } catch (const std::exception& ex) {
            ACSDK_ERROR(LX(__func__).d("taskException", ex.what()));
        } catch (...) {
            ACSDK_ERROR(LX(__func__).d("taskException", "other"));
        }
#endif

        if (m_powerResource) {
            m_powerResource->release();
        }
    }

    return hasNext();
}

void SharedExecutor::shutdown() noexcept {
    Queue releaseQueue;
    {
        std::lock_guard<std::mutex> lock{m_queueMutex};
        m_shutdown = true;
        std::swap(releaseQueue, m_queue);
    }
    releaseQueue.clear();
    waitForSubmittedTasks();
}

bool SharedExecutor::isShutdown() noexcept {
    return m_shutdown;
}

}  // namespace threading
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
