/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <iterator>
#include <memory>
#include <sstream>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/SDKInterfaces/MockDirectiveSequencer.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>

#include "InteractionModel/InteractionModelCapabilityAgent.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace interactionModel {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace ::testing;

const std::string TEST_DIALOG_REQUEST_AVS = "2";

/// A sample Directive JSON string for the purposes of creating an AVSDirective object.
static const std::string CORRECT_NEW_DIALOG_REQUEST_DIRECTIVE_JSON_STRING = R"delim(
    {
        "directive": {
            "header": {
                "namespace": "InteractionModel",
                "name": "NewDialogRequest",
                "messageId": "12345"
            },
            "payload": {
                "dialogRequestId": "2"
            }
        }
    })delim";

/// An invalid NewDialogRequest directive with an incorrect name
static const std::string INCORRECT_NEW_DIALOG_REQUEST_DIRECTIVE_JSON_STRING_1 = R"delim(
    {
        "directive": {
            "header": {
                "namespace": "InteractionModel",
                "name": "NewDialogRequest1",
                "messageId": "12345"
            },
            "payload": {
                "dialogRequestId": "2"
            }
        }
    })delim";

/// An invalid NewDialogRequest directive with no payload
static const std::string INCORRECT_NEW_DIALOG_REQUEST_DIRECTIVE_JSON_STRING_2 = R"delim(
    {
        "directive": {
            "header": {
                "namespace": "InteractionModel",
                "name": "NewDialogRequest",
                "messageId": "12345"
            },
            "payload": {
                
            }
        }
    })delim";

/// An invalid NewDialogRequest with invalid dialogRequestID format
static const std::string INCORRECT_NEW_DIALOG_REQUEST_DIRECTIVE_JSON_STRING_3 = R"delim(
    {
        "directive": {
            "header": {
                "namespace": "InteractionModel",
                "name": "NewDialogRequest",
                "messageId": "12345"
            },
            "payload": {
                "dialogRequestId": 2
            }
        }
    })delim";

/// Test harness for @c InteractionModelCapabilityAgent class.
class InteractionModelCapabilityAgentTest : public Test {
public:
    /// Set up the test harness for running a test.
    void SetUp() override;

protected:
    /// The InteractionModelCapabilityAgent instance to be tested
    std::shared_ptr<InteractionModelCapabilityAgent> m_interactionModelCA;

    /// The mock @c DirectiveSequencerInterface.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockDirectiveSequencer> m_mockDirectiveSequencer;

    /// The mock @c ExceptionEncounteredSenderInterface.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender> m_mockExceptionEncounteredSender;
};

void InteractionModelCapabilityAgentTest::SetUp() {
    m_mockDirectiveSequencer = std::make_shared<avsCommon::sdkInterfaces::test::MockDirectiveSequencer>();

    m_mockExceptionEncounteredSender =
        std::make_shared<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender>();

    m_interactionModelCA =
        InteractionModelCapabilityAgent::create(m_mockDirectiveSequencer, m_mockExceptionEncounteredSender);

    ASSERT_NE(m_interactionModelCA, nullptr);
}

/**
 * Test to verify the @c InteractionModelCapabilityAgent can not be created if directiveSequencer param is null.
 */
TEST_F(InteractionModelCapabilityAgentTest, createNoDirectiveSequencer) {
    m_interactionModelCA = InteractionModelCapabilityAgent::create(nullptr, m_mockExceptionEncounteredSender);

    ASSERT_EQ(m_interactionModelCA, nullptr);
}
/**
 * Test to verify the @c InteractionModelCapabilityAgent can not be created if exceptionHandler param is null.
 */
TEST_F(InteractionModelCapabilityAgentTest, createNoExceptionHanlder) {
    m_interactionModelCA = InteractionModelCapabilityAgent::create(m_mockDirectiveSequencer, nullptr);

    ASSERT_EQ(m_interactionModelCA, nullptr);
}

/**
 * Test to verify if a valid NewDialogRequest directive will set the dialogRequestID in the directive sequencer.
 */
TEST_F(InteractionModelCapabilityAgentTest, processNewDialogRequestID) {
    // Create a dummy AVSDirective.
    auto directivePair = AVSDirective::create(CORRECT_NEW_DIALOG_REQUEST_DIRECTIVE_JSON_STRING, nullptr, "");
    std::shared_ptr<AVSDirective> directive = std::move(directivePair.first);

    EXPECT_CALL(*m_mockDirectiveSequencer, setDialogRequestId(TEST_DIALOG_REQUEST_AVS));
    m_interactionModelCA->handleDirectiveImmediately(directive);
}

/**
 * Test to verify if interface will ignore null directives
 */
TEST_F(InteractionModelCapabilityAgentTest, processNullDirective) {
    EXPECT_CALL(*m_mockDirectiveSequencer, setDialogRequestId(_)).Times(0);
    m_interactionModelCA->handleDirectiveImmediately(nullptr);
}

/**
 * Test to verify if interface will send exceptions when the directive received is invalid
 */
TEST_F(InteractionModelCapabilityAgentTest, processInvalidDirective) {
    std::shared_ptr<AVSDirective> directive1 =
        AVSDirective::create(INCORRECT_NEW_DIALOG_REQUEST_DIRECTIVE_JSON_STRING_1, nullptr, "").first;
    std::shared_ptr<AVSDirective> directive2 =
        AVSDirective::create(INCORRECT_NEW_DIALOG_REQUEST_DIRECTIVE_JSON_STRING_2, nullptr, "").first;
    std::shared_ptr<AVSDirective> directive3 =
        AVSDirective::create(INCORRECT_NEW_DIALOG_REQUEST_DIRECTIVE_JSON_STRING_3, nullptr, "").first;

    EXPECT_CALL(*m_mockDirectiveSequencer, setDialogRequestId(_)).Times(0);
    EXPECT_CALL(*m_mockExceptionEncounteredSender, sendExceptionEncountered(_, _, _)).Times(3);
    m_interactionModelCA->handleDirectiveImmediately(directive1);
    m_interactionModelCA->handleDirectiveImmediately(directive2);
    m_interactionModelCA->handleDirectiveImmediately(directive3);
}

}  // namespace test
}  // namespace interactionModel
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
