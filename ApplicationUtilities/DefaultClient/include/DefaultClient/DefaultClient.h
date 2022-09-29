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

#ifndef ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_DEFAULTCLIENT_H_
#define ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_DEFAULTCLIENT_H_

#include <ACL/Transport/MessageRouter.h>
#include <ACL/Transport/MessageRouterFactory.h>
#include <acsdkManufactory/Manufactory.h>
#include <ADSL/DirectiveSequencer.h>
#include <AIP/AudioInputProcessor.h>
#include <AIP/AudioProvider.h>
#include <acsdkAlerts/Storage/AlertStorageInterface.h>
#include <acsdkAlertsInterfaces/AlertsCapabilityAgentInterface.h>
#include <acsdkApplicationAudioPipelineFactoryInterfaces/ApplicationAudioPipelineFactoryInterface.h>
#include <acsdkNotificationsInterfaces/NotificationsNotifierInterface.h>
#include <acsdkSystemClockMonitorInterfaces/SystemClockMonitorInterface.h>
#include <Alexa/AlexaInterfaceCapabilityAgent.h>
#include <Alexa/AlexaInterfaceMessageSender.h>
#include <ApiGateway/ApiGatewayCapabilityAgent.h>
#include <acsdkAudioPlayer/AudioPlayer.h>
#include <acsdkAudioPlayerInterfaces/AudioPlayerObserverInterface.h>
#include <AVSCommon/AVS/Attachment/AttachmentManagerInterface.h>
#include <AVSCommon/AVS/AVSDiscoveryEndpointAttributes.h>
#include <AVSCommon/AVS/DialogUXStateAggregator.h>
#include <AVSCommon/AVS/ExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/SDKInterfaces/AVSGatewayManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Audio/AudioFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceConnectionRuleInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceManagerInterface.h>
#include <AVSCommon/SDKInterfaces/CallManagerInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesDelegateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/ChannelVolumeFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>
#include <AVSCommon/SDKInterfaces/Diagnostics/DiagnosticsInterface.h>
#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/DefaultEndpointAnnotation.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointBuilderInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointInterface.h>
#include <AVSCommon/SDKInterfaces/ExpectSpeechTimeoutHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/InternetConnectionMonitorInterface.h>
#include <AVSCommon/SDKInterfaces/LocaleAssetsManagerInterface.h>
#include <AVSCommon/SDKInterfaces/PowerResourceManagerInterface.h>
#include <AVSCommon/SDKInterfaces/SingleSettingObserverInterface.h>
#include <AVSCommon/SDKInterfaces/SpeechInteractionHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <AVSCommon/SDKInterfaces/SystemTimeZoneInterface.h>
#include <acsdk/TemplateRuntimeInterfaces/TemplateRuntimeObserverInterface.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerFactoryInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/Optional.h>
#include <acsdkBluetoothInterfaces/BluetoothLocalInterface.h>
#include <acsdkBluetoothInterfaces/BluetoothNotifierInterface.h>
#include <acsdkBluetoothInterfaces/BluetoothStorageInterface.h>
#include <acsdkNotificationsInterfaces/NotificationsStorageInterface.h>
#include <Captions/CaptionManagerInterface.h>
#include <Captions/CaptionPresenterInterface.h>
#include <CertifiedSender/CertifiedSender.h>
#include <CertifiedSender/SQLiteMessageStorage.h>
#include <acsdkDeviceSetupInterfaces/DeviceSetupInterface.h>
#include <acsdkDoNotDisturb/DoNotDisturbCapabilityAgent.h>
#include <Endpoints/EndpointRegistrationManager.h>
#include <acsdkEqualizer/EqualizerCapabilityAgent.h>
#include <acsdkEqualizerImplementations/EqualizerController.h>
#include <acsdkEqualizerInterfaces/EqualizerConfigurationInterface.h>
#include <acsdkEqualizerInterfaces/EqualizerModeControllerInterface.h>
#include <acsdkEqualizerInterfaces/EqualizerRuntimeSetupInterface.h>
#include <acsdkEqualizerInterfaces/EqualizerStorageInterface.h>
#include <acsdkExternalMediaPlayer/ExternalMediaPlayer.h>
#include <acsdkExternalMediaPlayerInterfaces/ExternalMediaAdapterHandlerInterface.h>
#include <acsdkShutdownManagerInterfaces/ShutdownManagerInterface.h>
#include <acsdkExternalMediaPlayerInterfaces/ExternalMediaPlayerInterface.h>
#include <acsdkInteractionModelInterfaces/InteractionModelNotifierInterface.h>
#include <acsdkStartupManagerInterfaces/StartupManagerInterface.h>
#include <InterruptModel/InterruptModel.h>

#ifdef ENABLE_PCC
#include <acsdk/PhoneCallControllerInterfaces/Phone/PhoneCallerInterface.h>
#include <acsdk/PhoneCallController/PhoneCallController.h>
#endif

#ifdef ENABLE_MCC
#include <acsdk/MeetingClientControllerInterfaces/Calendar/CalendarClientInterface.h>
#include <acsdk/MeetingClientControllerInterfaces/Meeting/MeetingClientInterface.h>
#include <acsdk/MeetingClientController/MeetingClientController.h>
#endif

#include <acsdk/AudioEncoderInterfaces/AudioEncoderInterface.h>
#include <AVSCommon/SDKInterfaces/SystemSoundPlayerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointInterface.h>
#include <Endpoints/Endpoint.h>
#include <PlaybackController/PlaybackController.h>
#include <PlaybackController/PlaybackRouter.h>
#include <RegistrationManager/RegistrationManagerInterface.h>
#include <RegistrationManager/RegistrationNotifierInterface.h>
#include <RegistrationManager/RegistrationObserverInterface.h>
#include <Settings/DeviceSettingsManager.h>
#include <Settings/Storage/DeviceSettingStorageInterface.h>
#include <SoftwareComponentReporter/SoftwareComponentReporterCapabilityAgent.h>
#include <acsdk/SDKClient/FeatureClientInterface.h>
#include <acsdk/SpeakerManager/Factories.h>
#include <SpeechSynthesizer/SpeechSynthesizer.h>
#include <System/SoftwareInfoSender.h>
#include <System/UserInactivityMonitor.h>

#ifdef ENABLE_REVOKE_AUTH
#include <System/RevokeAuthorizationHandler.h>
#endif

#include "DefaultClient/ConnectionRetryTrigger.h"
#include "DefaultClient/EqualizerRuntimeSetup.h"
#include "DefaultClient/ExternalCapabilitiesBuilderInterface.h"

