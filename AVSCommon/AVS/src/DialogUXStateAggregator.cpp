/*
 * DialogUXStateAggregator.cpp
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "AVSCommon/AVS/DialogUXStateAggregator.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

using namespace sdkInterfaces;

/// String to identify log entries originating from this file.
static const std::string TAG("DialogUXStateAggregator");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * A short timeout that is used to avoid going to the IDLE state immediately while waiting for other state changes.
 */
static const std::chrono::milliseconds SHORT_TIMEOUT{200};

DialogUXStateAggregator::DialogUXStateAggregator(std::chrono::milliseconds timeoutForThinkingToIdle) :
        m_currentState{DialogUXStateObserverInterface::DialogUXState::IDLE},
        m_timeoutForThinkingToIdle{timeoutForThinkingToIdle} {
}

void DialogUXStateAggregator::addObserver(std::shared_ptr<DialogUXStateObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("addObserverFailed").d("reason", "nullObserver"));
        return;
    }
    m_executor.submit([this, observer]() {
        m_observers.insert(observer);
        observer->onDialogUXStateChanged(m_currentState);
    });
}

void DialogUXStateAggregator::removeObserver(std::shared_ptr<DialogUXStateObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("removeObserverFailed").d("reason", "nullObserver"));
        return;
    }
    m_executor.submit([this, observer]() { m_observers.erase(observer); }).wait();
}

void DialogUXStateAggregator::onStateChanged(AudioInputProcessorObserverInterface::State state) {
    m_executor.submit([this, state]() {
        switch (state) {
            case AudioInputProcessorObserverInterface::State::IDLE:
                setState(DialogUXStateObserverInterface::DialogUXState::IDLE);
                return;
            case AudioInputProcessorObserverInterface::State::RECOGNIZING:
                setState(DialogUXStateObserverInterface::DialogUXState::LISTENING);
                return;
            case AudioInputProcessorObserverInterface::State::EXPECTING_SPEECH:
                setState(DialogUXStateObserverInterface::DialogUXState::LISTENING);
                return;
            case AudioInputProcessorObserverInterface::State::BUSY:
                setState(DialogUXStateObserverInterface::DialogUXState::THINKING);
                if (!m_thinkingToIdleTimer
                         .start(
                             m_timeoutForThinkingToIdle,
                             std::bind(&DialogUXStateAggregator::transitionFromThinkingTimedOut, this))
                         .valid()) {
                    ACSDK_ERROR(LX("failedToStartTimerFromThinkingToIdle"));
                }
                return;
        }
        ACSDK_ERROR(LX("unknownAudioInputProcessorState"));
    });
}

void DialogUXStateAggregator::onStateChanged(SpeechSynthesizerObserver::SpeechSynthesizerState state) {
    m_executor.submit([this, state]() {
        switch (state) {
            case SpeechSynthesizerObserver::SpeechSynthesizerState::PLAYING:
                setState(DialogUXStateObserverInterface::DialogUXState::SPEAKING);
                return;
            case SpeechSynthesizerObserver::SpeechSynthesizerState::FINISHED:
                if (DialogUXStateObserverInterface::DialogUXState::SPEAKING != m_currentState) {
                    return;
                }

                m_currentState = DialogUXStateObserverInterface::DialogUXState::FINISHED;

                if (!m_multiturnSpeakingToListeningTimer
                         .start(
                             SHORT_TIMEOUT, std::bind(&DialogUXStateAggregator::transitionFromSpeakingFinished, this))
                         .valid()) {
                    ACSDK_ERROR(LX("failedToStartTimerFromSpeakingFinishedToIdle"));
                }
                return;
        }
        ACSDK_ERROR(LX("unknownSpeechSynthesizerState"));
    });
}

void DialogUXStateAggregator::receive(const std::string& contextId, const std::string& message) {
    m_executor.submit([this]() {
        if (DialogUXStateObserverInterface::DialogUXState::THINKING == m_currentState) {
            /*
             * Stop the long timer and start a short timer so that either the state will change (i.e. Speech begins)
             * or we automatically go to idle after the short timeout (i.e. the directive received isn't related to
             * speech, like a setVolume directive).
             */
            m_thinkingToIdleTimer.stop();
            m_thinkingToIdleTimer.start(
                SHORT_TIMEOUT, std::bind(&DialogUXStateAggregator::transitionFromThinkingTimedOut, this));
        }
    });
}

void DialogUXStateAggregator::onConnectionStatusChanged(
    const ConnectionStatusObserverInterface::Status status,
    const ConnectionStatusObserverInterface::ChangedReason reason) {
    m_executor.submit([this, &status]() {
        if (status != avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status::CONNECTED) {
            setState(DialogUXStateObserverInterface::DialogUXState::IDLE);
        }
    });
}

void DialogUXStateAggregator::notifyObserversOfState() {
    for (auto observer : m_observers) {
        if (observer) {
            observer->onDialogUXStateChanged(m_currentState);
        }
    }
}

void DialogUXStateAggregator::transitionFromThinkingTimedOut() {
    m_executor.submit([this]() {
        if (DialogUXStateObserverInterface::DialogUXState::THINKING == m_currentState) {
            ACSDK_DEBUG(LX("transitionFromThinkingTimedOut"));
            setState(DialogUXStateObserverInterface::DialogUXState::IDLE);
        }
    });
}

void DialogUXStateAggregator::transitionFromSpeakingFinished() {
    m_executor.submit([this]() {
        if (DialogUXStateObserverInterface::DialogUXState::FINISHED == m_currentState) {
            setState(DialogUXStateObserverInterface::DialogUXState::IDLE);
        }
    });
}

void DialogUXStateAggregator::setState(sdkInterfaces::DialogUXStateObserverInterface::DialogUXState newState) {
    if (newState == m_currentState) {
        return;
    }
    m_thinkingToIdleTimer.stop();
    m_multiturnSpeakingToListeningTimer.stop();
    ACSDK_DEBUG(LX("setState").d("from", m_currentState).d("to", newState));
    m_currentState = newState;
    notifyObserversOfState();
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
