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

#include <AVSCommon/Utils/Logger/Logger.h>

#include "RegistrationManager/CustomerDataManager.h"
#include "SampleApp/InteractionManager.h"
#include "Endpoints/Endpoint.h"

#ifdef ENABLE_PCC
#include <SampleApp/PhoneCaller.h>
#endif

#ifdef ENABLE_MCC
#include <SampleApp/CalendarClient.h>
#include <SampleApp/MeetingClient.h>
#endif

namespace alexaClientSDK {
namespace sampleApp {

/// String to identify log entries originating from this file.
static const std::string TAG("InteractionManager");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;

InteractionManager::InteractionManager(
    std::shared_ptr<defaultClient::DefaultClient> client,
    std::shared_ptr<applicationUtilities::resources::audio::MicrophoneInterface> micWrapper,
    std::shared_ptr<sampleApp::UIManager> userInterface,
#ifdef ENABLE_PCC
    std::shared_ptr<sampleApp::PhoneCaller> phoneCaller,
#endif
#ifdef ENABLE_MCC
    std::shared_ptr<sampleApp::MeetingClient> meetingClient,
    std::shared_ptr<sampleApp::CalendarClient> calendarClient,
#endif
    capabilityAgents::aip::AudioProvider holdToTalkAudioProvider,
    capabilityAgents::aip::AudioProvider tapToTalkAudioProvider,
    std::shared_ptr<sampleApp::GuiRenderer> guiRenderer,
    capabilityAgents::aip::AudioProvider wakeWordAudioProvider,

#ifdef POWER_CONTROLLER
    std::shared_ptr<PeripheralEndpointPowerControllerHandler> powerControllerHandler,
#endif
#ifdef TOGGLE_CONTROLLER
    std::shared_ptr<PeripheralEndpointToggleControllerHandler> toggleControllerHandler,
#endif
#ifdef RANGE_CONTROLLER
    std::shared_ptr<PeripheralEndpointRangeControllerHandler> rangeControllerHandler,
#endif
#ifdef MODE_CONTROLLER
    std::shared_ptr<PeripheralEndpointModeControllerHandler> modeControllerHandler,
#endif
    std::shared_ptr<avsCommon::sdkInterfaces::CallManagerInterface> callManager,
    std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface> diagnostics) :
        RequiresShutdown{"InteractionManager"},
        m_client{client},
        m_micWrapper{micWrapper},
        m_userInterface{userInterface},
        m_guiRenderer{guiRenderer},
        m_callManager{callManager},
#ifdef ENABLE_PCC
        m_phoneCaller{phoneCaller},
#endif
#ifdef ENABLE_MCC
        m_meetingClient{meetingClient},
        m_calendarClient{calendarClient},
#endif
        m_holdToTalkAudioProvider{holdToTalkAudioProvider},
        m_tapToTalkAudioProvider{tapToTalkAudioProvider},
        m_wakeWordAudioProvider{wakeWordAudioProvider},
#ifdef POWER_CONTROLLER
        m_powerControllerHandler{powerControllerHandler},
#endif
#ifdef TOGGLE_CONTROLLER
        m_toggleControllerHandler{toggleControllerHandler},
#endif
#ifdef RANGE_CONTROLLER
        m_rangeControllerHandler{rangeControllerHandler},
#endif
#ifdef MODE_CONTROLLER
        m_modeControllerHandler{modeControllerHandler},
#endif
        m_isHoldOccurring{false},
        m_isTapOccurring{false},
        m_isCallConnected{false},
        m_isMicOn{true},
#ifdef ENABLE_ENDPOINT_CONTROLLERS
        m_friendlyNameToggle{true},
#endif
        m_diagnostics{diagnostics} {
    if (m_wakeWordAudioProvider) {
        m_micWrapper->startStreamingMicrophoneData();
    }
};

void InteractionManager::begin() {
    m_executor.submit([this]() {
        m_userInterface->printWelcomeScreen();
        if (m_diagnostics && m_diagnostics->getAudioInjector()) {
            m_userInterface->printAudioInjectionHeader();
        }
        m_userInterface->printHelpScreen();
    });
}

void InteractionManager::help() {
    m_executor.submit([this]() { m_userInterface->printHelpScreen(); });
}

void InteractionManager::limitedHelp() {
    m_executor.submit([this]() { m_userInterface->printLimitedHelp(); });
}

void InteractionManager::settings() {
    m_executor.submit([this]() { m_userInterface->printSettingsScreen(); });
}

void InteractionManager::clearCachedEndpointIdentifiers(
    const std::vector<avsCommon::sdkInterfaces::endpoints::EndpointIdentifier>& deletedEndpoints) {
    m_executor.submit([this, deletedEndpoints]() {
        if (m_dynamicEndpointId.hasValue() &&
            (deletedEndpoints.end() !=
             std::find(deletedEndpoints.begin(), deletedEndpoints.end(), m_dynamicEndpointId.value()))) {
            m_dynamicEndpointId.reset();
        }
    });
}

#if ENABLE_ENDPOINT_CONTROLLERS
void InteractionManager::endpointModification() {
    m_executor.submit([this]() { m_userInterface->printEndpointModificationScreen(); });
}

bool InteractionManager::addEndpoint(const std::string& friendlyName) {
    auto dynamicEndpointBuilder = m_client->createEndpointBuilder();
    if (!dynamicEndpointBuilder) {
        m_userInterface->printEndpointModificationError("Create endpoint builder failed!");
        return false;
    }

    auto derivedEndpointId = "dynamic";
    dynamicEndpointBuilder->withDerivedEndpointId(derivedEndpointId)
        .withDescription("dynamic light endpoint")
        .withFriendlyName(friendlyName)
        .withManufacturerName("Amazon.com")
        .withDisplayCategory({"OTHER"});

#ifdef POWER_CONTROLLER
    auto powerHandler = PeripheralEndpointPowerControllerHandler::create(derivedEndpointId);
    if (!powerHandler) {
        m_userInterface->printEndpointModificationError("Create endpoint power controller handler failed!");
        return false;
    }
    dynamicEndpointBuilder->withPowerController(powerHandler, true, true);
#endif

    auto endpoint = dynamicEndpointBuilder->build();
    if (!endpoint) {
        m_userInterface->printEndpointModificationError("Dynamic endpoint build failed.");
        return false;
    }

    m_dynamicEndpointId.set(endpoint->getEndpointId());
    auto result = m_client->registerEndpoint(std::move(endpoint));

    if (result.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        if (result.get() != alexaClientSDK::endpoints::EndpointRegistrationManager::RegistrationResult::SUCCEEDED) {
            return false;
        }
    }

    return true;
}

void InteractionManager::addDynamicEndpoint() {
    m_executor.submit([this]() {
        if (m_dynamicEndpointId.hasValue()) {
            m_userInterface->printEndpointModificationError("Dynamic endpoint already added.");
        } else if (!addEndpoint("light")) {
            m_userInterface->printEndpointModificationError("Failed to register dynamic endpoint!");
        }
    });
}

void InteractionManager::modifyDynamicEndpoint() {
    m_executor.submit([this]() {
        if (!m_dynamicEndpointId.hasValue()) {
            m_userInterface->printEndpointModificationError("Dynamic endpoint not added yet.");
        } else {
            bool result = false;

            if (m_friendlyNameToggle) {
                result = addEndpoint("lamp");
            } else {
                result = addEndpoint("light");
            }

            if (!result) {
                m_userInterface->printEndpointModificationError("Failed to modify dynamic endpoint!");
            } else {
                m_friendlyNameToggle = !m_friendlyNameToggle;
            }
        }
    });
}

void InteractionManager::deleteDynamicEndpoint() {
    m_executor.submit([this]() {
        if (!m_dynamicEndpointId.hasValue()) {
            m_userInterface->printEndpointModificationError("Dynamic endpoint not added yet.");
        } else {
            auto result = m_client->deregisterEndpoint(m_dynamicEndpointId.value());
            if (result.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                if (result.get() !=
                    alexaClientSDK::endpoints::EndpointRegistrationManager::DeregistrationResult::SUCCEEDED) {
                    m_userInterface->printEndpointModificationError("Failed to delete dynamic endpoint!");
                }
            }
        }
    });
}

void InteractionManager::endpointController() {
    m_executor.submit([this]() { m_userInterface->printEndpointControllerScreen(); });
}
#endif

#ifdef POWER_CONTROLLER
void InteractionManager::powerController() {
    m_executor.submit([this]() { m_userInterface->printPowerControllerScreen(); });
}
#endif

#ifdef TOGGLE_CONTROLLER
void InteractionManager::toggleController() {
    m_executor.submit([this]() { m_userInterface->printToggleControllerScreen(); });
}
#endif

#ifdef MODE_CONTROLLER
void InteractionManager::modeController() {
    m_executor.submit([this]() { m_userInterface->printModeControllerScreen(); });
}
#endif

#ifdef RANGE_CONTROLLER
void InteractionManager::rangeController() {
    m_executor.submit([this]() { m_userInterface->printRangeControllerScreen(); });
}
#endif

void InteractionManager::locale() {
    m_executor.submit([this]() { m_userInterface->printLocaleScreen(); });
}

void InteractionManager::alarmVolumeRamp() {
    m_executor.submit([this]() { m_userInterface->printAlarmVolumeRampScreen(); });
}

void InteractionManager::wakewordConfirmation() {
    m_executor.submit([this]() { m_userInterface->printWakeWordConfirmationScreen(); });
}

void InteractionManager::speechConfirmation() {
    m_executor.submit([this]() { m_userInterface->printSpeechConfirmationScreen(); });
}

void InteractionManager::timeZone() {
    m_executor.submit([this]() { m_userInterface->printTimeZoneScreen(); });
}

void InteractionManager::networkInfo() {
    m_executor.submit([this]() { m_userInterface->printNetworkInfoScreen(); });
}

void InteractionManager::networkInfoConnectionTypePrompt() {
    m_executor.submit([this]() { m_userInterface->printNetworkInfoConnectionTypePrompt(); });
}

void InteractionManager::networkInfoESSIDPrompt() {
    m_executor.submit([this]() { m_userInterface->printNetworkInfoESSIDPrompt(); });
}

void InteractionManager::networkInfoBSSIDPrompt() {
    m_executor.submit([this]() { m_userInterface->printNetworkInfoBSSIDPrompt(); });
}

void InteractionManager::networkInfoIpPrompt() {
    m_executor.submit([this]() { m_userInterface->printNetworkInfoIpPrompt(); });
}

void InteractionManager::networkInfoSubnetPrompt() {
    m_executor.submit([this]() { m_userInterface->printNetworkInfoSubnetPrompt(); });
}

void InteractionManager::networkInfoMacPrompt() {
    m_executor.submit([this]() { m_userInterface->printNetworkInfoMacPrompt(); });
}

void InteractionManager::networkInfoDHCPPrompt() {
    m_executor.submit([this]() { m_userInterface->printNetworkInfoDHCPPrompt(); });
}

void InteractionManager::networkInfoStaticIpPrompt() {
    m_executor.submit([this]() { m_userInterface->printNetworkInfoStaticIpPrompt(); });
}

void InteractionManager::doNotDisturb() {
    m_executor.submit([this]() { m_userInterface->printDoNotDisturbScreen(); });
}

void InteractionManager::errorValue() {
    m_executor.submit([this]() { m_userInterface->printErrorScreen(); });
}

void InteractionManager::microphoneToggle() {
    m_executor.submit([this]() {
        if (!m_wakeWordAudioProvider) {
            return;
        }
        if (m_isMicOn) {
            m_isMicOn = false;
            if (m_micWrapper->isStreaming()) {
                m_micWrapper->stopStreamingMicrophoneData();
            }
            m_userInterface->microphoneOff();
        } else {
            m_isMicOn = true;
            if (!m_micWrapper->isStreaming() && m_wakeWordAudioProvider) {
                m_micWrapper->startStreamingMicrophoneData();
            }
            m_userInterface->microphoneOn();
        }
    });
}

void InteractionManager::holdToggled() {
    m_executor.submit([this]() {
        if (!m_isMicOn) {
            return;
        }

        if (!m_isHoldOccurring) {
            if (m_client->notifyOfHoldToTalkStart(m_holdToTalkAudioProvider).get()) {
                m_isHoldOccurring = true;
            }
        } else {
            m_isHoldOccurring = false;
            m_client->notifyOfHoldToTalkEnd();
        }
    });
}

void InteractionManager::tap() {
    m_executor.submit([this]() {
        if (!m_isMicOn) {
            return;
        }

        if (!m_isTapOccurring) {
            if (m_client->notifyOfTapToTalk(m_tapToTalkAudioProvider).get()) {
                m_isTapOccurring = true;
            }
        } else {
            m_isTapOccurring = false;
            m_client->notifyOfTapToTalkEnd();
        }
    });
}

void InteractionManager::stopForegroundActivity() {
    m_executor.submit([this]() { m_client->stopForegroundActivity(); });
}

void InteractionManager::playbackPlay() {
    m_executor.submit([this]() { m_client->getPlaybackRouter()->buttonPressed(PlaybackButton::PLAY); });
}

void InteractionManager::playbackPause() {
    m_executor.submit([this]() { m_client->getPlaybackRouter()->buttonPressed(PlaybackButton::PAUSE); });
}

void InteractionManager::playbackNext() {
    m_executor.submit([this]() { m_client->getPlaybackRouter()->buttonPressed(PlaybackButton::NEXT); });
}

void InteractionManager::playbackPrevious() {
    m_executor.submit([this]() { m_client->getPlaybackRouter()->buttonPressed(PlaybackButton::PREVIOUS); });
}

void InteractionManager::playbackSkipForward() {
    m_executor.submit([this]() { m_client->getPlaybackRouter()->buttonPressed(PlaybackButton::SKIP_FORWARD); });
}

void InteractionManager::playbackSkipBackward() {
    m_executor.submit([this]() { m_client->getPlaybackRouter()->buttonPressed(PlaybackButton::SKIP_BACKWARD); });
}

void InteractionManager::playbackShuffle() {
    sendGuiToggleEvent(GuiRenderer::TOGGLE_NAME_SHUFFLE, PlaybackToggle::SHUFFLE);
}

void InteractionManager::playbackLoop() {
    sendGuiToggleEvent(GuiRenderer::TOGGLE_NAME_LOOP, PlaybackToggle::LOOP);
}

void InteractionManager::playbackRepeat() {
    sendGuiToggleEvent(GuiRenderer::TOGGLE_NAME_REPEAT, PlaybackToggle::REPEAT);
}

void InteractionManager::playbackThumbsUp() {
    sendGuiToggleEvent(GuiRenderer::TOGGLE_NAME_THUMBSUP, PlaybackToggle::THUMBS_UP);
}

void InteractionManager::playbackThumbsDown() {
    sendGuiToggleEvent(GuiRenderer::TOGGLE_NAME_THUMBSDOWN, PlaybackToggle::THUMBS_DOWN);
}

void InteractionManager::sendGuiToggleEvent(const std::string& toggleName, PlaybackToggle toggleType) {
    bool action = false;
    if (m_guiRenderer) {
        action = !m_guiRenderer->getGuiToggleState(toggleName);
    }
    m_executor.submit(
        [this, toggleType, action]() { m_client->getPlaybackRouter()->togglePressed(toggleType, action); });
}

void InteractionManager::speakerControl() {
    m_executor.submit([this]() { m_userInterface->printSpeakerControlScreen(); });
}

void InteractionManager::firmwareVersionControl() {
    m_executor.submit([this]() { m_userInterface->printFirmwareVersionControlScreen(); });
}

void InteractionManager::setFirmwareVersion(avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion) {
    m_executor.submit([this, firmwareVersion]() { m_client->setFirmwareVersion(firmwareVersion); });
}

void InteractionManager::volumeControl() {
    m_executor.submit([this]() { m_userInterface->printVolumeControlScreen(); });
}

void InteractionManager::adjustVolume(avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type, int8_t delta) {
    m_executor.submit([this, type, delta]() {
        /*
         * Group the unmute action as part of the same affordance that caused the volume change, so we don't
         * send another event. This isn't a requirement by AVS.
         */
        std::future<bool> unmuteFuture = m_client->getSpeakerManager()->setMute(
            type,
            false,
            SpeakerManagerInterface::NotificationProperties(
                SpeakerManagerObserverInterface::Source::LOCAL_API, false, false));
        if (!unmuteFuture.valid()) {
            return;
        }
        unmuteFuture.get();

        std::future<bool> future =
            m_client->getSpeakerManager()->adjustVolume(type, delta, SpeakerManagerInterface::NotificationProperties());
        if (!future.valid()) {
            return;
        }
        future.get();
    });
}

void InteractionManager::setMute(avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type, bool mute) {
    m_executor.submit([this, type, mute]() {
        std::future<bool> future =
            m_client->getSpeakerManager()->setMute(type, mute, SpeakerManagerInterface::NotificationProperties());
        future.get();
    });
}

void InteractionManager::confirmResetDevice() {
    m_executor.submit([this]() { m_userInterface->printResetConfirmation(); });
}

void InteractionManager::resetDevice() {
    // This is a blocking operation. No interaction will be allowed during / after resetDevice
    auto result = m_executor.submit([this]() {
        m_client->getRegistrationManager()->logout();
        m_userInterface->printResetWarning();
    });
    result.wait();
}

void InteractionManager::confirmReauthorizeDevice() {
    m_executor.submit([this]() { m_userInterface->printReauthorizeConfirmation(); });
}

#ifdef ENABLE_COMMS
void InteractionManager::commsControl() {
    m_executor.submit([this]() {
        if (m_client->isCommsEnabled()) {
            m_userInterface->printCommsControlScreen();
        } else {
            m_userInterface->printCommsNotSupported();
        }
    });
}

void InteractionManager::acceptCall() {
    m_executor.submit([this]() {
        if (m_client->isCommsEnabled()) {
            m_client->acceptCommsCall();
        } else {
            m_userInterface->printCommsNotSupported();
        }
    });
}

void InteractionManager::stopCall() {
    m_executor.submit([this]() {
        if (m_client->isCommsEnabled()) {
            m_client->stopCommsCall();
        } else {
            m_userInterface->printCommsNotSupported();
        }
    });
}

void InteractionManager::muteCallToggle() {
    m_executor.submit([this]() {
        if (m_client->isCommsCallMuted()) {
            m_client->unmuteCommsCall();
            m_userInterface->printUnmuteCallScreen();
        } else {
            m_client->muteCommsCall();
            m_userInterface->printMuteCallScreen();
        }
    });
}

void InteractionManager::sendDtmf(avsCommon::sdkInterfaces::CallManagerInterface::DTMFTone dtmfTone) {
    m_executor.submit([this, dtmfTone]() {
        if (m_client->isCommsEnabled()) {
            m_client->sendDtmf(dtmfTone);
        } else {
            m_userInterface->printCommsNotSupported();
        }
    });
}

void InteractionManager::dtmfControl() {
    m_executor.submit([this]() { m_userInterface->printDtmfScreen(); });
}

void InteractionManager::errorDtmf() {
    m_executor.submit([this]() { m_userInterface->printDtmfErrorScreen(); });
}
#endif

void InteractionManager::onDialogUXStateChanged(DialogUXState state) {
    m_executor.submit([this, state]() {
        if (DialogUXState::LISTENING == state) {
            if (m_isMicOn && !m_micWrapper->isStreaming()) {
                m_micWrapper->startStreamingMicrophoneData();
            }
        } else {
            // reset tap-to-talk state
            m_isTapOccurring = false;

            // if wakeword is disabled and no call is occurring, turn off microphone
            if (!m_wakeWordAudioProvider && !m_isCallConnected && m_micWrapper->isStreaming()) {
                m_micWrapper->stopStreamingMicrophoneData();
            }
        }
    });
}

void InteractionManager::onCallStateChange(CallState state) {
    m_executor.submit([this, state]() {
        if (CallState::CALL_CONNECTED == state) {
            if (!m_micWrapper->isStreaming()) {
                m_micWrapper->startStreamingMicrophoneData();
            }
            m_isCallConnected = true;
        } else {
            // reset call state
            m_isCallConnected = false;

            // if wakeword is disabled, turn off microphone when call is not connected and tap is not occurring
            if (!m_wakeWordAudioProvider && !m_isTapOccurring && m_micWrapper->isStreaming()) {
                m_micWrapper->stopStreamingMicrophoneData();
            }
        }
    });
}

void InteractionManager::setSpeechConfirmation(settings::SpeechConfirmationSettingType value) {
    m_client->getSettingsManager()->setValue<settings::SPEECH_CONFIRMATION>(value);
}

void InteractionManager::setWakewordConfirmation(settings::WakeWordConfirmationSettingType value) {
    m_client->getSettingsManager()->setValue<settings::WAKEWORD_CONFIRMATION>(value);
}

void InteractionManager::setTimeZone(const std::string& value) {
    m_client->getSettingsManager()->setValue<settings::TIMEZONE>(value);
}

void InteractionManager::setLocale(const settings::DeviceLocales& value) {
    m_client->getSettingsManager()->setValue<settings::LOCALE>(value);
}

settings::types::NetworkInfo InteractionManager::getNetworkInfo() {
    auto netSettings = m_client->getSettingsManager()->getValue<settings::NETWORK_INFO>();
    return netSettings.second;
}

void InteractionManager::setNetworkInfo(const settings::types::NetworkInfo& value) {
    m_client->getSettingsManager()->setValue<settings::NETWORK_INFO>(value);
}

void InteractionManager::setAlarmVolumeRamp(bool enable) {
    m_client->getSettingsManager()->setValue<settings::ALARM_VOLUME_RAMP>(settings::types::toAlarmRamp(enable));
}

#ifdef POWER_CONTROLLER
void InteractionManager::setPowerState(const bool powerState) {
    m_powerControllerHandler->setPowerState(powerState);
}
#endif

#ifdef TOGGLE_CONTROLLER
void InteractionManager::setToggleState(const bool toggleState) {
    m_toggleControllerHandler->setToggleState(toggleState);
}
#endif

#ifdef RANGE_CONTROLLER
void InteractionManager::setRangeValue(const int rangeValue) {
    m_rangeControllerHandler->setRangeValue(rangeValue);
}
#endif

#ifdef MODE_CONTROLLER
void InteractionManager::setMode(const std::string mode) {
    m_modeControllerHandler->setMode(mode);
}
#endif

#ifdef ENABLE_PCC
void InteractionManager::phoneControl() {
    m_executor.submit([this]() { m_userInterface->printPhoneControlScreen(); });
}

void InteractionManager::callId() {
    m_executor.submit([this]() { m_userInterface->printCallIdScreen(); });
}

void InteractionManager::callerId() {
    m_executor.submit([this]() { m_userInterface->printCallerIdScreen(); });
}

void InteractionManager::sendCallActivated(const std::string& callId) {
    m_executor.submit([this, callId]() {
        if (m_phoneCaller) {
            m_phoneCaller->sendCallActivated(callId);
        }
    });
}
void InteractionManager::sendCallTerminated(const std::string& callId) {
    m_executor.submit([this, callId]() {
        if (m_phoneCaller) {
            m_phoneCaller->sendCallTerminated(callId);
        }
    });
}

void InteractionManager::sendCallFailed(const std::string& callId) {
    m_executor.submit([this, callId]() {
        if (m_phoneCaller) {
            m_phoneCaller->sendCallFailed(callId);
        }
    });
}

void InteractionManager::sendCallReceived(const std::string& callId, const std::string& callerId) {
    m_executor.submit([this, callId, callerId]() {
        if (m_phoneCaller) {
            m_phoneCaller->sendCallReceived(callId, callerId);
        }
    });
}

void InteractionManager::sendCallerIdReceived(const std::string& callId, const std::string& callerId) {
    m_executor.submit([this, callId, callerId]() {
        if (m_phoneCaller) {
            m_phoneCaller->sendCallerIdReceived(callId, callerId);
        }
    });
}

void InteractionManager::sendInboundRingingStarted(const std::string& callId) {
    m_executor.submit([this, callId]() {
        if (m_phoneCaller) {
            m_phoneCaller->sendInboundRingingStarted(callId);
        }
    });
}

void InteractionManager::sendOutboundCallRequested(const std::string& callId) {
    m_executor.submit([this, callId]() {
        if (m_phoneCaller) {
            m_phoneCaller->sendDialStarted(callId);
        }
    });
}

void InteractionManager::sendOutboundRingingStarted(const std::string& callId) {
    m_executor.submit([this, callId]() {
        if (m_phoneCaller) {
            m_phoneCaller->sendOutboundRingingStarted(callId);
        }
    });
}

void InteractionManager::sendSendDtmfSucceeded(const std::string& callId) {
    m_executor.submit([this, callId]() {
        if (m_phoneCaller) {
            m_phoneCaller->sendSendDtmfSucceeded(callId);
        }
    });
}

void InteractionManager::sendSendDtmfFailed(const std::string& callId) {
    m_executor.submit([this, callId]() {
        if (m_phoneCaller) {
            m_phoneCaller->sendSendDtmfFailed(callId);
        }
    });
}
#endif

#ifdef ENABLE_MCC
void InteractionManager::meetingControl() {
    m_executor.submit([this]() { m_userInterface->printMeetingControlScreen(); });
}

void InteractionManager::sessionId() {
    m_executor.submit([this]() { m_userInterface->printSessionIdScreen(); });
}

void InteractionManager::calendarItemsFile() {
    m_executor.submit([this]() { m_userInterface->printCalendarItemsScreen(); });
}

void InteractionManager::sendMeetingJoined(const std::string& sessionId) {
    m_executor.submit([this, sessionId]() {
        if (m_meetingClient) {
            m_meetingClient->sendMeetingJoined(sessionId);
        }
    });
}
void InteractionManager::sendMeetingEnded(const std::string& sessionId) {
    m_executor.submit([this, sessionId]() {
        if (m_meetingClient) {
            m_meetingClient->sendMeetingEnded(sessionId);
        }
    });
}

void InteractionManager::sendSetCurrentMeetingSession(const std::string& sessionId) {
    m_executor.submit([this, sessionId]() {
        if (m_meetingClient) {
            m_meetingClient->sendSetCurrentMeetingSession(sessionId);
        }
    });
}

void InteractionManager::sendClearCurrentMeetingSession() {
    m_executor.submit([this]() {
        if (m_meetingClient) {
            m_meetingClient->sendClearCurrentMeetingSession();
        }
    });
}

void InteractionManager::sendConferenceConfigurationChanged() {
    m_executor.submit([this]() {
        if (m_meetingClient) {
            m_meetingClient->sendConferenceConfigurationChanged();
        }
    });
}

void InteractionManager::sendMeetingClientErrorOccured(const std::string& sessionId) {
    m_executor.submit([this, sessionId]() {
        if (m_meetingClient) {
            m_meetingClient->sendMeetingClientErrorOccured(sessionId);
        }
    });
}

void InteractionManager::sendCalendarItemsRetrieved(const std::string& calendarItemsFile) {
    m_executor.submit([this, calendarItemsFile]() {
        if (m_calendarClient) {
            m_calendarClient->sendCalendarItemsRetrieved(calendarItemsFile);
        }
    });
}

void InteractionManager::sendCalendarClientErrorOccured() {
    m_executor.submit([this]() {
        if (m_calendarClient) {
            m_calendarClient->sendCalendarClientErrorOccured();
        }
    });
}
#endif

void InteractionManager::setDoNotDisturbMode(bool enable) {
    m_client->getSettingsManager()->setValue<settings::DO_NOT_DISTURB>(enable);
}

void InteractionManager::diagnosticsControl() {
    m_executor.submit([this]() { m_userInterface->printDiagnosticsScreen(); });
}

void InteractionManager::devicePropertiesControl() {
    m_executor.submit([this]() { m_userInterface->printDevicePropertiesScreen(); });
}

void InteractionManager::showDeviceProperties() {
    m_executor.submit([this]() {
        if (m_diagnostics) {
            auto deviceProperties = m_diagnostics->getDevicePropertyAggregator();
            if (deviceProperties) {
                m_userInterface->printAllDeviceProperties(deviceProperties->getAllDeviceProperties());
            }
        }
    });
}

void InteractionManager::audioInjectionControl() {
    m_executor.submit([this]() { m_userInterface->printAudioInjectionScreen(); });
}

void InteractionManager::injectWavFile(const std::string& absoluteFilePath) {
    m_executor.submit([this, absoluteFilePath]() {
        if (!m_diagnostics) {
            ACSDK_ERROR(LX("audioInjectionFailed").d("reason", "nullDiagnosticObject"));
            m_userInterface->printAudioInjectionFailureMessage();
            return;
        }
        auto audioInjector = m_diagnostics->getAudioInjector();
        if (!audioInjector) {
            ACSDK_ERROR(LX("audioInjectionFailed").d("reason", "nullAudioInjector"));
            m_userInterface->printAudioInjectionFailureMessage();
            return;
        }

        // Notify DefaultClient of tap-to-talk if wakeword is disabled.
        if (!m_wakeWordAudioProvider) {
            if (!m_client->notifyOfTapToTalk(m_tapToTalkAudioProvider).get()) {
                m_userInterface->printAudioInjectionFailureMessage();
                return;
            }
        }

        if (!audioInjector->injectAudio(absoluteFilePath)) {
            m_userInterface->printAudioInjectionFailureMessage();
            return;
        }
    });
}

void InteractionManager::deviceProtocolTraceControl() {
    m_executor.submit([this]() { m_userInterface->printDeviceProtocolTracerScreen(); });
}

void InteractionManager::printProtocolTrace() {
    m_executor.submit([this]() {
        if (m_diagnostics) {
            auto protocolTrace = m_diagnostics->getProtocolTracer();
            if (protocolTrace) {
                m_userInterface->printProtocolTrace(protocolTrace->getProtocolTrace());
            }
        }
    });
}

void InteractionManager::setProtocolTraceFlag(bool enabled) {
    m_executor.submit([this, enabled]() {
        if (m_diagnostics) {
            auto protocolTrace = m_diagnostics->getProtocolTracer();
            if (protocolTrace) {
                protocolTrace->setProtocolTraceFlag(enabled);
                m_userInterface->printProtocolTraceFlag(enabled);
            }
        }
    });
}

void InteractionManager::clearProtocolTrace() {
    m_executor.submit([this]() {
        if (m_diagnostics) {
            auto protocolTrace = m_diagnostics->getProtocolTracer();
            if (protocolTrace) {
                protocolTrace->clearTracedMessages();
            }
        }
    });
}

void InteractionManager::startMicrophone() {
    m_micWrapper->startStreamingMicrophoneData();
}

void InteractionManager::stopMicrophone() {
    m_micWrapper->stopStreamingMicrophoneData();
}

void InteractionManager::doShutdown() {
    m_client.reset();
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
