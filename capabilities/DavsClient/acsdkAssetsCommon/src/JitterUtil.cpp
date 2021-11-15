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

#include "acsdkAssetsCommon/JitterUtil.h"

#include <AVSCommon/Utils/Logger/Logger.h>

#include <mutex>
#include <random>

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {
namespace jitterUtil {

using namespace std;
using namespace chrono;

/// String to identify log entries originating from this file.
static const std::string TAG{"JitterUtil"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

milliseconds jitter(milliseconds baseValue, float jitterFactor) {
    std::random_device rd;
    std::mt19937 mt{rd()};

    if (jitterFactor <= 0 || jitterFactor >= 1) {
        ACSDK_ERROR(LX("jitter").m("Returning without jitter").d("bad jitter", jitterFactor));
        return baseValue;
    }

    // jitter between +/- JITTER_FACTOR of current time
    auto jitterSize = static_cast<int64_t>(jitterFactor * baseValue.count());
    auto jitterRand = std::uniform_int_distribution<int64_t>(-jitterSize, jitterSize);
    return baseValue + milliseconds(jitterRand(mt));
}

milliseconds expJitter(milliseconds baseValue, float jitterFactor) {
    return baseValue + jitter(baseValue, jitterFactor);
}

}  // namespace jitterUtil
}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK