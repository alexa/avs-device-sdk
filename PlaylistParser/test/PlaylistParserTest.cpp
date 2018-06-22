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

#include <memory>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <unordered_map>

#include <gmock/gmock.h>
#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterface.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include "PlaylistParser/PlaylistParser.h"

namespace alexaClientSDK {
namespace playlistParser {
namespace test {

using namespace avsCommon::utils::playlistParser;
using namespace ::testing;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::avs;

/// Short time out for when callbacks are expected not to occur.
static const auto SHORT_TIMEOUT = std::chrono::milliseconds(50);

/// Test M3U url.
static const std::string TEST_M3U_PLAYLIST_URL{"http://sanjayisthecoolest.com/sample.m3u"};

static const std::string TEST_M3U_PLAYLIST_CONTENT =
    "http://stream.radiotime.com/sample.mp3\n"
    "http://live-mp3-128.kexp.org\n";

static const size_t TEST_M3U_PLAYLIST_URL_EXPECTED_PARSES = 2;

static const std::vector<std::chrono::milliseconds> TEST_M3U_DURATIONS = {std::chrono::milliseconds{-1},
                                                                          std::chrono::milliseconds{-1},
                                                                          std::chrono::milliseconds{-1}};

static const std::vector<std::string> TEST_M3U_PLAYLIST_URLS = {"http://stream.radiotime.com/sample.mp3",
                                                                "http://live-mp3-128.kexp.org"};

/// Test M3U url with relative urls within.
static const std::string TEST_M3U_RELATIVE_PLAYLIST_URL{"http://sanjayisthecoolest.com/sampleRelativeUrls.m3u"};

static const std::string TEST_M3U_RELATIVE_PLAYLIST_CONTENT =
    "../test.mp3\n"
    "test2.aac\n";

static const size_t TEST_M3U_RELATIVE_PLAYLIST_URL_EXPECTED_PARSES = 2;

static const std::vector<std::string> TEST_M3U_RELATIVE_PLAYLIST_URLS = {"http://sanjayisthecoolest.com/../test.mp3",
                                                                         "http://sanjayisthecoolest.com/test2.aac"};

/// A test playlist in HLS format.
static const std::string TEST_HLS_PLAYLIST_URL{"http://sanjayisthecoolest.com/sample.m3u8"};

static const std::string TEST_HLS_PLAYLIST_CONTENT =
    "#EXTM3U\n"
    "#EXT-X-TARGETDURATION:10\n"
    "#EXT-X-MEDIA-SEQUENCE:9684358\n"
    "#EXTINF:10,RADIO\n"
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684358.aac\n"
    "#EXTINF:10.0,RADIO\n"
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684359.aac\n"
    "#EXTINF:10,RADIO\n"
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac\n"
    "#EXTINF:10.34,RADIO\n"
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac\n"
    "#EXTINF:10.344,RADIO\n"
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac\n"
    "#EXTINF:10.3444,RADIO\n"
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac\n"
    "#EXTINF:10.002,RADIO\n"
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac\n"
    "#EXTINF:10.0022,RADIO\n"
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac\n"
    "#EXTINF : 10.0022,RADIO\n"
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac\n"
    "#EXTINF : -1,RADIO\n"
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac\n"
    "#EXTINF:123ms,RADIO\n"
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac\n"
    "#EXTINF:123 ms,RADIO\n"
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac\n"
    "#EXTINF:123.0ms,RADIO\n"
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac\n"
    "#EXTINF:123ms,\n"
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac\n"
    "#EXTINF:123 ,hi\n"
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac\n"
    "#EXT-X-ENDLIST\n";

static const size_t TEST_HLS_PLAYLIST_URL_EXPECTED_PARSES = 15;

static const std::vector<std::string> TEST_HLS_PLAYLIST_URLS = {
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684358.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684359.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac"};

static const std::vector<std::chrono::milliseconds> TEST_HLS_DURATIONS = {std::chrono::milliseconds{10000},
                                                                          std::chrono::milliseconds{10000},
                                                                          std::chrono::milliseconds{10000},
                                                                          std::chrono::milliseconds{10340},
                                                                          std::chrono::milliseconds{10344},
                                                                          std::chrono::milliseconds{10344},
                                                                          std::chrono::milliseconds{10002},
                                                                          std::chrono::milliseconds{10002},
                                                                          std::chrono::milliseconds{10002},
                                                                          std::chrono::milliseconds{-1},
                                                                          std::chrono::milliseconds{-1},
                                                                          std::chrono::milliseconds{-1},
                                                                          std::chrono::milliseconds{-1},
                                                                          std::chrono::milliseconds{-1},
                                                                          std::chrono::milliseconds{123000}};

/// A test playlist in PLS format.
static const std::string TEST_PLS_PLAYLIST_URL{"http://sanjayisthecoolest.com/sample3.pls"};

static const std::string TEST_PLS_CONTENT =
    "[playlist]\n"
    "NumberOfEntries=2\n"
    "File1=http://stream.radiotime.com/sample.mp3\n"
    "Length1=-1\n"
    "File2=http://live-mp3-128.kexp.org\n"
    "Length2=-1\n";

static const size_t TEST_PLS_PLAYLIST_URL_EXPECTED_PARSES = 2;

static const std::vector<std::string> TEST_PLS_PLAYLIST_URLS = {"http://stream.radiotime.com/sample.mp3",
                                                                "http://live-mp3-128.kexp.org"};

static const std::string TEST_HLS_RECURSIVE_PLAYLIST_URL{"recursiveSample.m3u8"};

static const std::string TEST_HLS_RECURSIVE_PLAYLIST_CONTENT = TEST_HLS_PLAYLIST_CONTENT + TEST_M3U_PLAYLIST_URL;

static const size_t TEST_HLS_RECURSIVE_PLAYLIST_URL_EXPECTED_PARSES =
    TEST_HLS_PLAYLIST_URL_EXPECTED_PARSES + TEST_M3U_PLAYLIST_URL_EXPECTED_PARSES;

static const std::vector<std::string> TEST_HLS_RECURSIVE_PLAYLIST_URLS = {
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684358.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684359.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://stream.radiotime.com/sample.mp3",
    "http://live-mp3-128.kexp.org"};

/// A test playlist in HLS format.
static const std::string TEST_HLS_LIVE_STREAM_PLAYLIST_URL{"http://sanjayisthecoolest.com/liveStream.m3u8"};

static const std::string TEST_HLS_LIVE_STREAM_PLAYLIST_CONTENT_1 =
    "#EXTM3U\n"
    "#EXT-X-TARGETDURATION:10\n"
    "#EXT-X-MEDIA-SEQUENCE:9684358\n"
    "#EXTINF:10,RADIO\n"
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684358.aac\n"
    "#EXTINF:10.0,RADIO\n"
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684359.aac\n"
    "#EXTINF:10,RADIO\n"
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac\n";

static const std::string TEST_HLS_LIVE_STREAM_PLAYLIST_CONTENT_2 =
    "#EXTM3U\n"
    "#EXT-X-TARGETDURATION:10\n"
    "#EXT-X-MEDIA-SEQUENCE:9684360\n"
    "#EXTINF:10,RADIO\n"
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac\n"
    "#EXTINF:10,RADIO\n"
    "http://sanjay.com/chunk.mp3\n"
    "#EXTINF:10,RADIO\n"
    "http://sanjay.com/anotherChunk.mp3\n"
    "#EXT-X-ENDLIST\n";

static const size_t TEST_HLS_LIVE_STREAM_PLAYLIST_EXPECTED_PARSES = 5;

static const std::vector<std::string> TEST_HLS_LIVE_STREAM_PLAYLIST_URLS = {
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684358.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684359.aac",
    "http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac",
    "http://sanjay.com/chunk.mp3",
    "http://sanjay.com/anotherChunk.mp3"};

static const std::vector<std::chrono::milliseconds> TEST_HLS_LIVE_STREAM_DURATIONS = {std::chrono::milliseconds{10000},
                                                                                      std::chrono::milliseconds{10000},
                                                                                      std::chrono::milliseconds{10000},
                                                                                      std::chrono::milliseconds{10000},
                                                                                      std::chrono::milliseconds{10000}};

static const size_t NUM_PARSES_EXPECTED_WHEN_NO_PARSING = 1;

static const std::unordered_map<std::string, std::string> urlsToContentTypes{
    // Valid playlist content types
    {TEST_M3U_PLAYLIST_URL, "audio/mpegurl"},
    {TEST_M3U_RELATIVE_PLAYLIST_URL, "audio/mpegurl"},
    {TEST_HLS_PLAYLIST_URL, "application/vnd.apple.mpegurl"},
    {TEST_PLS_PLAYLIST_URL, "audio/x-scpls"},
    {TEST_HLS_RECURSIVE_PLAYLIST_URL, "audio/mpegurl"},
    {TEST_HLS_LIVE_STREAM_PLAYLIST_URL, "audio/mpegurl"},
    // Not playlist content types
    {"http://stream.radiotime.com/sample.mp3", "audio/mpeg"},
    {"http://live-mp3-128.kexp.org", "audio/mpeg"},
    {"http://76.74.255.139/bismarck/live/bismarck.mov_9684358.aac", "audio/mpeg"},
    {"http://76.74.255.139/bismarck/live/bismarck.mov_9684359.aac", "audio/mpeg"},
    {"http://76.74.255.139/bismarck/live/bismarck.mov_9684360.aac", "audio/mpeg"},
    {"http://stream.radiotime.com/sample.mp3", "audio/mpeg"},
    {"http://live-mp3-128.kexp.org", "audio/mpeg"},
    {"http://sanjayisthecoolest.com/../test.mp3", "audio/mpeg"},
    {"http://sanjayisthecoolest.com/test2.aac", "audio/mpeg"},
    {"http://sanjay.com/chunk.mp3", "audio/mpeg"},
    {"http://sanjay.com/anotherChunk.mp3", "audio/mpeg"}};

static std::unordered_map<std::string, std::string> urlsToContent{
    {TEST_M3U_PLAYLIST_URL, TEST_M3U_PLAYLIST_CONTENT},
    {TEST_M3U_RELATIVE_PLAYLIST_URL, TEST_M3U_RELATIVE_PLAYLIST_CONTENT},
    {TEST_HLS_PLAYLIST_URL, TEST_HLS_PLAYLIST_CONTENT},
    {TEST_PLS_PLAYLIST_URL, TEST_PLS_CONTENT},
    {TEST_HLS_RECURSIVE_PLAYLIST_URL, TEST_HLS_RECURSIVE_PLAYLIST_CONTENT},
    {TEST_HLS_LIVE_STREAM_PLAYLIST_URL, TEST_HLS_LIVE_STREAM_PLAYLIST_CONTENT_1}};

/// A mock content fetcher
class MockContentFetcher : public avsCommon::sdkInterfaces::HTTPContentFetcherInterface {
public:
    MockContentFetcher(const std::string& url) : m_url{url} {
    }

    std::unique_ptr<avsCommon::utils::HTTPContent> getContent(
        FetchOptions fetchOption,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentWriter> writer) {
        if (fetchOption == FetchOptions::CONTENT_TYPE) {
            auto it1 = urlsToContentTypes.find(m_url);
            if (it1 == urlsToContentTypes.end()) {
                return nullptr;
            } else {
                std::promise<long> statusPromise;
                auto statusFuture = statusPromise.get_future();
                statusPromise.set_value(200);
                std::promise<std::string> contentTypePromise;
                auto contentTypeFuture = contentTypePromise.get_future();
                contentTypePromise.set_value(it1->second);
                return avsCommon::utils::memory::make_unique<avsCommon::utils::HTTPContent>(
                    avsCommon::utils::HTTPContent{std::move(statusFuture), std::move(contentTypeFuture), nullptr});
            }
        } else if (fetchOption == FetchOptions::ENTIRE_BODY) {
            auto it2 = urlsToContent.find(m_url);
            if (it2 == urlsToContent.end()) {
                return nullptr;
            } else {
                static bool liveStreamPlaylistRequested = false;
                if (m_url == TEST_HLS_LIVE_STREAM_PLAYLIST_URL) {
                    if (!liveStreamPlaylistRequested) {
                        it2->second = TEST_HLS_LIVE_STREAM_PLAYLIST_CONTENT_1;
                        liveStreamPlaylistRequested = true;
                    } else {
                        it2->second = TEST_HLS_LIVE_STREAM_PLAYLIST_CONTENT_2;
                    }
                }
                std::promise<long> statusPromise;
                auto statusFuture = statusPromise.get_future();
                statusPromise.set_value(200);
                std::promise<std::string> contentTypePromise;
                auto contentTypeFuture = contentTypePromise.get_future();
                contentTypePromise.set_value("");
                return avsCommon::utils::memory::make_unique<avsCommon::utils::HTTPContent>(
                    avsCommon::utils::HTTPContent{
                        std::move(statusFuture), std::move(contentTypeFuture), writeStringIntoAttachment(it2->second)});
            }
        } else {
            return nullptr;
        }
    }

private:
    std::shared_ptr<avsCommon::avs::attachment::InProcessAttachment> writeStringIntoAttachment(
        const std::string& string) {
        static int id = 0;
        std::shared_ptr<avsCommon::avs::attachment::InProcessAttachment> stream =
            std::make_shared<avsCommon::avs::attachment::InProcessAttachment>(std::to_string(id++));
        if (!stream) {
            return nullptr;
        }
        auto writer = stream->createWriter();
        if (!writer) {
            return nullptr;
        }
        avsCommon::avs::attachment::AttachmentWriter::WriteStatus writeStatus;
        writer->write(string.data(), string.size(), &writeStatus);
        return stream;
    };

    std::string m_url;
};

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

    void onPlaylistEntryParsed(
        int requestId,
        std::string url,
        avsCommon::utils::playlistParser::PlaylistParseResult parseResult,
        std::chrono::milliseconds duration) {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_parseResults.push_back({requestId, url, parseResult, duration});
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
            ASSERT_EQ(results.at(i).parseResult, avsCommon::utils::playlistParser::PlaylistParseResult::SUCCESS);
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
            ASSERT_EQ(results.at(i).parseResult, avsCommon::utils::playlistParser::PlaylistParseResult::SUCCESS);
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
            ASSERT_EQ(results.at(i).parseResult, avsCommon::utils::playlistParser::PlaylistParseResult::SUCCESS);
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
            ASSERT_EQ(results.at(i).parseResult, avsCommon::utils::playlistParser::PlaylistParseResult::SUCCESS);
        } else {
            ASSERT_EQ(results.at(i).parseResult, avsCommon::utils::playlistParser::PlaylistParseResult::STILL_ONGOING);
        }
    }
}

/**
 * Tests parsing of a recursive M3U/HLS playlist.
 * Calls @c parsePlaylist and expects that the result of the parsing is successful.
 */
TEST_F(PlaylistParserTest, testParsingRecursiveHlsPlaylist) {
    ASSERT_TRUE(playlistParser->parsePlaylist(TEST_HLS_RECURSIVE_PLAYLIST_URL, testObserver));
    auto results = testObserver->waitForNCallbacks(TEST_HLS_RECURSIVE_PLAYLIST_URL_EXPECTED_PARSES);
    ASSERT_EQ(TEST_HLS_RECURSIVE_PLAYLIST_URL_EXPECTED_PARSES, results.size());
    for (unsigned int i = 0; i < results.size(); ++i) {
        ASSERT_EQ(results.at(i).url, TEST_HLS_RECURSIVE_PLAYLIST_URLS.at(i));
        if (i == results.size() - 1) {
            ASSERT_EQ(results.at(i).parseResult, avsCommon::utils::playlistParser::PlaylistParseResult::SUCCESS);
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
        playlistParser->parsePlaylist(TEST_HLS_PLAYLIST_URL, testObserver, {PlaylistParser::PlaylistType::M3U8}));
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
            ASSERT_EQ(results.at(i).parseResult, avsCommon::utils::playlistParser::PlaylistParseResult::SUCCESS);
        } else {
            ASSERT_EQ(results.at(i).parseResult, avsCommon::utils::playlistParser::PlaylistParseResult::STILL_ONGOING);
        }
    }
}

}  // namespace test
}  // namespace playlistParser
}  // namespace alexaClientSDK
