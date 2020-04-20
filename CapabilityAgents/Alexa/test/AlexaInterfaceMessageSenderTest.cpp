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

#include <chrono>
#include <memory>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <AVSCommon/AVS/AVSDirective.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/Utils/Timing/TimePoint.h>

#include "Alexa/AlexaInterfaceMessageSender.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alexa {
namespace test {

using namespace ::testing;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils;
using namespace avsCommon::utils::timing;
using namespace rapidjson;

/// Amount of time for the test to wait for event to be sent.
static const std::chrono::seconds MY_WAIT_TIMEOUT(2);

/// Name for PowerController.
static const std::string NAME_POWER_CONTROLLER("PowerController");

/// Namespace for PowerController.
static const std::string NAMESPACE_POWER_CONTROLLER("Alexa.PowerController");

/// Name for TurnOn directive to PowerController.
static const std::string NAME_TURN_ON("TurnOn");

/// Name for powerState.
static const std::string POWER_STATE("powerState");

/// Value for powerState ON.
static const std::string POWER_STATE_ON("\"ON\"");

/// Event key.
static const std::string EVENT("event");

/// Header key.
static const std::string HEADER("header");

/// Message Id key.
static const std::string MESSAGE_ID("messageId");

/// Message Id for testing.
static const std::string MESSAGE_ID_TEST("MessageId_Test");

/// Correlation token Key.
static const std::string CORRELATION_TOKEN("correlationToken");

/// Correlation token for testing.
static const std::string CORRELATION_TOKEN_TEST("CorrelationToken_Test");

/// Event correlation token Key.
static const std::string EVENT_CORRELATION_TOKEN("eventCorrelationToken");

/// Event correlation token for testing.
static const std::string EVENT_CORRELATION_TOKEN_TEST("EventCorrelationToken_Test");

/// Payload key
static const std::string PAYLOAD_VERSION("version");

/// A speech recognizer payload for testing
static const std::string PAYLOAD_VERSION_TEST("3");

/// Payload key
static const std::string TIME_OF_SAMPLE("timeOfSample");

/// A speech recognizer payload for testing
static const std::string TIME_OF_SAMPLE_TEST("2017-02-03T16:20:50.523Z");

/// Payload key
static const std::string ENDPOINT_ID("endpointId");

/// A speech recognizer payload for testing
static const std::string ENDPOINT_ID_TEST("EndpointId_Test");

/// Payload key
static const std::string ERROR_ENDPOINT_UNREACHABLE("ENDPOINT_UNREACHABLE");

/// A speech recognizer payload for testing
static const std::string ERROR_ENDPOINT_UNREACHABLE_MESSAGE("Endpoint unreachable message");

/// Payload key
static const std::string PAYLOAD("payload");

/// A speech recognizer payload for testing
static const std::string PAYLOAD_TEST("payload_Test");

// clang-format off
/// A StateReport context for testing.
static const std::string STATE_REPORT_CONTEXT =
        "\"context\":{"
            "\"properties\":["
              "{"
                "\"namespace\":\"Alexa.PowerController\","
                "\"name\":\"powerState\","
                "\"value\":\"ON\","
                "\"timeOfSample\":\""+TIME_OF_SAMPLE_TEST+"\","
                "\"uncertaintyInMilliseconds\": 0"
              "}"
            "]"
        "}";

/// A StateReport event with context for testing.
static const std::string STATE_REPORT_EVENT_JSON_STRING =
    "{"
        "\"event\":{"
            "\"header\":{"
                "\"namespace\":\"Alexa\","
                "\"name\":\"StateReport\","
                "\"messageId\":\""+MESSAGE_ID_TEST+"\","
                "\"correlationToken\":\""+CORRELATION_TOKEN_TEST+"\","
                "\"eventCorrelationToken\":\""+EVENT_CORRELATION_TOKEN_TEST+"\","
                "\"payloadVersion\":\""+PAYLOAD_VERSION_TEST+"\""
            "},"
            "\"endpoint\":{"
                "\"endpointId\":\""+ENDPOINT_ID_TEST+"\""
            "},"
            "\"payload\":{}"
        "},"
        +STATE_REPORT_CONTEXT+
    "}";

/// A StateReport event for testing.
static const std::string STATE_REPORT_EVENT_NO_CONTEXT_JSON_STRING =
    "{"
        "\"event\":{"
            "\"header\":{"
                "\"namespace\":\"Alexa\","
                "\"name\":\"StateReport\","
                "\"messageId\":\""+MESSAGE_ID_TEST+"\","
                "\"correlationToken\":\""+CORRELATION_TOKEN_TEST+"\","
                "\"eventCorrelationToken\":\""+EVENT_CORRELATION_TOKEN_TEST+"\","
                "\"payloadVersion\":\""+PAYLOAD_VERSION_TEST+"\""
            "},"
            "\"endpoint\":{"
                "\"endpointId\":\""+ENDPOINT_ID_TEST+"\""
            "},"
            "\"payload\":{}"
        "}"
    "}";

/// Sample response from MockContextManager.
static const std::string TURNON_PROPERTIES_STRING =
    "\"properties\":["
      "{"
        "\"namespace\":\"Alexa.PowerController\","
        "\"name\":\"powerState\","
        "\"value\":\"ON\","
        "\"timeOfSample\":\""+TIME_OF_SAMPLE_TEST+"\","
        "\"uncertaintyInMilliseconds\": 0"
      "}"
    "]";

/// Sample response from MockContextManager.
static const std::string TURNON_CONTEXT_STRING =
    "\"context\":{"
        +TURNON_PROPERTIES_STRING+
    "}";

/// Sample response from PowerControllerCapabilityAgent.
static const std::string TURNON_RESPONSE_EVENT_STRING =
    "\"event\":{"
        "\"header\":{"
            "\"namespace\":\"Alexa\","
            "\"name\":\"Response\","
            "\"messageId\":\""+MESSAGE_ID_TEST+"\","
            "\"correlationToken\":\""+CORRELATION_TOKEN_TEST+"\","
            "\"eventCorrelationToken\":\""+EVENT_CORRELATION_TOKEN_TEST+"\","
            "\"payloadVersion\":\""+PAYLOAD_VERSION_TEST+"\""
        "},"
        "\"endpoint\":{"
            "\"endpointId\":\""+ENDPOINT_ID_TEST+"\""
        "},"
        "\"payload\":{}"
    "}";

/// Complete event response with context.
static const std::string TURNON_RESPONSE_EVENT_WITH_CONTEXT_STRING =
    "{"
        +TURNON_RESPONSE_EVENT_STRING+","
        +TURNON_CONTEXT_STRING+
    "}";

/// Complete event response without context.
static const std::string TURNON_RESPONSE_EVENT_WITHOUT_CONTEXT_STRING =
    "{"
        +TURNON_RESPONSE_EVENT_STRING+
    "}";

/// Sample error response
static const std::string ERROR_RESPONSE_EVENT_STRING =
    "{"
        "\"event\":{"
            "\"header\":{"
                "\"namespace\":\"Alexa\","
                "\"name\":\"ErrorResponse\","
                "\"messageId\":\""+MESSAGE_ID_TEST+"\","
                "\"correlationToken\":\""+CORRELATION_TOKEN_TEST+"\","
                "\"eventCorrelationToken\":\""+EVENT_CORRELATION_TOKEN_TEST+"\","
                "\"payloadVersion\":\""+PAYLOAD_VERSION_TEST+"\""
            "},"
            "\"endpoint\":{"
                "\"endpointId\":\""+ENDPOINT_ID_TEST+"\""
            "},"
            "\"payload\":{"
                "\"type\":\""+ERROR_ENDPOINT_UNREACHABLE+"\","
                "\"message\":\""+ERROR_ENDPOINT_UNREACHABLE_MESSAGE+"\""
            "}"
        "}"
    "}";

/// Sample deferred response
static const std::string DEFERRED_RESPONSE_EVENT_STRING =
    "{"
        "\"event\":{"
            "\"header\":{"
                "\"namespace\":\"Alexa\","
                "\"name\":\"DeferredResponse\","
                "\"messageId\":\""+MESSAGE_ID_TEST+"\","
                "\"correlationToken\":\""+CORRELATION_TOKEN_TEST+"\","
                "\"eventCorrelationToken\":\""+EVENT_CORRELATION_TOKEN_TEST+"\","
                "\"payloadVersion\":\""+PAYLOAD_VERSION_TEST+"\""
            "},"
            "\"payload\":{"
                "\"estimatedDeferralInSeconds\":7"
            "}"
        "}"
    "}";

/// Sample response from PowerControllerCapabilityAgent.
static const std::string TURNON_CHANGE_REPORT_WITH_CHANGE_EVENT_STRING =
    "{"
        "\"context\":{"
            "\"properties\":[]"
        "},"
        "\"event\":{"
            "\"header\":{"
                "\"namespace\":\"Alexa\","
                "\"name\":\"ChangeReport\","
                "\"messageId\":\""+MESSAGE_ID_TEST+"\","
                "\"eventCorrelationToken\":\""+EVENT_CORRELATION_TOKEN_TEST+"\","
                "\"payloadVersion\":\""+PAYLOAD_VERSION_TEST+"\""
            "},"
            "\"endpoint\":{"
                "\"endpointId\":\""+ENDPOINT_ID_TEST+"\""
            "},"
            "\"payload\":{"
                "\"change\":{"
                    "\"cause\":{"
                        "\"type\":\"ALEXA_INTERACTION\""
                    "},"
                    +TURNON_PROPERTIES_STRING+
                "}"
            "}"
        "}"
    "}";
// clang-format on

/// Test harness for @c AlexaInterfaceMessageSender class.
class AlexaInterfaceMessageSenderTest : public ::testing::Test {
public:
    /// Set up/tear down the test harness for running a test.
    void SetUp() override;
    void TearDown() override;

public:
    /**
     * Build a test AVSMessageEndpoint object.
     *
     * @return the test object
     */
    AVSMessageEndpoint buildTestEndpoint(void);

    /**
     * Build a test CapabilityTag object.
     *
     * @return the test object
     */
    CapabilityTag buildTestTag(void);

    /**
     * Build a test CapabilityTag object.
     *
     * @return the test object
     */
    CapabilityState buildTestState(void);

    /**
     * Helper function to create an AlexaInterfaceMessageSender and set the expects for it.
     *
     * @return pointer to the new AlexaInterfaceMessageSender
     */
    std::shared_ptr<AlexaInterfaceMessageSender> createMessageSender(void);

    /**
     * Helper function to remove the messageId.
     *
     * @param document The document from which to remove the messageId.
     * @param messageId The messageId that was removed (if successful).
     * @return bool Indicates whether removing the messageId was successful.
     */
    bool removeMessageId(Document* document, std::string* messageId);

    /**
     * Helper function to remove the eventCorrelationToken.
     *
     * @param document The document from which to remove the eventCorrelationToken.
     * @param eventCorrelationToken The eventCorrelationToken that was removed (if successful).
     * @return bool Indicates whether removing the eventCorrelationToken was successful.
     */
    bool removeEventCorrelationToken(Document* document, std::string* eventCorrelationToken);

    /**
     * Setup mocks for sending an event that expects the event to be sent on the happy path.
     *
     * @param sender the AlexaInterfaceMessageSender to use
     * @param jsonEventString the expected event as a JSON string
     * @param sendStatus the send status result to be returned
     * @param triggerOperation the function to call to start the sending process
     * @return true on success, false on failure
     */
    bool expectEventSent(
        const std::shared_ptr<AlexaInterfaceMessageSender>& sender,
        const std::string& jsonEventString,
        MessageRequestObserverInterface::Status sendStatus,
        std::function<void()> triggerOperation);

