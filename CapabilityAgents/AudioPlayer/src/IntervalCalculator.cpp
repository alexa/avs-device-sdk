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

#include "AudioPlayer/IntervalCalculator.h"

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace audioPlayer {

/// String to identify log entries originating from this file.
static const std::string TAG("IntervalCalculator");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

bool getIntervalStart(
    const std::chrono::milliseconds& interval,
    const std::chrono::milliseconds& offset,
    std::chrono::milliseconds* intervalStart) {
    if (!intervalStart || interval <= std::chrono::milliseconds::zero() || offset < std::chrono::milliseconds::zero()) {
        ACSDK_ERROR(LX(__func__)
                        .d("interval", interval.count())
                        .d("offset", offset.count())
                        .d("*intervalStart", intervalStart)
                        .m("failure, returning false"));
        return false;
    }

    if (interval >= offset) {
        *intervalStart = interval - offset;
    } else {
        *intervalStart = interval - (offset % interval);
    }

    ACSDK_DEBUG5(
        LX(__func__).d("interval", interval.count()).d("offset", offset.count()).d("*intervalStart", intervalStart));
    return true;
}

}  // namespace audioPlayer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
