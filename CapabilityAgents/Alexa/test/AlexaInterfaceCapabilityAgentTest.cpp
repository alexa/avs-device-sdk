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

#include <future>
#include <memory>
#include <gmock/gmock.h>

#include <Alexa/AlexaInterfaceCapabilityAgent.h>
#include <Alexa/AlexaInterfaceMessageSenderInternalInterface.h>
#include <AVSCommon/AVS/AVSDirective.h>
#include <AVSCommon/AVS/NamespaceAndName.h>
#include <AVSCommon/SDKInterfaces/AlexaEventProcessedObserverInterface.h>
#include <AVSCommon/SDKInterfaces/MockAlexaInterfaceMessageSender.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/Utils/Memory/Memory.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alexa {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils::memory;

using namespace ::testing;

/// The namespace for this capability agent.
static const std::string NAMESPACE = "Alexa";

/// The Alexa.EventProcessed directive name.
static const std::string EVENT_PROCESSED_DIRECTIVE_NAME = "EventProcessed";

/// The Alexa.ReportState directive name.
static const std::string REPORT_STATE_DIRECTIVE_NAME = "ReportState";

/// The @c EventProcessed directive signature.
static const NamespaceAndName EVENT_PROCESSED{NAMESPACE, EVENT_PROCESSED_DIRECTIVE_NAME};

/// The @c ReportState directive signature.
static const NamespaceAndName REPORT_STATE{NAMESPACE, REPORT_STATE_DIRECTIVE_NAME};

/// The @c test messageId.
static const std::string TEST_ENDPOINT_ID = "test-endpoint";

/// The @c test messageId.
static const std::string TEST_MESSAGE_ID = "abcdefg";

/// The @c test EventCorrelationToken.
static const std::string TEST_EVENTCORRELATION_TOKEN = "abcdefg";

// clang-format off
/// A sample Directive JSON string for the purposes of creating an AVSDirective object.
static const std::string VALID_EVENT_PROCESSED_DIRECTIVE =
    "{"
        "\"directive\":{"
            "\"header\":{"
                "\"namespace\":\""+NAMESPACE+"\","
                "\"name\":\""+EVENT_PROCESSED.name+"\","
                "\"messageId\":\""+TEST_MESSAGE_ID+"\","
                "\"eventCorrelationToken\":\""+TEST_EVENTCORRELATION_TOKEN+"\""
            "},"
            "\"payload\":{}"
        "}"
    "}";

/// A directive with an incorrect name.
static const std::string UNKNOWN_DIRECTIVE =
    "{"
        "\"directive\":{"
            "\"header\":{"
                "\"namespace\":\""+NAMESPACE+"\","
                "\"name\":\"UnknownDirective\","
                "\"messageId\":\""+TEST_MESSAGE_ID+"\","
                "\"eventCorrelationToken\":\""+TEST_EVENTCORRELATION_TOKEN+"\""
            "},"
            "\"payload\":{}"
        "}"
    "}";

/// An invalid @c EventProcessed Directive with no eventCorrelationToken
static const std::string EVENT_PROCESSED_WITH_NO_EVENT_CORRELATION_TOKEN =
    "{"
        "\"directive\":{"
            "\"header\":{"
                "\"namespace\":\""+NAMESPACE+"\","
                "\"name\":\""+EVENT_PROCESSED.name+"\","
                "\"messageId\":\""+TEST_MESSAGE_ID+"\""
            "},"
            "\"payload\":{}"
        "}"
    "}";

/// A sample Directive JSON string for the purposes of creating an AVSDirective object.
static const std::string VALID_ALEXA_REPORTSTATE_DIRECTIVE =
    "{"
        "\"directive\":{"
            "\"header\":{"
                "\"namespace\":\""+NAMESPACE+"\","
                "\"name\":\""+REPORT_STATE.name+"\","
                "\"messageId\":\""+TEST_MESSAGE_ID+"\","
                "\"correlationToken\":\""+TEST_EVENTCORRELATION_TOKEN+"\""
            "},"
            "\"endpoint\":{"
                "\"endpointId\":\""+TEST_ENDPOINT_ID+"\""
            "},"
            "\"payload\":{}"
        "}"
    "}";

