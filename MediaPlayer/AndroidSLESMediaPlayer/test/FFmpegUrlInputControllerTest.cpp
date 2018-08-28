/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include <sstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/common.h>
#include <libavutil/error.h>
}

#include <AVSCommon/Utils/PlaylistParser/IterativePlaylistParserInterface.h>

#include "AndroidSLESMediaPlayer/FFmpegUrlInputController.h"

namespace alexaClientSDK {
namespace mediaPlayer {
namespace android {
namespace test {

using namespace ::testing;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::playlistParser;

/// Represent a dummy url that will be used as playlist.
static const std::string PLAYLIST_URL{"playlist"};

/// Constant string used to mock a media url.
static const std::string MEDIA_URL_1{"url1"};

/// MP3 test file path relative to the input folder.
static const std::string MP3_FILE_PATH{"/fox_dog.mp3"};

/// MP3 approximate duration.
static const std::chrono::seconds DURATION{5};

/// Zero offset.
static const std::chrono::milliseconds ZERO_OFFSET = std::chrono::milliseconds::zero();

/// Test file path.
static std::string INPUT_FOLDER;

/// Constant struct used to mock return of playlist @c next() method. Calls to @c getContext with this entry will fail.
static const IterativePlaylistParserInterface::PlaylistEntry INVALID_URL_ENTRY{
    .parseResult = PlaylistParseResult::STILL_ONGOING,
    .url = MEDIA_URL_1,
    .duration = DURATION};

class MockPlaylistParser : public IterativePlaylistParserInterface {
public:
    MOCK_METHOD1(initializeParsing, bool(std::string url));
    MOCK_METHOD0(next, PlaylistEntry());
    MOCK_METHOD0(abort, void());
};

class FFmpegUrlInputControllerTest : public Test {
protected:
    void SetUp() override {
        m_parser = std::unique_ptr<MockPlaylistParser>(new MockPlaylistParser());
    }

    void TearDown() override {
        m_parser.reset();
    }

