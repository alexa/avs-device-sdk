/*
 * PlaylistParserTest.cpp
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <gmock/gmock.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include "PlaylistParser/PlaylistParser.h"

namespace alexaClientSDK {
namespace playlistParser {
namespace test {

using namespace avsCommon::utils::playlistParser;
using namespace ::testing;

/// String to identify log entries originating from this file.
static const std::string TAG("PlaylistParserTest");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The path to the input Dir containing the test audio files.
std::string inputsDirPath;

/// The prefix for the file URI scheme.
static const std::string FILE_URI_PREFIX("file://");

/// Test playlist url.
static const std::string TEST_PLAYLIST{"/sample2.m3u"};

/// A test playlist url. One of the links on this playlist redirects to another playlist.
static const std::string TEST_ASX_PLAYLIST{"/sample1.asx"};

/// A test playlist in PLS format.
static const std::string TEST_PLS_PLAYLIST{"/sample3.pls"};

/// A test playlist in HLS format.
static const std::string TEST_HLS_PLAYLIST{"/sample.m3u8"};

/**
 * Mock AttachmentReader.
 */
class TestParserObserver : public avsCommon::utils::playlistParser::PlaylistParserObserverInterface {
public:
    /**
     * Creates an instance of the @c TestParserObserver.
     * @return An instance of the @c TestParserObserver.
     */
    static std::shared_ptr<TestParserObserver> create();

    /// Destructor
    ~TestParserObserver() = default;

    void onPlaylistParsed(std::string playlistUrl, std::queue<std::string> urls,
            avsCommon::utils::playlistParser::PlaylistParseResult parseResult) override;

    /**
     * Waits for playlist parsing to complete.
     *
     * @param expectedResult The expected result from parsing the playlist.
     * @param duration The duration to wait for playlist parsing to complete.
     * @return The result of parsing the playlist.
     */
    bool waitForPlaylistParsed(const PlaylistParseResult expectedResult,
            const std::chrono::seconds duration = std::chrono::seconds(5));

    /**
     * Constructor.
     */
    TestParserObserver();

    /// Mutex to serialize access to @c m_playlistParsed and @c m_parseResult.
    std::mutex m_mutex;

    /// Condition Variable to wake @c waitForPlaylistParsed
    std::condition_variable m_wakePlaylistParsed;

    /// Flag to indicate when playlist has been parsed.
    bool m_playlistParsed;

    /// The result of parsing the playlist.
    PlaylistParseResult m_parseResult;

    /// The initial playlist Url.
    std::string m_initialUrl;

    /// Urls extracted from the playlist.
    std::queue<std::string> m_urls;
};

std::shared_ptr<TestParserObserver> TestParserObserver::create() {
    std::shared_ptr<TestParserObserver> playlistObserver(new TestParserObserver);
    return playlistObserver;
}

void TestParserObserver::onPlaylistParsed(std::string playlistUrl, std::queue<std::string> urls,
            avsCommon::utils::playlistParser::PlaylistParseResult parseResult) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_playlistParsed = true;
    m_parseResult = parseResult;
    m_urls = urls;
    m_initialUrl = playlistUrl;
    m_wakePlaylistParsed.notify_one();
}

bool TestParserObserver::waitForPlaylistParsed(const PlaylistParseResult expectedResult,
            const std::chrono::seconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakePlaylistParsed.wait_for(lock, duration, [this]() { return m_playlistParsed; } ))
    {
        return false;
    }
    return (expectedResult == m_parseResult);
}

TestParserObserver::TestParserObserver()
        :
        m_playlistParsed{false},
        m_parseResult{PlaylistParseResult::PARSE_RESULT_ERROR} {
}

class PlaylistParserTest: public ::testing::Test{
public:
    void SetUp() override;

    void TearDown() override;

    /// Instance of the @c PlaylistParser.
    std::shared_ptr<PlaylistParser> m_playlistParser;

    /// Instance of the @c TestParserObserver.
    std::shared_ptr<TestParserObserver> m_parserObserver;

    /// The main event loop.
    GMainLoop* m_loop;

    /// The thread on which the main event loop is launched.
    std::thread m_mainLoopThread;

};

