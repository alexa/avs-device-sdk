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

#include <chrono>
#include <memory>
#include <string>

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

#include "acsdk/AlexaInputController/private/InputControllerCapabilityAgent.h"

namespace alexaClientSDK {
namespace alexaInputController {
namespace test {

using namespace ::testing;
using namespace alexaInputControllerInterfaces;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils;
using namespace avsCommon::utils::memory;

/// Timeout when waiting for futures to be set.
static std::chrono::milliseconds TIMEOUT(1000);

/// Time of sample used for testing
static const std::string TIME_OF_SAMPLE_TEST("2017-02-03T16:20:50.523Z");

/// The test EndpointId
static const endpoints::EndpointIdentifier TEST_ENDPOINT_ID("testEndpointId");

/// The namespace for capability agent.
static const std::string NAMESPACE{"Alexa.InputController"};

/// The supported version
static const std::string INTERFACE_VERSION{"3"};

/// The name for SelectInput directive
static const std::string NAME_SETMODE{"SelectInput"};

/// An unknown directive signature.
static const std::string UNKNOWN_DIRECTIVE{"Unknown"};

/// The name for modeValue property
static const std::string INPUT_PROPERTY_NAME{"input"};

/// Event key.
static const std::string EVENT("event");

/// Header key.
static const std::string HEADER("header");

/// MessageId key.
static const std::string MESSAGE_ID("messageId");

/// MessageId for testing.
static const std::string MESSAGE_ID_TEST("MessageId_Test");

/// CD Input string for testing.
static const Input CD_INPUT_TEST = Input::CD;

/// HDMI 1 Input string for testing.
static const Input HDMI_1_INPUT_TEST = Input::HDMI_1;

/// Dialog request Id key.
static const std::string DIALOG_REQUEST_ID("dialogRequestId");

/// DialogRequestId for testing.
static const std::string DIALOG_REQUEST_ID_TEST("DialogRequestId_Test");

/// Correlation token for testing.
static const std::string CORRELATION_TOKEN_TEST("CorrelationToken_Test");

/// Event correlation for testing.
static const std::string EVENT_CORRELATION_TOKEN_TEST("EventCorrelationToken_Test");

static const InputControllerInterface::SupportedInputs DEFAULT_ALLOWED_INPUT{{Input::HDMI_1, {"HDMI1"}},
                                                                             {Input::CD, {"CD"}}};

/// A simple SelectInput directive JSON string.
static const std::string SELECT_INPUT_DIRECTIVE_JSON_STRING = R"delim(
    {
        "input": "HDMI 1"
    }
)delim";

/// A simple SelectInput directive JSON string.
static const std::string SELECT_INPUT_UNSUPPORTED_DIRECTIVE_JSON_STRING = R"delim(
    {
        "input": "HDMI 99"
    }
)delim";

/// A SelectInput directive without input field
static const std::string SELECT_INPUT_MISSING_TAG_DIRECTIVE_JSON_STRING = R"delim(
    {
    }
)delim";

/// A SelectInput directive with an empty input
static const std::string SELECT_INPUT_EMPTY_INPUT_DIRECTIVE_JSON_STRING = R"delim(
    {
        "input": ""
    }
)delim";

class MockHandler : public InputControllerInterface {
public:
    MOCK_METHOD0(getSupportedInputsMock, SupportedInputs());
    MOCK_METHOD1(setInputMock, InputControllerInterface::Response(Input));
    MOCK_METHOD0(getInputMock, Input());
    MOCK_METHOD1(addObserverMock, bool(std::weak_ptr<InputControllerObserverInterface>));
    MOCK_METHOD1(removeObserverMock, void(std::weak_ptr<InputControllerObserverInterface>));

    SupportedInputs getSupportedInputs() {
        return getSupportedInputsMock();
    }

    InputControllerInterface::Response setInput(Input input) {
        return setInputMock(input);
    }

    Input getInput() {
        return getInputMock();
    }

    bool addObserver(std::weak_ptr<InputControllerObserverInterface> observer) {
        return addObserverMock(observer);
    }

