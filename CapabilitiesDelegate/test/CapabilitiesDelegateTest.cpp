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

#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <AVSCommon/SDKInterfaces/CapabilitiesDelegateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/WaitEvent.h>
#include <CapabilitiesDelegate/CapabilitiesDelegate.h>
#include <CapabilitiesDelegate/PostConnectCapabilitiesPublisher.h>
#include <CapabilitiesDelegate/Storage/CapabilitiesDelegateStorageInterface.h>
#include <CapabilitiesDelegate/Utils/DiscoveryUtils.h>
#include <RegistrationManager/MockCustomerDataManager.h>

#include "MockAuthDelegate.h"
#include "MockCapabilitiesDelegateObserver.h"
#include "MockCapabilitiesStorage.h"
#include "MockDiscoveryEventSender.h"

namespace alexaClientSDK {
namespace capabilitiesDelegate {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::endpoints;
using namespace avsCommon::utils::json::jsonUtils;
using namespace avsCommon::utils;
using namespace capabilitiesDelegate::utils;

using namespace ::testing;

/// Test string for auth token.
static const std::string TEST_AUTH_TOKEN = "TEST_AUTH_TOKEN";
/// The event key in the discovery event.
static const std::string EVENT_KEY = "event";
/// The header key in the discovery event.
static const std::string HEADER_KEY = "header";
/// The eventCorrelationToken key in the discovery event.
static const std::string EVENT_CORRELATION_TOKEN_KEY = "eventCorrelationToken";
/// Constant representing the timeout for test events.
/// @note Use a large enough value that should not fail even in slower systems.
static const std::chrono::seconds MY_WAIT_TIMEOUT{5};
/// A Test client Id.
static const std::string TEST_CLIENT_ID = "TEST_CLIENT_ID";
/// A Test product Id.
static const std::string TEST_PRODUCT_ID = "TEST_PRODUCT_ID";
/// A Test serial number.
static const std::string TEST_SERIAL_NUMBER = "TEST_SERIAL_NUMBER";
/// A Test product Id.
static const std::string TEST_REGISTRATION_KEY = "TEST_REGISTRATION_KEY";
/// A Test product Id Key.
static const std::string TEST_PRODUCT_ID_KEY = "TEST_PRODUCT_ID_KEY";
/// A Test manufacturer name.
static const std::string TEST_MANUFACTURER_NAME = "TEST_MANUFACTURER_NAME";
/// A Test description.
static const std::string TEST_DESCRIPTION = "TEST_DESCRIPTION";
/// A Test display category.
static const std::string TEST_DISPLAY_CATEGORY = "TEST_DISPLAY_CATEGORY";

/**
 * Structure to store event data from a Discovery event JSON.
 */
struct EventData {
    std::string namespaceString;
    std::string nameString;
    std::string payloadVersionString;
    std::string eventCorrelationTokenString;
    std::vector<std::string> endpointIdsInPayload;
    std::string authToken;
};

/**
 * Create a test @c AVSDiscoveryEndpointAttributes.
 * @param endpointId Optional endpointId to use in these attributes. Default "TEST_ENDPOINT_ID".
 * @return a @c AVSDiscoveryEndpointAttributes structure.
 */
AVSDiscoveryEndpointAttributes createEndpointAttributes(const std::string endpointId = "TEST_ENDPOINT_ID") {
    AVSDiscoveryEndpointAttributes attributes;

    attributes.endpointId = endpointId;
    attributes.description = TEST_DESCRIPTION;
    attributes.manufacturerName = TEST_MANUFACTURER_NAME;
    attributes.displayCategories = {TEST_DISPLAY_CATEGORY};

    return attributes;
}

/**
 * Create a test @c AVSDiscoveryEndpointAttributes::Registration object.
 * @param productId Optional product Id to use in these attributes.
 * @param serialNumber Optional device serial number to use in these attributes.
 * @param registrationKey Optional registration key to use in these attributes.
 * @param productIdKey Optional product Id Key to use in these attributes.
 * @return a @c AVSDiscoveryEndpointAttributes::Registration structure.
 */
AVSDiscoveryEndpointAttributes::Registration createEndpointRegistration(
    const std::string productID = TEST_PRODUCT_ID,
    const std::string serialNumber = TEST_SERIAL_NUMBER,
    const std::string registrationKey = TEST_REGISTRATION_KEY,
    const std::string productIdKey = TEST_PRODUCT_ID_KEY) {
    return AVSDiscoveryEndpointAttributes::Registration(productID, serialNumber, registrationKey, productIdKey);
}

/**
 * Creates a test @c CapabilityConfiguration.
 * @return a @c CapabilityConfiguration structure.
 */
CapabilityConfiguration createCapabilityConfiguration(
    const CapabilityConfiguration::AdditionalConfigurations& additionalConfigurationsIn =
        CapabilityConfiguration::AdditionalConfigurations()) {
    return CapabilityConfiguration(
        "TEST_TYPE",
        "TEST_INTERFACE",
        "TEST_VERSION",
        avsCommon::utils::Optional<std::string>(),
        avsCommon::utils::Optional<CapabilityConfiguration::Properties>(),
        additionalConfigurationsIn);
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
    /// Validates AuthDelegate calls.
    void validateAuthDelegate();

    /// Helper that validates dynamically adding an endpoint. Used for testing dynamic delete.
    void addEndpoint(AVSDiscoveryEndpointAttributes attributes, CapabilityConfiguration configuration);

    /// Helper that validates dynamically adding an endpoint. Used for testing dynamic delete.
    void addEndpointToCapabilitiesDelegate(
        std::shared_ptr<CapabilitiesDelegate>,
        AVSDiscoveryEndpointAttributes attributes,
        CapabilityConfiguration configuration);

    /*
     * Gets the event correlation token string.
     * @param request The message request to parse.
     * @param[out] eventCorrelationTokenString The parsed event correlation token string.
     */
    void getEventCorrelationTokenString(
        std::shared_ptr<MessageRequest> request,
        std::string& eventCorrelationTokenString);

    /// The mock Auth Delegate instance.
    std::shared_ptr<MockAuthDelegate> m_mockAuthDelegate;

    /// The mock Capabilities Storage instance.
    std::shared_ptr<MockCapabilitiesDelegateStorage> m_mockCapabilitiesStorage;

    /// The mock CapabilitiesDelegate observer instance.
    std::shared_ptr<MockCapabilitiesDelegateObserver> m_mockCapabilitiesDelegateObserver;

    /// The data manager required to build the base object
    std::shared_ptr<registrationManager::MockCustomerDataManager> m_dataManager;

    /// The mock @c MessageSenderInterface used in the tests.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockMessageSender> m_mockMessageSender;

    /// The instance of the capabilitiesDelegate used in the tests.
    std::shared_ptr<CapabilitiesDelegate> m_capabilitiesDelegate;
};

void CapabilitiesDelegateTest::SetUp() {
    m_mockCapabilitiesStorage = std::make_shared<StrictMock<MockCapabilitiesDelegateStorage>>();
    m_mockAuthDelegate = std::make_shared<StrictMock<MockAuthDelegate>>();
    m_mockCapabilitiesDelegateObserver = std::make_shared<StrictMock<MockCapabilitiesDelegateObserver>>();
    m_dataManager = std::make_shared<NiceMock<registrationManager::MockCustomerDataManager>>();
    m_mockMessageSender = std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockMessageSender>>();

    /// Expect calls to storage.
    EXPECT_CALL(*m_mockCapabilitiesStorage, open()).WillOnce(Return(true));
    m_capabilitiesDelegate = CapabilitiesDelegate::create(m_mockAuthDelegate, m_mockCapabilitiesStorage, m_dataManager);

    ASSERT_NE(m_capabilitiesDelegate, nullptr);

    /// Add a new observer and it receives notifications of the current capabilities state.
    m_mockCapabilitiesDelegateObserver = std::make_shared<StrictMock<MockCapabilitiesDelegateObserver>>();
    EXPECT_CALL(*m_mockCapabilitiesDelegateObserver, onCapabilitiesStateChange(_, _, _, _))
        .WillOnce(Invoke([](CapabilitiesDelegateObserverInterface::State newState,
                            CapabilitiesDelegateObserverInterface::Error newError,
                            std::vector<std::string> addOrUpdateReportEndpointIdentifiers,
                            std::vector<std::string> deleteReportEndpointIdentifiers) {
            EXPECT_EQ(newState, CapabilitiesDelegateObserverInterface::State::UNINITIALIZED);
            EXPECT_EQ(newError, CapabilitiesDelegateObserverInterface::Error::UNINITIALIZED);
            EXPECT_EQ(addOrUpdateReportEndpointIdentifiers, std::vector<std::string>{});
            EXPECT_EQ(deleteReportEndpointIdentifiers, std::vector<std::string>{});
        }));

    m_capabilitiesDelegate->addCapabilitiesObserver(m_mockCapabilitiesDelegateObserver);
    m_capabilitiesDelegate->setMessageSender(m_mockMessageSender);
}

void CapabilitiesDelegateTest::TearDown() {
    m_capabilitiesDelegate->shutdown();
}

void CapabilitiesDelegateTest::validateAuthDelegate() {
    EXPECT_CALL(*m_mockAuthDelegate, addAuthObserver(_))
        .WillRepeatedly(Invoke([](const std::shared_ptr<AuthObserverInterface>& observer) {
            observer->onAuthStateChange(AuthObserverInterface::State::REFRESHED, AuthObserverInterface::Error::SUCCESS);
        }));
    EXPECT_CALL(*m_mockAuthDelegate, removeAuthObserver(_)).Times(AtLeast(1));
    EXPECT_CALL(*m_mockAuthDelegate, getAuthToken()).WillRepeatedly(Return(TEST_AUTH_TOKEN));
}

void CapabilitiesDelegateTest::getEventCorrelationTokenString(
    std::shared_ptr<MessageRequest> request,
    std::string& eventCorrelationTokenString) {
    auto eventJson = request->getJsonContent();

    std::string eventString;
    retrieveValue(eventJson, EVENT_KEY, &eventString);

    std::string headerString;
    retrieveValue(eventString, HEADER_KEY, &headerString);

    retrieveValue(headerString, EVENT_CORRELATION_TOKEN_KEY, &eventCorrelationTokenString);
}

void CapabilitiesDelegateTest::addEndpoint(
    AVSDiscoveryEndpointAttributes attributes,
    CapabilityConfiguration configuration) {
    WaitEvent e;

    m_capabilitiesDelegate->onConnectionStatusChanged(
        ConnectionStatusObserverInterface::Status::CONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::SUCCESS);

    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).WillOnce(Invoke([this](std::shared_ptr<MessageRequest> request) {
        std::string eventCorrelationTokenString;
        getEventCorrelationTokenString(request, eventCorrelationTokenString);

        request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
        m_capabilitiesDelegate->onAlexaEventProcessedReceived(eventCorrelationTokenString);
    }));

    EXPECT_CALL(*m_mockCapabilitiesStorage, store(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockCapabilitiesStorage, erase((std::unordered_map<std::string, std::string>{})))
        .WillOnce(Return(true));

    EXPECT_CALL(
        *m_mockCapabilitiesDelegateObserver,
        onCapabilitiesStateChange(
            CapabilitiesDelegateObserverInterface::State::SUCCESS,
            CapabilitiesDelegateObserverInterface::Error::SUCCESS,
            _,
            _))
        .Times(1)
        .WillOnce(Invoke([&](CapabilitiesDelegateObserverInterface::State state,
                             CapabilitiesDelegateObserverInterface::Error error,
                             std::vector<std::string> addedEndpoints,
                             std::vector<std::string> deletedEndpoints) { e.wakeUp(); }));

    ASSERT_TRUE(m_capabilitiesDelegate->addOrUpdateEndpoint(attributes, {configuration}));

    ASSERT_TRUE(e.wait(MY_WAIT_TIMEOUT));
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
 * Test if the addDiscoveryObserver method gets triggered when the setDiscoveryEventSender method is called.
 * Test if DiscoveryEventSender is stopped when the shutdown method is called.
 * Test if the removeDiscoveryObserver method gets triggered when the shutdown method is called.
 */
TEST_F(CapabilitiesDelegateTest, test_shutdown) {
    auto discoveryEventSender = std::make_shared<StrictMock<MockDiscoveryEventSender>>();
    EXPECT_CALL(*discoveryEventSender, addDiscoveryStatusObserver(_))
        .WillOnce(Invoke([this](const std::shared_ptr<DiscoveryStatusObserverInterface>& observer) {
            EXPECT_EQ(observer, m_capabilitiesDelegate);
        }));
    m_capabilitiesDelegate->setDiscoveryEventSender(discoveryEventSender);

    EXPECT_CALL(*discoveryEventSender, removeDiscoveryStatusObserver(_))
        .WillOnce(Invoke([this](const std::shared_ptr<DiscoveryStatusObserverInterface>& observer) {
            EXPECT_EQ(observer, m_capabilitiesDelegate);
        }));
    EXPECT_CALL(*discoveryEventSender, stop()).Times(1);

    m_capabilitiesDelegate->shutdown();
}

/**
 * Tests the addCapabilitiesObserver() method.
 */
TEST_F(CapabilitiesDelegateTest, test_addCapabilitiesObserver) {
    /// Adding null observer doesn't fail.
    m_capabilitiesDelegate->addCapabilitiesObserver(nullptr);

    /// Add a new observer and it receives notifications of the current capabilities state.
    auto mockObserver = std::make_shared<StrictMock<MockCapabilitiesDelegateObserver>>();

    EXPECT_CALL(*mockObserver, onCapabilitiesStateChange(_, _, _, _))
        .WillOnce(Invoke([](CapabilitiesDelegateObserverInterface::State newState,
                            CapabilitiesDelegateObserverInterface::Error newError,
                            std::vector<std::string> addOrUpdateReportEndpointIdentifiers,
                            std::vector<std::string> deleteReportEndpointIdentifiers) {
            EXPECT_EQ(newState, CapabilitiesDelegateObserverInterface::State::UNINITIALIZED);
            EXPECT_EQ(newError, CapabilitiesDelegateObserverInterface::Error::UNINITIALIZED);
            EXPECT_EQ(addOrUpdateReportEndpointIdentifiers, std::vector<std::string>{});
            EXPECT_EQ(deleteReportEndpointIdentifiers, std::vector<std::string>{});
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

    EXPECT_CALL(*m_mockCapabilitiesDelegateObserver, onCapabilitiesStateChange(_, _, _, _))
        .WillOnce(Invoke([](CapabilitiesDelegateObserverInterface::State newState,
                            CapabilitiesDelegateObserverInterface::Error newError,
                            std::vector<std::string> addOrUpdateReportEndpointIdentifiers,
                            std::vector<std::string> deleteReportEndpointIdentifiers) {
            EXPECT_EQ(newState, CapabilitiesDelegateObserverInterface::State::SUCCESS);
            EXPECT_EQ(newError, CapabilitiesDelegateObserverInterface::Error::SUCCESS);

            std::sort(addOrUpdateReportEndpointIdentifiers.begin(), addOrUpdateReportEndpointIdentifiers.end());
            EXPECT_EQ(addOrUpdateReportEndpointIdentifiers, (std::vector<std::string>{"add_1", "update_1"}));
            EXPECT_EQ(deleteReportEndpointIdentifiers, (std::vector<std::string>{"delete_1"}));
        }));

    /// Check if store and erase is triggered and if observer gets notified.
    m_capabilitiesDelegate->onDiscoveryCompleted(addOrUpdateReportEndpoints, deleteReportEndpoints);

    /// Check removing observer does not send notifications to the observer.
    m_capabilitiesDelegate->removeCapabilitiesObserver(m_mockCapabilitiesDelegateObserver);

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

    EXPECT_CALL(*m_mockCapabilitiesDelegateObserver, onCapabilitiesStateChange(_, _, _, _))
        .WillOnce(Invoke([](CapabilitiesDelegateObserverInterface::State newState,
                            CapabilitiesDelegateObserverInterface::Error newError,
                            std::vector<std::string> addOrUpdateReportEndpointIdentifiers,
                            std::vector<std::string> deleteReportEndpointIdentifiers) {
            EXPECT_EQ(newState, CapabilitiesDelegateObserverInterface::State::FATAL_ERROR);
            EXPECT_EQ(newError, CapabilitiesDelegateObserverInterface::Error::UNKNOWN_ERROR);

            std::sort(addOrUpdateReportEndpointIdentifiers.begin(), addOrUpdateReportEndpointIdentifiers.end());
            EXPECT_EQ(addOrUpdateReportEndpointIdentifiers, (std::vector<std::string>{"add_1", "update_1"}));
            EXPECT_EQ(deleteReportEndpointIdentifiers, std::vector<std::string>{"delete_1"});
        }));

    /// Check if store and erase is triggered and if observer gets notified.
    m_capabilitiesDelegate->onDiscoveryCompleted(addOrUpdateReportEndpoints, deleteReportEndpoints);
}

/**
 * Check notifications when onDiscoveryFailure() method is called.
 */
TEST_F(CapabilitiesDelegateTest, test_onDiscoveryFailure) {
    /// validate retriable error response
    EXPECT_CALL(*m_mockCapabilitiesDelegateObserver, onCapabilitiesStateChange(_, _, _, _))
        .WillOnce(Invoke([](CapabilitiesDelegateObserverInterface::State newState,
                            CapabilitiesDelegateObserverInterface::Error newError,
                            std::vector<std::string> addOrUpdateReportEndpointIdentifiers,
                            std::vector<std::string> deleteReportEndpointIdentifiers) {
            EXPECT_EQ(newState, CapabilitiesDelegateObserverInterface::State::RETRIABLE_ERROR);
            EXPECT_EQ(newError, CapabilitiesDelegateObserverInterface::Error::SERVER_INTERNAL_ERROR);
            EXPECT_EQ(addOrUpdateReportEndpointIdentifiers, std::vector<std::string>{});
            EXPECT_EQ(deleteReportEndpointIdentifiers, std::vector<std::string>{});
        }));

    m_capabilitiesDelegate->onDiscoveryFailure(MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2);

    /// validate invalid auth error response
    EXPECT_CALL(*m_mockCapabilitiesDelegateObserver, onCapabilitiesStateChange(_, _, _, _))
        .WillOnce(Invoke([](CapabilitiesDelegateObserverInterface::State newState,
                            CapabilitiesDelegateObserverInterface::Error newError,
                            std::vector<std::string> addOrUpdateReportEndpointIdentifiers,
                            std::vector<std::string> deleteReportEndpointIdentifiers) {
            EXPECT_EQ(newState, CapabilitiesDelegateObserverInterface::State::FATAL_ERROR);
            EXPECT_EQ(newError, CapabilitiesDelegateObserverInterface::Error::FORBIDDEN);
            EXPECT_EQ(addOrUpdateReportEndpointIdentifiers, std::vector<std::string>{});
            EXPECT_EQ(deleteReportEndpointIdentifiers, std::vector<std::string>{});
        }));

    m_capabilitiesDelegate->onDiscoveryFailure(MessageRequestObserverInterface::Status::INVALID_AUTH);

    /// validate bad request error response
    EXPECT_CALL(*m_mockCapabilitiesDelegateObserver, onCapabilitiesStateChange(_, _, _, _))
        .WillOnce(Invoke([](CapabilitiesDelegateObserverInterface::State newState,
                            CapabilitiesDelegateObserverInterface::Error newError,
                            std::vector<std::string> addOrUpdateReportEndpointIdentifiers,
                            std::vector<std::string> deleteReportEndpointIdentifiers) {
            EXPECT_EQ(newState, CapabilitiesDelegateObserverInterface::State::FATAL_ERROR);
            EXPECT_EQ(newError, CapabilitiesDelegateObserverInterface::Error::BAD_REQUEST);
            EXPECT_EQ(addOrUpdateReportEndpointIdentifiers, std::vector<std::string>{});
            EXPECT_EQ(deleteReportEndpointIdentifiers, std::vector<std::string>{});
        }));

    m_capabilitiesDelegate->onDiscoveryFailure(MessageRequestObserverInterface::Status::BAD_REQUEST);

    /// other responses
    EXPECT_CALL(*m_mockCapabilitiesDelegateObserver, onCapabilitiesStateChange(_, _, _, _))
        .WillOnce(Invoke([](CapabilitiesDelegateObserverInterface::State newState,
                            CapabilitiesDelegateObserverInterface::Error newError,
                            std::vector<std::string> addOrUpdateReportEndpointIdentifiers,
                            std::vector<std::string> deleteReportEndpointIdentifiers) {
            EXPECT_EQ(newState, CapabilitiesDelegateObserverInterface::State::RETRIABLE_ERROR);
            EXPECT_EQ(newError, CapabilitiesDelegateObserverInterface::Error::UNKNOWN_ERROR);
            EXPECT_EQ(addOrUpdateReportEndpointIdentifiers, std::vector<std::string>{});
            EXPECT_EQ(deleteReportEndpointIdentifiers, std::vector<std::string>{});
        }));

    m_capabilitiesDelegate->onDiscoveryFailure(MessageRequestObserverInterface::Status::THROTTLED);
}

/**
 * Tests if addOrUpdateEndpoint returns false for invalid input.
 */
TEST_F(CapabilitiesDelegateTest, test_addOrUpdateEndpointReturnsFalseWithInvalidInput) {
    /// Invalid AVSDiscoveryEndpointAttributes.
    auto attributes = createEndpointAttributes("endpointId");
    auto capabilityConfig = createCapabilityConfiguration();

    /// Empty Capabilities.
    ASSERT_FALSE(m_capabilitiesDelegate->addOrUpdateEndpoint(attributes, std::vector<CapabilityConfiguration>()));

    /// Invalid CapabilityConfiguration.
    capabilityConfig.version = "";
    ASSERT_FALSE(m_capabilitiesDelegate->addOrUpdateEndpoint(attributes, {capabilityConfig}));

    /// EndpointAttributes does not have endpointID which is required.
    attributes.endpointId = "";
    ASSERT_FALSE(m_capabilitiesDelegate->addOrUpdateEndpoint(attributes, {capabilityConfig}));

    /// EndpointConfiguration too big
    attributes = createEndpointAttributes("endpointId");
    std::string hugeAdditionalAttribute(256 * 1024, 'X');
    std::map<std::string, std::string> additionalAttributes;
    additionalAttributes["test"] = "{\"test\":\"" + hugeAdditionalAttribute + "\"}";
    capabilityConfig = createCapabilityConfiguration(additionalAttributes);
    ASSERT_FALSE(m_capabilitiesDelegate->addOrUpdateEndpoint(attributes, {capabilityConfig}));

    /// Return false if endpoint is a duplicate in pending list.
    attributes = createEndpointAttributes("duplicateId");
    capabilityConfig = createCapabilityConfiguration();
    ASSERT_TRUE(m_capabilitiesDelegate->addOrUpdateEndpoint(attributes, {capabilityConfig}));
    ASSERT_FALSE(m_capabilitiesDelegate->addOrUpdateEndpoint(attributes, {capabilityConfig}));
}

/**
 * Tests dynamic addOrUpdateEndpoint happy path.
 */
TEST_F(CapabilitiesDelegateTest, test_dynamicAddOrUpdateEndpoint) {
    /// Set-up.
    WaitEvent e;
    validateAuthDelegate();
    m_capabilitiesDelegate->onConnectionStatusChanged(
        ConnectionStatusObserverInterface::Status::CONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::SUCCESS);
    auto attributes = createEndpointAttributes("endpointId");
    auto capabilityConfig = createCapabilityConfiguration();

    /// Expect calls to MessageSender.
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .Times(AtLeast(1))
        .WillOnce(Invoke([this](std::shared_ptr<MessageRequest> request) {
            std::string eventCorrelationTokenString;
            getEventCorrelationTokenString(request, eventCorrelationTokenString);

            request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
            m_capabilitiesDelegate->onAlexaEventProcessedReceived(eventCorrelationTokenString);
        }));

    /// Expect call to storage.
    EXPECT_CALL(*m_mockCapabilitiesStorage, store(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockCapabilitiesStorage, erase((std::unordered_map<std::string, std::string>{})))
        .WillOnce(Return(true));

    /// Expect callback to CapabilitiesObserver.
    EXPECT_CALL(*m_mockCapabilitiesDelegateObserver, onCapabilitiesStateChange(_, _, _, _))
        .WillOnce(Invoke([&](CapabilitiesDelegateObserverInterface::State state,
                             CapabilitiesDelegateObserverInterface::Error error,
                             std::vector<std::string> addedEndpoints,
                             std::vector<std::string> deletedEndpoints) {
            EXPECT_EQ(state, CapabilitiesDelegateObserverInterface::State::SUCCESS);
            EXPECT_EQ(error, CapabilitiesDelegateObserverInterface::Error::SUCCESS);
            EXPECT_EQ(addedEndpoints, (std::vector<std::string>{"endpointId"}));
            EXPECT_EQ(deletedEndpoints, (std::vector<std::string>{}));

            e.wakeUp();
        }));

    ASSERT_TRUE(m_capabilitiesDelegate->addOrUpdateEndpoint(attributes, {capabilityConfig}));

    ASSERT_TRUE(e.wait(MY_WAIT_TIMEOUT));
}

/**
 * Tests if deleteEndpoint returns false for invalid input.
 */
TEST_F(CapabilitiesDelegateTest, test_deleteEndpointReturnsFalseWithInvalidInput) {
    /// Invalid AVSDiscoveryEndpointAttributes.

    auto attributes = createEndpointAttributes("endpointId");
    auto capabilityConfig = createCapabilityConfiguration();

    /// Empty Capabilities.
    ASSERT_FALSE(m_capabilitiesDelegate->deleteEndpoint(attributes, std::vector<CapabilityConfiguration>()));

    /// Invalid CapabilityConfiguration.
    capabilityConfig.version = "";
    ASSERT_FALSE(m_capabilitiesDelegate->deleteEndpoint(attributes, {capabilityConfig}));

    /// EndpointAttributes does not have endpointID which is required.
    attributes.endpointId = "";
    ASSERT_FALSE(m_capabilitiesDelegate->deleteEndpoint(attributes, {capabilityConfig}));
}

/**
 * Tests dynamic deleteEndpoint happy path.
 */
TEST_F(CapabilitiesDelegateTest, test_dynamicDeleteEndpoint) {
    /// Set-up.
    WaitEvent e;
    validateAuthDelegate();
    m_capabilitiesDelegate->onConnectionStatusChanged(
        ConnectionStatusObserverInterface::Status::CONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::SUCCESS);

    auto attributes = createEndpointAttributes("deleteId");
    auto capabilityConfig = createCapabilityConfiguration();

    auto configJson = getEndpointConfigJson(attributes, std::vector<CapabilityConfiguration>{capabilityConfig});

    /// Add endpoint (so we can delete it).
    addEndpoint(attributes, capabilityConfig);

    /// Expect calls to MessageSender.
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .WillRepeatedly(Invoke([](std::shared_ptr<MessageRequest> request) {
            request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
        }));

    /// Expect call to storage.
    EXPECT_CALL(*m_mockCapabilitiesStorage, store(_)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockCapabilitiesStorage,
        erase(std::unordered_map<std::string, std::string>({{attributes.endpointId, configJson}})))
        .WillOnce(Return(true));

    /// Expect callback to CapabilitiesObserver.
    EXPECT_CALL(*m_mockCapabilitiesDelegateObserver, onCapabilitiesStateChange(_, _, _, _))
        .WillOnce(Invoke([&](CapabilitiesDelegateObserverInterface::State state,
                             CapabilitiesDelegateObserverInterface::Error error,
                             std::vector<std::string> addedEndpoints,
                             std::vector<std::string> deletedEndpoints) {
            EXPECT_EQ(state, CapabilitiesDelegateObserverInterface::State::SUCCESS);
            EXPECT_EQ(error, CapabilitiesDelegateObserverInterface::Error::SUCCESS);
            EXPECT_EQ(addedEndpoints, (std::vector<std::string>{}));
            EXPECT_EQ(deletedEndpoints, (std::vector<std::string>{"deleteId"}));
            e.wakeUp();
        }));

    ASSERT_TRUE(m_capabilitiesDelegate->deleteEndpoint(attributes, {capabilityConfig}));

    ASSERT_TRUE(e.wait(MY_WAIT_TIMEOUT));
}

/**
 * Tests dynamic deleteEndpoint should fail if endpoint is unregistered.
 */
TEST_F(CapabilitiesDelegateTest, test_dynamicDeleteEndpointWhenEndpointNotRegisteredShouldFail) {
    /// Set-up.
    auto attributes = createEndpointAttributes("deleteId");
    auto capabilityConfig = createCapabilityConfiguration();

    ASSERT_FALSE(m_capabilitiesDelegate->deleteEndpoint(attributes, {capabilityConfig}));
}

/***
 * Tests if the createPostConnectOperation() creates the @c PostConnectCapabilitiesPublisher when registered endpoint
 * configurations are different from the ones in storage.
 */
TEST_F(CapabilitiesDelegateTest, test_createPostConnectOperationWithDifferentEndpointConfigs) {
    auto endpointAttributes = createEndpointAttributes("endpointId");
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

    instance->addOrUpdateEndpoint(endpointAttributes, {capabilityConfig});
    /// Endpoint config is different from the endpoint config created with the test endpoint attributes so a
    /// post connect operation is created.
    auto publisher = instance->createPostConnectOperation();
    instance->shutdown();

    ASSERT_NE(publisher, nullptr);
}

/**
 * Tests if the createPostConnectOperation() does not create a new @c PostConnectCapabilitiesPublisher when registered
 * pending endpoint configurations are same as the ones in storage.
 * Tests if CapabilitiesDelegate reports this as a success to observers as there are pending endpoints.
 */
TEST_F(CapabilitiesDelegateTest, test_createPostConnectOperationWithPendingEndpointsWithSameEndpointConfigs) {
    auto endpointAttributes = createEndpointAttributes("endpointId");
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
    EXPECT_CALL(*m_mockCapabilitiesDelegateObserver, onCapabilitiesStateChange(_, _, _, _))
        .WillOnce(Invoke([](CapabilitiesDelegateObserverInterface::State newState,
                            CapabilitiesDelegateObserverInterface::Error newError,
                            std::vector<std::string> addOrUpdateReportEndpointIdentifiers,
                            std::vector<std::string> deleteReportEndpointIdentifiers) {
            EXPECT_EQ(newState, CapabilitiesDelegateObserverInterface::State::UNINITIALIZED);
            EXPECT_EQ(newError, CapabilitiesDelegateObserverInterface::Error::UNINITIALIZED);
            EXPECT_EQ(addOrUpdateReportEndpointIdentifiers, std::vector<std::string>{});
            EXPECT_EQ(deleteReportEndpointIdentifiers, std::vector<std::string>{});
        }));

    auto instance = CapabilitiesDelegate::create(m_mockAuthDelegate, m_mockCapabilitiesStorage, m_dataManager);
    instance->addCapabilitiesObserver(m_mockCapabilitiesDelegateObserver);
    instance->addOrUpdateEndpoint(endpointAttributes, capabilityConfigs);

    /// Endpoint config is same as the endpoint config created with the test endpoint attributes so a
    /// post connect operation is not created. However, we do expect an observer callback as there were pending
    /// endpoints.
    EXPECT_CALL(
        *m_mockCapabilitiesDelegateObserver,
        onCapabilitiesStateChange(
            CapabilitiesDelegateObserverInterface::State::SUCCESS,
            CapabilitiesDelegateObserverInterface::Error::SUCCESS,
            std::vector<std::string>{endpointAttributes.endpointId},
            _));
    auto publisher = instance->createPostConnectOperation();

    ASSERT_EQ(publisher, nullptr);

    // Clean-up.
    instance->shutdown();
}

/**
 * Tests if the createPostConnectOperation() does not create a new @c PostConnectCapabilitiesPublisher when registered
 * pending endpoint configurations are same as the ones in storage.
 * Tests that CapabilitiesDelegate does not notify observers on re-connect, as there were no changes and no pending
 * endpoints expecting to be registered. This ensures observers are not notified unnecessarily during re-connects.
 */
TEST_F(CapabilitiesDelegateTest, test_createPostConnectOperationWithoutPendingEndpointsAndSameEndpointConfigs) {
    /// Set-up.
    auto endpointAttributes = createEndpointAttributes("endpointId");
    auto capabilityConfig = createCapabilityConfiguration();
    std::vector<CapabilityConfiguration> capabilityConfigs = {capabilityConfig};

    std::string endpointConfig = utils::getEndpointConfigJson(endpointAttributes, capabilityConfigs);
    EXPECT_CALL(*m_mockCapabilitiesStorage, open()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockCapabilitiesStorage, load(_))
        .Times(2)
        .WillRepeatedly(
            Invoke([endpointAttributes, endpointConfig](std::unordered_map<std::string, std::string>* storedEndpoints) {
                storedEndpoints->insert({endpointAttributes.endpointId, endpointConfig});
                return true;
            }));
    EXPECT_CALL(*m_mockCapabilitiesDelegateObserver, onCapabilitiesStateChange(_, _, _, _))
        .WillOnce(Invoke([](CapabilitiesDelegateObserverInterface::State newState,
                            CapabilitiesDelegateObserverInterface::Error newError,
                            std::vector<std::string> addOrUpdateReportEndpointIdentifiers,
                            std::vector<std::string> deleteReportEndpointIdentifiers) {
            EXPECT_EQ(newState, CapabilitiesDelegateObserverInterface::State::UNINITIALIZED);
            EXPECT_EQ(newError, CapabilitiesDelegateObserverInterface::Error::UNINITIALIZED);
            EXPECT_EQ(addOrUpdateReportEndpointIdentifiers, std::vector<std::string>{});
            EXPECT_EQ(deleteReportEndpointIdentifiers, std::vector<std::string>{});
        }));

    auto instance = CapabilitiesDelegate::create(m_mockAuthDelegate, m_mockCapabilitiesStorage, m_dataManager);
    instance->addCapabilitiesObserver(m_mockCapabilitiesDelegateObserver);

    /// Add the endpoint here before creating the initial post-connect, which forces the endpoint to be cached in
    /// CapabilitiesDelegate.
    /// Here we do expect an observer callback because there is a pending endpoint. This is test set-up.s
    instance->addOrUpdateEndpoint(endpointAttributes, capabilityConfigs);
    EXPECT_CALL(
        *m_mockCapabilitiesDelegateObserver,
        onCapabilitiesStateChange(
            CapabilitiesDelegateObserverInterface::State::SUCCESS,
            CapabilitiesDelegateObserverInterface::Error::SUCCESS,
            std::vector<std::string>{endpointAttributes.endpointId},
            _));
    auto publisher = instance->createPostConnectOperation();
    ASSERT_EQ(publisher, nullptr);

    /// Test.

    /// Create another post-connect operation; this simulates the re-connect.
    /// There are no pending endpoints to send in CapabilitiesDelegate, and the cached endpoint in CapabilitiesDelegate
    /// matches what is stored in the database, so expect no observer callback as well as a null post connect operation.
    /// Strict mocks will catch the observer callback if it happens incorrectly.
    publisher = instance->createPostConnectOperation();

    /// Endpoint config is same as the endpoint config created with the test endpoint attributes so a
    /// post connect operation is not created.
    ASSERT_EQ(publisher, nullptr);

    // Clean-up.
    instance->shutdown();
}

/**
 * Tests if the createPostConnectOperation() caches pending endpoints even when it does not create a new post-connect
 * operation as capabilities have not change.
 */
TEST_F(CapabilitiesDelegateTest, test_createPostConnectOperationCachesEndpoints) {
    auto endpointAttributes = createEndpointAttributes("endpointId");
    auto capabilityConfig = createCapabilityConfiguration();
    std::vector<CapabilityConfiguration> capabilityConfigs = {capabilityConfig};

    std::string endpointConfig = utils::getEndpointConfigJson(endpointAttributes, capabilityConfigs);
    EXPECT_CALL(*m_mockCapabilitiesStorage, open()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockCapabilitiesStorage, load(_))
        .Times(2)
        .WillOnce(
            Invoke([endpointAttributes, endpointConfig](std::unordered_map<std::string, std::string>* storedEndpoints) {
                /// Returning a stored endpoint here is necessary to force CapabilitiesDelegate to cache the endpoint
                /// on the first call to createPostConnectOperation().
                storedEndpoints->insert({endpointAttributes.endpointId, endpointConfig});
                return true;
            }))
        .WillOnce(
            Invoke([endpointAttributes, endpointConfig](std::unordered_map<std::string, std::string>* storedEndpoints) {
                /// This represents an empty database, so that when createPostConnectOperation() is called a second
                /// time, we can verify that it creates a non-null post-connect publisher to send the cached endpoint.
                return true;
            }));
    EXPECT_CALL(*m_mockCapabilitiesDelegateObserver, onCapabilitiesStateChange(_, _, _, _))
        .WillOnce(Invoke([](CapabilitiesDelegateObserverInterface::State newState,
                            CapabilitiesDelegateObserverInterface::Error newError,
                            std::vector<std::string> addOrUpdateReportEndpointIdentifiers,
                            std::vector<std::string> deleteReportEndpointIdentifiers) {
            EXPECT_EQ(newState, CapabilitiesDelegateObserverInterface::State::UNINITIALIZED);
            EXPECT_EQ(newError, CapabilitiesDelegateObserverInterface::Error::UNINITIALIZED);
            EXPECT_EQ(addOrUpdateReportEndpointIdentifiers, std::vector<std::string>{});
            EXPECT_EQ(deleteReportEndpointIdentifiers, std::vector<std::string>{});
        }));

    auto instance = CapabilitiesDelegate::create(m_mockAuthDelegate, m_mockCapabilitiesStorage, m_dataManager);
    instance->addCapabilitiesObserver(m_mockCapabilitiesDelegateObserver);

    /// Endpoint config is same as the endpoint config created with the test endpoint attributes so a
    /// post connect operation is not created, but we add the endpoint here so that it is cached in CapabilitiesDelegate
    /// for testing later.
    /// Expect an observer callback here, since there is a pending endpoint.
    instance->addOrUpdateEndpoint(endpointAttributes, capabilityConfigs);
    EXPECT_CALL(
        *m_mockCapabilitiesDelegateObserver,
        onCapabilitiesStateChange(
            CapabilitiesDelegateObserverInterface::State::SUCCESS,
            CapabilitiesDelegateObserverInterface::Error::SUCCESS,
            std::vector<std::string>{endpointAttributes.endpointId},
            _));
    auto publisher = instance->createPostConnectOperation();
    ASSERT_EQ(publisher, nullptr);

    /// Test.
    /// Create another post-connect operation; this simulates a re-connect. We can verify whether the endpoint
    /// from the first createPostConnectOperation() was cached.

    /// The database is empty, but because there should be a cached endpoint, we expect a non-null post-connect
    /// publisher to send that endpoint to AVS.
    publisher = instance->createPostConnectOperation();
    ASSERT_NE(publisher, nullptr);

    // Clean-up.
    instance->shutdown();
}

/**
 * Tests if the createPostConnectOperation() creates a new @c PostConnectCapabilitiesPublisher when registered
 * endpoint configurations are same as the ones in storage, but there is one additional stored endpoint that is
 * not registered (and needs to be deleted).
 * Tests observers are NOT notified of added endpoints since there are no pending endpoints.s
 * Tests if CapabilitiesDelegate returns a non-null post connect operation, since there is a stale endpoint to delete.
 */
TEST_F(
    CapabilitiesDelegateTest,
    test_createPostConnectOperationWithStaleEndpointAndWithoutPendingEndpointsAndSameEndpointConfigs) {
    auto endpointAttributes = createEndpointAttributes("endpointId");
    auto capabilityConfig = createCapabilityConfiguration();
    std::vector<CapabilityConfiguration> capabilityConfigs = {capabilityConfig};

    auto staleEndpointAttributes = createEndpointAttributes("staleEndpointId");
    auto staleEndpointConfiguration = createCapabilityConfiguration();
    std::vector<CapabilityConfiguration> staleCapabilityConfigs = {staleEndpointConfiguration};

    std::string endpointConfig = utils::getEndpointConfigJson(endpointAttributes, capabilityConfigs);
    std::string staleEndpointConfig = utils::getEndpointConfigJson(staleEndpointAttributes, staleCapabilityConfigs);
    EXPECT_CALL(*m_mockCapabilitiesStorage, open()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockCapabilitiesStorage, load(_))
        .Times(2)
        .WillOnce(Invoke([endpointAttributes, endpointConfig, staleEndpointAttributes, staleEndpointConfig](
                             std::unordered_map<std::string, std::string>* storedEndpoints) {
            storedEndpoints->insert({endpointAttributes.endpointId, endpointConfig});
            return true;
        }))
        .WillOnce(Invoke([endpointAttributes, endpointConfig, staleEndpointAttributes, staleEndpointConfig](
                             std::unordered_map<std::string, std::string>* storedEndpoints) {
            storedEndpoints->insert({endpointAttributes.endpointId, endpointConfig});
            /// Stale endpoint is in the database but not registered.
            storedEndpoints->insert({staleEndpointAttributes.endpointId, staleEndpointConfig});
            return true;
        }));
    EXPECT_CALL(*m_mockCapabilitiesDelegateObserver, onCapabilitiesStateChange(_, _, _, _))
        .WillOnce(Invoke([](CapabilitiesDelegateObserverInterface::State newState,
                            CapabilitiesDelegateObserverInterface::Error newError,
                            std::vector<std::string> addOrUpdateReportEndpointIdentifiers,
                            std::vector<std::string> deleteReportEndpointIdentifiers) {
            EXPECT_EQ(newState, CapabilitiesDelegateObserverInterface::State::UNINITIALIZED);
            EXPECT_EQ(newError, CapabilitiesDelegateObserverInterface::Error::UNINITIALIZED);
            EXPECT_EQ(addOrUpdateReportEndpointIdentifiers, std::vector<std::string>{});
            EXPECT_EQ(deleteReportEndpointIdentifiers, std::vector<std::string>{});
        }));

    auto instance = CapabilitiesDelegate::create(m_mockAuthDelegate, m_mockCapabilitiesStorage, m_dataManager);
    instance->addCapabilitiesObserver(m_mockCapabilitiesDelegateObserver);

    /// Endpoint config is same as the endpoint config created with the test endpoint attributes so a
    /// post connect operation is not created, but add it here so that it is cached in CapabilitiesDelegate.
    instance->addOrUpdateEndpoint(endpointAttributes, capabilityConfigs);
    EXPECT_CALL(
        *m_mockCapabilitiesDelegateObserver,
        onCapabilitiesStateChange(
            CapabilitiesDelegateObserverInterface::State::SUCCESS,
            CapabilitiesDelegateObserverInterface::Error::SUCCESS,
            std::vector<std::string>{endpointAttributes.endpointId},
            _));
    auto publisher = instance->createPostConnectOperation();
    ASSERT_EQ(publisher, nullptr);

    /// Create another post-connect operation; this simulates a re-connect.
    /// There is a stale endpoint to be sent, so we expect a non-null post connect publisher.
    /// However, there is no pending endpoint to register, so we expect no immediate observer notification.
    publisher = instance->createPostConnectOperation();

    /// Endpoint config is same as the endpoint config created with the test endpoint attributes so a
    /// post connect operation is not created.
    ASSERT_NE(publisher, nullptr);

    // Clean-up.
    instance->shutdown();
}

/**
 * Tests if the createPostConnectOperation() creates a new @c PostConnectCapabilitiesPublisher when there is an
 * endpoint in storage that is not registered (that is, a stale endpoint in the database).
 * Tests if CapabilitiesDelegate returns a non-null post connect operation, since there is a stale endpoint to delete.
 */
TEST_F(CapabilitiesDelegateTest, test_createPostConnectOperationWithStaleEndpoint) {
    auto staleEndpointAttributes = createEndpointAttributes("staleEndpointId");
    auto staleEndpointConfiguration = createCapabilityConfiguration();
    std::vector<CapabilityConfiguration> staleCapabilityConfigs = {staleEndpointConfiguration};

    std::string staleEndpointConfig = utils::getEndpointConfigJson(staleEndpointAttributes, staleCapabilityConfigs);
    EXPECT_CALL(*m_mockCapabilitiesStorage, open()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockCapabilitiesStorage, load(_))
        .WillOnce(Invoke([staleEndpointAttributes,
                          staleEndpointConfig](std::unordered_map<std::string, std::string>* storedEndpoints) {
            storedEndpoints->insert({staleEndpointAttributes.endpointId, staleEndpointConfig});
            return true;
        }));
    EXPECT_CALL(*m_mockCapabilitiesDelegateObserver, onCapabilitiesStateChange(_, _, _, _))
        .WillOnce(Invoke([](CapabilitiesDelegateObserverInterface::State newState,
                            CapabilitiesDelegateObserverInterface::Error newError,
                            std::vector<std::string> addOrUpdateReportEndpointIdentifiers,
                            std::vector<std::string> deleteReportEndpointIdentifiers) {
            EXPECT_EQ(newState, CapabilitiesDelegateObserverInterface::State::UNINITIALIZED);
            EXPECT_EQ(newError, CapabilitiesDelegateObserverInterface::Error::UNINITIALIZED);
            EXPECT_EQ(addOrUpdateReportEndpointIdentifiers, std::vector<std::string>{});
            EXPECT_EQ(deleteReportEndpointIdentifiers, std::vector<std::string>{});
        }));

    auto instance = CapabilitiesDelegate::create(m_mockAuthDelegate, m_mockCapabilitiesStorage, m_dataManager);
    instance->addCapabilitiesObserver(m_mockCapabilitiesDelegateObserver);

    /// There is a stale endpoint to be deleted, so we expect a non-null post connect publisher.
    auto publisher = instance->createPostConnectOperation();

    /// Endpoint config is same as the endpoint config created with the test endpoint attributes so a
    /// post connect operation is not created.
    ASSERT_NE(publisher, nullptr);

    // Clean-up.
    instance->shutdown();
}

/**
 * Tests if the createPostConnectOperation() creates a new @c PostConnectCapabilitiesPublisher when registered
 * endpoint configurations are same as the ones in storage, but there is one additional stored endpoint that is
 * not registered (and needs to be deleted).
 * Tests CapabilitiesObservers are notified of added endpoints even though they were not published in an event.
 * Tests if CapabilitiesDelegate returns a non-null post connect operation, since there is a stale endpoint to delete.
 */
TEST_F(
    CapabilitiesDelegateTest,
    test_createPostConnectOperationWithStaleEndpointAndPendingEndpointsWithSameEndpointConfigs) {
    auto unchangedEndpointAttributes = createEndpointAttributes("endpointId");
    auto unchangedEndpointConfiguration = createCapabilityConfiguration();
    std::vector<CapabilityConfiguration> unchangedCapabilityConfigs = {unchangedEndpointConfiguration};

    auto staleEndpointAttributes = createEndpointAttributes("staleEndpointId");
    auto staleEndpointConfiguration = createCapabilityConfiguration();
    std::vector<CapabilityConfiguration> staleCapabilityConfigs = {staleEndpointConfiguration};

    std::string unchangedEndpointConfig =
        utils::getEndpointConfigJson(unchangedEndpointAttributes, unchangedCapabilityConfigs);
    std::string staleEndpointConfig = utils::getEndpointConfigJson(staleEndpointAttributes, staleCapabilityConfigs);
    EXPECT_CALL(*m_mockCapabilitiesStorage, open()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockCapabilitiesStorage, load(_))
        .Times(1)
        .WillOnce(
            Invoke([unchangedEndpointAttributes, unchangedEndpointConfig, staleEndpointAttributes, staleEndpointConfig](
                       std::unordered_map<std::string, std::string>* storedEndpoints) {
                storedEndpoints->insert({unchangedEndpointAttributes.endpointId, unchangedEndpointConfig});
                storedEndpoints->insert({staleEndpointAttributes.endpointId, staleEndpointConfig});
                return true;
            }));
    EXPECT_CALL(*m_mockCapabilitiesDelegateObserver, onCapabilitiesStateChange(_, _, _, _))
        .WillOnce(Invoke([](CapabilitiesDelegateObserverInterface::State newState,
                            CapabilitiesDelegateObserverInterface::Error newError,
                            std::vector<std::string> addOrUpdateReportEndpointIdentifiers,
                            std::vector<std::string> deleteReportEndpointIdentifiers) {
            EXPECT_EQ(newState, CapabilitiesDelegateObserverInterface::State::UNINITIALIZED);
            EXPECT_EQ(newError, CapabilitiesDelegateObserverInterface::Error::UNINITIALIZED);
            EXPECT_EQ(addOrUpdateReportEndpointIdentifiers, std::vector<std::string>{});
            EXPECT_EQ(deleteReportEndpointIdentifiers, std::vector<std::string>{});
        }));

    auto instance = CapabilitiesDelegate::create(m_mockAuthDelegate, m_mockCapabilitiesStorage, m_dataManager);
    instance->addCapabilitiesObserver(m_mockCapabilitiesDelegateObserver);
    instance->addOrUpdateEndpoint(unchangedEndpointAttributes, unchangedCapabilityConfigs);

    /// Observer callback should only contain the pending endpoint to add (since that is already registered),
    /// but not the stale endpoint to delete (since that still needs to be sent to AVS).
    EXPECT_CALL(
        *m_mockCapabilitiesDelegateObserver,
        onCapabilitiesStateChange(
            CapabilitiesDelegateObserverInterface::State::SUCCESS,
            CapabilitiesDelegateObserverInterface::Error::SUCCESS,
            std::vector<std::string>{unchangedEndpointAttributes.endpointId},
            std::vector<std::string>{}));

    auto publisher = instance->createPostConnectOperation();
    ASSERT_NE(publisher, nullptr);

    // Clean-up.
    instance->shutdown();
}

/**
 * Tests if the createPostConnectOperation() creates a new @c PostConnectCapabilitiesPublisher when registered
 * endpoint configurations are same as the ones in storage, but there is one additional stored endpoint that is
 * not registered (and needs to be deleted).
 * Tests CapabilitiesObservers are notified of added endpoints even though they were not published in an event.
 * Tests if CapabilitiesDelegate returns a non-null post connect operation, since there is a stale endpoint to delete.
 */
TEST_F(
    CapabilitiesDelegateTest,
    test_createPostConnectOperationWithNewEndpointAndPendingEndpointsWithSameEndpointConfigs) {
    auto unchangedEndpointAttributes = createEndpointAttributes("endpointId");
    auto unchangedEndpointConfiguration = createCapabilityConfiguration();
    std::vector<CapabilityConfiguration> unchangedCapabilityConfigs = {unchangedEndpointConfiguration};

    auto newEndpointAttributes = createEndpointAttributes("newEndpointId");
    auto newEndpointConfiguration = createCapabilityConfiguration();
    std::vector<CapabilityConfiguration> newCapabilityConfigs = {newEndpointConfiguration};

    std::string unchangedEndpointConfig =
        utils::getEndpointConfigJson(unchangedEndpointAttributes, unchangedCapabilityConfigs);
    std::string newEndpointConfig = utils::getEndpointConfigJson(newEndpointAttributes, newCapabilityConfigs);
    EXPECT_CALL(*m_mockCapabilitiesStorage, open()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockCapabilitiesStorage, load(_))
        .Times(1)
        .WillOnce(
            Invoke([unchangedEndpointAttributes, unchangedEndpointConfig, newEndpointAttributes, newEndpointConfig](
                       std::unordered_map<std::string, std::string>* storedEndpoints) {
                storedEndpoints->insert({unchangedEndpointAttributes.endpointId, unchangedEndpointConfig});
                return true;
            }));
    EXPECT_CALL(*m_mockCapabilitiesDelegateObserver, onCapabilitiesStateChange(_, _, _, _))
        .WillOnce(Invoke([](CapabilitiesDelegateObserverInterface::State newState,
                            CapabilitiesDelegateObserverInterface::Error newError,
                            std::vector<std::string> addOrUpdateReportEndpointIdentifiers,
                            std::vector<std::string> deleteReportEndpointIdentifiers) {
            EXPECT_EQ(newState, CapabilitiesDelegateObserverInterface::State::UNINITIALIZED);
            EXPECT_EQ(newError, CapabilitiesDelegateObserverInterface::Error::UNINITIALIZED);
            EXPECT_EQ(addOrUpdateReportEndpointIdentifiers, std::vector<std::string>{});
            EXPECT_EQ(deleteReportEndpointIdentifiers, std::vector<std::string>{});
        }));

    auto instance = CapabilitiesDelegate::create(m_mockAuthDelegate, m_mockCapabilitiesStorage, m_dataManager);
    instance->addCapabilitiesObserver(m_mockCapabilitiesDelegateObserver);
    instance->addOrUpdateEndpoint(unchangedEndpointAttributes, unchangedCapabilityConfigs);
    instance->addOrUpdateEndpoint(newEndpointAttributes, newCapabilityConfigs);

    /// Observer callback should only contain the pending endpoint to add (since that is already registered),
    /// but not the stale endpoint to delete (since that still needs to be sent to AVS).
    EXPECT_CALL(
        *m_mockCapabilitiesDelegateObserver,
        onCapabilitiesStateChange(
            CapabilitiesDelegateObserverInterface::State::SUCCESS,
            CapabilitiesDelegateObserverInterface::Error::SUCCESS,
            std::vector<std::string>{unchangedEndpointAttributes.endpointId},
            std::vector<std::string>{}));

    auto publisher = instance->createPostConnectOperation();
    ASSERT_NE(publisher, nullptr);

    // Clean-up.
    instance->shutdown();
}

/**
 * Tests if before the stale endpoint is deleted and the stale endpoint is added, that the first
 * createPostConnectOperation will create a deleteReport for the stale endpoint, but the second
 * createPostConnectOperation will return a nullptr operation because the stale endpoint has been added, and this
 * results in no change in capabilities.
 */
TEST_F(
    CapabilitiesDelegateTest,
    test_createTwoPostConnectOperationWithStaleEndpointAndPendingEndpointsWithSameEndpointConfigs) {
    auto unchangedEndpointAttributes = createEndpointAttributes("endpointId");
    auto unchangedEndpointConfiguration = createCapabilityConfiguration();
    std::vector<CapabilityConfiguration> unchangedCapabilityConfigs = {unchangedEndpointConfiguration};

    auto staleEndpointAttributes = createEndpointAttributes("staleEndpointId");
    auto staleEndpointConfiguration = createCapabilityConfiguration();
    std::vector<CapabilityConfiguration> staleCapabilityConfigs = {staleEndpointConfiguration};

    std::string unchangedEndpointConfig =
        utils::getEndpointConfigJson(unchangedEndpointAttributes, unchangedCapabilityConfigs);
    std::string staleEndpointConfig = utils::getEndpointConfigJson(staleEndpointAttributes, staleCapabilityConfigs);
    EXPECT_CALL(*m_mockCapabilitiesStorage, open()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockCapabilitiesStorage, load(_))
        .Times(2)
        .WillRepeatedly(
            Invoke([unchangedEndpointAttributes, unchangedEndpointConfig, staleEndpointAttributes, staleEndpointConfig](
                       std::unordered_map<std::string, std::string>* storedEndpoints) {
                storedEndpoints->insert({unchangedEndpointAttributes.endpointId, unchangedEndpointConfig});
                storedEndpoints->insert({staleEndpointAttributes.endpointId, staleEndpointConfig});
                return true;
            }));
    int numCallbacks = 0;
    EXPECT_CALL(*m_mockCapabilitiesDelegateObserver, onCapabilitiesStateChange(_, _, _, _))
        .Times(2)
        .WillRepeatedly(Invoke([&numCallbacks](
                                   CapabilitiesDelegateObserverInterface::State newState,
                                   CapabilitiesDelegateObserverInterface::Error newError,
                                   std::vector<std::string> addOrUpdateReportEndpointIdentifiers,
                                   std::vector<std::string> deleteReportEndpointIdentifiers) {
            if (numCallbacks == 0) {
                EXPECT_EQ(newState, CapabilitiesDelegateObserverInterface::State::UNINITIALIZED);
                EXPECT_EQ(newError, CapabilitiesDelegateObserverInterface::Error::UNINITIALIZED);
                EXPECT_EQ(addOrUpdateReportEndpointIdentifiers, std::vector<std::string>{});
                EXPECT_EQ(deleteReportEndpointIdentifiers, std::vector<std::string>{});
            } else {
                EXPECT_EQ(newState, CapabilitiesDelegateObserverInterface::State::SUCCESS);
                EXPECT_EQ(newError, CapabilitiesDelegateObserverInterface::Error::SUCCESS);
                EXPECT_EQ(addOrUpdateReportEndpointIdentifiers, std::vector<std::string>{"staleEndpointId"});
                EXPECT_EQ(deleteReportEndpointIdentifiers, std::vector<std::string>{});
            }
            numCallbacks++;
        }));

    auto instance = CapabilitiesDelegate::create(m_mockAuthDelegate, m_mockCapabilitiesStorage, m_dataManager);
    instance->addCapabilitiesObserver(m_mockCapabilitiesDelegateObserver);
    instance->addOrUpdateEndpoint(unchangedEndpointAttributes, unchangedCapabilityConfigs);

    /// Observer callback should only contain the pending endpoint to add (since that is already registered),
    /// but not the stale endpoint to delete (since that still needs to be sent to AVS).
    EXPECT_CALL(
        *m_mockCapabilitiesDelegateObserver,
        onCapabilitiesStateChange(
            CapabilitiesDelegateObserverInterface::State::SUCCESS,
            CapabilitiesDelegateObserverInterface::Error::SUCCESS,
            std::vector<std::string>{unchangedEndpointAttributes.endpointId},
            std::vector<std::string>{}));

    auto publisher = instance->createPostConnectOperation();
    ASSERT_NE(publisher, nullptr);

    instance->addOrUpdateEndpoint(staleEndpointAttributes, staleCapabilityConfigs);
    auto publisher1 = instance->createPostConnectOperation();
    ASSERT_EQ(publisher1, nullptr);

    // Clean-up.
    instance->shutdown();
}

/**
 * Tests if the createPostConnectOperation() creates a new @c PostConnectCapabilitiesPublisher when storage is empty.
 * When the capabilities are successfully published, a subsequent call to createPostConnectOperation() results in a
 * nullptr.
 *
 * @note This test simulates a fresh device which sends a successful Discovery event followed by a reconnection.
 * Since the discovery event is already sent and the result is stored in the database, a new post connect capabilities
 * publisher is not created on reconnection.
 */
TEST_F(CapabilitiesDelegateTest, test_createPostConnectOperationWithReconnects) {
    auto endpointAttributes = createEndpointAttributes();
    auto capabilityConfig = createCapabilityConfiguration();
    std::vector<CapabilityConfiguration> capabilityConfigs = {capabilityConfig};

    std::string endpointConfig = utils::getEndpointConfigJson(endpointAttributes, capabilityConfigs);
    std::unordered_map<std::string, std::string> addOrUpdateReportEndpoints = {
        {endpointAttributes.endpointId, endpointConfig}};
    std::unordered_map<std::string, std::string> emptyDeleteReportEndpoints;

    EXPECT_CALL(*m_mockCapabilitiesStorage, open()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockCapabilitiesStorage, load(_)).Times(1).WillOnce(Return(true));

    auto instance = CapabilitiesDelegate::create(m_mockAuthDelegate, m_mockCapabilitiesStorage, m_dataManager);
    instance->addOrUpdateEndpoint(endpointAttributes, capabilityConfigs);

    /// Endpoint config in storage is empty, create a new post connect operation.
    auto publisher = instance->createPostConnectOperation();
    ASSERT_NE(publisher, nullptr);

    /// Expect the successfully published endpoint configuration to be stored and erase.
    EXPECT_CALL(*m_mockCapabilitiesStorage, store(_))
        .WillOnce(Invoke(
            [addOrUpdateReportEndpoints](const std::unordered_map<std::string, std::string>& endpointIdToConfigMap) {
                EXPECT_EQ(endpointIdToConfigMap, addOrUpdateReportEndpoints);
                return true;
            }));

    EXPECT_CALL(*m_mockCapabilitiesStorage, erase(emptyDeleteReportEndpoints))
        .WillOnce(Invoke(
            [emptyDeleteReportEndpoints](const std::unordered_map<std::string, std::string>& endpointIdToConfigMap) {
                EXPECT_EQ(endpointIdToConfigMap, emptyDeleteReportEndpoints);
                return true;
            }));

    /// Notify that discovery has successfully completed.
    instance->onDiscoveryCompleted(addOrUpdateReportEndpoints, emptyDeleteReportEndpoints);

    /// Expect call to load endpoint configuration
    EXPECT_CALL(*m_mockCapabilitiesStorage, load(_))
        .WillOnce(Invoke([addOrUpdateReportEndpoints](std::unordered_map<std::string, std::string>* storedEndpoints) {
            *storedEndpoints = addOrUpdateReportEndpoints;
            return true;
        }));

    /// PostConnectOperation after reconnection
    auto postConnectOperationOnReconnection = instance->createPostConnectOperation();
    ASSERT_EQ(postConnectOperationOnReconnection, nullptr);

    instance->shutdown();
}

/**
 * Tests if setDiscoveryEventSender always stops the previous DiscoveryEventSender. This is critical because
 * even though an individual DiscoveryEventSender contains exponential backoff and timeout logic to avoid spamming
 * the backend service, if *many* DiscoveryEventSender objects are created (e.g. in a re-connect loop with many
 * post-connect operations) and any of these are failed to be stopped before creating the next one, there is a real risk
 * of throttling.
 *
 * This test creates and adds many DiscoveryEventSenders asynchronously. We verify that every DiscoveryEventSender is
 * stopped.
 */
TEST_F(CapabilitiesDelegateTest, test_setDiscoveryEventSenderStopsPreviousDiscoveryEventSender) {
    const int numberOfSenders = 20;

    /// Set up the CapabilitiesDelegate.
    EXPECT_CALL(*m_mockCapabilitiesStorage, open()).WillOnce(Return(true));
    auto instance = CapabilitiesDelegate::create(m_mockAuthDelegate, m_mockCapabilitiesStorage, m_dataManager);
    ASSERT_NE(instance, nullptr);

    /// Create the mock DiscoveryEventSenders.
    std::vector<std::shared_ptr<StrictMock<MockDiscoveryEventSender>>> discoveryEventSenders(numberOfSenders);
    for (int i = 0; i < numberOfSenders; i++) {
        auto sender = std::make_shared<StrictMock<MockDiscoveryEventSender>>();
        EXPECT_CALL(*sender, addDiscoveryStatusObserver(_));

        /// The last DES to be added will only have stop() called on it in CapabilitiesDelegate's own shutdown
        /// method. Non-mock DiscoveryEventSenders also call stop() on themselves in their own dtor, but
        /// that is not the case here as we're using mocks.
        EXPECT_CALL(*sender, removeDiscoveryStatusObserver(_));
        EXPECT_CALL(*sender, stop());

        discoveryEventSenders[i] = sender;
    }

    /// Spin up the threads that will all call setDiscoveryEventSender(...).
    std::vector<std::thread> threads;
    for (int j = 0; j < numberOfSenders; j++) {
        threads.emplace_back(
            [instance, &discoveryEventSenders, j]() { instance->setDiscoveryEventSender(discoveryEventSenders[j]); });
    }

    /// Wait for the threads.
    for (auto& thread : threads) {
        thread.join();
    }

    instance->shutdown();
}

/**
 * Test if the CapabilitiesDelegate calls the clearDatabase() method when the onAVSGatewayChanged() method is called.
 */
TEST_F(CapabilitiesDelegateTest, test_onAVSGatewayChangedNotification) {
    EXPECT_CALL(*m_mockCapabilitiesStorage, clearDatabase()).WillOnce(Return(true));
    m_capabilitiesDelegate->onAVSGatewayChanged("TEST_GATEWAY");
}

/**
 * Test if post-connect operation will send all registered and pending endpoints if database is empty
 * (eg after onAVSGatewayChanged).
 * Test if pending delete for a registered endpoint will mean that endpoint is not sent in addOrUpdateReport.
 */
TEST_F(CapabilitiesDelegateTest, test_reconnectWhenStorageIsEmpty) {
    /// Set-up.
    WaitEvent e;
    validateAuthDelegate();
    auto capabilityConfig = createCapabilityConfiguration();

    /// Add a test endpoint.
    const std::string firstEndpointId = "add_1";
    auto firstEndpointAttributes = createEndpointAttributes(firstEndpointId);
    addEndpoint(firstEndpointAttributes, capabilityConfig);

    /// Add another test endpoint.
    const std::string secondEndpointId = "add_2";
    auto secondEndpointAttributes = createEndpointAttributes(secondEndpointId);
    addEndpoint(secondEndpointAttributes, capabilityConfig);

    /// Add a third test endpoint. Get the capability json to test pending delete.
    const std::string thirdEndpointId = "add_3";
    auto thirdEndpointAttributes = createEndpointAttributes(thirdEndpointId);
    addEndpoint(thirdEndpointAttributes, capabilityConfig);

    /// Expect calls to MessageSender.
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .WillOnce(Invoke([this](std::shared_ptr<MessageRequest> request) {
            std::string eventCorrelationTokenString;
            getEventCorrelationTokenString(request, eventCorrelationTokenString);

            request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
            m_capabilitiesDelegate->onAlexaEventProcessedReceived(eventCorrelationTokenString);
        }))
        .WillOnce(Invoke([](std::shared_ptr<MessageRequest> request) {
            request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
        }));

    /// Expect calls to storage.
    EXPECT_CALL(*m_mockCapabilitiesStorage, store(_)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockCapabilitiesStorage,
        erase((std::unordered_map<std::string, std::string>{
            {thirdEndpointId, utils::getEndpointConfigJson(thirdEndpointAttributes, {capabilityConfig})}})))
        .WillOnce(Return(true));

    /// Expect calls to CapabilitiesObserver.
    EXPECT_CALL(*m_mockCapabilitiesDelegateObserver, onCapabilitiesStateChange(_, _, _, _))
        .WillOnce(Invoke([&](CapabilitiesDelegateObserverInterface::State state,
                             CapabilitiesDelegateObserverInterface::Error error,
                             std::vector<std::string> addedEndpoints,
                             std::vector<std::string> deletedEndpoints) {
            EXPECT_EQ(state, CapabilitiesDelegateObserverInterface::State::SUCCESS);
            EXPECT_EQ(error, CapabilitiesDelegateObserverInterface::Error::SUCCESS);

            std::sort(addedEndpoints.begin(), addedEndpoints.end());
            EXPECT_EQ(addedEndpoints, (std::vector<std::string>{firstEndpointId, secondEndpointId}));
            EXPECT_EQ(deletedEndpoints, (std::vector<std::string>{thirdEndpointId}));
            e.wakeUp();
        }));

    /// Disconnect to force a pending delete.
    m_capabilitiesDelegate->onConnectionStatusChanged(
        ConnectionStatusObserverInterface::Status::DISCONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::SERVER_SIDE_DISCONNECT);

    ASSERT_TRUE(m_capabilitiesDelegate->deleteEndpoint(thirdEndpointAttributes, {capabilityConfig}));

    /// Test post-connect with an empty database.
    EXPECT_CALL(*m_mockCapabilitiesStorage, load(_))
        .Times(1)
        .WillOnce(Invoke([](std::unordered_map<std::string, std::string>* storedEndpoints) { return true; }));

    auto postconnect = m_capabilitiesDelegate->createPostConnectOperation();
    ASSERT_NE(postconnect, nullptr);
    ASSERT_TRUE(postconnect->performOperation(m_mockMessageSender));

    ASSERT_TRUE(e.wait(MY_WAIT_TIMEOUT));
}

/**
 * Test if the CapabilitiesDelegate defers dynamically adding or deleting endpoints while there is a
 * Discovery event in-flight.
 */
TEST_F(CapabilitiesDelegateTest, test_deferSendDiscoveryEventsWhileDiscoveryEventSenderInFlight) {
    /// Set-up.
    validateAuthDelegate();

    auto capabilityConfig = createCapabilityConfiguration();

    const std::string firstEndpointId = "add_1";
    auto firstEndpointAttributes = createEndpointAttributes(firstEndpointId);

    /// Add a second test endpoint. Get the capability json for the last endpoint to test pending delete.
    const std::string secondEndpointId = "delete_1";
    auto secondEndpointAttributes = createEndpointAttributes(secondEndpointId);
    auto deleteCapabilityConfigJson = utils::getEndpointConfigJson(secondEndpointAttributes, {capabilityConfig});

    m_capabilitiesDelegate->onConnectionStatusChanged(
        ConnectionStatusObserverInterface::Status::CONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::SUCCESS);

    addEndpoint(secondEndpointAttributes, capabilityConfig);

    /// Add a DiscoveryEventSender to simulate Discovery event in-flight.
    auto discoveryEventSender = std::make_shared<StrictMock<MockDiscoveryEventSender>>();
    EXPECT_CALL(*discoveryEventSender, addDiscoveryStatusObserver(_))
        .WillOnce(Invoke([this](const std::shared_ptr<DiscoveryStatusObserverInterface>& observer) {
            EXPECT_EQ(observer, m_capabilitiesDelegate);
        }));
    EXPECT_CALL(*discoveryEventSender, removeDiscoveryStatusObserver(_))
        .WillOnce(Invoke([this](const std::shared_ptr<DiscoveryStatusObserverInterface>& observer) {
            EXPECT_EQ(observer, m_capabilitiesDelegate);
        }));
    EXPECT_CALL(*discoveryEventSender, stop());
    m_capabilitiesDelegate->setDiscoveryEventSender(discoveryEventSender);

    /// Expect no callback to CapabilitiesObserver, since these endpoints remain in pending.
    EXPECT_CALL(
        *m_mockCapabilitiesDelegateObserver,
        onCapabilitiesStateChange(
            _, _, std::vector<std::string>{firstEndpointId}, std::vector<std::string>{secondEndpointId}))
        .Times(0);

    ASSERT_TRUE(m_capabilitiesDelegate->addOrUpdateEndpoint(firstEndpointAttributes, {capabilityConfig}));
    ASSERT_TRUE(m_capabilitiesDelegate->deleteEndpoint(secondEndpointAttributes, {capabilityConfig}));
}

/**
 * Tests that CapabilitiesDelegate is in the correct state before notifying its observers.
 */
TEST_F(CapabilitiesDelegateTest, test_observerCallingIntoCapabilitiesDelegateOnSuccessNotificationSucceeds) {
    WaitEvent e;

    /// Set-up.
    validateAuthDelegate();

    auto capabilityConfig = createCapabilityConfiguration();

    /// Add an endpoint.
    const std::string endpointId = "delete_1";
    auto endpointAttributes = createEndpointAttributes(endpointId);
    auto capabilityConfigJson = utils::getEndpointConfigJson(endpointAttributes, {capabilityConfig});

    m_capabilitiesDelegate->onConnectionStatusChanged(
        ConnectionStatusObserverInterface::Status::CONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::SUCCESS);

    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .Times(2)
        .WillOnce(Invoke([this](std::shared_ptr<MessageRequest> request) {
            std::string eventCorrelationTokenString;
            getEventCorrelationTokenString(request, eventCorrelationTokenString);

            request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
            m_capabilitiesDelegate->onAlexaEventProcessedReceived(eventCorrelationTokenString);
        }))
        .WillOnce(Invoke([&](std::shared_ptr<MessageRequest> request) {
            request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
        }));

    EXPECT_CALL(*m_mockCapabilitiesStorage, store(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockCapabilitiesStorage, erase(std::unordered_map<std::string, std::string>{}))
        .WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockCapabilitiesStorage,
        erase((std::unordered_map<std::string, std::string>{{endpointId, capabilityConfigJson}})))
        .WillOnce(Return(true));

    /// On the first successful capabilities change, call from observer back into CapabilitiesDelegate.
    EXPECT_CALL(
        *m_mockCapabilitiesDelegateObserver,
        onCapabilitiesStateChange(
            CapabilitiesDelegateObserverInterface::State::SUCCESS,
            CapabilitiesDelegateObserverInterface::Error::SUCCESS,
            _,
            _))
        .Times(2)
        .WillOnce(Invoke([&, this](
                             CapabilitiesDelegateObserverInterface::State state,
                             CapabilitiesDelegateObserverInterface::Error error,
                             std::vector<std::string> addedEndpoints,
                             std::vector<std::string> deletedEndpoints) {
            /// If the below is false with "endpoint not registered", this means that the CapabilitiesDelegate
            /// state is not correct when it notifies its observers.
            ASSERT_TRUE(m_capabilitiesDelegate->deleteEndpoint(endpointAttributes, {capabilityConfig}));
        }))
        .WillOnce(Invoke([&](CapabilitiesDelegateObserverInterface::State state,
                             CapabilitiesDelegateObserverInterface::Error error,
                             std::vector<std::string> addedEndpoints,
                             std::vector<std::string> deletedEndpoints) { e.wakeUp(); }));

    ASSERT_TRUE(m_capabilitiesDelegate->addOrUpdateEndpoint(endpointAttributes, {capabilityConfig}));

    ASSERT_TRUE(e.wait(MY_WAIT_TIMEOUT));
}

/**
 * Test if pending endpoints are not sent while disconnected.
 */
TEST_F(CapabilitiesDelegateTest, test_doNotSendEndpointsWhileDisconnected) {
    /// Set-up.
    m_capabilitiesDelegate->onConnectionStatusChanged(
        ConnectionStatusObserverInterface::Status::DISCONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::SERVER_SIDE_DISCONNECT);
    auto attributes = createEndpointAttributes("add_1");
    auto capabilityConfig = createCapabilityConfiguration();

    /// Strict mock will catch unexpected calls.
    ASSERT_TRUE(m_capabilitiesDelegate->addOrUpdateEndpoint(attributes, {capabilityConfig}));
}

/**
 * Test if pending endpoints are sent on re-connect.
 */
TEST_F(CapabilitiesDelegateTest, test_reconnectTriggersSendPendingEndpoints) {
    /// Set-up.
    WaitEvent e;
    validateAuthDelegate();
    auto capabilityConfig = createCapabilityConfiguration();

    const std::string firstEndpointId = "add_1";
    auto firstEndpointAttributes = createEndpointAttributes(firstEndpointId);

    const std::string secondEndpointId = "add_2";
    auto secondEndpointAttributes = createEndpointAttributes(secondEndpointId);

    /// Add a third test endpoint. Get the capability json for the last endpoint to test pending delete.
    const std::string thirdEndpointId = "delete_1";
    auto thirdEndpointAttributes = createEndpointAttributes(thirdEndpointId);
    auto deleteCapabilityConfigJson = utils::getEndpointConfigJson(thirdEndpointAttributes, {capabilityConfig});
    addEndpoint(thirdEndpointAttributes, capabilityConfig);

    /// Expect calls to MessageSender.
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this](std::shared_ptr<MessageRequest> request) {
            std::string eventCorrelationTokenString;
            getEventCorrelationTokenString(request, eventCorrelationTokenString);

            request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
            m_capabilitiesDelegate->onAlexaEventProcessedReceived(eventCorrelationTokenString);
        }));

    /// Expect calls to storage.
    EXPECT_CALL(*m_mockCapabilitiesStorage, store(_)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockCapabilitiesStorage,
        erase((std::unordered_map<std::string, std::string>{{thirdEndpointId, deleteCapabilityConfigJson}})))
        .WillOnce(Return(true));

    /// Expect calls to CapabilitiesObserver.
    EXPECT_CALL(*m_mockCapabilitiesDelegateObserver, onCapabilitiesStateChange(_, _, _, _))
        .WillOnce(Invoke([&](CapabilitiesDelegateObserverInterface::State state,
                             CapabilitiesDelegateObserverInterface::Error error,
                             std::vector<std::string> addedEndpoints,
                             std::vector<std::string> deletedEndpoints) {
            EXPECT_EQ(state, CapabilitiesDelegateObserverInterface::State::SUCCESS);
            EXPECT_EQ(error, CapabilitiesDelegateObserverInterface::Error::SUCCESS);

            std::sort(addedEndpoints.begin(), addedEndpoints.end());
            EXPECT_EQ(addedEndpoints, (std::vector<std::string>{firstEndpointId, secondEndpointId}));
            EXPECT_EQ(deletedEndpoints, (std::vector<std::string>{thirdEndpointId}));
            e.wakeUp();
        }));

    /// Test.
    m_capabilitiesDelegate->onConnectionStatusChanged(
        ConnectionStatusObserverInterface::Status::DISCONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::SERVER_SIDE_DISCONNECT);

    ASSERT_TRUE(m_capabilitiesDelegate->addOrUpdateEndpoint(firstEndpointAttributes, {capabilityConfig}));
    ASSERT_TRUE(m_capabilitiesDelegate->addOrUpdateEndpoint(secondEndpointAttributes, {capabilityConfig}));
    ASSERT_TRUE(m_capabilitiesDelegate->deleteEndpoint(thirdEndpointAttributes, {capabilityConfig}));

    m_capabilitiesDelegate->onConnectionStatusChanged(
        ConnectionStatusObserverInterface::Status::CONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::SUCCESS);

    ASSERT_TRUE(e.wait(MY_WAIT_TIMEOUT));
}

/**
 * Test if trying to delete an endpoint that is pending in addOrUpdate (and vice versa) results in failure.
 */
TEST_F(CapabilitiesDelegateTest, test_duplicateEndpointInPendingAddOrUpdateAndDeleteShouldFail) {
    /// Set-up.
    auto deleteEndpointAttributes = createEndpointAttributes("delete_1");
    auto addEndpointAttributes = createEndpointAttributes("add_1");
    auto capabilityConfig = createCapabilityConfiguration();

    /// Add an endpoint now so we can test pending delete.
    validateAuthDelegate();
    addEndpoint(deleteEndpointAttributes, capabilityConfig);

    /// Disconnect to force all endpoints into pending.
    m_capabilitiesDelegate->onConnectionStatusChanged(
        ConnectionStatusObserverInterface::Status::DISCONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::SERVER_SIDE_DISCONNECT);

    ASSERT_TRUE(m_capabilitiesDelegate->addOrUpdateEndpoint(addEndpointAttributes, {capabilityConfig}));
    ASSERT_FALSE(m_capabilitiesDelegate->deleteEndpoint(addEndpointAttributes, {capabilityConfig}));

    ASSERT_TRUE(m_capabilitiesDelegate->deleteEndpoint(deleteEndpointAttributes, {capabilityConfig}));
    ASSERT_FALSE(m_capabilitiesDelegate->addOrUpdateEndpoint(deleteEndpointAttributes, {capabilityConfig}));
}

/**
 * Test endpoint registrations.
 * Confirm that changing the registration of an endpoint results in a failure.
 * Also confirm that adding an endpoint with a different registration results in a failure.
 */
TEST_F(CapabilitiesDelegateTest, test_registration) {
    const std::string endpointID1{"TEST_ENDPOINT_ID_1"};
    const std::string endpointID2{"TEST_ENDPOINT_ID_2"};
    const std::string endpointID3{"TEST_ENDPOINT_ID_3"};

    /// Configure endpointId1 attributes with a non-empty registration.
    auto endpointAttributes1 = createEndpointAttributes(endpointID1);
    endpointAttributes1.registration = createEndpointRegistration();

    auto capabilityConfig = createCapabilityConfiguration();

    ASSERT_TRUE(m_capabilitiesDelegate->addOrUpdateEndpoint(endpointAttributes1, {capabilityConfig}));

    /// Try endpointId1 with empty registration.
    auto updatedEndpointAttributes1 = createEndpointAttributes(endpointID1);
    ASSERT_FALSE(m_capabilitiesDelegate->addOrUpdateEndpoint(updatedEndpointAttributes1, {capabilityConfig}));

    /// Configure endpointId2 attributes with a non-empty registration.
    auto endpointAttributes2 = createEndpointAttributes(endpointID2);
    endpointAttributes2.registration = createEndpointRegistration();
    ASSERT_TRUE(m_capabilitiesDelegate->addOrUpdateEndpoint(endpointAttributes2, {capabilityConfig}));

    /// Try endpointId2 with non-empty registration.
    auto updatedEndpointAttributes2 = createEndpointAttributes(endpointID2);
    updatedEndpointAttributes2.registration = createEndpointRegistration("UPDATED_PRODUCT_ID");
    ASSERT_FALSE(m_capabilitiesDelegate->addOrUpdateEndpoint(updatedEndpointAttributes2, {capabilityConfig}));

    /// Try endpointId3 with different and non-empty registration.
    auto updatedEndpointAttributes3 = createEndpointAttributes(endpointID3);
    updatedEndpointAttributes3.registration = createEndpointRegistration("UPDATED_PRODUCT_ID");
    ASSERT_FALSE(m_capabilitiesDelegate->addOrUpdateEndpoint(updatedEndpointAttributes3, {capabilityConfig}));
}

/**
 * Test adding 3 endpoints that share registration information (i.e. they are de-duplicated in the cloud).
 * Verify that other endpoints are sent in the discovery message whenever an endpoint is added.
 * Finally, update the first endpoint that was sent and confirm that all endpoints are sent in the discovery message.
 */
TEST_F(CapabilitiesDelegateTest, test_addAndUpdateOfDeduplicatedEndpoints) {
    m_capabilitiesDelegate->onConnectionStatusChanged(
        ConnectionStatusObserverInterface::Status::CONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::SUCCESS);

    WaitEvent e;
    validateAuthDelegate();

    const std::string endpointID1{"TEST_ENDPOINT_ID_1"};
    const std::string endpointID2{"TEST_ENDPOINT_ID_2"};
    const std::string endpointID3{"TEST_ENDPOINT_ID_3"};

    /// Set-up.
    auto endpointAttributes1 = createEndpointAttributes(endpointID1);
    endpointAttributes1.registration = createEndpointRegistration();

    auto endpointAttributes2 = createEndpointAttributes(endpointID2);
    endpointAttributes2.registration = createEndpointRegistration();

    auto endpointAttributes3 = createEndpointAttributes(endpointID3);
    endpointAttributes3.registration = createEndpointRegistration();

    auto capabilityConfig = createCapabilityConfiguration();

    /// endpointId1 is being registered first. The Discovery message should only contain it.
    /// Expect calls to MessageSender.
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .Times(Exactly(1))
        .WillOnce(Invoke([this](std::shared_ptr<MessageRequest> request) {
            std::string eventCorrelationTokenString;
            getEventCorrelationTokenString(request, eventCorrelationTokenString);

            request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
            m_capabilitiesDelegate->onAlexaEventProcessedReceived(eventCorrelationTokenString);
        }));
    /// Expect call to storage.
    EXPECT_CALL(*m_mockCapabilitiesStorage, store(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockCapabilitiesStorage, erase((std::unordered_map<std::string, std::string>{})))
        .WillOnce(Return(true));

    /// Expect callback to CapabilitiesObserver.
    EXPECT_CALL(*m_mockCapabilitiesDelegateObserver, onCapabilitiesStateChange(_, _, _, _))
        .WillOnce(Invoke([&](CapabilitiesDelegateObserverInterface::State state,
                             CapabilitiesDelegateObserverInterface::Error error,
                             std::vector<std::string> addedEndpoints,
                             std::vector<std::string> deletedEndpoints) {
            EXPECT_EQ(state, CapabilitiesDelegateObserverInterface::State::SUCCESS);
            EXPECT_EQ(error, CapabilitiesDelegateObserverInterface::Error::SUCCESS);
            EXPECT_EQ(addedEndpoints, std::vector<std::string>{endpointID1});
            EXPECT_EQ(deletedEndpoints, (std::vector<std::string>{}));

            e.wakeUp();
        }));
    ASSERT_TRUE(m_capabilitiesDelegate->addOrUpdateEndpoint(endpointAttributes1, {capabilityConfig}));
    ASSERT_TRUE(e.wait(MY_WAIT_TIMEOUT));
    e.reset();

    /// endpointId1 has already been registered. Confirm that it is added when endpointId2 is added.
    /// Expect calls to MessageSender.
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .Times(Exactly(1))
        .WillOnce(Invoke([this](std::shared_ptr<MessageRequest> request) {
            std::string eventCorrelationTokenString;
            getEventCorrelationTokenString(request, eventCorrelationTokenString);

            request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
            m_capabilitiesDelegate->onAlexaEventProcessedReceived(eventCorrelationTokenString);
        }));
    /// Expect call to storage.
    EXPECT_CALL(*m_mockCapabilitiesStorage, store(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockCapabilitiesStorage, erase((std::unordered_map<std::string, std::string>{})))
        .WillOnce(Return(true));

    /// Expect callback to CapabilitiesObserver.
    EXPECT_CALL(*m_mockCapabilitiesDelegateObserver, onCapabilitiesStateChange(_, _, _, _))
        .WillOnce(Invoke([&](CapabilitiesDelegateObserverInterface::State state,
                             CapabilitiesDelegateObserverInterface::Error error,
                             std::vector<std::string> addedEndpoints,
                             std::vector<std::string> deletedEndpoints) {
            std::vector<std::string> expectedAddedEndpoints{endpointID1, endpointID2};
            std::sort(expectedAddedEndpoints.begin(), expectedAddedEndpoints.end());
            std::sort(addedEndpoints.begin(), addedEndpoints.end());
            EXPECT_EQ(state, CapabilitiesDelegateObserverInterface::State::SUCCESS);
            EXPECT_EQ(error, CapabilitiesDelegateObserverInterface::Error::SUCCESS);
            EXPECT_EQ(addedEndpoints, expectedAddedEndpoints);
            EXPECT_EQ(deletedEndpoints, (std::vector<std::string>{}));

            e.wakeUp();
        }));

    ASSERT_TRUE(m_capabilitiesDelegate->addOrUpdateEndpoint(endpointAttributes2, {capabilityConfig}));
    ASSERT_TRUE(e.wait(MY_WAIT_TIMEOUT));
    e.reset();

    /// endpointId1 and endpointId2 have already been registered. Confirm that they are added when endpointId3 is added.
    /// Expect calls to MessageSender.
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .Times(Exactly(1))
        .WillOnce(Invoke([this](std::shared_ptr<MessageRequest> request) {
            std::string eventCorrelationTokenString;
            getEventCorrelationTokenString(request, eventCorrelationTokenString);

            request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
            m_capabilitiesDelegate->onAlexaEventProcessedReceived(eventCorrelationTokenString);
        }));
    /// Expect call to storage.
    EXPECT_CALL(*m_mockCapabilitiesStorage, store(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockCapabilitiesStorage, erase((std::unordered_map<std::string, std::string>{})))
        .WillOnce(Return(true));

    /// Expect callback to CapabilitiesObserver.
    EXPECT_CALL(*m_mockCapabilitiesDelegateObserver, onCapabilitiesStateChange(_, _, _, _))
        .WillOnce(Invoke([&](CapabilitiesDelegateObserverInterface::State state,
                             CapabilitiesDelegateObserverInterface::Error error,
                             std::vector<std::string> addedEndpoints,
                             std::vector<std::string> deletedEndpoints) {
            std::vector<std::string> expectedAddedEndpoints{endpointID1, endpointID2, endpointID3};
            std::sort(expectedAddedEndpoints.begin(), expectedAddedEndpoints.end());
            std::sort(addedEndpoints.begin(), addedEndpoints.end());
            EXPECT_EQ(state, CapabilitiesDelegateObserverInterface::State::SUCCESS);
            EXPECT_EQ(error, CapabilitiesDelegateObserverInterface::Error::SUCCESS);
            EXPECT_EQ(addedEndpoints, expectedAddedEndpoints);
            EXPECT_EQ(deletedEndpoints, (std::vector<std::string>{}));

            e.wakeUp();
        }));

    ASSERT_TRUE(m_capabilitiesDelegate->addOrUpdateEndpoint(endpointAttributes3, {capabilityConfig}));
    ASSERT_TRUE(e.wait(MY_WAIT_TIMEOUT));
    e.reset();

    /// Update the configuration
    std::string additionalAttribute(256 * 10, 'X');
    std::map<std::string, std::string> additionalAttributes;
    additionalAttributes["test"] = "{\"test\":\"" + additionalAttribute + "\"}";
    auto updatedCapabilityConfig = createCapabilityConfiguration(additionalAttributes);

    auto updatedEndpointAttributes1 = createEndpointAttributes(endpointID1);
    updatedEndpointAttributes1.registration = createEndpointRegistration();

    /// endpointId1, endpointId2 and endpointId3 have already been registered. Confirm that they are added when
    /// endpointId1 is updated.
    /// Expect calls to MessageSender.
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .Times(Exactly(1))
        .WillOnce(Invoke([this](std::shared_ptr<MessageRequest> request) {
            std::string eventCorrelationTokenString;
            getEventCorrelationTokenString(request, eventCorrelationTokenString);

            request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
            m_capabilitiesDelegate->onAlexaEventProcessedReceived(eventCorrelationTokenString);
        }));
    /// Expect call to storage.
    EXPECT_CALL(*m_mockCapabilitiesStorage, store(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockCapabilitiesStorage, erase((std::unordered_map<std::string, std::string>{})))
        .WillOnce(Return(true));

    /// Expect callback to CapabilitiesObserver.
    EXPECT_CALL(*m_mockCapabilitiesDelegateObserver, onCapabilitiesStateChange(_, _, _, _))
        .WillOnce(Invoke([&](CapabilitiesDelegateObserverInterface::State state,
                             CapabilitiesDelegateObserverInterface::Error error,
                             std::vector<std::string> addedEndpoints,
                             std::vector<std::string> deletedEndpoints) {
            std::vector<std::string> expectedAddedEndpoints{endpointID1, endpointID2, endpointID3};
            std::sort(expectedAddedEndpoints.begin(), expectedAddedEndpoints.end());
            std::sort(addedEndpoints.begin(), addedEndpoints.end());
            EXPECT_EQ(state, CapabilitiesDelegateObserverInterface::State::SUCCESS);
            EXPECT_EQ(error, CapabilitiesDelegateObserverInterface::Error::SUCCESS);
            EXPECT_EQ(addedEndpoints, expectedAddedEndpoints);
            EXPECT_EQ(deletedEndpoints, (std::vector<std::string>{}));

            e.wakeUp();
        }));

    ASSERT_TRUE(m_capabilitiesDelegate->addOrUpdateEndpoint(updatedEndpointAttributes1, {updatedCapabilityConfig}));
    ASSERT_TRUE(e.wait(MY_WAIT_TIMEOUT));
}

/**
 * Test that deleting a de-duplicated endpoint fails.
 */
TEST_F(CapabilitiesDelegateTest, test_deduplictedDeletionFailure) {
    m_capabilitiesDelegate->onConnectionStatusChanged(
        ConnectionStatusObserverInterface::Status::CONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::SUCCESS);

    WaitEvent e;
    validateAuthDelegate();

    const std::string endpointID1{"TEST_ENDPOINT_ID_1"};
    const std::string endpointID2{"TEST_ENDPOINT_ID_2"};

    auto endpointAttributes1 = createEndpointAttributes(endpointID1);
    endpointAttributes1.registration = createEndpointRegistration();

    auto capabilityConfig = createCapabilityConfiguration();
    auto capabilityConfigJson = utils::getEndpointConfigJson(endpointAttributes1, {capabilityConfig});

    /// endpointId1 is being registered first. The Discovery message should only contain it.
    /// Expect calls to MessageSender.
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .Times(Exactly(1))
        .WillOnce(Invoke([this](std::shared_ptr<MessageRequest> request) {
            std::string eventCorrelationTokenString;
            getEventCorrelationTokenString(request, eventCorrelationTokenString);

            request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
            m_capabilitiesDelegate->onAlexaEventProcessedReceived(eventCorrelationTokenString);
        }));
    /// Expect call to storage.
    EXPECT_CALL(*m_mockCapabilitiesStorage, store(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockCapabilitiesStorage, erase((std::unordered_map<std::string, std::string>{})))
        .WillOnce(Return(true));

    /// Expect callback to CapabilitiesObserver.
    EXPECT_CALL(*m_mockCapabilitiesDelegateObserver, onCapabilitiesStateChange(_, _, _, _))
        .WillOnce(Invoke([&](CapabilitiesDelegateObserverInterface::State state,
                             CapabilitiesDelegateObserverInterface::Error error,
                             std::vector<std::string> addedEndpoints,
                             std::vector<std::string> deletedEndpoints) {
            EXPECT_EQ(state, CapabilitiesDelegateObserverInterface::State::SUCCESS);
            EXPECT_EQ(error, CapabilitiesDelegateObserverInterface::Error::SUCCESS);
            EXPECT_EQ(addedEndpoints, std::vector<std::string>{endpointID1});
            EXPECT_EQ(deletedEndpoints, (std::vector<std::string>{}));

            e.wakeUp();
        }));
    ASSERT_TRUE(m_capabilitiesDelegate->addOrUpdateEndpoint(endpointAttributes1, {capabilityConfig}));
    ASSERT_TRUE(e.wait(MY_WAIT_TIMEOUT));
    e.reset();

    /// Deleting endpoint 1 should fail.
    ASSERT_FALSE(m_capabilitiesDelegate->deleteEndpoint(endpointAttributes1, {capabilityConfig}));
}

/**
 * Test adding two pairs of 3 endpoints that share registration information (i.e. they are de-duplicated in the cloud).
 * For this test, endpointid3 and endpointId6 have large configurations, preventing them from being sent in the same
 * message.
 * Verify that the endpoints are split into two messages, each containing 3 endpoints.
 */
TEST_F(CapabilitiesDelegateTest, test_SplitMessagePendingDeduplicatedEndpoints) {
    m_capabilitiesDelegate->onConnectionStatusChanged(
        ConnectionStatusObserverInterface::Status::DISCONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::SERVER_SIDE_DISCONNECT);

    WaitEvent e;
    validateAuthDelegate();

    const std::string endpointID1{"TEST_ENDPOINT_ID_1"};
    const std::string endpointID2{"TEST_ENDPOINT_ID_2"};
    const std::string endpointID3{"TEST_ENDPOINT_ID_3"};

    /// Set-up the first set of deduplicated endpoints.
    auto endpointAttributes1 = createEndpointAttributes(endpointID1);
    endpointAttributes1.registration = createEndpointRegistration();

    auto endpointAttributes2 = createEndpointAttributes(endpointID2);
    endpointAttributes2.registration = createEndpointRegistration();

    auto endpointAttributes3 = createEndpointAttributes(endpointID3);
    endpointAttributes3.registration = createEndpointRegistration();

    const std::string endpointID4{"TEST_ENDPOINT_ID_4"};
    const std::string endpointID5{"TEST_ENDPOINT_ID_5"};
    const std::string endpointID6{"TEST_ENDPOINT_ID_6"};

    /// Set-up the second set of endpoints.
    auto endpointAttributes4 = createEndpointAttributes(endpointID4);

    auto endpointAttributes5 = createEndpointAttributes(endpointID5);

    auto endpointAttributes6 = createEndpointAttributes(endpointID6);

    /// Create a large capability configuration for endpoint3 and endpoint6.
    std::string additionalAttribute(240 * 1024, 'X');
    std::map<std::string, std::string> additionalAttributes;
    additionalAttributes["test"] = "{\"test\":\"" + additionalAttribute + "\"}";
    auto largeCapabilityConfig = createCapabilityConfiguration(additionalAttributes);

    /// Create a default capability configuration for the other endpoints.
    auto capabilityConfig = createCapabilityConfiguration();

    ASSERT_TRUE(m_capabilitiesDelegate->addOrUpdateEndpoint(endpointAttributes1, {capabilityConfig}));

    ASSERT_TRUE(m_capabilitiesDelegate->addOrUpdateEndpoint(endpointAttributes2, {capabilityConfig}));

    ASSERT_TRUE(m_capabilitiesDelegate->addOrUpdateEndpoint(endpointAttributes3, {largeCapabilityConfig}));

    ASSERT_TRUE(m_capabilitiesDelegate->addOrUpdateEndpoint(endpointAttributes4, {capabilityConfig}));

    ASSERT_TRUE(m_capabilitiesDelegate->addOrUpdateEndpoint(endpointAttributes5, {capabilityConfig}));

    ASSERT_TRUE(m_capabilitiesDelegate->addOrUpdateEndpoint(endpointAttributes6, {largeCapabilityConfig}));

    /// Upon connect, expect two messages to be sent. One with endpoints 1,2 and 3, and the other with endpoints 4, 5
    /// and 6. Expect calls to MessageSender.
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .Times(Exactly(2))
        .WillRepeatedly(Invoke([this](std::shared_ptr<MessageRequest> request) {
            std::string eventCorrelationTokenString;
            getEventCorrelationTokenString(request, eventCorrelationTokenString);

            request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
            m_capabilitiesDelegate->onAlexaEventProcessedReceived(eventCorrelationTokenString);
        }));
    /// Expect call to storage.
    EXPECT_CALL(*m_mockCapabilitiesStorage, store(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockCapabilitiesStorage, erase((std::unordered_map<std::string, std::string>{})))
        .WillRepeatedly(Return(true));

    /// Expect callback to CapabilitiesObserver.
    std::vector<std::vector<std::string>> expectedAddedEndpoints{{endpointID1, endpointID2, endpointID3},
                                                                 {endpointID4, endpointID5, endpointID6}};
    int other{0};
    EXPECT_CALL(*m_mockCapabilitiesDelegateObserver, onCapabilitiesStateChange(_, _, _, _))
        .Times(Exactly(2))
        .WillOnce(Invoke([&](CapabilitiesDelegateObserverInterface::State state,
                             CapabilitiesDelegateObserverInterface::Error error,
                             std::vector<std::string> addedEndpoints,
                             std::vector<std::string> deletedEndpoints) {
            std::sort(addedEndpoints.begin(), addedEndpoints.end());
            EXPECT_EQ(state, CapabilitiesDelegateObserverInterface::State::SUCCESS);
            EXPECT_EQ(error, CapabilitiesDelegateObserverInterface::Error::SUCCESS);
            if (addedEndpoints[0] == endpointID1) {
                other = 1;
                EXPECT_EQ(addedEndpoints, expectedAddedEndpoints[0]);
            } else {
                EXPECT_EQ(addedEndpoints, expectedAddedEndpoints[1]);
            }
            EXPECT_EQ(deletedEndpoints, (std::vector<std::string>{}));
        }))
        .WillOnce(Invoke([&](CapabilitiesDelegateObserverInterface::State state,
                             CapabilitiesDelegateObserverInterface::Error error,
                             std::vector<std::string> addedEndpoints,
                             std::vector<std::string> deletedEndpoints) {
            std::sort(addedEndpoints.begin(), addedEndpoints.end());
            EXPECT_EQ(state, CapabilitiesDelegateObserverInterface::State::SUCCESS);
            EXPECT_EQ(error, CapabilitiesDelegateObserverInterface::Error::SUCCESS);
            EXPECT_EQ(addedEndpoints, expectedAddedEndpoints[other]);
            EXPECT_EQ(deletedEndpoints, (std::vector<std::string>{}));
            e.wakeUp();
        }));

    m_capabilitiesDelegate->onConnectionStatusChanged(
        ConnectionStatusObserverInterface::Status::CONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::SUCCESS);

    ASSERT_TRUE(e.wait(MY_WAIT_TIMEOUT));
}

/**
 * Test limits of the Alexa.Discovery interface.
 * Specifically test :
 * 1. Adding more than 300 deduplicated endpoints triggers a failure.
 * 2. Having more than 100 capabilities per deduplicated endpoint triggers a failure.
 * This is because a single discovery message cannot contain more than 300 endpoints, per the limits imposed here:
 * https://developer.amazon.com/en-US/docs/alexa/device-apis/alexa-discovery.html#limits
 */
TEST_F(CapabilitiesDelegateTest, test_endpointLimits) {
    // Create deduplicated endpoints
    for (size_t i = 0; i < getMaxEndpoints(); ++i) {
        const std::string testEndpointID = "TEST_ENDPOINT_ID_" + std::to_string(i);

        auto endpointAttributes = createEndpointAttributes(testEndpointID);
        endpointAttributes.registration = createEndpointRegistration();
        auto capabilityConfig = createCapabilityConfiguration();

        ASSERT_TRUE(m_capabilitiesDelegate->addOrUpdateEndpoint(endpointAttributes, {capabilityConfig}));
    }
    const std::string lastEndpointID = "TEST_ENDPOINT_ID_LAST";
    auto lastEndpointAttributes = createEndpointAttributes(lastEndpointID);
    lastEndpointAttributes.registration = createEndpointRegistration();
    auto lastCapabilityConfig = createCapabilityConfiguration();

    ASSERT_FALSE(m_capabilitiesDelegate->addOrUpdateEndpoint(lastEndpointAttributes, {lastCapabilityConfig}));

    const std::string endpointID = "TEST_ENDPOINT_ID";

    /// Add a deduplicated endpoint with more than MAX_CAPABILITIES_PER_ENDPOINT capabilities.
    auto endpointAttributes = createEndpointAttributes(endpointID);
    endpointAttributes.registration = createEndpointRegistration();
    std::vector<CapabilityConfiguration> largeConfig;
    for (size_t i = 0; i <= getMaxCapabilitiesPerEndpoint(); ++i) {
        largeConfig.push_back(createCapabilityConfiguration());
    }

    ASSERT_FALSE(m_capabilitiesDelegate->addOrUpdateEndpoint(endpointAttributes, largeConfig));
}

/**
 * Test updating a deduplicated endpoint when it is in flight.
 */
TEST_F(CapabilitiesDelegateTest, test_updateDeduplicatedEndpointWhenInflight) {
    m_capabilitiesDelegate->onConnectionStatusChanged(
        ConnectionStatusObserverInterface::Status::CONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::SUCCESS);
    WaitEvent e;
    validateAuthDelegate();

    const std::string endpointID{"TEST_ENDPOINT_ID"};

    /// Set-up.
    auto endpointAttributes = createEndpointAttributes(endpointID);
    endpointAttributes.registration = createEndpointRegistration();

    auto capabilityConfig = createCapabilityConfiguration();

    addEndpoint(endpointAttributes, {capabilityConfig});

    /// Add a DiscoveryEventSender to simulate Discovery event in-flight.
    auto discoveryEventSender = std::make_shared<StrictMock<MockDiscoveryEventSender>>();
    EXPECT_CALL(*discoveryEventSender, addDiscoveryStatusObserver(_))
        .WillOnce(Invoke([this](const std::shared_ptr<DiscoveryStatusObserverInterface>& observer) {
            EXPECT_EQ(observer, m_capabilitiesDelegate);
        }));
    EXPECT_CALL(*discoveryEventSender, removeDiscoveryStatusObserver(_))
        .WillOnce(Invoke([this](const std::shared_ptr<DiscoveryStatusObserverInterface>& observer) {
            EXPECT_EQ(observer, m_capabilitiesDelegate);
        }));
    EXPECT_CALL(*discoveryEventSender, stop());
    m_capabilitiesDelegate->setDiscoveryEventSender(discoveryEventSender);

    /// Create an updated capability configuration for the update to endpoint1.
    std::string additionalAttribute(2 * 1024, 'X');
    std::map<std::string, std::string> additionalAttributes;
    additionalAttributes["test"] = "{\"test\":\"" + additionalAttribute + "\"}";
    auto updatedCapabilityConfig = createCapabilityConfiguration(additionalAttributes);

    /// Expect no callback to CapabilitiesObserver, since the update remains in pending.
    EXPECT_CALL(
        *m_mockCapabilitiesDelegateObserver,
        onCapabilitiesStateChange(_, _, std::vector<std::string>{endpointID}, std::vector<std::string>{}))
        .Times(0);

    /// endpointId1 is inflight. Should be able to add it again to the pending update set.
    ASSERT_TRUE(m_capabilitiesDelegate->addOrUpdateEndpoint(endpointAttributes, {updatedCapabilityConfig}));
}

/**
 * Test adding a deduplicated endpoint when another one is in flight.
 * Verify that the first endpoint is also sent in the discovery message when the second endpoint is added.
 */
TEST_F(CapabilitiesDelegateTest, test_addDeduplicatedEndpointWhenOtherInflight) {
    m_capabilitiesDelegate->onConnectionStatusChanged(
        ConnectionStatusObserverInterface::Status::CONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::SUCCESS);

    WaitEvent e;
    validateAuthDelegate();

    const std::string endpointID1{"TEST_ENDPOINT_ID_1"};
    const std::string endpointID2{"TEST_ENDPOINT_ID_2"};

    /// Set-up.
    auto endpointAttributes1 = createEndpointAttributes(endpointID1);
    endpointAttributes1.registration = createEndpointRegistration();

    auto endpointAttributes2 = createEndpointAttributes(endpointID2);
    endpointAttributes2.registration = createEndpointRegistration();

    auto capabilityConfig = createCapabilityConfiguration();

    addEndpoint(endpointAttributes1, {capabilityConfig});

    /// Add a DiscoveryEventSender to simulate Discovery event in-flight.
    auto discoveryEventSender = std::make_shared<StrictMock<MockDiscoveryEventSender>>();
    EXPECT_CALL(*discoveryEventSender, addDiscoveryStatusObserver(_))
        .WillOnce(Invoke([this](const std::shared_ptr<DiscoveryStatusObserverInterface>& observer) {
            EXPECT_EQ(observer, m_capabilitiesDelegate);
        }));
    EXPECT_CALL(*discoveryEventSender, removeDiscoveryStatusObserver(_))
        .WillOnce(Invoke([this](const std::shared_ptr<DiscoveryStatusObserverInterface>& observer) {
            EXPECT_EQ(observer, m_capabilitiesDelegate);
        }));
    EXPECT_CALL(*discoveryEventSender, stop());
    m_capabilitiesDelegate->setDiscoveryEventSender(discoveryEventSender);

    /// Expect no callback to CapabilitiesObserver, since the add of endpoint2 remains in pending.
    EXPECT_CALL(
        *m_mockCapabilitiesDelegateObserver,
        onCapabilitiesStateChange(_, _, std::vector<std::string>{endpointID1, endpointID2}, std::vector<std::string>{}))
        .Times(0);

    /// Endpoint1 is in flight. Confirm that Endpoint2 is added to the pending endpoints.
    ASSERT_TRUE(m_capabilitiesDelegate->addOrUpdateEndpoint(endpointAttributes2, {capabilityConfig}));
}

}  // namespace test
}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK
