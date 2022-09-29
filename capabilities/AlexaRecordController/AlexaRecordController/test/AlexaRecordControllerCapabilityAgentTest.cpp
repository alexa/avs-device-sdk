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

#include <acsdk/AlexaRecordController/private/AlexaRecordControllerCapabilityAgent.h>
#include <acsdk/AlexaRecordControllerInterfaces/RecordControllerInterface.h>

#include "AVSCommon/SDKInterfaces/MockAlexaInterfaceMessageSender.h"
#include "AVSCommon/AVS/Attachment/MockAttachmentManager.h"
#include "AVSCommon/SDKInterfaces/MockContextManager.h"
#include "AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h"
#include "AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h"
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/Timing/TimePoint.h>
#include <AVSCommon/Utils/WaitEvent.h>
#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace alexaRecordController {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::utils;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::memory;
using namespace ::testing;

using namespace alexaRecordController;
using namespace alexaRecordControllerInterfaces;

// For readability.
using EndpointIdentifier = endpoints::EndpointIdentifier;

/// Timeout when waiting for futures to be set.
static std::chrono::milliseconds TIMEOUT(1000);

/// Time of sample used for testing
static const std::string TIME_OF_SAMPLE_TEST{"2017-02-03T16:20:50.523Z"};

/// The namespace for capability agent.
static const std::string NAMESPACE{"Alexa.RecordController"};

/// The supported version
static const std::string INTERFACE_VERSION{"3"};

/// The name for StartRecording directive
static const std::string NAME_STARTRECORDING{"StartRecording"};

/// The name for StopRecording directive
static const std::string NAME_STOPRECORDING{"StopRecording"};

/// An unknown directive signature.
static const std::string UNKNOWN_DIRECTIVE{"Unknown"};

/// The test EndpointId
static const EndpointIdentifier TEST_ENDPOINT_ID{"testEndpointId"};

/// Event key.
static const std::string EVENT{"event"};

/// MessageId for testing.
static const std::string MESSAGE_ID_TEST{"MessageId_Test"};

/// DialogRequestId for testing.
static const std::string DIALOG_REQUEST_ID_TEST{"DialogRequestId_Test"};

/// Correlation token for testing.
static const std::string CORRELATION_TOKEN_TEST{"CorrelationToken_Test"};

/// NOT_RECORDING state string for testing.
static const std::string TEST_STATE_NOT_RECORDING{"NOT_RECORDING"};

/// RECORDING state string for testing.
static const std::string TEST_STATE_RECORDING{"RECORDING"};

/// The name of recording state property
static const std::string RECORDINGSTATE_PROPERTY_NAME{"recordingState"};

class MockRecordControllerHandlerInterface : public RecordControllerInterface {
public:
    MOCK_METHOD0(startRecordingMock, Response());
    MOCK_METHOD0(stopRecordingMock, Response());
    MOCK_METHOD0(isRecordingMock, bool());

    Response startRecording() {
        return startRecordingMock();
    }

    Response stopRecording() {
        return stopRecordingMock();
    }

    bool isRecording() {
        return isRecordingMock();
    }
};

class AlexaRecordControllerCapabilityAgentTest : public ::testing::Test {
public:
    /// SetUp before each test case.
    void SetUp() override;

    /// Clean up the test harness after running a test.
    void TearDown() override;

    /// Constructor.
    AlexaRecordControllerCapabilityAgentTest() {
    }

protected:
    /**
     * Function to create AlexaRecordControllerCapabilityAgent based on the reportable flags
     * and sets required ContextManager and AlexaRecordController mocks.
     */
    std::shared_ptr<AlexaRecordControllerCapabilityAgent> createCapabilityAgentAndSetExpects(bool retrievable);

    /// A mock @c RecordControllerInterface object.
    std::shared_ptr<StrictMock<MockRecordControllerHandlerInterface>> m_mockRecordController;

    // a context manager
    std::shared_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockContextManager>> m_mockContextManager;

    /// The mock @c MessageSenderInterface.
    std::shared_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockAlexaInterfaceMessageSender>> m_mockResponseSender;

