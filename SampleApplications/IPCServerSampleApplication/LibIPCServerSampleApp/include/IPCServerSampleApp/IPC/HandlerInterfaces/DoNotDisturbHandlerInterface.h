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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_DONOTDISTURBHANDLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_DONOTDISTURBHANDLERINTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

/// The enabled json key in the message.
static const char ENABLED_TAG[] = "enabled";

/**
 * A contract to handle DoNotDisturb mode.
 */
class DoNotDisturbHandlerInterface {
public:
    /**
     * Destructor
     */
    virtual ~DoNotDisturbHandlerInterface() = default;

    /**
     * Set Do Not Disturb mode.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void doNotDisturbStateChanged(const std::string& message) = 0;

    /**
     * Request the SDK to inform current Do Not Disturb setting state.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void doNotDisturbStateRequest(const std::string& message) = 0;
};

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_DONOTDISTURBHANDLERINTERFACE_H_