namespace alexaClientSDK {
namespace defaultClient {

/**
 * This class serves to instantiate each default component with of the SDK with no specializations to provide an
 * "out-of-box" component that users may utilize for AVS interaction.
 */
class DefaultClient
        : public avsCommon::sdkInterfaces::SpeechInteractionHandlerInterface
        , public sdkClient::FeatureClientInterface {
public:
    using DefaultClientSubsetManufactory = acsdkManufactory::Manufactory<
        std::shared_ptr<acsdkAlertsInterfaces::AlertsCapabilityAgentInterface>,
        std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>,
        std::shared_ptr<acsdkAudioPlayerInterfaces::AudioPlayerInterface>,
        std::shared_ptr<acsdkBluetoothInterfaces::BluetoothLocalInterface>,
        std::shared_ptr<acsdkBluetoothInterfaces::BluetoothNotifierInterface>,
        std::shared_ptr<acsdkEqualizerInterfaces::EqualizerRuntimeSetupInterface>,
        std::shared_ptr<acsdkExternalMediaPlayer::ExternalMediaPlayer>,
        std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerInterface>,
        std::shared_ptr<acsdkInteractionModelInterfaces::InteractionModelNotifierInterface>,
        std::shared_ptr<acsdkNotificationsInterfaces::NotificationsNotifierInterface>,
        std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface>,
        std::shared_ptr<acsdkStartupManagerInterfaces::StartupManagerInterface>,
        std::shared_ptr<acsdkSystemClockMonitorInterfaces::SystemClockMonitorInterface>,
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
        std::shared_ptr<capabilityAgents::alexa::AlexaInterfaceMessageSender>,
        std::shared_ptr<capabilityAgents::doNotDisturb::DoNotDisturbCapabilityAgent>,
        std::shared_ptr<captions::CaptionManagerInterface>,
        std::shared_ptr<certifiedSender::CertifiedSender>,
        std::shared_ptr<registrationManager::CustomerDataManagerInterface>,
        std::shared_ptr<registrationManager::RegistrationManagerInterface>,
        std::shared_ptr<registrationManager::RegistrationNotifierInterface>,
        std::shared_ptr<settings::DeviceSettingsManager>,
        std::shared_ptr<settings::storage::DeviceSettingStorageInterface>,
        std::shared_ptr<audioEncoderInterfaces::AudioEncoderInterface>,
        std::shared_ptr<acsdkDeviceSetupInterfaces::DeviceSetupInterface>>;

    using DefaultClientManufactory = acsdkManufactory::Manufactory<
        std::shared_ptr<acsdkAlertsInterfaces::AlertsCapabilityAgentInterface>,
        std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>,
        std::shared_ptr<acsdkAudioPlayerInterfaces::AudioPlayerInterface>,
        std::shared_ptr<acsdkBluetoothInterfaces::BluetoothLocalInterface>,
        std::shared_ptr<acsdkBluetoothInterfaces::BluetoothNotifierInterface>,
        std::shared_ptr<acsdkEqualizerInterfaces::EqualizerRuntimeSetupInterface>,
        std::shared_ptr<acsdkExternalMediaPlayer::ExternalMediaPlayer>,
        std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerInterface>,
        std::shared_ptr<acsdkInteractionModelInterfaces::InteractionModelNotifierInterface>,
        std::shared_ptr<acsdkNotificationsInterfaces::NotificationsNotifierInterface>,
        std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface>,
        std::shared_ptr<acsdkStartupManagerInterfaces::StartupManagerInterface>,
        std::shared_ptr<acsdkSystemClockMonitorInterfaces::SystemClockMonitorInterface>,
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
        std::shared_ptr<capabilityAgents::alexa::AlexaInterfaceMessageSender>,
        std::shared_ptr<capabilityAgents::doNotDisturb::DoNotDisturbCapabilityAgent>,
        std::shared_ptr<captions::CaptionManagerInterface>,
        std::shared_ptr<certifiedSender::CertifiedSender>,
        std::shared_ptr<registrationManager::CustomerDataManagerInterface>,
        std::shared_ptr<registrationManager::RegistrationManagerInterface>,
        std::shared_ptr<registrationManager::RegistrationNotifierInterface>,
        std::shared_ptr<settings::DeviceSettingsManager>,
        std::shared_ptr<settings::storage::DeviceSettingStorageInterface>,
        std::shared_ptr<audioEncoderInterfaces::AudioEncoderInterface>,
        std::shared_ptr<acsdkDeviceSetupInterfaces::DeviceSetupInterface>>;

    /**
     * Creates and initializes a default AVS SDK client. To connect the client to AVS, users should make a call to
     * connect() after creation.
     *
     * @param manufactory @c Manufactory for creating various instances used by DefaultClient.
     * @param ringtoneMediaPlayer The media player to play Comms ringtones.
     * @param ringtoneSpeaker The speaker to control volume of Comms ringtones.
     * @param additionalSpeakers A map of additional speakers to receive volume changes.
#ifdef ENABLE_PCC
     * @param phoneSpeaker Interface to speaker for phone calls.
     * @param phoneCaller Interface for making phone calls.
#endif
#ifdef ENABLE_MCC
     * @param meetingSpeaker Interface to speaker for meetings.
     * @param meetingClient Meeting client for meeting client controller.
     * @param calendarClient Calendar client for meeting client controller.
#endif
#ifdef ENABLE_COMMS_AUDIO_PROXY
     * @param commsMediaPlayer The media player to play Comms calling audio.
     * @param commsSpeaker The speaker to control volume of Comms calling audio.
     * @param sharedDataStream The stream to use which has the audio from microphone.
#endif
     * @param alexaDialogStateObservers Observers that can be used to be notified of Alexa dialog related UX state
     * changes.
     * @param connectionObservers Observers that can be used to be notified of connection status changes.
     * @param isGuiSupported Whether the device supports GUI.
     * @param firmwareVersion The firmware version to report to @c AVS or @c INVALID_FIRMWARE_VERSION.
     * @param sendSoftwareInfoOnConnected Whether to send SoftwareInfo upon connecting to @c AVS.
     * @param softwareInfoSenderObserver Object to receive notifications about sending SoftwareInfo.
     * @param diagnostics Diagnostics interface which provides suite of APIs for diagnostic insight into SDK.
     * @param externalCapabilitiesBuilder Optional object used to build capabilities that are not included in the SDK.
     * @param firstInteractionAudioProvider Optional object used in the first interaction started from
     * the alexa voice service
     * @param sdkClientRegistry Optional object used when the @c SDKClientBuilder is used to construct DefaultClient
     * @return A @c std::unique_ptr to a DefaultClient if all went well or @c nullptr otherwise.
     */
    static std::unique_ptr<DefaultClient> create(
        const std::shared_ptr<DefaultClientSubsetManufactory>& manufactory,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> ringtoneMediaPlayer,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> ringtoneSpeaker,
        const std::multimap<
            avsCommon::sdkInterfaces::ChannelVolumeInterface::Type,
            std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>> additionalSpeakers,
#ifdef ENABLE_PCC
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> phoneSpeaker,
        std::shared_ptr<phoneCallControllerInterfaces::phone::PhoneCallerInterface> phoneCaller,
#endif
#ifdef ENABLE_MCC
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> meetingSpeaker,
        std::shared_ptr<meetingClientControllerInterfaces::meeting::MeetingClientInterface> meetingClient,
        std::shared_ptr<meetingClientControllerInterfaces::calendar::CalendarClientInterface> calendarClient,
#endif
#ifdef ENABLE_COMMS_AUDIO_PROXY
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> commsMediaPlayer,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> commsSpeaker,
        std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> sharedDataStream,
#endif
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface>>
            alexaDialogStateObservers,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>>
            connectionObservers,
        bool isGuiSupported,
        avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion =
            avsCommon::sdkInterfaces::softwareInfo::INVALID_FIRMWARE_VERSION,
        bool sendSoftwareInfoOnConnected = false,
        std::shared_ptr<avsCommon::sdkInterfaces::SoftwareInfoSenderObserverInterface> softwareInfoSenderObserver =
            nullptr,
        std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface> diagnostics = nullptr,
        const std::shared_ptr<ExternalCapabilitiesBuilderInterface>& externalCapabilitiesBuilder = nullptr,
        capabilityAgents::aip::AudioProvider firstInteractionAudioProvider =
            capabilityAgents::aip::AudioProvider::null(),
        const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry = nullptr);

    /**
     * Creates and initializes a default AVS SDK client. To connect the client to AVS, users should make a call to
     * connect() after creation.
     *
     * @deprecated
     * @param deviceInfo DeviceInfo which reflects the device setup credentials.
     * @param customerDataManager CustomerDataManager instance to be used by RegistrationManager and instances of
     * all classes extending CustomDataHandler.
     * @param externalMusicProviderMediaPlayers The map of <players, mediaPlayer> to use to play content from each
     * external music provider.
     * @param externalMusicProviderSpeakers The map of <players, speaker> to use to track volume of each
     * external music provider media player.
     * @param adapterCreationMap The map of <players, adapterCreationMethod> to use when creating the adapters for the
     * different music providers supported by ExternalMediaPlayer.
     * @param speakMediaPlayer The media player to use to play Alexa speech from.
     * @param audioMediaPlayerFactory The media player factory to use to generate players for Alexa audio content.
     * @param alertsMediaPlayer The media player to use to play alerts from.
     * @param notificationsMediaPlayer The media player to play notification indicators.
     * @param bluetoothMediaPlayer The media player to play bluetooth content.
     * @param ringtoneMediaPlayer The media player to play Comms ringtones.
     * @param systemSoundMediaPlayer The media player to play system sounds.
     * @param speakSpeaker The speaker to control volume of Alexa speech.
     * @param audioSpeakers A list of speakers to control volume of Alexa audio content.
     * @param alertsSpeaker The speaker to control volume of alerts.
     * @param notificationsSpeaker The speaker to control volume of notifications.
     * @param bluetoothSpeaker The speaker to control volume of bluetooth.
     * @param ringtoneSpeaker The speaker to control volume of Comms ringtones.
     * @param systemSoundSpeaker The speaker to control volume of system sounds.
     * @param additionalSpeakers A map of additional speakers to receive volume changes.
#ifdef ENABLE_COMMS_AUDIO_PROXY
     * @param commsMediaPlayer The media player to play Comms calling audio.
     * @param commsSpeaker The speaker to control volume of Comms calling audio.
     * @param sharedDataStream The stream to use which has the audio from microphone.
#endif
     * @param equalizerRuntimeSetup Equalizer component runtime setup
     * @param audioFactory The audioFactory is a component that provides unique audio streams.
     * @param authDelegate The component that provides the client with valid LWA authorization.
     * @param alertStorage The storage interface that will be used to store alerts.
     * @param messageStorage The storage interface that will be used to store certified sender messages.
     * @param notificationsStorage The storage interface that will be used to store notification indicators.
     * @param deviceSettingStorage The storage interface that will be used to store device settings.
     * @param bluetoothStorage The storage interface that will be used to store bluetooth data.
     * @param miscStorage The storage interface that will be used to store key / value pairs.
     * @param alexaDialogStateObservers Observers that can be used to be notified of Alexa dialog related UX state
     * changes.
     * @param connectionObservers Observers that can be used to be notified of connection status changes.
     * @param isGuiSupported Whether the device supports GUI.
     * @param capabilitiesDelegate The component that provides the client with the ability to send messages to the
     * Capabilities API.
     * @param contextManager The @c ContextManager which will provide the context for various components.
     * @param transportFactory The object passed in here will be used whenever a new transport object
     * for AVS communication is needed.
     * @param avsGatewayManager The @c AVSGatewayManager instance used to create the ApiGateway CA.
     * @param localeAssetsManager The device locale assets manager.
     * @param enabledConnectionRules The set of @c BluetoothDeviceConnectionRuleInterface instances used to
     * create the Bluetooth CA.
     * @param systemTimezone Optional object used to set the system timezone.
     * @param firmwareVersion The firmware version to report to @c AVS or @c INVALID_FIRMWARE_VERSION.
     * @param sendSoftwareInfoOnConnected Whether to send SoftwareInfo upon connecting to @c AVS.
     * @param softwareInfoSenderObserver Object to receive notifications about sending SoftwareInfo.
     * @param bluetoothDeviceManager The @c BluetoothDeviceManager instance used to create the Bluetooth CA.
     * @param metricRecorder The metric recorder object used to capture metrics.
     * @param powerResourceManager Object to manage power resource.
     * @param diagnostics Diagnostics interface which provides suite of APIs for diagnostic insight into SDK.
     * @param externalCapabilitiesBuilder Optional object used to build capabilities that are not included in the SDK.
     * @param channelVolumeFactory Optional object used to build @c ChannelVolumeInterface in the SDK.
     * @param startAlertSchedulingOnInitialization Whether to start scheduling alerts after client initialization. If
     * this is set to false, no alert scheduling will occur until onSystemClockSynchronized is called.
     * @param messageRouterFactory Object used to instantiate @c MessageRouter in the SDK.
     * @param expectSpeechTimeoutHandler An optional object that applications may provide to specify external handling
of the @c ExpectSpeech directive's timeout. If provided, this function must remain valid for the lifetime of the @c
AudioInputProcessor.
     * @param firstInteractionAudioProvider Optional object used in the first interaction started from
     * the alexa voice service
     * @param cryptoFactory Optional Encryption facilities factory.
     * @param sdkClientRegistry Optional object used when the @c SDKClientBuilder is used to construct DefaultClient
     * @return A @c std::unique_ptr to a DefaultClient if all went well or @c nullptr otherwise.
     */
    static std::unique_ptr<DefaultClient> create(
        std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo,
        std::shared_ptr<registrationManager::CustomerDataManagerInterface> customerDataManager,
        const std::unordered_map<std::string, std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>>&
            externalMusicProviderMediaPlayers,
        const std::unordered_map<std::string, std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>&
            externalMusicProviderSpeakers,
        const acsdkExternalMediaPlayer::ExternalMediaPlayer::AdapterCreationMap& adapterCreationMap,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> speakMediaPlayer,
        std::unique_ptr<avsCommon::utils::mediaPlayer::MediaPlayerFactoryInterface> audioMediaPlayerFactory,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> alertsMediaPlayer,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> notificationsMediaPlayer,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> bluetoothMediaPlayer,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> ringtoneMediaPlayer,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> systemSoundMediaPlayer,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> speakSpeaker,
        std::vector<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>> audioSpeakers,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> alertsSpeaker,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> notificationsSpeaker,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> bluetoothSpeaker,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> ringtoneSpeaker,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> systemSoundSpeaker,
        const std::multimap<
            avsCommon::sdkInterfaces::ChannelVolumeInterface::Type,
            std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>> additionalSpeakers,
#ifdef ENABLE_PCC
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> phoneSpeaker,
        std::shared_ptr<phoneCallControllerInterfaces::phone::PhoneCallerInterface> phoneCaller,
#endif
#ifdef ENABLE_MCC
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> meetingSpeaker,
        std::shared_ptr<meetingClientControllerInterfaces::meeting::MeetingClientInterface> meetingClient,
        std::shared_ptr<meetingClientControllerInterfaces::calendar::CalendarClientInterface> calendarClient,
#endif
#ifdef ENABLE_COMMS_AUDIO_PROXY
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> commsMediaPlayer,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> commsSpeaker,
        std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> sharedDataStream,
#endif
        std::shared_ptr<EqualizerRuntimeSetup> equalizerRuntimeSetup,
        std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface> audioFactory,
        std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
        std::shared_ptr<acsdkAlerts::storage::AlertStorageInterface> alertStorage,
        std::shared_ptr<certifiedSender::MessageStorageInterface> messageStorage,
        std::shared_ptr<acsdkNotificationsInterfaces::NotificationsStorageInterface> notificationsStorage,
        std::shared_ptr<settings::storage::DeviceSettingStorageInterface> deviceSettingStorage,
        std::shared_ptr<acsdkBluetoothInterfaces::BluetoothStorageInterface> bluetoothStorage,
        std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> miscStorage,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface>>
            alexaDialogStateObservers,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>>
            connectionObservers,
        std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionMonitorInterface> internetConnectionMonitor,
        bool isGuiSupported,
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<alexaClientSDK::acl::TransportFactoryInterface> transportFactory,
        std::shared_ptr<avsCommon::sdkInterfaces::AVSGatewayManagerInterface> avsGatewayManager,
        std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface> localeAssetsManager,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceConnectionRuleInterface>>
            enabledConnectionRules = std::unordered_set<
                std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceConnectionRuleInterface>>(),
        std::shared_ptr<avsCommon::sdkInterfaces::SystemTimeZoneInterface> systemTimezone = nullptr,
        avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion =
            avsCommon::sdkInterfaces::softwareInfo::INVALID_FIRMWARE_VERSION,
        bool sendSoftwareInfoOnConnected = false,
        std::shared_ptr<avsCommon::sdkInterfaces::SoftwareInfoSenderObserverInterface> softwareInfoSenderObserver =
            nullptr,
        std::unique_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceManagerInterface> bluetoothDeviceManager =
            nullptr,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder = nullptr,
        std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface> powerResourceManager = nullptr,
        std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface> diagnostics = nullptr,
        const std::shared_ptr<ExternalCapabilitiesBuilderInterface>& externalCapabilitiesBuilder = nullptr,
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeFactoryInterface> channelVolumeFactory =
            speakerManager::createChannelVolumeFactory(),
        bool startAlertSchedulingOnInitialization = true,
        std::shared_ptr<alexaClientSDK::acl::MessageRouterFactoryInterface> messageRouterFactory =
            std::make_shared<alexaClientSDK::acl::MessageRouterFactory>(),
        const std::shared_ptr<avsCommon::sdkInterfaces::ExpectSpeechTimeoutHandlerInterface>&
            expectSpeechTimeoutHandler = nullptr,
        capabilityAgents::aip::AudioProvider firstInteractionAudioProvider =
            capabilityAgents::aip::AudioProvider::null(),
        const std::shared_ptr<alexaClientSDK::cryptoInterfaces::CryptoFactoryInterface>& cryptoFactory = nullptr,
        const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry = nullptr);

    /// @c FeatureClientInterface functions
    /// @{
    bool configure(const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry) override;
    void doShutdown() override;
    /// @}

    /**
     * Connects the client to AVS. After this call, users can observe the state of the connection asynchronously by
     * using a connectionObserver that was passed in to the create() function.
     *
     * @param performReset True if the client wishes to reset the AVS gateway and clear the previous user's
     * registration status as well on connection.
     *
     */
    void connect(bool performReset = true);

    /**
     * Disconnects the client from AVS if it is connected. After the call, users can observer the state of the
     * connection asynchronously by using a connectionObserver that was passed in to the create() function.
     */
    void disconnect();

    /**
     * Get the gateway URL for the AVS connection.
     *
     * @return The URL for the current AVS gateway.
     */
    std::string getAVSGateway();

    /**
     * Stops the foreground activity if there is one. This acts as a "stop" button that can be used to stop an
     * ongoing activity. This call will block until the foreground activity has stopped all user-observable activities.
     */
    void stopForegroundActivity();

    /**
     * Stops all active channels/activities if there are any(has acquired focus atleast once). This acts as a "stop"
     * button that can be used to stop all ongoing activities. This call will block until all the activities has
     * stopped. There is a possibility of race condition of some activity about to start when this API is invoked.
     */
    void stopAllActivities();

    /**
     * This function provides a way for application code to request this object stop any active alert as the result
     * of a user action, such as pressing a physical 'stop' button on the device.
     */
    void localStopActiveAlert();

    /**
     * Adds an observer to be notified of Alexa dialog related UX state.
     *
     * @param observer The observer to add.
     */
    void addAlexaDialogStateObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface> observer);

    /**
     * Removes an observer to be notified of Alexa dialog related UX state.
     *
     * @note This is a synchronous call which can not be made by an observer callback.  Attempting to call
     *     @c removeObserver() from @c DialogUXStateObserverInterface::onDialogUXStateChanged() will result in a
     *     deadlock.
     *
     * @param observer The observer to remove.
     */
    void removeAlexaDialogStateObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface> observer);

    /**
     * Adds an observer to be notified when a message arrives from AVS.
     *
     * @param observer The observer to add.
     */
    void addMessageObserver(std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer);

    /**
     * Removes an observer to be notified when a message arrives from AVS.
     *
     * @param observer The observer to remove.
     */
    void removeMessageObserver(std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer);

    /**
     * Adds an observer to be notified of connection status changes.
     *
     * @param observer The observer to add.
     */
    void addConnectionObserver(std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> observer);

    /**
     * Removes an observer to be notified of connection status changes.
     *
     * @param observer The observer to remove.
     */
    void removeConnectionObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> observer);

    /**
     * Adds an observer to be notified of internet connection status changes.
     *
     * @param observer The observer to add.
     */
    void addInternetConnectionObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionObserverInterface> observer);

    /**
     * Removes an observer to be notified of internet connection status changes.
     *
     * @param observer The observer to remove.
     */
    void removeInternetConnectionObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionObserverInterface> observer);

    /**
     * Adds an observer to be notified of alert state changes.
     *
     * @param observer The observer to add.
     */
    void addAlertsObserver(std::shared_ptr<acsdkAlertsInterfaces::AlertObserverInterface> observer);

    /**
     * Removes an observer to be notified of alert state changes.
     *
     * @param observer The observer to remove.
     */
    void removeAlertsObserver(std::shared_ptr<acsdkAlertsInterfaces::AlertObserverInterface> observer);

    /**
     * Adds an observer to be notified of @c AudioPlayer state changes. This can be used to be be notified of the @c
     * PlayerActivity of the @c AudioPlayer.
     *
     * @param observer The observer to add.
     */
    void addAudioPlayerObserver(std::shared_ptr<acsdkAudioPlayerInterfaces::AudioPlayerObserverInterface> observer);

    /**
     * Removes an observer to be notified of @c AudioPlayer state changes.
     *
     * @param observer The observer to remove.
     */
    void removeAudioPlayerObserver(std::shared_ptr<acsdkAudioPlayerInterfaces::AudioPlayerObserverInterface> observer);

    /**
     * Adds an observer to be notified when a TemplateRuntime directive is received.
     *
     * @param observer The observer to add.
     */
    void addTemplateRuntimeObserver(
        std::shared_ptr<templateRuntimeInterfaces::TemplateRuntimeObserverInterface> observer);

    /**
     * Removes an observer to be notified when a TemplateRuntime directive is received.
     *
     * @param observer The observer to remove.
     */
    void removeTemplateRuntimeObserver(
        std::shared_ptr<templateRuntimeInterfaces::TemplateRuntimeObserverInterface> observer);

    /**
     * Adds an observer to be notified of IndicatorState changes.
     *
     * @param observer The observer to add.
     */
    void addNotificationsObserver(
        std::shared_ptr<acsdkNotificationsInterfaces::NotificationsObserverInterface> observer);

    /**
     * Removes an observer to be notified of IndicatorState changes.
     *
     * @param observer The observer to remove.
     */
    void removeNotificationsObserver(
        std::shared_ptr<acsdkNotificationsInterfaces::NotificationsObserverInterface> observer);

    /**
     * Adds an observer to be notified of ExternalMediaPlayer changes
     *
     * @param observer The observer to add.
     */
    void addExternalMediaPlayerObserver(
        std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerObserverInterface> observer);

    /**
     * Removes an observer to be notified of ExternalMediaPlayer changes.
     *
     * @param observer The observer to remove.
     */
    void removeExternalMediaPlayerObserver(
        std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerObserverInterface> observer);

    /**
     * Adds an observer to be notified of bluetooth device changes.
     *
     * @param observer The @c BluetoothDeviceObserverInterface to add.
     */
    void addBluetoothDeviceObserver(
        std::shared_ptr<acsdkBluetoothInterfaces::BluetoothDeviceObserverInterface> observer);

    /**
     * Removes an observer to be notified of bluetooth device changes.
     *
     * @param observer The @c BluetoothDeviceObserverInterface to remove.
     */
    void removeBluetoothDeviceObserver(
        std::shared_ptr<acsdkBluetoothInterfaces::BluetoothDeviceObserverInterface> observer);

    /**
     * Adds a presenter responsible for displaying formatted captions content.
     *
     * @deprecated Applications should use the manufactory to create their CaptionPresenterInterface and
     * inject it with the CaptionManagerInterface. See the SampleApplicationComponent for an example.
     * @param presenter The @c CaptionPresenterInterface to add.
     */
    void addCaptionPresenter(std::shared_ptr<captions::CaptionPresenterInterface> presenter);

    /**
     * Sets the media players that can produce or control captioned content.
     *
     * @deprecated Applications should use an ApplicationAudioPipelineFactoryInterface to instantiate media players
     * and register them with the CaptionManager when they transition to the manufactory (e.g. see
     * GstreamerAudioPipelineFactory).
     * @param mediaPlayers The @c MediaPlayerInterface instances to add.
     */
    void setCaptionMediaPlayers(
        const std::vector<std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>>& mediaPlayers);

    /*
     * Get a reference to the PlaybackRouter
     *
     * @return shared_ptr to the PlaybackRouter.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> getPlaybackRouter() const;

    /**
     * Adds a SpeakerManagerObserver to be alerted when the volume and mute changes.
     *
     * @param observer The observer to be notified upon volume and mute changes.
     */
    void addSpeakerManagerObserver(std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerObserverInterface> observer);

    /**
     * Removes a SpeakerManagerObserver from being alerted when the volume and mute changes.
     *
     * @param observer The observer to be notified upon volume and mute changes.
     */
    void removeSpeakerManagerObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerObserverInterface> observer);

    /**
     * Get a shared_ptr to the SpeakerManager.
     *
     * @return shared_ptr to the SpeakerManager.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> getSpeakerManager();

    /**
     * Adds a SpeechSynthesizerObserver to be alerted on SpeechSynthesizer state changes
     * @param observer  The observer to be notified upon the state change
     */
    void addSpeechSynthesizerObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::SpeechSynthesizerObserverInterface> observer);

    /**
     * Removes a SpeechSynthesizerObserver from being alerted on SpeechSynthesizer state changes
     * @param observer  The observer to be removed
     */
    void removeSpeechSynthesizerObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::SpeechSynthesizerObserverInterface> observer);

    /**
     * Get a shared_ptr to the RegistrationManager.
     *
     * @return shared_ptr to the RegistrationManager.
     */
    std::shared_ptr<registrationManager::RegistrationManagerInterface> getRegistrationManager();

    /**
     * Adds an @c RegistrationObserverInterface to the @c RegistrationNotifier.
     *
     * @param observer - The @c RegistrationObserverInterfaces to be notified of logout events.
     */
    void addRegistrationObserver(const std::shared_ptr<registrationManager::RegistrationObserverInterface>& observer);

    /**
     * Removes an @c RegistrationObserverInterface to the @c RegistrationNotifier.
     *
     * @param observer - The @c RegistrationObserverInterfaces to be notified of logout events.
     */
    void removeRegistrationObserver(
        const std::shared_ptr<registrationManager::RegistrationObserverInterface>& observer);

