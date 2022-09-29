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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_ALEXACAPTIONSNAMESPACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_ALEXACAPTIONSNAMESPACE_H_

#include <string>

#include "IPCServerSampleApp/Messages/Message.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {
namespace IPCNamespaces {

/// The message namespace for Captions.
static const char IPC_MSG_NAMESPACE_CAPTIONS[] = "AlexaCaptions";

/// The namespace version for Captions.
static const int IPC_MSG_VERSION_CAPTIONS(1);

/// The message name for renderCaptions.
static const char IPC_MSG_NAME_RENDER_CAPTIONS[] = "renderCaptions";

/// The message name for setCaptionsState.
static const char IPC_MSG_NAME_SET_CAPTIONS_STATE[] = "setCaptionsState";

/// The enabled json key in the message.
static const char IPC_MSG_NAME_ENABLED_TAG[] = "enabled";

/**
 *  The @c RenderCaptionsMessage instructs the IPC Client to render Alexa captions
 */
class RenderCaptionsMessage : public messages::Message {
public:
    /**
     * Constructor.
     *
     * @param payload The RenderCaptionsMessage payload to serialize.
     */
    explicit RenderCaptionsMessage(const std::string& payload) :
            Message(IPC_MSG_NAMESPACE_CAPTIONS, IPC_MSG_VERSION_CAPTIONS, IPC_MSG_NAME_RENDER_CAPTIONS) {
        setParsedPayload(payload);
    };
};

/**
 *  The @c SetCaptionsStateMessage instructs the IPC Client to set the state of Alexa captions
 */
class SetCaptionsStateMessage : public messages::Message {
public:
    /**
     * Constructor.
     *
     * @param enabled Boolean flag to set json enabled for the SetCaptionsStateMessage
     */
    explicit SetCaptionsStateMessage(const bool enabled) :
            Message(IPC_MSG_NAMESPACE_CAPTIONS, IPC_MSG_VERSION_CAPTIONS, IPC_MSG_NAME_SET_CAPTIONS_STATE) {
        setEnabledInPayload(enabled);
        addPayload();
    };
};

}  // namespace IPCNamespaces
}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_ALEXACAPTIONSNAMESPACE_H_
