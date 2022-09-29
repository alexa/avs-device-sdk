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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_LIVEVIEWCAMERAHANDLER_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_LIVEVIEWCAMERAHANDLER_H_

#include <memory>
#include <string>

#include <AVSCommon/Utils/RequiresShutdown.h>

#include "IPCServerSampleApp/IPC/IPCHandlerRegistrationInterface.h"
#include "IPCServerSampleApp/IPC/HandlerInterfaces/LiveViewCameraHandlerInterface.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

/**
 * A contract for the handler that will handle the LiveViewCamera namespace messages coming from the IPC client.
 */
class LiveViewCameraHandler
        : public IPCHandlerBase
        , public alexaClientSDK::avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<LiveViewCameraHandler> {
public:
    /**
     * Create an instance of @c LiveViewCameraHandler
     *
     * @param ipcHandlerRegistrar Used to register @c LiveViewCameraHandler.
     * @param liveViewCameraComponent Handles the parsed LiveViewCamera payloads.
     * @return Shared pointer to @c LiveViewCameraHandler
     */
    static std::shared_ptr<LiveViewCameraHandler> create(
        std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
        std::shared_ptr<LiveViewCameraHandlerInterface> liveViewCameraComponent);

    /**
     * Sends a renderCamera IPC message to the IPC client.
     *
     * @param startLiveViewPayload The payload of the corresponding StartLiveView directive.
     */
    void renderCamera(const std::string& startLiveViewPayload);

    /**
     * Sends a setCameraState IPC message to the IPC client.
     *
     * @param cameraState Enumerated live view camera state.
     */
    void setCameraState(const std::string& cameraState);

    /**
     * Sends a clearCamera IPC message to the IPC client.
     */
    void clearCamera();

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /// Register this handler for its namespace and its methods for the corresponding namespace functions.
    void registerHandlers();

private:
    /**
     * Constructor.
     *
     * @param ipcHandlerRegistrar Used to register @c LiveViewCameraHandler.
     * @param liveViewCameraComponent Handles the parsed LiveViewCamera payloads.
     */
    LiveViewCameraHandler(
        std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
        std::shared_ptr<LiveViewCameraHandlerInterface> liveViewCameraComponent);

    /// A reference to the @c IPCHandlerRegistrationInterface used to register/deregister @c LiveViewCameraHandler.
    std::shared_ptr<IPCHandlerRegistrationInterface> m_ipcHandlerRegistrar;

    /// A reference to the IPC dispatcher to dispatch messages to the IPC Client.
    std::shared_ptr<IPCDispatcherInterface> m_ipcDispatcher;

    /// A reference to the @c LiveViewCameraHandlerInterface that will handle the parsed payloads.
    std::shared_ptr<LiveViewCameraHandlerInterface> m_liveViewCameraComponent;
};

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_LIVEVIEWCAMERAHANDLER_H_
