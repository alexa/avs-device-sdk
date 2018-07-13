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

#include <ctime>
#include <string>

#include <gtest/gtest.h>

#include "AVSCommon/Utils/Timing/SafeCTimeAccess.h"
#include "AVSCommon/Utils/Timing/TimeUtils.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {
namespace test {

TEST(TimeTest, testStringConversion) {
    TimeUtils timeUtils;
    std::string dateStr{"1986-08-10T21:30:00+0000"};
    int64_t date;
    auto success = timeUtils.convert8601TimeStringToUnix(dateStr, &date);
    ASSERT_TRUE(success);

    auto dateTimeT = static_cast<time_t>(date);
    std::tm dateTm;
    auto safeCTimeAccess = SafeCTimeAccess::instance();
    ASSERT_TRUE(safeCTimeAccess->getGmtime(dateTimeT, &dateTm));
    ASSERT_EQ(dateTm.tm_year, 86);
    ASSERT_EQ(dateTm.tm_mon, 7);
    ASSERT_EQ(dateTm.tm_mday, 10);
    ASSERT_EQ(dateTm.tm_hour, 21);
    ASSERT_EQ(dateTm.tm_min, 30);
}

TEST(TimeTest, testStringConversionError) {
    TimeUtils timeUtils;
    std::string dateStr{"1986-8-10T21:30:00+0000"};
    int64_t date;
    auto success = timeUtils.convert8601TimeStringToUnix(dateStr, &date);
    ASSERT_FALSE(success);
}

TEST(TimeTest, testStringConversionNullParam) {
    TimeUtils timeUtils;
    std::string dateStr{"1986-8-10T21:30:00+0000"};
    auto success = timeUtils.convert8601TimeStringToUnix(dateStr, nullptr);
    ASSERT_FALSE(success);
}

TEST(TimeTest, testTimeConversion) {
    TimeUtils timeUtils;
    std::time_t randomDate = 524089800;
    std::tm date;
    auto safeCTimeAccess = SafeCTimeAccess::instance();
    ASSERT_TRUE(safeCTimeAccess->getGmtime(randomDate, &date));
    std::time_t convertBack;
    auto success = timeUtils.convertToUtcTimeT(&date, &convertBack);

    ASSERT_TRUE(success);
    ASSERT_EQ(randomDate, convertBack);
}

TEST(TimeTest, testCurrentTime) {
    TimeUtils timeUtils;
    int64_t time = -1;
    auto success = timeUtils.getCurrentUnixTime(&time);

    ASSERT_TRUE(success);
    ASSERT_GT(time, 0);
}

TEST(TimeTest, testCurrentTimeNullParam) {
    TimeUtils timeUtils;
    auto success = timeUtils.getCurrentUnixTime(nullptr);
    ASSERT_FALSE(success);
}

/**
 * Helper function to run through the test cases for time to string conversions.
 *
 * @param expectedString The string that convertTimeToUtcIso8601Rfc3339 should generate.
 * @param t The timeval to convert.
 */
static void testIso8601ConversionHelper(const std::string& expectedString, const struct timeval& t) {
    TimeUtils timeUtils;
    std::string resultString;
    EXPECT_TRUE(timeUtils.convertTimeToUtcIso8601Rfc3339(t, &resultString));
    EXPECT_EQ(expectedString, resultString);
}

TEST(TimeTest, testIso8601Conversion) {
    testIso8601ConversionHelper("1970-01-01T00:00:00.000Z", {0, 0});
    testIso8601ConversionHelper("1970-01-01T00:00:01.000Z", {1, 0});
    testIso8601ConversionHelper("1970-01-01T00:00:00.001Z", {0, 1000});
    testIso8601ConversionHelper("1970-01-01T00:01:00.000Z", {60, 0});
    testIso8601ConversionHelper("1970-01-01T01:00:00.000Z", {60 * 60, 0});
    testIso8601ConversionHelper("1970-01-02T00:00:00.000Z", {60 * 60 * 24, 0});
    testIso8601ConversionHelper("1970-02-01T00:00:00.000Z", {60 * 60 * 24 * 31, 0});
    testIso8601ConversionHelper("1971-01-01T00:00:00.000Z", {60 * 60 * 24 * 365, 0});

    testIso8601ConversionHelper("1970-01-02T00:00:00.000Z", {60 * 60 * 24, 999});
    testIso8601ConversionHelper("1970-01-02T00:00:00.001Z", {60 * 60 * 24, 1000});
    testIso8601ConversionHelper("1970-01-02T00:00:00.001Z", {60 * 60 * 24, 1001});
    testIso8601ConversionHelper("1970-01-02T00:00:00.001Z", {60 * 60 * 24, 1999});
    testIso8601ConversionHelper("1970-01-02T00:00:00.002Z", {60 * 60 * 24, 2000});
    testIso8601ConversionHelper("1970-01-02T00:00:00.002Z", {60 * 60 * 24, 2001});
    testIso8601ConversionHelper("1970-01-02T00:00:00.202Z", {60 * 60 * 24, 202001});
}

}  // namespace test
}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
