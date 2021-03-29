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

#include <acsdkApplicationAudioPipelineFactoryInterfaces/MockApplicationAudioPipelineFactory.h>
#include <AVSCommon/SDKInterfaces/Audio/MockSystemSoundAudioFactory.h>
#include <AVSCommon/Utils/MediaPlayer/MockMediaPlayer.h>

#include "SystemSoundPlayer/SystemSoundPlayer.h"

namespace alexaClientSDK {
namespace applicationUtilities {
namespace systemSoundPlayer {
namespace test {

using namespace acsdkApplicationAudioPipelineFactoryInterfaces::test;
using namespace avsCommon::sdkInterfaces::audio;
using namespace avsCommon::sdkInterfaces::audio::test;
using namespace avsCommon::utils::mediaPlayer::test;
using namespace ::testing;

/// Stub class that implements AudioFactoryInterface.
class StubAudioFactory : public AudioFactoryInterface {
public:
    static std::shared_ptr<StubAudioFactory> createStubAudioFactory(
        std::shared_ptr<SystemSoundAudioFactoryInterface> systemSoundFactory);

    std::shared_ptr<avsCommon::sdkInterfaces::audio::AlertsAudioFactoryInterface> alerts() const override;
    std::shared_ptr<avsCommon::sdkInterfaces::audio::NotificationsAudioFactoryInterface> notifications() const override;
    std::shared_ptr<avsCommon::sdkInterfaces::audio::CommunicationsAudioFactoryInterface> communications()
        const override;
    std::shared_ptr<avsCommon::sdkInterfaces::audio::SystemSoundAudioFactoryInterface> systemSounds() const override;

private:
    StubAudioFactory(std::shared_ptr<SystemSoundAudioFactoryInterface> systemSoundFactory);

    std::shared_ptr<avsCommon::sdkInterfaces::audio::SystemSoundAudioFactoryInterface> m_systemSoundAudioFactory;
};

StubAudioFactory::StubAudioFactory(std::shared_ptr<SystemSoundAudioFactoryInterface> systemSoundFactory) :
        m_systemSoundAudioFactory{systemSoundFactory} {
}

std::shared_ptr<StubAudioFactory> StubAudioFactory::createStubAudioFactory(
    std::shared_ptr<SystemSoundAudioFactoryInterface> systemSoundFactory) {
    return std::shared_ptr<StubAudioFactory>(new StubAudioFactory(systemSoundFactory));
}

std::shared_ptr<avsCommon::sdkInterfaces::audio::AlertsAudioFactoryInterface> StubAudioFactory::alerts() const {
    return nullptr;
}

std::shared_ptr<avsCommon::sdkInterfaces::audio::NotificationsAudioFactoryInterface> StubAudioFactory::notifications()
    const {
    return nullptr;
}

std::shared_ptr<avsCommon::sdkInterfaces::audio::CommunicationsAudioFactoryInterface> StubAudioFactory::communications()
    const {
    return nullptr;
}

std::shared_ptr<avsCommon::sdkInterfaces::audio::SystemSoundAudioFactoryInterface> StubAudioFactory::systemSounds()
    const {
    return m_systemSoundAudioFactory;
}

/// SystemSoundPlayerTest unit tests.
class SystemSoundPlayerTest : public ::testing::Test {
public:
    void SetUp() override;
    void TearDown() override;

    /// @c SystemSoundPlayer to test
    std::shared_ptr<avsCommon::sdkInterfaces::SystemSoundPlayerInterface> m_systemSoundPlayer;

    /// Player to send the audio to.
    std::shared_ptr<MockMediaPlayer> m_mockMediaPlayer;

    /// Mock application audio pipeline factory.
    std::shared_ptr<MockApplicationAudioPipelineFactory> m_mockAudioPipelineFactory;

    /// Factory to provide the mock system sound audio factory.
    std::shared_ptr<StubAudioFactory> m_stubAudioFactory;

