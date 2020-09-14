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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_MEDIAPLAYEROBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_MEDIAPLAYEROBSERVERINTERFACE_H_

#include <string>
#include <vector>
#include <memory>

#include <AVSCommon/Utils/MediaPlayer/ErrorTypes.h>

#include "MediaPlayerInterface.h"
#include "MediaPlayerState.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace mediaPlayer {

/**
 * A player observer will receive notifications when the player starts playing or when it stops playing a stream.
 * A pointer to the @c MediaPlayerObserverInterface needs to be provided to a @c MediaPlayer for it to notify the
 * observer.
 *
 * @warning An observer should never call a method from the observed media player while handling a callback.
 * This may cause a deadlock while trying to re-acquire a mutex.
 *
 * @warning Be aware that there is a high risk of deadlock if the observer calls media player functions while holding
 * an exclusive lock. The deadlock may happen because the call to media player functions may end up calling the same
 * observer which will try to acquire the same lock that it already has. One way to avoid this issue is by using a
 * recursive lock.
 */
class MediaPlayerObserverInterface {
public:
    /// A type that identifies which source is currently being operated on.
    using SourceId = MediaPlayerInterface::SourceId;

    /// The different types of metadata "stream tags".
    enum class TagType { STRING, UINT, INT, DOUBLE, BOOLEAN };

    /**
     * Structure to hold the key, value and type of tag that is found.
     */
    struct TagKeyValueType {
        /// Key extracted from the stream tag.
        std::string key;
        /// Value extracted from the stream tag.
        std::string value;
        /// Type of the stream tag.
        MediaPlayerObserverInterface::TagType type;
    };

    typedef std::vector<TagKeyValueType> VectorOfTags;

    /**
     * Destructor.
     */
    virtual ~MediaPlayerObserverInterface() = default;

    /**
     * This is an indication to the observer that the @c MediaPlayer has read its first byte of data.
     *
     * @param id The id of the source to which this callback corresponds to.
     * @param state Metadata about the media player state
     */
    virtual void onFirstByteRead(SourceId id, const MediaPlayerState& state) = 0;

    /**
     * This is an indication to the observer that the @c MediaPlayer has started playing the source specified by
     * the id.
     *
     * @note The observer must quickly return to quickly from this callback. Failure to do so could block the @c
     * MediaPlayer from further processing.
     *
     * @param id The id of the source to which this callback corresponds to.
     * @param state Metadata about the media player state
     */
    virtual void onPlaybackStarted(SourceId id, const MediaPlayerState& state) = 0;

    /**
     * This is an indication to the observer that the @c MediaPlayer finished the source.
     *
     * @note The observer must quickly return to quickly from this callback. Failure to do so could block the @c
     * MediaPlayer from further processing.
     *
     * @param id The id of the source to which this callback corresponds to.
     * @param state Metadata about the media player state
     */
    virtual void onPlaybackFinished(SourceId id, const MediaPlayerState& state) = 0;

    /**
     * This is an indication to the observer that the @c MediaPlayer encountered an error. Errors can occur during
     * playback.
     *
     * @note The observer must quickly return from this callback. Failure to do so could block the @c MediaPlayer from
     * further processing.
     *
     * @param id The id of the source to which this callback corresponds to.
     * @param type The type of error encountered by the @c MediaPlayerInterface.
     * @param error The error encountered by the @c MediaPlayerInterface.
     * @param state Metadata about the media player state
     */
    virtual void onPlaybackError(
        SourceId id,
        const ErrorType& type,
        std::string error,
        const MediaPlayerState& state) = 0;

    /**
     * This is an indication to the observer that the @c MediaPlayer has paused playing the source.
     *
     * @note The observer must quickly return from this callback. Failure to do so could block the @c MediaPlayer from
     * further processing.

     * @param SourceId The id of the source to which this callback corresponds to.
     * @param MediaPlayerState Metadata about the media player state
     */
    virtual void onPlaybackPaused(SourceId, const MediaPlayerState&){};

    /**
     * This is an indication to the observer that the @c MediaPlayer has resumed playing the source.
     *
     * @note The observer must quickly return from this callback. Failure to do so could block the @c MediaPlayer from
     * further processing.
     *
     * @param SourceId The id of the source to which this callback corresponds to.
     * @param MediaPlayerState Metadata about the media player state
     */
    virtual void onPlaybackResumed(SourceId, const MediaPlayerState&){};

    /**
     * This is an indication to the observer that the @c MediaPlayer has stopped the source.
     *
     * @note The observer must quickly return from this callback. Failure to do so could block the @c MediaPlayer from
     * further processing.
     *
     * @param SourceId The id of the source to which this callback corresponds to.
     * @param MediaPlayerState Metadata about the media player state
     */
    virtual void onPlaybackStopped(SourceId, const MediaPlayerState&){};

    /**
     * This is an indication to the observer that the @c MediaPlayer is experiencing a buffer underrun.
     * This will only be sent after playback has started. Playback will be paused until the buffer is filled.
     *
     * @note The observer must quickly return from this callback. Failure to do so could block the @c MediaPlayer from
     * further processing.
     *
     * @param SourceId The id of the source to which this callback corresponds to.
     * @param MediaPlayerState Metadata about the media player state
     */
    virtual void onBufferUnderrun(SourceId, const MediaPlayerState&) {
    }

    /**
     * This is an indication to the observer that the @c MediaPlayer's buffer has refilled. This will only be sent after
     * playback has started. Playback will resume.
     *
     * @note The observer must quickly return from this callback. Failure to do so could block the @c MediaPlayer from
     * further processing.
     *
     * @param SourceId The id of the source to which this callback corresponds to.
     * @param MediaPlayerState Metadata about the media player state
     */
    virtual void onBufferRefilled(SourceId, const MediaPlayerState&) {
    }

    /**
     * This is an indication to the observer that the @c MediaPlayer has completed buffering of the source specified by
     * the id.  This can be sent anytime after a source is set.
     * This notification is part of @c AudioPlayer's implementation for pre-buffering, and must be called by
     * @c MediaPlayer implementations for this feature to work properly.
     *
     * @note The observer must quickly return from this callback. Failure to do so could block the @c MediaPlayer from
     * further processing.
     *
     * @param SourceId The id of the source to which this callback corresponds to.
     * @param MediaPlayerState Metadata about the media player state
     */
    virtual void onBufferingComplete(SourceId, const MediaPlayerState&) {
    }

    /**
     * This is an indication to the observer that the @c MediaPlayer has seeked in the source specified by
     * the id.  This can be sent anytime after @c onPlaybackStarted has been called
     *
     * @note The observer must quickly return from this callback. Failure to do so could block the @c MediaPlayer from
     * further processing.
     *
     * @param SourceId The id of the source to which this callback corresponds to.
     * @param MediaPlayerState Metadata about the media player state at the point seek started
     * @param MediaPlayerState Metadata about the media player state at the point the seek completed, or, if stopped /
     * paused, the point playback will be resumed.
     */
    virtual void onSeeked(SourceId, const MediaPlayerState&, const MediaPlayerState&) {
    }

    /**
     * This is an indication to the observer that the @c MediaPlayer has found tags in the stream.
     * Tags are key value pairs extracted from the metadata of the stream. There can be multiple
     * tags that have the same key. Vector preserves the order of insertion
     * (push_back) which may come in handy.
     *
     * @note The observer must quickly returnfrom this callback. Failure to do so could block the @c MediaPlayer from
     * further processing.
     *
     * @param SourceId The id of the source to which this callback corresponds to.
     * @param VectorOfTags The vector containing stream tags.
     * @param MediaPlayerState Metadata about the media player state
     */
    virtual void onTags(SourceId, std::unique_ptr<const VectorOfTags>, const MediaPlayerState&){};
};

/**
 * Write a @c MediaPlayerState to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param state The state value to write to teh @c ostream as a string
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const MediaPlayerState& state) {
    return stream << "MediaPlayerState: offsetInMilliseconds=" << state.offset.count();
}

}  // namespace mediaPlayer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_MEDIAPLAYEROBSERVERINTERFACE_H_
