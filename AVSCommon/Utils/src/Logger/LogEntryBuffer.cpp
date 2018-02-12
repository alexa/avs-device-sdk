/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <cstring>
#include "AVSCommon/Utils/Logger/LogEntryBuffer.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

LogEntryBuffer::LogEntryBuffer() : m_base(m_smallBuffer) {
    // -1 so there is always room to append a null terminator.
    auto end = m_base + ACSDK_LOG_ENTRY_BUFFER_SMALL_BUFFER_SIZE - 1;
    setg(m_base, m_base, end);
    setp(m_base, end);
}

LogEntryBuffer::int_type LogEntryBuffer::overflow(int_type ch) {
    if (traits_type::eof() == ch) {
        return traits_type::eof();
    }

    auto size = pptr() - m_base;

    if (!m_largeBuffer) {
        m_largeBuffer.reset(new std::vector<char>(ACSDK_LOG_ENTRY_BUFFER_SMALL_BUFFER_SIZE * 2));
        memcpy(m_largeBuffer->data(), m_base, size);
    } else {
        m_largeBuffer->resize(m_largeBuffer->size() * 2);
    }

    auto newBase = m_largeBuffer->data();
    auto delta = newBase - m_base;
    // -1 so there is always room to append a null terminator.
    auto newEnd = newBase + m_largeBuffer->size() - 1;
    setp(newBase + size, newEnd);
    setg(newBase, gptr() + delta, newEnd);
    m_base = newBase;

    *pptr() = ch;
    pbump(1);
    return ch;
}

const char* LogEntryBuffer::c_str() const {
    /*
     * Although declared const, this method may modify the current buffer one byte past the accumulated data (to
     * null terminate the string this method returns).  It is preferable to keep this method const because passing
     * a non-const rvalue instance of this class (or an enclosing class) to a function would require a copy or move
     * just to call c_str().  Such a copy or move would defeat the main purpose of this class, which is to avoid
     * unnecessary copies or memory allocations.
     */
    *pptr() = 0;
    return m_base;
}

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
