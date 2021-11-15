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

/// @file DefaultEndpointBuilderTest.cpp

#include <gtest/gtest.h>

#include <AVSCommon/SDKInterfaces/MockCapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandler.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <Endpoints/DefaultEndpointBuilder.h>
#include <MockAlexaInterfaceMessageSenderInternal.h>

namespace alexaClientSDK {
namespace endpoints {
namespace test {

static const std::shared_ptr<avsCommon::avs::CapabilityConfiguration> VALID_CAPABILITY_CONFIGURATION =
    std::make_shared<avsCommon::avs::CapabilityConfiguration>("TEST_TYPE", "TEST_INTERFACE_NAME", "2.0");
static const std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>>
    VALID_CAPABILITY_CONFIGURATION_SET{VALID_CAPABILITY_CONFIGURATION};

using namespace acsdkManufactory;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::endpoints;
using namespace avsCommon::sdkInterfaces::test;
using namespace ::testing;

/// Test harness for @c EndpointBuilder class.
class DefaultEndpointBuilderTest : public Test {
public:
    /// Setup the test harness for running the test.
    void SetUp() override;

    /// Clean up the test harness after running the test.
    void TearDown() override;

    /// Create a DefaultEndpointBuilder.
    Annotated<DefaultEndpointAnnotation, EndpointBuilderInterface> createDefaultEndpointBuilder();

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

void DefaultEndpointBuilderTest::SetUp() {
    m_deviceInfo = avsCommon::utils::DeviceInfo::create(
        "TEST_CLIENT_ID", "TEST_PRODUCT_ID", "1234", "TEST_MANUFACTURER_NAME", "TEST_DESCRIPTION");
    m_mockContextManager = std::make_shared<MockContextManager>();
    m_mockExceptionSender = std::make_shared<MockExceptionEncounteredSender>();
    m_mockResponseSender = std::make_shared<capabilityAgents::alexa::test::MockAlexaInterfaceMessageSenderInternal>();
    m_mockCapabilityConfigurationInterface = std::make_shared<MockCapabilityConfigurationInterface>();
    m_mockDirectiveHandler = std::make_shared<MockDirectiveHandler>();
}

void DefaultEndpointBuilderTest::TearDown() {
}

Annotated<DefaultEndpointAnnotation, EndpointBuilderInterface> DefaultEndpointBuilderTest::
    createDefaultEndpointBuilder() {
    return DefaultEndpointBuilder::createDefaultEndpointBuilderInterface(
        m_deviceInfo, m_mockContextManager, m_mockExceptionSender, m_mockResponseSender);
}

/**
 * Tests @c createDefaultEndpointBuilderInterface with valid parameters, expecting to successfully configure and create
 * default endpoint.
 */
TEST_F(DefaultEndpointBuilderTest, test_createDefaultEndpointBuilderInterface) {
    auto defaultEndpointBuilder = createDefaultEndpointBuilder();
    ASSERT_NE(defaultEndpointBuilder->build(), nullptr);
}

/**
 * Tests @c createDefaultEndpointBuilderInterface with null parameters, expecting to return @c nullptr.
 */
TEST_F(DefaultEndpointBuilderTest, test_createDefaultEndpointBuilderInterfaceInvalidBuilder) {
    auto defaultEndpointBuilder =
        DefaultEndpointBuilder::createDefaultEndpointBuilderInterface(nullptr, nullptr, nullptr, nullptr);
    ASSERT_EQ(defaultEndpointBuilder.get(), nullptr);
}

/**
 * Tests that endpoint id, friendly name, description, manufacturer name, and additional attributes cannot be updated
 * after a default endpoint builder is created.
 *
 * @note This test also tests @c withDerivedEndpointId, @c withEndpointId, @c withFriendlyName, @c withDescription,
 * @c withManufacturerName, and @c withAdditionalAttributes.
 */
TEST_F(DefaultEndpointBuilderTest, test_cannotUpdateDefaultAttributes) {
    auto defaultEndpointBuilder = createDefaultEndpointBuilder();
    defaultEndpointBuilder->withDerivedEndpointId("TEST_DERIVED_ENDPOINT_ID")
        .withEndpointId("TEST_ENDPOINT_ID")
        .withFriendlyName("TEST_FRIENDLY_NAME")
        .withDescription("TEST_DESCRIPTION_OVERWRITE")
        .withManufacturerName("TEST_MANUFACTURER_NAME_OVERWRITE")
        .withAdditionalAttributes(
            "TEST_MANUFACTURER_NAME",
            "TEST_MODEL",
            "TEST_SERIAL_NUMBER",
            "TEST_FIRMWARE_VERSION",
            "TEST_SOFTWARE_VERSION",
            "TEST_CUSTOM_IDENTIFIER");

    auto defaultEndpoint = defaultEndpointBuilder->build();
    auto derivedEndpointId = m_deviceInfo->getDefaultEndpointId() + "::" + "TEST_DERIVED_ENDPOINT_ID";
    ASSERT_NE(defaultEndpoint->getEndpointId(), derivedEndpointId);
    ASSERT_NE(defaultEndpoint->getEndpointId(), "TEST_ENDPOINT_ID");
    ASSERT_TRUE(defaultEndpoint->getAttributes().friendlyName.empty());
    ASSERT_NE(defaultEndpoint->getAttributes().description, "TEST_DESCRIPTION_OVERWRITE");
    ASSERT_NE(defaultEndpoint->getAttributes().manufacturerName, "TEST_MANUFACTURER_NAME_OVERWRITE");
    ASSERT_FALSE(defaultEndpoint->getAttributes().additionalAttributes.hasValue());
}

/**
 * Tests @c withDisplayCategory with valid parameters, expecting that display category is successfully updated.
 */
TEST_F(DefaultEndpointBuilderTest, test_withDisplayCategory) {
    auto defaultEndpointBuilder = createDefaultEndpointBuilder();
    defaultEndpointBuilder->withDisplayCategory({"TEST_DISPLAY_CATEGORY"});

    auto defaultEndpoint = defaultEndpointBuilder->build();
    ASSERT_FALSE(defaultEndpoint->getAttributes().displayCategories.empty());
}

/**
 * Tests @c withConnections with valid parameters, expecting that connections are updated successfully.
 */
TEST_F(DefaultEndpointBuilderTest, test_withConnections) {
    std::map<std::string, std::string> connection;
    connection.insert({"testKey", "textValue"});

    auto defaultEndpointBuilder = createDefaultEndpointBuilder();
    defaultEndpointBuilder->withConnections({connection});
    auto defaultEndpoint = defaultEndpointBuilder->build();
    ASSERT_FALSE(defaultEndpoint->getAttributes().connections.empty());
}

/**
 * Tests @c withCookies with valid parameters, expecting that cookies are updated successfully.
 */
TEST_F(DefaultEndpointBuilderTest, test_withCookies) {
    std::map<std::string, std::string> cookie;
    cookie.insert({"testKey", "testValue"});

    auto defaultEndpointBuilder = createDefaultEndpointBuilder();
    defaultEndpointBuilder->withCookies(cookie);
    auto defaultEndpoint = defaultEndpointBuilder->build();
    ASSERT_FALSE(defaultEndpoint->getAttributes().cookies.empty());
}

#ifndef POWER_CONTROLLER
/**
 * Tests @c withPowerController without being enabled, expecting @c build to fail and to return @c nullptr.
 *
 * @note This function is deprecated use the new @c withEndpointCapabilitiesBuilder() method instead.
 */
TEST_F(DefaultEndpointBuilderTest, test_withPowerControllerNotEnabled) {
    auto defaultEndpointBuilder = createDefaultEndpointBuilder();
    auto powerController = std::shared_ptr<powerController::PowerControllerInterface>();
    defaultEndpointBuilder->withPowerController(powerController, true, true);
    ASSERT_EQ(defaultEndpointBuilder->build(), nullptr);
}
#endif

#ifndef TOGGLE_CONTROLLER
/**
 * Tests @c withToggleController without being enabled, expecting @c build to fail and to return @c nullptr.
 *
 * @note This function is deprecated use the new @c withEndpointCapabilitiesBuilder() method instead.
 */
TEST_F(DefaultEndpointBuilderTest, test_withToggleControllerNotEnabled) {
    auto defaultEndpointBuilder = createDefaultEndpointBuilder();
    auto toggleController = std::shared_ptr<toggleController::ToggleControllerInterface>();
    toggleController::ToggleControllerAttributes toggleControllerAttributes;
    defaultEndpointBuilder->withToggleController(
        toggleController, "TEST_INSTANCE", toggleControllerAttributes, true, true, false);
    ASSERT_EQ(defaultEndpointBuilder->build(), nullptr);
}
#endif

#ifndef MODE_CONTROLLER
/**
 * Tests @c withModeController without being enabled, expecting @c build to fail and to return @c nullptr.
 *
 * @note This function is deprecated use the new @c withEndpointCapabilitiesBuilder() method instead.
 */
TEST_F(DefaultEndpointBuilderTest, test_withModeControllerNotEnabled) {
    auto defaultEndpointBuilder = createDefaultEndpointBuilder();
    auto modeController = std::shared_ptr<modeController::ModeControllerInterface>();
    modeController::ModeControllerAttributes modeControllerAttributes;
    defaultEndpointBuilder->withModeController(
        modeController, "TEST_INSTANCE", modeControllerAttributes, true, true, false);
    ASSERT_EQ(defaultEndpointBuilder->build(), nullptr);
}
#endif

#ifndef RANGE_CONTROLLER
/**
 * Tests @c withRangeController without being enabled, expecting @c build to fail and to return @c nullptr.
 *
 * @note This function is deprecated use the new @c withEndpointCapabilitiesBuilder() method instead.
 */
TEST_F(DefaultEndpointBuilderTest, test_withRangeControllerNotEnabled) {
    auto defaultEndpointBuilder = createDefaultEndpointBuilder();
    auto rangeController = std::shared_ptr<rangeController::RangeControllerInterface>();
    rangeController::RangeControllerAttributes rangeControllerAttributes;
    defaultEndpointBuilder->withRangeController(
        rangeController, "TEST_INSTANCE", rangeControllerAttributes, true, true, false);
    ASSERT_EQ(defaultEndpointBuilder->build(), nullptr);
}
#endif

/**
 * Tests @c withCapability with valid parameters, expecting capabilities to be updated successfully.
 */
TEST_F(DefaultEndpointBuilderTest, test_withCapability) {
    auto defaultEndpointBuilder = createDefaultEndpointBuilder();
    defaultEndpointBuilder->withCapability(*VALID_CAPABILITY_CONFIGURATION, m_mockDirectiveHandler);
    auto defaultEndpoint = defaultEndpointBuilder->build();
    ASSERT_FALSE(defaultEndpoint->getCapabilities().empty());
}

/**
 * Tests @c withCapability with valid parameters, expecting capabilities to be updated successfully.
 */
TEST_F(DefaultEndpointBuilderTest, test_withCapabilityInterface) {
    auto defaultEndpointBuilder = createDefaultEndpointBuilder();
    EXPECT_CALL(*m_mockCapabilityConfigurationInterface, getCapabilityConfigurations())
        .WillOnce(Return(VALID_CAPABILITY_CONFIGURATION_SET));
    defaultEndpointBuilder->withCapability(m_mockCapabilityConfigurationInterface, m_mockDirectiveHandler);

    auto defaultEndpoint = defaultEndpointBuilder->build();
    ASSERT_FALSE(defaultEndpoint->getCapabilities().empty());
}

/**
 * Tests @c withCapabilityConfiguration with valid parameters, expecting capabilities to be updated successfully.
 */
TEST_F(DefaultEndpointBuilderTest, test_withCapabilityConfiguration) {
    auto defaultEndpointBuilder = createDefaultEndpointBuilder();
    EXPECT_CALL(*m_mockCapabilityConfigurationInterface, getCapabilityConfigurations())
        .WillOnce(Return(VALID_CAPABILITY_CONFIGURATION_SET));
    defaultEndpointBuilder->withCapabilityConfiguration(m_mockCapabilityConfigurationInterface);

    auto defaultEndpoint = defaultEndpointBuilder->build();
    ASSERT_FALSE(defaultEndpoint->getCapabilityConfigurations().empty());
}

/**
 * Tests @c build with minimal required valid parameters, expecting to successfully build a default endpoint.
 */
TEST_F(DefaultEndpointBuilderTest, test_buildSuccess) {
    auto defaultEndpointBuilder = createDefaultEndpointBuilder();
    ASSERT_NE(defaultEndpointBuilder->build(), nullptr);
}

/**
 * Tests @c build with an already built default endpoint, expecting the build to fail, since only one default endpoint
 * can be built from a single default endpoint builder. Expecting the second call to create a duplicate default endpoint
 * to return @c nullptr.
 */
TEST_F(DefaultEndpointBuilderTest, test_buildDuplicate) {
    auto defaultEndpointBuilder = createDefaultEndpointBuilder();
    auto defaultEndpoint = defaultEndpointBuilder->build();
    ASSERT_NE(defaultEndpoint, nullptr);
    ASSERT_EQ(defaultEndpointBuilder->build(), nullptr);
}

}  // namespace test
}  // namespace endpoints
}  // namespace alexaClientSDK