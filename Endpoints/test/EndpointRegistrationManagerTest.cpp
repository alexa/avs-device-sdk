/*
 * Copyright 2019-2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/AVS/AVSDirective.h>
#include <AVSCommon/AVS/AVSDiscoveryEndpointAttributes.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesObserverInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/MockEndpoint.h>
#include <AVSCommon/SDKInterfaces/Endpoints/MockEndpointRegistrationObserver.h>
#include <AVSCommon/SDKInterfaces/MockCapabilitiesDelegate.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandler.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveSequencer.h>
#include <Endpoints/EndpointRegistrationManager.h>

namespace alexaClientSDK {
namespace endpoints {
namespace test {

using namespace ::testing;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::endpoints;
using namespace avsCommon::sdkInterfaces::endpoints::test;
using namespace avsCommon::sdkInterfaces::test;

// Timeout for tests.
std::chrono::milliseconds TIMEOUT{100};

// Alias for the registration result.
using RegistrationResult = EndpointRegistrationManager::RegistrationResult;

/**
 * Test class that initializes the endpoint registration manager and mock their dependencies.
 */
class EndpointRegistrationManagerTest : public Test {
protected:
    /// The setup method run before every test.
    void SetUp() override;

    /// The tear down method run after every test.
    void TearDown() override;

    std::shared_ptr<StrictMock<MockDirectiveSequencer>> m_sequencer;
    std::shared_ptr<StrictMock<MockCapabilitiesDelegate>> m_capabilitiesDelegate;
    std::shared_ptr<StrictMock<MockEndpointRegistrationObserver>> m_registrationObserver;
    std::shared_ptr<CapabilitiesObserverInterface> m_capabilitiesObserver;
    std::unique_ptr<EndpointRegistrationManager> m_manager;
};

void EndpointRegistrationManagerTest::SetUp() {
    m_sequencer = std::make_shared<StrictMock<MockDirectiveSequencer>>();
    m_capabilitiesDelegate = std::make_shared<StrictMock<MockCapabilitiesDelegate>>();
    m_registrationObserver = std::make_shared<StrictMock<MockEndpointRegistrationObserver>>();

    EXPECT_CALL(*m_capabilitiesDelegate, addCapabilitiesObserver(_))
        .WillOnce(Invoke(
            [this](std::shared_ptr<CapabilitiesObserverInterface> observer) { m_capabilitiesObserver = observer; }));
    m_manager = EndpointRegistrationManager::create(m_sequencer, m_capabilitiesDelegate);
    m_manager->addObserver(m_registrationObserver);

    ASSERT_THAT(m_capabilitiesObserver, NotNull());
}

void EndpointRegistrationManagerTest::TearDown() {
    EXPECT_CALL(*m_capabilitiesDelegate, removeCapabilitiesObserver(_));
    m_manager.reset();
    m_capabilitiesDelegate.reset();

    EXPECT_CALL(*m_sequencer, doShutdown());
    m_sequencer->shutdown();
    m_sequencer.reset();
}

TEST_F(EndpointRegistrationManagerTest, test_createWithNullParametersFails) {
    ASSERT_THAT(EndpointRegistrationManager::create(nullptr, m_capabilitiesDelegate), IsNull());
    ASSERT_THAT(EndpointRegistrationManager::create(m_sequencer, nullptr), IsNull());
}

TEST_F(EndpointRegistrationManagerTest, test_registerEndpointSucceeds) {
    // Configure endpoint object expectations.
    auto endpoint = std::make_shared<MockEndpoint>();
    AVSDiscoveryEndpointAttributes attributes;
    EndpointIdentifier endpointId = "EndpointId";
    CapabilityConfiguration configuration{"Type", "InterfaceName", "1.0"};
    std::vector<CapabilityConfiguration> configurations{{configuration}};
    std::unordered_map<CapabilityConfiguration, std::shared_ptr<DirectiveHandlerInterface>> capabilities;
    std::shared_ptr<DirectiveHandlerInterface> handler = std::make_shared<MockDirectiveHandler>();
    capabilities[configuration] = handler;
    EXPECT_CALL(*endpoint, getCapabilities()).WillRepeatedly(Return(capabilities));
    EXPECT_CALL(*endpoint, getCapabilityConfigurations()).WillRepeatedly(Return(configurations));
    EXPECT_CALL(*endpoint, getAttributes()).WillRepeatedly(Return(attributes));
    EXPECT_CALL(*endpoint, getEndpointId()).WillRepeatedly(Return(endpointId));

    // Expect directive sequencer and capabilities delegate calls.
    EXPECT_CALL(*m_sequencer, addDirectiveHandler(handler)).WillOnce(Return(true));
    EXPECT_CALL(*m_capabilitiesDelegate, registerEndpoint(_, configurations)).WillOnce(Return(true));

    // Check that register endpoint was enqueued.
    auto result = m_manager->registerEndpoint(endpoint);
    ASSERT_EQ(result.wait_for(std::chrono::milliseconds::zero()), std::future_status::timeout);

    // Expect that the observer will be notified that the endpoint was registered.
    EXPECT_CALL(*m_registrationObserver, onEndpointRegistration(endpointId, _, RegistrationResult::SUCCEEDED));

    m_capabilitiesObserver->onCapabilitiesStateChange(
        CapabilitiesObserverInterface::State::SUCCESS, CapabilitiesObserverInterface::Error::SUCCESS);
    ASSERT_EQ(result.wait_for(TIMEOUT), std::future_status::ready);
    EXPECT_EQ(result.get(), RegistrationResult::SUCCEEDED);
}

