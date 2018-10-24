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

#include <string>
#include <array>
#include <tuple>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "AVSCommon/AVS/CapabilityAgent.h"
#include "AVSCommon/AVS/Attachment/AttachmentManager.h"
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>

using namespace testing;

namespace alexaClientSDK {
namespace avsCommon {
namespace test {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace rapidjson;

/// Namespace for SpeechRecognizer.
static const std::string NAMESPACE_SPEECH_RECOGNIZER("SpeechRecognizer");

/// Name for directive to SpeechRecognizer.
static const std::string NAME_STOP_CAPTURE("StopCapture");

/// Name for SpeechRecognizer state.
static const std::string NAME_RECOGNIZE("Recognize");

/// Event key.
static const std::string EVENT("event");

/// Header key.
static const std::string HEADER("header");

/// Message Id key.
static const std::string MESSAGE_ID("messageId");

/// Message Id for testing.
static const std::string MESSAGE_ID_TEST("MessageId_Test");

/// Dialog request Id Key.
static const std::string DIALOG_REQUEST_ID("dialogRequestId");

/// DialogRequestId for testing.
static const std::string DIALOG_REQUEST_ID_TEST("DialogRequestId_Test");

/// Payload key
static const std::string PAYLOAD("payload");

/// A speech recognizer payload for testing
static const std::string PAYLOAD_TEST("payload_Test");

/// A string to send with the sendExceptionEncounteredAndReportFailed method
static const std::string EXCEPTION_ENCOUNTERED_STRING("encountered_exception");

/// A payload for testing
// clang-format off
static const std::string PAYLOAD_SPEECH_RECOGNIZER =
        "{"
            "\"profile\":\"CLOSE_TALK\","
            "\"format\":\"AUDIO_L16_RATE_16000_CHANNELS_1\""
        "}";
// clang-format on

/// A context for testing
// clang-format off
static const std::string CONTEXT_TEST =
        "{"
            "\"context\":["
                "{"
                    "\"header\":{"
                        "\"namespace\":\"SpeechSynthesizer\","
                        "\"name\":\"SpeechState\""
                    "},"
                    "\"payload\":{"
                        "\"playerActivity\":\"FINISHED\","
                        "\"offsetInMilliseconds\":0,"
                        "\"token\":\"\""
                    "}"
                "}"
            "]"
        "}";
// clang-format on

/**
 * TestEvents for testing. Tuple contains test event string to compare the result string with, the dialogRequestID and
 * the context to be passed as arguments.
 */
// clang-format off
static const std::tuple<std::string, std::string, std::string> testEventWithDialogReqIdAndContext = {
    std::make_tuple(
        /// Event with context and dialog request id.
        "{"
            "\"context\":["
                "{"
                    "\"header\":{"
                        "\"namespace\":\"SpeechSynthesizer\","
                        "\"name\":\"SpeechState\""
                    "},"
                    "\"payload\":{"
                        "\"playerActivity\":\"FINISHED\","
                        "\"offsetInMilliseconds\":0,"
                        "\"token\":\"\""
                    "}"
                "}"
            "],"
            "\"event\":{"
                "\"header\":{"
                        "\"namespace\":\"SpeechRecognizer\","
                        "\"name\":\"Recognize\","
                        "\"messageId\":\""+MESSAGE_ID_TEST+"\","
                        "\"dialogRequestId\":\""+DIALOG_REQUEST_ID_TEST+"\""
                "},"
                "\"payload\":{"
                    "\"profile\":\"CLOSE_TALK\","
                    "\"format\":\"AUDIO_L16_RATE_16000_CHANNELS_1\""
                "}"
            "}"
        "}", DIALOG_REQUEST_ID_TEST, CONTEXT_TEST)};
// clang-format on

// clang-format off
static const std::tuple<std::string, std::string, std::string> testEventWithDialogReqIdNoContext = {
    std::make_tuple(
        /// An event with no context.
        "{"
            "\"event\":{"
                "\"header\":{"
                        "\"namespace\":\"SpeechRecognizer\","
                        "\"name\":\"Recognize\","
                        "\"messageId\":\""+MESSAGE_ID_TEST+"\","
                        "\"dialogRequestId\":\""+DIALOG_REQUEST_ID_TEST+"\""
                "},"
                "\"payload\":{"
                    "\"profile\":\"CLOSE_TALK\","
                    "\"format\":\"AUDIO_L16_RATE_16000_CHANNELS_1\""
                "}"
            "}"
        "}",DIALOG_REQUEST_ID_TEST, "")};
// clang-format on

// clang-format off
static const std::tuple<std::string, std::string, std::string> testEventWithoutDialogReqIdOrContext = {
    std::make_tuple(
        /// An event with no dialog request id and no context for testing.
        "{"
            "\"event\":{"
                "\"header\":{"
                        "\"namespace\":\"SpeechRecognizer\","
                        "\"name\":\"Recognize\","
                        "\"messageId\":\""+MESSAGE_ID_TEST+"\""
                "},"
                "\"payload\":{"
                    "\"profile\":\"CLOSE_TALK\","
                    "\"format\":\"AUDIO_L16_RATE_16000_CHANNELS_1\""
                "}"
            "}"
        "}", "", "")};
// clang-format on

// clang-format off
static const std::tuple<std::string, std::string, std::string> testEventWithContextAndNoDialogReqId = {
    std::make_tuple(
        /// An event with no dialog request id for testing.
        "{"
            "\"context\":["
                "{"
                    "\"header\":{"
                        "\"namespace\":\"SpeechSynthesizer\","
                        "\"name\":\"SpeechState\""
                    "},"
                    "\"payload\":{"
                        "\"playerActivity\":\"FINISHED\","
                        "\"offsetInMilliseconds\":0,"
                        "\"token\":\"\""
                    "}"
                "}"
            "],"
            "\"event\":{"
                "\"header\":{"
                        "\"namespace\":\"SpeechRecognizer\","
                        "\"name\":\"Recognize\","
                        "\"messageId\":\""+MESSAGE_ID_TEST+"\""
                "},"
                "\"payload\":{"
                    "\"profile\":\"CLOSE_TALK\","
                    "\"format\":\"AUDIO_L16_RATE_16000_CHANNELS_1\""
                "}"
            "}"
        "}", "", CONTEXT_TEST)};
// clang-format on

/// Mock @c DirectiveHandlerResultInterface implementation.
class MockResult : public DirectiveHandlerResultInterface {
public:
    void setCompleted() override;

