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

#include "RangeController/RangeControllerAttributeBuilder.h"
#include "RangeController/RangeControllerCapabilityAgent.h"

#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockAlexaInterfaceMessageSender.h>
#include <AVSCommon/SDKInterfaces/RangeController/RangeControllerAttributes.h>
#include <AVSCommon/SDKInterfaces/RangeController/RangeControllerInterface.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/Timing/TimePoint.h>
#include <AVSCommon/Utils/WaitEvent.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace rangeController {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment::test;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::rangeController;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils::memory;
using namespace rapidjson;
using namespace ::testing;

// For readability.
using EndpointIdentifier = endpoints::EndpointIdentifier;

/// Timeout when waiting for futures to be set.
static std::chrono::milliseconds TIMEOUT(1000);

/// The namespace for capability agent.
static const std::string NAMESPACE{"Alexa.RangeController"};

/// The supported version
static const std::string INTERFACE_VERSION{"3"};

/// The name for SetRangeValue directive
static const std::string NAME_SETRANGEVALUE{"SetRangeValue"};

/// The name for AdjustRangeValue directive
static const std::string NAME_ADJUSTRANGEVALUE{"AdjustRangeValue"};

/// The name for rangeValue property
static const std::string RANGEVALUE_PROPERTY_NAME{"rangeValue"};

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

/// Minimum range value for test.
static double RANGE_VALUE_MINUMUM = 0;

/// Maximum range value for test.
static double RANGE_VALUE_MAXIMUM = 100;

/// Medium preset for test.
static double RANGE_VALUE_MEDIUM = 50;

/// Range precision for test.
static double RANGE_PRECISION = 1;

/// SetRangeValue payload for testing
// clang-format off
static const std::string SET_RANGE_VALUE_PAYLOAD_TEST = R"({
    "rangeValue":)" + std::to_string(RANGE_VALUE_MEDIUM) + R"(
})";
// clang-format on

/// AdjustRangeValue payload for testing
// clang-format off
static const std::string ADJUST_RANGE_VALUE_PAYLOAD_TEST = R"({
    "rangeValueDelta":)" + std::to_string(RANGE_PRECISION) + R"(,
	"rangeValueDeltaDefault":false
})";
// clang-format on

class MockRangeControllerInterface : public RangeControllerInterface {
public:
    using RangeValueResult = std::pair<AlexaResponseType, std::string>;
    using GetRangeValueResult = std::pair<AlexaResponseType, avsCommon::utils::Optional<RangeState>>;

    MOCK_METHOD0(getConfiguration, RangeControllerConfiguration());

    MOCK_METHOD2(setRangeValue, RangeValueResult(const double value, const AlexaStateChangeCauseType cause));
    MOCK_METHOD2(
        adjustRangeValue,
        RangeValueResult(const double rangeValueDelta, const AlexaStateChangeCauseType cause));
    MOCK_METHOD0(getRangeState, GetRangeValueResult());
    MOCK_METHOD1(addObserver, bool(std::shared_ptr<RangeControllerObserverInterface>));
    MOCK_METHOD1(removeObserver, void(const std::shared_ptr<RangeControllerObserverInterface>&));
};

class RangeControllerCapabilityAgentTest : public ::testing::Test {
public:
    /// SetUp before each test case.
    void SetUp() override;

    /// Constructor.
    RangeControllerCapabilityAgentTest() {
    }

