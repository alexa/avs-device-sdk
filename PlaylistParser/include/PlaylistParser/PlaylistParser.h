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
     * Retrieves playlist content and stores it into a string.
     *
     * @param contentFetcher Object used to retrieve url content.
     * @param [out] content The playlist content.
     * @return @c true if no error occured or @c false otherwise.
     * @note This function should be used to retrieve content specifically from playlist URLs. Attempting to use this
     * on a media URL could be blocking forever as the URL might point to a live stream.
     */
    bool getPlaylistContent(
        std::unique_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterface> contentFetcher,
        std::string* content);

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
