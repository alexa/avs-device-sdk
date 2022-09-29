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

#include <acsdkExternalMediaPlayerInterfaces/ExternalMediaAdapterConstants.h>
#include <acsdkNotifications/NotificationRenderer.h>
#include <acsdkSystemClockMonitorInterfaces/SystemClockMonitorObserverInterface.h>
#include <acsdk/SDKClient/SDKClientRegistry.h>
#include <acsdk/TemplateRuntime/TemplateRuntimeFactory.h>
#include <ADSL/MessageInterpreter.h>
#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/AVS/CapabilityChangeNotifier.h>
#include <AVSCommon/AVS/ExceptionEncounteredSender.h>
#include <AVSCommon/AVS/SpeakerConstants/SpeakerConstants.h>
#include <AVSCommon/SDKInterfaces/InternetConnectionMonitorInterface.h>
#include <AVSCommon/Utils/Bluetooth/BluetoothEventBus.h>
#include <AVSCommon/Utils/MediaPlayer/PooledMediaResourceProvider.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/Network/InternetConnectionMonitor.h>
#include <Audio/SystemSoundAudioFactory.h>
#include <Endpoints/EndpointBuilder.h>
#include <InterruptModel/InterruptModel.h>
#include <System/LocaleHandler.h>
#include <System/ReportStateHandler.h>
#include <System/SystemCapabilityProvider.h>
#include <System/TimeZoneHandler.h>
#include <System/UserInactivityMonitor.h>
#include <SystemSoundPlayer/SystemSoundPlayer.h>
#include <acsdkBluetooth/Bluetooth.h>
#include <acsdkBluetooth/DeviceConnectionRulesAdapter.h>

#include "DefaultClient/DefaultClient.h"
#include "DefaultClient/DefaultClientComponent.h"
#include "DefaultClient/StubApplicationAudioPipelineFactory.h"
#include "SDKComponent/SDKComponent.h"

namespace alexaClientSDK {
namespace defaultClient {

using namespace acsdkApplicationAudioPipelineFactoryInterfaces;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::endpoints;
using namespace alexaClientSDK::endpoints;

using namespace alexaClientSDK::avsCommon::sdkInterfaces;
using namespace alexaClientSDK::avsCommon::utils;

/// String to identify log entries originating from this file.
#define TAG "DefaultClient"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<DefaultClient> DefaultClient::create(
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
    avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion,
    bool sendSoftwareInfoOnConnected,
    std::shared_ptr<avsCommon::sdkInterfaces::SoftwareInfoSenderObserverInterface> softwareInfoSenderObserver,
    std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface> diagnostics,
    const std::shared_ptr<ExternalCapabilitiesBuilderInterface>& externalCapabilitiesBuilder,
    capabilityAgents::aip::AudioProvider firstInteractionAudioProvider,
    const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry) {
    std::unique_ptr<DefaultClient> defaultClient(new DefaultClient());
    if (!defaultClient->initialize(
            manufactory,
            ringtoneMediaPlayer,
            ringtoneSpeaker,
            additionalSpeakers,
#ifdef ENABLE_PCC
            phoneSpeaker,
            phoneCaller,
#endif
#ifdef ENABLE_MCC
            meetingSpeaker,
            meetingClient,
            calendarClient,
#endif
#ifdef ENABLE_COMMS_AUDIO_PROXY
            commsMediaPlayer,
            commsSpeaker,
            sharedDataStream,
#endif
            alexaDialogStateObservers,
            connectionObservers,
            isGuiSupported,
            firmwareVersion,
            sendSoftwareInfoOnConnected,
            softwareInfoSenderObserver,
            diagnostics,
            externalCapabilitiesBuilder,
            firstInteractionAudioProvider,
            sdkClientRegistry)) {
        auto shutdownManager = defaultClient->getShutdownManager();
        if (shutdownManager) {
            shutdownManager->shutdown();
        }
        return nullptr;
    }
    return defaultClient;
}

std::unique_ptr<DefaultClient> DefaultClient::create(
    std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo,
    std::shared_ptr<registrationManager::CustomerDataManagerInterface> customerDataManager,
    const std::unordered_map<std::string, std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>>&
        externalMusicProviderMediaPlayers,
    const std::unordered_map<std::string, std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>&
        externalMusicProviderSpeakers,
    const acsdkExternalMediaPlayer::ExternalMediaPlayer::AdapterCreationMap& adapterCreationMap,
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> speakMediaPlayer,
    std::unique_ptr<mediaPlayer::MediaPlayerFactoryInterface> audioMediaPlayerFactory,
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
        enabledConnectionRules,
    std::shared_ptr<avsCommon::sdkInterfaces::SystemTimeZoneInterface> systemTimezone,
    avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion,
    bool sendSoftwareInfoOnConnected,
    std::shared_ptr<avsCommon::sdkInterfaces::SoftwareInfoSenderObserverInterface> softwareInfoSenderObserver,
    std::unique_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceManagerInterface> bluetoothDeviceManager,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
    std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface> powerResourceManager,
    std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface> diagnostics,
    const std::shared_ptr<ExternalCapabilitiesBuilderInterface>& externalCapabilitiesBuilder,
    std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeFactoryInterface> channelVolumeFactory,
    bool startAlertSchedulingOnInitialization,
    std::shared_ptr<alexaClientSDK::acl::MessageRouterFactoryInterface> messageRouterFactory,
    const std::shared_ptr<avsCommon::sdkInterfaces::ExpectSpeechTimeoutHandlerInterface>& expectSpeechTimeoutHandler,
    capabilityAgents::aip::AudioProvider firstInteractionAudioProvider,
    const std::shared_ptr<alexaClientSDK::cryptoInterfaces::CryptoFactoryInterface>& cryptoFactory,
    const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry) {

    if (!equalizerRuntimeSetup) {
        equalizerRuntimeSetup = std::make_shared<defaultClient::EqualizerRuntimeSetup>(false);
    }

    auto bluetoothConnectionRulesProvider =
        std::make_shared<acsdkBluetooth::DeviceConnectionRulesAdapter>(enabledConnectionRules);

    auto stubAudioPipelineFactory = StubApplicationAudioPipelineFactory::create(channelVolumeFactory);
    if (!stubAudioPipelineFactory) {
        ACSDK_ERROR(LX("createFailed").d("reason", "failed to create audio pipeline"));
        return nullptr;
    }

    /// Add pre-created speakers and media players to the stub factory.
    stubAudioPipelineFactory->addApplicationMediaInterfaces(
        acsdkNotifications::NOTIFICATIONS_MEDIA_PLAYER_NAME, notificationsMediaPlayer, notificationsSpeaker);
    stubAudioPipelineFactory->addApplicationMediaInterfaces(
        acsdkAlerts::ALERTS_MEDIA_PLAYER_NAME, alertsMediaPlayer, alertsSpeaker);
    stubAudioPipelineFactory->addApplicationMediaInterfaces(
        capabilityAgents::speechSynthesizer::SPEAK_MEDIA_PLAYER_NAME, speakMediaPlayer, speakSpeaker);
    stubAudioPipelineFactory->addApplicationMediaInterfaces(
        applicationUtilities::systemSoundPlayer::SYSTEM_SOUND_MEDIA_PLAYER_NAME,
        systemSoundMediaPlayer,
        systemSoundSpeaker);
    stubAudioPipelineFactory->addApplicationMediaInterfaces(
        acsdkBluetooth::BLUETOOTH_MEDIA_PLAYER_NAME, bluetoothMediaPlayer, bluetoothSpeaker);

    for (const auto& adapter : adapterCreationMap) {
        auto mediaPlayerIt = externalMusicProviderMediaPlayers.find(adapter.first);
        auto speakerIt = externalMusicProviderSpeakers.find(adapter.first);

        if (mediaPlayerIt == externalMusicProviderMediaPlayers.end()) {
            ACSDK_ERROR(LX("externalMediaAdapterCreationFailed")
                            .d(acsdkExternalMediaPlayerInterfaces::PLAYER_ID, adapter.first)
                            .d("reason", "nullMediaPlayer"));
            continue;
        }

        if (speakerIt == externalMusicProviderSpeakers.end()) {
            ACSDK_ERROR(LX("externalMediaAdapterCreationFailed")
                            .d(acsdkExternalMediaPlayerInterfaces::PLAYER_ID, adapter.first)
                            .d("reason", "nullSpeaker"));
            continue;
        }

        stubAudioPipelineFactory->addApplicationMediaInterfaces(
            adapter.first + "MediaPlayer", (*mediaPlayerIt).second, (*speakerIt).second);
    }

    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>> audioChannelVolumeInterfaces;
    for (auto& it : audioSpeakers) {
        audioChannelVolumeInterfaces.push_back(channelVolumeFactory->createChannelVolumeInterface(it));
    }
    auto audioMediaPlayerFactoryAdapter = mediaPlayer::PooledMediaResourceProvider::adaptMediaPlayerFactoryInterface(
        std::move(audioMediaPlayerFactory), audioChannelVolumeInterfaces);

    DefaultClientComponent component = getComponent(
        authDelegate,
        contextManager,
        localeAssetsManager,
        deviceInfo,
        customerDataManager,
        miscStorage,
        internetConnectionMonitor,
        avsGatewayManager,
        capabilitiesDelegate,
        metricRecorder,
        diagnostics,
        transportFactory,
        messageRouterFactory,
        channelVolumeFactory,
        expectSpeechTimeoutHandler,
        equalizerRuntimeSetup,
        stubAudioPipelineFactory,
        audioMediaPlayerFactoryAdapter,
        messageStorage,
        powerResourceManager,
        adapterCreationMap,
        systemTimezone,
        std::move(deviceSettingStorage),
        startAlertSchedulingOnInitialization,
        audioFactory,
        std::move(alertStorage),
        std::move(bluetoothDeviceManager),
        std::move(bluetoothStorage),
        bluetoothConnectionRulesProvider,
        std::move(notificationsStorage),
        cryptoFactory);
    auto manufactory = DefaultClientManufactory::create(component);

    auto speakerManager = manufactory->get<std::shared_ptr<SpeakerManagerInterface>>();
    if (!speakerManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullSpeakerManager"));
        return nullptr;
    }
    for (const auto& it : audioChannelVolumeInterfaces) {
        speakerManager->addChannelVolumeInterface(it);
    }

    auto startupManager = manufactory->get<std::shared_ptr<acsdkStartupManagerInterfaces::StartupManagerInterface>>();
    if (!startupManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullStartupManager"));
        return nullptr;
    }
    startupManager->startup();

    return create(
        std::move(manufactory),
        ringtoneMediaPlayer,
        ringtoneSpeaker,
        additionalSpeakers,
#ifdef ENABLE_PCC
        phoneSpeaker,
        phoneCaller,
#endif
#ifdef ENABLE_MCC
        meetingSpeaker,
        meetingClient,
        calendarClient,
#endif
#ifdef ENABLE_COMMS_AUDIO_PROXY
        commsMediaPlayer,
        commsSpeaker,
        sharedDataStream,
#endif
        alexaDialogStateObservers,
        connectionObservers,
        isGuiSupported,
        firmwareVersion,
        sendSoftwareInfoOnConnected,
        softwareInfoSenderObserver,
        diagnostics,
        externalCapabilitiesBuilder,
        firstInteractionAudioProvider,
        sdkClientRegistry);
}

bool DefaultClient::initialize(
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
    const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry) {

    m_shutdownManager = manufactory->get<std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface>>();
    if (!m_shutdownManager) {
        ACSDK_ERROR(LX("initializeFailed").m("Failed to get ShutdownManager!"));
        return false;
    }

    if (!ringtoneMediaPlayer) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullRingtoneMediaPlayer"));
        return false;
    }

