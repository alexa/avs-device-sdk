/*
 * TimeUtils.cpp
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ctime>
#include <chrono>
#include <random>

#include "AVSCommon/Utils/Timing/TimeUtils.h"
#include "AVSCommon/Utils/Logger/Logger.h"
#include "AVSCommon/Utils/String/StringUtils.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {

using namespace avsCommon::utils::logger;
using namespace avsCommon::utils::string;

/// String to identify log entries originating from this file.
static const std::string TAG("TimeUtils");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The length of the year element in an ISO-8601 formatted string.
static const int ENCODED_TIME_STRING_YEAR_STRING_LENGTH = 4;
/// The length of the month element in an ISO-8601 formatted string.
static const int ENCODED_TIME_STRING_MONTH_STRING_LENGTH = 2;
/// The length of the day element in an ISO-8601 formatted string.
static const int ENCODED_TIME_STRING_DAY_STRING_LENGTH = 2;
/// The length of the hour element in an ISO-8601 formatted string.
static const int ENCODED_TIME_STRING_HOUR_STRING_LENGTH = 2;
/// The length of the minute element in an ISO-8601 formatted string.
static const int ENCODED_TIME_STRING_MINUTE_STRING_LENGTH = 2;
/// The length of the second element in an ISO-8601 formatted string.
static const int ENCODED_TIME_STRING_SECOND_STRING_LENGTH = 2;
/// The length of the post-fix element in an ISO-8601 formatted string.
static const int ENCODED_TIME_STRING_POSTFIX_STRING_LENGTH = 4;
/// The dash separator used in an ISO-8601 formatted string.
static const std::string ENCODED_TIME_STRING_DASH_SEPARATOR_STRING = "-";
/// The 'T' separator used in an ISO-8601 formatted string.
static const std::string ENCODED_TIME_STRING_T_SEPARATOR_STRING = "T";
/// The colon separator used in an ISO-8601 formatted string.
static const std::string ENCODED_TIME_STRING_COLON_SEPARATOR_STRING = ":";
/// The plus separator used in an ISO-8601 formatted string.
static const std::string ENCODED_TIME_STRING_PLUS_SEPARATOR_STRING = "+";

/// The offset into an ISO-8601 formatted string where the year begins.
static const unsigned long ENCODED_TIME_STRING_YEAR_OFFSET = 0;
/// The offset into an ISO-8601 formatted string where the month begins.
static const unsigned long ENCODED_TIME_STRING_MONTH_OFFSET = ENCODED_TIME_STRING_YEAR_OFFSET +
                                                              ENCODED_TIME_STRING_YEAR_STRING_LENGTH +
                                                              ENCODED_TIME_STRING_DASH_SEPARATOR_STRING.length();
/// The offset into an ISO-8601 formatted string where the day begins.
static const unsigned long ENCODED_TIME_STRING_DAY_OFFSET = ENCODED_TIME_STRING_MONTH_OFFSET +
                                                            ENCODED_TIME_STRING_MONTH_STRING_LENGTH +
                                                            ENCODED_TIME_STRING_DASH_SEPARATOR_STRING.length();
/// The offset into an ISO-8601 formatted string where the hour begins.
static const unsigned long ENCODED_TIME_STRING_HOUR_OFFSET = ENCODED_TIME_STRING_DAY_OFFSET +
                                                             ENCODED_TIME_STRING_DAY_STRING_LENGTH +
                                                             ENCODED_TIME_STRING_T_SEPARATOR_STRING.length();
/// The offset into an ISO-8601 formatted string where the minute begins.
static const unsigned long ENCODED_TIME_STRING_MINUTE_OFFSET = ENCODED_TIME_STRING_HOUR_OFFSET +
                                                               ENCODED_TIME_STRING_HOUR_STRING_LENGTH +
                                                               ENCODED_TIME_STRING_COLON_SEPARATOR_STRING.length();
/// The offset into an ISO-8601 formatted string where the second begins.
static const unsigned long ENCODED_TIME_STRING_SECOND_OFFSET = ENCODED_TIME_STRING_MINUTE_OFFSET +
                                                               ENCODED_TIME_STRING_MINUTE_STRING_LENGTH +
                                                               ENCODED_TIME_STRING_COLON_SEPARATOR_STRING.length();

/// The total expected length of an ISO-8601 formatted string.
static const unsigned long ENCODED_TIME_STRING_EXPECTED_LENGTH =
    ENCODED_TIME_STRING_SECOND_OFFSET + ENCODED_TIME_STRING_SECOND_STRING_LENGTH +
    ENCODED_TIME_STRING_PLUS_SEPARATOR_STRING.length() + ENCODED_TIME_STRING_POSTFIX_STRING_LENGTH;

bool convert8601TimeStringToUnix(const std::string& timeString, int64_t* unixTime) {
    // TODO : ACSDK-387 to investigate a simpler and more portable implementation of this logic.

    if (!unixTime) {
        ACSDK_ERROR(LX("convert8601TimeStringToUnixFailed").m("unixTime parameter was nullptr."));
        return false;
    }

    std::time_t rawtime = std::time(0);
    auto timeInfo = std::localtime(&rawtime);

    if (!timeInfo) {
        ACSDK_ERROR(LX("convert8601TimeStringToUnixFailed").m("localtime returned nullptr."));
        return false;
    }

    if (timeString.length() != ENCODED_TIME_STRING_EXPECTED_LENGTH) {
        ACSDK_ERROR(LX("convert8601TimeStringToUnixFailed").d("unexpected time string length:", timeString.length()));
        return false;
    }

    if (!stringToInt(
            timeString.substr(ENCODED_TIME_STRING_YEAR_OFFSET, ENCODED_TIME_STRING_YEAR_STRING_LENGTH),
            &(timeInfo->tm_year))) {
        ACSDK_ERROR(LX("convert8601TimeStringToUnixFailed").m("error parsing year. Input:" + timeString));
        return false;
    }

    if (!stringToInt(
            timeString.substr(ENCODED_TIME_STRING_MONTH_OFFSET, ENCODED_TIME_STRING_MONTH_STRING_LENGTH),
            &(timeInfo->tm_mon))) {
        ACSDK_ERROR(LX("convert8601TimeStringToUnixFailed").m("error parsing month. Input:" + timeString));
        return false;
    }

    if (!stringToInt(
            timeString.substr(ENCODED_TIME_STRING_DAY_OFFSET, ENCODED_TIME_STRING_DAY_STRING_LENGTH),
            &(timeInfo->tm_mday))) {
        ACSDK_ERROR(LX("convert8601TimeStringToUnixFailed").m("error parsing day. Input:" + timeString));
        return false;
    }

    if (!stringToInt(
            timeString.substr(ENCODED_TIME_STRING_HOUR_OFFSET, ENCODED_TIME_STRING_HOUR_STRING_LENGTH),
            &(timeInfo->tm_hour))) {
        ACSDK_ERROR(LX("convert8601TimeStringToUnixFailed").m("error parsing hour. Input:" + timeString));
        return false;
    }

    if (!stringToInt(
            timeString.substr(ENCODED_TIME_STRING_MINUTE_OFFSET, ENCODED_TIME_STRING_MINUTE_STRING_LENGTH),
            &(timeInfo->tm_min))) {
        ACSDK_ERROR(LX("convert8601TimeStringToUnixFailed").m("error parsing minute. Input:" + timeString));
        return false;
    }

    if (!stringToInt(
            timeString.substr(ENCODED_TIME_STRING_SECOND_OFFSET, ENCODED_TIME_STRING_SECOND_STRING_LENGTH),
            &(timeInfo->tm_sec))) {
        ACSDK_ERROR(LX("convert8601TimeStringToUnixFailed").m("error parsing second. Input:" + timeString));
        return false;
    }

    // adjust for unix epoch.
    timeInfo->tm_year -= 1900;
    timeInfo->tm_mon -= 1;

    *unixTime = static_cast<int64_t>(std::mktime(timeInfo));

    return true;
}

bool getCurrentUnixTime(int64_t* currentTime) {
    // TODO : ACSDK-387 to investigate a simpler and more portable implementation of this logic.

    if (!currentTime) {
        ACSDK_ERROR(LX("getCurrentUnixTimeFailed").m("currentTime parameter was nullptr."));
        return false;
    }

    std::time_t rawtime = std::time(0);
    auto timeInfo = std::localtime(&rawtime);
    if (!timeInfo) {
        ACSDK_ERROR(LX("getCurrentUnixTimeFailed").m("localtime returned nullptr."));
        return false;
    }
    *currentTime = static_cast<int64_t>(std::mktime(timeInfo));

    return true;
}

}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
