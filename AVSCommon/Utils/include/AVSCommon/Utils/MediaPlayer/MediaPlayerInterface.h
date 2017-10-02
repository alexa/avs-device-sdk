/*
 * MediaPlayerInterface.h
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

#ifndef ALEXA_CLIENT_SDK_AVS_COMMON_UTILS_INCLUDE_AVS_COMMON_UTILS_MEDIA_PLAYER_MEDIA_PLAYER_INTERFACE_H_
#define ALEXA_CLIENT_SDK_AVS_COMMON_UTILS_INCLUDE_AVS_COMMON_UTILS_MEDIA_PLAYER_MEDIA_PLAYER_INTERFACE_H_

#include <chrono>
#include <cstdint>
#include <future>
#include <memory>

#include "AVSCommon/AVS/Attachment/AttachmentReader.h"
#include "MediaPlayerObserverInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace mediaPlayer {

/**
 * An enum class used to specify the status of an operation performed by the @c MediaPlayer.
 */
enum class MediaPlayerStatus {
    /// The operation was successful.
    SUCCESS,

    /// The operation is pending. If there is an error, it may be notified via onPlaybackError.
    PENDING,

    /// An error was encountered and the operation failed.
    FAILURE
};

/// Represents offset returned when MediaPlayer is in an invalid state.
static const std::chrono::milliseconds MEDIA_PLAYER_INVALID_OFFSET{-1};

/**
 * A MediaPlayer allows for sourcing, playback control, navigation, and querying the state of media content.
 */
class MediaPlayerInterface {
public:
    /**
     * Destructor.
     */
    virtual ~MediaPlayerInterface() = default;

    /**
     * Set the source to play. The source should be set before issuing @c play or @c stop.
     *
     * The @c MediaPlayer can handle only one source at a time.
     *
     * @param attachmentReader Object with which to read an incoming audio attachment.
     *
     * @return @c SUCCESS if the source was set successfully else @c FAILURE. If setSource is called when audio is
     * currently playing, the playing audio will be stopped and the source set to the new value. If there is an error
     * stopping the player, this will return @c FAILURE.
     */
    virtual MediaPlayerStatus setSource(
        std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader) = 0;

    /**
     * Set the source to play. The source should be set before issuing @c play or @c stop.
     *
     * The @c MediaPlayer can handle only one source at a time.
     *
     * @param url The url to set as the source.
     *
     * @return @c SUCCESS if the source was set successfully else @c FAILURE. If setSource is called when audio is
     * currently playing, the playing audio will be stopped and the source set to the new value. If there is an error
     * stopping the player, this will return @c FAILURE.
     */
    virtual MediaPlayerStatus setSource(const std::string& url) = 0;

    /**
     * Set the source to play. The source should be set before issuing @c play or @c stop.
     *
     * The @c MediaPlayer can handle only one source at a time.
     *
     * @param stream Object with which to read an incoming audio stream.
     * @param repeat Whether the audio stream should be played in a loop until stopped.
     *
     * @return @c SUCCESS if the the source was set successfully else @c FAILURE. If setSource is called when audio is
     * currently playing, the playing audio will be stopped and the source set to the new value. If there is an error
     * stopping the player, this will return @c FAILURE.
     */
    virtual MediaPlayerStatus setSource(std::shared_ptr<std::istream> stream, bool repeat) = 0;

    /**
     * Set the offset for playback. A seek will be performed to the offset at the next @c play() command.
     *
     * The following situations will reset the offset:
     * # A seek attempt is made (ie. via play()).
     * # A new source is set.
     *
     * @param offset The offset in milliseconds to seek to.
     *
     * @return @c SUCCESS if the offset was successfully set, and FAILURE for any error.
     */
    virtual MediaPlayerStatus setOffset(std::chrono::milliseconds offset) {
        return MediaPlayerStatus::FAILURE;
    }

    /**
     * Start playing audio. The source should be set before issuing @c play. If @c play is called without
     * setting source, it will return an error. If @c play is called when audio is already playing,
     * there is no effect. Status returned will be @c SUCCESS.
     * If @c play is called again after @c stop on the same source, then the audio plays from the beginning.
     *
     * @return @c SUCCESS if the state transition to play was successful. If state transition is pending then it returns
     * @c PENDING and the state transition status is notified via @c onPlaybackStarted or @c onPlaybackError. If state
     * transition was unsuccessful, returns @c FAILURE.
     */
    virtual MediaPlayerStatus play() = 0;

    /**
     * Stop playing the audio. Once audio has been stopped, starting playback again will start from the beginning.
     * The source should be set before issuing @c stop. If @c stop is called without setting source, it will
     * return an error.
     * If @c stop is called when audio has already stopped, there is no effect. Status returned will be @c SUCCESS.
     *
     * @return @c SUCCESS if the state transition to stop was successful. If state transition is pending then it returns
     * @c PENDING and the state transition status is notified via @c onPlaybackStarted or @c onPlaybackError. If state
     * transition was unsuccessful, returns @c FAILURE.
     */
    virtual MediaPlayerStatus stop() = 0;

    /**
     * Pause playing the audio. Once audio has been paused, calling @c resume() will start the audio.
     * The source should be set before issuing @c pause. If @c pause is called without setting source, it will
     * return an error.
     * Calling @c pause will only have an effect when audio is currently playing. Calling @c pause in all other states
     * will have no effect, and result in a return of @c FAILURE.
     *
     * @return @c SUCCESS if the state transition to pause was successful. If state transition is pending then it
     * returns
     * @c PENDING and the state transition status is notified via @c onPlaybackPaused or @c onPlaybackError. If state
     * transition was unsuccessful, returns @c FAILURE.
     */
    virtual MediaPlayerStatus pause() = 0;

    /**
     * Resume playing the paused audio. The source should be set before issuing @c resume. If @c resume is called
     * without setting source, it will return an error. Calling @c resume will only have an effect when audio is
     * currently paused. Calling @c resume in other states will have no effect, and result in a return of @c FAILURE.
     *
     * @return @c SUCCESS if the state transition to play was successful. If state transition is pending then it returns
     * @c PENDING and the state transition status is notified via @c onPlaybackResumed or @c onPlaybackError. If state
     * transition was unsuccessful, returns @c FAILURE.
     */
    virtual MediaPlayerStatus resume() = 0;

    /**
     * Returns the offset, in milliseconds, of the media stream.
     *
     * @return If a stream is playing, the offset in milliseconds that the stream has been playing,
     * if there is no stream playing it returns @c MEDIA_PLAYER_INVALID_OFFSET.
     */
    virtual std::chrono::milliseconds getOffset() = 0;

    /**
     * Sets an observer to be notified when playback state changes.
     *
     * @param playerObserver The observer to send the notifications to.
     */
    virtual void setObserver(
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface> playerObserver) = 0;
};

}  // namespace mediaPlayer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVS_COMMON_UTILS_INCLUDE_AVS_COMMON_UTILS_MEDIA_PLAYER_MEDIA_PLAYER_INTERFACE_H_