/// A sample Directive JSON string for the purposes of creating an AVSDirective object.
static const std::string INVALID_ALEXA_REPORTSTATE_DIRECTIVE_NO_ENDPOINT =
    "{"
        "\"directive\":{"
            "\"header\":{"
                "\"namespace\":\""+NAMESPACE+"\","
                "\"name\":\""+REPORT_STATE.name+"\","
                "\"messageId\":\""+TEST_MESSAGE_ID+"\","
                "\"correlationToken\":\""+TEST_EVENTCORRELATION_TOKEN+"\""
            "},"
            "\"payload\":{}"
        "}"
    "}";
// clang-format on

/// Timeout when waiting for future to be set.
static std::chrono::milliseconds TIMEOUT(1000);

/// A test @c EventProcessedObserver.
class TestEventProcessedObserver : public avsCommon::sdkInterfaces::AlexaEventProcessedObserverInterface {
public:
    bool waitForEventProcessed(const std::string& eventConrrelationToken) {
        std::unique_lock<std::mutex> lock{m_mutex};
        auto predicate = [this, eventConrrelationToken] { return m_eventCorrelationToken == eventConrrelationToken; };
        return m_wakeTrigger.wait_for(lock, TIMEOUT, predicate);
    }

    void onAlexaEventProcessedReceived(const std::string& eventCorrelationToken) {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_eventCorrelationToken = eventCorrelationToken;
        m_wakeTrigger.notify_one();
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_wakeTrigger;
    std::string m_eventCorrelationToken;
};

/// A mock @c MockAlexaInterfaceMessageSenderInternal

class MockAlexaInterfaceMessageSenderInternal : public AlexaInterfaceMessageSenderInternalInterface {
public:
    MOCK_METHOD3(
        sendStateReportEvent,
        bool(
            const std::string& instance,
            const std::string& correlationToken,
            const avsCommon::avs::AVSMessageEndpoint& endpoint));
    MOCK_METHOD4(
        sendResponseEvent,
        bool(
            const std::string& instance,
            const std::string& correlationToken,
            const avsCommon::avs::AVSMessageEndpoint& endpoint,
            const std::string& jsonPayload));
    MOCK_METHOD5(
        sendErrorResponseEvent,
        bool(
            const std::string& instance,
            const std::string& correlationToken,
            const avsCommon::avs::AVSMessageEndpoint& endpoint,
            const ErrorResponseType errorType,
            const std::string& errorMessage));
    MOCK_METHOD3(
        sendDeferredResponseEvent,
        bool(const std::string& instance, const std::string& correlationToken, const int estimatedDeferralInSeconds));
    MOCK_METHOD1(
        alexaResponseTypeToErrorType,
        ErrorResponseType(const avsCommon::avs::AlexaResponseType& responseType));
};

/**
 * Test harness for @c EventProcessedHandler class.
 */
class AlexaInterfaceCapabilityAgentTest : public Test {
public:
    /// Setup the test harness for running a test.
    void SetUp() override;

    /// Clean up the test harness for running a test.
    void TearDown() override;

    /// Function to set the promise and wake @ m_wakeSetCompleteFuture.
    void wakeOnSetCompleted();

    /**
     * Constructor.
     */
    AlexaInterfaceCapabilityAgentTest() :
            m_wakeSetCompletedPromise{},
            m_wakeSetCompletedFuture{m_wakeSetCompletedPromise.get_future()} {
    }

protected:
    /// Promise to synchronize directive handling through setCompleted.
    std::promise<void> m_wakeSetCompletedPromise;

    /// Future to synchronize directive handling through setCompleted.
    std::future<void> m_wakeSetCompletedFuture;

    /// A strict mock that allows the test to strictly monitor the handling of directives.
    std::unique_ptr<StrictMock<MockDirectiveHandlerResult>> m_mockDirectiveHandlerResult;

    /// Device info object to pass to the CA.
    std::shared_ptr<avsCommon::utils::DeviceInfo> m_deviceInfo;

