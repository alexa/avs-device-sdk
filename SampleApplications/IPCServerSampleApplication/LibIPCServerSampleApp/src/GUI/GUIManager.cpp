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

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <acsdk/AlexaPresentationFeatureClient/AlexaPresentationFeatureClient.h>
#include <acsdk/PresentationOrchestratorFeatureClient/PresentationOrchestratorFeatureClient.h>
#include <acsdk/VisualCharacteristicsFeatureClient/VisualCharacteristicsFeatureClient.h>
#include <acsdk/VisualStateTrackerFeatureClient/VisualStateTrackerFeatureClient.h>
#include <AVSCommon/AVS/PlaybackButtons.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <Settings/SettingObserverInterface.h>

#include "IPCServerSampleApp/GUI/GUIManager.h"

#ifdef UWP_BUILD
#include <UWPSampleApp/Utils.h>
#include <SSSDKCommon/AudioFileUtil.h>
#endif

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace gui {

using namespace alexaClientSDK;
using namespace avsCommon::utils::json;
using namespace avsCommon::avs;

/// String to identify log entries originating from this file.
static const std::string TAG("GUIManager");

/**
 * Create a LogEntry using this file's TAG and the specified event string.x1
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

/// Interface name to use for focus requests.
static const std::string APL_INTERFACE("Alexa.Presentation.APL");

/// String to identify the Shuffle Toggle of PlaybackController.
static const std::string SHUFFLE_TOGGLE_ID("shuffle");

/// String to identify the Loop Toggle of PlaybackController.
static const std::string LOOP_TOGGLE_ID("loop");

/// String to identify the Repeat Toggle of PlaybackController.
static const std::string REPEAT_TOGGLE_ID("repeat");

/// String to identify the Repeat Toggle of PlaybackController.
static const std::string THUMBSUP_TOGGLE_ID("thumbsUp");

/// String to identify the Repeat Toggle of PlaybackController.
static const std::string THUMBSDOWN_TOGGLE_ID("thumbsDown");

/// String identifier for the home target.
static const std::string TARGET_HOME("Home");

/// Map to match a toggle command id to the corresponding enum value.
static const std::map<std::string, avsCommon::avs::PlaybackToggle> TOGGLE_COMMAND_ID_TO_TOGGLE = {
    {SHUFFLE_TOGGLE_ID, avsCommon::avs::PlaybackToggle::SHUFFLE},
    {LOOP_TOGGLE_ID, avsCommon::avs::PlaybackToggle::LOOP},
    {REPEAT_TOGGLE_ID, avsCommon::avs::PlaybackToggle::LOOP},
    {THUMBSUP_TOGGLE_ID, avsCommon::avs::PlaybackToggle::THUMBS_UP},
    {THUMBSDOWN_TOGGLE_ID, avsCommon::avs::PlaybackToggle::THUMBS_DOWN}};

std::shared_ptr<GUIManager> GUIManager::create(
    std::shared_ptr<gui::GUIClientInterface> guiClient,
    std::shared_ptr<gui::GUIActivityEventNotifierInterface> activityEventNotifier,
    std::shared_ptr<applicationUtilities::resources::audio::MicrophoneInterface> micWrapper,
    std::shared_ptr<common::InteractionManager> interactionManager,
    std::shared_ptr<common::EndpointAlexaLauncherHandler> launcherHandler) {
    if (!guiClient) {
        ACSDK_CRITICAL(LX(__func__).d("reason", "null guiClient"));
        return nullptr;
    }
    if (!activityEventNotifier) {
        ACSDK_CRITICAL(LX(__func__).d("reason", "null activityEventNotifier"));
        return nullptr;
    }
    if (!micWrapper) {
        ACSDK_CRITICAL(LX(__func__).d("reason", "null micWrapper"));
        return nullptr;
    }

    if (!interactionManager) {
        ACSDK_CRITICAL(LX(__func__).d("reason", "null interactionManager"));
        return nullptr;
    }

    std::shared_ptr<GUIManager> guiManager(
        new GUIManager(guiClient, activityEventNotifier, micWrapper, interactionManager, launcherHandler));
    guiManager->initialize();

    return guiManager;
}

GUIManager::GUIManager(
    std::shared_ptr<gui::GUIClientInterface> guiClient,
    std::shared_ptr<gui::GUIActivityEventNotifierInterface> activityEventNotifier,
    std::shared_ptr<applicationUtilities::resources::audio::MicrophoneInterface> micWrapper,
    std::shared_ptr<common::InteractionManager> interactionManager,
    std::shared_ptr<common::EndpointAlexaLauncherHandler> launcherHandler) :
        RequiresShutdown{"GUIManager"},
        m_activityEventNotifier{activityEventNotifier},
        m_playerActivityState{PlayerActivity::FINISHED},
        m_launcherHandler{launcherHandler},
        m_interactionManager{interactionManager} {
    m_guiClient = guiClient;
    m_isSpeakingOrListening = false;
    m_clearAlertChannelOnForegrounded = false;
    m_audioInputProcessorState = AudioInputProcessorObserverInterface::State::IDLE;
    m_channelFocusStates[avsCommon::sdkInterfaces::FocusManagerInterface::DIALOG_CHANNEL_NAME] = FocusState::NONE;
    m_channelFocusStates[avsCommon::sdkInterfaces::FocusManagerInterface::ALERT_CHANNEL_NAME] = FocusState::NONE;
    m_channelFocusStates[avsCommon::sdkInterfaces::FocusManagerInterface::CONTENT_CHANNEL_NAME] = FocusState::NONE;
    m_channelFocusStates[avsCommon::sdkInterfaces::FocusManagerInterface::COMMUNICATIONS_CHANNEL_NAME] =
        FocusState::NONE;
    m_channelFocusStates[avsCommon::sdkInterfaces::FocusManagerInterface::VISUAL_CHANNEL_NAME] = FocusState::NONE;
    m_interfaceHoldingAudioFocus = {};

#ifdef UWP_BUILD
    m_micWrapper = std::dynamic_pointer_cast<alexaSmartScreenSDK::sssdkCommon::NullMicrophone>(micWrapper);
#else
    m_micWrapper = micWrapper;
#endif

    m_micWrapper->startStreamingMicrophoneData();
}

void GUIManager::initialize() {
    m_doNotDisturbIPCHandler = ipc::DoNotDisturbHandler::create(m_guiClient->getIPCRouter(), shared_from_this());
    m_interactionManagerIPCHandler =
        ipc::InteractionManagerHandler::create(m_guiClient->getIPCRouter(), shared_from_this());
    m_loggerIPCHandler = ipc::LoggerHandler::create(m_guiClient->getIPCRouter(), shared_from_this());
    m_sessionSetupIPCHandler = ipc::SessionSetupHandler::create(m_guiClient->getIPCRouter(), shared_from_this());
    m_windowManagerIPCHandler = ipc::WindowManagerHandler::create(m_guiClient->getIPCRouter(), shared_from_this());

    if (m_launcherHandler) {
        m_launcherHandler->registerLaunchTargetCallback(
            TARGET_HOME, [this]() { m_executor.submit([this]() { executeExitNavigation(); }); });
    }
}

void GUIManager::handleRecognizeSpeechRequest(capabilityAgents::aip::Initiator initiator, bool start) {
    ACSDK_DEBUG9(LX(__func__).d("initiator", initiatorToString(initiator)).d("start", start));
    m_executor.submit([this, initiator]() {
        if (capabilityAgents::aip::Initiator::TAP == initiator) {
            m_interactionManager->tap();
        } else if (capabilityAgents::aip::Initiator::PRESS_AND_HOLD == initiator) {
            m_interactionManager->holdToggled();
        }
    });
}

void GUIManager::handleMicrophoneToggle() {
    ACSDK_DEBUG5(LX(__func__));
    m_interactionManager->microphoneToggle();
}

void GUIManager::handlePlaybackPlay() {
    m_interactionManager->playbackPlay();
}

void GUIManager::handlePlaybackPause() {
    m_interactionManager->playbackPause();
}

void GUIManager::handlePlaybackNext() {
    m_interactionManager->playbackNext();
}

void GUIManager::handlePlaybackPrevious() {
    m_interactionManager->playbackPrevious();
}

void GUIManager::handlePlaybackSeekTo(int offset) {
    ACSDK_DEBUG9(LX(__func__).d("offset", offset));
    std::chrono::milliseconds seekToPosition(offset);
    m_executor.submit(
        [this, seekToPosition]() { m_defaultClient->getPlaybackRouter()->localSeekTo(seekToPosition, false); });
}

void GUIManager::handlePlaybackSkipForward() {
    m_interactionManager->playbackSkipForward();
}

void GUIManager::handlePlaybackSkipBackward() {
    m_interactionManager->playbackSkipBackward();
}

void GUIManager::handlePlaybackToggle(const std::string& name, bool checked) {
    m_executor.submit([this, name, checked]() {
        auto it = TOGGLE_COMMAND_ID_TO_TOGGLE.find(name);
        if (it == TOGGLE_COMMAND_ID_TO_TOGGLE.end()) {
            ACSDK_ERROR(LX(__func__).d("Invalid Toggle Name", name));
            return;
        }

        switch (it->second) {
            case avsCommon::avs::PlaybackToggle::SHUFFLE:
                m_interactionManager->playbackShuffle(checked);
                break;
            case avsCommon::avs::PlaybackToggle::LOOP:
                m_interactionManager->playbackLoop(checked);
                break;
            case avsCommon::avs::PlaybackToggle::THUMBS_UP:
                m_interactionManager->playbackThumbsUp(checked);
                break;
            case avsCommon::avs::PlaybackToggle::THUMBS_DOWN:
                m_interactionManager->playbackThumbsDown(checked);
                break;
            case avsCommon::avs::PlaybackToggle::REPEAT:
                m_interactionManager->playbackRepeat(checked);
                break;
        }
    });
}

void GUIManager::setFirmwareVersion(avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion) {
    m_interactionManager->setFirmwareVersion(firmwareVersion);
}

void GUIManager::adjustVolume(avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type, int8_t delta) {
    m_interactionManager->adjustVolume(type, delta);
}

void GUIManager::setMute(avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type, bool mute) {
    m_interactionManager->setMute(type, mute);
}

void GUIManager::resetDevice() {
    m_interactionManager->resetDevice();
}

void GUIManager::acceptCall() {
#ifdef ENABLE_COMMS
    m_interactionManager->acceptCall();
#endif
}

void GUIManager::stopCall() {
#ifdef ENABLE_COMMS
    m_interactionManager->stopCall();
#endif
}

void GUIManager::enableLocalVideo() {
#ifdef ENABLE_COMMS
    m_executor.submit([this]() {
        if (m_defaultClient->isCommsEnabled()) {
            m_defaultClient->enableLocalVideo();
        } else {
            ACSDK_WARN(LX(__func__).m("Communication not supported."));
        }
    });
#endif
}

void GUIManager::disableLocalVideo() {
#ifdef ENABLE_COMMS
    m_executor.submit([this]() {
        if (m_defaultClient->isCommsEnabled()) {
            m_defaultClient->disableLocalVideo();
        } else {
            ACSDK_WARN(LX(__func__).m("Communication not supported."));
        }
    });
#endif
}

#ifdef ENABLE_COMMS
void GUIManager::sendDtmf(avsCommon::sdkInterfaces::CallManagerInterface::DTMFTone dtmfTone) {
    m_interactionManager->sendDtmf(dtmfTone);
}
#endif
bool GUIManager::handleFocusAcquireRequest(
    std::string avsInterface,
    std::string channelName,
    avsCommon::avs::ContentType contentType,
    std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> channelObserver) {
    return m_executor
        .submit([this, channelName, channelObserver, contentType, avsInterface]() {
            auto activity = acl::FocusManagerInterface::Activity::create(
                avsInterface, channelObserver, std::chrono::milliseconds::zero(), contentType);

            bool focusAcquired = m_defaultClient->getAudioFocusManager()->acquireChannel(channelName, activity);
            if (focusAcquired) {
                m_interfaceHoldingAudioFocus = avsInterface;
            }
            return focusAcquired;
        })
        .get();
}

bool GUIManager::handleFocusReleaseRequest(
    std::string avsInterface,
    std::string channelName,
    std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> channelObserver) {
    return m_executor
        .submit([this, avsInterface, channelName, channelObserver]() {
            if (avsInterface == m_interfaceHoldingAudioFocus) {
                bool focusReleased =
                    m_defaultClient->getAudioFocusManager()->releaseChannel(channelName, channelObserver).get();
                if (focusReleased) {
                    m_interfaceHoldingAudioFocus.clear();
                }
                return focusReleased;
            }
            return false;
        })
        .get();
}

void GUIManager::handleGUIActivityEvent(avsCommon::sdkInterfaces::GUIActivityEvent event, const std::string& source) {
    m_executor.submit([this, source, event]() {
        if (avsCommon::sdkInterfaces::GUIActivityEvent::INTERRUPT == event && m_isSpeakingOrListening) {
            ACSDK_DEBUG3(LX(__func__).d("Interrupted activity while speaking or listening", event));
            if (avsCommon::avs::FocusState::FOREGROUND ==
                m_channelFocusStates[afml::FocusManager::DIALOG_CHANNEL_NAME]) {
                m_defaultClient->stopForegroundActivity();
            }
        }

        m_activityEventNotifier->notifyObserversOfGUIActivityEvent(source.empty() ? TAG : source, event);
    });
}

void GUIManager::handleNavigationEvent(NavigationEvent event) {
    m_executor.submit([this, event]() {
        ACSDK_DEBUG3(LX(__func__).d("processNavigationEvent in executor", navigationEventToString(event)));

        // TODO : Implement more robust visual presentation and granular channel orchestration for local navigation
        switch (event) {
            case NavigationEvent::BACK:
                executeBackNavigation();
                break;
            case NavigationEvent::EXIT:
                executeExitNavigation();
                break;
            default:
                // Not possible as returned above.
                break;
        }
    });
}

void GUIManager::executeBackNavigation() {
    /**
     * Back Navigation supports the following use cases:
     * 1. GUIClient managed back, for traversal of a UI client implemented backstack.
     * 2. Back from ALL other active audio channel(s) and /or visual card to audio content/PlayerInfo card.
     * 3. Back from audio content content/PlayerInfo card to 'home' state.
     */

    std::string focusedWindowId = m_presentationOrchestratorStateTracker->getFocusedWindowId();

    bool dialogChannelActive =
        FocusState::NONE != m_channelFocusStates[avsCommon::sdkInterfaces::FocusManagerInterface::DIALOG_CHANNEL_NAME];
    bool alertChannelActive =
        FocusState::NONE != m_channelFocusStates[avsCommon::sdkInterfaces::FocusManagerInterface::ALERT_CHANNEL_NAME];
    bool contentChannelActive =
        FocusState::NONE != m_channelFocusStates[avsCommon::sdkInterfaces::FocusManagerInterface::CONTENT_CHANNEL_NAME];
    bool nonPlayerInfoWindowDisplaying =
        !focusedWindowId.empty() && focusedWindowId != m_audioPlaybackUIWindowId &&
        focusedWindowId != m_templateRuntimePresentationAdapterBridge->getRenderPlayerInfoWindowId();

    /**
     * Always stop the foreground activity unless we're playing audio content, AND dialog and alerts aren't
     active,
     * AND we are still presenting GUI over PlayerInfo.  In that case we should only clear the card.
     */
    bool stopForegroundActivity =
        !(PlayerActivity::PLAYING == m_playerActivityState && nonPlayerInfoWindowDisplaying && !dialogChannelActive &&
          !alertChannelActive);

    /**
     * Always clear the displayed presentations unless:
     * - dialog OR alerts channel is active
     * - AND audio content channel is active, but there is no NonPlayerInfoDisplay UI displayed
     * In that case we should stop the foreground activity (the dialog or alert), but not clear the presentation.
     * TODO: Add case for live view controller here.
     */
    bool clearPresentations =
        !((dialogChannelActive || alertChannelActive) && (contentChannelActive && !nonPlayerInfoWindowDisplaying));

    /**
     * Stopping foreground audio activity happens before we allow GUIClient to handle 'visual' back navigation.
     */
    if (stopForegroundActivity) {
        /**
         * If both dialog and alerts are active,
         * stop dialog first (which has priority), and then stop alerts when it becomes foregrounded.
         */
        if (dialogChannelActive && alertChannelActive) {
            m_clearAlertChannelOnForegrounded = true;
        }
        m_defaultClient->stopForegroundActivity();
    }

    /**
     * BACK will attempt to let the Presentation Orchestrator handle visual navigation before clearing.
     * This allows for things like backstack traversal.
     */
    if (clearPresentations && m_presentationOrchestrator->navigateBack()) {
        /// Clear clout context unless waiting to clear Alert channel first
        if (!m_clearAlertChannelOnForegrounded) {
            forceClearDialogChannelFocus();
        }
    }
}

