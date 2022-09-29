/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_ERRORTYPES_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_ERRORTYPES_H_

#include <ostream>
#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace mediaPlayer {

/// Identifies the specific type of error in a @c PlaybackFailed event.
enum class ErrorType {
    /// None
    NONE,
    /// An unknown error occurred.
    MEDIA_ERROR_UNKNOWN,
    /// The server recognized the request as being malformed (bad request, unauthorized, forbidden, not found, etc).
    MEDIA_ERROR_INVALID_REQUEST,
    /// The client was unable to reach the service.
    MEDIA_ERROR_SERVICE_UNAVAILABLE,
    /// The server accepted the request, but was unable to process the request as expected.
    MEDIA_ERROR_INTERNAL_SERVER_ERROR,
    /// There was an internal error on the client.
    MEDIA_ERROR_INTERNAL_DEVICE_ERROR,
    /// There was an bad request
    MEDIA_ERROR_HTTP_BAD_REQUEST,
    /// Request is unauthorized
    MEDIA_ERROR_HTTP_UNAUTHORIZED,
    /// Request is forbidden
    MEDIA_ERROR_HTTP_FORBIDDEN,
    /// Http resource not found
    MEDIA_ERROR_HTTP_NOT_FOUND,
    /// There was a conflict
    MEDIA_ERROR_HTTP_CONFLICT,
    /// Range is invalid
    MEDIA_ERROR_HTTP_INVALID_RANGE,
    /// There were too many requests
    MEDIA_ERROR_HTTP_TOO_MANY_REQUESTS,
    /// Bad Gateway
    MEDIA_ERROR_HTTP_BAD_GATEWAY,
    /// Service is unavailable
    MEDIA_ERROR_HTTP_SERVICE_UNAVAILABLE,
    /// Gateway timeout occurred
    MEDIA_ERROR_HTTP_GATEWAY_TIMEOUT,
    /// There was a timeout
    MEDIA_ERROR_TIMED_OUT,
    /// URL was malformed
    MEDIA_ERROR_URL_MALFORMED,
    /// Couldn't resolve host
    MEDIA_ERROR_COULD_NOT_RESOLVE_HOST,
    /// Coudln't connect to server
    MEDIA_ERROR_COULD_NOT_CONNECT,
    /// The resource is not seekable
    MEDIA_ERROR_NOT_SEEKABLE,
    /// Unsupported error
    MEDIA_ERROR_UNSUPPORTED,
    /// Text file, hence not playable
    MEDIA_ERROR_POSSIBLY_TEXT,
    /// IO error
    MEDIA_ERROR_IO,
    /// Invalid command
    MEDIA_ERROR_INVALID_COMMAND,
    /// Playlist error occurred
    MEDIA_ERROR_PLAYLIST_ERROR,
    /// Decryption Flow failure
    MEDIA_ERROR_DECRYPTION_FLOW
};

/**
 * Convert an @c ErrorType to an AVS-compliant @c std::string.
 *
 * @param errorType The @c ErrorType to convert.
 * @return The AVS-compliant string representation of @c errorType.
 */
inline std::string errorTypeToString(ErrorType errorType) {
    switch (errorType) {
        case ErrorType::NONE:
            return "NONE";
        case ErrorType::MEDIA_ERROR_UNKNOWN:
            return "MEDIA_ERROR_UNKNOWN";
        case ErrorType::MEDIA_ERROR_INVALID_REQUEST:
            return "MEDIA_ERROR_INVALID_REQUEST";
        case ErrorType::MEDIA_ERROR_SERVICE_UNAVAILABLE:
            return "MEDIA_ERROR_SERVICE_UNAVAILABLE";
        case ErrorType::MEDIA_ERROR_INTERNAL_SERVER_ERROR:
            return "MEDIA_ERROR_INTERNAL_SERVER_ERROR";
        case ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR:
            return "MEDIA_ERROR_INTERNAL_DEVICE_ERROR";
        case ErrorType::MEDIA_ERROR_HTTP_BAD_REQUEST:
            return "MEDIA_ERROR_HTTP_BAD_REQUEST";
        case ErrorType::MEDIA_ERROR_HTTP_UNAUTHORIZED:
            return "MEDIA_ERROR_HTTP_UNAUTHORIZED";
        case ErrorType::MEDIA_ERROR_HTTP_FORBIDDEN:
            return "MEDIA_ERROR_HTTP_FORBIDDEN";
        case ErrorType::MEDIA_ERROR_HTTP_NOT_FOUND:
            return "MEDIA_ERROR_HTTP_NOT_FOUND";
        case ErrorType::MEDIA_ERROR_HTTP_CONFLICT:
            return "MEDIA_ERROR_HTTP_CONFLICT";
        case ErrorType::MEDIA_ERROR_HTTP_INVALID_RANGE:
            return "MEDIA_ERROR_HTTP_INVALID_RANGE";
        case ErrorType::MEDIA_ERROR_HTTP_TOO_MANY_REQUESTS:
            return "MEDIA_ERROR_HTTP_TOO_MANY_REQUESTS";
        case ErrorType::MEDIA_ERROR_HTTP_BAD_GATEWAY:
            return "MEDIA_ERROR_HTTP_BAD_GATEWAY";
        case ErrorType::MEDIA_ERROR_HTTP_SERVICE_UNAVAILABLE:
            return "MEDIA_ERROR_HTTP_SERVICE_UNAVAILABLE";
        case ErrorType::MEDIA_ERROR_HTTP_GATEWAY_TIMEOUT:
            return "MEDIA_ERROR_HTTP_GATEWAY_TIMEOUT";
        case ErrorType::MEDIA_ERROR_TIMED_OUT:
            return "MEDIA_ERROR_TIMED_OUT";
        case ErrorType::MEDIA_ERROR_URL_MALFORMED:
            return "MEDIA_ERROR_URL_MALFORMED";
        case ErrorType::MEDIA_ERROR_COULD_NOT_RESOLVE_HOST:
            return "MEDIA_ERROR_COULD_NOT_RESOLVE_HOST";
        case ErrorType::MEDIA_ERROR_COULD_NOT_CONNECT:
            return "MEDIA_ERROR_COULD_NOT_CONNECT";
        case ErrorType::MEDIA_ERROR_NOT_SEEKABLE:
            return "MEDIA_ERROR_NOT_SEEKABLE";
        case ErrorType::MEDIA_ERROR_UNSUPPORTED:
            return "MEDIA_ERROR_UNSUPPORTED";
        case ErrorType::MEDIA_ERROR_POSSIBLY_TEXT:
            return "MEDIA_ERROR_TEXT_PLAYBACK";
        case ErrorType::MEDIA_ERROR_IO:
            return "MEDIA_ERROR_IO";
        case ErrorType::MEDIA_ERROR_INVALID_COMMAND:
            return "MEDIA_ERROR_INVALID_COMMAND";
        case ErrorType::MEDIA_ERROR_PLAYLIST_ERROR:
            return "MEDIA_ERROR_PLAYLIST_ERROR";
        case ErrorType::MEDIA_ERROR_DECRYPTION_FLOW:
            return "MEDIA_ERROR_DECRYPTION_FLOW";
    }
    return "unknown ErrorType";
}

/**
 * Write an @c ErrorType value to an @c ostream.
 *
 * @param stream The stream to write the value to.
 * @param errorType The @c ErrorType value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const ErrorType& errorType) {
    return stream << errorTypeToString(errorType);
}

}  // namespace mediaPlayer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_ERRORTYPES_H_
