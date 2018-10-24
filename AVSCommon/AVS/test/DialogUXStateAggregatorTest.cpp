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

#include <gtest/gtest.h>

#include "AVSCommon/AVS/DialogUXStateAggregator.h"
#include "AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h"
#include "AVSCommon/Utils/Memory/Memory.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace test {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::avs;

/// Long time out for observers to wait for the state change callback (we should not reach this).
static const auto DEFAULT_TIMEOUT = std::chrono::seconds(5);

/// Short time out for when callbacks are expected not to occur.
static const auto SHORT_TIMEOUT = std::chrono::milliseconds(50);

/// Time out for testing if transitionFromThinking timeout has occurred.  This needs to be longer than the SHORT_TIMEOUT
/// defined in DialogUXStateAggregator.cpp.
static const auto TRANSITION_FROM_THINKING_TIMEOUT = std::chrono::milliseconds(300);

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
     * @param timeout An optional timeout parameter to wait for to make sure no state change has occured.
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
TEST_F(DialogUXAggregatorTest, testIdleAtBeginning) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);
}

/// Tests that a new observer added receives the current state.
TEST_F(DialogUXAggregatorTest, testInvalidAtBeginningForMultipleObservers) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->addObserver(m_anotherTestObserver);

    assertStateChange(m_anotherTestObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);
}

/// Tests that the removing observer functionality works properly by asserting no state change on a removed observer.
TEST_F(DialogUXAggregatorTest, testRemoveObserver) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->addObserver(m_anotherTestObserver);
    assertStateChange(m_anotherTestObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->removeObserver(m_anotherTestObserver);
    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::RECOGNIZING);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::LISTENING);
    assertNoStateChange(m_anotherTestObserver);
}

/// Tests that multiple callbacks aren't issued if the state shouldn't change.
TEST_F(DialogUXAggregatorTest, aipIdleLeadsToIdleState) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::IDLE);
    assertNoStateChange(m_testObserver);
}

/// Tests that the AIP recognizing state leads to the LISTENING state.
TEST_F(DialogUXAggregatorTest, aipRecognizeLeadsToListeningState) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::RECOGNIZING);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::LISTENING);
}

/// Tests that the AIP recognizing state leads to the LISTENING state.
TEST_F(DialogUXAggregatorTest, aipIdleLeadsToIdle) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::RECOGNIZING);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::LISTENING);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::IDLE);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);
}

/// Tests that the AIP expecting speech state leads to the EXPECTING state.
TEST_F(DialogUXAggregatorTest, aipExpectingSpeechLeadsToListeningState) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::EXPECTING_SPEECH);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::EXPECTING);
}

/// Tests that the AIP busy state leads to the THINKING state.
TEST_F(DialogUXAggregatorTest, aipBusyLeadsToThinkingState) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::BUSY);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::THINKING);
}

/// Tests that BUSY state goes to IDLE after the specified timeout.
TEST_F(DialogUXAggregatorTest, busyGoesToIdleAfterTimeout) {
    std::shared_ptr<DialogUXStateAggregator> anotherAggregator =
        std::make_shared<DialogUXStateAggregator>(std::chrono::milliseconds(200));

    anotherAggregator->addObserver(m_anotherTestObserver);

    assertStateChange(m_anotherTestObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    anotherAggregator->onStateChanged(AudioInputProcessorObserverInterface::State::BUSY);
    assertStateChange(m_anotherTestObserver, DialogUXStateObserverInterface::DialogUXState::THINKING);

    assertStateChange(
        m_anotherTestObserver, DialogUXStateObserverInterface::DialogUXState::IDLE, std::chrono::milliseconds(400));
}

/// Tests that the BUSY state remains in BUSY immediately if a message is received.
TEST_F(DialogUXAggregatorTest, busyThenReceiveRemainsInBusyImmediately) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::BUSY);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::THINKING);

    m_aggregator->receive("", "");

    assertNoStateChange(m_testObserver);
}

/// Tests that the BUSY state goes to IDLE after a message is received after a short timeout.
TEST_F(DialogUXAggregatorTest, busyThenReceiveGoesToIdleAfterShortTimeout) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::BUSY);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::THINKING);

    m_aggregator->receive("", "");

    assertStateChange(
        m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE, std::chrono::milliseconds(250));
}

/// Tests that the BUSY state goes to IDLE after a SpeechSynthesizer speak state is received.
TEST_F(DialogUXAggregatorTest, busyThenReceiveThenSpeakGoesToSpeak) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::BUSY);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::THINKING);

    m_aggregator->receive("", "");

    m_aggregator->onStateChanged(sdkInterfaces::SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING);

    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::SPEAKING);
}

/**
 * Tests that the BUSY state goes to SPEAKING but not IDLE after both a message is received and a SpeechSynthesizer
 * speak state is received.
 */
TEST_F(DialogUXAggregatorTest, busyThenReceiveThenSpeakGoesToSpeakButNotIdle) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::BUSY);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::THINKING);

    m_aggregator->receive("", "");

    m_aggregator->onStateChanged(sdkInterfaces::SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING);

    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::SPEAKING);

    assertNoStateChange(m_testObserver);
}

/// Tests that both SpeechSynthesizer and AudioInputProcessor finished/idle state leads to the IDLE state.
TEST_F(DialogUXAggregatorTest, speakingAndRecognizingFinishedGoesToIdle) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::BUSY);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::THINKING);

    m_aggregator->receive("", "");

    m_aggregator->onStateChanged(sdkInterfaces::SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING);

    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::SPEAKING);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::IDLE);
    m_aggregator->onStateChanged(sdkInterfaces::SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);

    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);
}

/// Tests that SpeechSynthesizer or AudioInputProcessor non-idle state prevents the IDLE state.
TEST_F(DialogUXAggregatorTest, nonIdleObservantsPreventsIdle) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    // AIP is active, SS is not. Expected: non idle
    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::BUSY);
    m_aggregator->onStateChanged(sdkInterfaces::SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::THINKING);

    // Both AIP and SS are inactive. Expected: idle
    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::IDLE);
    m_aggregator->onStateChanged(sdkInterfaces::SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    // AIP is inactive, SS is active. Expected: non-idle
    m_aggregator->receive("", "");
    m_aggregator->onStateChanged(sdkInterfaces::SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::SPEAKING);

    // AIP is inactive, SS is inactive: Expected: idle
    m_aggregator->onStateChanged(sdkInterfaces::SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);
}

/// Tests that a SpeechSynthesizer finished state does not go to the IDLE state after a very short timeout.
TEST_F(DialogUXAggregatorTest, speakingFinishedDoesNotGoesToIdleImmediately) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::BUSY);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::THINKING);

    m_aggregator->receive("", "");

    m_aggregator->onStateChanged(sdkInterfaces::SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING);

    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::SPEAKING);

    m_aggregator->onStateChanged(sdkInterfaces::SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);

    assertNoStateChange(m_testObserver);
}

/// Tests that a simple message receive does nothing.
TEST_F(DialogUXAggregatorTest, simpleReceiveDoesNothing) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->receive("", "");

    assertNoStateChange(m_testObserver);

    m_aggregator->onStateChanged(sdkInterfaces::SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING);

    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::SPEAKING);

    m_aggregator->receive("", "");

    assertNoStateChange(m_testObserver);
}

/// Tests that the THINKING state remains in THINKING if SpeechSynthesizer reports GAINING_FOCUS and a new message is
/// received.
TEST_F(DialogUXAggregatorTest, thinkingThenReceiveRemainsInThinkingIfSpeechSynthesizerReportsGainingFocus) {
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::IDLE);

    m_aggregator->onStateChanged(AudioInputProcessorObserverInterface::State::BUSY);
    assertStateChange(m_testObserver, DialogUXStateObserverInterface::DialogUXState::THINKING);

    m_aggregator->receive("", "");

    m_aggregator->onStateChanged(
        sdkInterfaces::SpeechSynthesizerObserverInterface::SpeechSynthesizerState::GAINING_FOCUS);

    // Make sure after SpeechSynthesizer reports GAINING_FOCUS, that it would stay in THINKING state
    m_aggregator->receive("", "");

    assertNoStateChange(m_testObserver, TRANSITION_FROM_THINKING_TIMEOUT);
}

}  // namespace test
}  // namespace avsCommon
}  // namespace alexaClientSDK
