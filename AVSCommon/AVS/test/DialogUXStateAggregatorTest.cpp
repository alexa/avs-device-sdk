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

#include <gtest/gtest.h>

#include "AVSCommon/AVS/DialogUXStateAggregator.h"
#include "AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h"
#include "AVSCommon/Utils/Memory/Memory.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;

/// Long time out for observers to wait for the state change callback (we should not reach this).
static const auto DEFAULT_TIMEOUT = std::chrono::seconds(5);

/// Short time out for when callbacks are expected not to occur.
static const auto SHORT_TIMEOUT = std::chrono::milliseconds(50);

/// Time out for testing if transitionFromThinking or transitionFromListneing timeouts have occurred.
// This needs to be longer than the values passed into the DialogUXStateAggregator.
static const auto TRANSITION_TIMEOUT = std::chrono::milliseconds(300);

/// Dummy value for a media player source id
static const avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId TEST_SOURCE_ID = -1;

/// A test observer that mocks out the DialogUXStateObserverInterface##onDialogUXStateChanged() call.
class TestObserver : public DialogUXStateObserverInterface {
public:
    /**
     * Constructor.
     */
    TestObserver() : m_state{DialogUXStateObserverInterface::DialogUXState::IDLE}, m_changeOccurred{false} {
    }

    /**
     * Implementation of the DialogUXStateObserverInterface##onDialogUXStateChanged() callback.
     *
     * @param newState The new UX state of the observer.
     */
    void onDialogUXStateChanged(DialogUXStateObserverInterface::DialogUXState newState) override {
        std::unique_lock<std::mutex> lock{m_mutex};
        m_state = newState;
        m_changeOccurred = true;
        m_UXChanged.notify_one();
    }

    /**
     * Waits for the UXStateObserverInterface##onUXStateChanged() callback.
     *
     * @param timeout The amount of time to wait for the callback.
     * @param uxChanged An output parameter that notifies the caller whether a callback occurred.
     * @return Returns @c true if the callback occured within the timeout period and @c false otherwise.
     */
    DialogUXStateObserverInterface::DialogUXState waitForStateChange(
        std::chrono::milliseconds timeout,
        bool* uxChanged) {
        std::unique_lock<std::mutex> lock{m_mutex};
        bool success = m_UXChanged.wait_for(lock, timeout, [this]() { return m_changeOccurred; });

        if (!success) {
            *uxChanged = false;
        } else {
            m_changeOccurred = false;
            *uxChanged = true;
        }
        return m_state;
    }

private:
    /// The UX state of the observer.
    DialogUXStateObserverInterface::DialogUXState m_state;

    /// A lock to guard against state changes.
    std::mutex m_mutex;

    /// A condition variable to wait for state changes.
    std::condition_variable m_UXChanged;

    /// A boolean flag so that we can re-use the observer even after a callback has occurred.
    bool m_changeOccurred;
};

/// Manages testing state changes
class StateChangeManager {
public:
    /**
     * Checks that a state change occurred and that the ux state received is the same as the expected ux state.
     *
     * @param observer The UX state observer.
     * @param expectedState The expected UX state.
     * @param timeout An optional timeout parameter to wait for a state change
     */
    void assertStateChange(
        std::shared_ptr<TestObserver> observer,
        DialogUXStateObserverInterface::DialogUXState expectedState,
        std::chrono::milliseconds timeout = DEFAULT_TIMEOUT) {
        ASSERT_TRUE(observer);
        bool stateChanged = false;
        auto receivedState = observer->waitForStateChange(timeout, &stateChanged);
        ASSERT_TRUE(stateChanged);
        ASSERT_EQ(expectedState, receivedState);
    }

    /**
     * Checks that a state change does not occur by waiting for the timeout duration.
     *
     * @param observer The UX state observer.
     * @param timeout An optional timeout parameter to wait for to make sure no state change has occurred.
     */
    void assertNoStateChange(
        std::shared_ptr<TestObserver> observer,
        std::chrono::milliseconds timeout = SHORT_TIMEOUT) {
        ASSERT_TRUE(observer);
        bool stateChanged = false;
        // Will wait for the short timeout duration before succeeding
        observer->waitForStateChange(timeout, &stateChanged);
        ASSERT_FALSE(stateChanged);
    }
};

