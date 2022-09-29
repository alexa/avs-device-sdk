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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_APLCLIENTNAMESPACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_APLCLIENTNAMESPACE_H_

#include <string>

#include "IPCServerSampleApp/Messages/Message.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {
namespace IPCNamespaces {

/// The message namespace for APLClient.
static const char IPC_MSG_NAMESPACE_APL[] = "APLClient";

/// The namespace version for APL.
static const int IPC_MSG_VERSION_APL(1);

/// The message name for sendMessageToViewhost.
static const char IPC_MESSAGE_NAME_VIEWHOST_MESSAGE[] = "sendMessageToViewhost";

/// The message name for createRenderer.
static const char IPC_MESSAGE_NAME_CREATE_RENDERER[] = "createRenderer";

/**
 * The @c AplViewhostMessage provides drawing updates to the GUI Client's APL renderer.
 */
class AplViewhostMessage : public messages::Message {
public:
    /**
     * Constructor.
     *
     * @param windowId The target windowId for this message.
     * @param payload The APL Core message object to serialize.
     */
    AplViewhostMessage(const std::string& windowId, const std::string& payload) :
            Message(IPC_MSG_NAMESPACE_APL, IPC_MSG_VERSION_APL, IPC_MESSAGE_NAME_VIEWHOST_MESSAGE) {
        setWindowIdInPayload(windowId);
        setParsedPayloadInPayload(payload);
        addPayload();
    }
};

/**
 * The @c AplCreateRendererMessage provides the GUI Client with information to trigger an APL document render in the
 * targeted window.
 */
class AplCreateRendererMessage : public messages::Message {
public:
    /**
     * Constructor.
     *
     * @param windowId The id of the window to target with the APL document render.
     * @param token The presentation token of the APL document to render
     */
    AplCreateRendererMessage(const std::string& windowId, const std::string& token) :
            Message(IPC_MSG_NAMESPACE_APL, IPC_MSG_VERSION_APL, IPC_MESSAGE_NAME_CREATE_RENDERER) {
        setWindowIdInPayload(windowId);
        setTokenInPayload(token);
        addPayload();
    }
};

}  // namespace IPCNamespaces
}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_APLCLIENTNAMESPACE_H_
