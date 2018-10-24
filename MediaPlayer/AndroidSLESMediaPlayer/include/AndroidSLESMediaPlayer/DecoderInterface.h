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
#ifndef ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_DECODERINTERFACE_H_
#define ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_DECODERINTERFACE_H_

#include <utility>

namespace alexaClientSDK {
namespace mediaPlayer {
namespace android {

/**
 * A @DecoderInterface provides a method to fill up buffers with raw audio. The read method shall respect the audio
 * frame boundaries, i.e., one frame shall not be split into more than one buffer.
 *
 * The output should always be:
 *
 * - 16bit Linear PCM
 * - 48kHz sample rate
 * - 2 channels (Left | Right)
 * - Native-endianness
 */
class DecoderInterface {
public:
    /// Represents one byte of data.
    using Byte = uint8_t;

    /// Represent the decoder read status.
    enum class Status {
        /// The read request was successful and there is still more data to be read.
        OK,
        /// The decoder has decoded the entire input and there is no more data left to be read.
        DONE,
        /// The decoder found an error.
        ERROR
    };

    /**
     * Fill buffer with decoded audio data.
     *
     * @param[out] Buffer where the data will be copied to.
     * @param size The buffer size in number of bytes.
     *
     * @return The decoding status and the number of bytes read.
     */
    virtual std::pair<Status, size_t> read(Byte* buffer, size_t size) = 0;

    /**
     * Abort the decoding process.
     *
     * This method can be called in parallel with @c read. The @c read will fail in this case. Future reads will
     * also fail.
     */
    virtual void abort() = 0;

    /**
     * Virtual destructor.
     */
    virtual ~DecoderInterface() = default;
};

}  // namespace android
}  // namespace mediaPlayer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_DECODERINTERFACE_H_
