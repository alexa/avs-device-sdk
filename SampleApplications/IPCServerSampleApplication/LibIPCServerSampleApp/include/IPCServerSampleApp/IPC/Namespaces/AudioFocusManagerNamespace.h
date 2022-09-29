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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_AUDIOFOCUSMANAGERNAMESPACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_AUDIOFOCUSMANAGERNAMESPACE_H_

#include "IPCServerSampleApp/Messages/Message.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {
namespace IPCNamespaces {

/// The message namespace for AudioFocusManager.
static const char IPC_MSG_NAMESPACE_AUDIO_FOCUS_MANAGER[] = "AudioFocusManager";

/// The namespace version for AudioFocusManager.
static const int IPC_MSG_VERSION_AUDIO_FOCUS_MANAGER(1);

/// The message name for processChannelResult.
static const char IPC_MSG_NAME_PROCESS_CHANNEL_RESULT[] = "processChannelResult";

/// The result json key in the message.
static const char IPC_MSG_RESULT_TAG[] = "result";

/// The message name for processFocusChanged.
static const char IPC_MSG_NAME_PROCESS_FOCUS_CHANGED[] = "processFocusChanged";

/// The focusState json key in the message.
static const char IPC_MSG_FOCUS_STATE_TAG[] = "focusState";

/**
 * The @c ProcessFocusChangedMessage provides the IPC Client with Focus state changes for the corresponding token.
 */
class ProcessFocusChangedMessage : public messages::Message {
public:
    /**
     * Constructor.
     *
     * @param token The requester token.
     * @param focusState The channel focus state.
     */
    ProcessFocusChangedMessage(unsigned token, avsCommon::avs::FocusState focusState) :
            Message(
                IPC_MSG_NAMESPACE_AUDIO_FOCUS_MANAGER,
                IPC_MSG_VERSION_AUDIO_FOCUS_MANAGER,
                IPC_MSG_NAME_PROCESS_FOCUS_CHANGED) {
        setTokenInPayload(token);
        addMemberInPayload(IPC_MSG_FOCUS_STATE_TAG, focusStateToString(focusState));
        addPayload();
    }
};

/**
 * The @c ProcessChannelResultMessage provides the IPC Client with the result of `acquireChannelRequest` and
 * `releaseChannelRequest` requests processing.
 */
class ProcessChannelResultMessage : public messages::Message {
public:
    /**
     * Constructor.
     *
     * @param token The requester token.
     * @param result The result of channel focus request processing.
     */
    ProcessChannelResultMessage(unsigned token, bool result) :
            Message(
                IPC_MSG_NAMESPACE_AUDIO_FOCUS_MANAGER,
                IPC_MSG_VERSION_AUDIO_FOCUS_MANAGER,
                IPC_MSG_NAME_PROCESS_CHANNEL_RESULT) {
        setTokenInPayload(token);
        addMemberInPayload(IPC_MSG_RESULT_TAG, result ? "true" : "false");
        addPayload();
    }
};

}  // namespace IPCNamespaces
}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_AUDIOFOCUSMANAGERNAMESPACE_H_
