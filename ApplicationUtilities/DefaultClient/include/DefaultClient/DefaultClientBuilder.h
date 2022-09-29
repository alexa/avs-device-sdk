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

#ifndef ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_DEFAULTCLIENTBUILDER_H_
#define ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_DEFAULTCLIENTBUILDER_H_

#include <ACL/Transport/MessageRouter.h>
#include <ACL/Transport/MessageRouterFactory.h>
#include <AIP/AudioInputProcessor.h>
#include <AIP/AudioProvider.h>
#include <AVSCommon/SDKInterfaces/AVSGatewayManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Audio/AudioFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceConnectionRuleInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceManagerInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/ChannelVolumeFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>
#include <AVSCommon/SDKInterfaces/Diagnostics/DiagnosticsInterface.h>
#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/ExpectSpeechTimeoutHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/InternetConnectionMonitorInterface.h>
#include <AVSCommon/SDKInterfaces/LocaleAssetsManagerInterface.h>
#include <AVSCommon/SDKInterfaces/PowerResourceManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <AVSCommon/SDKInterfaces/SystemTimeZoneInterface.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerFactoryInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/Optional.h>
#include <CertifiedSender/CertifiedSender.h>
#include <Endpoints/Endpoint.h>
#include <Settings/Storage/DeviceSettingStorageInterface.h>
#include <System/SoftwareInfoSender.h>
#include <acsdk/SDKClient/FeatureClientBuilderInterface.h>
#include <acsdk/SDKClient/SDKClientRegistry.h>
#include <acsdkAlerts/Storage/AlertStorageInterface.h>
#include <acsdkAudioPlayer/AudioPlayer.h>
#include <acsdkBluetoothInterfaces/BluetoothStorageInterface.h>
#include <acsdkExternalMediaPlayer/ExternalMediaPlayer.h>
#include <acsdkNotificationsInterfaces/NotificationsStorageInterface.h>

#ifdef ENABLE_PCC
#include <acsdk/PhoneCallControllerInterfaces/Phone/PhoneCallerInterface.h>
#include <acsdk/PhoneCallController/PhoneCallController.h>
#endif

#ifdef ENABLE_MCC
#include <acsdk/MeetingClientControllerInterfaces/Calendar/CalendarClientInterface.h>
#include <acsdk/MeetingClientControllerInterfaces/Meeting/MeetingClientInterface.h>
#include <acsdk/MeetingClientController/MeetingClientController.h>
#endif

#include "DefaultClient/EqualizerRuntimeSetup.h"
#include "DefaultClient/ExternalCapabilitiesBuilderInterface.h"

#include "DefaultClient.h"

namespace alexaClientSDK {
namespace defaultClient {
/**
 * The DefaultClientBuilder is used to construct DefaultClient when used with the ClientBuilder
 */
class DefaultClientBuilder : public sdkClient::FeatureClientBuilderInterface {
public:
    /**
     * Creates and initializes a default AVS SDK client. To connect the client to AVS, users should make a call to
     * connect() after creation.
     *
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
     * @return A @c std::unique_ptr to a DefaultClient if all went well or @c nullptr otherwise.
     */
    static std::unique_ptr<DefaultClientBuilder> create(
        std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo,
        std::shared_ptr<registrationManager::CustomerDataManagerInterface> customerDataManager,
        std::unordered_map<std::string, std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>>
            externalMusicProviderMediaPlayers,
        std::unordered_map<std::string, std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>
            externalMusicProviderSpeakers,
        acsdkExternalMediaPlayer::ExternalMediaPlayer::AdapterCreationMap adapterCreationMap,
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
        std::multimap<
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
        std::shared_ptr<avsCommon::avs::AudioInputStream> sharedDataStream,
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
        std::shared_ptr<ExternalCapabilitiesBuilderInterface> externalCapabilitiesBuilder = nullptr,
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeFactoryInterface> channelVolumeFactory =
            speakerManager::createChannelVolumeFactory(),
        bool startAlertSchedulingOnInitialization = true,
        std::shared_ptr<alexaClientSDK::acl::MessageRouterFactoryInterface> messageRouterFactory =
            std::make_shared<alexaClientSDK::acl::MessageRouterFactory>(),
        std::shared_ptr<avsCommon::sdkInterfaces::ExpectSpeechTimeoutHandlerInterface> expectSpeechTimeoutHandler =
            nullptr,
        capabilityAgents::aip::AudioProvider firstInteractionAudioProvider =
            capabilityAgents::aip::AudioProvider::null(),
        std::shared_ptr<alexaClientSDK::cryptoInterfaces::CryptoFactoryInterface> cryptoFactory = nullptr);

