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

#include "acsdkStartupManager/StartupManager.h"

namespace alexaClientSDK {
namespace acsdkStartupManager {

using namespace acsdkStartupManagerInterfaces;

/// String to identify log entries originating from this file.
#define TAG "StartupManager"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * Helper function for notifying observers of startup.
 *
 * @note This was introduced to prevent cppcheck from complaining that @c result was
 * always true.
 *
 * @param result Accumulated result of startup notifications.  If false, skip calling
 * observer->startup().  If observer->startup() returns false, set result to false.
 * @param observer The observer to notify of startup.
 */
static void notifyObserverOfStartup(bool* result, std::shared_ptr<RequiresStartupInterface> observer) {
    if (!result) {
        ACSDK_ERROR(LX("notifyObserverFailed").d("reason", "nullResult"));
        return;
    }

    if (!*result) {
        ACSDK_ERROR(LX("skippingCallToStartup").d("reason", "startupAborted"));
        return;
    }

    if (!observer->startup()) {
        ACSDK_ERROR(LX("startupAborted").d("reason", "doStartupFailed"));
        *result = false;
    }
}

std::shared_ptr<StartupManagerInterface> StartupManager::createStartupManagerInterface(
    const std::shared_ptr<StartupNotifierInterface>& notifier) {
    if (!notifier) {
        ACSDK_ERROR(LX("createStartupManagerInterfaceFailed").d("reason", "nullNotifier"));
        return nullptr;
    }

    return std::shared_ptr<StartupManager>(new StartupManager(notifier));
}

bool StartupManager::startup() {
    if (!m_notifier) {
        ACSDK_ERROR(LX("startupAlreadyCalled"));
        return false;
    }
    bool result = true;
    m_notifier->notifyObservers(
        [&result](std::shared_ptr<RequiresStartupInterface> observer) { notifyObserverOfStartup(&result, observer); });
    m_notifier.reset();
    return result;
}

StartupManager::StartupManager(const std::shared_ptr<StartupNotifierInterface>& notifier) : m_notifier{notifier} {
}

}  // namespace acsdkStartupManager
}  // namespace alexaClientSDK