    void setFailed(const std::string& description) override;
};

void MockResult::setCompleted() {
    // default no-op
}

void MockResult::setFailed(const std::string& description) {
    // default no-op
}

class MockCapabilityAgent : public CapabilityAgent {
public:
    // Expand polymorphic matching in this scope to include these inherited methods.
    using DirectiveHandlerInterface::cancelDirective;
    using DirectiveHandlerInterface::handleDirective;
    using DirectiveHandlerInterface::preHandleDirective;

    /**
     * Creates an instance of the @c MockCapabilityAgent.
     *
     * @param nameSpace The namespace of the Capability Agent.
     * @param m_exceptionSender The @c ExceptionEncounteredSenderInterface instance.
     * @return A shared pointer to an instance of the @c MockCapabilityAgent.
     */
    static std::shared_ptr<MockCapabilityAgent> create(
        const std::string nameSpace,
        const std::shared_ptr<MockExceptionEncounteredSender> m_exceptionSender);

    /**
     * MockCapabilityAgent Constructor.
     *
     * @param nameSpace The namespace of the Capability Agent.
     * @param m_exceptionSender The @c ExceptionEncounteredSenderInterface instance.
     */
    MockCapabilityAgent(
        const std::string& nameSpace,
        const std::shared_ptr<MockExceptionEncounteredSender> m_exceptionSender);

    ~MockCapabilityAgent() override;

    enum class FunctionCalled {
        NONE,

        HANDLE_DIRECTIVE_IMMEDIATELY,

        PREHANDLE_DIRECTIVE,

        HANDLE_DIRECTIVE,

        CANCEL_DIRECTIVE
    };

    void handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) override;

    void preHandleDirective(std::shared_ptr<DirectiveInfo> info) override;

    void handleDirective(std::shared_ptr<DirectiveInfo> info) override;

    void cancelDirective(std::shared_ptr<DirectiveInfo> info) override;

    avs::DirectiveHandlerConfiguration getConfiguration() const override;

    /**
     * Wrapper function to test protected method @c sendExceptionEncounteredAndReportFailed
     *
     * @param directiveIn The AVSDirective to pass to @c sendExceptionEncounteredAndReportFailed.
     * @param resultIn The DirectiveHandlerResultInterface to pass to @c sendExceptionEncounteredAndReportFailed.
     */
    void testsendExceptionEncounteredAndReportFailed(
        std::shared_ptr<AVSDirective> directiveIn,
        std::unique_ptr<sdkInterfaces::DirectiveHandlerResultInterface> resultIn);

    FunctionCalled waitForFunctionCalls(const std::chrono::milliseconds duration = std::chrono::milliseconds(400));

