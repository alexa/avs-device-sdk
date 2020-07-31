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

#include <chrono>

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

    /// Offset in milliseconds
    std::chrono::milliseconds offset;

    /// Duration
    std::chrono::milliseconds duration;

    /**
     * Overload the == operator for equality checks
     *
     * @param other The other @c MediaPlayerState to compare
     * @return Whether @c this is equivalent to @c other
     */
    bool operator==(const MediaPlayerState& other) const {
        return offset == other.offset;
    }
};

}  // namespace mediaPlayer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_MEDIAPLAYERSTATE_H_
