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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_ENDIAN_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_ENDIAN_H_

#include <cstdint>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

/// Function used to check the machine endianness.
inline bool littleEndianMachine() {
    uint16_t original = 1;
    uint8_t* words = reinterpret_cast<uint8_t*>(&original);
    return words[0] == 1;
}

// Function used to swap endian of uint16_t
inline uint16_t swapEndian(uint16_t input) {
    return ((input & 0x00ff) << 8 | (input & 0xff00) >> 8);
}

// Function used to swap endian of uint32_t
inline uint32_t swapEndian(uint32_t input) {
    return (
        (input & 0xff000000) >> 24 |  // move byte 3 to byte 0
        (input & 0x0000ff00) << 8 |   // move byte 1 to byte 2
        (input & 0x00ff0000) >> 8 |   // move byte 2 to byte 1
        (input & 0x000000ff) << 24);  // byte 0 to byte 3
}

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_ENDIAN_H_