    /// A strict mock that allows the test to strictly monitor the exceptions being sent.
    std::shared_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender>> m_mockExceptionSender;

    /// A strict mock that allows the test to strictly monitor the handling of directives.
    std::unique_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>>
        m_mockDirectiveHandlerResult;

    /// The current recording state
    bool m_isRecording;
};

void AlexaRecordControllerCapabilityAgentTest::SetUp() {
    m_mockExceptionSender =
        std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender>>();
    m_mockDirectiveHandlerResult =
        make_unique<StrictMock<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>>();
    m_mockRecordController = std::make_shared<StrictMock<MockRecordControllerHandlerInterface>>();
    m_mockContextManager = std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockContextManager>>();
    m_mockResponseSender =
        std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockAlexaInterfaceMessageSender>>();

    auto timePoint = avsCommon::utils::timing::TimePoint();
    timePoint.setTime_ISO_8601(TIME_OF_SAMPLE_TEST);

    m_isRecording = false;
}

void AlexaRecordControllerCapabilityAgentTest::TearDown() {
}

std::shared_ptr<AlexaRecordControllerCapabilityAgent> AlexaRecordControllerCapabilityAgentTest::
    createCapabilityAgentAndSetExpects(bool retrievable) {
    // ContextManager Mocks
    if (retrievable) {
        // Expects a non-null CA reference during create.
        EXPECT_CALL(*m_mockContextManager, addStateProvider(_, ::testing::NotNull()));

        // Expects a null when CA is shutdown.
        EXPECT_CALL(*m_mockContextManager, removeStateProvider(_));
    }

    auto recordControllerCapabilityAgent = AlexaRecordControllerCapabilityAgent::create(
        TEST_ENDPOINT_ID,
        m_mockRecordController,
        m_mockContextManager,
        m_mockResponseSender,
        m_mockExceptionSender,
        retrievable);
    return recordControllerCapabilityAgent;
}

std::shared_ptr<AVSDirective> buildAVSDirective(std::string directiveName) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<avsCommon::avs::attachment::test::MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE, directiveName, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST, CORRELATION_TOKEN_TEST, INTERFACE_VERSION);
    auto avsMessageEndpoint = AVSMessageEndpoint(TEST_ENDPOINT_ID);

    return AVSDirective::create("", avsMessageHeader, "", attachmentManager, "", avsMessageEndpoint);
}

/// Test that create() returns a nullptr if called with invalid arguments.
TEST_F(AlexaRecordControllerCapabilityAgentTest, test_givenInvalidParameters_create_shouldFail) {
    ASSERT_THAT(
        AlexaRecordControllerCapabilityAgent::create(
            "", m_mockRecordController, m_mockContextManager, m_mockResponseSender, m_mockExceptionSender, true),
        IsNull());
    ASSERT_THAT(
        AlexaRecordControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID, nullptr, m_mockContextManager, m_mockResponseSender, m_mockExceptionSender, true),
        IsNull());
    ASSERT_THAT(
        AlexaRecordControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID, m_mockRecordController, nullptr, m_mockResponseSender, m_mockExceptionSender, true),
        IsNull());
    ASSERT_THAT(
        AlexaRecordControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID, m_mockRecordController, m_mockContextManager, nullptr, m_mockExceptionSender, true),
        IsNull());
    ASSERT_THAT(
        AlexaRecordControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID, m_mockRecordController, m_mockContextManager, m_mockResponseSender, nullptr, true),
        IsNull());
}

/**
 * Test successful handling of StartRecording directive.
 */
TEST_F(AlexaRecordControllerCapabilityAgentTest, test_startRecordingDirective_successCase) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockRecordController, startRecordingMock()).WillOnce(Invoke([]() {
        return RecordControllerInterface::Response();
    }));

    EXPECT_CALL(*m_mockRecordController, isRecordingMock()).WillOnce(Invoke([]() { return true; }));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _));

    auto recordControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true);
    ASSERT_THAT(recordControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_STARTRECORDING);
    recordControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    recordControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    recordControllerCapabilityAgent->shutdown();
}

/**
 * Test error path of StartRecording directive.
 */
TEST_F(AlexaRecordControllerCapabilityAgentTest, test_startRecordingDirective_errorCase) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockRecordController, startRecordingMock()).WillOnce(Invoke([]() {
        return RecordControllerInterface::Response{
            RecordControllerInterface::Response::Type::FAILED_ENDPOINT_UNREACHABLE, "TestEndpointNotReachable"};
    }));

    EXPECT_CALL(
        *m_mockResponseSender,
        sendErrorResponseEvent(
            _, _, _, Matcher<AlexaInterfaceMessageSenderInterface::ErrorResponseType>(testing::_), _));

    auto recordControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true);
    ASSERT_THAT(recordControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_STARTRECORDING);
    recordControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    recordControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    recordControllerCapabilityAgent->shutdown();
}

/**
 * Test successful handling of StopRecording directive.
 */
TEST_F(AlexaRecordControllerCapabilityAgentTest, test_stopRecordingDirective_successCase) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockRecordController, stopRecordingMock()).WillOnce(Invoke([]() {
        return RecordControllerInterface::Response();
    }));

    EXPECT_CALL(*m_mockRecordController, isRecordingMock()).WillOnce(Invoke([]() { return false; }));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _));

    auto recordControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true);
    ASSERT_THAT(recordControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_STOPRECORDING);
    recordControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    recordControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    recordControllerCapabilityAgent->shutdown();
}

/**
 * Test error path of StopRecording directive.
 */
TEST_F(AlexaRecordControllerCapabilityAgentTest, test_stopRecordingDirective_errorCase) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockRecordController, stopRecordingMock()).WillOnce(Invoke([]() {
        return RecordControllerInterface::Response{
            RecordControllerInterface::Response::Type::FAILED_ENDPOINT_UNREACHABLE, "TestEndpointNotReachable"};
    }));

    EXPECT_CALL(
        *m_mockResponseSender,
        sendErrorResponseEvent(
            _, _, _, Matcher<AlexaInterfaceMessageSenderInterface::ErrorResponseType>(testing::_), _));

    auto recordControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true);
    ASSERT_THAT(recordControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_STOPRECORDING);
    recordControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    recordControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    recordControllerCapabilityAgent->shutdown();
}

/**
 * Tests unknown Directive.
 * Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(AlexaRecordControllerCapabilityAgentTest, test_unknownDirective) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    auto recordControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true);
    ASSERT_THAT(recordControllerCapabilityAgent, NotNull());

    recordControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(UNKNOWN_DIRECTIVE), std::move(m_mockDirectiveHandlerResult));
    recordControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    recordControllerCapabilityAgent->shutdown();
}

/**
 * Tests unknown Directive with retrievable set.
 * Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(AlexaRecordControllerCapabilityAgentTest, test_unknownDirectiveWithProactivelyReportedAndRetrievableTrue) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    auto recordControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true);
    ASSERT_THAT(recordControllerCapabilityAgent, NotNull());

    recordControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(UNKNOWN_DIRECTIVE), std::move(m_mockDirectiveHandlerResult));
    recordControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    recordControllerCapabilityAgent->shutdown();
}

/**
 * Tests unknown Directive with retrievable set to false.
 * Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(AlexaRecordControllerCapabilityAgentTest, test_unknownDirectiveWithProactivelyReportedAndRetrievableFalse) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    auto recordControllerCapabilityAgent = createCapabilityAgentAndSetExpects(false);
    ASSERT_THAT(recordControllerCapabilityAgent, NotNull());

    recordControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(UNKNOWN_DIRECTIVE), std::move(m_mockDirectiveHandlerResult));
    recordControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    recordControllerCapabilityAgent->shutdown();
}

}  // namespace test
}  // namespace alexaRecordController
}  // namespace alexaClientSDK
