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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_TEMPLATERUNTIMEHANDLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_TEMPLATERUNTIMEHANDLERINTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

/// Render Template window id json key in the message.
static const char RENDER_TEMPLATE_WINDOW_ID_TAG[] = "renderTemplateWindowId";

/// Render Player Info window id json key in the message.
static const char RENDER_PLAYER_INFO_WINDOW_ID_TAG[] = "renderPlayerInfoWindowId";

/**
 * A contract to handle TemplateRuntime.
 */
class TemplateRuntimeHandlerInterface {
public:
    /**
     * Destructor
     */
    virtual ~TemplateRuntimeHandlerInterface() = default;

    /**
     * IPC Client sends this event to report the windowId’s used in the IPC Client to display RenderTemplate and
     * RenderPlayerInfo payloads to the Alexa client for purposes of presentation orchestration.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void windowIdReport(const std::string& message) = 0;
};

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_TEMPLATERUNTIMEHANDLERINTERFACE_H_
