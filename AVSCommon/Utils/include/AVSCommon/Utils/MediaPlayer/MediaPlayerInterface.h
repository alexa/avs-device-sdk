/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_MEDIAPLAYERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_MEDIAPLAYERINTERFACE_H_

#include <chrono>
#include <cstdint>
#include <future>
#include <memory>

#include "AVSCommon/AVS/Attachment/AttachmentReader.h"
#include "AVSCommon/Utils/AudioFormat.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace mediaPlayer {

/// Represents offset returned when MediaPlayer is in an invalid state.
static const std::chrono::milliseconds MEDIA_PLAYER_INVALID_OFFSET{-1};

/// Forward-declare the observer class.
class MediaPlayerObserverInterface;

/**
 * A @c MediaPlayerInterface allows for sourcing, playback control, navigation, and querying the state of media content.
 * A @c MediaPlayerInterface implementation must only handle one source at a time.
 *
 * Each playback controlling API call (i.e. @c play(), @c pause(), @c stop(),  @c resume()) that succeeds will also
 * result in a callback to the observer. To see how to tell when a method succeeded, please refer to the documentation
 * of each method.
 *
 * An implementation can call @c onPlaybackError() at any time.  If an @c onPlaybackError() callback occurs while a
 * plaback controlling API call is waiting for a callback, the original callback must not be made, and the
 * implementation should revert to a stopped state.  Any subsequent operations after an @c onPlaybackError() callback
 * must be preceded by a new @c setSource() call.
 *
 * Implementations must make a call to @c onPlaybackStopped() with the previous @c SourceId when a new source is
 * set if the previous source was in a non-stopped state.
 *
 * @c note A @c MediaPlayerInterface implementation must be able to support the various audio formats listed at:
 * https://developer.amazon.com/docs/alexa-voice-service/recommended-media-support.html.
 */
class MediaPlayerInterface {
public:
    /// A type that identifies which source is currently being operated on.
    using SourceId = uint64_t;

    /// An @c SourceId used to represent an error from calls to @c setSource().
    static const SourceId ERROR = 0;

    /**
     * Destructor.
     */
    virtual ~MediaPlayerInterface() = default;

    /**
     * Set an @c AttachmentReader source to play. The source should be set before making calls to any of the playback
     * control APIs. If any source was set prior to this call, that source will be discarded.
     *
     * @note A @c MediaPlayerInterface implementation must handle only one source at a time. An implementation must call
     * @c MediaPlayerObserverInterface::onPlaybackStopped() with the previous source's id if there was a source set.
     *
     * @param attachmentReader Object with which to read an incoming audio attachment.
     * @param format The audioFormat to be used to interpret raw audio data.
     * @return The @c SourceId that represents the source being handled as a result of this call. @c ERROR will be
     *     returned if the source failed to be set.
     */
    virtual SourceId setSource(
        std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader,
        const avsCommon::utils::AudioFormat* format = nullptr) = 0;

    /**
     * Set a url source to play. The source should be set before making calls to any of the playback control APIs. If
     * any source was set prior to this call, that source will be discarded.
     *
     * @note A @c MediaPlayerInterface implementation must handle only one source at a time. An implementation must call
     * @c MediaPlayerObserverInterface::onPlaybackStopped() with the previous source's id if there was a source set.
     *
     * @param url The url to set as the source.
     * @param offset An optional offset parameter to start playing from when a @c play() call is made.
     * @param repeat An optional parameter to play the url source in a loop.
     *
     * @return The @c SourceId that represents the source being handled as a result of this call. @c ERROR will be
     *     returned if the source failed to be set.
     */
    virtual SourceId setSource(
        const std::string& url,
        std::chrono::milliseconds offset = std::chrono::milliseconds::zero(),
        bool repeat = false) = 0;

    /**
     * Set an @c istream source to play. The source should be set before making calls to any of the playback control
     * APIs. If any source was set prior to this call, that source will be discarded.
     *
     * @note A @c MediaPlayerInterface implementation must handle only one source at a time. An implementation must call
     * @c MediaPlayerObserverInterface::onPlaybackStopped() with the previous source's id if there was a source set.
     *
     * @param stream Object from which to read an incoming audio stream.
     * @param repeat Whether the audio stream should be played in a loop until stopped.
     *
     * @return The @c SourceId that represents the source being handled as a result of this call. @c ERROR will be
     * returned if the source failed to be set.
     */
    virtual SourceId setSource(std::shared_ptr<std::istream> stream, bool repeat) = 0;

    /**
     * Starts playing audio specified by the @c setSource() call.
     *
     * The source must be set before issuing @c play().
     *
     * If @c play() is called
     * @li without making a @c setSource(), @c false will be returned.
     * @li when audio is already playing, @c false will be returned.
     * @li after a play() call has already been made but no callback or return code has been issued
     *     yet, @c false will be returned.
     *
     * If the id does not match the id of the active source, then @c false will be returned.
     * If the @c play() succeeded, @c true will be returned.
     * When @c true is returned, a callback will be made to either @c MediaPlayerObserverInterface::onPlaybackStarted()
     * or to @c MediaPlayerObserverInterface::onPlaybackError().
     *
     * @param id The id of the source on which to operate.
     *
     * @return @c true if the call succeeded, in which case a callback will be made, or @c false otherwise.
     */
    virtual bool play(SourceId id) = 0;

    /**
     * Stops playing the audio specified by the @c setSource() call.
     *
     * The source must be set before issuing @c stop().
     *
     * Once @c stop() has been called, subsequent @c play() calls will fail.
     * If @c stop() is called when audio has already stopped, @c false will be returned.
     * If the id does not match the id of the active source, then @c false will be returned.
     * If the @c stop() succeeded, @c true will be returned.
     * When @c true is returned, a callback will be made to either @c MediaPlayerObserverInterface::onPlaybackStopped()
     * or to @c MediaPlayerObserverInterface::onPlaybackError().
     *
     * @param id The id of the source on which to operate.
     *
     * @return @c true if the call succeeded, in which case a callback will be made, or @c false otherwise.
     */
    virtual bool stop(SourceId id) = 0;

    /**
     * Pauses playing audio specified by the @c setSource() call.
     *
     * The source must be set before issuing @c pause().
     * If @c pause() is called
     * @li without making a @c setSource(), @c false will be returned.
     * @li when audio is not starting/resuming/playing, @c false will be returned.
     * @li when a play() or resume() call has already been made, but no callback has been issued
     *     yet for those functions, the audio stream will pause without playing any audio.  Implementations must call
     *     both @c MediaPlayerObserverInterface::onPlaybackStarted() /
     *     @c MediaPlayerObserverInterface::onPlaybackResumed and @c MediaPlayerObserverInterface::onPlaybackPaused()
     *     in this scenario, as both the @c play() / @c resume() and the @c pause() are required to have corresponding
     *     callbacks.
     *
     * If the id does not match the id of the active source, then @c false will be returned.
     * If the @c pause() succeeded, @c true will be returned.
     * When @c true is returned, a callback will be made to either @c MediaPlayerObserverInterface::onPlaybackPaused()
     * or to @c MediaPlayerObserverInterface::onPlaybackError().
     *
     * @param id The id of the source on which to operate.
     *
     * @return @c true if the call succeeded, in which case a callback will be made, or @c false otherwise.
     */
    virtual bool pause(SourceId id) = 0;

    /**
     * Resumes playing the paused audio specified by the @c setSource() call.
     *
     * The source must be set before issuing @c resume().
     * If @c resume() is called
     * @li without making a @c setSource(), @c false will be returned.
     * @li when audio is already playing, @c false will be returned.
     * @li when audio is not paused, @c false will be returned.
     * @li after a resume() call has already been made but no callback or return code has been issued yet, @c false will
     *     be returned.
     *
     * If the id does not match the id of the active source, then @c false will be returned.
     * If the @c resume() succeeded, @c true will be returned.
     * When @c true is returned, a callback will be made to either @c MediaPlayerObserverInterface::onPlaybackResumed()
     * or to @c MediaPlayerObserverInterface::onPlaybackError().
     *
     * @param id The id of the source on which to operate.
     *
     * @return @c true if the call succeeded, in which case a callback will be made, or @c false otherwise.
     */
    virtual bool resume(SourceId id) = 0;

    /**
     * Returns the offset, in milliseconds, of the media source.
     *
     * @param id The id of the source on which to operate.
     *
     * @return If the specified source is playing, the offset in milliseconds that the source has been playing
     *      will be returned. If the specified source is not playing, the last offset it played will be returned.
     */
    virtual std::chrono::milliseconds getOffset(SourceId id) = 0;

    /**
     * Returns the number of bytes queued up in the media player buffers.
     *
     * @return The number of bytes currently queued in this MediaPlayer's buffer.
     */
    virtual uint64_t getNumBytesBuffered() = 0;

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

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_MEDIAPLAYERINTERFACE_H_
