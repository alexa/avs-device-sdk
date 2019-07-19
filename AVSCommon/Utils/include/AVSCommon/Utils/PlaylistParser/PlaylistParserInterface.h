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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_PLAYLISTPARSER_PLAYLISTPARSERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_PLAYLISTPARSER_PLAYLISTPARSERINTERFACE_H_

#include <memory>
#include <string>
#include <vector>

#include "PlaylistParserObserverInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace playlistParser {

/**
 * An interface that can be implemented to parse playlists in a DFS manner.
 */
class PlaylistParserInterface {
public:
    /// The different types of playlists that are currently supported
    enum class PlaylistType {
        M3U,

        EXT_M3U,

        PLS
    };

    /**
     * Destructor.
     */
    virtual ~PlaylistParserInterface() = default;

    /**
     * This function returns immediately. It parses the playlist specified in the @c url asynchronously. The playlist
     * will be parsed in a DFS manner. If the playlist contains a link to another playlist, then it will proceed to
     * parse that before proceeding. Callbacks willbe issued @c PlaylistParserObserverInterface via the @c
     * onPlaylistParsed call whenever a entry has been parsed.
     *
     * @param url The url of the playlist to be parsed.
     * @param observer The observer to be notified of playlist parsing.
     * @param playlistTypesToNotBeParsed The playlist types to skip parsing of.
     * @return 0 if adding a new playlist parsing request to the queue failed or the id of the request otherwise. This
     * id will be included in the callback to notify the observer which original request the callback is referencing.
     */
    virtual int parsePlaylist(
        std::string url,
        std::shared_ptr<PlaylistParserObserverInterface> observer,
        std::vector<PlaylistType> playlistTypesToNotBeParsed = std::vector<PlaylistType>()) = 0;
};

}  // namespace playlistParser
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_PLAYLISTPARSER_PLAYLISTPARSERINTERFACE_H_