TEST_F(EndpointRegistrationManagerTest, test_registerEndpointWhenCapabilityRegistrationEndWithFatalFailureFails) {
    // Configure endpoint object expectations.
    auto endpoint = std::make_shared<MockEndpoint>();
    AVSDiscoveryEndpointAttributes attributes;
    EndpointIdentifier endpointId = "EndpointId";
    CapabilityConfiguration configuration{"Type", "InterfaceName", "1.0"};
    std::vector<CapabilityConfiguration> configurations{{configuration}};
    std::unordered_map<CapabilityConfiguration, std::shared_ptr<DirectiveHandlerInterface>> capabilities;
    EXPECT_CALL(*endpoint, getCapabilities()).WillRepeatedly(Return(capabilities));
    EXPECT_CALL(*endpoint, getCapabilityConfigurations()).WillRepeatedly(Return(configurations));
    EXPECT_CALL(*endpoint, getAttributes()).WillRepeatedly(Return(attributes));
    EXPECT_CALL(*endpoint, getEndpointId()).WillRepeatedly(Return(endpointId));

    // Expect directive sequencer and capabilities delegate calls.
    EXPECT_CALL(*m_capabilitiesDelegate, registerEndpoint(_, configurations)).WillOnce(Return(true));

    // Check that register endpoint was enqueued.
    auto result = m_manager->registerEndpoint(endpoint);
    ASSERT_EQ(result.wait_for(std::chrono::milliseconds::zero()), std::future_status::timeout);

    // Expect that the observer will be notified that the endpoint registration has failed.
    EXPECT_CALL(
        *m_registrationObserver, onEndpointRegistration(endpointId, _, RegistrationResult::CONFIGURATION_ERROR));

    m_capabilitiesObserver->onCapabilitiesStateChange(
        CapabilitiesObserverInterface::State::FATAL_ERROR, CapabilitiesObserverInterface::Error::BAD_REQUEST);
    ASSERT_EQ(result.wait_for(TIMEOUT), std::future_status::ready);
    EXPECT_EQ(result.get(), RegistrationResult::CONFIGURATION_ERROR);
}

TEST_F(EndpointRegistrationManagerTest, test_registerNullEndpointFailsImmediately) {
    auto result = m_manager->registerEndpoint(nullptr);
    ASSERT_EQ(result.wait_for(std::chrono::milliseconds(0)), std::future_status::ready);
    EXPECT_EQ(result.get(), RegistrationResult::CONFIGURATION_ERROR);
}

// Disabling unit test according to ACSDK-3414
TEST_F(EndpointRegistrationManagerTest, DISABLED_test_registerDuplicatedEndpointIdFails) {
    // Configure endpoint object expectations.
    auto endpoint = std::make_shared<MockEndpoint>();
    AVSDiscoveryEndpointAttributes attributes;
    EndpointIdentifier endpointId = "EndpointId";
    CapabilityConfiguration configuration{"Type", "InterfaceName", "1.0"};
    std::vector<CapabilityConfiguration> configurations{{configuration}};
    std::unordered_map<CapabilityConfiguration, std::shared_ptr<DirectiveHandlerInterface>> capabilities;
    EXPECT_CALL(*endpoint, getCapabilities()).WillRepeatedly(Return(capabilities));
    EXPECT_CALL(*endpoint, getCapabilityConfigurations()).WillRepeatedly(Return(configurations));
    EXPECT_CALL(*endpoint, getAttributes()).WillRepeatedly(Return(attributes));
    EXPECT_CALL(*endpoint, getEndpointId()).WillRepeatedly(Return(endpointId));

    // Expect directive sequencer and capabilities delegate calls.
    EXPECT_CALL(*m_capabilitiesDelegate, registerEndpoint(_, configurations)).WillOnce(Return(true));

    // Check that register endpoint was enqueued.
    auto result = m_manager->registerEndpoint(endpoint);
    ASSERT_EQ(result.wait_for(TIMEOUT), std::future_status::timeout);

    // Check that the redundant registration fails.
    auto resultDuplicated = m_manager->registerEndpoint(endpoint);
    ASSERT_EQ(resultDuplicated.wait_for(std::chrono::milliseconds::zero()), std::future_status::ready);
    EXPECT_EQ(resultDuplicated.get(), RegistrationResult::PENDING_REGISTRATION);
}

TEST_F(EndpointRegistrationManagerTest, test_registerEndpointWithInvalidHandlerFails) {
    // Configure endpoint object expectations.
    auto endpoint = std::make_shared<MockEndpoint>();
    AVSDiscoveryEndpointAttributes attributes;
    EndpointIdentifier endpointId = "EndpointId";
    CapabilityConfiguration configuration{"Type", "InterfaceName", "1.0"};
    std::vector<CapabilityConfiguration> configurations{{configuration}};
    std::unordered_map<CapabilityConfiguration, std::shared_ptr<DirectiveHandlerInterface>> capabilities;
    std::shared_ptr<DirectiveHandlerInterface> handler = std::make_shared<MockDirectiveHandler>();
    capabilities[configuration] = handler;
    EXPECT_CALL(*endpoint, getCapabilities()).WillRepeatedly(Return(capabilities));
    EXPECT_CALL(*endpoint, getCapabilityConfigurations()).WillRepeatedly(Return(configurations));
    EXPECT_CALL(*endpoint, getAttributes()).WillRepeatedly(Return(attributes));
    EXPECT_CALL(*endpoint, getEndpointId()).WillRepeatedly(Return(endpointId));

    // Expect directive sequencer to fail.
    EXPECT_CALL(*m_sequencer, addDirectiveHandler(handler)).WillOnce(Return(false));

    // Expect that the observer will be notified that the endpoint registration has failed.
    EXPECT_CALL(
        *m_registrationObserver, onEndpointRegistration(endpointId, _, RegistrationResult::CONFIGURATION_ERROR));

    auto result = m_manager->registerEndpoint(endpoint);
    ASSERT_EQ(result.wait_for(TIMEOUT), std::future_status::ready);
    EXPECT_EQ(result.get(), RegistrationResult::CONFIGURATION_ERROR);
}

TEST_F(EndpointRegistrationManagerTest, test_registerEndpointWithInvalidCapabilityFails) {
    // Configure endpoint object expectations.
    auto endpoint = std::make_shared<MockEndpoint>();
    AVSDiscoveryEndpointAttributes attributes;
    EndpointIdentifier endpointId = "EndpointId";
    CapabilityConfiguration configuration{"Type", "InterfaceName", "1.0"};
    std::vector<CapabilityConfiguration> configurations{{configuration}};
    std::unordered_map<CapabilityConfiguration, std::shared_ptr<DirectiveHandlerInterface>> capabilities;
    EXPECT_CALL(*endpoint, getCapabilities()).WillRepeatedly(Return(capabilities));
    EXPECT_CALL(*endpoint, getCapabilityConfigurations()).WillRepeatedly(Return(configurations));
    EXPECT_CALL(*endpoint, getAttributes()).WillRepeatedly(Return(attributes));
    EXPECT_CALL(*endpoint, getEndpointId()).WillRepeatedly(Return(endpointId));

    // Expect directive sequencer and capabilities delegate calls.
    EXPECT_CALL(*m_capabilitiesDelegate, registerEndpoint(_, configurations)).WillOnce(Return(false));

    // Expect that the observer will be notified that the endpoint registration has failed.
    EXPECT_CALL(*m_registrationObserver, onEndpointRegistration(endpointId, _, RegistrationResult::INTERNAL_ERROR));

    auto result = m_manager->registerEndpoint(endpoint);
    ASSERT_EQ(result.wait_for(TIMEOUT), std::future_status::ready);
    EXPECT_EQ(result.get(), RegistrationResult::INTERNAL_ERROR);
}

TEST_F(EndpointRegistrationManagerTest, test_registerEndpointAfterRegistrationDisabledFails) {
    // Configure endpoint object expectations.
    auto endpoint = std::make_shared<MockEndpoint>();
    EndpointIdentifier endpointId = "EndpointId";
    EXPECT_CALL(*endpoint, getEndpointId()).WillRepeatedly(Return(endpointId));

    // Expect that the observer will be notified that the endpoint registration has failed.
    EXPECT_CALL(
        *m_registrationObserver, onEndpointRegistration(endpointId, _, RegistrationResult::REGISTRATION_DISABLED));

    // Call register endpoint after disabling the registration.
    m_manager->disableRegistration();
    auto result = m_manager->registerEndpoint(endpoint);
    ASSERT_EQ(result.wait_for(TIMEOUT), std::future_status::ready);
    EXPECT_EQ(result.get(), RegistrationResult::REGISTRATION_DISABLED);
}

}  // namespace test
}  // namespace endpoints
}  // namespace alexaClientSDK
