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

#ifndef ALEXA_CLIENT_SDK_PLAYLISTPARSER_INCLUDE_PLAYLISTPARSER_PLAYLISTPARSER_H_
#define ALEXA_CLIENT_SDK_PLAYLISTPARSER_INCLUDE_PLAYLISTPARSER_PLAYLISTPARSER_H_

#include <deque>
#include <memory>
#include <unordered_map>

#include <AVSCommon/AVS/Attachment/AttachmentReader.h>
#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterfaceFactoryInterface.h>
#include <AVSCommon/Utils/PlaylistParser/PlaylistParserInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace playlistParser {

/**
 *
 */
class PlaylistParser
        : public avsCommon::utils::playlistParser::PlaylistParserInterface
        , public avsCommon::utils::RequiresShutdown {
public:
    /**
     * Creates a new @c PlaylistParser instance.
     *
     * @param contentFetcherFactory A factory that can create @c HTTPContentFetcherInterfaces.
     * @return An @c std::unique_ptr to a new @c PlaylistParser if successful or @c nullptr otherwise.
     */
    static std::unique_ptr<PlaylistParser> create(
        std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory);

    int parsePlaylist(
        std::string url,
        std::shared_ptr<avsCommon::utils::playlistParser::PlaylistParserObserverInterface> observer,
        std::vector<PlaylistType> playlistTypesToNotBeParsed = std::vector<PlaylistType>()) override;

    /// A return value that indicates a failure to start the playlist parsing.
    static const int START_FAILURE = 0;

    void doShutdown() override;

private:
    /// A struct to contain a URL encountered in a playlist and metadata surrounding it.
    struct UrlAndInfo {
        std::string url;
        std::chrono::milliseconds length;
    };

    /// A struct used to encapsulate information retrieved from an M3U playlist.
    struct M3UContent {
        std::vector<UrlAndInfo> childrenUrls;
        bool endlistTagPresent;
        bool streamInfTagPresent;
        M3UContent() : endlistTagPresent{false}, streamInfTagPresent{false} {};
    };

    /**
     * Constructor.
     *
     * @param contentFetcherFactory The object that will be used to create objects with which to fetch content from
     * urls.
     */
    PlaylistParser(
        std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory);

    /**
     * Parses the playlist pointed to by the url specified in a depth first search manner.
     *
     * @param id The id of the request.
     * @param observer The observer to notify.
     * @param rootUrl The initial URL to parse.
     */
    void doDepthFirstSearch(
        int id,
        std::shared_ptr<avsCommon::utils::playlistParser::PlaylistParserObserverInterface> observer,
        const std::string& rootUrl,
        std::vector<PlaylistType> playlistTypesToNotBeParsed);

    /**
     * Parses an M3U playlist and returns the "children" URLs in the order they appeared in the playlist.
     *
     * @param content The content to parse.
     * @return A vector of URLs and their relevant metadata in the order they appeared in the playlist.
     */
    static M3UContent parseM3UContent(const std::string& playlistURL, const std::string& content);

    /**
     * Parses an PLS playlist and returns the "children" URLs in the order they appeared in the playlist.
     *
     * @param content The content to parse.
     * @return A vector of URLs in the order they appeared in the playlist.
     */
    static std::vector<std::string> parsePLSContent(const std::string& playlistURL, const std::string& content);

    /**
     * Determines the playlist type of an M3U playlist.
     *
     * @param playlistContent The M3U playlist in @c std::string format.
     * @return @c true if the playlist is M3U8 or @c false otherwise
     */
    static bool isM3UPlaylistM3U8(const std::string& playlistContent);

    /**
     * Removes a carriage return from the end of a line. This is required to handle Windows style line breaks ('\r\n').
     * std::getline reads by default up to '\n' so at times, the '\r' may be included when readinglines.
     *
     * @param line The line to parse
     */
    static void removeCarriageReturnFromLine(std::string* line);

    /**
     * Retrieves content from a URL and stores it into a string.
     *
     * @param url The URL to retrieve from.
     * @param [out] content The playlist content.
     * @return @c true if no error occured or @c false otherwise.
     * @note This function should be used to retrieve content specifically from playlist URLs. Attempting to use this
     * on a media URL could be blocking forever as the URL might point to a live stream.
     */
    bool getContentFromPlaylistUrlIntoString(const std::string& url, std::string* content) const;

    /**
     * Determines whether the provided url is an absolute url as opposed to a relative url. This is done by simply
     * checking to see if the string contains the substring "://".
     *
     * @param url The url to check.
     * @return @c true if the url is an absolute url and @c false otherwise.
     */
    static bool isURLAbsolute(const std::string& url);

    /**
     * Creates an absolute url, given a base url and a relative path from that url. For example, if
     * "www.awesomewebsite.com/music/test.m3u" was the base url and the relative path was "music.mp3",
     * "www.awesomewebsite.com/music/music.mp3" would be returned.
     *
     * @param baseURL The base url to add the relative path to.
     * @param relativePath The relative path to add to the base url.
     * @param [out] absoluteURL The absolute url generated
     * @return @c true If everything was successful and @c false otherwise.
     */
    static bool getAbsoluteURLFromRelativePathToURL(
        std::string baseURL,
        std::string relativePath,
        std::string* absoluteURL);

    /**
     * Used to parse the time length out of a playlist entry metadata line that starts with #EXTINF.
     *
     * @param line The line to parse.
     * @return The time length of the entry or @c PlaylistParserObserverInterface::INVALID_DURATION if the time length
     * was unable to be parsed.
     */
    static std::chrono::milliseconds parseRuntime(std::string line);

    /// Used to retrieve content from URLs
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> m_contentFetcherFactory;

    /// Used to indicate that a shutdown is occurring.
    std::atomic<bool> m_shuttingDown;

    /**
     * @c Executor which queues up operations from asynchronous API calls.
     *
     * @note This declaration needs to come *after* the Executor Thread Variables above so that the thread shuts down
     *     before the Executor Thread Variables are destroyed.
     */
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace playlistParser
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_PLAYLISTPARSER_INCLUDE_PLAYLISTPARSER_PLAYLISTPARSER_H_