    /// build rangeControllerAttributes
    avsCommon::utils::Optional<avsCommon::sdkInterfaces::rangeController::RangeControllerAttributes>
    buildRangeControllerAttribute(const avsCommon::avs::CapabilityResources& capabilityResources) {
        auto rangeControllerAttributeBuilder =
            capabilityAgents::rangeController::RangeControllerAttributeBuilder::create();

        avsCommon::sdkInterfaces::rangeController::PresetResources minimumPresetResource;
        if (!minimumPresetResource.addFriendlyNameWithAssetId(avsCommon::avs::resources::ASSET_ALEXA_VALUE_MINIMUM) ||
            !minimumPresetResource.addFriendlyNameWithText("Low", "en-US")) {
            return avsCommon::utils::Optional<avsCommon::sdkInterfaces::rangeController::RangeControllerAttributes>();
        }
        avsCommon::sdkInterfaces::rangeController::PresetResources mediumPresetResource;
        if (!mediumPresetResource.addFriendlyNameWithAssetId(avsCommon::avs::resources::ASSET_ALEXA_VALUE_MEDIUM)) {
            return avsCommon::utils::Optional<avsCommon::sdkInterfaces::rangeController::RangeControllerAttributes>();
        }
        avsCommon::sdkInterfaces::rangeController::PresetResources maximumPresetResource;
        if (!maximumPresetResource.addFriendlyNameWithAssetId(avsCommon::avs::resources::ASSET_ALEXA_VALUE_MAXIMUM) ||
            !maximumPresetResource.addFriendlyNameWithText("High", "en-US")) {
            return avsCommon::utils::Optional<avsCommon::sdkInterfaces::rangeController::RangeControllerAttributes>();
        }

        rangeControllerAttributeBuilder->withCapabilityResources(capabilityResources)
            .withUnitOfMeasure(avsCommon::avs::resources::ALEXA_UNIT_ANGLE_DEGREES)
            .addPreset(std::make_pair(RANGE_VALUE_MINUMUM, minimumPresetResource))
            .addPreset(std::make_pair(RANGE_VALUE_MEDIUM, mediumPresetResource))
            .addPreset(std::make_pair(RANGE_VALUE_MAXIMUM, maximumPresetResource));
        return rangeControllerAttributeBuilder->build();
    }

protected:
    /**
     * Function to create RangeControllerCapabilityAgent based on the reportable flags
     * and sets required ContextManager and RangeController mocks.
     */
    std::shared_ptr<RangeControllerCapabilityAgent> createCapabilityAgentAndSetExpects(
        const avsCommon::sdkInterfaces::rangeController::RangeControllerAttributes& rangeControllerAttributes,
        bool proactivelyReported,
        bool retrievable,
        bool nonControllable);

    /// The test @c RangeState as MEDIUM
    RangeControllerInterface::RangeState m_testRangeValueMedium;

    /// A mock @c RangeContollerInterface object.
    std::shared_ptr<StrictMock<MockRangeControllerInterface>> m_mockRangeController;

    /// A mock @c RangeContollerObserverInterface object.
    std::shared_ptr<RangeControllerObserverInterface> m_observer;

    // a context manager
    std::shared_ptr<StrictMock<MockContextManager>> m_mockContextManager;

    /// Response sender mock to track events being sent.
    std::shared_ptr<MockAlexaInterfaceMessageSender> m_mockResponseSender;

    // A strict mock that allows the test to strictly monitor the exceptions being sent.
    std::shared_ptr<StrictMock<MockExceptionEncounteredSender>> m_mockExceptionSender;

    /// A strict mock that allows the test to strictly monitor the handling of directives.
    std::unique_ptr<StrictMock<MockDirectiveHandlerResult>> m_mockDirectiveHandlerResult;
};

void RangeControllerCapabilityAgentTest::SetUp() {
    m_mockExceptionSender = std::make_shared<StrictMock<MockExceptionEncounteredSender>>();
    m_mockDirectiveHandlerResult = make_unique<StrictMock<MockDirectiveHandlerResult>>();
    m_mockRangeController = std::make_shared<StrictMock<MockRangeControllerInterface>>();
    m_mockContextManager = std::make_shared<testing::StrictMock<MockContextManager>>();
    m_mockResponseSender = std::make_shared<MockAlexaInterfaceMessageSender>();

    auto timePoint = avsCommon::utils::timing::TimePoint();
    timePoint.setTime_ISO_8601(TIME_OF_SAMPLE_TEST);

    m_testRangeValueMedium =
        RangeControllerInterface::RangeState{RANGE_VALUE_MEDIUM, timePoint, std::chrono::milliseconds(0)};
}

