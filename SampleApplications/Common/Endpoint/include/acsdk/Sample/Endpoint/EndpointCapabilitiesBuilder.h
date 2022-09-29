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

#ifndef ACSDK_SAMPLE_ENDPOINT_ENDPOINTCAPABILITIESBUILDER_H_
#define ACSDK_SAMPLE_ENDPOINT_ENDPOINTCAPABILITIESBUILDER_H_

#include <list>
#include <memory>
#include <unordered_set>

#include <acsdk/Sample/Endpoint/EndpointFocusAdapter.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointCapabilitiesBuilderInterface.h>

#include <acsdk/Sample/Endpoint/EndpointAlexaLauncherHandler.h>

#ifdef ALEXA_KEYPAD_CONTROLLER
#include <acsdk/Sample/Endpoint/EndpointAlexaKeypadControllerHandler.h>
#endif

#ifdef ALEXA_PLAYBACK_CONTROLLER
#include <acsdkAlexaPlaybackControllerInterfaces/AlexaPlaybackControllerObserverInterface.h>
#include <acsdk/Sample/Endpoint/EndpointAlexaPlaybackControllerHandler.h>
#endif

#ifdef ALEXA_SEEK_CONTROLLER
#include <acsdk/Sample/Endpoint/EndpointAlexaSeekControllerHandler.h>
#endif

#ifdef ALEXA_VIDEO_RECORDER
#include <acsdk/Sample/Endpoint/EndpointAlexaVideoRecorderHandler.h>
#endif

#ifdef ALEXA_CHANNEL_CONTROLLER
#include <acsdk/Sample/Endpoint/EndpointAlexaChannelControllerHandler.h>
#endif

#ifdef ALEXA_RECORD_CONTROLLER
#include <acsdk/Sample/Endpoint/EndpointAlexaRecordControllerHandler.h>
#endif

#ifdef ALEXA_REMOTE_VIDEO_PLAYER
#include <acsdk/Sample/Endpoint/EndpointAlexaRemoteVideoPlayerHandler.h>
#endif

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {
/**
 * An implementation of a @c EndpointCapabilitiesBuilderInterface.
 */
class EndpointCapabilitiesBuilder
        : public alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesBuilderInterface {
public:
    /**
     * Constructor.
     * @param focusAdapter The @c EndpointFocusAdapter to handle visual and audio focus. Set to nullptr by default.
     */
    EndpointCapabilitiesBuilder(std::shared_ptr<EndpointFocusAdapter> focusAdapter = nullptr);

    /// @name @c EndpointCapabilitiesBuilderInterface methods.
    /// @{
    std::pair<std::list<Capability>, std::list<std::shared_ptr<alexaClientSDK::avsCommon::utils::RequiresShutdown>>>
    buildCapabilities(
        const alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>&
            responseSender,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>&
            exceptionSender) override;
    /// @}

    /**
     * Gets the Alexa launcher handler.
     * @return A shared pointer to the Alexa launcher hanlder.
     */
    std::shared_ptr<EndpointAlexaLauncherHandler> getAlexaLauncherHandler() const;

private:
    /// Mutex to protect concurrent calls to EndpointCapabilitiesBuilder class
    std::mutex m_buildCapabilitiesMutex;

    /// Flag to check if capabilities have already been built
    bool m_capabilitiesBuilt;

    /// The @c EndpointFocusAdapter used to manage audio focus.
    std::shared_ptr<EndpointFocusAdapter> m_focusAdapter;

    /// Handler for Launcher directives
    std::shared_ptr<EndpointAlexaLauncherHandler> m_launcherHandler;

#ifdef ALEXA_KEYPAD_CONTROLLER
    /// Handler for Keypad Controller directives
    std::shared_ptr<EndpointAlexaKeypadControllerHandler> m_keypadControllerHandler;
#endif

#ifdef ALEXA_PLAYBACK_CONTROLLER
    /// Handler for Playback Controller directives
    std::shared_ptr<EndpointAlexaPlaybackControllerHandler> m_playbackControllerHandler;
#endif

#ifdef ALEXA_SEEK_CONTROLLER
    /// Handler for Seek Controller directives
    std::shared_ptr<EndpointAlexaSeekControllerHandler> m_seekControllerHandler;
#endif

#ifdef ALEXA_VIDEO_RECORDER
    /// Handler for Video Recorder directives
    std::shared_ptr<EndpointAlexaVideoRecorderHandler> m_videoRecorderHandler;
#endif

#ifdef ALEXA_CHANNEL_CONTROLLER
    /// Handler for Channel Controller directives
    std::shared_ptr<EndpointAlexaChannelControllerHandler> m_channelControllerHandler;
#endif

#ifdef ALEXA_RECORD_CONTROLLER
    /// Handler for Record Controller directives
    std::shared_ptr<EndpointAlexaRecordControllerHandler> m_recordControllerHandler;
#endif

#ifdef ALEXA_REMOTE_VIDEO_PLAYER
    /// Handler for Remote Video Player directives.
    std::shared_ptr<EndpointAlexaRemoteVideoPlayerHandler> m_remoteVideoPlayerHandler;
#endif
};

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ACSDK_SAMPLE_ENDPOINT_ENDPOINTCAPABILITIESBUILDER_H_
