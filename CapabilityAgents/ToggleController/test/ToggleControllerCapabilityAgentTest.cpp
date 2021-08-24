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

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "ToggleController/ToggleControllerAttributeBuilder.h"
#include "ToggleController/ToggleControllerCapabilityAgent.h"

#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockAlexaInterfaceMessageSender.h>
#include <AVSCommon/SDKInterfaces/ToggleController/ToggleControllerAttributes.h>
#include <AVSCommon/SDKInterfaces/ToggleController/ToggleControllerInterface.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/Timing/TimePoint.h>
#include <AVSCommon/Utils/WaitEvent.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace toggleController {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment::test;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::toggleController;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils::memory;
using namespace rapidjson;
using namespace ::testing;

// For readability.
using EndpointIdentifier = endpoints::EndpointIdentifier;

/// Timeout when waiting for futures to be set.
static std::chrono::milliseconds TIMEOUT(1000);

/// The namespace for capability agent.
static const std::string NAMESPACE{"Alexa.ToggleController"};

/// The supported version
static const std::string INTERFACE_VERSION{"3"};

/// The name for TurnOn directive
static const std::string NAME_TURNON{"TurnOn"};

/// The name for TurnOff directive
static const std::string NAME_TURNOFF{"TurnOff"};

/// The name for toggleState property
static const std::string TOGGLESTATE_PROPERTY_NAME{"toggleState"};

/// An unknown directive signature.
static const std::string UNKNOWN_DIRECTIVE{"Unknown"};

/// The test EndpointId
static const EndpointIdentifier TEST_ENDPOINT_ID("testEndpointId");

/// The test instance
static const EndpointIdentifier TEST_INSTANCE("testInstance");

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

/// Time of sample key
static const std::string TIME_OF_SAMPLE("timeOfSample");

/// Time of sample used for testing
static const std::string TIME_OF_SAMPLE_TEST("2017-02-03T16:20:50.523Z");

class MockToggleControllerInterface : public ToggleControllerInterface {
public:
    using SetToggleStateResult = std::pair<AlexaResponseType, std::string>;
    using GetToggleStateResult = std::pair<AlexaResponseType, avsCommon::utils::Optional<ToggleState>>;

    MOCK_METHOD2(setToggleState, SetToggleStateResult(const bool state, const AlexaStateChangeCauseType cause));
    MOCK_METHOD0(getToggleState, GetToggleStateResult());
    MOCK_METHOD1(addObserver, bool(std::shared_ptr<ToggleControllerObserverInterface>));
    MOCK_METHOD1(removeObserver, void(const std::shared_ptr<ToggleControllerObserverInterface>&));
};

class ToggleControllerCapabilityAgentTest : public ::testing::Test {
public:
    /// SetUp before each test case.
    void SetUp() override;

    /// Constructor.
    ToggleControllerCapabilityAgentTest() {
    }

    /// build toggleControllerAttributes
    avsCommon::utils::Optional<avsCommon::sdkInterfaces::toggleController::ToggleControllerAttributes>
    buildToggleControllerAttribute(const avsCommon::avs::CapabilityResources& capabilityResources) {
        auto toggleControllerAttributeBuilder =
            capabilityAgents::toggleController::ToggleControllerAttributeBuilder::create();
        toggleControllerAttributeBuilder->withCapabilityResources(capabilityResources);
        return toggleControllerAttributeBuilder->build();
    }

protected:
    /**
     * Function to create ToggleControllerCapabilityAgent based on the reportable flags
     * and sets required ContextManager and ToggleController mocks.
     */
    std::shared_ptr<ToggleControllerCapabilityAgent> createCapabilityAgentAndSetExpects(
        const avsCommon::sdkInterfaces::toggleController::ToggleControllerAttributes& toggleControllerAttributes,
        bool proactivelyReported,
        bool retrievable,
        bool nonControllable);

    /// The test @c ToggleState as ON
    ToggleControllerInterface::ToggleState TEST_POWER_STATE_ON;

    /// The test @c ToggleState as OFF
    ToggleControllerInterface::ToggleState TEST_POWER_STATE_OFF;

    /// A mock @c ToggleContollerInterface object.
    std::shared_ptr<StrictMock<MockToggleControllerInterface>> m_mockToggleController;

    /// A mock @c ToggleContollerObserverInterface object.
    std::shared_ptr<ToggleControllerObserverInterface> m_observer;

