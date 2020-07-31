
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

#include "Settings/SettingEventRequestObserver.h"

#include <AVSCommon/Utils/Logger/Logger.h>

/// String to identify log entries originating from this file.
static const std::string TAG("SettingEventRequestObserver");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace settings {

void SettingEventRequestObserver::onSendCompleted(
    avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status status) {
    m_promise.set_value(status);
}

void SettingEventRequestObserver::onExceptionReceived(const std::string& exceptionMessage) {
    ACSDK_ERROR(LX("exceptionReceived").d("message", exceptionMessage));
}

std::shared_future<avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status> SettingEventRequestObserver::
    getResponseFuture() {
    return m_promise.get_future();
}

}  // namespace settings
}  // namespace alexaClientSDK
