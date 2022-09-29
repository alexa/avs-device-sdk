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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_IPCHANDLERREGISTRATIONINTERFACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_IPCHANDLERREGISTRATIONINTERFACE_H_
#include <string>
#include <memory>

#include "IPCServerSampleApp/IPC/IPCDispatcherInterface.h"
#include "IPCServerSampleApp/IPC/IPCHandlerBase.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

/**
 * A contract to register IPC message handler for a given namespace.
 */
class IPCHandlerRegistrationInterface {
public:
    /**
     * Default Destructor.
     */
    virtual ~IPCHandlerRegistrationInterface() = default;

    /**
     * Registers a IPC handler interface for a given namespace
     *
     * @param ipcNamespace Namespace for the IPC handler to be registered.
     * @param handler A weak pointer to the handler instance.
     * @return an instance of Dispatcher used to sending message over IPC channel.
     */
    virtual std::shared_ptr<IPCDispatcherInterface> registerHandler(
        const std::string& ipcNamespace,
        std::weak_ptr<IPCHandlerBase> handler) = 0;

    /**
     * De-registers a IPC handler interface for a given namespace
     * @param ipcNamespace Namespace for the IPC handler to be de-registered.
     *
     * @return true if successful.
     */
    virtual bool deregisterHandler(const std::string& ipcNamespace) = 0;
};

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_IPCHANDLERREGISTRATIONINTERFACE_H_
