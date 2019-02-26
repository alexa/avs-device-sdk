/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <AVSCommon/Utils/Memory/Memory.h>

#include "PlaylistParser/MockContentFetcher.h"
#include <PlaylistParser/IterativePlaylistParser.h>

#include "AndroidSLESMediaPlayer/FFmpegUrlInputController.h"

namespace alexaClientSDK {
namespace mediaPlayer {
namespace android {
namespace test {

using namespace ::testing;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::playlistParser;
using namespace alexaClientSDK::playlistParser::test;

/// Represent a dummy url that will be used as playlist.
static const std::string PLAYLIST_URL{"playlist"};

/// Constant string used to mock a media url.
static const std::string MEDIA_URL_1{"url1"};

/// MP3 test file path relative to the input folder.
static const std::string MP3_FILE_PATH{"/fox_dog.mp3"};

/// MP3 approximate duration.
static const std::chrono::seconds DURATION{5};

static const bool DO_NOT_REPEAT = false;

/// Zero offset.
static const std::chrono::milliseconds ZERO_OFFSET = std::chrono::milliseconds::zero();

/// Test file path.
static std::string INPUT_FOLDER;

/// Constant struct used to mock return of playlist @c next() method. Calls to @c getContext with this entry will fail.
static const PlaylistEntry INVALID_URL_ENTRY = PlaylistEntry::createErrorEntry(MEDIA_URL_1);

/// A mock factory that creates mock content fetchers
class MockContentFetcherFactory : public avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface {
public:
    MOCK_METHOD1(
        create,
        std::unique_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterface>(const std::string& url));
};

class MockPlaylistParser : public IterativePlaylistParserInterface {
public:
    MOCK_METHOD1(initializeParsing, bool(std::string url));
    MOCK_METHOD0(next, PlaylistEntry());
    MOCK_METHOD0(abort, void());

    MockPlaylistParser() {
        auto mockFactory = std::make_shared<MockContentFetcherFactory>();
        EXPECT_CALL(*mockFactory, create(_)).WillRepeatedly(Invoke([](const std::string& url) {
            return avsCommon::utils::memory::make_unique<MockContentFetcher>(url);
        }));
        m_realParser = alexaClientSDK::playlistParser::IterativePlaylistParser::create(mockFactory);
    }

    bool initializeParsingReal(std::string url) {
        return m_realParser->initializeParsing(url);
    }

    PlaylistEntry nextReal() {
        return m_realParser->next();
    }

private:
    std::unique_ptr<alexaClientSDK::playlistParser::IterativePlaylistParser> m_realParser;
};

class FFmpegUrlInputControllerTest : public Test {
protected:
    void SetUp() override {
        m_parser = std::shared_ptr<MockPlaylistParser>(new MockPlaylistParser());
    }

    void TearDown() override {
        m_parser.reset();
    }

