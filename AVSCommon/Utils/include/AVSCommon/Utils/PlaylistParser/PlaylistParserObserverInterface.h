/*
 * PlaylistParserObserverInterface.h
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVS_COMMON_UTILS_INCLUDE_AVS_COMMON_UTILS_PLAYLIST_PARSER_PLAYLIST_PARSER_OBSERVER_INTERFACE_H_
#define ALEXA_CLIENT_SDK_AVS_COMMON_UTILS_INCLUDE_AVS_COMMON_UTILS_PLAYLIST_PARSER_PLAYLIST_PARSER_OBSERVER_INTERFACE_H_

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

    /// The playlist was parsed successfully.
    PARSE_RESULT_SUCCESS,

    /// The playlist could not be handled.
    PARSE_RESULT_UNHANDLED,

    /// There was an error parsing the playlist.
    PARSE_RESULT_ERROR,

    /// The playlist was ignored due to its scheme or MIME type.
    PARSE_RESULT_IGNORED,

    ///Parsing of the playlist was cancelled part-way through.
    PARSE_RESULT_CANCELLED
};

/**
 * An observer of the playlist parser.
 */
class PlaylistParserObserverInterface {
public:
    /**
     * Destructor.
     */
    virtual ~PlaylistParserObserverInterface() = default;

    /**
     * Notification that the playlist parsing has been completed.
     *
     * @param playlistUrl The playlist that was parsed.
     * @param urls A list of the urls extracted from the playlist.
     * @param parseResult The result of parsing the playlist.
     */
    virtual void onPlaylistParsed(
            std::string playlistUrl,
            std::queue<std::string> urls,
            PlaylistParseResult parseResult) = 0;
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
        case PlaylistParseResult::PARSE_RESULT_SUCCESS:
            stream << "PARSE_RESULT_SUCCESS";
            break;
        case PlaylistParseResult::PARSE_RESULT_UNHANDLED:
            stream << "PARSE_RESULT_UNHANDLED";
            break;
        case PlaylistParseResult::PARSE_RESULT_ERROR:
            stream << "PARSE_RESULT_ERROR";
            break;
        case PlaylistParseResult::PARSE_RESULT_IGNORED:
            stream << "PARSE_RESULT_IGNORED";
            break;
        case PlaylistParseResult::PARSE_RESULT_CANCELLED:
            stream << "PARSE_RESULT_CANCELLED";
            break;
    }
    return stream;
}

} // namespace playlistParser
} // namespace utils
} // namespace avsCommon
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_AVS_COMMON_UTILS_INCLUDE_AVS_COMMON_UTILS_PLAYLIST_PARSER_PLAYLIST_PARSER_OBSERVER_INTERFACE_H_