/// Test fixture for testing DialogUXStateAggregator.
class DialogUXAggregatorTest
        : public ::testing::Test
        , public StateChangeManager {
protected:
    /// The UX state aggregator
    std::shared_ptr<DialogUXStateAggregator> m_aggregator;

    /// A test observer.
    std::shared_ptr<TestObserver> m_testObserver;

    /// Another test observer
    std::shared_ptr<TestObserver> m_anotherTestObserver;

    /// A MediaPlayerState object passed to onStateChange by SpeechSynthesizer
    avsCommon::utils::mediaPlayer::MediaPlayerState m_testMediaPlayerState;

    /// A vector of AudioAnalyzerState objects passed to onStateChanged by SpeechSynthesizer
    std::vector<avsCommon::utils::audioAnalyzer::AudioAnalyzerState> m_testAudioAnalyzerState;

    virtual void SetUp() {
        m_aggregator = std::make_shared<DialogUXStateAggregator>();
        ASSERT_TRUE(m_aggregator);
        m_testObserver = std::make_shared<TestObserver>();
        ASSERT_TRUE(m_testObserver);
        m_aggregator->addObserver(m_testObserver);
        m_anotherTestObserver = std::make_shared<TestObserver>();
        ASSERT_TRUE(m_anotherTestObserver);
    }
};

/// Tests that an observer starts off in the IDLE state.
TEST_F(DialogUXAggregatorTest, test_idleAtBeginning) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);
}

/// Tests that a new observer added receives the current state.
TEST_F(DialogUXAggregatorTest, test_invalidAtBeginningForMultipleObservers) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->addObserver(m_anotherTestObserver);

    assertStateChange(m_anotherTestObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);
}

/// Tests that the removing observer functionality works properly by asserting no state change on a removed observer.
TEST_F(DialogUXAggregatorTest, test_removeObserver) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->addObserver(m_anotherTestObserver);
    assertStateChange(m_anotherTestObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->removeObserver(m_anotherTestObserver);
    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::RECOGNIZING);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::LISTENING);
    assertNoStateChange(m_anotherTestObserver);
}

/// Tests that multiple callbacks aren't issued if the state shouldn't change.
TEST_F(DialogUXAggregatorTest, test_aipIdleLeadsToIdleState) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::IDLE);
    assertNoStateChange(m_testObserver);
}

/// Tests that the AIP recognizing state leads to the LISTENING state.
TEST_F(DialogUXAggregatorTest, test_aipRecognizeLeadsToListeningState) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::RECOGNIZING);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::LISTENING);
}

/// Tests that the AIP recognizing state leads to the LISTENING state.
TEST_F(DialogUXAggregatorTest, test_aipIdleLeadsToIdle) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::RECOGNIZING);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::LISTENING);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::IDLE);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);
}

/// Tests that the AIP expecting speech state leads to the EXPECTING state.
TEST_F(DialogUXAggregatorTest, test_aipExpectingSpeechLeadsToListeningState) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::EXPECTING_SPEECH);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::EXPECTING);
}

/// Tests that the AIP busy state leads to the LISTENING state.
TEST_F(DialogUXAggregatorTest, test_aipBusyLeadsToListeningState) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::BUSY);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::LISTENING);
}

/// Tests that the RequestProcessingStarted leads to the THINKING state.
TEST_F(DialogUXAggregatorTest, test_requestProcessingStartedLeadsToThinkingState) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::BUSY);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::LISTENING);

    m_aggregator->onRequestProcessingStarted();
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::THINKING);
}