    // Initialize various locals from manufactory.
    auto metricRecorder = manufactory->get<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>();
    if (!metricRecorder) {
        ACSDK_DEBUG7(LX(__func__).m("metrics disabled"));
    }

    auto customerDataManager = manufactory->get<std::shared_ptr<registrationManager::CustomerDataManagerInterface>>();
    if (!customerDataManager) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullAttachmentManager"));
        return false;
    }

    auto attachmentManager =
        manufactory->get<std::shared_ptr<avsCommon::avs::attachment::AttachmentManagerInterface>>();
    if (!attachmentManager) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullDefaultEndpointBuilder"));
        return false;
    }

    auto localeAssetsManager =
        manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>>();
    if (!localeAssetsManager) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullLocaleAssetsManager"));
        return false;
    }

    auto miscStorage = manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface>>();
    if (!miscStorage) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullMiscStorage"));
        return false;
    }

    auto channelVolumeFactory =
        manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeFactoryInterface>>();
    if (!channelVolumeFactory) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullChannelVolumeFactory"));
        return false;
    }

    auto audioPipelineFactory = manufactory->get<std::shared_ptr<ApplicationAudioPipelineFactoryInterface>>();
    if (!audioPipelineFactory) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullAudioPipelineFactory"));
        return false;
    }

    auto powerResourceManager =
        manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface>>();
    if (!powerResourceManager) {
        ACSDK_DEBUG7(LX(__func__).m("power resource management disabled"));
    }

    auto systemTimezone = manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::SystemTimeZoneInterface>>();
    if (!systemTimezone) {
        ACSDK_DEBUG7(LX(__func__).m("using default null SystemTimeZone"));
    }

    auto deviceSettingStorage = manufactory->get<std::shared_ptr<settings::storage::DeviceSettingStorageInterface>>();
    if (!deviceSettingStorage) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullDeviceSettingStorage"));
        return false;
    }

    auto audioFactory = manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface>>();
    if (!audioFactory) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullAudioFactory"));
        return false;
    }

    auto userInactivityMonitor =
        manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::UserInactivityMonitorInterface>>();
    if (!userInactivityMonitor) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullUserInactivityMonitorInterface"));
        return false;
    }

    // Initialize various members from manufactory.
    m_avsGatewayManager = manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::AVSGatewayManagerInterface>>();
    if (!m_avsGatewayManager) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullAVSGatewayManager"));
        return false;
    }

    m_internetConnectionMonitor =
        manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionMonitorInterface>>();
    if (!m_internetConnectionMonitor) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullInternetConnectionMonitor"));
        return false;
    }

    m_connectionManager = manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface>>();
    if (!m_connectionManager) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullConnectionManager"));
        return false;
    }

    m_contextManager = manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>>();
    if (!m_contextManager) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullContextManager"));
        return false;
    }

    m_capabilitiesDelegate =
        manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface>>();
    if (!m_capabilitiesDelegate) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullCapabilitiesDelegate"));
        return false;
    }

    m_deviceInfo = manufactory->get<std::shared_ptr<avsCommon::utils::DeviceInfo>>();
    if (!m_deviceInfo) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullDeviceInfo"));
        return false;
    }

    m_authDelegate = manufactory->get<std::shared_ptr<AuthDelegateInterface>>();
    if (!m_authDelegate) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullAuthDelegate"));
        return false;
    }

    m_exceptionSender =
        manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>>();
    if (!m_exceptionSender) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullExceptionSender"));
        return false;
    }

    m_alexaMessageSender = manufactory->get<std::shared_ptr<capabilityAgents::alexa::AlexaInterfaceMessageSender>>();
    if (!m_alexaMessageSender) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullAlexaMessageSender"));
        return false;
    }
    if (externalCapabilitiesBuilder) {
        ACSDK_INFO(LX(__FUNCTION__).m("Supply m_alexaMessageSender to externalCapabilitiesBuilder"));
        externalCapabilitiesBuilder->withAlexaInterfaceMessageSender(m_alexaMessageSender);
    }

    m_speakerManager = manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface>>();
    if (!m_speakerManager) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullSpeakerManager"));
        return false;
    }

    m_defaultEndpointBuilder = manufactory->get<acsdkManufactory::Annotated<
        DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface>>();
    if (!m_defaultEndpointBuilder) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullDefaultEndpointBuilder"));
        return false;
    }

    m_captionManager = manufactory->get<std::shared_ptr<captions::CaptionManagerInterface>>();
    if (!m_captionManager) {
        ACSDK_DEBUG5(LX("nullCaptionManager").m("captions disabled"));
    }

    m_equalizerRuntimeSetup =
        manufactory->get<std::shared_ptr<acsdkEqualizerInterfaces::EqualizerRuntimeSetupInterface>>();
    if (!m_equalizerRuntimeSetup) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullEqualizerRuntimeSetup"));
        return false;
    }

    m_audioFocusManager =
        manufactory
            ->get<acsdkManufactory::Annotated<avsCommon::sdkInterfaces::AudioFocusAnnotation, FocusManagerInterface>>();
    if (!m_audioFocusManager) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullAudioFocusManager"));
        return false;
    }

    m_playbackRouter = manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface>>();
    if (!m_playbackRouter) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullPlaybackRouter"));
        return false;
    }

    m_audioPlayer = manufactory->get<std::shared_ptr<acsdkAudioPlayerInterfaces::AudioPlayerInterface>>();
    if (!m_audioPlayer) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullAudioPlayer"));
        return false;
    }

    m_certifiedSender = manufactory->get<std::shared_ptr<certifiedSender::CertifiedSender>>();
    if (!m_certifiedSender) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullCertifiedSender"));
        return false;
    }

    m_externalMediaPlayer =
        manufactory->get<std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerInterface>>();
    if (!m_externalMediaPlayer) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullExternalMediaPlayer"));
        return false;
    }

    m_deviceSettingsManager = manufactory->get<std::shared_ptr<settings::DeviceSettingsManager>>();
    if (!m_deviceSettingsManager) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "createDeviceSettingsManagerFailed"));
        return false;
    }

    m_systemClockMonitor =
        manufactory->get<std::shared_ptr<acsdkSystemClockMonitorInterfaces::SystemClockMonitorInterface>>();
    if (!m_systemClockMonitor) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullSystemClockMonitorManager"));
        return false;
    }

    m_alertsCapabilityAgent =
        manufactory->get<std::shared_ptr<acsdkAlertsInterfaces::AlertsCapabilityAgentInterface>>();
    if (!m_alertsCapabilityAgent) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateAlertsCapabilityAgent"));
        return false;
    }

    m_bluetoothLocal = manufactory->get<std::shared_ptr<acsdkBluetoothInterfaces::BluetoothLocalInterface>>();
    if (!m_bluetoothLocal) {
#ifdef BLUETOOTH_ENABLED
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateBluetoothLocal"));
        return false;
#else
        ACSDK_DEBUG5(LX("nullBluetooth").m("Bluetooth disabled"));
#endif
    }

    m_bluetoothNotifier = manufactory->get<std::shared_ptr<acsdkBluetoothInterfaces::BluetoothNotifierInterface>>();
    if (!m_bluetoothNotifier) {
#ifdef BLUETOOTH_ENABLED
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateBluetoothNotifier"));
        return false;
#else
        ACSDK_DEBUG5(LX("nullBluetoothNotifier").m("Bluetooth disabled"));
#endif
    }

    m_dialogUXStateAggregator = manufactory->get<std::shared_ptr<DialogUXStateAggregator>>();
    if (!m_dialogUXStateAggregator) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateDialogUXStateAggregator"));
        return false;
    }

    m_notificationsNotifier =
        manufactory->get<std::shared_ptr<acsdkNotificationsInterfaces::NotificationsNotifierInterface>>();
    if (!m_notificationsNotifier) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateNotificationsCapabilityAgent"));
    }

    m_directiveSequencer = manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface>>();
    if (!m_directiveSequencer) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateDirectiveSequencer"));
        return false;
    }

    m_registrationManager = manufactory->get<std::shared_ptr<registrationManager::RegistrationManagerInterface>>();
    if (!m_registrationManager) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateRegistrationManager"));
        return false;
    }

    m_registrationNotifier = manufactory->get<std::shared_ptr<registrationManager::RegistrationNotifierInterface>>();
    if (!m_registrationNotifier) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateRegistrationNotifier"));
        return false;
    }

    m_interactionModelNotifier =
        manufactory->get<std::shared_ptr<acsdkInteractionModelInterfaces::InteractionModelNotifierInterface>>();
    if (!m_interactionModelNotifier) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateInteractionModelNotifier"));
        return false;
    }

    m_deviceSetup = manufactory->get<std::shared_ptr<acsdkDeviceSetupInterfaces::DeviceSetupInterface>>();
    if (!m_deviceSetup) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateDeviceSetupCapabilityAgent"));
        return false;
    }

    m_softwareReporterCapabilityAgent =
        capabilityAgents::softwareComponentReporter::SoftwareComponentReporterCapabilityAgent::create();
    if (!m_softwareReporterCapabilityAgent) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullSoftwareComponentReporterCapabilityAgent"));
        return false;
    }

    if (!applicationUtilities::SDKComponent::SDKComponent::registerComponent(m_softwareReporterCapabilityAgent)) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToRegisterSDKComponent"));
        return false;
    }

    for (auto& observer : connectionObservers) {
        m_connectionManager->addConnectionStatusObserver(observer);
    }

    for (auto observer : alexaDialogStateObservers) {
        m_dialogUXStateAggregator->addObserver(observer);
    }

    /*
     * Creating the Message Interpreter - This component takes care of converting
     * ACL messages to Directives for the
     * Directive Sequencer to process. This essentially "glues" together the ACL
     * and ADSL.
     */
    auto messageInterpreter = std::make_shared<adsl::MessageInterpreter>(
        m_exceptionSender, m_directiveSequencer, attachmentManager, metricRecorder);

    m_connectionManager->addMessageObserver(messageInterpreter);

    // Create endpoint related objects.
    m_capabilitiesDelegate->setMessageSender(m_connectionManager);
    m_avsGatewayManager->addObserver(m_capabilitiesDelegate);
    addConnectionObserver(m_capabilitiesDelegate);
    m_endpointRegistrationManager = EndpointRegistrationManager::create(
        m_directiveSequencer, m_capabilitiesDelegate, m_deviceInfo->getDefaultEndpointId());
    if (!m_endpointRegistrationManager) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "endpointRegistrationManagerCreateFailed"));
        return false;
    }
    localeAssetsManager->setEndpointRegistrationManager(m_endpointRegistrationManager);

    auto wakeWordConfirmationSetting =
        m_deviceSettingsManager->getSetting<settings::DeviceSettingsIndex::WAKEWORD_CONFIRMATION>();
    auto speechConfirmationSetting =
        m_deviceSettingsManager->getSetting<settings::DeviceSettingsIndex::SPEECH_CONFIRMATION>();
    auto wakeWordsSetting = m_deviceSettingsManager->getSetting<settings::DeviceSettingsIndex::WAKE_WORDS>();
    auto capabilityChangeNotifier = std::make_shared<avsCommon::avs::CapabilityChangeNotifier>();
    capabilityChangeNotifier->addObserver(localeAssetsManager);

    /*
     * Creating the Audio Input Processor - This component is the Capability Agent
     * that implements the SpeechRecognizer interface of AVS.
     */
    m_audioInputProcessor = capabilityAgents::aip::AudioInputProcessor::create(
        m_directiveSequencer,
        m_connectionManager,
        m_contextManager,
        m_audioFocusManager,
        m_dialogUXStateAggregator,
        m_exceptionSender,
        userInactivityMonitor,
        manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::SystemSoundPlayerInterface>>(),
        localeAssetsManager,
        wakeWordConfirmationSetting,
        speechConfirmationSetting,
        capabilityChangeNotifier,
        wakeWordsSetting,
        manufactory->get<std::shared_ptr<audioEncoderInterfaces::AudioEncoderInterface>>(),
        firstInteractionAudioProvider,
        powerResourceManager,
        metricRecorder,
        manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::ExpectSpeechTimeoutHandlerInterface>>());

    if (!m_audioInputProcessor) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateAudioInputProcessor"));
        return false;
    }
    // when internet is disconnected during dialog, terminate dialog
    addInternetConnectionObserver(m_audioInputProcessor);

    m_audioInputProcessor->addObserver(m_dialogUXStateAggregator);

    m_connectionRetryTrigger = ConnectionRetryTrigger::create(m_connectionManager, m_audioInputProcessor);
    if (!m_connectionRetryTrigger) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateConnectionRetryTrigger"));
        return false;
    }

    /*
     * Creating the Speech Synthesizer - This component is the Capability Agent
     * that implements the SpeechSynthesizer
     * interface of AVS.
     */
    m_speechSynthesizer = capabilityAgents::speechSynthesizer::SpeechSynthesizer::createSpeechSynthesizer(
        audioPipelineFactory,
        m_connectionManager,
        m_audioFocusManager,
        m_contextManager,
        m_exceptionSender,
        metricRecorder,
        m_dialogUXStateAggregator,
        m_captionManager,
        powerResourceManager);

    if (!m_speechSynthesizer) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateSpeechSynthesizer"));
        return false;
    }

    m_speechSynthesizer->addObserver(m_dialogUXStateAggregator);

    // create @c SpeakerInterfaces for each @c Type
    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>> allAvsSpeakers{};
    // parse additional Speakers into the right speaker list.
    for (const auto& it : additionalSpeakers) {
        if (ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME == it.first) {
            allAvsSpeakers.push_back(it.second);
        }
    }

