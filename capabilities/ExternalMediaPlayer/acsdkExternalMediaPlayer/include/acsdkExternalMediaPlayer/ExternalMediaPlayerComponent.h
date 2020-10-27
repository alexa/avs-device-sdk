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

#ifndef ACSDKEXTERNALMEDIAPLAYER_EXTERNALMEDIAPLAYERCOMPONENT_H_
#define ACSDKEXTERNALMEDIAPLAYER_EXTERNALMEDIAPLAYERCOMPONENT_H_

#include <memory>

#include <acsdkApplicationAudioPipelineFactoryInterfaces/ApplicationAudioPipelineFactoryInterface.h>
#include <acsdkExternalMediaPlayerInterfaces/ExternalMediaPlayerInterface.h>
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
#include <AVSCommon/SDKInterfaces/SpeakerManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/DefaultEndpointAnnotation.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointCapabilitiesRegistrarInterface.h>
#include <CertifiedSender/CertifiedSender.h>

#include "acsdkExternalMediaPlayer/ExternalMediaPlayer.h"

namespace alexaClientSDK {
namespace acsdkExternalMediaPlayer {

/**
 * Creates an manufactory component that exports a shared pointer to an implementation of @c
 * ExternalMediaPlayerInterface.
 *
 * Although this component currently also exports an @c ExternalMediaPlayer concrete type, applications
 * must not use this export as it is included here only during the transition to manufactory and will be
 * removed in a later release.
 *
 * @return A component.
 */
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
getComponent();

/**
 * Creates an manufactory component that exports a shared pointer to an implementation of @c
 * ExternalMediaPlayerInterface as well as @c ExternalMediaPlayer.
 *
 * This method is provided for backwards compatibility where the ExternalMediaPlayer must create the adapters
 * at initialization. Prefer creating components for ExternalMediaPlayerAdapters such that the adapter can
 * register its own handler with the ExternalMediaPlayer.
 *
 * Although this component currently also exports an @c ExternalMediaPlayer concrete type, applications
 * must not use this export as it is included here only during the transition to manufactory and will be
 * removed in a later release.
 *
 * @param adapterCreationMap Map of AdapterCreationFunctions so that the EMP can create the adapters at
 * initialization.
 * @return A component.
 */
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
    const acsdkExternalMediaPlayer::ExternalMediaPlayer::AdapterCreationMap& adapterCreationMap);

}  // namespace acsdkExternalMediaPlayer
}  // namespace alexaClientSDK

#endif  // ACSDKEXTERNALMEDIAPLAYER_EXTERNALMEDIAPLAYERCOMPONENT_H_