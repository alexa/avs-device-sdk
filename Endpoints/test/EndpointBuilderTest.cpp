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

/// @file EndpointBuilderTest.cpp

#include <gtest/gtest.h>

#include <AVSCommon/SDKInterfaces/MockCapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandler.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <Endpoints/EndpointBuilder.h>
#include <MockAlexaInterfaceMessageSenderInternal.h>

namespace alexaClientSDK {
namespace endpoints {
namespace test {

const static std::string EMPTY_STRING = "";
static const std::shared_ptr<avsCommon::avs::CapabilityConfiguration> VALID_CAPABILITY_CONFIGURATION =
    std::make_shared<avsCommon::avs::CapabilityConfiguration>("TEST_TYPE", "TEST_INTERFACE_NAME", "2.0");
static const std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>>
    VALID_CAPABILITY_CONFIGURATION_SET{VALID_CAPABILITY_CONFIGURATION};
static const std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>>
    INVALID_CAPABILITY_CONFIGURATION_SET{nullptr};

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace ::testing;

/// Test harness for @c EndpointBuilder class.
class EndpointBuilderTest : public Test {
public:
    /// Setup the test harness for running the test.
    void SetUp() override;

    /// Clean up the test harness after running the test.
    void TearDown() override;

    /// Setup used for most but not all test cases.
    EndpointBuilder createEndpointBuilder();

