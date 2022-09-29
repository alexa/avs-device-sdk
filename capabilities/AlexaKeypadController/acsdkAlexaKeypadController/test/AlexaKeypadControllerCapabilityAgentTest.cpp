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

#include <acsdkAlexaKeypadControllerInterfaces/AlexaKeypadControllerInterface.h>
#include <acsdkAlexaKeypadControllerInterfaces/Keystroke.h>
#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockAlexaInterfaceMessageSender.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/Timing/TimePoint.h>
#include <AVSCommon/Utils/WaitEvent.h>

#include "acsdkAlexaKeypadController/AlexaKeypadControllerCapabilityAgent.h"

namespace alexaClientSDK {
namespace acsdkAlexaKeypadController {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::memory;
using namespace ::testing;

using namespace acsdkAlexaKeypadController;
using namespace acsdkAlexaKeypadControllerInterfaces;

// For readability.
using EndpointIdentifier = endpoints::EndpointIdentifier;

/// Timeout when waiting for futures to be set.
static std::chrono::milliseconds TIMEOUT(1000);

/// The namespace for capability agent.
static const std::string NAMESPACE{"Alexa.KeypadController"};

/// The supported version
static const std::string INTERFACE_VERSION{"3"};

/// The name for SendKeystroke directive
static const std::string NAME_SENDKEYSTROKE{"SendKeystroke"};

/// An unknown directive signature.
static const std::string UNKNOWN_DIRECTIVE{"Unknown"};

/// The test EndpointId
static const EndpointIdentifier TEST_ENDPOINT_ID("testEndpointId");

/// Event key.
static const std::string EVENT("event");

/// Header key.
static const std::string HEADER("header");

/// MessageId key.
static const std::string MESSAGE_ID("messageId");

/// MessageId for testing.
static const std::string MESSAGE_ID_TEST("MessageId_Test");

/// Dialog request Id key.
static const std::string DIALOG_REQUEST_ID("dialogRequestId");

/// DialogRequestId for testing.
static const std::string DIALOG_REQUEST_ID_TEST("DialogRequestId_Test");

/// Correlation token key.
static const std::string CORRELATION_TOKEN("correlationToken");

/// Correlation token for testing.
static const std::string CORRELATION_TOKEN_TEST("CorrelationToken_Test");

/// Event correlation token key.
static const std::string EVENT_CORRELATION_TOKEN("eventCorrelationToken");

/// Event correlation for testing.
static const std::string EVENT_CORRELATION_TOKEN_TEST("EventCorrelationToken_Test");

// Sample KeypadController payload
static const std::string PAYLOAD = R"delim( { "keystroke": "SELECT" } )delim";

class MockAlexaKeypadControllerInterface : public AlexaKeypadControllerInterface {
public:
    using HandleKeystroke = acsdkAlexaKeypadControllerInterfaces::AlexaKeypadControllerInterface::Response;
    using GetSupportedKeys = std::set<Keystroke>;

    MOCK_METHOD1(handleKeystroke, HandleKeystroke(Keystroke keystroke));
    MOCK_METHOD0(getSupportedKeys, GetSupportedKeys());
};

class AlexaKeypadControllerCapabilityAgentTest : public ::testing::Test {
public:
    /// SetUp before each test case.
    void SetUp() override;

    /// Clean up the test harness after running a test.
    void TearDown() override;

    /// Constructor.
    AlexaKeypadControllerCapabilityAgentTest() {
    }

protected:
    /**
     * Function to create AlexaKeypadControllerCapabilityAgent based on the reportable flags and sets required
     * ContextManager and KeypadController mocks.
     */
    std::shared_ptr<AlexaKeypadControllerCapabilityAgent> createCapabilityAgentAndSetExpects();

    /// A mock @c AlexaKeypadControllerInterface object.
    std::shared_ptr<StrictMock<MockAlexaKeypadControllerInterface>> m_mockKeypadController;

    /// The mock @c MockContextManager
    std::shared_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockContextManager>> m_mockContextManager;

    /// The mock @c MessageSenderInterface.
    std::shared_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockAlexaInterfaceMessageSender>> m_mockResponseSender;

    /// A strict mock that allows the test to strictly monitor the exceptions being sent.
    std::shared_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender>> m_mockExceptionSender;

    /// A strict mock that allows the test to strictly monitor the handling of directives.
    std::unique_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>>
        m_mockDirectiveHandlerResult;

