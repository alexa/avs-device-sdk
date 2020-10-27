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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_DISTANTFUTURE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_DISTANTFUTURE_H_

#include <chrono>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {

/**
 * Get a consistent future @c steady_clock::time_point that we deem unreachable (110+ years), avoiding
 * steady_clock::time_point::max() which results in an overflow due to a gcc bug.
 * @see
 * https://gcc-bugs.gcc.gnu.narkive.com/LBRgUQhD/bug-c-58931-new-condition-variable-wait-until-overflowed-by-large-time-point-steady-clock
 *
 * @return Get a consistent @c steady_clock::time_point that we deem unreachable.
 */
inline std::chrono::steady_clock::time_point getDistantFuture() {
    static const auto distantFuture = std::chrono::steady_clock::now() + std::chrono::hours(1000000);
    return distantFuture;
}

}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_DISTANTFUTURE_H_
