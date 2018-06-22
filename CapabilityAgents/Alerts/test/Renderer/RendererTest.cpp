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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <AVSCommon/Utils/MediaPlayer/MockMediaPlayer.h>

#include "Alerts/Renderer/Renderer.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {
namespace renderer {
namespace test {

using namespace avsCommon::utils::mediaPlayer::test;

/// Amount of time that the renderer observer should wait for a task to finish.
static const std::chrono::milliseconds TEST_TIMEOUT{100};

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

class MockRendererObserver : public RendererObserverInterface {
public:
    bool waitFor(RendererObserverInterface::State newState) {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_conditionVariable.wait_for(lock, TEST_TIMEOUT, [this, newState] { return m_state == newState; });
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

    SourceId setSource(const std::string& url, std::chrono::milliseconds offset = std::chrono::milliseconds::zero())
        override {
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

private:
    SourceId m_sourceIdRetVal;
    bool m_playRetVal;
    bool m_stopRetVal;
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

    static std::unique_ptr<std::istream> audioFactoryFunc() {
        return std::unique_ptr<std::istream>(new std::stringstream());
    }
};

RendererTest::RendererTest() :
        m_observer{std::make_shared<MockRendererObserver>()},
        m_mediaPlayer{TestMediaPlayer::create()},
        m_renderer{Renderer::create(m_mediaPlayer)} {
    m_renderer->setObserver(m_observer);
}

RendererTest::~RendererTest() {
    m_mediaPlayer->setObserver(nullptr);
    m_mediaPlayer.reset();
}

void RendererTest::SetUpTest() {
    std::function<std::unique_ptr<std::istream>()> audioFactory = RendererTest::audioFactoryFunc;
    std::vector<std::string> urls = {TEST_URL1, TEST_URL2};
    m_renderer->setObserver(m_observer);
    m_renderer->start(audioFactory, urls, TEST_LOOP_COUNT, TEST_LOOP_PAUSE);
}

void RendererTest::TearDown() {
    m_mediaPlayer->setSourceRetVal(TEST_SOURCE_ID_GOOD);
    m_mediaPlayer->setPlayRetVal(true);
    m_mediaPlayer->setStopRetVal(true);
}

/**
 * Test if the Renderer class creates an object appropriately and fails when it must
 */
TEST_F(RendererTest, create) {
    /// m_renderer was created using create() in the constructor. Check if not null
    ASSERT_NE(m_renderer, nullptr);

    /// confirm we return a nullptr if a nullptr was passed in
    ASSERT_EQ(Renderer::create(nullptr), nullptr);
}

/**
 * Test if the Renderer starts
 */
TEST_F(RendererTest, start) {
    SetUpTest();

    ASSERT_TRUE(m_observer->waitFor(RendererObserverInterface::State::UNSET));
}

/**
 * Test if the Renderer stops
 */
TEST_F(RendererTest, stop) {
    SetUpTest();

    m_renderer->stop();

    ASSERT_FALSE(m_observer->waitFor(RendererObserverInterface::State::ERROR));
}

/**
 * Test if the Renderer errors out when it cant stop
 */
TEST_F(RendererTest, stopError) {
    SetUpTest();
    m_mediaPlayer->setStopRetVal(false);

    m_renderer->stop();

    ASSERT_TRUE(m_observer->waitFor(RendererObserverInterface::State::ERROR));
}

/**
 * Test if the Renderer correctly handles Playback starting
 */
TEST_F(RendererTest, onPlaybackStarted) {
    SetUpTest();

    /// shouldn't start if the source is bad
    m_renderer->onPlaybackStarted(TEST_SOURCE_ID_BAD);
    ASSERT_FALSE(m_observer->waitFor(RendererObserverInterface::State::STARTED));

    /// should start if the source is good
    m_renderer->onPlaybackStarted(TEST_SOURCE_ID_GOOD);
    ASSERT_TRUE(m_observer->waitFor(RendererObserverInterface::State::STARTED));
}

/**
 * Test if the Renderer correctly handles Playback stopping
 */
TEST_F(RendererTest, onPlaybackStopped) {
    SetUpTest();

    /// shouldn't stop if the source is bad
    m_renderer->onPlaybackStopped(TEST_SOURCE_ID_BAD);
    ASSERT_FALSE(m_observer->waitFor(RendererObserverInterface::State::STOPPED));

    /// should stop if the source is good
    m_renderer->onPlaybackStopped(TEST_SOURCE_ID_GOOD);
    ASSERT_TRUE(m_observer->waitFor(RendererObserverInterface::State::STOPPED));
}

/**
 * Test if the Renderer correctly handles Playback finishing
 */
TEST_F(RendererTest, onPlaybackFinished) {
    avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId blankSourceId{};

    /// shouldn't finish if the source is bad
    m_renderer->onPlaybackFinished(TEST_SOURCE_ID_BAD);
    ASSERT_FALSE(m_observer->waitFor(RendererObserverInterface::State::STOPPED));

    /// should finish if the source is good
    m_renderer->onPlaybackFinished(blankSourceId);
    ASSERT_TRUE(m_observer->waitFor(RendererObserverInterface::State::STOPPED));
}

/**
 * Test if the Renderer gracefully handles errors when Playback finishing
 */
TEST_F(RendererTest, onPlaybackFinishedError) {
    SetUpTest();

    /// shouldn't finish even if the source is good, if the media player is errored out
    m_mediaPlayer->setSourceRetVal(avsCommon::utils::mediaPlayer::MediaPlayerInterface::ERROR);
    m_renderer->onPlaybackFinished(TEST_SOURCE_ID_GOOD);
    ASSERT_FALSE(m_observer->waitFor(RendererObserverInterface::State::STOPPED));

    /// shouldn't finish even if the source is good, if the media player can't play it
    m_mediaPlayer->setSourceRetVal(TEST_SOURCE_ID_GOOD);
    m_mediaPlayer->setPlayRetVal(false);
    m_renderer->onPlaybackFinished(TEST_SOURCE_ID_GOOD);
    ASSERT_FALSE(m_observer->waitFor(RendererObserverInterface::State::STOPPED));
}

/**
 * Test if the Renderer correctly handles Playback erroring out
 */
TEST_F(RendererTest, onPlaybackError) {
    const avsCommon::utils::mediaPlayer::ErrorType& errorType =
        avsCommon::utils::mediaPlayer::ErrorType::MEDIA_ERROR_INVALID_REQUEST;
    std::string errorMsg = "testError";

    SetUpTest();

    /// shouldn't respond with errors if the source is bad
    m_renderer->onPlaybackError(TEST_SOURCE_ID_BAD, errorType, errorMsg);
    ASSERT_FALSE(m_observer->waitFor(RendererObserverInterface::State::ERROR));

    /// shouldn't respond with errors if the source is good
    m_renderer->onPlaybackError(TEST_SOURCE_ID_GOOD, errorType, errorMsg);
    ASSERT_TRUE(m_observer->waitFor(RendererObserverInterface::State::ERROR));
}

}  // namespace test
}  // namespace renderer
}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
