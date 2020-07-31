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

#include <ADSL/MessageInterpreter.h>
#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/AVS/ExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/InternetConnectionMonitorInterface.h>
#include <AVSCommon/SDKInterfaces/SystemClockMonitorObserverInterface.h>
#include <AVSCommon/Utils/Bluetooth/BluetoothEventBus.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/Network/InternetConnectionMonitor.h>
#include <Audio/SystemSoundAudioFactory.h>

#include <SystemSoundPlayer/SystemSoundPlayer.h>

#ifdef ENABLE_CAPTIONS
#include <Captions/LibwebvttParserAdapter.h>
#include <Captions/TimingAdapterFactory.h>
#endif

#ifdef ENABLE_OPUS
#include <SpeechEncoder/OpusEncoderContext.h>
#endif

#ifdef ENABLE_PCC
#include <AVSCommon/SDKInterfaces/Phone/PhoneCallerInterface.h>
#include <PhoneCallController/PhoneCallController.h>
#endif

#ifdef ENABLE_MCC
#include <AVSCommon/SDKInterfaces/Calendar/CalendarClientInterface.h>
#include <AVSCommon/SDKInterfaces/Meeting/MeetingClientInterface.h>
#include <MeetingClientController/MeetingClientController.h>
#endif

#include <AVSCommon/AVS/SpeakerConstants/SpeakerConstants.h>
#include <InterruptModel/InterruptModel.h>
#include <System/LocaleHandler.h>
#include <System/ReportStateHandler.h>
#include <System/SystemCapabilityProvider.h>
#include <System/TimeZoneHandler.h>
#include <System/UserInactivityMonitor.h>

#ifdef BLUETOOTH_BLUEZ
#include <BlueZ/BlueZBluetoothDeviceManager.h>
#include <acsdkBluetooth/BluetoothMediaInputTransformer.h>
#endif
#include "DefaultClient/DefaultClient.h"
#include "DefaultClient/DeviceSettingsManagerBuilder.h"
#include "SDKComponent/SDKComponent.h"

namespace alexaClientSDK {
namespace defaultClient {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::endpoints;
using namespace alexaClientSDK::endpoints;

/// Key for audio channel array configurations in configuration node.
static const std::string AUDIO_CHANNEL_CONFIG_KEY = "audioChannels";

/// Key for visual channel array configurations in configuration node.
static const std::string VISUAL_CHANNEL_CONFIG_KEY = "visualChannels";

/// Key for the interrupt model configuration
static const std::string INTERRUPT_MODEL_CONFIG_KEY = "interruptModel";

using namespace alexaClientSDK::avsCommon::sdkInterfaces;
using namespace alexaClientSDK::avsCommon::utils;

/// String to identify log entries originating from this file.
static const std::string TAG("DefaultClient");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<DefaultClient> DefaultClient::create(
    std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo,
    std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
    const std::unordered_map<std::string, std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>>&
        externalMusicProviderMediaPlayers,
    const std::unordered_map<std::string, std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>&
        externalMusicProviderSpeakers,
    const capabilityAgents::externalMediaPlayer::ExternalMediaPlayer::AdapterCreationMap& adapterCreationMap,
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
    std::shared_ptr<avsCommon::sdkInterfaces::phone::PhoneCallerInterface> phoneCaller,
#endif
#ifdef ENABLE_MCC
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> meetingSpeaker,
    std::shared_ptr<avsCommon::sdkInterfaces::meeting::MeetingClientInterface> meetingClient,
    std::shared_ptr<avsCommon::sdkInterfaces::calendar::CalendarClientInterface> calendarClient,
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
    std::unique_ptr<settings::storage::DeviceSettingStorageInterface> deviceSettingStorage,
    std::shared_ptr<acsdkBluetooth::BluetoothStorageInterface> bluetoothStorage,
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
    std::shared_ptr<alexaClientSDK::acl::MessageRouterFactoryInterface> messageRouterFactory) {
    if (!deviceInfo) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullDeviceInfo"));
        return nullptr;
    }

    std::unique_ptr<DefaultClient> defaultClient(new DefaultClient(*deviceInfo));
    if (!defaultClient->initialize(
            customerDataManager,
            externalMusicProviderMediaPlayers,
            externalMusicProviderSpeakers,
            adapterCreationMap,
            speakMediaPlayer,
            std::move(audioMediaPlayerFactory),
            alertsMediaPlayer,
            notificationsMediaPlayer,
            bluetoothMediaPlayer,
            ringtoneMediaPlayer,
            systemSoundMediaPlayer,
            speakSpeaker,
            audioSpeakers,
            alertsSpeaker,
            notificationsSpeaker,
            bluetoothSpeaker,
            ringtoneSpeaker,
            systemSoundSpeaker,
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
            equalizerRuntimeSetup,
            audioFactory,
            authDelegate,
            alertStorage,
            messageStorage,
            notificationsStorage,
            std::move(deviceSettingStorage),
            bluetoothStorage,
            miscStorage,
            alexaDialogStateObservers,
            connectionObservers,
            internetConnectionMonitor,
            isGuiSupported,
            capabilitiesDelegate,
            contextManager,
            transportFactory,
            avsGatewayManager,
            localeAssetsManager,
            enabledConnectionRules,
            systemTimezone,
            firmwareVersion,
            sendSoftwareInfoOnConnected,
            softwareInfoSenderObserver,
            std::move(bluetoothDeviceManager),
            std::move(metricRecorder),
            powerResourceManager,
            diagnostics,
            externalCapabilitiesBuilder,
            channelVolumeFactory,
            startAlertSchedulingOnInitialization,
            messageRouterFactory)) {
        return nullptr;
    }

