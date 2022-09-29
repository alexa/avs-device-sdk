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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_LOGGERHANDLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_LOGGERHANDLERINTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

/// The level json key in the message.
static const char LEVEL_TAG[] = "level";

/// The message json key in the message.
static const char MESSAGE_TAG[] = "message";

/**
 * A contract to handle Logger.
 */
class LoggerHandlerInterface {
public:
    /**
     * Destructor
     */
    virtual ~LoggerHandlerInterface() = default;

    /**
     * IPC Client sends this event to log a message to the SDK’s native logger implementation
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void logEvent(const std::string& message) = 0;
};

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_LOGGERHANDLERINTERFACE_H_
