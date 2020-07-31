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

#include <AVSCommon/AVS/CapabilityConfiguration.h>

#include <string>
#include <unordered_map>

#include <gtest/gtest.h>

using namespace ::testing;

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace test {

/// Test values for @c CapabilityConfiguration.
/// @{
static const std::string TEST_TYPE = "type";
static const std::string TEST_NAME = "interface";
static const std::string TEST_VERSION = "version";
static const std::string TEST_INSTANCE = "instance";
static const std::string TEST_CONFIGURATIONS = "configurations";
static const std::string TEST_KEY = "key";
static const std::string TEST_VALUE = "value";
static const bool TEST_PROACTIVELY_REPORTED = true;
static const bool TEST_RETRIEVABLE = true;
static const std::string TEST_PROPERTY_1 = "property1";
static const std::string TEST_PROPERTY_2 = "property2";
/// @}

/**
 * Test harness for @c CapabilityConfiguration class.
 */
class CapabilityConfigurationTest : public ::testing::Test {};

CapabilityConfiguration createTestCapabilityConfiguration(bool differentSupportedList = false) {
    CapabilityConfiguration::Properties properties1;
    properties1.isProactivelyReported = TEST_PROACTIVELY_REPORTED;
    properties1.isRetrievable = TEST_RETRIEVABLE;
    if (!differentSupportedList) {
        properties1.supportedList.push_back(TEST_PROPERTY_1);
    } else {
        properties1.supportedList.push_back(TEST_PROPERTY_2);
    }

    CapabilityConfiguration::AdditionalConfigurations additionalConfigurations1;
    additionalConfigurations1.insert({CAPABILITY_INTERFACE_CONFIGURATIONS_KEY, TEST_CONFIGURATIONS});
    additionalConfigurations1.insert({TEST_KEY, TEST_VALUE});

    CapabilityConfiguration instance(
        TEST_TYPE, TEST_NAME, TEST_VERSION, TEST_INSTANCE, properties1, additionalConfigurations1);

    return instance;
}

std::shared_ptr<CapabilityConfiguration> createTestCapabilityConfigurationPtr(bool differentSupportedList = false) {
    CapabilityConfiguration::Properties properties1;
    properties1.isProactivelyReported = TEST_PROACTIVELY_REPORTED;
    properties1.isRetrievable = TEST_RETRIEVABLE;
    if (!differentSupportedList) {
        properties1.supportedList.push_back(TEST_PROPERTY_1);
    } else {
        properties1.supportedList.push_back(TEST_PROPERTY_2);
    }

    CapabilityConfiguration::AdditionalConfigurations additionalConfigurations1;
    additionalConfigurations1.insert({CAPABILITY_INTERFACE_CONFIGURATIONS_KEY, TEST_CONFIGURATIONS});
    additionalConfigurations1.insert({TEST_KEY, TEST_VALUE});

    return std::make_shared<CapabilityConfiguration>(
        TEST_TYPE, TEST_NAME, TEST_VERSION, TEST_INSTANCE, properties1, additionalConfigurations1);
}

/// Test the constructor
TEST_F(CapabilityConfigurationTest, test_constructorUsingMap) {
    std::unordered_map<std::string, std::string> capabilityConfigurationMap;
    capabilityConfigurationMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, TEST_TYPE});
    capabilityConfigurationMap.insert({CAPABILITY_INTERFACE_NAME_KEY, TEST_NAME});
    capabilityConfigurationMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, TEST_VERSION});
    capabilityConfigurationMap.insert({CAPABILITY_INTERFACE_CONFIGURATIONS_KEY, TEST_CONFIGURATIONS});
    capabilityConfigurationMap.insert({TEST_KEY, TEST_VALUE});

    CapabilityConfiguration instance(capabilityConfigurationMap);

    ASSERT_EQ(instance.type, TEST_TYPE);
    ASSERT_EQ(instance.interfaceName, TEST_NAME);
    ASSERT_EQ(instance.version, TEST_VERSION);
    ASSERT_EQ(instance.additionalConfigurations.size(), std::size_t(1));
    auto it = instance.additionalConfigurations.find(CAPABILITY_INTERFACE_CONFIGURATIONS_KEY);
    if (it != instance.additionalConfigurations.end()) {
        ASSERT_EQ(it->second, TEST_CONFIGURATIONS);
    }
}

