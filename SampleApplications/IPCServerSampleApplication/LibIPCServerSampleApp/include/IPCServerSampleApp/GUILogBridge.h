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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUILOGBRIDGE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUILOGBRIDGE_H_

#include <string>

#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

/**
 * Enum used to specify the severity assigned to an IPC client log message.
 */
enum class IPCLogLevel {
    /// Intermediate debug log level.
    DEBUG5,
    /// Logs of normal operations, to be used in release builds.
    INFO,
    /// Log of an event that may indicate a problem.
    WARN,
    /// Log of an event that indicates an error.
    ERROR,
    /// An unknown severity level.
    UNKNOWN
};

/**
 * Convert a string into a @c IPCLogLevel value.
 *
 * @param logLevel The @c std::string value to convert to a @c IPCLogLevel value.
 * @return The @c IPCLogLevel conversion of the @c std::string value.
 */
inline IPCLogLevel ipcLogLevelFromString(const std::string& logLevel) {
    if ("DEBUG" == logLevel) {
        return IPCLogLevel::DEBUG5;
    } else if ("INFO" == logLevel) {
        return IPCLogLevel::INFO;
    } else if ("WARN" == logLevel) {
        return IPCLogLevel::WARN;
    } else if ("ERROR" == logLevel) {
        return IPCLogLevel::ERROR;
    } else {
        return IPCLogLevel::UNKNOWN;
    }
}

/**
 * Simple class to direct GUI log to SDK log.
 */
class GUILogBridge {
public:
    /**
     * Transform IPC LogEvent level to SDK one according to guidelines and log it.
     *
     * @param logLevel IPCLoglevel.
     * @param message Event/log message.
     */
    void log(IPCLogLevel logLevel, const std::string& message);

private:
    /**
     * Internal function to execute logging.
     */
    static void executeLog(IPCLogLevel logLevel, const std::string& message);

    /// This is the worker thread for the @c GUILogBridge.
    avsCommon::utils::threading::Executor m_executor;
};
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUILOGBRIDGE_H_
