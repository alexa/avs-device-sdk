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

#include "ModeController/ModeControllerAttributeBuilder.h"
#include "ModeController/ModeControllerCapabilityAgent.h"

#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockAlexaInterfaceMessageSender.h>
#include <AVSCommon/SDKInterfaces/ModeController/ModeControllerAttributes.h>
#include <AVSCommon/SDKInterfaces/ModeController/ModeControllerInterface.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/Timing/TimePoint.h>
#include <AVSCommon/Utils/WaitEvent.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace modeController {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment::test;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::modeController;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils::memory;
using namespace rapidjson;
using namespace ::testing;

// For readability.
using EndpointIdentifier = endpoints::EndpointIdentifier;

/// Timeout when waiting for futures to be set.
static std::chrono::milliseconds TIMEOUT(1000);

/// The namespace for capability agent.
static const std::string NAMESPACE{"Alexa.ModeController"};

/// The supported version
static const std::string INTERFACE_VERSION{"3"};

/// The name for SetMode directive
static const std::string NAME_SETMODE{"SetMode"};

/// The name for AdjustMode directive
static const std::string NAME_ADJUSTMODE{"AdjustMode"};

/// The name for modeValue property
static const std::string MODE_PROPERTY_NAME{"mode"};

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

/// Mode for testing
static std::string MODE_OPEN = "open";

/// Mode for testing
static std::string MODE_CLOSED = "close";

/// Mode Delta for testing.
static int MODE_DELTA = 1;

/// SetModeValue payload for testing
// clang-format off
static const std::string SET_MODE_PAYLOAD_TEST = R"({
    "mode":")" + MODE_OPEN + R"("
})";
// clang-format on

/// AdjustModeValue payload for testing
// clang-format off
static const std::string ADJUST_MODE_PAYLOAD_TEST = R"({
    "modeDelta":1
})";
// clang-format on

class MockModeControllerInterface : public ModeControllerInterface {
public:
    using ModeResult = std::pair<AlexaResponseType, std::string>;
    using GetModeResult = std::pair<AlexaResponseType, avsCommon::utils::Optional<ModeState>>;

    MOCK_METHOD0(getConfiguration, ModeControllerConfiguration());

    MOCK_METHOD2(setMode, ModeResult(const std::string& mode, const AlexaStateChangeCauseType cause));
    MOCK_METHOD2(adjustMode, ModeResult(const int modeDelta, const AlexaStateChangeCauseType cause));
    MOCK_METHOD0(getMode, GetModeResult());
    MOCK_METHOD1(addObserver, bool(std::shared_ptr<ModeControllerObserverInterface>));
    MOCK_METHOD1(removeObserver, void(const std::shared_ptr<ModeControllerObserverInterface>&));
};

class ModeControllerCapabilityAgentTest : public ::testing::Test {
public:
    /// SetUp before each test case.
    void SetUp() override;

    /// Constructor.
    ModeControllerCapabilityAgentTest() {
    }

    /// build modeControllerAttributes
    avsCommon::utils::Optional<avsCommon::sdkInterfaces::modeController::ModeControllerAttributes>
    buildModeControllerAttribute(const avsCommon::avs::CapabilityResources& capabilityResources) {
        auto modeControllerAttributeBuilder =
            capabilityAgents::modeController::ModeControllerAttributeBuilder::create();

        avsCommon::sdkInterfaces::modeController::ModeResources closeModeResource;
        if (!closeModeResource.addFriendlyNameWithText("Close", "en-US")) {
            return avsCommon::utils::Optional<avsCommon::sdkInterfaces::modeController::ModeControllerAttributes>();
        }
        avsCommon::sdkInterfaces::modeController::ModeResources openModeResource;
        if (!openModeResource.addFriendlyNameWithText("Open", "en-US")) {
            return avsCommon::utils::Optional<avsCommon::sdkInterfaces::modeController::ModeControllerAttributes>();
        }

        modeControllerAttributeBuilder->withCapabilityResources(capabilityResources)
            .setOrdered(true)
            .addMode(MODE_CLOSED, closeModeResource)
            .addMode(MODE_OPEN, openModeResource);
        return modeControllerAttributeBuilder->build();
    }

protected:
    /**
     * Function to create ModeControllerCapabilityAgent based on the reportable flags
     * and sets required ContextManager and ModeController mocks.
     */
    std::shared_ptr<ModeControllerCapabilityAgent> createCapabilityAgentAndSetExpects(
        const avsCommon::sdkInterfaces::modeController::ModeControllerAttributes& modeControllerAttributes,
        bool proactivelyReported,
        bool retrievable,
        bool nonControllable);

