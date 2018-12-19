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

#include <iomanip>
#include <sstream>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/PlaylistParser/PlaylistParserObserverInterface.h>

#include "PlaylistParser/M3UParser.h"
#include "PlaylistParser/PlaylistUtils.h"

namespace alexaClientSDK {
namespace playlistParser {

using namespace avsCommon::utils::playlistParser;

/// String to identify log entries originating from this file.
static const std::string TAG("M3UParser");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The first line of an Extended M3U playlist.
static const std::string EXT_M3U_PLAYLIST_HEADER = "#EXTM3U";

/// HLS #EXTINF tag.
static const std::string EXTINF = "#EXTINF";

/**
 * A tag present in a live stream playlist that indicates that the next URL points to a playlist. Attributes of this tag
 * include information such as bitrate, codecs, and others.
 */
static const std::string EXTSTREAMINF = "#EXT-X-STREAM-INF";

/**
 * A tag present in a live stream playlist indicating that no more URLs will be added to the playlist on subsequent
 * requests.
 */
static const std::string ENDLIST = "#EXT-X-ENDLIST";

/// EXT-X-KEY HLS tag.
static const std::string EXT_KEY = "#EXT-X-KEY:";

/// EXT-X-MAP HLS tag.
static const std::string EXT_MAP = "#EXT-X-MAP:";

/// EXT-X-BYTERANGE HLS tag.
static const std::string EXT_BYTERANGE = "#EXT-X-BYTERANGE:";

/// Method encryption attribute.
static const std::string METHOD_ATTR = "METHOD=";

/// HLS tag attribute for URI.
static const std::string URI_ATTR = "URI=\"";

/// Initialization Vector encryption attribute.
static const std::string IV_ATTR = "IV=";

/// Byte range encryption attribute.
static const std::string BYTERANGE_ATTR = "BYTERANGE=\"";

/// Encryption method: NONE.
static const std::string ENCRYPTION_METHOD_NONE = "NONE";

/// Encryption method: AES-128.
static const std::string ENCRYPTION_METHOD_AES_128 = "AES-128";

/// Encryption method: SAMPLE-AES.
static const std::string ENCRYPTION_METHOD_SAMPLE_AES = "SAMPLE-AES";

/// An invalid duration.
static const auto INVALID_DURATION = std::chrono::milliseconds(-1);

/// Length of initialization vector as hex string.
static const int IV_HEX_STRING_LENGTH = 32;

/**
 * Helper method to parse runtime from #EXTINF tag.
 *
 * @param line The #EXTINF line in HLS playlist.
 * @return parsed runtime in milliseconds from line.
 */
std::chrono::milliseconds parseRuntime(std::string line);

/**
 * Helper method to convert a line in HLS playlist to absolute URL.
 *
 * @param baseURL The URL of the HLS playlist.
 * @param url The URL found in HLS playlist.
 * @param[out] absoluteURL Absolute URL parsed from @c line and converted to an absolute URL.
 * @return @c true if method is successful or @c false otherwise.
 */
bool getAbsoluteURL(const std::string& baseURL, const std::string& url, std::string* absoluteURL);

/**
 * Helper method to check if line starts with prefix.
 *
 * @param line A HLS line.
 * @param prefix The prefix to check.
 * @return @c true if @c line starts with @c prefix, @c false otherwise.
 */
inline bool hasPrefix(const std::string& line, const std::string& prefix) {
    return line.compare(0, prefix.length(), prefix) == 0;
}

PlayItem::PlayItem(std::string playlistURL) : type(Type::PLAYLIST_URL), playlistURL(playlistURL) {
}

PlayItem::PlayItem(PlaylistEntry playlistEntry) : type(Type::MEDIA_INFO), playlistEntry(playlistEntry) {
}

M3UContent::M3UContent(const std::vector<std::string> variantURLs) : variantURLs(variantURLs), isLive(false) {
}

M3UContent::M3UContent(const std::vector<PlaylistEntry> entries, bool isLive) : entries(entries), isLive(isLive) {
}

bool M3UContent::isMasterPlaylist() const {
    return !variantURLs.empty();
}

bool M3UContent::empty() const {
    return entries.empty() && variantURLs.empty();
}

static std::string to16ByteHexString(int number) {
    std::stringstream ss;
    ss << "0x" << std::setfill('0') << std::setw(IV_HEX_STRING_LENGTH) << std::hex << number;
    return ss.str();
}

M3UContent parseM3UContent(const std::string& playlistURL, const std::string& content) {
    /*
     * An M3U playlist is formatted such that all metadata information is prepended with a '#' and everything else is a
     * URL to play.
     */
    std::istringstream iss(content);
    std::string line;

    auto isPlaylistExtendedM3U = false;
    auto isLive = true;
    auto isMasterPlaylist = false;
    auto duration = INVALID_DURATION;
    auto encryptionInfo = EncryptionInfo();
    int mediaSequenceNumber = 1;
    ByteRange byteRange = std::make_tuple(0, 0);
    std::vector<std::string> variantURLs;
    std::vector<PlaylistEntry> entries;

    while (std::getline(iss, line)) {
        removeCarriageReturnFromLine(&line);
        std::istringstream linestream(line);
        char firstChar;
        linestream >> firstChar;
        if (!linestream) {
            continue;
        }
        if (firstChar == '#') {
            if (hasPrefix(line, EXT_M3U_PLAYLIST_HEADER)) {
                isPlaylistExtendedM3U = true;
            } else if (hasPrefix(line, EXTINF)) {
                duration = parseRuntime(line);
            } else if (hasPrefix(line, EXTSTREAMINF)) {
                isMasterPlaylist = true;
            } else if (hasPrefix(line, ENDLIST)) {
                isLive = false;
                if (!entries.empty()) {
                    entries.back().parseResult = PlaylistParseResult::FINISHED;
                }
                break;
            } else if (hasPrefix(line, EXT_KEY)) {
                encryptionInfo = parseHLSEncryptionLine(line, playlistURL);
            } else if (hasPrefix(line, EXT_BYTERANGE)) {
                byteRange = parseHLSByteRangeLine(line);
            } else if (hasPrefix(line, EXT_MAP)) {
                auto mediaInitInfo = parseHLSMapLine(line, playlistURL);
                mediaInitInfo.encryptionInfo = encryptionInfo;
                entries.push_back(mediaInitInfo);
            }
            continue;
        }

        // at this point, "line" is a url
        std::string absoluteURL;
        if (!getAbsoluteURL(playlistURL, line, &absoluteURL)) {
            // Failed to retrieve URL from line, bail
            continue;
        }

        if (isMasterPlaylist) {
            variantURLs.push_back(absoluteURL);
        } else {
            auto entryEncryptionInfo(encryptionInfo);
            if (entryEncryptionInfo.initVector.empty()) {
                entryEncryptionInfo.initVector = to16ByteHexString(mediaSequenceNumber);
            }
            auto parseResult = PlaylistParseResult::STILL_ONGOING;
            entries.push_back(PlaylistEntry(
                absoluteURL, duration, parseResult, PlaylistEntry::Type::MEDIA_INFO, byteRange, entryEncryptionInfo));
            ++mediaSequenceNumber;
        }

        byteRange = std::make_tuple(0, 0);
    }

    if (!isPlaylistExtendedM3U && !entries.empty()) {
        entries.back().parseResult = PlaylistParseResult::FINISHED;
    }

    return isMasterPlaylist ? M3UContent(variantURLs) : M3UContent(entries, isLive);
}

bool getAbsoluteURL(const std::string& baseURL, const std::string& url, std::string* absoluteURL) {
    if (isURLAbsolute(url)) {
        *absoluteURL = url;
        return true;
    }

    return getAbsoluteURLFromRelativePathToURL(baseURL, url, absoluteURL);
}

bool isPlaylistExtendedM3U(const std::string& playlistContent) {
    std::istringstream iss(playlistContent);
    std::string line;
    if (std::getline(iss, line)) {
        if (line.compare(0, EXT_M3U_PLAYLIST_HEADER.length(), EXT_M3U_PLAYLIST_HEADER) == 0) {
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

std::chrono::milliseconds parseRuntime(std::string line) {
    // #EXTINF:1234.00, blah blah blah have you ever heard the tragedy of darth plagueis the wise?
    auto runner = EXTINF.length();

    // skip whitespace
    while (runner < line.length() && std::isspace(line.at(runner))) {
        ++runner;
    }
    if (runner == line.length()) {
        return INVALID_DURATION;
    }

    // find colon
    if (line.at(runner) != ':') {
        return INVALID_DURATION;
    }
    ++runner;

    // skip whitespace
    while (runner < line.length() && std::isspace(line.at(runner))) {
        ++runner;
    }
    if (runner == line.length()) {
        return INVALID_DURATION;
    }
    // from here, we should be reading numbers or a '.' only, so the fractional part of the seconds
    auto stringFromHereOnwards = line.substr(runner);
    std::istringstream iss(stringFromHereOnwards);
    int seconds;
    char nextChar;
    iss >> seconds;
    if (!iss) {
        return INVALID_DURATION;
    }
    if (seconds < 0) {
        return INVALID_DURATION;
    }
    std::chrono::milliseconds duration = std::chrono::seconds(seconds);
    iss >> nextChar;
    if (!iss) {
        return duration;
    }
    if (nextChar == '.') {
        int digitsSoFar = 0;
        unsigned int fractionalSeconds = 0;
        // we only care about the first 3 (sig figs = millisecond limit)
        while (digitsSoFar < 3) {
            iss >> nextChar;
            if (!iss) {
                break;
            }
            if (!isdigit(nextChar)) {
                break;
            }
            fractionalSeconds *= 10;
            fractionalSeconds += (nextChar - '0');
            ++digitsSoFar;
        }
        // if we read say "1", this is equivalent to 0.1 s or 100 ms
        while (digitsSoFar < 3) {
            fractionalSeconds *= 10;
            ++digitsSoFar;
        }
        duration += std::chrono::milliseconds(fractionalSeconds);
    }
    do {
        if (isdigit(nextChar)) {
            continue;
        } else {
            if (nextChar == ',') {
                break;
            } else {
                return INVALID_DURATION;
            }
        }
    } while (iss >> nextChar);
    return duration;
}

std::string parseURIAttribute(const std::string& line, const std::string& baseURL) {
    size_t pos = line.find(URI_ATTR);
    if (pos == std::string::npos) {
        ACSDK_ERROR(LX("parseURIAttribute").d("reason", "noURI"));
        return std::string();
    }

    // find closing quotes
    size_t beginPos = pos + URI_ATTR.length();
    size_t closePos = line.find("\"", beginPos, 1);
    if (closePos == std::string::npos) {
        ACSDK_ERROR(LX("parseURIAttribute").d("reason", "closingQuotesForURINotFound"));
        return std::string();
    }

    std::string uri = line.substr(beginPos, closePos - beginPos);
    std::string absoluteURL;

    // Get absolute url
    if (!getAbsoluteURL(baseURL, uri, &absoluteURL)) {
        ACSDK_ERROR(LX("parseURIAttribute").d("reason", "getAbsoluteURLFailed"));
        return std::string();
    }
    return absoluteURL;
}

EncryptionInfo parseHLSEncryptionLine(const std::string& line, const std::string& baseURL) {
    auto pos = line.find(METHOD_ATTR);
    if (pos == std::string::npos) {
        ACSDK_ERROR(LX("parseHLSEncryptionLineFailed").d("reason", "noMethod"));
        return EncryptionInfo();
    }

    pos += METHOD_ATTR.length();
    EncryptionInfo::Method method;

    if (line.compare(pos, ENCRYPTION_METHOD_NONE.length(), ENCRYPTION_METHOD_NONE) == 0) {
        ACSDK_INFO(LX("parseHLSEncryptionLine").d("reason", "notEncrypted"));
        return EncryptionInfo();
    } else if (line.compare(pos, ENCRYPTION_METHOD_AES_128.length(), ENCRYPTION_METHOD_AES_128) == 0) {
        method = EncryptionInfo::Method::AES_128;
    } else if (line.compare(pos, ENCRYPTION_METHOD_SAMPLE_AES.length(), ENCRYPTION_METHOD_SAMPLE_AES) == 0) {
        method = EncryptionInfo::Method::SAMPLE_AES;
    } else {
        ACSDK_ERROR(LX("parseHLSEncryptionLineFailed").d("reason", "unSupportedEncryption").d("line", line.c_str()));
        return EncryptionInfo();
    }

    auto keyURL = parseURIAttribute(line, baseURL);
    if (keyURL.empty()) {
        return EncryptionInfo();
    }

    std::string encryptionIV;
    pos = line.find(IV_ATTR);
    if (pos != std::string::npos) {
        ACSDK_INFO(LX("parseHLSEncryptionLineFailed").d("reason", "foundEncryptedIV"));
        encryptionIV = line.substr(pos + IV_ATTR.length());
        size_t commaPos = encryptionIV.find(",");
        if (commaPos != std::string::npos) {
            encryptionIV = encryptionIV.substr(0, commaPos);
        }
    }

    return EncryptionInfo(method, keyURL, encryptionIV);
}

ByteRange parseByteRange(const std::string& byteRange) {
    // 1234[@5678]. Note: 5678 is optional.
    ACSDK_DEBUG9(LX("parseByteRange").d("byteRange", byteRange.c_str()));
    auto pos = byteRange.find("@");
    if (pos == std::string::npos) {
        // TODO ACSDK-2112: Use previous range when position is not available.
        ACSDK_WARN(LX("parseByteRangeWarn").d("reason", "No @ seen").d("range", byteRange.c_str()));
        return std::make_tuple(0, 0);
    }
    std::stringstream rangeStream(byteRange.substr(0, pos));
    std::stringstream atStream(byteRange.substr(pos + 1));

    long at = 0, range = 0;
    rangeStream >> range;
    atStream >> at;

    if (!at && !range) {
        ACSDK_ERROR(LX("parseByteRangeFailed").d("reason", "string to long failed"));
        return std::make_tuple(0, 0);
    }

    // -1 because current byte included
    return std::make_tuple(at, at + range - 1);
}

ByteRange parseHLSByteRangeLine(const std::string& line) {
    size_t pos = line.find(":");
    return parseByteRange(line.substr(pos + 1));
}

PlaylistEntry parseHLSMapLine(const std::string& line, const std::string& baseURL) {
    std::string url = parseURIAttribute(line, baseURL);
    if (url.empty()) {
        return PlaylistEntry::createErrorEntry(url);
    }

    ByteRange byteRange;
    size_t pos = line.find(BYTERANGE_ATTR);
    if (pos != std::string::npos) {
        size_t beginPos = pos + BYTERANGE_ATTR.length();
        size_t closePos = line.find('\"', beginPos);
        if (closePos != std::string::npos) {
            byteRange = parseByteRange(line.substr(beginPos, closePos - beginPos));
        }
    }
    return PlaylistEntry::createMediaInitInfo(url, byteRange);
}

}  // namespace playlistParser
}  // namespace alexaClientSDK
