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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "ContextManager/ContextManager.h"

using namespace testing;

namespace alexaClientSDK {
namespace contextManager {
namespace test {

using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;

/// Payload for SpeechSynthesizer state when it is playing back audio.
static const std::string SPEECH_SYNTHESIZER_PAYLOAD_PLAYING =
    "{"
    "\"playerActivity\":\"PLAYING\","
    "\"offsetInMilliseconds\":5,"
    "\"token\":\"\""
    "}";
/// Payload for SpeechSynthesizer state when playback has finished.
static const std::string SPEECH_SYNTHESIZER_PAYLOAD_FINISHED =
    "{"
    "\"playerActivity\":\"FINISHED\","
    "\"offsetInMilliseconds\":0,"
    "\"token\":\"\""
    "}";

/// Payload for AudioPlayer.
static const std::string AUDIO_PLAYER_PAYLOAD =
    "{"
    "\"playerActivity\":\"FINISHED\","
    "\"offsetInMilliseconds\":0,"
    "\"token\":\"\""
    "}";

/// Payload for Alerts.
static const std::string ALERTS_PAYLOAD =
    "{"
    "\"allAlerts\": ["
    "{"
    "\"token\": \"\","
    "\"type\": \"TIMER\","
    "\"scheduledTime\": \"\""
    "}"
    "],"
    "\"activeAlerts\": ["
    "{"
    "\"token\": \"\","
    "\"type\": \"TIMER\","
    "\"scheduledTime\": \"\""
    "}"
    "]"
    "}";

/// Context used for testing.
static const std::string CONTEXT_TEST =
    "{"
    "\"context\":["
    "{"
    "\"header\":{"
    "\"namespace\":\"AudioPlayer\","
    "\"name\":\"PlaybackState\""
    "},"
    "\"payload\":{"
    "\"playerActivity\":\"FINISHED\","
    "\"offsetInMilliseconds\":0,"
    "\"token\":\"\""
    "}"
    "},"
    "{"
    "\"header\":{"
    "\"namespace\":\"SpeechSynthesizer\","
    "\"name\":\"SpeechState\""
    "},"
    "\"payload\":{"
    "\"playerActivity\":\"FINISHED\","
    "\"offsetInMilliseconds\":0,"
    "\"token\":\"\""
    "}"
    "}"
    "]"
    "}";

/// Default time @c doProvide sleeps before it calls @c setState.
static const std::chrono::milliseconds DEFAULT_SLEEP_TIME = std::chrono::milliseconds(10);

/// Time @c doProvide sleeps before it calls @c setState to induce a timeout.
static const std::chrono::milliseconds TIMEOUT_SLEEP_TIME = std::chrono::milliseconds(100);

/**
 * Default timeout for the @c ContextRequester to get the context.This needs to be modified if the
 * @c TIMEOUT_SLEEP_TIME is modified. The value should be less than the @c TIMEOUT_SLEEP_TIME.
 */
static const std::chrono::milliseconds DEFAULT_TIMEOUT = std::chrono::milliseconds(50);

/// Timeout for the @c ContextRequester to get the failure.
static const std::chrono::milliseconds FAILURE_TIMEOUT = std::chrono::milliseconds(110);

/// Namespace for SpeechSynthesizer.
static const std::string NAMESPACE_SPEECH_SYNTHESIZER("SpeechSynthesizer");

/// Namespace for AudioPlayer.
static const std::string NAMESPACE_AUDIO_PLAYER("AudioPlayer");

/// Namespace for Alerts.
static const std::string NAMESPACE_ALERTS("Alerts");

/// Name for SpeechSynthesizer state.
static const std::string NAME_SPEECH_STATE("SpeechState");

/// Name for AudioPlayer state.
static const std::string NAME_PLAYBACK_STATE("PlaybackState");

/// Name for Alerts state.
static const std::string NAME_ALERTS_STATE("AlertsState");

/// SpeechSynthesizer namespace and name
static const NamespaceAndName SPEECH_SYNTHESIZER(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEECH_STATE);

/// AudioPlayer namespace and name
static const NamespaceAndName AUDIO_PLAYER(NAMESPACE_AUDIO_PLAYER, NAME_PLAYBACK_STATE);

/// Alerts namespace and name
static const NamespaceAndName ALERTS(NAMESPACE_ALERTS, NAME_ALERTS_STATE);

/// Dummy provider namespace and name
static const NamespaceAndName DUMMY_PROVIDER("Dummy", "DummyName");

/**
 * @c MockContextRequester used to verify @c ContextManager behavior.
 */
class MockContextRequester : public ContextRequesterInterface {
public:
    /**
     * Creates an instance of the @c MockContextRequester.
     *
     * @param contextManager Pointer to an instance of the @c ContextManager.
     * @return A shared pointer to an instance of the @c MockContextRequester.
     */
    static std::shared_ptr<MockContextRequester> create(std::shared_ptr<ContextManager> contextManager);

    /**
     * Constructor
     *
     * @param contextManager Pointer to an instance of the @c ContextManager.
     */
    MockContextRequester(std::shared_ptr<ContextManager> contextManager);

    ~MockContextRequester();

    void onContextAvailable(const std::string& context) override;

    void onContextFailure(const ContextRequestError error) override;

    /**
     * Waits for a specified time for the context to be available @c getContext.
     *
     * @param duration Number of milliseconds to wait before giving up.
     * @return @c true if a context or failure was received within the specified duration, else @c false.
     */
    bool waitForContext(const std::chrono::milliseconds duration = std::chrono::milliseconds(200));

    /**
     * Waits for a specified time for the context request to fail on a @c getContext.
     *
     * @param duration Number of milliseconds to wait before giving up.
     * @return @c true if a failure was received within the specified duration, else @c false.
     */
    bool waitForFailure(const std::chrono::milliseconds duration = std::chrono::milliseconds(200));

    /**
     * Function to read the stored context.
     *
     * @return The context.
     */
    std::string& getContextString();

private:
    /// Instance of @ ContextManager
    std::shared_ptr<ContextManager> m_contextManager;

    /// Condition variable to wake the @ waitForContext.
    std::condition_variable m_wakeTriggerSuccess;

    /// Condition variable to wake the @ waitForFailure.
    std::condition_variable m_wakeTriggerFailure;

    /// mutex to protect @c m_contextAvailable.
    std::mutex m_mutexSuccess;

    /// mutex to protect @c m_contextFailure.
    std::mutex m_mutexFailure;

    /// Flag to indicate context is available.
    bool m_contextAvailable;

    /// Flag to indicate there was a failure during @c getContext.
    bool m_contextFailure;

    /// String to hold the context returned by the @c ContextManager.
    std::string m_context;
};

std::shared_ptr<MockContextRequester> MockContextRequester::create(std::shared_ptr<ContextManager> contextManager) {
    auto result = std::make_shared<MockContextRequester>(contextManager);
    return result;
}

MockContextRequester::MockContextRequester(std::shared_ptr<ContextManager> contextManager) :
        m_contextManager{contextManager},
        m_contextAvailable{false},
        m_contextFailure{false} {
}

MockContextRequester::~MockContextRequester() {
}

void MockContextRequester::onContextAvailable(const std::string& context) {
    std::unique_lock<std::mutex> lock(m_mutexSuccess);
    m_context = context;
    m_contextAvailable = true;
    m_wakeTriggerSuccess.notify_one();
}

void MockContextRequester::onContextFailure(const ContextRequestError error) {
    std::unique_lock<std::mutex> lock(m_mutexFailure);
    m_contextFailure = true;
    m_wakeTriggerFailure.notify_one();
}

bool MockContextRequester::waitForContext(const std::chrono::milliseconds duration) {
    std::unique_lock<std::mutex> lock(m_mutexSuccess);
    if (!m_wakeTriggerSuccess.wait_for(lock, duration, [this]() { return (m_contextAvailable || m_contextFailure); })) {
        return false;
    }
    return true;
}

bool MockContextRequester::waitForFailure(const std::chrono::milliseconds duration) {
    std::unique_lock<std::mutex> lock(m_mutexFailure);
    if (!m_wakeTriggerFailure.wait_for(lock, duration, [this]() { return (m_contextAvailable || m_contextFailure); })) {
        return false;
    }
    return true;
}

std::string& MockContextRequester::getContextString() {
    return m_context;
}

/**
 * @c MockStateProvider used to verify @c ContextManager behavior.
 */
class MockStateProvider : public StateProviderInterface {
public:
    /**
     * Creates an instance of the @c MockStateProvider.
     *
     * @param contextManager Pointer to an instance of the @c ContextManager.
     * @param namespaceAndName The namespace and name of the @c StateProvider.
     * @param jsonState The state of the @c StateProvider set in response to the @c provideState.
     * @param refreshPolicy The refresh policy for the state set in response to the @c provideState.
     * @param delayTime The time @c m_doProvideThread sleeps before updating state via @c setState.
     * @return A pointer to an instance of the @c MockStateProvider.
     */
    static std::shared_ptr<MockStateProvider> create(
        std::shared_ptr<ContextManager> contextManager,
        const NamespaceAndName& namespaceAndName,
        const std::string& state,
        const StateRefreshPolicy& refreshPolicy,
        const std::chrono::milliseconds delayTime);

