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

/// @file EndpointTest.cpp

#include <gtest/gtest.h>

#include <AVSCommon/SDKInterfaces/MockDirectiveHandler.h>
#include <AVSCommon/Utils/Common/MockRequiresShutdown.h>
#include <Endpoints/Endpoint.h>

namespace alexaClientSDK {
namespace endpoints {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::endpoints;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils;
using namespace avsCommon::utils::test;
using namespace ::testing;

static const CapabilityConfiguration CAPABILITY_CONFIGURATION =
    CapabilityConfiguration({"TEST_TYPE", "TEST_INTERFACE_NAME", "2.0"});
static const std::string EMPTY_ID = "";
static const std::string EMPTY_STRING = "";

/// Test harness for @c Endpoint class.
class EndpointTest : public Test {
public:
    /// Setup the test harness for running the test.
    void SetUp() override;

    /// Clean up the test harness after running the test.
    void TearDown() override;

    /// Create valid attributes for testing.
    AVSDiscoveryEndpointAttributes createValidAttributes();

    /// Mock of @c DirectiveHandlerInterface.
    std::shared_ptr<MockDirectiveHandler> m_mockDirectiveHandler;
};

void EndpointTest::SetUp() {
    m_mockDirectiveHandler = std::make_shared<MockDirectiveHandler>();
}

void EndpointTest::TearDown() {
}

AVSDiscoveryEndpointAttributes EndpointTest::createValidAttributes() {
    AVSDiscoveryEndpointAttributes attributes;
    attributes.endpointId = "TEST_ENDPOINT_ID";
    attributes.friendlyName = "TEST_FRIENDLY_NAME";
    attributes.description = "TEST_DESCRIPTION";
    attributes.manufacturerName = "TEST_MANUFACTURER_NAME";
    attributes.displayCategories = {"TEST_DISPLAY_CATEGORY"};
    return attributes;
}

/**
 * Tests @c Endpoint constructor, expecting to successfully create a new Endpoint.
 */
TEST_F(EndpointTest, test_endpointConstructor) {
    auto attributes = createValidAttributes();
    auto endpoint = new Endpoint(attributes);
    ASSERT_NE(endpoint, nullptr);
}

/**
 * Tests @c addRequireShutdownObjects, expecting to successfully update require shutdown objects without errors or
 * crashing. Then expects that @c doShutdown is called from within the endpoint destructor to release resources.
 */
TEST_F(EndpointTest, test_addRequireShutdownObjects) {
    auto attributes = createValidAttributes();
    auto endpoint = std::make_shared<Endpoint>(attributes);
    std::shared_ptr<MockRequiresShutdown> mockRequiresShutdown =
        std::make_shared<MockRequiresShutdown>("TEST_REQUIRE_SHUTDOWN_OBJECT");
    endpoint->addRequireShutdownObjects({mockRequiresShutdown});
    EXPECT_CALL(*mockRequiresShutdown, doShutdown());
}

/**
 * Tests @c getEndpointId, expecting value to be correct.
 */
TEST_F(EndpointTest, test_getEndpointId) {
    auto attributes = createValidAttributes();
    auto endpoint = std::make_shared<Endpoint>(attributes);
    ASSERT_EQ(endpoint->getEndpointId(), "TEST_ENDPOINT_ID");
}

/**
 * Tests @c update with an empty endpoint id, expecting @c update to fail and to return @c false.
 */
TEST_F(EndpointTest, test_updateWithValidAttributesAndInvalidEndpointId) {
    auto attributes = createValidAttributes();
    auto endpoint = std::make_shared<Endpoint>(attributes);
    attributes.endpointId = "TEST_ENDPOINT_ID_OVERWRITE";
    auto endpointModificationData = std::make_shared<EndpointModificationData>(
        EndpointModificationData(EMPTY_ID, attributes, {CAPABILITY_CONFIGURATION}, {}, {}, {}));
    ASSERT_FALSE(endpoint->update(endpointModificationData));
}

/**
 * Tests @c update with invalid updated attributes (ex: friendly name being an empty string) and valid endpoint data,
 * expecting @c update to fail and to return @c false.
 */
TEST_F(EndpointTest, test_updateWithInvalidAttributesAndValidEndpointAttributes) {
    auto attributes = createValidAttributes();
    auto endpoint = std::make_shared<Endpoint>(attributes);
    attributes.friendlyName = EMPTY_STRING;
    auto endpointModificationData = std::make_shared<EndpointModificationData>(
        EndpointModificationData("TEST_ENDPOINT_ID", attributes, {CAPABILITY_CONFIGURATION}, {}, {}, {}));
    ASSERT_FALSE(endpoint->update(endpointModificationData));
}

/**
 * Tests @c update with valid updated attributes and valid endpoint data, expecting @c update to succeed and to return
 * @c true.
 */
TEST_F(EndpointTest, test_updateSuccess) {
    auto attributes = createValidAttributes();
    auto endpoint = std::make_shared<Endpoint>(attributes);
    auto endpointModificationData = std::make_shared<EndpointModificationData>(
        EndpointModificationData("TEST_ENDPOINT_ID", attributes, {CAPABILITY_CONFIGURATION}, {}, {}, {}));
    ASSERT_TRUE(endpoint->update(endpointModificationData));
}

/**
 * Tests @c update by first adding a capability through @c addCapabilityConfiguration, then calling update with the same
 * capability, expecting @c update to succeed by keeping one copy of the same capability and to return @c true.
 *
 * @note This test also tests @c addCapabilityConfiguration.
 */
TEST_F(EndpointTest, test_updateDuplicateCapabilities) {
    auto attributes = createValidAttributes();
    auto endpoint = std::make_shared<Endpoint>(attributes);
    ASSERT_TRUE(endpoint->getCapabilities().empty());
    endpoint->addCapabilityConfiguration(CAPABILITY_CONFIGURATION);
    ASSERT_TRUE(endpoint->getCapabilities().size() == 1);
    auto endpointModificationData = std::make_shared<EndpointModificationData>(
        EndpointModificationData("TEST_ENDPOINT_ID", attributes, {CAPABILITY_CONFIGURATION}, {}, {}, {}));
    ASSERT_TRUE(endpoint->update(endpointModificationData));
    ASSERT_TRUE(endpoint->getCapabilities().size() == 1);
}

/**
 * Tests @c update by first adding a capability through @c addCapabilityConfiguration, then calling update with the same
 * capability, expecting @c update to succeed by still having the remaining copy in the currenCapabilities.
 * Tests updates on both additions to ensure that order does not matter.
 * Tests a remove to ensure that the correct instance is removed after update.
 */
TEST_F(EndpointTest, test_updateSameInterfaceDifferentInstances) {
    auto attributes = createValidAttributes();
    auto endpoint = std::make_shared<Endpoint>(attributes);
    ASSERT_TRUE(endpoint->getCapabilities().empty());

    const auto instanceOne = avsCommon::utils::Optional<std::string>("TV.1");
    const auto instanceTwo = avsCommon::utils::Optional<std::string>("TV.2");
    const CapabilityConfiguration capabilityConfigurationOne =
        CapabilityConfiguration({"TEST_TYPE", "TEST_INTERFACE_NAME", "2.0", instanceOne});
    const CapabilityConfiguration capabilityConfigurationTwo =
        CapabilityConfiguration({"TEST_TYPE", "TEST_INTERFACE_NAME", "2.0", instanceTwo});
    ASSERT_EQ(capabilityConfigurationOne.instanceName.value(), "TV.1");
    ASSERT_EQ(capabilityConfigurationTwo.instanceName.value(), "TV.2");

    endpoint->addCapabilityConfiguration(capabilityConfigurationOne);
    ASSERT_TRUE(endpoint->getCapabilities().size() == 1);
    endpoint->addCapabilityConfiguration(capabilityConfigurationTwo);
    ASSERT_TRUE(endpoint->getCapabilities().size() == 2);

    auto endpointModificationDataOne = std::make_shared<EndpointModificationData>(
        EndpointModificationData("TEST_ENDPOINT_ID", attributes, {capabilityConfigurationOne}, {}, {}, {}));
    ASSERT_TRUE(endpoint->update(endpointModificationDataOne));
    ASSERT_TRUE(endpoint->getCapabilities().size() == 2);

    auto endpointModificationDataTwo = std::make_shared<EndpointModificationData>(
        EndpointModificationData("TEST_ENDPOINT_ID", attributes, {capabilityConfigurationTwo}, {}, {}, {}));
    ASSERT_TRUE(endpoint->update(endpointModificationDataTwo));
    ASSERT_TRUE(endpoint->getCapabilities().size() == 2);

    auto endpointModificationDataThree = std::make_shared<EndpointModificationData>(
        EndpointModificationData("TEST_ENDPOINT_ID", attributes, {}, {}, {capabilityConfigurationTwo}, {}));
    ASSERT_TRUE(endpoint->update(endpointModificationDataThree));
    ASSERT_TRUE(endpoint->getCapabilities().size() == 1);
    ASSERT_TRUE(endpoint->getCapabilities().begin()->first.instanceName.value() == "TV.1");
}

/**
 * Tests @c addCapability with a null directive handler, expecting @c addCapability to fail and to return @c false.
 */
TEST_F(EndpointTest, test_addCapabilityNullDirectiveHandler) {
    auto attributes = createValidAttributes();
    auto endpoint = std::make_shared<Endpoint>(attributes);
    ASSERT_FALSE(endpoint->addCapability(CAPABILITY_CONFIGURATION, nullptr));
}

/**
 * Tests @c addCapability with a capability duplicate, expecting @c addCapability to return @c false, after attempting
 * to add a duplicate.
 */
TEST_F(EndpointTest, test_addCapabilityDuplicate) {
    auto attributes = createValidAttributes();
    auto endpoint = std::make_shared<Endpoint>(attributes);
    ASSERT_TRUE(endpoint->getCapabilities().empty());
    ASSERT_TRUE(endpoint->addCapability(CAPABILITY_CONFIGURATION, m_mockDirectiveHandler));
    ASSERT_FALSE(endpoint->addCapability(CAPABILITY_CONFIGURATION, m_mockDirectiveHandler));
}

/**
 * Tests @c addCapability with valid parameters, expecting @c addCapability to succeed and to return @c true.
 */
TEST_F(EndpointTest, test_addCapabilitySuccess) {
    auto attributes = createValidAttributes();
    auto endpoint = std::make_shared<Endpoint>(attributes);
    ASSERT_TRUE(endpoint->addCapability(CAPABILITY_CONFIGURATION, m_mockDirectiveHandler));
}

/**
 * Tests @c removeCapability with a capability that does not exist, expecting @c removeCapability to fail and to return
 * @c false.
 */
TEST_F(EndpointTest, test_removeCapabilityThatDoesNotExist) {
    auto attributes = createValidAttributes();
    auto endpoint = std::make_shared<Endpoint>(attributes);
    ASSERT_TRUE(endpoint->getCapabilities().empty());
    ASSERT_FALSE(endpoint->removeCapability(CAPABILITY_CONFIGURATION));
}

/**
 * Tests @c removeCapability with a capability that does exist, expecting @c removeCapability to succeed and to return
 * @c true.
 *
 * @note This test also tests @c addCapability.
 */
TEST_F(EndpointTest, test_removeCapabilitySuccess) {
    auto attributes = createValidAttributes();
    auto endpoint = std::make_shared<Endpoint>(attributes);
    ASSERT_TRUE(endpoint->addCapability(CAPABILITY_CONFIGURATION, m_mockDirectiveHandler));
    ASSERT_TRUE(endpoint->removeCapability(CAPABILITY_CONFIGURATION));
}

/**
 * Tests @c addCapabilityConfiguration with a duplicate capability configuration. Specifically the first addition should
 * succeed, but the second addition should fail, expecting @c removeCapability to return @c false, after attempting to
 * add a duplicate.
 */
TEST_F(EndpointTest, test_addCapabilityConfigurationDuplicate) {
    auto attributes = createValidAttributes();
    auto endpoint = std::make_shared<Endpoint>(attributes);
    ASSERT_TRUE(endpoint->addCapabilityConfiguration(CAPABILITY_CONFIGURATION));
    ASSERT_FALSE(endpoint->addCapabilityConfiguration(CAPABILITY_CONFIGURATION));
}

/**
 * Tests @c validateEndpointAttributes with an invalid/empty endpoint id, expecting @c validateEndpointAttributes to
 * fail and to return @c false.
 */
TEST_F(EndpointTest, test_validateEndpointAttributesInvalidEndpointId) {
    auto attributes = createValidAttributes();
    auto endpoint = std::make_shared<Endpoint>(attributes);
    attributes.endpointId = EMPTY_STRING;
    ASSERT_FALSE(endpoint->validateEndpointAttributes(attributes));
}

/**
 * Tests @c validateEndpointAttributes with an invalid/empty friendly name, expecting @c validateEndpointAttributes to
 * fail and to return @c false.
 */
TEST_F(EndpointTest, test_validateEndpointAttributesInvalidFriendlyName) {
    auto attributes = createValidAttributes();
    auto endpoint = std::make_shared<Endpoint>(attributes);
    attributes.friendlyName = EMPTY_STRING;
    ASSERT_FALSE(endpoint->validateEndpointAttributes(attributes));
}

/**
 * Tests @c validateEndpointAttributes with an invalid/empty description, expecting @c validateEndpointAttributes to
 * fail and to return @c false.
 */
TEST_F(EndpointTest, test_validateEndpointAttributesInvalidDescription) {
    auto attributes = createValidAttributes();
    auto endpoint = std::make_shared<Endpoint>(attributes);
    attributes.description = EMPTY_STRING;
    ASSERT_FALSE(endpoint->validateEndpointAttributes(attributes));
}

/**
 * Tests @c validateEndpointAttributes with an invalid/empty manufacturer name, expecting @c validateEndpointAttributes
 * to fail and to return @c false.
 */
TEST_F(EndpointTest, test_validateEndpointAttributesInvalidManufacturerName) {
    auto attributes = createValidAttributes();
    auto endpoint = std::make_shared<Endpoint>(attributes);
    attributes.manufacturerName = EMPTY_STRING;
    ASSERT_FALSE(endpoint->validateEndpointAttributes(attributes));
}

/**
 * Tests @c validateEndpointAttributes with invalid/oversized additional attributes, expecting @c
 * validateEndpointAttributes to fail and to return @c false.
 */
TEST_F(EndpointTest, test_validateEndpointAttributesInvalidAdditionalAttributes) {
    auto attributes = createValidAttributes();
    auto endpoint = std::make_shared<Endpoint>(attributes);
    std::string invalidManufacturerNameLength(300, 'c');
    attributes.additionalAttributes =
        Optional<AVSDiscoveryEndpointAttributes::AdditionalAttributes>({invalidManufacturerNameLength,
                                                                        "TEST_MODEL",
                                                                        "TEST_SERIAL_NUMBER",
                                                                        "TEST_FIRMWARE_VERSION",
                                                                        "TEST_SOFTWARE_VERSION",
                                                                        "TEST_CUSTOM_IDENTIFIER"});
    ASSERT_FALSE(endpoint->validateEndpointAttributes(attributes));
}

/**
 * Tests @c validateEndpointAttributes with invalid connections, expecting @c validateEndpointAttributes to fail and to
 * return @c false.
 */
TEST_F(EndpointTest, test_validateEndpointAttributesInvalidConnections) {
    auto attributes = createValidAttributes();
    auto endpoint = std::make_shared<Endpoint>(attributes);
    std::map<std::string, std::string> invalidConnections;
    invalidConnections.insert({"connectionKey", ""});
    attributes.connections = {invalidConnections};
    ASSERT_FALSE(endpoint->validateEndpointAttributes(attributes));
}

/**
 * Tests @c validateEndpointAttributes with invalid/oversized cookies, expecting @c validateEndpointAttributes to fail
 * and to return @c false.
 */
TEST_F(EndpointTest, test_validateEndpointAttributesInvalidCookies) {
    auto attributes = createValidAttributes();
    auto endpoint = std::make_shared<Endpoint>(attributes);
    std::map<std::string, std::string> invalidCookies;
    std::string invalidCookieLength(6000, 'c');
    invalidCookies.insert({"cookieKey", invalidCookieLength});
    attributes.cookies = invalidCookies;
    ASSERT_FALSE(endpoint->validateEndpointAttributes(attributes));
}

/**
 * Tests @c validateEndpointAttributes with valid parameters, expecting @c validateEndpointAttributes to succeed and to
 * return @c true.
 */
TEST_F(EndpointTest, test_validateEndpointAttributesSuccess) {
    auto attributes = createValidAttributes();
    auto endpoint = std::make_shared<Endpoint>(attributes);
    ASSERT_TRUE(endpoint->validateEndpointAttributes(attributes));
}

}  // namespace test
}  // namespace endpoints
}  // namespace alexaClientSDK