    /**
     * Function used by SDKClientBuilder to construct an instance of DefaultClient
     * @param sdkClientRegistry The @c SDKClientRegistry object
     * @return An instance of the @c DefaultClient, or nullptr if creation failed
     */
    std::shared_ptr<DefaultClient> construct(const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry);

    /// @c FeatureClientBuilderInterface functions
    /// @{
    std::string name() override;
    /// @}

private:
    /// Constructor
    DefaultClientBuilder();

    /// Flag indicating whether the @c construct method has previously been called on this instance
    bool m_constructed;

    /// DeviceInfo which reflects the device setup credentials.
    std::shared_ptr<alexaClientSDK::avsCommon::utils::DeviceInfo> m_deviceInfo;

    /// CustomerDataManager instance to be used by RegistrationManager and instances of all classes extending
    /// CustomDataHandler.
    std::shared_ptr<alexaClientSDK::registrationManager::CustomerDataManagerInterface> m_customerDataManager;

    /// The map of <players, mediaPlayer> to use to play content from each external music provider.
    std::
        unordered_map<std::string, std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface>>
            m_externalMusicProviderMediaPlayers;

    /// The map of <players, speaker> to use to track volume of each external music provider media player.
    std::unordered_map<std::string, std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface>>
        m_externalMusicProviderSpeakers;

    /// The map of <players, adapterCreationMethod> to use when creating the adapters for the different music providers
    /// supported by ExternalMediaPlayer.
    alexaClientSDK::acsdkExternalMediaPlayer::ExternalMediaPlayer::AdapterCreationMap m_adapterCreationMap;

    /// The media player to use to play Alexa speech from.
    std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_speakMediaPlayer;

    /// The media player factory to use to generate players for Alexa audio content.
    std::unique_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerFactoryInterface>
        m_audioMediaPlayerFactory;

    /// The media player to use to play alerts from.
    std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_alertsMediaPlayer;

    /// The media player to play notification indicators.
    std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_notificationsMediaPlayer;

    /// The media player to play bluetooth content.
    std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_bluetoothMediaPlayer;

    /// The media player to play Comms ringtones.
    std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_ringtoneMediaPlayer;

    /// The media player to play system sounds.
    std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_systemSoundMediaPlayer;

    /// The speaker to control volume of Alexa speech.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> m_speakSpeaker;

    /// A list of speakers to control volume of Alexa audio content.
    std::vector<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface>> m_audioSpeakers;

    /// The speaker to control volume of alerts.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> m_alertsSpeaker;

    /// The speaker to control volume of notifications.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> m_notificationsSpeaker;

    /// The speaker to control volume of bluetooth.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> m_bluetoothSpeaker;

    /// The speaker to control volume of Comms ringtones.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> m_ringtoneSpeaker;

    /// The speaker to control volume of system sounds.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> m_systemSoundSpeaker;

    /// A map of additional speakers to receive volume changes.
    std::multimap<
        alexaClientSDK::avsCommon::sdkInterfaces::ChannelVolumeInterface::Type,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface>>
        m_additionalSpeakers;

    /// Equalizer component runtime setup
    std::shared_ptr<EqualizerRuntimeSetup> m_equalizerRuntimeSetup;

    /// The audioFactory is a component that provides unique audio streams.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::audio::AudioFactoryInterface> m_audioFactory;

    /// The component that provides the client with valid LWA authorization.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> m_authDelegate;

    /// The storage interface that will be used to store alerts.
    std::shared_ptr<alexaClientSDK::acsdkAlerts::storage::AlertStorageInterface> m_alertStorage;

    /// The storage interface that will be used to store certified sender messages.
    std::shared_ptr<alexaClientSDK::certifiedSender::MessageStorageInterface> m_messageStorage;

    /// The storage interface that will be used to store notification indicators.
    std::shared_ptr<alexaClientSDK::acsdkNotificationsInterfaces::NotificationsStorageInterface> m_notificationsStorage;

    /// The storage interface that will be used to store device settings.
    std::shared_ptr<alexaClientSDK::settings::storage::DeviceSettingStorageInterface> m_deviceSettingStorage;

    /// The storage interface that will be used to store bluetooth data.
    std::shared_ptr<alexaClientSDK::acsdkBluetoothInterfaces::BluetoothStorageInterface> m_bluetoothStorage;

    /// The storage interface that will be used to store key / value pairs.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::storage::MiscStorageInterface> m_miscStorage;

    /// Observers that can be used to be notified of Alexa dialog related UX state changes.
    std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::DialogUXStateObserverInterface>>
        m_alexaDialogStateObservers;

    /// Observers that can be used to be notified of connection status changes.
    std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>>
        m_connectionObservers;