#ifdef ENABLE_PCC
    allAvsSpeakers.push_back(phoneSpeaker);
#endif

#ifdef ENABLE_MCC
    allAvsSpeakers.push_back(meetingSpeaker);
#endif

#ifdef ENABLE_COMMS_AUDIO_PROXY
    allAvsSpeakers.push_back(commsSpeaker);
#endif

    // create @c ChannelVolumeInterface instances for all @c SpeakerInterface instances
    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>> allAvsChannelVolumeInterfaces,
        allChannelVolumeInterfaces;

    // create allAvsChannelVolumeInterfaces using allAvsSpeakers
    for (auto& it : allAvsSpeakers) {
        allAvsChannelVolumeInterfaces.push_back(channelVolumeFactory->createChannelVolumeInterface(it));
    }

    // create @c ChannelVolumeInterface for ringtoneSpeaker
    std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface> ringtoneChannelVolumeInterface =
        channelVolumeFactory->createChannelVolumeInterface(ringtoneSpeaker);
    allAvsChannelVolumeInterfaces.push_back(ringtoneChannelVolumeInterface);

    allChannelVolumeInterfaces.insert(
        allChannelVolumeInterfaces.end(), allAvsChannelVolumeInterfaces.begin(), allAvsChannelVolumeInterfaces.end());

    for (const auto& channelVolumeInterface : allChannelVolumeInterfaces) {
        m_speakerManager->addChannelVolumeInterface(channelVolumeInterface);
    }

