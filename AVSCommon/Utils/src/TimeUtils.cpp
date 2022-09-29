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
#define TAG "TimeUtils"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The length of the year element in an ISO-8601 formatted string.
static const std::size_t ENCODED_TIME_STRING_YEAR_STRING_LENGTH = 4;
/// The length of the month element in an ISO-8601 formatted string.
static const std::size_t ENCODED_TIME_STRING_MONTH_STRING_LENGTH = 2;
/// The length of the day element in an ISO-8601 formatted string.
static const std::size_t ENCODED_TIME_STRING_DAY_STRING_LENGTH = 2;
/// The length of the hour element in an ISO-8601 formatted string.
static const std::size_t ENCODED_TIME_STRING_HOUR_STRING_LENGTH = 2;
/// The length of the minute element in an ISO-8601 formatted string.
static const std::size_t ENCODED_TIME_STRING_MINUTE_STRING_LENGTH = 2;
/// The length of the second element in an ISO-8601 formatted string.
static const std::size_t ENCODED_TIME_STRING_SECOND_STRING_LENGTH = 2;
/// The length of the post-fix element in an ISO-8601 formatted string.
static const std::size_t ENCODED_TIME_STRING_POSTFIX_STRING_LENGTH = 4;
/// The dash separator used in an ISO-8601 formatted string.
static const std::string ENCODED_TIME_STRING_DASH_SEPARATOR_STRING = "-";
/// The 'T' separator used in an ISO-8601 formatted string.
static const std::string ENCODED_TIME_STRING_T_SEPARATOR_STRING = "T";
/// The 'Z' zone designator used in an ISO-8601 formatted string for zero UTC offset.
static const std::string ENCODED_TIME_STRING_Z_DESIGNATOR = "Z";
/// The colon separator used in an ISO-8601 formatted string.
static const std::string ENCODED_TIME_STRING_COLON_SEPARATOR_STRING = ":";
/// The plus separator used in an ISO-8601 formatted string.
static const std::string ENCODED_TIME_STRING_PLUS_SEPARATOR_STRING = "+";

/// The offset into an ISO-8601 formatted string where the year begins.
static const unsigned long ENCODED_TIME_STRING_YEAR_OFFSET = 0;
/// The offset into an ISO-8601 formatted string where the month begins.
static const std::size_t ENCODED_TIME_STRING_MONTH_OFFSET = ENCODED_TIME_STRING_YEAR_OFFSET +
                                                            ENCODED_TIME_STRING_YEAR_STRING_LENGTH +
                                                            ENCODED_TIME_STRING_DASH_SEPARATOR_STRING.length();
/// The offset into an ISO-8601 formatted string where the day begins.
static const std::size_t ENCODED_TIME_STRING_DAY_OFFSET = ENCODED_TIME_STRING_MONTH_OFFSET +
                                                          ENCODED_TIME_STRING_MONTH_STRING_LENGTH +
                                                          ENCODED_TIME_STRING_DASH_SEPARATOR_STRING.length();
/// The offset into an ISO-8601 formatted string where the hour begins.
static const std::size_t ENCODED_TIME_STRING_HOUR_OFFSET = ENCODED_TIME_STRING_DAY_OFFSET +
                                                           ENCODED_TIME_STRING_DAY_STRING_LENGTH +
                                                           ENCODED_TIME_STRING_T_SEPARATOR_STRING.length();
/// The offset into an ISO-8601 formatted string where the minute begins.
static const std::size_t ENCODED_TIME_STRING_MINUTE_OFFSET = ENCODED_TIME_STRING_HOUR_OFFSET +
                                                             ENCODED_TIME_STRING_HOUR_STRING_LENGTH +
                                                             ENCODED_TIME_STRING_COLON_SEPARATOR_STRING.length();
/// The offset into an ISO-8601 formatted string where the second begins.
static const std::size_t ENCODED_TIME_STRING_SECOND_OFFSET = ENCODED_TIME_STRING_MINUTE_OFFSET +
                                                             ENCODED_TIME_STRING_MINUTE_STRING_LENGTH +
                                                             ENCODED_TIME_STRING_COLON_SEPARATOR_STRING.length();

/// The total expected length of an ISO-8601 formatted string.
static const std::size_t ENCODED_TIME_STRING_EXPECTED_LENGTH =
    ENCODED_TIME_STRING_SECOND_OFFSET + ENCODED_TIME_STRING_SECOND_STRING_LENGTH +
    ENCODED_TIME_STRING_PLUS_SEPARATOR_STRING.length() + ENCODED_TIME_STRING_POSTFIX_STRING_LENGTH;

/// The total expected length of an ISO-8601 formatted string with UTC time.
static const std::size_t ENCODED_TIME_STRING_EXPECTED_LENGTH_UTC = ENCODED_TIME_STRING_SECOND_OFFSET +
                                                                   ENCODED_TIME_STRING_SECOND_STRING_LENGTH +
                                                                   ENCODED_TIME_STRING_Z_DESIGNATOR.length();

/**
 * Utility function that wraps localtime conversion to std::time_t.
 *
 * This function also creates a copy of the given timeStruct since mktime can
 * change the object. Note that the tm_isdst is set to -1 as we leave it to
 * std::mktime to determine if daylight savings is in effect.
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
    tmCopy.tm_isdst = -1;
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

    if (!convertToLocalTimeT(utcTm, &converted) || !localtimeOffset(converted, &offset)) {
        ACSDK_ERROR(LX("convertToUtcTimeT").m("failed to convert to local time"));
        return false;
    }

    // adjust converted time
    *ret = converted - offset;
    return true;
}

bool TimeUtils::convert8601TimeStringToUtcTimePoint(
    const std::string& iso8601TimeString,
    std::chrono::system_clock::time_point* tp) {
    if (!tp) {
        ACSDK_ERROR(LX("convert8601TimeStringToUtcTimePoint").m("tp was nullptr."));
        return false;
    }
    std::time_t timeT;
    if (!convert8601TimeStringToTimeT(iso8601TimeString, &timeT)) {
        ACSDK_ERROR(LX("convert8601TimeStringToUtcTimePointFailed").m("convert8601TimeStringToTimeT failed"));
        return false;
    }
    *tp = std::chrono::system_clock::from_time_t(timeT);
    return true;
}

bool TimeUtils::convert8601TimeStringToUnix(const std::string& timeString, int64_t* convertedTime) {
    if (!convertedTime) {
        ACSDK_ERROR(LX("convert8601TimeStringToUnixFailed").m("convertedTime was nullptr."));
        return false;
    }
    std::time_t timeT;
    if (!convert8601TimeStringToTimeT(timeString, &timeT)) {
        ACSDK_ERROR(LX("convert8601TimeStringToUnixFailed").m("convert8601TimeStringToTimeT failed"));
        return false;
    }

    *convertedTime = static_cast<int64_t>(timeT);
    return true;
}

bool TimeUtils::convert8601TimeStringToTimeT(const std::string& iso8601TimeString, std::time_t* timeT) {
    // TODO : Use std::get_time once we only support compilers that implement this function (GCC 5.1+ / Clang 3.3+)

    if (!timeT) {
        ACSDK_ERROR(LX("convert8601TimeStringToTimeTFailed").m("timeT parameter was nullptr."));
        return false;
    }

    std::tm timeInfo;

    if (iso8601TimeString.length() != ENCODED_TIME_STRING_EXPECTED_LENGTH &&
        iso8601TimeString.length() != ENCODED_TIME_STRING_EXPECTED_LENGTH_UTC) {
        ACSDK_ERROR(
            LX("convert8601TimeStringToTimeTFailed").d("unexpected time string length:", iso8601TimeString.length()));
        return false;
    }

    if (!stringToInt(
            iso8601TimeString.substr(ENCODED_TIME_STRING_YEAR_OFFSET, ENCODED_TIME_STRING_YEAR_STRING_LENGTH),
            &(timeInfo.tm_year))) {
        ACSDK_ERROR(LX("convert8601TimeStringToTimeTFailed").m("error parsing year. Input:" + iso8601TimeString));
        return false;
    }

    if (!stringToInt(
            iso8601TimeString.substr(ENCODED_TIME_STRING_MONTH_OFFSET, ENCODED_TIME_STRING_MONTH_STRING_LENGTH),
            &(timeInfo.tm_mon))) {
        ACSDK_ERROR(LX("convert8601TimeStringToTimeTFailed").m("error parsing month. Input:" + iso8601TimeString));
        return false;
    }

    if (!stringToInt(
            iso8601TimeString.substr(ENCODED_TIME_STRING_DAY_OFFSET, ENCODED_TIME_STRING_DAY_STRING_LENGTH),
            &(timeInfo.tm_mday))) {
        ACSDK_ERROR(LX("convert8601TimeStringToTimeTFailed").m("error parsing day. Input:" + iso8601TimeString));
        return false;
    }

    if (!stringToInt(
            iso8601TimeString.substr(ENCODED_TIME_STRING_HOUR_OFFSET, ENCODED_TIME_STRING_HOUR_STRING_LENGTH),
            &(timeInfo.tm_hour))) {
        ACSDK_ERROR(LX("convert8601TimeStringToTimeTFailed").m("error parsing hour. Input:" + iso8601TimeString));
        return false;
    }

    if (!stringToInt(
            iso8601TimeString.substr(ENCODED_TIME_STRING_MINUTE_OFFSET, ENCODED_TIME_STRING_MINUTE_STRING_LENGTH),
            &(timeInfo.tm_min))) {
        ACSDK_ERROR(LX("convert8601TimeStringToTimeTFailed").m("error parsing minute. Input:" + iso8601TimeString));
        return false;
    }

    if (!stringToInt(
            iso8601TimeString.substr(ENCODED_TIME_STRING_SECOND_OFFSET, ENCODED_TIME_STRING_SECOND_STRING_LENGTH),
            &(timeInfo.tm_sec))) {
        ACSDK_ERROR(LX("convert8601TimeStringToTimeTFailed").m("error parsing second. Input:" + iso8601TimeString));
        return false;
    }

    // adjust for C struct tm standard
    timeInfo.tm_year -= 1900;
    timeInfo.tm_mon -= 1;

    return convertToUtcTimeT(&timeInfo, timeT);
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

bool TimeUtils::convertTimeToUtcIso8601Rfc3339(
    const std::chrono::system_clock::time_point& tp,
    std::string* iso8601TimeString) {
    // The length of the RFC 3339 string for the time is maximum 28 characters, include an extra byte for the '\0'
    // terminator.
    char buf[29];
    memset(buf, 0, sizeof(buf));

    // Need to assign it to time_t since time_t in some platforms is long long
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
    auto sec = std::chrono::duration_cast<std::chrono::seconds>(ms);
    const time_t timeSecs = static_cast<time_t>(sec.count());

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
    millisecondTrailer << buf << "." << std::setfill('0') << std::setw(3) << (ms.count() % 1000) << "Z";

    *iso8601TimeString = millisecondTrailer.str();
    return true;
}

bool TimeUtils::localtimeOffset(std::time_t referenceTime, std::time_t* ret) {
    std::tm utcTm;
    std::time_t utc;
    std::tm localTm;
    std::time_t local;
    if (!m_safeCTimeAccess->getGmtime(referenceTime, &utcTm) || !convertToLocalTimeT(&utcTm, &utc) ||
        !m_safeCTimeAccess->getLocaltime(referenceTime, &localTm) || !convertToLocalTimeT(&localTm, &local)) {
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
