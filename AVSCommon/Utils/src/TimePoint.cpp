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

#include "AVSCommon/Utils/Timing/TimePoint.h"

#include "AVSCommon/Utils/Logger/Logger.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {

using namespace avsCommon::utils::logger;

/// String to identify log entries originating from this file.
static const std::string TAG("TimePoint");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

TimePoint::TimePoint() : m_time_Unix{0} {
}

bool TimePoint::setTime_ISO_8601(const std::string& time_ISO_8601) {
    int64_t tempUnixTime = 0;
    if (!m_timeUtils.convert8601TimeStringToUnix(time_ISO_8601, &tempUnixTime)) {
        ACSDK_ERROR(LX("setTime_ISO_8601Failed").d("input", time_ISO_8601).m("Could not convert to Unix time."));
        return false;
    }

    m_time_ISO_8601 = time_ISO_8601;
    m_time_Unix = tempUnixTime;
    return true;
}

std::string TimePoint::getTime_ISO_8601() const {
    return m_time_ISO_8601;
}

int64_t TimePoint::getTime_Unix() const {
    return m_time_Unix;
}

}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