    return defaultClient;
}

DefaultClient::DefaultClient(const avsCommon::utils::DeviceInfo& deviceInfo) : m_deviceInfo{deviceInfo} {
}

bool DefaultClient::initialize(
    std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
    const std::unordered_map<std::string, std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>>&
        externalMusicProviderMediaPlayers,
    const std::unordered_map<std::string, std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>&
        externalMusicProviderSpeakers,
    const capabilityAgents::externalMediaPlayer::ExternalMediaPlayer::AdapterCreationMap& adapterCreationMap,
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
    std::shared_ptr<avsCommon::sdkInterfaces::phone::PhoneCallerInterface> phoneCaller,
#endif
#ifdef ENABLE_MCC
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> meetingSpeaker,
    std::shared_ptr<avsCommon::sdkInterfaces::meeting::MeetingClientInterface> meetingClient,
    std::shared_ptr<avsCommon::sdkInterfaces::calendar::CalendarClientInterface> calendarClient,
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
    std::shared_ptr<acsdkBluetooth::BluetoothStorageInterface> bluetoothStorage,
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
    std::shared_ptr<alexaClientSDK::acl::MessageRouterFactoryInterface> messageRouterFactory) {

    if (!audioFactory) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullAudioFactory"));
        return false;
    }

    if (!speakMediaPlayer) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullSpeakMediaPlayer"));
        return false;
    }

    if (!audioMediaPlayerFactory) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullAudioMediaPlayerFactory"));
        return false;
    }

    if (!alertsMediaPlayer) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullAlertsMediaPlayer"));
        return false;
    }

    if (!notificationsMediaPlayer) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullNotificationsMediaPlayer"));
        return false;
    }

    if (!bluetoothMediaPlayer) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullBluetoothMediaPlayer"));
        return false;
    }

    if (!ringtoneMediaPlayer) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullRingtoneMediaPlayer"));
        return false;
    }

    if (!systemSoundMediaPlayer) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullSystemSoundMediaPlayer"));
        return false;
    }

    if (!authDelegate) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullAuthDelegate"));
        return false;
    }

    if (!capabilitiesDelegate) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullCapabilitiesDelegate"));
        return false;
    }

    if (!deviceSettingStorage) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullDeviceSettingStorage"));
        return false;
    }

    if (!contextManager) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullContextManager"));
        return false;
    }

    if (!transportFactory) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullTransportFactory"));
        return false;
    }

    if (!avsGatewayManager) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullAVSGatewayManager"));
        return false;
    }

    if (!channelVolumeFactory) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullChannelVolumeFactory"));
        return false;
    }

    if (!messageRouterFactory) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullMessageRouterFactory"));
    }

    m_softwareReporterCapabilityAgent =
        capabilityAgents::softwareComponentReporter::SoftwareComponentReporterCapabilityAgent::create();
    if (!m_softwareReporterCapabilityAgent) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullSoftwareReporterCapabilityAgent"));
        return false;
    }

    if (!applicationUtilities::SDKComponent::SDKComponent::registerComponent(m_softwareReporterCapabilityAgent)) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToRegisterSDKComponent"));
        return false;
    }

    m_avsGatewayManager = avsGatewayManager;

    m_dialogUXStateAggregator = std::make_shared<avsCommon::avs::DialogUXStateAggregator>(metricRecorder);

    for (auto observer : alexaDialogStateObservers) {
        m_dialogUXStateAggregator->addObserver(observer);
    }

    /*
     * Creating the Attachment Manager - This component deals with managing
     * attachments and allows for readers and
     * writers to be created to handle the attachment.
     */
    auto attachmentManager = std::make_shared<avsCommon::avs::attachment::AttachmentManager>(
        avsCommon::avs::attachment::AttachmentManager::AttachmentType::IN_PROCESS);

    /*
     * Creating the message router - This component actually maintains the
     * connection to AVS over HTTP2. It is created
     * using the auth delegate, which provides authorization to connect to AVS,
     * and the attachment manager, which helps
     * ACL write attachments received from AVS.
     */
    m_messageRouter = messageRouterFactory->createMessageRouter(authDelegate, attachmentManager, transportFactory);

    if (!internetConnectionMonitor) {
        ACSDK_CRITICAL(LX("initializeFailed").d("reason", "internetConnectionMonitor was nullptr"));
        return false;
    }
    m_internetConnectionMonitor = internetConnectionMonitor;

    /*
     * Creating the connection manager - This component is the overarching
     * connection manager that glues together all
     * the other networking components into one easy-to-use component.
     */
    m_connectionManager = acl::AVSConnectionManager::create(
        m_messageRouter, false, connectionObservers, {m_dialogUXStateAggregator}, internetConnectionMonitor);
    if (!m_connectionManager) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateConnectionManager"));
        return false;
    }

    /*
     * Creating our certified sender - this component guarantees that messages
     * given to it (expected to be JSON
     * formatted AVS Events) will be sent to AVS.  This nicely decouples strict
     * message sending from components which
     * require an Event be sent, even in conditions when there is no active AVS
     * connection.
     */
    m_certifiedSender = certifiedSender::CertifiedSender::create(
        m_connectionManager, m_connectionManager, messageStorage, customerDataManager);
    if (!m_certifiedSender) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateCertifiedSender"));
        return false;
    }

    /*
     * Creating the Exception Sender - This component helps the SDK send
     * exceptions when it is unable to handle a
     * directive sent by AVS. For that reason, the Directive Sequencer and each
     * Capability Agent will need this
     * component.
     */
    m_exceptionSender = avsCommon::avs::ExceptionEncounteredSender::create(m_connectionManager);
    if (!m_exceptionSender) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateExceptionSender"));
        return false;
    }

    /*
     * Creating the Directive Sequencer - This is the component that deals with
     * the sequencing and ordering of
     * directives sent from AVS and forwarding them along to the appropriate
     * Capability Agent that deals with
     * directives in that Namespace/Name.
     */
    m_directiveSequencer = adsl::DirectiveSequencer::create(m_exceptionSender, metricRecorder);
    if (!m_directiveSequencer) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateDirectiveSequencer"));
        return false;
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

    /*
     * Creating the Registration Manager - This component is responsible for
     * implementing any customer registration
     * operation such as login and logout
     */
    m_registrationManager = std::make_shared<registrationManager::RegistrationManager>(
        m_directiveSequencer, m_connectionManager, customerDataManager);

    // Create endpoint related objects.
    m_contextManager = contextManager;
    m_capabilitiesDelegate = std::move(capabilitiesDelegate);
    m_capabilitiesDelegate->setMessageSender(m_connectionManager);
    m_avsGatewayManager->addObserver(m_capabilitiesDelegate);
    addConnectionObserver(m_capabilitiesDelegate);
    m_endpointRegistrationManager = EndpointRegistrationManager::create(
        m_directiveSequencer, m_capabilitiesDelegate, m_deviceInfo.getDefaultEndpointId());
    if (!m_endpointRegistrationManager) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "endpointRegistrationManagerCreateFailed"));
        return false;
    }

    m_deviceSettingStorage = deviceSettingStorage;
    if (!m_deviceSettingStorage->open()) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "deviceSettingStorageOpenFailed"));
        return false;
    }

    /*
     * Creating the DoNotDisturb Capability Agent.
     *
     * TODO(ACSDK-2279): Keep this here till we can inject DND setting into the DND CA.
     */
    m_dndCapabilityAgent = capabilityAgents::doNotDisturb::DoNotDisturbCapabilityAgent::create(
        m_exceptionSender, m_connectionManager, m_deviceSettingStorage);

    if (!m_dndCapabilityAgent) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateDNDCapabilityAgent"));
        return false;
    }

    addConnectionObserver(m_dndCapabilityAgent);

    DeviceSettingsManagerBuilder settingsManagerBuilder{
        m_deviceSettingStorage, m_connectionManager, m_connectionManager, customerDataManager};
    settingsManagerBuilder.withDoNotDisturbSetting(m_dndCapabilityAgent)
        .withAlarmVolumeRampSetting()
        .withWakeWordConfirmationSetting()
        .withSpeechConfirmationSetting()
        .withTimeZoneSetting(systemTimezone)
        .withNetworkInfoSetting();

    if (localeAssetsManager->getDefaultSupportedWakeWords().empty()) {
        settingsManagerBuilder.withLocaleSetting(localeAssetsManager);
    } else {
        settingsManagerBuilder.withLocaleAndWakeWordsSettings(localeAssetsManager);
    }

    m_deviceSettingsManager = settingsManagerBuilder.build();
    if (!m_deviceSettingsManager) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "createDeviceSettingsManagerFailed"));
        return false;
    }

    /*
     * Creating the Audio Activity Tracker - This component is responsibly for
     * reporting the audio channel focus
     * information to AVS.
     */
    m_audioActivityTracker = afml::AudioActivityTracker::create(contextManager);

    /**
     *  Interrupt Model object
     */
    auto interruptModel = alexaClientSDK::afml::interruptModel::InterruptModel::create(
        alexaClientSDK::avsCommon::utils::configuration::ConfigurationNode::getRoot()[INTERRUPT_MODEL_CONFIG_KEY]);

    // Read audioChannels configuration from config file
    std::vector<afml::FocusManager::ChannelConfiguration> audioVirtualChannelConfiguration;
    if (!afml::FocusManager::ChannelConfiguration::readChannelConfiguration(
            AUDIO_CHANNEL_CONFIG_KEY, &audioVirtualChannelConfiguration)) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToReadAudioChannelConfiguration"));
        return false;
    }

    /*
     * Creating the Focus Manager - This component deals with the management of
     * layered audio focus across various
     * components. It handles granting access to Channels as well as pushing
     * different "Channels" to foreground,
     * background, or no focus based on which other Channels are active and the
     * priorities of those Channels. Each
     * Capability Agent will require the Focus Manager in order to request access
     * to the Channel it wishes to play on.
     */
    m_audioFocusManager = std::make_shared<afml::FocusManager>(
        afml::FocusManager::getDefaultAudioChannels(),
        m_audioActivityTracker,
        audioVirtualChannelConfiguration,
        interruptModel);

