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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_IPCHANDLERBASE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_IPCHANDLERBASE_H_

#include <memory>
#include <string>
#include <unordered_map>

#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

/**
 * A Base class to assist IPC components to handle messages coming from IPC client.
 */
class IPCHandlerBase {
public:
    /**
     * Invokes a method to handle an IPC message.
     * @param methodName Name of the method to be invoked.
     * @param message Message to be handled by a concrete method.
     */
    void invokeMethod(const std::string& methodName, const std::string& message);

    /// Alias for the message handler function.
    using IPCMessageHandler = std::function<void(const std::string&)>;

    /**
     * Destructor.
     */
    ~IPCHandlerBase();

protected:
    /**
     * Base IPC handler constructor. Note: We do not expect this class to be instantiated on its own.
     *
     * @param namespaceName Namespace of the realized IPC handler class.
     * @param executor Executor context to support operations asynchronously.
     */
    IPCHandlerBase(
        const std::string& namespaceName,
        std::shared_ptr<alexaClientSDK::avsCommon::utils::threading::Executor> executor);

    /**
     * Registers the method that handles an IPC message.
     *
     * @param methodName Name of the IPC method for which handler has to be registered.
     * @param handlerFunction Function reference to be invoked for the said method.
     * @return true is successful.
     */
    bool registerIPCMessageHandlerMethod(const std::string& methodName, IPCMessageHandler handlerFunction);

    /**
     * De-registers the method that handles an IPC message.
     *
     * @param methodName Name of the IPC method for which handler has to be registered.
     * @return true is successful.
     */
    bool deregisterIPCMessageHandlerMethod(const std::string& methodName);

    /// Name of the namespace of the realized IPC handler class.
    std::string m_namespaceName;

    /// Dictionary to hold the IPC message handler methods.
    std::unordered_map<std::string, IPCMessageHandler> m_messageHandlers;

    /// Executor context to support operations asynchronously.
    std::shared_ptr<alexaClientSDK::avsCommon::utils::threading::Executor> m_executor;
};

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_IPCHANDLERBASE_H_
