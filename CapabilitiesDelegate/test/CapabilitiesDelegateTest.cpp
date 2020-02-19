/*
 * Copyright 2018-2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <AVSCommon/SDKInterfaces/CapabilitiesObserverInterface.h>
#include <CapabilitiesDelegate/CapabilitiesDelegate.h>
#include <CapabilitiesDelegate/PostConnectCapabilitiesPublisher.h>
#include <CapabilitiesDelegate/Storage/CapabilitiesDelegateStorageInterface.h>
#include <CapabilitiesDelegate/Utils/DiscoveryUtils.h>

#include "MockAuthDelegate.h"
#include "MockCapabilitiesObserver.h"
#include "MockCapabilitiesStorage.h"

namespace alexaClientSDK {
namespace capabilitiesDelegate {
namespace test {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::endpoints;
using namespace avsCommon::avs;
using namespace ::testing;

/**
 * Mock discovery event sender class.
 */
class MockDiscoveryEventSender : public DiscoveryEventSenderInterface {
public:
    MOCK_METHOD1(addDiscoveryStatusObserver, void(const std::shared_ptr<DiscoveryStatusObserverInterface>&));
    MOCK_METHOD1(removeDiscoveryStatusObserver, void(const std::shared_ptr<DiscoveryStatusObserverInterface>&));
    MOCK_METHOD1(onAlexaEventProcessedReceived, void(const std::string&));
};

/**
 * Create a test @c AVSDiscoveryEndpointAttributes.
 * @return a @c AVSDiscoveryEndpointAttributes structure.
 */
AVSDiscoveryEndpointAttributes createEndpointAttributes() {
    AVSDiscoveryEndpointAttributes attributes;

    attributes.endpointId = "TEST_ENDPOINT_ID";
    attributes.description = "TEST_DESCRIPTION";
    attributes.manufacturerName = "TEST_MANUFACTURER_NAME";
    attributes.displayCategories = {"TEST_DISPLAY_CATEGORY"};

    return attributes;
}

/**
 * Creates a test @c CapabilityConfiguration.
 * @return a @c CapabilityConfiguration structure.
 */
CapabilityConfiguration createCapabilityConfiguration() {
    return CapabilityConfiguration("TEST_TYPE", "TEST_INTERFACE", "TEST_VERSION");
}

/**
 * Test harness for @c CapabilitiesDelegate class.
 */
class CapabilitiesDelegateTest : public Test {
public:
    /**
     * Set up the test harness for running a test.
     */
    void SetUp() override;

    /**
     * Tear down the test harness after running a test.
     */
    void TearDown() override;

protected:
    /// The mock Auth Delegate instance.
    std::shared_ptr<MockAuthDelegate> m_mockAuthDelegate;

    /// The mock Capabilities Storage instance.
    std::shared_ptr<MockCapabilitiesDelegateStorage> m_mockCapabilitiesStorage;

    /// The mock Capabilities observer instance.
    std::shared_ptr<MockCapabilitiesObserver> m_mockCapabilitiesObserver;

    /// The data manager required to build the base object
    std::shared_ptr<registrationManager::CustomerDataManager> m_dataManager;

    /// The instance of the capabilitiesDelegate used in the tests.
    std::shared_ptr<CapabilitiesDelegate> m_capabilitiesDelegate;
};

