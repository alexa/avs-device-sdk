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

#include "PowerController/PowerControllerCapabilityAgent.h"

#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockAlexaInterfaceMessageSender.h>
#include <AVSCommon/SDKInterfaces/PowerController/PowerControllerInterface.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/Timing/TimePoint.h>
#include <AVSCommon/Utils/WaitEvent.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace powerController {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment::test;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::powerController;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils::memory;
using namespace ::testing;

// For readability.
using EndpointIdentifier = endpoints::EndpointIdentifier;

/// Timeout when waiting for futures to be set.
static std::chrono::milliseconds TIMEOUT(1000);

/// The namespace for capability agent.
static const std::string NAMESPACE{"Alexa.PowerController"};

/// The supported version
static const std::string INTERFACE_VERSION{"3"};

/// The name for TurnOn directive
static const std::string NAME_TURNON{"TurnOn"};

/// The name for TurnOff directive
static const std::string NAME_TURNOFF{"TurnOff"};

/// The name for powerState property
static const std::string POWERSTATE_PROPERTY_NAME{"powerState"};

/// Json value for ON power state
static const std::string POWERSTATE_ON{R"("ON")"};

/// Json value for OFF power state
static const std::string POWERSTATE_OFF{R"("OFF")"};

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

/// Time of sample key
static const std::string TIME_OF_SAMPLE("timeOfSample");

/// Time of sample used for testing
static const std::string TIME_OF_SAMPLE_TEST("2017-02-03T16:20:50.523Z");

class MockPowerControllerInterface : public PowerControllerInterface {
public:
    using SetPowerStateResult = std::pair<AlexaResponseType, std::string>;
    using GetPowerStateResult = std::pair<AlexaResponseType, avsCommon::utils::Optional<PowerState>>;

    MOCK_METHOD2(setPowerState, SetPowerStateResult(const bool state, const AlexaStateChangeCauseType cause));
    MOCK_METHOD0(getPowerState, GetPowerStateResult());
    MOCK_METHOD1(addObserver, bool(std::shared_ptr<PowerControllerObserverInterface>));
    MOCK_METHOD1(removeObserver, void(const std::shared_ptr<PowerControllerObserverInterface>&));
};

class PowerControllerCapabilityAgentTest : public ::testing::Test {
public:
    /// SetUp before each test case.
    void SetUp() override;

    /// Clean up the test harness after running a test.
    void TearDown() override;

    /// Constructor.
    PowerControllerCapabilityAgentTest() {
    }

protected:
    /**
     * Function to create PowerControllerCapabilityAgent based on the reportable flags
     * and sets required ContextManager and PowerController mocks.
     */
    std::shared_ptr<PowerControllerCapabilityAgent> createCapabilityAgentAndSetExpects(
        bool proactivelyReported,
        bool retrievable);

    /// The test @c PowerState as ON
    PowerControllerInterface::PowerState m_testPowerStateOn;

    /// A mock @c PowerContollerInterface object.
    std::shared_ptr<StrictMock<MockPowerControllerInterface>> m_mockPowerController;

    /// A mock @c PowerContollerObserverInterface object.
    std::shared_ptr<PowerControllerObserverInterface> m_observer;

    // a context manager
    std::shared_ptr<StrictMock<MockContextManager>> m_mockContextManager;

    /// Response sender mock to track events being sent.
    std::shared_ptr<MockAlexaInterfaceMessageSender> m_mockResponseSender;

    // A strict mock that allows the test to strictly monitor the exceptions being sent.
    std::shared_ptr<StrictMock<MockExceptionEncounteredSender>> m_mockExceptionSender;

    /// A strict mock that allows the test to strictly monitor the handling of directives.
    std::unique_ptr<StrictMock<MockDirectiveHandlerResult>> m_mockDirectiveHandlerResult;
};

void PowerControllerCapabilityAgentTest::SetUp() {
    m_mockExceptionSender = std::make_shared<StrictMock<MockExceptionEncounteredSender>>();
    m_mockDirectiveHandlerResult = make_unique<StrictMock<MockDirectiveHandlerResult>>();
    m_mockPowerController = std::make_shared<StrictMock<MockPowerControllerInterface>>();
    m_mockContextManager = std::make_shared<testing::StrictMock<MockContextManager>>();
    m_mockResponseSender = std::make_shared<MockAlexaInterfaceMessageSender>();

    auto timePoint = avsCommon::utils::timing::TimePoint();
    timePoint.setTime_ISO_8601(TIME_OF_SAMPLE_TEST);

    m_testPowerStateOn = PowerControllerInterface::PowerState{true, timePoint, std::chrono::milliseconds(0)};
}

