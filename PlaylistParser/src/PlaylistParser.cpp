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

#include <algorithm>
#include <deque>
#include <sstream>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/PlaylistParser/PlaylistParserObserverInterface.h>

#include "PlaylistParser/PlaylistParser.h"
#include "PlaylistParser/PlaylistUtils.h"

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

/// The id of each request.
static int g_id = 0;

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
        ACSDK_ERROR(LX("parsePlaylistFailed").d("reason", "emptyUrl"));
        return START_FAILURE;
    }
    if (!observer) {
        ACSDK_ERROR(LX("parsePlaylistFailed").d("reason", "emptyObserver"));
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
                urlsToParse.empty() ? avsCommon::utils::playlistParser::PlaylistParseResult::FINISHED
                                    : avsCommon::utils::playlistParser::PlaylistParseResult::STILL_ONGOING,
                urlAndInfo.length);
            continue;
        }
        auto contentFetcher = m_contentFetcherFactory->create(urlAndInfo.url);
        auto httpContent = contentFetcher->getContent(
            avsCommon::sdkInterfaces::HTTPContentFetcherInterface::FetchOptions::CONTENT_TYPE);
        if (!httpContent) {
            ACSDK_ERROR(LX("getHTTPContent").d("reason", "nullHTTPContentReceived"));
            observer->onPlaylistEntryParsed(
                id, urlAndInfo.url, avsCommon::utils::playlistParser::PlaylistParseResult::ERROR, urlAndInfo.length);
            return;
        }
        do {
            if (m_shuttingDown) {
                ACSDK_DEBUG9(LX("doDepthFirstSearch").d("info", "shuttingDown"));
                return;
            }
        } while (!httpContent->isReady(WAIT_FOR_FUTURE_READY_TIMEOUT));
        if (!(*httpContent)) {
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
            if (!getPlaylistContent(m_contentFetcherFactory->create(urlAndInfo.url), &playlistContent)) {
                ACSDK_ERROR(LX("failedToRetrieveContent").sensitive("url", urlAndInfo.url));
                observer->onPlaylistEntryParsed(
                    id,
                    urlAndInfo.url,
                    avsCommon::utils::playlistParser::PlaylistParseResult::ERROR,
                    urlAndInfo.length);
                return;
            }
            // This playlist may either be M3U or EXT_M3U so some additional parsing is required.
            bool isExtendedM3U = isPlaylistExtendedM3U(playlistContent);
            if (isExtendedM3U) {
                ACSDK_DEBUG9(LX("isM3UExtendedPlaylist").sensitive("url", urlAndInfo.url));
            } else {
                ACSDK_DEBUG9(LX("isPlainM3UPlaylist").sensitive("url", urlAndInfo.url));
            }
            // TODO: investigate refactoring the two std::finds into a common place
            if (std::find(
                    playlistTypesToNotBeParsed.begin(),
                    playlistTypesToNotBeParsed.end(),
                    isExtendedM3U ? PlaylistType::EXT_M3U : PlaylistType::M3U) != playlistTypesToNotBeParsed.end()) {
                observer->onPlaylistEntryParsed(
                    id,
                    urlAndInfo.url,
                    urlsToParse.empty() ? avsCommon::utils::playlistParser::PlaylistParseResult::FINISHED
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
            if (isExtendedM3U) {
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
                    urlsToParse.empty() ? avsCommon::utils::playlistParser::PlaylistParseResult::FINISHED
                                        : avsCommon::utils::playlistParser::PlaylistParseResult::STILL_ONGOING,
                    urlAndInfo.length);
                continue;
            }
            std::string playlistContent;
            if (!getPlaylistContent(m_contentFetcherFactory->create(urlAndInfo.url), &playlistContent)) {
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
            // This is a non-playlist URL or a playlist that we don't support (M3U, EXT_M3U, PLS).
            observer->onPlaylistEntryParsed(
                id,
                urlAndInfo.url,
                urlsToParse.empty() ? avsCommon::utils::playlistParser::PlaylistParseResult::FINISHED
                                    : avsCommon::utils::playlistParser::PlaylistParseResult::STILL_ONGOING,
                urlAndInfo.length);
        }
    }
}

bool PlaylistParser::getPlaylistContent(
    std::unique_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterface> contentFetcher,
    std::string* content) {
    if (!contentFetcher) {
        ACSDK_ERROR(LX("getContentFromPlaylistUrlIntoStringFailed").d("reason", "nullContentFetcher"));
        return false;
    }

    if (!content) {
        ACSDK_ERROR(LX("getContentFromPlaylistUrlIntoStringFailed").d("reason", "nullString"));
        return false;
    }
    auto httpContent =
        contentFetcher->getContent(avsCommon::sdkInterfaces::HTTPContentFetcherInterface::FetchOptions::ENTIRE_BODY);
    if (!httpContent) {
        ACSDK_ERROR(LX("getContentFromPlaylistUrlIntoStringFailed").d("reason", "nullHTTPContentReceived"));
        return false;
    }
    do {
        if (m_shuttingDown) {
            ACSDK_DEBUG9(LX("getContentFromPlaylistUrlIntoStringFailed").d("info", "shuttingDown"));
            return false;
        }
    } while (!httpContent->isReady(WAIT_FOR_FUTURE_READY_TIMEOUT));

    return extractPlaylistContent(std::move(httpContent), content);
}

void PlaylistParser::doShutdown() {
    m_shuttingDown = true;
    m_executor.shutdown();
}

}  // namespace playlistParser
}  // namespace alexaClientSDK