    std::shared_ptr<MockPlaylistParser> m_parser;
};

/// Test input controller create succeed.
TEST_F(FFmpegUrlInputControllerTest, testCreateSucceed) {
    EXPECT_CALL(*m_parser, initializeParsing(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_parser, next()).WillOnce(Return(INVALID_URL_ENTRY));
    auto reader = FFmpegUrlInputController::create(m_parser, PLAYLIST_URL, ZERO_OFFSET, DO_NOT_REPEAT);
    EXPECT_NE(reader, nullptr);
}

/// Test input controller create with null playlist parser failed.
TEST_F(FFmpegUrlInputControllerTest, testCreateNullParserFailed) {
    auto reader = FFmpegUrlInputController::create(nullptr, PLAYLIST_URL, ZERO_OFFSET, DO_NOT_REPEAT);
    EXPECT_EQ(reader, nullptr);
}

/// Test input controller create with empty url failed.
TEST_F(FFmpegUrlInputControllerTest, testCreateEmptyUrlFailed) {
    auto reader = FFmpegUrlInputController::create(m_parser, "", ZERO_OFFSET, DO_NOT_REPEAT);
    EXPECT_EQ(reader, nullptr);
}

/// Test input controller create with null playlist parser failed.
TEST_F(FFmpegUrlInputControllerTest, testCreateInvalidUrlFailed) {
    EXPECT_CALL(*m_parser, initializeParsing(_)).WillOnce(Return(false));
    auto reader = FFmpegUrlInputController::create(m_parser, PLAYLIST_URL, ZERO_OFFSET, DO_NOT_REPEAT);
    EXPECT_EQ(reader, nullptr);
}

/// Test input controller getContext.
TEST_F(FFmpegUrlInputControllerTest, testGetContextSucceed) {
    PlaylistEntry validEntry = PlaylistEntry(INPUT_FOLDER + MP3_FILE_PATH, DURATION, PlaylistParseResult::FINISHED);
    EXPECT_CALL(*m_parser, initializeParsing(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_parser, next()).WillOnce(Return(validEntry));
    auto reader = FFmpegUrlInputController::create(m_parser, PLAYLIST_URL, ZERO_OFFSET, DO_NOT_REPEAT);
    ASSERT_NE(reader, nullptr);

    auto context = reader->getCurrentFormatContext();
    EXPECT_EQ(std::get<0>(context), FFmpegUrlInputController::Result::OK);
    ASSERT_NE(std::get<1>(context), nullptr);
    EXPECT_NE(std::get<1>(context)->url, nullptr);
    EXPECT_EQ(std::get<2>(context), ZERO_OFFSET);
}

/// Test input controller getContext with non-zero offset.
TEST_F(FFmpegUrlInputControllerTest, testGetContextOffsetSucceed) {
    PlaylistEntry validEntry = PlaylistEntry(INPUT_FOLDER + MP3_FILE_PATH, DURATION, PlaylistParseResult::FINISHED);
    EXPECT_CALL(*m_parser, initializeParsing(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_parser, next()).WillOnce(Return(validEntry));
    std::chrono::milliseconds skipHalf{DURATION / 2};
    auto reader = FFmpegUrlInputController::create(m_parser, PLAYLIST_URL, skipHalf, DO_NOT_REPEAT);
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
    auto reader = FFmpegUrlInputController::create(m_parser, PLAYLIST_URL, ZERO_OFFSET, DO_NOT_REPEAT);
    ASSERT_NE(reader, nullptr);

    auto context = reader->getCurrentFormatContext();
    EXPECT_EQ(std::get<0>(context), FFmpegUrlInputController::Result::ERROR);
    EXPECT_EQ(std::get<1>(context), nullptr);
}

/// Test get context after switching files.
TEST_F(FFmpegUrlInputControllerTest, testGetContextAfterNext) {
    PlaylistEntry validEntry = PlaylistEntry(INPUT_FOLDER + MP3_FILE_PATH, DURATION, PlaylistParseResult::FINISHED);

    // Parser will return INVALID_URL_ENTRY first, then validEntry
    EXPECT_CALL(*m_parser, next()).WillOnce(Return(validEntry));
    EXPECT_CALL(*m_parser, next()).Times(1).WillRepeatedly(Return(INVALID_URL_ENTRY)).RetiresOnSaturation();
    EXPECT_CALL(*m_parser, initializeParsing(_)).WillOnce(Return(true));

    auto reader = FFmpegUrlInputController::create(m_parser, PLAYLIST_URL, ZERO_OFFSET, DO_NOT_REPEAT);
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
    PlaylistEntry validEntry =
        PlaylistEntry(INPUT_FOLDER + MP3_FILE_PATH, DURATION, PlaylistParseResult::STILL_ONGOING);
    EXPECT_CALL(*m_parser, initializeParsing(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_parser, next()).WillOnce(Return(validEntry));
    auto reader = FFmpegUrlInputController::create(m_parser, PLAYLIST_URL, ZERO_OFFSET, DO_NOT_REPEAT);
    ASSERT_NE(reader, nullptr);

    EXPECT_TRUE(reader->hasNext());
}

/// Test has next after playlist parser is done.
TEST_F(FFmpegUrlInputControllerTest, testDone) {
    PlaylistEntry validEntry = PlaylistEntry(INPUT_FOLDER + MP3_FILE_PATH, DURATION, PlaylistParseResult::FINISHED);
    EXPECT_CALL(*m_parser, initializeParsing(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_parser, next()).WillOnce(Return(validEntry));
    auto reader = FFmpegUrlInputController::create(m_parser, PLAYLIST_URL, ZERO_OFFSET, DO_NOT_REPEAT);
    ASSERT_NE(reader, nullptr);

    EXPECT_FALSE(reader->hasNext());
}

/// Test parsing playlist with repeat on
TEST_F(FFmpegUrlInputControllerTest, testPlaylistRepeat) {
    bool repeat = true;
    EXPECT_CALL(*m_parser, initializeParsing(TEST_M3U_PLAYLIST_URL))
        .WillOnce(Invoke(m_parser.get(), &MockPlaylistParser::initializeParsingReal))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_parser, next())
        .WillOnce(Invoke(m_parser.get(), &MockPlaylistParser::nextReal))
        .RetiresOnSaturation();
    auto reader = FFmpegUrlInputController::create(m_parser, TEST_M3U_PLAYLIST_URL, ZERO_OFFSET, repeat);
    ASSERT_NE(reader, nullptr);
    EXPECT_EQ(reader->getCurrentUrl(), TEST_M3U_PLAYLIST_URLS[0]);

    size_t i, index;
    size_t loopSize = TEST_M3U_PLAYLIST_URLS.size() * 3;
    for (i = 1; i < loopSize; i++) {
        index = (i % TEST_M3U_PLAYLIST_URLS.size());
        EXPECT_TRUE(reader->hasNext());
        EXPECT_CALL(*m_parser, next())
            .WillOnce(Invoke(m_parser.get(), &MockPlaylistParser::nextReal))
            .RetiresOnSaturation();

        if (index == (TEST_M3U_PLAYLIST_URLS.size() - 1)) {
            EXPECT_CALL(*m_parser, initializeParsing(TEST_M3U_PLAYLIST_URL))
                .WillOnce(Invoke(m_parser.get(), &MockPlaylistParser::initializeParsingReal))
                .RetiresOnSaturation();
        }
        EXPECT_TRUE(reader->next());
        EXPECT_EQ(reader->getCurrentUrl(), TEST_M3U_PLAYLIST_URLS[index]);
    }
}

/// Test parsing media url with repeat on
TEST_F(FFmpegUrlInputControllerTest, testMediaUrlRepeat) {
    bool repeat = true;
    EXPECT_CALL(*m_parser, initializeParsing(TEST_MEDIA_URL))
        .Times(2)
        .WillRepeatedly(Invoke(m_parser.get(), &MockPlaylistParser::initializeParsingReal))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_parser, next())
        .WillOnce(Invoke(m_parser.get(), &MockPlaylistParser::nextReal))
        .RetiresOnSaturation();
    auto reader = FFmpegUrlInputController::create(m_parser, TEST_MEDIA_URL, ZERO_OFFSET, repeat);
    ASSERT_NE(reader, nullptr);

    EXPECT_TRUE(reader->hasNext());
    EXPECT_CALL(*m_parser, initializeParsing(TEST_MEDIA_URL))
        .WillOnce(Invoke(m_parser.get(), &MockPlaylistParser::initializeParsingReal));
    EXPECT_CALL(*m_parser, next()).WillOnce(Invoke(m_parser.get(), &MockPlaylistParser::nextReal));
    EXPECT_TRUE(reader->next());
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
