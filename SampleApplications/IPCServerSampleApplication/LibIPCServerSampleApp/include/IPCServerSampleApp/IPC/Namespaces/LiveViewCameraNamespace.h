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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_LIVEVIEWCAMERANAMESPACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_LIVEVIEWCAMERANAMESPACE_H_

#include <memory>
#include <string>

#include "IPCServerSampleApp/Messages/Message.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {
namespace IPCNamespaces {

/// The message namespace for LiveViewCamera.
static const char IPC_MSG_NAMESPACE_LIVE_VIEW_CAMERA[] = "LiveViewCamera";

/// The namespace version for LiveViewCamera.
static const int IPC_MSG_VERSION_LIVE_VIEW_CAMERA(1);

/// The message name for renderCamera.
static const char IPC_MSG_NAME_RENDER_CAMERA[] = "renderCamera";

/// The message name for setCameraState.
static const char IPC_MSG_NAME_SET_CAMERA_STATE[] = "setCameraState";

/// The message name for clearCamera.
static const char IPC_MSG_NAME_CLEAR_CAMERA[] = "clearCamera";

/// The startLiveViewPayload key in RenderCamera message.
static const char IPC_MSG_START_LIVE_VIEW_PAYLOAD_TAG[] = "startLiveViewPayload";

/**
 *  The @c RenderCameraMessage informs the IPC Client to present the live view camera stream and UI.
 */
class RenderCameraMessage : public messages::Message {
public:
    /**
     * Constructor.
     *
     * @param startLiveViewPayload StartLiveView directive payload.
     */
    explicit RenderCameraMessage(const std::string& startLiveViewPayload) :
            Message(IPC_MSG_NAMESPACE_LIVE_VIEW_CAMERA, IPC_MSG_VERSION_LIVE_VIEW_CAMERA, IPC_MSG_NAME_RENDER_CAMERA) {
        rapidjson::Value payload(rapidjson::kObjectType);
        rapidjson::Document messageDocument(&alloc());
        payload.AddMember(
            rapidjson::Value(IPC_MSG_START_LIVE_VIEW_PAYLOAD_TAG, alloc()).Move(),
            messageDocument.Parse(startLiveViewPayload),
            alloc());
        setPayload(std::move(payload));
    };
};

/**
 *  The @c SetCameraStateMessage informs the IPC Client of changes in the state of the active live view camera.
 */
class SetCameraStateMessage : public messages::Message {
public:
    /**
     * Constructor.
     *
     * @param cameraState Live view camera state.
     */
    explicit SetCameraStateMessage(const std::string& cameraState) :
            Message(
                IPC_MSG_NAMESPACE_LIVE_VIEW_CAMERA,
                IPC_MSG_VERSION_LIVE_VIEW_CAMERA,
                IPC_MSG_NAME_SET_CAMERA_STATE) {
        setStateInPayload(cameraState);
        addPayload();
    };
};

/**
 *  The @c ClearCameraMessage informs the IPC Client to present the live view camera stream and UI.
 */
class ClearCameraMessage : public messages::Message {
public:
    /**
     * Constructor.
     */
    explicit ClearCameraMessage() :
            Message(IPC_MSG_NAMESPACE_LIVE_VIEW_CAMERA, IPC_MSG_VERSION_LIVE_VIEW_CAMERA, IPC_MSG_NAME_CLEAR_CAMERA) {
        addPayload();
    };
};

}  // namespace IPCNamespaces
}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_LIVEVIEWCAMERANAMESPACE_H_
