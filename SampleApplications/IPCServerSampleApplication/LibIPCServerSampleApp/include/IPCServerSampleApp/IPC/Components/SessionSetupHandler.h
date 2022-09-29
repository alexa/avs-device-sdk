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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_SESSIONSETUPHANDLER_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_SESSIONSETUPHANDLER_H_

#include <string>

#include <AVSCommon/Utils/RequiresShutdown.h>

#include "IPCServerSampleApp/IPC/IPCHandlerRegistrationInterface.h"
#include "IPCServerSampleApp/IPC/HandlerInterfaces/SessionSetupHandlerInterface.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

/**
 * A contract for the handler that will handle the SessionSetup namespace messages coming from the IPC client.
 */
class SessionSetupHandler
        : public IPCHandlerBase
        , public alexaClientSDK::avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<SessionSetupHandler> {
public:
    /**
     * Create an instance of @c SessionSetupHandler
     *
     * @param ipcHandlerRegistrar Used to register @c SessionSetupHandler.
     * @param sessionSetupComponent Handles the parsed SessionSetup payloads.
     * @return Shared pointer to @c SessionSetupHandler
     */
    static std::shared_ptr<SessionSetupHandler> create(
        std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
        std::shared_ptr<SessionSetupHandlerInterface> sessionSetupComponent);

    /**
     * Sends a configureClient IPC message to the IPC client.
     *
     * @param payload Payload is an opaque object defined by the IPC Client.
     */
    void dispatchConfigureClient(const std::string& payload);

    /**
     * Sends a initializeClient IPC message to the IPC client.
     *
     * @param ipcVersion Version number of the ACSDK IPC framework requesting the connection to the IPC client.
     */
    void dispatchInitializeClient(const std::string& ipcVersion);

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
     * @param ipcHandlerRegistrar Used to register @c SessionSetupHandler.
     * @param sessionSetupComponent Handles the parsed SessionSetup payloads.
     */
    SessionSetupHandler(
        std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
        std::shared_ptr<SessionSetupHandlerInterface> sessionSetupComponent);

    /// A reference to the @c IPCHandlerRegistrationInterface used to register/deregister @c SessionSetupHandler.
    std::shared_ptr<IPCHandlerRegistrationInterface> m_ipcHandlerRegistrar;

    /// A reference to the IPC dispatcher to dispatch messages to the IPC Client.
    std::shared_ptr<IPCDispatcherInterface> m_ipcDispatcher;

    /// A reference to the @c SessionSetupHandlerInterface that will handle the parsed payloads.
    std::shared_ptr<SessionSetupHandlerInterface> m_sessionSetupComponent;
};

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_SESSIONSETUPHANDLER_H_