#ifdef ENABLE_REVOKE_AUTH
    /**
     * Adds a RevokeAuthorizationObserver to be alerted when a revoke authorization request occurs.
     *
     * @param observer The observer to be notified of revoke authorization requests.
     */
    void addRevokeAuthorizationObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::RevokeAuthorizationObserverInterface> observer);

    /**
     * Removes a RevokeAuthorizationObserver from being alerted when a revoke authorization request occurs.
     *
     * @param observer The observer to no longer be notified of revoke authorization requests.
     */
    void removeRevokeAuthorizationObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::RevokeAuthorizationObserverInterface> observer);
#endif

    /**
     * Get a shared_ptr to the EqualizerController.
     *
     * @note Be sure to release all references to the returned @c EqualizerController before releasing the last
     * reference to the @c DefaultClient.
     * @return shared_ptr to the EqualizerController.
     */
    std::shared_ptr<acsdkEqualizer::EqualizerController> getEqualizerController();

    /**
     * Adds a EqualizerControllerListener to be notified of Equalizer state changes.
     *
     * @param listener The listener to be notified of Equalizer state changes.
     */
    void addEqualizerControllerListener(
        std::shared_ptr<acsdkEqualizerInterfaces::EqualizerControllerListenerInterface> listener);

    /**
     * Removes a EqualizerControllerListener to be notified of Equalizer state changes.
     *
     * @param listener The listener to no longer be notified of Equalizer state changes.
     */
    void removeEqualizerControllerListener(
        std::shared_ptr<acsdkEqualizerInterfaces::EqualizerControllerListenerInterface> listener);

    /**
     * Adds a ContextManagerObserver to be notified of Context state changes.
     *
     * @param observer The observer to be notified of Context state changes.
     */
    void addContextManagerObserver(std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerObserverInterface> observer);

    /**
     * Removes a ContextManagerObserver to be notified of Context state changes.
     *
     * @param observer The observer to no longer be notified of Context state changes.
     */
    void removeContextManagerObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerObserverInterface> observer);

    /**
     * Update the firmware version.
     *
     * @param firmwareVersion The new firmware version.
     * @return Whether the setting was accepted.
     */
    bool setFirmwareVersion(avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion);

    /// @name SpeechInteractionHandlerInterface Methods
    /// @{
    std::future<bool> notifyOfWakeWord(
        capabilityAgents::aip::AudioProvider wakeWordAudioProvider,
        avsCommon::avs::AudioInputStream::Index beginIndex,
        avsCommon::avs::AudioInputStream::Index endIndex,
        std::string keyword,
        std::chrono::steady_clock::time_point startOfSpeechTimestamp,
        std::shared_ptr<const std::vector<char>> KWDMetadata = nullptr) override;

    std::future<bool> notifyOfTapToTalk(
        capabilityAgents::aip::AudioProvider tapToTalkAudioProvider,
        avsCommon::avs::AudioInputStream::Index beginIndex = capabilityAgents::aip::AudioInputProcessor::INVALID_INDEX,
        std::chrono::steady_clock::time_point startOfSpeechTimestamp = std::chrono::steady_clock::now()) override;

    std::future<bool> notifyOfHoldToTalkStart(
        capabilityAgents::aip::AudioProvider holdToTalkAudioProvider,
        std::chrono::steady_clock::time_point startOfSpeechTimestamp = std::chrono::steady_clock::now(),
        avsCommon::avs::AudioInputStream::Index beginIndex =
            capabilityAgents::aip::AudioInputProcessor::INVALID_INDEX) override;

    std::future<bool> notifyOfHoldToTalkEnd() override;

    std::future<bool> notifyOfTapToTalkEnd() override;
    /// }

    /**
     * Retrieves the device settings manager which can be used to access device settings.
     *
     * @return Pointer to the device settings manager.
     */
    std::shared_ptr<settings::DeviceSettingsManager> getSettingsManager();

    /**
     * Creates an endpoint builder which can be used to configure a new endpoint.
     *
     * @note Endpoints must be registered with the Endpoint Registration Manager to be controlled by the device.
     *
     * @return An endpoint builder.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface> createEndpointBuilder();

    /**
     * Registers an endpoint with the @c EndpointRegistrationManagerInterface.
     *
     * @param endpoint A pointer to the @c EndpointInterface to be registered.
     * @return A future that will resolve to the @c RegistrationResult for registering this endpoint.
     */
    std::future<endpoints::EndpointRegistrationManager::RegistrationResult> registerEndpoint(
        std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointInterface> endpoint);

    /**
     * Updates an endpoint with the @c EndpointRegistrationManagerInterface.
     *
     * @param endpointId The @c EndpointIdentifier of the endpoint to update.
     * @param endpointModificationData A pointer to the @c endpointModificationData to be updated.
     * @return A future that will resolve to the @c UpdateResult for updating this endpoint.
     */
    std::future<endpoints::EndpointRegistrationManager::UpdateResult> updateEndpoint(
        const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
        const std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointModificationData>& endpointModificationData);

    /**
     * Deregisters an endpoint with the @c EndpointRegistrationManagerInterface.
     *
     * @param endpointId The @c EndpointIdentifier of the endpoint to deregister.
     * @return A future that will resolve to the @c DeregistratonResult for deregistering this endpoint.
     */
    std::future<endpoints::EndpointRegistrationManager::DeregistrationResult> deregisterEndpoint(
        avsCommon::sdkInterfaces::endpoints::EndpointIdentifier endpointId);

    /**
     * Retrieves the builder for the default endpoint which contains all capabilities associated with this client.
     *
     * This builder can be used to add extra capabilities to the default endpoint.
     *
     * @return The default endpoint builder.
     * @warning The default endpoint can only be configured before you call @c connect(). Adding new components after
     * the client has been connected will fail.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface> getDefaultEndpointBuilder();

    /**
     * Add observer for endpoint registration manager.
     *
     * @param observer The observer to add.
     */
    void addEndpointRegistrationManagerObserver(
        const std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointRegistrationObserverInterface>& observer);

    /**
     * Removes observer for endpoint registration manager.
     *
     * @param observer The observer to remove.
     */
    void removeEndpointRegistrationManagerObserver(
        const std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointRegistrationObserverInterface>& observer);

    /**
     * Adds an @c AudioInputProcessorObserver to be alerted on @c AudioInputProcessor related state changes.
     *
     * @param observer The observer to be notified upon @c AudioInputProcessor related state changes.
     */
    void addAudioInputProcessorObserver(
        const std::shared_ptr<avsCommon::sdkInterfaces::AudioInputProcessorObserverInterface>& observer);

    /**
     * Removes an @c AudioInputProcessorObserver to be alerted on @c AudioInputProcessor related state changes.
     *
     * @param observer The observer to be removed as an @c AudioInputProcessor observer.
     */
    void removeAudioInputProcessorObserver(
        const std::shared_ptr<avsCommon::sdkInterfaces::AudioInputProcessorObserverInterface>& observer);

    /**
     * Adds an observer to be notified when the call state has changed.
     *
     * @param observer The observer to add.
     */
    void addCallStateObserver(std::shared_ptr<avsCommon::sdkInterfaces::CallStateObserverInterface> observer);

    /**
     * Removes an observer to be notified when the call state has changed.
     *
     * @param observer The observer to remove.
     */
    void removeCallStateObserver(std::shared_ptr<avsCommon::sdkInterfaces::CallStateObserverInterface> observer);

    /**
     * Lets the caller know if Comms is enabled.
     *
     * @return True if comms is enabled.
     */
    bool isCommsEnabled();

    /**
     * Accepts an incoming phone-call.
     */
    void acceptCommsCall();

    /**
     * Send dtmf tones during the call.
     *
     * @param dtmfTone The signal of the dtmf message.
     */
    void sendDtmf(avsCommon::sdkInterfaces::CallManagerInterface::DTMFTone dtmfTone);

    /**
     * Stops a phone-call.
     */
    void stopCommsCall();

    /**
     * This function stops playback of the current song in the @c AudioPlayer.
     */
    void audioPlayerLocalStop();

    /**
     * Check if the Comms call is muted.
     *
     * @return Whether the comms call is muted.
     */
    bool isCommsCallMuted();

    /**
     * Mute comms Call.
     */
    void muteCommsCall();

    /**
     * Unmute Comms call.
     */
    void unmuteCommsCall();

    /**
     * Enable the video of local device in an active call.
     */
    void enableVideo();

    /**
     * Disable the video of local device in an active call.
     */
    void disableVideo();

    /*
     * To be called when system clock is synchronized.
     */
    void onSystemClockSynchronized();

    /**
     * Register an ExternalMediaPlayerAdapterHandler with the ExternalMediaPlayer CA
     * Multiple adapter handlers can be added to the ExternalMediaPlayer by repeatedly calling this function.
     *
     * @param externalMediaPlayerAdapterHandler Pointer to the EMPAdapterHandler to register
     */
    void registerExternalMediaPlayerAdapterHandler(
        std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaAdapterHandlerInterface>
            externalMediaPlayerAdapterHandler);

    /**
     * Gets the @c ShutdownManagerInterface for when it is time to shut down the SDK.
     *
     * This method is required to support legacy applications that have not transitioned to fully integrating
     * the manufactory.
     * @return A shared_ptr to the @c ShutdownManagerInterface.
     */
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface> getShutdownManager();

    /**
     * Destructor.
     */
    ~DefaultClient();

    /**
     * Set encoding for the audio format. The new encoding will be used for future utterances. Any audio stream already
     * in progress will not be affected.
     *
     * @param encoding The encoding format to use.
     * @return @true on success, @c false on failure to set the encoding.
     */
    bool setEncodingAudioFormat(avsCommon::utils::AudioFormat::Encoding encoding);

    /**
     * Request for multiple audio streams with provided encodings for a single Recognize event. Calling this function
     * will override any previous encoding specified by a call to @c setEncodingAudioFormat()
     * @param encodings Encoding formats to use for audio streams.
     * @return Result with encodings confirmed in request.
     */
    capabilityAgents::aip::AudioInputProcessor::EncodingFormatResponse requestEncodingAudioFormats(
        const capabilityAgents::aip::AudioInputProcessor::EncodingFormatRequest& encodings);

    /**
     * Gets the @c DeviceSetupInterface for when it is time to send the DeviceSetupComplete event to AVS.
     *
     * This method is required to support legacy applications that have not transitioned to fully integrating
     * the manufactory.
     * @return A shared_ptr to the @c DeviceSetupInterface.
     */
    std::shared_ptr<acsdkDeviceSetupInterfaces::DeviceSetupInterface> getDeviceSetup();

    /**
     * Gets the @c BluetoothLocalInterface for local applications that wish to invoke Bluetooth functionality.
     *
     * @return A shared_ptr to the @c BluetoothLocalInterface.
     */
    std::shared_ptr<acsdkBluetoothInterfaces::BluetoothLocalInterface> getBluetoothLocal();

    /**
     * Stops any ongoing interaction with the SDK by resetting the state of the @c AudioInputProcessor.
     *
     * This method is intended for use when a device needs to stop the current user interaction with alexa, for example
     * when the audio input state needs to be returned to idle as a result of an event such as a back or exit button
     * press. Calling this method has no effect on ongoing Alexa speech, audio playback or visual state.
     */
    void stopInteraction();

    /**
     * Get a reference to the audio focus manager
     *
     * @return shared_ptr to the audio @c FocusManagerInterface, callers should perform a nullptr check as the returned
     * pointer may be null
     */
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::FocusManagerInterface> getAudioFocusManager();

