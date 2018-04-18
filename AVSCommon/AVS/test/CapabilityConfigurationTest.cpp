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

#include <AVSCommon/AVS/CapabilityConfiguration.h>

#include <string>
#include <unordered_map>

#include <gtest/gtest.h>

using namespace ::testing;

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace test {

/// Key 1 for an entry of @c CapabilityConfiguration
static const std::string KEY1 = "key1";
/// Key 2 for an entry of @c CapabilityConfiguration
static const std::string KEY2 = "key2";
/// Value 1 for an entry of @c CapabilityConfiguration
static const std::string VALUE1 = "value1";
/// Value 2 for an entry of @c CapabilityConfiguration
static const std::string VALUE2 = "value2";

/**
 * Test harness for @c CapabilityConfiguration class.
 */
class CapabilityConfigurationTest : public ::testing::Test {};

/// Test the constructor
TEST_F(CapabilityConfigurationTest, testConstructor) {
    std::unordered_map<std::string, std::string> capabilityConfigurationMap;
    CapabilityConfiguration instance(capabilityConfigurationMap);
    ASSERT_TRUE(instance.capabilityConfiguration.empty());
}

/// Test the == operator
TEST_F(CapabilityConfigurationTest, testEquality) {
    std::unordered_map<std::string, std::string> lhsCapabilityConfigurationMap;
    lhsCapabilityConfigurationMap.insert({KEY1, VALUE1});

    std::unordered_map<std::string, std::string> rhsCapabilityConfigurationMap;
    rhsCapabilityConfigurationMap.insert({KEY1, VALUE1});

    CapabilityConfiguration lhsInstance(lhsCapabilityConfigurationMap);
    CapabilityConfiguration rhsInstance(rhsCapabilityConfigurationMap);

    ASSERT_EQ(lhsInstance, rhsInstance);
    ASSERT_EQ(std::hash<CapabilityConfiguration>{}(lhsInstance), std::hash<CapabilityConfiguration>{}(rhsInstance));
}

/// Test the != operator
TEST_F(CapabilityConfigurationTest, testInequality) {
    std::unordered_map<std::string, std::string> lhsCapabilityConfigurationMap;
    lhsCapabilityConfigurationMap.insert({KEY1, VALUE1});

    std::unordered_map<std::string, std::string> rhsCapabilityConfigurationMap;
    rhsCapabilityConfigurationMap.insert({KEY2, VALUE2});

    CapabilityConfiguration lhsInstance(lhsCapabilityConfigurationMap);
    CapabilityConfiguration rhsInstance(rhsCapabilityConfigurationMap);

    ASSERT_NE(lhsInstance, rhsInstance);
    ASSERT_NE(std::hash<CapabilityConfiguration>{}(lhsInstance), std::hash<CapabilityConfiguration>{}(rhsInstance));
}

/// Test if equality and hash works if you have multiple entries in maps that are the same but in different order
TEST_F(CapabilityConfigurationTest, testMultipleValues) {
    std::unordered_map<std::string, std::string> lhsCapabilityConfigurationMap;
    lhsCapabilityConfigurationMap.insert({KEY1, VALUE1});
    lhsCapabilityConfigurationMap.insert({KEY2, VALUE2});

    std::unordered_map<std::string, std::string> rhsCapabilityConfigurationMap;
    rhsCapabilityConfigurationMap.insert({KEY2, VALUE2});
    rhsCapabilityConfigurationMap.insert({KEY1, VALUE1});

    CapabilityConfiguration lhsInstance(lhsCapabilityConfigurationMap);
    CapabilityConfiguration rhsInstance(rhsCapabilityConfigurationMap);
    ASSERT_EQ(lhsInstance, rhsInstance);
    ASSERT_EQ(std::hash<CapabilityConfiguration>{}(lhsInstance), std::hash<CapabilityConfiguration>{}(rhsInstance));
}

}  // namespace test
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
