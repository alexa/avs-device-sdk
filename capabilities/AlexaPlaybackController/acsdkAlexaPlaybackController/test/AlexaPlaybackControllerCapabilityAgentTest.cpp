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

#include <acsdkAlexaPlaybackControllerInterfaces/AlexaPlaybackControllerInterface.h>
#include <acsdkAlexaPlaybackControllerInterfaces/AlexaPlaybackControllerObserverInterface.h>
#include <acsdkAlexaPlaybackControllerInterfaces/PlaybackOperation.h>
#include <acsdkAlexaPlaybackControllerInterfaces/PlaybackState.h>
#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockAlexaInterfaceMessageSender.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/WaitEvent.h>

#include "acsdkAlexaPlaybackController/AlexaPlaybackControllerCapabilityAgent.h"

namespace alexaClientSDK {
namespace acsdkAlexaPlaybackController {
namespace test {

using namespace alexaClientSDK::avsCommon::avs;
using namespace alexaClientSDK::avsCommon::sdkInterfaces;
using namespace alexaClientSDK::avsCommon::utils::memory;
using namespace ::testing;

using namespace acsdkAlexaPlaybackController;
using namespace acsdkAlexaPlaybackControllerInterfaces;

// For readability.
using EndpointIdentifier = endpoints::EndpointIdentifier;

/// Timeout when waiting for futures to be set.
static std::chrono::milliseconds TIMEOUT(1000);

/// The namespace for capability agent.
static const std::string NAMESPACE{"Alexa.PlaybackController"};

/// The namespace for capability agent.
static const std::string NAMESPACE_STATE_REPORT{"Alexa.PlaybackStateReporter"};

/// Property name for Alexa.PlaybackStateReporter
static const std::string STATE_REPORTER_PROPERTY{"playbackState"};

/// The supported version
static const std::string INTERFACE_VERSION{"3"};

/// The name for Play directive
static const std::string NAME_PLAY{"Play"};

/// The name for Pause directive
static const std::string NAME_PAUSE{"Pause"};

/// The name for Stop directive
static const std::string NAME_STOP{"Stop"};

/// The name for StartOver directive
static const std::string NAME_START_OVER{"StartOver"};

/// The name for Previous directive
static const std::string NAME_PREVIOUS{"Previous"};

/// The name for Next directive
static const std::string NAME_NEXT{"Next"};

/// The name for Rewind directive
static const std::string NAME_REWIND{"Rewind"};

/// The name for FastForward directive
static const std::string NAME_FAST_FORWARD{"FastForward"};

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

/// The name of playback state property
static const std::string PLAYBACKSTATE_PROPERTY_NAME{"playbackState"};

class MockAlexaPlaybackControllerInterface : public AlexaPlaybackControllerInterface {
public:
    using Response = acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response;
    using GetPlaybackState = acsdkAlexaPlaybackControllerInterfaces::PlaybackState;

    MOCK_METHOD0(play, Response());
    MOCK_METHOD0(pause, Response());
    MOCK_METHOD0(stop, Response());
    MOCK_METHOD0(startOver, Response());
    MOCK_METHOD0(previous, Response());
    MOCK_METHOD0(next, Response());
    MOCK_METHOD0(rewind, Response());
    MOCK_METHOD0(fastForward, Response());
    MOCK_METHOD0(getPlaybackState, GetPlaybackState());
    MOCK_METHOD1(addObserver, bool(const std::weak_ptr<AlexaPlaybackControllerObserverInterface>&));
    MOCK_METHOD1(removeObserver, void(const std::weak_ptr<AlexaPlaybackControllerObserverInterface>&));
    MOCK_METHOD0(getSupportedOperations, std::set<PlaybackOperation>());
};

class AlexaPlaybackControllerCapabilityAgentTest : public ::testing::Test {
public:
    /// SetUp before each test case.
    void SetUp() override;

    /// Clean up the test harness after running a test.
    void TearDown() override;