private:
    /**
     * Constructor
     */
    DefaultClient();

    /**
     * Initializes the SDK and "glues" all the components together.
     *
     * @param manufactory @c Manufactory for creating various instances used by DefaultlClient.
     * @param ringtoneMediaPlayer The media player to play Comms ringtones.
     * @param ringtoneSpeaker The speaker to control volume of Comms ringtones.
     * @param additionalSpeakers A map of additional speakers to receive volume changes.
     * @param notificationsStorage The storage interface that will be used to store notification indicators.
     * @param alexaDialogStateObservers Observers that can be used to be notified of Alexa dialog related UX state
     * changes.
     * @param connectionObservers Observers that can be used to be notified of connection status changes.
     * @param isGuiSupported Whether the device supports GUI.
     * @param firmwareVersion The firmware version to report to @c AVS or @c INVALID_FIRMWARE_VERSION.
     * @param sendSoftwareInfoOnConnected Whether to send SoftwareInfo upon connecting to @c AVS.
     * @param softwareInfoSenderObserver Object to receive notifications about sending SoftwareInfo.
     * @param diagnostics Diagnostics interface that provides suite of APIs for insights into SDK.
     * @param externalCapabilitiesBuilder Object used to build capabilities that are not included in the SDK.
     * @param firstInteractionAudioProvider Optional object used in the first interaction started from
     * the alexa voice service
     * @param sdkClientRegistry Optional object used when the @c SDKClientBuilder is used to construct DefaultClient
     * @return Whether the SDK was initialized properly.
     */
    bool initialize(
        const std::shared_ptr<DefaultClientManufactory>& manufactory,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> ringtoneMediaPlayer,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> ringtoneSpeaker,
        const std::multimap<
            avsCommon::sdkInterfaces::ChannelVolumeInterface::Type,
            std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>> additionalSpeakers,
#ifdef ENABLE_PCC
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> phoneSpeaker,
        std::shared_ptr<phoneCallControllerInterfaces::phone::PhoneCallerInterface> phoneCaller,
#endif
#ifdef ENABLE_MCC
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> meetingSpeaker,
        std::shared_ptr<meetingClientControllerInterfaces::meeting::MeetingClientInterface> meetingClient,
        std::shared_ptr<meetingClientControllerInterfaces::calendar::CalendarClientInterface> calendarClient,
#endif
#ifdef ENABLE_COMMS_AUDIO_PROXY
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> commsMediaPlayer,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> commsSpeaker,
        std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> sharedDataStream,
#endif
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface>>
            alexaDialogStateObservers,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>>
            connectionObservers,
        bool isGuiSupported,
        avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion,
        bool sendSoftwareInfoOnConnected,
        std::shared_ptr<avsCommon::sdkInterfaces::SoftwareInfoSenderObserverInterface> softwareInfoSenderObserver,
        std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface> diagnostics,
        const std::shared_ptr<ExternalCapabilitiesBuilderInterface>& externalCapabilitiesBuilder,
        capabilityAgents::aip::AudioProvider firstInteractionAudioProvider,
        const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry = nullptr);

    /// The directive sequencer.
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> m_directiveSequencer;

    /// The focus manager for audio channels.
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> m_audioFocusManager;

    /// The connection manager.
    std::shared_ptr<acl::AVSConnectionManagerInterface> m_connectionManager;

    /// The internet connection monitor.
    std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionMonitorInterface> m_internetConnectionMonitor;

    /// The captions manager.
    std::shared_ptr<captions::CaptionManagerInterface> m_captionManager;

    /// The exception sender.
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> m_exceptionSender;

    /// The certified sender.
    std::shared_ptr<certifiedSender::CertifiedSender> m_certifiedSender;

    /// The audio input processor.
    std::shared_ptr<capabilityAgents::aip::AudioInputProcessor> m_audioInputProcessor;

    /// The speech synthesizer.
    std::shared_ptr<capabilityAgents::speechSynthesizer::SpeechSynthesizer> m_speechSynthesizer;

    /// The audio player.
    std::shared_ptr<acsdkAudioPlayerInterfaces::AudioPlayerInterface> m_audioPlayer;

    /// The external media player.
    std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerInterface> m_externalMediaPlayer;

    /// The alexa interface message sender.
    std::shared_ptr<capabilityAgents::alexa::AlexaInterfaceMessageSender> m_alexaMessageSender;

    /// The api gateway capability agent.
    std::shared_ptr<capabilityAgents::apiGateway::ApiGatewayCapabilityAgent> m_apiGatewayCapabilityAgent;

    /// The alerts capability agent.
    std::shared_ptr<acsdkAlertsInterfaces::AlertsCapabilityAgentInterface> m_alertsCapabilityAgent;

    /// The bluetooth capability agent.
    std::shared_ptr<acsdkBluetoothInterfaces::BluetoothLocalInterface> m_bluetoothLocal;

    /// The bluetooth notifier.
    std::shared_ptr<acsdkBluetoothInterfaces::BluetoothNotifierInterface> m_bluetoothNotifier;

    /// The interaction model notifier.
    std::shared_ptr<acsdkInteractionModelInterfaces::InteractionModelNotifierInterface> m_interactionModelNotifier;

    /// The Notifications notifier.
    std::shared_ptr<acsdkNotificationsInterfaces::NotificationsNotifierInterface> m_notificationsNotifier;

