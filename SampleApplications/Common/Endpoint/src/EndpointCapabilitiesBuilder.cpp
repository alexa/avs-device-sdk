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

#include "acsdk/Sample/Endpoint/EndpointCapabilitiesBuilder.h"

#include <AVSCommon/Utils/Logger/Logger.h>

#ifdef ALEXA_LAUNCHER
#include <acsdkAlexaLauncher/AlexaLauncherFactory.h>
#endif

#ifdef ALEXA_KEYPAD_CONTROLLER
#include <acsdkAlexaKeypadController/AlexaKeypadControllerFactory.h>
#endif

#ifdef ALEXA_PLAYBACK_CONTROLLER
#include <acsdkAlexaPlaybackController/AlexaPlaybackControllerFactory.h>
#endif

#ifdef ALEXA_SEEK_CONTROLLER
#include <acsdkAlexaSeekController/AlexaSeekControllerFactory.h>
#endif

#ifdef ALEXA_VIDEO_RECORDER
#include <acsdkAlexaVideoRecorder/AlexaVideoRecorderFactory.h>
#endif

#ifdef ALEXA_CHANNEL_CONTROLLER
#include <acsdk/AlexaChannelController/AlexaChannelControllerFactory.h>
#endif

#ifdef ALEXA_RECORD_CONTROLLER
#include <acsdk/AlexaRecordController/AlexaRecordControllerFactory.h>
#endif

#ifdef ALEXA_REMOTE_VIDEO_PLAYER
#include <acsdk/AlexaRemoteVideoPlayer/AlexaRemoteVideoPlayerFactory.h>
#endif

/// String to identify log entries originating from this file.
#define TAG "EndpointCapabilitiesBuilder"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

EndpointCapabilitiesBuilder::EndpointCapabilitiesBuilder(std::shared_ptr<EndpointFocusAdapter> focusAdapter) :
        m_capabilitiesBuilt(false),
        m_focusAdapter{std::move(focusAdapter)} {
    ACSDK_DEBUG5(LX(__func__));
}

#ifdef ENABLE_VIDEO_CONTROLLERS
/*
 * Helper method to populate the capabilities and requires shutdown lists for the different CapabilityAgents
 *
 * @param [in] capabilityConfigurationInterface The capabilityConfigurationInterface of the capability agent being
 * implemented.
 * @param [in] directiveHandler The interface used to handle the directives.
 * @param [in] requiresShutdown Instance of @c RequiresShutdown used for cleaning up the capability agent during
 * shutdown.
 * @param [out] capabilities The list of Capability Objects to be updated.
 * @param [out] requireShutdownObjects The list of requiresShutdown objects that this capability agent needs to be
 * added to.
 */
static void populateLists(
    const std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface>& capabilityConfigurationInterface,
    const std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface>& directiveHandler,
    const std::shared_ptr<avsCommon::utils::RequiresShutdown>& requiresShutdown,
    std::list<EndpointCapabilitiesBuilder::Capability>& capabilities,
    std::list<std::shared_ptr<alexaClientSDK::avsCommon::utils::RequiresShutdown>>& requireShutdownObjects) {
    if (!capabilityConfigurationInterface || !directiveHandler || !requiresShutdown) {
        ACSDK_ERROR(LX("populateListsFailed").d("reason", "nullCapabilityAgentData"));
        return;
    }
    auto configurations = capabilityConfigurationInterface->getCapabilityConfigurations();
    if (configurations.empty() || configurations.find(nullptr) != configurations.end()) {
        ACSDK_ERROR(LX("populateListsFailed").d("reason", "invalidConfiguration"));
        return;
    }
    for (const auto& configurationPtr : configurations) {
        auto& configuration = *configurationPtr;
        capabilities.push_back({configuration, directiveHandler});
    }
    requireShutdownObjects.push_back(requiresShutdown);
}
#endif  // ENABLE_VIDEO_CONTROLLERS

std::pair<
    std::list<alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesBuilderInterface::Capability>,
    std::list<std::shared_ptr<alexaClientSDK::avsCommon::utils::RequiresShutdown>>>