#ifdef ENABLE_CAPTIONS
    /*
     * Creating the Caption Manager - This component deals with handling captioned content.
     */
    auto webvttParser = captions::LibwebvttParserAdapter::getInstance();
    m_captionManager = captions::CaptionManager::create(webvttParser);
#endif

    /*
     * Creating the User Inactivity Monitor - This component is responsibly for
     * updating AVS of user inactivity as
     * described in the System Interface of AVS.
     */
    m_userInactivityMonitor =
        capabilityAgents::system::UserInactivityMonitor::create(m_connectionManager, m_exceptionSender);
    if (!m_userInactivityMonitor) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateUserInactivityMonitor"));
        return false;
    }

    m_systemSoundPlayer = applicationUtilities::systemSoundPlayer::SystemSoundPlayer::create(
        systemSoundMediaPlayer, audioFactory->systemSounds());

    auto wakeWordConfirmationSetting =
        settingsManagerBuilder.getSetting<settings::DeviceSettingsIndex::WAKEWORD_CONFIRMATION>();
    auto speechConfirmationSetting =
        settingsManagerBuilder.getSetting<settings::DeviceSettingsIndex::SPEECH_CONFIRMATION>();
    auto wakeWordsSetting = settingsManagerBuilder.getSetting<settings::DeviceSettingsIndex::WAKE_WORDS>();

/*
 * Creating the Audio Input Processor - This component is the Capability Agent
 * that implements the SpeechRecognizer interface of AVS.
 */
#ifdef ENABLE_OPUS
    m_audioInputProcessor = capabilityAgents::aip::AudioInputProcessor::create(
        m_directiveSequencer,
        m_connectionManager,
        contextManager,
        m_audioFocusManager,
        m_dialogUXStateAggregator,
        m_exceptionSender,
        m_userInactivityMonitor,
        m_systemSoundPlayer,
        localeAssetsManager,
        wakeWordConfirmationSetting,
        speechConfirmationSetting,
        wakeWordsSetting,
        std::make_shared<speechencoder::SpeechEncoder>(std::make_shared<speechencoder::OpusEncoderContext>()),
        capabilityAgents::aip::AudioProvider::null(),
        powerResourceManager,
        metricRecorder);
#else
    m_audioInputProcessor = capabilityAgents::aip::AudioInputProcessor::create(
        m_directiveSequencer,
        m_connectionManager,
        contextManager,
        m_audioFocusManager,
        m_dialogUXStateAggregator,
        m_exceptionSender,
        m_userInactivityMonitor,
        m_systemSoundPlayer,
        localeAssetsManager,
        wakeWordConfirmationSetting,
        speechConfirmationSetting,
        wakeWordsSetting,
        nullptr,
        capabilityAgents::aip::AudioProvider::null(),
        powerResourceManager,
        metricRecorder);