#ifdef ENABLE_PCC
    /// The phoneCallController capability agent.
    std::shared_ptr<phoneCallController::PhoneCallController> m_phoneCallControllerCapabilityAgent;
#endif

#ifdef ENABLE_MCC
    /// The MeetingClientController capability agent.
    std::shared_ptr<meetingClientController::MeetingClientController> m_meetingClientControllerCapabilityAgent;
#endif

    /// The call manager capability agent.
    std::shared_ptr<avsCommon::sdkInterfaces::CallManagerInterface> m_callManager;

    /// The Alexa dialog UX aggregator.
    std::shared_ptr<avsCommon::avs::DialogUXStateAggregator> m_dialogUXStateAggregator;

    /// The playbackRouter.
    std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> m_playbackRouter;

    /// The speakerManager. Used for controlling the volume and mute settings of @c SpeakerInterface objects.
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> m_speakerManager;

    /// The TemplateRuntime capability agent.
    std::shared_ptr<templateRuntimeInterfaces::TemplateRuntimeInterface> m_templateRuntime;

    /// The Equalizer capability agent.
    std::shared_ptr<acsdkEqualizer::EqualizerCapabilityAgent> m_equalizerCapabilityAgent;

    /// The @c EqualizerController instance.
    std::shared_ptr<acsdkEqualizer::EqualizerController> m_equalizerController;

    /// Equalizer runtime setup to be used in the SDK.
    std::shared_ptr<acsdkEqualizerInterfaces::EqualizerRuntimeSetupInterface> m_equalizerRuntimeSetup;

    /// Mutex to serialize access to m_softwareInfoSender.
    std::mutex m_softwareInfoSenderMutex;

    /// The System.SoftwareInfoSender capability agent.
    std::shared_ptr<capabilityAgents::system::SoftwareInfoSender> m_softwareInfoSender;