void GUIManager::executeExitNavigation() {
    /// EXIT will immediately clear everything.
    m_presentationOrchestrator->clearPresentations();
    if (m_alexaPresentationAPL) {
        m_alexaPresentationAPL->clearExecuteCommands();
    }
    m_defaultClient->stopAllActivities();
    forceClearDialogChannelFocus();
}

void GUIManager::forceExit() {
    m_executor.submit([this]() { executeExitNavigation(); });
}

std::vector<visualCharacteristicsInterfaces::WindowTemplate> GUIManager::getWindowTemplates() {
    return m_executor.submit([this]() { return m_visualCharacteristics->getWindowTemplates(); }).get();
}

std::vector<visualCharacteristicsInterfaces::InteractionMode> GUIManager::getInteractionModes() {
    return m_executor.submit([this]() { return m_visualCharacteristics->getInteractionModes(); }).get();
}

visualCharacteristicsInterfaces::DisplayCharacteristics GUIManager::getDisplayCharacteristics() {
    return m_executor.submit([this]() { return m_visualCharacteristics->getDisplayCharacteristics(); }).get();
}

static bool shouldReportWindowInstance(
    const presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance& instance) {
    const auto& interfaces = instance.supportedInterfaces;
    return std::find(interfaces.begin(), interfaces.end(), APL_INTERFACE) != interfaces.end();
}

void GUIManager::setWindowInstances(
    const std::vector<presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance>& instances,
    const std::string& defaultWindowInstanceId,
    const std::string& audioPlaybackUIWindowId) {
    ACSDK_DEBUG0(LX(__func__)
                     .d("defaultWindowInstanceId", defaultWindowInstanceId)
                     .d("audioPlaybackUIWindowId", audioPlaybackUIWindowId));
    m_executor.submit([this, instances, defaultWindowInstanceId, audioPlaybackUIWindowId]() {
        setDefaultWindowId(defaultWindowInstanceId);
        setAudioPlaybackUIWindowId(audioPlaybackUIWindowId);
        std::vector<visualCharacteristicsInterfaces::WindowInstance> reportableInstances;
        for (const auto& instance : instances) {
            if (shouldReportWindowInstance(instance)) {
                reportableInstances.push_back(instance);
            }
        }

        m_visualCharacteristics->setWindowInstances(reportableInstances, defaultWindowInstanceId);
        m_presentationOrchestratorStateTracker->setWindows(instances);
    });
}

bool GUIManager::addWindowInstance(
    const presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance& instance) {
    ACSDK_DEBUG5(LX(__func__).d("windowId", instance.id));
    return m_executor
        .submit([this, instance]() {
            if (shouldReportWindowInstance(instance) && !m_visualCharacteristics->addWindowInstance(instance)) {
                ACSDK_ERROR(LX("addWindowInstanceFailed")
                                .d("reason", "Failed to add window instance to visual characteristics"));
                return false;
            }

            m_presentationOrchestratorStateTracker->addWindow(instance);
            return true;
        })
        .get();
}

bool GUIManager::removeWindowInstance(const std::string& windowInstanceId) {
    ACSDK_DEBUG5(LX(__func__).d("windowId", windowInstanceId));
    return m_executor
        .submit([this, windowInstanceId]() { return m_visualCharacteristics->removeWindowInstance(windowInstanceId); })
        .get();
}

void GUIManager::updateWindowInstance(
    const presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance& instance) {
    ACSDK_DEBUG5(LX(__func__).d("windowId", instance.id));
    m_executor.submit([this, instance]() { m_visualCharacteristics->updateWindowInstance(instance); });
}

bool GUIManager::setDefaultWindowInstance(const std::string& windowInstanceId) {
    ACSDK_DEBUG5(LX(__func__).d("windowId", windowInstanceId));
    return m_executor
        .submit(
            [this, windowInstanceId]() { return m_visualCharacteristics->setDefaultWindowInstance(windowInstanceId); })
        .get();
}

bool GUIManager::serializeInteractionMode(
    const std::vector<visualCharacteristicsInterfaces::InteractionMode>& interactionModes,
    std::string& serializedJson) {
    ACSDK_DEBUG5(LX(__func__));
    return m_executor
        .submit([this, interactionModes, &serializedJson]() {
            return m_visualCharacteristicsSerializer->serializeInteractionModes(interactionModes, serializedJson);
        })
        .get();
}

bool GUIManager::serializeWindowTemplate(
    const std::vector<visualCharacteristicsInterfaces::WindowTemplate>& windowTemplates,
    std::string& serializedJson) {
    ACSDK_DEBUG5(LX(__func__));
    return m_executor
        .submit([this, windowTemplates, &serializedJson]() {
            return m_visualCharacteristicsSerializer->serializeWindowTemplate(windowTemplates, serializedJson);
        })
        .get();
}

bool GUIManager::serializeDisplayCharacteristics(
    const visualCharacteristicsInterfaces::DisplayCharacteristics& display,
    std::string& serializedJson) {
    ACSDK_DEBUG5(LX(__func__));
    return m_executor
        .submit([this, display, &serializedJson]() {
            return m_visualCharacteristicsSerializer->serializeDisplayCharacteristics(display, serializedJson);
        })
        .get();
}

void GUIManager::doNotDisturbStateChanged(const std::string& message) {
    bool enabled = false;
    if (!jsonUtils::retrieveValue(message, ipc::ENABLED_TAG, &enabled)) {
        ACSDK_ERROR(LX(__func__).d("reason", "enabledNotFound"));
        return;
    }
    m_defaultClient->getSettingsManager()->setValue<settings::DO_NOT_DISTURB>(enabled);
}

void GUIManager::doNotDisturbStateRequest(const std::string& message) {
    if (m_doNotDisturbIPCHandler && m_settingsManager) {
        m_doNotDisturbIPCHandler->dispatchSetDoNotDisturbState(
            m_settingsManager->getValue<settings::DO_NOT_DISTURB>(false).second);
    } else {
        if (!m_doNotDisturbIPCHandler) {
            ACSDK_WARN(LX(__func__).d("reason", "doNotDisturbSettingHandlerNotFound"));
        }
        if (!m_settingsManager) {
            ACSDK_WARN(LX(__func__).d("reason", "settingsManagerNotFound"));
        }
    }
}

void GUIManager::namespaceVersionsReport(const std::string& message) {
    m_ipcVersionManager->handleAssertNamespaceVersionsFromString(message);
}

void GUIManager::clientInitialized(const std::string& message) {
    bool isIPCVersionSupported = false;
    bool errorState = false;
    if (!jsonUtils::retrieveValue(message, ipc::IS_IPC_VERSION_SUPPORTED_TAG, &isIPCVersionSupported)) {
        ACSDK_ERROR(LX(__func__).d("reason", "isIPCVersionSupportedNotFound"));
        errorState = true;
    }

    if (!isIPCVersionSupported && !errorState) {
        ACSDK_WARN(LX(__func__)
                       .d("reason", "IPC Version not supported by client")
                       .d("IPCVersion", ipc::IS_IPC_VERSION_SUPPORTED_TAG));
    }

    if (m_guiClient->finalizeClientInitialization(errorState)) {
        // Init locale for gui layer after GUI client is done initializing.
        handleLocaleChange();
    }
}

void GUIManager::clientConfigRequest(const std::string& message) {
    if (m_sessionSetupIPCHandler) {
        m_sessionSetupIPCHandler->dispatchConfigureClient(message);
    } else {
        ACSDK_WARN(LX(__func__).d("reason", "sessionSetupHandlerNotFound"));
    }
}

void GUIManager::initClient() {
    if (m_sessionSetupIPCHandler) {
        m_sessionSetupIPCHandler->dispatchInitializeClient(ipc::IPCNamespaces::IPC_FRAMEWORK_VERSION);
    } else {
        ACSDK_WARN(LX(__func__).d("reason", "sessionSetupHandlerNotFound"));
    }
}

void GUIManager::visualCharacteristicsRequest(const std::string& message) {
    rapidjson::Document payload;
    if (!jsonUtils::parseJSON(message, &payload)) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalid message string"));
        return;
    }

    if (!payload.HasMember(ipc::CHARACTERISTICS_TAG)) {
        ACSDK_ERROR(LX(__func__).d("reason", "characteristics key not found"));
        return;
    }

    avsCommon::utils::Optional<std::string> displayCharacteristicOpt;
    avsCommon::utils::Optional<std::string> interactionModesOpt;
    avsCommon::utils::Optional<std::string> windowTemplatesOpt;
    auto characteristicsSet = jsonUtils::retrieveStringArray<std::set<std::string>>(payload[ipc::CHARACTERISTICS_TAG]);
    for (auto const& characteristic : characteristicsSet) {
        if (ipc::DEVICE_DISPLAY_TAG == characteristic) {
            auto displayCharacteristics = getDisplayCharacteristics();
            std::string serializedDisplayCharacteristics;
            if (!serializeDisplayCharacteristics(displayCharacteristics, serializedDisplayCharacteristics)) {
                ACSDK_WARN(LX(__func__).d("reason", "unable to serialize display characteristics"));
                continue;
            }

            displayCharacteristicOpt.set(serializedDisplayCharacteristics);
            continue;
        }

        if (ipc::INTERACTION_MODES_TAG == characteristic) {
            auto interactionModes = getInteractionModes();
            std::string serializedInteractionModes;
            if (!serializeInteractionMode(interactionModes, serializedInteractionModes)) {
                ACSDK_WARN(LX(__func__).d("reason", "unable to serialize interaction modes"));
                continue;
            }

            interactionModesOpt.set(serializedInteractionModes);
            continue;
        }

        if (ipc::WINDOW_TEMPLATES_TAG == characteristic) {
            auto windowTemplates = getWindowTemplates();
            std::string serializedWindowTemplates;
            if (!serializeWindowTemplate(windowTemplates, serializedWindowTemplates)) {
                ACSDK_WARN(LX(__func__).d("reason", "unable to serialize window templates"));
                continue;
            }

            windowTemplatesOpt.set(serializedWindowTemplates);
            continue;
        }
    }

    if (m_windowManagerIPCHandler) {
        m_windowManagerIPCHandler->dispatchSetVisualCharacteristics(
            displayCharacteristicOpt, interactionModesOpt, windowTemplatesOpt);
    } else {
        ACSDK_WARN(LX(__func__).d("reason", "windowManagerHandlerNotFound"));
    }
}

void GUIManager::setDefaultWindowId(const std::string& windowId) {
    m_defaultWindowId = windowId;
    m_aplRuntimePresentationAdapter->setDefaultWindowId(m_defaultWindowId);
}

void GUIManager::setAudioPlaybackUIWindowId(const std::string& windowId) {
    m_audioPlaybackUIWindowId = windowId;
    if (!m_templateRuntimePresentationAdapterBridge->setRenderPlayerInfoWindowId(m_audioPlaybackUIWindowId)) {
        ACSDK_WARN(LX(__func__).d("reason", "player info window already set."));
    }
}

void GUIManager::defaultWindowInstanceChanged(const std::string& message) {
    rapidjson::Document payload;
    if (!jsonUtils::parseJSON(message, &payload)) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalid message string"));
        return;
    }

    if (!payload.IsObject()) {
        ACSDK_ERROR(LX(__func__).d("reason", "payload not an object"));
        return;
    }

    std::string defaultWindowId;
    if (!jsonUtils::retrieveValue(payload, ipc::DEFAULT_WINDOW_ID_TAG, &defaultWindowId)) {
        ACSDK_ERROR(LX(__func__).d("reason", "default window ID not found"));
        return;
    }

    if (!setDefaultWindowInstance(defaultWindowId)) {
        ACSDK_ERROR(LX(__func__)
                        .d("reason", "unable to set the default window instance")
                        .d("defaultWindowId", defaultWindowId));
        return;
    }

    // Set Default Window Id
    setDefaultWindowId(defaultWindowId);
}

void GUIManager::windowInstancesReport(const std::string& message) {
    rapidjson::Document payload;
    if (!jsonUtils::parseJSON(message, &payload)) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalid message string"));
        return;
    }

    std::string defaultWindowId;
    if (!jsonUtils::retrieveValue(message, ipc::DEFAULT_WINDOW_ID_TAG, &defaultWindowId)) {
        ACSDK_ERROR(LX(__func__).d("reason", "default window ID not found"));
        return;
    }

    std::string audioPlaybackUIWindowId;
    if (!jsonUtils::retrieveValue(message, ipc::AUDIO_PLAYBACK_UI_WINDOW_ID_TAG, &audioPlaybackUIWindowId)) {
        ACSDK_ERROR(LX(__func__).d("reason", "audio playback UI window ID not found"));
        return;
    }

    if (!payload.HasMember(ipc::WINDOW_INSTANCES_TAG)) {
        ACSDK_ERROR(LX(__func__).d("reason", "window instances key not found"));
        return;
    }
    const auto& windowInstances = payload[ipc::WINDOW_INSTANCES_TAG];
    if (!windowInstances.IsArray()) {
        ACSDK_ERROR(LX(__func__).d("reason", "window instances not an array"));
        return;
    }

    std::vector<presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance> instances;
    for (auto const& windowInstance : windowInstances.GetArray()) {
        presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance instance;
        if (!parseWindowInstance(windowInstance, instance)) {
            ACSDK_ERROR(LX(__func__).d("reason", "unable to parse all window instances"));
            return;
        }

        instances.push_back(instance);
    }

    // Set Window Instances
    setWindowInstances(instances, defaultWindowId, audioPlaybackUIWindowId);
}

bool GUIManager::parseWindowInstance(
    const rapidjson::Value& payload,
    presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance& windowInstance) {
    if (!jsonUtils::retrieveValue(payload, ipc::WINDOW_ID_TAG, &windowInstance.id)) {
        ACSDK_ERROR(LX(__func__).d("reason", "window ID not found"));
        return false;
    }

    if (!jsonUtils::retrieveValue(payload, ipc::TEMPLATE_ID_TAG, &windowInstance.templateId)) {
        ACSDK_ERROR(LX(__func__).d("reason", "template ID not found"));
        return false;
    }

    if (!jsonUtils::retrieveValue(payload, ipc::INTERACTION_MODE_TAG, &windowInstance.interactionMode)) {
        ACSDK_ERROR(LX(__func__).d("reason", "interaction mode not found"));
        return false;
    }

    if (!jsonUtils::retrieveValue(payload, ipc::SIZE_CONFIGURATION_ID_TAG, &windowInstance.sizeConfigurationId)) {
        ACSDK_ERROR(LX(__func__).d("reason", "size configuration ID not found"));
        return false;
    }

    if (!payload.HasMember(ipc::SUPPORTED_INTERFACES)) {
        ACSDK_ERROR(LX(__func__).d("reason", "supportedInterfaces not found"));
        return false;
    }

    const auto& supportedInterfacesValue = payload[ipc::SUPPORTED_INTERFACES];
    if (!supportedInterfacesValue.IsArray()) {
        ACSDK_ERROR(LX(__func__).d("reason", "supportedInterfaces is not an array"));
        return false;
    }
    windowInstance.supportedInterfaces =
        jsonUtils::retrieveStringArray<std::vector<std::string>>(supportedInterfacesValue);

    int64_t zOrderIndex;
    if (!jsonUtils::retrieveValue(payload, ipc::Z_ORDER_INDEX, &zOrderIndex)) {
        ACSDK_ERROR(LX(__func__).d("reason", "zOrderIndex not found"));
        return false;
    }

    windowInstance.zOrderIndex = zOrderIndex;

    return true;
}

void GUIManager::windowInstancesAdded(const std::string& message) {
    rapidjson::Document payload;
    if (!jsonUtils::parseJSON(message, &payload)) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalid message string"));
        return;
    }

    if (!payload.IsObject()) {
        ACSDK_ERROR(LX(__func__).d("reason", "payload not an object"));
        return;
    }

    if (!payload.HasMember(ipc::WINDOW_INSTANCES_TAG)) {
        ACSDK_ERROR(LX(__func__).d("reason", "window instances key not found"));
        return;
    }

    const auto& windowInstances = payload[ipc::WINDOW_INSTANCES_TAG];
    if (!windowInstances.IsArray()) {
        ACSDK_ERROR(LX(__func__).d("reason", "window instances not an array"));
        return;
    }
    for (auto const& windowInstance : windowInstances.GetArray()) {
        presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance instance;
        if (!parseWindowInstance(windowInstance, instance)) {
            ACSDK_ERROR(LX(__func__).d("reason", "unable to parse all window instances"));
            return;
        }
        if (!addWindowInstance(instance)) {
            ACSDK_ERROR(LX(__func__).d("reason", "unable to add window instance").d("windowId", instance.id));
            return;
        }
    }
}

void GUIManager::windowInstancesRemoved(const std::string& message) {
    rapidjson::Document payload;
    if (!jsonUtils::parseJSON(message, &payload)) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalid message string"));
        return;
    }

    if (!payload.IsObject()) {
        ACSDK_ERROR(LX(__func__).d("reason", "payload not an object"));
        return;
    }

    if (!payload.HasMember(ipc::WINDOW_IDS_TAG)) {
        ACSDK_ERROR(LX(__func__).d("reason", "window ids key not found"));
        return;
    }

    const auto& windowIds = payload[ipc::WINDOW_IDS_TAG];
    if (!windowIds.IsArray()) {
        ACSDK_ERROR(LX(__func__).d("reason", "window ids not an array"));
        return;
    }

    auto windowIdsSet = jsonUtils::retrieveStringArray<std::set<std::string>>(payload[ipc::WINDOW_IDS_TAG]);
    for (auto const& windowId : windowIdsSet) {
        if (!removeWindowInstance(windowId)) {
            ACSDK_WARN(LX(__func__).d("reason", "unable to remove window instance").d("windowId", windowId));
            continue;
        }
    }
}

void GUIManager::windowInstancesUpdated(const std::string& message) {
    rapidjson::Document payload;
    if (!jsonUtils::parseJSON(message, &payload)) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalid message string"));
        return;
    }

    if (!payload.IsObject()) {
        ACSDK_ERROR(LX(__func__).d("reason", "payload not an object"));
        return;
    }

    if (!payload.HasMember(ipc::WINDOW_INSTANCES_TAG)) {
        ACSDK_ERROR(LX(__func__).d("reason", "window instances key not found"));
        return;
    }
    const auto& windowInstances = payload[ipc::WINDOW_INSTANCES_TAG];
    if (!windowInstances.IsArray()) {
        ACSDK_ERROR(LX(__func__).d("reason", "window instances not an array"));
        return;
    }
    for (const auto& windowInstance : windowInstances.GetArray()) {
        presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance instance;
        if (!parseWindowInstance(windowInstance, instance)) {
            ACSDK_WARN(LX(__func__).d("reason", "unable to parse window instance"));
            continue;
        }
        updateWindowInstance(instance);
    }
}

void GUIManager::recognizeSpeechRequest(const std::string& message) {
    ACSDK_DEBUG0(LX(__func__));

    rapidjson::Document payload;
    if (!jsonUtils::parseJSON(message, &payload)) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalid message string"));
        return;
    }

    std::string initiatorType;
    if (!jsonUtils::retrieveValue(payload, ipc::AUDIO_INPUT_INITIATOR_TYPE_TAG, &initiatorType)) {
        ACSDK_ERROR(LX(__func__).d("reason", "initiatorTypeNotFound"));
        return;
    }

    capabilityAgents::aip::Initiator initiator;
    if (initiatorToString(capabilityAgents::aip::Initiator::PRESS_AND_HOLD) == initiatorType) {
        initiator = capabilityAgents::aip::Initiator::PRESS_AND_HOLD;
    } else if (initiatorToString(capabilityAgents::aip::Initiator::TAP) == initiatorType) {
        initiator = capabilityAgents::aip::Initiator::TAP;
    } else {
        ACSDK_ERROR(LX(__func__).d("reason", "invalid initiatorType"));
        return;
    }

    std::string captureStateStr;
    if (!jsonUtils::retrieveValue(payload, ipc::CAPTURE_STATE_TAG, &captureStateStr)) {
        ACSDK_ERROR(LX(__func__).d("reason", "captureStateNotFound"));
        return;
    }

    CaptureState captureState = captureStateFromString(captureStateStr);
    if (CaptureState::UNKNOWN == captureState) {
        ACSDK_ERROR(LX(__func__).d("reason", "received unknown type of captureState"));
        return;
    }
    bool startCapture = CaptureState::START == captureState;

    handleRecognizeSpeechRequest(initiator, startCapture);
}

void GUIManager::navigationEvent(const std::string& message) {
    rapidjson::Document payload;
    if (!jsonUtils::parseJSON(message, &payload)) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalid message string"));
        return;
    }

    std::string event;
    if (!jsonUtils::retrieveValue(payload, ipc::EVENT_TAG, &event)) {
        ACSDK_ERROR(LX(__func__).d("reason", "eventNotFound"));
        return;
    }

    NavigationEvent navigationEvent = navigationEventFromString(event);
    if (NavigationEvent::UNKNOWN == navigationEvent) {
        ACSDK_ERROR(LX(__func__).d("reason", "received unknown type of event"));
        return;
    }

    handleNavigationEvent(navigationEvent);
}

void GUIManager::guiActivityEvent(const std::string& message) {
    ACSDK_DEBUG5(LX(__func__));

    rapidjson::Document payload;
    if (!jsonUtils::parseJSON(message, &payload)) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalid message string"));
        return;
    }

    std::string event;
    if (!jsonUtils::retrieveValue(payload, ipc::EVENT_TAG, &event)) {
        ACSDK_ERROR(LX(__func__).d("reason", "eventNotFound"));
        return;
    }
    avsCommon::sdkInterfaces::GUIActivityEvent activityEvent =
        avsCommon::sdkInterfaces::guiActivityEventFromString(event);
    if (avsCommon::sdkInterfaces::GUIActivityEvent::UNKNOWN == activityEvent) {
        ACSDK_ERROR(LX(__func__).d("reason", "received unknown type of event"));
        return;
    }

    handleGUIActivityEvent(activityEvent);
}

void GUIManager::logEvent(const std::string& message) {
    std::string level;
    if (!jsonUtils::retrieveValue(message, ipc::LEVEL_TAG, &level)) {
        ACSDK_ERROR(LX(__func__).d("reason", "levelNotFound"));
        return;
    }

    IPCLogLevel logLevel = ipcLogLevelFromString(level);
    if (IPCLogLevel::UNKNOWN == logLevel) {
        ACSDK_ERROR(LX(__func__).d("reason", "received unknown type of logLevel"));
        return;
    }

    std::string logMessage;
    if (!jsonUtils::retrieveValue(message, ipc::MESSAGE_TAG, &logMessage)) {
        ACSDK_ERROR(LX(__func__).d("reason", "messageNotFound"));
        return;
    }

    m_guiLogBridge.log(logLevel, logMessage);
}

#ifdef ENABLE_COMMS
void GUIManager::onCallStateInfoChange(const CallStateInfo& stateInfo) {
    m_guiClient->sendCallStateInfo(stateInfo);
}

void GUIManager::onCallStateChange(CallState callState) {
    m_interactionManager->onCallStateChange(callState);
}
#endif

void GUIManager::onDialogUXStateChanged(DialogUXState state) {
    m_executor.submit([this, state]() {
        switch (state) {
            case DialogUXState::SPEAKING:
                m_isSpeakingOrListening = true;
                break;
            case DialogUXState::EXPECTING:
            case DialogUXState::FINISHED:
            case DialogUXState::IDLE:
            case DialogUXState::THINKING:
                m_isSpeakingOrListening = false;
                break;
            case DialogUXState::LISTENING:
                m_isSpeakingOrListening = true;
            default:
                break;
        }
    });
}

void GUIManager::onUserEvent() {
    ACSDK_DEBUG0(LX(__func__).m(AudioInputProcessorObserverInterface::stateToString(m_audioInputProcessorState)));
    if (AudioInputProcessorObserverInterface::State::EXPECTING_SPEECH == m_audioInputProcessorState) {
        m_defaultClient->stopInteraction();
    }
}

void GUIManager::onStateChanged(AudioInputProcessorObserverInterface::State state) {
    m_audioInputProcessorState = state;

    // Interrupt activity on speech recognizing
    if (state == AudioInputProcessorObserverInterface::State::RECOGNIZING) {
        handleGUIActivityEvent(
            avsCommon::sdkInterfaces::GUIActivityEvent::INTERRUPT,
            "AudioInputProcessor" + AudioInputProcessorObserverInterface::stateToString(state));
    }
}

void GUIManager::onPlayerActivityChanged(avsCommon::avs::PlayerActivity state, const Context& context) {
    m_executor.submit([this, state]() { m_playerActivityState = state; });
}

void GUIManager::onStateChanged(
    const std::string& windowId,
    const presentationOrchestratorInterfaces::PresentationMetadata& metadata) {
    ACSDK_DEBUG(LX(__func__).d("windowId", windowId).d("interface", metadata.interfaceName));
    if (!windowId.empty() && metadata.interfaceName.empty()) {
        ACSDK_DEBUG(LX("clearWindow").d("windowId", windowId));
        m_windowManagerIPCHandler->dispatchClearWindow(windowId);
    }
}

void GUIManager::onFocusChanged(const std::string& channelName, FocusState newFocus) {
    m_executor.submit([this, channelName, newFocus]() {
        ACSDK_DEBUG(
            LX("ChannelFocusChanged").d("channelName", channelName).d("newFocus", focusStateToString(newFocus)));

        m_channelFocusStates[channelName] = newFocus;

        /// Handle use case to clear Alerts channel when foregrounded.
        if (channelName == avsCommon::sdkInterfaces::FocusManagerInterface::ALERT_CHANNEL_NAME &&
            FocusState::FOREGROUND == newFocus && m_clearAlertChannelOnForegrounded) {
            m_defaultClient->stopForegroundActivity();
            forceClearDialogChannelFocus();
            m_clearAlertChannelOnForegrounded = false;
        }
    });
}

void GUIManager::setClient(std::shared_ptr<sdkClient::SDKClientRegistry> clientRegistry) {
    auto result = m_executor.submit([this, clientRegistry]() {
        if (!clientRegistry) {
            ACSDK_CRITICAL(LX("setClientFailed").d("reason", "null clientRegistry"));
            return;
        }
        m_defaultClient = clientRegistry->get<defaultClient::DefaultClient>();
        if (!m_defaultClient) {
            ACSDK_CRITICAL(LX("setClientFailed").d("reason", "null default client"));
            return;
        }

        auto visualStateTrackerFeatureClient = clientRegistry->get<featureClient::VisualStateTrackerFeatureClient>();
        if (!visualStateTrackerFeatureClient) {
            ACSDK_CRITICAL(LX("setClientFailed").d("reason", "null state tracker client"));
            return;
        }

        m_presentationOrchestratorStateTracker =
            visualStateTrackerFeatureClient->getPresentationOrchestratorStateTracker();
        m_presentationOrchestratorStateTracker->addStateObserver(shared_from_this());

        auto visualCharacteristicsFeatureClient =
            clientRegistry->get<featureClient::VisualCharacteristicsFeatureClient>();
        if (!visualCharacteristicsFeatureClient) {
            ACSDK_CRITICAL(LX("setClientFailed").d("reason", "null visual characteristics client"));
            return;
        }

        m_visualCharacteristics = visualCharacteristicsFeatureClient->getVisualCharacteristics();
        m_visualCharacteristicsSerializer = visualCharacteristicsFeatureClient->getVisualCharacteristicsSerializer();

        auto presentationOrchestratorFeatureClient =
            clientRegistry->get<featureClient::PresentationOrchestratorFeatureClient>();
        if (!presentationOrchestratorFeatureClient) {
            ACSDK_CRITICAL(LX("setClientFailed").d("reason", "null presentation orchestrator feature client"));
            return;
        }

        m_presentationOrchestrator = presentationOrchestratorFeatureClient->getPresentationOrchestrator();

        auto alexaPresentationFeatureClient = clientRegistry->get<featureClient::AlexaPresentationFeatureClient>();
        if (alexaPresentationFeatureClient) {
            m_alexaPresentationAPL = alexaPresentationFeatureClient->getAPLCapabilityAgent();
        } else {
            ACSDK_WARN(LX("setClientIncomplete").d("reason", "null alexa presentation feature client"));
        }

        m_settingsManager = m_defaultClient->getSettingsManager();
        if (!m_settingsManager) {
            ACSDK_CRITICAL(LX("setClientFailed").d("reason", "Unable to retrieve settings manager"));
            return;
        }

        m_timezoneHelper = TimezoneHelper::create(m_settingsManager);
        if (!m_timezoneHelper) {
            ACSDK_CRITICAL(LX("setClientFailed").d("reason", "Unable to create timezone helper"));
        }
    });
    result.wait();
}

