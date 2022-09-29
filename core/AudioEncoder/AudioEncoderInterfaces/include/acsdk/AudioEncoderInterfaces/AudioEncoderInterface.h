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

#ifndef ACSDK_AUDIOENCODERINTERFACES_AUDIOENCODERINTERFACE_H_
#define ACSDK_AUDIOENCODERINTERFACES_AUDIOENCODERINTERFACE_H_

#include <memory>

#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/Utils/AudioFormat.h>

#include <acsdk/AudioEncoderInterfaces/BlockAudioEncoderInterface.h>

namespace alexaClientSDK {
namespace audioEncoderInterfaces {

/**
 * @brief Interface for encoding audio stream.
 *
 * This class provides generic interface between audio stream encoder implementation and the application who wants to
 * encode audio stream.
 *
 * Encoder converts input audio stream into encoded format. Each encoder implementation is expected to support one
 * output format which doesn't change over time.
 *
 * Encoding operation starts with #startEncoding() call and finished in one of the following cases:
 * - Encoder reaches end of input stream
 * - Encoder encounters an error
 * - Encoding operation is stopped using #stopEncoding() operation
 *
 * Encoder supports a single encoding session, but there can be more than one encoder in the system.
 *
 * @see AudioEncoderInterface::AudioStream
 * @see AudioEncoderInterface::AudioFormat
 */
class AudioEncoderInterface {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~AudioEncoderInterface() = default;

    /**
     * @brief Start the new encoding session.
     *
     * Method starts the new encoding session. Object can manage only single session at the same time, thus this call
     * will fail if ongoing encoding session exists.
     *
     * If the method succeeds, client may request to stop encoding with #stopEncoding() call. The encoding session will
     * continue until stopped, end of @a inputStream is reached, or encoder encounters an error.
     *
     * When this method is called, encoder may change the format returned with #getAVSFormatName() method based on
     * input format.
     *
     * @param[in]   inputStream     The @c AudioStream to stream audio from. (i.e.input stream that has raw PCM frames.)
     * @param[in]   inputFormat     The @c AudioFormat to describe the audio format of the input stream.
     * @param[in]   begin           The index where encoding should begin.
     * @param[in]   reference       The reference for the index.
     *
     * @return Stream for encoding output. Encoder creates a new stream for storing the encoding results. On error the
     *         result is nullptr.
     */
    virtual std::shared_ptr<avsCommon::avs::AudioInputStream> startEncoding(
        const std::shared_ptr<avsCommon::avs::AudioInputStream>& inputStream,
        avsCommon::utils::AudioFormat inputFormat,
        avsCommon::avs::AudioInputStream::Index begin,
        avsCommon::avs::AudioInputStream::Reader::Reference reference) = 0;

    /**
     * @brief Stop current encoding session.
     *
     * Method stops current encoding session if it exists. If there is an ongoing encoding session, the method returns
     * after the session is terminated.
     *
     * This method has an execution timeout, and even if @a stopImmediately is false, encoding will be aborted if it
     * cannot be stopped gracefully.
     *
     * @param[in] stopImmediately Flag indicating that encoding should stop immediately. If this flag is set to @c false
     *                            (the default), encoding will continue until any existing data in the buffer has been
     *                            encoded. If this flag is set to @c true, existing data in the buffer which has not
     *                            already been encoded will be discarded, and encoding will stop immediately.
     */
    virtual void stopEncoding(bool stopImmediately = false) = 0;

    /**
     * @brief Get output encoding type.
     *
     * Method returns output encoding type.
     *
     * @return Output encoding supported by this encoder.
     */
    virtual avsCommon::utils::AudioFormat::Encoding getEncoding() = 0;

    /**
     * @brief Get output format name for AVS service.
     *
     * This method returns output format name for AVS service. The encoder may change the format depending on
     * configuration when encoding starts with #startEncoding() call, and the clients should use this method after it.
     * The AVS format name doesn't change until the next call to #startEncoding().
     *
     * @return The string interpretation of the output format, that AVS cloud service can recognize.
     *
     * @see https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/speechrecognizer.html#recognize
     */
    virtual std::string getAVSFormatName() = 0;
};

}  // namespace audioEncoderInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_AUDIOENCODERINTERFACES_AUDIOENCODERINTERFACE_H_