void PowerControllerCapabilityAgentTest::TearDown() {
}

std::shared_ptr<PowerControllerCapabilityAgent> PowerControllerCapabilityAgentTest::createCapabilityAgentAndSetExpects(
    bool proactivelyReported,
    bool retrievable) {
    // ContextManager Mocks
    if (retrievable) {
        // Expects a non-null CA reference during create.
        EXPECT_CALL(*m_mockContextManager, addStateProvider(_, ::testing::NotNull()));

        // Expects a null when CA is shutdown.
        EXPECT_CALL(*m_mockContextManager, removeStateProvider(_));
    }

    if (proactivelyReported) {
        // addObserver Called during the initialization.
        EXPECT_CALL(*m_mockPowerController, addObserver(_))
            .WillOnce(WithArg<0>(Invoke([this](std::shared_ptr<PowerControllerObserverInterface> observer) {
                m_observer = observer;
                return true;
            })));

        // removeObserver is called during the shutdown
        EXPECT_CALL(*m_mockPowerController, removeObserver(_))
            .WillOnce(WithArg<0>(
                Invoke([this](std::shared_ptr<PowerControllerObserverInterface> observer) { m_observer = nullptr; })));
    }

    auto powerControllerCapabilityAgent = PowerControllerCapabilityAgent::create(
        TEST_ENDPOINT_ID,
        m_mockPowerController,
        m_mockContextManager,
        m_mockResponseSender,
        m_mockExceptionSender,
        proactivelyReported,
        retrievable);
    return powerControllerCapabilityAgent;
}

std::shared_ptr<AVSDirective> buildAVSDirective(std::string directiveName) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE, directiveName, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST, CORRELATION_TOKEN_TEST, INTERFACE_VERSION);
    auto avsMessageEndpoint = AVSMessageEndpoint(TEST_ENDPOINT_ID);

    return AVSDirective::create("", avsMessageHeader, "", attachmentManager, "", avsMessageEndpoint);
}

/// Test that create() returns a nullptr if called with invalid arguments.
TEST_F(PowerControllerCapabilityAgentTest, test_givenInvalidParameters_create_shouldFail) {
    ASSERT_THAT(
        PowerControllerCapabilityAgent::create(
            "", m_mockPowerController, m_mockContextManager, m_mockResponseSender, m_mockExceptionSender, true, true),
        IsNull());
    ASSERT_THAT(
        PowerControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID, nullptr, m_mockContextManager, m_mockResponseSender, m_mockExceptionSender, true, true),
        IsNull());
    ASSERT_THAT(
        PowerControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID, m_mockPowerController, nullptr, m_mockResponseSender, m_mockExceptionSender, true, true),
        IsNull());
    ASSERT_THAT(
        PowerControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID, m_mockPowerController, m_mockContextManager, nullptr, m_mockExceptionSender, true, true),
        IsNull());
    ASSERT_THAT(
        PowerControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID, m_mockPowerController, m_mockContextManager, m_mockResponseSender, nullptr, true, true),
        IsNull());
}

/**
 * Test successful handling of TurnOn directive.
 */
TEST_F(PowerControllerCapabilityAgentTest, test_turnOnDirective_successCase) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockPowerController, setPowerState(true, _))
        .WillOnce(WithArg<1>(
            Invoke([](AlexaStateChangeCauseType cause) { return std::make_pair(AlexaResponseType::SUCCESS, ""); })));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _));

    auto powerControllerCapabilityAgent = createCapabilityAgentAndSetExpects(false, true);
    ASSERT_THAT(powerControllerCapabilityAgent, NotNull());

    // Simulate directive
    powerControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(NAME_TURNON), std::move(m_mockDirectiveHandlerResult));
    powerControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    powerControllerCapabilityAgent->shutdown();
}

/**
 * Test error path of TurnOn directive.
 */
TEST_F(PowerControllerCapabilityAgentTest, test_turnOnDirective_errorCase) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockPowerController, setPowerState(true, _))
        .WillOnce(WithArg<1>(Invoke([](AlexaStateChangeCauseType cause) {
            return std::make_pair(AlexaResponseType::ENDPOINT_UNREACHABLE, "TestEndpointNotReachable");
        })));

    EXPECT_CALL(*m_mockResponseSender, sendErrorResponseEvent(_, _, _, _, _));

    auto powerControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(powerControllerCapabilityAgent, NotNull());

    // Simulate directive
    powerControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(NAME_TURNON), std::move(m_mockDirectiveHandlerResult));
    powerControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    powerControllerCapabilityAgent->shutdown();
}

/**
 * Test triggering of reportStateChange and subsequent call to the
 * ContextManager to build the context.
 */
TEST_F(PowerControllerCapabilityAgentTest, test_reportStateChange_successCase) {
    avsCommon::utils::WaitEvent waitEvent;
    auto powerControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(powerControllerCapabilityAgent, NotNull());

    // TurnOn case
    EXPECT_CALL(*m_mockPowerController, setPowerState(true, _))
        .WillOnce(WithArg<1>(Invoke([this](AlexaStateChangeCauseType cause) {
            m_observer->onPowerStateChanged(m_testPowerStateOn, cause);
            return std::make_pair(AlexaResponseType::SUCCESS, "");
        })));

    EXPECT_CALL(*m_mockContextManager, reportStateChange(_, _, _))
        .WillOnce(InvokeWithoutArgs([powerControllerCapabilityAgent] {
            const unsigned int stateRequestToken = 1;
            powerControllerCapabilityAgent->provideState(
                CapabilityTag(NAMESPACE, POWERSTATE_PROPERTY_NAME, TEST_ENDPOINT_ID), stateRequestToken);
        }));

    EXPECT_CALL(*m_mockPowerController, getPowerState())
        .WillOnce(InvokeWithoutArgs([this]() -> MockPowerControllerInterface::GetPowerStateResult {
            return std::make_pair(
                AlexaResponseType::SUCCESS,
                avsCommon::utils::Optional<PowerControllerInterface::PowerState>(m_testPowerStateOn));
        }));

    EXPECT_CALL(*m_mockContextManager, provideStateResponse(_, _, _)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    m_mockPowerController->setPowerState(true, AlexaStateChangeCauseType::APP_INTERACTION);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    powerControllerCapabilityAgent->shutdown();
}

/**
 * Test triggering of reportStateChange and subsequent failure to get the getPowerState and
 * call to the ContextManager to report the failure.
 */
TEST_F(PowerControllerCapabilityAgentTest, test_reportStateChange_errorCase) {
    avsCommon::utils::WaitEvent waitEvent;
    auto powerControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(powerControllerCapabilityAgent, NotNull());

    // TurnOn case
    EXPECT_CALL(*m_mockPowerController, setPowerState(true, _))
        .WillOnce(WithArg<1>(Invoke([this](AlexaStateChangeCauseType cause) {
            m_observer->onPowerStateChanged(m_testPowerStateOn, cause);
            return std::make_pair(AlexaResponseType::SUCCESS, "");
        })));

    EXPECT_CALL(*m_mockContextManager, reportStateChange(_, _, _))
        .WillOnce(InvokeWithoutArgs([powerControllerCapabilityAgent] {
            const unsigned int stateRequestToken = 1;
            powerControllerCapabilityAgent->provideState(
                CapabilityTag(NAMESPACE, POWERSTATE_PROPERTY_NAME, TEST_ENDPOINT_ID), stateRequestToken);
        }));

    EXPECT_CALL(*m_mockPowerController, getPowerState())
        .WillOnce(InvokeWithoutArgs([]() -> MockPowerControllerInterface::GetPowerStateResult {
            return std::make_pair(
                AlexaResponseType::ENDPOINT_UNREACHABLE,
                avsCommon::utils::Optional<PowerControllerInterface::PowerState>());
        }));

    EXPECT_CALL(*m_mockContextManager, provideStateUnavailableResponse(_, _, _))
        .WillOnce(InvokeWithoutArgs([&waitEvent]() { waitEvent.wakeUp(); }));

    m_mockPowerController->setPowerState(true, AlexaStateChangeCauseType::APP_INTERACTION);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    powerControllerCapabilityAgent->shutdown();
}

/**
 * Tests unknown Directive with both proactively reported and retrievable set.
 * Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(PowerControllerCapabilityAgentTest, test_unknownDirectiveWithProactivelyReportedAndRetrievableTrue) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    auto powerControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(powerControllerCapabilityAgent, NotNull());

    powerControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(UNKNOWN_DIRECTIVE), std::move(m_mockDirectiveHandlerResult));
    powerControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    powerControllerCapabilityAgent->shutdown();
}

/**
 * Tests unknown Directive with both proactively reported and retrievable as false.
 * Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(PowerControllerCapabilityAgentTest, test_unknownDirectiveWithProactivelyReportedAndRetrievableFalse) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    auto powerControllerCapabilityAgent = createCapabilityAgentAndSetExpects(false, false);
    ASSERT_THAT(powerControllerCapabilityAgent, NotNull());

    powerControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(UNKNOWN_DIRECTIVE), std::move(m_mockDirectiveHandlerResult));
    powerControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    powerControllerCapabilityAgent->shutdown();
}

}  // namespace test
}  // namespace powerController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