    /// A strict mock that allows the test to strictly monitor the exceptions being sent.
    std::shared_ptr<StrictMock<MockExceptionEncounteredSender>> m_mockExceptionSender;

    /// A mock that allows the test to monitor calls to @c MockAlexaInterfaceMessageSenderInternal.
    std::shared_ptr<MockAlexaInterfaceMessageSenderInternal> m_mockAlexaMessageSender;

    /// A pointer to an instance of @c AlexaInterfaceCapabilityAgent that will be instantiated per test.
    std::shared_ptr<AlexaInterfaceCapabilityAgent> m_alexaInterfaceCapabilityAgent;
};

void AlexaInterfaceCapabilityAgentTest::SetUp() {
    m_deviceInfo = avsCommon::utils::DeviceInfo::create(
        "testClientId",
        "testProductId",
        "testSerialNumber",
        "testManufacturer",
        "testDescription",
        "testFriendlyName",
        "testDeviceType");
    m_mockExceptionSender = std::make_shared<StrictMock<MockExceptionEncounteredSender>>();
    m_mockDirectiveHandlerResult = make_unique<StrictMock<MockDirectiveHandlerResult>>();
    m_mockAlexaMessageSender = std::make_shared<MockAlexaInterfaceMessageSenderInternal>();

    m_alexaInterfaceCapabilityAgent = AlexaInterfaceCapabilityAgent::create(
        *m_deviceInfo, TEST_ENDPOINT_ID, m_mockExceptionSender, m_mockAlexaMessageSender);

    ASSERT_NE(m_alexaInterfaceCapabilityAgent, nullptr);
}

void AlexaInterfaceCapabilityAgentTest::TearDown() {
}

void AlexaInterfaceCapabilityAgentTest::wakeOnSetCompleted() {
    m_wakeSetCompletedPromise.set_value();
}

/**
 * Tests if create method returns null with null @c ExceptionSender and null @c ReportStateHandler.
 */
TEST_F(AlexaInterfaceCapabilityAgentTest, createWithInvalidParameters) {
    auto alexaInterfaceCA =
        AlexaInterfaceCapabilityAgent::create(*m_deviceInfo, TEST_ENDPOINT_ID, nullptr, m_mockAlexaMessageSender);
    ASSERT_EQ(alexaInterfaceCA, nullptr);

    alexaInterfaceCA =
        AlexaInterfaceCapabilityAgent::create(*m_deviceInfo, TEST_ENDPOINT_ID, m_mockExceptionSender, nullptr);
    ASSERT_EQ(alexaInterfaceCA, nullptr);
}

/**
 * Tests if Alexa.EventProcessed and Alexa.ReportState are present on the default endpoint config.
 */
TEST_F(AlexaInterfaceCapabilityAgentTest, testDefaultEndpoint) {
    auto endpointId = m_deviceInfo->getDefaultEndpointId();
    auto alexaInterfaceCA = AlexaInterfaceCapabilityAgent::create(
        *m_deviceInfo, endpointId, m_mockExceptionSender, m_mockAlexaMessageSender);
    ASSERT_NE(alexaInterfaceCA, nullptr);

    auto configuration = alexaInterfaceCA->getConfiguration();
    EXPECT_NE(configuration.find(EVENT_PROCESSED), configuration.end());
    EXPECT_NE(configuration.find({NAMESPACE, REPORT_STATE_DIRECTIVE_NAME, endpointId}), configuration.end());
}

/**
 * Tests if Alexa.ReportState is present and Alexa.EventProcessed is NOT present on a non-default endpoint config.
 */
TEST_F(AlexaInterfaceCapabilityAgentTest, testNonDefaultEndpoint) {
    auto endpointId = TEST_ENDPOINT_ID;
    auto alexaInterfaceCA = AlexaInterfaceCapabilityAgent::create(
        *m_deviceInfo, endpointId, m_mockExceptionSender, m_mockAlexaMessageSender);
    ASSERT_NE(alexaInterfaceCA, nullptr);

    auto configuration = alexaInterfaceCA->getConfiguration();
    EXPECT_EQ(configuration.find(EVENT_PROCESSED), configuration.end());
    EXPECT_NE(configuration.find({NAMESPACE, REPORT_STATE_DIRECTIVE_NAME, endpointId}), configuration.end());
}

/**
 * Tests if sendExceptionEncountered and setFailed are called for invalid directives.
 */
TEST_F(AlexaInterfaceCapabilityAgentTest, testValidUnknownDirective) {
    auto directivePair = AVSDirective::create(UNKNOWN_DIRECTIVE, nullptr, "");
    std::shared_ptr<AVSDirective> directive = std::move(directivePair.first);
    ASSERT_THAT(directive, NotNull());

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, Eq(ExceptionErrorType::UNSUPPORTED_OPERATION), _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_))
        .WillOnce(InvokeWithoutArgs(this, &AlexaInterfaceCapabilityAgentTest::wakeOnSetCompleted));
    EXPECT_CALL(*m_mockAlexaMessageSender, sendStateReportEvent(_, _, _)).Times(Exactly(0));

    m_alexaInterfaceCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    m_alexaInterfaceCapabilityAgent->CapabilityAgent::handleDirective(directive->getMessageId());
    EXPECT_EQ(m_wakeSetCompletedFuture.wait_for(TIMEOUT), std::future_status::ready);
}

