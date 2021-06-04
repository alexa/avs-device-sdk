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

#include <future>
#include <memory>
#include <random>
#include <sstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <acsdkApplicationAudioPipelineFactoryInterfaces/MockApplicationAudioPipelineFactory.h>
#include <acsdkShutdownManagerInterfaces/MockShutdownNotifier.h>
#include <AVSCommon/AVS/MixingBehavior.h>
#include <AVSCommon/SDKInterfaces/MockFocusManager.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/MediaPlayer/MockMediaPlayer.h>

#include <acsdkNotifications/NotificationRenderer.h>

namespace alexaClientSDK {
namespace acsdkNotifications {
namespace test {

using namespace ::testing;
using namespace acsdkApplicationAudioPipelineFactoryInterfaces;
using namespace acsdkApplicationAudioPipelineFactoryInterfaces::test;
using namespace acsdkShutdownManagerInterfaces::test;
using namespace alexaClientSDK::avsCommon::sdkInterfaces::test;
using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::utils;
using namespace avsCommon::utils::mediaPlayer;
using namespace avsCommon::utils::mediaPlayer::test;

/// Timeout value to use when no wait is desired (e.g. to check the status of a
/// signal)
static const std::chrono::milliseconds ZERO_TIMEOUT{0};

/// (short) Timeout value to use when reaching the timeout is expected.
static const std::chrono::milliseconds EXPECTED_TIMEOUT{100};

/// (long) Timeout value to use when reaching the timeout is NOT expected.
static const std::chrono::milliseconds UNEXPECTED_TIMEOUT{5000};

/**
 * Factory of non-null istreams.
 *
 * @return a non-null istream.
 */
static std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()> goodStreamFunction =
    [] {
        return std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>(
            std::unique_ptr<std::istream>(new std::stringstream), avsCommon::utils::MediaType::MPEG);
    };

/**
 * A NotificationRendererObserver to be observed by tests.
 */
class MockNotificationRendererObserver : public acsdkNotificationsInterfaces::NotificationRendererObserverInterface {
public:
    MockNotificationRendererObserver();

    MOCK_METHOD0(onNotificationRenderingFinished, void());

    /**
     * Wait for the @c onNotificationRenderingFinished() callback.
     * @param timeout The maximum time to wait for the notification.
     * @return Whether or not the notification was received.
     */
    bool waitForFinished(const std::chrono::milliseconds timeout = UNEXPECTED_TIMEOUT);

private:
    /// Future used to wait for the @c onNotificationRenderingFinished() callback.
    std::future<void> m_future;

    /// Promise used to wake the @c waitForFinished().
    std::promise<void> m_promise;
};

MockNotificationRendererObserver::MockNotificationRendererObserver() {
    m_future = m_promise.get_future();
    ON_CALL(*this, onNotificationRenderingFinished()).WillByDefault(InvokeWithoutArgs([this]() {
        m_promise.set_value();
    }));
}

bool MockNotificationRendererObserver::waitForFinished(const std::chrono::milliseconds timeout) {
    return m_future.wait_for(timeout) != std::future_status::timeout;
}

/**
 * Future, Promise pair used to conveniently block and resume execution.
 */
struct FuturePromisePair {
public:
    FuturePromisePair();

    /// The promise to fulfill
    std::promise<void> promise;

    /// The future that will be awoken when the promise is fulfilled.
    std::future<void> future;
};

FuturePromisePair::FuturePromisePair() : promise{}, future{promise.get_future()} {
}

/**
 * Test rig.
 */
class NotificationRendererTest : public ::testing::Test {
public:
    void SetUp() override;

    void TearDown() override;

    /// Mock audio pipeline factory with which to exercise NotificationRenderer.
    std::shared_ptr<MockApplicationAudioPipelineFactory> m_audioPipelineFactory;

    /// Mock player with which to exercise NotificationRenderer.
    std::shared_ptr<MockMediaPlayer> m_player;

    /// Mock Focus Manager with which to exercise NotificationRenderer.
    std::shared_ptr<MockFocusManager> m_focusManager;

    /// Mock ShutdownNotifier with which to exercise NotificationRenderer.
    std::shared_ptr<MockShutdownNotifier> m_shutdownNotifier;

    /// The NotificationRenderer instance to exercise.
    std::shared_ptr<NotificationRenderer> m_renderer;

