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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_AUDIOFOCUSMANAGERHANDLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_AUDIOFOCUSMANAGERHANDLERINTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

/// AVS interface json key in the message.
static const char AVS_INTERFACE_TAG[] = "avsInterface";

/// The token json key in the message.
static const char AUDIO_FOCUS_MANAGER_TOKEN_TAG[] = "token";

/// The channel name key in the message.
static const char CHANNEL_NAME_TAG[] = "channelName";

/// The content type key in the message.
static const char CONTENT_TYPE_TAG[] = "contentType";

/**
 * A contract to handle AudioFocusManager.
 */
class AudioFocusManagerHandlerInterface {
public:
    /**
     * Destructor
     */
    virtual ~AudioFocusManagerHandlerInterface() = default;

    /**
     * IPC Client sends this event to request the SDK to acquire audio focus for the given channel.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void acquireChannelRequest(const std::string& message) = 0;

    /**
     * IPC Client sends this event to request the SDK to release audio focus for the given channel.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void releaseChannelRequest(const std::string& message) = 0;

    /**
     * IPC Client sends this event to inform the SDK that it has received the processFocusChanged directive.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void focusChangedReport(const std::string& message) = 0;
};

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_AUDIOFOCUSMANAGERHANDLERINTERFACE_H_
