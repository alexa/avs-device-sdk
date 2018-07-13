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

#include <memory>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <AVSCommon/AVS/ExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h>

using namespace testing;

namespace alexaClientSDK {
namespace avsCommon {
namespace test {

using namespace utils::json::jsonUtils;

/// The namespace for this event
static const std::string NAMESPACE = "System";

/// JSON key for the context section of a message.
static const std::string MESSAGE_CONTEXT_KEY = "context";

/// JSON key for the event section of a message.
static const std::string MESSAGE_EVENT_KEY = "event";

/// JSON key for the header section of a message.
static const std::string MESSAGE_HEADER_KEY = "header";

/// JSON key for the namespace field of a message header.
static const std::string MESSAGE_NAMESPACE_KEY = "namespace";

/// JSON key for the name field of a message header.
static const std::string MESSAGE_NAME_KEY = "name";

/// JSON key for the message ID field of a message header.
static const std::string MESSAGE_MESSAGE_ID_KEY = "messageId";

/// JSON key for the payload section of an message.
static const std::string MESSAGE_PAYLOAD_KEY = "payload";

/// JSON value for a ExceptionEncountered event's name.
static const std::string EXCEPTION_ENCOUNTERED_EVENT_NAME = "ExceptionEncountered";

/// JSON key for the Unparsed Directive field of an ExceptionEncountered event.
static const std::string UNPARSED_DIRECTIVE_KEY = "unparsedDirective";

/// JSON key for the error field of an ExceptionEncountered event.
static const std::string EXCEPTION_ENCOUNTERED_ERROR_KEY = "error";

/// JSON key for the type field of a ExceptionEncountered event's error.
static const std::string ERROR_TYPE_KEY = "type";

/// JSON key for the message field of a ExceptionEncountered event's error description.
static const std::string ERROR_MESSAGE_KEY = "message";

/// String to send unparsed Directive in @c testExceptionEncounteredSucceeds
static const std::string UNPARSED_DIRECTIVE_JSON_STRING = "unparsedDirective Json String";

/**
 * Utility class which captures parameters to a ExceptionEncountered event,
 * and provides functions to send and verify the event
 * using those parameters.
 */
class ExceptionEncounteredEvent {
public:
    /**
     * Constructs an object which captures the parameters to send in a ExceptionEncountered Event.
     * Parameters are passed through
     * directly to @c ExceptionEncounteredSender::sendExceptionEncountered().
     */
    ExceptionEncounteredEvent(
        const std::string& unparsedDirective,
        avs::ExceptionErrorType error,
        const std::string& errorDescription);

    /**
     * This function sends a ExceptionEncountered event using the provided @c exceptionencounteredsender.
     *
     * @param exceptionencounteredsender The @c ExceptionEncounteredSender to call on
     * @c ExceptionEncounteredSender::sendExceptionEncountered().
     */
    void send(std::shared_ptr<avs::ExceptionEncounteredSender> exceptionencounteredsender);

    /**
     * This function verifies that JSON content of a ExceptionEncountered event @c MessageRequest is correct.
     * This function signature matches that of @c MessageSenderInterface::sendMessage() so that an
     * @c EXPECT_CALL() can @c Invoke() this function directly.
     *
     * @param request The @c MessageRequest to verify.
     */
    void verifyMessage(std::shared_ptr<avsCommon::avs::MessageRequest> request);

private:
    /// The Unparsed Directive string to use for this ExceptionEncountered event
    std::string m_unparsedDirective;

    /// The Error type to use for this ExceptionEncountered event
    avs::ExceptionErrorType m_error;

    /// The Error Description message to use for this ExceptionEncountered event
    std::string m_errorDescription;
};

ExceptionEncounteredEvent::ExceptionEncounteredEvent(
    const std::string& unparsedDirective,
    avs::ExceptionErrorType error,
    const std::string& errorDescription) :
        m_unparsedDirective{unparsedDirective},
        m_error{error},
        m_errorDescription{errorDescription} {
}

void ExceptionEncounteredEvent::send(std::shared_ptr<avs::ExceptionEncounteredSender> exceptionEncounteredSender) {
    exceptionEncounteredSender->sendExceptionEncountered(m_unparsedDirective, m_error, m_errorDescription);
}

void ExceptionEncounteredEvent::verifyMessage(std::shared_ptr<avsCommon::avs::MessageRequest> request) {
    rapidjson::Document document;
    document.Parse(request->getJsonContent().c_str());
    EXPECT_FALSE(document.HasParseError())
        << "rapidjson detected a parsing error at offset:" << std::to_string(document.GetErrorOffset())
        << ", error message: " << GetParseError_En(document.GetParseError());

    auto context = document.FindMember(MESSAGE_CONTEXT_KEY);
    ASSERT_NE(context, document.MemberEnd());
    auto event = document.FindMember(MESSAGE_EVENT_KEY);
    ASSERT_NE(event, document.MemberEnd());

    auto header = event->value.FindMember(MESSAGE_HEADER_KEY);
    ASSERT_NE(header, event->value.MemberEnd());
    auto payload = event->value.FindMember(MESSAGE_PAYLOAD_KEY);
    ASSERT_NE(payload, event->value.MemberEnd());

    std::string temp_string = "";
    ASSERT_TRUE(retrieveValue(header->value, MESSAGE_NAMESPACE_KEY, &temp_string));
    EXPECT_EQ(temp_string, NAMESPACE);
    ASSERT_TRUE(retrieveValue(header->value, MESSAGE_NAME_KEY, &temp_string));
    EXPECT_EQ(temp_string, EXCEPTION_ENCOUNTERED_EVENT_NAME);
    ASSERT_TRUE(retrieveValue(header->value, MESSAGE_MESSAGE_ID_KEY, &temp_string));
    ASSERT_NE(temp_string, "");

    ASSERT_TRUE(retrieveValue(payload->value, UNPARSED_DIRECTIVE_KEY, &temp_string));
    EXPECT_EQ(temp_string, m_unparsedDirective);
    auto error = payload->value.FindMember(EXCEPTION_ENCOUNTERED_ERROR_KEY);
    ASSERT_NE(error, payload->value.MemberEnd());

    std::ostringstream errorType;
    errorType << m_error;
    ASSERT_TRUE(retrieveValue(error->value, ERROR_TYPE_KEY, &temp_string));
    EXPECT_EQ(temp_string, errorType.str());
    ASSERT_TRUE(retrieveValue(error->value, ERROR_MESSAGE_KEY, &temp_string));
    EXPECT_EQ(temp_string, m_errorDescription);
}

/// Test harness for @c ExceptionEncounteredSender class.
class ExceptionEncounteredSenderTest : public ::testing::Test {
public:
    /// Set up the test harness for running a test.
    void SetUp() override;

protected:
    /**
     * Function to send a exceptionencountered event and verify that it succeeds.  Parameters are passed through
     * to @c ExceptionEncounteredEvent::ExceptionEncounteredEvent().
     *
     * @return @c true if the exceptionencountered event sent correctly, else @c false.
     */
    bool testExceptionEncounteredSucceeds(
        const std::string& unparsedDirective,
        avs::ExceptionErrorType error,
        const std::string& errorDescription);

    /// The mock @c MessageSenderInterface.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockMessageSender> m_mockMessageSender;

    /// The @c ExceptionEncounteredSender to test
    std::shared_ptr<avs::ExceptionEncounteredSender> m_exceptionEncounteredSender;

    /// The @c ExceptionEncounteredEvent from the @c testExceptionEncounteredSucceeds() call
    std::shared_ptr<ExceptionEncounteredEvent> m_exceptionEncounteredEvent;
};

void ExceptionEncounteredSenderTest::SetUp() {
    m_mockMessageSender = std::make_shared<avsCommon::sdkInterfaces::test::MockMessageSender>();
    m_exceptionEncounteredSender = avs::ExceptionEncounteredSender::create(m_mockMessageSender);
    EXPECT_NE(m_exceptionEncounteredSender, nullptr);
}

bool ExceptionEncounteredSenderTest::testExceptionEncounteredSucceeds(
    const std::string& unparsedDirective,
    avs::ExceptionErrorType error,
    const std::string& errorDescription) {
    bool done = false;
    m_exceptionEncounteredEvent =
        std::make_shared<ExceptionEncounteredEvent>(unparsedDirective, error, errorDescription);

    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .WillOnce(Invoke(m_exceptionEncounteredEvent.get(), &ExceptionEncounteredEvent::verifyMessage));
    m_exceptionEncounteredEvent->send(m_exceptionEncounteredSender);
    done = true;
    return done;
}

/*
 * This function sends @c ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED and verifies that
 * @c ExceptionEncounteredSender::sendExceptionEncountered send the event.
 */
TEST_F(ExceptionEncounteredSenderTest, errorTypeUnexpectedInformationReceived) {
    ASSERT_TRUE(testExceptionEncounteredSucceeds(
        UNPARSED_DIRECTIVE_JSON_STRING,
        avs::ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED,
        "The directive sent was malformed"));
}
/*
 * This function sends @c ExceptionErrorType::UNSUPPORTED_OPERATION and verifies that
 * @c ExceptionEncounteredSender::sendExceptionEncountered send the event.
 */
TEST_F(ExceptionEncounteredSenderTest, errorTypeUnexpectedOperation) {
    ASSERT_TRUE(testExceptionEncounteredSucceeds(
        UNPARSED_DIRECTIVE_JSON_STRING, avs::ExceptionErrorType::UNSUPPORTED_OPERATION, "Operation not supported"));
}
/*
 * This function sends @c ExceptionErrorType::INTERNAL_ERROR and verifies that
 * @c ExceptionEncounteredSender::sendExceptionEncountered send the event.
 */
TEST_F(ExceptionEncounteredSenderTest, errorTypeInternalError) {
    ASSERT_TRUE(testExceptionEncounteredSucceeds(
        UNPARSED_DIRECTIVE_JSON_STRING, avs::ExceptionErrorType::INTERNAL_ERROR, "An error occurred with the device"));
}
}  // namespace test
}  // namespace avsCommon
}  // namespace alexaClientSDK