    /// Factory to generate the system sound audio streams.
    std::shared_ptr<MockSystemSoundAudioFactory> m_mockSystemSoundAudioFactory;
};

void SystemSoundPlayerTest::SetUp() {
    m_mockSystemSoundAudioFactory = MockSystemSoundAudioFactory::create();
    m_mockMediaPlayer = MockMediaPlayer::create();
    m_stubAudioFactory = StubAudioFactory::createStubAudioFactory(m_mockSystemSoundAudioFactory);
    m_mockAudioPipelineFactory = std::make_shared<StrictMock<MockApplicationAudioPipelineFactory>>();

    EXPECT_CALL(
        *(m_mockAudioPipelineFactory.get()),
        createApplicationMediaInterfaces(SYSTEM_SOUND_MEDIA_PLAYER_NAME, _, _, _, _, _))
        .WillRepeatedly(Return(std::make_shared<avsCommon::sdkInterfaces::ApplicationMediaInterfaces>(
            m_mockMediaPlayer, nullptr, nullptr, nullptr, nullptr)));

    m_systemSoundPlayer =
        SystemSoundPlayer::createSystemSoundPlayerInterface(m_mockAudioPipelineFactory, m_stubAudioFactory);
}

void SystemSoundPlayerTest::TearDown() {
    m_mockMediaPlayer->shutdown();
}

/**
 * Test createSystemSoundPlayerInterface() simple failure cases
 */
TEST_F(SystemSoundPlayerTest, test_createSystemSoundPlayerInterfaceFailureCases) {
    /// Expect failure with null audio pipeline factory.
    auto testSystemSoundPlayer = SystemSoundPlayer::createSystemSoundPlayerInterface(nullptr, m_stubAudioFactory);
    EXPECT_EQ(testSystemSoundPlayer, nullptr);

    /// Expect failure with null audio factory.
    testSystemSoundPlayer = SystemSoundPlayer::createSystemSoundPlayerInterface(m_mockAudioPipelineFactory, nullptr);
    EXPECT_EQ(testSystemSoundPlayer, nullptr);

    /// Expect failure when audio pipeline factory returns a nullptr for the application media interfaces.
    auto failedAudioPipelineFactory = std::make_shared<StrictMock<MockApplicationAudioPipelineFactory>>();
    EXPECT_CALL(
        *(failedAudioPipelineFactory.get()),
        createApplicationMediaInterfaces(SYSTEM_SOUND_MEDIA_PLAYER_NAME, _, _, _, _, _))
        .WillOnce(Return(std::shared_ptr<avsCommon::sdkInterfaces::ApplicationMediaInterfaces>(nullptr)));
    testSystemSoundPlayer =
        SystemSoundPlayer::createSystemSoundPlayerInterface(failedAudioPipelineFactory, m_stubAudioFactory);
    EXPECT_EQ(testSystemSoundPlayer, nullptr);

    /// Expect failure when application media interfaces contains a null media player.
    failedAudioPipelineFactory = std::make_shared<StrictMock<MockApplicationAudioPipelineFactory>>();
    EXPECT_CALL(
        *(failedAudioPipelineFactory.get()),
        createApplicationMediaInterfaces(SYSTEM_SOUND_MEDIA_PLAYER_NAME, _, _, _, _, _))
        .WillOnce(Return(std::make_shared<avsCommon::sdkInterfaces::ApplicationMediaInterfaces>(
            nullptr, nullptr, nullptr, nullptr, nullptr)));
    testSystemSoundPlayer =
        SystemSoundPlayer::createSystemSoundPlayerInterface(failedAudioPipelineFactory, m_stubAudioFactory);
    EXPECT_EQ(testSystemSoundPlayer, nullptr);

    /// Expect failure when audio factory returns a nullptr for the system sounds audio factory.
    auto failedAudioFactory = StubAudioFactory::createStubAudioFactory(nullptr);
    testSystemSoundPlayer =
        SystemSoundPlayer::createSystemSoundPlayerInterface(m_mockAudioPipelineFactory, failedAudioFactory);
    EXPECT_EQ(testSystemSoundPlayer, nullptr);
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
