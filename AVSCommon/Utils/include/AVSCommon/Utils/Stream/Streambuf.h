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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_STREAM_STREAMBUF_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_STREAM_STREAMBUF_H_

#include <sstream>
#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace stream {

/**
 * This class takes byte array and makes a non-copying std::streambuf out of it.
 */
class Streambuf : public std::streambuf {
public:
    /**
     * Streambuf
     *
     * @param data the beginning of the byte array
     * @param length the size of the byte array
     */
    Streambuf(const unsigned char* data, size_t length);

    std::streampos seekoff(
        std::streamoff off,
        std::ios_base::seekdir way,
        std::ios_base::openmode which = std::ios_base::in) override;
    std::streampos seekpos(std::streampos sp, std::ios_base::openmode which = std::ios_base::in) override;

private:
    /// @name @c std::streambuf method overrides
    /// @{
    int_type underflow() override;
    int_type pbackfail(int_type ch) override;
    std::streamsize showmanyc() override;
    /// @}

    /// The start of the buffer to stream.
    char* const m_begin;

    /// The end (one byte past the last byte) of the buffer to stream.
    char* const m_end;
};

}  // namespace stream
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_STREAM_STREAMBUF_H_
