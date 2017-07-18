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

#include "SampleApp/InteractionManager.h"

namespace alexaClientSDK {
namespace sampleApp {

InteractionManager::InteractionManager(
        std::shared_ptr<defaultClient::DefaultClient> client, 
        std::shared_ptr<sampleApp::PortAudioMicrophoneWrapper> micWrapper,
        std::shared_ptr<sampleApp::UIManager> userInterface,
        capabilityAgents::aip::AudioProvider holdToTalkAudioProvider,
        capabilityAgents::aip::AudioProvider tapToTalkAudioProvider,
        capabilityAgents::aip::AudioProvider wakeWordAudioProvider) : 
    m_client{client}, 
    m_micWrapper{micWrapper}, 
    m_userInterface{userInterface},
    m_holdToTalkAudioProvider{holdToTalkAudioProvider},
    m_tapToTalkAudioProvider{tapToTalkAudioProvider},
    m_wakeWordAudioProvider{wakeWordAudioProvider},
    m_isHoldOccurring{false},
    m_isTapOccurring{false},
    m_isMicOn{false} { 
    if (m_wakeWordAudioProvider) {
        m_isMicOn = true;
        m_micWrapper->startStreamingMicrophoneData();
    }
};

void InteractionManager::begin() {
    m_executor.submit(
        [this] () {
            m_userInterface->printWelcomeScreen();
            m_userInterface->printHelpScreen();
        }
    );
}

void InteractionManager::help() {
    m_executor.submit(
        [this] () {
            m_userInterface->printHelpScreen();
        }
    );
}

void InteractionManager::onDialogUXStateChanged(
        avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState state) {
    m_executor.submit(
        [this, state] () {
            /*
             * This is an optimization that stops the microphone from continuously streaming audio data into the buffer
             * when a tap to talk recognize finishes. This isn't strictly necessary, as the SDK will not consume the
             * audio data when not in the middle of an active recognition.
             */
            if (avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::IDLE == state && 
                    m_isTapOccurring) {
                m_isTapOccurring = false;
                if (!m_wakeWordAudioProvider || (m_wakeWordAudioProvider && !m_isMicOn)) {
                    m_micWrapper->stopStreamingMicrophoneData();
                }
            }
        }
    );
}

void InteractionManager::microphoneToggle() {
    m_executor.submit(
        [this] () {
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
        }
    );
}

void InteractionManager::holdToggled() {
    m_executor.submit(
        [this] () {
            if (!m_isHoldOccurring) {
                if (m_client->notifyOfHoldToTalkStart(m_holdToTalkAudioProvider).get()) {
                    m_isHoldOccurring = true;
                    m_micWrapper->startStreamingMicrophoneData();
                }
            } else {
                m_isHoldOccurring = false;
                m_client->notifyOfHoldToTalkEnd();
                if (!m_wakeWordAudioProvider || (m_wakeWordAudioProvider && !m_isMicOn)) {
                    m_micWrapper->stopStreamingMicrophoneData();
                }
            }
        }
    );
}

void InteractionManager::tap() {
    m_executor.submit(
        [this] () {
            if (m_client->notifyOfTapToTalk(m_tapToTalkAudioProvider).get()) {
                m_isTapOccurring = true;
                m_micWrapper->startStreamingMicrophoneData();
            }
        }
    );
}

void InteractionManager::stopForegroundActivity() {
    m_executor.submit(
        [this] () {
            m_client->stopForegroundActivity();
        }
    );
}

} // namespace sampleApp
} // namespace alexaClientSDK