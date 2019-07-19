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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_PLAYLISTPARSER_ITERATIVEPLAYLISTPARSERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_PLAYLISTPARSER_ITERATIVEPLAYLISTPARSERINTERFACE_H_

#include <memory>
#include <string>
#include <vector>

#include "PlaylistParserObserverInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace playlistParser {

/**
 * An interface that can be implemented to parse playlists in a depth first search manner.
 */
class IterativePlaylistParserInterface {
public:
    /**
     * Initialize the playlist parsing. Once succesfully initialized, the method @c next can be used to retrieve
     * playlist entries.
     *
     * @param url The root url that can either point to one media file or a playlist to be parsed.
     * @return @c true if it succeeds; false otherwise.
     */
    virtual bool initializeParsing(std::string url) = 0;

    /**
     * Get the next element in the playlist.
     *
     * @return The parsing result. The @c url field will be valid if @c parseResult is different than ERROR.
     */
    virtual PlaylistEntry next() = 0;

    /**
     * Abort the current playlist parsing by causing ongoing and future calls to @c next to fail. Calls to
     * @c initializeParsing will reset @c abort.
     */
    virtual void abort() = 0;

    /**
     * Destructor.
     */
    virtual ~IterativePlaylistParserInterface() = default;
};

}  // namespace playlistParser
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_PLAYLISTPARSER_ITERATIVEPLAYLISTPARSERINTERFACE_H_
