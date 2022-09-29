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

#ifndef ACSDK_AUDIOENCODER_AUDIOENCODERPARAMS_H_
#define ACSDK_AUDIOENCODER_AUDIOENCODERPARAMS_H_

#include <chrono>
#include <cstddef>

namespace alexaClientSDK {
namespace audioEncoder {

/**
 * @brief Audio encoder parameters.
 *
 * These parameters affect performance and memory consumption by audio encoder.
 *
 * @see #createAudioEncoder()
 */
struct AudioEncoderParams {
    /**
     * @brief Reading timeout from input audio stream.
     *
     * Read timeout control how frequently audio encoder checks for stop signal while reading frame.
     */
    std::chrono::milliseconds readTimeoutMs;

    /**
     * @brief Timeout between write retrying.
     *
     * Write timeout control how frequently audio encoder checks for stop signal while writing encoded frame.
     */
    std::chrono::milliseconds writeTimeoutMs;

    /**
     * @brief Timeout for graceful stop operation.
     *
     * This timeout controls for how long the audio encoder tries to stop operation gracefully. Audio encoder will
     * abort operation after timeout elapses.
     */
    std::chrono::milliseconds stopTimeoutMs;

    /**
     * @brief The maximum number of readers of the output stream.
     *
     * Audio encoder constructs the output stream and it needs to know how many readers will consume the data.
     */
    std::size_t maxOutputStreamReaders;

    /**
     * @brief The maximum number of packets to be buffered to the output stream.
     *
     * Audio encoder constructs the output stream and it needs to know how much data to buffer.
     */
    std::size_t maxOutputStreamBufferedPackets;

// GCC 4.x and 5.x reached end of support before some of c++11 amendments were made. In particular, these compilers
// do not have default constructors for list initialization from aggregates. This issue has been addressed in GCC 6.x.
// http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1467
#if !defined(__clang__) && defined(__GNUC__) && (__GNUC__ < 6)
    /**
     * Copy constructor.
     *
     * This constructor is required only for GCC before version 6.x.
     *
     * @param[in] params Source parameters.
     *
     * @see http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1467
     */
    AudioEncoderParams(const AudioEncoderParams& params);

    /**
     * Explicit constructor with parameters.
     *
     * This constructor is required only for GCC before version 6.x.
     *
     * @param[in] readTimeoutMs                  Reading timeout from input audio stream.
     * @param[in] writeTimeoutMs                 Timeout between write retrying.
     * @param[in] stopTimeoutMs                  Timeout for graceful stop operation.
     * @param[in] maxOutputStreamReaders         The maximum number of readers of the output stream.
     * @param[in] maxOutputStreamBufferedPackets The maximum number of packets to be buffered to the output stream.
     *
     * @see http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1467
     */
    AudioEncoderParams(
        std::chrono::milliseconds readTimeoutMs,
        std::chrono::milliseconds writeTimeoutMs,
        std::chrono::milliseconds stopTimeoutMs,
        std::size_t maxOutputStreamReaders,
        std::size_t maxOutputStreamBufferedPackets);
#endif
};

}  // namespace audioEncoder
}  // namespace alexaClientSDK

#endif  // ACSDK_AUDIOENCODER_AUDIOENCODERPARAMS_H_
