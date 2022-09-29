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

#include "acsdkAlexaPlaybackControllerInterfaces/PlaybackOperation.h"

namespace alexaClientSDK {
namespace acsdkAlexaPlaybackControllerInterfaces {

std::string playbackOperationToString(PlaybackOperation playbackOperation) {
    switch (playbackOperation) {
        case PlaybackOperation::PLAY:
            return "Play";
        case PlaybackOperation::PAUSE:
            return "Pause";
        case PlaybackOperation::STOP:
            return "Stop";
        case PlaybackOperation::START_OVER:
            return "StartOver";
        case PlaybackOperation::PREVIOUS:
            return "Previous";
        case PlaybackOperation::NEXT:
            return "Next";
        case PlaybackOperation::REWIND:
            return "Rewind";
        case PlaybackOperation::FAST_FORWARD:
            return "FastForward";
    }

    return "";
}

}  // namespace acsdkAlexaPlaybackControllerInterfaces
}  // namespace alexaClientSDK
