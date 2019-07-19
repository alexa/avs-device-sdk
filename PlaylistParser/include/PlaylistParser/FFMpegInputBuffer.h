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

#ifndef ALEXA_CLIENT_SDK_PLAYLISTPARSER_INCLUDE_PLAYLISTPARSER_FFMPEGINPUTBUFFER_H_
#define ALEXA_CLIENT_SDK_PLAYLISTPARSER_INCLUDE_PLAYLISTPARSER_FFMPEGINPUTBUFFER_H_

#include <cstring>
#include <memory>
#include <vector>

namespace alexaClientSDK {
namespace playlistParser {

/**
 * Helper class to write contents to FFMpeg buffers.
 */
class FFMpegInputBuffer {
public:
    /**
     * Constructor
     *
     * @param inputBytes Input bytes to be written to FFMpeg buffer.
     */
    FFMpegInputBuffer(const std::vector<unsigned char>& inputBytes);

    /**
     * Copy content from input buffer to FFMpeg buffer.
     *
     * @param size The size of content to read.
     * @param data The pointer to buffer to write content.
     * @return Size of actual content written.
     */
    int read(int size, uint8_t* data);

    /**
     * Set current offset.
     *
     * @param offset Current offset to set.
     * @return @c true if successful or @c false otherwise.
     */
    bool setOffset(int64_t offset);

    /**
     * Get current offset.
     *
     * @return Current offset.
     */
    int64_t getOffset() const;

    /**
     * Get input buffer size.
     *
     * @return Input buffer size.
     */
    int64_t getSize() const;

private:
    /// Input Buffer.
    std::vector<unsigned char> m_inputBytes;

    /// Current offset.
    int64_t m_offset;
};

}  // namespace playlistParser
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_PLAYLISTPARSER_INCLUDE_PLAYLISTPARSER_FFMPEGINPUTBUFFER_H_