#ifdef ENABLE_REVOKE_AUTH
    /// The System.RevokeAuthorizationHandler directive handler.
    std::shared_ptr<capabilityAgents::system::RevokeAuthorizationHandler> m_revokeAuthorizationHandler;
#endif

    /// The @c AuthDelegateInterface used for authorization events.
    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> m_authDelegate;

    /// The RegistrationManager used to control customer registration.
    std::shared_ptr<registrationManager::RegistrationManagerInterface> m_registrationManager;

    /// The @c RegistrationNotifier used to notify RegistrationObservers.
    std::shared_ptr<registrationManager::RegistrationNotifierInterface> m_registrationNotifier;

    /// Module responsible for managing device settings.
    std::shared_ptr<settings::DeviceSettingsManager> m_deviceSettingsManager;

    /// DeviceInfo which reflects the device setup credentials.
    std::shared_ptr<avsCommon::utils::DeviceInfo> m_deviceInfo;

    /// The device context manager.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The endpoint registration manager.
    std::shared_ptr<endpoints::EndpointRegistrationManager> m_endpointRegistrationManager;

    /// The endpoint builder for the default endpoint with AVS Capabilities.
    std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface> m_defaultEndpointBuilder;

    /// The @c AVSGatewayManager instance used in the AVS Gateway connection sequence.
    std::shared_ptr<avsCommon::sdkInterfaces::AVSGatewayManagerInterface> m_avsGatewayManager;

    /// The component that provides the client with the ability to send messages to the Capabilities API.
    std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> m_capabilitiesDelegate;

    /// Diagnostic interface.
    std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface> m_diagnostics;

    /// The system clock monitor manager.
    std::shared_ptr<acsdkSystemClockMonitorInterfaces::SystemClockMonitorInterface> m_systemClockMonitor;

    /// The list of objects to be shutdown. Shutdown will occur in the reverse order of occurrence.
    std::list<std::shared_ptr<avsCommon::utils::RequiresShutdown>> m_shutdownObjects;

    /// Used to wake up connection retries when the user tries to use the client.
    std::shared_ptr<ConnectionRetryTrigger> m_connectionRetryTrigger;

    /// A set of SoftwareInfoSenderObservers.
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::SoftwareInfoSenderObserverInterface>>
        m_softwareInfoSenderObservers;

    /// The SoftwareComponentReporter Capability Agent.
    std::shared_ptr<capabilityAgents::softwareComponentReporter::SoftwareComponentReporterCapabilityAgent>
        m_softwareReporterCapabilityAgent;

    /// The @c ShutdownManagerInterface for shutting down the SDK.
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface> m_shutdownManager;

    /// The @c DeviceSetupInterface
    std::shared_ptr<acsdkDeviceSetupInterfaces::DeviceSetupInterface> m_deviceSetup;
};

}  // namespace defaultClient
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_DEFAULTCLIENT_H_
