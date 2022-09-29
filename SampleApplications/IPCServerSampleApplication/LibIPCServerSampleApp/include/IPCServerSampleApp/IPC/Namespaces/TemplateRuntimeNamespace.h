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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_TEMPLATERUNTIMENAMESPACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_TEMPLATERUNTIMENAMESPACE_H_

#include <string>

#include "IPCServerSampleApp/Messages/Message.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {
namespace IPCNamespaces {

/// The message namespace for TemplateRuntime.
static const char IPC_MSG_NAMESPACE_TEMPLATE_RUNTIME[] = "TemplateRuntime";

/// The namespace version for TemplateRuntime.
static const int IPC_MSG_VERSION_TEMPLATE_RUNTIME(1);

/// The message name for clearPlayerInfoCard.
static const char IPC_MSG_NAME_CLEAR_PLAYER_INFO_CARD[] = "clearPlayerInfoCard";

/// The message name for clearTemplateCard.
static const char IPC_MSG_NAME_CLEAR_TEMPLATE_CARD[] = "clearTemplateCard";

/// The message name for renderPlayerInfo.
static const char IPC_MSG_NAME_RENDER_PLAYER_INFO[] = "renderPlayerInfo";

/// The audioPlayerState json key in the message.
static const char IPC_MSG_AUDIO_PLAYER_STATE_TAG[] = "audioPlayerState";

/// The audioOffset json key in the message.
static const char IPC_MSG_AUDIO_OFFSET_TAG[] = "audioOffset";

/// The message handler name for renderTemplate.
static const char IPC_MSG_NAME_RENDER_TEMPLATE[] = "renderTemplate";

/**
 *  The @c RenderPlayerInfoMessage instructs the GUI Client to display visual metadata associated with a media item,
 * such as a song or playlist. It contains the datasource and AudioPlayer state information required to synchronize the
 * UI with the active AudioPlayer.
 */
class RenderPlayerInfoMessage : public messages::Message {
public:
    /**
     * Constructor.
     *
     * @param jsonPayload The RenderPlayerInfo payload.
     * @param audioPlayerInfo @c The TemplateRuntimeObserverInterface::AudioPlayerInfo object containing player state
     * and offset values.
     */
    RenderPlayerInfoMessage(
        const std::string& jsonPayload,
        templateRuntimeInterfaces::TemplateRuntimeObserverInterface::AudioPlayerInfo audioPlayerInfo) :
            Message(
                IPC_MSG_NAMESPACE_TEMPLATE_RUNTIME,
                IPC_MSG_VERSION_TEMPLATE_RUNTIME,
                IPC_MSG_NAME_RENDER_PLAYER_INFO) {
        addMemberInPayload(IPC_MSG_AUDIO_PLAYER_STATE_TAG, playerActivityToString(audioPlayerInfo.audioPlayerState));
        addMemberInPayload(IPC_MSG_AUDIO_OFFSET_TAG, audioPlayerInfo.mediaProperties->getAudioItemOffset().count());
        setParsedPayloadInPayload(jsonPayload);
        addPayload();
    }
};

/**
 * The @c RenderTemplateMessage instructs the GUI Client to draw visual metadata to the screen.
 */
class RenderTemplateMessage : public messages::Message {
public:
    /**
     * Constructor.
     *
     * @param jsonPayload The RenderTemplate payload.
     */
    RenderTemplateMessage(const std::string& jsonPayload) :
            Message(
                IPC_MSG_NAMESPACE_TEMPLATE_RUNTIME,
                IPC_MSG_VERSION_TEMPLATE_RUNTIME,
                IPC_MSG_NAME_RENDER_TEMPLATE) {
        setParsedPayload(jsonPayload);
    }
};

/**
 *  The @c ClearPlayerInfoCardMessage instructs the GUI Client to clear the audio media player UI from the screen.
 */
class ClearPlayerInfoCardMessage : public messages::Message {
public:
    /**
     * Constructor.
     *
     */
    ClearPlayerInfoCardMessage() :
            Message(
                IPC_MSG_NAMESPACE_TEMPLATE_RUNTIME,
                IPC_MSG_VERSION_TEMPLATE_RUNTIME,
                IPC_MSG_NAME_CLEAR_PLAYER_INFO_CARD) {
        addPayload();
    }
};

/**
 *  The @c ClearRenderTemplateCardMessage instructs the GUI Client to clear visual content from the screen.
 */
class ClearRenderTemplateCardMessage : public messages::Message {
public:
    /**
     * Constructor.
     *
     */
    ClearRenderTemplateCardMessage() :
            Message(
                IPC_MSG_NAMESPACE_TEMPLATE_RUNTIME,
                IPC_MSG_VERSION_TEMPLATE_RUNTIME,
                IPC_MSG_NAME_CLEAR_TEMPLATE_CARD) {
        addPayload();
    }
};

}  // namespace IPCNamespaces
}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_TEMPLATERUNTIMENAMESPACE_H_
