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
#include <thread>
#include <gmock/gmock.h>

#include <AVSCommon/SDKInterfaces/MockPostConnectSendMessage.h>
#include <AVSCommon/SDKInterfaces/PostConnectOperationInterface.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/PromiseFuturePair.h>
#include <CapabilitiesDelegate/PostConnectCapabilitiesPublisher.h>

#include "MockAuthDelegate.h"

namespace alexaClientSDK {
namespace capabilitiesDelegate {
namespace test {

using namespace ::testing;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils;
using namespace avsCommon::utils::json::jsonUtils;

/// The Discovery event Namsepace.
static const std::string DISCOVERY_NAMESPACE = "Alexa.Discovery";
/// The AddOrUpdateReport Namespace.
static const std::string ADD_OR_UPDATE_REPORT_NAME = "AddOrUpdateReport";
/// The DeleteReport Namespace.
static const std::string DELETE_REPORT_NAME = "DeleteReport";
/// The payload version of the discovery event.
static const std::string DISCOVERY_PAYLOAD_VERSION = "3";
/// The scope key in the discovery event.
static const std::string SCOPE_KEY = "scope";
/// The scope key in the discovery event.
static const std::string TYPE_KEY = "type";
/// The scope key in the discovery event.
static const std::string BEARER_TOKEN_KEY = "bearerToken";
/// The scope key in the discovery event.
static const std::string TOKEN_KEY = "token";
/// The scope key in the discovery event.
static const std::string ENDPOINTS_KEY = "endpoints";
/// The scope key in the discovery event.
static const std::string ENDPOINTID_KEY = "endpointId";
/// The event key in the discovery event.
static const std::string EVENT_KEY = "event";
/// The header key in the discovery event.
static const std::string HEADER_KEY = "header";
/// The namespace  key in the discovery event.
static const std::string NAMESPACE_KEY = "namespace";
/// The name key in the discovery event.
static const std::string NAME_KEY = "name";
/// The payload version key in the discovery event.
static const std::string PAYLOAD_VERSION_KEY = "payloadVersion";
/// The eventCorrelationToken key in the discovery event.
static const std::string EVENT_CORRELATION_TOKEN_KEY = "eventCorrelationToken";
/// The payload key in the discovery event.
static const std::string PAYLOAD_KEY = "payload";
/// Test string for auth token.
static const std::string TEST_AUTH_TOKEN = "TEST_AUTH_TOKEN";
/// Test string for endpointID 1
static const std::string TEST_ENDPOINT_ID_1 = "1";
/// Test string for endpointId 2
static const std::string TEST_ENDPOINT_ID_2 = "2";
/// The test endpointId to endpointConfig map AddOrUpdateReport event endpoints.
static const std::unordered_map<std::string, std::string> TEST_ADD_OR_UPDATE_ENDPOINTS = {
    {TEST_ENDPOINT_ID_1, R"({"endpointId":")" + TEST_ENDPOINT_ID_1 + R"("})"}};
/// The test endpointId to endpointConfig map DeleteReport event endpoints.
static const std::unordered_map<std::string, std::string> TEST_DELETE_ENDPOINTS = {
    {TEST_ENDPOINT_ID_2, R"({"endpointId":")" + TEST_ENDPOINT_ID_2 + R"("})"}};
/// The expected AddOrUpdateReport endpoint Ids.
static const std::vector<std::string> EXPECTED_ADD_OR_UPDATE_ENDPOINT_IDS = {TEST_ENDPOINT_ID_1};
/// The expected DeleteReport endpoint Ids.
static const std::vector<std::string> EXPECTED_DELETE_ENDPOINT_IDS = {TEST_ENDPOINT_ID_2};
/// The test string for eventCorrelationToken sent in headers.
static const std::string TEST_EVENT_CORRELATION_TOKEN = "TEST_EVENT_CORRELATION_TOKEN";
/// The test retry count.
static const int TEST_RETRY_COUNT = 2;
/// Maximum number of endpoints that can be sent in a Discovery event.
static const int MAX_ENDPOINTS_PER_EVENT = 300;

/**
 * Structure to store event data from a discovery event JSON.
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
 * Parses the auth token sent in the Discovery event and stores the data in the @c EventData structure.
 *
 * @param payloadString The payload of the discovery event.
 * @param eventData The pointer to the @c EventData structure.
 * @return True if parsing is successful, else false.
 */
bool parseAuthToken(const std::string& payloadString, EventData* eventData) {
    std::string scopeString;
    if (!retrieveValue(payloadString, "scope", &scopeString)) {
        return false;
    }

    std::string bearerToken;
    if (!retrieveValue(scopeString, "type", &bearerToken)) {
        return false;
    }

    if (bearerToken != "BearerToken") {
        return false;
    }

    if (!retrieveValue(scopeString, "token", &eventData->authToken)) {
        return false;
    }

    return true;
}

/**
 * Parses the event JSON and stores the data in the @c EventData structure.
 *
 * @param payloadString The payload string.
 * @param eventData The pointer to the @c EventData structure.
 * @return True if parsing is successful, else false.
 */
bool parseEndpointsIds(const std::string& payloadString, EventData* eventData) {
    rapidjson::Document document;
    document.Parse(payloadString);

    if (document.HasParseError()) {
        return false;
    }

    if (!jsonArrayExists(document, ENDPOINTS_KEY)) {
        return false;
    }

    auto endpointsJsonArray = document.FindMember(ENDPOINTS_KEY)->value.GetArray();
    for (rapidjson::SizeType i = 0; i < endpointsJsonArray.Size(); ++i) {
        std::string endpointId;
        if (!retrieveValue(endpointsJsonArray[i], ENDPOINTID_KEY, &endpointId)) {
            return false;
        }
        eventData->endpointIdsInPayload.push_back(endpointId);
    }

    return true;
}

/**
 * Parses the event JSON and stores the data in the @c EventData structure.
 *
 * @param eventJson The event data.
 * @param eventData The pointer to the @c EventData structure.
 * @return True if the parsing is successful, else false.
 */
bool parseEventJson(const std::string& eventJson, EventData* eventData) {
    std::string eventString;
    if (!retrieveValue(eventJson, EVENT_KEY, &eventString)) {
        return false;
    }

    std::string headerString;
    if (!retrieveValue(eventString, HEADER_KEY, &headerString)) {
        return false;
    }

    if (!retrieveValue(headerString, NAMESPACE_KEY, &eventData->namespaceString)) {
        return false;
    }

    if (!retrieveValue(headerString, NAME_KEY, &eventData->nameString)) {
        return false;
    }

    if (!retrieveValue(headerString, PAYLOAD_VERSION_KEY, &eventData->payloadVersionString)) {
        return false;
    }

    if (!retrieveValue(headerString, EVENT_CORRELATION_TOKEN_KEY, &eventData->eventCorrelationTokenString)) {
        return false;
    }

    std::string payloadString;
    if (!retrieveValue(eventString, PAYLOAD_KEY, &payloadString)) {
        return false;
    }

    if (!parseAuthToken(payloadString, eventData)) {
        return false;
    }

    if (!parseEndpointsIds(payloadString, eventData)) {
        return false;
    }

    return true;
}

/**
 * Validates if the event data structure contains expected discovery event fields.
 * @param eventData The @c EventData containing the parsed discovery fields
 * @param eventName The name of the discovery event to validate against.
 * @param endpointIds The optional endpointIds list to be validated against the payload.
 */
void validateDiscoveryEvent(
    const EventData& eventData,
    const std::string& eventName,
    const std::vector<std::string>& endpointIds = std::vector<std::string>()) {
    ASSERT_EQ(eventData.namespaceString, DISCOVERY_NAMESPACE);
    ASSERT_EQ(eventData.nameString, eventName);
    ASSERT_EQ(eventData.payloadVersionString, DISCOVERY_PAYLOAD_VERSION);
    ASSERT_EQ(eventData.authToken, TEST_AUTH_TOKEN);
    if (!endpointIds.empty()) {
        ASSERT_EQ(eventData.endpointIdsInPayload, endpointIds);
    }
}

/**
 * Mock class for the @c DiscoveryStatusObserverInterface.
 */
class MockDiscoveryStatusObserver : public DiscoveryStatusObserverInterface {
public:
    MOCK_METHOD2(
        onDiscoveryCompleted,
        void(const std::unordered_map<std::string, std::string>&, const std::unordered_map<std::string, std::string>&));
    MOCK_METHOD1(onDiscoveryFailure, void(MessageRequestObserverInterface::Status));
};

/**
 * Test harness for @c PostConnectCapabilitiesPublisher class.
 */
class PostConnectCapabilitiesPublisherTest : public Test {
public:
    /// Setup the test harness for running the test.
    void SetUp() override;

