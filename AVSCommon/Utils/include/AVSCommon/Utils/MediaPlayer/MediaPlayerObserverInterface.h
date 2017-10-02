/*
 * MediaPlayerObserverInterface.h
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

#ifndef ALEXA_CLIENT_SDK_AVS_COMMON_UTILS_INCLUDE_AVS_COMMON_UTILS_MEDIA_PLAYER_MEDIA_PLAYER_OBSERVER_INTERFACE_H_
#define ALEXA_CLIENT_SDK_AVS_COMMON_UTILS_INCLUDE_AVS_COMMON_UTILS_MEDIA_PLAYER_MEDIA_PLAYER_OBSERVER_INTERFACE_H_

#include <string>

#include <AVSCommon/Utils/MediaPlayer/ErrorTypes.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace mediaPlayer {

/**
 * A player observer will receive notifications when the player starts playing or when it stops playing a stream.
 * A pointer to the @c MediaPlayerObserverInterface needs to be provided to a @c MediaPlayer for it to notify the
 * observer.
 */
class MediaPlayerObserverInterface {
public:
    /**
     * Destructor.
     */
    virtual ~MediaPlayerObserverInterface() = default;

    /**
     * This is an indication to the observer that the @c MediaPlayer has starting playing the audio data.
     *
     * @note The observer has to return soon. Otherwise this could block the @c MediaPlayer from processing other
     * signals or playback.
     */
    virtual void onPlaybackStarted() = 0;

    /**
     * This is an indication to the observer that the @c MediaPlayer has starting finished the audio data.
     *
     * @note The observer has to return soon. Otherwise this could block the @c MediaPlayer from processing other
     * signals or playback.
     */
    virtual void onPlaybackFinished() = 0;

    /**
     * This is an indication to the observer that the @c MediaPlayer encountered an error. Errors can occur during
     * playback.
     *
     * @note The observer has to return soon. Otherwise this could block the @c MediaPlayer from processing other
     * signals or playback.
     *
     * @param type The AVS error to report to AVS
     * @param error The message to report to AVS
     */
    virtual void onPlaybackError(const ErrorType& type, std::string error) = 0;

    /**
     * This is an indication to the observer that the @c MediaPlayer has paused playing the audio data.
     *
     * @note The observer has to return soon. Otherwise this could block the @c MediaPlayer from processing other
     * signals or playback.
     */
    virtual void onPlaybackPaused(){};

    /**
     * This is an indication to the observer that the @c MediaPlayer has resumed playing the audio data.
     *
     * @note The observer has to return soon. Otherwise this could block the @c MediaPlayer from processing other
     * signals or playback.
     */
    virtual void onPlaybackResumed(){};

    /**
     * This is an indication to the observer that the @c MediaPlayer is experiencing a buffer underrun.
     * This will only be sent after playback has started. Playback will be paused until the buffer is filled.
     *
     * @note The observer has to return soon. Otherwise this could block the @c MediaPlayer from processing other
     * signals or playback.
     */
    virtual void onBufferUnderrun() {
    }

    /**
     * This is an indication to the observer that the @c MediaPlayer's buffer has refilled. This will only be sent after
     * playback has started. Playback will resume.
     *
     * @note The observer has to return soon. Otherwise this could block the @c MediaPlayer from processing other
     * signals or playback.
     */
    virtual void onBufferRefilled() {
    }
};

}  // namespace mediaPlayer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVS_COMMON_UTILS_INCLUDE_AVS_COMMON_UTILS_MEDIA_PLAYER_MEDIA_PLAYER_OBSERVER_INTERFACE_H_