EndpointCapabilitiesBuilder::buildCapabilities(
    const alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>&
        responseSender,
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>&
        exceptionSender) {
    ACSDK_DEBUG5(LX(__func__));
    std::pair<
        std::list<
            alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesBuilderInterface::Capability>,
        std::list<std::shared_ptr<alexaClientSDK::avsCommon::utils::RequiresShutdown>>>
        retValue;
    std::list<alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesBuilderInterface::Capability>
        capabilities;
    std::list<std::shared_ptr<alexaClientSDK::avsCommon::utils::RequiresShutdown>> requireShutdownObjects;

    std::lock_guard<std::mutex> lock(m_buildCapabilitiesMutex);

    if (m_capabilitiesBuilt) {
        ACSDK_ERROR(LX(__func__).d("buildCapabilitiesFailed reason", "Capabilities already built!"));
        return retValue;
    }

#ifdef ALEXA_LAUNCHER
    m_launcherHandler = EndpointAlexaLauncherHandler::create(endpointId);
    if (m_launcherHandler) {
        acsdkAlexaLauncher::AlexaLauncherFactory launcherFactory;
        auto launcherCA = launcherFactory.create(
            endpointId, m_launcherHandler, contextManager, responseSender, exceptionSender, false, true);

        if (launcherCA.hasValue()) {
            populateLists(
                launcherCA.value().capabilityConfigurationInterface,
                launcherCA.value().directiveHandler,
                launcherCA.value().requiresShutdown,
                capabilities,
                requireShutdownObjects);
        } else {
            ACSDK_ERROR(LX("Failed to create Alexa launcher CA!"));
        }
    } else {
        ACSDK_ERROR(LX("Failed to create Alexa launcher handler!"));
    }
#endif  // ALEXA_LAUNCHER

#ifdef ALEXA_KEYPAD_CONTROLLER
    auto keypadControllerHandler = EndpointAlexaKeypadControllerHandler::create(endpointId);
    if (keypadControllerHandler) {
        acsdkAlexaKeypadController::AlexaKeypadControllerFactory keypadControllerFactory;
        auto keypadControllerCA = keypadControllerFactory.create(
            endpointId, keypadControllerHandler, contextManager, responseSender, exceptionSender);

        if (keypadControllerCA.hasValue()) {
            populateLists(
                keypadControllerCA.value().capabilityConfigurationInterface,
                keypadControllerCA.value().directiveHandler,
                keypadControllerCA.value().requiresShutdown,
                capabilities,
                requireShutdownObjects);
        } else {
            ACSDK_ERROR(LX("Failed to create Alexa keypad controller CA!"));
        }
    } else {
        ACSDK_ERROR(LX("Failed to create Alexa keypad controller handler!"));
    }
#endif  // ALEXA_KEYPAD_CONTROLLER

#ifdef ALEXA_PLAYBACK_CONTROLLER
    auto playbackControllerHandler = EndpointAlexaPlaybackControllerHandler::create(endpointId, m_focusAdapter);
    if (playbackControllerHandler) {
        acsdkAlexaPlaybackController::AlexaPlaybackControllerFactory playbackControllerFactory;

#ifdef ALEXA_PLAYBACK_STATE_REPORTER
        // passing isProactivelyReported and isRetrievable as true for sending PlaybackStateReporter events
        auto playbackControllerCA = playbackControllerFactory.create(
            endpointId, playbackControllerHandler, contextManager, responseSender, exceptionSender, true, true);
#else
        auto playbackControllerCA = playbackControllerFactory.create(
            endpointId, playbackControllerHandler, contextManager, responseSender, exceptionSender, false, false);
#endif

        if (playbackControllerCA.hasValue()) {
            populateLists(
                playbackControllerCA.value().capabilityConfigurationInterface,
                playbackControllerCA.value().directiveHandler,
                playbackControllerCA.value().requiresShutdown,
                capabilities,
                requireShutdownObjects);
        } else {
            ACSDK_ERROR(LX("Failed to create Alexa playback controller CA!"));
        }
    } else {
        ACSDK_ERROR(LX("Failed to create Alexa playback controller handler!"));
    }
#endif  // ALEXA_PLAYBACK_CONTROLLER

#ifdef ALEXA_SEEK_CONTROLLER
    auto seekControllerHandler = EndpointAlexaSeekControllerHandler::create(endpointId);
    if (seekControllerHandler) {
        acsdkAlexaSeekController::AlexaSeekControllerFactory seekControllerFactory;
        auto seekControllerCA = seekControllerFactory.create(
            endpointId, seekControllerHandler, contextManager, responseSender, exceptionSender, true);

        if (seekControllerCA.hasValue()) {
            populateLists(
                seekControllerCA.value().capabilityConfigurationInterface,
                seekControllerCA.value().directiveHandler,
                seekControllerCA.value().requiresShutdown,
                capabilities,
                requireShutdownObjects);
        } else {
            ACSDK_ERROR(LX("Failed to create Alexa seek controller CA!"));
        }
    } else {
        ACSDK_ERROR(LX("Failed to create Alexa seek controller handler!"));
    }
#endif  // ALEXA_SEEK_CONTROLLER

#ifdef ALEXA_VIDEO_RECORDER
    m_videoRecorderHandler = EndpointAlexaVideoRecorderHandler::create(endpointId);
    if (m_videoRecorderHandler) {
        acsdkAlexaVideoRecorder::AlexaVideoRecorderFactory videoRecorderFactory;
        auto videoRecorderCA = videoRecorderFactory.create(
            endpointId, m_videoRecorderHandler, contextManager, responseSender, exceptionSender);

        if (videoRecorderCA.hasValue()) {
            populateLists(
                videoRecorderCA.value().capabilityConfigurationInterface,
                videoRecorderCA.value().directiveHandler,
                videoRecorderCA.value().requiresShutdown,
                capabilities,
                requireShutdownObjects);
        } else {
            ACSDK_ERROR(LX("Failed to create video recorder CA!"));
        }
    } else {
        ACSDK_ERROR(LX("Failed to create video recorder handler!"));
    }
#endif  // ALEXA_VIDEO_RECORDER

#ifdef ALEXA_CHANNEL_CONTROLLER
    m_channelControllerHandler = EndpointAlexaChannelControllerHandler::create(endpointId);
    if (m_channelControllerHandler) {
        alexaChannelController::AlexaChannelControllerFactory channelControllerFactory;
        // passing isProactivelyReported and isRetrievable as true for sending ChannelController events
        auto channelControllerCA = channelControllerFactory.create(
            endpointId, m_channelControllerHandler, contextManager, responseSender, exceptionSender, true, true);

        if (channelControllerCA.hasValue()) {
            populateLists(
                channelControllerCA.value().capabilityConfigurationInterface,
                channelControllerCA.value().directiveHandler,
                channelControllerCA.value().requiresShutdown,
                capabilities,
                requireShutdownObjects);
        } else {
            ACSDK_ERROR(LX("Failed to create Alexa Channel Controller CA!"));
        }
    } else {
        ACSDK_ERROR(LX("Failed to create Alexa Channel Controller handler!"));
    }
#endif  // ALEXA_CHANNEL_CONTROLLER

#ifdef ALEXA_RECORD_CONTROLLER
    m_recordControllerHandler = EndpointAlexaRecordControllerHandler::create(endpointId);
    if (m_recordControllerHandler) {
        alexaRecordController::AlexaRecordControllerFactory recordControllerFactory;
        auto recordControllerCA = recordControllerFactory.create(
            endpointId, m_recordControllerHandler, contextManager, responseSender, exceptionSender, true);

        if (recordControllerCA.hasValue()) {
            populateLists(
                recordControllerCA.value().capabilityConfigurationInterface,
                recordControllerCA.value().directiveHandler,
                recordControllerCA.value().requiresShutdown,
                capabilities,
                requireShutdownObjects);
        } else {
            ACSDK_ERROR(LX("Failed to create Alexa Record Controller CA!"));
        }
    } else {
        ACSDK_ERROR(LX("Failed to create Alexa Record Controller handler!"));
    }
#endif  // ALEXA_RECORD_CONTROLLER

#ifdef ALEXA_REMOTE_VIDEO_PLAYER
    m_remoteVideoPlayerHandler = EndpointAlexaRemoteVideoPlayerHandler::create(endpointId, m_focusAdapter);
    if (m_remoteVideoPlayerHandler) {
        alexaRemoteVideoPlayer::AlexaRemoteVideoPlayerFactory remoteVideoPlayerFactory;
        auto remoteVideoPlayerCA = remoteVideoPlayerFactory.create(
            endpointId, m_remoteVideoPlayerHandler, contextManager, responseSender, exceptionSender);

        if (remoteVideoPlayerCA.hasValue()) {
            populateLists(
                remoteVideoPlayerCA.value().capabilityConfigurationInterface,
                remoteVideoPlayerCA.value().directiveHandler,
                remoteVideoPlayerCA.value().requiresShutdown,
                capabilities,
                requireShutdownObjects);
        } else {
            ACSDK_ERROR(LX("Failed to create Remote Video Player CA!"));
        }
    } else {
        ACSDK_ERROR(LX("Failed to create Remote Video Player handler!"));
    }
#endif  // ALEXA_REMOTE_VIDEO_PLAYER

    retValue.first.swap(capabilities);
    retValue.second.swap(requireShutdownObjects);
    m_capabilitiesBuilt = true;
    return retValue;
}

std::shared_ptr<EndpointAlexaLauncherHandler> EndpointCapabilitiesBuilder::getAlexaLauncherHandler() const {
    return m_launcherHandler;
}

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK
