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

#include "PlaylistParser/PlaylistUtils.h"

#include <algorithm>
#include <sstream>

#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterface.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/PlaylistParser/PlaylistParserObserverInterface.h>

namespace alexaClientSDK {
namespace playlistParser {

/// String to identify log entries originating from this file.
static const std::string TAG("PlaylistUtils");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The number of bytes read from the attachment with each read in the read loop.
static const size_t CHUNK_SIZE(1024);

/// The first line of an Extended M3U playlist.
static const std::string EXT_M3U_PLAYLIST_HEADER = "#EXTM3U";

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

/// The first line of a PLS playlist.
static const std::string PLS_PLAYLIST_HEADER = "[playlist]";

/// The beginning of a line in a PLS file indicating a URL.
static const std::string PLS_FILE = "File";

static const std::chrono::milliseconds INVALID_DURATION =
    avsCommon::utils::playlistParser::PlaylistParserObserverInterface::INVALID_DURATION;

bool extractPlaylistContent(std::unique_ptr<avsCommon::utils::HTTPContent> httpContent, std::string* content) {
    if (!content) {
        ACSDK_ERROR(LX("getContentFromPlaylistUrlIntoStringFailed").d("reason", "nullString"));
        return false;
    }
    if (!httpContent) {
        ACSDK_ERROR(LX("getContentFromPlaylistUrlIntoStringFailed").d("reason", "nullHTTPContentReceived"));
        return false;
    }
    if (!(*httpContent)) {
        ACSDK_ERROR(LX("getContentFromPlaylistUrlIntoStringFailed").d("reason", "badHTTPContentReceived"));
        return false;
    }

    auto reader = httpContent->dataStream->createReader(avsCommon::utils::sds::ReaderPolicy::BLOCKING);
    if (!reader) {
        ACSDK_ERROR(LX("getContentFromPlaylistUrlIntoStringFailed").d("reason", "failedToCreateStreamReader"));
        return false;
    }
    avsCommon::avs::attachment::AttachmentReader::ReadStatus readStatus =
        avsCommon::avs::attachment::AttachmentReader::ReadStatus::OK;
    std::string playlistContent;
    std::vector<char> buffer(CHUNK_SIZE, 0);
    bool streamClosed = false;
    while (!streamClosed) {
        auto bytesRead = reader->read(buffer.data(), buffer.size(), &readStatus);
        switch (readStatus) {
            case avsCommon::avs::attachment::AttachmentReader::ReadStatus::CLOSED:
                streamClosed = true;
                if (bytesRead == 0) {
                    break;
                }
                /* FALL THROUGH - to add any data received even if closed */
            case avsCommon::avs::attachment::AttachmentReader::ReadStatus::OK:
            case avsCommon::avs::attachment::AttachmentReader::ReadStatus::OK_WOULDBLOCK:
            case avsCommon::avs::attachment::AttachmentReader::ReadStatus::OK_TIMEDOUT:
                playlistContent.append(buffer.data(), bytesRead);
                break;
            case avsCommon::avs::attachment::AttachmentReader::ReadStatus::ERROR_OVERRUN:
            case avsCommon::avs::attachment::AttachmentReader::ReadStatus::ERROR_BYTES_LESS_THAN_WORD_SIZE:
            case avsCommon::avs::attachment::AttachmentReader::ReadStatus::ERROR_INTERNAL:
                ACSDK_ERROR(LX("getContentFromPlaylistUrlIntoStringFailed").d("reason", "readError"));
                return false;
        }
    }
    *content = playlistContent;
    return true;
}

M3UContent parseM3UContent(const std::string& playlistURL, const std::string& content) {
    /*
     * An M3U playlist is formatted such that all metadata information is prepended with a '#' and everything else is a
     * URL to play.
     */
    M3UContent parsedContent;
    std::istringstream iss(content);
    std::string line;
    UrlAndInfo entry;
    entry.length = INVALID_DURATION;
    while (std::getline(iss, line)) {
        removeCarriageReturnFromLine(&line);
        std::istringstream iss2(line);
        char firstChar;
        iss2 >> firstChar;
        if (!iss2) {
            continue;
        }
        if (firstChar == '#') {
            if (line.compare(0, EXTINF.length(), EXTINF) == 0) {
                entry.length = parseRuntime(line);
            } else if (line.compare(0, EXTSTREAMINF.length(), EXTSTREAMINF) == 0) {
                parsedContent.streamInfTagPresent = true;
            } else if (line.compare(0, ENDLIST.length(), ENDLIST) == 0) {
                parsedContent.endlistTagPresent = true;
            }
            continue;
        }
        // at this point, "line" is a url
        if (isURLAbsolute(line)) {
            entry.url = line;
            parsedContent.childrenUrls.push_back(entry);
            entry.url.clear();
            entry.length = INVALID_DURATION;
        } else {
            std::string absoluteURL;
            if (getAbsoluteURLFromRelativePathToURL(playlistURL, line, &absoluteURL)) {
                entry.url = absoluteURL;
                parsedContent.childrenUrls.push_back(entry);
                entry.url.clear();
                entry.length = INVALID_DURATION;
            }
        }
    }
    return parsedContent;
}

std::vector<std::string> parsePLSContent(const std::string& playlistURL, const std::string& content) {
    /*
     * A PLS playlist is formatted such that all URLs to play are prepended with "File'N'=", where 'N' refers to the
     * numbered URL. For example "File1=url.com ... File2="anotherurl.com".
     */
    std::vector<std::string> urlsParsed;
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        removeCarriageReturnFromLine(&line);
        if (line.compare(0, PLS_FILE.length(), PLS_FILE) == 0) {
            std::string url = line.substr(line.find_first_of('=') + 1);
            if (isURLAbsolute(url)) {
                urlsParsed.push_back(url);
            } else {
                std::string absoluteURL;
                if (getAbsoluteURLFromRelativePathToURL(playlistURL, url, &absoluteURL)) {
                    urlsParsed.push_back(absoluteURL);
                }
            }
        }
    }
    return urlsParsed;
}

void removeCarriageReturnFromLine(std::string* line) {
    if (!line) {
        return;
    }
    if (!line->empty() && (line->back() == '\r' || line->back() == '\n')) {
        line->pop_back();
    }
}

bool isURLAbsolute(const std::string& url) {
    return url.find("://") != std::string::npos;
}

bool getAbsoluteURLFromRelativePathToURL(std::string baseURL, std::string relativePath, std::string* absoluteURL) {
    auto positionOfLastSlash = baseURL.find_last_of('/');
    if (positionOfLastSlash == std::string::npos) {
        return false;
    } else {
        if (!absoluteURL) {
            return false;
        }
        baseURL.resize(positionOfLastSlash + 1);
        *absoluteURL = baseURL + relativePath;
        return true;
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

}  // namespace playlistParser
}  // namespace alexaClientSDK