    const std::pair<std::string, std::string> callBuildJsonEventString(
        const std::string& eventName,
        const std::string& dialogRequestIdValue,
        const std::string& jsonPayloadValue,
        const std::string& jsonContext);

private:
    /// flag to indicate which function has been called.
    FunctionCalled m_functionCalled;

    /// Condition variable to wake the @c waitForFunctionCalls.
    std::condition_variable m_wakeTrigger;

    /// mutex to protect @c m_contextAvailable.
    std::mutex m_mutex;
};

std::shared_ptr<MockCapabilityAgent> MockCapabilityAgent::create(
    const std::string nameSpace,
    const std::shared_ptr<MockExceptionEncounteredSender> m_exceptionSender) {
    return std::make_shared<MockCapabilityAgent>(nameSpace, m_exceptionSender);
}

MockCapabilityAgent::MockCapabilityAgent(
    const std::string& nameSpace,
    const std::shared_ptr<MockExceptionEncounteredSender> m_exceptionSender) :
        CapabilityAgent(nameSpace, m_exceptionSender),
        m_functionCalled{FunctionCalled::NONE} {
}

MockCapabilityAgent::~MockCapabilityAgent() {
}

void MockCapabilityAgent::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_functionCalled = FunctionCalled::HANDLE_DIRECTIVE_IMMEDIATELY;
    m_wakeTrigger.notify_one();
}

void MockCapabilityAgent::preHandleDirective(std::shared_ptr<DirectiveInfo>) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_functionCalled = FunctionCalled::PREHANDLE_DIRECTIVE;
    m_wakeTrigger.notify_one();
}

void MockCapabilityAgent::handleDirective(std::shared_ptr<DirectiveInfo>) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_functionCalled = FunctionCalled::HANDLE_DIRECTIVE;
    m_wakeTrigger.notify_one();
}

void MockCapabilityAgent::cancelDirective(std::shared_ptr<DirectiveInfo>) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_functionCalled = FunctionCalled::CANCEL_DIRECTIVE;
    m_wakeTrigger.notify_one();
}

avs::DirectiveHandlerConfiguration MockCapabilityAgent::getConfiguration() const {
    // Not using an empty initializer list here to account for a GCC 4.9.2 regression
    return avs::DirectiveHandlerConfiguration();
}

void MockCapabilityAgent::testsendExceptionEncounteredAndReportFailed(
    std::shared_ptr<AVSDirective> directiveIn,
    std::unique_ptr<sdkInterfaces::DirectiveHandlerResultInterface> resultIn) {
    std::shared_ptr<CapabilityAgent::DirectiveInfo> m_directiveInfo =
        CapabilityAgent::createDirectiveInfo(directiveIn, std::move(resultIn));
    sendExceptionEncounteredAndReportFailed(
        m_directiveInfo, EXCEPTION_ENCOUNTERED_STRING, ExceptionErrorType::INTERNAL_ERROR);
}

MockCapabilityAgent::FunctionCalled MockCapabilityAgent::waitForFunctionCalls(
    const std::chrono::milliseconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakeTrigger.wait_for(lock, duration, [this]() { return (m_functionCalled != FunctionCalled::NONE); })) {
        return FunctionCalled::NONE;
    }
    return m_functionCalled;
}

const std::pair<std::string, std::string> MockCapabilityAgent::callBuildJsonEventString(
    const std::string& eventName,
    const std::string& dialogRequestIdValue,
    const std::string& jsonPayloadValue,
    const std::string& jsonContext) {
    return CapabilityAgent::buildJsonEventString(eventName, dialogRequestIdValue, jsonPayloadValue, jsonContext);
}

class CapabilityAgentTest : public ::testing::Test {
public:
    /**
     * Tests the @c buildJsonEventString functionality. Calls @c buildJsonEventString and compares it to the test
     * string. Assert if the strings are not equal.
     *
     * @param testTuple A tuple of test string, dialogRequestId and context.
     * @param dialogRequestIdPresent Whether a dialogRequestId is expected or not. This helps decide which parts of the
     * string to compare.
     */
    void testBuildJsonEventString(
        std::tuple<std::string, std::string, std::string> testTuple,
        bool dialogRequestIdPresent);

    void SetUp() override;

    std::shared_ptr<MockCapabilityAgent> m_capabilityAgent;

    std::shared_ptr<AttachmentManager> m_attachmentManager;

    // mockExceptionSender
    std::shared_ptr<MockExceptionEncounteredSender> m_exceptionSender;
};