#ifdef ENABLE_PCC
    /*
     * Creating the PhoneCallController - This component is the Capability Agent
     * that implements the
     * PhoneCallController interface of AVS
     */
    m_phoneCallControllerCapabilityAgent = phoneCallController::PhoneCallController::create(
        m_contextManager, m_connectionManager, phoneCaller, phoneSpeaker, m_audioFocusManager, m_exceptionSender);
    if (!m_phoneCallControllerCapabilityAgent) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreatePhoneCallControllerCapabilityAgent"));
    }
#endif

#ifdef ENABLE_MCC
    /*
     * Creating the MeetingClientController - This component is the Capability Agent that implements the
     * MeetingClientController interface of AVS
     */
    m_meetingClientControllerCapabilityAgent = meetingClientController::MeetingClientController::create(
        m_contextManager,
        m_connectionManager,
        meetingClient,
        calendarClient,
        m_speakerManager,
        m_audioFocusManager,
        m_exceptionSender);
    if (!m_meetingClientControllerCapabilityAgent) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateMeetingClientControllerCapabilityAgent"));
    }
#endif

    if (isGuiSupported) {
        auto renderPlayerInfoCardsProviderRegistrar =
            manufactory
                ->get<std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderRegistrarInterface>>();
        if (!renderPlayerInfoCardsProviderRegistrar) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "nullRenderPlayerInfoCardsProviderRegistrar"));
            return false;
        }

        /*
         * Creating the TemplateRuntime Capability Agent - This component is the
         * Capability Agent that implements the
         * TemplateRuntime interface of AVS.
         */
        auto templateRuntimeData = templateRuntime::TemplateRuntimeFactory::create(
            renderPlayerInfoCardsProviderRegistrar, m_exceptionSender, m_defaultEndpointBuilder);
        if (!templateRuntimeData.hasValue()) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateTemplateRuntimeCapabilityAgent"));
            return false;
        }
        m_templateRuntime = templateRuntimeData.value().templateRuntime;
        m_shutdownObjects.push_back(templateRuntimeData.value().requiresShutdown);
        if (externalCapabilitiesBuilder) {
            externalCapabilitiesBuilder->withTemplateRunTime(m_templateRuntime);
        }
    }

    /*
     * Creating the Equalizer Capability Agent and related implementations if
     * enabled
     */
    if (m_equalizerRuntimeSetup->isEnabled()) {
        auto equalizerController = acsdkEqualizer::EqualizerController::create(
            m_equalizerRuntimeSetup->getModeController(),
            m_equalizerRuntimeSetup->getConfiguration(),
            m_equalizerRuntimeSetup->getStorage());

        if (!equalizerController) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateEqualizerController"));
            return false;
        }

        m_equalizerCapabilityAgent = acsdkEqualizer::EqualizerCapabilityAgent::create(
            equalizerController,
            m_capabilitiesDelegate,
            m_equalizerRuntimeSetup->getStorage(),
            customerDataManager,
            m_exceptionSender,
            m_contextManager,
            m_connectionManager);
        if (!m_equalizerCapabilityAgent) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateEqualizerCapabilityAgent"));
            return false;
        }

        m_equalizerController = equalizerController;
        // Register equalizers
        for (auto& equalizer : m_equalizerRuntimeSetup->getAllEqualizers()) {
            equalizerController->registerEqualizer(equalizer);
        }

        // Add all equalizer controller listeners
        for (auto& listener : m_equalizerRuntimeSetup->getAllEqualizerControllerListeners()) {
            equalizerController->addListener(listener);
        }
    } else {
        ACSDK_DEBUG3(LX(__func__).m("Equalizer is disabled"));
    }

    /*
     * Creating the TimeZone Handler - This component is responsible for handling directives related to time zones.
     */
    auto timezoneHandler = capabilityAgents::system::TimeZoneHandler::create(
        m_deviceSettingsManager->getSetting<settings::TIMEZONE>(), m_exceptionSender);
    if (!timezoneHandler) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateTimeZoneHandler"));
        return false;
    }

    /*
     * Creating the Locale Handler - This component is responsible for handling directives related to locales.
     */
    auto localeHandler = capabilityAgents::system::LocaleHandler::create(
        m_exceptionSender, m_deviceSettingsManager->getSetting<settings::DeviceSettingsIndex::LOCALE>());
    if (!localeHandler) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateLocaleHandler"));
        return false;
    }

    /*
     * Creating the ReportState Handler - This component is responsible for the ReportState directives.
     */
    auto reportGenerator = capabilityAgents::system::StateReportGenerator::create(
        m_deviceSettingsManager, m_deviceSettingsManager->getConfigurations());
    if (!reportGenerator.hasValue()) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateStateReportGenerator"));
        return false;
    }

    std::vector<capabilityAgents::system::StateReportGenerator> reportGenerators{{reportGenerator.value()}};
    std::shared_ptr<capabilityAgents::system::ReportStateHandler> reportStateHandler =
        capabilityAgents::system::ReportStateHandler::create(
            customerDataManager,
            m_exceptionSender,
            m_connectionManager,
            m_connectionManager,
            miscStorage,
            reportGenerators);
    if (!reportStateHandler) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateReportStateHandler"));
        return false;
    }

    /*
     * Creating the SystemCapabilityProvider - This component is responsible for
     * publishing information about the System
     * capability agent.
     */
    auto systemCapabilityProvider =
        capabilityAgents::system::SystemCapabilityProvider::create(localeAssetsManager, capabilityChangeNotifier);
    if (!systemCapabilityProvider) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateSystemCapabilityProvider"));
        return false;
    }

