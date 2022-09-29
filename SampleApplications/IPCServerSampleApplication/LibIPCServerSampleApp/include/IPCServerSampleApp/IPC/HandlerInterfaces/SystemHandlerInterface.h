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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_SYSTEMHANDLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_SYSTEMHANDLERINTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

/**
 * A contract to handle System.
 */
class SystemHandlerInterface {
public:
    /**
     * Destructor
     */
    virtual ~SystemHandlerInterface() = default;

    /**
     * IPC Client sends this event to request the native client to return the authorization state through the
     * setAuthorizationState directive.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void authorizationStateRequest(const std::string& message) = 0;

    /**
     * IPC Client sends this event to request the native client to return the Alexa state through the setAlexaState
     * directive.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void alexaStateRequest(const std::string& message) = 0;

    /**
     * IPC Client sends this event to request the native client to return the authorization information through the
     * completeAuthorization directive.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void authorizationInfoRequest(const std::string& message) = 0;

    /**
     * IPC Client sends this event to request the native client to return the supported locales through the setLocales
     * directive.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void localesRequest(const std::string& message) = 0;
};

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_SYSTEMHANDLERINTERFACE_H_
