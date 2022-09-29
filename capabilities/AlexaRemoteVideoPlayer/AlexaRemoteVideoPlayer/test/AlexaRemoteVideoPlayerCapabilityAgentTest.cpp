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

#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockAlexaInterfaceMessageSender.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/WaitEvent.h>

#include "acsdk/AlexaRemoteVideoPlayer/private/AlexaRemoteVideoPlayerCapabilityAgent.h"

namespace alexaClientSDK {
namespace alexaRemoteVideoPlayer {
namespace test {

using namespace alexaClientSDK::avsCommon::avs;
using namespace alexaClientSDK::avsCommon::sdkInterfaces;
using namespace alexaClientSDK::avsCommon::utils::memory;
using namespace ::testing;

using namespace alexaRemoteVideoPlayer;
using namespace alexaRemoteVideoPlayerInterfaces;

// For readability.
using EndpointIdentifier = endpoints::EndpointIdentifier;

/// Timeout when waiting for futures to be set.
static std::chrono::milliseconds TIMEOUT(1000);

/// The namespace for capability agent.
static const std::string NAMESPACE{"Alexa.RemoteVideoPlayer"};

/// The supported version
static const std::string INTERFACE_VERSION{"3.1"};

/// The name for SearchAndPlay directive
static const std::string NAME_SEARCHANDPLAY{"SearchAndPlay"};

/// The name for SearchAndDisplayResults directive
static const std::string NAME_SEARCHANDDISPLAYRESULTS{"SearchAndDisplayResults"};

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

// Sample RemoteVideoPlayer payload
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
    "searchText": {
        "transcribed": "Jack Ryan"
    },
    "timeWindow": {
        "end": "2016-09-07T23:59:00Z",
        "start": "2016-09-01T00:00:00Z"
    }
})";

class MockAlexaRemoteVideoPlayerInterface : public RemoteVideoPlayerInterface {
public:
    MOCK_METHOD1(playVideoMock, Response(RemoteVideoPlayerRequest*));
    MOCK_METHOD1(displaySearchResultsMock, Response(RemoteVideoPlayerRequest*));
    MOCK_METHOD0(getConfigurationMock, Configuration());

    Response playVideo(std::unique_ptr<RemoteVideoPlayerRequest> request) {
        return playVideoMock(request.get());
    }

    Response displaySearchResults(std::unique_ptr<RemoteVideoPlayerRequest> request) {
        return displaySearchResultsMock(request.get());
    }

    Configuration getConfiguration() {
        return getConfigurationMock();
    }
};

class AlexaRemoteVideoPlayerCapabilityAgentTest : public ::testing::Test {
public:
    /// SetUp before each test case.
    void SetUp() override;

    /// Clean up the test harness after running a test.
    void TearDown() override;

    /// Constructor.
    AlexaRemoteVideoPlayerCapabilityAgentTest() {
    }

protected:
    /// A mock @c RemoteVideoPlayerInterface object.
    std::shared_ptr<StrictMock<MockAlexaRemoteVideoPlayerInterface>> m_mockRemoteVideoPlayer;

    // a context manager
    std::shared_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockContextManager>> m_mockContextManager;

    /// The mock @c MessageSenderInterface.
    std::shared_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockAlexaInterfaceMessageSender>> m_mockResponseSender;

    /// A strict mock that allows the test to strictly monitor the exceptions being sent.
    std::shared_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender>> m_mockExceptionSender;

    /// A strict mock that allows the test to strictly monitor the handling of directives.
    std::unique_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>>
        m_mockDirectiveHandlerResult;

    /// A pointer to an instance of the RemoteVideoPlayer that will be instantiated per test.
    std::shared_ptr<AlexaRemoteVideoPlayerCapabilityAgent> m_RemoteVideoPlayerCapablityAgent;
};

void AlexaRemoteVideoPlayerCapabilityAgentTest::SetUp() {
    m_mockExceptionSender =
        std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender>>();
    m_mockDirectiveHandlerResult =
        make_unique<StrictMock<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>>();
    m_mockRemoteVideoPlayer = std::make_shared<StrictMock<MockAlexaRemoteVideoPlayerInterface>>();
    m_mockContextManager = std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockContextManager>>();
    m_mockResponseSender =
        std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockAlexaInterfaceMessageSender>>();
    m_RemoteVideoPlayerCapablityAgent = AlexaRemoteVideoPlayerCapabilityAgent::create(
        TEST_ENDPOINT_ID, m_mockRemoteVideoPlayer, m_mockContextManager, m_mockResponseSender, m_mockExceptionSender);
}

void AlexaRemoteVideoPlayerCapabilityAgentTest::TearDown() {
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
TEST_F(AlexaRemoteVideoPlayerCapabilityAgentTest, test_givenInvalidParameters_create_shouldFail) {
    ASSERT_THAT(
        AlexaRemoteVideoPlayerCapabilityAgent::create(
            "", m_mockRemoteVideoPlayer, m_mockContextManager, m_mockResponseSender, m_mockExceptionSender),
        IsNull());
    ASSERT_THAT(
        AlexaRemoteVideoPlayerCapabilityAgent::create(
            TEST_ENDPOINT_ID, nullptr, m_mockContextManager, m_mockResponseSender, m_mockExceptionSender),
        IsNull());
    ASSERT_THAT(
        AlexaRemoteVideoPlayerCapabilityAgent::create(
            TEST_ENDPOINT_ID, m_mockRemoteVideoPlayer, nullptr, m_mockResponseSender, m_mockExceptionSender),
        IsNull());
    ASSERT_THAT(
        AlexaRemoteVideoPlayerCapabilityAgent::create(
            TEST_ENDPOINT_ID, m_mockRemoteVideoPlayer, m_mockContextManager, nullptr, m_mockExceptionSender),
        IsNull());
    ASSERT_THAT(
        AlexaRemoteVideoPlayerCapabilityAgent::create(
            TEST_ENDPOINT_ID, m_mockRemoteVideoPlayer, m_mockContextManager, m_mockResponseSender, nullptr),
        IsNull());
}

/**
 * Test successful handling of SearchAndPlay directive.
 */
TEST_F(AlexaRemoteVideoPlayerCapabilityAgentTest, test_searchAndPlayDirective_successCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockRemoteVideoPlayer, playVideoMock(_)).WillOnce(InvokeWithoutArgs([] {
        return RemoteVideoPlayerInterface::Response();
    }));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _, _, _));

    ASSERT_THAT(m_RemoteVideoPlayerCapablityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_SEARCHANDPLAY);
    m_RemoteVideoPlayerCapablityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    m_RemoteVideoPlayerCapablityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    m_RemoteVideoPlayerCapablityAgent->shutdown();
}

/**
 * Test error path of SearchAndPlay directive.
 */
TEST_F(AlexaRemoteVideoPlayerCapabilityAgentTest, test_searchAndPlayDirective_errorCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockRemoteVideoPlayer, playVideoMock(_)).WillOnce(InvokeWithoutArgs([] {
        return RemoteVideoPlayerInterface::Response(
            RemoteVideoPlayerInterface::Response::Type::FAILED_ENDPOINT_UNREACHABLE, "TestEndpointNotReachable");
    }));

    EXPECT_CALL(
        *m_mockResponseSender,
        sendErrorResponseEvent(
            _, _, _, Matcher<AlexaInterfaceMessageSenderInterface::ErrorResponseType>(testing::_), _));

    ASSERT_THAT(m_RemoteVideoPlayerCapablityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_SEARCHANDPLAY);
    m_RemoteVideoPlayerCapablityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    m_RemoteVideoPlayerCapablityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    m_RemoteVideoPlayerCapablityAgent->shutdown();
}

/**
 * Test successful handling of SearchAndDisplayResults directive.
 */
TEST_F(AlexaRemoteVideoPlayerCapabilityAgentTest, test_searchAndDisplayResultsDirective_successCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockRemoteVideoPlayer, displaySearchResultsMock(_)).WillOnce(InvokeWithoutArgs([] {
        return RemoteVideoPlayerInterface::Response();
    }));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _, _, _));

    ASSERT_THAT(m_RemoteVideoPlayerCapablityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_SEARCHANDDISPLAYRESULTS);
    m_RemoteVideoPlayerCapablityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    m_RemoteVideoPlayerCapablityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    m_RemoteVideoPlayerCapablityAgent->shutdown();
}

/**
 * Test error path of SearchAndDisplayResults directive.
 */
TEST_F(AlexaRemoteVideoPlayerCapabilityAgentTest, test_searchAndDisplayResultsDirective_errorCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockRemoteVideoPlayer, displaySearchResultsMock(_)).WillOnce(InvokeWithoutArgs([] {
        return RemoteVideoPlayerInterface::Response(
            RemoteVideoPlayerInterface::Response::Type::FAILED_ENDPOINT_UNREACHABLE, "TestEndpointNotReachable");
    }));

    EXPECT_CALL(
        *m_mockResponseSender,
        sendErrorResponseEvent(
            _, _, _, Matcher<AlexaInterfaceMessageSenderInterface::ErrorResponseType>(testing::_), _));

    ASSERT_THAT(m_RemoteVideoPlayerCapablityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_SEARCHANDDISPLAYRESULTS);
    m_RemoteVideoPlayerCapablityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    m_RemoteVideoPlayerCapablityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    m_RemoteVideoPlayerCapablityAgent->shutdown();
}

/**
 * Tests unknown Directive.
 * Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(AlexaRemoteVideoPlayerCapabilityAgentTest, test_unknownDirective) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    ASSERT_THAT(m_RemoteVideoPlayerCapablityAgent, NotNull());

    m_RemoteVideoPlayerCapablityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(UNKNOWN_DIRECTIVE), std::move(m_mockDirectiveHandlerResult));
    m_RemoteVideoPlayerCapablityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    m_RemoteVideoPlayerCapablityAgent->shutdown();
}

}  // namespace test
}  // namespace alexaRemoteVideoPlayer
}  // namespace alexaClientSDK