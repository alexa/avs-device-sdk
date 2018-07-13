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
#include <thread>

#include <gtest/gtest.h>

#include "AVSCommon/Utils/Timing/SafeCTimeAccess.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {
namespace test {

/// A bound on the upper time to check.
const time_t LARGE_TIME_VALUE = (0x1 << 30) - 1;

/// Test to verify that getGmtime returns failure if a nullptr is passed for the result variable.
TEST(SafeCTimeAccessTest, getGmtimeNullReturnValue) {
    auto safeCTimeAccess = SafeCTimeAccess::instance();
    ASSERT_FALSE(safeCTimeAccess->getGmtime(0, nullptr));
}

/// Test to verify that getLocaltime returns failure if a nullptr is passed for the result variable.
TEST(SafeCTimeAccessTest, getLocaltimeNullReturnValue) {
    auto safeCTimeAccess = SafeCTimeAccess::instance();
    ASSERT_FALSE(safeCTimeAccess->getLocaltime(0, nullptr));
}

/**
 * Utility function to check to see if two std::tm objects are equal.
 *
 * @param a One of the structures to check.
 * @param b The other structure to check.
 */
static void checkTm(const std::tm& a, const std::tm& b) {
    EXPECT_EQ(a.tm_sec, b.tm_sec);
    EXPECT_EQ(a.tm_min, b.tm_min);
    EXPECT_EQ(a.tm_hour, b.tm_hour);
    EXPECT_EQ(a.tm_mday, b.tm_mday);
    EXPECT_EQ(a.tm_mon, b.tm_mon);
    EXPECT_EQ(a.tm_year, b.tm_year);
    EXPECT_EQ(a.tm_wday, b.tm_wday);
    EXPECT_EQ(a.tm_yday, b.tm_yday);
    EXPECT_EQ(a.tm_isdst, b.tm_isdst);
}

/**
 * Helper function to run through the test cases for getGmtime testing.
 *
 * @param expected The std::tm structure string that getGmtime should generate.
 * @param t The time_t to convert.
 */
static void testGmtimeHelper(const std::tm& expected, const time_t& t) {
    auto safeCTimeAccess = SafeCTimeAccess::instance();
    std::tm result;
    EXPECT_TRUE(safeCTimeAccess->getGmtime(t, &result));
    checkTm(expected, result);
}

/**
 * Helper function to run through the test cases for getLocaltime testing.
 *
 * @param expected The std::tm structure string that getLocaltime should generate.
 * @param t The time_t to convert.
 */
static void testLocaltimeHelper(const std::tm& expected, const time_t& t) {
    auto safeCTimeAccess = SafeCTimeAccess::instance();
    std::tm result;
    EXPECT_TRUE(safeCTimeAccess->getLocaltime(t, &result));
    checkTm(expected, result);
}

/// Test to verify that getGmtime returns the correct calendar date for the Unix epoch.
TEST(SafeCTimeAccessTest, getGmtimeAtTheEpoch) {
    std::tm epoch;
    epoch.tm_sec = 0;
    epoch.tm_min = 0;
    epoch.tm_hour = 0;
    epoch.tm_mday = 1;
    epoch.tm_mon = 0;
    epoch.tm_year = 70;
    epoch.tm_wday = 4;
    epoch.tm_yday = 0;
    epoch.tm_isdst = 0;
    testGmtimeHelper(epoch, 0);
}

/// Test to verify that getGmtime returns the same calendar date as std::gmtime.
TEST(SafeCTimeAccessTest, getGmtime) {
    for (time_t t = 0; t < LARGE_TIME_VALUE; t = 2 * (t + 1)) {
        auto gmtimeResult = std::gmtime(&t);
        ASSERT_NE(nullptr, gmtimeResult);
        testGmtimeHelper(*gmtimeResult, t);
    }
}

/// Test to verify that getLocaltime returns the same calendar date as std::localtime.
TEST(SafeCTimeAccessTest, getLocaltime) {
    for (time_t t = 0; t < LARGE_TIME_VALUE; t = 2 * (t + 1)) {
        auto localtimeResult = std::localtime(&t);
        ASSERT_NE(nullptr, localtimeResult);
        testLocaltimeHelper(*localtimeResult, t);
    }
}

/**
 * The test code for SafeCTimeTest::getGmtime and SafeCTimeTest::getLocaltime is almost identical, this allows switching
 * between them.
 */
enum class TestType { GMTIME, LOCALTIME };

/// Mutex to guard access to the results vector.
static std::mutex g_resultsLock;

/**
 * Function that accesses the safe time functions that is called from many threads.  We call the function in a tight
 * loop with no lock and save the results to be checked later.
 *
 * @param startingSeed This is used to make sure that different time values are checked.
 * @param type The flavor of testing to perform.  Can be GMTIME or LOCALTIME.
 * @param results A vector where the results are appended to be checked later.
 */
static void callSafeCTimeFunction(
    int startingSeed,
    const TestType& type,
    std::vector<std::pair<time_t, std::tm>>* results) {
    auto safeCTimeAccess = SafeCTimeAccess::instance();
    std::vector<std::pair<time_t, std::tm>> internalResults;
    for (int i = 0; i < 4; ++i) {
        for (time_t t = startingSeed; t < LARGE_TIME_VALUE; t = 1.5 * (t + 1)) {
            std::tm result;
            switch (type) {
                case TestType::GMTIME:
                    EXPECT_TRUE(safeCTimeAccess->getGmtime(t, &result));
                    break;
                case TestType::LOCALTIME:
                    EXPECT_TRUE(safeCTimeAccess->getLocaltime(t, &result));
                    break;
                default:
                    EXPECT_TRUE(false);
                    break;
            };
            internalResults.push_back(std::make_pair(t, result));
        }
    }

    std::lock_guard<std::mutex> lock{g_resultsLock};
    results->insert(std::end(*results), std::begin(internalResults), std::end(internalResults));
}

/**
 * Main function for testing multithreaded access to the safe time functions.  This function starts multiple threads
 * that continuously access the time functions in an externally thread-unsafe manner.  The results are collected in a
 * single vector, where they are compared against the matching system function.
 *
 * @param type The flavor of testing to perform.  Can be GMTIME or LOCALTIME.
 */
static void checkSafeCTimeFunction(const TestType& type) {
    std::vector<std::pair<time_t, std::tm>> results;
    const int NUMBER_OF_THREADS = 254;
    std::thread threads[NUMBER_OF_THREADS];
    for (int threadIndex = 0; threadIndex < NUMBER_OF_THREADS; ++threadIndex) {
        threads[threadIndex] = std::thread(callSafeCTimeFunction, threadIndex, type, &results);
    }

    for (int threadIndex = 0; threadIndex < NUMBER_OF_THREADS; ++threadIndex) {
        threads[threadIndex].join();
    }

    for (const auto& result : results) {
        std::tm* stdResult;
        switch (type) {
            case TestType::GMTIME:
                stdResult = std::gmtime(&result.first);
                break;
            case TestType::LOCALTIME:
                stdResult = std::localtime(&result.first);
                break;
            default:
                EXPECT_TRUE(false);
                break;
        };
        EXPECT_NE(nullptr, stdResult);
        checkTm(*stdResult, result.second);
    }
}

/// Test to make sure that multithreaded access SafeCTimeAccess::getGmtime is safe.
TEST(SafeCTimeAccessTest, DISABLED_gmTimeMultithreadedAccess) {
    // TODO: ACSDK-1208 investigate Pi failure
    checkSafeCTimeFunction(TestType::GMTIME);
}

/// Test to make sure that multithreaded access SafeCTimeAccess::getLocaltimetime is safe.
TEST(SafeCTimeAccessTest, DISABLED_localtimeMultithreadedAccess) {
    // TODO: ACSDK-1208 investigate Pi failure
    checkSafeCTimeFunction(TestType::LOCALTIME);
}

}  // namespace test
}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
