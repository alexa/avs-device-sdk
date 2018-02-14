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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_SDS_READERPOLICY_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_SDS_READERPOLICY_H_

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace sds {

/// Specifies the policy to use for reading from the stream.
enum class ReaderPolicy {
    /**
     * A @c NONBLOCKING @c Reader will return any available data (up to the amount requested) immediately, without
     * waiting for more data to be written to the stream.  If no data is available, a @c NONBLOCKING @c Reader will
     * return @c Error::WOULDBLOCK.
     */
    NONBLOCKING,
    /**
     * A @c BLOCKING @c Reader will wait for up to the specified timeout (or forever if `(timeout == 0)`) for data
     * to become available.  As soon as at least one word is available, the @c Reader will return up to the
     * requested amount of data.  If no data becomes available in the specified timeout, a @c BLOCKING @c Reader
     * will return @c Error::TIMEDOUT.
     */
    BLOCKING
};

}  // namespace sds
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_SDS_READERPOLICY_H_