    /**
     * Setup mocks for sending an event, even if the context is invalid.
     *
     * @param jsonEventString the expected event as a JSON string
     * @param sendStatus the send status result to be returned
     * @param triggerOperation the function to call to start the sending process
     * @return true on success, false on failure
     */
    bool expectEventSentOnInvalidContext(
        const std::shared_ptr<AlexaInterfaceMessageSender>& sender,
        const std::string& jsonEventString,
        MessageRequestObserverInterface::Status sendStatus,
        std::function<void()> triggerOperation);

    /**
     * Setup mocks for sending an event that does not require context at all.
     *
     * @param jsonEventString the expected event as a JSON string
     * @param sendStatus the send status result to be returned
     * @param triggerOperation the function to call to start the sending process
     * @return true on success, false on failure
     */
    bool expectEventSentWithoutContext(
        const std::shared_ptr<AlexaInterfaceMessageSender>& sender,
        const std::string& jsonEventString,
        MessageRequestObserverInterface::Status sendStatus,
        std::function<void()> triggerOperation);

    /**
     * Setup mocks for an event that should not be sent if context is invalid.
     *
     * @param jsonEventString the expected event as a JSON string
     * @param sendStatus the send status result to be returned
     * @param triggerOperation the function to call to start the sending process
     * @return true on success, false on failure
     */
    bool expectEventNotSentOnInvalidContext(
        const std::shared_ptr<AlexaInterfaceMessageSender>& sender,
        const std::string& jsonEventString,
        MessageRequestObserverInterface::Status sendStatus,
        std::function<void()> triggerOperation);

