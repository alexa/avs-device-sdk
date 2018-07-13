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

#include <chrono>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <random>
#include <sstream>

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

/**
 * Utility function that wraps localtime conversion to std::time_t.
 *
 * This function also creates a copy of the given timeStruct since mktime can
 * change the object.
 *
 * @param timeStruct Required pointer to timeStruct to be converted to time_t.
 * @param[out] ret Required pointer to object where the result will be saved.
 * @return Whether the conversion was successful.
 */
static bool convertToLocalTimeT(const std::tm* timeStruct, std::time_t* ret) {
    if (timeStruct == nullptr) {
        return false;
    }

    std::tm tmCopy = *timeStruct;
    *ret = std::mktime(&tmCopy);
    return *ret >= 0;
}

TimeUtils::TimeUtils() : m_safeCTimeAccess{SafeCTimeAccess::instance()} {
}

bool TimeUtils::convertToUtcTimeT(const std::tm* utcTm, std::time_t* ret) {
    std::time_t converted;
    std::time_t offset;

    if (ret == nullptr) {
        ACSDK_ERROR(LX("convertToUtcTimeT").m("return variable is null"));
        return false;
    }

    if (!convertToLocalTimeT(utcTm, &converted) || !localtimeOffset(&offset)) {
        ACSDK_ERROR(LX("convertToUtcTimeT").m("failed to convert to local time"));
        return false;
    }

    // adjust converted time
    *ret = converted - offset;
    return true;
}

bool TimeUtils::convert8601TimeStringToUnix(const std::string& timeString, int64_t* convertedTime) {
    // TODO : Use std::get_time once we only support compilers that implement this function (GCC 5.1+ / Clang 3.3+)

    if (!convertedTime) {
        ACSDK_ERROR(LX("convert8601TimeStringToUnixFailed").m("convertedTime parameter was nullptr."));
        return false;
    }

    std::tm timeInfo;

    if (timeString.length() != ENCODED_TIME_STRING_EXPECTED_LENGTH) {
        ACSDK_ERROR(LX("convert8601TimeStringToUnixFailed").d("unexpected time string length:", timeString.length()));
        return false;
    }

    if (!stringToInt(
            timeString.substr(ENCODED_TIME_STRING_YEAR_OFFSET, ENCODED_TIME_STRING_YEAR_STRING_LENGTH),
            &(timeInfo.tm_year))) {
        ACSDK_ERROR(LX("convert8601TimeStringToUnixFailed").m("error parsing year. Input:" + timeString));
        return false;
    }

    if (!stringToInt(
            timeString.substr(ENCODED_TIME_STRING_MONTH_OFFSET, ENCODED_TIME_STRING_MONTH_STRING_LENGTH),
            &(timeInfo.tm_mon))) {
        ACSDK_ERROR(LX("convert8601TimeStringToUnixFailed").m("error parsing month. Input:" + timeString));
        return false;
    }

    if (!stringToInt(
            timeString.substr(ENCODED_TIME_STRING_DAY_OFFSET, ENCODED_TIME_STRING_DAY_STRING_LENGTH),
            &(timeInfo.tm_mday))) {
        ACSDK_ERROR(LX("convert8601TimeStringToUnixFailed").m("error parsing day. Input:" + timeString));
        return false;
    }

    if (!stringToInt(
            timeString.substr(ENCODED_TIME_STRING_HOUR_OFFSET, ENCODED_TIME_STRING_HOUR_STRING_LENGTH),
            &(timeInfo.tm_hour))) {
        ACSDK_ERROR(LX("convert8601TimeStringToUnixFailed").m("error parsing hour. Input:" + timeString));
        return false;
    }

    if (!stringToInt(
            timeString.substr(ENCODED_TIME_STRING_MINUTE_OFFSET, ENCODED_TIME_STRING_MINUTE_STRING_LENGTH),
            &(timeInfo.tm_min))) {
        ACSDK_ERROR(LX("convert8601TimeStringToUnixFailed").m("error parsing minute. Input:" + timeString));
        return false;
    }

    if (!stringToInt(
            timeString.substr(ENCODED_TIME_STRING_SECOND_OFFSET, ENCODED_TIME_STRING_SECOND_STRING_LENGTH),
            &(timeInfo.tm_sec))) {
        ACSDK_ERROR(LX("convert8601TimeStringToUnixFailed").m("error parsing second. Input:" + timeString));
        return false;
    }

    // adjust for C struct tm standard
    timeInfo.tm_isdst = 0;
    timeInfo.tm_year -= 1900;
    timeInfo.tm_mon -= 1;

    std::time_t convertedTimeT;
    bool ok = convertToUtcTimeT(&timeInfo, &convertedTimeT);

    if (!ok) {
        return false;
    }

    *convertedTime = static_cast<int64_t>(convertedTimeT);
    return true;
}

bool TimeUtils::getCurrentUnixTime(int64_t* currentTime) {
    if (!currentTime) {
        ACSDK_ERROR(LX("getCurrentUnixTimeFailed").m("currentTime parameter was nullptr."));
        return false;
    }

    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    *currentTime = static_cast<int64_t>(now);

    return now >= 0;
}

bool TimeUtils::convertTimeToUtcIso8601Rfc3339(const struct timeval& timeVal, std::string* iso8601TimeString) {
    // The length of the RFC 3339 string for the time is maximum 28 characters, include an extra byte for the '\0'
    // terminator.
    char buf[29];
    memset(buf, 0, sizeof(buf));

    // Need to assign it to time_t since time_t in some platforms is long long
    // and timeVal.tv_sec is long in some platforms
    const time_t timeSecs = timeVal.tv_sec;

    std::tm utcTm;
    if (!m_safeCTimeAccess->getGmtime(timeSecs, &utcTm)) {
        ACSDK_ERROR(LX("convertTimeToUtcIso8601Rfc3339").m("cannot retrieve tm struct"));
        return false;
    }

    // it's possible for std::strftime to correctly return length = 0, but not with the format string used.  In this
    // case length == 0 is an error.
    auto strftimeResult = std::strftime(buf, sizeof(buf) - 1, "%Y-%m-%dT%H:%M:%S", &utcTm);
    if (strftimeResult == 0) {
        ACSDK_ERROR(LX("convertTimeToUtcIso8601Rfc3339Failed").m("strftime(..) failed"));
        return false;
    }

    std::stringstream millisecondTrailer;
    millisecondTrailer << buf << "." << std::setfill('0') << std::setw(3) << (timeVal.tv_usec / 1000) << "Z";

    *iso8601TimeString = millisecondTrailer.str();
    return true;
}

bool TimeUtils::localtimeOffset(std::time_t* ret) {
    static const std::chrono::time_point<std::chrono::system_clock> timePoint{std::chrono::hours(24)};
    auto fixedTime = std::chrono::system_clock::to_time_t(timePoint);

    std::tm utcTm;
    std::time_t utc;
    std::tm localTm;
    std::time_t local;
    if (!m_safeCTimeAccess->getGmtime(fixedTime, &utcTm) || !convertToLocalTimeT(&utcTm, &utc) ||
        !m_safeCTimeAccess->getLocaltime(fixedTime, &localTm) || !convertToLocalTimeT(&localTm, &local)) {
        ACSDK_ERROR(LX("localtimeOffset").m("cannot retrieve tm struct"));
        return false;
    }

    *ret = utc - local;
    return true;
}

}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
