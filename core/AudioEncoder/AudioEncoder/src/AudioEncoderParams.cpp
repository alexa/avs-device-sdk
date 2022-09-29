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

#include <acsdk/AudioEncoder/AudioEncoderParams.h>

namespace alexaClientSDK {
namespace audioEncoder {

// GCC 4.x and 5.x reached end of support before some of c++11 amendments were made. In particular, these compilers
// do not have default constructors for list initialization from aggregates. This issue has been addressed in GCC 6.x.
// http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1467
#if !defined(__clang__) && defined(__GNUC__) && (__GNUC__ < 6)

AudioEncoderParams::AudioEncoderParams(
    std::chrono::milliseconds readTimeoutMs,
    std::chrono::milliseconds writeTimeoutMs,
    std::chrono::milliseconds stopTimeoutMs,
    std::size_t maxOutputStreamReaders,
    std::size_t maxOutputStreamBufferedPackets) :
        readTimeoutMs{readTimeoutMs},
        writeTimeoutMs{writeTimeoutMs},
        stopTimeoutMs{stopTimeoutMs},
        maxOutputStreamReaders{maxOutputStreamReaders},
        maxOutputStreamBufferedPackets{maxOutputStreamBufferedPackets} {
}

AudioEncoderParams::AudioEncoderParams(const AudioEncoderParams& params) :
        readTimeoutMs{params.readTimeoutMs},
        writeTimeoutMs{params.writeTimeoutMs},
        stopTimeoutMs{params.stopTimeoutMs},
        maxOutputStreamReaders{params.maxOutputStreamReaders},
        maxOutputStreamBufferedPackets{params.maxOutputStreamBufferedPackets} {
}

#endif

}  // namespace audioEncoder
}  // namespace alexaClientSDK