    /// A pointer to an instance of the KeypadController that will be instantiated per test.
    std::shared_ptr<AlexaKeypadControllerCapabilityAgent> m_KeypadControllerCapablityAgent;
};

void AlexaKeypadControllerCapabilityAgentTest::SetUp() {
    m_mockExceptionSender =
        std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender>>();
    m_mockDirectiveHandlerResult =
        make_unique<StrictMock<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>>();
    m_mockKeypadController = std::make_shared<StrictMock<MockAlexaKeypadControllerInterface>>();
    m_mockContextManager = std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockContextManager>>();
    m_mockResponseSender =
        std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockAlexaInterfaceMessageSender>>();
    m_KeypadControllerCapablityAgent = AlexaKeypadControllerCapabilityAgent::create(
        TEST_ENDPOINT_ID, m_mockKeypadController, m_mockContextManager, m_mockResponseSender, m_mockExceptionSender);
}

void AlexaKeypadControllerCapabilityAgentTest::TearDown() {
}

std::shared_ptr<AVSDirective> buildAVSDirective(std::string directiveName) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<avsCommon::avs::attachment::test::MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE, directiveName, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST, CORRELATION_TOKEN_TEST, INTERFACE_VERSION);
    auto avsMessageEndpoint = AVSMessageEndpoint(TEST_ENDPOINT_ID);

    return AVSDirective::create("", avsMessageHeader, PAYLOAD, attachmentManager, "", avsMessageEndpoint);
}

/// Test that create() returns a nullptr if called with invalid arguments.
TEST_F(AlexaKeypadControllerCapabilityAgentTest, test_givenInvalidParameters_create_shouldFail) {
    ASSERT_THAT(
        AlexaKeypadControllerCapabilityAgent::create(
            "", m_mockKeypadController, m_mockContextManager, m_mockResponseSender, m_mockExceptionSender),
        IsNull());
    ASSERT_THAT(
        AlexaKeypadControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID, nullptr, m_mockContextManager, m_mockResponseSender, m_mockExceptionSender),
        IsNull());
    ASSERT_THAT(
        AlexaKeypadControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID, m_mockKeypadController, nullptr, m_mockResponseSender, m_mockExceptionSender),
        IsNull());
    ASSERT_THAT(
        AlexaKeypadControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID, m_mockKeypadController, m_mockContextManager, nullptr, m_mockExceptionSender),
        IsNull());
    ASSERT_THAT(
        AlexaKeypadControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID, m_mockKeypadController, m_mockContextManager, m_mockResponseSender, nullptr),
        IsNull());
}

/**
 * Test successful handling of SendKeystroke directive.
 */
TEST_F(AlexaKeypadControllerCapabilityAgentTest, test_sendKeystrokeDirective_successCase) {
    avsCommon::utils::WaitEvent waitEvent;
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockKeypadController, handleKeystroke(_)).WillOnce(WithArg<0>(Invoke([](Keystroke keystroke) {
        return AlexaKeypadControllerInterface::Response();
    })));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _));

    ASSERT_THAT(m_KeypadControllerCapablityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_SENDKEYSTROKE);
    m_KeypadControllerCapablityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    m_KeypadControllerCapablityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    m_KeypadControllerCapablityAgent->shutdown();
}

/**
 * Test error path of SendKeystroke directive.
 */
TEST_F(AlexaKeypadControllerCapabilityAgentTest, test_sendKeystrokeDirective_errorCase) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockKeypadController, handleKeystroke(_)).WillOnce(WithArg<0>(Invoke([](Keystroke keystroke) {
        AlexaKeypadControllerInterface::Response keypadControllerResponse;
        keypadControllerResponse.type = AlexaKeypadControllerInterface::Response::Type::INTERNAL_ERROR;
        return keypadControllerResponse;
    })));

    EXPECT_CALL(
        *m_mockResponseSender,
        sendErrorResponseEvent(
            _, _, _, Matcher<AlexaInterfaceMessageSenderInterface::ErrorResponseType>(testing::_), _));
    ASSERT_THAT(m_KeypadControllerCapablityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_SENDKEYSTROKE);
    m_KeypadControllerCapablityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    m_KeypadControllerCapablityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    m_KeypadControllerCapablityAgent->shutdown();
}

/**
 * Tests unknown Directive.
 * Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(AlexaKeypadControllerCapabilityAgentTest, test_unknownDirective) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    ASSERT_THAT(m_KeypadControllerCapablityAgent, NotNull());

    m_KeypadControllerCapablityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(UNKNOWN_DIRECTIVE), std::move(m_mockDirectiveHandlerResult));
    m_KeypadControllerCapablityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    m_KeypadControllerCapablityAgent->shutdown();
}

}  // namespace test
}  // namespace acsdkAlexaKeypadController
}  // namespace alexaClientSDK
