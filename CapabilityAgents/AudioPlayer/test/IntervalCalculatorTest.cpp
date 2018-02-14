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

#include <gmock/gmock.h>

#include "AudioPlayer/IntervalCalculator.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace audioPlayer {
namespace test {

/// Test for when the interval is greater than the offset.
TEST(IntervalCalculatorTest, intervalGreaterThanOffset) {
    std::chrono::milliseconds intervalStart;
    ASSERT_TRUE(getIntervalStart(std::chrono::milliseconds(150), std::chrono::milliseconds(100), &intervalStart));
    ASSERT_EQ(intervalStart, std::chrono::milliseconds(50));
}

/// Test for when the interval is equal to the offset.
TEST(IntervalCalculatorTest, intervalEqualToOffset) {
    std::chrono::milliseconds intervalStart;
    ASSERT_TRUE(getIntervalStart(std::chrono::milliseconds(100), std::chrono::milliseconds(100), &intervalStart));
    ASSERT_EQ(intervalStart, std::chrono::milliseconds::zero());
}

/// Test for when the interval is less than the offset.
TEST(IntervalCalculatorTest, intervalLessThanOffset) {
    std::chrono::milliseconds intervalStart;
    ASSERT_TRUE(getIntervalStart(std::chrono::milliseconds(100), std::chrono::milliseconds(120), &intervalStart));
    ASSERT_EQ(intervalStart, std::chrono::milliseconds(80));
}

/// Test for when the interval is less than the offset but by more than one interval.
TEST(IntervalCalculatorTest, intervalLessThanOffsetByMultipleTimes) {
    std::chrono::milliseconds intervalStart;
    ASSERT_TRUE(getIntervalStart(std::chrono::milliseconds(100), std::chrono::milliseconds(220), &intervalStart));
    ASSERT_EQ(intervalStart, std::chrono::milliseconds(80));
}

/// Test for when the interval is negative.
TEST(IntervalCalculatorTest, intervalNegative) {
    std::chrono::milliseconds intervalStart;
    ASSERT_FALSE(getIntervalStart(std::chrono::milliseconds(-100), std::chrono::milliseconds(100), &intervalStart));
}

/// Test for when the offset is negative.
TEST(IntervalCalculatorTest, offsetNegative) {
    std::chrono::milliseconds intervalStart;
    ASSERT_FALSE(getIntervalStart(std::chrono::milliseconds(100), std::chrono::milliseconds(-100), &intervalStart));
}

/// Test for when the interval and offset are negative.
TEST(IntervalCalculatorTest, intervalAndoffsetNegative) {
    std::chrono::milliseconds intervalStart;
    ASSERT_FALSE(getIntervalStart(std::chrono::milliseconds(-100), std::chrono::milliseconds(-100), &intervalStart));
}

/// Test for when the interval is zero.
TEST(IntervalCalculatorTest, intervalZero) {
    std::chrono::milliseconds intervalStart;
    ASSERT_FALSE(getIntervalStart(std::chrono::milliseconds::zero(), std::chrono::milliseconds(100), &intervalStart));
}

/// Test for when the offset is zero.
TEST(IntervalCalculatorTest, offsetZero) {
    std::chrono::milliseconds intervalStart;
    ASSERT_TRUE(getIntervalStart(std::chrono::milliseconds(100), std::chrono::milliseconds::zero(), &intervalStart));
    ASSERT_EQ(intervalStart, std::chrono::milliseconds(100));
}

/// Test for null output parameter.
TEST(IntervalCalculatorTest, nullOutputParameter) {
    ASSERT_FALSE(getIntervalStart(std::chrono::milliseconds(200), std::chrono::milliseconds(100), nullptr));
}

}  // namespace test
}  // namespace audioPlayer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
