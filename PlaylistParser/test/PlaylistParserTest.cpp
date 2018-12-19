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

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Memory/Memory.h>

#include "PlaylistParser/MockContentFetcher.h"
#include "PlaylistParser/PlaylistParser.h"

namespace alexaClientSDK {
namespace playlistParser {
namespace test {

using namespace avsCommon::utils::playlistParser;
using namespace ::testing;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::avs;

/// A mock factory that creates mock content fetchers
class MockContentFetcherFactory : public avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface {
    std::unique_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterface> create(const std::string& url) {
        return avsCommon::utils::memory::make_unique<MockContentFetcher>(url);
    }
};

/**
 * Mock AttachmentReader.
 */
class TestParserObserver : public avsCommon::utils::playlistParser::PlaylistParserObserverInterface {
public:
    /// A struct used for bookkeeping of parse results
    struct ParseResult {
        int requestId;
        std::string url;
        avsCommon::utils::playlistParser::PlaylistParseResult parseResult;
        std::chrono::milliseconds duration;
    };

    void onPlaylistEntryParsed(int requestId, PlaylistEntry entry) {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_parseResults.push_back({requestId, entry.url, entry.parseResult, entry.duration});
        m_callbackOccurred.notify_one();
    }

    /**
     * Waits for the PlaylistParserObserverInterface##onPlaylistEntryParsed() call N times.
     *
     * @param numCallbacksExpected The number of callbacks expected.
     * @param timeout The amount of time to wait for the calls.
     * @return The parse results that actually occurred.
     */
    std::vector<ParseResult> waitForNCallbacks(
        size_t numCallbacksExpected,
        std::chrono::milliseconds timeout = SHORT_TIMEOUT) {
        std::unique_lock<std::mutex> lock{m_mutex};
        m_callbackOccurred.wait_for(
            lock, timeout, [this, numCallbacksExpected]() { return m_parseResults.size() == numCallbacksExpected; });
        return m_parseResults;
    }

private:
    /// The detection results that have occurred.
    std::vector<ParseResult> m_parseResults;

    /// A mutex to guard against new callbacks.
    std::mutex m_mutex;

    /// A condition variable to wait for callbacks.
    std::condition_variable m_callbackOccurred;
};

class PlaylistParserTest : public ::testing::Test {
protected:
    void SetUp() {
        mockFactory = std::make_shared<MockContentFetcherFactory>();
        playlistParser = PlaylistParser::create(mockFactory);
        testObserver = std::make_shared<TestParserObserver>();
    }

    void TearDown() {
        playlistParser->shutdown();
    }

    /// A mock factory to create mock content fetchers
    std::shared_ptr<MockContentFetcherFactory> mockFactory;

    /// Instance of the @c PlaylistParser.
    std::shared_ptr<PlaylistParser> playlistParser;