#endif

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
    m_speechSynthesizer = capabilityAgents::speechSynthesizer::SpeechSynthesizer::create(
        speakMediaPlayer,
        m_connectionManager,
        m_audioFocusManager,
        contextManager,
        m_exceptionSender,
        metricRecorder,
        m_dialogUXStateAggregator,
#ifdef ENABLE_CAPTIONS
        m_captionManager,
#else
        nullptr,
#endif
        powerResourceManager);

    if (!m_speechSynthesizer) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateSpeechSynthesizer"));
        return false;
    }

    m_speechSynthesizer->addObserver(m_dialogUXStateAggregator);

    /*
     * Creating the PlaybackController Capability Agent - This component is the
     * Capability Agent that implements the
     * PlaybackController interface of AVS.
     */
    m_playbackController =
        capabilityAgents::playbackController::PlaybackController::create(contextManager, m_connectionManager);
    if (!m_playbackController) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreatePlaybackController"));
        return false;
    }

    /*
     * Creating the PlaybackRouter - This component routes a playback button press
     * to the active handler.
     * The default handler is @c PlaybackController.
     */
    m_playbackRouter = capabilityAgents::playbackController::PlaybackRouter::create(m_playbackController);
    if (!m_playbackRouter) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreatePlaybackRouter"));
        return false;
    }

    // create @c SpeakerInterfaces for each @c Type
    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>> allAvsSpeakers{speakSpeaker,
                                                                                            systemSoundSpeaker};
    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>> allAlertSpeakers{alertsSpeaker,
                                                                                              notificationsSpeaker};
    // parse additional Speakers into the right speaker list.
    for (const auto& it : additionalSpeakers) {
        if (ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME == it.first) {
            allAvsSpeakers.push_back(it.second);
        } else if (ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME == it.first) {
            allAlertSpeakers.push_back(it.second);
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
        allAlertChannelVolumeInterfaces, allChannelVolumeInterfaces;

    // create allAvsChannelVolumeInterfaces using allAvsSpeakers
    for (auto& it : allAvsSpeakers) {
        allAvsChannelVolumeInterfaces.push_back(channelVolumeFactory->createChannelVolumeInterface(it));
    }

    // create @c ChannelVolumeInterface for audioSpeakers (later used by AudioPlayer CapabilityAgents)
    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>> audioChannelVolumeInterfaces;
    for (auto& it : audioSpeakers) {
        audioChannelVolumeInterfaces.push_back(channelVolumeFactory->createChannelVolumeInterface(it));
    }
    allAvsChannelVolumeInterfaces.insert(
        allAvsChannelVolumeInterfaces.end(), audioChannelVolumeInterfaces.begin(), audioChannelVolumeInterfaces.end());

    // create @c ChannelVolumeInterface for bluetoothSpeaker (later used by Bluetooth CapabilityAgent)
    std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface> bluetoothChannelVolumeInterface =
        channelVolumeFactory->createChannelVolumeInterface(bluetoothSpeaker);
    allAvsChannelVolumeInterfaces.push_back(bluetoothChannelVolumeInterface);

    // create @c ChannelVolumeInterface for ringtoneSpeaker
    std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface> ringtoneChannelVolumeInterface =
        channelVolumeFactory->createChannelVolumeInterface(ringtoneSpeaker);
    allAvsChannelVolumeInterfaces.push_back(ringtoneChannelVolumeInterface);

    // create @c ChannelVolumeInterface for allAlertSpeakers
    for (auto& it : allAlertSpeakers) {
        allAlertChannelVolumeInterfaces.push_back(
            channelVolumeFactory->createChannelVolumeInterface(it, ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME));
    }

    // create @c ChannelVolumeInterface for EMP Speakers (later used by ExternalMediaPlayer CapabilityAgent)
    capabilityAgents::externalMediaPlayer::ExternalMediaPlayer::AdapterSpeakerMap externalMusicProviderVolumeInterfaces;
    for (auto& it : externalMusicProviderSpeakers) {
        auto empChannelVolume = channelVolumeFactory->createChannelVolumeInterface(it.second);
        externalMusicProviderVolumeInterfaces[it.first] = empChannelVolume;
        allAvsChannelVolumeInterfaces.push_back(empChannelVolume);
    }

    /*
     * Creating the SpeakerManager Capability Agent - This component is the
     * Capability Agent that implements the
     * Speaker interface of AVS.
     */
    allChannelVolumeInterfaces.insert(
        allChannelVolumeInterfaces.end(), allAvsChannelVolumeInterfaces.begin(), allAvsChannelVolumeInterfaces.end());
    allChannelVolumeInterfaces.insert(
        allChannelVolumeInterfaces.end(),
        allAlertChannelVolumeInterfaces.begin(),
        allAlertChannelVolumeInterfaces.end());

    m_speakerManager = capabilityAgents::speakerManager::SpeakerManager::create(
        allChannelVolumeInterfaces, contextManager, m_connectionManager, m_exceptionSender, metricRecorder);
    if (!m_speakerManager) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateSpeakerManager"));
        return false;
    }

    /*
     * Creating the Audio Player - This component is the Capability Agent that
     * implements the Audio Player interface of AVS.
     */
    m_audioPlayer = acsdkAudioPlayer::AudioPlayer::create(
        std::move(audioMediaPlayerFactory),
        m_connectionManager,
        m_audioFocusManager,
        contextManager,
        m_exceptionSender,
        m_playbackRouter,
        audioChannelVolumeInterfaces,
#ifdef ENABLE_CAPTIONS
        m_captionManager,
#else
        nullptr,
#endif
        metricRecorder);

    if (!m_audioPlayer) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateAudioPlayer"));
        return false;
    }

    auto alertRenderer = acsdkAlerts::renderer::Renderer::create(alertsMediaPlayer, metricRecorder);

    if (!alertRenderer) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateAlarmRenderer"));
        return false;
    }

    /*
     * Creating the Alerts Capability Agent - This component is the Capability
     * Agent that implements the Alerts
     * interface of AVS.
     */
    m_alertsCapabilityAgent = acsdkAlerts::AlertsCapabilityAgent::create(
        m_connectionManager,
        m_connectionManager,
        m_certifiedSender,
        m_audioFocusManager,
        m_speakerManager,
        contextManager,
        m_exceptionSender,
        alertStorage,
        audioFactory->alerts(),
        alertRenderer,
        customerDataManager,
        settingsManagerBuilder.getSetting<settings::ALARM_VOLUME_RAMP>(),
        m_deviceSettingsManager,
        metricRecorder,
        startAlertSchedulingOnInitialization);

    if (!m_alertsCapabilityAgent) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateAlertsCapabilityAgent"));
        return false;
    }

    /*
     * Creating the System Clock Monitor - This component notifies the time sensitive
     * components when the system clock resynchronizes.
     */
    m_systemClockMonitor = std::shared_ptr<avsCommon::utils::timing::SystemClockMonitor>(
        new avsCommon::utils::timing::SystemClockMonitor());
    m_systemClockMonitor->addSystemClockMonitorObserver(m_alertsCapabilityAgent);

    addConnectionObserver(m_dialogUXStateAggregator);

    m_notificationsRenderer =
        acsdkNotifications::NotificationRenderer::create(notificationsMediaPlayer, m_audioFocusManager);
    if (!m_notificationsRenderer) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateNotificationsRenderer"));
        return false;
    }

    /*
     * Creating the Notifications Capability Agent - This component is the
     * Capability Agent that implements the
     * Notifications interface of AVS.
     */
    m_notificationsCapabilityAgent = acsdkNotifications::NotificationsCapabilityAgent::create(
        notificationsStorage,
        m_notificationsRenderer,
        contextManager,
        m_exceptionSender,
        audioFactory->notifications(),
        customerDataManager,
        metricRecorder);
    if (!m_notificationsCapabilityAgent) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateNotificationsCapabilityAgent"));
        return false;
    }

    m_interactionCapabilityAgent = capabilityAgents::interactionModel::InteractionModelCapabilityAgent::create(
        m_directiveSequencer, m_exceptionSender);
    if (!m_interactionCapabilityAgent) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateInteractionModelCapabilityAgent"));
        return false;
    }
    // Listen to when Request Processing Started (RPS) directive is received
    // to enter the THINKING mode (Interaction Model 1.1).
    m_interactionCapabilityAgent->addObserver(m_dialogUXStateAggregator);

