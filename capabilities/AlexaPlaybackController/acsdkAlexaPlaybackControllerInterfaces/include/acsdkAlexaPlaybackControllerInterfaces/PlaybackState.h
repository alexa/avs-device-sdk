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

#ifndef ACSDKALEXAPLAYBACKCONTROLLERINTERFACES_PLAYBACKSTATE_H_
#define ACSDKALEXAPLAYBACKCONTROLLERINTERFACES_PLAYBACKSTATE_H_

#include <string>

#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace acsdkAlexaPlaybackControllerInterfaces {

/**
 * The enumeration representing the playback status of the media played on a device reported understood by Alexa
 * interfaces. https://developer.amazon.com/en-US/docs/alexa/device-apis/alexa-property-schemas.html#playbackstate
 */
enum class PlaybackState {
    /// Media content is playing
    PLAYING,

    /// Media content is paused
    PAUSED,

    /// Media content is not playing
    STOPPED
};

/**
 * Convert a @c PlaybackState enum to its corresponding string value. Note that any @c PlaybackState that does not map
 * to @c PlaybackState enum value will return an empty string.
 *
 * @param playbackState The playback state value to convert.
 * @return The corresponding string value for @c PlaybackState. Returns an empty string if @c PlaybackState does not map
 * to a @c PlaybackState enum.
 */
std::string playbackStateToString(PlaybackState playbackState);

}  // namespace acsdkAlexaPlaybackControllerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKALEXAPLAYBACKCONTROLLERINTERFACES_PLAYBACKSTATE_H_
