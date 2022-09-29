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
#include <memory>

#include <acsdk/SpeakerManager/private/SpeakerManagerConfigHelper.h>
#include <acsdk/SpeakerManager/SpeakerManagerStorageState.h>
#include <acsdk/SpeakerManager/test/MockSpeakerManagerConfig.h>
#include <acsdk/SpeakerManager/test/MockSpeakerManagerStorage.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerObserverInterface.h>

namespace alexaClientSDK {
namespace speakerManager {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::avs::speakerConstants;
using namespace avsCommon::sdkInterfaces;
using namespace ::testing;

/// @addtogroup Test_acsdkSpeakerManager
/// @{

/// @brief Test fixture for SpeakerManagerConfigHelper.
class SpeakerManagerConfigHelperTest : public Test {
public:
    SpeakerManagerConfigHelperTest();

protected:
    /// SetUp before each test.
    void SetUp() override;

    /// TearDown after each test.
    void TearDown() override;

    /// Upstream interface mock
    std::shared_ptr<NiceMock<MockSpeakerManagerStorage>> m_stubStorage;
    /// Upstream interface mock
    std::shared_ptr<MockSpeakerManagerConfig> m_stubConfig;
};

/// @}

SpeakerManagerConfigHelperTest::SpeakerManagerConfigHelperTest() : m_stubStorage() {
}

void SpeakerManagerConfigHelperTest::SetUp() {
    m_stubStorage = std::make_shared<NiceMock<MockSpeakerManagerStorage>>();
    m_stubConfig = std::make_shared<NiceMock<MockSpeakerManagerConfig>>();
}

void SpeakerManagerConfigHelperTest::TearDown() {
    m_stubStorage.reset();
    m_stubConfig.reset();
}

TEST_F(SpeakerManagerConfigHelperTest, test_initDoesntCallLoadSave) {
    EXPECT_CALL(*m_stubStorage, loadState(_)).Times(0);
    EXPECT_CALL(*m_stubStorage, saveState(_)).Times(0);

    SpeakerManagerConfigHelper helper(m_stubConfig, m_stubStorage);
}

TEST_F(SpeakerManagerConfigHelperTest, test_getPersistentStorageFromConfiguration) {
    EXPECT_CALL(*m_stubStorage, loadState(_)).Times(0);
    EXPECT_CALL(*m_stubStorage, saveState(_)).Times(0);

    // Provide a valid configuration.
    EXPECT_CALL(*m_stubConfig, getPersistentStorage(_)).Times(1).WillOnce(Invoke([](bool& persistentStorage) {
        persistentStorage = true;
        return true;
    }));

    SpeakerManagerConfigHelper helper(m_stubConfig, m_stubStorage);
    ASSERT_TRUE(helper.getPersistentStorage());
}

TEST_F(SpeakerManagerConfigHelperTest, test_getMinUnmuteVolumeFromConfiguration) {
    EXPECT_CALL(*m_stubStorage, loadState(_)).Times(0);
    EXPECT_CALL(*m_stubStorage, saveState(_)).Times(0);

    // Provide a valid configuration.
    EXPECT_CALL(*m_stubConfig, getMinUnmuteVolume(_)).Times(1).WillOnce(Invoke([](std::uint8_t& res) {
        res = 3;
        return true;
    }));

    SpeakerManagerConfigHelper helper(m_stubConfig, m_stubStorage);
    ASSERT_EQ(3, helper.getMinUnmuteVolume());
}

TEST_F(SpeakerManagerConfigHelperTest, test_getPersistentStorageReturnsDefaults) {
    EXPECT_CALL(*m_stubStorage, loadState(_)).Times(0);
    EXPECT_CALL(*m_stubStorage, saveState(_)).Times(0);

    // Provide an empty configuration.
    EXPECT_CALL(*m_stubStorage, loadState(_)).Times(0);

    SpeakerManagerConfigHelper helper(m_stubConfig, m_stubStorage);
    ASSERT_FALSE(helper.getPersistentStorage());
}

TEST_F(SpeakerManagerConfigHelperTest, test_getMinUnmuteVolumeReturnsDefaults) {
    EXPECT_CALL(*m_stubStorage, loadState(_)).Times(0);
    EXPECT_CALL(*m_stubStorage, saveState(_)).Times(0);

    // Provide an empty configuration.
    EXPECT_CALL(*m_stubConfig, getMinUnmuteVolume(_)).Times(1).WillOnce(Return(false));

    SpeakerManagerConfigHelper helper(m_stubConfig, m_stubStorage);
    ASSERT_EQ(MIN_UNMUTE_VOLUME, helper.getMinUnmuteVolume());
}

TEST_F(SpeakerManagerConfigHelperTest, test_getRestoreMuteStateReturnsDefaults) {
    // Provide an empty configuration.
    EXPECT_CALL(*m_stubConfig, getRestoreMuteState(_)).Times(1).WillOnce(Return(false));

    SpeakerManagerConfigHelper helper(m_stubConfig, m_stubStorage);
    ASSERT_TRUE(helper.getRestoreMuteState());
}

TEST_F(SpeakerManagerConfigHelperTest, test_getPersistentStorageReturnsTrue) {
    EXPECT_CALL(*m_stubConfig, getPersistentStorage(_)).Times(1).WillOnce(Invoke([](bool& persistentStorage) {
        persistentStorage = true;
        return true;
    }));

    SpeakerManagerConfigHelper helper(m_stubConfig, m_stubStorage);
    ASSERT_TRUE(helper.getPersistentStorage());
}

TEST_F(SpeakerManagerConfigHelperTest, test_getPersistentStorageReturnsFalse) {
    EXPECT_CALL(*m_stubConfig, getPersistentStorage(_)).Times(1).WillOnce(Invoke([](bool& persistentStorage) {
        persistentStorage = false;
        return true;
    }));

    SpeakerManagerConfigHelper helper(m_stubConfig, m_stubStorage);
    ASSERT_FALSE(helper.getPersistentStorage());
}

TEST_F(SpeakerManagerConfigHelperTest, test_getRestoreMuteStateReturnsTrue) {
    EXPECT_CALL(*m_stubConfig, getRestoreMuteState(_)).Times(1).WillOnce(Invoke([](bool& restoreMuteState) {
        restoreMuteState = true;
        return true;
    }));

    SpeakerManagerConfigHelper helper(m_stubConfig, m_stubStorage);
    ASSERT_TRUE(helper.getRestoreMuteState());
}

TEST_F(SpeakerManagerConfigHelperTest, test_getRestoreMuteStateReturnsFalse) {
    EXPECT_CALL(*m_stubConfig, getRestoreMuteState(_)).Times(1).WillOnce(Invoke([](bool& restoreMuteState) {
        restoreMuteState = false;
        return true;
    }));

    SpeakerManagerConfigHelper helper(m_stubConfig, m_stubStorage);
    ASSERT_FALSE(helper.getRestoreMuteState());
}

TEST_F(SpeakerManagerConfigHelperTest, test_loadStateDelegate) {
    EXPECT_CALL(*m_stubStorage, loadState(_)).Times(1);
    EXPECT_CALL(*m_stubStorage, saveState(_)).Times(0);

    SpeakerManagerConfigHelper helper(m_stubConfig, m_stubStorage);

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

    EXPECT_CALL(*m_stubConfig, getDefaultSpeakerVolume(_))
        .Times(1)
        .WillOnce(Invoke([](std::uint8_t& defaultSpeakerVolume) {
            defaultSpeakerVolume = 5;
            return true;
        }));
    EXPECT_CALL(*m_stubConfig, getDefaultAlertsVolume(_))
        .Times(1)
        .WillOnce(Invoke([](std::uint8_t& defaultSpeakerVolume) {
            defaultSpeakerVolume = 6;
            return true;
        }));

    SpeakerManagerConfigHelper helper(m_stubConfig, m_stubStorage);

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

    EXPECT_CALL(*m_stubConfig, getDefaultSpeakerVolume(_)).Times(1).WillOnce(Return(false));
    EXPECT_CALL(*m_stubConfig, getDefaultAlertsVolume(_)).Times(1).WillOnce(Return(false));

    SpeakerManagerConfigHelper helper(m_stubConfig, m_stubStorage);

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

    SpeakerManagerConfigHelper helper(m_stubConfig, m_stubStorage);
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
}  // namespace alexaClientSDK
