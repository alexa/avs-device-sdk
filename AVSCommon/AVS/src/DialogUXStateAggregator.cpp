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

/// Custom Metrics prefix used by DialogUXStateAggregator.
static const std::string CUSTOM_METRIC_PREFIX = "CUSTOM-";

/// error metric for Listening timeout expires
static const std::string LISTENING_TIMEOUT_EXPIRES = "LISTENING_TIMEOUT_EXPIRES";

/// error metric for Thinking timeout expires
static const std::string THINKING_TIMEOUT_EXPIRES = "THINKING_TIMEOUT_EXPIRES";

constexpr std::chrono::milliseconds DialogUXStateAggregator::SHORT_TIMEOUT_FOR_THINKING_TO_IDLE;
constexpr std::chrono::seconds DialogUXStateAggregator::LONG_TIMEOUT_FOR_THINKING_TO_IDLE;
constexpr std::chrono::seconds DialogUXStateAggregator::LONG_TIMEOUT_FOR_LISTENING_TO_IDLE;

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
    std::chrono::milliseconds timeoutForListeningToIdle,
    std::chrono::milliseconds shortTimeoutForThinkingToIdle) :
        m_metricRecorder{metricRecorder},
        m_currentState{DialogUXStateObserverInterface::DialogUXState::IDLE},
        m_timeoutForThinkingToIdle{timeoutForThinkingToIdle},
        m_shortTimeoutForThinkingToIdle{shortTimeoutForThinkingToIdle},
        m_timeoutForListeningToIdle{timeoutForListeningToIdle},
        m_speechSynthesizerState{SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED},
        m_audioInputProcessorState{AudioInputProcessorObserverInterface::State::IDLE} {
    ACSDK_DEBUG8(LX("timeout values in milliseconds")
                     .d("m_timeoutForThinkingToIdle", m_timeoutForThinkingToIdle.count())
                     .d("m_shortTimeoutForThinkingToIdle", m_shortTimeoutForThinkingToIdle.count())
                     .d("m_timeoutForListeningToIdle", m_timeoutForListeningToIdle.count()));
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
                executeSetState(DialogUXStateObserverInterface::DialogUXState::LISTENING);
                return;
            case AudioInputProcessorObserverInterface::State::EXPECTING_SPEECH:
                onActivityStarted();
                executeSetState(DialogUXStateObserverInterface::DialogUXState::EXPECTING);
                return;
            case AudioInputProcessorObserverInterface::State::BUSY:
                if (executeSetState(DialogUXStateObserverInterface::DialogUXState::LISTENING) ||
                    DialogUXStateObserverInterface::DialogUXState::LISTENING == m_currentState) {
                    if (!m_listeningTimeoutTimer
                             .start(
                                 m_timeoutForListeningToIdle,
                                 std::bind(&DialogUXStateAggregator::transitionFromListeningTimedOut, this))
                             .valid()) {
                        ACSDK_ERROR(LX("failedToStartTimerFromListeningToIdle"));
                    }
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
                executeSetState(DialogUXStateObserverInterface::DialogUXState::SPEAKING);
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
    tryExitThinkingState();
}

void DialogUXStateAggregator::tryExitThinkingState() {
    m_executor.submit([this]() {
        if (DialogUXStateObserverInterface::DialogUXState::THINKING == m_currentState &&
            SpeechSynthesizerObserverInterface::SpeechSynthesizerState::GAINING_FOCUS != m_speechSynthesizerState) {
            ACSDK_DEBUG5(
                LX("Kicking off short timer").d("shortTimeout in ms", m_shortTimeoutForThinkingToIdle.count()));
            /*
             * Stop the long timer and start a short timer so that either the state will change (i.e. Speech begins)
             * or we automatically go to idle after the short timeout (i.e. the directive received isn't related to
             * speech, like a setVolume directive). Cannot automatically goto IDLE because it will cause
             * SpeechSynthesizer to release focus, which may happen before the Speak has rendered.
             */
            m_thinkingTimeoutTimer.stop();
            m_thinkingTimeoutTimer.start(
                m_shortTimeoutForThinkingToIdle,
                std::bind(&DialogUXStateAggregator::transitionFromThinkingTimedOut, this));
        }
    });
}

void DialogUXStateAggregator::onConnectionStatusChanged(
    const ConnectionStatusObserverInterface::Status status,
    const ConnectionStatusObserverInterface::ChangedReason reason) {
    m_executor.submit([this, status]() {
        if (status != avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status::CONNECTED) {
            executeSetState(DialogUXStateObserverInterface::DialogUXState::IDLE);
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
                executeSetState(DialogUXStateObserverInterface::DialogUXState::THINKING);

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
    // If receive() calls occur before onRequestProcessStarted() happens, we need to use this method as a fallback to
    // exit THINKING mode.
    tryExitThinkingState();
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
            executeSetState(DialogUXStateObserverInterface::DialogUXState::IDLE);

            submitMetric(m_metricRecorder, THINKING_TIMEOUT_EXPIRES);
        }
    });
}

void DialogUXStateAggregator::transitionFromListeningTimedOut() {
    m_executor.submit([this]() {
        if (DialogUXStateObserverInterface::DialogUXState::LISTENING == m_currentState) {
            ACSDK_DEBUG(LX("transitionFromListeningTimedOut"));
            executeSetState(DialogUXStateObserverInterface::DialogUXState::IDLE);

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
            executeSetState(sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::IDLE);
        }
    });
}

bool DialogUXStateAggregator::executeSetState(sdkInterfaces::DialogUXStateObserverInterface::DialogUXState newState) {
    bool validTransition = true;

    if (newState == m_currentState) {
        validTransition = false;
    } else {
        switch (m_currentState) {
            case DialogUXStateObserverInterface::DialogUXState::THINKING:
                if (DialogUXStateObserverInterface::DialogUXState::LISTENING == newState) {
                    validTransition = false;
                }

                break;
            default:
                break;
        }
    }

    ACSDK_DEBUG0(LX(__func__)
                     .d("from", m_currentState)
                     .d("to", newState)
                     .d("validTransition", validTransition ? "true" : "false"));

    if (!validTransition) {
        return false;
    }

    m_listeningTimeoutTimer.stop();
    m_thinkingTimeoutTimer.stop();
    m_multiturnSpeakingToListeningTimer.stop();
    m_currentState = newState;
    notifyObserversOfState();

    return true;
}

void DialogUXStateAggregator::tryEnterIdleState() {
    ACSDK_DEBUG5(LX(__func__));
    m_thinkingTimeoutTimer.stop();
    m_multiturnSpeakingToListeningTimer.stop();
    if (!m_multiturnSpeakingToListeningTimer
             .start(
                 m_shortTimeoutForThinkingToIdle, std::bind(&DialogUXStateAggregator::tryEnterIdleStateOnTimer, this))
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
