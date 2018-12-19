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

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/Utils/Memory/Memory.h>

#include "PlaylistParser/IterativePlaylistParser.h"
#include "PlaylistParser/MockContentFetcher.h"

namespace alexaClientSDK {
namespace playlistParser {
namespace test {

using namespace ::testing;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::playlistParser;

/// A mock factory that creates mock content fetchers
class MockContentFetcherFactory : public avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface {
public:
    MOCK_METHOD1(
        create,
        std::unique_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterface>(const std::string& url));
};  // namespace test

class IterativePlaylistParserTest : public ::testing::Test {
protected:
    /// Configure test instance.
    void SetUp();

    /// Checks that a url is correctly parsed.
    void testPlaylist(
        const std::string& url,
        const std::vector<std::string>& playlistUrls,
        const std::vector<std::chrono::milliseconds>& durations);

    /// Checks that a url is correctly parsed but ignore durations.
    void testPlaylist(const std::string& url, const std::vector<std::string>& playlistUrls);

    /// A mock factory to create mock content fetchers
    std::shared_ptr<MockContentFetcherFactory> m_mockFactory;

    /// Instance of the @c IterativePlaylistParser.
    std::shared_ptr<IterativePlaylistParser> m_parser;
};

void IterativePlaylistParserTest::SetUp() {
    m_mockFactory = std::make_shared<MockContentFetcherFactory>();
    EXPECT_CALL(*m_mockFactory, create(_)).WillRepeatedly(Invoke([](const std::string& url) {
        return avsCommon::utils::memory::make_unique<MockContentFetcher>(url);
    }));
    m_parser = IterativePlaylistParser::create(m_mockFactory);
}

void IterativePlaylistParserTest::testPlaylist(
    const std::string& url,
    const std::vector<std::string>& playlistUrls,
    const std::vector<std::chrono::milliseconds>& durations) {
    ASSERT_EQ(playlistUrls.size(), durations.size());
    EXPECT_TRUE(m_parser->initializeParsing(url));

    size_t i;
    for (i = 0; i < playlistUrls.size() - 1; ++i) {
        auto entry = m_parser->next();
        EXPECT_EQ(entry.parseResult, PlaylistParseResult::STILL_ONGOING);
        EXPECT_EQ(entry.url, playlistUrls[i]);
        EXPECT_EQ(entry.duration, durations[i]);
    }

    auto entry = m_parser->next();
    EXPECT_EQ(entry.parseResult, PlaylistParseResult::FINISHED);
    EXPECT_EQ(entry.url, playlistUrls[i]);
    EXPECT_EQ(entry.duration, durations[i]);
}

void IterativePlaylistParserTest::testPlaylist(const std::string& url, const std::vector<std::string>& playlistUrls) {
    EXPECT_TRUE(m_parser->initializeParsing(url));

    size_t i;
    for (i = 0; i < playlistUrls.size() - 1; ++i) {
        auto entry = m_parser->next();
        EXPECT_EQ(entry.parseResult, PlaylistParseResult::STILL_ONGOING);
        EXPECT_EQ(entry.url, playlistUrls[i]);
    }

    auto entry = m_parser->next();
    EXPECT_EQ(entry.parseResult, PlaylistParseResult::FINISHED);
    EXPECT_EQ(entry.url, playlistUrls[i]);
}

/**
 * Tests initialize failure due to an empty playlist url.
 */
TEST_F(IterativePlaylistParserTest, testInitializeFailed) {
    EXPECT_FALSE(m_parser->initializeParsing(""));
}

/**
 * Tests successful initialization with non-empty url.
 */
TEST_F(IterativePlaylistParserTest, testInitializeOk) {
    EXPECT_TRUE(m_parser->initializeParsing(TEST_M3U_PLAYLIST_URL));
}

/**
 * Tests parsing of a simple M3U playlist.
 */
TEST_F(IterativePlaylistParserTest, testParsingPlaylist) {
    testPlaylist(TEST_M3U_PLAYLIST_URL, TEST_M3U_PLAYLIST_URLS, TEST_M3U_DURATIONS);
}

/**
 * Tests parsing of an extended M3U/HLS playlist.
 */
TEST_F(IterativePlaylistParserTest, testParsingHlsPlaylist) {
    testPlaylist(TEST_HLS_PLAYLIST_URL, TEST_HLS_PLAYLIST_URLS, TEST_HLS_DURATIONS);
}

/**
 * Tests parsing of a PLS playlist.
 */
TEST_F(IterativePlaylistParserTest, testParsingPlsPlaylist) {
    testPlaylist(TEST_PLS_PLAYLIST_URL, TEST_PLS_PLAYLIST_URLS);
}

/**
 * Tests parsing of a simple M3U playlist with relative urls.
 */
TEST_F(IterativePlaylistParserTest, testParsingRelativePlaylist) {
    testPlaylist(TEST_M3U_RELATIVE_PLAYLIST_URL, TEST_M3U_RELATIVE_PLAYLIST_URLS);
}

/**
 * Tests parsing of a live stream HLS playlist.
 */
TEST_F(IterativePlaylistParserTest, testParsingLiveStreamPlaylist) {
    testPlaylist(TEST_HLS_LIVE_STREAM_PLAYLIST_URL, TEST_HLS_LIVE_STREAM_PLAYLIST_URLS, TEST_HLS_LIVE_STREAM_DURATIONS);
}

/**
 * Test parsing a media url. We expect the media to be the unique url.
 */
TEST_F(IterativePlaylistParserTest, testParseMediaUrl) {
    EXPECT_TRUE(m_parser->initializeParsing(TEST_MEDIA_URL));

    auto entry = m_parser->next();
    EXPECT_EQ(entry.parseResult, PlaylistParseResult::FINISHED);
    EXPECT_EQ(entry.url, TEST_MEDIA_URL);
}

/**
 * Test parsing a invalid url.
 */
TEST_F(IterativePlaylistParserTest, testParseInvalidUrl) {
    const std::string invalidUrl = "http://invalid.url";
    EXPECT_TRUE(m_parser->initializeParsing(invalidUrl));

    auto entry = m_parser->next();
    EXPECT_EQ(entry.parseResult, PlaylistParseResult::ERROR);
}

/**
 * Test calling @c next() after abort parsing.
 */
TEST_F(IterativePlaylistParserTest, testNextFailsAfterAbort) {
    EXPECT_TRUE(m_parser->initializeParsing(TEST_M3U_PLAYLIST_URL));
    m_parser->abort();

    auto entry = m_parser->next();
    auto expectedDuration = std::chrono::milliseconds(-1);
    EXPECT_EQ(entry.parseResult, PlaylistParseResult::ERROR);
    EXPECT_EQ(entry.url, "");
    EXPECT_EQ(entry.duration, expectedDuration);
}

}  // namespace test
}  // namespace playlistParser
}  // namespace alexaClientSDK
