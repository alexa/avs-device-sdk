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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_SESSIONSETUPNAMESPACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_SESSIONSETUPNAMESPACE_H_

#include <string>

#include <rapidjson/document.h>

#include "IPCServerSampleApp/Messages/Message.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {
namespace IPCNamespaces {

/// The version for the IPC Framework
static const char IPC_FRAMEWORK_VERSION[] = "1.0.0";

/// The message namespace for SessionSetup.
static const char IPC_MSG_NAMESPACE_SESSION_SETUP[] = "SessionSetup";

/// The namespace version for SessionSetup.
static const int IPC_MSG_VERSION_SESSION_SETUP(1);

/// The message name for initializeClient.
static const char IPC_MSG_NAME_INIT_CLIENT[] = "initializeClient";

/// The message name for configureClient.
static const char IPC_MSG_NAME_CONFIGURE_CLIENT[] = "configureClient";

/// The IPC Framework version key in the message.
static const char IPC_MSG_IPC_VERSION_TAG[] = "ipcVersion";

/**
 * The @c InitClientMessage contains information for initializing the IPC Client.
 */
class InitClientMessage : public messages::Message {
public:
    /**
     * Constructor.
     *
     * @param ipcVersion The version number for the IPC framework.
     */
    explicit InitClientMessage(const std::string& ipcVersion) :
            Message(IPC_MSG_NAMESPACE_SESSION_SETUP, IPC_MSG_VERSION_SESSION_SETUP, IPC_MSG_NAME_INIT_CLIENT) {
        addMemberInPayload(IPC_MSG_IPC_VERSION_TAG, ipcVersion);
        addPayload();
    }
};

/**
 * The @c ConfigureClientMessage contains any configuration required by the IPC client.
 */
class ConfigureClientMessage : public messages::Message {
public:
    /**
     * Constructor.
     *
     * @param payload payload for IPC Client config
     */
    explicit ConfigureClientMessage(const std::string& payload) :
            Message(IPC_MSG_NAMESPACE_SESSION_SETUP, IPC_MSG_VERSION_SESSION_SETUP, IPC_MSG_NAME_CONFIGURE_CLIENT) {
        rapidjson::Document messageDocument(&alloc());
        setPayload(std::move(messageDocument.Parse(payload)));
    }
};

}  // namespace IPCNamespaces
}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_SESSIONSETUPNAMESPACE_H_