    /// Clean up the test harness after running the test.
    void TearDown() override;

protected:
    /**
     * Validates if the expected methods get called on the auth delegate.
     *
     * @param expectAuthFailure True if a call to onAuthFailure() is expected, else false.
     */
    void validateCallsToAuthDelegate(bool expectAuthFailure = false);

    /// The mock @c PostConnectSendMessage used in the tests.
    std::shared_ptr<MockPostConnectSendMessage> m_mockPostConnectSendMessage;

    /// The instance of the @c AuthDelegate used in the tests.
    std::shared_ptr<MockAuthDelegate> m_mockAuthDelegate;

    /// The instance of the @c DiscoveryObserverStatusInterface used in the tests.
    std::shared_ptr<MockDiscoveryStatusObserver> m_mockDiscoveryStatusObserver;

    /// The instance of the @c PostConnectCapabilitiesPublisher to be tested.
    std::shared_ptr<PostConnectCapabilitiesPublisher> m_postConnectCapabilitiesPublisher;
};

void PostConnectCapabilitiesPublisherTest::SetUp() {
    m_mockPostConnectSendMessage = std::make_shared<StrictMock<MockPostConnectSendMessage>>();
    m_mockAuthDelegate = std::make_shared<StrictMock<MockAuthDelegate>>();
    m_mockDiscoveryStatusObserver = std::make_shared<StrictMock<MockDiscoveryStatusObserver>>();

    m_postConnectCapabilitiesPublisher = PostConnectCapabilitiesPublisher::create(
        TEST_ADD_OR_UPDATE_ENDPOINTS, TEST_DELETE_ENDPOINTS, m_mockAuthDelegate);
    m_postConnectCapabilitiesPublisher->addDiscoveryStatusObserver(m_mockDiscoveryStatusObserver);
}

void PostConnectCapabilitiesPublisherTest::TearDown() {
}

void PostConnectCapabilitiesPublisherTest::validateCallsToAuthDelegate(bool expectAuthFailure) {
    EXPECT_CALL(*m_mockAuthDelegate, addAuthObserver(_)).WillOnce(InvokeWithoutArgs([this]() {
        m_postConnectCapabilitiesPublisher->onAuthStateChange(
            AuthObserverInterface::State::REFRESHED, AuthObserverInterface::Error::SUCCESS);
    }));
    EXPECT_CALL(*m_mockAuthDelegate, getAuthToken()).WillRepeatedly(Return(TEST_AUTH_TOKEN));
    EXPECT_CALL(*m_mockAuthDelegate, removeAuthObserver(_)).WillOnce(Return());
    if (expectAuthFailure) {
        EXPECT_CALL(*m_mockAuthDelegate, onAuthFailure(TEST_AUTH_TOKEN));
    }
}

/**
 * Test create fails with invalid parameters
 */
TEST_F(PostConnectCapabilitiesPublisherTest, test_createWithInvalidParams) {
    auto instance =
        PostConnectCapabilitiesPublisher::create(TEST_ADD_OR_UPDATE_ENDPOINTS, TEST_DELETE_ENDPOINTS, nullptr);
    ASSERT_EQ(instance, nullptr);

    std::unordered_map<std::string, std::string> addOrUpdateEndpoints, deleteEndpoints;
    instance = PostConnectCapabilitiesPublisher::create(addOrUpdateEndpoints, deleteEndpoints, m_mockAuthDelegate);
    ASSERT_EQ(instance, nullptr);
}

/**
 * Test getPriority method returns expected value.
 */
TEST_F(PostConnectCapabilitiesPublisherTest, test_getPostConnectOperationPriority) {
    ASSERT_EQ(
        static_cast<unsigned int>(PostConnectOperationInterface::ENDPOINT_DISCOVERY_PRIORITY),
        m_postConnectCapabilitiesPublisher->getOperationPriority());
}

/**
 * Test that performOperation() fails if the PostConnectSendMessage is invalid.
 */
TEST_F(PostConnectCapabilitiesPublisherTest, test_performOperationWithInvalidPostConnectSender) {
    ASSERT_FALSE(m_postConnectCapabilitiesPublisher->performOperation(nullptr));
}

/**
 * Test Happy Path, if AddOrUpdateReport event and DeleteReport event get sent out.
 */
TEST_F(PostConnectCapabilitiesPublisherTest, test_performOperationSendsDiscoveryEvents) {
    validateCallsToAuthDelegate();

    auto sendAddOrUpdateReport = [this](std::shared_ptr<MessageRequest> request) {
        EventData eventData;
        ASSERT_TRUE(parseEventJson(request->getJsonContent(), &eventData));
        validateDiscoveryEvent(eventData, ADD_OR_UPDATE_REPORT_NAME, EXPECTED_ADD_OR_UPDATE_ENDPOINT_IDS);
        request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);

        m_postConnectCapabilitiesPublisher->onAlexaEventProcessedReceived(eventData.eventCorrelationTokenString);
    };

