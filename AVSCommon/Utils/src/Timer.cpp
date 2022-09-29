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
#include <AVSCommon/Utils/Timing/Timer.h>
#include <AVSCommon/Utils/Timing/TimerDelegateFactory.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {

using namespace sdkInterfaces::timing;

/// String to identify log entries originating from this file.
#define TAG "Timer"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

// Instantiate templates
template bool Timer::adaptTypesAndCallTask(
    const std::chrono::milliseconds&,
    const std::chrono::milliseconds&,
    PeriodType,
    size_t,
    std::function<void()>&&) noexcept;
template bool Timer::adaptTypesAndCallTask(
    const std::chrono::nanoseconds&,
    const std::chrono::nanoseconds&,
    PeriodType,
    size_t,
    std::function<void()>&&) noexcept;

Timer::Timer(const std::shared_ptr<TimerDelegateFactoryInterface>& timerDelegateFactory) noexcept {
    if (timerDelegateFactory) {
        m_timer = timerDelegateFactory->getTimerDelegate();
    } else {
        ACSDK_WARN(
            LX(__func__).d("reason", "nullTimerDelegateFactory").m("Falling back to default TimerDelegateFactory"));
        m_timer = TimerDelegateFactory{}.getTimerDelegate();
    }

    if (!m_timer) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullTimerDelegate"));
    }
}

Timer::~Timer() noexcept {
    stop();
}

void Timer::stop() noexcept {
    if (m_timer) {
        m_timer->stop();
    }
}

bool Timer::isActive() const noexcept {
    return m_timer && m_timer->isActive();
}

bool Timer::activate() noexcept {
    return m_timer && m_timer->activate();
}

bool Timer::callTask(
    const std::chrono::nanoseconds& delayNano,
    const std::chrono::nanoseconds& periodNano,
    PeriodType periodType,
    size_t maxCount,
    std::function<void()>&& task) noexcept {
    if (delayNano < std::chrono::nanoseconds::zero()) {
        ACSDK_ERROR(LX(__func__).d("reason", "negativeDelay"));
        return false;
    }
    if (periodNano < std::chrono::nanoseconds::zero()) {
        ACSDK_ERROR(LX(__func__).d("reason", "negativePeriod"));
        return false;
    }
    if (!task) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullTask"));
        return false;
    }

    // Don't start if already running.
    if (!activate()) {
        ACSDK_ERROR(LX(__func__).d("reason", "timerAlreadyActive"));
        return false;
    }

    if (!m_timer) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullTimerDelegate"));
        return false;
    }

    TimerDelegateInterface::PeriodType delegatePeriodType;

    switch (periodType) {
        case PeriodType::ABSOLUTE:
            delegatePeriodType = TimerDelegateInterface::PeriodType::ABSOLUTE;
            break;
        case PeriodType::RELATIVE:
            delegatePeriodType = TimerDelegateInterface::PeriodType::RELATIVE;
            break;
        default:
            ACSDK_ERROR(LX(__func__).d("reason", "badPeriodType"));
            return false;
    }

    m_timer->start(delayNano, periodNano, delegatePeriodType, maxCount, std::move(task));

    return true;
}

}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
