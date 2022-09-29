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

#ifndef ACSDK_AUDIOENCODER_PRIVATE_AUDIOENCODER_H_
#define ACSDK_AUDIOENCODER_PRIVATE_AUDIOENCODER_H_

#include <atomic>
#include <memory>
#include <thread>

#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/Utils/AudioFormat.h>
#include <AVSCommon/Utils/Threading/Executor.h>

#include <acsdk/AudioEncoderInterfaces/AudioEncoderInterface.h>
#include <acsdk/AudioEncoderInterfaces/BlockAudioEncoderInterface.h>
#include <acsdk/AudioEncoder/AudioEncoderParams.h>
#include <acsdk/AudioEncoder/private/AudioEncoderState.h>

namespace alexaClientSDK {
namespace audioEncoder {

/**
 * @brief Audio encoder for @c AudioInputStream.
 *
 * This class provides generic interface between backend encoder implementation and the application who wants to encode
 * audio stream within @c AudioInputStream.
 */
class AudioEncoder : public audioEncoderInterfaces::AudioEncoderInterface {
public:
    /**
     * Constructor.
     *
     * @param[in] blockAudioEncoder The backend encoder implementation.
     * @param[in] params            Encoder parameters.
     */
    AudioEncoder(
        const std::shared_ptr<audioEncoderInterfaces::BlockAudioEncoderInterface>& blockAudioEncoder,
        const AudioEncoderParams& params);

    /**
     * Destructor.
     */
    ~AudioEncoder();

    /// @name Methods from @c AudioEncoderInterface.
    /// @{
    std::shared_ptr<avsCommon::avs::AudioInputStream> startEncoding(
        const std::shared_ptr<avsCommon::avs::AudioInputStream>& inputStream,
        avsCommon::utils::AudioFormat inputFormat,
        avsCommon::avs::AudioInputStream::Index begin,
        avsCommon::avs::AudioInputStream::Reader::Reference reference) override;
    void stopEncoding(bool stopImmediately = false) override;
    avsCommon::utils::AudioFormat::Encoding getEncoding() override;
    std::string getAVSFormatName() override;
    /// @}

private:
    /**
     * @brief Clock to use.
     *
     * The implementation uses monotonic clock to detect stop timeouts.
     */
    using Clock = std::chrono::steady_clock;

    /**
     * @brief Byte buffer data type.
     */
    using Bytes = audioEncoderInterfaces::BlockAudioEncoderInterface::Bytes;

    /**
     * Thread loop
     */
    void encodeLoop(
        avsCommon::avs::AudioInputStream::Index begin,
        avsCommon::avs::AudioInputStream::Reader::Reference reference);

    /**
     * @brief Check if encoding is permitted to start next frame processing.
     *
     * Encoder is allowed to continue if #stopEncoding() has not been called.
     *
     * @return True if encoder can continue processing, or false if it must stop.
     *
     * @see #mayContinueEncoding()
     */
    bool mayProcessNextFrame();

    /**
     * @brief Check if encoding is permitted to continue.
     *
     * Encoder is allowed to continue if #stopEncoding() has not been called, or #stopEncoding() has been called with
     * @a stopImmediately flag set to false and stop timeout has not yet elapsed.
     *
     * @return True if encoder can continue processing, or false if it must stop.
     *
     * @see #mayProcessNextFrame()
     * @see #mayFinishEncoding()
     */
    bool mayContinueEncoding();

    /**
     * @brief Check if encoding is permitted to finish.
     *
     * Encoder is allowed to finish (flush the data) if #stopEncoding() has not been called, or #stopEncoding() has been
     * called with @a stopImmediately flag set to false and stop timeout has not yet elapsed, and if there has been no
     * error.
     *
     * @return True if encoder can finish processing, or false if it must stop.
     */
    bool mayFinishEncoding();

    /**
     * @brief Check if encoder needs to read more data from input stream.
     *
     * If @a m_encoder requires full frame for processing, this method checks if the amount of data accumulated in
     * @a m_inputBuffer equal to frame size. If @a m_encoder doesn't require full frame, this method checks if @a
     * m_inputBuffer has at least one word.
     *
     * @return True if there is not enough data to invoke audio encoder. False if there is enough data for audio
     *         encoding.
     */
    bool needReadMoreWordsFromInput();

