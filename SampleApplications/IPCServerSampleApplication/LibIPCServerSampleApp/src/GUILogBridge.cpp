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
#include "IPCServerSampleApp/GUILogBridge.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

/// String to identify log entries originating from this file.
static const std::string TAG("GUILogBridge");

/// String to identify event happened.
static const std::string GUI_LOG_EVENT("GUILog");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * Write an ACSDK log from the IPC Log Event
 * @param logLevel IPCLogLevel
 * @param message the message to log.
 */
void GUILogBridge::log(IPCLogLevel logLevel, const std::string& message) {
    m_executor.submit([logLevel, message]() { executeLog(logLevel, message); });
}

void GUILogBridge::executeLog(IPCLogLevel logLevel, const std::string& message) {
    switch (logLevel) {
        case IPCLogLevel::DEBUG5:
            ACSDK_DEBUG5(LX(GUI_LOG_EVENT).m(message));
            break;
        case IPCLogLevel::INFO:
            ACSDK_INFO(LX(GUI_LOG_EVENT).m(message));
            break;
        case IPCLogLevel::ERROR:
            ACSDK_ERROR(LX(GUI_LOG_EVENT).m(message));
            break;
        case IPCLogLevel::WARN:
        default:
            ACSDK_WARN(LX(GUI_LOG_EVENT).m(message));
            break;
    }
}

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK