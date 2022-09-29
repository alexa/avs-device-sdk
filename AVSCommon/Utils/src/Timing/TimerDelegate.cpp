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

#include <AVSCommon/Utils/Error/FinallyGuard.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Logger/ThreadMoniker.h>
#include <AVSCommon/Utils/Timing/TimerDelegate.h>

/// String to identify log entries originating from this file.
#define TAG "TimerDelegate"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {

using utils::logger::ThreadMoniker;

/**
 * @brief Helper method to invoke task and catch exception.
 *
 * @param[in, out] task Task reference. The reference is cleared before the method returns.
 */
static void safeInvokeTask(std::function<void()>& task) {
#if __cpp_exceptions || defined(__EXCEPTIONS)
    try {
#endif
        task();
#if __cpp_exceptions || defined(__EXCEPTIONS)
    } catch (const std::exception& ex) {
        ACSDK_ERROR(LX(__func__).d("taskException", ex.what()));
    } catch (...) {
        ACSDK_ERROR(LX(__func__).d("taskException", "other"));
    }
#endif
}

TimerDelegate::TimerDelegate() : m_running{false}, m_stopping{false} {
}

TimerDelegate::~TimerDelegate() {
    stop();
}

void TimerDelegate::start(
    std::chrono::nanoseconds delay,
    std::chrono::nanoseconds period,
    PeriodType periodType,
    size_t maxCount,
    std::function<void()> task) {
    auto moniker = ThreadMoniker::generateMoniker(ThreadMoniker::PREFIX_TIMER);
    ACSDK_DEBUG5(LX("init").d("moniker", moniker));

    if (!task) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullTask"));
    }

    std::lock_guard<std::mutex> lock(m_callMutex);
    cleanupLocked();
    activateLocked();
    m_thread = std::thread(
        &TimerDelegate::timerLoop, this, delay, period, periodType, maxCount, std::move(task), std::move(moniker));
}

void TimerDelegate::timerLoop(
    std::chrono::nanoseconds delay,
    std::chrono::nanoseconds period,
    PeriodType periodType,
    size_t maxCount,
    std::function<void()> task,
    std::string moniker) {
    ThreadMoniker::setThisThreadMoniker(std::move(moniker));

    // Timepoint to measure delay/period against.
    auto now = std::chrono::steady_clock::now();
    using utils::logger::ThreadMoniker;

    // Flag indicating whether we've drifted off schedule.
    bool offSchedule = false;

    for (size_t count = 0; maxCount == FOREVER || count < maxCount; ++count) {
        auto waitTime = (0 == count) ? delay : period;
        {
            std::unique_lock<std::mutex> lock(m_waitMutex);

            // Wait for stop() or a delay/period to elapse.
            if (m_waitCondition.wait_until(lock, now + waitTime, [this]() { return m_stopping; })) {
                m_stopping = false;
                m_running = false;
                return;
            }
        }

        switch (periodType) {
            case PeriodType::ABSOLUTE:
                // Update our estimate of where we should be after the delay.
                now += waitTime;

                // Run the task if we're still on schedule.
                if (!offSchedule) {
                    safeInvokeTask(task);
                }

                // If the task runtime put us off schedule, skip the next task run.
                if (now + period < std::chrono::steady_clock::now()) {
                    offSchedule = true;
                } else {
                    offSchedule = false;
                }
                break;

            case PeriodType::RELATIVE:
                safeInvokeTask(task);
                now = std::chrono::steady_clock::now();
                break;
        }
    }

    // release task reference
    task = nullptr;

    std::lock_guard<std::mutex> lockGuard(m_waitMutex);
    m_stopping = false;
    m_running = false;
}

void TimerDelegate::stop() {
    std::lock_guard<std::mutex> lock(m_callMutex);
    {
        std::lock_guard<std::mutex> stateLock(m_waitMutex);
        if (m_running) {
            m_stopping = true;
        }
    }
    m_waitCondition.notify_all();
    cleanupLocked();
}

bool TimerDelegate::activate() {
    std::lock_guard<std::mutex> lock(m_callMutex);
    return activateLocked();
}

bool TimerDelegate::activateLocked() {
    return !m_running.exchange(true);
}

bool TimerDelegate::isActive() const {
    return m_running;
}

void TimerDelegate::cleanupLocked() {
    if (std::this_thread::get_id() != m_thread.get_id() && m_thread.joinable()) {
        m_thread.join();
    }
}

}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
