/*
 * AudioFormat.h
 *
 * Copyright (c) 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVS_COMMON_INCLUDE_AVS_COMMON_AUDIO_FORMAT_H_
#define ALEXA_CLIENT_SDK_AVS_COMMON_INCLUDE_AVS_COMMON_AUDIO_FORMAT_H_

namespace alexaClientSDK {
namespace avsCommon {

/**
 * The format of audio data.
 */
struct AudioFormat {
    /**
     * An enum class used to represent the encoding of audio data.
     */
    enum class Encoding {
        /// Represents LPCM (Linear pulse code modulation) encoding.
        LPCM
    };

    /**
     * An enum class used to represent the endianness of audio data.
     */
    enum class Endianness {
        /// Represents little endianness.
        LITTLE,

        /// Represents big endianness.
        BIG
    };

    /// The encoding of the data.
    Encoding encoding;

    /// The endianness of the data.
    Endianness endianness;

    /// The number of samples recorded or played per second.
    unsigned int sampleRateHz;

    /// The bits per sample.
    unsigned int sampleSizeInBits;

    /// The number of channels.
    unsigned int numChannels;
};

} // namespace avsCommon
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_AVS_COMMON_INCLUDE_AVS_COMMON_AUDIO_FORMAT_H_