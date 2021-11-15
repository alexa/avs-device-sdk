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

#include <algorithm>
#include <memory>
#include <sstream>
#include <set>
#include <vector>
#include <future>

#include <AVSCommon/SDKInterfaces/SpeakerManagerObserverInterface.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <SpeakerManager/SpeakerManagerConstants.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "SpeakerManager/SpeakerManagerConfigHelper.h"
#include "SpeakerManager/SpeakerManagerStorageState.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace speakerManager {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::avs::speakerConstants;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::memory;
using namespace rapidjson;
using namespace ::testing;
using namespace alexaClientSDK::capabilityAgents::speakerManager;

static const std::string JSON_TEST_CONFIG =
    "{\"speakerManagerCapabilityAgent\":{\"minUnmuteVolume\":3,\"defaultSpeakerVolume\":5,\"defaultAlertsVolume\":6,"
    "\"restoreMuteState\":true}}";
static const std::string JSON_TEST_CONFIG_NO_MUTE =
    "{\"speakerManagerCapabilityAgent\":{\"minUnmuteVolume\":3,\"defaultSpeakerVolume\":5,\"defaultAlertsVolume\":6,"
    "\"restoreMuteState\":false}}";

class MockSpeakerManagerStorageInterface : public SpeakerManagerStorageInterface {
public:
    MOCK_METHOD1(loadState, bool(SpeakerManagerStorageState&));
    MOCK_METHOD1(saveState, bool(const SpeakerManagerStorageState&));
};

class SpeakerManagerConfigHelperTest : public Test {
public:
    SpeakerManagerConfigHelperTest();

protected:
    /// SetUp before each test.
    void SetUp() override;

    /// TearDown after each test.
    void TearDown() override;

    // Upstream interface mock
    std::shared_ptr<NiceMock<MockSpeakerManagerStorageInterface>> m_stubStorage;
};

SpeakerManagerConfigHelperTest::SpeakerManagerConfigHelperTest() : m_stubStorage() {
}

void SpeakerManagerConfigHelperTest::SetUp() {
    m_stubStorage = std::make_shared<NiceMock<MockSpeakerManagerStorageInterface>>();
}

void SpeakerManagerConfigHelperTest::TearDown() {
    m_stubStorage.reset();
}

TEST_F(SpeakerManagerConfigHelperTest, test_initDoesntCallLoadSave) {
    EXPECT_CALL(*m_stubStorage, loadState(_)).Times(0);
    EXPECT_CALL(*m_stubStorage, saveState(_)).Times(0);

    SpeakerManagerConfigHelper helper(m_stubStorage);
}

TEST_F(SpeakerManagerConfigHelperTest, test_getMinUnmuteVolumeFromConfiguration) {
    EXPECT_CALL(*m_stubStorage, loadState(_)).Times(0);
    EXPECT_CALL(*m_stubStorage, saveState(_)).Times(0);

    // Provide a valid configuration.
    std::shared_ptr<std::istream> istr(new std::istringstream(JSON_TEST_CONFIG));
    ConfigurationNode::uninitialize();
    ASSERT_TRUE(ConfigurationNode::initialize({istr}));

    SpeakerManagerConfigHelper helper(m_stubStorage);
    ASSERT_EQ(3, helper.getMinUnmuteVolume());
}

TEST_F(SpeakerManagerConfigHelperTest, test_getMinUnmuteVolumeReturnsDefaults) {
    EXPECT_CALL(*m_stubStorage, loadState(_)).Times(0);
    EXPECT_CALL(*m_stubStorage, saveState(_)).Times(0);

    // Provide an empty configuration.
    ConfigurationNode::uninitialize();
    ASSERT_TRUE(ConfigurationNode::initialize({}));

    SpeakerManagerConfigHelper helper(m_stubStorage);
    ASSERT_EQ(MIN_UNMUTE_VOLUME, helper.getMinUnmuteVolume());
}

TEST_F(SpeakerManagerConfigHelperTest, test_getRestoreMuteStateReturnsDefaults) {
    // Provide an empty configuration.
    ConfigurationNode::uninitialize();
    ASSERT_TRUE(ConfigurationNode::initialize({}));

    SpeakerManagerConfigHelper helper(m_stubStorage);
    ASSERT_TRUE(helper.getRestoreMuteState());
}

TEST_F(SpeakerManagerConfigHelperTest, test_getRestoreMuteStateReturnsTrue) {
    ConfigurationNode::uninitialize();
    std::shared_ptr<std::istream> istr(new std::istringstream(JSON_TEST_CONFIG));
    ASSERT_TRUE(ConfigurationNode::initialize({istr}));

    SpeakerManagerConfigHelper helper(m_stubStorage);
    ASSERT_TRUE(helper.getRestoreMuteState());
}