void CapabilityAgentTest::SetUp() {
    m_exceptionSender = std::make_shared<NiceMock<MockExceptionEncounteredSender>>();
    m_capabilityAgent = MockCapabilityAgent::create(NAMESPACE_SPEECH_RECOGNIZER, m_exceptionSender);
    m_attachmentManager = std::make_shared<AttachmentManager>(AttachmentManager::AttachmentType::IN_PROCESS);
}

/**
 * Helper function to remove the messageId.
 *
 * @param document The document from which to remove the messageId.
 * @param messageId The messageId that was removed (if successful).
 * @return bool Indicates whether removing the messageId was successful.
 */
bool removeMessageId(Document* document, std::string* messageId) {
    if (!document || !messageId) {
        return false;
    }

    auto it = document->FindMember(EVENT.c_str());
    if (it == document->MemberEnd()) return false;

    it = it->value.FindMember(HEADER.c_str());
    if (it == document->MemberEnd()) return false;
    auto& eventHeader = it->value;

    it = it->value.FindMember(MESSAGE_ID.c_str());
    if (it == document->MemberEnd()) return false;

    *messageId = it->value.GetString();
    eventHeader.RemoveMember(it);

    return true;
}

void CapabilityAgentTest::testBuildJsonEventString(
    std::tuple<std::string, std::string, std::string> testTuple,
    bool dialogRequestIdPresent) {
    std::string testString = std::get<0>(testTuple);
    std::pair<std::string, std::string> msgIdAndJsonEvent = m_capabilityAgent->callBuildJsonEventString(
        NAME_RECOGNIZE, std::get<1>(testTuple), PAYLOAD_SPEECH_RECOGNIZER, std::get<2>(testTuple));
    std::string& jsonEventString = msgIdAndJsonEvent.second;

    Document expected, actual;
    expected.Parse(testString);
    actual.Parse(jsonEventString);

    // messageId is randomly generated. Remove before comparing the Event strings.

    std::string expectedMsgId;
    ASSERT_TRUE(removeMessageId(&expected, &expectedMsgId));

    std::string actualMsgId;
    ASSERT_TRUE(removeMessageId(&actual, &actualMsgId));

    // Check that messageId in the output pair is equal to the messageId in the body.
    ASSERT_EQ(actualMsgId, msgIdAndJsonEvent.first);

    ASSERT_EQ(expected, actual);
}

/**
 * Call the @c handleDirectiveImmediately from the @c CapabilityAgent base class with a directive as the argument.
 * Expect the @c handleDirectiveImmediately with the argument of @c DirectiveAndResultInterface will be called.
 */
TEST_F(CapabilityAgentTest, testCallToHandleImmediately) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_RECOGNIZER, NAME_STOP_CAPTURE, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, "");
    m_capabilityAgent->handleDirectiveImmediately(directive);
    ASSERT_EQ(
        MockCapabilityAgent::FunctionCalled::HANDLE_DIRECTIVE_IMMEDIATELY, m_capabilityAgent->waitForFunctionCalls());
}

/**
 * Call the @c preHandleDirective from the @c CapabilityAgent base class with a directive as the argument.
 * Expect the @c preHandleDirective with the argument of @c DirectiveAndResultInterface will be called.
 */
TEST_F(CapabilityAgentTest, testCallToPrehandleDirective) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_RECOGNIZER, NAME_STOP_CAPTURE, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, "");
    std::unique_ptr<MockResult> dirHandlerResult(new MockResult);
    m_capabilityAgent->preHandleDirective(directive, std::move(dirHandlerResult));
    ASSERT_EQ(MockCapabilityAgent::FunctionCalled::PREHANDLE_DIRECTIVE, m_capabilityAgent->waitForFunctionCalls());
}

/**
 * Call the @c preHandleDirective from the @c CapabilityAgent base class with a directive. *
 * Call the @c handleDirective from the @c CapabilityAgent base class with a directive as the argument.
 * Expect the @c handleDirective with the argument of @c DirectiveAndResultInterface will be called.
 */
TEST_F(CapabilityAgentTest, testCallToHandleDirective) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_RECOGNIZER, NAME_STOP_CAPTURE, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, "");
    std::unique_ptr<MockResult> dirHandlerResult(new MockResult);
    m_capabilityAgent->preHandleDirective(directive, std::move(dirHandlerResult));
    ASSERT_EQ(MockCapabilityAgent::FunctionCalled::PREHANDLE_DIRECTIVE, m_capabilityAgent->waitForFunctionCalls());
    m_capabilityAgent->handleDirective(MESSAGE_ID_TEST);
    ASSERT_EQ(MockCapabilityAgent::FunctionCalled::HANDLE_DIRECTIVE, m_capabilityAgent->waitForFunctionCalls());
}