    /**
     * Constructor. It sets up the state and refresh policy that needs to be sent in response to a @c provideContext
     * request.
     *
     * @param contextManager Pointer to an instance of the @c ContextManager.
     * @param namespaceAndName The namespace and name of the @c StateProvider.
     * @param jsonState The state of the @c StateProvider set in response to the @c provideState.
     * @param refreshPolicy The refresh policy for the state set in response to the @c provideState.
     * @param delayTime The time @c m_doProvideThread sleeps before updating state via @c setState.
     * @return A pointer to an instance of the @c MockStateProvider.
     */
    MockStateProvider(
        std::shared_ptr<ContextManager> contextManager,
        const NamespaceAndName& namespaceAndName,
        const std::string& state,
        const StateRefreshPolicy& refreshPolicy,
        const std::chrono::milliseconds delayTime);

    ~MockStateProvider();

    void provideState(const NamespaceAndName& stateProviderName, unsigned int currentstateRequestToken) override;

    /**
     * Method for m_doProvideThread. It waits for @c m_delayTime and then calls @c setState with the state @c m_state
     * and the refresh policy @c m_refreshPolicy the @c stateProviderInterface was initialized with.
     */
    void doProvideState();

    unsigned int getCurrentstateRequestToken();

private:
    /// Pointer to an instance of the @c ContextManager
    std::shared_ptr<ContextManager> m_contextManager;

    /// The namespace and name of the @c stateProviderInterface
    NamespaceAndName m_namespaceAndName;

    /// The state provided to the @c ContextManager via @c setState.
    std::string m_state;

    /// The refresh policy for the state provided to the @c ContextManager via @c setState.
    StateRefreshPolicy m_refreshPolicy;

    /// The token provided by the @c ContextManager.
    unsigned int m_stateRequestToken;

    /// Thread to execute the doProvide.
    std::thread m_doProvideThread;

    /// Mutex to protect the @c m_provideState.
    std::mutex m_providerMutex;

    /// The condition variable used to wake up @c m_doProvideThread when the @c ContextManager requests for state.
    std::condition_variable m_providerWakeTrigger;

    /// The time the @c m_doProvideThread sleeps before providing state.
    std::chrono::milliseconds m_delayTime;

    /// Flag to indicate when a @c provideState has been called.
    bool m_provideState;