/**
 * Tests if sendExceptionEncountered and setFailed are called when eventCorrelationToken is not in the EventProcessed
 * directive.
 */
TEST_F(AlexaInterfaceCapabilityAgentTest, testInvalidEventProcessedDirective) {
    auto directivePair = AVSDirective::create(EVENT_PROCESSED_WITH_NO_EVENT_CORRELATION_TOKEN, nullptr, "");
    std::shared_ptr<AVSDirective> directive = std::move(directivePair.first);
    ASSERT_THAT(directive, NotNull());

    EXPECT_CALL(
        *m_mockExceptionSender,
        sendExceptionEncountered(_, Eq(ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED), _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_))
        .WillOnce(InvokeWithoutArgs(this, &AlexaInterfaceCapabilityAgentTest::wakeOnSetCompleted));
    EXPECT_CALL(*m_mockAlexaMessageSender, sendStateReportEvent(_, _, _)).Times(Exactly(0));

    m_alexaInterfaceCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    m_alexaInterfaceCapabilityAgent->CapabilityAgent::handleDirective(directive->getMessageId());
    EXPECT_EQ(m_wakeSetCompletedFuture.wait_for(TIMEOUT), std::future_status::ready);
}

/**
 * Tests if setHandlingCompleted is called and if the observers are notified when a valid EventProcessed directive is
 * received.
 */
TEST_F(AlexaInterfaceCapabilityAgentTest, testValidEventProcessedDirective) {
    auto directivePair = AVSDirective::create(VALID_EVENT_PROCESSED_DIRECTIVE, nullptr, "");
    std::shared_ptr<AVSDirective> directive = std::move(directivePair.first);
    ASSERT_THAT(directive, NotNull());

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _)).Times(Exactly(0));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .WillOnce(InvokeWithoutArgs(this, &AlexaInterfaceCapabilityAgentTest::wakeOnSetCompleted));
    EXPECT_CALL(*m_mockAlexaMessageSender, sendStateReportEvent(_, _, _)).Times(Exactly(0));

    auto testObserver = std::make_shared<TestEventProcessedObserver>();
    m_alexaInterfaceCapabilityAgent->addEventProcessedObserver(testObserver);
    m_alexaInterfaceCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    m_alexaInterfaceCapabilityAgent->CapabilityAgent::handleDirective(directive->getMessageId());
    EXPECT_EQ(m_wakeSetCompletedFuture.wait_for(TIMEOUT), std::future_status::ready);
    ASSERT_TRUE(testObserver->waitForEventProcessed(TEST_EVENTCORRELATION_TOKEN));
}

/**
 * Tests if @c sendStateReportEvent() and setHandlingCompleted are called for a valid ReportState directive.
 */