void CapabilitiesDelegateTest::SetUp() {
    m_mockCapabilitiesStorage = std::make_shared<StrictMock<MockCapabilitiesDelegateStorage>>();
    m_mockAuthDelegate = std::make_shared<StrictMock<MockAuthDelegate>>();
    m_mockCapabilitiesObserver = std::make_shared<StrictMock<MockCapabilitiesObserver>>();
    m_dataManager = std::make_shared<registrationManager::CustomerDataManager>();

    /// Expect calls to storage.
    EXPECT_CALL(*m_mockCapabilitiesStorage, open()).WillOnce(Return(true));
    m_capabilitiesDelegate = CapabilitiesDelegate::create(m_mockAuthDelegate, m_mockCapabilitiesStorage, m_dataManager);
    ASSERT_NE(m_capabilitiesDelegate, nullptr);

    /// Add a new observer and it receives notifications of the current capabilities state.
    m_mockCapabilitiesObserver = std::make_shared<StrictMock<MockCapabilitiesObserver>>();
    EXPECT_CALL(*m_mockCapabilitiesObserver, onCapabilitiesStateChange(_, _))
        .WillOnce(
            Invoke([](CapabilitiesObserverInterface::State newState, CapabilitiesObserverInterface::Error newError) {
                EXPECT_EQ(newState, CapabilitiesObserverInterface::State::UNINITIALIZED);
                EXPECT_EQ(newError, CapabilitiesObserverInterface::Error::UNINITIALIZED);
            }));

    m_capabilitiesDelegate->addCapabilitiesObserver(m_mockCapabilitiesObserver);
}

void CapabilitiesDelegateTest::TearDown() {
    m_capabilitiesDelegate->shutdown();
}

/**
 * Tests the create method with various configurations.
 */
TEST_F(CapabilitiesDelegateTest, test_createMethodWithInvalidParameters) {
    auto instance = CapabilitiesDelegate::create(nullptr, m_mockCapabilitiesStorage, m_dataManager);
    ASSERT_EQ(instance, nullptr);

    instance = CapabilitiesDelegate::create(m_mockAuthDelegate, nullptr, m_dataManager);
    ASSERT_EQ(instance, nullptr);

    instance = CapabilitiesDelegate::create(m_mockAuthDelegate, m_mockCapabilitiesStorage, nullptr);
    ASSERT_EQ(instance, nullptr);

    EXPECT_CALL(*m_mockCapabilitiesStorage, open()).WillOnce(Return(true));
    instance = CapabilitiesDelegate::create(m_mockAuthDelegate, m_mockCapabilitiesStorage, m_dataManager);
    ASSERT_NE(instance, nullptr);
    instance->shutdown();
}

/**
 * Tests the init method and if the open(), createDatabase() and load() methods get called on storage.
 */
TEST_F(CapabilitiesDelegateTest, test_init) {
    /// Test if creteDatabase fails create method return nullptr.
    EXPECT_CALL(*m_mockCapabilitiesStorage, open()).WillOnce(Return(false));
    EXPECT_CALL(*m_mockCapabilitiesStorage, createDatabase()).Times(1).WillOnce(Return(false));
    auto instance = CapabilitiesDelegate::create(m_mockAuthDelegate, m_mockCapabilitiesStorage, m_dataManager);
    ASSERT_EQ(instance, nullptr);

    /// Happy path.
    EXPECT_CALL(*m_mockCapabilitiesStorage, open()).WillOnce(Return(false));
    EXPECT_CALL(*m_mockCapabilitiesStorage, createDatabase()).WillOnce(Return(true));

    instance = CapabilitiesDelegate::create(m_mockAuthDelegate, m_mockCapabilitiesStorage, m_dataManager);
    ASSERT_NE(instance, nullptr);
    instance->shutdown();
}

/**
 * Tests if the invalidateCapabilities method triggers a database cleanup.
 */
TEST_F(CapabilitiesDelegateTest, test_invalidateCapabilities) {
    EXPECT_CALL(*m_mockCapabilitiesStorage, clearDatabase()).WillOnce(Return(true));
    m_capabilitiesDelegate->invalidateCapabilities();
}

/**
 * Tests if the clearData method triggers a database cleanup.
 */
TEST_F(CapabilitiesDelegateTest, test_clearData) {
    EXPECT_CALL(*m_mockCapabilitiesStorage, clearDatabase()).WillOnce(Return(true));
    m_capabilitiesDelegate->clearData();
}

/**
 * Tests if the addDiscoveryObserver method gets triggered when the addDiscoveryEventSender method is called.
 * Test if the removeDisoveryObserver method gets triggered when the shutdown method is called.
 */
