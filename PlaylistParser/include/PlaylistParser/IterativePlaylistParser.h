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

#ifndef ALEXA_CLIENT_SDK_PLAYLISTPARSER_INCLUDE_PLAYLISTPARSER_ITERATIVEPLAYLISTPARSER_H_
#define ALEXA_CLIENT_SDK_PLAYLISTPARSER_INCLUDE_PLAYLISTPARSER_ITERATIVEPLAYLISTPARSER_H_

#include <deque>
#include <memory>

#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterfaceFactoryInterface.h>
#include <AVSCommon/Utils/PlaylistParser/IterativePlaylistParserInterface.h>
#include <AVSCommon/Utils/PlaylistParser/PlaylistEntry.h>

#include "PlaylistParser/M3UParser.h"

namespace alexaClientSDK {
namespace playlistParser {

/**
 * This playlist parser implements an iterative depth first search algorithm to find audio urls inside a playlist url.
 * Each call to next will perform the search until it hits a leaf (a valid media). When the playlist is empty, it will
 * set the parse result to SUCCESS.
 *
 * This class is not thread safe except for method @c abort(). This method can be called while another thread is running
 * and it will cause ongoing and future calls to @c next to fail. Calls to @c initializeParsing will reset abort state.
 */
class IterativePlaylistParser : public avsCommon::utils::playlistParser::IterativePlaylistParserInterface {
public:
    /**
     * Creates a new @c IterativePlaylistParser instance.
     *
     * @param contentFetcherFactory A factory that can create @c HTTPContentFetcherInterfaces.
     * @return An @c std::unique_ptr to a new @c IterativePlaylistParser if successful or @c nullptr otherwise.
     */
    static std::unique_ptr<IterativePlaylistParser> create(
        std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory);

    /// @name IterativePlaylistParserInterface methods.
    /// @{
    bool initializeParsing(std::string url) override;
    avsCommon::utils::playlistParser::PlaylistEntry next() override;
    void abort() override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param contentFetcherFactory The object that will be used to create objects with which to fetch content from
     * urls.
     */
    IterativePlaylistParser(
        std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory);

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

    /// A queue with items to either a playlist url to parse or media info to play. This object is used to save the
    /// traversal state.
    std::deque<PlayItem> m_playQueue;

    /// A string to the last url parsed.
    std::string m_lastUrl;

    /// A flag used to abort an ongoing playlist parsing.
    std::atomic<bool> m_abort;
};

}  // namespace playlistParser
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_PLAYLISTPARSER_INCLUDE_PLAYLISTPARSER_ITERATIVEPLAYLISTPARSER_H_
