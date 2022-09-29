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

#include <acsdk/AudioEncoderInterfaces/AudioEncoderInterface.h>
#include <ACL/Transport/MessageRouterFactoryInterface.h>
#include <acsdkAlerts/Storage/AlertStorageInterface.h>
#include <acsdkAlertsInterfaces/AlertsCapabilityAgentInterface.h>
#include <acsdkApplicationAudioPipelineFactoryInterfaces/ApplicationAudioPipelineFactoryInterface.h>
#include <acsdkBluetoothInterfaces/BluetoothDeviceConnectionRulesProviderInterface.h>
#include <acsdkBluetoothInterfaces/BluetoothLocalInterface.h>
#include <acsdkBluetoothInterfaces/BluetoothNotifierInterface.h>
#include <acsdkBluetoothInterfaces/BluetoothStorageInterface.h>
#include <acsdkDoNotDisturb/DoNotDisturbCapabilityAgent.h>
#include <acsdkExternalMediaPlayer/ExternalMediaPlayer.h>
#include <acsdkInteractionModelInterfaces/InteractionModelNotifierInterface.h>
#include <acsdkManufactory/Component.h>
#include <acsdkNotificationsInterfaces/NotificationsNotifierInterface.h>
#include <acsdkNotificationsInterfaces/NotificationsStorageInterface.h>
#include <acsdkStartupManagerInterfaces/StartupManagerInterface.h>
#include <acsdkSystemClockMonitorInterfaces/SystemClockMonitorInterface.h>
#include <acsdk/CryptoInterfaces/CryptoFactoryInterface.h>
#include <Alexa/AlexaInterfaceMessageSender.h>
#include <AVSCommon/AVS/DialogUXStateAggregator.h>
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
#include <AVSCommon/SDKInterfaces/SystemTimeZoneInterface.h>
#include <AVSCommon/SDKInterfaces/UserInactivityMonitorInterface.h>
#include <AVSCommon/SDKInterfaces/Audio/AudioFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointBuilderInterface.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <Captions/CaptionManagerInterface.h>
#include <acsdkDeviceSetupInterfaces/DeviceSetupInterface.h>
#include <CertifiedSender/CertifiedSender.h>
#include <CertifiedSender/MessageStorageInterface.h>
#include <InterruptModel/InterruptModel.h>
#include <RegistrationManager/CustomerDataManagerInterface.h>
#include <RegistrationManager/RegistrationManagerInterface.h>
#include <RegistrationManager/RegistrationNotifierInterface.h>

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
    std::shared_ptr<acsdkAlertsInterfaces::AlertsCapabilityAgentInterface>,
    std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>,
    std::shared_ptr<acsdkAudioPlayerInterfaces::AudioPlayerInterface>,
    std::shared_ptr<acsdkBluetoothInterfaces::BluetoothLocalInterface>,
    std::shared_ptr<acsdkBluetoothInterfaces::BluetoothNotifierInterface>,
    std::shared_ptr<acsdkEqualizerInterfaces::EqualizerRuntimeSetupInterface>,
    std::shared_ptr<acsdkExternalMediaPlayer::ExternalMediaPlayer>,
    std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerInterface>,
    std::shared_ptr<acsdkNotificationsInterfaces::NotificationsNotifierInterface>,
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface>,
    std::shared_ptr<acsdkStartupManagerInterfaces::StartupManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface>,
    std::shared_ptr<afml::interruptModel::InterruptModel>,
    std::shared_ptr<avsCommon::avs::DialogUXStateAggregator>,
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
    std::shared_ptr<avsCommon::sdkInterfaces::SystemSoundPlayerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::SystemTimeZoneInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::UserInactivityMonitorInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface>,
    acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface>,
    std::shared_ptr<avsCommon::utils::DeviceInfo>,
    std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>,
    std::shared_ptr<acsdkSystemClockMonitorInterfaces::SystemClockMonitorInterface>,
    std::shared_ptr<capabilityAgents::alexa::AlexaInterfaceMessageSender>,
    std::shared_ptr<capabilityAgents::doNotDisturb::DoNotDisturbCapabilityAgent>,
    std::shared_ptr<acsdkInteractionModelInterfaces::InteractionModelNotifierInterface>,
    std::shared_ptr<captions::CaptionManagerInterface>,
    std::shared_ptr<certifiedSender::CertifiedSender>,
    std::shared_ptr<registrationManager::CustomerDataManagerInterface>,
    std::shared_ptr<registrationManager::RegistrationManagerInterface>,
    std::shared_ptr<registrationManager::RegistrationNotifierInterface>,
    std::shared_ptr<settings::DeviceSettingsManager>,
    std::shared_ptr<settings::storage::DeviceSettingStorageInterface>,
    std::shared_ptr<audioEncoderInterfaces::AudioEncoderInterface>,
    std::shared_ptr<acsdkDeviceSetupInterfaces::DeviceSetupInterface>,
    std::shared_ptr<alexaClientSDK::cryptoInterfaces::CryptoFactoryInterface>>;

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
    const std::shared_ptr<registrationManager::CustomerDataManagerInterface>& customerDataManager,
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
    const acsdkExternalMediaPlayer::ExternalMediaPlayer::AdapterCreationMap& adapterCreationMap,
    const std::shared_ptr<avsCommon::sdkInterfaces::SystemTimeZoneInterface>& systemTimeZone,
    const std::shared_ptr<settings::storage::DeviceSettingStorageInterface>& deviceSettingStorage,
    bool startAlertSchedulingOnInitialization,
    const std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface>& audioFactory,
    const std::shared_ptr<acsdkAlerts::storage::AlertStorageInterface>& alertStorage,
    const std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceManagerInterface>& bluetoothDeviceManager,
    const std::shared_ptr<acsdkBluetoothInterfaces::BluetoothStorageInterface>& bluetoothStorage,
    const std::shared_ptr<acsdkBluetoothInterfaces::BluetoothDeviceConnectionRulesProviderInterface>&
        bluetoothConnectionRulesProvider,
    const std::shared_ptr<acsdkNotificationsInterfaces::NotificationsStorageInterface>& notificationsStorage,
    const std::shared_ptr<alexaClientSDK::cryptoInterfaces::CryptoFactoryInterface>& cryptoFactory);

}  // namespace defaultClient
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_DEFAULTCLIENTCOMPONENT_H_