    auto sendDeleteReport = [](std::shared_ptr<MessageRequest> request) {
        EventData eventData;
        ASSERT_TRUE(parseEventJson(request->getJsonContent(), &eventData));
        validateDiscoveryEvent(eventData, DELETE_REPORT_NAME, EXPECTED_DELETE_ENDPOINT_IDS);
        request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
    };

    EXPECT_CALL(*m_mockPostConnectSendMessage, sendPostConnectMessage(_))
        .WillOnce(Invoke(sendAddOrUpdateReport))
        .WillOnce(Invoke(sendDeleteReport));

    EXPECT_CALL(
        *m_mockDiscoveryStatusObserver, onDiscoveryCompleted(TEST_ADD_OR_UPDATE_ENDPOINTS, TEST_DELETE_ENDPOINTS))
        .WillOnce(Return());

    ASSERT_TRUE(m_postConnectCapabilitiesPublisher->performOperation(m_mockPostConnectSendMessage));
}

/**
 * Test second call to performOperation fails when executed twice.
 */
TEST_F(PostConnectCapabilitiesPublisherTest, test_performOperationFailsWhenCalledTwice) {
    validateCallsToAuthDelegate();
    auto handleAddOrUpdateReport = [this](std::shared_ptr<MessageRequest> request) {
        EventData eventData;
        ASSERT_TRUE(parseEventJson(request->getJsonContent(), &eventData));
        validateDiscoveryEvent(eventData, ADD_OR_UPDATE_REPORT_NAME, EXPECTED_ADD_OR_UPDATE_ENDPOINT_IDS);
        request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);

        m_postConnectCapabilitiesPublisher->onAlexaEventProcessedReceived(eventData.eventCorrelationTokenString);
    };

