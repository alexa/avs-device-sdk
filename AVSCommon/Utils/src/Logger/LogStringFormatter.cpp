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

#include <cstdio>
#include <iostream>

#include "AVSCommon/Utils/Logger/LogStringFormatter.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

/// Format string for strftime() to produce date and time in the format "YYYY-MM-DD HH:MM:SS".
static const char* STRFTIME_FORMAT_STRING = "%Y-%m-%d %H:%M:%S";

/// Size of buffer needed to hold "YYYY-MM-DD HH:MM:SS" and a null terminator.
static const int DATE_AND_TIME_STRING_SIZE = 20;

/// Separator between date/time and millis.
static const char TIME_AND_MILLIS_SEPARATOR = '.';

// Format string for sprintf() to produce millis in the format "nnn".
static const char* MILLIS_FORMAT_STRING = "%03d";

/// Size of buffer needed to hold "nnn" (milliseconds value) and a null terminator
static const int MILLIS_STRING_SIZE = 4;

/// Separator string between milliseconds value and ExampleLogger name.
static const std::string MILLIS_AND_THREAD_SEPARATOR = " [";

/// Separator between thread ID and level indicator in log lines.
static const std::string THREAD_AND_LEVEL_SEPARATOR = "] ";

/// Separator between level indicator and text in log lines.
static const char LEVEL_AND_TEXT_SEPARATOR = ' ';

/// Number of milliseconds per second.
static const int MILLISECONDS_PER_SECOND = 1000;

LogStringFormatter::LogStringFormatter() : m_safeCTimeAccess(timing::SafeCTimeAccess::instance()) {
}

std::string LogStringFormatter::format(
    Level level,
    std::chrono::system_clock::time_point time,
    const char* threadMoniker,
    const char* text) {
    bool dateTimeFailure = false;
    bool millisecondFailure = false;
    char dateTimeString[DATE_AND_TIME_STRING_SIZE];
    auto timeAsTime_t = std::chrono::system_clock::to_time_t(time);
    std::tm timeAsTm;
    if (!m_safeCTimeAccess->getGmtime(timeAsTime_t, &timeAsTm) ||
        0 == strftime(dateTimeString, sizeof(dateTimeString), STRFTIME_FORMAT_STRING, &timeAsTm)) {
        dateTimeFailure = true;
    }
    auto timeMillisPart = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()).count() %
        MILLISECONDS_PER_SECOND);
    char millisString[MILLIS_STRING_SIZE];
    if (std::snprintf(millisString, sizeof(millisString), MILLIS_FORMAT_STRING, timeMillisPart) < 0) {
        millisecondFailure = true;
    }

    std::stringstream stringToEmit;
    stringToEmit << (dateTimeFailure ? "ERROR: strftime() failed.  Date and time not logged." : dateTimeString)
                 << TIME_AND_MILLIS_SEPARATOR
                 << (millisecondFailure ? "ERROR: snprintf() failed.  Milliseconds not logged." : millisString)
                 << MILLIS_AND_THREAD_SEPARATOR << threadMoniker << THREAD_AND_LEVEL_SEPARATOR
                 << convertLevelToChar(level) << LEVEL_AND_TEXT_SEPARATOR << text;
    return stringToEmit.str();
}

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