void PlaylistParserTest::SetUp() {
    m_loop = g_main_loop_new(nullptr, false);
    ASSERT_TRUE(m_loop);
    m_mainLoopThread = std::thread(g_main_loop_run, m_loop);
    m_parserObserver = TestParserObserver::create();
    m_playlistParser = PlaylistParser::create();
    ASSERT_TRUE(m_playlistParser);
}

void PlaylistParserTest::TearDown() {
    while (!g_main_loop_is_running(m_loop)){
        std::this_thread::yield();
    }
    g_main_loop_quit(m_loop);
    if (m_mainLoopThread.joinable()) {
        m_mainLoopThread.join();
    }
    g_main_loop_unref(m_loop);
}

/**
 * Tests parsing of an empty playlist. Calls @c parsePlaylist and expects it returns false.
 */
TEST_F(PlaylistParserTest, testEmptyUrl) {
    ASSERT_FALSE(m_playlistParser->parsePlaylist("", m_parserObserver));
}

/**
 * Tests passing a @c nullptr for the observer.
 */
TEST_F(PlaylistParserTest, testNullObserver) {
    ASSERT_FALSE(m_playlistParser->parsePlaylist(FILE_URI_PREFIX + inputsDirPath + TEST_PLAYLIST, nullptr));
}

/**
 * Tests parsing of a single playlist.
 * Calls @c parsePlaylist and expects that the result of the parsing is successful.
 */
TEST_F(PlaylistParserTest, testParsingPlaylist) {
    ASSERT_TRUE(m_playlistParser->parsePlaylist(FILE_URI_PREFIX + inputsDirPath + TEST_PLAYLIST, m_parserObserver));
    ASSERT_TRUE(m_parserObserver->waitForPlaylistParsed(PlaylistParseResult::PARSE_RESULT_SUCCESS));
}

/**
 * Tests parsing of a PLS playlist.
 * Calls @c parsePlaylist and expects that the result of the parsing is successful.
 */
TEST_F(PlaylistParserTest, testParsingPlsPlaylist) {
    ASSERT_TRUE(m_playlistParser->parsePlaylist(FILE_URI_PREFIX + inputsDirPath + TEST_PLS_PLAYLIST, m_parserObserver));
    ASSERT_TRUE(m_parserObserver->waitForPlaylistParsed(PlaylistParseResult::PARSE_RESULT_SUCCESS));
}

/**
 * Tests parsing of multiple playlists one of which is a recursive playlist.
 * Calls @c parsePlaylist on the different playlists and expects each of them to be parsed successfully.
 */
TEST_F(PlaylistParserTest, testParsingMultiplePlaylists) {
    auto m_parserObserver2 = TestParserObserver::create();
    ASSERT_TRUE(m_playlistParser->parsePlaylist(FILE_URI_PREFIX + inputsDirPath + TEST_PLAYLIST, m_parserObserver));
    ASSERT_TRUE(m_playlistParser->parsePlaylist(FILE_URI_PREFIX + inputsDirPath + TEST_ASX_PLAYLIST, m_parserObserver2));
    ASSERT_TRUE(m_parserObserver->waitForPlaylistParsed((PlaylistParseResult::PARSE_RESULT_SUCCESS)));
    ASSERT_TRUE(m_parserObserver2->waitForPlaylistParsed((PlaylistParseResult::PARSE_RESULT_SUCCESS)));
}

/**
 * Tests parsing of a single playlist which is a format that cannot be handled.
 * Calls @c parsePlaylist and expects that the result of the parsing is an error.
 */
TEST_F(PlaylistParserTest, testUnparsablePlaylist) {
    ASSERT_TRUE(m_playlistParser->parsePlaylist(FILE_URI_PREFIX + inputsDirPath + TEST_HLS_PLAYLIST, m_parserObserver));
    ASSERT_FALSE(m_parserObserver->waitForPlaylistParsed((PlaylistParseResult::PARSE_RESULT_SUCCESS)));
}

} // namespace test
} // namespace playlistParser
} // namespace alexaClientSDK

int main (int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    if (argc < 2) {
        std::cerr << "Usage: PlaylistParserTest <path/to/test/inputs/folder>" << std::endl;
    } else {
        alexaClientSDK::playlistParser::test::inputsDirPath = std::string(argv[1]);
        return RUN_ALL_TESTS();
    }
}
