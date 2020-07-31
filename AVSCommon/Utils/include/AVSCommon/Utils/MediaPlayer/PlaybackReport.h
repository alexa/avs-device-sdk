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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_PLAYBACKREPORT_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_PLAYBACKREPORT_H_

#include <chrono>

#include "AVSCommon/Utils/MediaPlayer/PlaybackAttributes.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace mediaPlayer {

/**
 * Struct to hold playback reports for adaptive bitrate schemes such as MPEG-DASH.
 * This struct is used for reporting the various playback options (that is
 * part of an adaptive bitrate scheme such as MPEG-DASH manifest) for Premium Audio. For MPEG-DASH, the
 * PlaybackReport represents a subset of the data in an AdaptationSet.
 */
struct PlaybackReport {
    /// Start offset for playback being reported.
    std::chrono::milliseconds startOffset;

    /// End offset for playback being reported.
    std::chrono::milliseconds endOffset;

    /// @c PlaybackAttributes for the stream being reported.
    PlaybackAttributes playbackAttributes;
};

}  // namespace mediaPlayer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_PLAYBACKREPORT_H_
