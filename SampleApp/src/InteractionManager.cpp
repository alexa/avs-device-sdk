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

#include "ESP/ESPDataProviderInterface.h"
#include "RegistrationManager/CustomerDataManager.h"
#include "SampleApp/InteractionManager.h"

#ifdef ENABLE_PCC
#include <SampleApp/PhoneCaller.h>
#endif

namespace alexaClientSDK {
namespace sampleApp {

using namespace avsCommon::avs;

InteractionManager::InteractionManager(
    std::shared_ptr<defaultClient::DefaultClient> client,
    std::shared_ptr<applicationUtilities::resources::audio::MicrophoneInterface> micWrapper,
    std::shared_ptr<sampleApp::UIManager> userInterface,
#ifdef ENABLE_PCC
    std::shared_ptr<sampleApp::PhoneCaller> phoneCaller,
#endif
    capabilityAgents::aip::AudioProvider holdToTalkAudioProvider,
    capabilityAgents::aip::AudioProvider tapToTalkAudioProvider,
    std::shared_ptr<sampleApp::GuiRenderer> guiRenderer,
    capabilityAgents::aip::AudioProvider wakeWordAudioProvider,
    std::shared_ptr<esp::ESPDataProviderInterface> espProvider,
    std::shared_ptr<esp::ESPDataModifierInterface> espModifier,
    std::shared_ptr<avsCommon::sdkInterfaces::CallManagerInterface> callManager) :
        RequiresShutdown{"InteractionManager"},
        m_client{client},
        m_micWrapper{micWrapper},
        m_userInterface{userInterface},
        m_guiRenderer{guiRenderer},
        m_espProvider{espProvider},
        m_espModifier{espModifier},
        m_callManager{callManager},
#ifdef ENABLE_PCC
        m_phoneCaller{phoneCaller},
#endif
        m_holdToTalkAudioProvider{holdToTalkAudioProvider},
        m_tapToTalkAudioProvider{tapToTalkAudioProvider},
        m_wakeWordAudioProvider{wakeWordAudioProvider},
        m_isHoldOccurring{false},
        m_isTapOccurring{false},
        m_isMicOn{true} {
    m_micWrapper->startStreamingMicrophoneData();
};

void InteractionManager::begin() {
    m_executor.submit([this]() {
        m_userInterface->printWelcomeScreen();
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

void InteractionManager::locale() {
    m_executor.submit([this]() { m_userInterface->printLocaleScreen(); });
}

void InteractionManager::doNotDisturb() {
    m_executor.submit([this]() { m_userInterface->printDoNotDisturbScreen(); });
}

void InteractionManager::errorValue() {
    m_executor.submit([this]() { m_userInterface->printErrorScreen(); });
}

void InteractionManager::changeSetting(const std::string& key, const std::string& value) {
    m_executor.submit([this, key, value]() { m_client->changeSetting(key, value); });
}

void InteractionManager::microphoneToggle() {
    m_executor.submit([this]() {
        if (!m_wakeWordAudioProvider) {
            return;
        }
        if (m_isMicOn) {
            m_isMicOn = false;
            m_micWrapper->stopStreamingMicrophoneData();
            m_userInterface->microphoneOff();
        } else {
            m_isMicOn = true;
            m_micWrapper->startStreamingMicrophoneData();
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

void InteractionManager::adjustVolume(avsCommon::sdkInterfaces::SpeakerInterface::Type type, int8_t delta) {
    m_executor.submit([this, type, delta]() {
        /*
         * Group the unmute action as part of the same affordance that caused the volume change, so we don't
         * send another event. This isn't a requirement by AVS.
         */
        std::future<bool> unmuteFuture = m_client->getSpeakerManager()->setMute(type, false, true);
        if (!unmuteFuture.valid()) {
            return;
        }
        unmuteFuture.get();

        std::future<bool> future = m_client->getSpeakerManager()->adjustVolume(type, delta);
        if (!future.valid()) {
            return;
        }
        future.get();
    });
}

void InteractionManager::setMute(avsCommon::sdkInterfaces::SpeakerInterface::Type type, bool mute) {
    m_executor.submit([this, type, mute]() {
        std::future<bool> future = m_client->getSpeakerManager()->setMute(type, mute);
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

void InteractionManager::espControl() {
    m_executor.submit([this]() {
        if (m_espProvider) {
            auto espData = m_espProvider->getESPData();
            m_userInterface->printESPControlScreen(
                m_espProvider->isEnabled(), espData.getVoiceEnergy(), espData.getAmbientEnergy());
        } else {
            m_userInterface->printESPNotSupported();
        }
    });
}

void InteractionManager::toggleESPSupport() {
    m_executor.submit([this]() {
        if (m_espProvider) {
            m_espProvider->isEnabled() ? m_espProvider->disable() : m_espProvider->enable();
        } else {
            m_userInterface->printESPNotSupported();
        }
    });
}

void InteractionManager::setESPVoiceEnergy(const std::string& voiceEnergy) {
    m_executor.submit([this, voiceEnergy]() {
        if (m_espProvider) {
            if (m_espModifier) {
                m_espModifier->setVoiceEnergy(voiceEnergy);
            } else {
                m_userInterface->printESPDataOverrideNotSupported();
            }
        } else {
            m_userInterface->printESPNotSupported();
        }
    });
}

void InteractionManager::setESPAmbientEnergy(const std::string& ambientEnergy) {
    m_executor.submit([this, ambientEnergy]() {
        if (m_espProvider) {
            if (m_espModifier) {
                m_espModifier->setAmbientEnergy(ambientEnergy);
            } else {
                m_userInterface->printESPDataOverrideNotSupported();
            }
        } else {
            m_userInterface->printESPNotSupported();
        }
    });
}

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

void InteractionManager::onDialogUXStateChanged(DialogUXState state) {
    // reset tap-to-talk state
    if (DialogUXState::LISTENING != state) {
        m_isTapOccurring = false;
    }
}

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

void InteractionManager::setDoNotDisturbMode(bool enable) {
    m_client->getSettingsManager()->setValue<settings::DO_NOT_DISTURB>(enable);
}

void InteractionManager::doShutdown() {
    m_client.reset();
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
