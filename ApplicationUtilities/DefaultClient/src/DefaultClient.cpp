/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "DefaultClient/DefaultClient.h"

#include <ACL/Transport/HTTP2MessageRouter.h>
#include <ACL/Transport/PostConnectObject.h>
#include <ADSL/MessageInterpreter.h>
#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/AVS/ExceptionEncounteredSender.h>
#include <ContextManager/ContextManager.h>
#include <Settings/SettingsUpdatedEventSender.h>
#include <System/EndpointHandler.h>
#include <System/UserInactivityMonitor.h>

namespace alexaClientSDK {
namespace defaultClient {

/// String to identify log entries originating from this file.
static const std::string TAG("DefaultClient");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<DefaultClient> DefaultClient::create(
    const std::unordered_map<std::string, std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>>&
        externalMusicProviderMediaPlayers,
    const capabilityAgents::externalMediaPlayer::ExternalMediaPlayer::AdapterCreationMap& adapterCreationMap,
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> speakMediaPlayer,
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> audioMediaPlayer,
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> alertsMediaPlayer,
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> notificationsMediaPlayer,
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> speakSpeaker,
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> audioSpeaker,
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> alertsSpeaker,
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> notificationsSpeaker,
    const std::vector<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>& additionalSpeakers,
    std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface> audioFactory,
    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
    std::shared_ptr<capabilityAgents::alerts::storage::AlertStorageInterface> alertStorage,
    std::shared_ptr<certifiedSender::MessageStorageInterface> messageStorage,
    std::shared_ptr<capabilityAgents::notifications::NotificationsStorageInterface> notificationsStorage,
    std::shared_ptr<capabilityAgents::settings::SettingsStorageInterface> settingsStorage,
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface>>
        alexaDialogStateObservers,
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>>
        connectionObservers,
    bool isGuiSupported,
    avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion,
    bool sendSoftwareInfoOnConnected,
    std::shared_ptr<avsCommon::sdkInterfaces::SoftwareInfoSenderObserverInterface> softwareInfoSenderObserver) {
    std::unique_ptr<DefaultClient> defaultClient(new DefaultClient());
    if (!defaultClient->initialize(
            externalMusicProviderMediaPlayers,
            adapterCreationMap,
            speakMediaPlayer,
            audioMediaPlayer,
            alertsMediaPlayer,
            notificationsMediaPlayer,
            speakSpeaker,
            audioSpeaker,
            alertsSpeaker,
            notificationsSpeaker,
            additionalSpeakers,
            audioFactory,
            authDelegate,
            alertStorage,
            messageStorage,
            notificationsStorage,
            settingsStorage,
            alexaDialogStateObservers,
            connectionObservers,
            isGuiSupported,
            firmwareVersion,
            sendSoftwareInfoOnConnected,
            softwareInfoSenderObserver)) {
        return nullptr;
    }

    return defaultClient;
}

bool DefaultClient::initialize(
    const std::unordered_map<std::string, std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>>&
        externalMusicProviderMediaPlayers,
    const capabilityAgents::externalMediaPlayer::ExternalMediaPlayer::AdapterCreationMap& adapterCreationMap,
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> speakMediaPlayer,
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> audioMediaPlayer,
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> alertsMediaPlayer,
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> notificationsMediaPlayer,
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> speakSpeaker,
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> audioSpeaker,
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> alertsSpeaker,
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> notificationsSpeaker,
    const std::vector<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>& additionalSpeakers,
    std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface> audioFactory,
    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
    std::shared_ptr<capabilityAgents::alerts::storage::AlertStorageInterface> alertStorage,
    std::shared_ptr<certifiedSender::MessageStorageInterface> messageStorage,
    std::shared_ptr<capabilityAgents::notifications::NotificationsStorageInterface> notificationsStorage,
    std::shared_ptr<capabilityAgents::settings::SettingsStorageInterface> settingsStorage,
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface>>
        alexaDialogStateObservers,
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>>
        connectionObservers,
    bool isGuiSupported,
    avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion,
    bool sendSoftwareInfoOnConnected,
    std::shared_ptr<avsCommon::sdkInterfaces::SoftwareInfoSenderObserverInterface> softwareInfoSenderObserver) {
    if (!audioFactory) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullAudioFactory"));
        return false;
    }

    if (!speakMediaPlayer) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullSpeakMediaPlayer"));
        return false;
    }

    if (!audioMediaPlayer) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullAudioMediaPlayer"));
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

    if (!authDelegate) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullAuthDelegate"));
        return false;
    }

    m_dialogUXStateAggregator = std::make_shared<avsCommon::avs::DialogUXStateAggregator>();

    for (auto observer : alexaDialogStateObservers) {
        m_dialogUXStateAggregator->addObserver(observer);
    }

    /*
     * Creating customerDataManager which will be used by the registrationManager and all classes that extend
     * CustomerDataHandler
     */
    auto customerDataManager = std::make_shared<registrationManager::CustomerDataManager>();

    /*
     * Creating the Attachment Manager - This component deals with managing attachments and allows for readers and
     * writers to be created to handle the attachment.
     */
    auto attachmentManager = std::make_shared<avsCommon::avs::attachment::AttachmentManager>(
        avsCommon::avs::attachment::AttachmentManager::AttachmentType::IN_PROCESS);

    /*
     * Creating the message router - This component actually maintains the connection to AVS over HTTP2. It is created
     * using the auth delegate, which provides authorization to connect to AVS, and the attachment manager, which helps
     * ACL write attachments received from AVS.
     */
    m_messageRouter = std::make_shared<acl::HTTP2MessageRouter>(authDelegate, attachmentManager);

    /*
     * Creating the connection manager - This component is the overarching connection manager that glues together all
     * the other networking components into one easy-to-use component.
     */
    m_connectionManager =
        acl::AVSConnectionManager::create(m_messageRouter, false, connectionObservers, {m_dialogUXStateAggregator});
    if (!m_connectionManager) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateConnectionManager"));
        return false;
    }

    /*
     * Creating our certified sender - this component guarantees that messages given to it (expected to be JSON
     * formatted AVS Events) will be sent to AVS.  This nicely decouples strict message sending from components which
     * require an Event be sent, even in conditions when there is no active AVS connection.
     */
    m_certifiedSender = certifiedSender::CertifiedSender::create(
        m_connectionManager, m_connectionManager, messageStorage, customerDataManager);
    if (!m_certifiedSender) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateCertifiedSender"));
        return false;
    }

    /*
     * Creating the Exception Sender - This component helps the SDK send exceptions when it is unable to handle a
     * directive sent by AVS. For that reason, the Directive Sequencer and each Capability Agent will need this
     * component.
     */
    m_exceptionSender = avsCommon::avs::ExceptionEncounteredSender::create(m_connectionManager);
    if (!m_exceptionSender) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateExceptionSender"));
        return false;
    }

    /*
     * Creating the Directive Sequencer - This is the component that deals with the sequencing and ordering of
     * directives sent from AVS and forwarding them along to the appropriate Capability Agent that deals with
     * directives in that Namespace/Name.
     */
    m_directiveSequencer = adsl::DirectiveSequencer::create(m_exceptionSender);
    if (!m_directiveSequencer) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateDirectiveSequencer"));
        return false;
    }

    /*
     * Creating the Message Interpreter - This component takes care of converting ACL messages to Directives for the
     * Directive Sequencer to process. This essentially "glues" together the ACL and ADSL.
     */
    auto messageInterpreter =
        std::make_shared<adsl::MessageInterpreter>(m_exceptionSender, m_directiveSequencer, attachmentManager);

    m_connectionManager->addMessageObserver(messageInterpreter);

    /*
     * Creating the Registration Manager - This component is responsible for implementing any customer registration
     * operation such as login and logout
     */
    m_registrationManager = std::make_shared<registrationManager::RegistrationManager>(
        m_directiveSequencer, m_connectionManager, customerDataManager);

    /*
     * Creating the Context Manager - This component manages the context of each of the components to update to AVS.
     * It is required for each of the capability agents so that they may provide their state just before any event is
     * fired off.
     */
    auto contextManager = contextManager::ContextManager::create();
    if (!contextManager) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateContextManager"));
        return false;
    }
    acl::PostConnectObject::init(contextManager);

    /*
     * Creating the Audio Activity Tracker - This component is responsibly for reporting the audio channel focus
     * information to AVS.
     */
    m_audioActivityTracker = afml::AudioActivityTracker::create(contextManager);

    /*
     * Creating the Focus Manager - This component deals with the management of layered audio focus across various
     * components. It handles granting access to Channels as well as pushing different "Channels" to foreground,
     * background, or no focus based on which other Channels are active and the priorities of those Channels. Each
     * Capability Agent will require the Focus Manager in order to request access to the Channel it wishes to play on.
     */
    m_audioFocusManager =
        std::make_shared<afml::FocusManager>(afml::FocusManager::DEFAULT_AUDIO_CHANNELS, m_audioActivityTracker);

    /*
     * Creating the User Inactivity Monitor - This component is responsibly for updating AVS of user inactivity as
     * described in the System Interface of AVS.
     */
    auto userInactivityMonitor =
        capabilityAgents::system::UserInactivityMonitor::create(m_connectionManager, m_exceptionSender);
    if (!userInactivityMonitor) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateUserInactivityMonitor"));
        return false;
    }

    /*
     * Creating the Audio Input Processor - This component is the Capability Agent that implments the SpeechRecognizer
     * interface of AVS.
     */
    m_audioInputProcessor = capabilityAgents::aip::AudioInputProcessor::create(
        m_directiveSequencer,
        m_connectionManager,
        contextManager,
        m_audioFocusManager,
        m_dialogUXStateAggregator,
        m_exceptionSender,
        userInactivityMonitor);
    if (!m_audioInputProcessor) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateAudioInputProcessor"));
        return false;
    }

    m_audioInputProcessor->addObserver(m_dialogUXStateAggregator);

    /*
     * Creating the Speech Synthesizer - This component is the Capability Agent that implements the SpeechSynthesizer
     * interface of AVS.
     */
    m_speechSynthesizer = capabilityAgents::speechSynthesizer::SpeechSynthesizer::create(
        speakMediaPlayer,
        m_connectionManager,
        m_audioFocusManager,
        contextManager,
        m_exceptionSender,
        m_dialogUXStateAggregator);
    if (!m_speechSynthesizer) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateSpeechSynthesizer"));
        return false;
    }

    m_speechSynthesizer->addObserver(m_dialogUXStateAggregator);

    /*
     * Creating the PlaybackController Capability Agent - This component is the Capability Agent that implements the
     * PlaybackController interface of AVS.
     */
    m_playbackController =
        capabilityAgents::playbackController::PlaybackController::create(contextManager, m_connectionManager);
    if (!m_playbackController) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreatePlaybackController"));
        return false;
    }

    /*
     * Creating the PlaybackRouter - This component routes a playback button press to the active handler.
     * The default handler is @c PlaybackController.
     */
    m_playbackRouter = capabilityAgents::playbackController::PlaybackRouter::create(m_playbackController);
    if (!m_playbackRouter) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreatePlaybackRouter"));
        return false;
    }

    /*
     * Creating the Audio Player - This component is the Capability Agent that implements the AudioPlayer
     * interface of AVS.
     */
    m_audioPlayer = capabilityAgents::audioPlayer::AudioPlayer::create(
        audioMediaPlayer,
        m_connectionManager,
        m_audioFocusManager,
        contextManager,
        m_exceptionSender,
        m_playbackRouter);
    if (!m_audioPlayer) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateAudioPlayer"));
        return false;
    }

    /*
     * Creating the Alerts Capability Agent - This component is the Capability Agent that implements the Alerts
     * interface of AVS.
     */
    m_alertsCapabilityAgent = capabilityAgents::alerts::AlertsCapabilityAgent::create(
        m_connectionManager,
        m_certifiedSender,
        m_audioFocusManager,
        contextManager,
        m_exceptionSender,
        alertStorage,
        audioFactory->alerts(),
        capabilityAgents::alerts::renderer::Renderer::create(alertsMediaPlayer),
        customerDataManager);
    if (!m_alertsCapabilityAgent) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateAlertsCapabilityAgent"));
        return false;
    }

    addConnectionObserver(m_alertsCapabilityAgent);

    addConnectionObserver(m_dialogUXStateAggregator);

    /*
     * Creating the Notifications Capability Agent - This component is the Capability Agent that implements the
     * Notifications interface of AVS.
     */
    m_notificationsCapabilityAgent = capabilityAgents::notifications::NotificationsCapabilityAgent::create(
        notificationsStorage,
        capabilityAgents::notifications::NotificationRenderer::create(notificationsMediaPlayer),
        contextManager,
        m_exceptionSender,
        audioFactory->notifications(),
        customerDataManager);
    if (!m_notificationsCapabilityAgent) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateNotificationsCapabilityAgent"));
        return false;
    }

    std::shared_ptr<capabilityAgents::settings::SettingsUpdatedEventSender> settingsUpdatedEventSender =
        alexaClientSDK::capabilityAgents::settings::SettingsUpdatedEventSender::create(m_connectionManager);
    if (!settingsUpdatedEventSender) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateSettingsObserver"));
        return false;
    }

    /*
     * Creating the Setting object - This component implements the Setting interface of AVS.
     */
    m_settings = capabilityAgents::settings::Settings::create(
        settingsStorage, {settingsUpdatedEventSender}, customerDataManager);

    if (!m_settings) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateSettingsObject"));
        return false;
    }

    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>> allSpeakers = {
        speakSpeaker, audioSpeaker, alertsSpeaker, notificationsSpeaker};
    allSpeakers.insert(allSpeakers.end(), additionalSpeakers.begin(), additionalSpeakers.end());

    /*
     * Creating the SpeakerManager Capability Agent - This component is the Capability Agent that implements the
     * Speaker interface of AVS.
     */
    m_speakerManager = capabilityAgents::speakerManager::SpeakerManager::create(
        allSpeakers, contextManager, m_connectionManager, m_exceptionSender);
    if (!m_speakerManager) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateSpeakerManager"));
        return false;
    }

    /*
     * Creating the ExternalMediaPlayer CA - This component is the Capability Agent that implements the
     * ExternalMediaPlayer interface of AVS.
     */
    m_externalMediaPlayer = capabilityAgents::externalMediaPlayer::ExternalMediaPlayer::create(
        externalMusicProviderMediaPlayers,
        adapterCreationMap,
        m_speakerManager,
        m_connectionManager,
        m_audioFocusManager,
        contextManager,
        m_exceptionSender,
        m_playbackRouter);
    if (!m_externalMediaPlayer) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateExternalMediaPlayer"));
        return false;
    }

    m_speakerManager->addSpeaker(m_externalMediaPlayer);

    if (isGuiSupported) {
        /*
         * Creating the Visual Activity Tracker - This component is responsibly for reporting the visual channel focus
         * information to AVS.
         */
        m_visualActivityTracker = afml::VisualActivityTracker::create(contextManager);

        /*
         * Creating the Visual Focus Manager - This component deals with the management of visual focus across various
         * components. It handles granting access to Channels as well as pushing different "Channels" to foreground,
         * background, or no focus based on which other Channels are active and the priorities of those Channels. Each
         * Capability Agent will require the Focus Manager in order to request access to the Channel it wishes to play
         * on.
         */
        m_visualFocusManager =
            std::make_shared<afml::FocusManager>(afml::FocusManager::DEFAULT_VISUAL_CHANNELS, m_visualActivityTracker);

        /*
         * Creating the TemplateRuntime Capability Agent - This component is the Capability Agent that implements the
         * TemplateRuntime interface of AVS.
         */
        m_templateRuntime = capabilityAgents::templateRuntime::TemplateRuntime::create(
            m_audioPlayer, m_visualFocusManager, m_exceptionSender);
        if (!m_templateRuntime) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateTemplateRuntimeCapabilityAgent"));
            return false;
        }
        m_dialogUXStateAggregator->addObserver(m_templateRuntime);
    }

    /*
     * Creating the Endpoint Handler - This component is responsible for handling directives from AVS instructing the
     * client to change the endpoint to connect to.
     */
    auto endpointHandler = capabilityAgents::system::EndpointHandler::create(m_connectionManager, m_exceptionSender);
    if (!endpointHandler) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateEndpointHandler"));
        return false;
    }

    if (avsCommon::sdkInterfaces::softwareInfo::isValidFirmwareVersion(firmwareVersion)) {
        auto tempSender = capabilityAgents::system::SoftwareInfoSender::create(
            firmwareVersion,
            sendSoftwareInfoOnConnected,
            softwareInfoSenderObserver,
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

    /*
     * The following two statements show how to register capability agents to the directive sequencer.
     */
    if (!m_directiveSequencer->addDirectiveHandler(m_speechSynthesizer)) {
        ACSDK_ERROR(LX("initializeFailed")
                        .d("reason", "unableToRegisterDirectiveHandler")
                        .d("directiveHandler", "SpeechSynthesizer"));
        return false;
    }

    if (!m_directiveSequencer->addDirectiveHandler(m_audioPlayer)) {
        ACSDK_ERROR(LX("initializeFailed")
                        .d("reason", "unableToRegisterDirectiveHandler")
                        .d("directiveHandler", "AudioPlayer"));
        return false;
    }

    if (!m_directiveSequencer->addDirectiveHandler(m_externalMediaPlayer)) {
        ACSDK_ERROR(LX("initializeFailed")
                        .d("reason", "unableToRegisterDirectiveHandler")
                        .d("directiveHandler", "ExternalMediaPlayer"));
        return false;
    }

    if (!m_directiveSequencer->addDirectiveHandler(m_audioInputProcessor)) {
        ACSDK_ERROR(LX("initializeFailed")
                        .d("reason", "unableToRegisterDirectiveHandler")
                        .d("directiveHandler", "AudioInputProcessor"));
        return false;
    }

    if (!m_directiveSequencer->addDirectiveHandler(m_alertsCapabilityAgent)) {
        ACSDK_ERROR(LX("initializeFailed")
                        .d("reason", "unableToRegisterDirectiveHandler")
                        .d("directiveHandler", "AlertsCapabilityAgent"));
        return false;
    }

    if (!m_directiveSequencer->addDirectiveHandler(endpointHandler)) {
        ACSDK_ERROR(LX("initializeFailed")
                        .d("reason", "unableToRegisterDirectiveHandler")
                        .d("directiveHandler", "EndpointHandler"));
        return false;
    }

    if (!m_directiveSequencer->addDirectiveHandler(userInactivityMonitor)) {
        ACSDK_ERROR(LX("initializeFailed")
                        .d("reason", "unableToRegisterDirectiveHandler")
                        .d("directiveHandler", "UserInactivityMonitor"));
        return false;
    }

    if (!m_directiveSequencer->addDirectiveHandler(m_speakerManager)) {
        ACSDK_ERROR(LX("initializeFailed")
                        .d("reason", "unableToRegisterDirectiveHandler")
                        .d("directiveHandler", "SpeakerManager"));
        return false;
    }

    if (isGuiSupported) {
        if (!m_directiveSequencer->addDirectiveHandler(m_templateRuntime)) {
            ACSDK_ERROR(LX("initializeFailed")
                            .d("reason", "unableToRegisterDirectiveHandler")
                            .d("directiveHandler", "TemplateRuntime"));
            return false;
        }
    }

    if (!m_directiveSequencer->addDirectiveHandler(m_notificationsCapabilityAgent)) {
        ACSDK_ERROR(LX("initializeFailed")
                        .d("reason", "unableToRegisterDirectiveHandler")
                        .d("directiveHandler", "NotificationsCapabilityAgent"));
        return false;
    }
    return true;
}

void DefaultClient::connect(const std::string& avsEndpoint) {
    if (!avsEndpoint.empty()) {
        m_connectionManager->setAVSEndpoint(avsEndpoint);
    }
    m_connectionManager->enable();
}

void DefaultClient::disconnect() {
    m_connectionManager->disable();
}

void DefaultClient::stopForegroundActivity() {
    m_audioFocusManager->stopForegroundActivity();
}

void DefaultClient::addAlexaDialogStateObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface> observer) {
    m_dialogUXStateAggregator->addObserver(observer);
}

void DefaultClient::removeAlexaDialogStateObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface> observer) {
    m_dialogUXStateAggregator->removeObserver(observer);
}

void DefaultClient::addConnectionObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> observer) {
    m_connectionManager->addConnectionStatusObserver(observer);
}

void DefaultClient::removeConnectionObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> observer) {
    m_connectionManager->removeConnectionStatusObserver(observer);
}

void DefaultClient::addAlertsObserver(std::shared_ptr<capabilityAgents::alerts::AlertObserverInterface> observer) {
    m_alertsCapabilityAgent->addObserver(observer);
}

void DefaultClient::removeAlertsObserver(std::shared_ptr<capabilityAgents::alerts::AlertObserverInterface> observer) {
    m_alertsCapabilityAgent->removeObserver(observer);
}

void DefaultClient::addAudioPlayerObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::AudioPlayerObserverInterface> observer) {
    m_audioPlayer->addObserver(observer);
}

void DefaultClient::removeAudioPlayerObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::AudioPlayerObserverInterface> observer) {
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

void DefaultClient::addSettingObserver(
    const std::string& key,
    std::shared_ptr<avsCommon::sdkInterfaces::SingleSettingObserverInterface> observer) {
    m_settings->addSingleSettingObserver(key, observer);
}

void DefaultClient::removeSettingObserver(
    const std::string& key,
    std::shared_ptr<avsCommon::sdkInterfaces::SingleSettingObserverInterface> observer) {
    m_settings->removeSingleSettingObserver(key, observer);
}

void DefaultClient::addNotificationsObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::NotificationsObserverInterface> observer) {
    m_notificationsCapabilityAgent->addObserver(observer);
}

