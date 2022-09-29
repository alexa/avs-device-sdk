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

#ifndef ACSDKALEXAPLAYBACKCONTROLLERINTERFACES_PLAYBACKOPERATION_H_
#define ACSDKALEXAPLAYBACKCONTROLLERINTERFACES_PLAYBACKOPERATION_H_

#include <string>

#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace acsdkAlexaPlaybackControllerInterfaces {

/**
 * The PlaybackOperation enumeration carries out playback controller actions such as play, pause and stop audio or video
 * content.
 */
enum class PlaybackOperation {
    /// Play audio or video content
    PLAY,

    /// Pause audio or video content
    PAUSE,

    /// Stop audio or video content
    STOP,

    /// Restart audio or video content
    START_OVER,

    /// Go to the previous audio or video content
    PREVIOUS,

    /// Go to the next audio or video content
    NEXT,

    /// Rewind the audio or video content
    REWIND,

    /// Fast forward the audio or video content
    FAST_FORWARD
};

/**
 * Convert a @c PlaybackOperation enum to its corresponding string value. Note that any @c PlaybackOperation that does
 * not map to @c PlaybackOperation enum value will return an empty string.
 *
 * @param playbackOperation The playback operation value to convert.
 * @return The corresponding string value for @c PlaybackOperation. Returns an empty string if @c PlaybackOperation does
 * not map to a @c PlaybackOperation enum.
 */
std::string playbackOperationToString(PlaybackOperation playbackOperation);

}  // namespace acsdkAlexaPlaybackControllerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKALEXAPLAYBACKCONTROLLERINTERFACES_PLAYBACKOPERATION_H_
