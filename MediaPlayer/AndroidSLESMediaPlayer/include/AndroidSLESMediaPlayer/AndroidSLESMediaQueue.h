/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#ifndef ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_ANDROIDSLESMEDIAQUEUE_H_
#define ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_ANDROIDSLESMEDIAQUEUE_H_

#include <array>
#include <atomic>
#include <chrono>
#include <mutex>
#include <vector>

#include <SLES/OpenSLES_Android.h>

#include <AVSCommon/Utils/Threading/Executor.h>
#include <AndroidUtilities/AndroidSLESObject.h>

#include "AndroidSLESMediaPlayer/DecoderInterface.h"
#include "AndroidSLESMediaPlayer/PlaybackConfiguration.h"

namespace alexaClientSDK {
namespace mediaPlayer {
namespace android {

/**
 * Class responsible for reading raw audio from a decoder and feeding it to the underlying media player queue.
 *
 * For that, the @c AndroidSLESMediaQueue has a parallel job that:
 * 1- Fill its unused buffers with raw audio by calling the decoder @c read function.
 * 2- Enqueue the buffers into the OpenSL ES media player queue.
 * 3- Wait until the media queue finishes playing one of the buffers.
 * 4- Once the media player is done with a buffer, it calls @c onBufferFree.
 * 5- The @c AndroidSLESMediaQueue mark the  buffer as unused and go back to 1.
 *
 */
class AndroidSLESMediaQueue {
public:
    using Byte = DecoderInterface::Byte;

    /// Represent the event types that will be sent to the @c StatusCallback when the queue change state.
    enum class QueueEvent {
        /// Sent when the queue encountered an unrecoverable error.
        ERROR,
        /// Sent when there is no more input data to feed the player.
        FINISHED_READING,
        /// Sent when all buffers are free and playing is over.
        FINISHED_PLAYING
    };

    /**
     * Callback method signature called by the @c AndroidSLESMediaQueue when there is a queue event.
     *
     * @param event The event that has occurred.
     * @param reason A description of what triggered the error. It can be an empty string depending on the event
     * triggered.
     */
    using EventCallback = std::function<void(QueueEvent event, const std::string& reason)>;

    /**
     * Creates a new @c AndroidSLESMediaQueue object.
     *
     * @param queueObject The AndroidSLESObject that represent the OpenSL ES queue.
     * @param decoder The decoder object used to get raw audio.
     * @param onStatusChanged A function object that should point to a valid callback function called whenever the
     * queue state changes.
     * @param playbackConfiguration The playback configuration parameters.
     * @return Pointer to the new object if object could be successfully created; otherwise, return a @c nullptr.
     */
    static std::unique_ptr<AndroidSLESMediaQueue> create(
        std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESObject> queueObject,
        std::unique_ptr<DecoderInterface> decoder,
        EventCallback onStatusChanged,
        const PlaybackConfiguration& playbackConfiguration);

    /**
     * The callback function to be called when the media player is no longer reading from a buffer.
     *
     * @warning: This function should not lock or have any heavy processing because it is called from the media
     * player thread. It should also avoid logging statements.
     */
    void onBufferFree();

    /**
     * Destructor.
     */
    ~AndroidSLESMediaQueue();

    /**
     * Get number of bytes that is currently queued.
     *
     * @return The number of bytes currently queued.
     */
    size_t getNumBytesBuffered() const;

    /**
     * Get number of bytes that has been played so far since this object was created.
     *
     * @return The number of bytes played so far.
     */
    size_t getNumBytesPlayed() const;

    /// The number of buffers to use.
    static constexpr size_t NUMBER_OF_BUFFERS{4u};

    /// Buffer size for the decoded data. This has to be big enough to be used with the decoder.
    static constexpr size_t BUFFER_SIZE{131072u};

private:
    /**
     * Constructor.
     *
     * @param queueObject The AndroidSLESObject that represent the OpenSL ES queue.
     * @param bufferQueue The OpenSL ES buffer queue representation.
     * @param decoder The decoder object used to get raw audio.
     * @param callbackFunction A function object that should point to a valid callback function called whenever the
     * queue state changes.
     */
    AndroidSLESMediaQueue(
        std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESObject> queueObject,
        SLAndroidSimpleBufferQueueItf bufferQueue,
        std::unique_ptr<DecoderInterface> decoder,
        EventCallback callbackFunction);

    /// Fill buffer with decoded raw audio and enqueue it back to the media player.
    void fillBuffer();

    /**
     * Enqueue all buffers to be filled. This should only be called when all buffers are clean.
     *
     * @param playbackConfig The playback configuration parameters.
     */
    void fillAllBuffers(const PlaybackConfiguration& configuration);

    /**
     * Enqueue a buffer with one silent sample.
     *
     * @note This is a workaround of an issue found on Android where play after stop triggers an assertion error. It
     * looks like the prefetch status is not being reset by stop playback and clear buffers.(see ACSDK-1895 for more
     * details)
     * @param configuration The playback configuration used to generate the silence samples.
     */
    void enqueueSilence(const PlaybackConfiguration& configuration);

    /// Executor used to serialize buffer filling.
    avsCommon::utils::threading::Executor m_executor;

    /// Internal AndroidSLES engine object.
    std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESObject> m_slObject;

    /// Internal AndroidSLES queue interface.
    SLAndroidSimpleBufferQueueItf m_queueInterface;

    /// Internal buffers.
    std::array<std::array<Byte, BUFFER_SIZE>, NUMBER_OF_BUFFERS> m_buffers;

    /// Tracks the length of each buffer in words which is used to estimate the playback position and bytes buffered.
    std::vector<size_t> m_bufferSizes;

    /// Pointer to the audio decoder.
    std::unique_ptr<DecoderInterface> m_decoder;

    /// Index of the next buffer that has to be filled.
    size_t m_index;

    /// Finished processing all the input.
    bool m_inputEof;

    /// Hit a non-recoverable error.
    bool m_failure;

    /// Callback function used to report status change.
    EventCallback m_eventCallback;

    /// The number of words that has been buffered.
    std::atomic<size_t> m_bufferedWords;

    /// The number of words that has been played so far.
    std::atomic<size_t> m_playedWords;
};

}  // namespace android
}  // namespace mediaPlayer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_ANDROIDSLESMEDIAQUEUE_H_