    void removeObserver(std::weak_ptr<InputControllerObserverInterface> observer) {
        removeObserverMock(observer);
    }
};

/// Test harness for @c InputControllerCapabilityAgentTest class.
class InputControllerCapabilityAgentTest : public Test {
public:
    /// Set up the test harness for running a test.
    void SetUp() override;

protected:
    /**
     * Function to create ModeControllerCapabilityAgent based on the reportable flags
     * and sets required ContextManager and ModeController mocks.
     */
    std::shared_ptr<AlexaInputControllerCapabilityAgent> createCapabilityAgentAndSetExpects(
        const InputControllerInterface::SupportedInputs& supportedInputsSet,
        bool proactivelyReported,
        bool retrievable);

    /// The AlexaInputControllerCapabilityAgent instance to be tested
    std::shared_ptr<AlexaInputControllerCapabilityAgent> m_inputControllerCA;

    // a context manager
    std::shared_ptr<StrictMock<MockContextManager>> m_mockContextManager;

    /// A mock @c InputControllerObserverInterface object.
    std::weak_ptr<InputControllerObserverInterface> m_observer;

    /// A mock @c InputControllerInterface object.
    std::shared_ptr<StrictMock<MockHandler>> m_mockInputHandler;

    /// Response sender mock to track events being sent.
    std::shared_ptr<MockAlexaInterfaceMessageSender> m_mockResponseSender;

    /// A strict mock that allows the test to strictly monitor the handling of directives.
    std::unique_ptr<StrictMock<MockDirectiveHandlerResult>> m_mockDirectiveHandlerResult;

    // A strict mock that allows the test to strictly monitor the exceptions being sent.
    std::shared_ptr<StrictMock<MockExceptionEncounteredSender>> m_mockExceptionSender;

    InputControllerInterface::SupportedInputs m_supportedInputs;
};

void InputControllerCapabilityAgentTest::SetUp() {
    m_mockExceptionSender = std::make_shared<StrictMock<MockExceptionEncounteredSender>>();
    m_mockDirectiveHandlerResult = make_unique<StrictMock<MockDirectiveHandlerResult>>();
    m_mockInputHandler = std::make_shared<StrictMock<MockHandler>>();
    m_mockContextManager = std::make_shared<testing::StrictMock<MockContextManager>>();
    m_mockResponseSender = std::make_shared<MockAlexaInterfaceMessageSender>();
}

std::shared_ptr<AlexaInputControllerCapabilityAgent> InputControllerCapabilityAgentTest::
    createCapabilityAgentAndSetExpects(
        const InputControllerInterface::SupportedInputs& inputSupportedSet,
        bool proactivelyReported,
        bool retrievable) {
    m_supportedInputs = inputSupportedSet;
    // ContextManager Mocks
    if (retrievable) {
        // Expects a non-null CA reference during create.
        EXPECT_CALL(*m_mockContextManager, addStateProvider(_, ::testing::NotNull()));

        // Expects a null when CA is shutdown.
        EXPECT_CALL(*m_mockContextManager, removeStateProvider(_));
    }

    if (proactivelyReported) {
        // addObserver Called during the initialization.
        EXPECT_CALL(*m_mockInputHandler, addObserverMock(_))
            .WillOnce(WithArg<0>(Invoke([this](std::weak_ptr<InputControllerObserverInterface> observer) {
                m_observer = observer;
                return true;
            })));

        // removeObserver is called during the shutdown
        EXPECT_CALL(*m_mockInputHandler, removeObserverMock(_))
            .WillOnce(WithArg<0>(
                Invoke([this](std::weak_ptr<InputControllerObserverInterface> observer) { m_observer.reset(); })));
    }

    auto inputControllerCA = AlexaInputControllerCapabilityAgent::create(
        TEST_ENDPOINT_ID,
        m_mockInputHandler,
        m_mockContextManager,
        m_mockResponseSender,
        m_mockExceptionSender,
        proactivelyReported,
        retrievable);

    return inputControllerCA;
}

/**
 * Utility function to build the @c AVSDirective for test.
 *
 * @param directiveName The name of the test directive to build.
 * @param payload The unparsed payload of directive in JSON.
 *
 * @return The created AVSDirective object or @c nullptr if build failed.
 */
static std::shared_ptr<AVSDirective> buildAVSDirective(
    std::string directiveName,
    std::string payload,
    std::string messageId = MESSAGE_ID_TEST) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<avsCommon::avs::attachment::test::MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE,
        directiveName,
        messageId,
        DIALOG_REQUEST_ID_TEST,
        CORRELATION_TOKEN_TEST,
        EVENT_CORRELATION_TOKEN_TEST,
        INTERFACE_VERSION,
        TEST_ENDPOINT_ID);
    auto avsMessageEndpoint = AVSMessageEndpoint(TEST_ENDPOINT_ID);

    return AVSDirective::create("", avsMessageHeader, payload, attachmentManager, "", avsMessageEndpoint);
}

/// Test that create() returns a nullptr if called with invalid arguments.
TEST_F(InputControllerCapabilityAgentTest, test_givenInvalidParameters_create_shouldFail) {
    ASSERT_THAT(
        AlexaInputControllerCapabilityAgent::create(
            "", m_mockInputHandler, m_mockContextManager, m_mockResponseSender, m_mockExceptionSender, true, true),
        IsNull());
    EXPECT_THAT(
        AlexaInputControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID, nullptr, m_mockContextManager, m_mockResponseSender, m_mockExceptionSender, true, true),
        IsNull());
    EXPECT_THAT(
        AlexaInputControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID, m_mockInputHandler, nullptr, m_mockResponseSender, m_mockExceptionSender, true, true),
        IsNull());
    EXPECT_THAT(
        AlexaInputControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID, m_mockInputHandler, m_mockContextManager, nullptr, m_mockExceptionSender, true, true),
        IsNull());
    EXPECT_THAT(
        AlexaInputControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID, m_mockInputHandler, m_mockContextManager, m_mockResponseSender, nullptr, true, true),
        IsNull());
}

/**
 * Test successful handling of SelectInput directive.
 */
TEST_F(InputControllerCapabilityAgentTest, test_selectInputDirective_successCase) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockInputHandler, getSupportedInputsMock()).WillOnce(InvokeWithoutArgs([] {
        return DEFAULT_ALLOWED_INPUT;
    }));

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockInputHandler, setInputMock(HDMI_1_INPUT_TEST)).WillOnce(InvokeWithoutArgs([] {
        return InputControllerInterface::Response();
    }));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _));

    auto inputControllerCapabilityAgent = createCapabilityAgentAndSetExpects(DEFAULT_ALLOWED_INPUT, false, true);
    ASSERT_THAT(inputControllerCapabilityAgent, NotNull());

    // Simulate directive
    inputControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(NAME_SETMODE, SELECT_INPUT_DIRECTIVE_JSON_STRING), std::move(m_mockDirectiveHandlerResult));
    inputControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    inputControllerCapabilityAgent->shutdown();
}

/**
 * Test error path of SelectInput directive.
 */
TEST_F(InputControllerCapabilityAgentTest, test_selectInputDirective_errorCase) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockInputHandler, getSupportedInputsMock()).WillOnce(InvokeWithoutArgs([] {
        return DEFAULT_ALLOWED_INPUT;
    }));

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockInputHandler, setInputMock(HDMI_1_INPUT_TEST)).WillOnce(InvokeWithoutArgs([] {
        InputControllerInterface::Response response;
        response.type = InputControllerInterface::Response::Type::FAILED_ENDPOINT_UNREACHABLE;
        response.errorMessage = "TestEndpointNotReachable";
        return response;
    }));

    EXPECT_CALL(
        *m_mockResponseSender,
        sendErrorResponseEvent(
            _, _, _, Matcher<AlexaInterfaceMessageSenderInterface::ErrorResponseType>(testing::_), _));

    auto inputControllerCapabilityAgent = createCapabilityAgentAndSetExpects(DEFAULT_ALLOWED_INPUT, false, true);
    ASSERT_THAT(inputControllerCapabilityAgent, NotNull());

    // Simulate directive
    inputControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(NAME_SETMODE, SELECT_INPUT_DIRECTIVE_JSON_STRING), std::move(m_mockDirectiveHandlerResult));
    inputControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    inputControllerCapabilityAgent->shutdown();
}