    /// Flag to indicate when the @c MockStateProvider is shutting down. When set, @c m_doProvideThread needs to return.
    bool m_stateProviderShutdown;
};

std::shared_ptr<MockStateProvider> MockStateProvider::create(
    std::shared_ptr<ContextManager> contextManager,
    const NamespaceAndName& stateProviderName,
    const std::string& state,
    const StateRefreshPolicy& refreshPolicy,
    const std::chrono::milliseconds delayTime) {
    auto result =
        std::make_shared<MockStateProvider>(contextManager, stateProviderName, state, refreshPolicy, delayTime);
    return result;
}

MockStateProvider::MockStateProvider(
    std::shared_ptr<ContextManager> contextManager,
    const NamespaceAndName& stateProviderName,
    const std::string& state,
    const StateRefreshPolicy& refreshPolicy,
    const std::chrono::milliseconds delayTime) :
        m_contextManager{contextManager},
        m_namespaceAndName{stateProviderName},
        m_state{state},
        m_refreshPolicy{refreshPolicy},
        m_stateRequestToken{0},
        m_delayTime{delayTime},
        m_provideState{false},
        m_stateProviderShutdown{false} {
    m_doProvideThread = std::thread(&MockStateProvider::doProvideState, this);
}

MockStateProvider::~MockStateProvider() {
    std::unique_lock<std::mutex> lock(m_providerMutex);
    m_stateProviderShutdown = true;
    m_providerWakeTrigger.notify_one();
    lock.unlock();
    if (m_doProvideThread.joinable()) {
        m_doProvideThread.join();
    }
}

void MockStateProvider::provideState(const NamespaceAndName& stateProviderName, unsigned int stateRequestToken) {
    m_stateRequestToken = stateRequestToken;
    std::lock_guard<std::mutex> lock(m_providerMutex);
    m_provideState = true;
    m_providerWakeTrigger.notify_one();
}

void MockStateProvider::doProvideState() {
    while (true) {
        std::unique_lock<std::mutex> lock(m_providerMutex);
        m_providerWakeTrigger.wait(lock, [this]() { return (m_provideState || m_stateProviderShutdown); });
        if (m_stateProviderShutdown) {
            lock.unlock();
            return;
        }
        std::this_thread::sleep_for(m_delayTime);
        EXPECT_EQ(
            SetStateResult::SUCCESS,
            m_contextManager->setState(m_namespaceAndName, m_state, m_refreshPolicy, m_stateRequestToken));
        m_provideState = false;
        lock.unlock();
    }
}

unsigned int MockStateProvider::getCurrentstateRequestToken() {
    return m_stateRequestToken;
}

/// Context Manager Test
class ContextManagerTest : public ::testing::Test {
public:
    /**
     * Setup the @c ContextManagerTest.  Creates a @c ContextManager. Creates a couple of @c MockStateProviders and
     * a @c MockContextRequester.
     */
    void SetUp() override;

    /// @c ContextManager to test
    std::shared_ptr<ContextManager> m_contextManager;

