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
#include <memory>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <AVSCommon/SDKInterfaces/MockSpeakerInterface.h>

#include <acsdk/SpeakerManager/private/ChannelVolumeManager.h>

namespace alexaClientSDK {
namespace speakerManager {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::avs::speakerConstants;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace ::testing;

/// Initial Settings for underlying @c SpeakerInterface.
static const SpeakerInterface::SpeakerSettings INITIAL_SETTINGS{AVS_SET_VOLUME_MAX / 2, false};

/// @brief Test fixture for ChannelVolumeManager.
/// @ingroup Test_acsdkSpeakerManager
class ChannelVolumeManagerTest : public Test {
public:
    /// SetUp before each test.
    void SetUp();

    /// TearDown after each test.
    void TearDown();

    /// Helper function to retrieve current Volume
    int8_t getCurrentVolume() {
        SpeakerInterface::SpeakerSettings settings;
        if (m_speaker->getSpeakerSettings(&settings)) {
            return settings.volume;
        }
        return AVS_SET_VOLUME_MIN;
    }

protected:
    std::shared_ptr<MockSpeakerInterface> m_speaker;
    std::shared_ptr<ChannelVolumeManager> unit;
};

void ChannelVolumeManagerTest::SetUp() {
    m_speaker = std::make_shared<NiceMock<MockSpeakerInterface>>();
    m_speaker->DelegateToReal();
    m_speaker->setVolume(INITIAL_SETTINGS.volume);
    m_speaker->setMute(INITIAL_SETTINGS.mute);
    unit = ChannelVolumeManager::create(m_speaker);
}

void ChannelVolumeManagerTest::TearDown() {
    unit.reset();
    m_speaker.reset();
}

static int8_t defaultVolumeCurve(int8_t currentVolume) {
    const float lowerBreakPointFraction = 0.20f, upperBreakPointFraction = 0.40f;
    const int8_t lowerBreakPoint = static_cast<int8_t>(AVS_SET_VOLUME_MAX * lowerBreakPointFraction);
    const int8_t upperBreakPoint = static_cast<int8_t>(AVS_SET_VOLUME_MAX * upperBreakPointFraction);

    if (currentVolume >= upperBreakPoint)
        return lowerBreakPoint;
    else if (currentVolume >= lowerBreakPoint && currentVolume <= upperBreakPoint)
        return (currentVolume - lowerBreakPoint);
    else
        return avsCommon::avs::speakerConstants::AVS_SET_VOLUME_MIN;
}

TEST_F(ChannelVolumeManagerTest, test_createTest) {
    ASSERT_EQ(nullptr, ChannelVolumeManager::create(nullptr));

    // default created ChannelVolumeInterface::Type must be AVS_SPEAKER_VOLUME
    auto instance = ChannelVolumeManager::create(m_speaker);
    ASSERT_EQ(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, instance->getSpeakerType());
}

TEST_F(ChannelVolumeManagerTest, test_startDuckingCallAttenuatesVolume) {
    auto currentVolume = getCurrentVolume();
    auto desiredAttenuatedVolume = defaultVolumeCurve(currentVolume);
    EXPECT_CALL(*m_speaker, setVolume(desiredAttenuatedVolume)).Times(1);

    // invoke ducking
    ASSERT_TRUE(unit->startDucking());

    // confirm current volume matches desiredAttenuatedVolume
    ASSERT_EQ(desiredAttenuatedVolume, getCurrentVolume());
}

TEST_F(ChannelVolumeManagerTest, test_stopDuckingCallRestoresVolume) {
    auto currentVolume = getCurrentVolume();

    // invoke ducking
    ASSERT_TRUE(unit->startDucking());

    // expect restoration of channel volume.
    EXPECT_CALL(*m_speaker, setVolume(currentVolume)).Times(1);

    // stop ducking
    ASSERT_TRUE(unit->stopDucking());
}

TEST_F(ChannelVolumeManagerTest, test_getSpeakerSettingsReturnsUnduckedVolume) {
    auto currentVolume = getCurrentVolume();
    auto desiredAttenuatedVolume = defaultVolumeCurve(currentVolume);
    EXPECT_CALL(*m_speaker, setVolume(desiredAttenuatedVolume)).Times(1);
    // start ducking
    ASSERT_TRUE(unit->startDucking());

    // call getSpeakerSettings
    SpeakerInterface::SpeakerSettings settings;
    ASSERT_EQ(true, unit->getSpeakerSettings(&settings));
    ASSERT_EQ(settings.volume, INITIAL_SETTINGS.volume);
}

TEST_F(ChannelVolumeManagerTest, test_volumeIsRestoredToLatestUnduckedVolume) {
    auto currentVolume = getCurrentVolume();
    auto desiredAttenuatedVolume = defaultVolumeCurve(currentVolume);
    EXPECT_CALL(*m_speaker, setVolume(desiredAttenuatedVolume)).Times(1);
    // start ducking
    ASSERT_TRUE(unit->startDucking());

    // set unducked volume
    auto newUnduckedVolume = static_cast<int8_t>(INITIAL_SETTINGS.volume * 2);
    ASSERT_EQ(true, unit->setUnduckedVolume(newUnduckedVolume));

    // when stopDucking is called : volume should be restored to unducked volume
    EXPECT_CALL(*m_speaker, setVolume(newUnduckedVolume)).Times(1);
    ASSERT_TRUE(unit->stopDucking());
    ASSERT_EQ(newUnduckedVolume, getCurrentVolume());
}

TEST_F(ChannelVolumeManagerTest, test_startDuckingWhenAlreadyDucked) {
    auto currentVolume = getCurrentVolume();
    auto desiredAttenuatedVolume = defaultVolumeCurve(currentVolume);
    EXPECT_CALL(*m_speaker, setVolume(desiredAttenuatedVolume)).Times(1);
    // start ducking
    ASSERT_TRUE(unit->startDucking());

    // another startDucking call must not set volume again and just return true early
    EXPECT_CALL(*m_speaker, setVolume(_)).Times(0);
    ASSERT_TRUE(unit->startDucking());
}

TEST_F(ChannelVolumeManagerTest, test_stopDuckingWhenAlreadyUnducked) {
    auto currentVolume = getCurrentVolume();
    auto desiredAttenuatedVolume = defaultVolumeCurve(currentVolume);
    EXPECT_CALL(*m_speaker, setVolume(desiredAttenuatedVolume)).Times(1);
    // start ducking
    ASSERT_TRUE(unit->startDucking());

    // when stopDucking is called : volume should be restored to unducked volume
    EXPECT_CALL(*m_speaker, setVolume(currentVolume)).Times(1);
    ASSERT_TRUE(unit->stopDucking());

    // another stopDucking call must not set volume again and just return true early
    EXPECT_CALL(*m_speaker, setVolume(_)).Times(0);
    ASSERT_TRUE(unit->stopDucking());
}
}  // namespace test
}  // namespace speakerManager
}  // namespace alexaClientSDK