    /// Mock observer with which to monitor callbacks from @c m_renderer.
    std::shared_ptr<MockNotificationRendererObserver> m_observer;
};

void NotificationRendererTest::SetUp() {
    avsCommon::utils::logger::getConsoleLogger()->setLevel(avsCommon::utils::logger::Level::DEBUG9);

    m_audioPipelineFactory = std::make_shared<MockApplicationAudioPipelineFactory>();
    m_player = MockMediaPlayer::create();
    m_focusManager = std::make_shared<avsCommon::sdkInterfaces::test::MockFocusManager>();
    m_shutdownNotifier = std::make_shared<StrictMock<MockShutdownNotifier>>();

    m_renderer = NotificationRenderer::create(m_player, m_focusManager);
    ASSERT_TRUE(m_renderer);
    m_observer.reset(new MockNotificationRendererObserver());
    m_renderer->addObserver(m_observer);

    ON_CALL(*(m_focusManager.get()), acquireChannel(_, _)).WillByDefault(Return(true));
    ON_CALL(*m_focusManager, releaseChannel(_, _)).WillByDefault(InvokeWithoutArgs([] {
        auto releaseChannelSuccess = std::make_shared<std::promise<bool>>();
        std::future<bool> returnValue = releaseChannelSuccess->get_future();
        releaseChannelSuccess->set_value(true);
        return returnValue;
    }));
}

void NotificationRendererTest::TearDown() {
    m_player->shutdown();
}

/**
 * Test that create fails with a null MediaPlayer
 */
TEST_F(NotificationRendererTest, test_createWithNullMediaPlayer) {
    std::shared_ptr<MediaPlayerInterface> mediaPlayer = nullptr;
    auto renderer = NotificationRenderer::create(mediaPlayer, m_focusManager);
    ASSERT_FALSE(renderer);
}

/**
 * Test that create fails with a null AudioPipelineFactory.
 */
TEST_F(NotificationRendererTest, test_createWithNullApplicationAudioPipelineFactory) {
    std::shared_ptr<MockApplicationAudioPipelineFactory> factory = nullptr;
    auto renderer = NotificationRenderer::createNotificationRendererInterface(
        factory,
        acsdkManufactory::
            Annotated<avsCommon::sdkInterfaces::AudioFocusAnnotation, avsCommon::sdkInterfaces::FocusManagerInterface>(
                m_focusManager),
        m_shutdownNotifier);
    ASSERT_FALSE(renderer);
}

/**
 * Test that create fails with a null shutdown notifier.
 */
TEST_F(NotificationRendererTest, test_createWithNullShutdownNotifier) {
    auto renderer = NotificationRenderer::createNotificationRendererInterface(
        m_audioPipelineFactory,
        acsdkManufactory::
            Annotated<avsCommon::sdkInterfaces::AudioFocusAnnotation, avsCommon::sdkInterfaces::FocusManagerInterface>(
                m_focusManager),
        nullptr);
    ASSERT_FALSE(renderer);
}

/**
 * Test that create fails with a null FocusManager
 */
TEST_F(NotificationRendererTest, test_createWithNullFocusManager) {
    std::shared_ptr<acsdkNotificationsInterfaces::NotificationRendererInterface> renderer =
        NotificationRenderer::create(m_player, nullptr);
    ASSERT_FALSE(renderer);

    renderer =
        NotificationRenderer::createNotificationRendererInterface(m_audioPipelineFactory, nullptr, m_shutdownNotifier);
    ASSERT_FALSE(renderer);
}

/**
 * Test that renderer creates a correctly-configured notifications application media interfaces with the
 * audio pipeline factory.
 */
TEST_F(NotificationRendererTest, test_createNotificationsMediaPlayerWithAudioPipelineFactory) {
    bool equalizerAvailable = false;
    bool enableLiveMode = false;
    bool isCaptionable = false;
    avsCommon::sdkInterfaces::ChannelVolumeInterface::Type channelVolumeType =
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME;

    EXPECT_CALL(*(m_shutdownNotifier.get()), addObserver(_));
    EXPECT_CALL(
        *(m_audioPipelineFactory.get()),
        createApplicationMediaInterfaces(
            NOTIFICATIONS_MEDIA_PLAYER_NAME, equalizerAvailable, enableLiveMode, isCaptionable, channelVolumeType, _))
        .Times(1)
        .WillOnce(Return(std::make_shared<avsCommon::sdkInterfaces::ApplicationMediaInterfaces>(
            m_player, nullptr, nullptr, nullptr)));
    auto renderer = NotificationRenderer::createNotificationRendererInterface(
        m_audioPipelineFactory,
        acsdkManufactory::
            Annotated<avsCommon::sdkInterfaces::AudioFocusAnnotation, avsCommon::sdkInterfaces::FocusManagerInterface>(
                m_focusManager),
        m_shutdownNotifier);
    ASSERT_TRUE(renderer);
}

/**
 * Exercise rendering the preferred stream.  Verify that the MediaPlayer's
 * setSource() and play() methods get called (once each) and that the
 * NotificationRenderer's observer gets called back to indicate that playback
 * had completed.
 */
TEST_F(NotificationRendererTest, test_playPreferredStream) {
    EXPECT_CALL(*(m_player.get()), urlSetSource(_)).Times(1);
    EXPECT_CALL(*(m_player.get()), streamSetSource(_, _)).Times(0);
    EXPECT_CALL(*(m_player.get()), play(_)).Times(1);
    EXPECT_CALL(*(m_observer.get()), onNotificationRenderingFinished()).Times(1);

    m_renderer->renderNotification(goodStreamFunction, "");
    m_renderer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::UNDEFINED);
    m_player->waitUntilPlaybackStarted();
    m_player->mockFinished(m_player->getCurrentSourceId());
    ASSERT_TRUE(m_observer->waitForFinished());
}

/**
 * Exercise rendering the default stream.  Verify that the MediaPlayer's
 * setSource() and play() methods get called (once each) and that the
 * NotificationRenderer's observer gets called back to indicate that playback
 * had completed.
 */
TEST_F(NotificationRendererTest, testTimer_playDefaultStream) {
    EXPECT_CALL(*(m_player.get()), urlSetSource(_)).Times(1);
    EXPECT_CALL(*(m_player.get()), streamSetSource(_, _)).Times(1);
    EXPECT_CALL(*(m_player.get()), play(_)).Times(2);
    EXPECT_CALL(*(m_observer.get()), onNotificationRenderingFinished()).Times(1);

    m_renderer->renderNotification(goodStreamFunction, "");
    m_renderer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::UNDEFINED);

    m_player->waitUntilPlaybackStarted();
    m_player->mockError(m_player->getCurrentSourceId());
    m_player->waitUntilPlaybackError();

    ASSERT_FALSE(m_observer->waitForFinished(ZERO_TIMEOUT));

    m_player->waitUntilNextSetSource();
    m_player->mockFinished(m_player->getCurrentSourceId());
    ASSERT_TRUE(m_observer->waitForFinished());
}

/**
 * Exercise making a second @c renderNotification() call while a previous
 * one is still outstanding.  Verify that it is rejected.
 */
TEST_F(NotificationRendererTest, test_secondPlayRejected) {
    EXPECT_CALL(*(m_player.get()), urlSetSource(_)).Times(1);
    EXPECT_CALL(*(m_player.get()), streamSetSource(_, _)).Times(0);
    EXPECT_CALL(*(m_player.get()), play(_)).Times(1);
    EXPECT_CALL(*(m_observer.get()), onNotificationRenderingFinished()).Times(1);

    ASSERT_TRUE(m_renderer->renderNotification(goodStreamFunction, ""));
    m_renderer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::UNDEFINED);
    m_player->waitUntilPlaybackStarted();
    ASSERT_FALSE(m_renderer->renderNotification(goodStreamFunction, ""));
    m_player->mockFinished(m_player->getCurrentSourceId());
    ASSERT_TRUE(m_observer->waitForFinished());
}

/**
 * Exercise rendering the default stream.  Verify that a call to @c
 * renderNotification() while the default stream is playing is rejected.
 */
TEST_F(NotificationRendererTest, testTimer_secondPlayWhilePlayingDefaultStream) {
    EXPECT_CALL(*(m_player.get()), urlSetSource(_)).Times(1);
    EXPECT_CALL(*(m_player.get()), streamSetSource(_, _)).Times(1);
    EXPECT_CALL(*(m_player.get()), play(_)).Times(2);
    EXPECT_CALL(*(m_observer.get()), onNotificationRenderingFinished()).Times(1);

    ASSERT_TRUE(m_renderer->renderNotification(goodStreamFunction, ""));
    m_renderer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::UNDEFINED);
    m_player->waitUntilPlaybackStarted();
    m_player->mockError(m_player->getCurrentSourceId());
    m_player->waitUntilPlaybackError();

    ASSERT_FALSE(m_observer->waitForFinished(ZERO_TIMEOUT));

    m_player->waitUntilNextSetSource();
    m_player->waitUntilPlaybackStarted();

    ASSERT_FALSE(m_renderer->renderNotification(goodStreamFunction, ""));
    m_player->mockFinished(m_player->getCurrentSourceId());

    ASSERT_TRUE(m_observer->waitForFinished());
}

/**
 * Exercise cancelNotificationRendering().  Verify that it causes rendering to
 * complete.
 */
