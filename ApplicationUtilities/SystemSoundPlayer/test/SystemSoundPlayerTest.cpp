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

#include <AVSCommon/SDKInterfaces/Audio/MockSystemSoundAudioFactory.h>
#include <AVSCommon/Utils/MediaPlayer/MockMediaPlayer.h>

#include "SystemSoundPlayer/SystemSoundPlayer.h"

namespace alexaClientSDK {
namespace applicationUtilities {
namespace systemSoundPlayer {
namespace test {

using namespace avsCommon::sdkInterfaces::audio::test;
using namespace avsCommon::utils::mediaPlayer::test;
using namespace ::testing;

class SystemSoundPlayerTest : public ::testing::Test {
public:
    void SetUp() override;
    void TearDown() override;

    /// @c SystemSoundPlayer to test
    std::shared_ptr<SystemSoundPlayer> m_systemSoundPlayer;

    /// Player to send the audio to.
    std::shared_ptr<MockMediaPlayer> m_mockMediaPlayer;

    /// Factory to generate the system sound audio streams.
    std::shared_ptr<MockSystemSoundAudioFactory> m_mockSystemSoundAudioFactory;
};

void SystemSoundPlayerTest::SetUp() {
    m_mockSystemSoundAudioFactory = MockSystemSoundAudioFactory::create();
    m_mockMediaPlayer = MockMediaPlayer::create();

    m_systemSoundPlayer = SystemSoundPlayer::create(m_mockMediaPlayer, m_mockSystemSoundAudioFactory);
}

void SystemSoundPlayerTest::TearDown() {
    m_mockMediaPlayer->shutdown();
}

/**
 * Test create() with nullptrs
 */
TEST_F(SystemSoundPlayerTest, test_createWithNullPointers) {
    auto testSystemSoundPlayer = SystemSoundPlayer::create(nullptr, m_mockSystemSoundAudioFactory);
    EXPECT_EQ(testSystemSoundPlayer, nullptr);

    testSystemSoundPlayer = SystemSoundPlayer::create(m_mockMediaPlayer, nullptr);
    EXPECT_EQ(testSystemSoundPlayer, nullptr);
}

/**
 * Test System Sound Player correctly plays wake word notification tone.
 */
TEST_F(SystemSoundPlayerTest, test_playWakeWord) {
    EXPECT_CALL(*m_mockSystemSoundAudioFactory, wakeWordNotificationTone()).Times(1);
    EXPECT_CALL(*(m_mockMediaPlayer.get()), streamSetSource(_, _)).Times(1);
    EXPECT_CALL(*(m_mockMediaPlayer.get()), play(_)).WillOnce(Return(true));
    std::shared_future<bool> playToneFuture =
        m_systemSoundPlayer->playTone(SystemSoundPlayer::Tone::WAKEWORD_NOTIFICATION);

    m_mockMediaPlayer->mockFinished(m_mockMediaPlayer->getCurrentSourceId());
    playToneFuture.wait();

    ASSERT_TRUE(playToneFuture.get());
}

/**
 * Test System Sound Player correctly plays end speech tone.
 */
TEST_F(SystemSoundPlayerTest, test_playEndSpeech) {
    EXPECT_CALL(*m_mockSystemSoundAudioFactory, endSpeechTone()).Times(1);
    EXPECT_CALL(*(m_mockMediaPlayer.get()), streamSetSource(_, _)).Times(1);
    EXPECT_CALL(*(m_mockMediaPlayer.get()), play(_)).WillOnce(Return(true));
    std::shared_future<bool> playToneFuture = m_systemSoundPlayer->playTone(SystemSoundPlayer::Tone::END_SPEECH);

    m_mockMediaPlayer->mockFinished(m_mockMediaPlayer->getCurrentSourceId());
    playToneFuture.wait();

    ASSERT_TRUE(playToneFuture.get());
}

/**
 * Test System Sound Player returns false on playback error.
 */
TEST_F(SystemSoundPlayerTest, test_failPlayback) {
    EXPECT_CALL(*m_mockSystemSoundAudioFactory, endSpeechTone()).Times(1);
    EXPECT_CALL(*(m_mockMediaPlayer.get()), streamSetSource(_, _)).Times(1);
    EXPECT_CALL(*(m_mockMediaPlayer.get()), play(_)).WillOnce(Return(true));
    std::shared_future<bool> playToneFuture = m_systemSoundPlayer->playTone(SystemSoundPlayer::Tone::END_SPEECH);

    m_mockMediaPlayer->mockError(m_mockMediaPlayer->getCurrentSourceId());
    playToneFuture.wait();

    ASSERT_FALSE(playToneFuture.get());
}

/**
 * Test System Sound Player blocks and return false when busy playing a Tone.
 */
TEST_F(SystemSoundPlayerTest, test_playBeforeFinish) {
    std::shared_future<bool> playToneFuture = m_systemSoundPlayer->playTone(SystemSoundPlayer::Tone::END_SPEECH);

    EXPECT_CALL(*m_mockSystemSoundAudioFactory, endSpeechTone()).Times(0);
    EXPECT_CALL(*(m_mockMediaPlayer.get()), streamSetSource(_, _)).Times(0);
    EXPECT_CALL(*(m_mockMediaPlayer.get()), play(_)).Times(0);

    std::shared_future<bool> playToneFutureFail = m_systemSoundPlayer->playTone(SystemSoundPlayer::Tone::END_SPEECH);
    playToneFutureFail.wait();

    ASSERT_FALSE(playToneFutureFail.get());

    m_mockMediaPlayer->mockFinished(m_mockMediaPlayer->getCurrentSourceId());
    playToneFuture.wait();

    ASSERT_TRUE(playToneFuture.get());
}

/**
 * Test System Sound Player unblocks after playTone finishes.
 */
TEST_F(SystemSoundPlayerTest, test_sequentialPlayback) {
    EXPECT_CALL(*m_mockSystemSoundAudioFactory, endSpeechTone()).Times(2);
    EXPECT_CALL(*(m_mockMediaPlayer.get()), streamSetSource(_, _)).Times(2);
    EXPECT_CALL(*(m_mockMediaPlayer.get()), play(_)).Times(2).WillRepeatedly(Return(true));

    std::shared_future<bool> playToneFuture = m_systemSoundPlayer->playTone(SystemSoundPlayer::Tone::END_SPEECH);
    m_mockMediaPlayer->mockFinished(m_mockMediaPlayer->getCurrentSourceId());
    playToneFuture.wait();

    ASSERT_TRUE(playToneFuture.get());

    playToneFuture = m_systemSoundPlayer->playTone(SystemSoundPlayer::Tone::END_SPEECH);
    m_mockMediaPlayer->mockFinished(m_mockMediaPlayer->getCurrentSourceId());
    playToneFuture.wait();

    ASSERT_TRUE(playToneFuture.get());
}

}  // namespace test
}  // namespace systemSoundPlayer
}  // namespace applicationUtilities
}  // namespace alexaClientSDK
