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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_TEMPLATERUNTIMEHANDLER_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_TEMPLATERUNTIMEHANDLER_H_

#include <string>

#include <acsdk/Sample/TemplateRuntime/TemplateRuntimePresentationAdapterObserverInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

#include "IPCServerSampleApp/IPC/IPCHandlerRegistrationInterface.h"
#include "IPCServerSampleApp/IPC/HandlerInterfaces/TemplateRuntimeHandlerInterface.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

/**
 * A contract for the handler that will handle the TemplateRuntime namespace messages coming from the IPC client.
 */
class TemplateRuntimeHandler
        : public IPCHandlerBase
        , public alexaClientSDK::avsCommon::utils::RequiresShutdown
        , public common::TemplateRuntimePresentationAdapterObserverInterface
        , public std::enable_shared_from_this<TemplateRuntimeHandler> {
public:
    /**
     * Create an instance of @c TemplateRuntimeHandler
     *
     * @param ipcHandlerRegistrar Used to register @c TemplateRuntimeHandler.
     * @param templateRuntimeComponent Pointer to the @c TemplateRuntimeHandlerInterface.
     * @return Shared pointer to @c TemplateRuntimeHandler
     */
    static std::shared_ptr<TemplateRuntimeHandler> create(
        const std::shared_ptr<IPCHandlerRegistrationInterface>& ipcHandlerRegistrar,
        const std::shared_ptr<TemplateRuntimeHandlerInterface>& templateRuntimeComponent);

    /// @name TemplateRuntimePresentationAdapterObserverInterface Functions
    /// @{
    void renderTemplateCard(const std::string& jsonPayload) override;
    void renderPlayerInfoCard(
        const std::string& jsonPayload,
        templateRuntimeInterfaces::TemplateRuntimeObserverInterface::AudioPlayerInfo audioPlayerInfo) override;
    void clearRenderTemplateCard() override;
    void clearPlayerInfoCard() override;
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
     * @param ipcHandlerRegistrar Used to register @c TemplateRuntimeHandler.
     * @param templateRuntimeComponent Pointer to the @c TemplateRuntimeHandlerInterface.
     */
    TemplateRuntimeHandler(
        std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
        std::shared_ptr<TemplateRuntimeHandlerInterface> templateRuntimeComponent);

    /// A reference to the @c IPCHandlerRegistrationInterface used to register/deregister @c TemplateRuntimeHandler.
    std::shared_ptr<IPCHandlerRegistrationInterface> m_ipcHandlerRegistrar;

    /// A reference to the IPC dispatcher to dispatch messages to the IPC Client.
    std::shared_ptr<IPCDispatcherInterface> m_ipcDispatcher;

    /// A reference to the @c TemplateRuntimeHandlerInterface that will handle the parsed payloads.
    std::shared_ptr<TemplateRuntimeHandlerInterface> m_templateRuntimeComponent;
};

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_TEMPLATERUNTIMEHANDLER_H_
