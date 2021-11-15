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

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/SDKInterfaces/DirectiveSequencerInterface.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/Utils/Optional.h>

#include <acsdkInputController/InputControllerFactory.h>

namespace alexaClientSDK {
namespace acsdkInputController {
namespace test {

using namespace ::testing;
using namespace acsdkInputControllerInterfaces;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::configuration;

/// A simple SelectInput directive JSON string.
static const std::string SELECT_INPUT_DIRECTIVE_JSON_STRING = R"delim(
    {
        "directive": {
            "header": {
                "namespace": "Alexa.InputController",
                "name": "SelectInput",
                "messageId": "12345",
                "dialogRequestId": "2"
            },
            "payload": {
                "input": "HDMI1"
            }
        }
    }
)delim";

/// A SelectInput directive with invalid input (i.e. not part of the configuration as specified in @c
/// INPUT_CONTROLLER_CONFIG_JSON)
static const std::string SELECT_INPUT_DIRECTIVE_INVALID_INPUT_JSON_STRING = R"delim(
    {
        "directive": {
            "header": {
                "namespace": "Alexa.InputController",
                "name": "SelectInput",
                "messageId": "12345",
                "dialogRequestId": "2"
            },
            "payload": {
                "input": "RCA"
            }
        }
    }
)delim";

/// A SelectInput directive without input field
static const std::string SELECT_INPUT_MISSING_TAG_DIRECTIVE_JSON_STRING = R"delim(
    {
        "directive": {
            "header": {
                "namespace": "Alexa.InputController",
                "name": "SelectInput",
                "messageId": "12345",
                "dialogRequestId": "2"
            },
            "payload": {
            }
        }
    }
)delim";

/// A SelectInput directive with an empty input
static const std::string SELECT_INPUT_EMPTY_INPUT_DIRECTIVE_JSON_STRING = R"delim(
    {
        "directive": {
            "header": {
                "namespace": "Alexa.InputController",
                "name": "SelectInput",
                "messageId": "12345",
                "dialogRequestId": "2"
            },
            "payload": {
                "input": ""
            }
        }
    }
)delim";

/// The valid input configuration used for testing
static const InputControllerHandlerInterface::InputFriendlyNameType INPUT_CONTROLLER_GOOD_CONFIG{
    {"HDMI1", {"TV", "Television"}},
    {"HDMI2", {"Game Console"}}};

/// An invalid input configuration with duplicated friendly names across inputs
static const InputControllerHandlerInterface::InputFriendlyNameType INPUT_CONTROLLER_DUPLICATE_FRIENDLY_NAMES_CONFIG{
    {"HDMI1", {"TV", "Television"}},
    {"HDMI2", {"TV"}}};

/// An invalid input configuration with no inputs
static const InputControllerHandlerInterface::InputFriendlyNameType INPUT_CONTROLLER_EMPTY_CONFIG{};

// Timeout to wait before indicating a test failed.
static const std::chrono::milliseconds TIMEOUT{500};

/// A Mock for @c InputControllerHandlerInterface
class MockHandler : public InputControllerHandlerInterface {
public:
    /// @name InputControllerHandlerInterface Functions
    /// @{
    InputConfigurations getConfiguration() override;
    bool onInputChange(const std::string& input) override;
    /// @}

    /**
     * Waits for the onInputChange() function to be called.
     *
     * @return An @c Optional of input.  If callback is before timeout, the Optional will be filled with input,
     * otherwise and Empty optional if there was no callback.
     */
    Optional<std::string> waitOnInputChange();

    /// Ctor
    MockHandler(InputControllerHandlerInterface::InputFriendlyNameType inputConfigs) : m_inputsConfig{inputConfigs} {};

protected:
    std::mutex m_mutex;
    std::condition_variable m_cond;
    bool m_inputCalled = false;
    std::string m_input;
    InputControllerHandlerInterface::InputFriendlyNameType m_inputsConfig;
};

bool MockHandler::onInputChange(const std::string& input) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_input = input;
    m_inputCalled = true;
    m_cond.notify_all();
    return true;
}

InputControllerHandlerInterface::InputConfigurations MockHandler::getConfiguration() {
    return {m_inputsConfig};
}

Optional<std::string> MockHandler::waitOnInputChange() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond.wait_for(lock, TIMEOUT, [this] { return m_inputCalled; });
    if (m_inputCalled) {
        return Optional<std::string>(m_input);
    }
    return Optional<std::string>();
}

/// Test harness for @c InputControllerCapabilityAgentTest class.
class InputControllerCapabilityAgentTest : public Test {
public:
    /// Set up the test harness for running a test.
    void SetUp() override;

protected:
    /// The InputControllerCapabilityAgent instance to be tested
    InputControllerFactoryInterfaces m_inputControllerCA;

