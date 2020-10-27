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

#include <acsdkManufactory/ComponentAccumulator.h>

#include "acsdkExternalMediaPlayer/ExternalMediaPlayerComponent.h"

namespace alexaClientSDK {
namespace acsdkExternalMediaPlayer {

/// String to identify log entries originating from this file.
static const std::string TAG("ExternalMediaPlayerComponent");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * Returns an std::function to serve as the factory method for a @c ExternalMediaPlayer given a map of
 * adapter creation methods.
 *
 * @deprecated Applications should prefer to create components for their external media adapters; this
 * is provided for backwards compatibility only.
 *
 * @param adapterCreationMap A map of <player ID: AdapterCreateFunction> such that ExternalMediaPlayer can
 * create the adapters in this map.
 * @return A @c ExternalMediaPlayer.
 */
static std::function<std::shared_ptr<acsdkExternalMediaPlayer::ExternalMediaPlayer>(
    std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderRegistrarInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface>,
    acsdkManufactory::
        Annotated<avsCommon::sdkInterfaces::AudioFocusAnnotation, avsCommon::sdkInterfaces::FocusManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>,
    std::shared_ptr<certifiedSender::CertifiedSender>,
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>,
    std::shared_ptr<acsdkStartupManagerInterfaces::StartupNotifierInterface>,
    acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>,
    std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>)>
getCreateExternalMediaPlayerWithAdapters(
    const acsdkExternalMediaPlayer::ExternalMediaPlayer::AdapterCreationMap& adapterCreationMap) {
    return
        [adapterCreationMap](
            std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderRegistrarInterface>
                renderPlayerInfoCardsRegistrar,
            std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager,
            acsdkManufactory::Annotated<
                avsCommon::sdkInterfaces::AudioFocusAnnotation,
                avsCommon::sdkInterfaces::FocusManagerInterface> audioFocusManager,
            std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> playbackRouter,
            std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
            std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
            std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
            std::shared_ptr<certifiedSender::CertifiedSender> certifiedSender,
            std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface> shutdownNotifier,
            std::shared_ptr<acsdkStartupManagerInterfaces::StartupNotifierInterface> startupNotifier,
            acsdkManufactory::Annotated<
                avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
                avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>
                endpointCapabilitiesRegistrar,
            std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
            std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>
                audioPipelineFactory) -> std::shared_ptr<acsdkExternalMediaPlayer::ExternalMediaPlayer> {
            auto externalMediaPlayer = ExternalMediaPlayer::createExternalMediaPlayerWithAdapters(
                adapterCreationMap,
                audioPipelineFactory,
                messageSender,
                certifiedSender,
                audioFocusManager,
                contextManager,
                exceptionEncounteredSender,
                playbackRouter,
                endpointCapabilitiesRegistrar,
                shutdownNotifier,
                startupNotifier,
                renderPlayerInfoCardsRegistrar,
                metricRecorder,
                speakerManager);

            if (!externalMediaPlayer) {
                ACSDK_ERROR(LX("getCreateExternalMediaPlayerFailed").m("failed to create external media player"));
                return nullptr;
            }

            return externalMediaPlayer;
        };
}

acsdkManufactory::Component<
    std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerInterface>,
    std::shared_ptr<acsdkExternalMediaPlayer::ExternalMediaPlayer>,  /// Applications should not use this export.
    acsdkManufactory::Import<
        std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderRegistrarInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<certifiedSender::CertifiedSender>>,
    acsdkManufactory::Import<std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>>,
    acsdkManufactory::Import<std::shared_ptr<acsdkStartupManagerInterfaces::StartupNotifierInterface>>,
    acsdkManufactory::Import<acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>>
getComponent() {
    return acsdkManufactory::ComponentAccumulator<>()
        .addRetainedFactory(ExternalMediaPlayer::createExternalMediaPlayerInterface)
        .addRequiredFactory(ExternalMediaPlayer::createExternalMediaPlayer);
}

acsdkManufactory::Component<
    std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerInterface>,
    std::shared_ptr<acsdkExternalMediaPlayer::ExternalMediaPlayer>,  /// Applications should not use this export.
    acsdkManufactory::Import<
        std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderRegistrarInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface>>,
    acsdkManufactory::Import<acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::AudioFocusAnnotation,
        avsCommon::sdkInterfaces::FocusManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<certifiedSender::CertifiedSender>>,
    acsdkManufactory::Import<std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>>,
    acsdkManufactory::Import<std::shared_ptr<acsdkStartupManagerInterfaces::StartupNotifierInterface>>,
    acsdkManufactory::Import<acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>,
    acsdkManufactory::Import<
        std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>>>
getBackwardsCompatibleComponent(
    const acsdkExternalMediaPlayer::ExternalMediaPlayer::AdapterCreationMap& adapterCreationMap) {
    return acsdkManufactory::ComponentAccumulator<>()
        .addRetainedFactory(ExternalMediaPlayer::createExternalMediaPlayerInterface)
        .addRequiredFactory(getCreateExternalMediaPlayerWithAdapters(adapterCreationMap));
}

}  // namespace acsdkExternalMediaPlayer
}  // namespace alexaClientSDK
