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

#include "acsdkAlexaLauncher/AlexaLauncherCapabilityAgent.h"
#include "acsdkAlexaLauncherInterfaces/AlexaLauncherInterface.h"
#include "acsdkAlexaLauncherInterfaces/AlexaLauncherObserverInterface.h"

#include "AVSCommon/AVS/Attachment/MockAttachmentManager.h"
#include "AVSCommon/SDKInterfaces/MockContextManager.h"
#include "AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h"
#include "AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h"
#include "AVSCommon/SDKInterfaces/MockAlexaInterfaceMessageSender.h"

#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/Timing/TimePoint.h>
#include <AVSCommon/Utils/WaitEvent.h>

namespace alexaClientSDK {
namespace acsdkAlexaLauncher {
namespace test {

using namespace alexaClientSDK::avsCommon::avs;
using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::sdkInterfaces;
using namespace alexaClientSDK::avsCommon::utils::memory;
using namespace ::testing;

using namespace acsdkAlexaLauncher;
using namespace acsdkAlexaLauncherInterfaces;

// For readability.
using EndpointIdentifier = endpoints::EndpointIdentifier;

/// Timeout when waiting for futures to be set.
static std::chrono::milliseconds TIMEOUT(1000);

/// Time of sample used for testing
static const std::string TIME_OF_SAMPLE_TEST("2017-02-03T16:20:50.523Z");

/// The namespace for capability agent.
static const std::string NAMESPACE{"Alexa.Launcher"};

/// The supported version
static const std::string INTERFACE_VERSION{"3"};

/// The name for LaunchTarget directive
static const std::string NAME_LAUNCHTARGET{"LaunchTarget"};

/// An unknown directive signature.
static const std::string UNKNOWN_DIRECTIVE{"Unknown"};

/// The test EndpointId
static const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier TEST_ENDPOINT_ID("testEndpointId");

/// Event key.
static const std::string EVENT("event");

/// MessageId for testing.
static const std::string MESSAGE_ID_TEST("MessageId_Test");

/// DialogRequestId for testing.
static const std::string DIALOG_REQUEST_ID_TEST("DialogRequestId_Test");

/// Correlation token for testing.
static const std::string CORRELATION_TOKEN_TEST("CorrelationToken_Test");

// Sample LaunchTarget payload
static const std::string LAUNCH_TARGET_PAYLOD = R"delim( {
            "identifier": "amzn1.alexa-ask-target.app.72095",
            "name": "Amazon Video" } )delim";

class MockAlexaLauncherInterface : public AlexaLauncherInterface {
public:
    using LaunchTarget = acsdkAlexaLauncherInterfaces::AlexaLauncherInterface::Response;
    using GetLauncherTargetState = acsdkAlexaLauncherInterfaces::TargetState;

    MOCK_METHOD1(launchTarget, LaunchTarget(const acsdkAlexaLauncherInterfaces::TargetState& targetState));
    MOCK_METHOD0(getLauncherTargetState, GetLauncherTargetState());
    MOCK_METHOD1(addObserver, bool(const std::weak_ptr<AlexaLauncherObserverInterface>&));
    MOCK_METHOD1(removeObserver, void(const std::weak_ptr<AlexaLauncherObserverInterface>&));
};

class AlexaLauncherCapabilityAgentTest : public ::testing::Test {
public:
    /// SetUp before each test case.
    void SetUp() override;

    /// Clean up the test harness after running a test.
    void TearDown() override;

    /// Constructor.
    AlexaLauncherCapabilityAgentTest() {
    }

protected:
    /**
     * Function to create AlexaLauncherCapabilityAgent based on the reportable flags
     * and sets required ContextManager and Launcher mocks.
     */
    std::shared_ptr<AlexaLauncherCapabilityAgent> createCapabilityAgentAndSetExpects(bool provideState);

    /// A mock @c AlexaLauncherInterface object.
    std::shared_ptr<StrictMock<MockAlexaLauncherInterface>> m_mockLauncher;

    // a context manager
    std::shared_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockContextManager>> m_mockContextManager;

    /// The mock @c MessageSenderInterface.
    std::shared_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockAlexaInterfaceMessageSender>> m_mockResponseSender;

    /// A strict mock that allows the test to strictly monitor the exceptions being sent.
    std::shared_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender>> m_mockExceptionSender;

    /// A strict mock that allows the test to strictly monitor the handling of directives.
    std::unique_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>>
        m_mockDirectiveHandlerResult;

    /// The test @c acsdkAlexaLauncherInterfaces::TargetState
    acsdkAlexaLauncherInterfaces::TargetState m_targetState;
};

void AlexaLauncherCapabilityAgentTest::SetUp() {
    m_mockExceptionSender =
        std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender>>();
    m_mockDirectiveHandlerResult =
        make_unique<StrictMock<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>>();
    m_mockLauncher = std::make_shared<StrictMock<MockAlexaLauncherInterface>>();
    m_mockContextManager = std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockContextManager>>();
    m_mockResponseSender =
        std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockAlexaInterfaceMessageSender>>();

    auto timePoint = alexaClientSDK::avsCommon::utils::timing::TimePoint();
    timePoint.setTime_ISO_8601(TIME_OF_SAMPLE_TEST);

    // setup sample target object for generating acsdkAlexaLauncherInterfaces::TargetState
    m_targetState = acsdkAlexaLauncherInterfaces::TargetState{"Amazon Video", "amzn1.alexa-ask-target.app.72095"};
}

void AlexaLauncherCapabilityAgentTest::TearDown() {
}

std::shared_ptr<AlexaLauncherCapabilityAgent> AlexaLauncherCapabilityAgentTest::createCapabilityAgentAndSetExpects(
    bool provideState) {
    // ContextManager Mocks
    if (provideState) {
        // Expects a non-null CA reference during create.
        EXPECT_CALL(*m_mockContextManager, addStateProvider(_, ::testing::NotNull()));

        // Expects a null when CA is shutdown.
        EXPECT_CALL(*m_mockContextManager, removeStateProvider(_));
    }

    EXPECT_CALL(*m_mockLauncher, addObserver(_)).WillOnce(InvokeWithoutArgs([]() { return true; }));

    auto launcherCapabilityAgent = AlexaLauncherCapabilityAgent::create(
        TEST_ENDPOINT_ID,
        m_mockLauncher,
        m_mockContextManager,
        m_mockResponseSender,
        m_mockExceptionSender,
        true,
        true);
    return launcherCapabilityAgent;
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
TEST_F(AlexaLauncherCapabilityAgentTest, test_givenInvalidParameters_create_shouldFail) {
    ASSERT_THAT(
        AlexaLauncherCapabilityAgent::create(
            "", m_mockLauncher, m_mockContextManager, m_mockResponseSender, m_mockExceptionSender, true, true),
        IsNull());
    ASSERT_THAT(
        AlexaLauncherCapabilityAgent::create(
            TEST_ENDPOINT_ID, nullptr, m_mockContextManager, m_mockResponseSender, m_mockExceptionSender, true, true),
        IsNull());
    ASSERT_THAT(
        AlexaLauncherCapabilityAgent::create(
            TEST_ENDPOINT_ID, m_mockLauncher, nullptr, m_mockResponseSender, m_mockExceptionSender, true, true),
        IsNull());
    ASSERT_THAT(
        AlexaLauncherCapabilityAgent::create(
            TEST_ENDPOINT_ID, m_mockLauncher, m_mockContextManager, nullptr, m_mockExceptionSender, true, true),
        IsNull());
    ASSERT_THAT(
        AlexaLauncherCapabilityAgent::create(
            TEST_ENDPOINT_ID, m_mockLauncher, m_mockContextManager, m_mockResponseSender, nullptr, true, true),
        IsNull());
}

/**
 * Test successful handling of LaunchTarget directive.
 */
TEST_F(AlexaLauncherCapabilityAgentTest, test_launchTargetDirective_successCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    auto launcherCapabilityAgent = createCapabilityAgentAndSetExpects(true);
    ASSERT_THAT(launcherCapabilityAgent, NotNull());

    EXPECT_CALL(*m_mockLauncher, launchTarget(_)).WillOnce(WithArg<0>(Invoke([](const TargetState& targetState) {
        acsdkAlexaLauncherInterfaces::AlexaLauncherInterface::Response launcherResponse(
            AlexaLauncherInterface::Response::Type::SUCCESS, "");
        return launcherResponse;
    })));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _));

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_LAUNCHTARGET, LAUNCH_TARGET_PAYLOD);
    launcherCapabilityAgent->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    launcherCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    launcherCapabilityAgent->shutdown();
}

/**
 * Test error path of LaunchTarget directive.
 */
TEST_F(AlexaLauncherCapabilityAgentTest, test_launchTargetDirective_errorCase) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockLauncher, launchTarget(_)).WillOnce(WithArg<0>(Invoke([](const TargetState& targetState) {
        acsdkAlexaLauncherInterfaces::AlexaLauncherInterface::Response launcherResponse(
            AlexaLauncherInterface::Response::Type::INTERNAL_ERROR, "TestEndpointNotReachable");
        return launcherResponse;
    })));

    EXPECT_CALL(
        *m_mockResponseSender,
        sendErrorResponseEvent(
            _, _, _, Matcher<AlexaInterfaceMessageSenderInterface::ErrorResponseType>(testing::_), _));
    auto launcherCapabilityAgent = createCapabilityAgentAndSetExpects(true);
    ASSERT_THAT(launcherCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_LAUNCHTARGET, LAUNCH_TARGET_PAYLOD);
    launcherCapabilityAgent->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    launcherCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    launcherCapabilityAgent->shutdown();
}

/**
 * Tests unknown Directive.
 * Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(AlexaLauncherCapabilityAgentTest, test_unknownDirective) {
    alexaClientSDK::avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    auto launcherCapabilityAgent = createCapabilityAgentAndSetExpects(true);
    ASSERT_THAT(launcherCapabilityAgent, NotNull());

    launcherCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(UNKNOWN_DIRECTIVE, ""), std::move(m_mockDirectiveHandlerResult));
    launcherCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    launcherCapabilityAgent->shutdown();
}
}  // namespace test
}  // namespace acsdkAlexaLauncher
}  // namespace alexaClientSDK
