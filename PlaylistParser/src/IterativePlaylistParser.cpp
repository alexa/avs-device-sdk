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

#include <algorithm>
#include <sstream>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/PlaylistParser/PlaylistParserObserverInterface.h>

#include "PlaylistParser/IterativePlaylistParser.h"

namespace alexaClientSDK {
namespace playlistParser {

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

static const std::chrono::milliseconds INVALID_DURATION =
    avsCommon::utils::playlistParser::PlaylistParserObserverInterface::INVALID_DURATION;

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
        ACSDK_ERROR(LX("parsePlaylistFailed").d("reason", "emptyUrl"));
        return false;
    }

    m_abort = false;
    m_lastUrl.clear();
    m_urlsToParse.clear();
    m_urlsToParse.push_back({url, INVALID_DURATION});
    return true;
}

IterativePlaylistParser::IterativePlaylistParser(
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory) :
        m_contentFetcherFactory{contentFetcherFactory},
        m_abort{false} {
}

IterativePlaylistParser::PlaylistEntry IterativePlaylistParser::next() {
    while (!m_urlsToParse.empty() && !m_abort) {
        auto urlAndInfo = m_urlsToParse.front();
        m_urlsToParse.pop_front();
        if (urlAndInfo.length != INVALID_DURATION) {
            // This is a media URL and not a playlist
            ACSDK_DEBUG9(LX("foundMediaURL"));
            return PlaylistEntry{
                .url = urlAndInfo.url,
                .duration = urlAndInfo.length,
                .parseResult = m_urlsToParse.empty()
                                   ? avsCommon::utils::playlistParser::PlaylistParseResult::FINISHED
                                   : avsCommon::utils::playlistParser::PlaylistParseResult::STILL_ONGOING};
        }
        auto contentFetcher = m_contentFetcherFactory->create(urlAndInfo.url);
        if (!contentFetcher) {
            ACSDK_ERROR(LX("nextFailed").d("reason", "createContentFetcherFailed").sensitive("url", urlAndInfo.url));
            return PlaylistEntry{.url = urlAndInfo.url,
                                 .duration = urlAndInfo.length,
                                 .parseResult = avsCommon::utils::playlistParser::PlaylistParseResult::ERROR};
        }

        auto httpContent = contentFetcher->getContent(
            avsCommon::sdkInterfaces::HTTPContentFetcherInterface::FetchOptions::CONTENT_TYPE);
        if (!httpContent) {
            ACSDK_ERROR(LX("getHTTPContent").d("reason", "nullHTTPContentReceived").sensitive("url", urlAndInfo.url));
            return PlaylistEntry{.url = urlAndInfo.url,
                                 .duration = urlAndInfo.length,
                                 .parseResult = avsCommon::utils::playlistParser::PlaylistParseResult::ERROR};
        }
        do {
            if (m_abort) {
                ACSDK_DEBUG9(LX("nextFailed").d("info", "aborting"));
                break;
            }
        } while (!httpContent->isReady(WAIT_FOR_FUTURE_READY_TIMEOUT));
        if (!(*httpContent)) {
            ACSDK_ERROR(LX("getHTTPContent").d("reason", "badHTTPContentReceived").sensitive("url", urlAndInfo.url));
            return PlaylistEntry{.url = urlAndInfo.url,
                                 .duration = urlAndInfo.length,
                                 .parseResult = avsCommon::utils::playlistParser::PlaylistParseResult::ERROR};
        }

        std::string contentType = httpContent->contentType.get();
        ACSDK_DEBUG9(LX("IterativePlaylistParser")
                         .d("contentType", contentType)
                         .sensitive("url", urlAndInfo.url)
                         .d("length", urlAndInfo.length.count()));
        std::transform(contentType.begin(), contentType.end(), contentType.begin(), ::tolower);
        // Checking the HTML content type to see if the URL is a playlist.
        if (contentType.find(M3U_CONTENT_TYPE) != std::string::npos) {
            std::string playlistContent;
            if (!getPlaylistContent(m_contentFetcherFactory->create(urlAndInfo.url), &playlistContent)) {
                ACSDK_ERROR(LX("failedToRetrieveContent").sensitive("url", urlAndInfo.url));
                return PlaylistEntry{.url = urlAndInfo.url,
                                     .duration = urlAndInfo.length,
                                     .parseResult = avsCommon::utils::playlistParser::PlaylistParseResult::ERROR};
            }
            // This playlist may either be M3U or EXT_M3U so some additional parsing is required.
            bool isExtendedM3U = isPlaylistExtendedM3U(playlistContent);
            if (isExtendedM3U) {
                ACSDK_DEBUG9(LX("isExtendedM3U").sensitive("url", urlAndInfo.url));
            } else {
                ACSDK_DEBUG9(LX("isPlainM3UPlaylist").sensitive("url", urlAndInfo.url));
            }
            auto M3UContent = parseM3UContent(urlAndInfo.url, playlistContent);
            const auto& childrenUrls = M3UContent.childrenUrls;
            if (childrenUrls.empty()) {
                ACSDK_ERROR(LX("noChildrenURLs"));
                return PlaylistEntry{.url = urlAndInfo.url,
                                     .duration = urlAndInfo.length,
                                     .parseResult = avsCommon::utils::playlistParser::PlaylistParseResult::ERROR};
            }
            ACSDK_DEBUG9((LX("foundChildrenURLsInPlaylist").d("num", childrenUrls.size())));
            if (isExtendedM3U) {
                if (M3UContent.streamInfTagPresent) {
                    // Indicates that we found a Variant Stream and that only one URL should be chosen from here
                    ACSDK_DEBUG9(LX("encounteredVariantStream").sensitive("url", urlAndInfo.url));
                    // Because we don't do any selective choosing based on bitrates or codecs, only push the first
                    // URL as a default.
                    m_urlsToParse.push_front(childrenUrls.front());
                } else {
                    // m_lastUrl is set when we actually parse some urls from the playlist - here, it is our
                    // first pass at this playlist
                    if (m_lastUrl.empty()) {
                        for (auto reverseIt = childrenUrls.rbegin(); reverseIt != childrenUrls.rend(); ++reverseIt) {
                            m_urlsToParse.push_front(*reverseIt);
                        }
                        m_lastUrl = childrenUrls.back().url;
                    } else {
                        // Setting this to 0 as an intial value so that if we don't see the last URL we parsed in
                        // the latest pass of the playlist, we stream all the URLs within the playlist as a sort of
                        // recovery mechanism. This way, if we parse this so far into the future that all the URLs
                        // we had previously seen are gone, we'll still stream the latest URLs.
                        int startPointForNewURLsAdded = 0;
                        for (int i = childrenUrls.size() - 1; i >= 0; --i) {
                            if (childrenUrls.at(i).url == m_lastUrl) {
                                // We need to add the URLs past this point
                                startPointForNewURLsAdded = i + 1;
                            }
                        }
                        for (int i = childrenUrls.size() - 1; i >= startPointForNewURLsAdded; --i) {
                            ACSDK_DEBUG9(LX("foundNewURLInLivePlaylist"));
                            m_urlsToParse.push_front(childrenUrls.at(i));
                            m_lastUrl = m_urlsToParse.back().url;
                        }
                    }
                    if (!M3UContent.endlistTagPresent) {
                        ACSDK_DEBUG9(LX("encounteredLiveHLSPlaylist")
                                         .sensitive("url", urlAndInfo.url)
                                         .d("info", "willRetryURLInFuture"));
                        /*
                         * Because this URL represents a live playlist which can have additional chunks added to it,
                         * we need to make a request to this URL again in the future to continue playback of
                         * additional chunks that get added.
                         */
                        m_urlsToParse.push_back(urlAndInfo);
                    }
                }
            } else {
                for (auto reverseIt = childrenUrls.rbegin(); reverseIt != childrenUrls.rend(); ++reverseIt) {
                    m_urlsToParse.push_front(*reverseIt);
                }
            }
        } else if (contentType.find(PLS_CONTENT_TYPE) != std::string::npos) {
            ACSDK_DEBUG9(LX("isPLSPlaylist").sensitive("url", urlAndInfo.url));
            std::string playlistContent;
            if (!getPlaylistContent(m_contentFetcherFactory->create(urlAndInfo.url), &playlistContent)) {
                return PlaylistEntry{.url = urlAndInfo.url,
                                     .duration = urlAndInfo.length,
                                     .parseResult = avsCommon::utils::playlistParser::PlaylistParseResult::ERROR};
            }
            auto childrenUrls = parsePLSContent(urlAndInfo.url, playlistContent);
            if (childrenUrls.empty()) {
                return PlaylistEntry{.url = urlAndInfo.url,
                                     .duration = urlAndInfo.length,
                                     .parseResult = avsCommon::utils::playlistParser::PlaylistParseResult::ERROR};
            }
            for (auto reverseIt = childrenUrls.rbegin(); reverseIt != childrenUrls.rend(); ++reverseIt) {
                m_urlsToParse.push_front({*reverseIt, INVALID_DURATION});
            }
        } else {
            ACSDK_DEBUG9(LX("foundNonPlaylistURL"));
            // This is a non-playlist URL or a playlist that we don't support (M3U, EXT_M3U, PLS).
            return PlaylistEntry{
                .url = urlAndInfo.url,
                .duration = urlAndInfo.length,
                .parseResult = m_urlsToParse.empty()
                                   ? avsCommon::utils::playlistParser::PlaylistParseResult::FINISHED
                                   : avsCommon::utils::playlistParser::PlaylistParseResult::STILL_ONGOING};
        }
    }
    ACSDK_DEBUG0(LX("nextFailed").d("reason", "parseAborted"));
    return PlaylistEntry{.url = "",
                         .duration = INVALID_DURATION,
                         .parseResult = avsCommon::utils::playlistParser::PlaylistParseResult::ERROR};
}

bool IterativePlaylistParser::getPlaylistContent(
    std::unique_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterface> contentFetcher,
    std::string* content) {
    if (!contentFetcher) {
        ACSDK_ERROR(LX("getContentFromPlaylistUrlIntoStringFailed").d("reason", "nullContentFetcher"));
        return false;
    }

    auto httpContent =
        contentFetcher->getContent(avsCommon::sdkInterfaces::HTTPContentFetcherInterface::FetchOptions::ENTIRE_BODY);
    if (!httpContent) {
        ACSDK_ERROR(LX("getContentFromPlaylistUrlIntoStringFailed").d("reason", "nullHTTPContentReceived"));
        return false;
    }
    do {
        if (m_abort) {
            ACSDK_DEBUG9(LX("getContentFromPlaylistUrlIntoStringFailed").d("reason", "abortParsing"));
            return false;
        }
    } while (!httpContent->isReady(WAIT_FOR_FUTURE_READY_TIMEOUT));

    return extractPlaylistContent(std::move(httpContent), content);
}

void IterativePlaylistParser::abort() {
    m_abort = true;
}

}  // namespace playlistParser
}  // namespace alexaClientSDK
