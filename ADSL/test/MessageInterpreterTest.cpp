/*
 * MessageInterpreterTest.cpp
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

/// @file MessageInterpreterTest.cpp

#include <AVSCommon/AttachmentManager.h>
#include <AVSCommon/MockExceptionEncounteredSender.h>
#include <ADSL/DirectiveSequencer.h>
#include <ADSL/MessageInterpreter.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

#include "ADSL/MockDirectiveSequencer.h"

namespace alexaClientSDK {
namespace adsl {

using namespace ::testing;
using namespace alexaClientSDK::acl;
using namespace alexaClientSDK::avsCommon;

/// The namespace in AVS message.
static const std::string NAMESPACE_TEST = "SpeechSynthesizer";
/// The name field in AVS message.
static const std::string NAME_TEST = "Speak";
/// The messageId in AVS message.
static const std::string MESSAGE_ID_TEST = "testMessageId";
/// The dialogRequestId in AVS message.
static const std::string DIALOG_REQUEST_ID_TEST = "dialogRequestIdTest";
/// The payload in AVS message.
static const std::string PAYLOAD_TEST = R"({"url":"cid:testCID","format":"testFormat","token":"testToken"})";
/// An invalid JSON string for testing.
static const std::string INVALID_JSON = "invalidTestJSON }}";

/// A sample AVS speak directive with all valid JSON keys.
static const std::string SPEAK_DIRECTIVE = R"({
    "directive": {
        "header": {
            "namespace":")" + NAMESPACE_TEST + R"(",
            "name": ")" + NAME_TEST + R"(",
            "messageId": ")" + MESSAGE_ID_TEST + R"(",
            "dialogRequestId": ")" + DIALOG_REQUEST_ID_TEST + R"("
        },
        "payload": )" + PAYLOAD_TEST + R"(
    }
})";

/// A sample AVS speak directive with invalid directive JSON keys.
static const std::string DIRECTIVE_INVALID_DIRECTIVE_KEY = R"({
    "Foo_directive": {
        "header": {
            "namespace":"namespace_test",
            "name": "name_test",
            "messageId": "messageId_test",
            "dialogRequestId": "dialogRequestId_test"
        },
        "payload":{}
    }
})";

/// A sample AVS speak directive with invalid header key.
static const std::string DIRECTIVE_INVALID_HEADER_KEY = R"({
    "directive": {
        "Foo_header": {
            "namespace":"namespace_test",
            "name": "name_test",
            "messageId": "messageId_test",
            "dialogRequestId": "dialogRequestId_test"
        },
        "payload":{}
    }
})";

/// A sample AVS speak directive with invalid namespace key.
static const std::string DIRECTIVE_INVALID_NAMESPACE_KEY = R"({
    "directive": {
        "header": {
            "Foo_namespace":"namespace_test",
            "name": "name_test",
            "messageId": "messageId_test",
            "dialogRequestId": "dialogRequestId_test"
        },
        "payload":{}
    }
})";

/// A sample AVS speak directive with invalid name key.
static const std::string DIRECTIVE_INVALID_NAME_KEY = R"({
    "directive": {
        "header": {
            "namespace":"namespace_test",
            "Foo_name": "name_test",
            "messageId": "messageId_test",
            "dialogRequestId": "dialogRequestId_test"
        },
        "payload":{}
    }
})";

/// A sample AVS speak directive with invalid messageId key.
static const std::string DIRECTIVE_INVALID_MESSAGEID_KEY = R"({
    "directive": {
        "header": {
            "namespace":"namespace_test",
            "name": "name_test",
            "Foo_messageId": "messageId_test",
            "dialogRequestId": "dialogRequestId_test"
        },
        "payload":{}
    }
})";

/// A sample AVS speak directive with no payload key.
static const std::string DIRECTIVE_NO_PAYLOAD = R"({
    "directive": {
        "header": {
            "namespace":"namespace_test",
            "name": "name_test",
            "messageId": "messageId_test"
        }
    }
})";

/// A sample AVS speak directive with invalid payload key.
static const std::string DIRECTIVE_INVALID_PAYLOAD_KEY = R"({
    "directive": {
        "header": {
            "namespace":"namespace_test",
            "name": "name_test",
            "Foo_messageId": "messageId_test",
            "dialogRequestId": "dialogRequestId_test"
        },
        "Foo_payload":{}
    }
})";

/// A sample AVS speak directive with no dialogRequestId key.
static const std::string DIRECTIVE_NO_DIALOG_REQUEST_ID_KEY = R"({
    "directive": {
        "header": {
            "namespace":"namespace_test",
            "name": "name_test",
            "messageId": "messageId_test"
        },
        "payload":{}
    }
})";

/**
 * Test fixture for testing MessageInterpreter class.
 */
class MessageIntepreterTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_mockExceptionEncounteredSender = std::make_shared<MockExceptionEncounteredSender>();
        m_mockDirectiveSequencer = std::make_shared<MockDirectiveSequencer>();
        m_messageInterpreter = std::make_shared<MessageInterpreter>(m_mockExceptionEncounteredSender,
            m_mockDirectiveSequencer);
        m_attachmentManager = std::make_shared<AttachmentManager>();
    }
    /// The mock ExceptionEncounteredSender.
    std::shared_ptr<MockExceptionEncounteredSender> m_mockExceptionEncounteredSender;
    /// The attachment manager.
    std::shared_ptr<AttachmentManagerInterface> m_attachmentManager;
    /// The mock directive sequencer.
    std::shared_ptr<MockDirectiveSequencer> m_mockDirectiveSequencer;
    /// The message interpreter under test.
    std::shared_ptr<MessageInterpreter> m_messageInterpreter;
};

/**
 * Test creating the AVSDirective with attachment manager set to @c nullptr. The AVSDirective shouldn't be created and
 * and passed to directive sequencer. ExceptionEncounteredEvent should be sent to AVS.
 */
TEST_F(MessageIntepreterTest, messageWithoutAttachmentManager) {
    EXPECT_CALL(*m_mockExceptionEncounteredSender, sendExceptionEncountered(_, _, _))
            .Times(1);
    EXPECT_CALL(*m_mockDirectiveSequencer, onDirective(_))
            .Times(0);
    std::shared_ptr<avsCommon::avs::Message> message = std::make_shared<avsCommon::avs::Message>(SPEAK_DIRECTIVE);
    m_messageInterpreter->receive(message);
}

/**
 * Test when the content of message is invalid JSON format. The AVSDirective shouldn't be created and
 * and passed to directive sequencer. ExceptionEncounteredEvent should be sent to AVS.
 */
TEST_F(MessageIntepreterTest, messageIsInValidJSON) {
    EXPECT_CALL(*m_mockExceptionEncounteredSender, sendExceptionEncountered(_, _, _))
            .Times(1);
    EXPECT_CALL(*m_mockDirectiveSequencer, onDirective(_))
            .Times(0);
    auto message = std::make_shared<avsCommon::avs::Message>(INVALID_JSON, m_attachmentManager);
    m_messageInterpreter->receive(message);
}

/**
 * Test when the message doesn't contain the directive key in JSON content. The AVSDirective shouldn't be created and
 * and passed to directive sequencer. ExceptionEncounteredEvent should be sent to AVS.
 */
TEST_F(MessageIntepreterTest, messageHasInvalidDirectiveKey) {
    EXPECT_CALL(*m_mockExceptionEncounteredSender, sendExceptionEncountered(_, _, _))
            .Times(1);
    EXPECT_CALL(*m_mockDirectiveSequencer, onDirective(_))
            .Times(0);
    auto message = std::make_shared<avsCommon::avs::Message>(DIRECTIVE_INVALID_DIRECTIVE_KEY, m_attachmentManager);
    m_messageInterpreter->receive(message);
}

/**
 * Test when the message doesn't contain the header key in JSON content. The AVSDirective shouldn't be created and
 * and passed to directive sequencer. ExceptionEncounteredEvent should be sent to AVS.
 */
TEST_F(MessageIntepreterTest, messageHasInvalidHeaderKey) {
    EXPECT_CALL(*m_mockExceptionEncounteredSender, sendExceptionEncountered(_, _, _))
            .Times(1);
    EXPECT_CALL(*m_mockDirectiveSequencer, onDirective(_))
            .Times(0);
    auto message = std::make_shared<avsCommon::avs::Message>(DIRECTIVE_INVALID_HEADER_KEY, m_attachmentManager);
    m_messageInterpreter->receive(message);
}

/**
 * Test when the message doesn't contain the namespace key in JSON content. The AVSDirective shouldn't be created and
 * and passed to directive sequencer. ExceptionEncounteredEvent should be sent to AVS.
 */
TEST_F(MessageIntepreterTest, messageHasInvalidNamespaceKey) {
    EXPECT_CALL(*m_mockExceptionEncounteredSender, sendExceptionEncountered(_, _, _))
            .Times(1);
    EXPECT_CALL(*m_mockDirectiveSequencer, onDirective(_))
            .Times(0);
    auto message = std::make_shared<avsCommon::avs::Message>(DIRECTIVE_INVALID_NAMESPACE_KEY, m_attachmentManager);
    m_messageInterpreter->receive(message);
}

/**
 * Test when the message doesn't contain the name key in JSON content. The AVSDirective shouldn't be created and
 * and passed to directive sequencer. ExceptionEncounteredEvent should be sent to AVS.
 */
TEST_F(MessageIntepreterTest, messageHasInvalidNameKey) {
    EXPECT_CALL(*m_mockExceptionEncounteredSender, sendExceptionEncountered(_, _, _))
            .Times(1);
    EXPECT_CALL(*m_mockDirectiveSequencer, onDirective(_))
            .Times(0);
    auto message = std::make_shared<avsCommon::avs::Message>(DIRECTIVE_INVALID_NAME_KEY, m_attachmentManager);
    m_messageInterpreter->receive(message);
}

/**
 * Test when the message doesn't contain the messageId key in JSON content. The AVSDirective shouldn't be created and
 * and passed to directive sequencer. ExceptionEncounteredEvent should be sent to AVS.
 */
TEST_F(MessageIntepreterTest, messageHasInvalidMessageIdKey) {
    EXPECT_CALL(*m_mockExceptionEncounteredSender, sendExceptionEncountered(_, _, _))
            .Times(1);
    EXPECT_CALL(*m_mockDirectiveSequencer, onDirective(_))
            .Times(0);
    auto message = std::make_shared<avsCommon::avs::Message>(DIRECTIVE_INVALID_MESSAGEID_KEY, m_attachmentManager);
    m_messageInterpreter->receive(message);
}

/**
 * Test when the message doesn't contain the dialogRequestId key in JSON content. DialogRequestId is optional, so
 * AVSDirective should be created and passed to the directive sequencer.
 */
TEST_F(MessageIntepreterTest, messageHasNoDialogRequestIdKey) {
    EXPECT_CALL(*m_mockExceptionEncounteredSender, sendExceptionEncountered(_, _, _))
            .Times(0);
    auto message = std::make_shared<avsCommon::avs::Message>(DIRECTIVE_NO_DIALOG_REQUEST_ID_KEY, m_attachmentManager);
    m_messageInterpreter->receive(message);
}

/**
 * Test when the message doesn't contain the payload key in JSON content. The AVSDirective shouldn't be created and
 * and passed to directive sequencer. ExceptionEncounteredEvent should be sent to AVS.
 */
TEST_F(MessageIntepreterTest, messageHasNoPayloadKey) {
    EXPECT_CALL(*m_mockExceptionEncounteredSender, sendExceptionEncountered(_, _, _))
            .Times(1);
    EXPECT_CALL(*m_mockDirectiveSequencer, onDirective(_))
            .Times(0);
    auto message = std::make_shared<avsCommon::avs::Message>(DIRECTIVE_NO_PAYLOAD, m_attachmentManager);
    m_messageInterpreter->receive(message);
}

/**
 * Test when the message contains an invalid payload key in JSON content. The AVSDirective shouldn't be created and
 * and passed to directive sequencer. ExceptionEncounteredEvent should be sent to AVS.
 */
TEST_F(MessageIntepreterTest, messageHasInvalidPayloadKey) {
    EXPECT_CALL(*m_mockExceptionEncounteredSender, sendExceptionEncountered(_, _, _))
            .Times(1);
    EXPECT_CALL(*m_mockDirectiveSequencer, onDirective(_))
            .Times(0);
    auto message = std::make_shared<avsCommon::avs::Message>(DIRECTIVE_INVALID_PAYLOAD_KEY, m_attachmentManager);
    m_messageInterpreter->receive(message);
}

/**
 * Test when the message is valid JSON content with all keys required in the header. An AVSDirective should be created
 * and passed to the directive sequencer.
 */
TEST_F(MessageIntepreterTest, messageIsValidDirective) {
    EXPECT_CALL(*m_mockExceptionEncounteredSender, sendExceptionEncountered(_, _, _))
            .Times(0);
    EXPECT_CALL(*m_mockDirectiveSequencer, onDirective(_))
            .Times(1)
            .WillOnce(Invoke([](std::shared_ptr<AVSDirective> avsDirective) -> bool {
                // This lambda function is necessary because the ASSERT_EQ is a macro that "returns void";
                auto verifyArguments = [](std::shared_ptr<AVSDirective> avsDirective) {
                    ASSERT_EQ(avsDirective->getNamespace(), NAMESPACE_TEST);
                    ASSERT_EQ(avsDirective->getName(), NAME_TEST);
                    ASSERT_EQ(avsDirective->getMessageId(), MESSAGE_ID_TEST);
                    ASSERT_EQ(avsDirective->getDialogRequestId(), DIALOG_REQUEST_ID_TEST);
                };
                verifyArguments(avsDirective);
                return true;
            }));
    auto message = std::make_shared<avsCommon::avs::Message>(SPEAK_DIRECTIVE, m_attachmentManager);
    m_messageInterpreter->receive(message);
}

} // namespace adsl
} // namespace alexaClientSDK
