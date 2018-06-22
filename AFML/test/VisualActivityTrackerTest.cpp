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

/// @file VisualActivityTrackerTest.cpp
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

#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>

#include "AFML/VisualActivityTracker.h"

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
static const std::string NAMESPACE_AUDIO_ACTIVITY_TRACKER("VisualActivityTracker");

/// The @c NamespaceAndName to send to the @c ContextManager.
static const NamespaceAndName NAMESPACE_AND_NAME_STATE{NAMESPACE_AUDIO_ACTIVITY_TRACKER, "ActivityState"};

/// Provide State Token for testing.
static const unsigned int PROVIDE_STATE_TOKEN_TEST{1};

/// The default Visual Channel name.
static const std::string VISUAL_CHANNEL_NAME{"Visual"};

/// The default Visual Interface name.
static const std::string VISUAL_INTERFACE_NAME{"TempateRuntime"};

/// The default Visual Channel priority.
static unsigned int VISUAL_CHANNEL_PRIORITY{100};

/// The default Invalid Channel name.
static const std::string INVALID_CHANNEL_NAME{"Invalid"};

/// The default Channel priority for the invalid channel.
static unsigned int INVALID_CHANNEL_PRIORITY{300};

/// Timeout to sleep before asking for provideState().
static const std::chrono::milliseconds SHORT_TIMEOUT_MS = std::chrono::milliseconds(5);

/// Test harness for @c VisualActivityTrackerTest class.
class VisualActivityTrackerTest : public ::testing::Test {
public:
    /// A constructor which initializes the promises and futures needed for the test class.
    VisualActivityTrackerTest();

    /// Set up the test harness for running a test.
    void SetUp() override;

    /// Clean up the test harness after running a test.
    void TearDown() override;

    /// @c VisualActivityTracker to test
    std::shared_ptr<VisualActivityTracker> m_VisualActivityTracker;

    /// @c ContextManager to provide state and update state.
    std::shared_ptr<MockContextManager> m_mockContextManager;

    /// A visualChannel used for testing.
    std::shared_ptr<Channel> m_visualChannel;

    /**
     * Verify that the provided state matches the expected state
     *
     * @param jsonState The state to verify
     * @param channels The set of channels that's passed into the VisualActivityTracker
     */
    void verifyState(const std::string& providedState, const std::vector<Channel::State>& channels);

