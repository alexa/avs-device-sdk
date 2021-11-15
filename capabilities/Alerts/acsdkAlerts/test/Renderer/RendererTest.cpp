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
#include <gmock/gmock.h>

#include <acsdkShutdownManagerInterfaces/MockShutdownNotifier.h>
#include <acsdkApplicationAudioPipelineFactoryInterfaces/MockApplicationAudioPipelineFactory.h>
#include <AVSCommon/Utils/MediaPlayer/MockMediaPlayer.h>
#include <AVSCommon/Utils/MediaPlayer/SourceConfig.h>
#include <AVSCommon/Utils/Network/MockInternetConnectionMonitor.h>
#include <Settings/DeviceSettingsManager.h>

#include "acsdkAlerts/Renderer/Renderer.h"

namespace alexaClientSDK {
namespace acsdkAlerts {
namespace renderer {
namespace test {

using namespace acsdkApplicationAudioPipelineFactoryInterfaces;
using namespace acsdkApplicationAudioPipelineFactoryInterfaces::test;
using namespace avsCommon::utils::mediaPlayer::test;
using namespace settings::types;
using namespace testing;
using MediaPlayerState = avsCommon::utils::mediaPlayer::MediaPlayerState;

/// Amount of time that the renderer observer should wait for a task to finish.
static const std::chrono::milliseconds TEST_TIMEOUT{100};

/// Default media player state to report for all playback events
static const MediaPlayerState DEFAULT_MEDIA_PLAYER_STATE = {std::chrono::milliseconds(0)};

/// Test source Id that exists for the tests
static const avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId TEST_SOURCE_ID_GOOD = 1234;

/// Test source Id that does not exist for the tests
static const avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId TEST_SOURCE_ID_BAD = 5678;

/// Test URLs for the renderer
static const std::string TEST_URL1 = "fake.url.one";
static const std::string TEST_URL2 = "fake.url.two";

/// Loop pause for the renderer.
static const std::chrono::milliseconds TEST_LOOP_PAUSE{100};

/// Loop count for the renderer.
static const int TEST_LOOP_COUNT = 2;

/// Loop background pause for the renderer.
static const auto TEST_BACKGROUND_LOOP_PAUSE = std::chrono::seconds(1);

/// Amount of time that the renderer observer should wait for a task to finish.
static const auto TEST_BACKGROUND_TIMEOUT = std::chrono::seconds(5);

static const std::string ALARM_NAME = "ALARM";

class MockRendererObserver : public RendererObserverInterface {
public:
    bool waitFor(RendererObserverInterface::State newState) {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_conditionVariable.wait_for(lock, TEST_TIMEOUT, [this, newState] { return m_state == newState; });
    }

    bool waitFor(RendererObserverInterface::State newState, std::chrono::milliseconds maxWait) {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_conditionVariable.wait_for(lock, maxWait, [this, newState] { return m_state == newState; });
    }

    void onRendererStateChange(RendererObserverInterface::State newState, const std::string& reason) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_state = newState;
        m_conditionVariable.notify_all();
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_conditionVariable;
    RendererObserverInterface::State m_state;
};

class TestMediaPlayer : public MockMediaPlayer {
public:
    TestMediaPlayer() {
        m_sourceIdRetVal = TEST_SOURCE_ID_GOOD;
        m_playRetVal = true;
        m_stopRetVal = true;
    }

    static std::shared_ptr<testing::NiceMock<TestMediaPlayer>> create() {
        return std::make_shared<testing::NiceMock<TestMediaPlayer>>();
    }

    bool play(SourceId id) override {
        return m_playRetVal;
    }

    bool stop(SourceId id) override {
        return m_stopRetVal;
    }

    SourceId setSource(
        const std::string& url,
        std::chrono::milliseconds offset = std::chrono::milliseconds::zero(),
        const avsCommon::utils::mediaPlayer::SourceConfig& config = avsCommon::utils::mediaPlayer::emptySourceConfig(),
        bool repeat = false,
        const avsCommon::utils::mediaPlayer::PlaybackContext& playbackContext =
            avsCommon::utils::mediaPlayer::PlaybackContext()) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sourceConfig = config;
        m_sourceChanged.notify_one();
        return m_sourceIdRetVal;
    }

    SourceId setSource(
        std::shared_ptr<std::istream> stream,
        bool repeat,
        const avsCommon::utils::mediaPlayer::SourceConfig& config,
        avsCommon::utils::MediaType format) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sourceConfig = config;
        m_sourceChanged.notify_one();
        return m_sourceIdRetVal;
    }

    SourceId setSource(
        std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader,
        const avsCommon::utils::AudioFormat* audioFormat,
        const avsCommon::utils::mediaPlayer::SourceConfig& config) override {
        return m_sourceIdRetVal;
    }

    void setSourceRetVal(SourceId sourceRetVal) {
        m_sourceIdRetVal = sourceRetVal;
    }

    void setPlayRetVal(bool playRetVal) {
        m_playRetVal = playRetVal;
    }

    void setStopRetVal(bool stopRetVal) {
        m_stopRetVal = stopRetVal;
    }

    /*
     * Wait for sourceConfig value to be set.
     */
    std::pair<bool, avsCommon::utils::mediaPlayer::SourceConfig> waitForSourceConfig(
        std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (m_sourceChanged.wait_for(lock, timeout) != std::cv_status::timeout) {
            return std::make_pair(true, m_sourceConfig);
        } else {
            return std::make_pair(false, avsCommon::utils::mediaPlayer::emptySourceConfig());
        }
    }

private:
    SourceId m_sourceIdRetVal;
    bool m_playRetVal;
    bool m_stopRetVal;

    /// A lock to guard against source changes.
    std::mutex m_mutex;

    /// A condition variable to wait for source changes.
    std::condition_variable m_sourceChanged;

    /// The latest source config.
    avsCommon::utils::mediaPlayer::SourceConfig m_sourceConfig;
};

class RendererTest : public ::testing::Test {
public:
    RendererTest();
    ~RendererTest();
    void SetUpTest();
    void TearDown() override;

protected:
    std::shared_ptr<MockRendererObserver> m_observer;
    std::shared_ptr<TestMediaPlayer> m_mediaPlayer;
    std::shared_ptr<Renderer> m_renderer;
    std::shared_ptr<MockApplicationAudioPipelineFactory> m_audioPipelineFactory;
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface> m_shutdownNotifier;
    std::shared_ptr<avsCommon::utils::network::test::MockInternetConnectionMonitor> m_mockConnectionMonitor;