    /// The test @c ModeState as open
    ModeControllerInterface::ModeState m_testModeOpen;

    /// A mock @c ModeContollerInterface object.
    std::shared_ptr<StrictMock<MockModeControllerInterface>> m_mockModeController;

    /// A mock @c ModeContollerObserverInterface object.
    std::shared_ptr<ModeControllerObserverInterface> m_observer;

    // a context manager
    std::shared_ptr<StrictMock<MockContextManager>> m_mockContextManager;

    /// Response sender mock to track events being sent.
    std::shared_ptr<MockAlexaInterfaceMessageSender> m_mockResponseSender;

    // A strict mock that allows the test to strictly monitor the exceptions being sent.
    std::shared_ptr<StrictMock<MockExceptionEncounteredSender>> m_mockExceptionSender;

    /// A strict mock that allows the test to strictly monitor the handling of directives.
    std::unique_ptr<StrictMock<MockDirectiveHandlerResult>> m_mockDirectiveHandlerResult;
};

void ModeControllerCapabilityAgentTest::SetUp() {
    m_mockExceptionSender = std::make_shared<StrictMock<MockExceptionEncounteredSender>>();
    m_mockDirectiveHandlerResult = make_unique<StrictMock<MockDirectiveHandlerResult>>();
    m_mockModeController = std::make_shared<StrictMock<MockModeControllerInterface>>();
    m_mockContextManager = std::make_shared<testing::StrictMock<MockContextManager>>();
    m_mockResponseSender = std::make_shared<MockAlexaInterfaceMessageSender>();

    auto timePoint = avsCommon::utils::timing::TimePoint();
    timePoint.setTime_ISO_8601(TIME_OF_SAMPLE_TEST);

    m_testModeOpen = ModeControllerInterface::ModeState{MODE_OPEN, timePoint, std::chrono::milliseconds(0)};
}

std::shared_ptr<ModeControllerCapabilityAgent> ModeControllerCapabilityAgentTest::createCapabilityAgentAndSetExpects(
    const avsCommon::sdkInterfaces::modeController::ModeControllerAttributes& modeControllerAttributes,
    bool proactivelyReported,
    bool retrievable,
    bool nonControllable) {
    EXPECT_CALL(*m_mockModeController, getConfiguration())
        .WillOnce(InvokeWithoutArgs([]() -> ModeControllerInterface::ModeControllerConfiguration {
            return {MODE_OPEN, MODE_CLOSED};
        }));

    // ContextManager Mocks
    if (retrievable) {
        // Expects a non-null CA reference during create.
        EXPECT_CALL(*m_mockContextManager, addStateProvider(_, ::testing::NotNull()));

        // Expects a null when CA is shutdown.
        EXPECT_CALL(*m_mockContextManager, removeStateProvider(_));
    }

    if (proactivelyReported) {
        // addObserver Called during the initialization.
        EXPECT_CALL(*m_mockModeController, addObserver(_))
            .WillOnce(WithArg<0>(Invoke([this](std::shared_ptr<ModeControllerObserverInterface> observer) {
                m_observer = observer;
                return true;
            })));

        // removeObserver is called during the shutdown
        EXPECT_CALL(*m_mockModeController, removeObserver(_))
            .WillOnce(WithArg<0>(
                Invoke([this](std::shared_ptr<ModeControllerObserverInterface> observer) { m_observer = nullptr; })));
    }

    auto modeControllerCapabilityAgent = ModeControllerCapabilityAgent::create(
        TEST_ENDPOINT_ID,
        TEST_INSTANCE,
        modeControllerAttributes,
        m_mockModeController,
        m_mockContextManager,
        m_mockResponseSender,
        m_mockExceptionSender,
        proactivelyReported,
        retrievable,
        nonControllable);
    return modeControllerCapabilityAgent;
}

/**
 * Utility function to build the @c AVSDirective for test.
 *
 * @param directiveName The name of the test directive to build.
 * @param payload The unparsed payload of directive in JSON.
 *
 * @return The created AVSDirective object or @c nullptr if build failed.
 */