    /**
     * A helper function to verify the context provided by the VisualActivityTracker matches the set the channels
     * notified via notifyOfActivityUpdates().
     *
     * @param channels The set of channels that's passed into the VisualActivityTracker
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

VisualActivityTrackerTest::VisualActivityTrackerTest() : m_wakeSetStateFuture{m_wakeSetStatePromise.get_future()} {
}

void VisualActivityTrackerTest::SetUp() {
    m_mockContextManager = std::make_shared<NiceMock<MockContextManager>>();
    m_VisualActivityTracker = VisualActivityTracker::create(m_mockContextManager);
    ASSERT_TRUE(m_mockContextManager != nullptr);

    m_visualChannel = std::make_shared<Channel>(VISUAL_CHANNEL_NAME, VISUAL_CHANNEL_PRIORITY);
    m_visualChannel->setInterface(VISUAL_INTERFACE_NAME);
    ASSERT_TRUE(m_visualChannel != nullptr);
}

void VisualActivityTrackerTest::TearDown() {
    m_VisualActivityTracker->shutdown();
}

void VisualActivityTrackerTest::verifyState(
    const std::string& providedState,
    const std::vector<Channel::State>& channels) {
    rapidjson::Document jsonContent;
    jsonContent.Parse(providedState);

    // VisualActivityTracker should return empty context if otherwise if the vector is empty.
    if (channels.size() == 0) {
        ASSERT_TRUE(providedState.empty());
        return;
    }

    // VisualActivityTracker should return empty context if any channel is not VISUAL_CHANNEL.
    for (auto& channel : channels) {
        if (FocusManagerInterface::VISUAL_CHANNEL_NAME != channel.name) {
            ASSERT_TRUE(providedState.empty());
            return;
        }
    }

    // Get the last element of the channels vector as that's the most recent updates.
    const auto& channel = channels.back();

    // If channel is not active, VisualActivityTracker should return empty context as well.
    if (FocusState::NONE == channel.focusState) {
        ASSERT_TRUE(providedState.empty());
        return;
    }

    // Get "focused" node.
    rapidjson::Value::ConstMemberIterator focusNode;
    ASSERT_TRUE(jsonUtils::findNode(jsonContent, "focused", &focusNode));

    // Get and verify interface name.
    std::string interfaceName;
    ASSERT_TRUE(jsonUtils::retrieveValue(focusNode->value, "interface", &interfaceName));
    ASSERT_EQ(interfaceName, channel.interfaceName);
}

void VisualActivityTrackerTest::provideUpdate(const std::vector<Channel::State>& channels) {
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
            InvokeWithoutArgs(this, &VisualActivityTrackerTest::wakeOnSetState)));

    m_VisualActivityTracker->notifyOfActivityUpdates(channels);
    std::this_thread::sleep_for(SHORT_TIMEOUT_MS);
    m_VisualActivityTracker->provideState(NAMESPACE_AND_NAME_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
}

SetStateResult VisualActivityTrackerTest::wakeOnSetState() {
    m_wakeSetStatePromise.set_value();
    return SetStateResult::SUCCESS;
}

/// Test if there's no activity updates, VisualActivityTracker will return an empty context.
TEST_F(VisualActivityTrackerTest, noActivityUpdate) {
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(NAMESPACE_AND_NAME_STATE, "", StateRefreshPolicy::SOMETIMES, PROVIDE_STATE_TOKEN_TEST))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &VisualActivityTrackerTest::wakeOnSetState));

    m_VisualActivityTracker->provideState(NAMESPACE_AND_NAME_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
}

/// Test if there's an empty vector of activity updates, VisualActivityTracker will return an empty context.
TEST_F(VisualActivityTrackerTest, emptyActivityUpdate) {
    std::vector<Channel::State> channels;
    provideUpdate(channels);
}

/// Test if there's an activityUpdate for one idle channel, VisualActivityTracker will return an empty context.
TEST_F(VisualActivityTrackerTest, oneIdleChannel) {
    std::vector<Channel::State> channels;
    m_visualChannel->setFocus(FocusState::NONE);
    channels.push_back(m_visualChannel->getState());
    provideUpdate(channels);
}

/// Test if there's an activityUpdate for one active channel, context will be reported correctly.
TEST_F(VisualActivityTrackerTest, oneActiveChannel) {
    std::vector<Channel::State> channels;
    m_visualChannel->setFocus(FocusState::FOREGROUND);
    channels.push_back(m_visualChannel->getState());
    provideUpdate(channels);
}

/*
 * Test if there's an vector of activity updates with one valid and one invalid channel, VisualActivityTracker will
 * return an empty context.
 */
TEST_F(VisualActivityTrackerTest, invalidChannelActivityUpdate) {
    std::vector<Channel::State> channels;
    auto invalidChannel = std::make_shared<Channel>(INVALID_CHANNEL_NAME, INVALID_CHANNEL_PRIORITY);
    m_visualChannel->setFocus(FocusState::FOREGROUND);
    channels.push_back(m_visualChannel->getState());
    channels.push_back(invalidChannel->getState());
    provideUpdate(channels);
}

/*
 * Test if there's an vector of activity updates with one valid channel, VisualActivityTracker take the state from the
 * last element of the vector.
 */
TEST_F(VisualActivityTrackerTest, validChannelTwoActivityUpdates) {
    std::vector<Channel::State> channels;
    m_visualChannel->setFocus(FocusState::FOREGROUND);
    channels.push_back(m_visualChannel->getState());
    m_visualChannel->setFocus(FocusState::BACKGROUND);
    channels.push_back(m_visualChannel->getState());
    provideUpdate(channels);
}

}  // namespace test
}  // namespace afml
}  // namespace alexaClientSDK