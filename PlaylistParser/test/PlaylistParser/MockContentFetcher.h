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
#ifndef ALEXA_CLIENT_SDK_PLAYLISTPARSER_TEST_PLAYLISTPARSER_MOCKCONTENTFETCHER_H_
#define ALEXA_CLIENT_SDK_PLAYLISTPARSER_TEST_PLAYLISTPARSER_MOCKCONTENTFETCHER_H_

#include <string>
#include <vector>
#include <unordered_map>

#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterface.h>

namespace alexaClientSDK {
namespace playlistParser {
namespace test {

using namespace avsCommon::sdkInterfaces;

/// Short time out for when callbacks are expected not to occur.
static const auto SHORT_TIMEOUT = std::chrono::milliseconds(100);

/// Test M3U url.
static const std::string TEST_M3U_PLAYLIST_URL{"http://sanjayisthecoolest.com/sample.m3u"};

static const std::string TEST_M3U_PLAYLIST_CONTENT =
    "http://stream.radiotime.com/sample.mp3\n"
    "http://live-mp3-128.kexp.org\n";

static const size_t TEST_M3U_PLAYLIST_URL_EXPECTED_PARSES = 2;

static const std::vector<std::chrono::milliseconds> TEST_M3U_DURATIONS = {std::chrono::milliseconds{-1},
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

static const std::string TEST_MEDIA_URL = "https://sanjay.com/some_media.mp3";

static const std::unordered_map<std::string, std::string> urlsToContentTypes{
    // Valid playlist content types
    {TEST_M3U_PLAYLIST_URL, "audio/mpegurl"},
    {TEST_M3U_RELATIVE_PLAYLIST_URL, "audio/mpegurl"},
    {TEST_HLS_PLAYLIST_URL, "application/vnd.apple.mpegurl"},
    {TEST_PLS_PLAYLIST_URL, "audio/x-scpls"},
    {TEST_HLS_LIVE_STREAM_PLAYLIST_URL, "audio/mpegurl"},
    // Not playlist content types
    {TEST_MEDIA_URL, "audio/mpeg"},
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
    {TEST_HLS_LIVE_STREAM_PLAYLIST_URL, TEST_HLS_LIVE_STREAM_PLAYLIST_CONTENT_1}};

/// A mock content fetcher
class MockContentFetcher : public HTTPContentFetcherInterface {
public:
    MockContentFetcher(const std::string& url) : m_url{url}, m_state{HTTPContentFetcherInterface::State::INITIALIZED} {
    }

    std::string getUrl() const override {
        return m_url;
    }

    HTTPContentFetcherInterface::Header getHeader(std::atomic<bool>* shouldShutdown) override {
        HTTPContentFetcherInterface::Header header;
        auto it1 = urlsToContentTypes.find(m_url);
        if (it1 == urlsToContentTypes.end()) {
            header.successful = false;
        } else {
            header.successful = true;
            header.responseCode = avsCommon::utils::http::HTTPResponseCode::SUCCESS_OK;
            header.contentType = it1->second;
            m_state = HTTPContentFetcherInterface::State::HEADER_DONE;
        }
        return header;
    }

    HTTPContentFetcherInterface::State getState() override {
        return m_state;
    }

    bool getBody(std::shared_ptr<avsCommon::avs::attachment::AttachmentWriter> writer) override {
        auto it2 = urlsToContent.find(m_url);
        if (it2 == urlsToContent.end()) {
            return false;
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
            auto attachment = writeStringIntoAttachment(it2->second, std::move(writer));
            if (!attachment) {
                return false;
            }
            m_state = HTTPContentFetcherInterface::State::BODY_DONE;
        }
        return true;
    }

    void shutdown() override {
    }

    std::unique_ptr<avsCommon::utils::HTTPContent> getContent(
        FetchOptions fetchOption,
        std::unique_ptr<avsCommon::avs::attachment::AttachmentWriter> writer,
        const std::vector<std::string>& customHeaders = std::vector<std::string>()) override {
        return nullptr;
    }

private:
    std::shared_ptr<avsCommon::avs::attachment::InProcessAttachment> writeStringIntoAttachment(
        const std::string& string,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentWriter> writer) {
        static int id = 0;
        std::shared_ptr<avsCommon::avs::attachment::InProcessAttachment> stream =
            std::make_shared<avsCommon::avs::attachment::InProcessAttachment>(std::to_string(id++));
        if (!stream) {
            return nullptr;
        }

        if (!writer) {
            writer = stream->createWriter();
        }
        avsCommon::avs::attachment::AttachmentWriter::WriteStatus writeStatus;
        writer->write(string.data(), string.size(), &writeStatus);
        return stream;
    };

    std::string m_url;

    HTTPContentFetcherInterface::State m_state;
};

}  // namespace test
}  // namespace playlistParser
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_PLAYLISTPARSER_TEST_PLAYLISTPARSER_MOCKCONTENTFETCHER_H_
