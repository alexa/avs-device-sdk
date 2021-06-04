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

#include "AVSCommon/Utils/Timing/Timer.h"
#include "AVSCommon/Utils/Timing/TimerDelegateFactory.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {

/// String to identify log entries originating from this file.
static const std::string TAG(Timer::getTag());

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

Timer::Timer(std::shared_ptr<sdkInterfaces::timing::TimerDelegateFactoryInterface> timerDelegateFactory) {
    if (!timerDelegateFactory) {
        ACSDK_WARN(
            LX(__func__).d("reason", "nullTimerDelegateFactory").m("Falling back to default TimerDelegateFactory"));
        timerDelegateFactory = std::make_shared<TimerDelegateFactory>();
        if (!timerDelegateFactory) {
            ACSDK_ERROR(LX(__func__).d("reason", "nullDefaultTimerDelegateFactory"));
            return;
        }
    }

    m_timer = timerDelegateFactory->getTimerDelegate();
    if (!m_timer) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullTimerDelegate"));
    }
}

Timer::~Timer() {
    stop();
}

void Timer::stop() {
    if (m_timer) {
        m_timer->stop();
    }
}

bool Timer::isActive() const {
    return m_timer && m_timer->isActive();
}

bool Timer::activate() {
    return m_timer && m_timer->activate();
}

}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
