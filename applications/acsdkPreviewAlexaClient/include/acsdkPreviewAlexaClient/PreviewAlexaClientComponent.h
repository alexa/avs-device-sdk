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

#ifndef ACSDKPREVIEWALEXACLIENT_PREVIEWALEXACLIENTCOMPONENT_H_
#define ACSDKPREVIEWALEXACLIENT_PREVIEWALEXACLIENTCOMPONENT_H_

#include <istream>
#include <memory>
#include <vector>

#include <acsdkAlertsInterfaces/AlertsCapabilityAgentInterface.h>
#include <acsdkApplicationAudioPipelineFactoryInterfaces/ApplicationAudioPipelineFactoryInterface.h>
#include <acsdkBluetoothInterfaces/BluetoothLocalInterface.h>
#include <acsdkBluetoothInterfaces/BluetoothNotifierInterface.h>
#include <acsdkCryptoInterfaces/CryptoFactoryInterface.h>
#include <acsdkCryptoInterfaces/KeyStoreInterface.h>
#include <acsdkDeviceSetupInterfaces/DeviceSetupInterface.h>
#include <acsdkEqualizerInterfaces/EqualizerModeControllerInterface.h>
#include <acsdkEqualizerInterfaces/EqualizerRuntimeSetupInterface.h>
#include <acsdkInteractionModelInterfaces/InteractionModelNotifierInterface.h>
#include <acsdkKWDImplementations/AbstractKeywordDetector.h>
#include <acsdkManufactory/Component.h>
#include <acsdkShutdownManagerInterfaces/ShutdownManagerInterface.h>
#include <acsdkStartupManagerInterfaces/StartupManagerInterface.h>
#include <acsdkSystemClockMonitorInterfaces/SystemClockMonitorInterface.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/AVS/Initialization/InitializationParameters.h>
#include <AVSCommon/SDKInterfaces/AudioFocusAnnotation.h>
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/ChannelVolumeFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Diagnostics/DiagnosticsInterface.h>
#include <AVSCommon/SDKInterfaces/ExpectSpeechTimeoutHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/SDKInterfaces/LocaleAssetsManagerInterface.h>
#include <AVSCommon/SDKInterfaces/VisualFocusAnnotation.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerFactoryInterface.h>
#include <CBLAuthDelegate/CBLAuthDelegateStorageInterface.h>
#include <CBLAuthDelegate/CBLAuthRequesterInterface.h>
#include <CertifiedSender/CertifiedSender.h>
#include <InterruptModel/InterruptModel.h>
#include <RegistrationManager/CustomerDataManagerInterface.h>
#include <SampleApp/PlatformSpecificValues.h>
#include <SampleApp/UIManager.h>
#include <SpeechEncoder/SpeechEncoder.h>

#ifdef ANDROID_MEDIA_PLAYER
#include <AndroidUtilities/AndroidSLESEngine.h>
#endif

namespace alexaClientSDK {
namespace acsdkPreviewAlexaClient {

/**
 * Definition of a Manufactory Component for PreviewAlexaClient.
 *
 * This Manufactory will change significantly over the next several releases while manufactory integration
 * is incrementally completed in the SDK. In the meantime, applications should not expect this definition to
 * remain stable.
 */
using PreviewAlexaClientComponent = acsdkManufactory::Component<
    acsdkManufactory::
        Annotated<avsCommon::sdkInterfaces::AudioFocusAnnotation, avsCommon::sdkInterfaces::FocusManagerInterface>,
    acsdkManufactory::
        Annotated<avsCommon::sdkInterfaces::VisualFocusAnnotation, avsCommon::sdkInterfaces::FocusManagerInterface>,
    acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface>,
    std::shared_ptr<acsdkAlertsInterfaces::AlertsCapabilityAgentInterface>,
    std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>,
    std::shared_ptr<acsdkAudioPlayerInterfaces::AudioPlayerInterface>,
    std::shared_ptr<acsdkBluetoothInterfaces::BluetoothLocalInterface>,
    std::shared_ptr<acsdkBluetoothInterfaces::BluetoothNotifierInterface>,
    std::shared_ptr<acsdkDeviceSetupInterfaces::DeviceSetupInterface>,
    std::shared_ptr<acsdkEqualizerInterfaces::EqualizerRuntimeSetupInterface>,
    std::shared_ptr<acsdkExternalMediaPlayer::ExternalMediaPlayer>,
    std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerInterface>,
    std::shared_ptr<acsdkInteractionModelInterfaces::InteractionModelNotifierInterface>,
    std::shared_ptr<acsdkKWDImplementations::AbstractKeywordDetector>,
    std::shared_ptr<acsdkNotificationsInterfaces::NotificationsNotifierInterface>,
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface>,
    std::shared_ptr<acsdkStartupManagerInterfaces::StartupManagerInterface>,
    std::shared_ptr<acsdkSystemClockMonitorInterfaces::SystemClockMonitorInterface>,
    std::shared_ptr<afml::interruptModel::InterruptModel>,
    std::shared_ptr<avsCommon::avs::attachment::AttachmentManagerInterface>,
    std::shared_ptr<avsCommon::avs::AudioInputStream>,
    std::shared_ptr<avsCommon::avs::DialogUXStateAggregator>,
    std::shared_ptr<avsCommon::avs::initialization::AlexaClientSDKInit>,
    std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::AVSGatewayManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeFactoryInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::ExpectSpeechTimeoutHandlerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionMonitorInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderRegistrarInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::SystemSoundPlayerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::SystemTimeZoneInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::UserInactivityMonitorInterface>,
    std::shared_ptr<avsCommon::utils::AudioFormat>,
    std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>,
    std::shared_ptr<avsCommon::utils::DeviceInfo>,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>,
    std::shared_ptr<capabilityAgents::alexa::AlexaInterfaceMessageSender>,
    std::shared_ptr<capabilityAgents::doNotDisturb::DoNotDisturbCapabilityAgent>,
    std::shared_ptr<captions::CaptionManagerInterface>,
    std::shared_ptr<certifiedSender::CertifiedSender>,
    std::shared_ptr<registrationManager::CustomerDataManagerInterface>,
    std::shared_ptr<registrationManager::RegistrationManagerInterface>,
    std::shared_ptr<registrationManager::RegistrationNotifierInterface>,
    std::shared_ptr<sampleApp::UIManager>,
    std::shared_ptr<settings::DeviceSettingsManager>,
    std::shared_ptr<settings::storage::DeviceSettingStorageInterface>,
    std::shared_ptr<speechencoder::SpeechEncoder>,
    std::shared_ptr<acsdkCryptoInterfaces::CryptoFactoryInterface>,
    std::shared_ptr<acsdkCryptoInterfaces::KeyStoreInterface>>;

/**
 * Get the manufactory @c Component for PreviewAlexaClient.
 *
 * @return The manufactory @c Component for PreviewAlexaClient.
 */
PreviewAlexaClientComponent getComponent(
    std::unique_ptr<avsCommon::avs::initialization::InitializationParameters> initParams,
    const std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface>& diagnostics,
    const std::shared_ptr<sampleApp::PlatformSpecificValues>& platformSpecificValues,
    const std::shared_ptr<avsCommon::sdkInterfaces::ExpectSpeechTimeoutHandlerInterface>& expectSpeechTimeoutHandler,
    const std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface>& powerResourceManager);

}  // namespace acsdkPreviewAlexaClient
}  // namespace alexaClientSDK

#endif  // ACSDKPREVIEWALEXACLIENT_PREVIEWALEXACLIENTCOMPONENT_H_