    /// @c MockStateProviders and @c MockContextRequesters
    std::shared_ptr<MockStateProvider> m_speechSynthesizer;
    std::shared_ptr<MockStateProvider> m_audioPlayer;
    std::shared_ptr<MockStateProvider> m_alerts;
    std::shared_ptr<MockContextRequester> m_contextRequester;
    std::shared_ptr<MockContextRequester> m_contextRequester2;
};

void ContextManagerTest::SetUp() {
    m_contextManager = ContextManager::create();
    m_speechSynthesizer = MockStateProvider::create(
        m_contextManager,
        SPEECH_SYNTHESIZER,
        SPEECH_SYNTHESIZER_PAYLOAD_FINISHED,
        StateRefreshPolicy::NEVER,
        DEFAULT_SLEEP_TIME);
    m_audioPlayer = MockStateProvider::create(
        m_contextManager, AUDIO_PLAYER, AUDIO_PLAYER_PAYLOAD, StateRefreshPolicy::NEVER, DEFAULT_SLEEP_TIME);
    m_contextManager->setStateProvider(SPEECH_SYNTHESIZER, m_speechSynthesizer);
    m_contextManager->setStateProvider(AUDIO_PLAYER, m_audioPlayer);
    m_contextRequester = MockContextRequester::create(m_contextManager);
}

/**
 * Set the state with a @c StateRefreshPolicy @c ALWAYS for a @c StateProviderInterface that is registered with the
 * @c ContextManager. Expect @c SetStateResult @c SUCCESS is returned.
 */
TEST_F(ContextManagerTest, testSetStateForRegisteredProvider) {
    ASSERT_EQ(
        SetStateResult::SUCCESS,
        m_contextManager->setState(
            SPEECH_SYNTHESIZER,
            SPEECH_SYNTHESIZER_PAYLOAD_PLAYING,
            StateRefreshPolicy::ALWAYS,
            m_speechSynthesizer->getCurrentstateRequestToken()));
}

/**
 * Set the state with a @c StateRefreshPolicy @c NEVER for a @c StateProviderInterface that is not registered with the
 * @c ContextManager. Expect @c SetStateResult @c SUCCESS is returned.
 */
TEST_F(ContextManagerTest, testSetStateForUnregisteredProvider) {
    ASSERT_EQ(SetStateResult::SUCCESS, m_contextManager->setState(ALERTS, ALERTS_PAYLOAD, StateRefreshPolicy::NEVER));
}

/**
 * Set the state with a @c StateRefreshPolicy @c ALWAYS for a @c StateProviderInterface that is not registered with
 * the @c ContextManager. Expect @c SetStateResult @c STATE_PROVIDER_NOT_REGISTERED is returned.
 */
TEST_F(ContextManagerTest, testSetStateForUnregisteredProviderWithRefreshPolicyAlways) {
    m_alerts = MockStateProvider::create(
        m_contextManager, ALERTS, ALERTS_PAYLOAD, StateRefreshPolicy::NEVER, DEFAULT_SLEEP_TIME);
    ASSERT_EQ(
        SetStateResult::STATE_PROVIDER_NOT_REGISTERED,
        m_contextManager->setState(
            ALERTS, ALERTS_PAYLOAD, StateRefreshPolicy::ALWAYS, m_alerts->getCurrentstateRequestToken()));
}

/**
 * Set the states with a @c StateRefreshPolicy @c ALWAYS for @c StateProviderInterfaces that are registered with the
 * @c ContextManager. Request for context by calling @c getContext. Expect that the context is returned within the
 * timeout period. Check the context that is returned by the @c ContextManager. Expect it should match the test value.
 */
TEST_F(ContextManagerTest, testGetContext) {
    ASSERT_EQ(
        SetStateResult::SUCCESS,
        m_contextManager->setState(
            SPEECH_SYNTHESIZER,
            SPEECH_SYNTHESIZER_PAYLOAD_PLAYING,
            StateRefreshPolicy::ALWAYS,
            m_speechSynthesizer->getCurrentstateRequestToken()));
    ASSERT_EQ(
        SetStateResult::SUCCESS,
        m_contextManager->setState(
            AUDIO_PLAYER,
            AUDIO_PLAYER_PAYLOAD,
            StateRefreshPolicy::ALWAYS,
            m_audioPlayer->getCurrentstateRequestToken()));
    m_contextManager->getContext(m_contextRequester);
    ASSERT_TRUE(m_contextRequester->waitForContext(DEFAULT_TIMEOUT));
    ASSERT_EQ(CONTEXT_TEST, m_contextRequester->getContextString());
}

/**
 * Set the states with a @c StateRefreshPolicy @c ALWAYS for @c StateProviderInterfaces that are registered with the
 * @c ContextManager. Request for context by calling @c getContext by multiple requesters. Expect that the context is
 * returned to each of the requesters within the timeout period.
 */
TEST_F(ContextManagerTest, testMultipleGetContextRequests) {
    ASSERT_EQ(
        SetStateResult::SUCCESS,
        m_contextManager->setState(
            SPEECH_SYNTHESIZER,
            SPEECH_SYNTHESIZER_PAYLOAD_PLAYING,
            StateRefreshPolicy::ALWAYS,
            m_speechSynthesizer->getCurrentstateRequestToken()));
    ASSERT_EQ(
        SetStateResult::SUCCESS,
        m_contextManager->setState(
            AUDIO_PLAYER,
            AUDIO_PLAYER_PAYLOAD,
            StateRefreshPolicy::ALWAYS,
            m_audioPlayer->getCurrentstateRequestToken()));
    m_contextRequester2 = MockContextRequester::create(m_contextManager);
    m_contextManager->getContext(m_contextRequester);
    m_contextManager->getContext(m_contextRequester2);
    ASSERT_TRUE(m_contextRequester->waitForContext(DEFAULT_TIMEOUT));
    ASSERT_EQ(CONTEXT_TEST, m_contextRequester->getContextString());
    ASSERT_TRUE(m_contextRequester2->waitForContext(DEFAULT_TIMEOUT));
    ASSERT_EQ(CONTEXT_TEST, m_contextRequester2->getContextString());
}

/**
 * Set the states with a @c StateRefreshPolicy @c ALWAYS for @c StateProviderInterfaces that are registered with the
 * @c ContextManager. Request for context by calling @c getContext. Expect that the context is returned within the
 * timeout period. Check the context that is returned by the @c ContextManager. Expect it should match the test value.
 * Register a registered @c StateProviderInterface again with the @c ContextManager. Then set the state for the same
 * @c StateProviderInterface with a @c StateRefreshPolicy @c ALWAYS. Expect @c SetStateResult @c SUCCESS is returned.
 * Request for context by calling @c getContext. Expect that the context is returned within the
 * timeout period. Check the context that is returned by the @c ContextManager matches the test context.
 */
TEST_F(ContextManagerTest, testSetProviderTwice) {
    ASSERT_EQ(
        SetStateResult::SUCCESS,
        m_contextManager->setState(
            SPEECH_SYNTHESIZER,
            SPEECH_SYNTHESIZER_PAYLOAD_PLAYING,
            StateRefreshPolicy::ALWAYS,
            m_speechSynthesizer->getCurrentstateRequestToken()));
    ASSERT_EQ(
        SetStateResult::SUCCESS,
        m_contextManager->setState(
            AUDIO_PLAYER,
            AUDIO_PLAYER_PAYLOAD,
            StateRefreshPolicy::ALWAYS,
            m_audioPlayer->getCurrentstateRequestToken()));
    m_contextManager->getContext(m_contextRequester);
    ASSERT_TRUE(m_contextRequester->waitForContext(DEFAULT_TIMEOUT));
    ASSERT_EQ(CONTEXT_TEST, m_contextRequester->getContextString());
    // Call setStateProvider for a registered StateProviderInterface.
    m_contextManager->setStateProvider(SPEECH_SYNTHESIZER, m_speechSynthesizer);
    ASSERT_EQ(
        SetStateResult::SUCCESS,
        m_contextManager->setState(
            SPEECH_SYNTHESIZER,
            SPEECH_SYNTHESIZER_PAYLOAD_PLAYING,
            StateRefreshPolicy::ALWAYS,
            m_speechSynthesizer->getCurrentstateRequestToken()));
    m_contextManager->getContext(m_contextRequester);
    ASSERT_TRUE(m_contextRequester->waitForContext(DEFAULT_TIMEOUT));
    ASSERT_EQ(CONTEXT_TEST, m_contextRequester->getContextString());
}

/**
 * Register a @c StateProviderInterfaces with the @c ContextManager which responds slowly to @provideState requests.
 * Set the states with a @c StateRefreshPolicy @c ALWAYS for @c StateProviderInterfaces. Request for context by calling
 * @c getContext. Expect that failure occurs due to timeout.
 */
TEST_F(ContextManagerTest, testProvideStateTimeout) {
    m_alerts = MockStateProvider::create(
        m_contextManager, ALERTS, ALERTS_PAYLOAD, StateRefreshPolicy::NEVER, TIMEOUT_SLEEP_TIME);
    m_contextManager->setStateProvider(ALERTS, m_alerts);
    ASSERT_EQ(
        SetStateResult::SUCCESS,
        m_contextManager->setState(
            SPEECH_SYNTHESIZER,
            SPEECH_SYNTHESIZER_PAYLOAD_PLAYING,
            StateRefreshPolicy::ALWAYS,
            m_speechSynthesizer->getCurrentstateRequestToken()));
    ASSERT_EQ(
        SetStateResult::SUCCESS,
        m_contextManager->setState(
            AUDIO_PLAYER,
            AUDIO_PLAYER_PAYLOAD,
            StateRefreshPolicy::ALWAYS,
            m_audioPlayer->getCurrentstateRequestToken()));
    ASSERT_EQ(
        SetStateResult::SUCCESS,
        m_contextManager->setState(
            ALERTS, ALERTS_PAYLOAD, StateRefreshPolicy::ALWAYS, m_alerts->getCurrentstateRequestToken()));
    m_contextManager->getContext(m_contextRequester);
    ASSERT_TRUE(m_contextRequester->waitForFailure(FAILURE_TIMEOUT));
}

/**
 * Remove a @c StateProviderInterface that was registered with the @c ContextManager. Set the state with a
 * @c StateRefreshPolicy @c ALWAYS for the @c StateProviderInterface that was unregistered. Expect @c SetStateResult
 * @c STATE_PROVIDER_NOT_REGISTERED is returned.
 */
TEST_F(ContextManagerTest, testRemoveProvider) {
    m_contextManager->setStateProvider(SPEECH_SYNTHESIZER, nullptr);
    ASSERT_EQ(
        SetStateResult::STATE_PROVIDER_NOT_REGISTERED,
        m_contextManager->setState(
            SPEECH_SYNTHESIZER,
            SPEECH_SYNTHESIZER_PAYLOAD_PLAYING,
            StateRefreshPolicy::ALWAYS,
            m_speechSynthesizer->getCurrentstateRequestToken()));
}

/**
 * Request for context by calling @c getContextSet. Wait for context. Set the state for a @c StateProviderInterface
 * that is registered with the @c ContextManager with a wrong token value. Expect that
 * @c SetStateResult @c STATE_TOKEN_OUTDATED is returned.
 */
TEST_F(ContextManagerTest, testIncorrectToken) {
    m_contextManager->getContext(m_contextRequester);
    ASSERT_TRUE(m_contextRequester->waitForContext(DEFAULT_TIMEOUT));
    ASSERT_EQ(
        SetStateResult::STATE_TOKEN_OUTDATED,
        m_contextManager->setState(
            SPEECH_SYNTHESIZER,
            SPEECH_SYNTHESIZER_PAYLOAD_PLAYING,
            StateRefreshPolicy::ALWAYS,
            m_speechSynthesizer->getCurrentstateRequestToken() + 1));
}

/**
 * Set the states with a @c StateRefreshPolicy @c ALWAYS for @c StateProviderInterfaces that are registered with the
 * @c ContextManager. Request for context by calling @c getContext. Expect that the context is returned within the
 * timeout period.
 *
 * There's a dummyProvider with StateRefreshPolicy @c SOMETIMES that returns an empty context.  Check ContextManager is
 * okay with it and would include the context provided by the dummyProvider.
 *
 * Check the context that is returned by the @c ContextManager. Expect it should match the test value.
 */
// ACSDK-1217 - ContextManagerTest::testEmptyProvider fails on Windows
#if !defined(_WIN32) || defined(RESOLVED_ACSDK_1217)
TEST_F(ContextManagerTest, testEmptyProvider) {
    auto dummyProvider = MockStateProvider::create(
        m_contextManager, DUMMY_PROVIDER, "", StateRefreshPolicy::SOMETIMES, DEFAULT_SLEEP_TIME);
    m_contextManager->setStateProvider(DUMMY_PROVIDER, dummyProvider);

    ASSERT_EQ(
        SetStateResult::SUCCESS,
        m_contextManager->setState(
            SPEECH_SYNTHESIZER,
            SPEECH_SYNTHESIZER_PAYLOAD_PLAYING,
            StateRefreshPolicy::ALWAYS,
            m_speechSynthesizer->getCurrentstateRequestToken()));
    ASSERT_EQ(
        SetStateResult::SUCCESS,
        m_contextManager->setState(
            AUDIO_PLAYER,
            AUDIO_PLAYER_PAYLOAD,
            StateRefreshPolicy::ALWAYS,
            m_audioPlayer->getCurrentstateRequestToken()));
    ASSERT_EQ(
        SetStateResult::SUCCESS,
        m_contextManager->setState(
            DUMMY_PROVIDER, "", StateRefreshPolicy::ALWAYS, dummyProvider->getCurrentstateRequestToken()));
    m_contextManager->getContext(m_contextRequester);
    ASSERT_TRUE(m_contextRequester->waitForContext(DEFAULT_TIMEOUT));
    ASSERT_EQ(CONTEXT_TEST, m_contextRequester->getContextString());
}
#endif

}  // namespace test
}  // namespace contextManager
}  // namespace alexaClientSDK
