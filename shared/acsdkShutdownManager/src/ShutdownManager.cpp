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

#include "acsdkShutdownManager/ShutdownManager.h"

namespace alexaClientSDK {
namespace acsdkShutdownManager {

using namespace acsdkShutdownManagerInterfaces;
using namespace avsCommon::utils;

/// String to identify log entries originating from this file.
#define TAG "ShutdownManager"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<ShutdownManagerInterface> ShutdownManager::createShutdownManagerInterface(
    const std::shared_ptr<ShutdownNotifierInterface>& notifier) {
    if (!notifier) {
        ACSDK_ERROR(LX("createShutdownManagerFailed").d("reason", "nullNotifier"));
        return nullptr;
    }

    return std::shared_ptr<ShutdownManager>(new ShutdownManager(notifier));
}

bool ShutdownManager::shutdown() {
    if (!m_notifier) {
        ACSDK_ERROR(LX("shutdownAlreadyCalled"));
        return false;
    }
    bool result =
        m_notifier->notifyObserversInReverse([](std::shared_ptr<RequiresShutdown> observer) { observer->shutdown(); });
    if (!result) {
        ACSDK_ERROR(LX("shutdownFailed").d("reason", "instancesAddedAfterShutdownStarted"));
    }
    m_notifier.reset();
    return result;
}

ShutdownManager::ShutdownManager(const std::shared_ptr<ShutdownNotifierInterface>& notifier) : m_notifier{notifier} {
}

}  // namespace acsdkShutdownManager
}  // namespace alexaClientSDK
