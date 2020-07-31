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

#include <gtest/gtest.h>

#include "AVSCommon/Utils/MacAddressString.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace test {

/**
 * Unit tests for @c MacAddressString class.
 */
class MacAddressStringTest : public ::testing::Test {};

/**
 * Tests @c MacAddressString create method with invalid hexadecimal values in Mac Address.
 */
TEST_F(MacAddressStringTest, test_createWithInvalidHexDigits) {
    ASSERT_EQ(MacAddressString::create("ga:00:01:02:03:04"), nullptr);
    ASSERT_EQ(MacAddressString::create("ag:00:01:02:03:04"), nullptr);
}

/**
 * Tests @c MacAddressString create method with invalid number of octets in the Mac Address.
 */
TEST_F(MacAddressStringTest, test_createWithInvalidNumberOfOctets) {
    ASSERT_EQ(MacAddressString::create("ab:cd:ef:00:01:02:03"), nullptr);
    ASSERT_EQ(MacAddressString::create("00:01:02:03"), nullptr);
    ASSERT_EQ(MacAddressString::create("00:01:02:03::::::"), nullptr);
}

/**
 * Test @c MacAddressString create method with no dividers in Mac Address.
 */
TEST_F(MacAddressStringTest, test_createWithNoDividers) {
    ASSERT_EQ(MacAddressString::create("abcdef012345"), nullptr);
}

/**
 * Test @c MacAddressString create method with invalid dividers in Mac Address.
 */
TEST_F(MacAddressStringTest, test_createWithInvalidDividers) {
    ASSERT_EQ(MacAddressString::create("ab::cd::ef::01::23::45"), nullptr);
    ASSERT_EQ(MacAddressString::create("ab,cd,ef,01,23,45"), nullptr);
    ASSERT_EQ(MacAddressString::create("ab:cd:ef:01:23:45:"), nullptr);
}

/**
 * Test @c MacAddressString create method with valid Mac address.
 */
TEST_F(MacAddressStringTest, test_createWithValidMacAddress) {
    std::string macAddress = "01:23:45:ab:cd:ef";
    std::string truncatedMacAddress = "XX:XX:XX:XX:cd:ef";
    auto macAddressString = MacAddressString::create(macAddress);

    ASSERT_NE(macAddressString, nullptr);
    ASSERT_EQ(macAddressString->getString(), macAddress);
    ASSERT_EQ(macAddressString->getTruncatedString(), truncatedMacAddress);
}

/**
 * Test @c MacAddressString create method with truncated Mac address is invalid.
 */
TEST_F(MacAddressStringTest, test_createWithTruncatedMacAddress) {
    ASSERT_EQ(MacAddressString::create("XX:XX:XX:XX:cd:ef"), nullptr);
    ASSERT_EQ(MacAddressString::create("XX:23:45:ab:cd:ef"), nullptr);
    ASSERT_EQ(MacAddressString::create("01:23:45:ab:cd:XX"), nullptr);
}

}  // namespace test
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
