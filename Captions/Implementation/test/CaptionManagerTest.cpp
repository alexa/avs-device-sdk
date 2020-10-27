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

/// @file CaptionManagerTest.cpp

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/MediaPlayer/MockMediaPlayer.h>
#include <Captions/CaptionManager.h>
#include <Captions/CaptionLine.h>

#include "MockCaptionParser.h"
#include "MockCaptionPresenter.h"
#include "TestTimingAdapterFactory.cpp"

namespace alexaClientSDK {
namespace captions {
namespace test {

using namespace ::testing;
using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::utils;
using namespace avsCommon::utils::mediaPlayer;
using namespace avsCommon::utils::mediaPlayer::test;

/**
 * Test rig.
 */
class CaptionManagerTest : public ::testing::Test {
public:
    void SetUp() override;

    void TearDown() override;

    /// The system under test.
    std::shared_ptr<CaptionManager> caption_manager;

    /// Mock player with which to exercise CaptionManager.
    std::shared_ptr<MockMediaPlayer> m_player;

    /// Mock parser with which to exercise CaptionManager.
    std::shared_ptr<MockCaptionParser> m_parser;

    /// Mock presenter with which to exercise CaptionManager.
    std::shared_ptr<NiceMock<MockCaptionPresenter>> m_presenter;

    /// An implementation of the timing adapter factory with which to exercise CaptionManager, which returns mocks.
    std::shared_ptr<TestTimingAdapterFactory> m_timingFactory;
};

void CaptionManagerTest::SetUp() {
    avsCommon::utils::logger::getConsoleLogger()->setLevel(avsCommon::utils::logger::Level::DEBUG9);

    m_player = MockMediaPlayer::create();

    m_parser = std::make_shared<NiceMock<MockCaptionParser>>();

    m_presenter = std::make_shared<NiceMock<MockCaptionPresenter>>();

    m_timingFactory = std::make_shared<TestTimingAdapterFactory>();

    caption_manager = CaptionManager::create(m_parser, m_timingFactory);

    caption_manager->addMediaPlayer(m_player);

    caption_manager->setCaptionPresenter(m_presenter);
}

void CaptionManagerTest::TearDown() {
    if (m_player) {
        m_player->shutdown();
    }
    if (caption_manager) {
        caption_manager->shutdown();
    }
}

/**
 * Sanity test to ensure the TestTimingAdapterFactory is returning the expected result.
 */
TEST_F(CaptionManagerTest, test_testTestTimingAdapterFactory) {
    auto mockTimingAdapter = m_timingFactory->getMockTimingAdapter();
    ASSERT_EQ(mockTimingAdapter, m_timingFactory->getTimingAdapter(nullptr));
}

/**
 * Test that CaptionManager::setMediaPlayers() adds a media player that is bound to media events.
 */
TEST_F(CaptionManagerTest, test_testSetMediaPlayerBindsMediaPlayer) {
    auto mockTimingAdapter = m_timingFactory->getMockTimingAdapter();
    auto sourceID1 = m_player->setSource("http://fake.url", std::chrono::milliseconds(0));

    // CaptionManager::setMediaPlayers() has already been called in CaptionManagerTest::SetUp().
    // in order to check this, the timing adapter needs to be pre-loaded.
    caption_manager->onParsed(CaptionFrame(sourceID1));

    EXPECT_CALL(*mockTimingAdapter, pause()).Times(1);

    // trigger the finished event from the media player
    m_player->mockPause(sourceID1);
}

/**
 * Test that create fails with null arguments
 */
TEST_F(CaptionManagerTest, test_createWithNullArgs) {
    auto manager = CaptionManager::create(nullptr, nullptr);
    ASSERT_FALSE(manager);
}

/**
 * Test that create succeeds with a null TimingAdapterFactory.
 */
TEST_F(CaptionManagerTest, test_createWithNullTimingAdapterFactory) {
    auto manager = CaptionManager::create(m_parser, nullptr);
    ASSERT_NE(nullptr, manager);
}

/**
 * Test that create fails with a null Parser.
 */
TEST_F(CaptionManagerTest, test_createWithNullParser) {
    auto manager = CaptionManager::create(nullptr, m_timingFactory);
    ASSERT_FALSE(manager);
}

/**
 * Test that the source ID is maintained from onParsed() to queueForDisplay().
 */
TEST_F(CaptionManagerTest, test_sourceIdDoesNotChange) {
    auto mockTimingAdapter = m_timingFactory->getMockTimingAdapter();
    int sourceID1 = 1;

    auto expectedCaptionFrame = CaptionFrame(sourceID1);
    EXPECT_CALL(*mockTimingAdapter.get(), queueForDisplay(expectedCaptionFrame, _)).Times(1);

    caption_manager->onParsed(CaptionFrame(sourceID1));
}

/**
 * Test the media focus behavior for a single media player instance.
 */
TEST_F(CaptionManagerTest, test_singleMediaPlayerPause) {
    auto mockTimingAdapter = m_timingFactory->getMockTimingAdapter();
    std::vector<CaptionLine> expectedLines;
    CaptionLine expectedLine1 = CaptionLine("The time is 2:17 PM.", {TextStyle()});
    expectedLines.emplace_back(expectedLine1);
    auto expectedCaptionFrame =
        CaptionFrame(1, std::chrono::milliseconds(1), std::chrono::milliseconds(0), expectedLines);
    EXPECT_CALL(*mockTimingAdapter.get(), queueForDisplay(expectedCaptionFrame, _)).Times(1);
    EXPECT_CALL(*m_presenter, getWrapIndex(_)).Times(1).WillOnce(Return(std::pair<bool, uint32_t>(false, 0)));

    std::vector<CaptionLine> lines;
    CaptionLine line = CaptionLine("The time is 2:17 PM.", {});
    lines.emplace_back(line);
    auto captionFrame = CaptionFrame(1, std::chrono::milliseconds(1), std::chrono::milliseconds(0), lines);
    caption_manager->onParsed(captionFrame);
}

/**
 * Tests the splitting behavior when the caption is all spaces.
 */
TEST_F(CaptionManagerTest, test_splitCaptionFrameWhitespaceOnly) {
    auto mockTimingAdapter = m_timingFactory->getMockTimingAdapter();
    std::vector<CaptionLine> expectedLines;
    CaptionLine expectedLine1 = CaptionLine("     ", {TextStyle()});
    expectedLines.emplace_back(expectedLine1);
    auto expectedCaptionFrame =
        CaptionFrame(1, std::chrono::milliseconds(1), std::chrono::milliseconds(0), expectedLines);
    EXPECT_CALL(*mockTimingAdapter, queueForDisplay(expectedCaptionFrame, _)).Times(1);
    EXPECT_CALL(*m_presenter, getWrapIndex(_)).Times(1).WillOnce(Return(std::pair<bool, uint32_t>(false, 0)));

    std::vector<CaptionLine> lines;
    CaptionLine line = CaptionLine("     ", {});
    lines.emplace_back(line);
    auto captionFrame = CaptionFrame(1, std::chrono::milliseconds(1), std::chrono::milliseconds(0), lines);
    caption_manager->onParsed(captionFrame);
}

/**
 * Tests the splitting behavior when the caption is all spaces after a line wrap.
 */
TEST_F(CaptionManagerTest, test_splitCaptionFrameWhitespaceAfterLineWrap) {
    auto mockTimingAdapter = m_timingFactory->getMockTimingAdapter();
    std::vector<CaptionLine> expectedLines;
    CaptionLine expectedLine1 = CaptionLine("The time is 2:17 PM.", {TextStyle()});
    expectedLines.emplace_back(expectedLine1);
    auto expectedCaptionFrame =
        CaptionFrame(1, std::chrono::milliseconds(1), std::chrono::milliseconds(0), expectedLines);
    EXPECT_CALL(*mockTimingAdapter, queueForDisplay(expectedCaptionFrame, _)).Times(1);
    EXPECT_CALL(*m_presenter, getWrapIndex(_))
        .Times(2)
        .WillOnce(Return(std::pair<bool, uint32_t>(true, 20)))
        .WillOnce(Return(std::pair<bool, uint32_t>(false, 0)));
    std::vector<CaptionLine> lines;
    CaptionLine line = CaptionLine("The time is 2:17 PM.     ", {});
    lines.emplace_back(line);
    auto captionFrame = CaptionFrame(1, std::chrono::milliseconds(1), std::chrono::milliseconds(0), lines);
    caption_manager->onParsed(captionFrame);
}

/**
 * Tests the splitting behavior when the a no whitespace is present at before the requested wrap index.
 */
TEST_F(CaptionManagerTest, test_splitCaptionFrameNoWhitespaceBeforeWrapIndex) {
    auto mockTimingAdapter = m_timingFactory->getMockTimingAdapter();
    std::vector<CaptionLine> expectedLines;
    CaptionLine expectedLine1 = CaptionLine("Thiscapti", {TextStyle()});
    CaptionLine expectedLine2 = CaptionLine("onhasnosp", {TextStyle()});
    CaptionLine expectedLine3 = CaptionLine("aces", {TextStyle()});
    expectedLines.emplace_back(expectedLine1);
    expectedLines.emplace_back(expectedLine2);
    expectedLines.emplace_back(expectedLine3);
    auto expectedCaptionFrame =
        CaptionFrame(1, std::chrono::milliseconds(1), std::chrono::milliseconds(0), expectedLines);
    EXPECT_CALL(*mockTimingAdapter, queueForDisplay(expectedCaptionFrame, _)).Times(1);
    EXPECT_CALL(*m_presenter, getWrapIndex(_))
        .Times(3)
        .WillOnce(Return(std::pair<bool, uint32_t>(true, 9)))
        .WillOnce(Return(std::pair<bool, uint32_t>(true, 9)))
        .WillOnce(Return(std::pair<bool, uint32_t>(false, 0)));
    std::vector<CaptionLine> lines;
    CaptionLine line = CaptionLine("Thiscaptionhasnospaces", {});
    lines.emplace_back(line);
    auto captionFrame = CaptionFrame(1, std::chrono::milliseconds(1), std::chrono::milliseconds(0), lines);
    caption_manager->onParsed(captionFrame);
}

/**
 * Tests the splitting behavior when no split is needed.
 */
TEST_F(CaptionManagerTest, test_splitCaptionFrameFalseWillNotSplitLine) {
    auto mockTimingAdapter = m_timingFactory->getMockTimingAdapter();
    std::vector<CaptionLine> expectedLines;
    CaptionLine expectedLine1 = CaptionLine("The time is 2:17 PM.", {TextStyle()});
    expectedLines.emplace_back(expectedLine1);
    auto expectedCaptionFrame =
        CaptionFrame(1, std::chrono::milliseconds(1), std::chrono::milliseconds(0), expectedLines);
    EXPECT_CALL(*mockTimingAdapter, queueForDisplay(expectedCaptionFrame, _)).Times(1);
    EXPECT_CALL(*m_presenter, getWrapIndex(_)).Times(1).WillOnce(Return(std::pair<bool, uint32_t>(false, 0)));

    std::vector<CaptionLine> lines;
    CaptionLine line = CaptionLine("The time is 2:17 PM.", {});
    lines.emplace_back(line);
    auto captionFrame = CaptionFrame(1, std::chrono::milliseconds(1), std::chrono::milliseconds(0), lines);
    caption_manager->onParsed(captionFrame);
}

/**
 * Tests the splitting of a caption frame at a character index that happens to fall on a space character ('0x20').
 */
TEST_F(CaptionManagerTest, test_splitCaptionFrameAtSpaceIndex) {
    auto mockTimingAdapter = m_timingFactory->getMockTimingAdapter();
    std::vector<CaptionLine> expectedLines;

    CaptionLine expectedLine1 = CaptionLine("The time is", {TextStyle()});
    CaptionLine expectedLine2 = CaptionLine("2:17 PM.", {TextStyle()});
    expectedLines.emplace_back(expectedLine1);
    expectedLines.emplace_back(expectedLine2);
    auto expectedCaptionFrame =
        CaptionFrame(1, std::chrono::milliseconds(1), std::chrono::milliseconds(0), expectedLines);
    EXPECT_CALL(*m_presenter, getWrapIndex(_))
        .Times(2)
        .WillOnce(Return(std::pair<bool, uint32_t>(true, 12)))
        .WillOnce(Return(std::pair<bool, uint32_t>(false, 0)));
    EXPECT_CALL(*mockTimingAdapter, queueForDisplay(expectedCaptionFrame, _)).Times(1);

    std::vector<CaptionLine> lines;
    CaptionLine line1 = CaptionLine("The time is 2:17 PM.", {});
    lines.emplace_back(line1);
    auto captionFrame = CaptionFrame(1, std::chrono::milliseconds(1), std::chrono::milliseconds(0), lines);
    caption_manager->onParsed(captionFrame);
}

/**
 * Test that CaptionManager::addMediaPlayer() does not add the same media player twice.
 */
TEST_F(CaptionManagerTest, test_testAddDuplicateMediaPlayerFails) {
    auto mockTimingAdapter = m_timingFactory->getMockTimingAdapter();

    EXPECT_CALL(*mockTimingAdapter, pause()).Times(1);

    // Re-add the player added in Setup() (a duplicate add).
    caption_manager->addMediaPlayer(m_player);

    // test that the media player is still bound
    auto sourceID2 = m_player->setSource("http://fake.url.com", std::chrono::milliseconds(0));

    caption_manager->onParsed(CaptionFrame(sourceID2));

    // trigger the finished event from the media player
    m_player->mockPause(sourceID2);
}

/**
 * Test that CaptionManager::addMediaPlayer() adds a media player that is bound to media events.
 */
TEST_F(CaptionManagerTest, test_testAddMediaPlayerBindsMediaPlayer) {
    auto playerToAdd = MockMediaPlayer::create();
    auto mockTimingAdapter = m_timingFactory->getMockTimingAdapter();
    auto sourceID1 = playerToAdd->setSource("http://fake.url", std::chrono::milliseconds(0));

    EXPECT_CALL(*mockTimingAdapter, pause()).Times(2);

    caption_manager->addMediaPlayer(playerToAdd);

    // in order to check this, the timing adapter needs to be pre-loaded.
    caption_manager->onParsed(CaptionFrame(sourceID1));

    // trigger the finished event from the media player
    playerToAdd->mockPause(sourceID1);

    // test that the other media players are also still bound
    auto sourceID2 = m_player->setSource("http://fake.url.com", std::chrono::milliseconds(0));

    caption_manager->onParsed(CaptionFrame(sourceID2));

    // trigger the finished event from the media player
    m_player->mockPause(sourceID2);
}

/**
 * Test that CaptionManager::removeMediaPlayer() removes a media player bound to media events.
 */
TEST_F(CaptionManagerTest, test_testRemoveMediaPlayerUnbindsMediaPlayer) {
    auto mockTimingAdapter = m_timingFactory->getMockTimingAdapter();
    auto sourceID1 = m_player->setSource("http://fake.url", std::chrono::milliseconds(0));

    caption_manager->removeMediaPlayer(m_player);

    // in order to check this, the timing adapter needs to be pre-loaded.
    caption_manager->onParsed(CaptionFrame(sourceID1));

    // no calls expected now that the media player was removed
    EXPECT_CALL(*mockTimingAdapter, pause()).Times(0);

    // trigger the finished event from the media player
    m_player->mockPause(sourceID1);
}

/**
 * Tests that the caption manager is enabled after construction when
 * captions are enabled.
 */
TEST_F(CaptionManagerTest, test_isEnabled) {
#ifdef ENABLE_CAPTIONS
    ASSERT_TRUE(caption_manager->isEnabled());
#else
    ASSERT_FALSE(caption_manager->isEnabled());
#endif
}

}  // namespace test
}  // namespace captions
}  // namespace alexaClientSDK
