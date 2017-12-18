/*
 * InteractionManager.cpp
 *
 * Copyright (c) 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "WebExtension/InteractionManager.h"

namespace alexaClientSDK {
namespace webExtension {

InteractionManager::InteractionManager(
    std::shared_ptr<defaultClient::DefaultClient> client,
    std::shared_ptr<webExtension::PortAudioMicrophoneWrapper> micWrapper,
    std::shared_ptr<webExtension::UIManager> userInterface,
    capabilityAgents::aip::AudioProvider holdToTalkAudioProvider,
    capabilityAgents::aip::AudioProvider tapToTalkAudioProvider,
    capabilityAgents::aip::AudioProvider wakeWordAudioProvider) :
        RequiresShutdown{"InteractionManager"},
        m_client{client},
        m_micWrapper{micWrapper},
        m_userInterface{userInterface},
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
    m_executor.submit([this]() { m_client->getPlaybackControllerInterface().playButtonPressed(); });
}

void InteractionManager::playbackPause() {
    m_executor.submit([this]() { m_client->getPlaybackControllerInterface().pauseButtonPressed(); });
}

void InteractionManager::playbackNext() {
    m_executor.submit([this]() { m_client->getPlaybackControllerInterface().nextButtonPressed(); });
}

void InteractionManager::playbackPrevious() {
    m_executor.submit([this]() { m_client->getPlaybackControllerInterface().previousButtonPressed(); });
}

void InteractionManager::speakerControl() {
    m_executor.submit([this]() { m_userInterface->printSpeakerControlScreen(); });
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

void InteractionManager::onDialogUXStateChanged(DialogUXState state) {
    // reset tap-to-talk state
    if (DialogUXState::LISTENING != state) {
        m_isTapOccurring = false;
    }
}

void InteractionManager::doShutdown() {
    m_client.reset();
}

}  // namespace webExtension
}  // namespace alexaClientSDK
