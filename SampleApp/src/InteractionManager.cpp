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
#include "SampleApp/InteractionManager.h"
#include "RegistrationManager/CustomerDataManager.h"

namespace alexaClientSDK {
namespace sampleApp {

InteractionManager::InteractionManager(
    std::shared_ptr<defaultClient::DefaultClient> client,
    std::shared_ptr<sampleApp::PortAudioMicrophoneWrapper> micWrapper,
    std::shared_ptr<sampleApp::UIManager> userInterface,
    capabilityAgents::aip::AudioProvider holdToTalkAudioProvider,
    capabilityAgents::aip::AudioProvider tapToTalkAudioProvider,
    capabilityAgents::aip::AudioProvider wakeWordAudioProvider,
    std::shared_ptr<esp::ESPDataProviderInterface> espProvider,
    std::shared_ptr<esp::ESPDataModifierInterface> espModifier) :
        RequiresShutdown{"InteractionManager"},
        m_client{client},
        m_micWrapper{micWrapper},
        m_userInterface{userInterface},
        m_espProvider{espProvider},
        m_espModifier{espModifier},
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

void InteractionManager::settings() {
    m_executor.submit([this]() { m_userInterface->printSettingsScreen(); });
}

void InteractionManager::locale() {
    m_executor.submit([this]() { m_userInterface->printLocaleScreen(); });
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
    m_executor.submit([this]() { m_client->getPlaybackRouter()->playButtonPressed(); });
}

void InteractionManager::playbackPause() {
    m_executor.submit([this]() { m_client->getPlaybackRouter()->pauseButtonPressed(); });
}

void InteractionManager::playbackNext() {
    m_executor.submit([this]() { m_client->getPlaybackRouter()->nextButtonPressed(); });
}

void InteractionManager::playbackPrevious() {
    m_executor.submit([this]() { m_client->getPlaybackRouter()->previousButtonPressed(); });
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

void InteractionManager::onDialogUXStateChanged(DialogUXState state) {
    // reset tap-to-talk state
    if (DialogUXState::LISTENING != state) {
        m_isTapOccurring = false;
    }
}

void InteractionManager::doShutdown() {
    m_client.reset();
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
