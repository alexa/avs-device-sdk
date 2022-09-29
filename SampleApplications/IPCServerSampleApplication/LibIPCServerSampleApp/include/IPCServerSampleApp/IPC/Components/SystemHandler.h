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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_SYSTEMHANDLER_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_SYSTEMHANDLER_H_

#include <string>

#include <AVSCommon/Utils/RequiresShutdown.h>

#include "IPCServerSampleApp/IPC/IPCHandlerRegistrationInterface.h"
#include "IPCServerSampleApp/IPC/HandlerInterfaces/SystemHandlerInterface.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

/**
 * A contract for the handler that will handle the System namespace messages coming from the IPC client.
 */
class SystemHandler
        : public IPCHandlerBase
        , public alexaClientSDK::avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<SystemHandler> {
public:
    /**
     * Create an instance of @c SystemHandler
     *
     * @param ipcHandlerRegistrar Used to register @c SystemHandler.
     * @param systemComponent Handles the parsed System payloads.
     * @return Shared pointer to @c SystemHandler
     */
    static std::shared_ptr<SystemHandler> create(
        std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
        std::shared_ptr<SystemHandlerInterface> systemComponent);

    /**
     * Provides the IPC Client with information to complete CBL based authorization.
     *
     * @param url The url to use to complete CBL-based LWA authrorization.
     * @param code The CBL code to use to complete authorization.
     * @param clientId AVS Device Id.
     */
    void completeAuthorization(const std::string& url, const std::string& code, const std::string& clientId);

    /**
     * Informs the IPC Client of changes in the state of the Alexa client connection.
     *
     * @param state Enumerated state of the Alexa client.
     */
    void setAlexaState(const std::string& state);

    /**
     * Informs the IPC Client of changes in Alexa Authorization status.
     *
     * @param state Enumerated authorization state.
     */
    void setAuthorizationState(const std::string& state);

    /**
     * Informs the  IPC Client of changes in supported locales for the SDK.
     *
     * @param localeStr The locale(s) for the device. In single-locale mode, contains one locale string.
     * In multi-locale mode, the first string indicates the primary locale, and any other strings correspond to
     * secondary locales.
     */
    void setLocales(const std::string& localeStr);

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
     * @param ipcHandlerRegistrar Used to register @c SystemHandler.
     * @param systemComponent Handles the parsed System payloads.
     */
    SystemHandler(
        std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
        std::shared_ptr<SystemHandlerInterface> systemComponent);

    /// A reference to the @c IPCHandlerRegistrationInterface used to register/deregister @c SystemHandler.
    std::shared_ptr<IPCHandlerRegistrationInterface> m_ipcHandlerRegistrar;

    /// A reference to the IPC dispatcher to dispatch messages to the IPC Client.
    std::shared_ptr<IPCDispatcherInterface> m_ipcDispatcher;

    /// A reference to the @c SystemHandlerInterface that will handle the parsed payloads.
    std::shared_ptr<SystemHandlerInterface> m_systemComponent;
};

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_SYSTEMHANDLER_H_