    auto handleDeleteReport = [](std::shared_ptr<MessageRequest> request) {
        EventData eventData;
        ASSERT_TRUE(parseEventJson(request->getJsonContent(), &eventData));
        validateDiscoveryEvent(eventData, DELETE_REPORT_NAME, EXPECTED_DELETE_ENDPOINT_IDS);
        request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
    };

    EXPECT_CALL(*m_mockPostConnectSendMessage, sendPostConnectMessage(_))
        .WillOnce(Invoke(handleAddOrUpdateReport))
        .WillOnce(Invoke(handleDeleteReport));

    EXPECT_CALL(
        *m_mockDiscoveryStatusObserver, onDiscoveryCompleted(TEST_ADD_OR_UPDATE_ENDPOINTS, TEST_DELETE_ENDPOINTS))
        .WillOnce(Return());

    ASSERT_TRUE(m_postConnectCapabilitiesPublisher->performOperation(m_mockPostConnectSendMessage));

    ASSERT_FALSE(m_postConnectCapabilitiesPublisher->performOperation(m_mockPostConnectSendMessage));
}

/**
 * This method returns the number of Discovery Events the given endpoints will be split into.
 *
 * @param numEndpoints The totol number of endpoints
 * @return The number of discovery events these endpoints will be split into.
 */
static int getExpecetedNumberOfDiscoveryEvents(int numEndpoints) {
    return numEndpoints / MAX_ENDPOINTS_PER_EVENT + ((numEndpoints % MAX_ENDPOINTS_PER_EVENT) != 0);
}

/**
 * Test if multiple discovery events are sent if the number of endpoints is greater than MAX endpoints.
 */
TEST_F(PostConnectCapabilitiesPublisherTest, test_performOperationSplitsEventsWithMaxEndpoints) {
    std::unordered_map<std::string, std::string> addOrUpdateReportEndpoints, deleteReportEndpoints;
    std::string endpointIdPrefix = "ENDPOINT_ID_";
    int testNumAddOrUpdateReportEndpoints = 1400;
    int testNumDeleteReportEndpoints = 299;

    for (int i = 1; i <= testNumAddOrUpdateReportEndpoints; ++i) {
        std::string endpointId = endpointIdPrefix + std::to_string(i);
        std::string endpointIdConfig = "{\"endpointId\":\"" + std::to_string(i) + "\"}";
        addOrUpdateReportEndpoints.insert({endpointId, endpointIdConfig});
    }

    for (int i = 1; i <= testNumDeleteReportEndpoints; ++i) {
        std::string endpointId = endpointIdPrefix + std::to_string(i);
        std::string endpointIdConfig = "{\"endpointId\":\"" + std::to_string(i) + "\"}";
        deleteReportEndpoints.insert({endpointId, endpointIdConfig});
    }

    auto postConnectCapabilitiesPublisher =
        PostConnectCapabilitiesPublisher::create(addOrUpdateReportEndpoints, deleteReportEndpoints, m_mockAuthDelegate);

    postConnectCapabilitiesPublisher->addDiscoveryStatusObserver(m_mockDiscoveryStatusObserver);

    auto handleAddOrUpdateReport = [postConnectCapabilitiesPublisher](std::shared_ptr<MessageRequest> request) {
        EventData eventData;
        ASSERT_TRUE(parseEventJson(request->getJsonContent(), &eventData));
        validateDiscoveryEvent(eventData, ADD_OR_UPDATE_REPORT_NAME);

        request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
        postConnectCapabilitiesPublisher->onAlexaEventProcessedReceived(eventData.eventCorrelationTokenString);
    };

    auto handleDeleteReport = [](std::shared_ptr<MessageRequest> request) {
        EventData eventData;
        ASSERT_TRUE(parseEventJson(request->getJsonContent(), &eventData));
        validateDiscoveryEvent(eventData, DELETE_REPORT_NAME);
        request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
    };

    int expectedNumOfAddOrUpdateReportEvents = getExpecetedNumberOfDiscoveryEvents(testNumAddOrUpdateReportEndpoints);
    int expectedNumOfDeleteReportEvents = getExpecetedNumberOfDiscoveryEvents(testNumDeleteReportEndpoints);
    {
        InSequence s;
        EXPECT_CALL(*m_mockPostConnectSendMessage, sendPostConnectMessage(_))
            .Times(expectedNumOfAddOrUpdateReportEvents)
            .WillRepeatedly(Invoke(handleAddOrUpdateReport));
        EXPECT_CALL(*m_mockPostConnectSendMessage, sendPostConnectMessage(_))
            .Times(expectedNumOfDeleteReportEvents)
            .WillRepeatedly(Invoke(handleDeleteReport));
    }

    EXPECT_CALL(*m_mockDiscoveryStatusObserver, onDiscoveryCompleted(addOrUpdateReportEndpoints, deleteReportEndpoints))
        .WillOnce(Return());

    EXPECT_CALL(*m_mockAuthDelegate, addAuthObserver(_))
        .WillOnce(InvokeWithoutArgs([&postConnectCapabilitiesPublisher]() {
            postConnectCapabilitiesPublisher->onAuthStateChange(
                AuthObserverInterface::State::REFRESHED, AuthObserverInterface::Error::SUCCESS);
        }));
    EXPECT_CALL(*m_mockAuthDelegate, getAuthToken()).WillRepeatedly(Return(TEST_AUTH_TOKEN));
    EXPECT_CALL(*m_mockAuthDelegate, removeAuthObserver(_)).WillRepeatedly(Return());

    ASSERT_TRUE(postConnectCapabilitiesPublisher->performOperation(m_mockPostConnectSendMessage));

    /// cleanup
    postConnectCapabilitiesPublisher->removeDiscoveryStatusObserver(m_mockDiscoveryStatusObserver);
}

/**
 * Test when AddOrUpdateReport response is 202 and DeleteReport response is 4xx.
 */
TEST_F(PostConnectCapabilitiesPublisherTest, test_deleteReportEventReceives4xxResponse) {
    validateCallsToAuthDelegate(true);
    auto handleAddOrUpdateReport = [this](std::shared_ptr<MessageRequest> request) {
        EventData eventData;
        ASSERT_TRUE(parseEventJson(request->getJsonContent(), &eventData));
        validateDiscoveryEvent(eventData, ADD_OR_UPDATE_REPORT_NAME, EXPECTED_ADD_OR_UPDATE_ENDPOINT_IDS);
        request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
        m_postConnectCapabilitiesPublisher->onAlexaEventProcessedReceived(eventData.eventCorrelationTokenString);
    };

    auto handleDeleteReport = [](std::shared_ptr<MessageRequest> request) {
        EventData eventData;
        ASSERT_TRUE(parseEventJson(request->getJsonContent(), &eventData));
        validateDiscoveryEvent(eventData, DELETE_REPORT_NAME, EXPECTED_DELETE_ENDPOINT_IDS);
        request->sendCompleted(MessageRequestObserverInterface::Status::INVALID_AUTH);
    };

    EXPECT_CALL(*m_mockPostConnectSendMessage, sendPostConnectMessage(_))
        .WillOnce(Invoke(handleAddOrUpdateReport))
        .WillOnce(Invoke(handleDeleteReport));

    EXPECT_CALL(
        *m_mockDiscoveryStatusObserver, onDiscoveryFailure(MessageRequestObserverInterface::Status::INVALID_AUTH))
        .WillOnce(Return());

    ASSERT_FALSE(m_postConnectCapabilitiesPublisher->performOperation(m_mockPostConnectSendMessage));
}

/**
 *  Test when AddOrUpdateReport response is 4xx, DeleteReport event is not sent.
 */
TEST_F(PostConnectCapabilitiesPublisherTest, test_addOrUpdateReportReceives4xxResponse) {
    validateCallsToAuthDelegate(true);
    auto handleAddOrUpdateReport = [this](std::shared_ptr<MessageRequest> request) {
        EventData eventData;
        ASSERT_TRUE(parseEventJson(request->getJsonContent(), &eventData));
        validateDiscoveryEvent(eventData, ADD_OR_UPDATE_REPORT_NAME, EXPECTED_ADD_OR_UPDATE_ENDPOINT_IDS);
        request->sendCompleted(MessageRequestObserverInterface::Status::INVALID_AUTH);
        m_postConnectCapabilitiesPublisher->onAlexaEventProcessedReceived(eventData.eventCorrelationTokenString);
    };

    EXPECT_CALL(*m_mockPostConnectSendMessage, sendPostConnectMessage(_)).WillOnce(Invoke(handleAddOrUpdateReport));

    EXPECT_CALL(
        *m_mockDiscoveryStatusObserver, onDiscoveryFailure(MessageRequestObserverInterface::Status::INVALID_AUTH))
        .WillOnce(Return());

    ASSERT_FALSE(m_postConnectCapabilitiesPublisher->performOperation(m_mockPostConnectSendMessage));
}

/**
 * Test when AddOrUpdateReport response is 5xx, there are retries and deleteReport is not sent.
 */
TEST_F(PostConnectCapabilitiesPublisherTest, test_retriesWhenAddOrUpdateReportReceives5xxResponse) {
    validateCallsToAuthDelegate();
    int retryCount = 0;
    auto handleAddOrUpdateReport = [this, &retryCount](std::shared_ptr<MessageRequest> request) {
        EventData eventData;
        ASSERT_TRUE(parseEventJson(request->getJsonContent(), &eventData));
        validateDiscoveryEvent(eventData, ADD_OR_UPDATE_REPORT_NAME, EXPECTED_ADD_OR_UPDATE_ENDPOINT_IDS);
        request->sendCompleted(MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2);

        /// wait until 3 retries and exit.
        static int count = 0;
        count++;
        if (TEST_RETRY_COUNT == count) {
            retryCount = TEST_RETRY_COUNT;
            /// spin a thread to initiate abort.
            std::thread localThread([this] { m_postConnectCapabilitiesPublisher->abortOperation(); });

            if (localThread.joinable()) {
                localThread.join();
            }
        }
    };

    EXPECT_CALL(
        *m_mockDiscoveryStatusObserver,
        onDiscoveryFailure(MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2))
        .WillRepeatedly(Return());

    EXPECT_CALL(*m_mockPostConnectSendMessage, sendPostConnectMessage(_))
        .WillRepeatedly(Invoke(handleAddOrUpdateReport));
    ASSERT_FALSE(m_postConnectCapabilitiesPublisher->performOperation(m_mockPostConnectSendMessage));
    ASSERT_EQ(retryCount, TEST_RETRY_COUNT);
}

/**
 * Test when AddOrUpdateReport response is 5xx, there are retries. If its successful after two retries, deleteReport
 * event also gets sent out.
 */
TEST_F(PostConnectCapabilitiesPublisherTest, test_addOrUpdateRetriesThenSuccessfulResponse) {
    validateCallsToAuthDelegate();
    int retryCount = 0;
    auto handleAddOrUpdateReport = [this, &retryCount](std::shared_ptr<MessageRequest> request) {
        EventData eventData;
        ASSERT_TRUE(parseEventJson(request->getJsonContent(), &eventData));
        validateDiscoveryEvent(eventData, ADD_OR_UPDATE_REPORT_NAME, EXPECTED_ADD_OR_UPDATE_ENDPOINT_IDS);
        if (retryCount < 2) {
            request->sendCompleted(MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2);
            retryCount++;
        } else {
            request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
            m_postConnectCapabilitiesPublisher->onAlexaEventProcessedReceived(eventData.eventCorrelationTokenString);
        }
    };

    auto handleDeleteReport = [](std::shared_ptr<MessageRequest> request) {
        EventData eventData;
        ASSERT_TRUE(parseEventJson(request->getJsonContent(), &eventData));
        validateDiscoveryEvent(eventData, DELETE_REPORT_NAME, EXPECTED_DELETE_ENDPOINT_IDS);
        request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
    };

    EXPECT_CALL(
        *m_mockDiscoveryStatusObserver,
        onDiscoveryFailure(MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2))
        .WillRepeatedly(Return());

    EXPECT_CALL(
        *m_mockDiscoveryStatusObserver, onDiscoveryCompleted(TEST_ADD_OR_UPDATE_ENDPOINTS, TEST_DELETE_ENDPOINTS))
        .WillOnce(Return());

    EXPECT_CALL(*m_mockPostConnectSendMessage, sendPostConnectMessage(_))
        .WillOnce(Invoke(handleAddOrUpdateReport))
        .WillOnce(Invoke(handleAddOrUpdateReport))
        .WillOnce(Invoke(handleAddOrUpdateReport))
        .WillOnce(Invoke(handleDeleteReport));

    ASSERT_TRUE(m_postConnectCapabilitiesPublisher->performOperation(m_mockPostConnectSendMessage));
}

/**
 * Test when DeleteReport response is 5xx, there are retries. If its successful after two retries, Discovery is
 * completed.
 */
TEST_F(PostConnectCapabilitiesPublisherTest, test_deleteReportRetriesThenSuccessfulResponse) {
    validateCallsToAuthDelegate();

    auto handleAddOrUpdateReport = [this](std::shared_ptr<MessageRequest> request) {
        EventData eventData;
        ASSERT_TRUE(parseEventJson(request->getJsonContent(), &eventData));
        validateDiscoveryEvent(eventData, ADD_OR_UPDATE_REPORT_NAME, EXPECTED_ADD_OR_UPDATE_ENDPOINT_IDS);
        request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
        m_postConnectCapabilitiesPublisher->onAlexaEventProcessedReceived(eventData.eventCorrelationTokenString);
    };

    int retryCount = 0;
    auto handleDeleteReport = [&retryCount](std::shared_ptr<MessageRequest> request) {
        EventData eventData;
        ASSERT_TRUE(parseEventJson(request->getJsonContent(), &eventData));
        validateDiscoveryEvent(eventData, DELETE_REPORT_NAME, EXPECTED_DELETE_ENDPOINT_IDS);
        if (retryCount < 2) {
            request->sendCompleted(MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2);
            retryCount++;
        } else {
            request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
        }
    };

    EXPECT_CALL(
        *m_mockDiscoveryStatusObserver,
        onDiscoveryFailure(MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2))
        .WillRepeatedly(Return());

    EXPECT_CALL(
        *m_mockDiscoveryStatusObserver, onDiscoveryCompleted(TEST_ADD_OR_UPDATE_ENDPOINTS, TEST_DELETE_ENDPOINTS))
        .WillOnce(Return());

    EXPECT_CALL(*m_mockPostConnectSendMessage, sendPostConnectMessage(_))
        .WillOnce(Invoke(handleAddOrUpdateReport))
        .WillOnce(Invoke(handleDeleteReport))
        .WillOnce(Invoke(handleDeleteReport))
        .WillOnce(Invoke(handleDeleteReport));

    ASSERT_TRUE(m_postConnectCapabilitiesPublisher->performOperation(m_mockPostConnectSendMessage));
}

/**
 * Test when EventProcessed directive is not received, the AddOrUpdateReport event is retried and deleteReport event
 * does not get sent.
 */
TEST_F(PostConnectCapabilitiesPublisherTest, test_retriesWhenEventProcessedDirectiveIsNotReceived) {
    validateCallsToAuthDelegate();
    int retryCount = 0;
    auto handleAddOrUpdateReport = [&retryCount, this](std::shared_ptr<MessageRequest> request) {
        EventData eventData;
        ASSERT_TRUE(parseEventJson(request->getJsonContent(), &eventData));
        validateDiscoveryEvent(eventData, ADD_OR_UPDATE_REPORT_NAME, EXPECTED_ADD_OR_UPDATE_ENDPOINT_IDS);
        request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);

        /// wait until 3 retries and exit.
        static int count = 0;
        count++;
        if (TEST_RETRY_COUNT == count) {
            retryCount = TEST_RETRY_COUNT;
            /// spin a thread to initiate abort.
            std::thread localThread([this] { m_postConnectCapabilitiesPublisher->abortOperation(); });

            if (localThread.joinable()) {
                localThread.join();
            }
        }
    };