/// Tests that LISTENING state goes to IDLE after the specified timeout.
TEST_F(DialogUXAggregatorTest, test_listeningGoesToIdleAfterTimeout) {
    std::shared_ptr<DialogUXStateAggregator> anotherAggregator = std::make_shared<DialogUXStateAggregator>(
        nullptr, std::chrono::milliseconds(200), std::chrono::milliseconds(200));

    anotherAggregator->addObserver(m_anotherTestObserver);

    assertStateChange(m_anotherTestObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    anotherAggregator->onStateChanged(AudioInputProcessorObserverInterface::State::BUSY);
    assertStateChange(m_anotherTestObserver, DialogUXStateObserverInterface::DialogUXState::LISTENING);

    assertStateChange(m_anotherTestObserver, DialogUXStateObserverInterface::DialogUXState::IDLE, TRANSITION_TIMEOUT);
}

/// Tests that THINKING state goes to IDLE after the specified timeout.
TEST_F(DialogUXAggregatorTest, test_thinkingGoesToIdleAfterTimeout) {
    std::shared_ptr<DialogUXStateAggregator> anotherAggregator = std::make_shared<DialogUXStateAggregator>(
        nullptr, std::chrono::milliseconds(200), std::chrono::milliseconds(200));

    anotherAggregator->addObserver(m_anotherTestObserver);
    assertStateChange(m_anotherTestObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    anotherAggregator->onStateChanged(AudioInputProcessorObserverInterface::State::BUSY);
    assertStateChange(m_anotherTestObserver, DialogUXStateObserverInterface::DialogUXState::LISTENING);

    anotherAggregator->onRequestProcessingStarted();
    assertStateChange(m_anotherTestObserver, DialogUXStateObserverInterface::DialogUXState::THINKING);

    assertStateChange(m_anotherTestObserver, DialogUXStateObserverInterface::DialogUXState::IDLE, TRANSITION_TIMEOUT);
}

/// Tests that the THINKING state transitions to IDLE after recieving a message and a long timeout.
TEST_F(DialogUXAggregatorTest, test_thinkingThenReceiveGoesToIdleAfterLongTimeout) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::BUSY);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::LISTENING);

    m_aggregator->onRequestProcessingStarted();
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::THINKING);

    m_aggregator->receive("", "");
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE, TRANSITION_TIMEOUT);
}

/**
 * Tests that the LISTENING state goes to SPEAKING but not IDLE after both a message is received and a SpeechSynthesizer
 * speak state is received.
 */
TEST_F(DialogUXAggregatorTest, test_listeningThenReceiveThenSpeakGoesToSpeakButNotIdle) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::BUSY);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::LISTENING);

    m_aggregator->onRequestProcessingStarted();
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::THINKING);

    m_aggregator->receive("", "");

    m_aggregator->onStateChanged(
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING,
        TEST_SOURCE_ID,
        m_testMediaPlayerState,
        m_testAudioAnalyzerState);

    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::SPEAKING);

    assertNoStateChange(m_testObserver);
}

/// Tests that both SpeechSynthesizer and AudioInputProcessor finished/idle state leads to the IDLE state.
TEST_F(DialogUXAggregatorTest, test_speakingAndRecognizingFinishedGoesToIdle) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::BUSY);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::LISTENING);
    m_aggregator->onRequestProcessingStarted();
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::THINKING);

    m_aggregator->receive("", "");

    m_aggregator->onStateChanged(
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING,
        TEST_SOURCE_ID,
        m_testMediaPlayerState,
        m_testAudioAnalyzerState);

    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::SPEAKING);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::IDLE);
    m_aggregator->onStateChanged(
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED,
        TEST_SOURCE_ID,
        m_testMediaPlayerState,
        m_testAudioAnalyzerState);

    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);
}

