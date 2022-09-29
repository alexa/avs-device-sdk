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

#include <chrono>
#include <cstddef>

#include <acsdk/AudioEncoder/AudioEncoderFactory.h>
#include <acsdk/AudioEncoder/private/AudioEncoder.h>

namespace alexaClientSDK {
namespace audioEncoder {

using audioEncoderInterfaces::AudioEncoderInterface;

/// Default value for @ref AudioEncoderParams#readTimeoutMs.
static constexpr std::chrono::milliseconds DEFAULT_READ_TIMEOUT_MS{10};
/// Default value for @ref AudioEncoderParams#writeTimeoutMs.
static constexpr std::chrono::milliseconds DEFAULT_WRITE_TIMEOUT_MS{100};
/// Default value for @ref AudioEncoderParams#stopTimeoutMs.
static constexpr std::chrono::milliseconds DEFAULT_STOP_TIMEOUT_MS{1000};
/// Default value for @ref AudioEncoderParams#maxOutputStreamReaders.
static constexpr std::size_t DEFAULT_MAX_OUTPUT_STREAM_READERS = 10;
/// Default value for @ref AudioEncoderParams#maxOutputStreamBufferedPackets.
static constexpr std::size_t DEFAULT_MAX_OUTPUT_STREAM_BUFFERED_PACKETS = 20;

std::unique_ptr<AudioEncoderInterface> createAudioEncoder(
    const std::shared_ptr<audioEncoderInterfaces::BlockAudioEncoderInterface>& encoder) {
    return createAudioEncoderWithParams(
        encoder,
        AudioEncoderParams{DEFAULT_READ_TIMEOUT_MS,
                           DEFAULT_WRITE_TIMEOUT_MS,
                           DEFAULT_STOP_TIMEOUT_MS,
                           DEFAULT_MAX_OUTPUT_STREAM_READERS,
                           DEFAULT_MAX_OUTPUT_STREAM_BUFFERED_PACKETS});
}

std::unique_ptr<AudioEncoderInterface> createAudioEncoderWithParams(
    const std::shared_ptr<audioEncoderInterfaces::BlockAudioEncoderInterface>& encoder,
    const AudioEncoderParams& params) {
    return std::unique_ptr<AudioEncoder>(new AudioEncoder(encoder, params));
}

}  // namespace audioEncoder
}  // namespace alexaClientSDK
