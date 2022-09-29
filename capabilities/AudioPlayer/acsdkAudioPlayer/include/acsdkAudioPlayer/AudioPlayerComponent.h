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

#ifndef ACSDKAUDIOPLAYER_AUDIOPLAYERCOMPONENT_H_
#define ACSDKAUDIOPLAYER_AUDIOPLAYERCOMPONENT_H_

#include <memory>

#include <acsdkApplicationAudioPipelineFactoryInterfaces/ApplicationAudioPipelineFactoryInterface.h>
#include <acsdkAudioPlayerInterfaces/AudioPlayerInterface.h>
#include <acsdk/CryptoInterfaces/CryptoFactoryInterface.h>
#include <acsdkManufactory/Annotated.h>
#include <acsdkManufactory/Component.h>
#include <acsdkManufactory/Import.h>
#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>
#include <AVSCommon/SDKInterfaces/AudioFocusAnnotation.h>
#include <AVSCommon/SDKInterfaces/ChannelVolumeFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/PlaybackRouterInterface.h>
#include <AVSCommon/SDKInterfaces/RenderPlayerInfoCardsProviderRegistrarInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/DefaultEndpointAnnotation.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointCapabilitiesRegistrarInterface.h>
#include <AVSCommon/Utils/MediaPlayer/PooledMediaResourceProviderInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <Captions/CaptionManagerInterface.h>

namespace alexaClientSDK {
namespace acsdkAudioPlayer {

/**
 * Creates an manufactory component that exports @c AudioPlayer-related implementations.
 *
 * @param configParentKey Optional key to the parent node of AudioPlayer-related configurations in the
 * ConfigurationNode. This is needed to maintain backwards compatibility with AlexaClientSDKConfig.json, which
 * previously placed AudioPlayer configurations under the sampleApp parent node.
 * @return A component.
 */
acsdkManufactory::Component<
    std::shared_ptr<acsdkAudioPlayerInterfaces::AudioPlayerInterface>,
    acsdkManufactory::Import<
        std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderRegistrarInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>>,
    acsdkManufactory::Import<
        std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>>,
    acsdkManufactory::Import<acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::AudioFocusAnnotation,
        avsCommon::sdkInterfaces::FocusManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface>>,
    acsdkManufactory::Import<std::shared_ptr<captions::CaptionManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>>,
    acsdkManufactory::Import<acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<cryptoInterfaces::CryptoFactoryInterface>>>
getComponent(const std::string& configParentKey = "");

/**
 * Creates an manufactory component that exports @c AudioPlayer-related implementations.
 *
 * @deprecated This is for backwards compatibility only to allow the application to inject a
 * PooledMediaResourceProviderInterface; prefer using getComponent.
 * @return A component.
 */
acsdkManufactory::Component<
    std::shared_ptr<acsdkAudioPlayerInterfaces::AudioPlayerInterface>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::mediaPlayer::PooledMediaResourceProviderInterface>>,
    acsdkManufactory::Import<
        std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderRegistrarInterface>>,
    acsdkManufactory::Import<acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::AudioFocusAnnotation,
        avsCommon::sdkInterfaces::FocusManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface>>,
    acsdkManufactory::Import<std::shared_ptr<captions::CaptionManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>>,
    acsdkManufactory::Import<acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<cryptoInterfaces::CryptoFactoryInterface>>>
getBackwardsCompatibleComponent();

}  // namespace acsdkAudioPlayer
}  // namespace alexaClientSDK

#endif  // ACSDKAUDIOPLAYER_AUDIOPLAYERCOMPONENT_H_