TEST_F(NotificationRendererTest, test_cancelNotificationRendering) {
    EXPECT_CALL(*(m_player.get()), urlSetSource(_)).Times(1);
    EXPECT_CALL(*(m_player.get()), streamSetSource(_, _)).Times(0);
    EXPECT_CALL(*(m_player.get()), play(_)).Times(1);
    EXPECT_CALL(*(m_player.get()), stop(_)).Times(1);
    EXPECT_CALL(*(m_observer.get()), onNotificationRenderingFinished()).Times(1);

    ASSERT_TRUE(m_renderer->renderNotification(goodStreamFunction, ""));
    m_renderer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::UNDEFINED);
    m_player->waitUntilPlaybackStarted();
    ASSERT_TRUE(m_renderer->cancelNotificationRendering());
    ASSERT_TRUE(m_observer->waitForFinished());
}

/**
 * Verify that calling renderNotification() while notification of rendering
 * completion is underway (but not from the callback itself) is accepted.
 * This verifies the use case where onNotificationRenderingFinished() is
 * used as a trigger to render the next notification.
 */
TEST_F(NotificationRendererTest, test_renderNotificationWhileNotifying) {
    FuturePromisePair signal;
    EXPECT_CALL(*(m_player.get()), urlSetSource(_)).Times(2);
    EXPECT_CALL(*(m_player.get()), streamSetSource(_, _)).Times(0);
    EXPECT_CALL(*(m_player.get()), play(_)).Times(2);
    EXPECT_CALL(*(m_observer.get()), onNotificationRenderingFinished()).WillOnce(InvokeWithoutArgs([&signal] {
        static int counter = 0;
        ASSERT_LT(counter, 2);
        if (0 == counter++) {
            signal.promise.set_value();
            // Yes, it is weak to use a sleep like this to order things, but if
            // the sleep is not long enough, this unit test will succeed, so it
            // won't create false test failures.
            std::this_thread::sleep_for(EXPECTED_TIMEOUT);
        }
    }));

    ASSERT_TRUE(m_renderer->renderNotification(goodStreamFunction, ""));
    m_renderer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::UNDEFINED);
    m_player->waitUntilPlaybackStarted();
    m_player->mockFinished(m_player->getCurrentSourceId());

    ASSERT_EQ(signal.future.wait_for(UNEXPECTED_TIMEOUT), std::future_status::ready);

    m_renderer->onFocusChanged(FocusState::NONE, MixingBehavior::UNDEFINED);
    ASSERT_TRUE(m_renderer->renderNotification(goodStreamFunction, ""));
    m_renderer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::UNDEFINED);

    m_player->waitUntilPlaybackStarted();
    m_player->waitUntilPlaybackFinished();
}

/**
 * Test that notification renders properly when acquireChannel succeeds and that
 * the audio channel is successfully released
 */
TEST_F(NotificationRendererTest, test_renderWhenAcquireChannelsSucceeds) {
    EXPECT_CALL(*(m_player.get()), urlSetSource(_)).Times(1);
    EXPECT_CALL(*(m_player.get()), streamSetSource(_, _)).Times(0);
    EXPECT_CALL(*(m_player.get()), play(_)).Times(1);
    EXPECT_CALL(*(m_observer.get()), onNotificationRenderingFinished()).Times(1);
    EXPECT_CALL(*(m_focusManager.get()), acquireChannel(_, _)).Times(1);
    EXPECT_CALL(*(m_focusManager.get()), releaseChannel(_, _)).Times(1);

    m_renderer->renderNotification(goodStreamFunction, "");
    m_renderer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::UNDEFINED);
    m_player->waitUntilPlaybackStarted();
    m_player->mockFinished(m_player->getCurrentSourceId());
    ASSERT_TRUE(m_observer->waitForFinished());
}

/**
 * Test that notification does not render when acquireChannel fails
 */
TEST_F(NotificationRendererTest, test_renderWhenAcquireChannelsFails) {
    EXPECT_CALL(*(m_player.get()), urlSetSource(_)).Times(1);
    EXPECT_CALL(*(m_player.get()), streamSetSource(_, _)).Times(0);
    EXPECT_CALL(*(m_player.get()), play(_)).Times(0);
    EXPECT_CALL(*(m_observer.get()), onNotificationRenderingFinished()).Times(0);
    EXPECT_CALL(*(m_focusManager.get()), acquireChannel(_, _)).Times(1).WillRepeatedly(Return(false));

    ASSERT_FALSE(m_renderer->renderNotification(goodStreamFunction, ""));
}

/**
 * Test that shutdown does the correct cleanup.
 */
TEST_F(NotificationRendererTest, testShutdown) {
    m_renderer->shutdown();
    m_renderer.reset();
    ASSERT_TRUE(m_player->getObservers().empty());
}

}  // namespace test
}  // namespace acsdkNotifications
}  // namespace alexaClientSDK
