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

#include "AVSCommon/Utils/Timing/SafeCTimeAccess.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {

std::shared_ptr<SafeCTimeAccess> SafeCTimeAccess::instance() {
    static std::shared_ptr<SafeCTimeAccess> s_safeCTimeAccess(new SafeCTimeAccess);
    return s_safeCTimeAccess;
}

bool SafeCTimeAccess::safeAccess(
    std::tm* (*timeAccessFunction)(const std::time_t* time),
    const std::time_t& time,
    std::tm* calendarTime) {
    // No logging on errors, because it's known that logging calls this function, which can cause recursion problems.

    if (!calendarTime) {
        return false;
    }

    bool succeeded = false;
    {
        std::lock_guard<std::mutex> lock{m_timeLock};
        auto tempCalendarTime = timeAccessFunction(&time);
        if (tempCalendarTime) {
            *calendarTime = *tempCalendarTime;
            succeeded = true;
        }
    }

    return succeeded;
}

bool SafeCTimeAccess::getGmtime(const std::time_t& time, std::tm* calendarTime) {
    return safeAccess(std::gmtime, time, calendarTime);
}

bool SafeCTimeAccess::getLocaltime(const std::time_t& time, std::tm* calendarTime) {
    return safeAccess(std::localtime, time, calendarTime);
}

}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
