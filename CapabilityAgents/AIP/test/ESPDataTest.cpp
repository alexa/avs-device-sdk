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
/// @file ESPDataTest.cpp

#include <gtest/gtest.h>

#include "AIP/ESPData.h"

using namespace testing;

namespace alexaClientSDK {
namespace capabilityAgents {
namespace aip {
namespace test {

/// Test harness for @c ESPData class.
class ESPDataTest : public ::testing::Test {};

/// Test for a normal positive number (with sign), expect verify() to return true.
TEST_F(ESPDataTest, NormalPositiveNumber) {
    ESPData data{"+0.123f", "+123.123"};
    EXPECT_TRUE(data.verify());
}

/// Test for a normal positive number (without sign), expect verify() to return true.
TEST_F(ESPDataTest, NormalPositiveNumberWithoutSign) {
    ESPData data{"0.123f", "123.123"};
    EXPECT_TRUE(data.verify());
}

/// Test for a normal negative number, expect verify() to return true.
TEST_F(ESPDataTest, NormalNegativeNumber) {
    ESPData data{"-0.123F", "-123.123"};
    EXPECT_TRUE(data.verify());
}

/// Test for a normal positive exponential number, expect verify() to return true.
TEST_F(ESPDataTest, NormalPositiveExponentialNumber) {
    ESPData data{"100e+9", ".6e19f"};
    EXPECT_TRUE(data.verify());
}

/// Test for a normal negative exponential number, expect verify() to return true.
TEST_F(ESPDataTest, NormalNegativeExponentialNumber) {
    ESPData data{"-100e-9", "-1.6e-19f"};
    EXPECT_TRUE(data.verify());
}

/// Test for a empty number, expect verify() to return false.
TEST_F(ESPDataTest, emptyNumber) {
    ESPData data{"", ""};
    EXPECT_FALSE(data.verify());
}

/// Test for a number with quotation mark, expect verify() to return false.
TEST_F(ESPDataTest, noDigitInNumber) {
    ESPData data{"\"", "\""};
    EXPECT_FALSE(data.verify());
}

}  // namespace test
}  // namespace aip
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
