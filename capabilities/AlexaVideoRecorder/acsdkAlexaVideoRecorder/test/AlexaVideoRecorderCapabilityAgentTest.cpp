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

#include "acsdkAlexaVideoRecorder/AlexaVideoRecorderCapabilityAgent.h"
#include "acsdkAlexaVideoRecorderInterfaces/VideoRecorderInterface.h"

#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MockAlexaInterfaceMessageSender.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/Timing/TimePoint.h>
#include <AVSCommon/Utils/WaitEvent.h>

namespace alexaClientSDK {
namespace acsdkAlexaVideoRecorder {
namespace test {
using namespace alexaClientSDK;
using namespace alexaClientSDK::avsCommon::avs;
using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::sdkInterfaces;
using namespace alexaClientSDK::avsCommon::utils::memory;
using namespace ::testing;

using namespace acsdkAlexaVideoRecorder;
using namespace acsdkAlexaVideoRecorderInterfaces;

// For readability.
using EndpointIdentifier = endpoints::EndpointIdentifier;

/// Timeout when waiting for futures to be set.
static std::chrono::milliseconds TIMEOUT(1000);

/// The namespace for capability agent.
static const std::string NAMESPACE{"Alexa.VideoRecorder"};

/// The supported version
static const std::string INTERFACE_VERSION{"3"};

/// The name for SearchAndRecord directive
static const std::string NAME_SEARCHANDRECORD{"SearchAndRecord"};

/// The name for CancelRecording directive
static const std::string NAME_CANCELRECORDING{"CancelRecording"};

/// The name for DeleteRecording directive
static const std::string NAME_DELETERECORDING{"DeleteRecording"};

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

// Sample VideoRecorder payload
static const std::string PAYLOAD = R"(
 {
    "entities": [
        {
            "value": "TV_SHOW",
            "type": "MediaType"
        },
        {
            "externalIds": {
                "gracenote": "ST0000000666681"
            },
            "value": "Prime Video",
            "type": "App"
        },
        {
            "externalIds": {
                "gracenote": "ST0000000666681"
            },
            "value": "Gaby Hoffman",
            "type": "Actor"
        },
        {
            "externalIds": {
                "gracenote": "ST0000000666681"
            },
            "entityMetadata": {
                "channelNumber": 1234,
                "channelCallSign": "KBTC"
            },
            "uri": "entity://provider/channel/1234",
            "value": "PBS",
            "type": "Channel"
        },
        {
            "externalIds": {
                "gracenote": "ST0000000666661"
            },
            "value": "Snow White",
            "type": "Character"
        },
        {
            "externalIds": {
                "gracenote": "ST0000000666661"
            },
            "value": "Games",
            "type": "Event"
        },
        {
            "value": "Intergalactic Wars",
            "type": "Franchise"
        },
        {
            "externalIds": {
                "gracenote": "ST0000000666661"
            },
            "value": "Games",
            "type": "Genre"
        },
        {
            "externalIds": {
                "gracenote": "ST0000000666661"
            },
            "value": "NCAA",
            "type": "League"
        },
        {
            "value": "TRUE",
            "type": "Popularity"
        },
        {
            "value": "Marvel",
            "type": "ProductionCompany"
        },
        {
            "value": "NEW",
            "type": "Recency"
        },
        {
            "value": "8",
            "type": "Episode"
        },
        {
            "value": "2",
            "type": "Season"
        },
        {
            "externalIds": {
                "gracenote": "ST0000000666661"
            },
            "value": "Football",
            "type": "Sport"
        },
        {
            "externalIds": {
                "gracenote": "ST0000000666661"
            },
            "value": "University of Washington Huskies",
            "type": "SportsTeam"
        },
        {
            "externalIds": {
                "gracenote": "ST0000000666661"
            },
            "value": "Manchester by the Sea",
            "type": "Video"
        },
        {
            "value": "HD",
            "type": "VideoResolution"
        }
    ],
    "quantifier": {
        "name": "NEW"
    },
    "timeWindow": {
        "end": "2016-09-07T23:59:00Z",
        "start": "2016-09-01T00:00:00Z"
    }
})";

class MockVideoRecorderHandlerInterface : public VideoRecorderInterface {
public:
    MOCK_METHOD1(searchAndRecordMock, Response(VideoRecorderRequest* payload));

    MOCK_METHOD1(cancelRecordingMock, Response(VideoRecorderRequest* payload));

    MOCK_METHOD1(deleteRecordingMock, Response(VideoRecorderRequest* payload));

    MOCK_METHOD0(isExtendedRecordingGUIShown, bool(void));

    MOCK_METHOD0(getStorageUsedPercentage, int(void));

    Response searchAndRecord(std::unique_ptr<VideoRecorderRequest> request) {
        return searchAndRecordMock(request.get());
    }

    Response cancelRecording(std::unique_ptr<VideoRecorderRequest> request) {
        return cancelRecordingMock(request.get());
    }

    Response deleteRecording(std::unique_ptr<VideoRecorderRequest> request) {
        return deleteRecordingMock(request.get());
    }
};

class AlexaVideoRecorderCapabilityAgentTest : public ::testing::Test {
public:
    /// SetUp before each test case.
    void SetUp() override;

    /// Clean up the test harness after running a test.
    void TearDown() override;

    /// Constructor.
    AlexaVideoRecorderCapabilityAgentTest() {
    }

protected:
    /**
     * Function to create AlexaVideoRecorderCapabilityAgent based on the reportable flags
     * and sets required ContextManager and VideoRecorder mocks.
     */
    std::shared_ptr<AlexaVideoRecorderCapabilityAgent> createCapabilityAgentAndSetExpects();

    /// A mock @c AlexaVideoRecorderInterface object.
    std::shared_ptr<StrictMock<MockVideoRecorderHandlerInterface>> m_mockVideoRecorder;

    // a context manager
    std::shared_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockContextManager>> m_mockContextManager;

    /// The mock @c MessageSenderInterface.
    std::shared_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockAlexaInterfaceMessageSender>> m_mockResponseSender;

    /// A strict mock that allows the test to strictly monitor the exceptions being sent.
    std::shared_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender>> m_mockExceptionSender;

    /// A strict mock that allows the test to strictly monitor the handling of directives.
    std::unique_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>>
        m_mockDirectiveHandlerResult;

    /// A pointer to an instance of the VideoRecorder that will be instantiated per test.
    std::shared_ptr<AlexaVideoRecorderCapabilityAgent> m_videoRecorderCapabilityAgent;
};

void AlexaVideoRecorderCapabilityAgentTest::SetUp() {
    m_mockExceptionSender =
        std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender>>();
    m_mockDirectiveHandlerResult =
        make_unique<StrictMock<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>>();
    m_mockVideoRecorder = std::make_shared<StrictMock<MockVideoRecorderHandlerInterface>>();
    m_mockContextManager = std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockContextManager>>();
    EXPECT_CALL(*m_mockContextManager, addStateProvider(_, _)).Times(Exactly(2));

    m_mockResponseSender =
        std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockAlexaInterfaceMessageSender>>();
    m_videoRecorderCapabilityAgent = AlexaVideoRecorderCapabilityAgent::create(
        TEST_ENDPOINT_ID, m_mockVideoRecorder, m_mockContextManager, m_mockResponseSender, m_mockExceptionSender);
}

void AlexaVideoRecorderCapabilityAgentTest::TearDown() {
    if (m_videoRecorderCapabilityAgent && !m_videoRecorderCapabilityAgent->isShutdown()) {
        EXPECT_CALL(*m_mockContextManager, removeStateProvider(_)).Times(2);
        m_videoRecorderCapabilityAgent->shutdown();
    }

    m_videoRecorderCapabilityAgent.reset();
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
TEST_F(AlexaVideoRecorderCapabilityAgentTest, test_givenInvalidParameters_create_shouldFail) {
    ASSERT_THAT(
        AlexaVideoRecorderCapabilityAgent::create(
            "", m_mockVideoRecorder, m_mockContextManager, m_mockResponseSender, m_mockExceptionSender),
        IsNull());
    ASSERT_THAT(
        AlexaVideoRecorderCapabilityAgent::create(
            TEST_ENDPOINT_ID, nullptr, m_mockContextManager, m_mockResponseSender, m_mockExceptionSender),
        IsNull());
    ASSERT_THAT(
        AlexaVideoRecorderCapabilityAgent::create(
            TEST_ENDPOINT_ID, m_mockVideoRecorder, nullptr, m_mockResponseSender, m_mockExceptionSender),
        IsNull());
    ASSERT_THAT(
        AlexaVideoRecorderCapabilityAgent::create(
            TEST_ENDPOINT_ID, m_mockVideoRecorder, m_mockContextManager, nullptr, m_mockExceptionSender),
        IsNull());
    ASSERT_THAT(
        AlexaVideoRecorderCapabilityAgent::create(
            TEST_ENDPOINT_ID, m_mockVideoRecorder, m_mockContextManager, m_mockResponseSender, nullptr),
        IsNull());
}

/**
 * Test successful handling of SearchAndRecord directive.
 */
TEST_F(AlexaVideoRecorderCapabilityAgentTest, test_searchAndRecordDirective_successCase) {
    VideoRecorderInterface::Response videoRecorderResponse;
    videoRecorderResponse.type = VideoRecorderInterface::Response::Type::SUCCESS;
    videoRecorderResponse.message = "SCHEDULED";

    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockVideoRecorder, searchAndRecordMock(_)).WillOnce(WithArg<0>(Return(videoRecorderResponse)));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _, _, _)).Times(1);

    ASSERT_THAT(m_videoRecorderCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_SEARCHANDRECORD);
    m_videoRecorderCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    m_videoRecorderCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    EXPECT_CALL(*m_mockContextManager, removeStateProvider(_)).Times(2);

    m_videoRecorderCapabilityAgent->shutdown();
}

/**
 * Test error path of SearchAndRecord directive.
 */
TEST_F(AlexaVideoRecorderCapabilityAgentTest, test_searchAndRecordDirective_errorCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;
    VideoRecorderInterface::Response videoRecorderResponse;
    videoRecorderResponse.type = VideoRecorderInterface::Response::Type::FAILED_TOO_MANY_FAILED_ATTEMPTS;
    videoRecorderResponse.message = "The operation can't be performed because there were too many failed attempts.";

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockVideoRecorder, searchAndRecordMock(_)).WillOnce(Return(videoRecorderResponse));

    EXPECT_CALL(
        *m_mockResponseSender,
        sendErrorResponseEvent(
            _, _, _, Matcher<AlexaInterfaceMessageSenderInterface::ErrorResponseType>(testing::_), _));

    ASSERT_THAT(m_videoRecorderCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_SEARCHANDRECORD);
    m_videoRecorderCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    m_videoRecorderCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    EXPECT_CALL(*m_mockContextManager, removeStateProvider(_)).Times(2);

    m_videoRecorderCapabilityAgent->shutdown();
}

/**
 * Test successful handling of CancelRecording directive.
 */
TEST_F(AlexaVideoRecorderCapabilityAgentTest, test_cancelRecordingDirective_successCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;
    VideoRecorderInterface::Response videoRecorderResponse;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));
    EXPECT_CALL(*m_mockVideoRecorder, cancelRecordingMock(_)).WillOnce(Return(videoRecorderResponse));
    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _, _, _)).Times(1);

    ASSERT_THAT(m_videoRecorderCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_CANCELRECORDING);
    m_videoRecorderCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    m_videoRecorderCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    EXPECT_CALL(*m_mockContextManager, removeStateProvider(_)).Times(2);
    m_videoRecorderCapabilityAgent->shutdown();
}

/**
 * Test error handling of CancelRecording directive.
 */
TEST_F(AlexaVideoRecorderCapabilityAgentTest, test_cancelRecordingDirective_errorCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;
    VideoRecorderInterface::Response videoRecorderResponse;
    videoRecorderResponse.type = VideoRecorderInterface::Response::Type::FAILED_TOO_MANY_FAILED_ATTEMPTS;
    videoRecorderResponse.message = "The operation can't be performed because there were too many failed attempts.";

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockVideoRecorder, cancelRecordingMock(_)).WillOnce(Return(videoRecorderResponse));

    EXPECT_CALL(
        *m_mockResponseSender,
        sendErrorResponseEvent(
            _, _, _, Matcher<AlexaInterfaceMessageSenderInterface::ErrorResponseType>(testing::_), _));

    ASSERT_THAT(m_videoRecorderCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_CANCELRECORDING);
    m_videoRecorderCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    m_videoRecorderCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    EXPECT_CALL(*m_mockContextManager, removeStateProvider(_)).Times(2);
    m_videoRecorderCapabilityAgent->shutdown();
}

/**
 * Test successful handling of DeleteRecording directive.
 */
TEST_F(AlexaVideoRecorderCapabilityAgentTest, test_deleteRecordingDirective_successCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;
    VideoRecorderInterface::Response videoRecorderResponse;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));
    EXPECT_CALL(*m_mockVideoRecorder, deleteRecordingMock(_)).WillOnce(Return(videoRecorderResponse));
    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _, _, _)).Times(1);

    ASSERT_THAT(m_videoRecorderCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_DELETERECORDING);
    m_videoRecorderCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    m_videoRecorderCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    EXPECT_CALL(*m_mockContextManager, removeStateProvider(_)).Times(2);

    m_videoRecorderCapabilityAgent->shutdown();
}

/**
 * Test error handling of DeleteRecording directive.
 */
TEST_F(AlexaVideoRecorderCapabilityAgentTest, test_deleteRecordingDirective_errorCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;
    VideoRecorderInterface::Response videoRecorderResponse;
    videoRecorderResponse.type = VideoRecorderInterface::Response::Type::FAILED_TOO_MANY_FAILED_ATTEMPTS;
    videoRecorderResponse.message = "The operation can't be performed because there were too many failed attempts.";

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockVideoRecorder, deleteRecordingMock(_)).WillOnce(Return(videoRecorderResponse));

    EXPECT_CALL(
        *m_mockResponseSender,
        sendErrorResponseEvent(
            _, _, _, Matcher<AlexaInterfaceMessageSenderInterface::ErrorResponseType>(testing::_), _));

    ASSERT_THAT(m_videoRecorderCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_DELETERECORDING);
    m_videoRecorderCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    m_videoRecorderCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    EXPECT_CALL(*m_mockContextManager, removeStateProvider(_)).Times(2);

    m_videoRecorderCapabilityAgent->shutdown();
}
/**
 * Tests unknown Directive.
 * Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(AlexaVideoRecorderCapabilityAgentTest, test_unknownDirective) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    ASSERT_THAT(m_videoRecorderCapabilityAgent, NotNull());

    m_videoRecorderCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(UNKNOWN_DIRECTIVE), std::move(m_mockDirectiveHandlerResult));
    m_videoRecorderCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    EXPECT_CALL(*m_mockContextManager, removeStateProvider(_)).Times(2);

    m_videoRecorderCapabilityAgent->shutdown();
}

}  // namespace test
}  // namespace acsdkAlexaVideoRecorder
}  // namespace alexaClientSDK