std::shared_ptr<RangeControllerCapabilityAgent> RangeControllerCapabilityAgentTest::createCapabilityAgentAndSetExpects(
    const avsCommon::sdkInterfaces::rangeController::RangeControllerAttributes& rangeControllerAttributes,
    bool proactivelyReported,
    bool retrievable,
    bool nonControllable) {
    EXPECT_CALL(*m_mockRangeController, getConfiguration())
        .WillOnce(InvokeWithoutArgs([]() -> RangeControllerInterface::RangeControllerConfiguration {
            return {RANGE_VALUE_MINUMUM, RANGE_VALUE_MAXIMUM, RANGE_PRECISION};
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
        EXPECT_CALL(*m_mockRangeController, addObserver(_))
            .WillOnce(WithArg<0>(Invoke([this](std::shared_ptr<RangeControllerObserverInterface> observer) {
                m_observer = observer;
                return true;
            })));

        // removeObserver is called during the shutdown
        EXPECT_CALL(*m_mockRangeController, removeObserver(_))
            .WillOnce(WithArg<0>(
                Invoke([this](std::shared_ptr<RangeControllerObserverInterface> observer) { m_observer = nullptr; })));
    }

    auto rangeControllerCapabilityAgent = RangeControllerCapabilityAgent::create(
        TEST_ENDPOINT_ID,
        TEST_INSTANCE,
        rangeControllerAttributes,
        m_mockRangeController,
        m_mockContextManager,
        m_mockResponseSender,
        m_mockExceptionSender,
        proactivelyReported,
        retrievable,
        nonControllable);
    return rangeControllerCapabilityAgent;
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
TEST_F(RangeControllerCapabilityAgentTest, test_givenInvalidParameters_create_shouldFail) {
    CapabilityResources emptyResource;
    auto emptyRangeControllerAttribute = buildRangeControllerAttribute(emptyResource);
    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("range", "en-US"));
    auto rangeControllerAttributes = buildRangeControllerAttribute(resource);
    ASSERT_TRUE(rangeControllerAttributes.hasValue());

    EXPECT_THAT(
        RangeControllerCapabilityAgent::create(
            "", "", emptyRangeControllerAttribute.value(), nullptr, nullptr, nullptr, nullptr, true, true),
        IsNull());
    EXPECT_THAT(
        RangeControllerCapabilityAgent::create(
            "",
            TEST_INSTANCE,
            rangeControllerAttributes.value(),
            m_mockRangeController,
            m_mockContextManager,
            m_mockResponseSender,
            m_mockExceptionSender,
            true,
            true),
        IsNull());
    EXPECT_THAT(
        RangeControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID,
            "",
            rangeControllerAttributes.value(),
            m_mockRangeController,
            m_mockContextManager,
            m_mockResponseSender,
            m_mockExceptionSender,
            true,
            true),
        IsNull());
    EXPECT_THAT(
        RangeControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID,
            TEST_INSTANCE,
            rangeControllerAttributes.value(),
            nullptr,
            m_mockContextManager,
            m_mockResponseSender,
            m_mockExceptionSender,
            true,
            true),
        IsNull());
    EXPECT_THAT(
        RangeControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID,
            TEST_INSTANCE,
            rangeControllerAttributes.value(),
            m_mockRangeController,
            nullptr,
            m_mockResponseSender,
            m_mockExceptionSender,
            true,
            true),
        IsNull());
    EXPECT_THAT(
        RangeControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID,
            TEST_INSTANCE,
            rangeControllerAttributes.value(),
            m_mockRangeController,
            m_mockContextManager,
            nullptr,
            m_mockExceptionSender,
            true,
            true),
        IsNull());
    EXPECT_THAT(
        RangeControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID,
            TEST_INSTANCE,
            rangeControllerAttributes.value(),
            m_mockRangeController,
            m_mockContextManager,
            m_mockResponseSender,
            nullptr,
            true,
            true),
        IsNull());
}