static std::shared_ptr<AVSDirective> buildAVSDirective(std::string directiveName, std::string payload) {
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

    return AVSDirective::create("", avsMessageHeader, payload, attachmentManager, "", avsMessageEndpoint);
}

/// Test that create() returns a nullptr if called with invalid arguments.
TEST_F(ModeControllerCapabilityAgentTest, test_givenInvalidParameters_create_shouldFail) {
    CapabilityResources emptyResource;
    auto emptyModeControllerAttribute = buildModeControllerAttribute(emptyResource);
    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("mode", "en-US"));
    auto modeControllerAttributes = buildModeControllerAttribute(resource);
    ASSERT_TRUE(modeControllerAttributes.hasValue());

    EXPECT_THAT(
        ModeControllerCapabilityAgent::create(
            "", "", emptyModeControllerAttribute.value(), nullptr, nullptr, nullptr, nullptr, true, true),
        IsNull());
    EXPECT_THAT(
        ModeControllerCapabilityAgent::create(
            "",
            TEST_INSTANCE,
            modeControllerAttributes.value(),
            m_mockModeController,
            m_mockContextManager,
            m_mockResponseSender,
            m_mockExceptionSender,
            true,
            true),
        IsNull());
    EXPECT_THAT(
        ModeControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID,
            "",
            modeControllerAttributes.value(),
            m_mockModeController,
            m_mockContextManager,
            m_mockResponseSender,
            m_mockExceptionSender,
            true,
            true),
        IsNull());
    EXPECT_THAT(
        ModeControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID,
            TEST_INSTANCE,
            modeControllerAttributes.value(),
            nullptr,
            m_mockContextManager,
            m_mockResponseSender,
            m_mockExceptionSender,
            true,
            true),
        IsNull());
    EXPECT_THAT(
        ModeControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID,
            TEST_INSTANCE,
            modeControllerAttributes.value(),
            m_mockModeController,
            nullptr,
            m_mockResponseSender,
            m_mockExceptionSender,
            true,
            true),
        IsNull());
    EXPECT_THAT(
        ModeControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID,
            TEST_INSTANCE,
            modeControllerAttributes.value(),
            m_mockModeController,
            m_mockContextManager,
            nullptr,
            m_mockExceptionSender,
            true,
            true),
        IsNull());
    EXPECT_THAT(
        ModeControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID,
            TEST_INSTANCE,
            modeControllerAttributes.value(),
            m_mockModeController,
            m_mockContextManager,
            m_mockResponseSender,
            nullptr,
            true,
            true),
        IsNull());
}

/**
 * Test successful handling of SetMode directive.
 */
TEST_F(ModeControllerCapabilityAgentTest, test_setModeDirective_successCase) {
    avsCommon::utils::WaitEvent waitEvent;
    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("mode", "en-US"));
    auto modeControllerAttributes = buildModeControllerAttribute(resource);
    ASSERT_TRUE(modeControllerAttributes.hasValue());

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockModeController, setMode(MODE_OPEN, _))
        .WillOnce(WithArg<1>(
            Invoke([](AlexaStateChangeCauseType cause) { return std::make_pair(AlexaResponseType::SUCCESS, ""); })));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _));

    auto modeControllerCapabilityAgent =
        createCapabilityAgentAndSetExpects(modeControllerAttributes.value(), false, true, false);
    ASSERT_THAT(modeControllerCapabilityAgent, NotNull());

    // Simulate directive
    modeControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(NAME_SETMODE, SET_MODE_PAYLOAD_TEST), std::move(m_mockDirectiveHandlerResult));
    modeControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    modeControllerCapabilityAgent->shutdown();
}

/**
 * Test error path of SetMode directive.
 */
TEST_F(ModeControllerCapabilityAgentTest, test_setModeDirective_errorCase) {
    avsCommon::utils::WaitEvent waitEvent;
    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("mode", "en-US"));
    auto modeControllerAttributes = buildModeControllerAttribute(resource);
    ASSERT_TRUE(modeControllerAttributes.hasValue());

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockModeController, setMode(MODE_OPEN, _))
        .WillOnce(WithArg<1>(Invoke([](AlexaStateChangeCauseType cause) {
            return std::make_pair(AlexaResponseType::ENDPOINT_UNREACHABLE, "TestEndpointNotReachable");
        })));

    EXPECT_CALL(*m_mockResponseSender, sendErrorResponseEvent(_, _, _, _, _));

    auto modeControllerCapabilityAgent =
        createCapabilityAgentAndSetExpects(modeControllerAttributes.value(), true, true, false);
    ASSERT_THAT(modeControllerCapabilityAgent, NotNull());

    // Simulate directive
    modeControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(NAME_SETMODE, SET_MODE_PAYLOAD_TEST), std::move(m_mockDirectiveHandlerResult));
    modeControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    modeControllerCapabilityAgent->shutdown();
}

