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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_TIMEUTILS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_TIMEUTILS_H_

#include <ctime>
#include <string>

#include "AVSCommon/Utils/RetryTimer.h"
#include "AVSCommon/Utils/Timing/SafeCTimeAccess.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {

/**
 * Class used to safely access the time utilities.
 */
class TimeUtils {
public:
    /**
     * Constructor.
     */
    TimeUtils();

    /**
     * Convert tm struct to time_t in UTC time
     *
     * This function is needed because mktime uses the current timezone. Hence, we calculate the current timezone
     * difference, and adjust the converted time.
     *
     * @param utcTm time to be converted. This should be in UTC time
     * @param[out] ret The converted UTC time to time_t
     * @return Whether the conversion was successful.
     */
    bool convertToUtcTimeT(const std::tm* utcTm, std::time_t* ret);

    /**
     * This function converts a string representing time, encoded in the ISO-8601 format, to what is commonly
     * known as Unix time (epoch).
     *
     * For completeness, the expected format of the input string is as follows:
     *
     * YYYY-MM-DDTHH:MM:SS+0000
     *
     * Where (in order of listing) :
     * Y means year
     * M means month
     * D means day
     * H means hour
     * M means minute
     * S means second
     *
     * So, for example:
     *
     * 1986-08-08T21:30:00+0000
     *
     * means the year 1986, August 8th, 9:30pm.
     *
     * @param timeString The time string, formatted as described above.
     * @param[out] unixTime The converted time into Unix epoch time.
     * @return Whether the conversion was successful.
     */
    bool convert8601TimeStringToUnix(const std::string& timeString, int64_t* unixTime);

    /**
     * Gets the current time in Unix epoch time, as a 64 bit integer.
     *
     * @param[out] currentTime The current time in Unix epoch time, as a 64 bit integer.
     * @return Whether the get time was successful.
     */
    bool getCurrentUnixTime(int64_t* currentTime);

    /**
     * Convert timeval struct to a ISO 8601 RFC 3339 date-time string.  This follows these specifications:
     *   - https://tools.ietf.org/html/rfc3339
     *
     * The end result will look like "1970-01-01T00:00:00.000Z"
     *
     * @param t The time from the Unix epoch to convert.
     * @param[out] iso8601TimeString The resulting time string.
     * @return True, if successful, false otherwise.
     */
    bool convertTimeToUtcIso8601Rfc3339(const struct timeval& t, std::string* iso8601TimeString);

private:
    /**
     * Calculate localtime offset in std::time_t.
     *
     * In order to calculate the timezone offset, we call gmtime and localtime giving the same arbitrary time point.
     * Then, we convert them back to time_t and calculate the conversion difference. The arbitrary time point is 24
     * hours past epoch, so we don't have to deal with negative time_t values.
     *
     * This function uses non-threadsafe time functions. Thus, it is important to use the SafeCTimeAccess class.
     *
     * @param[out] ret Required pointer to object where the result will be saved.
     * @return Whether it succeeded to calculate the localtime offset.
     */
    bool localtimeOffset(std::time_t* ret);

    /// Object used to safely access the system ctime functions.
    std::shared_ptr<SafeCTimeAccess> m_safeCTimeAccess;
};

}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_TIMEUTILS_H_
