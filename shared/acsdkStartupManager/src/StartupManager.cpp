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
static const std::string TAG("StartupManager");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

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
    m_notifier->notifyObservers([&result](std::shared_ptr<RequiresStartupInterface> observer) {
        if (!result) {
            ACSDK_ERROR(LX("skippingCallToStartup").d("reason", "startupAborted"));
            return;
        }
        if (!observer->startup()) {
            ACSDK_ERROR(LX("startupAborted").d("reason", "doStartupFailed"));
            result = false;
        }
    });
    m_notifier.reset();
    return result;
}

StartupManager::StartupManager(const std::shared_ptr<StartupNotifierInterface>& notifier) : m_notifier{notifier} {
}

}  // namespace acsdkStartupManager
}  // namespace alexaClientSDK
