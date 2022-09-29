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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_IPCROUTER_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_IPCROUTER_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

#include <Communication/MessageListenerInterface.h>
#include <Communication/MessagingServerInterface.h>

#include "IPCServerSampleApp/IPC/IPCDispatcherInterface.h"
#include "IPCServerSampleApp/IPC/IPCHandlerRegistrationInterface.h"
#include "IPCServerSampleApp/IPC/IPCHandlerBase.h"
#include "IPCServerSampleApp/IPC/IPCVersionManager.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

/**
 * The function of this class is responsible for managing the messages to and from IPC client.
 * Following are the rules:
 * 1. On initialization, router will register IPC messaging contracts used for sending and receiving messages over IPC.
 * 2. Router will also register all the feature IPC handlers that implements IPC channel for their respective
 *    namespaces. It can de-register the IPC components.
 * 3. While registered, router will provide a concrete ipc dispatcher (implemented by self).
 * 4. When router receives a message, it sniffs the namespace from header and passes on the payload/message to the
 *    handler and request to `invokeMethod`. Note that access to handler component registry is time-bound and
 *    thread-safe. To accommodate these attributes, this class deliberately avoids executor pattern and instead relies
 *    on mutex lock to minimize the critical section.
 * 5. When handler gets a message that needs to be communicated over IPC, router constructs appropriate message and
 *    dispatches.
 */
class IPCRouter
        : public IPCHandlerRegistrationInterface
        , public alexaClientSDK::avsCommon::utils::RequiresShutdown {
public:
    /**
     * Constructs an instance of an IPC Router.
     *
     * @param messagingServer An implementation of @c MessagingServerInterface.
     * @param ipcDispatcher An instance of a message dispatcher to IPC Client.
     * @param ipcVersionManager An instance of a @c IPCVersionManager.
     * @return an instance of IPCRouter.
     */
    static std::shared_ptr<IPCRouter> create(
        std::shared_ptr<communication::MessagingServerInterface> messagingServer,
        std::shared_ptr<IPCDispatcherInterface> ipcDispatcher,
        std::shared_ptr<IPCVersionManager> ipcVersionManager);

    /**
     * Called when a new message needs to be routed to registered ipc component.
     *
     * @note Blocking in this handler will block delivery of further messages.
     * @param message a serialized message that needs to be routed to respective IPC component.
     */
    void onMessage(const std::string& message);

    /// @name IPCHandlerRegistrationInterface Functions
    /// @{
    std::shared_ptr<IPCDispatcherInterface> registerHandler(
        const std::string& ipcNamespace,
        std::weak_ptr<IPCHandlerBase> handler) override;
    bool deregisterHandler(const std::string& ipcNamespace) override;
    /// @}

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

private:
    /// Inner struct to wrap a weak handler instance.
    struct HandlerInstance {
        const std::string& ipcNamespace;
        std::weak_ptr<IPCHandlerBase> handler;
    };

    /**
     * Create an instance of an IPC Router.
     *
     * @param messagingServer An implementation of @c MessagingServerInterface
     * @param ipcDispatcher An instance of a message dispatcher to IPC Client.
     * @param ipcVersionManager An instance of a @c IPCVersionManager.
     */
    IPCRouter(
        std::shared_ptr<communication::MessagingServerInterface> messagingServer,
        std::shared_ptr<IPCDispatcherInterface> ipcDispatcher,
        const std::shared_ptr<IPCVersionManager> ipcVersionManager);

    /**
     * If registered, gets the handler instance from the router registry.
     * Note: Caller should ensure this call is thread-safe; must lock handler registry.
     *
     * @param ipcNamespace Namespace for the IPC handler.
     * @return If available, returns the ipc handler instance else nullptr.
     */
    std::shared_ptr<IPCHandlerBase> getHandlerRegisteredLocked(const std::string& ipcNamespace);

    /// A dictionary used for storing registered IPC handlers.
    std::unordered_map<std::string, HandlerInstance> m_handlerMap;

    /// The server implementation.
    std::shared_ptr<communication::MessagingServerInterface> m_messagingServer;

    /// The Listener to receive the messages.
    std::shared_ptr<communication::MessageListenerInterface> m_messageListener;

    /// The IPC Version Manager
    std::shared_ptr<IPCVersionManager> m_ipcVersionManager;

    /// The dispatcher for sending message to IPC client.
    std::shared_ptr<IPCDispatcherInterface> m_ipcDispatcher;

    /// Mutex to ensure handlerMap is thread-safe.
    std::mutex m_mutex;
};

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_IPCROUTER_H_