TEST_F(SpeakerManagerConfigHelperTest, test_getRestoreMuteStateReturnsFalse) {
    ConfigurationNode::uninitialize();
    std::shared_ptr<std::istream> istr(new std::istringstream(JSON_TEST_CONFIG_NO_MUTE));
    ASSERT_TRUE(ConfigurationNode::initialize({istr}));

    SpeakerManagerConfigHelper helper(m_stubStorage);
    ASSERT_FALSE(helper.getRestoreMuteState());
}

TEST_F(SpeakerManagerConfigHelperTest, test_loadStateDelegate) {
    EXPECT_CALL(*m_stubStorage, loadState(_)).Times(1);
    EXPECT_CALL(*m_stubStorage, saveState(_)).Times(0);

    SpeakerManagerConfigHelper helper(m_stubStorage);

    ON_CALL(*m_stubStorage, loadState(_)).WillByDefault(Invoke([](SpeakerManagerStorageState& state) {
        state.speakerChannelState.channelVolume = 10;
        state.speakerChannelState.channelMuteStatus = true;
        state.alertsChannelState.channelMuteStatus = false;
        state.alertsChannelState.channelVolume = 20;
        return true;
    }));

    SpeakerManagerStorageState state = {{255, false}, {255, false}};
    helper.loadState(state);

    EXPECT_EQ(10, state.speakerChannelState.channelVolume);
    EXPECT_TRUE(state.speakerChannelState.channelMuteStatus);
    EXPECT_EQ(20, state.alertsChannelState.channelVolume);
    EXPECT_FALSE(state.alertsChannelState.channelMuteStatus);
}

TEST_F(SpeakerManagerConfigHelperTest, test_loadStateFromConfig) {
    EXPECT_CALL(*m_stubStorage, loadState(_)).Times(1);
    EXPECT_CALL(*m_stubStorage, saveState(_)).Times(0);

    std::shared_ptr<std::istream> istr(new std::istringstream(JSON_TEST_CONFIG));
    ConfigurationNode::uninitialize();
    ASSERT_TRUE(ConfigurationNode::initialize({istr}));

    SpeakerManagerConfigHelper helper(m_stubStorage);

    ON_CALL(*m_stubStorage, loadState(_)).WillByDefault(Return(false));

    SpeakerManagerStorageState state = {{255, true}, {255, true}};
    helper.loadState(state);

    EXPECT_EQ(5, state.speakerChannelState.channelVolume);
    EXPECT_FALSE(state.speakerChannelState.channelMuteStatus);
    EXPECT_EQ(6, state.alertsChannelState.channelVolume);
    EXPECT_FALSE(state.alertsChannelState.channelMuteStatus);
}

TEST_F(SpeakerManagerConfigHelperTest, test_loadStateDefaults) {
    EXPECT_CALL(*m_stubStorage, loadState(_)).Times(1);
    EXPECT_CALL(*m_stubStorage, saveState(_)).Times(0);

    ConfigurationNode::uninitialize();
    ASSERT_TRUE(ConfigurationNode::initialize({}));

    SpeakerManagerConfigHelper helper(m_stubStorage);

    ON_CALL(*m_stubStorage, loadState(_)).WillByDefault(Return(false));

    SpeakerManagerStorageState state = {{255, false}, {255, false}};
    helper.loadState(state);

    EXPECT_EQ(DEFAULT_SPEAKER_VOLUME, state.speakerChannelState.channelVolume);
    EXPECT_FALSE(state.speakerChannelState.channelMuteStatus);
    EXPECT_EQ(DEFAULT_ALERTS_VOLUME, state.alertsChannelState.channelVolume);
    EXPECT_FALSE(state.alertsChannelState.channelMuteStatus);
}

TEST_F(SpeakerManagerConfigHelperTest, test_saveState) {
    EXPECT_CALL(*m_stubStorage, loadState(_)).Times(0);
    EXPECT_CALL(*m_stubStorage, saveState(_)).Times(1);

    SpeakerManagerConfigHelper helper(m_stubStorage);
    SpeakerManagerStorageState saved = {{0, true}, {0, true}};

    ON_CALL(*m_stubStorage, saveState(_)).WillByDefault(Invoke([&saved](const SpeakerManagerStorageState& state) {
        saved = state;
        return true;
    }));

    SpeakerManagerStorageState state = {{255, false}, {255, false}};
    ASSERT_TRUE(helper.saveState(state));

    ASSERT_EQ(255, saved.speakerChannelState.channelVolume);
    ASSERT_FALSE(saved.speakerChannelState.channelMuteStatus);
    ASSERT_EQ(255, saved.alertsChannelState.channelVolume);
    ASSERT_FALSE(saved.alertsChannelState.channelMuteStatus);
}

}  // namespace test
}  // namespace speakerManager
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