#ifdef ENABLE_REVOKE_AUTH
    /*
     * Creating the RevokeAuthorizationHandler - This component is responsible for
     * handling RevokeAuthorization
     * directives from AVS to notify the client to clear out authorization and
     * re-enter the registration flow.
     */
    m_revokeAuthorizationHandler = capabilityAgents::system::RevokeAuthorizationHandler::create(m_exceptionSender);
    if (!m_revokeAuthorizationHandler) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateRevokeAuthorizationHandler"));
        return false;
    }
#endif

    m_apiGatewayCapabilityAgent =
        capabilityAgents::apiGateway::ApiGatewayCapabilityAgent::create(m_avsGatewayManager, m_exceptionSender);
    if (!m_apiGatewayCapabilityAgent) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateApiGatewayCapabilityAgent"));
    }

    /**
     * Optional DiagnosticsInterface which provides diagnostic insights into the SDK.
     */
    m_diagnostics = diagnostics;
    if (m_diagnostics) {
        m_diagnostics->setDiagnosticDependencies(m_directiveSequencer, attachmentManager, m_connectionManager);

        /**
         * Create and initialize the DevicePropertyAggregator.
         */
        auto deviceProperties = m_diagnostics->getDevicePropertyAggregator();
        if (deviceProperties) {
            deviceProperties->setContextManager(m_contextManager);
            deviceProperties->initializeVolume(m_speakerManager);
            deviceProperties->setDeviceSettingsManager(m_deviceSettingsManager);
            addSpeakerManagerObserver(deviceProperties);
            addAlertsObserver(deviceProperties);
            addConnectionObserver(deviceProperties);
            addNotificationsObserver(deviceProperties);
            addAudioPlayerObserver(deviceProperties);
            addAlexaDialogStateObserver(deviceProperties);
            m_authDelegate->addAuthObserver(deviceProperties);
        }

        /**
         * Initialize device protocol tracer.
         */
        auto protocolTrace = m_diagnostics->getProtocolTracer();
        if (protocolTrace) {
            addMessageObserver(protocolTrace);
        }
    } else {
        ACSDK_DEBUG0(LX(__func__).m("Diagnostics Not Enabled"));
    }

    /*
     * Register capability agents and capability configurations.
     */
    m_defaultEndpointBuilder->withCapability(m_speechSynthesizer, m_speechSynthesizer);
    m_defaultEndpointBuilder->withCapability(m_audioInputProcessor, m_audioInputProcessor);
    m_defaultEndpointBuilder->withCapability(m_apiGatewayCapabilityAgent, m_apiGatewayCapabilityAgent);
#ifdef ENABLE_PCC
    if (m_phoneCallControllerCapabilityAgent) {
        m_defaultEndpointBuilder->withCapability(
            m_phoneCallControllerCapabilityAgent, m_phoneCallControllerCapabilityAgent);
    }
#endif

#ifdef ENABLE_MCC
    if (m_meetingClientControllerCapabilityAgent) {
        m_defaultEndpointBuilder->withCapability(
            m_meetingClientControllerCapabilityAgent, m_meetingClientControllerCapabilityAgent);
    }
#endif

    if (m_equalizerCapabilityAgent) {
        m_defaultEndpointBuilder->withCapability(m_equalizerCapabilityAgent, m_equalizerCapabilityAgent);
    }

    // System CA is split into multiple directive handlers.
    m_defaultEndpointBuilder->withCapabilityConfiguration(systemCapabilityProvider);
    if (!m_directiveSequencer->addDirectiveHandler(std::move(localeHandler)) ||
        !m_directiveSequencer->addDirectiveHandler(std::move(timezoneHandler)) ||
#ifdef ENABLE_REVOKE_AUTH
        !m_directiveSequencer->addDirectiveHandler(m_revokeAuthorizationHandler) ||
#endif
        !m_directiveSequencer->addDirectiveHandler(reportStateHandler)) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToRegisterSystemDirectiveHandler"));
        return false;
    }

    if (externalCapabilitiesBuilder) {
        externalCapabilitiesBuilder->withSettingsStorage(deviceSettingStorage);
        externalCapabilitiesBuilder->withInternetConnectionMonitor(m_internetConnectionMonitor);
        externalCapabilitiesBuilder->withDialogUXStateAggregator(m_dialogUXStateAggregator);
        auto concreteExternalMediaPlayer =
            manufactory->get<std::shared_ptr<acsdkExternalMediaPlayer::ExternalMediaPlayer>>();
        auto externalCapabilities = externalCapabilitiesBuilder->buildCapabilities(
            concreteExternalMediaPlayer,
            m_connectionManager,
            m_connectionManager,
            m_exceptionSender,
            m_certifiedSender,
            m_audioFocusManager,
            customerDataManager,
            reportStateHandler,
            m_audioInputProcessor,
            m_speakerManager,
            m_directiveSequencer,
            userInactivityMonitor,
            m_contextManager,
            m_avsGatewayManager,
            ringtoneMediaPlayer,
            audioFactory,
            ringtoneChannelVolumeInterface,
#ifdef ENABLE_COMMS_AUDIO_PROXY
            commsMediaPlayer,
            commsSpeaker,
            sharedDataStream,
#endif
            powerResourceManager,
            m_softwareReporterCapabilityAgent,
            m_playbackRouter,
            m_endpointRegistrationManager,
            metricRecorder);
        for (auto& capability : externalCapabilities.first) {
            if (capability.configuration.hasValue()) {
                m_defaultEndpointBuilder->withCapability(capability.configuration.value(), capability.directiveHandler);
            } else {
                m_directiveSequencer->addDirectiveHandler(capability.directiveHandler);
            }
        }
        m_shutdownObjects.insert(
            m_shutdownObjects.end(), externalCapabilities.second.begin(), externalCapabilities.second.end());
        m_callManager = externalCapabilitiesBuilder->getCallManager();
    }

    if (softwareInfoSenderObserver) {
        m_softwareInfoSenderObservers.insert(softwareInfoSenderObserver);
    }

    if (m_callManager) {
        m_softwareInfoSenderObservers.insert(m_callManager);
    }

    if (avsCommon::sdkInterfaces::softwareInfo::isValidFirmwareVersion(firmwareVersion)) {
        auto tempSender = capabilityAgents::system::SoftwareInfoSender::create(
            firmwareVersion,
            sendSoftwareInfoOnConnected,
            m_softwareInfoSenderObservers,
            m_connectionManager,
            m_connectionManager,
            m_exceptionSender);
        if (tempSender) {
            std::lock_guard<std::mutex> lock(m_softwareInfoSenderMutex);
            m_softwareInfoSender = tempSender;
        } else {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateSoftwareInfoSender"));
            return false;
        }
    }

    m_defaultEndpointBuilder->withCapabilityConfiguration(m_softwareReporterCapabilityAgent);

    if (sdkClientRegistry) {
        sdkClientRegistry->registerComponent(m_defaultEndpointBuilder);
        sdkClientRegistry->registerComponent(m_connectionManager);
        sdkClientRegistry->registerComponent(m_exceptionSender);
        sdkClientRegistry->registerComponent(m_certifiedSender);
        sdkClientRegistry->registerComponent(m_audioFocusManager);
        sdkClientRegistry->registerComponent(customerDataManager);
        sdkClientRegistry->registerComponent(reportStateHandler);
        sdkClientRegistry->registerComponent(m_audioInputProcessor);
        sdkClientRegistry->registerComponent(m_speakerManager);
        sdkClientRegistry->registerComponent(m_directiveSequencer);
        sdkClientRegistry->registerComponent(userInactivityMonitor);
        sdkClientRegistry->registerComponent(m_contextManager);
        sdkClientRegistry->registerComponent(m_avsGatewayManager);
        sdkClientRegistry->registerComponent(audioFactory);
        sdkClientRegistry->registerComponent(powerResourceManager);
        sdkClientRegistry->registerComponent(m_softwareReporterCapabilityAgent);
        sdkClientRegistry->registerComponent(m_playbackRouter);
        sdkClientRegistry->registerComponent(m_endpointRegistrationManager);
        sdkClientRegistry->registerComponent(metricRecorder);
        sdkClientRegistry->registerComponent(m_alexaMessageSender);
    }
    return true;
}