/**
 * Test successful handling of AdjustMode directive.
 */
TEST_F(ModeControllerCapabilityAgentTest, test_adjustModeValueDirective_successCase) {
    avsCommon::utils::WaitEvent waitEvent;
    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("mode", "en-US"));
    auto modeControllerAttributes = buildModeControllerAttribute(resource);
    ASSERT_TRUE(modeControllerAttributes.hasValue());

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockModeController, adjustMode(MODE_DELTA, _))
        .WillOnce(WithArg<1>(
            Invoke([](AlexaStateChangeCauseType cause) { return std::make_pair(AlexaResponseType::SUCCESS, ""); })));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _));

    auto modeControllerCapabilityAgent =
        createCapabilityAgentAndSetExpects(modeControllerAttributes.value(), false, true, false);
    ASSERT_THAT(modeControllerCapabilityAgent, NotNull());

    // Simulate directive
    modeControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(NAME_ADJUSTMODE, ADJUST_MODE_PAYLOAD_TEST), std::move(m_mockDirectiveHandlerResult));
    modeControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    modeControllerCapabilityAgent->shutdown();
}

/**
 * Test error path of AdjustMode directive.
 */
TEST_F(ModeControllerCapabilityAgentTest, test_adjustModeValueDirective_errorCase) {
    avsCommon::utils::WaitEvent waitEvent;
    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("mode", "en-US"));
    auto modeControllerAttributes = buildModeControllerAttribute(resource);
    ASSERT_TRUE(modeControllerAttributes.hasValue());

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockModeController, adjustMode(MODE_DELTA, _))
        .WillOnce(WithArg<1>(Invoke([](AlexaStateChangeCauseType cause) {
            return std::make_pair(AlexaResponseType::ENDPOINT_UNREACHABLE, "TestEndpointNotReachable");
        })));

    EXPECT_CALL(*m_mockResponseSender, sendErrorResponseEvent(_, _, _, _, _));

    auto modeControllerCapabilityAgent =
        createCapabilityAgentAndSetExpects(modeControllerAttributes.value(), true, true, false);
    ASSERT_THAT(modeControllerCapabilityAgent, NotNull());

    // Simulate directive
    modeControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(NAME_ADJUSTMODE, ADJUST_MODE_PAYLOAD_TEST), std::move(m_mockDirectiveHandlerResult));
    modeControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    modeControllerCapabilityAgent->shutdown();
}

/**
 * Test triggering of reportStateChange and subsequent call to the
 * ContextManager to build the context.
 */
TEST_F(ModeControllerCapabilityAgentTest, test_reportStateChange_successCase) {
    avsCommon::utils::WaitEvent waitEvent;
    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("mode", "en-US"));
    auto modeControllerAttributes = buildModeControllerAttribute(resource);
    ASSERT_TRUE(modeControllerAttributes.hasValue());

    auto modeControllerCapabilityAgent =
        createCapabilityAgentAndSetExpects(modeControllerAttributes.value(), true, true, false);
    ASSERT_THAT(modeControllerCapabilityAgent, NotNull());

    EXPECT_CALL(*m_mockModeController, setMode(MODE_OPEN, _))
        .WillOnce(WithArg<1>(Invoke([this](AlexaStateChangeCauseType cause) {
            m_observer->onModeChanged(m_testModeOpen, cause);
            return std::make_pair(AlexaResponseType::SUCCESS, "");
        })));

    EXPECT_CALL(*m_mockContextManager, reportStateChange(_, _, _))
        .WillOnce(InvokeWithoutArgs([modeControllerCapabilityAgent] {
            const unsigned int stateRequestToken = 1;
            modeControllerCapabilityAgent->provideState(
                CapabilityTag(NAMESPACE, MODE_PROPERTY_NAME, TEST_ENDPOINT_ID, TEST_INSTANCE), stateRequestToken);
        }));

    EXPECT_CALL(*m_mockModeController, getMode())
        .WillOnce(InvokeWithoutArgs([this]() -> MockModeControllerInterface::GetModeResult {
            return std::make_pair(
                AlexaResponseType::SUCCESS,
                avsCommon::utils::Optional<ModeControllerInterface::ModeState>(m_testModeOpen));
        }));

    EXPECT_CALL(*m_mockContextManager, provideStateResponse(_, _, _)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    m_mockModeController->setMode(MODE_OPEN, AlexaStateChangeCauseType::APP_INTERACTION);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    modeControllerCapabilityAgent->shutdown();
}

/**
 * Test triggering of reportStateChange and subsequent failure to get the getMode and
 * call to the ContextManager to report the failure.
 */
TEST_F(ModeControllerCapabilityAgentTest, test_reportStateChange_errorCase) {
    avsCommon::utils::WaitEvent waitEvent;
    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("mode", "en-US"));
    auto modeControllerAttributes = buildModeControllerAttribute(resource);
    ASSERT_TRUE(modeControllerAttributes.hasValue());

    auto modeControllerCapabilityAgent =
        createCapabilityAgentAndSetExpects(modeControllerAttributes.value(), true, true, false);
    ASSERT_THAT(modeControllerCapabilityAgent, NotNull());

    EXPECT_CALL(*m_mockModeController, setMode(MODE_OPEN, _))
        .WillOnce(WithArg<1>(Invoke([this](AlexaStateChangeCauseType cause) {
            m_observer->onModeChanged(m_testModeOpen, cause);
            return std::make_pair(AlexaResponseType::SUCCESS, "");
        })));

    EXPECT_CALL(*m_mockContextManager, reportStateChange(_, _, _))
        .WillOnce(InvokeWithoutArgs([modeControllerCapabilityAgent] {
            const unsigned int stateRequestToken = 1;
            modeControllerCapabilityAgent->provideState(
                CapabilityTag(NAMESPACE, MODE_PROPERTY_NAME, TEST_ENDPOINT_ID, TEST_INSTANCE), stateRequestToken);
        }));

    EXPECT_CALL(*m_mockModeController, getMode())
        .WillOnce(InvokeWithoutArgs([]() -> MockModeControllerInterface::GetModeResult {
            return std::make_pair(
                AlexaResponseType::ENDPOINT_UNREACHABLE,
                avsCommon::utils::Optional<ModeControllerInterface::ModeState>());
        }));

    EXPECT_CALL(*m_mockContextManager, provideStateUnavailableResponse(_, _, _))
        .WillOnce(InvokeWithoutArgs([&waitEvent]() { waitEvent.wakeUp(); }));

    m_mockModeController->setMode(MODE_OPEN, AlexaStateChangeCauseType::APP_INTERACTION);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    modeControllerCapabilityAgent->shutdown();
}

/**
 * Tests unknown Directive with both proactively reported and retrievable set.
 * Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(ModeControllerCapabilityAgentTest, test_unknownDirectiveWithProactivelyReportedAndRetrievableTrue) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("mode", "en-US"));
    auto modeControllerAttributes = buildModeControllerAttribute(resource);
    ASSERT_TRUE(modeControllerAttributes.hasValue());

    auto modeControllerCapabilityAgent =
        createCapabilityAgentAndSetExpects(modeControllerAttributes.value(), true, true, false);
    ASSERT_THAT(modeControllerCapabilityAgent, NotNull());

    modeControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(UNKNOWN_DIRECTIVE, SET_MODE_PAYLOAD_TEST), std::move(m_mockDirectiveHandlerResult));
    modeControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    modeControllerCapabilityAgent->shutdown();
}

/**
 * Tests unknown Directive with both proactively reported and retrievable as false.
 * Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(ModeControllerCapabilityAgentTest, test_unknownDirectiveWithProactivelyReportedAndRetrievableFalse) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("mode", "en-US"));
    auto modeControllerAttributes = buildModeControllerAttribute(resource);
    ASSERT_TRUE(modeControllerAttributes.hasValue());

    auto modeControllerCapabilityAgent =
        createCapabilityAgentAndSetExpects(modeControllerAttributes.value(), false, false, false);
    modeControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(UNKNOWN_DIRECTIVE, SET_MODE_PAYLOAD_TEST), std::move(m_mockDirectiveHandlerResult));
    modeControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    modeControllerCapabilityAgent->shutdown();
}

}  // namespace test
}  // namespace modeController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