    /// The mock handler for testing
    std::shared_ptr<MockHandler> m_mockHandler;

    /// The mock @c ExceptionEncounteredSenderInterface.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender> m_mockExceptionEncounteredSender;
};

void InputControllerCapabilityAgentTest::SetUp() {
    m_mockExceptionEncounteredSender =
        std::make_shared<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender>();

    m_mockHandler = std::make_shared<MockHandler>(INPUT_CONTROLLER_GOOD_CONFIG);
    auto inputControllerCA = create(m_mockHandler, m_mockExceptionEncounteredSender);

    ASSERT_TRUE(inputControllerCA.hasValue());
    m_inputControllerCA = inputControllerCA.value();
}

/**
 * Test to verify the @c InputControllerCapabilityAgent cannot be created if handler param is null.
 */
TEST_F(InputControllerCapabilityAgentTest, test_createNoHandlerFail) {
    auto inputControllerCA = create(nullptr, m_mockExceptionEncounteredSender);
    ASSERT_FALSE(inputControllerCA.hasValue());
}

/**
 * Test to verify the @c InputControllerCapabilityAgent cannot be created if exceptionHandler param is null.
 */
TEST_F(InputControllerCapabilityAgentTest, test_createNoExceptionHandlerFail) {
    auto inputControllerCA = create(m_mockHandler, nullptr);
    ASSERT_FALSE(inputControllerCA.hasValue());
}

/**
 * Test to verify the @c InputControllerCapabilityAgent cannot be created if config param is empty.
 */
TEST_F(InputControllerCapabilityAgentTest, test_createEmptyConfigFail) {
    auto mockHandler = std::make_shared<MockHandler>(INPUT_CONTROLLER_EMPTY_CONFIG);
    auto inputControllerCA = create(mockHandler, m_mockExceptionEncounteredSender);
    ASSERT_FALSE(inputControllerCA.hasValue());
}

/**
 * Test to verify the @c InputControllerCapabilityAgent cannot be created if the configuration is not correct.
 */
TEST_F(InputControllerCapabilityAgentTest, test_createWithDuplicateFriendlyNamesFail) {
    auto mockHandler = std::make_shared<MockHandler>(INPUT_CONTROLLER_DUPLICATE_FRIENDLY_NAMES_CONFIG);
    auto inputControllerCA = create(mockHandler, m_mockExceptionEncounteredSender);
    ASSERT_FALSE(inputControllerCA.hasValue());
}

/**
 * Test to verify if a valid SelectInput directive will set the input successfully.
 */
TEST_F(InputControllerCapabilityAgentTest, test_selectInputDirectiveSuccess) {
    // Create a SelectInput AVSDirective.
    auto directivePair = AVSDirective::create(SELECT_INPUT_DIRECTIVE_JSON_STRING, nullptr, "");
    std::shared_ptr<AVSDirective> directive = std::move(directivePair.first);

    m_inputControllerCA.directiveHandler->handleDirectiveImmediately(directive);
    auto observerReturned = m_mockHandler->waitOnInputChange();
    EXPECT_TRUE(observerReturned.hasValue());
    EXPECT_EQ("HDMI1", observerReturned.value());
}

/**
 * Test to verify if interface will send exceptions when the directive received is invalid
 */
TEST_F(InputControllerCapabilityAgentTest, test_processInvalidDirectiveFail) {
    std::shared_ptr<AVSDirective> directive1 =
        AVSDirective::create(SELECT_INPUT_DIRECTIVE_INVALID_INPUT_JSON_STRING, nullptr, "").first;
    std::shared_ptr<AVSDirective> directive2 =
        AVSDirective::create(SELECT_INPUT_MISSING_TAG_DIRECTIVE_JSON_STRING, nullptr, "").first;
    std::shared_ptr<AVSDirective> directive3 =
        AVSDirective::create(SELECT_INPUT_EMPTY_INPUT_DIRECTIVE_JSON_STRING, nullptr, "").first;

    EXPECT_CALL(*m_mockExceptionEncounteredSender, sendExceptionEncountered(_, _, _)).Times(3);
    m_inputControllerCA.directiveHandler->handleDirectiveImmediately(directive1);
    m_inputControllerCA.directiveHandler->handleDirectiveImmediately(directive2);
    m_inputControllerCA.directiveHandler->handleDirectiveImmediately(directive3);

    auto observerReturned = m_mockHandler->waitOnInputChange();
    EXPECT_FALSE(observerReturned.hasValue());
}

}  // namespace test
}  // namespace acsdkInputController
}  // namespace alexaClientSDK