    /// Instance of the @c TestParserObserver.
    std::shared_ptr<TestParserObserver> testObserver;
};

/**
 * Tests parsing of an empty playlist. Calls @c parsePlaylist and expects it returns false.
 */
TEST_F(PlaylistParserTest, testEmptyUrl) {
    ASSERT_FALSE(playlistParser->parsePlaylist("", testObserver));
}

/**
 * Tests passing a @c nullptr for the observer.
 */
TEST_F(PlaylistParserTest, testNullObserver) {
    ASSERT_FALSE(playlistParser->parsePlaylist("blah", nullptr));
}

/**
 * Tests parsing of a simple M3U playlist.
 * Calls @c parsePlaylist and expects that the result of the parsing is successful.
 */
TEST_F(PlaylistParserTest, testParsingPlaylist) {
    ASSERT_TRUE(playlistParser->parsePlaylist(TEST_M3U_PLAYLIST_URL, testObserver));
    auto results = testObserver->waitForNCallbacks(TEST_M3U_PLAYLIST_URL_EXPECTED_PARSES);
    ASSERT_EQ(TEST_M3U_PLAYLIST_URL_EXPECTED_PARSES, results.size());
    for (unsigned int i = 0; i < results.size(); ++i) {
        ASSERT_EQ(results.at(i).url, TEST_M3U_PLAYLIST_URLS.at(i));
        ASSERT_EQ(results.at(i).duration, TEST_M3U_DURATIONS.at(i));
        if (i == results.size() - 1) {
            ASSERT_EQ(results.at(i).parseResult, avsCommon::utils::playlistParser::PlaylistParseResult::FINISHED);
        } else {
            ASSERT_EQ(results.at(i).parseResult, avsCommon::utils::playlistParser::PlaylistParseResult::STILL_ONGOING);
        }
    }
}

/**
 * Tests parsing of a simple M3U playlist with relative urls.
 * Calls @c parsePlaylist and expects that the result of the parsing is successful.
 */
TEST_F(PlaylistParserTest, testParsingRelativePlaylist) {
    ASSERT_TRUE(playlistParser->parsePlaylist(TEST_M3U_RELATIVE_PLAYLIST_URL, testObserver));
    auto results = testObserver->waitForNCallbacks(TEST_M3U_RELATIVE_PLAYLIST_URL_EXPECTED_PARSES);
    ASSERT_EQ(TEST_M3U_RELATIVE_PLAYLIST_URL_EXPECTED_PARSES, results.size());
    for (unsigned int i = 0; i < results.size(); ++i) {
        ASSERT_EQ(results.at(i).url, TEST_M3U_RELATIVE_PLAYLIST_URLS.at(i));
        if (i == results.size() - 1) {
            ASSERT_EQ(results.at(i).parseResult, avsCommon::utils::playlistParser::PlaylistParseResult::FINISHED);
        } else {
            ASSERT_EQ(results.at(i).parseResult, avsCommon::utils::playlistParser::PlaylistParseResult::STILL_ONGOING);
        }
    }
}

/**
 * Tests parsing of an extended M3U/HLS playlist.
 * Calls @c parsePlaylist and expects that the result of the parsing is successful.
 */
TEST_F(PlaylistParserTest, testParsingHlsPlaylist) {
    ASSERT_TRUE(playlistParser->parsePlaylist(TEST_HLS_PLAYLIST_URL, testObserver));
    auto results = testObserver->waitForNCallbacks(TEST_HLS_PLAYLIST_URL_EXPECTED_PARSES);
    ASSERT_EQ(TEST_HLS_PLAYLIST_URL_EXPECTED_PARSES, results.size());
    for (unsigned int i = 0; i < results.size(); ++i) {
        ASSERT_EQ(results.at(i).url, TEST_HLS_PLAYLIST_URLS.at(i));
        ASSERT_EQ(results.at(i).duration, TEST_HLS_DURATIONS.at(i));
        if (i == results.size() - 1) {
            ASSERT_EQ(results.at(i).parseResult, avsCommon::utils::playlistParser::PlaylistParseResult::FINISHED);
        } else {
            ASSERT_EQ(results.at(i).parseResult, avsCommon::utils::playlistParser::PlaylistParseResult::STILL_ONGOING);
        }
    }
}

/**
 * Tests parsing of a PLS playlist.
 * Calls @c parsePlaylist and expects that the result of the parsing is successful.
 */
TEST_F(PlaylistParserTest, testParsingPlsPlaylist) {
    ASSERT_TRUE(playlistParser->parsePlaylist(TEST_PLS_PLAYLIST_URL, testObserver));
    auto results = testObserver->waitForNCallbacks(TEST_PLS_PLAYLIST_URL_EXPECTED_PARSES);
    ASSERT_EQ(TEST_PLS_PLAYLIST_URL_EXPECTED_PARSES, results.size());
    for (unsigned int i = 0; i < results.size(); ++i) {
        ASSERT_EQ(results.at(i).url, TEST_PLS_PLAYLIST_URLS.at(i));
        if (i == results.size() - 1) {
            ASSERT_EQ(results.at(i).parseResult, avsCommon::utils::playlistParser::PlaylistParseResult::FINISHED);
        } else {
            ASSERT_EQ(results.at(i).parseResult, avsCommon::utils::playlistParser::PlaylistParseResult::STILL_ONGOING);
        }
    }
}

/**
 * Tests that the playlist parser skips parsing of unwanted playlist types.
 * Calls @c parsePlaylist and expects that the result of the parsing is successful.
 */
TEST_F(PlaylistParserTest, testNotParsingCertainPlaylistTypes) {
    ASSERT_TRUE(
        playlistParser->parsePlaylist(TEST_HLS_PLAYLIST_URL, testObserver, {PlaylistParser::PlaylistType::EXT_M3U}));
    auto results = testObserver->waitForNCallbacks(1);
    ASSERT_EQ(NUM_PARSES_EXPECTED_WHEN_NO_PARSING, results.size());
    ASSERT_EQ(results.at(0).url, TEST_HLS_PLAYLIST_URL);
}

/**
 * Tests parsing of a live stream HLS playlist.
 * Calls @c parsePlaylist and expects that the result of the parsing is successful.
 */
TEST_F(PlaylistParserTest, testParsingLiveStreamPlaylist) {
    ASSERT_TRUE(playlistParser->parsePlaylist(TEST_HLS_LIVE_STREAM_PLAYLIST_URL, testObserver));
    auto results = testObserver->waitForNCallbacks(TEST_HLS_LIVE_STREAM_PLAYLIST_EXPECTED_PARSES);
    ASSERT_EQ(TEST_HLS_LIVE_STREAM_PLAYLIST_EXPECTED_PARSES, results.size());
    for (unsigned int i = 0; i < results.size(); ++i) {
        ASSERT_EQ(results.at(i).url, TEST_HLS_LIVE_STREAM_PLAYLIST_URLS.at(i));
        ASSERT_EQ(results.at(i).duration, TEST_HLS_LIVE_STREAM_DURATIONS.at(i));
        if (i == results.size() - 1) {
            ASSERT_EQ(results.at(i).parseResult, avsCommon::utils::playlistParser::PlaylistParseResult::FINISHED);
        } else {
            ASSERT_EQ(results.at(i).parseResult, avsCommon::utils::playlistParser::PlaylistParseResult::STILL_ONGOING);
        }
    }
}

}  // namespace test
}  // namespace playlistParser
}  // namespace alexaClientSDK
