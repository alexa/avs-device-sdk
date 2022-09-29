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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_ALEXACAPTIONSHANDLER_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_ALEXACAPTIONSHANDLER_H_

#include <string>

#include <AVSCommon/Utils/RequiresShutdown.h>

#include "IPCServerSampleApp/IPC/IPCHandlerRegistrationInterface.h"
#include "IPCServerSampleApp/RenderCaptionsInterface.h"
#include "IPCServerSampleApp/SmartScreenCaptionStateManager.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

/**
 * A contract for the handler that will handle the AlexaCaptions namespace messages coming from the IPC client.
 */
class AlexaCaptionsHandler
        : public IPCHandlerBase
        , public RenderCaptionsInterface
        , public alexaClientSDK::avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<AlexaCaptionsHandler> {
public:
    /**
     * Create an instance of @c AlexaCaptionsHandler
     *
     * @param ipcHandlerRegistrar Used to register @c AlexaCaptionsHandler.
     * @param miscStorage An implementation of MiscStorageInterface
     * @return Shared pointer to @c AlexaCaptionsHandler.
     */
    static std::shared_ptr<AlexaCaptionsHandler> create(
        std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
        std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> miscStorage);

    /// @name RenderCaptionsInterface Function
    /// @{
    void renderCaptions(const std::string& payload) override;
    /// @}

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
     * @param ipcHandlerRegistrar Used to register @c AlexaCaptionsHandler.
     * @param miscStorage An implementation of MiscStorageInterface.
     */
    AlexaCaptionsHandler(
        std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
        std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> miscStorage);

    /// A reference to the @c IPCHandlerRegistrationInterface used to register/deregister IPC Handler.
    std::shared_ptr<IPCHandlerRegistrationInterface> m_ipcHandlerRegistrar;

    /// A reference to the IPC dispatcher to dispatch messages to the IPC Client.
    std::shared_ptr<IPCDispatcherInterface> m_ipcDispatcher;

    /// CaptionManager to manage settings for captions
    SmartScreenCaptionStateManager m_captionManager;
};

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_ALEXACAPTIONSHANDLER_H_
