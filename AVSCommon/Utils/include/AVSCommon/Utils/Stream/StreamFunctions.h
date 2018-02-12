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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_STREAM_STREAMFUNCTIONS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_STREAM_STREAMFUNCTIONS_H_

#include <memory>
#include <istream>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace stream {

/**
 * Implementation converts a byte array into a stream.  The data is never copied and multiple streams can be created to
 * the same data source without affecting each other
 *
 * @param data pointer to the data to make into a stream
 * @param length length of data
 */
std::unique_ptr<std::istream> streamFromData(const unsigned char* data, size_t length);

}  // namespace stream
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_STREAM_STREAMFUNCTIONS_H_
