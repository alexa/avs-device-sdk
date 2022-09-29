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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_DONOTDISTURBNAMESPACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_DONOTDISTURBNAMESPACE_H_

#include "IPCServerSampleApp/Messages/Message.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {
namespace IPCNamespaces {

/// The message namespace for Do Not Disturb.
static const char IPC_MSG_NAMESPACE_DO_NOT_DISTURB[] = "DoNotDisturb";

/// The namespace version for Do Not Disturb.
static const int IPC_MSG_VERSION_DO_NOT_DISTURB(1);

/// The message name for setDoNotDisturbState.
static const char IPC_MSG_NAME_SET_DND_SETTING_CHANGE[] = "setDoNotDisturbState";

/**
 *  The @c DoNotDisturbSettingChange informs the IPC Client of changes to DoNotDisturb state
 */
class SetDoNotDisturbStateMessage : public messages::Message {
public:
    /**
     * Constructor.
     *
     * @param enabled boolean indicating the current state of DoNotDisturb
     */
    explicit SetDoNotDisturbStateMessage(const bool enabled) :
            Message(
                IPC_MSG_NAMESPACE_DO_NOT_DISTURB,
                IPC_MSG_VERSION_DO_NOT_DISTURB,
                IPC_MSG_NAME_SET_DND_SETTING_CHANGE) {
        setEnabledInPayload(enabled);
        addPayload();
    };
};

}  // namespace IPCNamespaces
}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_DONOTDISTURBNAMESPACE_H_