    // a context manager
    std::shared_ptr<StrictMock<MockContextManager>> m_mockContextManager;

    /// Response sender mock to track events being sent.
    std::shared_ptr<MockAlexaInterfaceMessageSender> m_mockResponseSender;

    // A strict mock that allows the test to strictly monitor the exceptions being sent.
    std::shared_ptr<StrictMock<MockExceptionEncounteredSender>> m_mockExceptionSender;

    /// A strict mock that allows the test to strictly monitor the handling of directives.
    std::unique_ptr<StrictMock<MockDirectiveHandlerResult>> m_mockDirectiveHandlerResult;
};

void ToggleControllerCapabilityAgentTest::SetUp() {
    m_mockExceptionSender = std::make_shared<StrictMock<MockExceptionEncounteredSender>>();
    m_mockDirectiveHandlerResult = make_unique<StrictMock<MockDirectiveHandlerResult>>();
    m_mockToggleController = std::make_shared<StrictMock<MockToggleControllerInterface>>();
    m_mockContextManager = std::make_shared<testing::StrictMock<MockContextManager>>();
    m_mockResponseSender = std::make_shared<MockAlexaInterfaceMessageSender>();

    auto timePoint = avsCommon::utils::timing::TimePoint();
    timePoint.setTime_ISO_8601(TIME_OF_SAMPLE_TEST);

    TEST_POWER_STATE_ON = ToggleControllerInterface::ToggleState{true, timePoint, std::chrono::milliseconds(0)};
    TEST_POWER_STATE_OFF = ToggleControllerInterface::ToggleState{false, timePoint, std::chrono::milliseconds(0)};
}

std::shared_ptr<ToggleControllerCapabilityAgent> ToggleControllerCapabilityAgentTest::
    createCapabilityAgentAndSetExpects(
        const avsCommon::sdkInterfaces::toggleController::ToggleControllerAttributes& toggleControllerAttributes,
        bool proactivelyReported,
        bool retrievable,
        bool nonControllable) {
    // ContextManager Mocks
    if (retrievable) {
        // Expects a non-null CA reference during create.
        EXPECT_CALL(*m_mockContextManager, addStateProvider(_, ::testing::NotNull()));

        // Expects a null when CA is shutdown.
        EXPECT_CALL(*m_mockContextManager, removeStateProvider(_));
    }

    if (proactivelyReported) {
        // addObserver Called during the initialization.
        EXPECT_CALL(*m_mockToggleController, addObserver(_))
            .WillOnce(WithArg<0>(Invoke([this](std::shared_ptr<ToggleControllerObserverInterface> observer) {
                m_observer = observer;
                return true;
            })));

        // removeObserver is called during the shutdown
        EXPECT_CALL(*m_mockToggleController, removeObserver(_))
            .WillOnce(WithArg<0>(
                Invoke([this](std::shared_ptr<ToggleControllerObserverInterface> observer) { m_observer = nullptr; })));
    }

    auto toggleControllerCapabilityAgent = ToggleControllerCapabilityAgent::create(
        TEST_ENDPOINT_ID,
        TEST_INSTANCE,
        toggleControllerAttributes,
        m_mockToggleController,
        m_mockContextManager,
        m_mockResponseSender,
        m_mockExceptionSender,
        proactivelyReported,
        retrievable,
        nonControllable);
    return toggleControllerCapabilityAgent;
}

/**
 * Utility function to build the @c AVSDirective for test.
 *
 * @param directiveName The name of the test directive to build.
 *
 * @return The created AVSDirective object or @c nullptr if build failed.
 */
static std::shared_ptr<AVSDirective> buildAVSDirective(std::string directiveName) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE,
        directiveName,
        MESSAGE_ID_TEST,
        DIALOG_REQUEST_ID_TEST,
        CORRELATION_TOKEN_TEST,
        EVENT_CORRELATION_TOKEN_TEST,
        INTERFACE_VERSION,
        TEST_INSTANCE);
    auto avsMessageEndpoint = AVSMessageEndpoint(TEST_ENDPOINT_ID);

    return AVSDirective::create("", avsMessageHeader, "", attachmentManager, "", avsMessageEndpoint);
}