TEST_F(CapabilitiesDelegateTest, test_shutdownTriggeresRemoveDiscoveryObserver) {
    auto discoveryEventSender = std::make_shared<StrictMock<MockDiscoveryEventSender>>();
    EXPECT_CALL(*discoveryEventSender, addDiscoveryStatusObserver(_))
        .WillOnce(Invoke([this](const std::shared_ptr<DiscoveryStatusObserverInterface>& observer) {
            EXPECT_EQ(observer, m_capabilitiesDelegate);
        }));
    m_capabilitiesDelegate->addDiscoveryEventSender(discoveryEventSender);

    EXPECT_CALL(*discoveryEventSender, removeDiscoveryStatusObserver(_))
        .WillOnce(Invoke([this](const std::shared_ptr<DiscoveryStatusObserverInterface>& observer) {
            EXPECT_EQ(observer, m_capabilitiesDelegate);
        }));
    m_capabilitiesDelegate->shutdown();
}

/**
 * Tests the addCapabilitiesObserver() method.
 */
TEST_F(CapabilitiesDelegateTest, test_addCapabilitiesObserver) {
    /// Adding null observer doesn't fail.
    m_capabilitiesDelegate->addCapabilitiesObserver(nullptr);

    /// Add a new observer and it receives notifications of the current capabilities state.
    auto mockObserver = std::make_shared<StrictMock<MockCapabilitiesObserver>>();

    EXPECT_CALL(*mockObserver, onCapabilitiesStateChange(_, _))
        .WillOnce(
            Invoke([](CapabilitiesObserverInterface::State newState, CapabilitiesObserverInterface::Error newError) {
                EXPECT_EQ(newState, CapabilitiesObserverInterface::State::UNINITIALIZED);
                EXPECT_EQ(newError, CapabilitiesObserverInterface::Error::UNINITIALIZED);
            }));
    m_capabilitiesDelegate->addCapabilitiesObserver(mockObserver);

    /// Add existing observer and it does not get any notifications (strict mock would catch any extra notifications)
    m_capabilitiesDelegate->addCapabilitiesObserver(mockObserver);
}

/**
 * Tests for onDiscoveryCompleted() method.
 */
TEST_F(CapabilitiesDelegateTest, test_onDiscoveryCompleted) {
    std::unordered_map<std::string, std::string> addOrUpdateReportEndpoints = {{"add_1", "1"}, {"update_1", "2"}};
    std::unordered_map<std::string, std::string> deleteReportEndpoints = {{"delete_1", "1"}};

    EXPECT_CALL(*m_mockCapabilitiesStorage, store(addOrUpdateReportEndpoints)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockCapabilitiesStorage, erase(deleteReportEndpoints)).WillOnce(Return(true));

    EXPECT_CALL(*m_mockCapabilitiesObserver, onCapabilitiesStateChange(_, _))
        .WillOnce(
            Invoke([](CapabilitiesObserverInterface::State newState, CapabilitiesObserverInterface::Error newError) {
                EXPECT_EQ(newState, CapabilitiesObserverInterface::State::SUCCESS);
                EXPECT_EQ(newError, CapabilitiesObserverInterface::Error::SUCCESS);
            }));

    /// Check if store and erase is triggered and if observer gets notified.
    m_capabilitiesDelegate->onDiscoveryCompleted(addOrUpdateReportEndpoints, deleteReportEndpoints);

    /// Check removing observer does not send notifications to the observer.
    m_capabilitiesDelegate->removeCapabilitiesObserver(m_mockCapabilitiesObserver);

    EXPECT_CALL(*m_mockCapabilitiesStorage, store(addOrUpdateReportEndpoints)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockCapabilitiesStorage, erase(deleteReportEndpoints)).WillOnce(Return(true));

    /// Only store and erase is triggered, observer does not get notified (should fail as we use strict mock for
    /// observer).
    m_capabilitiesDelegate->onDiscoveryCompleted(addOrUpdateReportEndpoints, deleteReportEndpoints);
}

/**
 * Check onDiscoveryCompleted() but storage to device fails.
 */
