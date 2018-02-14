/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <ADSL/DirectiveSequencer.h>
#include <ADSL/MessageInterpreter.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

#include "ADSL/MockDirectiveSequencer.h"

namespace alexaClientSDK {
namespace adsl {
namespace test {

using namespace ::testing;
using namespace alexaClientSDK::avsCommon;
using namespace alexaClientSDK::avsCommon::avs;
using namespace alexaClientSDK::avsCommon::avs::attachment;
using namespace alexaClientSDK::avsCommon::sdkInterfaces::test;

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

static const std::string TEST_ATTACHMENT_CONTEXT_ID = "testContextId";

/// A sample AVS speak directive with all valid JSON keys.
// clang-format off
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
// clang-format on

/// A sample AVS speak directive with invalid directive JSON keys.
// clang-format off
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
// clang-format on

/// A sample AVS speak directive with invalid header key.
// clang-format off
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
// clang-format on

/// A sample AVS speak directive with invalid namespace key.
// clang-format off
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
// clang-format on

/// A sample AVS speak directive with invalid name key.
// clang-format off
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
// clang-format on

/// A sample AVS speak directive with invalid messageId key.
// clang-format off
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
// clang-format on

/// A sample AVS speak directive with no payload key.
// clang-format off
static const std::string DIRECTIVE_NO_PAYLOAD = R"({
    "directive": {
        "header": {
            "namespace":"namespace_test",
            "name": "name_test",
            "messageId": "messageId_test"
        }
    }
})";
// clang-format on

/// A sample AVS speak directive with invalid payload key.
// clang-format off
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
// clang-format on

/// A sample AVS speak directive with no dialogRequestId key.
// clang-format off
static const std::string DIRECTIVE_NO_DIALOG_REQUEST_ID_KEY = R"({
    "directive": {
        "header": {
            "namespace":")" + NAMESPACE_TEST + R"(",
            "name": ")" + NAME_TEST + R"(",
            "messageId": ")" + MESSAGE_ID_TEST + R"("
        },
        "payload": )" + PAYLOAD_TEST + R"(
    }
})";
// clang-format on

/**
 * Test fixture for testing MessageInterpreter class.
 */
class MessageIntepreterTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_mockExceptionEncounteredSender = std::make_shared<MockExceptionEncounteredSender>();
        m_mockDirectiveSequencer = std::make_shared<MockDirectiveSequencer>();
        m_attachmentManager = std::make_shared<AttachmentManager>(AttachmentManager::AttachmentType::IN_PROCESS);
        m_messageInterpreter = std::make_shared<MessageInterpreter>(
            m_mockExceptionEncounteredSender, m_mockDirectiveSequencer, m_attachmentManager);
    }
    /// The mock ExceptionEncounteredSender.
    std::shared_ptr<MockExceptionEncounteredSender> m_mockExceptionEncounteredSender;
    /// The attachment manager.
    std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> m_attachmentManager;
    /// The mock directive sequencer.
    std::shared_ptr<MockDirectiveSequencer> m_mockDirectiveSequencer;
    /// The message interpreter under test.
    std::shared_ptr<MessageInterpreter> m_messageInterpreter;
};

/**
 * Test when the content of message is invalid JSON format. The AVSDirective shouldn't be created and
 * and passed to directive sequencer. ExceptionEncounteredEvent should be sent to AVS.
 */
TEST_F(MessageIntepreterTest, messageIsInValidJSON) {
    EXPECT_CALL(*m_mockExceptionEncounteredSender, sendExceptionEncountered(_, _, _)).Times(1);
    EXPECT_CALL(*m_mockDirectiveSequencer, onDirective(_)).Times(0);
    m_messageInterpreter->receive(TEST_ATTACHMENT_CONTEXT_ID, INVALID_JSON);
}

/**
 * Test when the message doesn't contain the directive key in JSON content. The AVSDirective shouldn't be created and
 * and passed to directive sequencer. ExceptionEncounteredEvent should be sent to AVS.
 */
TEST_F(MessageIntepreterTest, messageHasInvalidDirectiveKey) {
    EXPECT_CALL(*m_mockExceptionEncounteredSender, sendExceptionEncountered(_, _, _)).Times(1);
    EXPECT_CALL(*m_mockDirectiveSequencer, onDirective(_)).Times(0);
    m_messageInterpreter->receive(TEST_ATTACHMENT_CONTEXT_ID, DIRECTIVE_INVALID_DIRECTIVE_KEY);
}

/**
 * Test when the message doesn't contain the header key in JSON content. The AVSDirective shouldn't be created and
 * and passed to directive sequencer. ExceptionEncounteredEvent should be sent to AVS.
 */
TEST_F(MessageIntepreterTest, messageHasInvalidHeaderKey) {
    EXPECT_CALL(*m_mockExceptionEncounteredSender, sendExceptionEncountered(_, _, _)).Times(1);
    EXPECT_CALL(*m_mockDirectiveSequencer, onDirective(_)).Times(0);
    m_messageInterpreter->receive(TEST_ATTACHMENT_CONTEXT_ID, DIRECTIVE_INVALID_HEADER_KEY);
}

/**
 * Test when the message doesn't contain the namespace key in JSON content. The AVSDirective shouldn't be created and
 * and passed to directive sequencer. ExceptionEncounteredEvent should be sent to AVS.
 */
