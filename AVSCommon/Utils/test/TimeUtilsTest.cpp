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

#include <string>
#include <ctime>
#include <gtest/gtest.h>

#include "AVSCommon/Utils/Timing/TimeUtils.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {
namespace test {

TEST(TimeTest, testStringConversion) {
    std::string dateStr{"1986-08-10T21:30:00+0000"};
    int64_t date;
    auto success = convert8601TimeStringToUnix(dateStr, &date);
    ASSERT_TRUE(success);

    auto dateTimeT = static_cast<time_t>(date);
    auto dateTm = std::gmtime(&dateTimeT);
    ASSERT_EQ(dateTm->tm_year, 86);
    ASSERT_EQ(dateTm->tm_mon, 7);
    ASSERT_EQ(dateTm->tm_mday, 10);
    ASSERT_EQ(dateTm->tm_hour, 21);
    ASSERT_EQ(dateTm->tm_min, 30);
}

TEST(TimeTest, testStringConversionError) {
    std::string dateStr{"1986-8-10T21:30:00+0000"};
    int64_t date;
    auto success = convert8601TimeStringToUnix(dateStr, &date);
    ASSERT_FALSE(success);
}

TEST(TimeTest, testStringConversionNullParam) {
    std::string dateStr{"1986-8-10T21:30:00+0000"};
    auto success = convert8601TimeStringToUnix(dateStr, nullptr);
    ASSERT_FALSE(success);
}

TEST(TimeTest, testTimeConversion) {
    std::time_t randomDate = 524089800;
    auto date = std::gmtime(&randomDate);
    std::time_t convertBack;
    auto success = convertToUtcTimeT(date, &convertBack);

    ASSERT_TRUE(success);
    ASSERT_EQ(randomDate, convertBack);
}

TEST(TimeTest, testCurrentTime) {
    int64_t time = -1;
    auto success = getCurrentUnixTime(&time);

    ASSERT_TRUE(success);
    ASSERT_GT(time, 0);
}

TEST(TimeTest, testCurrentTimeNullParam) {
    auto success = getCurrentUnixTime(nullptr);
    ASSERT_FALSE(success);
}

}  // namespace test
}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