    static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> audioFactoryFunc() {
        return std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>(
            std::unique_ptr<std::istream>(new std::stringstream()), avsCommon::utils::MediaType::MPEG);
    }
};

RendererTest::RendererTest() : m_observer{std::make_shared<MockRendererObserver>()} {
    m_audioPipelineFactory = std::make_shared<MockApplicationAudioPipelineFactory>();
    m_shutdownNotifier = std::make_shared<NiceMock<acsdkShutdownManagerInterfaces::test::MockShutdownNotifier>>();
    m_mockConnectionMonitor = std::make_shared<avsCommon::utils::network::test::MockInternetConnectionMonitor>();
    m_mediaPlayer = TestMediaPlayer::create();

    bool equalizerAvailable = false;
    bool enableLiveMode = false;
    bool isCaptionable = false;
    avsCommon::sdkInterfaces::ChannelVolumeInterface::Type channelVolumeType =
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME;

    EXPECT_CALL(
        *(m_audioPipelineFactory.get()),
        createApplicationMediaInterfaces(
            ALERTS_MEDIA_PLAYER_NAME, equalizerAvailable, enableLiveMode, isCaptionable, channelVolumeType, _))
        .Times(1)
        .WillOnce(Return(std::make_shared<avsCommon::sdkInterfaces::ApplicationMediaInterfaces>(
            m_mediaPlayer, nullptr, nullptr, nullptr)));

    m_renderer =
        Renderer::createAlertRenderer(m_audioPipelineFactory, nullptr, m_shutdownNotifier, m_mockConnectionMonitor);
}

RendererTest::~RendererTest() {
    m_mediaPlayer.reset();
}

void RendererTest::SetUpTest() {
    std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()> audioFactory =
        RendererTest::audioFactoryFunc;
    std::vector<std::string> urls = {TEST_URL1, TEST_URL2};
    m_renderer->start(m_observer, audioFactory, true, urls, TEST_LOOP_COUNT, TEST_LOOP_PAUSE);
}

void RendererTest::TearDown() {
    m_mediaPlayer->setSourceRetVal(TEST_SOURCE_ID_GOOD);
    m_mediaPlayer->setPlayRetVal(true);
    m_mediaPlayer->setStopRetVal(true);
}

/**
 * Test if the Renderer class creates an object appropriately and fails when it must
 */
TEST_F(RendererTest, test_createAlertRenderer) {
    /// m_renderer was created using create() in the constructor. Check if not null
    ASSERT_NE(m_renderer, nullptr);

    /// confirm we return a nullptr if a nullptr was passed in
    ASSERT_EQ(Renderer::createAlertRenderer(nullptr, nullptr, nullptr, nullptr), nullptr);
}

/**
 * Test if the Renderer class creates an object appropriately and fails when it must
 */
TEST_F(RendererTest, test_create) {
    ASSERT_NE(Renderer::create(m_mediaPlayer, nullptr, nullptr), nullptr);

    /// confirm we return a nullptr if a nullptr was passed in
    ASSERT_EQ(Renderer::create(nullptr, nullptr, nullptr), nullptr);
}

/**
 * Test if the Renderer starts
 */
TEST_F(RendererTest, test_start) {
    SetUpTest();

    ASSERT_TRUE(m_observer->waitFor(RendererObserverInterface::State::UNSET));

    m_mediaPlayer->shutdown();
}

/**
 * Test if the Renderer stops
 */
TEST_F(RendererTest, test_stop) {
    SetUpTest();

    m_renderer->stop();

    ASSERT_FALSE(m_observer->waitFor(RendererObserverInterface::State::ERROR));
}

/**
 * Test if the Renderer stops then restarts successfully
 */
TEST_F(RendererTest, test_restart) {
    std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()> audioFactory =
        RendererTest::audioFactoryFunc;
    std::vector<std::string> urls = {TEST_URL1, TEST_URL2};
    m_renderer->start(m_observer, audioFactory, true, urls, TEST_LOOP_COUNT, TEST_LOOP_PAUSE);

    m_renderer->onPlaybackStarted(TEST_SOURCE_ID_GOOD, DEFAULT_MEDIA_PLAYER_STATE);
    ASSERT_TRUE(m_observer->waitFor(RendererObserverInterface::State::STARTED));

    m_renderer->stop();
    ASSERT_FALSE(m_observer->waitFor(RendererObserverInterface::State::ERROR));
    m_renderer->onPlaybackStopped(TEST_SOURCE_ID_GOOD, DEFAULT_MEDIA_PLAYER_STATE);
    ASSERT_TRUE(m_observer->waitFor(RendererObserverInterface::State::STOPPED));

    m_renderer->start(m_observer, audioFactory, true, urls, TEST_LOOP_COUNT, TEST_LOOP_PAUSE);
    m_renderer->onPlaybackStarted(TEST_SOURCE_ID_GOOD, DEFAULT_MEDIA_PLAYER_STATE);
    ASSERT_TRUE(m_observer->waitFor(RendererObserverInterface::State::STARTED));
}

/**
 * Test if the Renderer errors out when it cant stop
 */
TEST_F(RendererTest, test_stopError) {
    SetUpTest();
    m_renderer->onPlaybackStarted(TEST_SOURCE_ID_GOOD, DEFAULT_MEDIA_PLAYER_STATE);
    ASSERT_TRUE(m_observer->waitFor(RendererObserverInterface::State::STARTED));

    m_mediaPlayer->setStopRetVal(false);

    const avsCommon::utils::mediaPlayer::ErrorType& errorType =
        avsCommon::utils::mediaPlayer::ErrorType::MEDIA_ERROR_INVALID_REQUEST;
    std::string errorMsg = "testError";

    m_renderer->stop();
    /// if stop fails, we should receive a PlaybackError from mediaplayer
    m_renderer->onPlaybackError(TEST_SOURCE_ID_GOOD, errorType, errorMsg, DEFAULT_MEDIA_PLAYER_STATE);

    ASSERT_TRUE(m_observer->waitFor(RendererObserverInterface::State::ERROR));
}

/**
 * Test if the Renderer correctly handles Playback starting
 */
TEST_F(RendererTest, test_onPlaybackStarted) {
    SetUpTest();

    /// shouldn't start if the source is bad
    m_renderer->onPlaybackStarted(TEST_SOURCE_ID_BAD, DEFAULT_MEDIA_PLAYER_STATE);
    ASSERT_FALSE(m_observer->waitFor(RendererObserverInterface::State::STARTED));

    /// should start if the source is good
    m_renderer->onPlaybackStarted(TEST_SOURCE_ID_GOOD, DEFAULT_MEDIA_PLAYER_STATE);
    ASSERT_TRUE(m_observer->waitFor(RendererObserverInterface::State::STARTED));
}

/**
 * Test if the Renderer correctly handles Playback stopping
 */
TEST_F(RendererTest, test_onPlaybackStopped) {
    SetUpTest();

    /// shouldn't stop if the source is bad
    m_renderer->onPlaybackStopped(TEST_SOURCE_ID_BAD, DEFAULT_MEDIA_PLAYER_STATE);
    ASSERT_FALSE(m_observer->waitFor(RendererObserverInterface::State::STOPPED));

    /// should stop if the source is good
    m_renderer->onPlaybackStopped(TEST_SOURCE_ID_GOOD, DEFAULT_MEDIA_PLAYER_STATE);
    ASSERT_TRUE(m_observer->waitFor(RendererObserverInterface::State::STOPPED));
}

/**
 * Test if the Renderer gracefully handles errors when Playback finishing
 */
TEST_F(RendererTest, test_onPlaybackFinishedError) {
    SetUpTest();

    /// shouldn't finish even if the source is good, if the media player is errored out
    m_mediaPlayer->setSourceRetVal(avsCommon::utils::mediaPlayer::MediaPlayerInterface::ERROR);
    m_renderer->onPlaybackFinished(TEST_SOURCE_ID_GOOD, DEFAULT_MEDIA_PLAYER_STATE);
    ASSERT_FALSE(m_observer->waitFor(RendererObserverInterface::State::STOPPED));

    /// shouldn't finish even if the source is good, if the media player can't play it
    m_mediaPlayer->setSourceRetVal(TEST_SOURCE_ID_GOOD);
    m_mediaPlayer->setPlayRetVal(false);
    m_renderer->onPlaybackFinished(TEST_SOURCE_ID_GOOD, DEFAULT_MEDIA_PLAYER_STATE);
    ASSERT_FALSE(m_observer->waitFor(RendererObserverInterface::State::STOPPED));
}

/**
 * Test if the Renderer correctly handles Playback erroring out
 */
TEST_F(RendererTest, test_onPlaybackError) {
    const avsCommon::utils::mediaPlayer::ErrorType& errorType =
        avsCommon::utils::mediaPlayer::ErrorType::MEDIA_ERROR_INVALID_REQUEST;
    std::string errorMsg = "testError";

    SetUpTest();

    /// shouldn't respond with errors if the source is bad
    m_renderer->onPlaybackError(TEST_SOURCE_ID_BAD, errorType, errorMsg, DEFAULT_MEDIA_PLAYER_STATE);
    ASSERT_FALSE(m_observer->waitFor(RendererObserverInterface::State::ERROR));

    /// shouldn't respond with errors if the source is good
    m_renderer->onPlaybackError(TEST_SOURCE_ID_GOOD, errorType, errorMsg, DEFAULT_MEDIA_PLAYER_STATE);
    ASSERT_TRUE(m_observer->waitFor(RendererObserverInterface::State::ERROR));
}

/**
 * Test empty URL with non-zero loop pause, simulating playing a default alarm audio on background
 */
TEST_F(RendererTest, testTimer_emptyURLNonZeroLoopPause) {
    std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()> audioFactory =
        RendererTest::audioFactoryFunc;
    std::vector<std::string> urls;

    // pass empty URLS with 10s pause and no loop count
    // this simulates playing a default alarm audio on background
    // it is expected to renderer to play the alert sound continuously at loop pause intervals
    m_renderer->start(m_observer, audioFactory, true, urls, TEST_LOOP_COUNT, TEST_BACKGROUND_LOOP_PAUSE);

    // mediaplayer starts playing the alarm audio, in this case audio is of 0 length
    m_renderer->onPlaybackStarted(TEST_SOURCE_ID_GOOD, DEFAULT_MEDIA_PLAYER_STATE);

    // record the time audio starts playing
    auto now = std::chrono::high_resolution_clock::now();

    // expect the renderer state to change to 'STARTED'
    ASSERT_TRUE(m_observer->waitFor(RendererObserverInterface::State::STARTED, TEST_BACKGROUND_TIMEOUT));

    // mediaplayer finishes playing the alarm audio
    m_renderer->onPlaybackFinished(TEST_SOURCE_ID_GOOD, DEFAULT_MEDIA_PLAYER_STATE);

    // mediaplayer starts playing the alarm audio, in this case audio is of 0 length
    m_renderer->onPlaybackStarted(TEST_SOURCE_ID_GOOD, DEFAULT_MEDIA_PLAYER_STATE);

    // mediaplayer finishes playing the alarm audio
    m_renderer->onPlaybackFinished(TEST_SOURCE_ID_GOOD, DEFAULT_MEDIA_PLAYER_STATE);

    ASSERT_TRUE(m_observer->waitFor(RendererObserverInterface::State::STARTED, TEST_BACKGROUND_TIMEOUT));

    // expect the renderer state to change to 'STOPPED' after TEST_BACKGROUND_LOOP_PAUSE
    ASSERT_TRUE(m_observer->waitFor(RendererObserverInterface::State::COMPLETED, TEST_BACKGROUND_TIMEOUT));

    // get the elapsed time
    auto elapsed = std::chrono::high_resolution_clock::now() - now;

    // check the elapsed time is ~TEST_BACKGROUND_LOOP_PAUSE
    ASSERT_TRUE((elapsed >= TEST_BACKGROUND_LOOP_PAUSE) && (elapsed < TEST_BACKGROUND_TIMEOUT));
}

/**
 * Test alarmVolumeRampRendering.
 */
TEST_F(RendererTest, test_alarmVolumeRampRendering) {
    std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()> audioFactory =
        RendererTest::audioFactoryFunc;
    std::vector<std::string> urls;

    // Pause interval for this test.
    const auto loopPause = std::chrono::seconds(1);

    // Create a thread that will observe the FadeIn config that is set to the MediaPlayer;
    std::thread sourceConfigObserver([this, loopPause]() {
        avsCommon::utils::mediaPlayer::SourceConfig config;
        bool ok;

        // Check that the initial gain is 0.
        std::tie(ok, config) = m_mediaPlayer->waitForSourceConfig(6 * loopPause);
        ASSERT_TRUE(ok);
        ASSERT_EQ(config.fadeInConfig.startGain, 0);
        m_renderer->onPlaybackStarted(TEST_SOURCE_ID_GOOD, DEFAULT_MEDIA_PLAYER_STATE);
        m_renderer->onPlaybackFinished(TEST_SOURCE_ID_GOOD, DEFAULT_MEDIA_PLAYER_STATE);

        // Check that the gain increases at each repetition.
        std::tie(ok, config) = m_mediaPlayer->waitForSourceConfig(6 * loopPause);
        ASSERT_TRUE(ok);
        ASSERT_GT(config.fadeInConfig.startGain, 0);
        m_renderer->onPlaybackStarted(TEST_SOURCE_ID_GOOD, DEFAULT_MEDIA_PLAYER_STATE);
        m_renderer->onPlaybackFinished(TEST_SOURCE_ID_GOOD, DEFAULT_MEDIA_PLAYER_STATE);
    });

    // pass empty URLS with 1s pause
    // this simulates playing a default alarm audio on background
    // it is expected to renderer to play the alert sound continuously at loop pause intervals
    constexpr int testLoopCount = 2;
    m_renderer->start(m_observer, audioFactory, true, urls, testLoopCount, loopPause);

    sourceConfigObserver.join();

    ASSERT_TRUE(m_observer->waitFor(RendererObserverInterface::State::COMPLETED));
}

}  // namespace test
}  // namespace renderer
}  // namespace acsdkAlerts
}  // namespace alexaClientSDK