TEST_F(MessageIntepreterTest, messageHasInvalidNamespaceKey) {
    EXPECT_CALL(*m_mockExceptionEncounteredSender, sendExceptionEncountered(_, _, _)).Times(1);
    EXPECT_CALL(*m_mockDirectiveSequencer, onDirective(_)).Times(0);
    m_messageInterpreter->receive(TEST_ATTACHMENT_CONTEXT_ID, DIRECTIVE_INVALID_NAMESPACE_KEY);
}

/**
 * Test when the message doesn't contain the name key in JSON content. The AVSDirective shouldn't be created and
 * and passed to directive sequencer. ExceptionEncounteredEvent should be sent to AVS.
 */
TEST_F(MessageIntepreterTest, messageHasInvalidNameKey) {
    EXPECT_CALL(*m_mockExceptionEncounteredSender, sendExceptionEncountered(_, _, _)).Times(1);
    EXPECT_CALL(*m_mockDirectiveSequencer, onDirective(_)).Times(0);
    m_messageInterpreter->receive(TEST_ATTACHMENT_CONTEXT_ID, DIRECTIVE_INVALID_NAME_KEY);
}

/**
 * Test when the message doesn't contain the messageId key in JSON content. The AVSDirective shouldn't be created and
 * and passed to directive sequencer. ExceptionEncounteredEvent should be sent to AVS.
 */
TEST_F(MessageIntepreterTest, messageHasInvalidMessageIdKey) {
    EXPECT_CALL(*m_mockExceptionEncounteredSender, sendExceptionEncountered(_, _, _)).Times(1);
    EXPECT_CALL(*m_mockDirectiveSequencer, onDirective(_)).Times(0);
    m_messageInterpreter->receive(TEST_ATTACHMENT_CONTEXT_ID, DIRECTIVE_INVALID_MESSAGEID_KEY);
}

/**
 * Test when the message doesn't contain the dialogRequestId key in JSON content. DialogRequestId is optional, so
 * AVSDirective should be created and passed to the directive sequencer.
 */
TEST_F(MessageIntepreterTest, messageHasNoDialogRequestIdKey) {
    EXPECT_CALL(*m_mockExceptionEncounteredSender, sendExceptionEncountered(_, _, _)).Times(0);
    EXPECT_CALL(*m_mockDirectiveSequencer, onDirective(_))
        .Times(1)
        .WillOnce(Invoke([](std::shared_ptr<AVSDirective> avsDirective) -> bool {
            EXPECT_EQ(avsDirective->getNamespace(), NAMESPACE_TEST);
            EXPECT_EQ(avsDirective->getName(), NAME_TEST);
            EXPECT_EQ(avsDirective->getMessageId(), MESSAGE_ID_TEST);
            EXPECT_TRUE(avsDirective->getDialogRequestId().empty());
            return true;
        }));
    m_messageInterpreter->receive(TEST_ATTACHMENT_CONTEXT_ID, DIRECTIVE_NO_DIALOG_REQUEST_ID_KEY);
}

/**
 * Test when the message doesn't contain the payload key in JSON content. The AVSDirective shouldn't be created and
 * and passed to directive sequencer. ExceptionEncounteredEvent should be sent to AVS.
 */
TEST_F(MessageIntepreterTest, messageHasNoPayloadKey) {
    EXPECT_CALL(*m_mockExceptionEncounteredSender, sendExceptionEncountered(_, _, _)).Times(1);
    EXPECT_CALL(*m_mockDirectiveSequencer, onDirective(_)).Times(0);
    m_messageInterpreter->receive(TEST_ATTACHMENT_CONTEXT_ID, DIRECTIVE_NO_PAYLOAD);
}

/**
 * Test when the message contains an invalid payload key in JSON content. The AVSDirective shouldn't be created and
 * and passed to directive sequencer. ExceptionEncounteredEvent should be sent to AVS.
 */
TEST_F(MessageIntepreterTest, messageHasInvalidPayloadKey) {
    EXPECT_CALL(*m_mockExceptionEncounteredSender, sendExceptionEncountered(_, _, _)).Times(1);
    EXPECT_CALL(*m_mockDirectiveSequencer, onDirective(_)).Times(0);
    m_messageInterpreter->receive(TEST_ATTACHMENT_CONTEXT_ID, DIRECTIVE_INVALID_PAYLOAD_KEY);
}

/**
 * Test when the message is valid JSON content with all keys required in the header. An AVSDirective should be created
 * and passed to the directive sequencer.
 */
TEST_F(MessageIntepreterTest, messageIsValidDirective) {
    EXPECT_CALL(*m_mockExceptionEncounteredSender, sendExceptionEncountered(_, _, _)).Times(0);
    EXPECT_CALL(*m_mockDirectiveSequencer, onDirective(_))
        .Times(1)
        .WillOnce(Invoke([](std::shared_ptr<AVSDirective> avsDirective) -> bool {
            EXPECT_EQ(avsDirective->getNamespace(), NAMESPACE_TEST);
            EXPECT_EQ(avsDirective->getName(), NAME_TEST);
            EXPECT_EQ(avsDirective->getMessageId(), MESSAGE_ID_TEST);
            EXPECT_EQ(avsDirective->getDialogRequestId(), DIALOG_REQUEST_ID_TEST);
            return true;
        }));
    m_messageInterpreter->receive(TEST_ATTACHMENT_CONTEXT_ID, SPEAK_DIRECTIVE);
}

}  // namespace test
}  // namespace adsl
}  // namespace alexaClientSDK
