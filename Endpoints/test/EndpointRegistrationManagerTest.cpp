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
#include <AVSCommon/Utils/WaitEvent.h>
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

/// Constant representing the timeout for test events.
/// @note Use a large enough value that should not fail even in slower systems.
static const std::chrono::seconds MY_WAIT_TIMEOUT{2};

/// The @c EndpointIdentifier for a mock default endpoint.
static const EndpointIdentifier DEFAULT_ENDPOINT_ID = "defaultEndpointId";

// Alias for the registration result.
using RegistrationResult = EndpointRegistrationManager::RegistrationResult;

// Alias for the deregistration result.
using DeregistrationResult = EndpointRegistrationManager::DeregistrationResult;

/**
 * Test class that initializes the endpoint registration manager and mock their dependencies.
 */
class EndpointRegistrationManagerTest : public Test {
protected:
    /// The setup method run before every test.
    void SetUp() override;

    /// The tear down method run after every test.
    void TearDown() override;

    /*
     * Helper to validate endpoint configuration.
     *
     * @param endpoint The mock endpoint to configure.
     * @param endpointId The endpointId of the endpoint being validated.
     * @param configurations Optional vector of @c CapabilityConfiguration with default test value.
     * @param capabilities Optional map of @c CapabilityConfiguration to @c DirectiveHandlerInterface with default
     * (empty) test value.
     */
    void validateEndpointConfiguration(
        const std::shared_ptr<MockEndpoint>& endpoint,
        EndpointIdentifier endpointId,
        std::vector<CapabilityConfiguration> configurations =
            std::vector<CapabilityConfiguration>{CapabilityConfiguration{"Type", "InterfaceName", "1.0"}},
        std::unordered_map<CapabilityConfiguration, std::shared_ptr<DirectiveHandlerInterface>> capabilities =
            std::unordered_map<CapabilityConfiguration, std::shared_ptr<DirectiveHandlerInterface>>{});

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
    m_manager = EndpointRegistrationManager::create(m_sequencer, m_capabilitiesDelegate, DEFAULT_ENDPOINT_ID);
    m_manager->addObserver(m_registrationObserver);

    ASSERT_THAT(m_capabilitiesObserver, NotNull());

    EXPECT_CALL(*m_capabilitiesDelegate, removeCapabilitiesObserver(_));
    EXPECT_CALL(*m_sequencer, doShutdown());
}

void EndpointRegistrationManagerTest::TearDown() {
    m_manager.reset();
    m_capabilitiesDelegate.reset();

    m_sequencer->shutdown();
    m_sequencer.reset();
}

void EndpointRegistrationManagerTest::validateEndpointConfiguration(
    const std::shared_ptr<MockEndpoint>& endpoint,
    EndpointIdentifier endpointId,
    std::vector<CapabilityConfiguration> configurations,
    std::unordered_map<CapabilityConfiguration, std::shared_ptr<DirectiveHandlerInterface>> capabilities) {
    AVSDiscoveryEndpointAttributes attributes;
    EXPECT_CALL(*endpoint, getCapabilities()).WillRepeatedly(Return(capabilities));
    EXPECT_CALL(*endpoint, getCapabilityConfigurations()).WillRepeatedly(Return(configurations));
    EXPECT_CALL(*endpoint, getAttributes()).WillRepeatedly(Return(attributes));
    EXPECT_CALL(*endpoint, getEndpointId()).WillRepeatedly(Return(endpointId));
}

/*
 * Test create with null parameters fails.
 */
TEST_F(EndpointRegistrationManagerTest, test_createWithNullParametersFails) {
    ASSERT_THAT(EndpointRegistrationManager::create(nullptr, m_capabilitiesDelegate, DEFAULT_ENDPOINT_ID), IsNull());
    ASSERT_THAT(EndpointRegistrationManager::create(m_sequencer, nullptr, DEFAULT_ENDPOINT_ID), IsNull());
    ASSERT_THAT(EndpointRegistrationManager::create(m_sequencer, m_capabilitiesDelegate, ""), IsNull());
}

/*
 * Test shutdown resolves all pending promises.
 */
TEST_F(EndpointRegistrationManagerTest, test_shutdownResolvesPendingPromises) {
    avsCommon::utils::WaitEvent e;

    // Configure endpoint object expectations.
    auto endpoint = std::make_shared<MockEndpoint>();
    EndpointIdentifier endpointId = "EndpointId";
    validateEndpointConfiguration(endpoint, endpointId);

    auto endpointToDelete = std::make_shared<MockEndpoint>();
    EndpointIdentifier endpointIdToDelete = "EndpointIdToDelete";
    validateEndpointConfiguration(endpointToDelete, endpointIdToDelete);

    // Expect observer and capabilities delegate calls.
    EXPECT_CALL(*m_registrationObserver, onEndpointRegistration(_, _, _)).Times(2);
    EXPECT_CALL(*m_capabilitiesDelegate, addOrUpdateEndpoint(_, _)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_registrationObserver, onEndpointDeregistration(endpointIdToDelete, _)).Times(1);
    EXPECT_CALL(*m_capabilitiesDelegate, deleteEndpoint(_, _))
        .WillOnce(Invoke([&](const avsCommon::avs::AVSDiscoveryEndpointAttributes& endpointAttributes,
                             const std::vector<avsCommon::avs::CapabilityConfiguration>& capabilities) {
            e.wakeUp();
            return true;
        }));

    // Successfully add an endpoint so we can test resolving the pending delete on shutdown.
    auto result = m_manager->registerEndpoint(endpointToDelete);
    m_capabilitiesObserver->onCapabilitiesStateChange(
        CapabilitiesObserverInterface::State::SUCCESS,
        CapabilitiesObserverInterface::Error::SUCCESS,
        {endpointIdToDelete},
        {});
    ASSERT_EQ(result.wait_for(MY_WAIT_TIMEOUT), std::future_status::ready);
    EXPECT_EQ(result.get(), RegistrationResult::SUCCEEDED);

    // Check that register endpoint was enqueued.
    auto addResult = m_manager->registerEndpoint(endpoint);
    ASSERT_EQ(addResult.wait_for(std::chrono::milliseconds(0)), std::future_status::timeout);

    // Check that deregister endpoint was enqueued.
    auto deleteResult = m_manager->deregisterEndpoint(endpointIdToDelete);
    ASSERT_EQ(deleteResult.wait_for(std::chrono::milliseconds(0)), std::future_status::timeout);

    ASSERT_TRUE(e.wait(MY_WAIT_TIMEOUT));

    // Test.
    m_manager.reset();
    ASSERT_EQ(addResult.wait_for(MY_WAIT_TIMEOUT), std::future_status::ready);
    EXPECT_EQ(addResult.get(), RegistrationResult::INTERNAL_ERROR);

    ASSERT_EQ(deleteResult.wait_for(MY_WAIT_TIMEOUT), std::future_status::ready);
    EXPECT_EQ(deleteResult.get(), DeregistrationResult::INTERNAL_ERROR);
}

