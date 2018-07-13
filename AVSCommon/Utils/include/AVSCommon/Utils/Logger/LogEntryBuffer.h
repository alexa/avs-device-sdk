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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_LOGENTRYBUFFER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_LOGENTRYBUFFER_H_

#include <memory>
#include <streambuf>
#include <vector>

/**
 * The size of @c LogEntryBuffer::m_smallBuffer.  Instances of @c LogEntryBuffer are expected to be allocated
 * on the stack in most use cases.  Rather than pick a value that would be large enough for almost any normal
 * log lines (e.g. 4096), a smaller value (128) that will handle the vast majority of typical log lines was
 * chosen to reduce the impact on the stack.
 *
 * #ifndef used here to allow overriding this value from the compiler command line.
 */
#ifndef ACSDK_LOG_ENTRY_BUFFER_SMALL_BUFFER_SIZE
#define ACSDK_LOG_ENTRY_BUFFER_SMALL_BUFFER_SIZE 128
#endif

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

/**
 * A simple override of @c std::streambuf to accumulate the content of a stream into a contiguous buffer so that
 * the stream contents do not need to be copied to be accessed as a string. To avoid needless heap allocation
 * the initial put buffer is a member variable that should be large enough to accommodate most log entries.
 */
class LogEntryBuffer : public std::streambuf {
public:
    /// Construct an empty LogEntryBuffer.
    LogEntryBuffer();

    int_type overflow(int_type ch) override;

    /**
     * Access the contents of the accumulated buffer as a string.
     * @return The contents of the accumulated buffer as a string. The pointer returned is only guaranteed to be
     * valid for the lifetime of this LogEntryBuffer, and only as long as no further modifications are made to it.
     */
    const char* c_str() const;

private:
    /// A small embedded buffer used unless the data to be buffered grows beyond its capacity.
    char m_smallBuffer[ACSDK_LOG_ENTRY_BUFFER_SMALL_BUFFER_SIZE];

    /// Pointer to the start of whatever memory the buffered data has accumulated in.
    char* m_base;

    /// A resizable buffer used if and when the size of the data to buffer has exceeded SMALL_BUFFER_SIZE.
    std::unique_ptr<std::vector<char>> m_largeBuffer;
};

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_LOGENTRYBUFFER_H_