void DefaultClient::connect(bool performReset) {
    if (performReset) {
        if (m_defaultEndpointBuilder) {
            // Build default endpoint.
            auto defaultEndpoint = m_defaultEndpointBuilder->build();
            if (!defaultEndpoint) {
                ACSDK_CRITICAL(LX("connectFailed").d("reason", "couldNotBuildDefaultEndpoint"));
                return;
            }

            // Register default endpoint. Only wait for immediate failures and return with a critical error, if so.
            // Otherwise, the default endpoint will be registered with AVS in the post-connect stage (once
            // m_connectionManager->enable() is called, below). We should not block on waiting for resultFuture
            // to be ready, since instead we rely on the post-connect operation and the onCapabilitiesStateChange
            // callback.
            auto resultFuture = m_endpointRegistrationManager->registerEndpoint(std::move(defaultEndpoint));
            if ((resultFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)) {
                auto result = resultFuture.get();
                if (result != alexaClientSDK::endpoints::EndpointRegistrationManager::RegistrationResult::SUCCEEDED) {
                    ACSDK_CRITICAL(LX("connectFailed").d("reason", "registrationFailed").d("result", result));
                    return;
                }
            }
            m_defaultEndpointBuilder.reset();
        }
        // Ensure default endpoint registration is enqueued with @c EndpointRegistrationManager
        // before proceeding with connection. Otherwise, we risk a race condition where the post-connect operations
        // are created before the default endpoint is enqueued for publishing to AVS.
        m_endpointRegistrationManager->waitForPendingRegistrationsToEnqueue();
        m_avsGatewayManager->setAVSGatewayAssigner(m_connectionManager);
    }
    m_connectionManager->enable();
}

void DefaultClient::disconnect() {
    m_connectionManager->disable();
}

std::string DefaultClient::getAVSGateway() {
    return m_connectionManager->getAVSGateway();
}

void DefaultClient::stopForegroundActivity() {
    m_audioFocusManager->stopForegroundActivity();
}

void DefaultClient::stopAllActivities() {
    m_audioFocusManager->stopAllActivities();
}

void DefaultClient::localStopActiveAlert() {
    m_alertsCapabilityAgent->onLocalStop();
}

void DefaultClient::addAlexaDialogStateObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface> observer) {
    m_dialogUXStateAggregator->addObserver(observer);
}

void DefaultClient::removeAlexaDialogStateObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface> observer) {
    m_dialogUXStateAggregator->removeObserver(observer);
}

void DefaultClient::addMessageObserver(std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer) {
    m_connectionManager->addMessageObserver(observer);
}

void DefaultClient::removeMessageObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer) {
    m_connectionManager->removeMessageObserver(observer);
}

void DefaultClient::addConnectionObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> observer) {
    m_connectionManager->addConnectionStatusObserver(observer);
}

void DefaultClient::removeConnectionObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> observer) {
    m_connectionManager->removeConnectionStatusObserver(observer);
}

void DefaultClient::addInternetConnectionObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionObserverInterface> observer) {
    m_internetConnectionMonitor->addInternetConnectionObserver(observer);
}

void DefaultClient::removeInternetConnectionObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionObserverInterface> observer) {
    m_internetConnectionMonitor->removeInternetConnectionObserver(observer);
}

void DefaultClient::addAlertsObserver(std::shared_ptr<acsdkAlertsInterfaces::AlertObserverInterface> observer) {
    m_alertsCapabilityAgent->addObserver(observer);
}

void DefaultClient::removeAlertsObserver(std::shared_ptr<acsdkAlertsInterfaces::AlertObserverInterface> observer) {
    m_alertsCapabilityAgent->removeObserver(observer);
}

void DefaultClient::addAudioPlayerObserver(
    std::shared_ptr<acsdkAudioPlayerInterfaces::AudioPlayerObserverInterface> observer) {
    m_audioPlayer->addObserver(observer);
}

void DefaultClient::removeAudioPlayerObserver(
    std::shared_ptr<acsdkAudioPlayerInterfaces::AudioPlayerObserverInterface> observer) {
    m_audioPlayer->removeObserver(observer);
}

void DefaultClient::addTemplateRuntimeObserver(
    std::shared_ptr<templateRuntimeInterfaces::TemplateRuntimeObserverInterface> observer) {
    if (!m_templateRuntime) {
        ACSDK_ERROR(LX("addTemplateRuntimeObserverFailed").d("reason", "guiNotSupported"));
        return;
    }
    m_templateRuntime->addObserver(observer);
}

void DefaultClient::removeTemplateRuntimeObserver(
    std::shared_ptr<templateRuntimeInterfaces::TemplateRuntimeObserverInterface> observer) {
    if (!m_templateRuntime) {
        ACSDK_ERROR(LX("removeTemplateRuntimeObserverFailed").d("reason", "guiNotSupported"));
        return;
    }
    m_templateRuntime->removeObserver(observer);
}

void DefaultClient::addNotificationsObserver(
    std::shared_ptr<acsdkNotificationsInterfaces::NotificationsObserverInterface> observer) {
    m_notificationsNotifier->addObserver(observer);
}

void DefaultClient::removeNotificationsObserver(
    std::shared_ptr<acsdkNotificationsInterfaces::NotificationsObserverInterface> observer) {
    m_notificationsNotifier->removeObserver(observer);
}

void DefaultClient::addExternalMediaPlayerObserver(
    std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerObserverInterface> observer) {
    m_externalMediaPlayer->addObserver(observer);
}

void DefaultClient::removeExternalMediaPlayerObserver(
    std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerObserverInterface> observer) {
    m_externalMediaPlayer->removeObserver(observer);
}

void DefaultClient::addCaptionPresenter(std::shared_ptr<captions::CaptionPresenterInterface> presenter) {
    if (m_captionManager) {
        m_captionManager->setCaptionPresenter(presenter);
    }
}

void DefaultClient::setCaptionMediaPlayers(
    const std::vector<std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>>& mediaPlayers) {
    if (m_captionManager) {
        m_captionManager->setMediaPlayers(mediaPlayers);
    }
}

void DefaultClient::addBluetoothDeviceObserver(
    std::shared_ptr<acsdkBluetoothInterfaces::BluetoothDeviceObserverInterface> observer) {
    if (!m_bluetoothNotifier) {
        ACSDK_DEBUG5(LX(__func__).m("Bluetooth disabled"));
        return;
    }
    m_bluetoothNotifier->addObserver(observer);
}

void DefaultClient::removeBluetoothDeviceObserver(
    std::shared_ptr<acsdkBluetoothInterfaces::BluetoothDeviceObserverInterface> observer) {
    if (m_bluetoothNotifier) {
        m_bluetoothNotifier->removeObserver(observer);
    }
}

#ifdef ENABLE_REVOKE_AUTH
void DefaultClient::addRevokeAuthorizationObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::RevokeAuthorizationObserverInterface> observer) {
    if (!m_revokeAuthorizationHandler) {
        ACSDK_ERROR(LX("addRevokeAuthorizationObserver").d("reason", "revokeAuthorizationNotSupported"));
        return;
    }
    m_revokeAuthorizationHandler->addObserver(observer);
}

void DefaultClient::removeRevokeAuthorizationObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::RevokeAuthorizationObserverInterface> observer) {
    if (!m_revokeAuthorizationHandler) {
        ACSDK_ERROR(LX("removeRevokeAuthorizationObserver").d("reason", "revokeAuthorizationNotSupported"));
        return;
    }
    m_revokeAuthorizationHandler->removeObserver(observer);
}
#endif

std::shared_ptr<settings::DeviceSettingsManager> DefaultClient::getSettingsManager() {
    return m_deviceSettingsManager;
}

std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> DefaultClient::getPlaybackRouter() const {
    return m_playbackRouter;
}

std::shared_ptr<registrationManager::RegistrationManagerInterface> DefaultClient::getRegistrationManager() {
    return m_registrationManager;
}

void DefaultClient::addRegistrationObserver(
    const std::shared_ptr<registrationManager::RegistrationObserverInterface>& observer) {
    m_registrationNotifier->addObserver(observer);
}