    /// The interface for monitoring and reporting internet connection status
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::InternetConnectionMonitorInterface>
        m_internetConnectionMonitor;

    /// Whether the device supports GUI.
    bool m_isGuiSupported;

    /// The component that provides the client with the ability to send messages to the Capabilities API.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> m_capabilitiesDelegate;

    /// The @c ContextManager which will provide the context for various components.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The object passed in here will be used whenever a new transport object for AVS communication is needed.
    std::shared_ptr<alexaClientSDK::acl::TransportFactoryInterface> m_transportFactory;

    /// The @c AVSGatewayManager instance used to create the ApiGateway CA.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AVSGatewayManagerInterface> m_avsGatewayManager;

    /// The device locale assets manager.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::LocaleAssetsManagerInterface> m_localeAssetsManager;

    /// The set of @c BluetoothDeviceConnectionRuleInterface instances used to create the Bluetooth CA.
    std::unordered_set<
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceConnectionRuleInterface>>
        m_enabledConnectionRules;

    /// Optional object used to set the system timezone.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SystemTimeZoneInterface> m_systemTimezone;

    /// The firmware version to report to @c AVS or @c INVALID_FIRMWARE_VERSION.
    alexaClientSDK::avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion m_firmwareVersion;

    /// Whether to send SoftwareInfo upon connecting to @c AVS.
    bool m_sendSoftwareInfoOnConnected;

    /// Object to receive notifications about sending SoftwareInfo.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SoftwareInfoSenderObserverInterface>
        m_softwareInfoSenderObserver;

    /// The @c BluetoothDeviceManager instance used to create the Bluetooth CA.
    std::unique_ptr<alexaClientSDK::avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceManagerInterface>
        m_bluetoothDeviceManager;

    /// The metric recorder object used to capture metrics.
    std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface> m_metricRecorder;

    /// Object to manage power resource.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::PowerResourceManagerInterface> m_powerResourceManager;

    /// Diagnostics interface which provides suite of APIs for diagnostic insight into SDK.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface> m_diagnostics;

    /// Optional object used to build capabilities that are not included in the SDK.
    std::shared_ptr<ExternalCapabilitiesBuilderInterface> m_externalCapabilitiesBuilder;

    /// Optional object used to build @c ChannelVolumeInterface in the SDK.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ChannelVolumeFactoryInterface> m_channelVolumeFactory;

    /// Whether to start scheduling alerts after client initialization. If this is set to false, no alert scheduling
    /// will occur until onSystemClockSynchronized is called.
    bool m_startAlertSchedulingOnInitialization;

    /// Object used to instantiate @c MessageRouter in the SDK.
    std::shared_ptr<alexaClientSDK::acl::MessageRouterFactoryInterface> m_messageRouterFactory;

    /// An optional object that applications may provide to specify external handling of the @c ExpectSpeech directive's
    /// timeout. If provided, this function must remain valid for the lifetime of the @c AudioInputProcessor.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExpectSpeechTimeoutHandlerInterface>
        m_expectSpeechTimeoutHandler;

    /// Optional object used in the first interaction started from the alexa voice service
    alexaClientSDK::capabilityAgents::aip::AudioProvider m_firstInteractionAudioProvider =
        alexaClientSDK::capabilityAgents::aip::AudioProvider::null();

    /// Optional Encryption facilities factory.
    std::shared_ptr<alexaClientSDK::cryptoInterfaces::CryptoFactoryInterface> m_cryptoFactory;

#ifdef ENABLE_COMMS_AUDIO_PROXY
    /// The media player to play Comms calling audio.
    std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_commsMediaPlayer;

    /// The speaker to control volume of Comms calling audio.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> m_commsSpeaker;

    /// The stream to use which has the audio from microphone.
    std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> m_sharedDataStream;
#endif

#ifdef ENABLE_PCC
    /// The speaker to control volume of phone audio.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> m_phoneSpeaker;

    /// The calling functions available on a calling device
    std::shared_ptr<phoneCallControllerInterfaces::phone::PhoneCallerInterface> m_phoneCaller;
#endif

#ifdef ENABLE_MCC
    /// The speaker to control volume of meeting audio.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> m_meetingSpeaker;

    /// The meeting functions available on a meeting device
    std::shared_ptr<meetingClientControllerInterfaces::meeting::MeetingClientInterface> m_meetingClient;

    /// The calendar functions available on a calendar device
    std::shared_ptr<meetingClientControllerInterfaces::calendar::CalendarClientInterface> m_calendarClient;
#endif
};
}  // namespace defaultClient
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_DEFAULTCLIENTBUILDER_H_
