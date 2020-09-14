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

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <AVSCommon/AVS/AVSDiscoveryEndpointAttributes.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <CapabilitiesDelegate/Utils/DiscoveryUtils.h>

namespace alexaClientSDK {
namespace capabilitiesDelegate {
namespace utils {
namespace test {

using namespace ::testing;
using namespace avsCommon::avs;
using namespace avsCommon::utils;
using namespace avsCommon::utils::json;
using namespace avsCommon::utils::json::jsonUtils;
using namespace avsCommon::sdkInterfaces::endpoints;
using namespace rapidjson;

/// Alias for CapabilityConfiguration::Properties
using Properties = CapabilityConfiguration::Properties;

/// A Test CapabilityConfiguration type.
static const std::string TEST_TYPE = "TEST_TYPE";

/// A Test CapabilityConfiguration interface name.
static const std::string TEST_INTERFACE_NAME = "TEST_INTERFACE_NAME";

/// A Test CapabilityConfiguration version.
static const std::string TEST_VERSION = "TEST_VERSION";

/// A Test CapabilityConfiguration instance name.
static const Optional<std::string> TEST_INSTANCE_NAME("TEST_INSTANCE_NAME");

/// A Test CapabilityConfiguration properties supportedList.
static const std::vector<std::string> TEST_SUPPORTED_LIST{"TEST_PROPERTY"};

/// A Test CapabilityConfiguration Optional properties.
static const Optional<Properties> TEST_PROPERTIES = Properties(false, false, TEST_SUPPORTED_LIST);

static const std::string EXPECTED_TEST_PROPERTIES_STRING =
    R"({"supported":[{"name":"TEST_PROPERTY"}],"proactivelyReported":false,"retrievable":false})";

/// A Test valid JSON value.
static const std::string TEST_VALID_CONFIG_JSON = R"({"key":{"key1":"value1"}})";

/// A Test CapabilityConfiguration type.
static const CapabilityConfiguration::AdditionalConfigurations TEST_ADDITIONAL_CONFIGURATIONS = {
    {"configuration", TEST_VALID_CONFIG_JSON}};

/// A Test Endpoint Id.
static const std::string TEST_ENDPOINT_ID = "TEST_ENDPOINT_ID";

/// A Test Friendly name.
static const std::string TEST_FRIENDLY_NAME = "TEST_FRIENDLY_NAME";

/// A Test description.
static const std::string TEST_DESCRIPTION = "TEST_DESCRIPTION";

/// A Test manufacturer name.
static const std::string TEST_MANUFACTURER_NAME = "TEST_MANUFACTURER_NAME";

/// A Test display category list.
static const std::vector<std::string> TEST_DISPLAY_CATEGORIES = {"TEST_DISPLAY_CATEGORY"};

/// A Test customer identifier.
static const std::string TEST_CUSTOMER_IDENTIFIER = "TEST_CUSTOMER_IDENTIFIER";

/// A Test software version.
static const std::string TEST_SOFTWARE_VERSION = "TEST_SOFTWARE_VERSION";

/// A Test firmware version.
static const std::string TEST_FIRMWARE_VERSION = "TEST_FIRMWARE_VERSION";

/// A Test serial number.
static const std::string TEST_SERIAL_NUMBER = "TEST_SERIAL_NUMBER";

/// A Test model.
static const std::string TEST_MODEL = "TEST_MODEL";

/// A Test manufacturer.
static const std::string TEST_MANUFACTURER = "TEST_MANUFACTURER";

/// A Test product ID.
static const std::string TEST_PRODUCT_ID = "TEST_PRODUCT_ID";

/// A Test registration key.
static const std::string TEST_REGISTRATION_KEY = "TEST_REGISTRATION_KEY";

/// A Test product ID key.
static const std::string TEST_PRODUCT_ID_KEY = "TEST_PRODUCT_ID_KEY";

/// A Test Endpoint configuration JSON.
static const std::string TEST_ENDPOINT_CONFIG = R"({"endpointId":")" + TEST_ENDPOINT_ID + R"("})";

/// A Test auth token.
static const std::string TEST_AUTH_TOKEN = "TEST_AUTH_TOKEN";

/// A test connections data.
static const std::vector<std::map<std::string, std::string>> TEST_CONNECTIONS_DATA = {{{"CON_1", "DATA_1"}},
                                                                                      {{"CON_2", "DATA_2"}}};

/// A test cookie data.
static const std::map<std::string, std::string> TEST_COOKIE_DATA = {{"KEY1", "VALUE1"}, {"KEY2", "VALUE2"}};

/// The expected name that appears in the header of the @c Discovery.AddOrUpdateReport event.
static const std::string ADD_OR_UPDATE_REPORT_EVENT_NAME = "AddOrUpdateReport";

/// The expected name that appears in the header of the @c Discovery.DeleteReport event.
static const std::string DELETE_REPORT_EVENT_NAME = "DeleteReport";

/**
 * This method returns a test @c AVSDiscoveryEndpointAttributes structure.
 * @return a new @c AVSDiscoveryEndpointAttributes structure containing test fields.
 */
AVSDiscoveryEndpointAttributes getTestEndpointAttributes() {
    AVSDiscoveryEndpointAttributes endpointAttributes;
    std::vector<CapabilityConfiguration> capabilities;

    endpointAttributes.endpointId = TEST_ENDPOINT_ID;
    endpointAttributes.friendlyName = TEST_FRIENDLY_NAME;
    endpointAttributes.description = TEST_DESCRIPTION;
    endpointAttributes.manufacturerName = TEST_MANUFACTURER_NAME;
    endpointAttributes.displayCategories = TEST_DISPLAY_CATEGORIES;

    AVSDiscoveryEndpointAttributes::Registration testRegistration(
        TEST_PRODUCT_ID, TEST_SERIAL_NUMBER, TEST_REGISTRATION_KEY, TEST_PRODUCT_ID_KEY);
    endpointAttributes.registration = testRegistration;

    AVSDiscoveryEndpointAttributes::AdditionalAttributes additionalAttributes;
    additionalAttributes.customIdentifier = TEST_CUSTOMER_IDENTIFIER;
    additionalAttributes.softwareVersion = TEST_SOFTWARE_VERSION;
    additionalAttributes.firmwareVersion = TEST_FIRMWARE_VERSION;
    additionalAttributes.serialNumber = TEST_SERIAL_NUMBER;
    additionalAttributes.model = TEST_MODEL;
    additionalAttributes.manufacturer = TEST_MANUFACTURER;
    endpointAttributes.additionalAttributes = additionalAttributes;

    std::vector<std::map<std::string, std::string>> connections = TEST_CONNECTIONS_DATA;
    endpointAttributes.connections = connections;

    std::map<std::string, std::string> cookie = TEST_COOKIE_DATA;
    endpointAttributes.cookies = cookie;

    CapabilityConfiguration capability(
        TEST_TYPE,
        TEST_INTERFACE_NAME,
        TEST_VERSION,
        TEST_INSTANCE_NAME,
        TEST_PROPERTIES,
        TEST_ADDITIONAL_CONFIGURATIONS);
    capabilities.push_back(capability);

    return endpointAttributes;
}

/**
 * Returns an array of @c CapabilityConfigurations to be used in unit tests.
 *
 * @return an array of @c CapabilityConfigurations.
 */
std::vector<CapabilityConfiguration> getTestCapabilities() {
    std::vector<CapabilityConfiguration> capabilities;
    CapabilityConfiguration capability(
        TEST_TYPE,
        TEST_INTERFACE_NAME,
        TEST_VERSION,
        TEST_INSTANCE_NAME,
        TEST_PROPERTIES,
        TEST_ADDITIONAL_CONFIGURATIONS);
    capabilities.push_back(capability);

    return capabilities;
}

/**
 * Creates an @c AVSDiscoveryEndpointAttributes structure using the parameters passed in.
 * @param endpointId The endpoint ID string.
 * @param friendlyName The friendly name string.
 * @param description The description string.
 * @param manufacturerName The manufacturer name string.
 * @param displayCategories The display categories list.
 * @return an @c AVSDiscoveryEndpointAttributes structure containing the params passed in.
 */
AVSDiscoveryEndpointAttributes createEndpointAttributes(
    const std::string& endpointId,
    const std::string& friendlyName,
    const std::string& description,
    const std::string& manufacturerName,
    const std::vector<std::string>& displayCategories) {
    AVSDiscoveryEndpointAttributes endpointAttributes;
    endpointAttributes.endpointId = endpointId;
    endpointAttributes.friendlyName = friendlyName;
    endpointAttributes.description = description;
    endpointAttributes.manufacturerName = manufacturerName;
    endpointAttributes.displayCategories = displayCategories;
    return endpointAttributes;
}

/**
 * Parses the JSON and validates if the expected fields for @c AVSDiscoveryEndpointAttributes are present.
 * @param endpointConfigJson The JSON string representation of the endpoint attributes which will be validated.
 */
void validateEndpointConfigJson(const std::string& endpointConfigJson) {
    std::string value;
    ASSERT_TRUE(jsonUtils::retrieveValue(endpointConfigJson, "endpointId", &value));
    ASSERT_EQ(value, TEST_ENDPOINT_ID);

    ASSERT_TRUE(jsonUtils::retrieveValue(endpointConfigJson, "friendlyName", &value));
    ASSERT_EQ(value, TEST_FRIENDLY_NAME);

    ASSERT_TRUE(jsonUtils::retrieveValue(endpointConfigJson, "manufacturerName", &value));
    ASSERT_EQ(value, TEST_MANUFACTURER_NAME);

    ASSERT_TRUE(jsonUtils::retrieveValue(endpointConfigJson, "description", &value));
    ASSERT_EQ(value, TEST_DESCRIPTION);

    auto displayCategoriesFromJson =
        jsonUtils::retrieveStringArray<std::vector<std::string>>(endpointConfigJson, "displayCategories");
    ASSERT_EQ(displayCategoriesFromJson, TEST_DISPLAY_CATEGORIES);

    /// Additional Attributes
    std::string additionalAttributesJson;
    ASSERT_TRUE(jsonUtils::retrieveValue(endpointConfigJson, "additionalAttributes", &additionalAttributesJson));

    ASSERT_TRUE(jsonUtils::retrieveValue(additionalAttributesJson, "manufacturer", &value));
    ASSERT_EQ(value, TEST_MANUFACTURER);

    ASSERT_TRUE(jsonUtils::retrieveValue(additionalAttributesJson, "model", &value));
    ASSERT_EQ(value, TEST_MODEL);

    ASSERT_TRUE(jsonUtils::retrieveValue(additionalAttributesJson, "serialNumber", &value));
    ASSERT_EQ(value, TEST_SERIAL_NUMBER);

    ASSERT_TRUE(jsonUtils::retrieveValue(additionalAttributesJson, "firmwareVersion", &value));
    ASSERT_EQ(value, TEST_FIRMWARE_VERSION);

    ASSERT_TRUE(jsonUtils::retrieveValue(additionalAttributesJson, "softwareVersion", &value));
    ASSERT_EQ(value, TEST_SOFTWARE_VERSION);

    ASSERT_TRUE(jsonUtils::retrieveValue(additionalAttributesJson, "customIdentifier", &value));
    ASSERT_EQ(value, TEST_CUSTOMER_IDENTIFIER);

    /// Registration
    std::string registrationJson;
    ASSERT_TRUE(jsonUtils::retrieveValue(endpointConfigJson, TEST_REGISTRATION_KEY, &registrationJson));

    std::string productId;
    ASSERT_TRUE(jsonUtils::retrieveValue(registrationJson, TEST_PRODUCT_ID_KEY, &productId));
    ASSERT_EQ(productId, TEST_PRODUCT_ID);

    std::string deviceSerialNumber;
    ASSERT_TRUE(jsonUtils::retrieveValue(registrationJson, "deviceSerialNumber", &deviceSerialNumber));
    ASSERT_EQ(deviceSerialNumber, TEST_SERIAL_NUMBER);

    /// Connections
    std::string connections;
    rapidjson::Document document;
    ASSERT_TRUE(jsonUtils::parseJSON(endpointConfigJson, &document));

    Value::ConstMemberIterator connectionsIt;
    ASSERT_TRUE(findNode(document, "connections", &connectionsIt));
    ASSERT_TRUE(connectionsIt->value.IsArray());

    /// iterate through connections
    std::vector<std::map<std::string, std::string>> connectionMaps;
    for (auto it = connectionsIt->value.Begin(); it != connectionsIt->value.End(); ++it) {
        std::map<std::string, std::string> connectionMap;
        for (auto iter = it->MemberBegin(); iter != it->MemberEnd(); ++iter) {
            if (iter->name.IsString() && iter->value.IsString()) {
                connectionMap[iter->name.GetString()] = iter->value.GetString();
            }
        }
        connectionMaps.push_back(connectionMap);
    }
    ASSERT_EQ(connectionMaps.size(), 2U);
    ASSERT_EQ(connectionMaps[0].size(), 1U);
    ASSERT_EQ(connectionMaps[1].size(), 1U);

    /// Cookie
    std::map<std::string, std::string> cookieMap = retrieveStringMap(document, "cookie");

    ASSERT_EQ(cookieMap.size(), 2U);

    /// Capabilities
    Value::ConstMemberIterator capabilitiesIt;
    ASSERT_TRUE(findNode(document, "capabilities", &capabilitiesIt));
    ASSERT_TRUE(capabilitiesIt->value.IsArray());

    /// iterate through capabilities array.
    std::vector<std::map<std::string, std::string>> capabilitiesMaps;
    for (auto it = capabilitiesIt->value.Begin(); it != capabilitiesIt->value.End(); ++it) {
        std::map<std::string, std::string> capabilitiesMap;
        for (auto iter = it->MemberBegin(); iter != it->MemberEnd(); ++iter) {
            if (iter->name.IsString() && iter->value.IsString()) {
                capabilitiesMap[iter->name.GetString()] = iter->value.GetString();
            } else if (
                iter->name.IsString() && !strcmp(iter->name.GetString(), "properties") && iter->value.IsObject()) {
                std::string propertiesString;
                ASSERT_TRUE(convertToValue(iter->value, &propertiesString));
                capabilitiesMap[iter->name.GetString()] = propertiesString;

            } else if (
                iter->name.IsString() && !strcmp(iter->name.GetString(), "configuration") && iter->value.IsObject()) {
                std::string configurationString;
                ASSERT_TRUE(convertToValue(iter->value, &configurationString));
                capabilitiesMap[iter->name.GetString()] = configurationString;
            }
        }
        capabilitiesMaps.push_back(capabilitiesMap);
    }

    ASSERT_EQ(capabilitiesMaps.size(), 1U);

    ASSERT_EQ(capabilitiesMaps[0]["interface"], TEST_INTERFACE_NAME);
    ASSERT_EQ(capabilitiesMaps[0]["version"], TEST_VERSION);
    ASSERT_EQ(capabilitiesMaps[0]["type"], TEST_TYPE);
    ASSERT_EQ(capabilitiesMaps[0]["instance"], TEST_INSTANCE_NAME.value());
    ASSERT_EQ(capabilitiesMaps[0]["properties"], EXPECTED_TEST_PROPERTIES_STRING);
    ASSERT_EQ(capabilitiesMaps[0]["configuration"], TEST_VALID_CONFIG_JSON);
}

/**
 * Parses the JSON and validates if the expected fields for @c Discovery events are present.
 * @param eventJson The JSON string representation of the event which will be validated.
 * @param expectedName The expected name of the event.
 * @param expectedAuthToken The expected authorization token of the event.
 * @param expectedEndpointIds The expected endpointIds to be sent in the event.
 * @param expectedEventCorrelationToken The expected eventCorrelationToken that needs to be sent.
 */
void validateDiscoveryEvent(
    const std::string& eventJson,
    const std::string& expectedName,
    const std::string& expectedAuthToken,
    const std::vector<std::string>& expectedEndpointIds,
    const std::string& expectedEventCorrelationToken = "") {
    Document document;
    ASSERT_TRUE(jsonUtils::parseJSON(eventJson, &document));

    Value::ConstMemberIterator eventIt;
    ASSERT_TRUE(findNode(document, "event", &eventIt));

    /// header
    Value::ConstMemberIterator headerIt;
    ASSERT_TRUE(findNode(eventIt->value, "header", &headerIt));

    std::string avsNamespace;
    ASSERT_TRUE(retrieveValue(headerIt->value, "namespace", &avsNamespace));
    ASSERT_EQ(avsNamespace, "Alexa.Discovery");

    std::string avsName;
    ASSERT_TRUE(retrieveValue(headerIt->value, "name", &avsName));
    ASSERT_EQ(avsName, expectedName);

    std::string payloadVersion;
    ASSERT_TRUE(retrieveValue(headerIt->value, "payloadVersion", &payloadVersion));
    ASSERT_EQ(payloadVersion, "3");

    if (!expectedEventCorrelationToken.empty()) {
        std::string eventCorrelationToken;
        ASSERT_TRUE(retrieveValue(headerIt->value, "eventCorrelationToken", &eventCorrelationToken));
        ASSERT_EQ(eventCorrelationToken, expectedEventCorrelationToken);
    }

    /// Payload
    Value::ConstMemberIterator payloadIt;
    ASSERT_TRUE(findNode(eventIt->value, "payload", &payloadIt));

    /// Scope
    Value::ConstMemberIterator scopeIt;
    ASSERT_TRUE(findNode(payloadIt->value, "scope", &scopeIt));

    std::string type;
    ASSERT_TRUE(retrieveValue(scopeIt->value, "type", &type));
    ASSERT_EQ(type, "BearerToken");

    std::string authToken;
    ASSERT_TRUE(retrieveValue(scopeIt->value, "token", &authToken));
    ASSERT_EQ(authToken, expectedAuthToken);

    /// Endpoints
    Value::ConstMemberIterator endpointsIt;
    ASSERT_TRUE(findNode(payloadIt->value, "endpoints", &endpointsIt));
    ASSERT_TRUE(endpointsIt->value.IsArray());

    std::vector<std::string> endpointIds;
    std::string endpointId;
    for (auto it = endpointsIt->value.Begin(); it != endpointsIt->value.End(); ++it) {
        ASSERT_TRUE(retrieveValue(*it, "endpointId", &endpointId));
        endpointIds.push_back(endpointId);
    }
    ASSERT_EQ(endpointIds, expectedEndpointIds);
}

/**
 * Test harness for testing @c DiscoveryUtils methods.
 */
class DiscoveryUtilsTest : public Test {};

/**
 * Test if the validateCapabilityConfiguration method works as expected.
 */
TEST_F(DiscoveryUtilsTest, test_validateCapabilityConfiguration) {
    /// Invalid Type
    ASSERT_FALSE(validateCapabilityConfiguration(CapabilityConfiguration("", TEST_INTERFACE_NAME, TEST_VERSION)));

    /// Invalid Interface Name
    ASSERT_FALSE(validateCapabilityConfiguration(CapabilityConfiguration(TEST_TYPE, "", TEST_VERSION)));

    /// Invalid Version
    ASSERT_FALSE(validateCapabilityConfiguration(CapabilityConfiguration(TEST_TYPE, TEST_INTERFACE_NAME, "")));

    /// Invalid instance name
    Optional<std::string> invalidInstanceName("");
    ASSERT_FALSE(validateCapabilityConfiguration(
        CapabilityConfiguration(TEST_TYPE, TEST_INTERFACE_NAME, TEST_VERSION, invalidInstanceName)));

    /// Invalid supported list
    std::vector<std::string> supportedList;
    Properties invalidSupportedListProperties(false, false, supportedList);
    ASSERT_FALSE(validateCapabilityConfiguration(CapabilityConfiguration(
        TEST_TYPE,
        TEST_INTERFACE_NAME,
        TEST_VERSION,
        TEST_INSTANCE_NAME,
        Optional<Properties>(invalidSupportedListProperties))));

    /// Invalid custom configuration
    CapabilityConfiguration::AdditionalConfigurations additionalConfigurations;
    additionalConfigurations.insert({"TEST", "abc:"});
    ASSERT_FALSE(validateCapabilityConfiguration(CapabilityConfiguration(
        TEST_TYPE, TEST_INTERFACE_NAME, TEST_VERSION, TEST_INSTANCE_NAME, TEST_PROPERTIES, additionalConfigurations)));

    /// Valid case with fully loaded @c CapabilityConfiguration.
    ASSERT_TRUE(validateCapabilityConfiguration(CapabilityConfiguration(
        TEST_TYPE,
        TEST_INTERFACE_NAME,
        TEST_VERSION,
        TEST_INSTANCE_NAME,
        TEST_PROPERTIES,
        TEST_ADDITIONAL_CONFIGURATIONS)));
}

/**
 * Test if the validateAVSDiscoveryEndpointAttributes method works as expected.
 */
TEST_F(DiscoveryUtilsTest, test_validateAVSDiscoveryEndpointAttributes) {
    /// Invalid EndpointID.
    ASSERT_FALSE(validateEndpointAttributes(createEndpointAttributes(
        "", TEST_FRIENDLY_NAME, TEST_DESCRIPTION, TEST_MANUFACTURER_NAME, TEST_DISPLAY_CATEGORIES)));

    /// Invalid Description
    ASSERT_FALSE(validateEndpointAttributes(createEndpointAttributes(
        TEST_ENDPOINT_ID, TEST_FRIENDLY_NAME, "", TEST_MANUFACTURER_NAME, TEST_DISPLAY_CATEGORIES)));

    /// Invalid Manufacturer Name
    ASSERT_FALSE(validateEndpointAttributes(
        createEndpointAttributes(TEST_ENDPOINT_ID, TEST_FRIENDLY_NAME, TEST_DESCRIPTION, "", TEST_DISPLAY_CATEGORIES)));

    /// Invalid Display Categories
    ASSERT_FALSE(validateEndpointAttributes(
        createEndpointAttributes(TEST_ENDPOINT_ID, TEST_FRIENDLY_NAME, TEST_DESCRIPTION, TEST_MANUFACTURER_NAME, {})));

    /// Valid Endpoint Attributes
    ASSERT_TRUE(validateEndpointAttributes(createEndpointAttributes(
        TEST_ENDPOINT_ID, TEST_FRIENDLY_NAME, TEST_DESCRIPTION, TEST_MANUFACTURER_NAME, TEST_DISPLAY_CATEGORIES)));
}

/**
 * Test if the getEndpointConfigJson method works as expected.
 */
TEST_F(DiscoveryUtilsTest, test_formatEndpointConfigJson) {
    validateEndpointConfigJson(getEndpointConfigJson(getTestEndpointAttributes(), getTestCapabilities()));
}

/**
 * Test Endpoint configuration json for DeleteReport events.
 */
TEST_F(DiscoveryUtilsTest, test_getDeleteReportEndpointConfigJson) {
    EXPECT_EQ(TEST_ENDPOINT_CONFIG, getDeleteReportEndpointConfigJson(TEST_ENDPOINT_ID));
}

/**
 * Test event format for @c Discovery.AddOrUpdateReport event.
 */
TEST_F(DiscoveryUtilsTest, test_discoveryAddOrUpdateReportEvent) {
    std::vector<std::string> testEndpointConfigs = {TEST_ENDPOINT_CONFIG};
    auto pair = getAddOrUpdateReportEventJson(testEndpointConfigs, TEST_AUTH_TOKEN);
    validateDiscoveryEvent(pair.first, ADD_OR_UPDATE_REPORT_EVENT_NAME, TEST_AUTH_TOKEN, {TEST_ENDPOINT_ID});
}

/**
 * Test event format for @c Discovery.DeleteReport event.
 */
TEST_F(DiscoveryUtilsTest, test_deleteReportEvent) {
    std::vector<std::string> testEndpointConfigs = {TEST_ENDPOINT_CONFIG};
    auto event = getDeleteReportEventJson(testEndpointConfigs, TEST_AUTH_TOKEN);
    validateDiscoveryEvent(event, DELETE_REPORT_EVENT_NAME, TEST_AUTH_TOKEN, {TEST_ENDPOINT_ID});
}

}  // namespace test
}  // namespace utils
}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK
