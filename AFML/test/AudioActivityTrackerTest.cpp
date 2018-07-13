/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

/// @file AudioActivityTrackerTest.cpp
#include <chrono>
#include <future>
#include <memory>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/String/StringUtils.h>

#include "AFML/AudioActivityTracker.h"

namespace alexaClientSDK {
namespace afml {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils::json;
using namespace ::testing;

/// Plenty of time for a test to complete.
static std::chrono::milliseconds WAIT_TIMEOUT(1000);

/// Namespace for AudioActivityTracke.
static const std::string NAMESPACE_AUDIO_ACTIVITY_TRACKER("AudioActivityTracker");

/// The @c NamespaceAndName to send to the @c ContextManager.
static const NamespaceAndName NAMESPACE_AND_NAME_STATE{NAMESPACE_AUDIO_ACTIVITY_TRACKER, "ActivityState"};

/// Provide State Token for testing.
static const unsigned int PROVIDE_STATE_TOKEN_TEST{1};

/// The default Dialog Channel name.
static const std::string DIALOG_CHANNEL_NAME{"Dialog"};

/// The default dialog Channel priority.
static unsigned int DIALOG_CHANNEL_PRIORITY{100};

/// The default Dialog interface name.
static const std::string DIALOG_INTERFACE_NAME{"SpeechSynthesizer"};

/// The SpeechRecognizer interface name.
static const std::string AIP_INTERFACE_NAME{"SpeechRecognizer"};

/// The default Content Channel name.
static const std::string CONTENT_CHANNEL_NAME{"Content"};

/// The default Content interface name.
static const std::string CONTENT_INTERFACE_NAME{"AudioPlayer"};

/// The default Content Channel priority.
static unsigned int CONTENT_CHANNEL_PRIORITY{300};

/// Timeout to sleep before asking for provideState().
static const std::chrono::milliseconds SHORT_TIMEOUT_MS = std::chrono::milliseconds(5);

class AudioActivityTrackerTest : public ::testing::Test {
public:
    AudioActivityTrackerTest();

    void SetUp() override;
    void TearDown() override;

    /// @c AudioActivityTracker to test
    std::shared_ptr<AudioActivityTracker> m_audioActivityTracker;

    /// @c ContextManager to provide state and update state.
    std::shared_ptr<MockContextManager> m_mockContextManager;

    /// A dialogChannel used for testing.
    std::shared_ptr<Channel> m_dialogChannel;

    /// A contentChannel used for testing.
    std::shared_ptr<Channel> m_contentChannel;

    /**
     * Verify that the provided state matches the expected state
     *
     * @param jsonState The state to verify
     * @param channels The set of channels that's passed into the AudioActivityTracker
     */
    void verifyState(const std::string& providedState, const std::vector<Channel::State>& channels);

    /**
     * A helper function to verify the context provided by the AudioActivityTracker matches the set the channels
     * notified via notifyOfActivityUpdates().
     *
     * @param channels The set of channels that's passed into the AudioActivityTracker
     */
    void provideUpdate(const std::vector<Channel::State>& channels);

    /**
     * This is invoked in response to a @c setState call.
     *
     * @return @c SUCCESS.
     */
    SetStateResult wakeOnSetState();

    /// Promise to be fulfilled when @c setState is called.
    std::promise<void> m_wakeSetStatePromise;