void DefaultClient::removeRegistrationObserver(
    const std::shared_ptr<registrationManager::RegistrationObserverInterface>& observer) {
    m_registrationNotifier->removeObserver(observer);
}

std::shared_ptr<acsdkEqualizer::EqualizerController> DefaultClient::getEqualizerController() {
    return m_equalizerController;
}

void DefaultClient::addEqualizerControllerListener(
    std::shared_ptr<acsdkEqualizerInterfaces::EqualizerControllerListenerInterface> listener) {
    if (m_equalizerController) {
        m_equalizerController->addListener(listener);
    }
}

void DefaultClient::removeEqualizerControllerListener(
    std::shared_ptr<acsdkEqualizerInterfaces::EqualizerControllerListenerInterface> listener) {
    if (m_equalizerController) {
        m_equalizerController->removeListener(listener);
    }
}

void DefaultClient::addContextManagerObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerObserverInterface> observer) {
    if (m_contextManager) {
        m_contextManager->addContextManagerObserver(observer);
    }
}

void DefaultClient::removeContextManagerObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerObserverInterface> observer) {
    if (m_contextManager) {
        m_contextManager->removeContextManagerObserver(observer);
    }
}

void DefaultClient::addSpeakerManagerObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerObserverInterface> observer) {
    m_speakerManager->addSpeakerManagerObserver(observer);
}

void DefaultClient::removeSpeakerManagerObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerObserverInterface> observer) {
    m_speakerManager->removeSpeakerManagerObserver(observer);
}

std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> DefaultClient::getSpeakerManager() {
    return m_speakerManager;
}

void DefaultClient::addSpeechSynthesizerObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::SpeechSynthesizerObserverInterface> observer) {
    m_speechSynthesizer->addObserver(observer);
}

void DefaultClient::removeSpeechSynthesizerObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::SpeechSynthesizerObserverInterface> observer) {
    m_speechSynthesizer->removeObserver(observer);
}

bool DefaultClient::setFirmwareVersion(avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion) {
    {
        std::lock_guard<std::mutex> lock(m_softwareInfoSenderMutex);

        if (!m_softwareInfoSender) {
            m_softwareInfoSender = capabilityAgents::system::SoftwareInfoSender::create(
                firmwareVersion,
                true,
                m_softwareInfoSenderObservers,
                m_connectionManager,
                m_connectionManager,
                m_exceptionSender);
            if (m_softwareInfoSender) {
                return true;
            }

            ACSDK_ERROR(LX("setFirmwareVersionFailed").d("reason", "unableToCreateSoftwareInfoSender"));
            return false;
        }
    }

    return m_softwareInfoSender->setFirmwareVersion(firmwareVersion);
}

std::future<bool> DefaultClient::notifyOfWakeWord(
    capabilityAgents::aip::AudioProvider wakeWordAudioProvider,
    avsCommon::avs::AudioInputStream::Index beginIndex,
    avsCommon::avs::AudioInputStream::Index endIndex,
    std::string keyword,
    std::chrono::steady_clock::time_point startOfSpeechTimestamp,
    std::shared_ptr<const std::vector<char>> KWDMetadata) {
    ACSDK_DEBUG5(LX(__func__).d("keyword", keyword).d("connected", m_connectionManager->isConnected()));

    if (!m_connectionManager->isConnected()) {
        std::promise<bool> ret;
        if (capabilityAgents::aip::AudioInputProcessor::KEYWORD_TEXT_STOP == keyword) {
            // Alexa Stop uttered while offline
            ACSDK_INFO(LX("notifyOfWakeWord").d("action", "localStop").d("reason", "stopUtteredWhileNotConnected"));
            stopForegroundActivity();

            // Returning as interaction handled
            ret.set_value(true);
            return ret.get_future();
        } else {
            // Ignore Alexa wake word while disconnected
            ACSDK_INFO(LX("notifyOfWakeWord").d("action", "ignoreAlexaWakeWord").d("reason", "networkDisconnected"));

            // Returning as interaction not handled
            ret.set_value(false);
            return ret.get_future();
        }
    }

    return m_audioInputProcessor->recognize(
        wakeWordAudioProvider,
        capabilityAgents::aip::Initiator::WAKEWORD,
        startOfSpeechTimestamp,
        beginIndex,
        endIndex,
        keyword,
        KWDMetadata);
}

std::future<bool> DefaultClient::notifyOfTapToTalk(
    capabilityAgents::aip::AudioProvider tapToTalkAudioProvider,
    avsCommon::avs::AudioInputStream::Index beginIndex,
    std::chrono::steady_clock::time_point startOfSpeechTimestamp) {
    ACSDK_DEBUG5(LX(__func__));
    return m_audioInputProcessor->recognize(
        tapToTalkAudioProvider, capabilityAgents::aip::Initiator::TAP, startOfSpeechTimestamp, beginIndex);
}

std::future<bool> DefaultClient::notifyOfHoldToTalkStart(
    capabilityAgents::aip::AudioProvider holdToTalkAudioProvider,
    std::chrono::steady_clock::time_point startOfSpeechTimestamp,
    avsCommon::avs::AudioInputStream::Index beginIndex) {
    ACSDK_DEBUG5(LX(__func__));
    return m_audioInputProcessor->recognize(
        holdToTalkAudioProvider, capabilityAgents::aip::Initiator::PRESS_AND_HOLD, startOfSpeechTimestamp, beginIndex);
}

std::future<bool> DefaultClient::notifyOfHoldToTalkEnd() {
    return m_audioInputProcessor->stopCapture();
}

std::future<bool> DefaultClient::notifyOfTapToTalkEnd() {
    return m_audioInputProcessor->stopCapture();
}

void DefaultClient::addEndpointRegistrationManagerObserver(
    const std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointRegistrationObserverInterface>& observer) {
    m_endpointRegistrationManager->addObserver(observer);
};

void DefaultClient::removeEndpointRegistrationManagerObserver(
    const std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointRegistrationObserverInterface>& observer) {
    m_endpointRegistrationManager->removeObserver(observer);
};

void DefaultClient::addAudioInputProcessorObserver(
    const std::shared_ptr<avsCommon::sdkInterfaces::AudioInputProcessorObserverInterface>& observer) {
    m_audioInputProcessor->addObserver(observer);
}

void DefaultClient::removeAudioInputProcessorObserver(
    const std::shared_ptr<avsCommon::sdkInterfaces::AudioInputProcessorObserverInterface>& observer) {
    m_audioInputProcessor->removeObserver(observer);
}

void DefaultClient::addCallStateObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::CallStateObserverInterface> observer) {
    if (m_callManager) {
        m_callManager->addObserver(observer);
    }
}

void DefaultClient::removeCallStateObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::CallStateObserverInterface> observer) {
    if (m_callManager) {
        m_callManager->removeObserver(observer);
    }
}

std::shared_ptr<EndpointBuilderInterface> DefaultClient::createEndpointBuilder() {
    return alexaClientSDK::endpoints::EndpointBuilder::create(
        m_deviceInfo, m_contextManager, m_exceptionSender, m_alexaMessageSender);
}

std::shared_ptr<EndpointBuilderInterface> DefaultClient::getDefaultEndpointBuilder() {
    return m_defaultEndpointBuilder;
}

std::future<alexaClientSDK::endpoints::EndpointRegistrationManager::RegistrationResult> DefaultClient::registerEndpoint(
    std::shared_ptr<EndpointInterface> endpoint) {
    if (m_endpointRegistrationManager) {
        return m_endpointRegistrationManager->registerEndpoint(endpoint);
    } else {
        ACSDK_ERROR(LX("registerEndpointFailed").d("reason", "invalid EndpointRegistrationManager"));
        std::promise<alexaClientSDK::endpoints::EndpointRegistrationManager::RegistrationResult> promise;
        promise.set_value(alexaClientSDK::endpoints::EndpointRegistrationManager::RegistrationResult::INTERNAL_ERROR);
        return promise.get_future();
    }
}