/// Test the constructor
TEST_F(CapabilityConfigurationTest, test_constructor) {
    auto instance = createTestCapabilityConfiguration();

    /// Check type, version, interface and instance.
    ASSERT_EQ(instance.type, TEST_TYPE);
    ASSERT_EQ(instance.interfaceName, TEST_NAME);
    ASSERT_EQ(instance.version, TEST_VERSION);
    ASSERT_TRUE(instance.instanceName.hasValue());
    ASSERT_EQ(instance.instanceName.value(), TEST_INSTANCE);

    /// Check Properties.
    ASSERT_TRUE(instance.properties.hasValue());
    ASSERT_EQ(instance.properties.value().isProactivelyReported, TEST_PROACTIVELY_REPORTED);
    ASSERT_EQ(instance.properties.value().isRetrievable, TEST_RETRIEVABLE);
    ASSERT_EQ(instance.properties.value().supportedList.size(), std::size_t(1));
    ASSERT_EQ(instance.properties.value().supportedList[0], TEST_PROPERTY_1);

    /// Check additional configurations.
    ASSERT_EQ(instance.additionalConfigurations.size(), std::size_t(2));
    auto it = instance.additionalConfigurations.find(CAPABILITY_INTERFACE_CONFIGURATIONS_KEY);
    if (it != instance.additionalConfigurations.end()) {
        ASSERT_EQ(it->second, TEST_CONFIGURATIONS);
    }
    it = instance.additionalConfigurations.find(TEST_KEY);
    if (it != instance.additionalConfigurations.end()) {
        ASSERT_EQ(it->second, TEST_VALUE);
    }
    ASSERT_TRUE(instance.instanceName.hasValue());
}

/// Test the operator ==
TEST_F(CapabilityConfigurationTest, test_equalityOperator) {
    auto lhs = createTestCapabilityConfiguration();
    auto rhs = createTestCapabilityConfiguration();
    ASSERT_EQ(lhs, rhs);
}

/// Test the operator !=
TEST_F(CapabilityConfigurationTest, test_inEqualityOperator) {
    auto lhs = createTestCapabilityConfiguration();
    auto rhs = createTestCapabilityConfiguration(true);
    ASSERT_NE(lhs, rhs);
}

/// Test if the hash function works as expected.
TEST_F(CapabilityConfigurationTest, test_structHashFunction) {
    auto cap1 = createTestCapabilityConfiguration();
    auto cap2 = createTestCapabilityConfiguration();
    auto cap3 = createTestCapabilityConfiguration(true);

    std::unordered_map<CapabilityConfiguration, std::string> testMap;

    /// use cap1 as a key.
    testMap.insert({cap1, TEST_VALUE});

    /// try to find key cap2
    auto it = testMap.find(cap2);
    ASSERT_NE(it, testMap.end());
    if (it != testMap.end()) {
        ASSERT_EQ(it->second, TEST_VALUE);
    }

    it = testMap.find(cap3);
    ASSERT_EQ(it, testMap.end());
}

/// Test if the hash function works as expected with pointers.
TEST_F(CapabilityConfigurationTest, test_pointerHashFunction) {
    auto cap1 = createTestCapabilityConfigurationPtr();
    auto cap2 = createTestCapabilityConfigurationPtr();
    auto cap3 = createTestCapabilityConfigurationPtr(true);

    std::unordered_map<std::shared_ptr<CapabilityConfiguration>, std::string> testMap;

    /// use cap1 as a key.
    testMap.insert({cap1, TEST_VALUE});

    /// try to find key cap2
    auto it = testMap.find(cap2);
    ASSERT_NE(it, testMap.end());
    if (it != testMap.end()) {
        ASSERT_EQ(it->second, TEST_VALUE);
    }

    it = testMap.find(cap3);
    ASSERT_EQ(it, testMap.end());
}

}  // namespace test
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