    std::unique_ptr<MockPlaylistParser> m_parser;
};

/// Test input controller create succeed.
TEST_F(FFmpegUrlInputControllerTest, testCreateSucceed) {
    EXPECT_CALL(*m_parser, initializeParsing(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_parser, next()).WillOnce(Return(INVALID_URL_ENTRY));
    auto reader = FFmpegUrlInputController::create(std::move(m_parser), PLAYLIST_URL, ZERO_OFFSET);
    EXPECT_NE(reader, nullptr);
}

/// Test input controller create with null playlist parser failed.
TEST_F(FFmpegUrlInputControllerTest, testCreateNullParserFailed) {
    auto reader = FFmpegUrlInputController::create(nullptr, PLAYLIST_URL, ZERO_OFFSET);
    EXPECT_EQ(reader, nullptr);
}

/// Test input controller create with empty url failed.
TEST_F(FFmpegUrlInputControllerTest, testCreateEmptyUrlFailed) {
    auto reader = FFmpegUrlInputController::create(std::move(m_parser), "", ZERO_OFFSET);
    EXPECT_EQ(reader, nullptr);
}

/// Test input controller create with null playlist parser failed.
TEST_F(FFmpegUrlInputControllerTest, testCreateInvalidUrlFailed) {
    EXPECT_CALL(*m_parser, initializeParsing(_)).WillOnce(Return(false));
    auto reader = FFmpegUrlInputController::create(std::move(m_parser), PLAYLIST_URL, ZERO_OFFSET);
    EXPECT_EQ(reader, nullptr);
}

/// Test input controller getContext.
TEST_F(FFmpegUrlInputControllerTest, testGetContextSucceed) {
    IterativePlaylistParserInterface::PlaylistEntry validEntry{
        .url = INPUT_FOLDER + MP3_FILE_PATH, .duration = DURATION, .parseResult = PlaylistParseResult::FINISHED};
    EXPECT_CALL(*m_parser, initializeParsing(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_parser, next()).WillOnce(Return(validEntry));
    auto reader = FFmpegUrlInputController::create(std::move(m_parser), PLAYLIST_URL, ZERO_OFFSET);
    ASSERT_NE(reader, nullptr);

    auto context = reader->getCurrentFormatContext();
    EXPECT_EQ(std::get<0>(context), FFmpegUrlInputController::Result::OK);
    ASSERT_NE(std::get<1>(context), nullptr);
    EXPECT_NE(std::get<1>(context)->url, nullptr);
    EXPECT_EQ(std::get<2>(context), ZERO_OFFSET);
}

/// Test input controller getContext with non-zero offset.
TEST_F(FFmpegUrlInputControllerTest, testGetContextOffsetSucceed) {
    IterativePlaylistParserInterface::PlaylistEntry validEntry{
        .url = INPUT_FOLDER + MP3_FILE_PATH, .duration = DURATION, .parseResult = PlaylistParseResult::FINISHED};
    EXPECT_CALL(*m_parser, initializeParsing(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_parser, next()).WillOnce(Return(validEntry));
    std::chrono::milliseconds skipHalf{DURATION / 2};
    auto reader = FFmpegUrlInputController::create(std::move(m_parser), PLAYLIST_URL, skipHalf);
    ASSERT_NE(reader, nullptr);

    auto context = reader->getCurrentFormatContext();
    EXPECT_EQ(std::get<0>(context), FFmpegUrlInputController::Result::OK);
    ASSERT_NE(std::get<1>(context), nullptr);
    EXPECT_NE(std::get<1>(context)->url, nullptr);
    EXPECT_EQ(std::get<2>(context), skipHalf);
}

/// Test input controller getContext.
TEST_F(FFmpegUrlInputControllerTest, testGetContextInvalidUrl) {
    EXPECT_CALL(*m_parser, initializeParsing(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_parser, next()).WillOnce(Return(INVALID_URL_ENTRY));
    auto reader = FFmpegUrlInputController::create(std::move(m_parser), PLAYLIST_URL, ZERO_OFFSET);
    ASSERT_NE(reader, nullptr);

    auto context = reader->getCurrentFormatContext();
    EXPECT_EQ(std::get<0>(context), FFmpegUrlInputController::Result::ERROR);
    EXPECT_EQ(std::get<1>(context), nullptr);
}

/// Test get context after switching files.
TEST_F(FFmpegUrlInputControllerTest, testGetContextAfterNext) {
    IterativePlaylistParserInterface::PlaylistEntry validEntry{
        .url = INPUT_FOLDER + MP3_FILE_PATH, .duration = DURATION, .parseResult = PlaylistParseResult::FINISHED};

    // Parser will return INVALID_URL_ENTRY first, then validEntry
    EXPECT_CALL(*m_parser, next()).WillOnce(Return(validEntry));
    EXPECT_CALL(*m_parser, next()).Times(1).WillRepeatedly(Return(INVALID_URL_ENTRY)).RetiresOnSaturation();
    EXPECT_CALL(*m_parser, initializeParsing(_)).WillOnce(Return(true));

    auto reader = FFmpegUrlInputController::create(std::move(m_parser), PLAYLIST_URL, ZERO_OFFSET);
    ASSERT_NE(reader, nullptr);

    ASSERT_TRUE(reader->hasNext());
    ASSERT_TRUE(reader->next());

    auto context = reader->getCurrentFormatContext();
    EXPECT_EQ(std::get<0>(context), FFmpegUrlInputController::Result::OK);
    ASSERT_NE(std::get<1>(context), nullptr);
    EXPECT_NE(std::get<1>(context)->url, nullptr);
}

/// Test has next when parser isn't done.
TEST_F(FFmpegUrlInputControllerTest, testHasNext) {
    IterativePlaylistParserInterface::PlaylistEntry validEntry{
        .url = INPUT_FOLDER + MP3_FILE_PATH, .duration = DURATION, .parseResult = PlaylistParseResult::STILL_ONGOING};
    EXPECT_CALL(*m_parser, initializeParsing(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_parser, next()).WillOnce(Return(validEntry));
    auto reader = FFmpegUrlInputController::create(std::move(m_parser), PLAYLIST_URL, ZERO_OFFSET);
    ASSERT_NE(reader, nullptr);

    EXPECT_TRUE(reader->hasNext());
}

/// Test has next after playlist parser is done.
TEST_F(FFmpegUrlInputControllerTest, testDone) {
    IterativePlaylistParserInterface::PlaylistEntry validEntry{
        .url = INPUT_FOLDER + MP3_FILE_PATH, .duration = DURATION, .parseResult = PlaylistParseResult::FINISHED};
    EXPECT_CALL(*m_parser, initializeParsing(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_parser, next()).WillOnce(Return(validEntry));
    auto reader = FFmpegUrlInputController::create(std::move(m_parser), PLAYLIST_URL, ZERO_OFFSET);
    ASSERT_NE(reader, nullptr);

    EXPECT_FALSE(reader->hasNext());
}

}  // namespace test
}  // namespace android
}  // namespace mediaPlayer
}  // namespace alexaClientSDK

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    if (argc < 2) {
        std::cerr << "Usage: " << std::string(argv[0]) << " <absolute path to test input file>" << std::endl;
        return -1;
    } else {
        alexaClientSDK::mediaPlayer::android::test::INPUT_FOLDER = std::string(argv[1]);
        return RUN_ALL_TESTS();
    }
}
