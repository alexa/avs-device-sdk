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

#include "PlaylistParser/M3UParser.h"
#include "PlaylistParser/PlaylistParser.h"
#include "PlaylistParser/PlaylistUtils.h"

namespace alexaClientSDK {
namespace playlistParser {

using namespace avsCommon::utils::playlistParser;

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

/// An invalid duration.
static const auto INVALID_DURATION = std::chrono::milliseconds(-1);

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
    std::shared_ptr<PlaylistParserObserverInterface> observer,
    const std::string& rootUrl,
    std::vector<PlaylistType> playlistTypesToNotBeParsed) {
    /*
     * A depth first search, as follows:
     * 1. Push root to vector.
     * 2. While vector isn't empty, pop from front and push children, in the order they appeared, to front of vector.
     */
    std::deque<PlayItem> playQueue;
    playQueue.push_front(rootUrl);
    std::string lastUrlParsed;
    while (!playQueue.empty() && !m_shuttingDown) {
        auto playItem = playQueue.front();
        playQueue.pop_front();
        if (playItem.type == PlayItem::Type::MEDIA_INFO) {
            // This is a media URL and not a playlist
            ACSDK_DEBUG9(LX("foundNonPlaylistURL"));
            observer->onPlaylistEntryParsed(id, playItem.playlistEntry);
            continue;
        }
        auto playlistURL = playItem.playlistURL;
        auto contentFetcher = m_contentFetcherFactory->create(playlistURL);
        auto httpContent = contentFetcher->getContent(
            avsCommon::sdkInterfaces::HTTPContentFetcherInterface::FetchOptions::CONTENT_TYPE);
        if (!httpContent) {
            ACSDK_ERROR(LX("doDepthFirstSearchFailed").d("reason", "nullHTTPContentReceived"));
            observer->onPlaylistEntryParsed(id, PlaylistEntry::createErrorEntry(playlistURL));
            return;
        }
        do {
            if (m_shuttingDown) {
                ACSDK_DEBUG9(LX("doDepthFirstSearch").d("info", "shuttingDown"));
                return;
            }
        } while (!httpContent->isReady(WAIT_FOR_FUTURE_READY_TIMEOUT));
        if (!httpContent->isStatusCodeSuccess()) {
            ACSDK_ERROR(LX("doDepthFirstSearchFailed")
                            .d("reason", "badHTTPContentReceived")
                            .d("statusCode", httpContent->getStatusCode()));
            observer->onPlaylistEntryParsed(id, PlaylistEntry::createErrorEntry(playlistURL));
            return;
        }

        std::string contentType = httpContent->getContentType();
        ACSDK_DEBUG9(LX("gotContentType").d("contentType", contentType).sensitive("url", playlistURL));
        std::transform(contentType.begin(), contentType.end(), contentType.begin(), ::tolower);
        // Checking the HTML content type to see if the URL is a playlist.
        if (contentType.find(M3U_CONTENT_TYPE) != std::string::npos) {
            std::string playlistContent;
            if (!getPlaylistContent(m_contentFetcherFactory->create(playlistURL), &playlistContent)) {
                ACSDK_ERROR(LX("failedToRetrieveContent").sensitive("url", playlistURL));
                observer->onPlaylistEntryParsed(id, PlaylistEntry::createErrorEntry(playlistURL));
                return;
            }
            // This playlist may either be M3U or EXT_M3U so some additional parsing is required.
            bool isExtendedM3U = isPlaylistExtendedM3U(playlistContent);
            if (isExtendedM3U) {
                ACSDK_DEBUG9(LX("isM3UExtendedPlaylist").sensitive("url", playlistURL));
            } else {
                ACSDK_DEBUG9(LX("isPlainM3UPlaylist").sensitive("url", playlistURL));
            }
            // TODO: investigate refactoring the two std::finds into a common place
            if (std::find(
                    playlistTypesToNotBeParsed.begin(),
                    playlistTypesToNotBeParsed.end(),
                    isExtendedM3U ? PlaylistType::EXT_M3U : PlaylistType::M3U) != playlistTypesToNotBeParsed.end()) {
                auto parseResult =
                    playQueue.empty() ? PlaylistParseResult::FINISHED : PlaylistParseResult::STILL_ONGOING;
                observer->onPlaylistEntryParsed(id, PlaylistEntry(playlistURL, INVALID_DURATION, parseResult));
                continue;
            }
            auto m3uContent = parseM3UContent(playlistURL, playlistContent);
            if (m3uContent.empty()) {
                ACSDK_ERROR(LX("doDepthFirstSearchFailed").d("reason", "noChildrenURLs"));
                observer->onPlaylistEntryParsed(id, PlaylistEntry::createErrorEntry(playlistURL));
                return;
            }
            ACSDK_DEBUG9((
                LX("foundChildrenURLsInPlaylist").d("num", m3uContent.variantURLs.size() + m3uContent.entries.size())));
            if (isExtendedM3U) {
                if (m3uContent.isMasterPlaylist()) {
                    // This is the Master Playlist and that only one URL should be chosen from here
                    ACSDK_DEBUG9(LX("encounteredMasterPlaylist").sensitive("url", playlistURL));
                    // Because we don't do any selective choosing based on bitrates or codecs, only push the first URL
                    // as a default.
                    playQueue.push_front(m3uContent.variantURLs.front());
                } else {
                    auto entries = m3uContent.entries;
                    // lastUrlParsed is set when we actually parse some urls from the playlist - here, it is our first
                    // pass at this playlist
                    if (lastUrlParsed.empty()) {
                        for (auto it = entries.begin(); it != entries.end(); ++it) {
                            observer->onPlaylistEntryParsed(id, *it);
                        }
                        lastUrlParsed = entries.back().url;
                    } else {
                        // Setting this to 0 as an intial value so that if we don't see the last URL we parsed in the
                        // latest pass of the playlist, we stream all the URLs within the playlist as a sort of
                        // recovery mechanism. This way, if we parse this so far into the future that all the URLs we
                        // had previously seen are gone, we'll still stream the latest URLs.
                        int startPointForNewURLsAdded = 0;
                        for (int i = entries.size() - 1; i >= 0; --i) {
                            if (entries.at(i).url == lastUrlParsed) {
                                // We need to add the URLs past this point
                                startPointForNewURLsAdded = i + 1;
                            }
                        }
                        for (unsigned i = startPointForNewURLsAdded; i < entries.size(); ++i) {
                            ACSDK_DEBUG9(LX("foundNewURLInLivePlaylist"));
                            observer->onPlaylistEntryParsed(id, entries.at(i));
                        }
                        lastUrlParsed = entries.back().url;
                    }
                    if (m3uContent.isLive) {
                        ACSDK_DEBUG9(LX("encounteredLiveHLSPlaylist")
                                         .sensitive("url", playlistURL)
                                         .d("info", "willRetryURLInFuture"));
                        /*
                         * Because this URL represents a live playlist which can have additional chunks added to it, we
                         * need to make a request to this URL again in the future to continue playback of additional
                         * chunks that get added.
                         */
                        playQueue.push_back(playlistURL);
                    }
                }
            } else {
                auto entries = m3uContent.entries;
                for (auto reverseIt = entries.rbegin(); reverseIt != entries.rend(); ++reverseIt) {
                    playQueue.push_front(reverseIt->url);
                }
            }
        } else if (contentType.find(PLS_CONTENT_TYPE) != std::string::npos) {
            ACSDK_DEBUG9(LX("isPLSPlaylist").sensitive("url", playlistURL));
            /*
             * This is for sure a PLS playlist, so if PLS is one of the desired playlist types to not be parsed, then
             * notify and return immediately.
             */
            if (std::find(playlistTypesToNotBeParsed.begin(), playlistTypesToNotBeParsed.end(), PlaylistType::PLS) !=
                playlistTypesToNotBeParsed.end()) {
                auto parseResult =
                    playQueue.empty() ? PlaylistParseResult::FINISHED : PlaylistParseResult::STILL_ONGOING;
                observer->onPlaylistEntryParsed(id, PlaylistEntry(playlistURL, INVALID_DURATION, parseResult));
                continue;
            }
            std::string playlistContent;
            if (!getPlaylistContent(m_contentFetcherFactory->create(playlistURL), &playlistContent)) {
                observer->onPlaylistEntryParsed(id, PlaylistEntry::createErrorEntry(playlistURL));
                return;
            }
            auto childrenUrls = parsePLSContent(playlistURL, playlistContent);
            if (childrenUrls.empty()) {
                observer->onPlaylistEntryParsed(id, PlaylistEntry::createErrorEntry(playlistURL));
                return;
            }
            for (auto reverseIt = childrenUrls.rbegin(); reverseIt != childrenUrls.rend(); ++reverseIt) {
                playQueue.push_front(*reverseIt);
            }
        } else {
            ACSDK_DEBUG9(LX("foundNonPlaylistURL"));
            // This is a non-playlist URL or a playlist that we don't support (M3U, EXT_M3U, PLS).
            auto parseResult = playQueue.empty() ? PlaylistParseResult::FINISHED : PlaylistParseResult::STILL_ONGOING;
            observer->onPlaylistEntryParsed(id, PlaylistEntry(playlistURL, INVALID_DURATION, parseResult));
        }
    }
}

bool PlaylistParser::getPlaylistContent(
    std::unique_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterface> contentFetcher,
    std::string* content) {
    if (!contentFetcher) {
        ACSDK_ERROR(LX("getPlaylistContentFailed").d("reason", "nullContentFetcher"));
        return false;
    }

    if (!content) {
        ACSDK_ERROR(LX("getPlaylistContentFailed").d("reason", "nullString"));
        return false;
    }
    auto httpContent =
        contentFetcher->getContent(avsCommon::sdkInterfaces::HTTPContentFetcherInterface::FetchOptions::ENTIRE_BODY);
    if (!httpContent) {
        ACSDK_ERROR(LX("getPlaylistContentFailed").d("reason", "nullHTTPContentReceived"));
        return false;
    }
    do {
        if (m_shuttingDown) {
            ACSDK_DEBUG9(LX("getPlaylistContentFailed").d("info", "shuttingDown"));
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
