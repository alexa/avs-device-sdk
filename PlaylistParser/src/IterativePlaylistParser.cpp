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

#include <algorithm>
#include <sstream>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/PlaylistParser/PlaylistParserObserverInterface.h>
#include <AVSCommon/Utils/String/StringUtils.h>

#include "PlaylistParser/IterativePlaylistParser.h"
#include "PlaylistParser/PlaylistUtils.h"

namespace alexaClientSDK {
namespace playlistParser {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::playlistParser;
using namespace avsCommon::utils::string;

/// String to identify log entries originating from this file.
static const std::string TAG("IterativePlaylistParser");

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

/// An invalid duration.
static const auto INVALID_DURATION = std::chrono::milliseconds(-1);

std::unique_ptr<IterativePlaylistParser> IterativePlaylistParser::create(
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory) {
    if (!contentFetcherFactory) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullContentFetcherFactory"));
        return nullptr;
    }
    return std::unique_ptr<IterativePlaylistParser>(new IterativePlaylistParser(contentFetcherFactory));
}

bool IterativePlaylistParser::initializeParsing(std::string url) {
    if (url.empty()) {
        ACSDK_ERROR(LX("initializeParsingFailed").d("reason", "emptyUrl"));
        return false;
    }

    m_abort = false;
    m_lastUrl.clear();
    m_playQueue.clear();
    m_playQueue.push_back(PlayItem(url));
    return true;
}

IterativePlaylistParser::IterativePlaylistParser(
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory) :
        m_contentFetcherFactory{contentFetcherFactory},
        m_abort{false} {
}

PlaylistEntry IterativePlaylistParser::next() {
    while (!m_playQueue.empty() && !m_abort) {
        auto playItem = m_playQueue.front();
        m_playQueue.pop_front();
        if (playItem.type == PlayItem::Type::MEDIA_INFO) {
            // This is a media URL and not a playlist
            ACSDK_DEBUG9(LX("foundMediaURL"));
            return playItem.playlistEntry;
        }
        auto playlistURL = playItem.playlistURL;
        auto contentFetcher = m_contentFetcherFactory->create(playItem.playlistURL);
        if (!contentFetcher) {
            ACSDK_ERROR(LX("nextFailed").d("reason", "createContentFetcherFailed").sensitive("url", playlistURL));
            return PlaylistEntry::createErrorEntry(playlistURL);
        }

        contentFetcher->getContent(HTTPContentFetcherInterface::FetchOptions::ENTIRE_BODY);
        auto header = contentFetcher->getHeader(&m_abort);
        if (m_abort) {
            ACSDK_DEBUG9(LX("nextFailed").d("info", "aborting"));
            break;
        }

        if (!header.successful || !isStatusCodeSuccess(header.responseCode)) {
            ACSDK_ERROR(LX("nextFailed")
                            .d("reason", "badHTTPContentReceived")
                            .d("statusCode", header.responseCode)
                            .sensitive("url", playlistURL));
            return PlaylistEntry::createErrorEntry(playlistURL);
        }

        ACSDK_DEBUG9(LX("contentReceived").d("contentType", header.contentType).sensitive("url", playlistURL));
        std::string lowerCaseContentType = stringToLowerCase(header.contentType);
        // Checking the HTML content type to see if the URL is a playlist.
        if (lowerCaseContentType.find(M3U_CONTENT_TYPE) != std::string::npos) {
            std::string playlistContent;
            auto contentFetcher = m_contentFetcherFactory->create(playlistURL);
            contentFetcher->getContent(HTTPContentFetcherInterface::FetchOptions::ENTIRE_BODY);
            if (!readFromContentFetcher(std::move(contentFetcher), &playlistContent, &m_abort)) {
                ACSDK_ERROR(LX("nextFailed").d("reason", "failedToRetrieveContent").sensitive("url", playlistURL));
                return PlaylistEntry::createErrorEntry(playlistURL);
            }
            // This playlist may either be M3U or EXT_M3U so some additional parsing is required.
            bool isExtendedM3U = isPlaylistExtendedM3U(playlistContent);
            if (isExtendedM3U) {
                ACSDK_DEBUG9(LX("isExtendedM3U").sensitive("url", playlistURL));
            } else {
                ACSDK_DEBUG9(LX("isPlainM3UPlaylist").sensitive("url", playlistURL));
            }
            auto m3uContent = parseM3UContent(playlistURL, playlistContent);
            if (m3uContent.empty()) {
                ACSDK_ERROR(LX("nextFailed").d("reason", "noChildrenURLs"));
                return PlaylistEntry::createErrorEntry(playlistURL);
            }
            ACSDK_DEBUG9((
                LX("foundChildrenURLsInPlaylist").d("num", m3uContent.entries.size() + m3uContent.variantURLs.size())));
            if (isExtendedM3U) {
                if (m3uContent.isMasterPlaylist()) {
                    // Indicates that we found a Variant Stream and that only one URL should be chosen from here
                    ACSDK_DEBUG9(LX("encounteredVariantStream").sensitive("url", playlistURL));
                    // Because we don't do any selective choosing based on bitrates or codecs, only push the first
                    // URL as a default.
                    m_playQueue.push_front(PlayItem(m3uContent.variantURLs.front()));
                } else {
                    auto entries = m3uContent.entries;
                    // m_lastUrl is set when we actually parse some urls from the playlist - here, it is our
                    // first pass at this playlist
                    if (m_lastUrl.empty()) {
                        for (auto reverseIt = entries.rbegin(); reverseIt != entries.rend(); ++reverseIt) {
                            m_playQueue.push_front(*reverseIt);
                        }
                        m_lastUrl = entries.back().url;
                    } else {
                        // Setting this to 0 as an intial value so that if we don't see the last URL we parsed in
                        // the latest pass of the playlist, we stream all the URLs within the playlist as a sort of
                        // recovery mechanism. This way, if we parse this so far into the future that all the URLs
                        // we had previously seen are gone, we'll still stream the latest URLs.
                        int startPointForNewURLsAdded = 0;
                        for (int i = entries.size() - 1; i >= 0; --i) {
                            if (entries.at(i).url == m_lastUrl) {
                                // We need to add the URLs past this point
                                startPointForNewURLsAdded = i + 1;
                            }
                        }
                        for (int i = entries.size() - 1; i >= startPointForNewURLsAdded; --i) {
                            ACSDK_DEBUG9(LX("foundNewURLInLivePlaylist"));
                            auto entry = entries.at(i);
                            m_playQueue.push_front(entry);
                        }
                        m_lastUrl = entries.back().url;
                    }
                    if (m3uContent.isLive) {
                        ACSDK_DEBUG9(LX("encounteredLiveHLSPlaylist")
                                         .sensitive("url", playlistURL)
                                         .d("info", "willRetryURLInFuture"));
                        /*
                         * Because this URL represents a live playlist which can have additional chunks added to it,
                         * we need to make a request to this URL again in the future to continue playback of
                         * additional chunks that get added.
                         */
                        m_playQueue.push_back(playlistURL);
                    }
                }
            } else {
                auto entries = m3uContent.entries;
                for (auto reverseIt = entries.rbegin(); reverseIt != entries.rend(); ++reverseIt) {
                    m_playQueue.push_front(*reverseIt);
                }
            }
        } else if (lowerCaseContentType.find(PLS_CONTENT_TYPE) != std::string::npos) {
            ACSDK_DEBUG9(LX("isPLSPlaylist").sensitive("url", playlistURL));
            std::string playlistContent;
            auto contentFetcher = m_contentFetcherFactory->create(playlistURL);
            contentFetcher->getContent(HTTPContentFetcherInterface::FetchOptions::ENTIRE_BODY);
            if (!readFromContentFetcher(std::move(contentFetcher), &playlistContent, &m_abort)) {
                ACSDK_ERROR(LX("nextFailed").d("reason", "failedToRetrieveContent").sensitive("url", playlistURL));
                return PlaylistEntry::createErrorEntry(playlistURL);
            }
            auto childrenUrls = parsePLSContent(playlistURL, playlistContent);
            if (childrenUrls.empty()) {
                ACSDK_ERROR(LX("nextFailed").d("reason", "noChildrenURLs"));
                return PlaylistEntry::createErrorEntry(playlistURL);
            }
            for (auto reverseIt = childrenUrls.rbegin(); reverseIt != childrenUrls.rend(); ++reverseIt) {
                auto parseResult = (reverseIt == childrenUrls.rbegin()) ? PlaylistParseResult::FINISHED
                                                                        : PlaylistParseResult::STILL_ONGOING;
                m_playQueue.push_front(PlaylistEntry(*reverseIt, INVALID_DURATION, parseResult));
            }
        } else {
            ACSDK_DEBUG9(LX("foundNonPlaylistURL"));
            // This is a non-playlist URL or a playlist that we don't support (M3U, EXT_M3U, PLS).
            auto parseResult = m_playQueue.empty() ? PlaylistParseResult::FINISHED : PlaylistParseResult::STILL_ONGOING;
            return PlaylistEntry(playlistURL, INVALID_DURATION, parseResult);
        }
    }
    ACSDK_DEBUG0(LX("nextFailed").d("reason", "parseAborted"));
    return PlaylistEntry::createErrorEntry("");
}

void IterativePlaylistParser::abort() {
    m_abort = true;
}

}  // namespace playlistParser
}  // namespace alexaClientSDK
