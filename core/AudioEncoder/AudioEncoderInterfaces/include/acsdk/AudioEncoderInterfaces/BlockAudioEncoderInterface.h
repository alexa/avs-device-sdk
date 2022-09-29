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

#ifndef ACSDK_AUDIOENCODERINTERFACES_BLOCKAUDIOENCODERINTERFACE_H_
#define ACSDK_AUDIOENCODERINTERFACES_BLOCKAUDIOENCODERINTERFACE_H_

#include <memory>
#include <vector>

#include <AVSCommon/Utils/AudioFormat.h>

namespace alexaClientSDK {
namespace audioEncoderInterfaces {

/**
 * @brief Block audio encoder interface.
 *
 * Block audio encoder provides a generic interface for converting audio stream data. The encoding is performed within
 * a session that is started with #init() call and ends with #close().
 *
 * After initialization block audio encoding has the following stages:
 * - start operation to generate encoded data prologue.
 * - sequential encoding operations to encode input and generated encoded output.
 * - flush operation to generate encoded data epilogue.
 *
 * @code{.cpp}
 * auto encoder = ...
 * encoder->init(inputAudioFormat);
 * ...
 * encoder->start(outputBuffer);
 * writeOutputData(outputBuffer);
 * while (hasMoreData()) {
 *   encoder->processSamples(inputData, inputDataSize, outputBuffer);
 *   writeOutputData(outputBuffer);
 * }
 * encoder->flush(outputBuffer);
 * writeOutputData(outputBuffer);
 * encoder->close();
 * @endcode
 *
 * After call to #close() the encoder instance can be reused for another encoding operation.
 *
 * @see AudioEncoderInterface
 */
class BlockAudioEncoderInterface {
public:
    /**
     * @brief Byte data type.
     */
    using Byte = unsigned char;

    /**
     * @brief Byte array data type for encoding output.
     */
    using Bytes = std::vector<Byte>;

    /**
     * @brief Pre-initialize the encoder.
     *
     * Pre-initialization before actual encoding session has began. Note that this function will be called everytime
     * before new encoding session is starting.
     *
     * @note Encoder may change some output parameters after this call, and consumers should check #getAVSFormatName()
     *       after calling this method.
     *
     * @param[in] inputFormat The @c AudioFormat describes the audio format of the future incoming PCM frames.
     *
     * @return true when initialization success.
     */
    virtual bool init(avsCommon::utils::AudioFormat inputFormat) = 0;

    /**
     * The maximum number of sample can be processed at the same time. In other words, this will limit input stream
     * buffering. Thus number of samples of #processSamples() call will never exceeds this limit.
     *
     * @return The number of sample (in words).
     */
    virtual size_t getInputFrameSize() = 0;

    /**
     * @brief Provide maximum output frame size.
     *
     * The method provides an estimate for an output frame size in bytes. This value is used to allocate necessary
     * buffer space in output audio stream.
     *
     * @return The maximum output length (in bytes).
     */
    virtual size_t getOutputFrameSize() = 0;

    /**
     * @brief Return if input must contain full frame.
     *
     * Determine whether input stream should be fully buffered with the maximum number of samples provided at
     * #getInputFrameSize() function. This value will change the behavior of how processSamples is called during the
     * encoding session. This is useful when backend encoder requires fixed length of input samples.
     *
     * In case the encoding session has been shutdown before the buffer is filled fully, this will cause any partial
     * data to be discarded (e.g. AudioEncoderInterface#stopEncoding() has been called, or reaches the end of the
     * data stream).
     *
     * @return true if processSamples should be called with fixed length of
     * input samples during the session.
     */
    virtual bool requiresFullyRead() = 0;

    /**
     * @brief Return output audio format.
     *
     * Method returns @c AudioFormat that describes encoded output.
     *
     * @note The number of channels in the resulting format is known only after encoding operation has started.
     *
     * @return The @c AudioFormat describes the encoded audio stream.
     */
    virtual avsCommon::utils::AudioFormat getAudioFormat() = 0;

    /**
     * @brief AVS format name for encoded audio.
     *
     * Method returns string interpretation of the output format, that AVS cloud service can recognize.
     *
     * @return Output format string.
     * @see https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/speechrecognizer.html#recognize
     */
    virtual std::string getAVSFormatName() = 0;

    /**
     * @brief Start the encoding session.
     *
     * This function starts a new encoding session after a call to #init(). If block audio encoder produces some data,
     * it can be returned through @a preamble buffer.
     *
     * After the session is started, user can call #processSamples() until session is closed with #close() call.
     *
     * @param[out] preamble Destination buffer for encoding preamble output. The method will append preamble bytes to
     *                      @a preamble if needed. On error @a preamble is not modified.
     *
     * @return true when success.
     */
    virtual bool start(Bytes& preamble) = 0;

    /**
     * @brief Encode a block of audio.
     *
     * This method encodes a block of input audio samples. The samples are provided in a form of byte array restricted
     * by @a begin and @a end iterator arguments. The input bytes must represent complete samples, and each sample size
     * is be known via @c AudioFormat used with #init() call. If #requiresFullyRead() returns true, the number of
     * samples must be equal to #getInputFrameSize(). If #requiresFullyRead() returns false, the number of samples must
     * be greater then zero and not more than #getInputFrameSize().
     *
     * This method can be called any number of times after #start() has been called.
     *
     * @param[in]   begin           Bytes iterator for the first byte of samples. This parameter must be less then
     *                              @a end, or the method will fail with an error.
     * @param[in]   end             Bytes iterator for the end of sample array. This parameter must be greater then
     *                              @a begin, or the method will fail with an error.
     * @param[out]  buffer          Output buffer for encoded data.
     *
     * @return true when success.
     */
    virtual bool processSamples(Bytes::const_iterator begin, Bytes::const_iterator end, Bytes& buffer) = 0;

    /**
     * @brief Flush buffered data if any.
     *
     * This method appends any encoded data to @a buffer.
     *
     * @param[out] buffer Buffer for appending data. The buffer contents is not modified on error.
     *
     * @return true when success.
     */
    virtual bool flush(Bytes& buffer) = 0;

    /**
     * @brief Close encoding session.
     *
     * Notify end of the session. Any backend library then may be deinitialized so it cleans memory and threads.
     */
    virtual void close() = 0;

    /**
     * Destructor.
     */
    virtual ~BlockAudioEncoderInterface() = default;
};

}  // namespace audioEncoderInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_AUDIOENCODERINTERFACES_BLOCKAUDIOENCODERINTERFACE_H_