/**
 * Test successful handling of SetRangeValue directive.
 */
TEST_F(RangeControllerCapabilityAgentTest, test_setRangeValueDirective_successCase) {
    avsCommon::utils::WaitEvent waitEvent;
    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("range", "en-US"));
    auto rangeControllerAttributes = buildRangeControllerAttribute(resource);
    ASSERT_TRUE(rangeControllerAttributes.hasValue());

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockRangeController, setRangeValue(RANGE_VALUE_MEDIUM, _))
        .WillOnce(WithArg<1>(
            Invoke([](AlexaStateChangeCauseType cause) { return std::make_pair(AlexaResponseType::SUCCESS, ""); })));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _));

    auto rangeControllerCapabilityAgent =
        createCapabilityAgentAndSetExpects(rangeControllerAttributes.value(), false, true, false);
    ASSERT_THAT(rangeControllerCapabilityAgent, NotNull());

    // Simulate directive
    rangeControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(NAME_SETRANGEVALUE, SET_RANGE_VALUE_PAYLOAD_TEST), std::move(m_mockDirectiveHandlerResult));
    rangeControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    rangeControllerCapabilityAgent->shutdown();
}

/**
 * Test error path of SetRangeValue directive.
 */
TEST_F(RangeControllerCapabilityAgentTest, test_setRangeValueDirective_errorCase) {
    avsCommon::utils::WaitEvent waitEvent;
    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("range", "en-US"));
    auto rangeControllerAttributes = buildRangeControllerAttribute(resource);
    ASSERT_TRUE(rangeControllerAttributes.hasValue());

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockRangeController, setRangeValue(RANGE_VALUE_MEDIUM, _))
        .WillOnce(WithArg<1>(Invoke([](AlexaStateChangeCauseType cause) {
            return std::make_pair(AlexaResponseType::ENDPOINT_UNREACHABLE, "TestEndpointNotReachable");
        })));

    EXPECT_CALL(*m_mockResponseSender, sendErrorResponseEvent(_, _, _, _, _));

    auto rangeControllerCapabilityAgent =
        createCapabilityAgentAndSetExpects(rangeControllerAttributes.value(), true, true, false);
    ASSERT_THAT(rangeControllerCapabilityAgent, NotNull());

    // Simulate directive
    rangeControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(NAME_SETRANGEVALUE, SET_RANGE_VALUE_PAYLOAD_TEST), std::move(m_mockDirectiveHandlerResult));
    rangeControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    rangeControllerCapabilityAgent->shutdown();
}

/**
 * Test successful handling of AdjustRangeValue directive.
 */
TEST_F(RangeControllerCapabilityAgentTest, test_adjustRangeValueDirective_successCase) {
    avsCommon::utils::WaitEvent waitEvent;
    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("range", "en-US"));
    auto rangeControllerAttributes = buildRangeControllerAttribute(resource);
    ASSERT_TRUE(rangeControllerAttributes.hasValue());

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockRangeController, adjustRangeValue(RANGE_PRECISION, _))
        .WillOnce(WithArg<1>(
            Invoke([](AlexaStateChangeCauseType cause) { return std::make_pair(AlexaResponseType::SUCCESS, ""); })));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _));

    auto rangeControllerCapabilityAgent =
        createCapabilityAgentAndSetExpects(rangeControllerAttributes.value(), false, true, false);
    ASSERT_THAT(rangeControllerCapabilityAgent, NotNull());

    // Simulate directive
    rangeControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(NAME_ADJUSTRANGEVALUE, ADJUST_RANGE_VALUE_PAYLOAD_TEST),
        std::move(m_mockDirectiveHandlerResult));
    rangeControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    rangeControllerCapabilityAgent->shutdown();
}

