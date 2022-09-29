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

#include "acsdkAlexaSeekController/AlexaSeekControllerCapabilityAgent.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <acsdkAlexaSeekControllerInterfaces/AlexaSeekControllerInterface.h>
#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MockAlexaInterfaceMessageSender.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/WaitEvent.h>

namespace alexaClientSDK {
namespace acsdkAlexaSeekController {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::memory;
using namespace ::testing;

using namespace acsdkAlexaSeekController;
using namespace acsdkAlexaSeekControllerInterfaces;

// For readability.
using EndpointIdentifier = endpoints::EndpointIdentifier;

/// Timeout when waiting for futures to be set.
static std::chrono::milliseconds TIMEOUT(1000);

/// The namespace for capability agent.
static const std::string NAMESPACE{"Alexa.SeekController"};

/// The supported version
static const std::string INTERFACE_VERSION{"3"};

/// The name for AdjustSeekPosition directive
static const std::string NAME_ADJUST_SEEK_POSITION{"AdjustSeekPosition"};

/// An unknown directive signature.
static const std::string UNKNOWN_DIRECTIVE{"Unknown"};

/// The test EndpointId
static const EndpointIdentifier TEST_ENDPOINT_ID("testEndpointId");

/// Event key.
static const std::string EVENT("event");

/// MessageId for testing.
static const std::string MESSAGE_ID_TEST("MessageId_Test");

/// DialogRequestId for testing.
static const std::string DIALOG_REQUEST_ID_TEST("DialogRequestId_Test");

/// Correlation token for testing.
static const std::string CORRELATION_TOKEN_TEST("CorrelationToken_Test");

// Sample AdjustSeekPosition payload
static const std::string ADJUST_SEEK_POSITION_PAYLOAD = R"delim( {
            "deltaPositionMilliseconds": -45000 } )delim";

class MockAlexaSeekControllerInterface : public AlexaSeekControllerInterface {
public:
    using Response = acsdkAlexaSeekControllerInterfaces::AlexaSeekControllerInterface::Response;

    MOCK_METHOD1(adjustSeekPosition, Response(const std::chrono::milliseconds& deltaPosition));
};

class AlexaSeekControllerCapabilityAgentTest : public ::testing::Test {
public:
    /// SetUp before each test case.
    void SetUp() override;

    /// Clean up the test harness after running a test.
    void TearDown() override;

    /// Constructor.
    AlexaSeekControllerCapabilityAgentTest() {
    }

protected:
    /**
     * Function to create AlexaSeekControllerCapabilityAgent based on the reportable flags
     * and sets required ContextManager and SeekController mocks.
     */
    std::shared_ptr<AlexaSeekControllerCapabilityAgent> createCapabilityAgentAndSetExpects(bool isRetrievable);

    /// A mock @c AlexaSeekControllerInterface object.
    std::shared_ptr<StrictMock<MockAlexaSeekControllerInterface>> m_mockSeekController;

    // a context manager
    std::shared_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockContextManager>> m_mockContextManager;

    /// The mock @c MessageSenderInterface.
    std::shared_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockAlexaInterfaceMessageSender>> m_mockResponseSender;

    /// A strict mock that allows the test to strictly monitor the exceptions being sent.
    std::shared_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender>> m_mockExceptionSender;

    /// A strict mock that allows the test to strictly monitor the handling of directives.
    std::unique_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>>
        m_mockDirectiveHandlerResult;
};

void AlexaSeekControllerCapabilityAgentTest::SetUp() {
    m_mockExceptionSender =
        std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender>>();
    m_mockDirectiveHandlerResult =
        make_unique<StrictMock<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>>();
    m_mockSeekController = std::make_shared<StrictMock<MockAlexaSeekControllerInterface>>();
    m_mockContextManager = std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockContextManager>>();
    m_mockResponseSender =
        std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockAlexaInterfaceMessageSender>>();
}

void AlexaSeekControllerCapabilityAgentTest::TearDown() {
}

std::shared_ptr<AlexaSeekControllerCapabilityAgent> AlexaSeekControllerCapabilityAgentTest::
    createCapabilityAgentAndSetExpects(bool retrievable) {
    // ContextManager Mocks
    if (retrievable) {
        // Expects a non-null CA reference during create.
        EXPECT_CALL(*m_mockContextManager, addStateProvider(_, ::testing::NotNull()));

        // Expects a null when CA is shutdown.
        EXPECT_CALL(*m_mockContextManager, removeStateProvider(_));
    }

    auto AlexaSeekControllerCapabilityAgent = AlexaSeekControllerCapabilityAgent::create(
        TEST_ENDPOINT_ID,
        m_mockSeekController,
        m_mockContextManager,
        m_mockResponseSender,
        m_mockExceptionSender,
        retrievable);
    return AlexaSeekControllerCapabilityAgent;
}

std::shared_ptr<AVSDirective> buildAVSDirective(std::string directiveName, std::string payload) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<avsCommon::avs::attachment::test::MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE, directiveName, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST, CORRELATION_TOKEN_TEST, INTERFACE_VERSION);
    auto avsMessageEndpoint = AVSMessageEndpoint(TEST_ENDPOINT_ID);

    return AVSDirective::create("", avsMessageHeader, payload, attachmentManager, "", avsMessageEndpoint);
}

/// Test that create() returns a nullptr if called with invalid arguments.
TEST_F(AlexaSeekControllerCapabilityAgentTest, test_givenInvalidParameters_create_shouldFail) {
    ASSERT_THAT(
        AlexaSeekControllerCapabilityAgent::create(
            "", m_mockSeekController, m_mockContextManager, m_mockResponseSender, m_mockExceptionSender, true),
        IsNull());
    ASSERT_THAT(
        AlexaSeekControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID, nullptr, m_mockContextManager, m_mockResponseSender, m_mockExceptionSender, true),
        IsNull());
    ASSERT_THAT(
        AlexaSeekControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID, m_mockSeekController, nullptr, m_mockResponseSender, m_mockExceptionSender, true),
        IsNull());
    ASSERT_THAT(
        AlexaSeekControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID, m_mockSeekController, m_mockContextManager, nullptr, m_mockExceptionSender, true),
        IsNull());
    ASSERT_THAT(
        AlexaSeekControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID, m_mockSeekController, m_mockContextManager, m_mockResponseSender, nullptr, true),
        IsNull());
}

/**
 * Test successful handling of AdjustSeekPosition directive.
 */
TEST_F(AlexaSeekControllerCapabilityAgentTest, test_adjustSeekPositionDirective_successCase) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockSeekController, adjustSeekPosition(_)).WillOnce(InvokeWithoutArgs([]() {
        acsdkAlexaSeekControllerInterfaces::AlexaSeekControllerInterface::Response seekControllerResponse(
            AlexaSeekControllerInterface::Response::Type::SUCCESS, "");
        return seekControllerResponse;
    }));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _));

    auto seekControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true);
    ASSERT_THAT(seekControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive =
        buildAVSDirective(NAME_ADJUST_SEEK_POSITION, ADJUST_SEEK_POSITION_PAYLOAD);
    seekControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    seekControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    seekControllerCapabilityAgent->shutdown();
}

/**
 * Test error path of AdjustSeekPosition directive.
 */
TEST_F(AlexaSeekControllerCapabilityAgentTest, test_adjustSeekPositionDirective_errorCase) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockSeekController, adjustSeekPosition(_)).WillOnce(InvokeWithoutArgs([]() {
        acsdkAlexaSeekControllerInterfaces::AlexaSeekControllerInterface::Response seekControllerResponse(
            AlexaSeekControllerInterface::Response::Type::NO_CONTENT_AVAILABLE, "No content available for playing");
        return seekControllerResponse;
    }));

    EXPECT_CALL(
        *m_mockResponseSender,
        sendErrorResponseEvent(
            _, _, _, Matcher<AlexaInterfaceMessageSenderInterface::ErrorResponseType>(testing::_), _));

    auto AlexaSeekControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true);
    ASSERT_THAT(AlexaSeekControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive =
        buildAVSDirective(NAME_ADJUST_SEEK_POSITION, ADJUST_SEEK_POSITION_PAYLOAD);
    AlexaSeekControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    AlexaSeekControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    AlexaSeekControllerCapabilityAgent->shutdown();
}

/**
 * Tests unknown Directive.
 * Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(AlexaSeekControllerCapabilityAgentTest, test_unknownDirective) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    auto AlexaSeekControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true);
    ASSERT_THAT(AlexaSeekControllerCapabilityAgent, NotNull());

    AlexaSeekControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(UNKNOWN_DIRECTIVE, ""), std::move(m_mockDirectiveHandlerResult));
    AlexaSeekControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    AlexaSeekControllerCapabilityAgent->shutdown();
}
}  // namespace test
}  // namespace acsdkAlexaSeekController
}  // namespace alexaClientSDK
