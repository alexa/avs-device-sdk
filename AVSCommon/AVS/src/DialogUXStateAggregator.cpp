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
#include <AVSCommon/Utils/Metrics/DataPointCounterBuilder.h>
#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>

#include "AVSCommon/AVS/DialogUXStateAggregator.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

using namespace sdkInterfaces;
using namespace avsCommon::utils::metrics;

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

/// Custom Metrics prefix used by DialogUXStateAggregator.
static const std::string CUSTOM_METRIC_PREFIX = "CUSTOM-";

/// error metric for Listening timeout expires
static const std::string LISTENING_TIMEOUT_EXPIRES = "LISTENING_TIMEOUT_EXPIRES";

/// error metric for Thinking timeout expires
static const std::string THINKING_TIMEOUT_EXPIRES = "THINKING_TIMEOUT_EXPIRES";

/**
 * Submits a metric of given event name
 * @param metricRecorder The @c MetricRecorderInterface which records Metric events
 * @param eventName The name of the metric event
 */
static void submitMetric(const std::shared_ptr<MetricRecorderInterface>& metricRecorder, const std::string& eventName) {
    if (!metricRecorder) {
        return;
    }

    auto metricEvent = MetricEventBuilder{}
                           .setActivityName(CUSTOM_METRIC_PREFIX + eventName)
                           .addDataPoint(DataPointCounterBuilder{}.setName(eventName).increment(1).build())
                           .build();

    if (metricEvent == nullptr) {
        ACSDK_ERROR(LX("Error creating metric."));
        return;
    }
    recordMetric(metricRecorder, metricEvent);
}

DialogUXStateAggregator::DialogUXStateAggregator(
    std::shared_ptr<MetricRecorderInterface> metricRecorder,
    std::chrono::milliseconds timeoutForThinkingToIdle,
    std::chrono::milliseconds timeoutForListeningToIdle) :
        m_metricRecorder{metricRecorder},
        m_currentState{DialogUXStateObserverInterface::DialogUXState::IDLE},
        m_timeoutForThinkingToIdle{timeoutForThinkingToIdle},
        m_timeoutForListeningToIdle{timeoutForListeningToIdle},
        m_speechSynthesizerState{SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED},
        m_audioInputProcessorState{AudioInputProcessorObserverInterface::State::IDLE} {
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
    m_audioInputProcessorState = state;

    m_executor.submit([this, state]() {
        switch (state) {
            case AudioInputProcessorObserverInterface::State::IDLE:
                tryEnterIdleState();
                return;
            case AudioInputProcessorObserverInterface::State::RECOGNIZING:
                onActivityStarted();
                setState(DialogUXStateObserverInterface::DialogUXState::LISTENING);
                return;
            case AudioInputProcessorObserverInterface::State::EXPECTING_SPEECH:
                onActivityStarted();
                setState(DialogUXStateObserverInterface::DialogUXState::EXPECTING);
                return;
            case AudioInputProcessorObserverInterface::State::BUSY:
                setState(DialogUXStateObserverInterface::DialogUXState::LISTENING);
                if (!m_listeningTimeoutTimer
                         .start(
                             m_timeoutForListeningToIdle,
                             std::bind(&DialogUXStateAggregator::transitionFromListeningTimedOut, this))
                         .valid()) {
                    ACSDK_ERROR(LX("failedToStartTimerFromListeningToIdle"));
                }
                return;
        }
        ACSDK_ERROR(LX("unknownAudioInputProcessorState"));
    });
}

void DialogUXStateAggregator::onStateChanged(
    SpeechSynthesizerObserverInterface::SpeechSynthesizerState state,
    const avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId mediaSourceId,
    const avsCommon::utils::Optional<avsCommon::utils::mediaPlayer::MediaPlayerState>& mediaPlayerState,
    const std::vector<avsCommon::utils::audioAnalyzer::AudioAnalyzerState>& audioAnalyzerState) {
    m_speechSynthesizerState = state;

    m_executor.submit([this, state]() {
        switch (state) {
            case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING:
                onActivityStarted();
                setState(DialogUXStateObserverInterface::DialogUXState::SPEAKING);
                return;
            case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED:
            case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::INTERRUPTED:
                tryEnterIdleState();
                return;
            case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::LOSING_FOCUS:
                return;
            case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::GAINING_FOCUS:
                onActivityStarted();
                return;
        }
        ACSDK_ERROR(LX("unknownSpeechSynthesizerState"));
    });
}

