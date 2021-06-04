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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_MEDIAPLAYERSTATE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_MEDIAPLAYERSTATE_H_

#include <AVSCommon/Utils/Optional.h>

#include <chrono>
#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace mediaPlayer {

const std::chrono::milliseconds DURATION_UNKNOWN = std::chrono::milliseconds(-1);

/**
 * Structure to hold metadata about the MediaPlayerState
 */
struct MediaPlayerState {
    /**
     * Structure to hold media protection information.
     */
    struct MediaPlayerProtection {
        /// Default Constructor
        MediaPlayerProtection() : clearLead(false) {
        }

        /// Constructor
        MediaPlayerProtection(const std::string& protectionScheme, bool clearLead) :
                protectionScheme(protectionScheme),
                clearLead(clearLead) {
        }

        /**
         * Overload the == operator for equality checks
         *
         * @param other The other @c MediaPlayerProtection to compare
         * @return Whether @c this is equivalent to @c other
         */
        bool operator==(const MediaPlayerProtection& other) const {
            return (protectionScheme == other.protectionScheme && clearLead == other.clearLead);
        }

        /// Is the track protected
        bool isProtected() const {
            return protectionScheme.size() != 0;
        }

        /**
         * Name of the protection scheme used to protect media.
         * This field will be empty if no protection scheme is used.
         */
        std::string protectionScheme;

        /**
         * Whether we found some clear lead data, possibly to mitigate latency, even though most of the
         * content was protected.
         */
        bool clearLead;
    };

    /**
     * Default Constructor, initializes the offset to zero.
     */
    MediaPlayerState() : offset(std::chrono::milliseconds::zero()), duration(DURATION_UNKNOWN) {
    }

    /**
     * Constructor.
     */
    MediaPlayerState(std::chrono::milliseconds offsetInMs, std::chrono::milliseconds duration_ = DURATION_UNKNOWN) :
            offset(offsetInMs),
            duration(duration_) {
    }

    /**
     * Constructor.
     */
    MediaPlayerState(
        std::chrono::milliseconds offsetInMs,
        const MediaPlayerProtection& mediaPlayerProtection_,
        std::chrono::milliseconds duration_ = DURATION_UNKNOWN) :
            offset(offsetInMs),
            duration(duration_),
            mediaPlayerProtection(mediaPlayerProtection_) {
    }

    /**
     * Constructor.
     */
    MediaPlayerState(
        std::chrono::milliseconds offsetInMs,
        const MediaPlayerProtection& mediaPlayerProtection_,
        const std::string& playlistType_,
        std::chrono::milliseconds duration_ = DURATION_UNKNOWN) :
            offset(offsetInMs),
            duration(duration_),
            mediaPlayerProtection(mediaPlayerProtection_),
            playlistType(playlistType_) {
    }

    /// Offset in milliseconds
    std::chrono::milliseconds offset;

    /// Duration
    std::chrono::milliseconds duration;

    /// Optional: MediaPlayerProtection information.
    Optional<MediaPlayerProtection> mediaPlayerProtection;

    /**
     * Optional: Playlist type, HLS, DASH, etc., of the current track.
     * Empty when adpative streaming is not in use for the current track.
     */
    Optional<std::string> playlistType;

    /**
     * Overload the == operator for equality checks
     *
     * @param other The other @c MediaPlayerState to compare
     * @return Whether @c this is equivalent to @c other
     */
    bool operator==(const MediaPlayerState& other) const {
        return offset == other.offset && duration == other.duration &&
               mediaPlayerProtection == other.mediaPlayerProtection && playlistType == other.playlistType;
    }
};

/**
 * Write a @c MediaPlayerProtection value to an @c ostream.
 *
 * @param stream The stream to write the value to.
 * @param mediaPlayerProtection The @c MediaPlayerProtection value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(
    std::ostream& stream,
    const MediaPlayerState::MediaPlayerProtection& mediaPlayerProtection) {
    stream << "MediaPlayerProtection: isProtected=" << std::boolalpha << mediaPlayerProtection.isProtected();
    if (mediaPlayerProtection.isProtected()) {
        stream << ",protectionScheme=" << mediaPlayerProtection.protectionScheme << ",clearLead=" << std::boolalpha
               << mediaPlayerProtection.clearLead;
    }
    return stream;
}

}  // namespace mediaPlayer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_MEDIAPLAYERSTATE_H_