    /// Future to notify when @c setState is called.
    std::future<void> m_wakeSetStateFuture;
};

AudioActivityTrackerTest::AudioActivityTrackerTest() :
        m_wakeSetStatePromise{},
        m_wakeSetStateFuture{m_wakeSetStatePromise.get_future()} {
}

void AudioActivityTrackerTest::SetUp() {
    m_mockContextManager = std::make_shared<NiceMock<MockContextManager>>();
    m_audioActivityTracker = AudioActivityTracker::create(m_mockContextManager);
    ASSERT_TRUE(m_mockContextManager != nullptr);

    m_dialogChannel = std::make_shared<Channel>(DIALOG_CHANNEL_NAME, DIALOG_CHANNEL_PRIORITY);
    m_dialogChannel->setInterface(DIALOG_INTERFACE_NAME);
    ASSERT_TRUE(m_dialogChannel != nullptr);

    m_contentChannel = std::make_shared<Channel>(CONTENT_CHANNEL_NAME, CONTENT_CHANNEL_PRIORITY);
    m_contentChannel->setInterface(CONTENT_INTERFACE_NAME);
    ASSERT_TRUE(m_contentChannel != nullptr);
}

void AudioActivityTrackerTest::TearDown() {
    m_audioActivityTracker->shutdown();
}

void AudioActivityTrackerTest::verifyState(
    const std::string& providedState,
    const std::vector<Channel::State>& channels) {
    rapidjson::Document jsonContent;
    jsonContent.Parse(providedState);

    for (auto& channel : channels) {
        rapidjson::Value::ConstMemberIterator channelNode;

        // channel name needs to be in lower case.
        auto channelName = avsCommon::utils::string::stringToLowerCase(channel.name);
        ASSERT_TRUE(jsonUtils::findNode(jsonContent, channelName, &channelNode));

        // Get and verify interface name.
        std::string interfaceName;
        std::string expectedInterfaceName = channel.interfaceName;
        // There is an requirement such that "SpeechRecognizer" is not a valid interface for the Dialog Channel.
        if (AIP_INTERFACE_NAME == expectedInterfaceName) {
            expectedInterfaceName = DIALOG_INTERFACE_NAME;
        }
        ASSERT_TRUE(jsonUtils::retrieveValue(channelNode->value, "interface", &interfaceName));
        ASSERT_EQ(interfaceName, expectedInterfaceName);

        // Get and verify idleTimeInMilliseconds.
        int64_t idleTime;
        bool isChannelActive = FocusState::NONE != channel.focusState;
        // If interface is "SpeechRecognizer", then channel should expected to be not active instead.
        if (AIP_INTERFACE_NAME == channel.interfaceName) {
            isChannelActive = false;
        }
        ASSERT_TRUE(jsonUtils::retrieveValue(channelNode->value, "idleTimeInMilliseconds", &idleTime));
        if (isChannelActive) {
            ASSERT_EQ(idleTime, 0);
        } else {
            ASSERT_NE(idleTime, 0);
        }
    }
}

void AudioActivityTrackerTest::provideUpdate(const std::vector<Channel::State>& channels) {
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(NAMESPACE_AND_NAME_STATE, _, StateRefreshPolicy::SOMETIMES, PROVIDE_STATE_TOKEN_TEST))
        .Times(1)
        .WillOnce(DoAll(
            // need to include all four arguments, but only care about jsonState
            Invoke([this, &channels](
                       const avsCommon::avs::NamespaceAndName& namespaceAndName,
                       const std::string& jsonState,
                       const avsCommon::avs::StateRefreshPolicy& refreshPolicy,
                       const unsigned int stateRequestToken) { verifyState(jsonState, channels); }),
            InvokeWithoutArgs(this, &AudioActivityTrackerTest::wakeOnSetState)));

    m_audioActivityTracker->notifyOfActivityUpdates(channels);
    std::this_thread::sleep_for(SHORT_TIMEOUT_MS);
    m_audioActivityTracker->provideState(NAMESPACE_AND_NAME_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
}

SetStateResult AudioActivityTrackerTest::wakeOnSetState() {
    m_wakeSetStatePromise.set_value();
    return SetStateResult::SUCCESS;
}

/// Test if there's no activity updates, AudioActivityTracker will return an empty context.
TEST_F(AudioActivityTrackerTest, noActivityUpdate) {
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(NAMESPACE_AND_NAME_STATE, "", StateRefreshPolicy::SOMETIMES, PROVIDE_STATE_TOKEN_TEST))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &AudioActivityTrackerTest::wakeOnSetState));

    m_audioActivityTracker->provideState(NAMESPACE_AND_NAME_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
}