/*
 * Test registering a new endpoint happy path.
 */
TEST_F(EndpointRegistrationManagerTest, test_registerEndpointSucceeds) {
    // Configure endpoint object expectations.
    CapabilityConfiguration configuration{"Type", "InterfaceName", "1.0"};
    std::vector<CapabilityConfiguration> configurations{{configuration}};
    std::unordered_map<CapabilityConfiguration, std::shared_ptr<DirectiveHandlerInterface>> capabilities;
    std::shared_ptr<DirectiveHandlerInterface> handler = std::make_shared<MockDirectiveHandler>();
    capabilities[configuration] = handler;

    auto endpoint = std::make_shared<MockEndpoint>();
    EndpointIdentifier endpointId = "EndpointId";
    validateEndpointConfiguration(endpoint, endpointId, configurations, capabilities);

    // Expect directive sequencer and capabilities delegate calls.
    EXPECT_CALL(*m_sequencer, addDirectiveHandler(handler)).WillOnce(Return(true));
    EXPECT_CALL(*m_capabilitiesDelegate, addOrUpdateEndpoint(_, configurations)).WillOnce(Return(true));

    // Check that register endpoint was enqueued.
    auto result = m_manager->registerEndpoint(endpoint);
    ASSERT_EQ(result.wait_for(std::chrono::milliseconds::zero()), std::future_status::timeout);

    // Expect that the observer will be notified that the endpoint was registered.
    EXPECT_CALL(*m_registrationObserver, onEndpointRegistration(endpointId, _, RegistrationResult::SUCCEEDED));

    m_capabilitiesObserver->onCapabilitiesStateChange(
        CapabilitiesObserverInterface::State::SUCCESS, CapabilitiesObserverInterface::Error::SUCCESS, {endpointId}, {});
    ASSERT_EQ(result.wait_for(MY_WAIT_TIMEOUT), std::future_status::ready);
    EXPECT_EQ(result.get(), RegistrationResult::SUCCEEDED);
}

/*
 * Test deregistering an endpoint happy path.
 */
TEST_F(EndpointRegistrationManagerTest, test_deregisterEndpointSucceeds) {
    // Configure endpoint object expectations.
    auto endpoint = std::make_shared<MockEndpoint>();
    EndpointIdentifier endpointId = "EndpointId";
    CapabilityConfiguration configuration{"Type", "InterfaceName", "1.0"};
    std::vector<CapabilityConfiguration> configurations{{configuration}};
    std::unordered_map<CapabilityConfiguration, std::shared_ptr<DirectiveHandlerInterface>> capabilities;
    std::shared_ptr<DirectiveHandlerInterface> handler = std::make_shared<MockDirectiveHandler>();
    capabilities[configuration] = handler;
    validateEndpointConfiguration(endpoint, endpointId, configurations, capabilities);

    // Set-up calls to capabilities delegate and sequencer.
    EXPECT_CALL(*m_sequencer, addDirectiveHandler(handler)).WillOnce(Return(true));
    EXPECT_CALL(*m_sequencer, removeDirectiveHandler(handler)).WillOnce(Return(true));
    EXPECT_CALL(*m_capabilitiesDelegate, addOrUpdateEndpoint(_, configurations)).WillOnce(Return(true));
    EXPECT_CALL(*m_capabilitiesDelegate, deleteEndpoint(_, configurations)).WillOnce(Return(true));

    // Add an endpoint so we can test delete.
    auto addResult = m_manager->registerEndpoint(endpoint);

    EXPECT_CALL(*m_registrationObserver, onEndpointRegistration(endpointId, _, RegistrationResult::SUCCEEDED));
    m_capabilitiesObserver->onCapabilitiesStateChange(
        CapabilitiesObserverInterface::State::SUCCESS, CapabilitiesObserverInterface::Error::SUCCESS, {endpointId}, {});

    ASSERT_EQ(addResult.wait_for(MY_WAIT_TIMEOUT), std::future_status::ready);
    EXPECT_EQ(addResult.get(), RegistrationResult::SUCCEEDED);

    // Test delete.
    auto deleteResult = m_manager->deregisterEndpoint(endpointId);
    EXPECT_CALL(*m_registrationObserver, onEndpointDeregistration(endpointId, DeregistrationResult::SUCCEEDED));
    m_capabilitiesObserver->onCapabilitiesStateChange(
        CapabilitiesObserverInterface::State::SUCCESS, CapabilitiesObserverInterface::Error::SUCCESS, {}, {endpointId});
    ASSERT_EQ(deleteResult.wait_for(MY_WAIT_TIMEOUT), std::future_status::ready);
    EXPECT_EQ(deleteResult.get(), DeregistrationResult::SUCCEEDED);
}

/*
 * Test modifying the default endpoint fails.
 */
TEST_F(EndpointRegistrationManagerTest, test_modifyDefaultEndpointFails) {
    // Configure endpoint object expectations.
    auto defaultEndpoint = std::make_shared<MockEndpoint>();
    CapabilityConfiguration configuration{"Type", "InterfaceName", "1.0"};
    std::vector<CapabilityConfiguration> configurations{{configuration}};
    std::unordered_map<CapabilityConfiguration, std::shared_ptr<DirectiveHandlerInterface>> capabilities;
    std::shared_ptr<DirectiveHandlerInterface> handler = std::make_shared<MockDirectiveHandler>();
    capabilities[configuration] = handler;
    validateEndpointConfiguration(defaultEndpoint, DEFAULT_ENDPOINT_ID, configurations, capabilities);

    auto updatedDefaultEndpoint = std::make_shared<MockEndpoint>();
    CapabilityConfiguration updatedConfiguration{"Type", "InterfaceName", "2.0"};
    std::vector<CapabilityConfiguration> updatedConfigurations{{updatedConfiguration}};
    std::unordered_map<CapabilityConfiguration, std::shared_ptr<DirectiveHandlerInterface>> updatedCapabilities;
    std::shared_ptr<DirectiveHandlerInterface> updatedHandler = std::make_shared<MockDirectiveHandler>();
    updatedCapabilities[updatedConfiguration] = updatedHandler;
    validateEndpointConfiguration(
        updatedDefaultEndpoint, DEFAULT_ENDPOINT_ID, updatedConfigurations, updatedCapabilities);

    // Expect calls for adding default endpoint.
    EXPECT_CALL(*m_sequencer, addDirectiveHandler(handler)).WillOnce(Return(true));
    EXPECT_CALL(*m_capabilitiesDelegate, addOrUpdateEndpoint(_, configurations)).WillOnce(Return(true));
    EXPECT_CALL(*m_registrationObserver, onEndpointRegistration(DEFAULT_ENDPOINT_ID, _, RegistrationResult::SUCCEEDED));

    // Check that register default endpoint was enqueued.
    auto result = m_manager->registerEndpoint(defaultEndpoint);
    ASSERT_EQ(result.wait_for(std::chrono::milliseconds::zero()), std::future_status::timeout);

    m_capabilitiesObserver->onCapabilitiesStateChange(
        CapabilitiesObserverInterface::State::SUCCESS,
        CapabilitiesObserverInterface::Error::SUCCESS,
        {DEFAULT_ENDPOINT_ID},
        {});
    ASSERT_EQ(result.wait_for(MY_WAIT_TIMEOUT), std::future_status::ready);
    EXPECT_EQ(result.get(), RegistrationResult::SUCCEEDED);

    // Check that updating the endpoint was enqueued.
    auto updateResult = m_manager->registerEndpoint(updatedDefaultEndpoint);
    ASSERT_EQ(updateResult.wait_for(std::chrono::milliseconds::zero()), std::future_status::ready);
    ASSERT_EQ(updateResult.get(), RegistrationResult::CONFIGURATION_ERROR);
}

/*
 * Test deregistering the default endpoint fails.
 */
TEST_F(EndpointRegistrationManagerTest, test_deregisterDefaultEndpointFails) {
    // Check that deregistering the endpoint fails.
    auto updateResult = m_manager->deregisterEndpoint(DEFAULT_ENDPOINT_ID);
    ASSERT_EQ(updateResult.wait_for(std::chrono::milliseconds::zero()), std::future_status::ready);
    ASSERT_EQ(updateResult.get(), DeregistrationResult::CONFIGURATION_ERROR);
}

/*
 * Test registering an endpoint fails when capability registration fails.
 */
TEST_F(EndpointRegistrationManagerTest, test_registerEndpointWhenCapabilityRegistrationEndsWithFatalErrorFails) {
    // Configure endpoint object expectations.
    auto endpoint = std::make_shared<MockEndpoint>();
    EndpointIdentifier endpointId = "EndpointId";
    CapabilityConfiguration configuration{"Type", "InterfaceName", "1.0"};
    std::vector<CapabilityConfiguration> configurations{{configuration}};
    std::unordered_map<CapabilityConfiguration, std::shared_ptr<DirectiveHandlerInterface>> capabilities;
    std::shared_ptr<DirectiveHandlerInterface> handler = std::make_shared<MockDirectiveHandler>();
    capabilities[configuration] = handler;
    validateEndpointConfiguration(endpoint, endpointId, configurations, capabilities);

    // Expect directive sequencer and capabilities delegate calls.
    EXPECT_CALL(*m_sequencer, addDirectiveHandler(handler)).WillOnce(Return(true));
    EXPECT_CALL(*m_sequencer, removeDirectiveHandler(handler)).WillOnce(Return(true));
    EXPECT_CALL(*m_capabilitiesDelegate, addOrUpdateEndpoint(_, configurations)).WillOnce(Return(true));

    // Check that register endpoint was enqueued.
    auto result = m_manager->registerEndpoint(endpoint);
    ASSERT_EQ(result.wait_for(std::chrono::milliseconds::zero()), std::future_status::timeout);

    // Expect that the observer will be notified that the endpoint was registered.
    EXPECT_CALL(
        *m_registrationObserver, onEndpointRegistration(endpointId, _, RegistrationResult::CONFIGURATION_ERROR));

    m_capabilitiesObserver->onCapabilitiesStateChange(
        CapabilitiesObserverInterface::State::FATAL_ERROR,
        CapabilitiesObserverInterface::Error::UNKNOWN_ERROR,
        {endpointId},
        {});
    ASSERT_EQ(result.wait_for(MY_WAIT_TIMEOUT), std::future_status::ready);
    EXPECT_EQ(result.get(), RegistrationResult::CONFIGURATION_ERROR);
}

/*
 * Test registering a null endpoint fails immediately.
 */
TEST_F(EndpointRegistrationManagerTest, test_registerNullEndpointFailsImmediately) {
    auto result = m_manager->registerEndpoint(nullptr);
    ASSERT_EQ(result.wait_for(std::chrono::milliseconds(0)), std::future_status::ready);
    EXPECT_EQ(result.get(), RegistrationResult::CONFIGURATION_ERROR);
}

/*
 * Test deregistering an endpoint fails immediately if the endpoint is not registered.
 */
TEST_F(EndpointRegistrationManagerTest, test_deregisterEndpointThatDoesNotExistFailsImmediately) {
    auto result = m_manager->deregisterEndpoint("endpointId");
    ASSERT_EQ(result.wait_for(std::chrono::milliseconds(0)), std::future_status::ready);
    EXPECT_EQ(result.get(), DeregistrationResult::NOT_REGISTERED);
}

/*
 * Test registering an endpoint fails while registration for the endpoint is in-progress.
 */
TEST_F(EndpointRegistrationManagerTest, test_registerEndpointWhileRegistrationInProgressFails) {
    avsCommon::utils::WaitEvent e;

    // Configure endpoint object expectations.
    auto endpoint = std::make_shared<MockEndpoint>();
    EndpointIdentifier endpointId = "EndpointId";
    validateEndpointConfiguration(endpoint, endpointId);

    // Expect observer and capabilities delegate calls.
    EXPECT_CALL(*m_registrationObserver, onEndpointRegistration(_, _, _)).Times(1);
    EXPECT_CALL(*m_capabilitiesDelegate, addOrUpdateEndpoint(_, _))
        .WillOnce(Invoke([&](const AVSDiscoveryEndpointAttributes& endpointAttributes,
                             const std::vector<CapabilityConfiguration>& capabilities) {
            e.wakeUp();
            return true;
        }));

    // Check that register endpoint was enqueued.
    auto result = m_manager->registerEndpoint(endpoint);
    ASSERT_EQ(result.wait_for(std::chrono::milliseconds::zero()), std::future_status::timeout);

    // Check that the redundant registration fails.
    auto resultDuplicated = m_manager->registerEndpoint(endpoint);
    ASSERT_EQ(resultDuplicated.wait_for(std::chrono::milliseconds::zero()), std::future_status::ready);
    EXPECT_EQ(resultDuplicated.get(), RegistrationResult::REGISTRATION_IN_PROGRESS);

    ASSERT_TRUE(e.wait(MY_WAIT_TIMEOUT));
}

/*
 * Test registering an endpoint fails while deregistration for the endpoint is in-progress.
 */
TEST_F(EndpointRegistrationManagerTest, test_registerEndpointWhileDeregistrationInProgressFails) {
    avsCommon::utils::WaitEvent e;

    // Configure endpoint object expectations.
    auto endpoint = std::make_shared<MockEndpoint>();
    EndpointIdentifier endpointId = "EndpointId";
    CapabilityConfiguration configuration{"Type", "InterfaceName", "1.0"};
    std::vector<CapabilityConfiguration> configurations{{configuration}};
    validateEndpointConfiguration(endpoint, endpointId, configurations);

    // Expect observer and capabilities delegate calls.
    EXPECT_CALL(*m_registrationObserver, onEndpointRegistration(endpointId, _, RegistrationResult::SUCCEEDED)).Times(1);
    EXPECT_CALL(*m_capabilitiesDelegate, addOrUpdateEndpoint(_, configurations)).WillOnce(Return(true));
    EXPECT_CALL(*m_capabilitiesDelegate, deleteEndpoint(_, configurations))
        .WillOnce(Invoke([&](const AVSDiscoveryEndpointAttributes& endpointAttributes,
                             const std::vector<CapabilityConfiguration>& capabilities) {
            e.wakeUp();
            return true;
        }));

    // Check that register endpoint succeeded.
    auto result = m_manager->registerEndpoint(endpoint);
    m_capabilitiesObserver->onCapabilitiesStateChange(
        CapabilitiesObserverInterface::State::SUCCESS, CapabilitiesObserverInterface::Error::SUCCESS, {endpointId}, {});
    ASSERT_EQ(result.wait_for(MY_WAIT_TIMEOUT), std::future_status::ready);
    ASSERT_EQ(result.get(), RegistrationResult::SUCCEEDED);

    // Check that deregister endpoint enqueued.
    auto deleteResult = m_manager->deregisterEndpoint(endpointId);
    EXPECT_CALL(*m_registrationObserver, onEndpointDeregistration(endpointId, _)).Times(1);
    ASSERT_EQ(deleteResult.wait_for(std::chrono::milliseconds::zero()), std::future_status::timeout);

    // Check that the registration fails.
    auto resultDuplicated = m_manager->registerEndpoint(endpoint);
    ASSERT_EQ(resultDuplicated.wait_for(std::chrono::milliseconds::zero()), std::future_status::ready);
    EXPECT_EQ(resultDuplicated.get(), RegistrationResult::PENDING_DEREGISTRATION);

    ASSERT_TRUE(e.wait(MY_WAIT_TIMEOUT));
}

/*
 * Test deregistering an endpoint fails while deregistration for the endpoint is in-progress.
 */
TEST_F(EndpointRegistrationManagerTest, test_deregisterEndpointWhileDeregistrationInProgressFails) {
    avsCommon::utils::WaitEvent e;

    // Configure endpoint object expectations.
    auto endpoint = std::make_shared<MockEndpoint>();
    EndpointIdentifier endpointId = "EndpointId";
    validateEndpointConfiguration(endpoint, endpointId);

    // Expect observer and capabilities delegate calls.
    EXPECT_CALL(*m_registrationObserver, onEndpointRegistration(endpointId, _, RegistrationResult::SUCCEEDED)).Times(1);
    EXPECT_CALL(*m_capabilitiesDelegate, addOrUpdateEndpoint(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*m_capabilitiesDelegate, deleteEndpoint(_, _))
        .WillOnce(Invoke([&](const AVSDiscoveryEndpointAttributes& endpointAttributes,
                             const std::vector<CapabilityConfiguration>& capabilities) {
            e.wakeUp();
            return true;
        }));

    // Check that register endpoint succeeded.
    auto result = m_manager->registerEndpoint(endpoint);
    m_capabilitiesObserver->onCapabilitiesStateChange(
        CapabilitiesObserverInterface::State::SUCCESS, CapabilitiesObserverInterface::Error::SUCCESS, {endpointId}, {});
    ASSERT_EQ(result.wait_for(MY_WAIT_TIMEOUT), std::future_status::ready);
    ASSERT_EQ(result.get(), RegistrationResult::SUCCEEDED);

    // Check that deregister endpoint enqueued.
    auto deleteResult = m_manager->deregisterEndpoint(endpointId);
    EXPECT_CALL(*m_registrationObserver, onEndpointDeregistration(endpointId, _)).Times(1);
    ASSERT_EQ(deleteResult.wait_for(std::chrono::milliseconds::zero()), std::future_status::timeout);

    // Check that the redundant deregistration fails.
    auto resultDuplicated = m_manager->deregisterEndpoint(endpointId);
    ASSERT_EQ(resultDuplicated.wait_for(std::chrono::milliseconds::zero()), std::future_status::ready);
    EXPECT_EQ(resultDuplicated.get(), DeregistrationResult::PENDING_DEREGISTRATION);

    ASSERT_TRUE(e.wait(MY_WAIT_TIMEOUT));
}

/*
 * Test deregistering an endpoint fails while registration for the endpoint is in-progress.
 */
TEST_F(EndpointRegistrationManagerTest, test_deregisterEndpointWhileRegistrationInProgressFails) {
    avsCommon::utils::WaitEvent e;

    // Configure endpoint object expectations.
    auto endpoint = std::make_shared<MockEndpoint>();
    EndpointIdentifier endpointId = "EndpointId";
    validateEndpointConfiguration(endpoint, endpointId);

    EXPECT_CALL(*m_registrationObserver, onEndpointRegistration(_, _, _));
    EXPECT_CALL(*m_capabilitiesDelegate, addOrUpdateEndpoint(_, _))
        .WillOnce(Invoke(
            [&](const AVSDiscoveryEndpointAttributes& endpointAttributes,
                const std::vector<CapabilityConfiguration>& capabilities) -> bool {
                e.wakeUp();
                return true;
            }));

    auto addResult = m_manager->registerEndpoint(endpoint);
    ASSERT_EQ(addResult.wait_for(std::chrono::milliseconds::zero()), std::future_status::timeout);

    /// Test.
    auto resultDuplicated = m_manager->deregisterEndpoint(endpointId);
    EXPECT_EQ(resultDuplicated.wait_for(std::chrono::milliseconds::zero()), std::future_status::ready);
    EXPECT_EQ(resultDuplicated.get(), DeregistrationResult::REGISTRATION_IN_PROGRESS);

    e.wait(std::chrono::seconds(MY_WAIT_TIMEOUT));
}

/*
 * Test registering a new endpoint fails with invalid handler.
 */
TEST_F(EndpointRegistrationManagerTest, test_registerEndpointWithInvalidHandlerFails) {
    // Configure endpoint object expectations.
    auto endpoint = std::make_shared<MockEndpoint>();
    EndpointIdentifier endpointId = "EndpointId";
    CapabilityConfiguration configuration{"Type", "InterfaceName", "1.0"};
    std::vector<CapabilityConfiguration> configurations{{configuration}};
    std::unordered_map<CapabilityConfiguration, std::shared_ptr<DirectiveHandlerInterface>> capabilities;
    std::shared_ptr<DirectiveHandlerInterface> handler = std::make_shared<MockDirectiveHandler>();
    capabilities[configuration] = handler;
    validateEndpointConfiguration(endpoint, endpointId, configurations, capabilities);

    // Expect directive sequencer to fail.
    EXPECT_CALL(*m_sequencer, addDirectiveHandler(handler)).WillOnce(Return(false));

    // Expect that the observer will be notified that the endpoint registration has failed.
    EXPECT_CALL(
        *m_registrationObserver, onEndpointRegistration(endpointId, _, RegistrationResult::CONFIGURATION_ERROR));

    auto result = m_manager->registerEndpoint(endpoint);
    ASSERT_EQ(result.wait_for(MY_WAIT_TIMEOUT), std::future_status::ready);
    EXPECT_EQ(result.get(), RegistrationResult::CONFIGURATION_ERROR);
}

/*
 * Test registering a new endpoint fails with invalid capability.
 */
TEST_F(EndpointRegistrationManagerTest, test_registerEndpointWithInvalidCapabilityFails) {
    // Configure endpoint object expectations.
    auto endpoint = std::make_shared<MockEndpoint>();
    EndpointIdentifier endpointId = "EndpointId";
    validateEndpointConfiguration(endpoint, endpointId);

    // Expect directive sequencer and capabilities delegate calls.
    EXPECT_CALL(*m_capabilitiesDelegate, addOrUpdateEndpoint(_, _)).WillOnce(Return(false));

    // Expect that the observer will be notified that the endpoint registration has failed.
    EXPECT_CALL(*m_registrationObserver, onEndpointRegistration(endpointId, _, RegistrationResult::INTERNAL_ERROR));

    auto result = m_manager->registerEndpoint(endpoint);
    ASSERT_EQ(result.wait_for(MY_WAIT_TIMEOUT), std::future_status::ready);
    EXPECT_EQ(result.get(), RegistrationResult::INTERNAL_ERROR);
}

/*
 * Test registering an existing endpoint happy path.
 */
TEST_F(EndpointRegistrationManagerTest, test_registerExistingEndpointSucceeds) {
    // Configure endpoint object expectations.
    auto endpoint = std::make_shared<MockEndpoint>();
    EndpointIdentifier endpointId = "EndpointId";
    CapabilityConfiguration configuration{"Type", "InterfaceName", "1.0"};
    std::vector<CapabilityConfiguration> configurations{{configuration}};
    std::unordered_map<CapabilityConfiguration, std::shared_ptr<DirectiveHandlerInterface>> capabilities;
    std::shared_ptr<DirectiveHandlerInterface> handler = std::make_shared<MockDirectiveHandler>();
    capabilities[configuration] = handler;
    validateEndpointConfiguration(endpoint, endpointId, configurations, capabilities);

    auto updatedEndpoint = std::make_shared<MockEndpoint>();
    CapabilityConfiguration updatedConfiguration{"Type", "InterfaceName", "2.0"};
    std::vector<CapabilityConfiguration> updatedConfigurations{{updatedConfiguration}};
    std::unordered_map<CapabilityConfiguration, std::shared_ptr<DirectiveHandlerInterface>> updatedCapabilities;
    std::shared_ptr<DirectiveHandlerInterface> updatedHandler = std::make_shared<MockDirectiveHandler>();
    updatedCapabilities[updatedConfiguration] = updatedHandler;
    validateEndpointConfiguration(updatedEndpoint, endpointId, updatedConfigurations, updatedCapabilities);

    // Expect directive sequencer and capabilities delegate calls for adding original endpoint.
    EXPECT_CALL(*m_sequencer, addDirectiveHandler(handler)).WillOnce(Return(true));
    EXPECT_CALL(*m_capabilitiesDelegate, addOrUpdateEndpoint(_, configurations)).WillOnce(Return(true));

    // Expect directive sequencer and capabilities delegate calls for updating the endpoint.
    EXPECT_CALL(*m_sequencer, removeDirectiveHandler(handler)).WillOnce(Return(true));
    EXPECT_CALL(*m_sequencer, addDirectiveHandler(updatedHandler)).WillOnce(Return(true));
    EXPECT_CALL(*m_capabilitiesDelegate, addOrUpdateEndpoint(_, updatedConfigurations)).WillOnce(Return(true));

    // Expect that the observer will be notified that the endpoint was registered twice: adding it, then updating.
    EXPECT_CALL(*m_registrationObserver, onEndpointRegistration(endpointId, _, RegistrationResult::SUCCEEDED)).Times(2);

    // Check that register endpoint was enqueued.
    auto result = m_manager->registerEndpoint(endpoint);
    ASSERT_EQ(result.wait_for(std::chrono::milliseconds::zero()), std::future_status::timeout);

    m_capabilitiesObserver->onCapabilitiesStateChange(
        CapabilitiesObserverInterface::State::SUCCESS, CapabilitiesObserverInterface::Error::SUCCESS, {endpointId}, {});
    ASSERT_EQ(result.wait_for(MY_WAIT_TIMEOUT), std::future_status::ready);
    EXPECT_EQ(result.get(), RegistrationResult::SUCCEEDED);

    // Check that updating the endpoint was enqueued.
    auto updateResult = m_manager->registerEndpoint(updatedEndpoint);
    ASSERT_EQ(updateResult.wait_for(std::chrono::milliseconds::zero()), std::future_status::timeout);

    m_capabilitiesObserver->onCapabilitiesStateChange(
        CapabilitiesObserverInterface::State::SUCCESS, CapabilitiesObserverInterface::Error::SUCCESS, {endpointId}, {});
    ASSERT_EQ(updateResult.wait_for(MY_WAIT_TIMEOUT), std::future_status::ready);
    EXPECT_EQ(updateResult.get(), RegistrationResult::SUCCEEDED);
}

/*
 * Test registering an existing endpoint. If it fails (due to capability registration failure), the original endpoint
 * should be restored and registering the endpoint should fail.
 */
TEST_F(
    EndpointRegistrationManagerTest,
    test_revertWhenRegisterExistingEndpointFailsDueToCapabilityRegistrationEndWithFatalFailure) {
    // Configure endpoint object expectations.
    auto endpoint = std::make_shared<MockEndpoint>();
    EndpointIdentifier endpointId = "EndpointId";
    CapabilityConfiguration configuration{"Type", "InterfaceName", "1.0"};
    std::vector<CapabilityConfiguration> configurations{{configuration}};
    std::unordered_map<CapabilityConfiguration, std::shared_ptr<DirectiveHandlerInterface>> capabilities;
    std::shared_ptr<DirectiveHandlerInterface> handler = std::make_shared<MockDirectiveHandler>();
    capabilities[configuration] = handler;
    validateEndpointConfiguration(endpoint, endpointId, configurations, capabilities);

    auto updatedEndpoint = std::make_shared<MockEndpoint>();
    CapabilityConfiguration updatedConfiguration{"Type", "InterfaceName", "2.0"};
    std::vector<CapabilityConfiguration> updatedConfigurations{{updatedConfiguration}};
    std::unordered_map<CapabilityConfiguration, std::shared_ptr<DirectiveHandlerInterface>> updatedCapabilities;
    std::shared_ptr<DirectiveHandlerInterface> updatedHandler = std::make_shared<MockDirectiveHandler>();
    updatedCapabilities[updatedConfiguration] = updatedHandler;
    validateEndpointConfiguration(updatedEndpoint, endpointId, updatedConfigurations, updatedCapabilities);

    // Expect directive sequencer and capabilities delegate calls for adding and then restoring original endpoint.
    EXPECT_CALL(*m_sequencer, addDirectiveHandler(handler)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_capabilitiesDelegate, addOrUpdateEndpoint(_, configurations)).WillOnce(Return(true));

    // Expect directive sequencer and capabilities delegate calls for updating the endpoint.
    EXPECT_CALL(*m_sequencer, removeDirectiveHandler(handler)).WillOnce(Return(true));
    EXPECT_CALL(*m_sequencer, addDirectiveHandler(updatedHandler)).WillOnce(Return(true));
    EXPECT_CALL(*m_capabilitiesDelegate, addOrUpdateEndpoint(_, updatedConfigurations)).WillOnce(Return(true));

    // Expect directive sequencer and capabilities delegate calls after updating the endpoint fails.
    EXPECT_CALL(*m_sequencer, removeDirectiveHandler(updatedHandler)).WillOnce(Return(true));

    // Expect that the observer will be notified that the endpoint was registered twice: adding it, then updating.
    EXPECT_CALL(*m_registrationObserver, onEndpointRegistration(endpointId, _, RegistrationResult::SUCCEEDED)).Times(1);
    EXPECT_CALL(*m_registrationObserver, onEndpointRegistration(endpointId, _, RegistrationResult::CONFIGURATION_ERROR))
        .Times(1);

    // Check that register endpoint was enqueued.
    auto result = m_manager->registerEndpoint(endpoint);
    ASSERT_EQ(result.wait_for(std::chrono::milliseconds::zero()), std::future_status::timeout);

    m_capabilitiesObserver->onCapabilitiesStateChange(
        CapabilitiesObserverInterface::State::SUCCESS, CapabilitiesObserverInterface::Error::SUCCESS, {endpointId}, {});
    ASSERT_EQ(result.wait_for(MY_WAIT_TIMEOUT), std::future_status::ready);
    EXPECT_EQ(result.get(), RegistrationResult::SUCCEEDED);

    // Check that updating the endpoint was enqueued.
    auto updateResult = m_manager->registerEndpoint(updatedEndpoint);
    ASSERT_EQ(updateResult.wait_for(std::chrono::milliseconds::zero()), std::future_status::timeout);

    // Fail the update.
    m_capabilitiesObserver->onCapabilitiesStateChange(
        CapabilitiesObserverInterface::State::FATAL_ERROR,
        CapabilitiesObserverInterface::Error::UNKNOWN_ERROR,
        {endpointId},
        {});
    ASSERT_EQ(updateResult.wait_for(MY_WAIT_TIMEOUT), std::future_status::ready);
    EXPECT_EQ(updateResult.get(), RegistrationResult::CONFIGURATION_ERROR);
}

/*
 * Test registering an existing endpoint. If it fails (due to directive handler failure), the original endpoint should
 * be restored and registering the endpoint should fail.
 */
TEST_F(EndpointRegistrationManagerTest, test_revertWhenRegisterExistingEndpointFailsDueToDirectiveHandlerFailure) {
    // Configure endpoint object expectations.
    auto endpoint = std::make_shared<MockEndpoint>();
    EndpointIdentifier endpointId = "EndpointId";
    CapabilityConfiguration configuration{"Type", "InterfaceName", "1.0"};
    std::vector<CapabilityConfiguration> configurations{{configuration}};
    std::unordered_map<CapabilityConfiguration, std::shared_ptr<DirectiveHandlerInterface>> capabilities;
    std::shared_ptr<DirectiveHandlerInterface> handler = std::make_shared<MockDirectiveHandler>();
    capabilities[configuration] = handler;
    validateEndpointConfiguration(endpoint, endpointId, configurations, capabilities);

    auto updatedEndpoint = std::make_shared<MockEndpoint>();
    CapabilityConfiguration updatedConfiguration{"Type", "InterfaceName", "2.0"};
    std::vector<CapabilityConfiguration> updatedConfigurations{{updatedConfiguration}};
    std::unordered_map<CapabilityConfiguration, std::shared_ptr<DirectiveHandlerInterface>> updatedCapabilities;
    std::shared_ptr<DirectiveHandlerInterface> updatedHandler = std::make_shared<MockDirectiveHandler>();
    updatedCapabilities[updatedConfiguration] = updatedHandler;
    validateEndpointConfiguration(updatedEndpoint, endpointId, updatedConfigurations, updatedCapabilities);

    // Expect directive sequencer and capabilities delegate calls for adding and then restoring original endpoint.
    EXPECT_CALL(*m_sequencer, addDirectiveHandler(handler)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_capabilitiesDelegate, addOrUpdateEndpoint(_, configurations)).WillOnce(Return(true));

    // Expect directive sequencer and capabilities delegate calls for updating the endpoint.
    EXPECT_CALL(*m_sequencer, removeDirectiveHandler(handler)).WillOnce(Return(true));
    EXPECT_CALL(*m_sequencer, addDirectiveHandler(updatedHandler)).WillOnce(Return(false));
    EXPECT_CALL(*m_capabilitiesDelegate, addOrUpdateEndpoint(_, updatedConfigurations)).Times(0);

    // Expect that the observer will be notified that the endpoint was registered twice: adding it, then updating.
    EXPECT_CALL(*m_registrationObserver, onEndpointRegistration(endpointId, _, RegistrationResult::SUCCEEDED)).Times(1);
    EXPECT_CALL(*m_registrationObserver, onEndpointRegistration(endpointId, _, RegistrationResult::CONFIGURATION_ERROR))
        .Times(1);

    // Check that register endpoint was enqueued.
    auto result = m_manager->registerEndpoint(endpoint);
    ASSERT_EQ(result.wait_for(std::chrono::milliseconds::zero()), std::future_status::timeout);

    m_capabilitiesObserver->onCapabilitiesStateChange(
        CapabilitiesObserverInterface::State::SUCCESS, CapabilitiesObserverInterface::Error::SUCCESS, {endpointId}, {});
    ASSERT_EQ(result.wait_for(MY_WAIT_TIMEOUT), std::future_status::ready);
    EXPECT_EQ(result.get(), RegistrationResult::SUCCEEDED);

    // Check that updating the endpoint failed.
    auto updateResult = m_manager->registerEndpoint(updatedEndpoint);
    ASSERT_EQ(updateResult.wait_for(MY_WAIT_TIMEOUT), std::future_status::ready);
    EXPECT_EQ(updateResult.get(), RegistrationResult::CONFIGURATION_ERROR);
}

/*
 * Test deregistering an endpoint. If it fails (due to capability registration failure), the original endpoint should be
 * restored and deregistering the endpoint should fail.
 */
TEST_F(
    EndpointRegistrationManagerTest,
    test_revertWhenDeregisterEndpointFailsDueToCapabilityRegistrationEndWithFatalFailure) {
    // Configure endpoint object expectations.
    auto endpoint = std::make_shared<MockEndpoint>();
    EndpointIdentifier endpointId = "EndpointId";
    CapabilityConfiguration configuration{"Type", "InterfaceName", "1.0"};
    std::vector<CapabilityConfiguration> configurations{{configuration}};
    std::unordered_map<CapabilityConfiguration, std::shared_ptr<DirectiveHandlerInterface>> capabilities;
    std::shared_ptr<DirectiveHandlerInterface> handler = std::make_shared<MockDirectiveHandler>();
    capabilities[configuration] = handler;
    validateEndpointConfiguration(endpoint, endpointId, configurations, capabilities);

    // Expect directive sequencer and capabilities delegate calls for adding and then restoring original endpoint.
    EXPECT_CALL(*m_sequencer, addDirectiveHandler(handler)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_capabilitiesDelegate, addOrUpdateEndpoint(_, configurations)).WillOnce(Return(true));

    // Expect directive sequencer and capabilities delegate calls for updating the endpoint.
    EXPECT_CALL(*m_sequencer, removeDirectiveHandler(handler)).WillOnce(Return(true));
    EXPECT_CALL(*m_capabilitiesDelegate, deleteEndpoint(_, configurations)).WillOnce(Return(true));

    // Expect that the observer will be notified that the endpoint was registered twice: adding it, then updating.
    EXPECT_CALL(*m_registrationObserver, onEndpointRegistration(endpointId, _, RegistrationResult::SUCCEEDED)).Times(1);
    EXPECT_CALL(
        *m_registrationObserver, onEndpointDeregistration(endpointId, DeregistrationResult::CONFIGURATION_ERROR))
        .Times(1);

    // Check that register endpoint was enqueued.
    auto result = m_manager->registerEndpoint(endpoint);
    ASSERT_EQ(result.wait_for(std::chrono::milliseconds::zero()), std::future_status::timeout);

    m_capabilitiesObserver->onCapabilitiesStateChange(
        CapabilitiesObserverInterface::State::SUCCESS, CapabilitiesObserverInterface::Error::SUCCESS, {endpointId}, {});
    ASSERT_EQ(result.wait_for(MY_WAIT_TIMEOUT), std::future_status::ready);
    EXPECT_EQ(result.get(), RegistrationResult::SUCCEEDED);

    // Check that deleting the endpoint failed.
    auto deleteResult = m_manager->deregisterEndpoint(endpointId);
    ASSERT_EQ(deleteResult.wait_for(std::chrono::milliseconds::zero()), std::future_status::timeout);

    m_capabilitiesObserver->onCapabilitiesStateChange(
        CapabilitiesObserverInterface::State::FATAL_ERROR,
        CapabilitiesObserverInterface::Error::UNKNOWN_ERROR,
        {},
        {endpointId});
    ASSERT_EQ(deleteResult.wait_for(MY_WAIT_TIMEOUT), std::future_status::ready);
    EXPECT_EQ(deleteResult.get(), DeregistrationResult::CONFIGURATION_ERROR);
}

/*
 * Test deregistering an endpoint. If it fails (due to directive handler failure), the original endpoint should be
 * restored and deregistering the endpoint should fail.
 */
TEST_F(EndpointRegistrationManagerTest, test_revertWhenDeregisterEndpointFailsDueToDirectiveHandlerFailure) {
    // Configure endpoint object expectations.
    auto endpoint = std::make_shared<MockEndpoint>();
    EndpointIdentifier endpointId = "EndpointId";
    CapabilityConfiguration configuration{"Type", "InterfaceName", "1.0"};
    std::vector<CapabilityConfiguration> configurations{{configuration}};
    std::unordered_map<CapabilityConfiguration, std::shared_ptr<DirectiveHandlerInterface>> capabilities;
    std::shared_ptr<DirectiveHandlerInterface> handler = std::make_shared<MockDirectiveHandler>();
    capabilities[configuration] = handler;
    validateEndpointConfiguration(endpoint, endpointId, configurations, capabilities);

    // Expect directive sequencer and capabilities delegate calls for adding and then restoring original endpoint.
    EXPECT_CALL(*m_sequencer, addDirectiveHandler(handler)).WillOnce(Return(true));
    EXPECT_CALL(*m_capabilitiesDelegate, addOrUpdateEndpoint(_, configurations)).WillOnce(Return(true));

    // Expect directive sequencer and capabilities delegate calls for updating the endpoint.
    EXPECT_CALL(*m_sequencer, removeDirectiveHandler(handler)).WillOnce(Return(false));
    EXPECT_CALL(*m_capabilitiesDelegate, deleteEndpoint(_, configurations)).Times(0);

    // Expect that the observer will be notified that the endpoint was registered twice: adding it, then updating.
    EXPECT_CALL(*m_registrationObserver, onEndpointRegistration(endpointId, _, RegistrationResult::SUCCEEDED)).Times(1);
    EXPECT_CALL(
        *m_registrationObserver, onEndpointDeregistration(endpointId, DeregistrationResult::CONFIGURATION_ERROR))
        .Times(1);

    // Check that register endpoint was enqueued.
    auto result = m_manager->registerEndpoint(endpoint);
    ASSERT_EQ(result.wait_for(std::chrono::milliseconds::zero()), std::future_status::timeout);

    m_capabilitiesObserver->onCapabilitiesStateChange(
        CapabilitiesObserverInterface::State::SUCCESS, CapabilitiesObserverInterface::Error::SUCCESS, {endpointId}, {});
    ASSERT_EQ(result.wait_for(MY_WAIT_TIMEOUT), std::future_status::ready);
    EXPECT_EQ(result.get(), RegistrationResult::SUCCEEDED);

    // Check that deleting the endpoint failed.
    auto deleteResult = m_manager->deregisterEndpoint(endpointId);
    ASSERT_EQ(deleteResult.wait_for(MY_WAIT_TIMEOUT), std::future_status::ready);
    EXPECT_EQ(deleteResult.get(), DeregistrationResult::CONFIGURATION_ERROR);
}

}  // namespace test
}  // namespace endpoints
}  // namespace alexaClientSDK
