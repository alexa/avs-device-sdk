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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_COMMUNICATIONSNAMESPACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_COMMUNICATIONSNAMESPACE_H_

#include "IPCServerSampleApp/Messages/Message.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {
namespace IPCNamespaces {

/// The message namespace for Comms.
static const char IPC_MSG_NAMESPACE_COMMUNICATIONS[] = "Communications";

/// The namespace version for Communications.
static const int IPC_MSG_VERSION_COMMUNICATIONS(1);

/// The callState json key in the message.
static const char IPC_MSG_CALL_STATE_TAG[] = "callState";

/// The message name for callStateChange.
static const char IPC_MSG_NAME_CALL_STATE_CHANGE[] = "callStateChange";

/// The callType json key in the message.
static const char IPC_MSG_CALL_TYPE_TAG[] = "callType";

/// The previousSipUserAgentState json key in the message.
static const char IPC_MSG_PREVIOUS_SIP_USER_AGENT_STATE_TAG[] = "previousSipUserAgentState";

/// The currentSipUserAgentState json key in the message.
static const char IPC_MSG_CURRENT_SIP_USER_AGENT_STATE_TAG[] = "currentSipUserAgentState";

/// The displayName json key in the message.
static const char IPC_MSG_DISPLAY_NAME_TAG[] = "displayName";

/// The endpointLabel json key in the message.
static const char IPC_MSG_END_POINT_LABEL_TAG[] = "endpointLabel";

/// The inboundCalleeName json key in the message.
static const char IPC_MSG_INBOUND_CALLEE_NAME_TAG[] = "inboundCalleeName";

/// The callProviderType json key in the message.
static const char IPC_MSG_CALL_PROVIDER_TYPE_TAG[] = "callProviderType";

/// The inboundRingtoneUrl json key in the message.
static const char IPC_MSG_INBOUND_RINGTONE_URL_TAG[] = "inboundRingtoneUrl";

/// The outboundRingbackUrl json key in the message.
static const char IPC_MSG_OUTBOUND_RINGBACK_URL_TAG[] = "outboundRingbackUrl";

/// The isDropIn json key in the message.
static const char IPC_MSG_IS_DROP_IN_TAG[] = "isDropIn";

/**
 * The @c CallStateChangeMessage contains information for communicating call state info to the GUI Client.
 */
class CallStateChangeMessage : public messages::Message {
public:
    /**
     * Constructor.
     *
     * @param callStateInfo The Comms client call state info.
     */
    CallStateChangeMessage(const avsCommon::sdkInterfaces::CallStateObserverInterface::CallStateInfo& callStateInfo) :
            Message(IPC_MSG_NAMESPACE_COMMUNICATIONS, IPC_MSG_VERSION_COMMUNICATIONS, IPC_MSG_NAME_CALL_STATE_CHANGE) {
        std::ostringstream oss;
        oss << callStateInfo.callState;
        addMemberInPayload(IPC_MSG_CALL_STATE_TAG, oss.str());
        addMemberInPayload(IPC_MSG_CALL_TYPE_TAG, callStateInfo.callType);
        addMemberInPayload(IPC_MSG_PREVIOUS_SIP_USER_AGENT_STATE_TAG, callStateInfo.previousSipUserAgentState);
        addMemberInPayload(IPC_MSG_CURRENT_SIP_USER_AGENT_STATE_TAG, callStateInfo.currentSipUserAgentState);
        addMemberInPayload(IPC_MSG_DISPLAY_NAME_TAG, callStateInfo.displayName);
        addMemberInPayload(IPC_MSG_END_POINT_LABEL_TAG, callStateInfo.endpointLabel);
        addMemberInPayload(IPC_MSG_INBOUND_CALLEE_NAME_TAG, callStateInfo.inboundCalleeName);
        addMemberInPayload(IPC_MSG_CALL_PROVIDER_TYPE_TAG, callStateInfo.callProviderType);
        addMemberInPayload(IPC_MSG_INBOUND_RINGTONE_URL_TAG, callStateInfo.inboundRingtoneUrl);
        addMemberInPayload(IPC_MSG_OUTBOUND_RINGBACK_URL_TAG, callStateInfo.outboundRingbackUrl);
        addMemberInPayload(IPC_MSG_IS_DROP_IN_TAG, callStateInfo.isDropIn);
        addPayload();
    }
};

}  // namespace IPCNamespaces
}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_COMMUNICATIONSNAMESPACE_H_
