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

#include "PlaylistParser/PlaylistParser.h"

#include <algorithm>
#include <sstream>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/PlaylistParser/PlaylistParserObserverInterface.h>

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
        RequiresShutdown{"PlaylistParser"},
        m_contentFetcherFactory{contentFetcherFactory},
        m_shuttingDown{false} {
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
    std::deque<UrlAndInfo> urlsToParse;
    urlsToParse.push_front({rootUrl, INVALID_DURATION});
    std::string lastUrlParsed;
    while (!urlsToParse.empty() && !m_shuttingDown) {
        auto urlAndInfo = urlsToParse.front();
        urlsToParse.pop_front();
        if (urlAndInfo.length != INVALID_DURATION) {
            // This is a media URL and not a playlist
            ACSDK_DEBUG9(LX("foundNonPlaylistURL"));
            observer->onPlaylistEntryParsed(
                id,
                urlAndInfo.url,
                urlsToParse.empty() ? avsCommon::utils::playlistParser::PlaylistParseResult::SUCCESS
                                    : avsCommon::utils::playlistParser::PlaylistParseResult::STILL_ONGOING,
                urlAndInfo.length);
            continue;
        }
        auto contentFetcher = m_contentFetcherFactory->create(urlAndInfo.url);
        auto httpContent = contentFetcher->getContent(
            avsCommon::sdkInterfaces::HTTPContentFetcherInterface::FetchOptions::CONTENT_TYPE);
        if (!httpContent || !(*httpContent)) {
            ACSDK_ERROR(LX("getHTTPContent").d("reason", "badHTTPContentReceived"));
            observer->onPlaylistEntryParsed(
                id, urlAndInfo.url, avsCommon::utils::playlistParser::PlaylistParseResult::ERROR, urlAndInfo.length);
            return;
        }
        std::string contentType = httpContent->contentType.get();
        ACSDK_DEBUG9(LX("PlaylistParser")
                         .d("contentType", contentType)
                         .sensitive("url", urlAndInfo.url)
                         .d("length", urlAndInfo.length.count()));
        std::transform(contentType.begin(), contentType.end(), contentType.begin(), ::tolower);
        // Checking the HTML content type to see if the URL is a playlist.
        if (contentType.find(M3U_CONTENT_TYPE) != std::string::npos) {
            std::string playlistContent;
            if (!getContentFromPlaylistUrlIntoString(urlAndInfo.url, &playlistContent)) {
                ACSDK_ERROR(LX("failedToRetrieveContent").sensitive("url", urlAndInfo.url));
                observer->onPlaylistEntryParsed(
                    id,
                    urlAndInfo.url,
                    avsCommon::utils::playlistParser::PlaylistParseResult::ERROR,
                    urlAndInfo.length);
                return;
            }
            // This playlist may either be M3U or M3U8 so some additional parsing is required.
            bool isM3U8 = isM3UPlaylistM3U8(playlistContent);
            if (isM3U8) {
                ACSDK_DEBUG9(LX("isM3U8Playlist").sensitive("url", urlAndInfo.url));
            } else {
                ACSDK_DEBUG9(LX("isPlainM3UPlaylist").sensitive("url", urlAndInfo.url));
            }
            // TODO: investigate refactoring the two std::finds into a common place
            if (std::find(
                    playlistTypesToNotBeParsed.begin(),
                    playlistTypesToNotBeParsed.end(),
                    isM3U8 ? PlaylistType::M3U8 : PlaylistType::M3U) != playlistTypesToNotBeParsed.end()) {
                observer->onPlaylistEntryParsed(
                    id,
                    urlAndInfo.url,
                    urlsToParse.empty() ? avsCommon::utils::playlistParser::PlaylistParseResult::SUCCESS
                                        : avsCommon::utils::playlistParser::PlaylistParseResult::STILL_ONGOING,
                    urlAndInfo.length);
                continue;
            }
            auto M3UContent = parseM3UContent(urlAndInfo.url, playlistContent);
            const auto& childrenUrls = M3UContent.childrenUrls;
            if (childrenUrls.empty()) {
                ACSDK_ERROR(LX("noChildrenURLs"));
                observer->onPlaylistEntryParsed(
                    id,
                    urlAndInfo.url,
                    avsCommon::utils::playlistParser::PlaylistParseResult::ERROR,
                    urlAndInfo.length);
                return;
            }
            ACSDK_DEBUG9((LX("foundChildrenURLsInPlaylist").d("num", childrenUrls.size())));
            if (isM3U8) {
                if (M3UContent.streamInfTagPresent) {
                    // Indicates that this is the Master Playlist and that only one URL should be chosen from here
                    ACSDK_DEBUG9(LX("encounteredMasterPlaylist").sensitive("url", urlAndInfo.url));
                    // Because we don't do any selective choosing based on bitrates or codecs, only push the first URL
                    // as a default.
                    urlsToParse.push_front(childrenUrls.front());
                } else {
                    // lastUrlParsed is set when we actually parse some urls from the playlist - here, it is our first
                    // pass at this playlist
                    if (lastUrlParsed.empty()) {
                        for (auto reverseIt = childrenUrls.rbegin(); reverseIt != childrenUrls.rend(); ++reverseIt) {
                            urlsToParse.push_front(*reverseIt);
                        }
                        lastUrlParsed = childrenUrls.back().url;
                    } else {
                        // Setting this to 0 as an intial value so that if we don't see the last URL we parsed in the
                        // latest pass of the playlist, we stream all the URLs within the playlist as a sort of
                        // recovery mechanism. This way, if we parse this so far into the future that all the URLs we
                        // had previously seen are gone, we'll still stream the latest URLs.
                        int startPointForNewURLsAdded = 0;
                        for (int i = childrenUrls.size() - 1; i >= 0; --i) {
                            if (childrenUrls.at(i).url == lastUrlParsed) {
                                // We need to add the URLs past this point
                                startPointForNewURLsAdded = i + 1;
                            }
                        }
                        for (int i = childrenUrls.size() - 1; i >= startPointForNewURLsAdded; --i) {
                            ACSDK_DEBUG9(LX("foundNewURLInLivePlaylist"));
                            urlsToParse.push_front(childrenUrls.at(i));
                            lastUrlParsed = urlsToParse.back().url;
                        }
                    }
                    if (!M3UContent.endlistTagPresent) {
                        ACSDK_DEBUG9(LX("encounteredLiveHLSPlaylist")
                                         .sensitive("url", urlAndInfo.url)
                                         .d("info", "willRetryURLInFuture"));
                        /*
                         * Because this URL represents a live playlist which can have additional chunks added to it, we
                         * need to make a request to this URL again in the future to continue playback of additional
                         * chunks that get added.
                         */
                        urlsToParse.push_back(urlAndInfo);
                    }
                }
            } else {
                for (auto reverseIt = childrenUrls.rbegin(); reverseIt != childrenUrls.rend(); ++reverseIt) {
                    urlsToParse.push_front(*reverseIt);
                }
            }
        } else if (contentType.find(PLS_CONTENT_TYPE) != std::string::npos) {
            ACSDK_DEBUG9(LX("isPLSPlaylist").sensitive("url", urlAndInfo.url));
            /*
             * This is for sure a PLS playlist, so if PLS is one of the desired playlist types to not be parsed, then
             * notify and return immediately.
             */
            if (std::find(playlistTypesToNotBeParsed.begin(), playlistTypesToNotBeParsed.end(), PlaylistType::PLS) !=
                playlistTypesToNotBeParsed.end()) {
                observer->onPlaylistEntryParsed(
                    id,
                    urlAndInfo.url,
                    urlsToParse.empty() ? avsCommon::utils::playlistParser::PlaylistParseResult::SUCCESS
                                        : avsCommon::utils::playlistParser::PlaylistParseResult::STILL_ONGOING,
                    urlAndInfo.length);
                continue;
            }
            std::string playlistContent;
            if (!getContentFromPlaylistUrlIntoString(urlAndInfo.url, &playlistContent)) {
                observer->onPlaylistEntryParsed(
                    id,
                    urlAndInfo.url,
                    avsCommon::utils::playlistParser::PlaylistParseResult::ERROR,
                    urlAndInfo.length);
                return;
            }
            auto childrenUrls = parsePLSContent(urlAndInfo.url, playlistContent);
            if (childrenUrls.empty()) {
                observer->onPlaylistEntryParsed(
                    id,
                    urlAndInfo.url,
                    avsCommon::utils::playlistParser::PlaylistParseResult::ERROR,
                    urlAndInfo.length);
                return;
            }
            for (auto reverseIt = childrenUrls.rbegin(); reverseIt != childrenUrls.rend(); ++reverseIt) {
                urlsToParse.push_front({*reverseIt, INVALID_DURATION});
            }
        } else {
            ACSDK_DEBUG9(LX("foundNonPlaylistURL"));
            // This is a non-playlist URL or a playlist that we don't support (M3U, M3U8, PLS).
            observer->onPlaylistEntryParsed(
                id,
                urlAndInfo.url,
                urlsToParse.empty() ? avsCommon::utils::playlistParser::PlaylistParseResult::SUCCESS
                                    : avsCommon::utils::playlistParser::PlaylistParseResult::STILL_ONGOING,
                urlAndInfo.length);
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

PlaylistParser::M3UContent PlaylistParser::parseM3UContent(const std::string& playlistURL, const std::string& content) {
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

std::chrono::milliseconds PlaylistParser::parseRuntime(std::string line) {
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

void PlaylistParser::doShutdown() {
    m_shuttingDown = true;
    m_executor.shutdown();
}

}  // namespace playlistParser
}  // namespace alexaClientSDK
