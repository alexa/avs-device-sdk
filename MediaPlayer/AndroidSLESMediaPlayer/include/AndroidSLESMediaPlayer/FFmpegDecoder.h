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
#ifndef ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_FFMPEGDECODER_H_
#define ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_FFMPEGDECODER_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

extern "C" {
#include <libavutil/samplefmt.h>
}

#include "AndroidSLESMediaPlayer/DecoderInterface.h"
#include "AndroidSLESMediaPlayer/FFmpegInputControllerInterface.h"
#include "AndroidSLESMediaPlayer/PlaybackConfiguration.h"

struct AVCodec;
struct AVCodecContext;
struct AVDictionary;
struct AVFormatContext;
struct AVFrame;
struct AVInputFormat;
struct AVIOContext;
struct SwrContext;

namespace alexaClientSDK {
namespace mediaPlayer {
namespace android {

/// The layout mask representing which channels should be enabled.
using LayoutMask = int64_t;

/**
 * Class responsible for decoding and re-sampling the audio from an input controller.
 *
 * The decoding is performed on demand. Every time the @c read method is called, the decoder will read the input, and
 * decode it until the provided buffer is full.
 *
 * Decoding is done when the @c DecodingState is equal to DONE or INVALID.
 *
 * @warning This class is not thread safe, except for the @c abort() method.
 */
class FFmpegDecoder : public DecoderInterface {
public:
    /**
     * Creates a new decoder buffer queue that reads input data using the given controller.
     *
     * @param inputController The controller used to retrieve input data.
     * @param outputConfig The decoder output configuration.
     * @return The new decoder buffer queue if create succeeds, @c nullptr otherwise.
     */
    static std::unique_ptr<FFmpegDecoder> create(
        std::unique_ptr<FFmpegInputControllerInterface> inputController,
        const PlaybackConfiguration& outputConfig);

    /// @name DecoderInterface method overrides.
    /// @{
    std::pair<Status, size_t> read(Byte* buffer, size_t size) override;
    void abort() override;
    /// @}

    /**
     * Callback function used by FFmpeg to check when a blocking operation should be interrupted.
     *
     * We interrupt FFmpeg in two different scenarios:
     *   - @c FFmpegDecoder state is @c INVALID (usually due to a call to @c abort).
     *   - FFmpeg initialization is taking too long.
     *
     * @note: The second option is a work around for ACSDK-1679. FFmpeg doesn't seem to be handling @c EAGAIN as
     * expected. If an @c EAGAIN occurs during @c avformat_find_stream_info, the method gets stuck. Thus, this method
     * will interrupt FFmpeg if initialization is taking too long.
     *
     * @return @c true if FFmpeg should be interrupted; false otherwise.
     */
    bool shouldInterruptFFmpeg();

private:
    /**
     * Constructor.
     *
     * @param inputController A controller for the input encoded data.
     * @param format The output sample format.
     * @param layout The output channels layout.
     * @param sampleRate The output sample rate in Hz.
     */
    FFmpegDecoder(
        std::unique_ptr<FFmpegInputControllerInterface> inputController,
        AVSampleFormat format,
        LayoutMask layout,
        int sampleRate);

    /**
     * This enumeration represents the states that the decoder can be in. The possible transitions are:
     *
     * INITIALIZING -> {DECODING, INVALID}
     * DECODING -> {INITIALIZING, FLUSHING_DECODER, INVALID}
     * FLUSHING_DECODER -> {FLUSHING_RESAMPLER, INVALID}
     * FLUSHING_RESAMPLER -> {FINISHED, INVALID}
     *
     * The transition from DECODING to INITIALIZING happens when input controller has next track.
     *
     * @note: The order of states matter since we use less than comparisons.
     */
    enum class DecodingState {
        /// The input provided still has data that needs to be decoded.
        DECODING,
        /// The input has been read completely, but decoding hasn't finished yet.
        FLUSHING_DECODER,
        /// The decoding has finished but the re-sampling might still have unread data.
        FLUSHING_RESAMPLER,
        /// Decoder is initializing.
        INITIALIZING,
        /// There is no more data to be decoded / re-sampled. Calls to @c read will return 0 bytes.
        FINISHED,
        /// The decoder has found an error and it is in an invalid state. Calls to @c read will return 0 bytes.
        INVALID
    };

    /// Friend relationship to allow accessing State to convert it to a string for logging.
    friend std::ostream& operator<<(std::ostream& stream, const DecodingState state);

    /**
     * Sets the @c m_state variable to the value given if and only if the transition is valid.
     *
     * @param nextState The state that we would like to set.
     */
    void setState(DecodingState nextState);

