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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_WINDOWMANAGERHANDLER_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_WINDOWMANAGERHANDLER_H_

#include <string>

#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

#include "IPCServerSampleApp/IPC/IPCHandlerRegistrationInterface.h"
#include "IPCServerSampleApp/IPC/HandlerInterfaces/WindowManagerHandlerInterface.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

/**
 * A contract for the handler that will handle the WindowManager namespace messages coming from the IPC client.
 */
class WindowManagerHandler
        : public IPCHandlerBase
        , public alexaClientSDK::avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<WindowManagerHandler> {
public:
    /**
     * Create an instance of @c WindowManagerHandler
     *
     * @param ipcHandlerRegistrar Used to register @c WindowManagerHandler.
     * @param windowManagerComponent Handles the parsed WindowManager payloads.
     * @return Shared pointer to @c WindowManagerHandler
     */
    static std::shared_ptr<WindowManagerHandler> create(
        std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
        std::shared_ptr<WindowManagerHandlerInterface> windowManagerComponent);

    /**
     * Sends a setVisualCharacteristics IPC message to the IPC client.
     *
     * @param displayCharacteristicOpt Serialized configuration object for the Alexa.Display capability (optional).
     * @param interactionModesOpt Serialized configuration object for the Alexa.InteractionMode capability (optional).
     * @param windowTemplatesOpt Serialized configuration object for the Alexa.DisplayWindow capability (optional).
     */
    void dispatchSetVisualCharacteristics(
        const avsCommon::utils::Optional<std::string>& displayCharacteristicOpt,
        const avsCommon::utils::Optional<std::string>& interactionModesOpt,
        const avsCommon::utils::Optional<std::string>& windowTemplatesOpt);

    /**
     * Informs the IPC client to clear the content of the given window.
     *
     * @param windowId Id of the window to clear in the IPC client.
     */
    void dispatchClearWindow(const std::string& windowId);

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
     * @param ipcHandlerRegistrar Used to register @c WindowManagerHandler.
     * @param windowManagerComponent Handles the parsed WindowManager payloads.
     */
    WindowManagerHandler(
        std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
        std::shared_ptr<WindowManagerHandlerInterface> windowManagerComponent);

    /// A reference to the @c IPCHandlerRegistrationInterface used to register/deregister @c WindowManagerHandler.
    std::shared_ptr<IPCHandlerRegistrationInterface> m_ipcHandlerRegistrar;

    /// A reference to the IPC dispatcher to dispatch messages to the IPC Client.
    std::shared_ptr<IPCDispatcherInterface> m_ipcDispatcher;

    /// A reference to the @c WindowManagerHandlerInterface that will handle the parsed payloads.
    std::shared_ptr<WindowManagerHandlerInterface> m_windowManagerComponent;
};

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_WINDOWMANAGERHANDLER_H_
