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

#include <unordered_set>

#include "AVSCommon/Utils/ID3Tags/ID3v2Tags.h"
#include "AVSCommon/Utils/Logger/Logger.h"

/// String to identify log entries originating from this file.
#define TAG "ID3v2Tags"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace id3Tags {

/// The offset for ID3V2 Major version.
static constexpr unsigned int ID3V2TAG_VERSION_MAJOR_OFFSET = 3;

/// The offset for ID3V2 size.
static constexpr unsigned int ID3V2TAG_SIZE_OFFSET = 6;

std::size_t getID3v2TagSize(const unsigned char* data, std::size_t bufferSize) {
    // Major version supported are 3 and 4
    const std::unordered_set<unsigned char> supportedMajorVersion{3, 4};

    if (!data) {
        ACSDK_ERROR(LX("getID3v2TagSizeFailed").m("nullData"));
        return 0;
    }

    if (bufferSize < ID3V2TAG_HEADER_SIZE) {
        return 0;
    }
    if (std::memcmp(data, ID3V2TAG_IDENTIFIER, sizeof(ID3V2TAG_IDENTIFIER)) == 0) {
        // check if major version is supported
        if (!supportedMajorVersion.count(data[ID3V2TAG_VERSION_MAJOR_OFFSET])) {
            ACSDK_DEBUG9(LX(__func__).d("versionNotSupported", static_cast<int>(data[ID3V2TAG_VERSION_MAJOR_OFFSET])));
            return 0;
        }

        std::size_t id3v2_size = 0;
        for (unsigned int i = ID3V2TAG_SIZE_OFFSET; i < ID3V2TAG_HEADER_SIZE; ++i) {
            auto value = data[i];
            // most significant bit must be zero
            if ((value & 0x80) != 0) {
                ACSDK_DEBUG9(LX(__func__).d("mostSignificantBitNotZero", static_cast<unsigned>(value)).d("offset", i));
                return 0;
            }

            // The ID3 tag size are stored as a 28 bits integer in 4 bytes, with the most significant bit of
            // each byte ignored.
            id3v2_size <<= 7;
            id3v2_size += (value & 0x7f);
        }

        if (id3v2_size == 0) {
            ACSDK_DEBUG9(LX(__func__).m("id3v2SizeIsZero"));
            return 0;
        }

        // Add ID3 header size
        id3v2_size += ID3V2TAG_HEADER_SIZE;
        ACSDK_DEBUG9(LX("ID3v2 tag detected").d("tagSize", id3v2_size));
        return id3v2_size;
    }
    return 0;
}

}  // namespace id3Tags
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