/// Test that create() returns a nullptr if called with invalid arguments.
TEST_F(ToggleControllerCapabilityAgentTest, test_givenInvalidParameters_create_shouldFail) {
    CapabilityResources emptyResource;
    auto emptyToggleControllerAttribute = buildToggleControllerAttribute(emptyResource);
    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("light", "en-US"));
    auto toggleControllerAttributes = buildToggleControllerAttribute(resource);
    ASSERT_TRUE(toggleControllerAttributes.hasValue());

    EXPECT_THAT(
        ToggleControllerCapabilityAgent::create(
            "", "", emptyToggleControllerAttribute.value(), nullptr, nullptr, nullptr, nullptr, true, true),
        IsNull());
    EXPECT_THAT(
        ToggleControllerCapabilityAgent::create(
            "",
            TEST_INSTANCE,
            toggleControllerAttributes.value(),
            m_mockToggleController,
            m_mockContextManager,
            m_mockResponseSender,
            m_mockExceptionSender,
            true,
            true),
        IsNull());
    EXPECT_THAT(
        ToggleControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID,
            "",
            toggleControllerAttributes.value(),
            m_mockToggleController,
            m_mockContextManager,
            m_mockResponseSender,
            m_mockExceptionSender,
            true,
            true),
        IsNull());
    EXPECT_THAT(
        ToggleControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID,
            TEST_INSTANCE,
            emptyToggleControllerAttribute.value(),
            m_mockToggleController,
            m_mockContextManager,
            m_mockResponseSender,
            m_mockExceptionSender,
            true,
            true),
        IsNull());
    EXPECT_THAT(
        ToggleControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID,
            TEST_INSTANCE,
            toggleControllerAttributes.value(),
            nullptr,
            m_mockContextManager,
            m_mockResponseSender,
            m_mockExceptionSender,
            true,
            true),
        IsNull());
    EXPECT_THAT(
        ToggleControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID,
            TEST_INSTANCE,
            toggleControllerAttributes.value(),
            m_mockToggleController,
            nullptr,
            m_mockResponseSender,
            m_mockExceptionSender,
            true,
            true),
        IsNull());
    EXPECT_THAT(
        ToggleControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID,
            TEST_INSTANCE,
            toggleControllerAttributes.value(),
            m_mockToggleController,
            m_mockContextManager,
            nullptr,
            m_mockExceptionSender,
            true,
            true),
        IsNull());
    EXPECT_THAT(
        ToggleControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID,
            TEST_INSTANCE,
            toggleControllerAttributes.value(),
            m_mockToggleController,
            m_mockContextManager,
            m_mockResponseSender,
            nullptr,
            true,
            true),
        IsNull());
}

/**
 * Test successful handling of TurnOn directive.
 */
TEST_F(ToggleControllerCapabilityAgentTest, test_turnOnDirective_successCase) {
    avsCommon::utils::WaitEvent waitEvent;
    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("light", "en-US"));
    auto toggleControllerAttributes = buildToggleControllerAttribute(resource);
    ASSERT_TRUE(toggleControllerAttributes.hasValue());

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockToggleController, setToggleState(true, _))
        .WillOnce(WithArg<1>(
            Invoke([](AlexaStateChangeCauseType cause) { return std::make_pair(AlexaResponseType::SUCCESS, ""); })));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _));

    auto toggleControllerCapabilityAgent =
        createCapabilityAgentAndSetExpects(toggleControllerAttributes.value(), false, true, false);
    ASSERT_THAT(toggleControllerCapabilityAgent, NotNull());

    // Simulate directive
    toggleControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(NAME_TURNON), std::move(m_mockDirectiveHandlerResult));
    toggleControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    toggleControllerCapabilityAgent->shutdown();
}

/**
 * Test error path of TurnOn directive.
 */
TEST_F(ToggleControllerCapabilityAgentTest, test_turnOnDirective_errorCase) {
    avsCommon::utils::WaitEvent waitEvent;
    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("light", "en-US"));
    auto toggleControllerAttributes = buildToggleControllerAttribute(resource);
    ASSERT_TRUE(toggleControllerAttributes.hasValue());

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockToggleController, setToggleState(true, _))
        .WillOnce(WithArg<1>(Invoke([](AlexaStateChangeCauseType cause) {
            return std::make_pair(AlexaResponseType::ENDPOINT_UNREACHABLE, "TestEndpointNotReachable");
        })));

    EXPECT_CALL(
        *m_mockResponseSender,
        sendErrorResponseEvent(_, _, _, Matcher<AlexaInterfaceMessageSenderInterface::ErrorResponseType>(_), _));

    auto toggleControllerCapabilityAgent =
        createCapabilityAgentAndSetExpects(toggleControllerAttributes.value(), true, true, false);
    ASSERT_THAT(toggleControllerCapabilityAgent, NotNull());

    // Simulate directive
    toggleControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(NAME_TURNON), std::move(m_mockDirectiveHandlerResult));
    toggleControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    toggleControllerCapabilityAgent->shutdown();
}

/**
 * Test successful handling of TurnOff directive.
 */
TEST_F(ToggleControllerCapabilityAgentTest, test_turnOffDirective_successCase) {
    avsCommon::utils::WaitEvent waitEvent;
    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("light", "en-US"));
    auto toggleControllerAttributes = buildToggleControllerAttribute(resource);
    ASSERT_TRUE(toggleControllerAttributes.hasValue());

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockToggleController, setToggleState(false, _))
        .WillOnce(WithArg<1>(
            Invoke([](AlexaStateChangeCauseType cause) { return std::make_pair(AlexaResponseType::SUCCESS, ""); })));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _));

    auto toggleControllerCapabilityAgent =
        createCapabilityAgentAndSetExpects(toggleControllerAttributes.value(), false, true, false);
    ASSERT_THAT(toggleControllerCapabilityAgent, NotNull());

    // Simulate directive
    toggleControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(NAME_TURNOFF), std::move(m_mockDirectiveHandlerResult));
    toggleControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    toggleControllerCapabilityAgent->shutdown();
}

/**
 * Test error path of TurnOff directive.
 */
TEST_F(ToggleControllerCapabilityAgentTest, test_turnOffDirective_errorCase) {
    avsCommon::utils::WaitEvent waitEvent;
    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("light", "en-US"));
    auto toggleControllerAttributes = buildToggleControllerAttribute(resource);
    ASSERT_TRUE(toggleControllerAttributes.hasValue());

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockToggleController, setToggleState(false, _))
        .WillOnce(WithArg<1>(Invoke([](AlexaStateChangeCauseType cause) {
            return std::make_pair(AlexaResponseType::ENDPOINT_UNREACHABLE, "TestEndpointNotReachable");
        })));

    EXPECT_CALL(
        *m_mockResponseSender,
        sendErrorResponseEvent(_, _, _, Matcher<AlexaInterfaceMessageSenderInterface::ErrorResponseType>(_), _));

    auto toggleControllerCapabilityAgent =
        createCapabilityAgentAndSetExpects(toggleControllerAttributes.value(), true, true, false);
    ASSERT_THAT(toggleControllerCapabilityAgent, NotNull());

    // Simulate directive
    toggleControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(NAME_TURNOFF), std::move(m_mockDirectiveHandlerResult));
    toggleControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    toggleControllerCapabilityAgent->shutdown();
}

/**
 * Test triggering of reportStateChange and subsequent call to the
 * ContextManager to build the context.
 */
TEST_F(ToggleControllerCapabilityAgentTest, test_reportStateChange_successCase) {
    avsCommon::utils::WaitEvent waitEvent;
    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("light", "en-US"));
    auto toggleControllerAttributes = buildToggleControllerAttribute(resource);
    ASSERT_TRUE(toggleControllerAttributes.hasValue());

    auto toggleControllerCapabilityAgent =
        createCapabilityAgentAndSetExpects(toggleControllerAttributes.value(), true, true, false);
    ASSERT_THAT(toggleControllerCapabilityAgent, NotNull());

    EXPECT_CALL(*m_mockToggleController, setToggleState(true, _))
        .WillOnce(WithArg<1>(Invoke([this](AlexaStateChangeCauseType cause) {
            m_observer->onToggleStateChanged(TEST_POWER_STATE_ON, cause);
            return std::make_pair(AlexaResponseType::SUCCESS, "");
        })));

    EXPECT_CALL(*m_mockContextManager, reportStateChange(_, _, _))
        .WillOnce(InvokeWithoutArgs([toggleControllerCapabilityAgent] {
            const unsigned int stateRequestToken = 1;
            toggleControllerCapabilityAgent->provideState(
                CapabilityTag(NAMESPACE, TOGGLESTATE_PROPERTY_NAME, TEST_ENDPOINT_ID, TEST_INSTANCE),
                stateRequestToken);
        }));

    EXPECT_CALL(*m_mockToggleController, getToggleState())
        .WillOnce(InvokeWithoutArgs([this]() -> MockToggleControllerInterface::GetToggleStateResult {
            return std::make_pair(
                AlexaResponseType::SUCCESS,
                avsCommon::utils::Optional<ToggleControllerInterface::ToggleState>(TEST_POWER_STATE_ON));
        }));

    EXPECT_CALL(*m_mockContextManager, provideStateResponse(_, _, _)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    m_mockToggleController->setToggleState(true, AlexaStateChangeCauseType::APP_INTERACTION);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    toggleControllerCapabilityAgent->shutdown();
}