    EXPECT_CALL(*m_mockDiscoveryStatusObserver, onDiscoveryFailure(MessageRequestObserverInterface::Status::TIMEDOUT))
        .WillRepeatedly(Return());

    EXPECT_CALL(*m_mockPostConnectSendMessage, sendPostConnectMessage(_))
        .WillRepeatedly(Invoke(handleAddOrUpdateReport));

    ASSERT_FALSE(m_postConnectCapabilitiesPublisher->performOperation(m_mockPostConnectSendMessage));
}

/**
 * Test when the abortOperation() gets called while waiting on the EventProcessed directive.
 */
TEST_F(PostConnectCapabilitiesPublisherTest, test_abortWhenWaitingOnEventProcessedDirective) {
    validateCallsToAuthDelegate();
    auto handleAddOrUpdateReport = [this](std::shared_ptr<MessageRequest> request) {
        EventData eventData;
        ASSERT_TRUE(parseEventJson(request->getJsonContent(), &eventData));
        validateDiscoveryEvent(eventData, ADD_OR_UPDATE_REPORT_NAME, EXPECTED_ADD_OR_UPDATE_ENDPOINT_IDS);
        request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);

        /// spin a thread to initiate abort.
        std::thread localThread([this] { m_postConnectCapabilitiesPublisher->abortOperation(); });

        if (localThread.joinable()) {
            localThread.join();
        }
    };

    EXPECT_CALL(*m_mockDiscoveryStatusObserver, onDiscoveryFailure(MessageRequestObserverInterface::Status::CANCELED))
        .WillRepeatedly(Return());

    EXPECT_CALL(*m_mockPostConnectSendMessage, sendPostConnectMessage(_)).WillOnce(Invoke(handleAddOrUpdateReport));

    ASSERT_FALSE(m_postConnectCapabilitiesPublisher->performOperation(m_mockPostConnectSendMessage));
}

/**
 * Test when EventProcessed directive is received with a wrong event correlation token, the AddOrUpdateReport event is
 * retried and deleteReport event does not get sent.
 */
TEST_F(PostConnectCapabilitiesPublisherTest, test_retriesWhenInvalidEventProcessedDirectiveIsReceived) {
    validateCallsToAuthDelegate();
    int retryCount = 0;
    auto handleAddOrUpdateReport = [&retryCount, this](std::shared_ptr<MessageRequest> request) {
        EventData eventData;
        ASSERT_TRUE(parseEventJson(request->getJsonContent(), &eventData));
        validateDiscoveryEvent(eventData, ADD_OR_UPDATE_REPORT_NAME, EXPECTED_ADD_OR_UPDATE_ENDPOINT_IDS);
        request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
        m_postConnectCapabilitiesPublisher->onAlexaEventProcessedReceived(TEST_EVENT_CORRELATION_TOKEN);

        /// wait until 3 retries and exit.
        static int count = 0;
        count++;
        if (TEST_RETRY_COUNT == count) {
            retryCount = TEST_RETRY_COUNT;
            /// spin a thread to initiate abort.
            std::thread localThread([this] { m_postConnectCapabilitiesPublisher->abortOperation(); });

            if (localThread.joinable()) {
                localThread.join();
            }
        }
    };

    EXPECT_CALL(*m_mockDiscoveryStatusObserver, onDiscoveryFailure(MessageRequestObserverInterface::Status::TIMEDOUT))
        .WillRepeatedly(Return());

    EXPECT_CALL(*m_mockPostConnectSendMessage, sendPostConnectMessage(_))
        .WillRepeatedly(Invoke(handleAddOrUpdateReport));

    ASSERT_FALSE(m_postConnectCapabilitiesPublisher->performOperation(m_mockPostConnectSendMessage));
    ASSERT_EQ(retryCount, TEST_RETRY_COUNT);
}

/**
 * Test auth token is empty.
 */
TEST_F(PostConnectCapabilitiesPublisherTest, test_performOperationWhenAuthTokenIsEmpty) {
    EXPECT_CALL(*m_mockAuthDelegate, addAuthObserver(_)).WillOnce(InvokeWithoutArgs([this]() {
        m_postConnectCapabilitiesPublisher->onAuthStateChange(
            AuthObserverInterface::State::REFRESHED, AuthObserverInterface::Error::SUCCESS);
    }));
    EXPECT_CALL(*m_mockAuthDelegate, getAuthToken()).WillRepeatedly(Return(""));
    EXPECT_CALL(*m_mockAuthDelegate, removeAuthObserver(_)).WillOnce(Return());

    ASSERT_FALSE(m_postConnectCapabilitiesPublisher->performOperation(m_mockPostConnectSendMessage));
}

/**
 * Test abortOperation when requesting auth token.
 */
TEST_F(PostConnectCapabilitiesPublisherTest, test_abortOperationWhenAuthTokenIsRequested) {
    /// Since the state is uninitialized performOperation() will be waitin in getAuthToken() method.
    EXPECT_CALL(*m_mockAuthDelegate, addAuthObserver(_)).WillOnce(InvokeWithoutArgs([this]() {
        m_postConnectCapabilitiesPublisher->onAuthStateChange(
            AuthObserverInterface::State::UNINITIALIZED, AuthObserverInterface::Error::SUCCESS);

        EXPECT_CALL(*m_mockAuthDelegate, removeAuthObserver(_)).WillOnce(Return());

        /// give a short time
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        /// spin a thread to initiate abort.
        std::thread localThread([this] { m_postConnectCapabilitiesPublisher->abortOperation(); });

        if (localThread.joinable()) {
            localThread.join();
        }
    }));
    ASSERT_FALSE(m_postConnectCapabilitiesPublisher->performOperation(m_mockPostConnectSendMessage));
}

}  // namespace test
}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK
