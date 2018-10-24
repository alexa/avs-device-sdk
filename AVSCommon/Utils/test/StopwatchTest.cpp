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

/// @file StopwatchTest.cpp

#include <chrono>
#include <thread>

#include <gtest/gtest.h>

#include "AVSCommon/Utils/Timing/Stopwatch.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {
namespace test {

static const std::chrono::milliseconds TESTABLE_TIME_INCREMENT{100};

/// Test harness for Stopwatch class.
class StopwatchTest : public ::testing::Test {
public:
    bool checkElapsed(int expectedIncrement);

    Stopwatch m_stopwatch;
};

bool StopwatchTest::checkElapsed(int expectedIncrement) {
    auto elapsed = m_stopwatch.getElapsed();
    if (elapsed < TESTABLE_TIME_INCREMENT * (expectedIncrement - 1)) {
        return false;
    }
    if (elapsed > TESTABLE_TIME_INCREMENT * (expectedIncrement + 1)) {
        return false;
    }
    return true;
}

/**
 * Test good sequencing of method calls.
 */
TEST_F(StopwatchTest, goodSequencing) {
    ASSERT_TRUE(m_stopwatch.start());
    ASSERT_TRUE(m_stopwatch.pause());
    ASSERT_TRUE(m_stopwatch.resume());
    ASSERT_TRUE(m_stopwatch.pause());
    ASSERT_TRUE(m_stopwatch.resume());
    m_stopwatch.stop();
    m_stopwatch.reset();
    ASSERT_TRUE(m_stopwatch.start());
    ASSERT_TRUE(m_stopwatch.pause());
    ASSERT_TRUE(m_stopwatch.resume());
    ASSERT_TRUE(m_stopwatch.pause());
    ASSERT_TRUE(m_stopwatch.resume());
    m_stopwatch.stop();
}

/**
 * Test bad sequencing of method calls.
 */
TEST_F(StopwatchTest, badSequencing) {
    // Must be reset to start().

    ASSERT_TRUE(m_stopwatch.start());
    ASSERT_FALSE(m_stopwatch.start());

    m_stopwatch.reset();
    ASSERT_TRUE(m_stopwatch.start());
    ASSERT_TRUE(m_stopwatch.pause());
    ASSERT_FALSE(m_stopwatch.start());

    m_stopwatch.reset();
    ASSERT_TRUE(m_stopwatch.start());
    ASSERT_TRUE(m_stopwatch.pause());
    ASSERT_TRUE(m_stopwatch.resume());
    ASSERT_FALSE(m_stopwatch.start());

    m_stopwatch.reset();
    ASSERT_TRUE(m_stopwatch.start());
    ASSERT_TRUE(m_stopwatch.pause());
    ASSERT_TRUE(m_stopwatch.resume());
    m_stopwatch.stop();
    ASSERT_FALSE(m_stopwatch.start());

    // Must be started to pause().

    m_stopwatch.reset();
    ASSERT_FALSE(m_stopwatch.pause());

    m_stopwatch.reset();
    ASSERT_TRUE(m_stopwatch.start());
    ASSERT_TRUE(m_stopwatch.pause());
    ASSERT_FALSE(m_stopwatch.pause());

    m_stopwatch.reset();
    ASSERT_TRUE(m_stopwatch.start());
    ASSERT_TRUE(m_stopwatch.pause());
    ASSERT_TRUE(m_stopwatch.resume());
    ASSERT_TRUE(m_stopwatch.pause());

    m_stopwatch.reset();
    ASSERT_TRUE(m_stopwatch.start());
    m_stopwatch.stop();
    ASSERT_FALSE(m_stopwatch.pause());

    // Must be paused to resume().

    m_stopwatch.reset();
    ASSERT_FALSE(m_stopwatch.resume());

    m_stopwatch.reset();
    ASSERT_TRUE(m_stopwatch.start());
    ASSERT_FALSE(m_stopwatch.resume());

    m_stopwatch.reset();
    ASSERT_TRUE(m_stopwatch.start());
    ASSERT_TRUE(m_stopwatch.pause());
    ASSERT_TRUE(m_stopwatch.resume());
    ASSERT_FALSE(m_stopwatch.resume());

    m_stopwatch.reset();
    ASSERT_TRUE(m_stopwatch.start());
    m_stopwatch.stop();
    ASSERT_FALSE(m_stopwatch.resume());
}

/**
 * Test report of elapsed time.  This test is timing sensitive.
 */
TEST_F(StopwatchTest, testElapsed) {
    // Expect progression after start().
    ASSERT_TRUE(m_stopwatch.start());
    std::this_thread::sleep_for(TESTABLE_TIME_INCREMENT * 2);
    ASSERT_TRUE(checkElapsed(2));

    // Expect NO progression during pause().
    ASSERT_TRUE(m_stopwatch.pause());
    ASSERT_TRUE(checkElapsed(2));
    std::this_thread::sleep_for(TESTABLE_TIME_INCREMENT * 2);
    ASSERT_TRUE(checkElapsed(2));

    // Expect progression after resume().
    ASSERT_TRUE(m_stopwatch.resume());
    ASSERT_TRUE(checkElapsed(2));
    std::this_thread::sleep_for(TESTABLE_TIME_INCREMENT * 2);
    ASSERT_TRUE(checkElapsed(4));

    // Expect NO progression during pause().
    ASSERT_TRUE(m_stopwatch.pause());
    ASSERT_TRUE(checkElapsed(4));
    std::this_thread::sleep_for(TESTABLE_TIME_INCREMENT * 2);
    ASSERT_TRUE(checkElapsed(4));

    // Expect progression after resume().
    ASSERT_TRUE(m_stopwatch.resume());
    ASSERT_TRUE(checkElapsed(4));
    std::this_thread::sleep_for(TESTABLE_TIME_INCREMENT * 2);
    ASSERT_TRUE(checkElapsed(6));

    // Expect NO progression after stop().
    m_stopwatch.stop();
    ASSERT_TRUE(checkElapsed(6));
    std::this_thread::sleep_for(TESTABLE_TIME_INCREMENT * 2);
    ASSERT_TRUE(checkElapsed(6));

    // Expect NO progression after reset().
    m_stopwatch.reset();
    ASSERT_TRUE(checkElapsed(0));
    std::this_thread::sleep_for(TESTABLE_TIME_INCREMENT * 2);
    ASSERT_TRUE(checkElapsed(0));

    // Expect start() works after reset()..
    ASSERT_TRUE(m_stopwatch.start());
    std::this_thread::sleep_for(TESTABLE_TIME_INCREMENT * 2);
    ASSERT_TRUE(checkElapsed(2));
}

}  // namespace test
}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