    /**
     * Check to see that an event JSON matches the expected result after removing fields that are always different.
     *
     * @param jsonEventString the event JSON string to check
     * @param testEventString the event JSON string to test against
     * @return true on success, false on failure
     */
    bool checkEventJson(std::string jsonEventString, std::string testEventString);

protected:
    /// Context manager mock.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockContextManager> m_mockContextManager;

    /// Message sender mock to track messages being sent.
    std::shared_ptr<MockMessageSender> m_messageSender;

    /// Test @c AVSContext object.
    AVSContext m_context;
};

void AlexaInterfaceMessageSenderTest::SetUp() {
    m_mockContextManager = std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockContextManager>>();
    m_messageSender = std::make_shared<StrictMock<MockMessageSender>>();

    m_context.addState(buildTestTag(), buildTestState());
}

void AlexaInterfaceMessageSenderTest::TearDown() {
    m_messageSender.reset();
    m_mockContextManager.reset();
}

AVSMessageEndpoint AlexaInterfaceMessageSenderTest::buildTestEndpoint(void) {
    return AVSMessageEndpoint(ENDPOINT_ID_TEST);
}

CapabilityTag AlexaInterfaceMessageSenderTest::buildTestTag(void) {
    return CapabilityTag(NAMESPACE_POWER_CONTROLLER, POWER_STATE, ENDPOINT_ID_TEST);
}

CapabilityState AlexaInterfaceMessageSenderTest::buildTestState(void) {
    TimePoint timePoint;
    timePoint.setTime_ISO_8601(TIME_OF_SAMPLE_TEST);
    return CapabilityState(POWER_STATE_ON, timePoint, 0);
}

bool AlexaInterfaceMessageSenderTest::removeMessageId(Document* document, std::string* messageId) {
    if (!document || !messageId) {
        return false;
    }

    auto it = document->FindMember(EVENT.c_str());
    if (it == document->MemberEnd()) return false;

    it = it->value.FindMember(HEADER.c_str());
    if (it == document->MemberEnd()) return false;
    auto& eventHeader = it->value;

    it = it->value.FindMember(MESSAGE_ID.c_str());
    if (it == document->MemberEnd()) return false;

    *messageId = it->value.GetString();
    eventHeader.RemoveMember(it);

    return true;
}

bool AlexaInterfaceMessageSenderTest::removeEventCorrelationToken(
    Document* document,
    std::string* eventCorrelationToken) {
    if (!document || !eventCorrelationToken) {
        return false;
    }

    auto it = document->FindMember(EVENT.c_str());
    if (it == document->MemberEnd()) return false;

    it = it->value.FindMember(HEADER.c_str());
    if (it == document->MemberEnd()) return false;
    auto& eventHeader = it->value;

    it = it->value.FindMember(EVENT_CORRELATION_TOKEN.c_str());
    if (it == document->MemberEnd()) return false;

    *eventCorrelationToken = it->value.GetString();
    eventHeader.RemoveMember(it);

    return true;
}

std::shared_ptr<AlexaInterfaceMessageSender> AlexaInterfaceMessageSenderTest::createMessageSender(void) {
    EXPECT_CALL(*m_mockContextManager, addContextManagerObserver(_)).Times(1);
    auto sender = AlexaInterfaceMessageSender::create(m_mockContextManager, m_messageSender);
    EXPECT_THAT(sender, NotNull());
    return sender;
}

bool AlexaInterfaceMessageSenderTest::expectEventSent(
    const std::shared_ptr<AlexaInterfaceMessageSender>& sender,
    const std::string& jsonEventString,
    MessageRequestObserverInterface::Status sendStatus,
    std::function<void()> triggerOperation) {
    std::promise<bool> eventPromise;
    std::promise<bool> contextPromise;

    // Expect getContext() and reply with onContextAvailable()
    EXPECT_CALL(*m_mockContextManager, getContext(_, _, _)).WillOnce(InvokeWithoutArgs([this, sender, &contextPromise] {
        const ContextRequestToken token = 1;
        sender->onContextAvailable(ENDPOINT_ID_TEST, m_context, token);
        contextPromise.set_value(true);
        return token;
    }));

    // Expect sendMessage() and reply with sendCompleted()
    EXPECT_CALL(*m_messageSender, sendMessage(_))
        .Times(1)
        .WillOnce(Invoke([this, sendStatus, &jsonEventString, &eventPromise](std::shared_ptr<MessageRequest> request) {
            if (checkEventJson(request->getJsonContent(), jsonEventString)) {
                request->sendCompleted(sendStatus);
                eventPromise.set_value(true);
            } else {
                // Unexpected event
                eventPromise.set_value(false);
            }
        }));

    triggerOperation();

    EXPECT_TRUE(contextPromise.get_future().wait_for(MY_WAIT_TIMEOUT) == std::future_status::ready);

    auto sendFuture = eventPromise.get_future();
    bool isSendFutureReady = false;
    isSendFutureReady = sendFuture.wait_for(MY_WAIT_TIMEOUT) == std::future_status::ready;

    // Workaround GTEST issue where expectations can hold a reference to a shared_ptr even after we wait for the future.
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(m_messageSender.get()));
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(m_mockContextManager.get()));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    if (!isSendFutureReady) {
        return false;
    }

    return sendFuture.get();
}

bool AlexaInterfaceMessageSenderTest::expectEventSentOnInvalidContext(
    const std::shared_ptr<AlexaInterfaceMessageSender>& sender,
    const std::string& jsonEventString,
    MessageRequestObserverInterface::Status sendStatus,
    std::function<void()> triggerOperation) {
    std::promise<bool> eventPromise;
    std::promise<bool> contextPromise;

    // Expect getContext() and reply with onContextFailure()
    EXPECT_CALL(*m_mockContextManager, getContext(_, _, _)).WillOnce(InvokeWithoutArgs([sender, &contextPromise] {
        const ContextRequestToken token = 1;
        sender->onContextFailure(ContextRequestError::STATE_PROVIDER_TIMEDOUT, token);
        contextPromise.set_value(false);
        return token;
    }));

    // Expect sendMessage() and reply with sendCompleted()
    EXPECT_CALL(*m_messageSender, sendMessage(_))
        .Times(1)
        .WillOnce(Invoke([this, sendStatus, &jsonEventString, &eventPromise](std::shared_ptr<MessageRequest> request) {
            if (checkEventJson(request->getJsonContent(), jsonEventString)) {
                request->sendCompleted(sendStatus);
                eventPromise.set_value(true);
            } else {
                // Unexpected event
                eventPromise.set_value(false);
            }
        }));

    triggerOperation();

    EXPECT_TRUE(contextPromise.get_future().wait_for(MY_WAIT_TIMEOUT) == std::future_status::ready);

    auto sendFuture = eventPromise.get_future();
    bool isSendFutureReady = false;
    isSendFutureReady = sendFuture.wait_for(MY_WAIT_TIMEOUT) == std::future_status::ready;

    // Workaround GTEST issue where expectations can hold a reference to a shared_ptr even after we wait for the future.
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(m_messageSender.get()));
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(m_mockContextManager.get()));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    if (!isSendFutureReady) {
        return false;
    }

    return sendFuture.get();
}

