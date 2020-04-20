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

#include <AVSCommon/Utils/MediaPlayer/PooledMediaPlayerFactory.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/MediaPlayer/MockMediaPlayer.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace alexaClientSDK {
namespace mediaPlayer {
namespace test {

using namespace ::testing;
using namespace avsCommon::utils::mediaPlayer;
using namespace avsCommon::utils::mediaPlayer::test;

/// String to identify log entries originating from this file.
static const std::string TAG("PooledMediaPlayerFactoryTest");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

class TestFactoryObserver : public MediaPlayerFactoryObserverInterface {
public:
    TestFactoryObserver() = default;
    virtual ~TestFactoryObserver() = default;

    MOCK_METHOD0(onReadyToProvideNextPlayer, void());
};

class PooledMediaPlayerFactoryTest : public ::testing::Test {
public:
    PooledMediaPlayerFactoryTest() = default;
    virtual ~PooledMediaPlayerFactoryTest() = default;

    void SetUp() override;
    void TearDown() override;
    const std::vector<std::shared_ptr<MediaPlayerInterface>> createPlayers(int playerCnt);

    std::shared_ptr<TestFactoryObserver> m_mockObserver;
    std::shared_ptr<PooledMediaPlayerFactory> m_factory;
    std::vector<std::shared_ptr<MediaPlayerInterface>> m_playerList;
};

void PooledMediaPlayerFactoryTest::SetUp() {
    MockMediaPlayer::enableConcurrentMediaPlayers();
    m_mockObserver = std::make_shared<TestFactoryObserver>();
}

void PooledMediaPlayerFactoryTest::TearDown() {
    for (std::shared_ptr<MediaPlayerInterface> player : m_playerList) {
        static_cast<MockMediaPlayer*>(player.get())->shutdown();
    }
    m_playerList.clear();
    if (m_factory) {
        m_factory.reset();
    }
    m_mockObserver.reset();
}

const std::vector<std::shared_ptr<MediaPlayerInterface>> PooledMediaPlayerFactoryTest::createPlayers(int playerCnt) {
    for (int i = 0; i < playerCnt; i++) {
        m_playerList.push_back(MockMediaPlayer::create());
    }
    return m_playerList;
}

TEST_F(PooledMediaPlayerFactoryTest, test_acquirePlayers) {
    for (int i = 1; i <= 3; i++) {
        m_factory = PooledMediaPlayerFactory::create(createPlayers(i));

        for (int j = 0; j < i; j++) {
            auto player = m_factory->acquireMediaPlayer();
            ASSERT_NE(player, nullptr);
        }

        auto player = m_factory->acquireMediaPlayer();
        ASSERT_EQ(player, nullptr);

        m_playerList.clear();
    }
}

TEST_F(PooledMediaPlayerFactoryTest, test_releasePlayers) {
    m_factory = PooledMediaPlayerFactory::create(createPlayers(1));

    auto player = m_factory->acquireMediaPlayer();
    ASSERT_NE(player, nullptr);
    ASSERT_TRUE(m_factory->releaseMediaPlayer(player));

    // double release
    ASSERT_FALSE(m_factory->releaseMediaPlayer(player));
    // not acquired
    auto newPlayer = MockMediaPlayer::create();
    ASSERT_FALSE(m_factory->releaseMediaPlayer(newPlayer));
    newPlayer->shutdown();
    // nullptr
    ASSERT_FALSE(m_factory->releaseMediaPlayer(nullptr));
}

TEST_F(PooledMediaPlayerFactoryTest, test_recyclePlayers) {
    for (int i = 1; i <= 10; i++) {
        m_factory = PooledMediaPlayerFactory::create(createPlayers(i));

        for (int j = 0; j < i; j++) {
            auto player = m_factory->acquireMediaPlayer();
            ASSERT_NE(player, nullptr);
            ASSERT_TRUE(m_factory->releaseMediaPlayer(player));
        }
        for (int j = 0; j < i * 5; j++) {
            auto player = m_factory->acquireMediaPlayer();
            auto pos = std::find(m_playerList.begin(), m_playerList.end(), player);
            ASSERT_NE(pos, m_playerList.end());
            ASSERT_TRUE(m_factory->releaseMediaPlayer(player));
        }

        m_playerList.clear();
    }
}

TEST_F(PooledMediaPlayerFactoryTest, test_onReadyCallback) {
    m_factory = PooledMediaPlayerFactory::create(createPlayers(2));
    m_factory->addObserver(m_mockObserver);

    int count = 0;
    // onReadyToProvideNextPlayer should only be called when the
    // pool player count goes from 0 -> 1
    EXPECT_CALL(*m_mockObserver, onReadyToProvideNextPlayer()).Times(2).WillRepeatedly(Invoke([&count]() { count++; }));
    ;

    auto player1 = m_factory->acquireMediaPlayer();
    ASSERT_NE(player1, nullptr);
    auto player2 = m_factory->acquireMediaPlayer();
    ASSERT_NE(player2, nullptr);

    EXPECT_EQ(0, count);

    ASSERT_TRUE(m_factory->releaseMediaPlayer(player1));
    EXPECT_EQ(1, count);
    ASSERT_TRUE(m_factory->releaseMediaPlayer(player2));
    EXPECT_EQ(1, count);

    player1 = m_factory->acquireMediaPlayer();
    ASSERT_NE(player1, nullptr);
    ASSERT_TRUE(m_factory->releaseMediaPlayer(player1));
    EXPECT_EQ(1, count);

    player1 = m_factory->acquireMediaPlayer();
    ASSERT_NE(player1, nullptr);
    player2 = m_factory->acquireMediaPlayer();
    ASSERT_NE(player2, nullptr);

    ASSERT_TRUE(m_factory->releaseMediaPlayer(player1));
    EXPECT_EQ(2, count);
    ASSERT_TRUE(m_factory->releaseMediaPlayer(player2));
    EXPECT_EQ(2, count);
}

TEST_F(PooledMediaPlayerFactoryTest, test_isMediaPlayerAvailable) {
    m_factory = PooledMediaPlayerFactory::create(createPlayers(2));

    ASSERT_TRUE(m_factory->isMediaPlayerAvailable());
    auto player1 = m_factory->acquireMediaPlayer();
    ASSERT_TRUE(m_factory->isMediaPlayerAvailable());
    auto player2 = m_factory->acquireMediaPlayer();
    ASSERT_FALSE(m_factory->isMediaPlayerAvailable());

    m_factory->releaseMediaPlayer(player1);
    ASSERT_TRUE(m_factory->isMediaPlayerAvailable());
    m_factory->releaseMediaPlayer(player2);
}

}  // namespace test
}  // namespace mediaPlayer
}  // namespace alexaClientSDK
