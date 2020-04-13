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

#include <string>

#include <gtest/gtest.h>

#include "PlaylistParser/M3UParser.h"

namespace alexaClientSDK {
namespace playlistParser {
namespace test {

using namespace avsCommon::utils::playlistParser;
using namespace alexaClientSDK::playlistParser;
using namespace ::testing;

/// Default encryption info (EncryptionInfo::Method::NONE).
static const auto noEncryption = EncryptionInfo();

/// Default byte range.
static const ByteRange defaultByteRange{0, 0};

/// Header for HLS playlist.
static const std::string EXT_M3U_HEADER = "#EXTM3U\n";

/// Test media URL.
static const std::string MEDIA_URL = "https://www.amazon.com/audio.aac";

/// Test HLS playlist URL.
static const std::string PLAYLIST_URL = "https://www.amazon.com/playlist.m3u8";

/// Test master playlist URL.
static const std::string MASTER_PLAYLIST_URL = "https://www.amazon.com/master-playlist.m3u8";

/**
 * Helper method to assert that encryption info match.
 *
 * @param lhs Expected encryption info.
 * @param rhs Actual encryption info.
 */
void matchEncryptionInfo(const EncryptionInfo& lhs, const EncryptionInfo& rhs) {
    EXPECT_EQ(lhs.method, rhs.method);
    EXPECT_EQ(lhs.keyURL, rhs.keyURL);
    EXPECT_EQ(lhs.initVector, rhs.initVector);
}

TEST(M3UParserTest, test_parseKeyNoMethod) {
    std::string line = "#EXT-X-KEY:";

    auto result = parseHLSEncryptionLine(line, "");

    matchEncryptionInfo(noEncryption, result);
}

TEST(M3UParserTest, test_parseKeyMethodNone) {
    std::string line = "#EXT-X-KEY:METHOD=NONE";

    auto result = parseHLSEncryptionLine(line, "");

    matchEncryptionInfo(noEncryption, result);
}

TEST(M3UParserTest, test_parseKeyMissingKeyURL) {
    std::string line = "#EXT-X-KEY:METHOD=AES-128";

    auto result = parseHLSEncryptionLine(line, "");

    matchEncryptionInfo(noEncryption, result);
}

TEST(M3UParserTest, test_parseKeyUnknownMethod) {
    std::string line = "#EXT-X-KEY:METHOD=UNKNOWN";

    auto result = parseHLSEncryptionLine(line, "");

    matchEncryptionInfo(noEncryption, result);
}

TEST(M3UParserTest, test_parseKeyURLClosingQuote) {
    std::string line = "#EXT-X-KEY:METHOD=SAMPLE-AES,URI=\"https://www.amazon.com";

    auto result = parseHLSEncryptionLine(line, "");

    matchEncryptionInfo(noEncryption, result);
}

TEST(M3UParserTest, test_parseKeyValidURL) {
    std::string line = "#EXT-X-KEY:METHOD=SAMPLE-AES,URI=\"https://www.amazon.com\"";

    auto result = parseHLSEncryptionLine(line, "");

    EncryptionInfo expected{EncryptionInfo::Method::SAMPLE_AES, "https://www.amazon.com", ""};
    matchEncryptionInfo(expected, result);
}

TEST(M3UParserTest, test_parseKeyValidInitVector) {
    std::string line =
        "#EXT-X-KEY:METHOD=SAMPLE-AES,IV=0x8e2d35559338d21f2586e79d6cd5c606,URI=\"https://www.amazon.com\"";

    auto result = parseHLSEncryptionLine(line, "");

    EncryptionInfo expected{
        EncryptionInfo::Method::SAMPLE_AES, "https://www.amazon.com", "0x8e2d35559338d21f2586e79d6cd5c606"};
    matchEncryptionInfo(expected, result);
}

TEST(M3UParserTest, test_parseMediaSequence) {
    std::string line = "#EXT-X-MEDIA-SEQUENCE: 12345";

    auto result = parsePlaylistMediaSequence(line);

    EXPECT_EQ(12345, result);
}

TEST(M3UParserTest, test_parseMediaSequenceNoSpace) {
    std::string line = "#EXT-X-MEDIA-SEQUENCE:12345";

    auto result = parsePlaylistMediaSequence(line);

    EXPECT_EQ(12345, result);
}

TEST(M3UParserTest, test_parseEmptyMediaSequence) {
    std::string line = "#EXT-X-MEDIA-SEQUENCE:         ";

    auto result = parsePlaylistMediaSequence(line);

    EXPECT_EQ(INVALID_MEDIA_SEQUENCE, result);
}

TEST(M3UParserTest, test_parseManySpacesMediaSequence) {
    std::string line = "#EXT-X-MEDIA-SEQUENCE:         12345     ";

    auto result = parsePlaylistMediaSequence(line);

    EXPECT_EQ(12345, result);
}

TEST(M3UParserTest, test_parseInvalidMediaSequence) {
    std::string line = "#EXT-X-MEDIA-SEQUENCE: invalid";

    auto result = parsePlaylistMediaSequence(line);

    EXPECT_EQ(INVALID_MEDIA_SEQUENCE, result);
}

TEST(M3UParserTest, test_parseKeyEncryptionInfo) {
    std::string playlist = EXT_M3U_HEADER + "#EXT-X-KEY:METHOD=SAMPLE-AES,URI=\"https://www.amazon.com\"\n" + MEDIA_URL;

    auto m3uContent = parseM3UContent(PLAYLIST_URL, playlist);

    EncryptionInfo expected{
        EncryptionInfo::Method::SAMPLE_AES, "https://www.amazon.com", "0x00000000000000000000000000000001"};
    matchEncryptionInfo(expected, m3uContent.entries[0].encryptionInfo);
}

TEST(M3UParserTest, test_parseByteRange) {
    std::string line = "#EXT-X-BYTERANGE:82112@752321";

    auto result = parseHLSByteRangeLine(line);

    EXPECT_EQ(std::make_tuple(752321, 752321 + 82112 - 1), result);
}

TEST(M3UParserTest, test_parseByteRangeMissingColon) {
    std::string line = "#EXT-X-BYTERANGE";

    auto result = parseHLSByteRangeLine(line);

    EXPECT_EQ(defaultByteRange, result);
}

TEST(M3UParserTest, test_parseByteRangeMissingAt) {
    std::string line = "#EXT-X-BYTERANGE:1234";

    auto result = parseHLSByteRangeLine(line);

    EXPECT_EQ(defaultByteRange, result);
}

TEST(M3UParserTest, test_parseByteRangeNonDecimal) {
    std::string line = "#EXT-X-BYTERANGE:abcd@efgh";

    auto result = parseHLSByteRangeLine(line);

    EXPECT_EQ(defaultByteRange, result);
}

TEST(M3UParserTest, test_hLSParseByteRange) {
    std::string playlist = EXT_M3U_HEADER + "#EXT-X-BYTERANGE:82112@752321\n" + MEDIA_URL;

    auto m3uContent = parseM3UContent(PLAYLIST_URL, playlist);

    EXPECT_EQ(std::make_tuple(752321, 752321 + 82112 - 1), m3uContent.entries[0].byteRange);
}

TEST(M3UParserTest, test_parseMAPMissingURL) {
    std::string line = "#EXT-X-MAP:";

    auto result = parseHLSMapLine(line, "");

    EXPECT_EQ(PlaylistParseResult::ERROR, result.parseResult);
}

TEST(M3UParserTest, test_parseMAPValidURL) {
    std::string line = "#EXT-X-MAP:URI=\"https://www.amazon.com\"";

    auto result = parseHLSMapLine(line, "");

    EXPECT_EQ("https://www.amazon.com", result.url);
    EXPECT_EQ(defaultByteRange, result.byteRange);
    EXPECT_EQ(PlaylistEntry::Type::MEDIA_INIT_INFO, result.type);
}

TEST(M3UParserTest, test_parseMAPValidByteRange) {
    std::string line = "#EXT-X-MAP:URI=\"https://www.amazon.com\",BYTERANGE=\"1234@5678\"";

    auto result = parseHLSMapLine(line, "");

    EXPECT_EQ("https://www.amazon.com", result.url);
    EXPECT_EQ(std::make_tuple(5678, 5678 + 1234 - 1), result.byteRange);
    EXPECT_EQ(PlaylistEntry::Type::MEDIA_INIT_INFO, result.type);
}

TEST(M3UParserTest, test_hLSParseMAP) {
    std::string playlist = EXT_M3U_HEADER + "#EXT-X-MAP:URI=\"https://www.amazon.com\"\n" + MEDIA_URL;

    auto m3uContent = parseM3UContent(PLAYLIST_URL, playlist);

    auto entry = m3uContent.entries[0];
    EXPECT_EQ("https://www.amazon.com", entry.url);
    EXPECT_EQ(PlaylistEntry::Type::MEDIA_INIT_INFO, entry.type);
}

TEST(M3UParserTest, test_masterPlaylist) {
    std::string playlist = EXT_M3U_HEADER + "#EXT-X-STREAM-INF\n" + PLAYLIST_URL;

    auto m3uContent = parseM3UContent(MASTER_PLAYLIST_URL, playlist);

    EXPECT_EQ(PLAYLIST_URL, m3uContent.variantURLs[0]);
    EXPECT_TRUE(m3uContent.isMasterPlaylist());
}

}  // namespace test
}  // namespace playlistParser
}  // namespace alexaClientSDK