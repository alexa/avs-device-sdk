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

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/Timing/TimerDelegateFactory.h>
#include <AVSCommon/Utils/Timing/StopTaskTimer.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {
namespace test {

using namespace avsCommon::sdkInterfaces::timing;
using namespace avsCommon::utils;

/// String to identify log entries originating from this file.
#define TAG "StopTaskTimer"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

bool StopTaskTimerDelegateFactory::supportsLowPowerMode() {
    return true;
}

std::unique_ptr<TimerDelegateInterface> StopTaskTimerDelegateFactory::getTimerDelegate() {
    return memory::make_unique<StopTaskTimer>();
}

StopTaskTimer::StopTaskTimer() {
    ACSDK_DEBUG5(LX(__func__));
    m_delegate = std::make_shared<TimerDelegateFactory>()->getTimerDelegate();
}

StopTaskTimer::~StopTaskTimer() {
    ACSDK_DEBUG5(LX(__func__));
    m_delegate->stop();
}

void StopTaskTimer::start(
    std::chrono::nanoseconds delay,
    std::chrono::nanoseconds period,
    PeriodType periodType,
    size_t maxCount,
    std::function<void()> task) {
    auto synchronizedTask = [this, task] {
        std::unique_lock<std::mutex> lock(m_taskMutex, std::try_to_lock);
        if (lock.owns_lock()) {
            task();
        }
    };

    std::lock_guard<std::mutex> lock(m_mutex);
    m_task = task;
    m_delegate->start(delay, period, periodType, maxCount, synchronizedTask);
}

void StopTaskTimer::stop() {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);
    if (isActive()) {
        // Wait until any current executing tasked has finished.
        // The objective is to force the task to run from the beginning.
        std::lock_guard<std::mutex> taskLock(m_taskMutex);
        m_task();
        m_delegate->stop();
    }
}

bool StopTaskTimer::activate() {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);
    return m_delegate->activate();
}

bool StopTaskTimer::isActive() const {
    ACSDK_DEBUG5(LX(__func__));

    return m_delegate->isActive();
}

}  // namespace test
}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