/**
 * Call the @c handleDirective from the @c CapabilityAgent base class with a directive as the argument. No
 * @c preHandleDirective is called before handleDirective. Expect @c handleDirective to return @c false.
 */
TEST_F(CapabilityAgentTest, testCallToHandleDirectiveWithNoPrehandle) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_RECOGNIZER, NAME_STOP_CAPTURE, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, "");
    ASSERT_FALSE(m_capabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST));
}

/**
 * Call the @c cancelDirective from the @c CapabilityAgent base class with a directive as the argument.
 * Expect the @c cancelDirective with the argument of @c DirectiveAndResultInterface will be called.
 */
TEST_F(CapabilityAgentTest, testCallToCancelDirective) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_RECOGNIZER, NAME_STOP_CAPTURE, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, "");
    std::unique_ptr<MockResult> dirHandlerResult(new MockResult);
    m_capabilityAgent->preHandleDirective(directive, std::move(dirHandlerResult));
    ASSERT_EQ(MockCapabilityAgent::FunctionCalled::PREHANDLE_DIRECTIVE, m_capabilityAgent->waitForFunctionCalls());
    m_capabilityAgent->cancelDirective(MESSAGE_ID_TEST);
    ASSERT_EQ(MockCapabilityAgent::FunctionCalled::CANCEL_DIRECTIVE, m_capabilityAgent->waitForFunctionCalls());
}

/**
 * Call the @c cancelDirective from the @c CapabilityAgent base class with a directive as the argument. No
 * @c preHandleDirective is called before handleDirective. Expect the @c cancelDirective with the argument of
 * @c DirectiveAndResultInterface will not be called.
 */
TEST_F(CapabilityAgentTest, testCallToCancelDirectiveWithNoPrehandle) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_RECOGNIZER, NAME_STOP_CAPTURE, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, "");
    m_capabilityAgent->CapabilityAgent::cancelDirective(MESSAGE_ID_TEST);
    ASSERT_EQ(MockCapabilityAgent::FunctionCalled::NONE, m_capabilityAgent->waitForFunctionCalls());
}

/**
 * Call the @c callBuildJsonEventString with dialogRequestID and context. Expect a json event string that matches the
 * corresponding @c testEvent. The messageId will not match since it is a random number. Verify the string before and
 * after the messageId.
 */
TEST_F(CapabilityAgentTest, testWithDialogIdAndContext) {
    testBuildJsonEventString(testEventWithDialogReqIdAndContext, true);
}

/**
 * Call the @c callBuildJsonEventString with dialogRequestId and without context. Expect a json event string that
 * matches the corresponding @c testEvent. The messageId will not match since it is a random number. Verify the string
 * before and after the messageId.
 */
TEST_F(CapabilityAgentTest, testWithDialogIdAndNoContext) {
    testBuildJsonEventString(testEventWithDialogReqIdNoContext, true);
}

/**
 * Call the @c callBuildJsonEventString  without context and without dialogRequestId. Expect a json event string
 * that matches the corresponding @c testEvent. The messageId will not match since it is a random number.
 * Verify the string before and after the messageId.
 */
TEST_F(CapabilityAgentTest, testWithoutDialogIdOrContext) {
    testBuildJsonEventString(testEventWithoutDialogReqIdOrContext, false);
}

/**
 * Call the @c callBuildJsonEventString multiple times with context and without dialogRequestId. Expect a json event
 * string that matches the corresponding @c testEvent. The messageId will not match since it is a random number.
 * Verify the string before and after the messageId.
 */
TEST_F(CapabilityAgentTest, testWithContextAndNoDialogId) {
    testBuildJsonEventString(testEventWithContextAndNoDialogReqId, false);
}

/**
 * Call sendExceptionEncounteredAndReportFailed with info pointing to null directive. Expect sendExceptionEncountered to
 * not be called. Send again with info pointing to a valid directive. Expect sendExceptionEncountered to be called
 */
TEST_F(CapabilityAgentTest, testsendExceptionEncounteredWithNullInfo) {
    EXPECT_CALL(*(m_exceptionSender.get()), sendExceptionEncountered(_, _, _)).Times(0);
    m_capabilityAgent->testsendExceptionEncounteredAndReportFailed(nullptr, nullptr);

    EXPECT_CALL(*(m_exceptionSender.get()), sendExceptionEncountered(_, _, _)).Times(1);
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_RECOGNIZER, NAME_STOP_CAPTURE, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, "");
    m_capabilityAgent->testsendExceptionEncounteredAndReportFailed(directive, nullptr);
}

}  // namespace test
}  // namespace avsCommon
}  // namespace alexaClientSDK
