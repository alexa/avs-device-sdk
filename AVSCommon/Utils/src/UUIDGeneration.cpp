/*
 * UUIDGeneration.cpp
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <climits>
#include <algorithm>
#include <functional>

#include "AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace uuidGeneration {

/// Number of bytes in a UUID.
static const size_t UUID_SIZE_IN_BYTES = 16;

/// Byte that contains the UUID version.
static const size_t UUID_VERSION_BYTE = 6;

/// Mask to clear the UUID version bits.
static const size_t UUID_VERSION_MASK = 0x0f;

/// The UUID version, shifted into the correct position in the byte.
static const uint8_t UUID_VERSION_VALUE = 4 << 4;

/// Byte that contains the UUID variant.
static const size_t UUID_VARIANT_BYTE_POSITION = 8;

/// Mask to clear the UUID variant bits.
static const size_t UUID_VARIANT_MASK = 0x3f;

/// The UUID variant, shifted into the correct position in the byte.
static const size_t UUID_VARIANT_VALUE = 2 << 6;

/// Separator used between UUID fields.
static const std::string SEPARATOR("-");

/// Number of bytes before first field separator.
static const size_t FIELD1_END = 4;

/// Number of bytes before second field separator.
static const size_t FIELD2_END = 6;

/// Number of bytes before third field separator.
static const size_t FIELD3_END = 8;

/// Number of bytes before third field separator.
static const size_t FIELD4_END = 10;

const std::string generateUUID() {
    uint8_t uuidBytes[UUID_SIZE_IN_BYTES];
    static bool seeded = false;
    static std::independent_bits_engine<std::default_random_engine, CHAR_BIT, uint8_t> ibe;
    static std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    if (!seeded) {
        std::random_device rd;
        ibe.seed(rd() +
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::chrono::steady_clock::now().time_since_epoch()).count());
        seeded = true;
    }
    std::generate(std::begin(uuidBytes), std::end(uuidBytes), std::ref(ibe));
    lock.unlock();

    uuidBytes[UUID_VERSION_BYTE] &= UUID_VERSION_MASK;
    uuidBytes[UUID_VERSION_BYTE] |= UUID_VERSION_VALUE;
    uuidBytes[UUID_VARIANT_BYTE_POSITION] &= UUID_VARIANT_MASK;
    uuidBytes[UUID_VARIANT_BYTE_POSITION] |= UUID_VARIANT_VALUE;
    std::ostringstream uuidText;
    for (size_t i = 0; i < sizeof(uuidBytes); ++i) {
        if (FIELD1_END == i || FIELD2_END == i || FIELD3_END == i || FIELD4_END == i) {
            uuidText << SEPARATOR;
        }
        uuidText << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(uuidBytes[i]);
    }
    return uuidText.str();
}

} // namespace uuidGeneration
} // namespace utils
} // namespace avsCommon
} // namespace alexaClientSDK