void DefaultClient::removeNotificationsObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::NotificationsObserverInterface> observer) {
    m_notificationsCapabilityAgent->removeObserver(observer);
}

void DefaultClient::changeSetting(const std::string& key, const std::string& value) {
    m_settings->changeSetting(key, value);
}

void DefaultClient::sendDefaultSettings() {
    m_settings->sendDefaultSettings();
}

std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> DefaultClient::getPlaybackRouter() const {
    return m_playbackRouter;
}

std::shared_ptr<registrationManager::RegistrationManager> DefaultClient::getRegistrationManager() {
    return m_registrationManager;
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

bool DefaultClient::setFirmwareVersion(avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion) {
    {
        std::lock_guard<std::mutex> lock(m_softwareInfoSenderMutex);

        if (!m_softwareInfoSender) {
            m_softwareInfoSender = capabilityAgents::system::SoftwareInfoSender::create(
                firmwareVersion, true, nullptr, m_connectionManager, m_connectionManager, m_exceptionSender);
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
    const capabilityAgents::aip::ESPData& espData) {
    return m_audioInputProcessor->recognize(
        wakeWordAudioProvider, capabilityAgents::aip::Initiator::WAKEWORD, beginIndex, endIndex, keyword, espData);
}

std::future<bool> DefaultClient::notifyOfTapToTalk(
    capabilityAgents::aip::AudioProvider tapToTalkAudioProvider,
    avsCommon::avs::AudioInputStream::Index beginIndex) {
    return m_audioInputProcessor->recognize(tapToTalkAudioProvider, capabilityAgents::aip::Initiator::TAP, beginIndex);
}

std::future<bool> DefaultClient::notifyOfHoldToTalkStart(capabilityAgents::aip::AudioProvider holdToTalkAudioProvider) {
    return m_audioInputProcessor->recognize(holdToTalkAudioProvider, capabilityAgents::aip::Initiator::PRESS_AND_HOLD);
}

std::future<bool> DefaultClient::notifyOfHoldToTalkEnd() {
    return m_audioInputProcessor->stopCapture();
}

std::future<bool> DefaultClient::notifyOfTapToTalkEnd() {
    return m_audioInputProcessor->stopCapture();
}

DefaultClient::~DefaultClient() {
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
}

}  // namespace defaultClient
}  // namespace alexaClientSDK