bool AlexaInterfaceMessageSenderTest::expectEventSentWithoutContext(
    const std::shared_ptr<AlexaInterfaceMessageSender>& sender,
    const std::string& jsonEventString,
    MessageRequestObserverInterface::Status sendStatus,
    std::function<void()> triggerOperation) {
    std::promise<bool> eventPromise;
    std::promise<bool> contextPromise;

    // Expect sendMessage() and reply with sendCompleted()
    EXPECT_CALL(*m_messageSender, sendMessage(_))
        .Times(1)
        .WillOnce(Invoke([this, sendStatus, &jsonEventString, &eventPromise](std::shared_ptr<MessageRequest> request) {
            if (checkEventJson(request->getJsonContent(), jsonEventString)) {
                request->sendCompleted(sendStatus);
                eventPromise.set_value(true);
            } else {
                // Unexpected event
                eventPromise.set_value(false);
            }
        }));

    triggerOperation();

    auto sendFuture = eventPromise.get_future();
    bool isSendFutureReady = false;
    isSendFutureReady = sendFuture.wait_for(MY_WAIT_TIMEOUT) == std::future_status::ready;

    // Workaround GTEST issue where expectations can hold a reference to a shared_ptr even after we wait for the future.
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(m_messageSender.get()));
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(m_mockContextManager.get()));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    if (!isSendFutureReady) {
        return false;
    }

    return sendFuture.get();
}

bool AlexaInterfaceMessageSenderTest::expectEventNotSentOnInvalidContext(
    const std::shared_ptr<AlexaInterfaceMessageSender>& sender,
    const std::string& jsonEventString,
    MessageRequestObserverInterface::Status sendStatus,
    std::function<void()> triggerOperation) {
    std::promise<bool> eventPromise;
    std::promise<bool> contextPromise;

    // Expect getContext() and reply with onContextFailure()
    EXPECT_CALL(*m_mockContextManager, getContext(_, _, _)).WillOnce(InvokeWithoutArgs([sender, &contextPromise] {
        const ContextRequestToken token = 1;
        sender->onContextFailure(ContextRequestError::STATE_PROVIDER_TIMEDOUT, token);
        contextPromise.set_value(false);
        return token;
    }));

    triggerOperation();

    EXPECT_TRUE(contextPromise.get_future().wait_for(MY_WAIT_TIMEOUT) == std::future_status::ready);

    auto sendFuture = eventPromise.get_future();
    bool isSendFutureReady = false;
    isSendFutureReady = sendFuture.wait_for(MY_WAIT_TIMEOUT) == std::future_status::ready;

    // Workaround GTEST issue where expectations can hold a reference to a shared_ptr even after we wait for the future.
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(m_messageSender.get()));
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(m_mockContextManager.get()));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    if (!isSendFutureReady) {
        return false;
    }

    return sendFuture.get();
}

bool AlexaInterfaceMessageSenderTest::checkEventJson(std::string jsonEventString, std::string testEventString) {
    Document expected, actual;
    expected.Parse(testEventString);
    actual.Parse(jsonEventString);

    // messageId and eventCorrelationToken are randomly generated. Remove before comparing the Event strings.
    std::string value;
    EXPECT_TRUE(removeMessageId(&expected, &value));
    EXPECT_TRUE(removeMessageId(&actual, &value));
    EXPECT_TRUE(removeEventCorrelationToken(&expected, &value));
    EXPECT_TRUE(removeEventCorrelationToken(&actual, &value));

    EXPECT_EQ(expected, actual) << "Expected: " << testEventString << "\nValue: " << jsonEventString;
    return true;
}

TEST_F(AlexaInterfaceMessageSenderTest, test_givenInvalidParameters_create_shouldFail) {
    auto handler = AlexaInterfaceMessageSender::create(nullptr, m_messageSender);
    EXPECT_THAT(handler, IsNull());

    handler = AlexaInterfaceMessageSender::create(m_mockContextManager, nullptr);
    EXPECT_THAT(handler, IsNull());
}

TEST_F(AlexaInterfaceMessageSenderTest, test_sendReportState_shouldSucceedAndSend) {
    auto sender = createMessageSender();
    bool result = false;
    bool directiveResponseEventSent = expectEventSent(
        sender,
        STATE_REPORT_EVENT_JSON_STRING,
        MessageRequestObserverInterface::Status::SUCCESS,
        [this, sender, &result]() {
            result = sender->sendStateReportEvent("", CORRELATION_TOKEN_TEST, buildTestEndpoint());
        });

    ASSERT_TRUE(result);
    ASSERT_TRUE(directiveResponseEventSent);
}