/**
 * Test triggering of reportStateChange and subsequent call to the
 * ContextManager to build the context.
 */
TEST_F(InputControllerCapabilityAgentTest, test_reportStateChange_successCase) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockInputHandler, getSupportedInputsMock()).WillOnce(InvokeWithoutArgs([] {
        return DEFAULT_ALLOWED_INPUT;
    }));

    auto inputControllerCapabilityAgent = createCapabilityAgentAndSetExpects(DEFAULT_ALLOWED_INPUT, true, true);
    ASSERT_THAT(inputControllerCapabilityAgent, NotNull());

    EXPECT_CALL(*m_mockInputHandler, setInputMock(CD_INPUT_TEST)).WillOnce(InvokeWithoutArgs([this] {
        auto m_observer_shared = m_observer.lock();
        m_observer_shared->onInputChanged(CD_INPUT_TEST);
        return InputControllerInterface::Response();
    }));

    EXPECT_CALL(*m_mockContextManager, reportStateChange(_, _, _))
        .WillOnce(InvokeWithoutArgs([inputControllerCapabilityAgent] {
            const unsigned int stateRequestToken = 1;
            inputControllerCapabilityAgent->provideState(
                CapabilityTag(NAMESPACE, INPUT_PROPERTY_NAME, TEST_ENDPOINT_ID), stateRequestToken);
        }));

    EXPECT_CALL(*m_mockInputHandler, getInputMock()).WillOnce(InvokeWithoutArgs([]() { return CD_INPUT_TEST; }));

    EXPECT_CALL(*m_mockContextManager, provideStateResponse(_, _, _)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    m_mockInputHandler->setInputMock(CD_INPUT_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    inputControllerCapabilityAgent->shutdown();
}

/**
 * Tests unknown Directives.
 * Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(InputControllerCapabilityAgentTest, test_unknownInvalidDirective) {
    static const std::vector<std::string> invalidPayloads{SELECT_INPUT_UNSUPPORTED_DIRECTIVE_JSON_STRING,
                                                          SELECT_INPUT_MISSING_TAG_DIRECTIVE_JSON_STRING,
                                                          SELECT_INPUT_EMPTY_INPUT_DIRECTIVE_JSON_STRING};

    EXPECT_CALL(*m_mockInputHandler, getSupportedInputsMock()).WillOnce(InvokeWithoutArgs([] {
        return DEFAULT_ALLOWED_INPUT;
    }));

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _)).Times(invalidPayloads.size());

    auto inputControllerCapabilityAgent = createCapabilityAgentAndSetExpects(DEFAULT_ALLOWED_INPUT, true, true);
    ASSERT_THAT(inputControllerCapabilityAgent, NotNull());

    std::string messageId = MESSAGE_ID_TEST;

    for (const auto& payload : invalidPayloads) {
        avsCommon::utils::WaitEvent waitEvent;

        messageId += "1";

        auto mockDirectiveHandlerResult = make_unique<StrictMock<MockDirectiveHandlerResult>>();

        EXPECT_CALL(*mockDirectiveHandlerResult, setFailed(_)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
            waitEvent.wakeUp();
        }));

        inputControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
            buildAVSDirective(NAME_SETMODE, payload, messageId), std::move(mockDirectiveHandlerResult));
        inputControllerCapabilityAgent->CapabilityAgent::handleDirective(messageId);

        EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    }

    inputControllerCapabilityAgent->shutdown();
}

}  // namespace test
}  // namespace alexaInputController
}  // namespace alexaClientSDK