TEST_F(AlexaInterfaceCapabilityAgentTest, testValidReportStateDirective) {
    auto directivePair = AVSDirective::create(VALID_ALEXA_REPORTSTATE_DIRECTIVE, nullptr, "");
    std::shared_ptr<AVSDirective> directive = std::move(directivePair.first);
    ASSERT_THAT(directive, NotNull());

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _)).Times(Exactly(0));
    EXPECT_CALL(*m_mockAlexaMessageSender, sendErrorResponseEvent(_, _, _, _, _)).Times(Exactly(0));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .WillOnce(InvokeWithoutArgs(this, &AlexaInterfaceCapabilityAgentTest::wakeOnSetCompleted));
    EXPECT_CALL(*m_mockAlexaMessageSender, sendStateReportEvent(_, _, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([](const std::string& instance,
                            const std::string& correlationToken,
                            const avsCommon::avs::AVSMessageEndpoint& endpoint) {
            EXPECT_EQ(correlationToken, TEST_EVENTCORRELATION_TOKEN);
            EXPECT_EQ(endpoint.endpointId, TEST_ENDPOINT_ID);
            return true;
        }));

    auto testObserver = std::make_shared<TestEventProcessedObserver>();
    m_alexaInterfaceCapabilityAgent->addEventProcessedObserver(testObserver);
    m_alexaInterfaceCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    m_alexaInterfaceCapabilityAgent->CapabilityAgent::handleDirective(directive->getMessageId());
    EXPECT_EQ(m_wakeSetCompletedFuture.wait_for(TIMEOUT), std::future_status::ready);
}

/**
 * Tests if @c sendErrorResponse() and setHandlingCompleted are called for ReportState failure.
 */
TEST_F(AlexaInterfaceCapabilityAgentTest, testValidReportStateDirectiveReportStateFailure) {
    auto directivePair = AVSDirective::create(VALID_ALEXA_REPORTSTATE_DIRECTIVE, nullptr, "");
    std::shared_ptr<AVSDirective> directive = std::move(directivePair.first);
    ASSERT_THAT(directive, NotNull());

    EXPECT_CALL(
        *m_mockAlexaMessageSender,
        sendErrorResponseEvent(
            _, _, _, Eq(AlexaInterfaceMessageSenderInternalInterface::ErrorResponseType::INTERNAL_ERROR), _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .WillOnce(InvokeWithoutArgs(this, &AlexaInterfaceCapabilityAgentTest::wakeOnSetCompleted));
    EXPECT_CALL(*m_mockAlexaMessageSender, sendStateReportEvent(_, _, _)).WillOnce(Return(false));

    m_alexaInterfaceCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    m_alexaInterfaceCapabilityAgent->CapabilityAgent::handleDirective(directive->getMessageId());
    EXPECT_EQ(m_wakeSetCompletedFuture.wait_for(TIMEOUT), std::future_status::ready);
}

/**
 * Tests if @c sendErrorResponse() and setHandlingCompleted is called for a ReportState directive without an endpoint.
 */
TEST_F(AlexaInterfaceCapabilityAgentTest, testInvalidReportStateDirectiveNoEndpoint) {
    auto directivePair = AVSDirective::create(INVALID_ALEXA_REPORTSTATE_DIRECTIVE_NO_ENDPOINT, nullptr, "");
    std::shared_ptr<AVSDirective> directive = std::move(directivePair.first);
    ASSERT_THAT(directive, NotNull());

    EXPECT_CALL(
        *m_mockAlexaMessageSender,
        sendErrorResponseEvent(
            _, _, _, Eq(AlexaInterfaceMessageSenderInternalInterface::ErrorResponseType::INVALID_DIRECTIVE), _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .WillOnce(InvokeWithoutArgs(this, &AlexaInterfaceCapabilityAgentTest::wakeOnSetCompleted));

    m_alexaInterfaceCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    m_alexaInterfaceCapabilityAgent->CapabilityAgent::handleDirective(directive->getMessageId());
    EXPECT_EQ(m_wakeSetCompletedFuture.wait_for(TIMEOUT), std::future_status::ready);
}

}  // namespace test
}  // namespace alexa
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