std::chrono::milliseconds GUIManager::getDeviceTimezoneOffset() {
    if (!m_timezoneHelper) {
        ACSDK_WARN(LX("getDeviceTimezoneOffsetFailed").d("reason", "null TimezoneHelper"));
        return std::chrono::milliseconds::zero();
    }
    return m_timezoneHelper->getDeviceTimezoneOffset();
}

void GUIManager::doShutdown() {
    ACSDK_DEBUG3(LX(__func__));
    m_executor.shutdown();
    m_guiClient.reset();
    m_micWrapper.reset();
    m_interactionManager.reset();
}

bool GUIManager::configureSettingsNotifications() {
    bool future =
        m_executor
            .submit([this]() {
                if (!m_settingsManager) {
                    ACSDK_ERROR(LX("configureSettingsNotificationsFailed").d("reason", "nullSettingsManager"));
                    return false;
                }

                m_callbacks = settings::SettingCallbacks<settings::DeviceSettingsManager>::create(m_settingsManager);

                if (!m_callbacks) {
                    ACSDK_ERROR(LX("configureSettingsNotificationsFailed").d("reason", "createCallbacksFailed"));
                    return false;
                }

                bool callback = m_callbacks->add<settings::DeviceSettingsIndex::DO_NOT_DISTURB>(
                    [this](bool enable, settings::SettingNotifications notifications) {
                        if (m_doNotDisturbIPCHandler && m_settingsManager) {
                            m_doNotDisturbIPCHandler->dispatchSetDoNotDisturbState(
                                m_settingsManager->getValue<settings::DO_NOT_DISTURB>(false).second);
                        } else {
                            if (!m_doNotDisturbIPCHandler) {
                                ACSDK_WARN(LX(__func__).d("reason", "doNotDisturbSettingHandlerNotFound"));
                            }
                            if (!m_settingsManager) {
                                ACSDK_WARN(LX(__func__).d("reason", "settingsManagerNotFound"));
                            }
                        }
                    });

                callback &= m_callbacks->add<settings::DeviceSettingsIndex::LOCALE>(
                    [this](const settings::DeviceLocales& value, settings::SettingNotifications notifications) {
                        handleLocaleChange();
                    });
                return callback;
            })
            .get();

    return future;
}

void GUIManager::handleLocaleChange() {
    auto localeSetting = m_defaultClient->getSettingsManager()->getValue<settings::DeviceSettingsIndex::LOCALE>();
    if (!localeSetting.first) {
        ACSDK_WARN(LX(__func__).m("Invalid locales array from settings."));
        return;
    }
    auto locales = localeSetting.second;
    rapidjson::Document document;
    document.SetArray();

    auto& allocator = document.GetAllocator();
    for (const auto& locale : locales) {
        document.PushBack(rapidjson::Value().SetString(locale.c_str(), locale.length(), allocator), allocator);
    }

    rapidjson::StringBuffer strbuf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    if (!document.Accept(writer)) {
        ACSDK_WARN(LX(__func__).m("Error serializing document payload."));
        return;
    }

    auto localeStr = strbuf.GetString();
    ACSDK_DEBUG3(LX(__func__).d("LocaleChanged", localeStr));
    m_guiClient->setLocales(localeStr);
}

void GUIManager::setIpcVersionManager(std::shared_ptr<ipc::IPCVersionManager> ipcVersionManager) {
    m_ipcVersionManager = std::move(ipcVersionManager);
}

void GUIManager::setAPLRuntimePresentationAdapter(
    std::shared_ptr<APLRuntimePresentationAdapter> aplRuntimePresentationAdapter) {
    m_aplRuntimePresentationAdapter = std::move(aplRuntimePresentationAdapter);
}

void GUIManager::setTemplateRuntimePresentationAdapterBridge(
    std::shared_ptr<TemplateRuntimePresentationAdapterBridge> templateRuntimePresentationAdapterBridge) {
    m_templateRuntimePresentationAdapterBridge = std::move(templateRuntimePresentationAdapterBridge);
}

void GUIManager::handleOnMessagingServerConnectionOpened() {
    if (m_doNotDisturbIPCHandler && m_settingsManager) {
        m_doNotDisturbIPCHandler->dispatchSetDoNotDisturbState(
            m_settingsManager->getValue<settings::DO_NOT_DISTURB>(false).second);
    } else {
        if (!m_doNotDisturbIPCHandler) {
            ACSDK_WARN(LX(__func__).d("reason", "doNotDisturbSettingHandlerNotFound"));
        }
        if (!m_settingsManager) {
            ACSDK_WARN(LX(__func__).d("reason", "settingsManagerNotFound"));
        }
    }
}

void GUIManager::handleDocumentTerminated(const std::string& token, bool failed) {
    // Only stop audio if it is coming from APL Audio (SpeakItem, SpeakList, etc.)
    if (APL_INTERFACE == m_interfaceHoldingAudioFocus) {
        m_defaultClient->stopForegroundActivity();
    }
}

/// === Workaround start ===
/**
 * In order to support multi-turn interactions SDK processes SpeechSynthesizer audio context in special way. This
 * leads to skill context not been cleared on cloud side when we locally exit. In order to fix that we should grab
 * DIALOG channel by interface processed in normal way and proceed as before.
 * More global AVS C++ SDK solution to be implemented later.
 */
/// Interface name to use for focus requests.

void GUIManager::forceClearDialogChannelFocus() {
    ACSDK_DEBUG5(LX(__func__).m("Force Clear Dialog Channel"));
    m_defaultClient->getAudioFocusManager()->acquireChannel(
        avsCommon::sdkInterfaces::FocusManagerInterface::DIALOG_CHANNEL_NAME, shared_from_this(), APL_INTERFACE);
}

void GUIManager::onFocusChanged(avsCommon::avs::FocusState newFocus, avsCommon::avs::MixingBehavior behavior) {
    if (newFocus == avsCommon::avs::FocusState::FOREGROUND) {
        m_defaultClient->stopForegroundActivity();
        m_defaultClient->stopInteraction();
    }
}
/// === Workaround end ===

#ifdef UWP_BUILD
void GUIManager::inputAudioFile(const std::string& audioFile) {
    bool errorOccured = false;
    auto audioData = alexaSmartScreenSDK::sssdkCommon::AudioFileUtil::readAudioFromFile(audioFile, errorOccured);
    if (errorOccured) {
        return;
    }
    handleTapToTalk();
    m_micWrapper->writeAudioData(audioData);
}
#endif

}  // namespace gui
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