/**
 * Test error path of AdjustRangeValue directive.
 */
TEST_F(RangeControllerCapabilityAgentTest, test_adjustRangeValueDirective_errorCase) {
    avsCommon::utils::WaitEvent waitEvent;
    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("range", "en-US"));
    auto rangeControllerAttributes = buildRangeControllerAttribute(resource);
    ASSERT_TRUE(rangeControllerAttributes.hasValue());

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockRangeController, adjustRangeValue(RANGE_PRECISION, _))
        .WillOnce(WithArg<1>(Invoke([](AlexaStateChangeCauseType cause) {
            return std::make_pair(AlexaResponseType::ENDPOINT_UNREACHABLE, "TestEndpointNotReachable");
        })));

    EXPECT_CALL(*m_mockResponseSender, sendErrorResponseEvent(_, _, _, _, _));

    auto rangeControllerCapabilityAgent =
        createCapabilityAgentAndSetExpects(rangeControllerAttributes.value(), true, true, false);
    ASSERT_THAT(rangeControllerCapabilityAgent, NotNull());

    // Simulate directive
    rangeControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(NAME_ADJUSTRANGEVALUE, ADJUST_RANGE_VALUE_PAYLOAD_TEST),
        std::move(m_mockDirectiveHandlerResult));
    rangeControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    rangeControllerCapabilityAgent->shutdown();
}

/**
 * Test triggering of reportStateChange and subsequent call to the
 * ContextManager to build the context.
 */
TEST_F(RangeControllerCapabilityAgentTest, test_reportStateChange_successCase) {
    avsCommon::utils::WaitEvent waitEvent;
    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("range", "en-US"));
    auto rangeControllerAttributes = buildRangeControllerAttribute(resource);
    ASSERT_TRUE(rangeControllerAttributes.hasValue());

    auto rangeControllerCapabilityAgent =
        createCapabilityAgentAndSetExpects(rangeControllerAttributes.value(), true, true, false);
    ASSERT_THAT(rangeControllerCapabilityAgent, NotNull());

    EXPECT_CALL(*m_mockRangeController, setRangeValue(RANGE_VALUE_MEDIUM, _))
        .WillOnce(WithArg<1>(Invoke([this](AlexaStateChangeCauseType cause) {
            m_observer->onRangeChanged(m_testRangeValueMedium, cause);
            return std::make_pair(AlexaResponseType::SUCCESS, "");
        })));

    EXPECT_CALL(*m_mockContextManager, reportStateChange(_, _, _))
        .WillOnce(InvokeWithoutArgs([rangeControllerCapabilityAgent] {
            const unsigned int stateRequestToken = 1;
            rangeControllerCapabilityAgent->provideState(
                CapabilityTag(NAMESPACE, RANGEVALUE_PROPERTY_NAME, TEST_ENDPOINT_ID, TEST_INSTANCE), stateRequestToken);
        }));

    EXPECT_CALL(*m_mockRangeController, getRangeState())
        .WillOnce(InvokeWithoutArgs([this]() -> MockRangeControllerInterface::GetRangeValueResult {
            return std::make_pair(
                AlexaResponseType::SUCCESS,
                avsCommon::utils::Optional<RangeControllerInterface::RangeState>(m_testRangeValueMedium));
        }));

    EXPECT_CALL(*m_mockContextManager, provideStateResponse(_, _, _)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    m_mockRangeController->setRangeValue(RANGE_VALUE_MEDIUM, AlexaStateChangeCauseType::APP_INTERACTION);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    rangeControllerCapabilityAgent->shutdown();
}

/**
 * Test triggering of reportStateChange and subsequent failure to get the getRangeState and
 * call to the ContextManager to report the failure.
 */