/// Tests that SpeechSynthesizer or AudioInputProcessor non-idle state prevents the IDLE state.
TEST_F(DialogUXAggregatorTest, test_nonIdleObservantsPreventsIdle) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    // AIP is active, SS is not. Expected: non idle
    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::BUSY);
    m_aggregator->onStateChanged(
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED,
        TEST_SOURCE_ID,
        m_testMediaPlayerState,
        m_testAudioAnalyzerState);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::LISTENING);

    // Both AIP and SS are inactive. Expected: idle
    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::IDLE);
    m_aggregator->onStateChanged(
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED,
        TEST_SOURCE_ID,
        m_testMediaPlayerState,
        m_testAudioAnalyzerState);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    // AIP is inactive, SS is active. Expected: non-idle
    m_aggregator->onStateChanged(
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING,
        TEST_SOURCE_ID,
        m_testMediaPlayerState,
        m_testAudioAnalyzerState);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::SPEAKING);

    // AIP is inactive, SS is inactive: Expected: idle
    m_aggregator->onStateChanged(
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED,
        TEST_SOURCE_ID,
        m_testMediaPlayerState,
        m_testAudioAnalyzerState);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);
}

/// Tests that a SpeechSynthesizer finished state does not go to the IDLE state after a very short timeout.
TEST_F(DialogUXAggregatorTest, test_speakingFinishedDoesNotGoesToIdleImmediately) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::BUSY);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::LISTENING);
    m_aggregator->onRequestProcessingStarted();
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::THINKING);

    m_aggregator->receive("", "");

    m_aggregator->onStateChanged(
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING,
        TEST_SOURCE_ID,
        m_testMediaPlayerState,
        m_testAudioAnalyzerState);

    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::SPEAKING);

    m_aggregator->onStateChanged(
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED,
        TEST_SOURCE_ID,
        m_testMediaPlayerState,
        m_testAudioAnalyzerState);

    assertNoStateChange(m_testObserver);
}

/// Tests that a simple message receive does nothing.
TEST_F(DialogUXAggregatorTest, test_simpleReceiveDoesNothing) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->receive("", "");

    assertNoStateChange(m_testObserver);

    m_aggregator->onStateChanged(
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING,
        TEST_SOURCE_ID,
        m_testMediaPlayerState,
        m_testAudioAnalyzerState);

    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::SPEAKING);

    m_aggregator->receive("", "");

    assertNoStateChange(m_testObserver);
}

/// Tests that the THINKING state remains in THINKING if SpeechSynthesizer reports GAINING_FOCUS and a new message is
/// received.
TEST_F(DialogUXAggregatorTest, test_thinkingThenReceiveRemainsInThinkingIfSpeechSynthesizerReportsGainingFocus) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::BUSY);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::LISTENING);
    m_aggregator->onRequestProcessingStarted();
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::THINKING);

    m_aggregator->receive("", "");

    m_aggregator->onStateChanged(
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::GAINING_FOCUS,
        TEST_SOURCE_ID,
        m_testMediaPlayerState,
        m_testAudioAnalyzerState);

    // Make sure after SpeechSynthesizer reports GAINING_FOCUS, that it would stay in THINKING state
    m_aggregator->receive("", "");

    assertNoStateChange(m_testObserver, TRANSITION_TIMEOUT);
}

/// Tests that only certain states are allowed to transition into THINKING from an RPS.
TEST_F(DialogUXAggregatorTest, test_validStatesForRPSToThinking) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);
    m_aggregator->onRequestProcessingStarted();
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::THINKING);

    m_aggregator->receive("", "");
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);
    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::BUSY);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::LISTENING);
    m_aggregator->onRequestProcessingStarted();
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::THINKING);

    m_aggregator->receive("", "");
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);
    m_aggregator->onStateChanged(
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING,
        TEST_SOURCE_ID,
        m_testMediaPlayerState,
        m_testAudioAnalyzerState);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::SPEAKING);
    m_aggregator->onRequestProcessingStarted();
    assertNoStateChange(m_testObserver);

    // Reset to IDLE
    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::IDLE);
    m_aggregator->onStateChanged(
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED,
        TEST_SOURCE_ID,
        m_testMediaPlayerState,
        m_testAudioAnalyzerState);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);
    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::EXPECTING_SPEECH);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::EXPECTING);
    m_aggregator->onRequestProcessingStarted();
    assertNoStateChange(m_testObserver);
}

}  // namespace test
}  // namespace avsCommon
}  // namespace alexaClientSDK
