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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_APLCLIENTHANDLER_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_APLCLIENTHANDLER_H_

#include <string>

#include <AVSCommon/Utils/RequiresShutdown.h>

#include "IPCServerSampleApp/IPC/IPCHandlerRegistrationInterface.h"
#include "IPCServerSampleApp/IPC/HandlerInterfaces/APLClientHandlerInterface.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

/**
 * A contract for the handler that will handle the APLClient namespace messages coming from the IPC client.
 */
class APLClientHandler
        : public IPCHandlerBase
        , public alexaClientSDK::avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<APLClientHandler> {
public:
    /**
     * Create an instance of @c APLClientHandler
     *
     * @param ipcHandlerRegistrar Used to register @c APLClientHandler.
     * @param aplClientComponent Handles the parsed APLClient payloads.
     * @return Shared pointer to @c APLClientHandler
     */
    static std::shared_ptr<APLClientHandler> create(
        std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
        std::shared_ptr<APLClientHandlerInterface> aplClientComponent);

    /**
     * Sends a createRenderer IPC message to the IPC client.
     *
     * @param windowId Identifies which window to render the specified APL document.
     * @param token Unique identifier for the APL document.
     */
    void dispatchCreateRenderer(const std::string& windowId, const std::string& token);

    /**
     * Sends a sendMessageToViewhost IPC message to the IPC client.
     *
     * @param windowId Identifies the APL window/document instance for which the APL core message is intended.
     * @param payload Opaque serialized APL Core message payload to be provided to the IPC APL Viewhost.
     */
    void dispatchSendMessageToViewhost(const std::string& windowId, const std::string& payload);

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
     * @param ipcHandlerRegistrar Used to register @c APLClientHandler.
     * @param aplClientComponent Handles the parsed APLClient payloads.
     */
    APLClientHandler(
        std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
        std::shared_ptr<APLClientHandlerInterface> aplClientComponent);

    /// A reference to the @c IPCHandlerRegistrationInterface used to register/deregister @c APLClientHandler.
    std::shared_ptr<IPCHandlerRegistrationInterface> m_ipcHandlerRegistrar;

    /// A reference to the IPC dispatcher to dispatch messages to the IPC Client.
    std::shared_ptr<IPCDispatcherInterface> m_ipcDispatcher;

    /// A reference to the @c APLClientHandlerInterface that will handle the parsed payloads.
    std::shared_ptr<APLClientHandlerInterface> m_aplClientComponent;
};

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_APLCLIENTHANDLER_H_