#ifdef ENABLE_PCC
    /*
     * Creating the PhoneCallController - This component is the Capability Agent
     * that implements the
     * PhoneCallController interface of AVS
     */
    m_phoneCallControllerCapabilityAgent = capabilityAgents::phoneCallController::PhoneCallController::create(
        contextManager, m_connectionManager, phoneCaller, phoneSpeaker, m_audioFocusManager, m_exceptionSender);
    if (!m_phoneCallControllerCapabilityAgent) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreatePhoneCallControllerCapabilityAgent"));
    }
#endif

#ifdef ENABLE_MCC
    /*
     * Creating the MeetingClientController - This component is the Capability Agent that implements the
     * MeetingClientController interface of AVS
     */
    m_meetingClientControllerCapabilityAgent =
        capabilityAgents::meetingClientController::MeetingClientController::create(
            contextManager,
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

    /*
     * Creating the ExternalMediaPlayer CA - This component is the Capability
     * Agent that implements the ExternalMediaPlayer interface of AVS.
     */
    m_externalMediaPlayer = capabilityAgents::externalMediaPlayer::ExternalMediaPlayer::create(
        externalMusicProviderMediaPlayers,
        externalMusicProviderVolumeInterfaces,
        adapterCreationMap,
        m_speakerManager,
        m_connectionManager,
        m_certifiedSender,
        m_audioFocusManager,
        contextManager,
        m_exceptionSender,
        m_playbackRouter,
        metricRecorder);
    if (!m_externalMediaPlayer) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateExternalMediaPlayer"));
        return false;
    }

    if (isGuiSupported) {
        /*
         * Creating the Visual Activity Tracker - This component is responsibly for
         * reporting the visual channel focus
         * information to AVS.
         */
        m_visualActivityTracker = afml::VisualActivityTracker::create(contextManager);

        // Read visualVirtualChannels from config file
        std::vector<afml::FocusManager::ChannelConfiguration> visualVirtualChannelConfiguration;
        if (!afml::FocusManager::ChannelConfiguration::readChannelConfiguration(
                VISUAL_CHANNEL_CONFIG_KEY, &visualVirtualChannelConfiguration)) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToReadVisualChannels"));
            return false;
        }

        /*
         * Creating the Visual Focus Manager - This component deals with the
         * management of visual focus across various
         * components. It handles granting access to Channels as well as pushing
         * different "Channels" to foreground,
         * background, or no focus based on which other Channels are active and the
         * priorities of those Channels. Each
         * Capability Agent will require the Focus Manager in order to request
         * access to the Channel it wishes to play
         * on.
         */
        m_visualFocusManager = std::make_shared<afml::FocusManager>(
            afml::FocusManager::getDefaultVisualChannels(),
            m_visualActivityTracker,
            visualVirtualChannelConfiguration,
            interruptModel);

        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderInterface>>
            renderPlayerInfoCardsProviders = {m_audioPlayer, m_externalMediaPlayer};

        /*
         * Creating the TemplateRuntime Capability Agent - This component is the
         * Capability Agent that implements the
         * TemplateRuntime interface of AVS.
         */
        m_templateRuntime = capabilityAgents::templateRuntime::TemplateRuntime::create(
            renderPlayerInfoCardsProviders, m_visualFocusManager, m_exceptionSender);
        if (!m_templateRuntime) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateTemplateRuntimeCapabilityAgent"));
            return false;
        }
        m_dialogUXStateAggregator->addObserver(m_templateRuntime);
        if (externalCapabilitiesBuilder) {
            externalCapabilitiesBuilder->withTemplateRunTime(m_templateRuntime);
        }
    }

    /*
     * Creating the Equalizer Capability Agent and related implementations if
     * enabled
     */
    m_equalizerRuntimeSetup = equalizerRuntimeSetup;
    if (nullptr != m_equalizerRuntimeSetup) {
        auto equalizerController = equalizer::EqualizerController::create(
            equalizerRuntimeSetup->getModeController(),
            equalizerRuntimeSetup->getConfiguration(),
            equalizerRuntimeSetup->getStorage());

        if (!equalizerController) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateEqualizerController"));
            return false;
        }

        m_equalizerCapabilityAgent = capabilityAgents::equalizer::EqualizerCapabilityAgent::create(
            equalizerController,
            m_capabilitiesDelegate,
            equalizerRuntimeSetup->getStorage(),
            customerDataManager,
            m_exceptionSender,
            contextManager,
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
        settingsManagerBuilder.getSetting<settings::TIMEZONE>(), m_exceptionSender);
    if (!timezoneHandler) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateTimeZoneHandler"));
        return false;
    }

    /*
     * Creating the Locale Handler - This component is responsible for handling directives related to locales.
     */
    auto localeHandler = capabilityAgents::system::LocaleHandler::create(
        m_exceptionSender, settingsManagerBuilder.getSetting<settings::DeviceSettingsIndex::LOCALE>());
    if (!localeHandler) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateLocaleHandler"));
        return false;
    }

    /*
     * Creating the ReportState Handler - This component is responsible for the ReportState directives.
     */
    auto reportGenerator = capabilityAgents::system::StateReportGenerator::create(
        m_deviceSettingsManager, settingsManagerBuilder.getConfigurations());
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
    auto systemCapabilityProvider = capabilityAgents::system::SystemCapabilityProvider::create(localeAssetsManager);
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

    if (bluetoothDeviceManager) {
        ACSDK_DEBUG5(LX(__func__).m("Creating Bluetooth CA"));

        // Create a temporary pointer to the eventBus inside of
        // bluetoothDeviceManager so that
        // the unique ptr for bluetoothDeviceManager can be moved.
        auto eventBus = bluetoothDeviceManager->getEventBus();

        auto bluetoothMediaInputTransformer =
            acsdkBluetooth::BluetoothMediaInputTransformer::create(eventBus, m_playbackRouter);

        /*
         * Creating the Bluetooth Capability Agent - This component is responsible
         * for handling directives from AVS
         * regarding bluetooth functionality.
         */
        m_bluetooth = acsdkBluetooth::Bluetooth::create(
            contextManager,
            m_audioFocusManager,
            m_connectionManager,
            m_exceptionSender,
            std::move(bluetoothStorage),
            std::move(bluetoothDeviceManager),
            std::move(eventBus),
            bluetoothMediaPlayer,
            customerDataManager,
            enabledConnectionRules,
            bluetoothChannelVolumeInterface,
            bluetoothMediaInputTransformer);
    } else {
        ACSDK_DEBUG5(LX("bluetoothCapabilityAgentDisabled").d("reason", "nullBluetoothDeviceManager"));
    }

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
        m_diagnostics->setDiagnosticDependencies(m_directiveSequencer, attachmentManager);

        /**
         * Create and initialize the DevicePropertyAggregator.
         */
        auto deviceProperties = m_diagnostics->getDevicePropertyAggregator();
        if (deviceProperties) {
            deviceProperties->setContextManager(contextManager);
            deviceProperties->initializeVolume(m_speakerManager);

            addSpeakerManagerObserver(deviceProperties);
            addAlertsObserver(deviceProperties);
            addConnectionObserver(deviceProperties);
            addNotificationsObserver(deviceProperties);
            addAudioPlayerObserver(deviceProperties);
            addAlexaDialogStateObserver(deviceProperties);
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
     * Create the AlexaInterfaceMessageSender for use by endpoint-based capability agents.
     */
    m_alexaMessageSender =
        capabilityAgents::alexa::AlexaInterfaceMessageSender::create(m_contextManager, m_connectionManager);
    if (!m_alexaMessageSender) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateAlexaMessageSender"));
        return false;
    }

    /*
     * Create the AlexaInterfaceCapabilityAgent for the default endpoint.
     */
    m_alexaCapabilityAgent = capabilityAgents::alexa::AlexaInterfaceCapabilityAgent::create(
        m_deviceInfo, m_deviceInfo.getDefaultEndpointId(), m_exceptionSender, m_alexaMessageSender);
    if (!m_alexaCapabilityAgent) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateAlexaCapabilityAgent"));
        return false;
    }

    // Add capabilitiesDelegate as an observer to EventProcessed messages.
    m_alexaCapabilityAgent->addEventProcessedObserver(m_capabilitiesDelegate);

    /**
     * Configure alexa client default endpoint.
     */
    m_defaultEndpointBuilder =
        EndpointBuilder::create(m_deviceInfo, m_contextManager, m_exceptionSender, m_alexaMessageSender);

    /*
     * Register capability agents and capability configurations.
     */
    m_defaultEndpointBuilder->withCapabilityConfiguration(m_softwareReporterCapabilityAgent);
    m_defaultEndpointBuilder->withCapability(m_speechSynthesizer, m_speechSynthesizer);
    m_defaultEndpointBuilder->withCapability(m_audioPlayer, m_audioPlayer);
    m_defaultEndpointBuilder->withCapability(m_externalMediaPlayer, m_externalMediaPlayer);
    m_defaultEndpointBuilder->withCapability(m_audioInputProcessor, m_audioInputProcessor);
    m_defaultEndpointBuilder->withCapability(m_alertsCapabilityAgent, m_alertsCapabilityAgent);
    m_defaultEndpointBuilder->withCapability(m_apiGatewayCapabilityAgent, m_apiGatewayCapabilityAgent);
    m_defaultEndpointBuilder->withCapability(
        m_alexaCapabilityAgent->getCapabilityConfiguration(), m_alexaCapabilityAgent);
    m_defaultEndpointBuilder->withCapabilityConfiguration(m_audioActivityTracker);
    m_defaultEndpointBuilder->withCapabilityConfiguration(m_playbackController);
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

    m_defaultEndpointBuilder->withCapability(m_speakerManager, m_speakerManager);

    if (isGuiSupported) {
        m_defaultEndpointBuilder->withCapability(m_templateRuntime, m_templateRuntime);
        m_defaultEndpointBuilder->withCapabilityConfiguration(m_visualActivityTracker);
    }

    m_defaultEndpointBuilder->withCapability(m_notificationsCapabilityAgent, m_notificationsCapabilityAgent);
    m_defaultEndpointBuilder->withCapability(m_interactionCapabilityAgent, m_interactionCapabilityAgent);

    if (m_bluetooth) {
        m_defaultEndpointBuilder->withCapability(m_bluetooth, m_bluetooth);
    }

    if (m_equalizerCapabilityAgent) {
        m_defaultEndpointBuilder->withCapability(m_equalizerCapabilityAgent, m_equalizerCapabilityAgent);
    }

    m_defaultEndpointBuilder->withCapability(m_dndCapabilityAgent, m_dndCapabilityAgent);

    // System CA is split into multiple directive handlers.
    m_defaultEndpointBuilder->withCapabilityConfiguration(systemCapabilityProvider);
    if (!m_directiveSequencer->addDirectiveHandler(std::move(localeHandler)) ||
        !m_directiveSequencer->addDirectiveHandler(std::move(timezoneHandler)) ||
        !m_directiveSequencer->addDirectiveHandler(reportStateHandler) ||
#ifdef ENABLE_REVOKE_AUTH
        !m_directiveSequencer->addDirectiveHandler(m_revokeAuthorizationHandler) ||
#endif
        !m_directiveSequencer->addDirectiveHandler(m_userInactivityMonitor)) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToRegisterSystemDirectiveHandler"));
        return false;
    }

    if (externalCapabilitiesBuilder) {
        externalCapabilitiesBuilder->withSettingsStorage(m_deviceSettingStorage);
        externalCapabilitiesBuilder->withInternetConnectionMonitor(m_internetConnectionMonitor);
        externalCapabilitiesBuilder->withDialogUXStateAggregator(m_dialogUXStateAggregator);
        if (isGuiSupported) {
            externalCapabilitiesBuilder->withVisualFocusManager(m_visualFocusManager);
        }
        auto externalCapabilities = externalCapabilitiesBuilder->buildCapabilities(
            m_externalMediaPlayer,
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
            m_userInactivityMonitor,
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
            powerResourceManager);
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

    if (!m_defaultEndpointBuilder->finishDefaultEndpointConfiguration()) {
        ACSDK_ERROR(LX("initializedFailed").d("reason", "defaultEndpointConfigurationFailed"));
        return false;
    }

    return true;
}

void DefaultClient::connect(bool performReset) {
    if (performReset) {
        if (m_defaultEndpointBuilder) {
            // Build default endpoint.
            auto defaultEndpoint = m_defaultEndpointBuilder->buildDefaultEndpoint();
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
    std::shared_ptr<avsCommon::sdkInterfaces::TemplateRuntimeObserverInterface> observer) {
    if (!m_templateRuntime) {
        ACSDK_ERROR(LX("addTemplateRuntimeObserverFailed").d("reason", "guiNotSupported"));
        return;
    }
    m_templateRuntime->addObserver(observer);
}

void DefaultClient::removeTemplateRuntimeObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::TemplateRuntimeObserverInterface> observer) {
    if (!m_templateRuntime) {
        ACSDK_ERROR(LX("removeTemplateRuntimeObserverFailed").d("reason", "guiNotSupported"));
        return;
    }
    m_templateRuntime->removeObserver(observer);
}

void DefaultClient::TemplateRuntimeDisplayCardCleared() {
    if (!m_templateRuntime) {
        ACSDK_ERROR(LX("TemplateRuntimeDisplayCardClearedFailed").d("reason", "guiNotSupported"));
        return;
    }
    m_templateRuntime->displayCardCleared();
}

void DefaultClient::addNotificationsObserver(
    std::shared_ptr<acsdkNotificationsInterfaces::NotificationsObserverInterface> observer) {
    m_notificationsCapabilityAgent->addObserver(observer);
}

void DefaultClient::removeNotificationsObserver(
    std::shared_ptr<acsdkNotificationsInterfaces::NotificationsObserverInterface> observer) {
    m_notificationsCapabilityAgent->removeObserver(observer);
}

void DefaultClient::addExternalMediaPlayerObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::externalMediaPlayer::ExternalMediaPlayerObserverInterface> observer) {
    m_externalMediaPlayer->addObserver(observer);
}

void DefaultClient::removeExternalMediaPlayerObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::externalMediaPlayer::ExternalMediaPlayerObserverInterface> observer) {
    m_externalMediaPlayer->removeObserver(observer);
}

void DefaultClient::addCaptionPresenter(std::shared_ptr<captions::CaptionPresenterInterface> presenter) {
#ifdef ENABLE_CAPTIONS
    if (m_captionManager) {
        m_captionManager->setCaptionPresenter(presenter);
    }
#endif
}

void DefaultClient::setCaptionMediaPlayers(
    const std::vector<std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>>& mediaPlayers) {
#ifdef ENABLE_CAPTIONS
    if (m_captionManager) {
        m_captionManager->setMediaPlayers(mediaPlayers);
    }
#endif
}

void DefaultClient::addBluetoothDeviceObserver(
    std::shared_ptr<acsdkBluetoothInterfaces::BluetoothDeviceObserverInterface> observer) {
    if (!m_bluetooth) {
        ACSDK_DEBUG5(LX(__func__).m("bluetooth is disabled, not adding observer"));
        return;
    }
    m_bluetooth->addObserver(observer);
}

void DefaultClient::removeBluetoothDeviceObserver(
    std::shared_ptr<acsdkBluetoothInterfaces::BluetoothDeviceObserverInterface> observer) {
    if (!m_bluetooth) {
        return;
    }
    m_bluetooth->removeObserver(observer);
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

std::shared_ptr<registrationManager::RegistrationManager> DefaultClient::getRegistrationManager() {
    return m_registrationManager;
}

std::shared_ptr<equalizer::EqualizerController> DefaultClient::getEqualizerController() {
    return m_equalizerController;
}

void DefaultClient::addEqualizerControllerListener(
    std::shared_ptr<avsCommon::sdkInterfaces::audio::EqualizerControllerListenerInterface> listener) {
    if (m_equalizerController) {
        m_equalizerController->addListener(listener);
    }
}

void DefaultClient::removeEqualizerControllerListener(
    std::shared_ptr<avsCommon::sdkInterfaces::audio::EqualizerControllerListenerInterface> listener) {
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
    std::chrono::steady_clock::time_point startOfSpeechTimestamp) {
    ACSDK_DEBUG5(LX(__func__));
    return m_audioInputProcessor->recognize(
        holdToTalkAudioProvider, capabilityAgents::aip::Initiator::PRESS_AND_HOLD, startOfSpeechTimestamp);
}

std::future<bool> DefaultClient::notifyOfHoldToTalkEnd() {
    return m_audioInputProcessor->stopCapture();
}

std::future<bool> DefaultClient::notifyOfTapToTalkEnd() {
    return m_audioInputProcessor->stopCapture();
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

std::unique_ptr<EndpointBuilderInterface> DefaultClient::createEndpointBuilder() {
    return EndpointBuilder::create(m_deviceInfo, m_contextManager, m_exceptionSender, m_alexaMessageSender);
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

void DefaultClient::onSystemClockSynchronized() {
    m_systemClockMonitor->notifySystemClockSynchronized();
}

DefaultClient::~DefaultClient() {
    while (!m_shutdownObjects.empty()) {
        if (m_shutdownObjects.back()) {
            m_shutdownObjects.back()->shutdown();
        }
        m_shutdownObjects.pop_back();
    }

    if (m_directiveSequencer) {
        ACSDK_DEBUG5(LX("DirectiveSequencerShutdown"));
        m_directiveSequencer->shutdown();
    }
    if (m_speakerManager) {
        ACSDK_DEBUG5(LX("SpeakerManagerShutdown"));
        m_speakerManager->shutdown();
    }
    if (m_templateRuntime) {
        ACSDK_DEBUG5(LX("TemplateRuntimeShutdown"));
        m_templateRuntime->shutdown();
    }
    if (m_audioInputProcessor) {
        ACSDK_DEBUG5(LX("AIPShutdown"));
        removeInternetConnectionObserver(m_audioInputProcessor);
        m_audioInputProcessor->shutdown();
    }
    if (m_audioPlayer) {
        ACSDK_DEBUG5(LX("AudioPlayerShutdown"));
        m_audioPlayer->shutdown();
    }
    if (m_externalMediaPlayer) {
        ACSDK_DEBUG5(LX("ExternalMediaPlayerShutdown"));
        m_externalMediaPlayer->shutdown();
    }
    if (m_speechSynthesizer) {
        ACSDK_DEBUG5(LX("SpeechSynthesizerShutdown"));
        m_speechSynthesizer->shutdown();
    }
    if (m_alertsCapabilityAgent) {
        m_systemClockMonitor->removeSystemClockMonitorObserver(m_alertsCapabilityAgent);
        ACSDK_DEBUG5(LX("AlertsShutdown"));
        m_alertsCapabilityAgent->shutdown();
    }
    if (m_playbackController) {
        ACSDK_DEBUG5(LX("PlaybackControllerShutdown"));
        m_playbackController->shutdown();
    }
    if (m_softwareInfoSender) {
        ACSDK_DEBUG5(LX("SoftwareInfoShutdown"));
        m_softwareInfoSender->shutdown();
    }
    if (m_messageRouter) {
        ACSDK_DEBUG5(LX("MessageRouterShutdown."));
        m_messageRouter->shutdown();
    }
    if (m_connectionManager) {
        ACSDK_DEBUG5(LX("ConnectionManagerShutdown."));
        m_connectionManager->shutdown();
    }
    if (m_certifiedSender) {
        ACSDK_DEBUG5(LX("CertifiedSenderShutdown."));
        m_certifiedSender->shutdown();
    }
    if (m_audioActivityTracker) {
        ACSDK_DEBUG5(LX("AudioActivityTrackerShutdown."));
        m_audioActivityTracker->shutdown();
    }
    if (m_visualActivityTracker) {
        ACSDK_DEBUG5(LX("VisualActivityTrackerShutdown."));
        m_visualActivityTracker->shutdown();
    }
    if (m_playbackRouter) {
        ACSDK_DEBUG5(LX("PlaybackRouterShutdown."));
        m_playbackRouter->shutdown();
    }
    if (m_notificationsCapabilityAgent) {
        ACSDK_DEBUG5(LX("NotificationsShutdown."));
        m_notificationsCapabilityAgent->shutdown();
    }
    if (m_notificationsRenderer) {
        ACSDK_DEBUG5(LX("NotificationsRendererShutdown."));
        m_notificationsRenderer->shutdown();
    }
#ifdef ENABLE_CAPTIONS
    if (m_captionManager) {
        ACSDK_DEBUG5(LX("CaptionManagerShutdown."));
        m_captionManager->shutdown();
    }
#endif
    if (m_bluetooth) {
        ACSDK_DEBUG5(LX("BluetoothShutdown."));
        m_bluetooth->shutdown();
    }

    if (m_userInactivityMonitor) {
        ACSDK_DEBUG5(LX("UserInactivityMonitorShutdown."));
        m_userInactivityMonitor->shutdown();
    }

    if (m_apiGatewayCapabilityAgent) {
        ACSDK_DEBUG5(LX("CallApiGatewayCapabilityAgentShutdown."));
        m_apiGatewayCapabilityAgent->shutdown();
    }

    if (m_alexaCapabilityAgent) {
        m_alexaCapabilityAgent->removeEventProcessedObserver(m_capabilitiesDelegate);
    }

    if (m_alexaMessageSender) {
        ACSDK_DEBUG5(LX("CallAlexaInterfaceMessageSenderShutdown."));
        m_alexaMessageSender->shutdown();
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
    if (m_dndCapabilityAgent) {
        ACSDK_DEBUG5(LX("DNDCapabilityAgentShutdown"));
        removeConnectionObserver(m_dndCapabilityAgent);
        m_dndCapabilityAgent->shutdown();
    }
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

    if (m_deviceSettingStorage) {
        ACSDK_DEBUG5(LX("CloseSettingStorage"));
        m_deviceSettingStorage->close();
    }

    if (m_diagnostics) {
        m_diagnostics->setDiagnosticDependencies(nullptr, nullptr);

        auto deviceProperties = m_diagnostics->getDevicePropertyAggregator();
        if (deviceProperties) {
            deviceProperties->setContextManager(nullptr);

            removeSpeakerManagerObserver(deviceProperties);
            removeAlertsObserver(deviceProperties);
            removeConnectionObserver(deviceProperties);
            removeNotificationsObserver(deviceProperties);
            removeAudioPlayerObserver(deviceProperties);
            removeAlexaDialogStateObserver(deviceProperties);
        }

        auto protocolTrace = m_diagnostics->getProtocolTracer();
        if (protocolTrace) {
            removeMessageObserver(protocolTrace);
        }
    }
}

}  // namespace defaultClient
}  // namespace alexaClientSDK
