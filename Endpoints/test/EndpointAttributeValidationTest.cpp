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

#include <AVSCommon/AVS/AVSDiscoveryEndpointAttributes.h>
#include <Endpoints/EndpointAttributeValidation.h>

namespace alexaClientSDK {
namespace endpoints {
namespace test {

using namespace ::testing;
using namespace avsCommon::avs;

class EndpointAttributeValidationTest : public Test {};

static const size_t MIN_SIZE = 1;

/**
 * Test for checking valid endpointId.
 */
TEST_F(EndpointAttributeValidationTest, test_validEndpointIdLengthBoundaries) {
    std::string minSizeId(MIN_SIZE, 'c');
    EXPECT_TRUE(isEndpointIdValid(minSizeId));

    std::string maxSizeId(AVSDiscoveryEndpointAttributes::MAX_ENDPOINT_IDENTIFIER_LENGTH, 'c');
    EXPECT_TRUE(isEndpointIdValid(maxSizeId));
}

/**
 * Test for checking invalid endpointId.
 */
TEST_F(EndpointAttributeValidationTest, test_invalidEndpointIdLengthBoundariesReturnsFalse) {
    std::string minSizeId(MIN_SIZE - 1, 'c');
    EXPECT_FALSE(isEndpointIdValid(minSizeId));

    std::string maxSizeId(AVSDiscoveryEndpointAttributes::MAX_ENDPOINT_IDENTIFIER_LENGTH + 1, 'c');
    EXPECT_FALSE(isEndpointIdValid(maxSizeId));
}

/**
 * Test for checking valid endpoint friendly name.
 */
TEST_F(EndpointAttributeValidationTest, test_validFriendlyNameLengthBoundaries) {
    std::string minSizeFriendlyName(MIN_SIZE, 'c');
    EXPECT_TRUE(isFriendlyNameValid(minSizeFriendlyName));

    std::string maxSizeFriendlyName(AVSDiscoveryEndpointAttributes::MAX_FRIENDLY_NAME_LENGTH, 'c');
    EXPECT_TRUE(isFriendlyNameValid(maxSizeFriendlyName));
}

/**
 * Test for checking invalid endpoint friendly name.
 */
TEST_F(EndpointAttributeValidationTest, test_invalidFriendlyNameLengthBoundariesReturnsFalse) {
    std::string minSizeFriendlyName(MIN_SIZE - 1, 'c');
    EXPECT_FALSE(isFriendlyNameValid(minSizeFriendlyName));

    std::string maxSizeFriendlyName(AVSDiscoveryEndpointAttributes::MAX_FRIENDLY_NAME_LENGTH + 1, 'c');
    EXPECT_FALSE(isFriendlyNameValid(maxSizeFriendlyName));
}

/**
 * Test for checking valid endpoint description field.
 */
TEST_F(EndpointAttributeValidationTest, test_validDescriptionLengthBoundaries) {
    std::string minSizeDescription(MIN_SIZE, 'c');
    EXPECT_TRUE(isDescriptionValid(minSizeDescription));

    std::string maxSizeDescription(AVSDiscoveryEndpointAttributes::MAX_DESCRIPTION_LENGTH, 'c');
    EXPECT_TRUE(isDescriptionValid(maxSizeDescription));
}

/**
 * Test for checking invalid endpoint description field.
 */
TEST_F(EndpointAttributeValidationTest, test_invalidDescriptionLengthBoundariesReturnsFalse) {
    std::string minSizeDescription(MIN_SIZE - 1, 'c');
    EXPECT_FALSE(isDescriptionValid(minSizeDescription));

    std::string maxSizeDescription(AVSDiscoveryEndpointAttributes::MAX_DESCRIPTION_LENGTH + 1, 'c');
    EXPECT_FALSE(isDescriptionValid(maxSizeDescription));
}

/**
 * Test for checking valid manufacturer name field.
 */
TEST_F(EndpointAttributeValidationTest, test_validManufacturerNameLengthBoundaries) {
    std::string minSizeManufacturerName(MIN_SIZE, 'c');
    EXPECT_TRUE(isManufacturerNameValid(minSizeManufacturerName));

    std::string maxSizeManufacturerName(AVSDiscoveryEndpointAttributes::MAX_MANUFACTURER_NAME_LENGTH, 'c');
    EXPECT_TRUE(isManufacturerNameValid(maxSizeManufacturerName));
}

/**
 * Test for checking invalid manufacturer name field.
 */
TEST_F(EndpointAttributeValidationTest, test_invalidManufacturerNameLengthBoundariesReturnsFalse) {
    std::string minSizeManufacturerName(MIN_SIZE - 1, 'c');
    EXPECT_FALSE(isManufacturerNameValid(minSizeManufacturerName));

    std::string maxSizeManufacturerName(AVSDiscoveryEndpointAttributes::MAX_MANUFACTURER_NAME_LENGTH + 1, 'c');
    EXPECT_FALSE(isManufacturerNameValid(maxSizeManufacturerName));
}

/**
 * Test for checking valid manufacturer field.
 */
TEST_F(EndpointAttributeValidationTest, test_validManufacturerAttributeBoundary) {
    AVSDiscoveryEndpointAttributes::AdditionalAttributes attributes;
    attributes.manufacturer =
        std::string(AVSDiscoveryEndpointAttributes::MAX_ENDPOINT_ADDITIONAL_ATTRIBUTES_LENGTH, 'c');
    EXPECT_TRUE(isAdditionalAttributesValid(attributes));
}

/**
 * Test for checking invalid manufacturer field.
 */
TEST_F(EndpointAttributeValidationTest, test_invalidManufacturerAttributeBoundaryReturnsFalse) {
    AVSDiscoveryEndpointAttributes::AdditionalAttributes attributes;
    attributes.manufacturer =
        std::string(AVSDiscoveryEndpointAttributes::MAX_ENDPOINT_ADDITIONAL_ATTRIBUTES_LENGTH + 1, 'c');
    EXPECT_FALSE(isAdditionalAttributesValid(attributes));
}

/**
 * Test for checking valid model field.
 */
TEST_F(EndpointAttributeValidationTest, test_validModelAttributeBoundary) {
    AVSDiscoveryEndpointAttributes::AdditionalAttributes attributes;
    attributes.model = std::string(AVSDiscoveryEndpointAttributes::MAX_ENDPOINT_ADDITIONAL_ATTRIBUTES_LENGTH, 'c');
    EXPECT_TRUE(isAdditionalAttributesValid(attributes));
}

/**
 * Test for checking invalid model field.
 */
TEST_F(EndpointAttributeValidationTest, test_invalidModelAttributeBoundaryReturnsFalse) {
    AVSDiscoveryEndpointAttributes::AdditionalAttributes attributes;
    attributes.model = std::string(AVSDiscoveryEndpointAttributes::MAX_ENDPOINT_ADDITIONAL_ATTRIBUTES_LENGTH + 1, 'c');
    EXPECT_FALSE(isAdditionalAttributesValid(attributes));
}

/**
 * Test for checking valid serial number field.
 */
TEST_F(EndpointAttributeValidationTest, test_validSerialNumberAttributeBoundary) {
    AVSDiscoveryEndpointAttributes::AdditionalAttributes attributes;
    attributes.serialNumber =
        std::string(AVSDiscoveryEndpointAttributes::MAX_ENDPOINT_ADDITIONAL_ATTRIBUTES_LENGTH, 'c');
    EXPECT_TRUE(isAdditionalAttributesValid(attributes));
}

/**
 * Test for checking invalid serial number field.
 */
TEST_F(EndpointAttributeValidationTest, test_invalidSerialNumberAttributeBoundaryReturnsFalse) {
    AVSDiscoveryEndpointAttributes::AdditionalAttributes attributes;
    attributes.serialNumber =
        std::string(AVSDiscoveryEndpointAttributes::MAX_ENDPOINT_ADDITIONAL_ATTRIBUTES_LENGTH + 1, 'c');
    EXPECT_FALSE(isAdditionalAttributesValid(attributes));
}

/**
 * Test for checking valid firmwareVersion field.
 */
TEST_F(EndpointAttributeValidationTest, test_validFirmwareVersionAttributeBoundary) {
    AVSDiscoveryEndpointAttributes::AdditionalAttributes attributes;
    attributes.firmwareVersion =
        std::string(AVSDiscoveryEndpointAttributes::MAX_ENDPOINT_ADDITIONAL_ATTRIBUTES_LENGTH, 'c');
    EXPECT_TRUE(isAdditionalAttributesValid(attributes));
}

/**
 * Test for checking invalid firmwareVersion field.
 */
TEST_F(EndpointAttributeValidationTest, test_invalidFirmwareVersionAttributeBoundaryReturnsFalse) {
    AVSDiscoveryEndpointAttributes::AdditionalAttributes attributes;
    attributes.firmwareVersion =
        std::string(AVSDiscoveryEndpointAttributes::MAX_ENDPOINT_ADDITIONAL_ATTRIBUTES_LENGTH + 1, 'c');
    EXPECT_FALSE(isAdditionalAttributesValid(attributes));
}

/**
 * Test for checking valid softwareVersion field.
 */
TEST_F(EndpointAttributeValidationTest, test_validSoftwareVersionAttributeBoundary) {
    AVSDiscoveryEndpointAttributes::AdditionalAttributes attributes;
    attributes.softwareVersion =
        std::string(AVSDiscoveryEndpointAttributes::MAX_ENDPOINT_ADDITIONAL_ATTRIBUTES_LENGTH, 'c');
    EXPECT_TRUE(isAdditionalAttributesValid(attributes));
}

/**
 * Test for checking invalid softwareVersion field.
 */
TEST_F(EndpointAttributeValidationTest, test_invalidSoftwareVersionAttributeBoundaryReturnsFalse) {
    AVSDiscoveryEndpointAttributes::AdditionalAttributes attributes;
    attributes.softwareVersion =
        std::string(AVSDiscoveryEndpointAttributes::MAX_ENDPOINT_ADDITIONAL_ATTRIBUTES_LENGTH + 1, 'c');
    EXPECT_FALSE(isAdditionalAttributesValid(attributes));
}

/**
 * Test for checking valid CustomIdentifier field.
 */
TEST_F(EndpointAttributeValidationTest, test_validCustomIdentifierAttributeBoundary) {
    AVSDiscoveryEndpointAttributes::AdditionalAttributes attributes;
    attributes.customIdentifier =
        std::string(AVSDiscoveryEndpointAttributes::MAX_ENDPOINT_ADDITIONAL_ATTRIBUTES_LENGTH, 'c');
    EXPECT_TRUE(isAdditionalAttributesValid(attributes));
}

/**
 * Test for checking invalid CustomIdentifier field.
 */
TEST_F(EndpointAttributeValidationTest, test_invalidCustomIdentifierAttributeBoundaryReturnsFalse) {
    AVSDiscoveryEndpointAttributes::AdditionalAttributes attributes;
    attributes.customIdentifier =
        std::string(AVSDiscoveryEndpointAttributes::MAX_ENDPOINT_ADDITIONAL_ATTRIBUTES_LENGTH + 1, 'c');
    EXPECT_FALSE(isAdditionalAttributesValid(attributes));
}

/**
 * Test for checking valid cookie data.
 */
TEST_F(EndpointAttributeValidationTest, test_validCookiesSize) {
    std::map<std::string, std::string> emptyCookies;
    EXPECT_TRUE(areCookiesValid(emptyCookies));

    std::map<std::string, std::string> oneCookie{{"name", "value"}};
    EXPECT_TRUE(areCookiesValid(oneCookie));
}

/**
 * Test for checking invalid cookie data.
 */
TEST_F(EndpointAttributeValidationTest, test_invalidCookiesSizeReturnsFalse) {
    std::map<std::string, std::string> cookies;
    size_t bytes = 0;
    while (bytes < AVSDiscoveryEndpointAttributes::MAX_COOKIES_SIZE_BYTES) {
        std::string name = std::to_string(bytes);
        std::string value{"value"};
        cookies[name] = value;
        bytes += name.size() + value.size();
    }
    EXPECT_FALSE(areCookiesValid(cookies));
}

/**
 * Test for checking valid connections data.
 */
TEST_F(EndpointAttributeValidationTest, test_validConnectionsReturnsTrue) {
    std::vector<std::map<std::string, std::string>> validConnections;
    auto value1 = std::string(AVSDiscoveryEndpointAttributes::MAX_CONNECTIONS_VALUE_LENGTH, 'a');
    auto value2 = std::string(AVSDiscoveryEndpointAttributes::MAX_CONNECTIONS_VALUE_LENGTH, 'b');
    validConnections.push_back({{"name1", value1}});
    validConnections.push_back({{"name2", value2}});

    EXPECT_TRUE(areConnectionsValid(validConnections));

    std::vector<std::map<std::string, std::string>> emptyConnections;
    EXPECT_TRUE(areConnectionsValid(emptyConnections));
}

/**
 * Test for checking invalid connections data.
 */
TEST_F(EndpointAttributeValidationTest, test_invalidConnectionsReturnsFalse) {
    std::vector<std::map<std::string, std::string>> emptyValueConnections;
    auto value1 = std::string(AVSDiscoveryEndpointAttributes::MAX_CONNECTIONS_VALUE_LENGTH, 'a');
    emptyValueConnections.push_back({{"name1", value1}});
    emptyValueConnections.push_back({{"name2", ""}});

    EXPECT_FALSE(areConnectionsValid(emptyValueConnections));

    std::vector<std::map<std::string, std::string>> invalidConnections;
    auto value2 = std::string(AVSDiscoveryEndpointAttributes::MAX_CONNECTIONS_VALUE_LENGTH + 1, 'b');
    invalidConnections.push_back({{"name1", value1}});
    invalidConnections.push_back({{"name2", value2}});

    EXPECT_FALSE(areConnectionsValid(invalidConnections));
}

}  // namespace test
}  // namespace endpoints
}  // namespace alexaClientSDK