    /**
     * @brief Read audio data from input stream.
     *
     * This method reads audio data from input stream until there is enough data for audio encoding, or until there is
     * no more data, or an error occurred.
     *
     * @return True if there is enough data for audio encoding, False if end of stream has been reached, or error
     *         occurred.
     *
     * @see #encodeWordsIntoOutput()
     */
    bool readWordsFromInput();

    /**
     * @brief Encode buffered data.
     *
     * This method encodes data from @a m_inputBuffer into @a m_outputBuffer. The method appends encoded data into
     * m_outputBuffer, as it may have incomplete word left from a previous write operation.
     *
     * @return True if data has been encoded, false if operation has been terminated or there is an error.
     *
     * @see #readWordsFromInput()
     * @see #writeEncodedWordsFromOutput()
     */
    bool encodeWordsIntoOutput();

    /**
     * @brief Write encoded data into output stream.
     *
     * This method writes encoded data from @a m_outputBuffer into @a m_outputStream. Because output buffer may contain
     * incomplete words, it is possible that some bytes are left in output buffer after write succeeds. These bytes are
     * moved into output buffer's heads.
     *
     * Because output stream may not be able to accept all data at once, this method repeats writes until all complete
     * words are written, or encoding operation has been terminated, or there is an error.
     *
     * @return True if operation succeeds, False if operation fails or has been terminated.
     */
    bool writeEncodedWordsFromOutput();

    /**
     * @brief Complete encoding operation.
     *
     * This method updates object state and notifies any blocked threads that encoding was completed or stopped.
     *
     * This method doesn't reset @a m_outputStream, as it must be accessible after the encoding operation completion.
     */
    void completeEncodingAndNotifyWaiters();

    /**
     * @brief Change the encoder state to error.
     *
     * The method updates the encoder state to indicate the error has encountered and the encoding shall stop.
     */
    void setErrorState();

    /// Backend implementation
    const std::shared_ptr<audioEncoderInterfaces::BlockAudioEncoderInterface> m_encoder;

    /// Encoder parameters.
    const AudioEncoderParams m_params;

    /// Mutex to control object state
    std::mutex m_mutex;

    /// Internal Executor for managing encoding thread
    avsCommon::utils::threading::Executor m_executor;

    /// Flag if @a m_encoder requires full frame for encoding.
    bool m_requireFullFrameSize;

    /// Maximum single encoded frame size (words) initialized from EncoderContext
    size_t m_maxInputFrameSizeWords;

    /// Maximum output frame size (bytes) initialized from EncoderContext
    size_t m_maxOutputFrameSizeBytes;

    /// Encoder word size in bytes initialized from EncoderContext
    std::size_t m_outputWordSizeBytes;

    /// Output stream buffer size in bytes
    std::size_t m_outputBufferSizeBytes;

    /// Condition variable for notifications to blocked threads
    std::condition_variable m_cond;

    /// Current encoder state. Access to this variable must be synchronized
    std::atomic<AudioEncoderState> m_state;

    /// Data input stream. Access to this variable must be synchronized
    /// Audio input stream (PCM frames).
    std::shared_ptr<avsCommon::avs::AudioInputStream> m_inputStream;

    /// Reader for @a #m_inputStream.
    std::unique_ptr<avsCommon::avs::AudioInputStream::Reader> m_inputStreamReader;

    /// Data output stream. Access to this variable must be synchronized
    std::shared_ptr<avsCommon::avs::AudioInputStream> m_outputStream;

    /// Writer for @a #m_outputStream.
    std::unique_ptr<avsCommon::avs::AudioInputStream::Writer> m_outputStreamWriter;

    /// Input word size in bytes initialized from input stream.
    std::size_t m_inputWordSizeBytes;

    /// @brief Buffer for reading data from input stream.
    ///
    /// This buffer must be sufficient to hold
    Bytes m_inputBuffer;

    /// @brief Number of words in @a m_inputBuffer.
    std::size_t m_wordsInInputBuffer;

    /// @brief Buffer for holding encoded data before writing it to output stream.
    ///
    /// This buffer TBD
    // we need to allocate extra space for output buffer, because encoder produces result in bytes, not words, while
    // writer writes results in words. this may lead to situation when there are few bytes left in buffer, and the
    // buffer still has to store full encoding result.
    Bytes m_outputBuffer;

    /// @brief Timeout time point when executing graceful stop.
    Clock::time_point m_timeoutTimePoint;
};

}  // namespace audioEncoder
}  // namespace alexaClientSDK

#endif  // ACSDK_AUDIOENCODER_PRIVATE_AUDIOENCODER_H_