    /// Test instance of device info.
    std::shared_ptr<avsCommon::utils::DeviceInfo> m_deviceInfo;
    /// Mock of @c ContextManagerInterface.
    std::shared_ptr<MockContextManager> m_mockContextManager;
    /// Mock of @c ExceptionEncounteredSenderInterface.
    std::shared_ptr<MockExceptionEncounteredSender> m_mockExceptionSender;
    /// Mock of @c AlexaInterfaceMessageSenderInternalInterface.
    std::shared_ptr<capabilityAgents::alexa::test::MockAlexaInterfaceMessageSenderInternal> m_mockResponseSender;
    /// Mock of @c CapabilityConfigurationInterface.
    std::shared_ptr<MockCapabilityConfigurationInterface> m_mockCapabilityConfigurationInterface;
    /// Mock of @c DirectiveHandlerInterface.
    std::shared_ptr<MockDirectiveHandler> m_mockDirectiveHandler;
};

void EndpointBuilderTest::SetUp() {
    m_deviceInfo = avsCommon::utils::DeviceInfo::create(
        "TEST_CLIENT_ID", "TEST_PRODUCT_ID", "1234", "TEST_MANUFACTURER_NAME", "TEST_DESCRIPTION");
    m_mockContextManager = std::make_shared<MockContextManager>();
    m_mockExceptionSender = std::make_shared<MockExceptionEncounteredSender>();
    m_mockResponseSender = std::make_shared<capabilityAgents::alexa::test::MockAlexaInterfaceMessageSenderInternal>();
    m_mockCapabilityConfigurationInterface = std::make_shared<MockCapabilityConfigurationInterface>();
    m_mockDirectiveHandler = std::make_shared<MockDirectiveHandler>();
}

void EndpointBuilderTest::TearDown() {
}

EndpointBuilder EndpointBuilderTest::createEndpointBuilder() {
    auto endpointBuilder =
        EndpointBuilder::create(m_deviceInfo, m_mockContextManager, m_mockExceptionSender, m_mockResponseSender);
    /// EndpointBuilder requires friendly name to successfully build non-default endpoints
    return endpointBuilder->withFriendlyName("TEST_FRIENDLY_NAME");
}

/**
 * Tests @c create with null device info, expecting @c create to fail and to return @c nullptr.
 */
TEST_F(EndpointBuilderTest, test_createEndpointBuilderNullDeviceInfo) {
    ASSERT_EQ(
        EndpointBuilder::create(nullptr, m_mockContextManager, m_mockExceptionSender, m_mockResponseSender), nullptr);
}

/**
 * Tests @c create with null context manager, expecting @c create to fail and to return @c nullptr.
 */
TEST_F(EndpointBuilderTest, test_createEndpointBuilderNullContextManager) {
    ASSERT_EQ(EndpointBuilder::create(m_deviceInfo, nullptr, m_mockExceptionSender, m_mockResponseSender), nullptr);
}

/**
 * Tests @c create with null exception sender, expecting @c create to fail and to return @c nullptr.
 */
TEST_F(EndpointBuilderTest, test_createEndpointBuilderNullExceptionSender) {
    ASSERT_EQ(EndpointBuilder::create(m_deviceInfo, m_mockContextManager, nullptr, m_mockResponseSender), nullptr);
}

/**
 * Tests @c create with null response sender, expecting @c create to fail and to return @c nullptr.
 */
TEST_F(EndpointBuilderTest, test_createEndpointBuilderNullAlexaMessageSender) {
    ASSERT_EQ(EndpointBuilder::create(m_deviceInfo, m_mockContextManager, m_mockExceptionSender, nullptr), nullptr);
}

/**
 * Tests @c create with correct parameters, expecting to return @c EndpointBuilder.
 */
TEST_F(EndpointBuilderTest, test_createEndpointBuilderSuccess) {
    ASSERT_NE(
        EndpointBuilder::create(m_deviceInfo, m_mockContextManager, m_mockExceptionSender, m_mockResponseSender),
        nullptr);
}

/**
 * Tests @c create with correct parameters (using non-shared ptr deviceInfo), expecting to return @c EndpointBuilder.
 */
TEST_F(EndpointBuilderTest, test_createEndpointBuilderWithoutPointerSuccess) {
    ASSERT_NE(
        EndpointBuilder::create(*m_deviceInfo, m_mockContextManager, m_mockExceptionSender, m_mockResponseSender),
        nullptr);
}

/**
 * Tests @c finalizeAttributes, expecting that each attribute remains unchanged.
 *
 * @note This test also tests @c withDerivedEndpointId, @c withEndpointId, @c withFriendlyName, @c withDescription,
 * @c withManufacturerName, @c withDisplayCategory, and @c withAdditionalAttributes.
 */
TEST_F(EndpointBuilderTest, test_withConfigurationFinalized) {
    auto endpointBuilder = createEndpointBuilder();
    endpointBuilder.finalizeAttributes();
    endpointBuilder.withDerivedEndpointId("TEST_DERIVED_ENDPOINT_ID")
        .withEndpointId("TEST_ENDPOINT_ID")
        .withFriendlyName("TEST_FRIENDLY_NAME_OVERWRITE")
        .withDescription("TEST_DESCRIPTION")
        .withManufacturerName("TEST_MANUFACTURER_NAME")
        .withDisplayCategory({"TEST_DISPLAY_CATEGORY"})
        .withAdditionalAttributes(
            "TEST_MANUFACTURER_NAME",
            "TEST_MODEL",
            "TEST_SERIAL_NUMBER",
            "TEST_FIRMWARE_VERSION",
            "TEST_SOFTWARE_VERSION",
            "TEST_CUSTOM_IDENTIFIER");

    auto endpoint = endpointBuilder.build();
    ASSERT_TRUE(endpoint->getEndpointId().empty());
    ASSERT_EQ(endpoint->getAttributes().friendlyName, "TEST_FRIENDLY_NAME");
    ASSERT_TRUE(endpoint->getAttributes().description.empty());
    ASSERT_TRUE(endpoint->getAttributes().manufacturerName.empty());
    ASSERT_TRUE(endpoint->getAttributes().displayCategories.empty());
    ASSERT_FALSE(endpoint->getAttributes().additionalAttributes.hasValue());
}

/**
 * Tests @c withDerivedEndpointId with an invalid length, expecting endpoint id to remain unchanged.
 */
TEST_F(EndpointBuilderTest, test_withDerivedEndpointIdLengthExceeded) {
    auto endpointBuilder = createEndpointBuilder();
    std::string invalid_length(300, 'c');
    endpointBuilder.withDerivedEndpointId(invalid_length);
    auto endpoint = endpointBuilder.build();
    ASSERT_TRUE(endpoint->getEndpointId().empty());
}

/**
 * Tests @c withDerivedEndpointId, expecting to update endpoint id successfully.
 */
TEST_F(EndpointBuilderTest, test_withDerivedEndpointIdSuccess) {
    auto endpointBuilder = createEndpointBuilder();
    endpointBuilder.withDerivedEndpointId("TEST_DERIVED_ENDPOINT_ID");
    auto endpoint = endpointBuilder.build();
    auto derivedEndpointId = m_deviceInfo->getDefaultEndpointId() + "::" + "TEST_DERIVED_ENDPOINT_ID";
    ASSERT_EQ(endpoint->getEndpointId(), derivedEndpointId);
}

/**
 * Tests that each attribute can be successfully updated when provided with valid input.
 *
 * @note This test also tests @c withEndpointId, @c withFriendlyName, @c withDescription, @c withManufacturerName,
 * @c withDisplayCategory, @c withAdditionalAttributes, @c withConnections, and @c withCookies.
 * @c withDerivedEndpointId, is tested separately so that it does not interfere with @c withEndpointId.
 */
TEST_F(EndpointBuilderTest, test_attributesUpdatedSuccess) {
    auto endpointBuilder = createEndpointBuilder();
    endpointBuilder.withEndpointId("TEST_ENDPOINT_ID")
        .withFriendlyName("TEST_FRIENDLY_NAME")
        .withFriendlyName("TEST_FRIENDLY_NAME_OVERWRITE")
        .withDescription("TEST_DESCRIPTION")
        .withManufacturerName("TEST_MANUFACTURER_NAME")
        .withDisplayCategory({"TEST_DISPLAY_CATEGORY"})
        .withAdditionalAttributes(
            "TEST_MANUFACTURER_NAME",
            "TEST_MODEL",
            "TEST_SERIAL_NUMBER",
            "TEST_FIRMWARE_VERSION",
            "TEST_SOFTWARE_VERSION",
            "TEST_CUSTOM_IDENTIFIER");

    std::map<std::string, std::string> validConnection;
    validConnection.insert({"testKey", "testValue"});
    endpointBuilder.withConnections({validConnection});

    std::map<std::string, std::string> validCookie;
    validCookie.insert({"testKey", "testValue"});
    endpointBuilder.withCookies(validCookie);

    auto endpoint = endpointBuilder.build();
    ASSERT_EQ(endpoint->getEndpointId(), "TEST_ENDPOINT_ID");
    ASSERT_EQ(endpoint->getAttributes().friendlyName, "TEST_FRIENDLY_NAME_OVERWRITE");
    ASSERT_EQ(endpoint->getAttributes().description, "TEST_DESCRIPTION");
    ASSERT_EQ(endpoint->getAttributes().manufacturerName, "TEST_MANUFACTURER_NAME");
    ASSERT_FALSE(endpoint->getAttributes().displayCategories.empty());
    ASSERT_EQ(endpoint->getAttributes().displayCategories.front(), "TEST_DISPLAY_CATEGORY");
    ASSERT_TRUE(endpoint->getAttributes().additionalAttributes.hasValue());
    ASSERT_EQ(endpoint->getAttributes().additionalAttributes.value().manufacturer, "TEST_MANUFACTURER_NAME");
    ASSERT_EQ(endpoint->getAttributes().additionalAttributes.value().model, "TEST_MODEL");
    ASSERT_EQ(endpoint->getAttributes().additionalAttributes.value().serialNumber, "TEST_SERIAL_NUMBER");
    ASSERT_EQ(endpoint->getAttributes().additionalAttributes.value().firmwareVersion, "TEST_FIRMWARE_VERSION");
    ASSERT_EQ(endpoint->getAttributes().additionalAttributes.value().softwareVersion, "TEST_SOFTWARE_VERSION");
    ASSERT_EQ(endpoint->getAttributes().additionalAttributes.value().customIdentifier, "TEST_CUSTOM_IDENTIFIER");
    ASSERT_FALSE(endpoint->getAttributes().connections.empty());
    ASSERT_EQ(endpoint->getAttributes().connections.front(), validConnection);
    ASSERT_FALSE(endpoint->getAttributes().cookies.empty());
    ASSERT_EQ(endpoint->getAttributes().cookies, validCookie);
}

/**
 * Tests @c withEndpointId with an invalid length, expecting @c build to fail and to return @c nullptr.
 */
TEST_F(EndpointBuilderTest, test_withEndpointIdInvalid) {
    auto endpointBuilder = createEndpointBuilder();
    std::string invalid_length(300, 'c');
    endpointBuilder.withEndpointId(invalid_length);
    ASSERT_EQ(endpointBuilder.build(), nullptr);
}

/**
 * Tests @c withFriendlyName with an invalid friendly name, expecting @c build to fail and to return @c nullptr.
 *
 * @note This test specifically does not use createEndpointBuilder.
 */
TEST_F(EndpointBuilderTest, test_withFriendlyNameInvalid) {
    auto endpointBuilder =
        EndpointBuilder::create(m_deviceInfo, m_mockContextManager, m_mockExceptionSender, m_mockResponseSender);
    endpointBuilder->withFriendlyName(EMPTY_STRING);
    ASSERT_EQ(endpointBuilder->build(), nullptr);
}

/**
 * Tests @c withDescription with an invalid description, expecting @c build to fail and to return @c nullptr.
 */
TEST_F(EndpointBuilderTest, test_withDescriptionInvalid) {
    auto endpointBuilder = createEndpointBuilder();
    endpointBuilder.withDescription(EMPTY_STRING);
    ASSERT_EQ(endpointBuilder.build(), nullptr);
}

/**
 * Tests @c withManufacturerName with an invalid manufacturer name, expecting @c build to fail and to return @c nullptr.
 */
TEST_F(EndpointBuilderTest, test_withManufacturerNameInvalid) {
    auto endpointBuilder = createEndpointBuilder();
    endpointBuilder.withManufacturerName(EMPTY_STRING);
    ASSERT_EQ(endpointBuilder.build(), nullptr);
}

/**
 * Tests @c withDisplayCategory with an invalid display category, expecting @c build to fail and to return @c nullptr.
 */
TEST_F(EndpointBuilderTest, test_withDisplayCategoryInvalid) {
    auto endpointBuilder = createEndpointBuilder();
    endpointBuilder.withDisplayCategory({});
    ASSERT_EQ(endpointBuilder.build(), nullptr);
}

/**
 * Tests @c withAdditionalAttributes with invalid additional attributes, expecting @c build to fail and to return @c
 * nullptr.
 */
TEST_F(EndpointBuilderTest, test_withAdditionalAttributesInvalid) {
    auto endpointBuilder = createEndpointBuilder();
    std::string invalid_length(300, 'c');
    endpointBuilder.withAdditionalAttributes(
        invalid_length,
        "TEST_MODEL",
        "TEST_SERIAL_NUMBER",
        "TEST_FIRMWARE_VERSION",
        "TEST_SOFTWARE_VERSION",
        "TEST_CUSTOM_IDENTIFIER");
    ASSERT_EQ(endpointBuilder.build(), nullptr);
}

/**
 * Tests @c withConnections with an invalid connection (ex: every connection key requires a value), expecting @c build
 * to fail and to return @c nullptr.
 */
TEST_F(EndpointBuilderTest, test_withConnectionsInvalid) {
    auto endpointBuilder = createEndpointBuilder();
    std::map<std::string, std::string> invalidConnection;
    invalidConnection.insert({"testKey", EMPTY_STRING});
    endpointBuilder.withConnections({invalidConnection});
    ASSERT_EQ(endpointBuilder.build(), nullptr);
}

/**
 * Tests @c withCookies with invalid cookies, expecting @c build to fail and to return @c nullptr.
 */
TEST_F(EndpointBuilderTest, test_withCookiesInvalid) {
    auto endpointBuilder = createEndpointBuilder();
    std::map<std::string, std::string> invalidCookie;
    std::string invalid_length(6000, 'c');
    invalidCookie.insert({invalid_length, invalid_length});
    endpointBuilder.withCookies(invalidCookie);
    ASSERT_EQ(endpointBuilder.build(), nullptr);
}

#ifndef POWER_CONTROLLER
/**
 * Tests @c withPowerController without being enabled, expecting @c build to fail and to return @c nullptr.
 *
 * @note This function is deprecated use the new @c withEndpointCapabilitiesBuilder() method instead.
 */
TEST_F(EndpointBuilderTest, test_withPowerControllerNotEnabled) {
    auto endpointBuilder = createEndpointBuilder();
    auto powerController = std::shared_ptr<powerController::PowerControllerInterface>();
    endpointBuilder.withPowerController(powerController, true, true);
    ASSERT_EQ(endpointBuilder.build(), nullptr);
}
#endif

#ifndef TOGGLE_CONTROLLER
/**
 * Tests @c withToggleController without being enabled, expecting @c build to fail and to return @c nullptr.
 *
 * @note This function is deprecated use the new @c withEndpointCapabilitiesBuilder() method instead.
 */
TEST_F(EndpointBuilderTest, test_withToggleControllerNotEnabled) {
    auto endpointBuilder = createEndpointBuilder();
    auto toggleController = std::shared_ptr<toggleController::ToggleControllerInterface>();
    toggleController::ToggleControllerAttributes toggleControllerAttributes;
    endpointBuilder.withToggleController(
        toggleController, "TEST_INSTANCE", toggleControllerAttributes, true, true, false);
    ASSERT_EQ(endpointBuilder.build(), nullptr);
}
#endif

#ifndef MODE_CONTROLLER
/**
 * Tests @c withModeController without being enabled, expecting @c build to fail and to return @c nullptr.
 *
 * @note This function is deprecated use the new @c withEndpointCapabilitiesBuilder() method instead.
 */
TEST_F(EndpointBuilderTest, test_withModeControllerNotEnabled) {
    auto endpointBuilder = createEndpointBuilder();
    auto modeController = std::shared_ptr<modeController::ModeControllerInterface>();
    modeController::ModeControllerAttributes modeControllerAttributes;
    endpointBuilder.withModeController(modeController, "TEST_INSTANCE", modeControllerAttributes, true, true, false);
    ASSERT_EQ(endpointBuilder.build(), nullptr);
}
#endif

#ifndef RANGE_CONTROLLER
/**
 * Tests @c withRangeController without being enabled, expecting @c build to fail and to return @c nullptr.
 *
 * @note This function is deprecated use the new @c withEndpointCapabilitiesBuilder() method instead.
 */
TEST_F(EndpointBuilderTest, test_withRangeControllerNotEnabled) {
    auto endpointBuilder = createEndpointBuilder();
    auto rangeController = std::shared_ptr<rangeController::RangeControllerInterface>();
    rangeController::RangeControllerAttributes rangeControllerAttributes;
    endpointBuilder.withRangeController(rangeController, "TEST_INSTANCE", rangeControllerAttributes, true, true, false);
    ASSERT_EQ(endpointBuilder.build(), nullptr);
}
#endif

/**
 * Tests @c withCapability with a valid capability configuration, expecting to update capabilities successfully.
 */
TEST_F(EndpointBuilderTest, test_withCapabilitySuccess) {
    auto endpointBuilder = createEndpointBuilder();
    endpointBuilder.withCapability(*VALID_CAPABILITY_CONFIGURATION, m_mockDirectiveHandler);
    auto endpoint = endpointBuilder.build();
    ASSERT_FALSE(endpoint->getCapabilities().empty());
}

/**
 * Tests @c withCapability with a null configuration interface, expecting @c build to fail and to return @c nullptr.
 */
TEST_F(EndpointBuilderTest, test_withCapabilityNullConfigurationInterface) {
    auto endpointBuilder = createEndpointBuilder();
    endpointBuilder.withCapability(nullptr, m_mockDirectiveHandler);
    ASSERT_EQ(endpointBuilder.build(), nullptr);
}

/**
 * Tests @c withCapability with a null directive handler, expecting @c build to fail and to return @c nullptr.
 */
TEST_F(EndpointBuilderTest, test_withCapabilityNullDirectiveHandler) {
    auto endpointBuilder = createEndpointBuilder();
    endpointBuilder.withCapability(m_mockCapabilityConfigurationInterface, nullptr);
    ASSERT_EQ(endpointBuilder.build(), nullptr);
}

/**
 * Tests @c withCapability with an invalid capability configuration, expecting @c build to fail and to return @c
 * nullptr.
 */
TEST_F(EndpointBuilderTest, test_withCapabilityInvalidConfiguration) {
    auto endpointBuilder = createEndpointBuilder();
    endpointBuilder.withCapability(m_mockCapabilityConfigurationInterface, m_mockDirectiveHandler);
    ASSERT_EQ(endpointBuilder.build(), nullptr);
}

/**
 * Tests @c withCapability with a valid capability configuration, expecting to update capabilities successfully.
 */
TEST_F(EndpointBuilderTest, test_withCapabilityUsingInterfaceSuccess) {
    auto endpointBuilder = createEndpointBuilder();
    EXPECT_CALL(*m_mockCapabilityConfigurationInterface, getCapabilityConfigurations())
        .WillOnce(Return(VALID_CAPABILITY_CONFIGURATION_SET));
    endpointBuilder.withCapability(m_mockCapabilityConfigurationInterface, m_mockDirectiveHandler);
    auto endpoint = endpointBuilder.build();
    ASSERT_FALSE(endpoint->getCapabilities().empty());
}

/**
 * Tests @c withCapabilityConfiguration with a null configuration interface, expecting @c build to fail and to return @c
 * nullptr.
 */
TEST_F(EndpointBuilderTest, test_withCapabilityConfigurationNullConfigurationInterface) {
    auto endpointBuilder = createEndpointBuilder();
    endpointBuilder.withCapabilityConfiguration(nullptr);
    ASSERT_EQ(endpointBuilder.build(), nullptr);
}

/**
 * Tests @c withCapabilityConfiguration with a null configuration, expecting @c build to fail and to return @c nullptr.
 */
TEST_F(EndpointBuilderTest, test_withCapabilityConfigurationNullConfiguration) {
    auto endpointBuilder = createEndpointBuilder();
    EXPECT_CALL(*m_mockCapabilityConfigurationInterface, getCapabilityConfigurations())
        .WillOnce(Return(INVALID_CAPABILITY_CONFIGURATION_SET));
    endpointBuilder.withCapabilityConfiguration(m_mockCapabilityConfigurationInterface);
    ASSERT_EQ(endpointBuilder.build(), nullptr);
}

/**
 * Tests @c withCapabilityConfiguration with a valid configuration, expecting to update capability configurations
 * successfully.
 */
TEST_F(EndpointBuilderTest, test_withCapabilityConfigurationSuccess) {
    auto endpointBuilder = createEndpointBuilder();
    EXPECT_CALL(*m_mockCapabilityConfigurationInterface, getCapabilityConfigurations())
        .WillOnce(Return(VALID_CAPABILITY_CONFIGURATION_SET));
    endpointBuilder.withCapabilityConfiguration(m_mockCapabilityConfigurationInterface);
    auto endpoint = endpointBuilder.build();
    ASSERT_FALSE(endpoint->getCapabilityConfigurations().empty());
}

/**
 * Tests @c build with minimal required valid parameters, expecting to successfully build an endpoint.
 */
TEST_F(EndpointBuilderTest, test_buildSuccess) {
    auto endpointBuilder = createEndpointBuilder();
    ASSERT_NE(endpointBuilder.build(), nullptr);
}

/**
 * Tests @c build with an already built endpoint, expecting the build to fail, since only one endpoint can be built from
 * a single endpoint builder. Expecting the second call to create a duplicate endpoint to return @c nullptr.
 */
TEST_F(EndpointBuilderTest, test_buildDuplicate) {
    auto endpointBuilder = createEndpointBuilder();
    auto endpoint = endpointBuilder.build();
    ASSERT_NE(endpoint, nullptr);
    ASSERT_EQ(endpointBuilder.build(), nullptr);
}

}  // namespace test
}  // namespace endpoints
}  // namespace alexaClientSDK