TEST_F(CapabilitiesDelegateTest, test_onDiscoveryCompletedButStorageFails) {
    std::unordered_map<std::string, std::string> addOrUpdateReportEndpoints = {{"add_1", "1"}, {"update_1", "2"}};
    std::unordered_map<std::string, std::string> deleteReportEndpoints = {{"delete_1", "1"}};

    EXPECT_CALL(*m_mockCapabilitiesStorage, store(addOrUpdateReportEndpoints)).WillOnce(Return(false));

    EXPECT_CALL(*m_mockCapabilitiesObserver, onCapabilitiesStateChange(_, _))
        .WillOnce(
            Invoke([](CapabilitiesObserverInterface::State newState, CapabilitiesObserverInterface::Error newError) {
                EXPECT_EQ(newState, CapabilitiesObserverInterface::State::FATAL_ERROR);
                EXPECT_EQ(newError, CapabilitiesObserverInterface::Error::UNKNOWN_ERROR);
            }));

    /// Check if store and erase is triggered and if observer gets notified.
    m_capabilitiesDelegate->onDiscoveryCompleted(addOrUpdateReportEndpoints, deleteReportEndpoints);
}

/**
 * Check notifications when onDiscoveryFailure() method is called.
 */
TEST_F(CapabilitiesDelegateTest, test_onDicoveryFailure) {
    /// validate retriable error response
    EXPECT_CALL(*m_mockCapabilitiesObserver, onCapabilitiesStateChange(_, _))
        .WillOnce(
            Invoke([](CapabilitiesObserverInterface::State newState, CapabilitiesObserverInterface::Error newError) {
                EXPECT_EQ(newState, CapabilitiesObserverInterface::State::RETRIABLE_ERROR);
                EXPECT_EQ(newError, CapabilitiesObserverInterface::Error::SERVER_INTERNAL_ERROR);
            }));

    m_capabilitiesDelegate->onDiscoveryFailure(MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2);

    /// validate invalid auth error response
    EXPECT_CALL(*m_mockCapabilitiesObserver, onCapabilitiesStateChange(_, _))
        .WillOnce(
            Invoke([](CapabilitiesObserverInterface::State newState, CapabilitiesObserverInterface::Error newError) {
                EXPECT_EQ(newState, CapabilitiesObserverInterface::State::FATAL_ERROR);
                EXPECT_EQ(newError, CapabilitiesObserverInterface::Error::FORBIDDEN);
            }));

    m_capabilitiesDelegate->onDiscoveryFailure(MessageRequestObserverInterface::Status::INVALID_AUTH);

    /// validate bad request error response
    EXPECT_CALL(*m_mockCapabilitiesObserver, onCapabilitiesStateChange(_, _))
        .WillOnce(
            Invoke([](CapabilitiesObserverInterface::State newState, CapabilitiesObserverInterface::Error newError) {
                EXPECT_EQ(newState, CapabilitiesObserverInterface::State::FATAL_ERROR);
                EXPECT_EQ(newError, CapabilitiesObserverInterface::Error::BAD_REQUEST);
            }));

    m_capabilitiesDelegate->onDiscoveryFailure(MessageRequestObserverInterface::Status::BAD_REQUEST);

    /// other responses
    EXPECT_CALL(*m_mockCapabilitiesObserver, onCapabilitiesStateChange(_, _))
        .WillOnce(
            Invoke([](CapabilitiesObserverInterface::State newState, CapabilitiesObserverInterface::Error newError) {
                EXPECT_EQ(newState, CapabilitiesObserverInterface::State::RETRIABLE_ERROR);
                EXPECT_EQ(newError, CapabilitiesObserverInterface::Error::UNKNOWN_ERROR);
            }));

    m_capabilitiesDelegate->onDiscoveryFailure(MessageRequestObserverInterface::Status::THROTTLED);
}

/**
 * Tests if the registerEndpoint returns true for new endpoints, and false for invalid input or existing endpoint.
 */
TEST_F(CapabilitiesDelegateTest, test_registerEndpoint) {
    /// Invalid AVSDiscoveryEndpointAttributes.

    auto attributes = createEndpointAttributes();
    auto capabilityConfig = createCapabilityConfiguration();

    /// Empty Capabilities.
    ASSERT_FALSE(m_capabilitiesDelegate->registerEndpoint(attributes, std::vector<CapabilityConfiguration>()));

    /// Invalid CapabilityConfiguration.
    capabilityConfig.version = "";
    ASSERT_FALSE(m_capabilitiesDelegate->registerEndpoint(attributes, {capabilityConfig}));

    /// Registering with same endpointAttributes fails.
    ASSERT_TRUE(m_capabilitiesDelegate->registerEndpoint(attributes, {createCapabilityConfiguration()}));
    ASSERT_FALSE(m_capabilitiesDelegate->registerEndpoint(attributes, {createCapabilityConfiguration()}));

    /// EndpointAttributes does not have endpointID which is required.
    attributes.endpointId = "";
    ASSERT_FALSE(m_capabilitiesDelegate->registerEndpoint(attributes, {capabilityConfig}));
}

/***
 * Tests if the createPostConnectOperation() creates the @c PostConnectCapabilitiesPublisher when registered endpoint
 * configurations are different from the ones in storage.
 */
TEST_F(CapabilitiesDelegateTest, test_createPostConnectOperationWithDifferentEndpointConfigs) {
    auto endpointAttributes = createEndpointAttributes();
    auto capabilityConfig = createCapabilityConfiguration();

    std::string endpointConfig = "TEST_CONFIG";
    EXPECT_CALL(*m_mockCapabilitiesStorage, open()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockCapabilitiesStorage, load(_))
        .Times(1)
        .WillOnce(
            Invoke([endpointAttributes, endpointConfig](std::unordered_map<std::string, std::string>* storedEndpoints) {
                storedEndpoints->insert({endpointAttributes.endpointId, endpointConfig});
                return true;
            }));

    auto instance = CapabilitiesDelegate::create(m_mockAuthDelegate, m_mockCapabilitiesStorage, m_dataManager);
    instance->registerEndpoint(endpointAttributes, {capabilityConfig});

    /// Endpoint config is different from the endpoint config created with the test endpoint attributes so a
    /// post connect operation is created.
    auto publisher = instance->createPostConnectOperation();
    instance->shutdown();

    ASSERT_NE(publisher, nullptr);
}

/**
 * Tests if the createPostConnectOperation() does not create a new @c PostConnectCapabilitiesPublisher when registered
 * endpoint configurations are same as the ones in storage.
 */
TEST_F(CapabilitiesDelegateTest, test_createPostConnectOperationWithSameEndpointConfigs) {
    auto endpointAttributes = createEndpointAttributes();
    auto capabilityConfig = createCapabilityConfiguration();
    std::vector<CapabilityConfiguration> capabilityConfigs = {capabilityConfig};

    std::string endpointConfig = utils::getEndpointConfigJson(endpointAttributes, capabilityConfigs);
    EXPECT_CALL(*m_mockCapabilitiesStorage, open()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockCapabilitiesStorage, load(_))
        .Times(1)
        .WillOnce(
            Invoke([endpointAttributes, endpointConfig](std::unordered_map<std::string, std::string>* storedEndpoints) {
                storedEndpoints->insert({endpointAttributes.endpointId, endpointConfig});
                return true;
            }));

    auto instance = CapabilitiesDelegate::create(m_mockAuthDelegate, m_mockCapabilitiesStorage, m_dataManager);
    instance->registerEndpoint(endpointAttributes, capabilityConfigs);

    /// Endpoint config is same as the endpoint config created with the test endpoint attributes so a
    /// post connect operation is not created.
    auto publisher = instance->createPostConnectOperation();

    ASSERT_EQ(publisher, nullptr);
}

/**
 * Test if the CapabilitiesDelegate calls the clearDatabase() method when the onAVSGatewayChanged() method is called.
 */
TEST_F(CapabilitiesDelegateTest, test_onAVSGatewayChangedNotification) {
    EXPECT_CALL(*m_mockCapabilitiesStorage, clearDatabase()).WillOnce(Return(true));
    m_capabilitiesDelegate->onAVSGatewayChanged("TEST_GATEWAY");
}

}  // namespace test
}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK
