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

#include "acsdkSystemClockMonitor/SystemClockMonitor.h"

namespace alexaClientSDK {
namespace acsdkSystemClockMonitor {

using namespace acsdkSystemClockMonitorInterfaces;
using namespace avsCommon::utils;

/// String to identify log entries originating from this file.
#define TAG "SystemClockMonitor"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<SystemClockMonitorInterface> SystemClockMonitor::createSystemClockMonitorInterface(
    const std::shared_ptr<SystemClockNotifierInterface>& notifier) {
    if (!notifier) {
        ACSDK_ERROR(LX("createSystemClockMonitorFailed").d("reason", "nullNotifier"));
        return nullptr;
    }

    return std::shared_ptr<SystemClockMonitor>(new SystemClockMonitor(notifier));
}

void SystemClockMonitor::onSystemClockSynchronized() {
    m_notifier->notifyObservers(
        [](std::shared_ptr<SystemClockMonitorObserverInterface> observer) { observer->onSystemClockSynchronized(); });
}

SystemClockMonitor::SystemClockMonitor(const std::shared_ptr<SystemClockNotifierInterface>& notifier) :
        m_notifier{notifier} {
}

}  // namespace acsdkSystemClockMonitor
}  // namespace alexaClientSDK