/**
 * Test triggering of reportStateChange and subsequent failure to get the getToggleState and
 * call to the ContextManager to report the failure.
 */
TEST_F(ToggleControllerCapabilityAgentTest, test_reportStateChange_errorCase) {
    avsCommon::utils::WaitEvent waitEvent;
    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("light", "en-US"));
    auto toggleControllerAttributes = buildToggleControllerAttribute(resource);
    ASSERT_TRUE(toggleControllerAttributes.hasValue());

    auto toggleControllerCapabilityAgent =
        createCapabilityAgentAndSetExpects(toggleControllerAttributes.value(), true, true, false);
    ASSERT_THAT(toggleControllerCapabilityAgent, NotNull());

    EXPECT_CALL(*m_mockToggleController, setToggleState(true, _))
        .WillOnce(WithArg<1>(Invoke([this](AlexaStateChangeCauseType cause) {
            m_observer->onToggleStateChanged(TEST_POWER_STATE_ON, cause);
            return std::make_pair(AlexaResponseType::SUCCESS, "");
        })));

    EXPECT_CALL(*m_mockContextManager, reportStateChange(_, _, _))
        .WillOnce(InvokeWithoutArgs([toggleControllerCapabilityAgent] {
            const unsigned int stateRequestToken = 1;
            toggleControllerCapabilityAgent->provideState(
                CapabilityTag(NAMESPACE, TOGGLESTATE_PROPERTY_NAME, TEST_ENDPOINT_ID, TEST_INSTANCE),
                stateRequestToken);
        }));

    EXPECT_CALL(*m_mockToggleController, getToggleState())
        .WillOnce(InvokeWithoutArgs([]() -> MockToggleControllerInterface::GetToggleStateResult {
            return std::make_pair(
                AlexaResponseType::ENDPOINT_UNREACHABLE,
                avsCommon::utils::Optional<ToggleControllerInterface::ToggleState>());
        }));

    EXPECT_CALL(*m_mockContextManager, provideStateUnavailableResponse(_, _, _))
        .WillOnce(InvokeWithoutArgs([&waitEvent]() { waitEvent.wakeUp(); }));

    m_mockToggleController->setToggleState(true, AlexaStateChangeCauseType::APP_INTERACTION);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    toggleControllerCapabilityAgent->shutdown();
}

/**
 * Tests unknown Directive with both proactively reported and retrievable set.
 * Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(ToggleControllerCapabilityAgentTest, test_unknownDirectiveWithProactivelyReportedAndRetrievableTrue) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("light", "en-US"));
    auto toggleControllerAttributes = buildToggleControllerAttribute(resource);
    ASSERT_TRUE(toggleControllerAttributes.hasValue());

    auto toggleControllerCapabilityAgent =
        createCapabilityAgentAndSetExpects(toggleControllerAttributes.value(), true, true, false);
    ASSERT_THAT(toggleControllerCapabilityAgent, NotNull());

    toggleControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(UNKNOWN_DIRECTIVE), std::move(m_mockDirectiveHandlerResult));
    toggleControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    toggleControllerCapabilityAgent->shutdown();
}

/**
 * Tests unknown Directive with both proactively reported and retrievable as false.
 * Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(ToggleControllerCapabilityAgentTest, test_unknownDirectiveWithProactivelyReportedAndRetrievableFalse) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("light", "en-US"));
    auto toggleControllerAttributes = buildToggleControllerAttribute(resource);
    ASSERT_TRUE(toggleControllerAttributes.hasValue());

    auto toggleControllerCapabilityAgent =
        createCapabilityAgentAndSetExpects(toggleControllerAttributes.value(), false, false, false);
    toggleControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(UNKNOWN_DIRECTIVE), std::move(m_mockDirectiveHandlerResult));
    toggleControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    toggleControllerCapabilityAgent->shutdown();
}

}  // namespace test
}  // namespace toggleController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