void DialogUXStateAggregator::receive(const std::string& contextId, const std::string& message) {
    m_executor.submit([this]() {
        if (DialogUXStateObserverInterface::DialogUXState::THINKING == m_currentState &&
            SpeechSynthesizerObserverInterface::SpeechSynthesizerState::GAINING_FOCUS != m_speechSynthesizerState) {
            /*
             * Stop the long timer and start a short timer so that either the state will change (i.e. Speech begins)
             * or we automatically go to idle after the short timeout (i.e. the directive received isn't related to
             * speech, like a setVolume directive). Cannot automatically goto IDLE because it will cause
             * SpeechSynthesizer to release focus, which may happen before the Speak has rendered.
             */
            m_thinkingTimeoutTimer.stop();
            m_thinkingTimeoutTimer.start(
                SHORT_TIMEOUT, std::bind(&DialogUXStateAggregator::transitionFromThinkingTimedOut, this));
        }
    });
}

void DialogUXStateAggregator::onConnectionStatusChanged(
    const ConnectionStatusObserverInterface::Status status,
    const ConnectionStatusObserverInterface::ChangedReason reason) {
    m_executor.submit([this, status]() {
        if (status != avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status::CONNECTED) {
            setState(DialogUXStateObserverInterface::DialogUXState::IDLE);
        }
    });
}

void DialogUXStateAggregator::onRequestProcessingStarted() {
    ACSDK_DEBUG(LX("onRequestProcessingStarted"));
    m_executor.submit([this]() {
        // Stop the listening timer
        m_listeningTimeoutTimer.stop();

        ACSDK_DEBUG0(LX("onRequestProcessingStartedLambda").d("currentState", m_currentState));

        switch (m_currentState) {
            // IDLE is included for the theoretical edgecase that RPS is received after the listening timeout occurs.
            case DialogUXStateObserverInterface::DialogUXState::IDLE:
                ACSDK_WARN(LX("onRequestProcessingStartedLambda").d("reason", "transitioningFromIdle"));
            /* FALL-THROUGH */
            case DialogUXStateObserverInterface::DialogUXState::LISTENING:
                setState(DialogUXStateObserverInterface::DialogUXState::THINKING);

                if (!m_thinkingTimeoutTimer
                         .start(
                             m_timeoutForThinkingToIdle,
                             std::bind(&DialogUXStateAggregator::transitionFromThinkingTimedOut, this))
                         .valid()) {
                    ACSDK_ERROR(LX("failedToStartTimerFromThinkingToIdle"));
                }
                break;
            default:
                ACSDK_ERROR(LX("onRequestProcessingStartedLambda")
                                .d("reason", "invalidState")
                                .d("currentState", m_currentState));
        }
    });
}

void DialogUXStateAggregator::onRequestProcessingCompleted() {
    // No-op
    /*
     * No particular processing is needed for this directive. The RequestProcessCompleted directive exists in the
     * Interaction Model 1.1 to let AVS activate a logic that stops the thinking mode without any other semantic. But
     * the specification is such that any directive will interrupt the thinking mode. So here we are simply confirming
     * that this directive is supported.
     */
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

            submitMetric(m_metricRecorder, THINKING_TIMEOUT_EXPIRES);
        }
    });
}

void DialogUXStateAggregator::transitionFromListeningTimedOut() {
    m_executor.submit([this]() {
        if (DialogUXStateObserverInterface::DialogUXState::LISTENING == m_currentState) {
            ACSDK_DEBUG(LX("transitionFromListeningTimedOut"));
            setState(DialogUXStateObserverInterface::DialogUXState::IDLE);

            submitMetric(m_metricRecorder, LISTENING_TIMEOUT_EXPIRES);
        }
    });
}

void DialogUXStateAggregator::tryEnterIdleStateOnTimer() {
    m_executor.submit([this]() {
        if (m_currentState != sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::IDLE &&
            m_audioInputProcessorState == AudioInputProcessorObserverInterface::State::IDLE &&
            (m_speechSynthesizerState == SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED ||
             m_speechSynthesizerState == SpeechSynthesizerObserverInterface::SpeechSynthesizerState::INTERRUPTED)) {
            setState(sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::IDLE);
        }
    });
}

void DialogUXStateAggregator::setState(sdkInterfaces::DialogUXStateObserverInterface::DialogUXState newState) {
    if (newState == m_currentState) {
        return;
    }
    m_listeningTimeoutTimer.stop();
    m_thinkingTimeoutTimer.stop();
    m_multiturnSpeakingToListeningTimer.stop();
    ACSDK_DEBUG(LX("setState").d("from", m_currentState).d("to", newState));
    m_currentState = newState;
    notifyObserversOfState();
}

void DialogUXStateAggregator::tryEnterIdleState() {
    ACSDK_DEBUG5(LX(__func__));
    m_thinkingTimeoutTimer.stop();
    m_multiturnSpeakingToListeningTimer.stop();
    if (!m_multiturnSpeakingToListeningTimer
             .start(SHORT_TIMEOUT, std::bind(&DialogUXStateAggregator::tryEnterIdleStateOnTimer, this))
             .valid()) {
        ACSDK_ERROR(LX("failedToStartTryEnterIdleStateTimer"));
    }
}

void DialogUXStateAggregator::onActivityStarted() {
    m_listeningTimeoutTimer.stop();
    m_thinkingTimeoutTimer.stop();
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
