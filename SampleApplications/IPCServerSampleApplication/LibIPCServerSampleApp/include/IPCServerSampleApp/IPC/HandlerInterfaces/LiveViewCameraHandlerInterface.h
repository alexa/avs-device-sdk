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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_LIVEVIEWCAMERAHANDLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_LIVEVIEWCAMERAHANDLERINTERFACE_H_

#include <memory>
#include <string>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

/// The enabled json key in the message.
static const char CAMERA_ENABLED_TAG[] = "enabled";

/// The window id json key in the message.
static const char CAMERA_WINDOW_ID_TAG[] = "windowId";

/**
 * A contract to handle LiveViewCamera.
 */
class LiveViewCameraHandlerInterface {
public:
    /**
     * Destructor
     */
    virtual ~LiveViewCameraHandlerInterface() = default;

    /**
     * IPC client has changed the state of the camera microphone used for 2-way communication.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void cameraMicrophoneStateChanged(const std::string& message) = 0;

    /**
     * IPC client rendered the first frame of the live view camera.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void cameraFirstFrameRendered(const std::string& message) = 0;

    /**
     * IPC client sends this event to report the windowId used for displaying live view camera.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void windowIdReport(const std::string& message) = 0;
};

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_LIVEVIEWCAMERAHANDLERINTERFACE_H_
