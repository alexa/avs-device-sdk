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

#ifndef ALEXA_CLIENT_SDK_ACSDKAUDIOPLAYERINTERFACES_INCLUDE_ACSDKAUDIOPLAYERINTERFACES_AUDIOPLAYEROBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_ACSDKAUDIOPLAYERINTERFACES_INCLUDE_ACSDKAUDIOPLAYERINTERFACES_AUDIOPLAYEROBSERVERINTERFACE_H_

#include <chrono>
#include <string>

#include <AVSCommon/AVS/PlayerActivity.h>
#include <AVSCommon/AVS/PlayRequestor.h>

namespace alexaClientSDK {
namespace acsdkAudioPlayerInterfaces {

/**
 * This class allows any observers of the @c AudioPlayer to be notified of changes in the @c AudioPlayer audio state.
 */
class AudioPlayerObserverInterface {
public:
    /**
     * Destructor
     */
    virtual ~AudioPlayerObserverInterface() = default;

    /// The context of the AudioPlayer when the observer is notified of the @c PlayerActivity state change.
    struct Context {
        /// The ID of the @c AudioItem that the @ AudioPlayer is handling.
        std::string audioItemId;

        /// The offset in millisecond from the start of the @c AudioItem.
        std::chrono::milliseconds offset;

        /// The @c PlayRequestor object in @c Play directive.
        avsCommon::avs::PlayRequestor playRequestor;

        /// Track protection name of the @c AudioItem
        std::string trackProtectionName;

        /// Track Playlist Type of the @c AudioItem
        std::string trackPlaylistType;

        /// The start offset in millisecond for the @c AudioItem in @c Play directive
        std::chrono::milliseconds startOffset;

        /// The end offset in millisecond for the @c AudioItem in @c Play directive
        std::chrono::milliseconds endOffset;
    };

    /// Enum representing Seek activities
    enum class SeekStatus {
        /// Initiating a seek to another play position within the current track
        SEEK_START,
        /// Finished seeking within the current track
        SEEK_COMPLETE,
    };

    /**
     * Used to notify the observer when the @c AudioPlayer has a change in @c PlayerActivity.
     *
     * @param state The @c PlayerActivity of the @c AudioPlayer.
     * @param context The @c Context of the @c AudioPlayer.
     */
    virtual void onPlayerActivityChanged(avsCommon::avs::PlayerActivity state, const Context& context) = 0;

    /**
     * Used to notify the observer when the @c AudioPlayer is seeking within the current track.
     *
     * @param seekStatus Type of activity being performed, represented by @c SeekStatus
     * @param context The @c Context of the @c AudioPlayer at the time of the seek activity.
     */
    virtual void onSeekActivity(const SeekStatus seekStatus, const Context& context);
};

inline void AudioPlayerObserverInterface::onSeekActivity(const SeekStatus seekStatus, const Context& context){};

}  // namespace acsdkAudioPlayerInterfaces
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACSDKAUDIOPLAYERINTERFACES_INCLUDE_ACSDKAUDIOPLAYERINTERFACES_AUDIOPLAYEROBSERVERINTERFACE_H_
