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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_LOGGERHANDLER_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_LOGGERHANDLER_H_

#include <string>

#include <AVSCommon/Utils/RequiresShutdown.h>

#include "IPCServerSampleApp/IPC/IPCHandlerRegistrationInterface.h"
#include "IPCServerSampleApp/IPC/HandlerInterfaces/LoggerHandlerInterface.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

/**
 * A contract for the handler that will handle the Logger namespace messages coming from the IPC client.
 */
class LoggerHandler
        : public IPCHandlerBase
        , public alexaClientSDK::avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<LoggerHandler> {
public:
    /**
     * Create an instance of @c LoggerHandler
     *
     * @param ipcHandlerRegistrar Used to register @c LoggerHandler.
     * @param loggerComponent Handles the parsed Logger payloads.
     * @return Shared pointer to @c LoggerHandler
     */
    static std::shared_ptr<LoggerHandler> create(
        std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
        std::shared_ptr<LoggerHandlerInterface> loggerComponent);

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
     * @param ipcHandlerRegistrar Used to register @c LoggerHandler.
     * @param loggerComponent Handles the parsed Logger payloads.
     */
    LoggerHandler(
        std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
        std::shared_ptr<LoggerHandlerInterface> loggerComponent);

    /// A reference to the @c IPCHandlerRegistrationInterface used to register/deregister @c LoggerHandler.
    std::shared_ptr<IPCHandlerRegistrationInterface> m_ipcHandlerRegistrar;

    /// A reference to the IPC dispatcher to dispatch messages to the IPC Client.
    std::shared_ptr<IPCDispatcherInterface> m_ipcDispatcher;

    /// A reference to the @c LoggerHandlerInterface that will handle the parsed payloads.
    std::shared_ptr<LoggerHandlerInterface> m_loggerComponent;
};

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_LOGGERHANDLER_H_
