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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_PLAYLISTPARSER_PLAYLISTPARSEROBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_PLAYLISTPARSER_PLAYLISTPARSEROBSERVERINTERFACE_H_

#include <chrono>
#include <ostream>
#include <queue>
#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace playlistParser {

/**
 * An enum class used to specify the result of a parsing operation.
 */
enum class PlaylistParseResult {

    /// The playlist has been fully parsed successfully. This indicates that parsing of the playlist has completed.
    SUCCESS,

    /**
     * The playlist parsing has encountered an error and will abort parsing. In this case, the url in the callback will
     * not be valid.
     */
    ERROR,

    /// The playlist parsing is still ongoing.
    STILL_ONGOING
};

/**
 * An observer of the playlist parser.
 */
class PlaylistParserObserverInterface {
public:
    /// An invalid duration.
    static constexpr std::chrono::milliseconds INVALID_DURATION = std::chrono::milliseconds(-1);

    /**
     * Destructor.
     */
    virtual ~PlaylistParserObserverInterface() = default;

    /**
     * Notification that an entry has been parsed.
     *
     * @param requestId The id of the callback to connect this callback to an original request.
     * @param url An entry that has been extracted.
     * @param parseResult The result of parsing the playlist.
     * @param duration A duration in milliseconds of the playlist entry, if it was able to be deduced based on
     * available playlist metadata.
     *
     * @note This function is always called from a single thread in PlayListParser.
     */
    virtual void onPlaylistEntryParsed(
        int requestId,
        std::string url,
        PlaylistParseResult parseResult,
        std::chrono::milliseconds duration = INVALID_DURATION) = 0;
};

/**
 * Write a @c PlaylistParseResult value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param result The PlaylistParseResult value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const PlaylistParseResult& result) {
    switch (result) {
        case PlaylistParseResult::SUCCESS:
            stream << "SUCCESS";
            break;
        case PlaylistParseResult::ERROR:
            stream << "ERROR";
            break;
        case PlaylistParseResult::STILL_ONGOING:
            stream << "STILL_ONGOING";
            break;
    }
    return stream;
}

}  // namespace playlistParser
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_PLAYLISTPARSER_PLAYLISTPARSEROBSERVERINTERFACE_H_
