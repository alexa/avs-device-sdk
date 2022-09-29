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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_SESSIONSETUPHANDLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_SESSIONSETUPHANDLERINTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

/// Entries json key in the message.
static const char ENTRIES_TAG[] = "entries";

/// Namespace json key in the message.
static const char NAMESPACE_TAG[] = "namespace";

/// Version json key in the message.
static const char VERSION_TAG[] = "version";

/// IsIPCVersionSupport json key in the message.
static const char IS_IPC_VERSION_SUPPORTED_TAG[] = "isIPCVersionSupported";

/**
 * A contract to handle SessionSetup.
 */
class SessionSetupHandlerInterface {
public:
    /**
     * Destructor
     */
    virtual ~SessionSetupHandlerInterface() = default;

    /**
     * IPC Client sends this event to inform the SDK of the supported IPC namespace versions in the client.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void namespaceVersionsReport(const std::string& message) = 0;

    /**
     * IPC Client sends this event to inform the SDK as a response to the initializeClient directive.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void clientInitialized(const std::string& message) = 0;

    /**
     * IPC Client sends this event to request SDK to send a client-defined config payload for the IPC Client via the
     * configureClient directive.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void clientConfigRequest(const std::string& message) = 0;
};

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_SESSIONSETUPHANDLERINTERFACE_H_