    /// Constructor.
    AlexaPlaybackControllerCapabilityAgentTest() {
    }

protected:
    /**
     * Function to create AlexaPlaybackControllerCapabilityAgent based on the reportable flags
     * and sets required ContextManager and PlaybackController mocks.
     */
    std::shared_ptr<AlexaPlaybackControllerCapabilityAgent> createCapabilityAgentAndSetExpects(
        bool proactivelyReported,
        bool retrievable);

    /// A mock @c AlexaPlaybackControllerInterface object.
    std::shared_ptr<StrictMock<MockAlexaPlaybackControllerInterface>> m_mockPlaybackController;

    /// A mock @c AlexaPlaybackControllerObserverInterface object.
    std::weak_ptr<AlexaPlaybackControllerObserverInterface> m_observer;

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

void AlexaPlaybackControllerCapabilityAgentTest::SetUp() {
    m_mockExceptionSender =
        std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender>>();
    m_mockDirectiveHandlerResult =
        make_unique<StrictMock<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>>();
    m_mockPlaybackController = std::make_shared<StrictMock<MockAlexaPlaybackControllerInterface>>();
    m_mockContextManager = std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockContextManager>>();
    m_mockResponseSender =
        std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockAlexaInterfaceMessageSender>>();
}

void AlexaPlaybackControllerCapabilityAgentTest::TearDown() {
}

std::shared_ptr<AlexaPlaybackControllerCapabilityAgent> AlexaPlaybackControllerCapabilityAgentTest::
    createCapabilityAgentAndSetExpects(bool proactivelyReported, bool retrievable) {
    // ContextManager Mocks
    if (retrievable) {
        // Expects a non-null CA reference during create.
        EXPECT_CALL(*m_mockContextManager, addStateProvider(_, ::testing::NotNull()));

        // Expects a null when CA is shutdown.
        EXPECT_CALL(*m_mockContextManager, removeStateProvider(_));
    }

    if (proactivelyReported) {
        // addObserver Called during the initialization.
        EXPECT_CALL(*m_mockPlaybackController, addObserver(_))
            .WillOnce(WithArg<0>(Invoke([this](std::weak_ptr<AlexaPlaybackControllerObserverInterface> observer) {
                m_observer = observer;
                return true;
            })));

        // removeObserver is called during the shutdown
        EXPECT_CALL(*m_mockPlaybackController, removeObserver(_))
            .WillOnce(WithArg<0>(Invoke(
                [this](std::weak_ptr<AlexaPlaybackControllerObserverInterface> observer) { m_observer.reset(); })));
    }

    auto playbackControllerCapabilityAgent = AlexaPlaybackControllerCapabilityAgent::create(
        TEST_ENDPOINT_ID,
        m_mockPlaybackController,
        m_mockContextManager,
        m_mockResponseSender,
        m_mockExceptionSender,
        proactivelyReported,
        retrievable);
    return playbackControllerCapabilityAgent;
}

std::shared_ptr<AVSDirective> buildAVSDirective(std::string directiveName) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<avsCommon::avs::attachment::test::MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE, directiveName, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST, CORRELATION_TOKEN_TEST, INTERFACE_VERSION);
    auto avsMessageEndpoint = AVSMessageEndpoint(TEST_ENDPOINT_ID);

    return AVSDirective::create("", avsMessageHeader, "", attachmentManager, "", avsMessageEndpoint);
}

/// Test that calls create() returns a nullptr if called with invalid arguments.
TEST_F(AlexaPlaybackControllerCapabilityAgentTest, test_givenInvalidParameters_create_shouldFail) {
    ASSERT_THAT(
        AlexaPlaybackControllerCapabilityAgent::create(
            "",
            m_mockPlaybackController,
            m_mockContextManager,
            m_mockResponseSender,
            m_mockExceptionSender,
            true,
            true),
        IsNull());
    ASSERT_THAT(
        AlexaPlaybackControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID, nullptr, m_mockContextManager, m_mockResponseSender, m_mockExceptionSender, true, true),
        IsNull());
    ASSERT_THAT(
        AlexaPlaybackControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID,
            m_mockPlaybackController,
            nullptr,
            m_mockResponseSender,
            m_mockExceptionSender,
            true,
            true),
        IsNull());
    ASSERT_THAT(
        AlexaPlaybackControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID,
            m_mockPlaybackController,
            m_mockContextManager,
            nullptr,
            m_mockExceptionSender,
            true,
            true),
        IsNull());
    ASSERT_THAT(
        AlexaPlaybackControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID,
            m_mockPlaybackController,
            m_mockContextManager,
            m_mockResponseSender,
            nullptr,
            true,
            true),
        IsNull());
}

/**
 * Test successful handling of Play directive.
 */
TEST_F(AlexaPlaybackControllerCapabilityAgentTest, test_playDirective_successCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockPlaybackController, play()).WillOnce(InvokeWithoutArgs([]() {
        acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response playbackControllerResponse(
            AlexaPlaybackControllerInterface::Response::Type::SUCCESS, "");
        return playbackControllerResponse;
    }));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _));

    auto playbackControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(playbackControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_PLAY);
    playbackControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    playbackControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    playbackControllerCapabilityAgent->shutdown();
}

/**
 * Test error path of Play directive.
 */
TEST_F(AlexaPlaybackControllerCapabilityAgentTest, test_playDirective_errorCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockPlaybackController, play()).WillOnce(InvokeWithoutArgs([]() {
        acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response playbackControllerResponse(
            AlexaPlaybackControllerInterface::Response::Type::NO_CONTENT_AVAILABLE, "No content available for playing");
        return playbackControllerResponse;
    }));

    EXPECT_CALL(
        *m_mockResponseSender,
        sendErrorResponseEvent(
            _, _, _, Matcher<AlexaInterfaceMessageSenderInterface::ErrorResponseType>(testing::_), _));
    auto playbackControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(playbackControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_PLAY);
    playbackControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    playbackControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    playbackControllerCapabilityAgent->shutdown();
}

/**
 * Test successful handling of Pause directive.
 */
TEST_F(AlexaPlaybackControllerCapabilityAgentTest, test_pauseDirective_successCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockPlaybackController, pause()).WillOnce(InvokeWithoutArgs([]() {
        acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response playbackControllerResponse(
            AlexaPlaybackControllerInterface::Response::Type::SUCCESS, "");
        return playbackControllerResponse;
    }));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _));

    auto playbackControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(playbackControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_PAUSE);
    playbackControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    playbackControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    playbackControllerCapabilityAgent->shutdown();
}

/**
 * Test error path of Pause directive.
 */
TEST_F(AlexaPlaybackControllerCapabilityAgentTest, test_pauseDirective_errorCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockPlaybackController, pause()).WillOnce(InvokeWithoutArgs([]() {
        acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response playbackControllerResponse(
            AlexaPlaybackControllerInterface::Response::Type::NO_CONTENT_AVAILABLE, "No content available for playing");
        return playbackControllerResponse;
    }));

    EXPECT_CALL(
        *m_mockResponseSender,
        sendErrorResponseEvent(
            _, _, _, Matcher<AlexaInterfaceMessageSenderInterface::ErrorResponseType>(testing::_), _));
    auto playbackControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(playbackControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_PAUSE);
    playbackControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    playbackControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    playbackControllerCapabilityAgent->shutdown();
}

/**
 * Test successful handling of Stop directive.
 */
TEST_F(AlexaPlaybackControllerCapabilityAgentTest, test_stopDirective_successCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockPlaybackController, stop()).WillOnce(InvokeWithoutArgs([]() {
        acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response playbackControllerResponse(
            AlexaPlaybackControllerInterface::Response::Type::SUCCESS, "");
        return playbackControllerResponse;
    }));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _));

    auto playbackControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(playbackControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_STOP);
    playbackControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    playbackControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    playbackControllerCapabilityAgent->shutdown();
}

/**
 * Test error path of Stop directive.
 */
TEST_F(AlexaPlaybackControllerCapabilityAgentTest, test_stopDirective_errorCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockPlaybackController, stop()).WillOnce(InvokeWithoutArgs([]() {
        acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response playbackControllerResponse(
            AlexaPlaybackControllerInterface::Response::Type::NO_CONTENT_AVAILABLE, "No content available for playing");
        return playbackControllerResponse;
    }));

    EXPECT_CALL(
        *m_mockResponseSender,
        sendErrorResponseEvent(
            _, _, _, Matcher<AlexaInterfaceMessageSenderInterface::ErrorResponseType>(testing::_), _));
    auto playbackControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(playbackControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_STOP);
    playbackControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    playbackControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    playbackControllerCapabilityAgent->shutdown();
}

/**
 * Test successful handling of StartOver directive.
 */
TEST_F(AlexaPlaybackControllerCapabilityAgentTest, test_startOverDirective_successCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockPlaybackController, startOver()).WillOnce(InvokeWithoutArgs([]() {
        acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response playbackControllerResponse(
            AlexaPlaybackControllerInterface::Response::Type::SUCCESS, "");
        return playbackControllerResponse;
    }));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _));

    auto playbackControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(playbackControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_START_OVER);
    playbackControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    playbackControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    playbackControllerCapabilityAgent->shutdown();
}

/**
 * Test error path of StartOver directive.
 */
TEST_F(AlexaPlaybackControllerCapabilityAgentTest, test_startOverDirective_errorCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockPlaybackController, startOver()).WillOnce(InvokeWithoutArgs([]() {
        AlexaPlaybackControllerInterface::Response playbackControllerResponse{
            AlexaPlaybackControllerInterface::Response::Type::NO_CONTENT_AVAILABLE, "No content available for playing"};
        return playbackControllerResponse;
    }));

    EXPECT_CALL(
        *m_mockResponseSender,
        sendErrorResponseEvent(
            _, _, _, Matcher<AlexaInterfaceMessageSenderInterface::ErrorResponseType>(testing::_), _));
    auto playbackControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(playbackControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_START_OVER);
    playbackControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    playbackControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    playbackControllerCapabilityAgent->shutdown();
}

/**
 * Test successful handling of Previous directive.
 */
TEST_F(AlexaPlaybackControllerCapabilityAgentTest, test_previousDirective_successCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockPlaybackController, previous()).WillOnce(InvokeWithoutArgs([]() {
        acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response playbackControllerResponse(
            AlexaPlaybackControllerInterface::Response::Type::SUCCESS, "");
        return playbackControllerResponse;
    }));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _));

    auto playbackControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(playbackControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_PREVIOUS);
    playbackControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    playbackControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    playbackControllerCapabilityAgent->shutdown();
}

/**
 * Test error path of Previous directive.
 */
TEST_F(AlexaPlaybackControllerCapabilityAgentTest, test_previousDirective_errorCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockPlaybackController, previous()).WillOnce(InvokeWithoutArgs([]() {
        acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response playbackControllerResponse(
            AlexaPlaybackControllerInterface::Response::Type::NO_CONTENT_AVAILABLE, "No content available for playing");
        return playbackControllerResponse;
    }));

    EXPECT_CALL(
        *m_mockResponseSender,
        sendErrorResponseEvent(
            _, _, _, Matcher<AlexaInterfaceMessageSenderInterface::ErrorResponseType>(testing::_), _));
    auto playbackControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(playbackControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_PREVIOUS);
    playbackControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    playbackControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    playbackControllerCapabilityAgent->shutdown();
}

/**
 * Test successful handling of Next directive.
 */
TEST_F(AlexaPlaybackControllerCapabilityAgentTest, test_nextDirective_successCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockPlaybackController, next()).WillOnce(InvokeWithoutArgs([]() {
        acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response playbackControllerResponse(
            AlexaPlaybackControllerInterface::Response::Type::SUCCESS, "");
        return playbackControllerResponse;
    }));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _));

    auto playbackControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(playbackControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_NEXT);
    playbackControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    playbackControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    playbackControllerCapabilityAgent->shutdown();
}

/**
 * Test error path of Next directive.
 */
TEST_F(AlexaPlaybackControllerCapabilityAgentTest, test_nextDirective_errorCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockPlaybackController, next()).WillOnce(InvokeWithoutArgs([]() {
        acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response playbackControllerResponse(
            AlexaPlaybackControllerInterface::Response::Type::NO_CONTENT_AVAILABLE, "No content available for playing");
        return playbackControllerResponse;
    }));

    EXPECT_CALL(
        *m_mockResponseSender,
        sendErrorResponseEvent(
            _, _, _, Matcher<AlexaInterfaceMessageSenderInterface::ErrorResponseType>(testing::_), _));

    auto playbackControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(playbackControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_NEXT);
    playbackControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    playbackControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    playbackControllerCapabilityAgent->shutdown();
}

/**
 * Test successful handling of Rewind directive.
 */
TEST_F(AlexaPlaybackControllerCapabilityAgentTest, test_rewindDirective_successCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockPlaybackController, rewind()).WillOnce(InvokeWithoutArgs([]() {
        acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response playbackControllerResponse(
            AlexaPlaybackControllerInterface::Response::Type::SUCCESS, "");
        return playbackControllerResponse;
    }));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _));

    auto playbackControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(playbackControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_REWIND);
    playbackControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    playbackControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    playbackControllerCapabilityAgent->shutdown();
}

/**
 * Test error path of Rewind directive.
 */
TEST_F(AlexaPlaybackControllerCapabilityAgentTest, test_rewindDirective_errorCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockPlaybackController, rewind()).WillOnce(InvokeWithoutArgs([]() {
        acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response playbackControllerResponse(
            AlexaPlaybackControllerInterface::Response::Type::NO_CONTENT_AVAILABLE, "No content available for playing");
        return playbackControllerResponse;
    }));

    EXPECT_CALL(
        *m_mockResponseSender,
        sendErrorResponseEvent(
            _, _, _, Matcher<AlexaInterfaceMessageSenderInterface::ErrorResponseType>(testing::_), _));
    auto playbackControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(playbackControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_REWIND);
    playbackControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    playbackControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    playbackControllerCapabilityAgent->shutdown();
}

/**
 * Test successful handling of FastForward directive.
 */
TEST_F(AlexaPlaybackControllerCapabilityAgentTest, test_fastForwardDirective_successCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockPlaybackController, fastForward()).WillOnce(InvokeWithoutArgs([]() {
        acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response playbackControllerResponse(
            AlexaPlaybackControllerInterface::Response::Type::SUCCESS, "");
        return playbackControllerResponse;
    }));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _));

    auto playbackControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(playbackControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_FAST_FORWARD);
    playbackControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    playbackControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    playbackControllerCapabilityAgent->shutdown();
}

/**
 * Test error path of FastForward directive.
 */
TEST_F(AlexaPlaybackControllerCapabilityAgentTest, test_fastForwardDirective_errorCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockPlaybackController, fastForward()).WillOnce(InvokeWithoutArgs([]() {
        acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response playbackControllerResponse(
            AlexaPlaybackControllerInterface::Response::Type::NO_CONTENT_AVAILABLE, "No content available for playing");
        return playbackControllerResponse;
    }));

    EXPECT_CALL(
        *m_mockResponseSender,
        sendErrorResponseEvent(
            _, _, _, Matcher<AlexaInterfaceMessageSenderInterface::ErrorResponseType>(testing::_), _));
    auto playbackControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(playbackControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_FAST_FORWARD);
    playbackControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    playbackControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    playbackControllerCapabilityAgent->shutdown();
}

/**
 * Tests unknown Directive.
 * Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(AlexaPlaybackControllerCapabilityAgentTest, test_unknownDirective) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    auto playbackControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(playbackControllerCapabilityAgent, NotNull());

    playbackControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(UNKNOWN_DIRECTIVE), std::move(m_mockDirectiveHandlerResult));
    playbackControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    playbackControllerCapabilityAgent->shutdown();
}

/**
 * Test triggering of reportStateChange and subsequent call to the
 * ContextManager to build the context.
 */
TEST_F(AlexaPlaybackControllerCapabilityAgentTest, test_reportStateChange_successCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;
    auto playbackControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(playbackControllerCapabilityAgent, NotNull());

    // Play case
    EXPECT_CALL(*m_mockPlaybackController, play()).WillOnce(InvokeWithoutArgs([this]() {
        m_observer.lock()->onPlaybackStateChanged(PlaybackState::PLAYING);
        acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response playbackControllerResponse(
            AlexaPlaybackControllerInterface::Response::Type::SUCCESS, "");
        return playbackControllerResponse;
    }));

    EXPECT_CALL(*m_mockContextManager, reportStateChange(_, _, _))
        .WillOnce(InvokeWithoutArgs([playbackControllerCapabilityAgent] {
            const unsigned int stateRequestToken = 1;
            playbackControllerCapabilityAgent->provideState(
                CapabilityTag(NAMESPACE_STATE_REPORT, STATE_REPORTER_PROPERTY, TEST_ENDPOINT_ID), stateRequestToken);
        }));

    EXPECT_CALL(*m_mockPlaybackController, getPlaybackState())
        .WillOnce(InvokeWithoutArgs(
            []() -> MockAlexaPlaybackControllerInterface::GetPlaybackState { return PlaybackState::PLAYING; }));

    EXPECT_CALL(*m_mockContextManager, provideStateResponse(_, _, _)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    m_mockPlaybackController->play();

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    playbackControllerCapabilityAgent->shutdown();
}

/**
 * Test triggering of reportStateChange and subsequent call to get the getPlaybackState and
 * call to the ContextManager to report the new playback state.
 */
TEST_F(AlexaPlaybackControllerCapabilityAgentTest, test_reportStateChange_errorCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;
    auto playbackControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(playbackControllerCapabilityAgent, NotNull());

    // Play case
    EXPECT_CALL(*m_mockPlaybackController, play()).WillOnce(InvokeWithoutArgs([this]() {
        m_observer.lock()->onPlaybackStateChanged(PlaybackState::PLAYING);
        AlexaPlaybackControllerInterface::Response playbackControllerResponse{
            AlexaPlaybackControllerInterface::Response::Type::PLAYBACK_OPERATION_NOT_SUPPORTED, ""};
        return playbackControllerResponse;
    }));

    EXPECT_CALL(*m_mockContextManager, reportStateChange(_, _, _))
        .WillOnce(InvokeWithoutArgs([playbackControllerCapabilityAgent] {
            const unsigned int stateRequestToken = 1;
            playbackControllerCapabilityAgent->provideState(
                CapabilityTag(NAMESPACE_STATE_REPORT, STATE_REPORTER_PROPERTY, TEST_ENDPOINT_ID), stateRequestToken);
        }));

    EXPECT_CALL(*m_mockPlaybackController, getPlaybackState())
        .WillOnce(InvokeWithoutArgs(
            []() -> MockAlexaPlaybackControllerInterface::GetPlaybackState { return PlaybackState::STOPPED; }));

    EXPECT_CALL(*m_mockContextManager, provideStateResponse(_, _, _)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    m_mockPlaybackController->play();

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    playbackControllerCapabilityAgent->shutdown();
}

/**
 * Tests unknown Directive with both proactively reported and retrievable set.
 * Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(AlexaPlaybackControllerCapabilityAgentTest, test_unknownDirectiveWithProactivelyReportedAndRetrievableTrue) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    auto playbackControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(playbackControllerCapabilityAgent, NotNull());

    playbackControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(UNKNOWN_DIRECTIVE), std::move(m_mockDirectiveHandlerResult));
    playbackControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    playbackControllerCapabilityAgent->shutdown();
}

/**
 * Tests unknown Directive with both proactively reported and retrievable as false.
 * Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(AlexaPlaybackControllerCapabilityAgentTest, test_unknownDirectiveWithProactivelyReportedAndRetrievableFalse) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    auto playbackControllerCapabilityAgent = createCapabilityAgentAndSetExpects(false, false);
    ASSERT_THAT(playbackControllerCapabilityAgent, NotNull());

    playbackControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(UNKNOWN_DIRECTIVE), std::move(m_mockDirectiveHandlerResult));
    playbackControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    playbackControllerCapabilityAgent->shutdown();
}

}  // namespace test
}  // namespace acsdkAlexaPlaybackController
}  // namespace alexaClientSDK
