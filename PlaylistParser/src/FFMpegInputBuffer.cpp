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
#include "PlaylistParser/FFMpegInputBuffer.h"

namespace alexaClientSDK {
namespace playlistParser {

FFMpegInputBuffer::FFMpegInputBuffer(const std::vector<unsigned char>& inputBytes) :
        m_inputBytes(inputBytes),
        m_offset(0) {
}

int FFMpegInputBuffer::read(int size, uint8_t* data) {
    if (!data) {
        return 0;
    }

    auto remainingSize = getSize() - m_offset;
    auto readSize = (remainingSize < size) ? remainingSize : size;
    if (readSize > 0) {
        std::memcpy(data, m_inputBytes.data() + m_offset, size);
        m_offset += size;
    }
    return readSize;
}

bool FFMpegInputBuffer::setOffset(int64_t offset) {
    if (offset > 0 && offset < getSize()) {
        m_offset = offset;
        return true;
    }
    return false;
}

int64_t FFMpegInputBuffer::getOffset() const {
    return m_offset;
}

int64_t FFMpegInputBuffer::getSize() const {
    return static_cast<int64_t>(m_inputBytes.size());
}

}  // namespace playlistParser
}  // namespace alexaClientSDK
