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

#include <AVSCommon/Utils/Timing/MultiTimer.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Logger/ThreadMoniker.h>
#include <AVSCommon/Utils/Timing/Timer.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {

/// String to identify log entries originating from this file.
#define TAG "MultiTimer"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Grace period used to avoid restarting the internal thread too often.
static const std::chrono::milliseconds GRACE_PERIOD{500};

std::shared_ptr<MultiTimer> MultiTimer::createMultiTimer() noexcept {
    return std::make_shared<MultiTimer>();
}

MultiTimer::MultiTimer() noexcept :
        m_timerMoniker{utils::logger::ThreadMoniker::generateMoniker(utils::logger::ThreadMoniker::PREFIX_TIMER)},
        m_isRunning{false},
        m_isBeingDestroyed{false},
        m_nextToken{0} {
    ACSDK_DEBUG5(LX("init").d("moniker", m_timerMoniker));
}

MultiTimer::~MultiTimer() noexcept {
    std::unique_lock<std::mutex> lock{m_waitMutex};
    m_timers.clear();
    m_tasks.clear();
    m_isBeingDestroyed = true;

    if (m_isRunning) {
        // Shutdown any pending operation.
        lock.unlock();
        m_waitCondition.notify_all();
    }
}

MultiTimer::Token MultiTimer::submitTask(const std::chrono::milliseconds& delay, std::function<void()> task) noexcept {
    std::unique_lock<std::mutex> lock{m_waitMutex};
    auto token = m_nextToken++;

    // Insert new task.
    TimePoint timePoint = std::chrono::steady_clock::now() + delay;
    m_timers.insert({timePoint, token});
    m_tasks.insert({token, {timePoint, task}});

    // Kick-off task execution if needed.
    if (!m_isRunning) {
        m_isRunning = true;
        m_timerThread.start(std::bind(&MultiTimer::executeTimer, this), m_timerMoniker);
    } else {
        // Wake up timer thread if the new task is the next to expire.
        if (m_timers.begin()->second == token) {
            lock.unlock();
            m_waitCondition.notify_one();
        }
    }
    return token;
}

void MultiTimer::cancelTask(Token token) noexcept {
    std::unique_lock<std::mutex> lock{m_waitMutex};
    auto taskIt = m_tasks.find(token);
    if (taskIt != m_tasks.end()) {
        auto& timePoint = taskIt->second.first;
        bool isNext = m_timers.begin()->first == timePoint;
        // Remove task
        auto timerIt = std::find(m_timers.begin(), m_timers.end(), decltype(m_timers)::value_type(timePoint, token));
        m_timers.erase(timerIt);
        m_tasks.erase(token);

        if (isNext) {
            // Wake up timer thread if the task that was removed was the next to expire.
            lock.unlock();
            m_waitCondition.notify_one();
        }
    }
}

bool MultiTimer::executeTimer() noexcept {
    std::unique_lock<std::mutex> lock{m_waitMutex};
    while (!m_timers.empty()) {
        auto now = std::chrono::steady_clock::now();
        auto nextIt = m_timers.begin();
        auto& nextTime = nextIt->first;
        auto waitTime = std::chrono::duration_cast<std::chrono::milliseconds>(nextTime - now);
        if (waitTime > std::chrono::milliseconds::zero()) {
            // Wait for next time.
            m_waitCondition.wait_for(lock, waitTime, [this, nextTime] {
                return m_isBeingDestroyed || m_timers.empty() || (m_timers.begin()->first < nextTime);
            });
        } else {
            // Execute task.
            auto taskIt = m_tasks.find(nextIt->second);
            if (taskIt != m_tasks.end()) {
                auto task = std::move(taskIt->second.second);
                m_tasks.erase(taskIt);
                m_timers.erase(nextIt);
                lock.unlock();
                if (task) {
                    task();
                }
                lock.lock();
            } else {
                m_timers.erase(nextIt);
            }
        }
    }
    return hasNextLocked(lock);
}

bool MultiTimer::hasNextLocked(std::unique_lock<std::mutex>& lock) noexcept {
    m_waitCondition.wait_for(lock, GRACE_PERIOD, [this] { return (!m_tasks.empty()) || m_isBeingDestroyed; });
    m_isRunning = (!m_isBeingDestroyed) && !m_tasks.empty();
    return m_isRunning;
}

}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
