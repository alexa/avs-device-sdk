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
#include <limits>

#include <AVSCommon/Utils/Logger/Logger.h>
#include "AVSCommon/Utils/Stream/Streambuf.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace stream {

/// String to identify log entries originating from this file.
#define TAG "Streambuf"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Constant to indicate the MAX stream offset of an integer.
static constexpr std::streamoff MAX_INT = std::numeric_limits<int>::max();

/// Constant to indicate a invalid offset.
static constexpr std::streamoff INVALID_OFFSET = std::streamoff(-1);

// There are two casts, as a streambuf uses Type=char.  This requires removing the const and removing the unsigned.
// setg only is for reading, so this operation is safe, although ugly.
Streambuf::Streambuf(const unsigned char* data, size_t length) :
        m_begin(reinterpret_cast<char*>(const_cast<unsigned char*>(data))),
        m_end(m_begin + length) {
    setg(m_begin, m_begin, m_end);
}

std::streampos Streambuf::seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which) {
    auto errorPos = std::streampos(INVALID_OFFSET);
    switch (way) {
        case std::ios_base::beg:
            setg(m_begin, m_begin + off, m_end);
            break;
        case std::ios_base::cur:
            if (off > MAX_INT) {
                ACSDK_ERROR(LX("seekoffFailed").d("reason", "offset out of limits").d("off", off).d("limit", MAX_INT));
                return errorPos;
            }
            gbump(static_cast<int>(off));
            break;
        case std::ios_base::end:
            setg(m_begin, m_end + off, m_end);
            break;
        default:
            return errorPos;
    }

    if (!gptr() || gptr() >= egptr() || gptr() < eback()) {
        return errorPos;
    }

    return gptr() - eback();
}

std::streampos Streambuf::seekpos(std::streampos sp, std::ios_base::openmode which) {
    return seekoff(sp - pos_type(off_type(0)), std::ios_base::beg, which);
}

Streambuf::int_type Streambuf::underflow() {
    if (gptr() == m_end) {
        return Streambuf::traits_type::eof();
    }
    return Streambuf::traits_type::to_int_type(*gptr());
}

Streambuf::int_type Streambuf::pbackfail(int_type ch) {
    if (gptr() <= eback() || gptr() > egptr() || (ch != Streambuf::traits_type::eof() && ch != egptr()[-1])) {
        return Streambuf::traits_type::eof();
    }
    gbump(-1);
    return ch;
}

std::streamsize Streambuf::showmanyc() {
    return egptr() - gptr();
}

}  // namespace stream
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
