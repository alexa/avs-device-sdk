/*
 * PlaylistParser.cpp
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

#include "PlaylistParser/PlaylistParser.h"

#include <algorithm>
#include <sstream>

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace playlistParser {

/// String to identify log entries originating from this file.
static const std::string TAG("PlaylistParser");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The HTML content-type of an M3U playlist.
static const std::string M3U_CONTENT_TYPE = "mpegurl";

/// The HTML content-type of a PLS playlist.
static const std::string PLS_CONTENT_TYPE = "scpls";

/// The number of bytes read from the attachment with each read in the read loop.
static const size_t CHUNK_SIZE(1024);

/// The id of each request.
static int g_id = 0;

/// The first line of an M3U8 playlist.
static const std::string M3U8_PLAYLIST_HEADER = "#EXTM3U";

/// The first line of a PLS playlist.
static const std::string PLS_PLAYLIST_HEADER = "[playlist]";

/// The beginning of a line in a PLS file indicating a URL.
static const std::string PLS_FILE = "File";

std::unique_ptr<PlaylistParser> PlaylistParser::create(
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory) {
    if (!contentFetcherFactory) {
        return nullptr;
    }
    return std::unique_ptr<PlaylistParser>(new PlaylistParser(contentFetcherFactory));
}

int PlaylistParser::parsePlaylist(
    std::string url,
    std::shared_ptr<avsCommon::utils::playlistParser::PlaylistParserObserverInterface> observer,
    std::vector<PlaylistType> playlistTypesToNotBeParsed) {
    if (url.empty()) {
        return START_FAILURE;
    }
    if (!observer) {
        return START_FAILURE;
    }

    auto id = ++g_id;

    m_executor.submit([this, id, observer, url, playlistTypesToNotBeParsed]() {
        doDepthFirstSearch(id, observer, url, playlistTypesToNotBeParsed);
    });
    return id;
}

PlaylistParser::PlaylistParser(
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory) :
        m_contentFetcherFactory{contentFetcherFactory} {
}

void PlaylistParser::doDepthFirstSearch(
    int id,
    std::shared_ptr<avsCommon::utils::playlistParser::PlaylistParserObserverInterface> observer,
    const std::string& rootUrl,
    std::vector<PlaylistType> playlistTypesToNotBeParsed) {
    /*
     * A depth first search, as follows:
     * 1. Push root to vector.
     * 2. While vector isn't empty, pop from front and push children, in the order they appeared, to front of vector.
     */
    std::deque<std::string> urlsToParse;
    urlsToParse.push_front(rootUrl);
    while (!urlsToParse.empty()) {
        auto url = urlsToParse.front();
        urlsToParse.pop_front();
        auto contentFetcher = m_contentFetcherFactory->create(url);
        auto httpContent = contentFetcher->getContent(
            avsCommon::sdkInterfaces::HTTPContentFetcherInterface::FetchOptions::CONTENT_TYPE);
        if (!httpContent || !(*httpContent)) {
            ACSDK_ERROR(LX("getHTTPContent").d("reason", "badHTTPContentReceived"));
            observer->onPlaylistEntryParsed(id, url, avsCommon::utils::playlistParser::PlaylistParseResult::ERROR);
            return;
        }
        std::string contentType = httpContent->contentType.get();
        ACSDK_DEBUG9(LX("PlaylistParser").d("contentType", contentType).sensitive("url", url));
        std::transform(contentType.begin(), contentType.end(), contentType.begin(), ::tolower);
        // Checking the HTML content type to see if the URL is a playlist.
        if (contentType.find(M3U_CONTENT_TYPE) != std::string::npos) {
            std::string playlistContent;
            if (!getContentFromPlaylistUrlIntoString(url, &playlistContent)) {
                ACSDK_ERROR(LX("failedToRetrieveContent").sensitive("url", url));
                observer->onPlaylistEntryParsed(id, url, avsCommon::utils::playlistParser::PlaylistParseResult::ERROR);
                return;
            }
            // This playlist may either be M3U or M3U8 so some additional parsing is required.
            bool isM3U8 = isM3UPlaylistM3U8(playlistContent);
            if (isM3U8) {
                ACSDK_DEBUG9(LX("isM3U8Playlist").sensitive("url", url));
            } else {
                ACSDK_DEBUG9(LX("isPlainM3UPlaylist").sensitive("url", url));
            }
            // TODO: investigate refactoring the two std::finds into a common place
            if (std::find(
                    playlistTypesToNotBeParsed.begin(),
                    playlistTypesToNotBeParsed.end(),
                    isM3U8 ? PlaylistType::M3U8 : PlaylistType::M3U) != playlistTypesToNotBeParsed.end()) {
                observer->onPlaylistEntryParsed(
                    id,
                    url,
                    urlsToParse.empty() ? avsCommon::utils::playlistParser::PlaylistParseResult::SUCCESS
                                        : avsCommon::utils::playlistParser::PlaylistParseResult::STILL_ONGOING);
                continue;
            }
            auto childrenUrls = parseM3UContent(url, playlistContent);
            if (childrenUrls.empty()) {
                ACSDK_ERROR(LX("noChildrenURLs"));
                observer->onPlaylistEntryParsed(id, url, avsCommon::utils::playlistParser::PlaylistParseResult::ERROR);
                return;
            }
            for (auto reverseIt = childrenUrls.rbegin(); reverseIt != childrenUrls.rend(); ++reverseIt) {
                urlsToParse.push_front(*reverseIt);
            }
        } else if (contentType.find(PLS_CONTENT_TYPE) != std::string::npos) {
            ACSDK_DEBUG9(LX("isPLSPlaylist").sensitive("url", url));
            /*
             * This is for sure a PLS playlist, so if PLS is one of the desired playlist types to not be parsed, then
             * notify and return immediately.
             */
            if (std::find(playlistTypesToNotBeParsed.begin(), playlistTypesToNotBeParsed.end(), PlaylistType::PLS) !=
                playlistTypesToNotBeParsed.end()) {
                observer->onPlaylistEntryParsed(
                    id,
                    url,
                    urlsToParse.empty() ? avsCommon::utils::playlistParser::PlaylistParseResult::SUCCESS
                                        : avsCommon::utils::playlistParser::PlaylistParseResult::STILL_ONGOING);
                continue;
            }
            std::string playlistContent;
            if (!getContentFromPlaylistUrlIntoString(url, &playlistContent)) {
                observer->onPlaylistEntryParsed(id, url, avsCommon::utils::playlistParser::PlaylistParseResult::ERROR);
                return;
            }
            auto childrenUrls = parsePLSContent(url, playlistContent);
            if (childrenUrls.empty()) {
                observer->onPlaylistEntryParsed(id, url, avsCommon::utils::playlistParser::PlaylistParseResult::ERROR);
                return;
            }
            for (auto reverseIt = childrenUrls.rbegin(); reverseIt != childrenUrls.rend(); ++reverseIt) {
                urlsToParse.push_front(*reverseIt);
            }
        } else {
            // This is a non-playlist URL or a playlist that we don't support (M3U, M3U8, PLS).
            observer->onPlaylistEntryParsed(
                id,
                url,
                urlsToParse.empty() ? avsCommon::utils::playlistParser::PlaylistParseResult::SUCCESS
                                    : avsCommon::utils::playlistParser::PlaylistParseResult::STILL_ONGOING);
        }
    }
}

bool PlaylistParser::getContentFromPlaylistUrlIntoString(const std::string& url, std::string* content) const {
    if (!content) {
        ACSDK_ERROR(LX("getContentFromPlaylistUrlIntoStringFailed").d("reason", "nullString"));
        return false;
    }
    auto contentFetcher = m_contentFetcherFactory->create(url);
    auto httpContent =
        contentFetcher->getContent(avsCommon::sdkInterfaces::HTTPContentFetcherInterface::FetchOptions::ENTIRE_BODY);
    if (!httpContent) {
        ACSDK_ERROR(LX("getContentFromPlaylistUrlIntoStringFailed").d("reason", "nullHTTPContentReceived"));
        return false;
    }
    if (!(*httpContent)) {
        ACSDK_ERROR(LX("getContentFromPlaylistUrlIntoStringFailed").d("reason", "badHTTPContentReceived"));
        return false;
    }
    auto reader = httpContent->dataStream->createReader(avsCommon::avs::attachment::AttachmentReader::Policy::BLOCKING);
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

std::vector<std::string> PlaylistParser::parseM3UContent(const std::string& playlistURL, const std::string& content) {
    /*
     * An M3U playlist is formatted such that all metadata information is prepended with a '#' and everything else is a
     * URL to play.
     */
    std::vector<std::string> urlsParsed;
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        removeCarriageReturnFromLine(&line);
        std::istringstream iss2(line);
        char firstChar;
        iss2 >> firstChar;
        if (!iss2 || firstChar == '#') {
            continue;
        }
        // at this point, "line" is a url
        if (isURLAbsolute(line)) {
            urlsParsed.push_back(line);
        } else {
            std::string absoluteURL;
            if (getAbsoluteURLFromRelativePathToURL(playlistURL, line, &absoluteURL)) {
                urlsParsed.push_back(absoluteURL);
            }
        }
    }
    return urlsParsed;
}

std::vector<std::string> PlaylistParser::parsePLSContent(const std::string& playlistURL, const std::string& content) {
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

void PlaylistParser::removeCarriageReturnFromLine(std::string* line) {
    if (!line) {
        return;
    }
    if (!line->empty() && (line->back() == '\r' || line->back() == '\n')) {
        line->pop_back();
    }
}

bool PlaylistParser::isURLAbsolute(const std::string& url) {
    return url.find("://") != std::string::npos;
}

bool PlaylistParser::getAbsoluteURLFromRelativePathToURL(
    std::string baseURL,
    std::string relativePath,
    std::string* absoluteURL) {
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

bool PlaylistParser::isM3UPlaylistM3U8(const std::string& playlistContent) {
    std::istringstream iss(playlistContent);
    std::string line;
    if (std::getline(iss, line)) {
        /*
         * This isn't the best way of determining whether a playlist is M3U8 or M3U. However, there isn't really a
         * better way that I could come up with. The playlist header I'm searching for is "EXTM3U" which indicates that
         * this playlist is an "Extended M3U" playlist as opposed to a plain M3U playlist. In my testing, I've found
         * that all M3U8 playlists are also extended M3U playlists, but this might not be guaranteed.
         */
        if (line.compare(0, M3U8_PLAYLIST_HEADER.length(), M3U8_PLAYLIST_HEADER) == 0) {
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
