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

#ifndef ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_DEFAULTCLIENTCOMPONENT_H_
#define ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_DEFAULTCLIENTCOMPONENT_H_

#include <memory>

#include <ACL/Transport/MessageRouterFactoryInterface.h>
#include <acsdkApplicationAudioPipelineFactoryInterfaces/ApplicationAudioPipelineFactoryInterface.h>
#include <acsdkExternalMediaPlayer/ExternalMediaPlayer.h>
#include <acsdkManufactory/Component.h>
#include <acsdkStartupManagerInterfaces/StartupManagerInterface.h>
#include <Alexa/AlexaInterfaceMessageSender.h>
#include <AVSCommon/SDKInterfaces/AudioFocusAnnotation.h>
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/SDKInterfaces/AVSGatewayManagerInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/ChannelVolumeFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ExpectSpeechTimeoutHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterfaceFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/InternetConnectionMonitorInterface.h>
#include <AVSCommon/SDKInterfaces/Diagnostics/DiagnosticsInterface.h>
#include <AVSCommon/SDKInterfaces/LocaleAssetsManagerInterface.h>
#include <AVSCommon/SDKInterfaces/PowerResourceManagerInterface.h>
#include <AVSCommon/SDKInterfaces/RenderPlayerInfoCardsProviderRegistrarInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointBuilderInterface.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <Captions/CaptionManagerInterface.h>
#include <CertifiedSender/CertifiedSender.h>
#include <CertifiedSender/MessageStorageInterface.h>
#include <InterruptModel/InterruptModel.h>
#include <RegistrationManager/CustomerDataManager.h>

#include "DefaultClient/EqualizerRuntimeSetup.h"
#include "DefaultClient/StubApplicationAudioPipelineFactory.h"

namespace alexaClientSDK {
namespace defaultClient {

/**
 * Definition of a Manufactory component for the Default Client.
 *
 * This component provides backwards compatibility for applications that use the Default Client's
 * non-manufactory method of initialization.
 */
using DefaultClientComponent = acsdkManufactory::Component<
    std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>,
    std::shared_ptr<acsdkAudioPlayerInterfaces::AudioPlayerInterface>,
    std::shared_ptr<acsdkEqualizerInterfaces::EqualizerRuntimeSetupInterface>,
    std::shared_ptr<acsdkExternalMediaPlayer::ExternalMediaPlayer>,  /// Applications should not use this export.
    std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerInterface>,
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface>,
    std::shared_ptr<acsdkStartupManagerInterfaces::StartupManagerInterface>,
    std::shared_ptr<afml::interruptModel::InterruptModel>,
    std::shared_ptr<avsCommon::avs::attachment::AttachmentManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::AVSGatewayManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeFactoryInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::ExpectSpeechTimeoutHandlerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>,
    acsdkManufactory::
        Annotated<avsCommon::sdkInterfaces::AudioFocusAnnotation, avsCommon::sdkInterfaces::FocusManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionMonitorInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderRegistrarInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface>,
    acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface>,
    std::shared_ptr<avsCommon::utils::DeviceInfo>,
    std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>,
    std::shared_ptr<capabilityAgents::alexa::AlexaInterfaceMessageSender>,
    std::shared_ptr<captions::CaptionManagerInterface>,
    std::shared_ptr<certifiedSender::CertifiedSender>,
    std::shared_ptr<registrationManager::CustomerDataManager>>;

/**
 * Get the manufactory @c Component for (legacy) @c DefaultClient initialization.
 *
 * @return The manufactory @c Component for (legacy) @c DefaultClient initialization.
 */
DefaultClientComponent getComponent(
    const std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>& authDelegate,
    const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
    const std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>& localeAssetsManager,
    const std::shared_ptr<avsCommon::utils::DeviceInfo>& deviceInfo,
    const std::shared_ptr<registrationManager::CustomerDataManager>& customerDataManager,
    const std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface>& miscStorage,
    const std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionMonitorInterface>& internetConnectionMonitor,
    const std::shared_ptr<avsCommon::sdkInterfaces::AVSGatewayManagerInterface>& avsGatewayManager,
    const std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface>& capabilitiesDelegate,
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder,
    const std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface>& diagnostics,
    const std::shared_ptr<alexaClientSDK::acl::TransportFactoryInterface>& transportFactory,
    const std::shared_ptr<alexaClientSDK::acl::MessageRouterFactoryInterface>& messageRouterFactory,
    const std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeFactoryInterface>& channelVolumeFactory,
    const std::shared_ptr<avsCommon::sdkInterfaces::ExpectSpeechTimeoutHandlerInterface>& expectSpeechTimeoutHandler,
    const std::shared_ptr<acsdkEqualizerInterfaces::EqualizerRuntimeSetupInterface>& equalizerRuntimeSetup,
    const std::shared_ptr<StubApplicationAudioPipelineFactory>& stubAudioPipelineFactory,
    const std::shared_ptr<avsCommon::utils::mediaPlayer::PooledMediaResourceProviderInterface>&
        audioMediaResourceProvider,
    const std::shared_ptr<certifiedSender::MessageStorageInterface>& messageStorage,
    const std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface>& powerResourceManager,
    const acsdkExternalMediaPlayer::ExternalMediaPlayer::AdapterCreationMap& adapterCreationMap);

}  // namespace defaultClient
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_DEFAULTCLIENTCOMPONENT_H_
