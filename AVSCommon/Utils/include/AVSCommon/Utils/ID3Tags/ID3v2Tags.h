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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_ID3TAGS_ID3V2TAGS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_ID3TAGS_ID3V2TAGS_H_

#include <cstddef>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace id3Tags {

/// Identifier for a ID3v2 Tag
constexpr unsigned char ID3V2TAG_IDENTIFIER[] = {'I', 'D', '3'};

/// The length of a ID3v2 header
constexpr unsigned int ID3V2TAG_HEADER_SIZE = 10;

/**
 * A function that reads from a unsigned char buffer and returns the length of an ID3v2 tag.  This includes checking
 * if the header size is valid.
 *
 * @param data The pointer to the unsigned char buffer.
 * @param bufferSize The length of the buffer.
 * @return The size of the ID3v2 tag.  Return 0 is the tag is not found in buffer.
 */
std::size_t getID3v2TagSize(const unsigned char* data, std::size_t bufferSize);

}  // namespace id3Tags
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_ID3TAGS_ID3V2TAGS_H_
