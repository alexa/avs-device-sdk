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

#include <AVSCommon/AVS/CapabilityResources.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace test {

using namespace ::testing;
using namespace avsCommon::avs;

/// The test locale string.
static std::string TEST_LOCALE = "en-US";
/// The fan friendly name string.
static std::string FAN_FRIENDLY_NAME = "fan";
/// The air conditioner friendly name string.
static std::string AIR_CONDITIONER_FRIENDLY_NAME = "air conditioner";
/// The expected friendly names json.
static std::string expectedFriendlyNamesJson =
    R"({)"
    R"("friendlyNames":[)"
    R"({"@type":"text","value":{"text":"fan","locale":"en-US"}},)"
    R"({"@type":"text","value":{"text":"air conditioner","locale":"en-US"}},)"
    R"({"@type":"asset","value":{"assetId":"Alexa.Setting.Temperature"}}])"
    R"(})";

/**
 * The test harness for @c CapabilityResources.
 */
class CapabilityResourcesTest : public Test {};

/**
 * Test if the addFriendlyNameWithAssetId method checks for an empty asset Id.
 */
TEST_F(CapabilityResourcesTest, test_addFriendlyNameWithEmptyAssetId) {
    CapabilityResources capabilityResources;
    ASSERT_FALSE(capabilityResources.addFriendlyNameWithAssetId(""));
    ASSERT_FALSE(capabilityResources.isValid());
    ASSERT_EQ(capabilityResources.toJson(), "{}");
}

/**
 * Test if the addFriendlyNameWithAssetId method checks for duplicate asset Ids.
 */
TEST_F(CapabilityResourcesTest, test_addFriendlyNameWithDuplicateAssetId) {
    CapabilityResources capabilityResources;
    ASSERT_TRUE(capabilityResources.addFriendlyNameWithAssetId(resources::ASSET_ALEXA_DEVICENAME_FAN));
    ASSERT_TRUE(capabilityResources.isValid());
    ASSERT_FALSE(capabilityResources.addFriendlyNameWithAssetId(resources::ASSET_ALEXA_DEVICENAME_FAN));
    ASSERT_FALSE(capabilityResources.isValid());
    ASSERT_EQ(capabilityResources.toJson(), "{}");
}

/**
 * Test if the addFriendlyNameWithText method checks for an empty name.
 */
TEST_F(CapabilityResourcesTest, test_addFriendlyNameWithEmptyText) {
    CapabilityResources capabilityResources;
    ASSERT_FALSE(capabilityResources.addFriendlyNameWithText("", TEST_LOCALE));
    ASSERT_FALSE(capabilityResources.isValid());
    ASSERT_EQ(capabilityResources.toJson(), "{}");
}

/**
 * Test if the addFriendlyNameWithText method checks for a very large friendly name.
 */
TEST_F(CapabilityResourcesTest, test_addFriendlyNameWithInvalidText) {
    CapabilityResources capabilityResources;
    size_t length = 16001;
    std::string invalidFriendlyName = std::string(length, 'a');
    ASSERT_FALSE(capabilityResources.addFriendlyNameWithText(invalidFriendlyName, TEST_LOCALE));
    ASSERT_FALSE(capabilityResources.isValid());
    ASSERT_EQ(capabilityResources.toJson(), "{}");
}

/**
 * Test if the addFriendlyNameWithText method checks for an empty locale.
 */
TEST_F(CapabilityResourcesTest, test_addFriendlyNameWithEmptyLocale) {
    CapabilityResources capabilityResources;
    ASSERT_FALSE(capabilityResources.addFriendlyNameWithText(FAN_FRIENDLY_NAME, ""));
    ASSERT_FALSE(capabilityResources.isValid());
    ASSERT_EQ(capabilityResources.toJson(), "{}");
}

/**
 * Test if the addFriendlyNameWithText method checks for duplicate entries.
 */
TEST_F(CapabilityResourcesTest, test_addFriendlyNameWithDuplicateText) {
    CapabilityResources capabilityResources;
    ASSERT_TRUE(capabilityResources.addFriendlyNameWithText(FAN_FRIENDLY_NAME, TEST_LOCALE));
    ASSERT_FALSE(capabilityResources.addFriendlyNameWithText(FAN_FRIENDLY_NAME, TEST_LOCALE));
    ASSERT_FALSE(capabilityResources.isValid());
    ASSERT_EQ(capabilityResources.toJson(), "{}");
}

/**
 * Test if the CapabilityResources returns a valid json.
 */
TEST_F(CapabilityResourcesTest, test_toJsonWithValidInput) {
    CapabilityResources capabilityResources;
    ASSERT_TRUE(capabilityResources.addFriendlyNameWithText(FAN_FRIENDLY_NAME, TEST_LOCALE));
    ASSERT_TRUE(capabilityResources.addFriendlyNameWithText(AIR_CONDITIONER_FRIENDLY_NAME, TEST_LOCALE));
    ASSERT_TRUE(capabilityResources.addFriendlyNameWithAssetId(resources::ASSET_ALEXA_SETTING_TEMPERATURE));
    ASSERT_TRUE(capabilityResources.isValid());
    ASSERT_EQ(capabilityResources.toJson(), expectedFriendlyNamesJson);
}

}  // namespace test
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
