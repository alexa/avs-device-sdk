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

#include <future>
#include <memory>
#include <random>
#include <sstream>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <AVSCommon/Utils/Logger/Logger.h>
#include "AVSCommon/Utils/MediaPlayer/MockMediaPlayer.h"

#include <Notifications/NotificationRenderer.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace notifications {
namespace test {

using namespace ::testing;
using namespace avsCommon;
using namespace avsCommon::utils;
using namespace avsCommon::utils::mediaPlayer;
using namespace avsCommon::utils::mediaPlayer::test;

/// Timeout value to use when no wait is desired (e.g. to check the status of a signal)
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
static std::function<std::unique_ptr<std::istream>()> goodStreamFunction = [] {
    return std::unique_ptr<std::istream>(new std::stringstream);
};

/**
 * A NotificationRendererObserver to be observed by tests.
 */
class MockNotificationRendererObserver : public NotificationRendererObserverInterface {
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

    /// Mock player with which to exercise NotificationRenderer.
    std::shared_ptr<MockMediaPlayer> m_player;

    /// The NotificationRenderer instance to exercise.
    std::shared_ptr<NotificationRenderer> m_renderer;

    /// Mock observer with which to monitor callbacks from @c m_renderer.
    std::shared_ptr<MockNotificationRendererObserver> m_observer;
};

void NotificationRendererTest::SetUp() {
    avsCommon::utils::logger::getConsoleLogger()->setLevel(avsCommon::utils::logger::Level::DEBUG9);

    m_player = MockMediaPlayer::create();
    m_renderer = NotificationRenderer::create(m_player);
    ASSERT_TRUE(m_renderer);
    m_observer.reset(new MockNotificationRendererObserver());
    m_renderer->addObserver(m_observer);
}

void NotificationRendererTest::TearDown() {
    m_player->shutdown();
}

/**
 * Test that create fails with a null MediaPlayer
 */
TEST_F(NotificationRendererTest, testCreateWithNullMediaPlayer) {
    auto renderer = NotificationRenderer::create(nullptr);
    ASSERT_FALSE(renderer);
}

/**
 * Exercise rendering the preferred stream.  Verify that the MediaPlayer's setSource() and play()
 * methods get called (once each) and that the NotificationRenderer's observer gets called
 * back to indicate that playback had completed.
 */
TEST_F(NotificationRendererTest, testPlayPreferredStream) {
    EXPECT_CALL(*(m_player.get()), urlSetSource(_)).Times(1);
    EXPECT_CALL(*(m_player.get()), streamSetSource(_, _)).Times(0);
    EXPECT_CALL(*(m_player.get()), play(_)).Times(1);
    EXPECT_CALL(*(m_observer.get()), onNotificationRenderingFinished()).Times(1);

    m_renderer->renderNotification(goodStreamFunction, "");
    m_player->waitUntilPlaybackStarted();
    m_player->mockFinished(m_player->getCurrentSourceId());
    ASSERT_TRUE(m_observer->waitForFinished());
}

/**
 * Exercise rendering the default stream.  Verify that the MediaPlayer's setSource() and play()
 * methods get called (once each) and that the NotificationRenderer's observer gets called
 * back to indicate that playback had completed.
 */
TEST_F(NotificationRendererTest, testPlayDefaultStream) {
    EXPECT_CALL(*(m_player.get()), urlSetSource(_)).Times(1);
    EXPECT_CALL(*(m_player.get()), streamSetSource(_, _)).Times(1);
    EXPECT_CALL(*(m_player.get()), play(_)).Times(2);
    EXPECT_CALL(*(m_observer.get()), onNotificationRenderingFinished()).Times(1);

    m_renderer->renderNotification(goodStreamFunction, "");

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
TEST_F(NotificationRendererTest, testSecondPlayRejected) {
    EXPECT_CALL(*(m_player.get()), urlSetSource(_)).Times(1);
    EXPECT_CALL(*(m_player.get()), streamSetSource(_, _)).Times(0);
    EXPECT_CALL(*(m_player.get()), play(_)).Times(1);
    EXPECT_CALL(*(m_observer.get()), onNotificationRenderingFinished()).Times(1);

    ASSERT_TRUE(m_renderer->renderNotification(goodStreamFunction, ""));
    m_player->waitUntilPlaybackStarted();
    ASSERT_FALSE(m_renderer->renderNotification(goodStreamFunction, ""));
    m_player->mockFinished(m_player->getCurrentSourceId());
    ASSERT_TRUE(m_observer->waitForFinished());
}

/**
 * Exercise rendering the default stream.  Verify that a call to @c renderNotification()
 * while the default stream is playing is rejected.
 */
TEST_F(NotificationRendererTest, testSecondPlayWhilePlayingDefaultStream) {
    EXPECT_CALL(*(m_player.get()), urlSetSource(_)).Times(1);
    EXPECT_CALL(*(m_player.get()), streamSetSource(_, _)).Times(1);
    EXPECT_CALL(*(m_player.get()), play(_)).Times(2);
    EXPECT_CALL(*(m_observer.get()), onNotificationRenderingFinished()).Times(1);

    ASSERT_TRUE(m_renderer->renderNotification(goodStreamFunction, ""));

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
 * Exercise cancelNotificationRendering().  Verify that it causes rendering to complete.
 */
TEST_F(NotificationRendererTest, testCancelNotificationRendering) {
    EXPECT_CALL(*(m_player.get()), urlSetSource(_)).Times(1);
    EXPECT_CALL(*(m_player.get()), streamSetSource(_, _)).Times(0);
    EXPECT_CALL(*(m_player.get()), play(_)).Times(1);
    EXPECT_CALL(*(m_player.get()), stop(_)).Times(1);
    EXPECT_CALL(*(m_observer.get()), onNotificationRenderingFinished()).Times(1);

    ASSERT_TRUE(m_renderer->renderNotification(goodStreamFunction, ""));
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
TEST_F(NotificationRendererTest, testRenderNotificationWhileNotifying) {
    FuturePromisePair signal;
    EXPECT_CALL(*(m_player.get()), urlSetSource(_)).Times(2);
    EXPECT_CALL(*(m_player.get()), streamSetSource(_, _)).Times(0);
    EXPECT_CALL(*(m_player.get()), play(_)).Times(2);
    EXPECT_CALL(*(m_observer.get()), onNotificationRenderingFinished()).WillOnce(InvokeWithoutArgs([&signal] {
        static int counter = 0;
        ASSERT_LT(counter, 2);
        if (0 == counter++) {
            signal.promise.set_value();
            // Yes, it is weak to use a sleep like this to order things, but if the sleep is not
            // long enough, this unit test will succeed, so it won't create false test failures.
            std::this_thread::sleep_for(EXPECTED_TIMEOUT);
        }
    }));

    ASSERT_TRUE(m_renderer->renderNotification(goodStreamFunction, ""));
    m_player->waitUntilPlaybackStarted();
    m_player->mockFinished(m_player->getCurrentSourceId());

    ASSERT_EQ(signal.future.wait_for(UNEXPECTED_TIMEOUT), std::future_status::ready);

    ASSERT_TRUE(m_renderer->renderNotification(goodStreamFunction, ""));

    m_player->waitUntilPlaybackStarted();
    m_player->waitUntilPlaybackFinished();
}

}  // namespace test
}  // namespace notifications
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