std::future<alexaClientSDK::endpoints::EndpointRegistrationManager::UpdateResult> DefaultClient::updateEndpoint(
    const EndpointIdentifier& endpointId,
    const std::shared_ptr<EndpointModificationData>& endpointModificationData) {
    if (m_endpointRegistrationManager) {
        return m_endpointRegistrationManager->updateEndpoint(endpointId, endpointModificationData);
    } else {
        ACSDK_ERROR(LX("updateEndpointFailed").d("reason", "invalid EndpointRegistrationManager"));
        std::promise<alexaClientSDK::endpoints::EndpointRegistrationManager::UpdateResult> promise;
        promise.set_value(alexaClientSDK::endpoints::EndpointRegistrationManager::UpdateResult::INTERNAL_ERROR);
        return promise.get_future();
    }
}

std::future<alexaClientSDK::endpoints::EndpointRegistrationManager::DeregistrationResult> DefaultClient::
    deregisterEndpoint(EndpointIdentifier endpointId) {
    if (m_endpointRegistrationManager) {
        return m_endpointRegistrationManager->deregisterEndpoint(endpointId);
    } else {
        ACSDK_ERROR(LX("deregisterEndpointFailed").d("reason", "invalid EndpointRegistrationManager"));
        std::promise<alexaClientSDK::endpoints::EndpointRegistrationManager::DeregistrationResult> promise;
        promise.set_value(alexaClientSDK::endpoints::EndpointRegistrationManager::DeregistrationResult::INTERNAL_ERROR);
        return promise.get_future();
    }
}

bool DefaultClient::isCommsEnabled() {
    return (m_callManager != nullptr);
}

void DefaultClient::acceptCommsCall() {
    if (m_callManager) {
        m_callManager->acceptCall();
    }
}

void DefaultClient::sendDtmf(avsCommon::sdkInterfaces::CallManagerInterface::DTMFTone dtmfTone) {
    if (m_callManager) {
        m_callManager->sendDtmf(dtmfTone);
    }
}

void DefaultClient::stopCommsCall() {
    if (m_callManager) {
        m_callManager->stopCall();
    }
}

void DefaultClient::audioPlayerLocalStop() {
    if (m_audioPlayer) {
        m_audioPlayer->stopPlayback();
    }
}

bool DefaultClient::isCommsCallMuted() {
    if (m_callManager) {
        return m_callManager->isSelfMuted();
    }
    return false;
}

void DefaultClient::muteCommsCall() {
    if (m_callManager) {
        m_callManager->muteSelf();
    }
}

void DefaultClient::unmuteCommsCall() {
    if (m_callManager) {
        m_callManager->unmuteSelf();
    }
}

void DefaultClient::enableVideo() {
    if (m_callManager) {
        m_callManager->enableVideo();
    }
}

void DefaultClient::disableVideo() {
    if (m_callManager) {
        m_callManager->disableVideo();
    }
}

void DefaultClient::onSystemClockSynchronized() {
    m_systemClockMonitor->onSystemClockSynchronized();
}

void DefaultClient::registerExternalMediaPlayerAdapterHandler(
    std::shared_ptr<alexaClientSDK::acsdkExternalMediaPlayerInterfaces::ExternalMediaAdapterHandlerInterface>
        externalMediaPlayerAdapterHandler) {
    if (m_externalMediaPlayer) {
        m_externalMediaPlayer->addAdapterHandler(externalMediaPlayerAdapterHandler);
    }
}

std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface> DefaultClient::getShutdownManager() {
    return m_shutdownManager;
}

std::shared_ptr<acsdkDeviceSetupInterfaces::DeviceSetupInterface> DefaultClient::getDeviceSetup() {
    return m_deviceSetup;
}

std::shared_ptr<acsdkBluetoothInterfaces::BluetoothLocalInterface> DefaultClient::getBluetoothLocal() {
    return m_bluetoothLocal;
}

DefaultClient::~DefaultClient() {
    shutdown();
}

bool DefaultClient::setEncodingAudioFormat(AudioFormat::Encoding encoding) {
    return m_audioInputProcessor->setEncodingAudioFormat(encoding);
}

capabilityAgents::aip::AudioInputProcessor::EncodingFormatResponse DefaultClient::requestEncodingAudioFormats(
    const capabilityAgents::aip::AudioInputProcessor::EncodingFormatRequest& encodings) {
    return m_audioInputProcessor->requestEncodingAudioFormats(encodings);
}

bool DefaultClient::configure(const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry) {
    return true;
}

DefaultClient::DefaultClient() : FeatureClientInterface(TAG) {
}

void DefaultClient::doShutdown() {
    if (m_shutdownManager) {
        m_shutdownManager->shutdown();
        m_shutdownManager.reset();
    }

    while (!m_shutdownObjects.empty()) {
        if (m_shutdownObjects.back()) {
            m_shutdownObjects.back()->shutdown();
        }
        m_shutdownObjects.pop_back();
    }

    if (m_endpointRegistrationManager) {
        ACSDK_DEBUG5(LX("EndpointRegistrationManagerShutdown"));
        m_endpointRegistrationManager->shutdown();
    }
    if (m_audioInputProcessor) {
        ACSDK_DEBUG5(LX("AIPShutdown"));
        removeInternetConnectionObserver(m_audioInputProcessor);
        m_audioInputProcessor->shutdown();
    }
    if (m_speechSynthesizer) {
        ACSDK_DEBUG5(LX("SpeechSynthesizerShutdown"));
        m_speechSynthesizer->shutdown();
    }
    if (m_softwareInfoSender) {
        ACSDK_DEBUG5(LX("SoftwareInfoShutdown"));
        m_softwareInfoSender->shutdown();
    }
    if (m_certifiedSender) {
        ACSDK_DEBUG5(LX("CertifiedSenderShutdown."));
        m_certifiedSender->shutdown();
    }

    if (m_apiGatewayCapabilityAgent) {
        ACSDK_DEBUG5(LX("CallApiGatewayCapabilityAgentShutdown."));
        m_apiGatewayCapabilityAgent->shutdown();
    }

#ifdef ENABLE_PCC
    if (m_phoneCallControllerCapabilityAgent) {
        ACSDK_DEBUG5(LX("PhoneCallControllerCapabilityAgentShutdown"));
        m_phoneCallControllerCapabilityAgent->shutdown();
    }
#endif
#ifdef ENABLE_MCC
    if (m_meetingClientControllerCapabilityAgent) {
        ACSDK_DEBUG5(LX("MeetingClientControllerCapabilityAgentShutdown"));
        m_meetingClientControllerCapabilityAgent->shutdown();
    }
#endif
    if (nullptr != m_equalizerCapabilityAgent) {
        for (auto& equalizer : m_equalizerRuntimeSetup->getAllEqualizers()) {
            m_equalizerController->unregisterEqualizer(equalizer);
        }
        for (auto& listener : m_equalizerRuntimeSetup->getAllEqualizerControllerListeners()) {
            m_equalizerController->removeListener(listener);
        }
        ACSDK_DEBUG5(LX("EqualizerCapabilityAgentShutdown"));
        m_equalizerCapabilityAgent->shutdown();
    }

    if (m_diagnostics) {
        m_diagnostics->setDiagnosticDependencies(nullptr, nullptr, nullptr);

        auto deviceProperties = m_diagnostics->getDevicePropertyAggregator();
        if (deviceProperties) {
            deviceProperties->setContextManager(nullptr);
            deviceProperties->setDeviceSettingsManager(nullptr);
            removeSpeakerManagerObserver(deviceProperties);
            removeAlertsObserver(deviceProperties);
            removeConnectionObserver(deviceProperties);
            removeNotificationsObserver(deviceProperties);
            removeAudioPlayerObserver(deviceProperties);
            removeAlexaDialogStateObserver(deviceProperties);
            m_authDelegate->removeAuthObserver(deviceProperties);
        }

        auto protocolTrace = m_diagnostics->getProtocolTracer();
        if (protocolTrace) {
            removeMessageObserver(protocolTrace);
        }
    }
}

void DefaultClient::stopInteraction() {
    if (m_audioInputProcessor) {
        m_audioInputProcessor->resetState();
    }
}

std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> DefaultClient::getAudioFocusManager() {
    return m_audioFocusManager;
}

}  // namespace defaultClient
}  // namespace alexaClientSDK
