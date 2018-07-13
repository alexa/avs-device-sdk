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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_SDS_INPROCESSSDS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_SDS_INPROCESSSDS_H_

#include <vector>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <string>

#include "SharedDataStream.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace sds {

/// Structure for specifying the traits of a SharedDataStream which works between threads in a single process.
struct InProcessSDSTraits {
    /// C++11 std::atomic is sufficient for in-process atomic variables.
    using AtomicIndex = std::atomic<uint64_t>;

    /// C++11 std::atomic is sufficient for in-process atomic variables.
    using AtomicBool = std::atomic<bool>;

    /// A std::vector provides a simple container to hold a buffer for in-process usage.
    using Buffer = std::vector<uint8_t>;

    /// A std::mutex provides a lock which will work for in-process usage.
    using Mutex = std::mutex;

    /// A std::condition_variable provides a condition variable which will work for in-process usage.
    using ConditionVariable = std::condition_variable;

    /// A unique identifier representing this combination of traits.
    static constexpr const char* traitsName = "alexaClientSDK::avsCommon::utils::sds::InProcessSDSTraits";
};

/// Type alias for a SharedDataStream which works between threads in a single process.
using InProcessSDS = SharedDataStream<InProcessSDSTraits>;

}  // namespace sds
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_SDS_INPROCESSSDS_H_
