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

#include "DefaultClient/DefaultClientBuilder.h"

namespace alexaClientSDK {
namespace defaultClient {
/// String to identify log entries originating from this file.
#define TAG "DefaultClientBuilder"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

// Friendly name for this builder component
static const char DEFAULT_CLIENT_BUILDER_NAME[] = "DefaultClientBuilder";

std::unique_ptr<DefaultClientBuilder> DefaultClientBuilder::create(
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
        enabledConnectionRules,
    std::shared_ptr<avsCommon::sdkInterfaces::SystemTimeZoneInterface> systemTimezone,
    avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion,
    bool sendSoftwareInfoOnConnected,
    std::shared_ptr<avsCommon::sdkInterfaces::SoftwareInfoSenderObserverInterface> softwareInfoSenderObserver,
    std::unique_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceManagerInterface> bluetoothDeviceManager,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
    std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface> powerResourceManager,
    std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface> diagnostics,
    std::shared_ptr<ExternalCapabilitiesBuilderInterface> externalCapabilitiesBuilder,
    std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeFactoryInterface> channelVolumeFactory,
    bool startAlertSchedulingOnInitialization,
    std::shared_ptr<alexaClientSDK::acl::MessageRouterFactoryInterface> messageRouterFactory,
    std::shared_ptr<avsCommon::sdkInterfaces::ExpectSpeechTimeoutHandlerInterface> expectSpeechTimeoutHandler,
    capabilityAgents::aip::AudioProvider firstInteractionAudioProvider,
    std::shared_ptr<alexaClientSDK::cryptoInterfaces::CryptoFactoryInterface> cryptoFactory) {
    auto builder = new DefaultClientBuilder();

    builder->m_deviceInfo = std::move(deviceInfo);
    builder->m_customerDataManager = std::move(customerDataManager);
    builder->m_externalMusicProviderMediaPlayers = std::move(externalMusicProviderMediaPlayers);
    builder->m_externalMusicProviderSpeakers = std::move(externalMusicProviderSpeakers);
    builder->m_adapterCreationMap = std::move(adapterCreationMap);
    builder->m_speakMediaPlayer = std::move(speakMediaPlayer);
    builder->m_audioMediaPlayerFactory = std::move(audioMediaPlayerFactory);
    builder->m_alertsMediaPlayer = std::move(alertsMediaPlayer);
    builder->m_notificationsMediaPlayer = std::move(notificationsMediaPlayer);
    builder->m_bluetoothMediaPlayer = std::move(bluetoothMediaPlayer);
    builder->m_ringtoneMediaPlayer = std::move(ringtoneMediaPlayer);
    builder->m_systemSoundMediaPlayer = std::move(systemSoundMediaPlayer);
    builder->m_speakSpeaker = std::move(speakSpeaker);
    builder->m_audioSpeakers = std::move(audioSpeakers);
    builder->m_alertsSpeaker = std::move(alertsSpeaker);
    builder->m_notificationsSpeaker = std::move(notificationsSpeaker);
    builder->m_bluetoothSpeaker = std::move(bluetoothSpeaker);
    builder->m_ringtoneSpeaker = std::move(ringtoneSpeaker);
    builder->m_systemSoundSpeaker = std::move(systemSoundSpeaker);
    builder->m_additionalSpeakers = std::move(additionalSpeakers);
#ifdef ENABLE_PCC
    builder->m_phoneSpeaker = std::move(phoneSpeaker);
    builder->m_phoneCaller = std::move(phoneCaller);
#endif
#ifdef ENABLE_MCC
    builder->m_meetingSpeaker = std::move(meetingSpeaker);
    builder->m_meetingClient = std::move(meetingClient);
    builder->m_calendarClient = std::move(calendarClient);
#endif
#ifdef ENABLE_COMMS_AUDIO_PROXY
    builder->m_commsMediaPlayer = std::move(commsMediaPlayer);
    builder->m_commsSpeaker = std::move(commsSpeaker);
    builder->m_sharedDataStream = std::move(sharedDataStream);
#endif
    builder->m_equalizerRuntimeSetup = std::move(equalizerRuntimeSetup);
    builder->m_audioFactory = std::move(audioFactory);
    builder->m_authDelegate = std::move(authDelegate);
    builder->m_alertStorage = std::move(alertStorage);
    builder->m_messageStorage = std::move(messageStorage);
    builder->m_notificationsStorage = std::move(notificationsStorage);
    builder->m_deviceSettingStorage = std::move(deviceSettingStorage);
    builder->m_bluetoothStorage = std::move(bluetoothStorage);
    builder->m_miscStorage = std::move(miscStorage);
    builder->m_alexaDialogStateObservers = std::move(alexaDialogStateObservers);
    builder->m_connectionObservers = std::move(connectionObservers);
    builder->m_internetConnectionMonitor = std::move(internetConnectionMonitor);
    builder->m_isGuiSupported = isGuiSupported;
    builder->m_capabilitiesDelegate = std::move(capabilitiesDelegate);
    builder->m_contextManager = std::move(contextManager);
    builder->m_transportFactory = std::move(transportFactory);
    builder->m_avsGatewayManager = std::move(avsGatewayManager);
    builder->m_localeAssetsManager = std::move(localeAssetsManager);
    builder->m_enabledConnectionRules = std::move(enabledConnectionRules);
    builder->m_systemTimezone = std::move(systemTimezone);
    builder->m_firmwareVersion = std::move(firmwareVersion);
    builder->m_sendSoftwareInfoOnConnected = sendSoftwareInfoOnConnected;
    builder->m_softwareInfoSenderObserver = std::move(softwareInfoSenderObserver);
    builder->m_bluetoothDeviceManager = std::move(bluetoothDeviceManager);
    builder->m_metricRecorder = std::move(metricRecorder);
    builder->m_powerResourceManager = std::move(powerResourceManager);
    builder->m_diagnostics = std::move(diagnostics);
    builder->m_externalCapabilitiesBuilder = std::move(externalCapabilitiesBuilder);
    builder->m_channelVolumeFactory = std::move(channelVolumeFactory);
    builder->m_startAlertSchedulingOnInitialization = startAlertSchedulingOnInitialization;
    builder->m_messageRouterFactory = std::move(messageRouterFactory);
    builder->m_expectSpeechTimeoutHandler = std::move(expectSpeechTimeoutHandler);
    builder->m_firstInteractionAudioProvider = std::move(firstInteractionAudioProvider);
    builder->m_cryptoFactory = std::move(cryptoFactory);

    return std::unique_ptr<DefaultClientBuilder>(builder);
}

DefaultClientBuilder::DefaultClientBuilder() : m_constructed(false) {
}

std::shared_ptr<DefaultClient> DefaultClientBuilder::construct(
    const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry) {
    if (m_constructed) {
        ACSDK_ERROR(LX("constructFailed").d("reason", "Repeated call to construct()"));
        return nullptr;
    }

    m_constructed = true;

    return DefaultClient::create(
        std::move(m_deviceInfo),
        std::move(m_customerDataManager),
        std::move(m_externalMusicProviderMediaPlayers),
        std::move(m_externalMusicProviderSpeakers),
        std::move(m_adapterCreationMap),
        std::move(m_speakMediaPlayer),
        std::move(m_audioMediaPlayerFactory),
        std::move(m_alertsMediaPlayer),
        std::move(m_notificationsMediaPlayer),
        std::move(m_bluetoothMediaPlayer),
        std::move(m_ringtoneMediaPlayer),
        std::move(m_systemSoundMediaPlayer),
        std::move(m_speakSpeaker),
        std::move(m_audioSpeakers),
        std::move(m_alertsSpeaker),
        std::move(m_notificationsSpeaker),
        std::move(m_bluetoothSpeaker),
        std::move(m_ringtoneSpeaker),
        std::move(m_systemSoundSpeaker),
        std::move(m_additionalSpeakers),
#ifdef ENABLE_PCC
        std::move(m_phoneSpeaker),
        std::move(m_phoneCaller),
#endif
#ifdef ENABLE_MCC
        std::move(m_meetingSpeaker),
        std::move(m_meetingClient),
        std::move(m_calendarClient),
#endif
#ifdef ENABLE_COMMS_AUDIO_PROXY
        std::move(m_commsMediaPlayer),
        std::move(m_commsSpeaker),
        std::move(m_sharedDataStream),
#endif
        std::move(m_equalizerRuntimeSetup),
        std::move(m_audioFactory),
        std::move(m_authDelegate),
        std::move(m_alertStorage),
        std::move(m_messageStorage),
        std::move(m_notificationsStorage),
        std::move(m_deviceSettingStorage),
        std::move(m_bluetoothStorage),
        std::move(m_miscStorage),
        std::move(m_alexaDialogStateObservers),
        std::move(m_connectionObservers),
        std::move(m_internetConnectionMonitor),
        m_isGuiSupported,
        std::move(m_capabilitiesDelegate),
        std::move(m_contextManager),
        std::move(m_transportFactory),
        std::move(m_avsGatewayManager),
        std::move(m_localeAssetsManager),
        std::move(m_enabledConnectionRules),
        std::move(m_systemTimezone),
        std::move(m_firmwareVersion),
        m_sendSoftwareInfoOnConnected,
        std::move(m_softwareInfoSenderObserver),
        std::move(m_bluetoothDeviceManager),
        std::move(m_metricRecorder),
        std::move(m_powerResourceManager),
        std::move(m_diagnostics),
        std::move(m_externalCapabilitiesBuilder),
        std::move(m_channelVolumeFactory),
        m_startAlertSchedulingOnInitialization,
        std::move(m_messageRouterFactory),
        std::move(m_expectSpeechTimeoutHandler),
        std::move(m_firstInteractionAudioProvider),
        std::move(m_cryptoFactory),
        sdkClientRegistry);
}

std::string DefaultClientBuilder::name() {
    return DEFAULT_CLIENT_BUILDER_NAME;
}
}  // namespace defaultClient
}  // namespace alexaClientSDK