/// Test if there's an empty set of activity updates, AudioActivityTracker will return an empty context.
TEST_F(AudioActivityTrackerTest, emptyActivityUpdate) {
    const std::vector<Channel::State> channels;
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(NAMESPACE_AND_NAME_STATE, "", StateRefreshPolicy::SOMETIMES, PROVIDE_STATE_TOKEN_TEST))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &AudioActivityTrackerTest::wakeOnSetState));

    m_audioActivityTracker->notifyOfActivityUpdates(channels);
    m_audioActivityTracker->provideState(NAMESPACE_AND_NAME_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
}

/// Test if there's an activityUpdate for one active channel, context will be reported correctly.
TEST_F(AudioActivityTrackerTest, oneActiveChannel) {
    std::vector<Channel::State> channels;
    m_dialogChannel->setFocus(FocusState::FOREGROUND);
    channels.push_back(m_dialogChannel->getState());
    provideUpdate(channels);
}

/*
 * Test if there's an activityUpdate for one Dialog channel with "SpeechRecognizer" as an interface,
 * AudioActivityTracker will ignore it and report empty context.
 */
TEST_F(AudioActivityTrackerTest, oneActiveChannelWithAIPAsInterface) {
    std::vector<Channel::State> channels;
    m_dialogChannel->setInterface(AIP_INTERFACE_NAME);
    m_dialogChannel->setFocus(FocusState::FOREGROUND);
    channels.push_back(m_dialogChannel->getState());
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(NAMESPACE_AND_NAME_STATE, "", StateRefreshPolicy::SOMETIMES, PROVIDE_STATE_TOKEN_TEST))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &AudioActivityTrackerTest::wakeOnSetState));

    m_audioActivityTracker->notifyOfActivityUpdates(channels);
    std::this_thread::sleep_for(SHORT_TIMEOUT_MS);
    m_audioActivityTracker->provideState(NAMESPACE_AND_NAME_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
}

/*
 * Test if there's an activityUpdate for one Dialog channel with "SpeechSynthesizer" and then "SpeechRecognizer" as an
 * interface, AudioActivityTracker will ignore the "SpeechRecognizer" interface going active but report
 * "SpeechSynthesizer" with idleTime not equal to zero.
 */
TEST_F(AudioActivityTrackerTest, oneActiveChannelWithDefaultAndAIPAsInterfaces) {
    std::vector<Channel::State> channels;
    m_dialogChannel->setFocus(FocusState::FOREGROUND);
    channels.push_back(m_dialogChannel->getState());
    provideUpdate(channels);
}

/// Test if there's an activityUpdate for two active channels, context will be reported correctly.
TEST_F(AudioActivityTrackerTest, twoActiveChannels) {
    std::vector<Channel::State> channels;
    m_dialogChannel->setFocus(FocusState::FOREGROUND);
    m_contentChannel->setFocus(FocusState::BACKGROUND);
    channels.push_back(m_dialogChannel->getState());
    channels.push_back(m_contentChannel->getState());
    provideUpdate(channels);
}

/// Test if there's an activityUpdate for one active and one idle channels, context will be reported correctly.
TEST_F(AudioActivityTrackerTest, oneActiveOneIdleChannels) {
    std::vector<Channel::State> channels;
    m_dialogChannel->setFocus(FocusState::FOREGROUND);
    m_contentChannel->setFocus(FocusState::BACKGROUND);
    m_dialogChannel->setFocus(FocusState::NONE);
    m_contentChannel->setFocus(FocusState::FOREGROUND);
    channels.push_back(m_dialogChannel->getState());
    channels.push_back(m_contentChannel->getState());
    provideUpdate(channels);
}

}  // namespace test
}  // namespace afml
}  // namespace alexaClientSDK