TEST_F(AlexaInterfaceMessageSenderTest, test_sendReportState_missingContext_shouldSucceedAndSend) {
    auto sender = createMessageSender();
    bool result = false;
    bool directiveResponseEventSent = expectEventSentOnInvalidContext(
        sender,
        STATE_REPORT_EVENT_NO_CONTEXT_JSON_STRING,
        MessageRequestObserverInterface::Status::SUCCESS,
        [this, sender, &result]() {
            result = sender->sendStateReportEvent("", CORRELATION_TOKEN_TEST, buildTestEndpoint());
        });

    ASSERT_TRUE(result);
    ASSERT_TRUE(directiveResponseEventSent);
}

TEST_F(AlexaInterfaceMessageSenderTest, test_sendResponse_shouldSend) {
    auto sender = createMessageSender();
    bool result = false;
    bool directiveResponseEventSent = expectEventSent(
        sender,
        TURNON_RESPONSE_EVENT_WITH_CONTEXT_STRING,
        MessageRequestObserverInterface::Status::SUCCESS,
        [this, sender, &result]() {
            result = sender->sendResponseEvent("", CORRELATION_TOKEN_TEST, buildTestEndpoint());
        });

    ASSERT_TRUE(result);
    ASSERT_TRUE(directiveResponseEventSent);
}

TEST_F(AlexaInterfaceMessageSenderTest, test_sendResponse_noContext_shouldSend) {
    auto sender = createMessageSender();
    bool result = false;
    bool directiveResponseEventSent = expectEventSentOnInvalidContext(
        sender,
        TURNON_RESPONSE_EVENT_WITHOUT_CONTEXT_STRING,
        MessageRequestObserverInterface::Status::SUCCESS,
        [this, sender, &result]() {
            result = sender->sendResponseEvent("", CORRELATION_TOKEN_TEST, buildTestEndpoint());
        });

    ASSERT_TRUE(result);
    ASSERT_TRUE(directiveResponseEventSent);
}

TEST_F(AlexaInterfaceMessageSenderTest, test_sendErrorResponse_shouldSend) {
    auto sender = createMessageSender();
    bool result = false;
    bool directiveResponseEventSent = expectEventSentWithoutContext(
        sender,
        ERROR_RESPONSE_EVENT_STRING,
        MessageRequestObserverInterface::Status::SUCCESS,
        [this, sender, &result]() {
            result = sender->sendErrorResponseEvent(
                "",
                CORRELATION_TOKEN_TEST,
                buildTestEndpoint(),
                AlexaInterfaceMessageSenderInterface::ErrorResponseType::ENDPOINT_UNREACHABLE,
                ERROR_ENDPOINT_UNREACHABLE_MESSAGE);
        });

    ASSERT_TRUE(result);
    ASSERT_TRUE(directiveResponseEventSent);
}

TEST_F(AlexaInterfaceMessageSenderTest, test_sendDeferredResponse_shouldSend) {
    auto sender = createMessageSender();
    bool result = false;
    bool directiveResponseEventSent = expectEventSentWithoutContext(
        sender, DEFERRED_RESPONSE_EVENT_STRING, MessageRequestObserverInterface::Status::SUCCESS, [sender, &result]() {
            result = sender->sendDeferredResponseEvent("", CORRELATION_TOKEN_TEST, 7);
        });

    ASSERT_TRUE(result);
    ASSERT_TRUE(directiveResponseEventSent);
}

TEST_F(AlexaInterfaceMessageSenderTest, test_sendResponse_withChange_shouldSend) {
    auto sender = createMessageSender();
    bool directiveResponseEventSent = expectEventSent(
        sender,
        TURNON_CHANGE_REPORT_WITH_CHANGE_EVENT_STRING,
        MessageRequestObserverInterface::Status::SUCCESS,
        [this, sender]() {
            sender->onStateChanged(buildTestTag(), buildTestState(), AlexaStateChangeCauseType::ALEXA_INTERACTION);
        });

    ASSERT_TRUE(directiveResponseEventSent);
}

TEST_F(AlexaInterfaceMessageSenderTest, test_sendResponse_withChange_withoutContext_shouldNotSend) {
    auto sender = createMessageSender();
    bool directiveResponseEventSent = expectEventNotSentOnInvalidContext(
        sender, "", MessageRequestObserverInterface::Status::SUCCESS, [this, sender]() {
            sender->onStateChanged(buildTestTag(), buildTestState(), AlexaStateChangeCauseType::ALEXA_INTERACTION);
        });

    ASSERT_FALSE(directiveResponseEventSent);
}

}  // namespace test
}  // namespace alexa
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