    /**
     * Internal class used to manage data that was decoded but that didn't fit the buffer passed to @c read.
     */
    class UnreadData {
    public:
        /**
         * Constructor
         *
         * @param format The output sample format.
         * @param layout The output channels layout.
         * @param sampleRate The output sample rate in Hz.
         */
        UnreadData(AVSampleFormat format, LayoutMask layout, int sampleRate);

        /**
         * Get the internal frame.
         *
         * @return A reference to the internal frame.
         */
        AVFrame& getFrame();

        /**
         * Get the current offset.
         *
         * @return The offset for the first unread sample.
         */
        int getOffset() const;

        /**
         * Resize the buffer if current capacity is less than the required minimum.
         *
         * @param minimumCapacity The minimum capacity required.
         */
        void resize(size_t minimumCapacity);

        /**
         * Return whether there is any data that hasn't been unread yet.
         *
         * @return true if there is no unread data, false otherwise.
         */
        bool isEmpty() const;

        /**
         * Set the read offset to given value.
         *
         * @param offset The new offset value pointing to a position that hasn't been read.
         */
        void setOffset(int offset);

    private:
        /// The resampleFrame buffer size. This is used to know when we need to resize the frame buffer.
        size_t m_capacity;

        /// The current offset of the unread data inside the frame.
        int m_offset;

        /// The frame where the data is stored.
        std::shared_ptr<AVFrame> m_frame;
    };

    /**
     * Read the data that has been decoded.
     *
     * @param buffer The buffer where the data will be written to.
     * @param size The buffer size in bytes.
     * @param bytesRead The number of bytes that has been read already. This is used to calculate the write offset and
     * the amount of data that can still be written to the buffer.
     * @return The number of bytes copied to @c buffer.
     */
    size_t readData(Byte* buffer, size_t size, size_t bytesRead);

    /**
     * Call the resampler for the given input frame. This method will fill @c m_unreadData with the resampled data.
     *
     * @param inputFrame The frame with the media data that has to be resampled.
     */
    void resample(std::shared_ptr<AVFrame> inputFrame);

    /// Call the decoder to start processing more input data.
    void decode();

    /// Set input controller to point to the next media.
    void next();

    /// Sleep according to the retry policy. Sleeping thread will be awaken by @c abort() call.
    void sleep();

    /**
     * Read the available decoded frame.
     *
     * @param decodedFrame A pointer to the frame where the new data will be written to.
     */
    void readDecodedFrame(std::shared_ptr<AVFrame>& decodedFrame);

    /**
     * Initialize the decoder. If it succeeds, it will set the decoder state to @c DECODING.
     *
     * If there is an unrecoverable error, it will set the decoder to @c INVALID state.
     */
    void initialize();

    /**
     * Parse the status returned by an FFmpeg function.
     *
     * - If there was a unrecoverable error, set the decoder state to @c INVALID.
     * - If the error is recoverable, call @c sleep before returning that an error was found (stay in the same state).
     * - If error represents a EOF, move to next state.
     *
     * @param status The status code returned by the ffmpeg function.
     * @param nextState The next state that the decoder should move to in case of a EOF error.
     * @param functionName The name of the function that was called. This is used for logging purpose.
     * @return @c true if status indicates that the operation succeeded or EOF was found; @c false, otherwise.
     */
    bool transitionStateUsingStatus(int status, DecodingState nextState, const std::string& functionName);

    /// The decoder state.
    std::atomic<DecodingState> m_state;

    /// A controller for the input data.
    std::unique_ptr<FFmpegInputControllerInterface> m_inputController;

    /// The output sample format.
    const AVSampleFormat m_outputFormat;

    /// The output channel layout.
    const LayoutMask m_outputLayout;

    /// The output sample rate.
    const int m_outputRate;

    /// Input format context object.
    std::shared_ptr<AVFormatContext> m_formatContext;

    /// Pointer to the codec context. This is used during the decoding process.
    std::shared_ptr<AVCodecContext> m_codecContext;

    /// Pointer to the resample context.
    std::shared_ptr<SwrContext> m_swrContext;

    /// Object that keeps the unread data leftover from the last @c read.
    UnreadData m_unreadData;

    /// Retry counter used to count the times where data was not available.
    size_t m_retryCount;

    /// Condition variable used to abort possible wait in the read cycle.
    std::condition_variable m_abortCondition;

    /// Time when the initialize method started. This is used to abort initialization that might be taking too long.
    std::chrono::time_point<std::chrono::steady_clock> m_initializeStartTime;
};

}  // namespace android
}  // namespace mediaPlayer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_FFMPEGDECODER_H_