TEST_F(RangeControllerCapabilityAgentTest, test_reportStateChange_errorCase) {
    avsCommon::utils::WaitEvent waitEvent;
    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("range", "en-US"));
    auto rangeControllerAttributes = buildRangeControllerAttribute(resource);
    ASSERT_TRUE(rangeControllerAttributes.hasValue());

    auto rangeControllerCapabilityAgent =
        createCapabilityAgentAndSetExpects(rangeControllerAttributes.value(), true, true, false);
    ASSERT_THAT(rangeControllerCapabilityAgent, NotNull());

    EXPECT_CALL(*m_mockRangeController, setRangeValue(RANGE_VALUE_MEDIUM, _))
        .WillOnce(WithArg<1>(Invoke([this](AlexaStateChangeCauseType cause) {
            m_observer->onRangeChanged(m_testRangeValueMedium, cause);
            return std::make_pair(AlexaResponseType::SUCCESS, "");
        })));

    EXPECT_CALL(*m_mockContextManager, reportStateChange(_, _, _))
        .WillOnce(InvokeWithoutArgs([rangeControllerCapabilityAgent] {
            const unsigned int stateRequestToken = 1;
            rangeControllerCapabilityAgent->provideState(
                CapabilityTag(NAMESPACE, RANGEVALUE_PROPERTY_NAME, TEST_ENDPOINT_ID, TEST_INSTANCE), stateRequestToken);
        }));

    EXPECT_CALL(*m_mockRangeController, getRangeState())
        .WillOnce(InvokeWithoutArgs([]() -> MockRangeControllerInterface::GetRangeValueResult {
            return std::make_pair(
                AlexaResponseType::ENDPOINT_UNREACHABLE,
                avsCommon::utils::Optional<RangeControllerInterface::RangeState>());
        }));

    EXPECT_CALL(*m_mockContextManager, provideStateUnavailableResponse(_, _, _))
        .WillOnce(InvokeWithoutArgs([&waitEvent]() { waitEvent.wakeUp(); }));

    m_mockRangeController->setRangeValue(RANGE_VALUE_MEDIUM, AlexaStateChangeCauseType::APP_INTERACTION);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    rangeControllerCapabilityAgent->shutdown();
}

/**
 * Tests unknown Directive with both proactively reported and retrievable set.
 * Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(RangeControllerCapabilityAgentTest, test_unknownDirectiveWithProactivelyReportedAndRetrievableTrue) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("range", "en-US"));
    auto rangeControllerAttributes = buildRangeControllerAttribute(resource);
    ASSERT_TRUE(rangeControllerAttributes.hasValue());

    auto rangeControllerCapabilityAgent =
        createCapabilityAgentAndSetExpects(rangeControllerAttributes.value(), true, true, false);
    ASSERT_THAT(rangeControllerCapabilityAgent, NotNull());

    rangeControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(UNKNOWN_DIRECTIVE, SET_RANGE_VALUE_PAYLOAD_TEST), std::move(m_mockDirectiveHandlerResult));
    rangeControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    rangeControllerCapabilityAgent->shutdown();
}

/**
 * Tests unknown Directive with both proactively reported and retrievable as false.
 * Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(RangeControllerCapabilityAgentTest, test_unknownDirectiveWithProactivelyReportedAndRetrievableFalse) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    CapabilityResources resource;
    ASSERT_TRUE(resource.addFriendlyNameWithText("range", "en-US"));
    auto rangeControllerAttributes = buildRangeControllerAttribute(resource);
    ASSERT_TRUE(rangeControllerAttributes.hasValue());

    auto rangeControllerCapabilityAgent =
        createCapabilityAgentAndSetExpects(rangeControllerAttributes.value(), false, false, false);
    rangeControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(UNKNOWN_DIRECTIVE, SET_RANGE_VALUE_PAYLOAD_TEST), std::move(m_mockDirectiveHandlerResult));
    rangeControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    rangeControllerCapabilityAgent->shutdown();
}

}  // namespace test
}  // namespace rangeController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
