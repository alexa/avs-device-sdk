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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_PLAYERACTIVITY_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_PLAYERACTIVITY_H_

#include <ostream>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/// Identifies the player state.
enum class PlayerActivity {
    /// Initial state, prior to acting on the first @c Play directive.
    IDLE,

    /// Indicates that audio is currently playing.
    PLAYING,

    /**
     * Indicates that audio playback was stopped due to an error or a directive which stops or replaces the current
     * stream.
     */
    STOPPED,

    /// Indicates that the audio stream has been paused.
    PAUSED,

    /// Indicates that a buffer underrun has occurred and the stream is buffering.
    BUFFER_UNDERRUN,

    /// Indicates that playback has finished.
    FINISHED
};

/*
 * Convert a @c PlayerActivity to an AVS-compliant @c std::string.
 *
 * @param playerActivity The @c PlayerActivity to convert.
 * @return The AVS-compliant string representation of @c playerActivity.
 */
inline std::string playerActivityToString(PlayerActivity playerActivity) {
    switch (playerActivity) {
        case PlayerActivity::IDLE:
            return "IDLE";
        case PlayerActivity::PLAYING:
            return "PLAYING";
        case PlayerActivity::STOPPED:
            return "STOPPED";
        case PlayerActivity::PAUSED:
            return "PAUSED";
        case PlayerActivity::BUFFER_UNDERRUN:
            return "BUFFER_UNDERRUN";
        case PlayerActivity::FINISHED:
            return "FINISHED";
    }
    return "unknown PlayerActivity";
}

/**
 * Write a @c PlayerActivity value to an @c ostream.
 *
 * @param stream The stream to write the value to.
 * @param playerActivity The @c PlayerActivity value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const PlayerActivity& playerActivity) {
    return stream << playerActivityToString(playerActivity);
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_PLAYERACTIVITY_H_
