/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "AVSCommon/Utils/Logger/Logger.h"
#include <AVSCommon/Utils/Timing/SystemClockMonitor.h>

/// String to identify log entries originating from this file.
static const std::string TAG("SystemClockMonitor");

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

void SystemClockMonitor::notifySystemClockSynchronized() {
    notifyObservers();
}

void SystemClockMonitor::addSystemClockMonitorObserver(
    const std::shared_ptr<avsCommon::sdkInterfaces::SystemClockMonitorObserverInterface>& observer) {
    if (!observer) {
        ACSDK_ERROR(LX("addObserverFailed").d("reason", "nullObserver"));
        return;
    }

    std::lock_guard<std::mutex> lock{m_systemClockObserverMutex};
    m_observers.insert(observer);
}

void SystemClockMonitor::removeSystemClockMonitorObserver(
    const std::shared_ptr<avsCommon::sdkInterfaces::SystemClockMonitorObserverInterface>& observer) {
    if (!observer) {
        ACSDK_ERROR(LX("removeObserverFailed").d("reason", "nullObserver"));
        return;
    }

    std::lock_guard<std::mutex> lock{m_systemClockObserverMutex};
    m_observers.erase(observer);
}

void SystemClockMonitor::notifyObservers() {
    std::unique_lock<std::mutex> lock{m_systemClockObserverMutex};
    auto observersCopy = m_observers;
    lock.unlock();

    for (const auto& observer : observersCopy) {
        observer->onSystemClockSynchronized();
    }
}

}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
