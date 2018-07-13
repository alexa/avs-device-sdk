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

// @file DirectiveSequencerTest.cpp

#include <chrono>
#include <future>
#include <string>
#include <memory>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>

#include "ADSL/DirectiveSequencer.h"
#include "MockDirectiveHandler.h"

using namespace ::testing;

namespace alexaClientSDK {
namespace adsl {
namespace test {

using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;

/// Long amount of time for handling a directive to allow other things to happen (we should not reach this).
static const std::chrono::milliseconds LONG_HANDLING_TIME_MS(30000);

/// Namespace for Test only directives.
static const std::string NAMESPACE_TEST("Test");

/// Namespace for Speaker directives.
static const std::string NAMESPACE_SPEAKER("Speaker");

/// Namespace for SpeechSynthesizer directives.
static const std::string NAMESPACE_SPEECH_SYNTHESIZER("SpeechSynthesizer");

/// Namespace for AudioPlayer directives.
static const std::string NAMESPACE_AUDIO_PLAYER("AudioPlayer");

/// Name for Test directive used to terminate tests.
static const std::string NAME_DONE("Done");

/// Name for Speaker::setVolume Directives.
static const std::string NAME_SET_VOLUME("SetVolume");

/// Name for SpeechSynthesizer::Speak directives.
static const std::string NAME_SPEAK("Speak");

/// Name for AudioPlayer::Play directives.
static const std::string NAME_PLAY("Play");

/// Name for Test::Blocking directives
static const std::string NAME_BLOCKING("Blocking");

/// Name for Test::Non-Blocking directives.
static const std::string NAME_NON_BLOCKING("Non-Blocking");

/// Name for Test::Handle-Immediately directives.
static const std::string NAME_HANDLE_IMMEDIATELY("Handle-Immediately");

/// MessageId for Testing:Done directives used to terminate tests.
static const std::string MESSAGE_ID_DONE("Message_Done");

/// Generic messageId used for tests.
static const std::string MESSAGE_ID_0("Message_0");

/// Generic MessageId used for test.
static const std::string MESSAGE_ID_1("Message_1");

/// Generic MessageId used for tests.
static const std::string MESSAGE_ID_2("Message_2");

/// Default DialogRequestId for directives used to terminate tests.
static const std::string DIALOG_REQUEST_ID_DONE("DialogRequestId_Done");

/// Generic DialogRequestId used for tests.
static const std::string DIALOG_REQUEST_ID_0("DialogRequestId_0");

/// Generic DialogRequestId used for tests.
static const std::string DIALOG_REQUEST_ID_1("DialogRequestId_1");

/// An unparsed directive for test.
static const std::string UNPARSED_DIRECTIVE("unparsedDirectiveForTest");

/// A paylaod for test;
static const std::string PAYLOAD_TEST("payloadForTest");

/// Generic DialogRequestId used for tests.
static const std::string DIALOG_REQUEST_ID_2("DialogRequestId_2");

static const std::string TEST_ATTACHMENT_CONTEXT_ID("TEST_ATTACHMENT_CONTEXT_ID");

/**
 * Mock ExceptionEncounteredSenderInterface implementation.
 */
class MockExceptionEncounteredSender : public avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface {
public:
    MOCK_METHOD3(sendExceptionEncountered, void(const std::string&, ExceptionErrorType, const std::string&));
};

/// DirectiveSequencerTest
class DirectiveSequencerTest : public ::testing::Test {
public:
    /**
     * Setup the DirectiveSequencerTest.  Creates a DirectiveSequencer and initialized it with a DirectiveHandler
     * that allows waiting for a final directive to be processed.
     */
    void SetUp() override;

    /**
     * TearDown the DirectiveSequencerTest.  Sends a Test.Done directive with the current dialogRequestId and
     * waits for it to be handled before allowing the test to complete.
     */
    void TearDown() override;

    /**
     * SetDialogRequestId().  Tests should use this method rather than m_sequencer->setDialogRequestId() so
     * that @c TearDown() knows which @c DialogRequestId to use for the Test::Done directive.
     * @param dialogRequestId The new value for DialogRequestId.
     */
    void setDialogRequestId(std::string dialogRequestId);

    /// Handler to invoke for the Test::Done directive.
    std::shared_ptr<MockDirectiveHandler> m_doneHandler;

    /// The DirectiveSequencer to test.
    std::shared_ptr<MockExceptionEncounteredSender> m_exceptionEncounteredSender;

    /// The DirectiveSequencer to test.
    std::unique_ptr<DirectiveSequencerInterface> m_sequencer;

    /// AttachmentManager with which to create directives.
    std::shared_ptr<AttachmentManager> m_attachmentManager;
};

void DirectiveSequencerTest::SetUp() {
    DirectiveHandlerConfiguration config;
    config[{NAMESPACE_TEST, NAME_DONE}] = BlockingPolicy::BLOCKING;
    m_doneHandler = MockDirectiveHandler::create(config, LONG_HANDLING_TIME_MS);
    m_attachmentManager = std::make_shared<AttachmentManager>(AttachmentManager::AttachmentType::IN_PROCESS);
    m_exceptionEncounteredSender = std::make_shared<NiceMock<MockExceptionEncounteredSender>>();
    m_sequencer = DirectiveSequencer::create(m_exceptionEncounteredSender);
    ASSERT_TRUE(m_sequencer);
    ASSERT_TRUE(m_sequencer->addDirectiveHandler(m_doneHandler));
}

void DirectiveSequencerTest::TearDown() {
    auto avsMessageHeader =
        std::make_shared<AVSMessageHeader>(NAMESPACE_TEST, NAME_DONE, MESSAGE_ID_DONE, DIALOG_REQUEST_ID_DONE);
    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);

    EXPECT_CALL(*(m_doneHandler.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(m_doneHandler.get()), preHandleDirective(directive, _)).Times(1);
    EXPECT_CALL(*(m_doneHandler.get()), handleDirective(_)).Times(1);
    EXPECT_CALL(*(m_doneHandler.get()), cancelDirective(_)).Times(0);

    m_sequencer->setDialogRequestId(DIALOG_REQUEST_ID_DONE);
    m_sequencer->onDirective(directive);
    m_doneHandler->waitUntilHandling();
    ASSERT_TRUE(m_sequencer->removeDirectiveHandler(m_doneHandler));
    m_sequencer->shutdown();
    m_sequencer.reset();
    m_doneHandler->doHandlingCompleted();
}

/**
 * Test DirectiveSequencer::create() with a nullptr @c ExceptionEncounteredSender.  Expect create to fail.
 */
TEST_F(DirectiveSequencerTest, testNullptrExceptionSender) {
    ASSERT_TRUE(m_sequencer);
    auto sequencer = DirectiveSequencer::create(nullptr);
    ASSERT_FALSE(sequencer);
}

/**
 * Verify core DirectiveSequencerTest.  Expect a new non-null instance of m_sequencer.
 */
TEST_F(DirectiveSequencerTest, testCreateAndDoneTrigger) {
    ASSERT_TRUE(m_sequencer);
}

/**
 * Exercise sending a @c nullptr to @c onDirective.  Expect that false is returned.
 */
TEST_F(DirectiveSequencerTest, testNullptrDirective) {
    ASSERT_FALSE(m_sequencer->onDirective(nullptr));
}

/**
 * Exercise sending a @c AVSDirective for which no handler has been registered.  Expect that
 * m_exceptionEncounteredSender will receive a request to send the ExceptionEncountered message.
 */
TEST_F(DirectiveSequencerTest, testUnhandledDirective) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_SPEAKER, NAME_SET_VOLUME, MESSAGE_ID_0);
    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);
    EXPECT_CALL(*(m_exceptionEncounteredSender.get()), sendExceptionEncountered(_, _, _)).Times(1);
    m_sequencer->onDirective(directive);
}

/**
 * Send a directive with an empty DialogRequestId.
 * Expect a call to handleDirectiveImmediately().
 */
TEST_F(DirectiveSequencerTest, testEmptyDialogRequestId) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_SPEAKER, NAME_SET_VOLUME, MESSAGE_ID_0);
    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);
    DirectiveHandlerConfiguration config;
    config[{NAMESPACE_SPEAKER, NAME_SET_VOLUME}] = BlockingPolicy::NON_BLOCKING;
    auto handler = MockDirectiveHandler::create(config);
    EXPECT_CALL(*(handler.get()), handleDirectiveImmediately(directive)).Times(0);
    EXPECT_CALL(*(handler.get()), preHandleDirective(_, _)).Times(1);
    EXPECT_CALL(*(handler.get()), handleDirective(_)).Times(1);
    EXPECT_CALL(*(handler.get()), cancelDirective(_)).Times(0);
    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler));
    m_sequencer->onDirective(directive);
    ASSERT_TRUE(handler->waitUntilHandling());
}

/**
 * Send a directive with a DialogRequestId but with HANDLE_IMMEDIATELY policy in its handlier.
 * Expect a call to handleDirectiveImmediately().
 */
TEST_F(DirectiveSequencerTest, testHandleImmediatelyHandler) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_TEST, NAME_HANDLE_IMMEDIATELY, MESSAGE_ID_0);
    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);
    DirectiveHandlerConfiguration config;
    config[{NAMESPACE_TEST, NAME_HANDLE_IMMEDIATELY}] = BlockingPolicy::HANDLE_IMMEDIATELY;
    auto handler = MockDirectiveHandler::create(config);
    EXPECT_CALL(*(handler.get()), handleDirectiveImmediately(directive)).Times(1);
    EXPECT_CALL(*(handler.get()), preHandleDirective(_, _)).Times(0);
    EXPECT_CALL(*(handler.get()), handleDirective(_)).Times(0);
    EXPECT_CALL(*(handler.get()), cancelDirective(_)).Times(0);
    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler));
    m_sequencer->onDirective(directive);
    ASSERT_TRUE(handler->waitUntilHandling());
}

/**
 * Set handlers (NON_BLOCKING) for two namespace,name pairs.  Then remove one and change the other.  Send directives
 * for each of the NamespaceAndName values.  Expect that the directive with no mapping is not seen by a handler and
 * that the one that still has a handler is handled.
 */
TEST_F(DirectiveSequencerTest, testRemovingAndChangingHandlers) {
    auto avsMessageHeader0 = std::make_shared<AVSMessageHeader>(NAMESPACE_SPEAKER, NAME_SET_VOLUME, MESSAGE_ID_0);
    std::shared_ptr<AVSDirective> directive0 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader0, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);
    auto avsMessageHeader1 = std::make_shared<AVSMessageHeader>(NAMESPACE_TEST, NAME_NON_BLOCKING, MESSAGE_ID_1);
    std::shared_ptr<AVSDirective> directive1 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader1, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);

    DirectiveHandlerConfiguration handler0Config;
    handler0Config[{NAMESPACE_SPEAKER, NAME_SET_VOLUME}] = BlockingPolicy::NON_BLOCKING;
    auto handler0 = MockDirectiveHandler::create(handler0Config);

    DirectiveHandlerConfiguration handler1Config;
    handler1Config[{NAMESPACE_TEST, NAME_NON_BLOCKING}] = BlockingPolicy::NON_BLOCKING;
    auto handler1 = MockDirectiveHandler::create(handler1Config);

    DirectiveHandlerConfiguration handler2Config;
    handler2Config[{NAMESPACE_TEST, NAME_NON_BLOCKING}] = BlockingPolicy::NON_BLOCKING;
    auto handler2 = MockDirectiveHandler::create(handler2Config);

    EXPECT_CALL(*(handler0.get()), handleDirectiveImmediately(directive1)).Times(0);
    EXPECT_CALL(*(handler0.get()), preHandleDirective(_, _)).Times(0);
    EXPECT_CALL(*(handler0.get()), handleDirective(_)).Times(0);
    EXPECT_CALL(*(handler0.get()), cancelDirective(_)).Times(0);

    EXPECT_CALL(*(handler1.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler1.get()), preHandleDirective(_, _)).Times(0);
    EXPECT_CALL(*(handler1.get()), handleDirective(_)).Times(0);
    EXPECT_CALL(*(handler1.get()), cancelDirective(_)).Times(0);

    EXPECT_CALL(*(handler2.get()), preHandleDirective(directive1, _)).Times(1);
    EXPECT_CALL(*(handler2.get()), handleDirective(MESSAGE_ID_1)).Times(1);

    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler0));
    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler1));

    ASSERT_TRUE(m_sequencer->removeDirectiveHandler(handler0));
    ASSERT_TRUE(m_sequencer->removeDirectiveHandler(handler1));

    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler2));

    m_sequencer->onDirective(directive0);
    m_sequencer->onDirective(directive1);
    ASSERT_TRUE(handler2->waitUntilHandling());
}

/**
 * Send a long running directive with an non-empty @c DialogRequestId and a BLOCKING policy. Expect a call to
 * @c preHandleDirective() and a call to @c handleDirective().  The @c AVSDirective is the cancelled, triggering
 * a call to cancelDirective() to close out the test.
 */
TEST_F(DirectiveSequencerTest, testBlockingDirective) {
    auto avsMessageHeader =
        std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_0, DIALOG_REQUEST_ID_0);
    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);

    DirectiveHandlerConfiguration handlerConfig;
    handlerConfig[{NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK}] = BlockingPolicy::BLOCKING;
    auto handler = MockDirectiveHandler::create(handlerConfig, LONG_HANDLING_TIME_MS);

    EXPECT_CALL(*(handler.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler.get()), preHandleDirective(directive, _)).Times(1);
    EXPECT_CALL(*(handler.get()), handleDirective(MESSAGE_ID_0)).Times(1);
    EXPECT_CALL(*(handler.get()), cancelDirective(_)).Times(1);

    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler));
    m_sequencer->setDialogRequestId(DIALOG_REQUEST_ID_0);
    m_sequencer->onDirective(directive);
    ASSERT_TRUE(handler->waitUntilHandling());
    m_sequencer->setDialogRequestId(DIALOG_REQUEST_ID_1);
    ASSERT_TRUE(handler->waitUntilCanceling());
}

/**
 * Send a long running directive with an non-empty @c DialogRequestId and a BLOCKING policy.
 */
TEST_F(DirectiveSequencerTest, testBlockingThenNonDialogDirective) {
    auto avsMessageHeader0 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_0, DIALOG_REQUEST_ID_0);
    std::shared_ptr<AVSDirective> directive0 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader0, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);

    auto avsMessageHeader1 = std::make_shared<AVSMessageHeader>(NAMESPACE_SPEAKER, NAME_SET_VOLUME, MESSAGE_ID_1);
    std::shared_ptr<AVSDirective> directive1 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader1, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);

    DirectiveHandlerConfiguration handler0Config;
    handler0Config[{NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK}] = BlockingPolicy::BLOCKING;
    auto handler0 = MockDirectiveHandler::create(handler0Config, LONG_HANDLING_TIME_MS);

    DirectiveHandlerConfiguration handler1Config;
    handler1Config[{NAMESPACE_SPEAKER, NAME_SET_VOLUME}] = BlockingPolicy::NON_BLOCKING;
    auto handler1 = MockDirectiveHandler::create(handler1Config);

    EXPECT_CALL(*(handler0.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler0.get()), preHandleDirective(directive0, _)).Times(1);
    EXPECT_CALL(*(handler0.get()), handleDirective(MESSAGE_ID_0)).Times(1);
    EXPECT_CALL(*(handler0.get()), cancelDirective(_)).Times(1);

    EXPECT_CALL(*(handler1.get()), handleDirectiveImmediately(directive1)).Times(0);
    EXPECT_CALL(*(handler1.get()), preHandleDirective(_, _)).Times(1);
    EXPECT_CALL(*(handler1.get()), handleDirective(_)).Times(1);
    EXPECT_CALL(*(handler1.get()), cancelDirective(_)).Times(0);

    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler0));
    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler1));

    m_sequencer->setDialogRequestId(DIALOG_REQUEST_ID_0);
    m_sequencer->onDirective(directive0);
    m_sequencer->onDirective(directive1);
    ASSERT_TRUE(handler1->waitUntilPreHandling());
    ASSERT_TRUE(handler0->waitUntilHandling());
    m_sequencer->setDialogRequestId(DIALOG_REQUEST_ID_1);
    ASSERT_TRUE(handler0->waitUntilCanceling());
}

/**
 * Send a directive with an non-empty @c DialogRequestId and a @c BLOCKING policy, to an @c DirectiveHandler with a
 * long handling time.  Once the @c DirectiveHandler is handling the @c AVSDirective, set the @c DirectiveSequencer's
 * @c DialogRequestId (simulating an outgoing @c SpeechRecognizer request). Expect a call to
 * @c preHandleDirective(@cAVSDirective) a call to @c handleDirective(@c MessageId, @c DirectiveHandlingResult),
 * and a call to @c cancelDirective(@c MessageId).
 */
TEST_F(DirectiveSequencerTest, testBargeIn) {
    auto avsMessageHeader =
        std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_0, DIALOG_REQUEST_ID_0);
    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);

    DirectiveHandlerConfiguration handlerConfig;
    handlerConfig[{NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK}] = BlockingPolicy::BLOCKING;
    auto handler = MockDirectiveHandler::create(handlerConfig, std::chrono::milliseconds(LONG_HANDLING_TIME_MS));

    EXPECT_CALL(*(handler.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler.get()), preHandleDirective(directive, _)).Times(1);
    EXPECT_CALL(*(handler.get()), handleDirective(MESSAGE_ID_0)).Times(1);
    EXPECT_CALL(*(handler.get()), cancelDirective(MESSAGE_ID_0)).Times(1);

    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler));
    m_sequencer->setDialogRequestId(DIALOG_REQUEST_ID_0);
    m_sequencer->onDirective(directive);
    ASSERT_TRUE(handler->waitUntilHandling());
    m_sequencer->setDialogRequestId(DIALOG_REQUEST_ID_1);
    ASSERT_TRUE(handler->waitUntilCanceling());
}

/**
 * Send an @c AVSDirective with an non-empty @c DialogRequestId and a @c BLOCKING policy followed by two
 * @c NON_BLOCKING @c AVSDirectives with the same @c DialogRequestId. Expect a call to
 * @c preHandleDirective(@c AVSDirective) and a call to @c handleDirective() for each @c AVSDirective.
 * Along the way we set the DialogRequestId to the same value to verify that that setting it to the
 * current value does not cancel queued directives.
 */
TEST_F(DirectiveSequencerTest, testBlockingThenNonBockingOnSameDialogId) {
    auto avsMessageHeader0 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_0, DIALOG_REQUEST_ID_0);
    std::shared_ptr<AVSDirective> directive0 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader0, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);
    auto avsMessageHeader1 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_1, DIALOG_REQUEST_ID_0);
    std::shared_ptr<AVSDirective> directive1 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader1, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);
    auto avsMessageHeader2 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_TEST, NAME_NON_BLOCKING, MESSAGE_ID_2, DIALOG_REQUEST_ID_0);
    std::shared_ptr<AVSDirective> directive2 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader2, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);

    DirectiveHandlerConfiguration handler0Config;
    handler0Config[{NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK}] = BlockingPolicy::BLOCKING;
    auto handler0 = MockDirectiveHandler::create(handler0Config);

    DirectiveHandlerConfiguration handler1Config;
    handler1Config[{NAMESPACE_AUDIO_PLAYER, NAME_PLAY}] = BlockingPolicy::NON_BLOCKING;
    auto handler1 = MockDirectiveHandler::create(handler1Config);

    DirectiveHandlerConfiguration handler2Config;
    handler2Config[{NAMESPACE_TEST, NAME_NON_BLOCKING}] = BlockingPolicy::NON_BLOCKING;
    auto handler2 = MockDirectiveHandler::create(handler2Config);

    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler0));
    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler1));
    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler2));

    EXPECT_CALL(*(handler0.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler0.get()), preHandleDirective(directive0, _)).Times(1);
    EXPECT_CALL(*(handler0.get()), handleDirective(MESSAGE_ID_0)).Times(1);
    EXPECT_CALL(*(handler0.get()), cancelDirective(_)).Times(0);

    EXPECT_CALL(*(handler1.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler1.get()), preHandleDirective(directive1, _)).Times(1);
    EXPECT_CALL(*(handler1.get()), handleDirective(MESSAGE_ID_1)).Times(1);
    EXPECT_CALL(*(handler1.get()), cancelDirective(_)).Times(0);

    EXPECT_CALL(*(handler2.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler2.get()), preHandleDirective(directive2, _)).Times(1);
    EXPECT_CALL(*(handler2.get()), handleDirective(MESSAGE_ID_2)).Times(1);
    EXPECT_CALL(*(handler2.get()), cancelDirective(_)).Times(0);

    m_sequencer->setDialogRequestId(DIALOG_REQUEST_ID_0);
    m_sequencer->onDirective(directive0);
    m_sequencer->onDirective(directive1);
    m_sequencer->setDialogRequestId(DIALOG_REQUEST_ID_0);
    m_sequencer->onDirective(directive2);
    m_sequencer->setDialogRequestId(DIALOG_REQUEST_ID_0);
    ASSERT_TRUE(handler1->waitUntilCompleted());
    ASSERT_TRUE(handler2->waitUntilCompleted());
}

/**
 * Send a long-running @c BLOCKING @c AVSDirective followed by a @c NON_BLOCKING @c AVSDirective with the same
 * @c DialogRequestId.  Once the first @c AVSDirective is being handled and the second @c AVSDirective is being
 * preHandled send a new @c AVDirective with a different @c DialogRequestId to simulate 'barge-in'. Expect that
 * the first two directives will be cancelled and the third one will be handled (and then cancelled at the
 * end by setting the dialogRequestId to close out the test).
 */
TEST_F(DirectiveSequencerTest, testThatBargeInDropsSubsequentDirectives) {
    auto avsMessageHeader0 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_0, DIALOG_REQUEST_ID_0);
    std::shared_ptr<AVSDirective> directive0 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader0, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);
    auto avsMessageHeader1 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_1, DIALOG_REQUEST_ID_0);
    std::shared_ptr<AVSDirective> directive1 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader1, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);
    auto avsMessageHeader2 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_TEST, NAME_BLOCKING, MESSAGE_ID_2, DIALOG_REQUEST_ID_1);
    std::shared_ptr<AVSDirective> directive2 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader2, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);

    DirectiveHandlerConfiguration handler0Config;
    handler0Config[{NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK}] = BlockingPolicy::BLOCKING;
    auto handler0 = MockDirectiveHandler::create(handler0Config, LONG_HANDLING_TIME_MS);

    DirectiveHandlerConfiguration handler1Config;
    handler1Config[{NAMESPACE_AUDIO_PLAYER, NAME_PLAY}] = BlockingPolicy::NON_BLOCKING;
    auto handler1 = MockDirectiveHandler::create(handler1Config);

    DirectiveHandlerConfiguration handler2Config;
    handler2Config[{NAMESPACE_TEST, NAME_BLOCKING}] = BlockingPolicy::BLOCKING;
    auto handler2 = MockDirectiveHandler::create(handler2Config, LONG_HANDLING_TIME_MS);

    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler0));
    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler1));
    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler2));

    EXPECT_CALL(*(handler0.get()), handleDirectiveImmediately(directive0)).Times(0);
    EXPECT_CALL(*(handler0.get()), preHandleDirective(directive0, _)).Times(1);
    EXPECT_CALL(*(handler0.get()), handleDirective(MESSAGE_ID_0)).Times(1);
    EXPECT_CALL(*(handler0.get()), cancelDirective(MESSAGE_ID_0)).Times(1);

    EXPECT_CALL(*(handler1.get()), handleDirectiveImmediately(directive1)).Times(0);
    EXPECT_CALL(*(handler1.get()), preHandleDirective(directive1, _)).Times(1);
    EXPECT_CALL(*(handler1.get()), handleDirective(MESSAGE_ID_1)).Times(0);
    EXPECT_CALL(*(handler1.get()), cancelDirective(MESSAGE_ID_1)).Times(1);

    EXPECT_CALL(*(handler2.get()), handleDirectiveImmediately(directive2)).Times(0);
    EXPECT_CALL(*(handler2.get()), preHandleDirective(directive2, _)).Times(1);
    EXPECT_CALL(*(handler2.get()), handleDirective(MESSAGE_ID_2)).Times(1);
    EXPECT_CALL(*(handler2.get()), cancelDirective(MESSAGE_ID_2)).Times(1);

    m_sequencer->setDialogRequestId(DIALOG_REQUEST_ID_0);
    m_sequencer->onDirective(directive0);
    m_sequencer->onDirective(directive1);
    ASSERT_TRUE(handler0->waitUntilHandling());
    ASSERT_TRUE(handler1->waitUntilPreHandling());
    m_sequencer->setDialogRequestId(DIALOG_REQUEST_ID_1);
    m_sequencer->onDirective(directive2);
    ASSERT_TRUE(handler2->waitUntilHandling());
    m_sequencer->setDialogRequestId(DIALOG_REQUEST_ID_2);
    ASSERT_TRUE(handler2->waitUntilCanceling());
}

/**
 * Send a long-running @c BLOCKING @c AVSDirective followed by a @c NON_BLOCKING @c AVSDirective with the same
 * @c DialogRequestId. When the first @c AVSDirective is preHandled, report a failure via @c setFailed().
 * Expect that the first @c AVSDirective will not be cancelled and that the second @c AVSDirective will be dropped
 * entirely.
 */
TEST_F(DirectiveSequencerTest, testPreHandleDirectiveError) {
    auto avsMessageHeader0 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_0, DIALOG_REQUEST_ID_0);
    std::shared_ptr<AVSDirective> directive0 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader0, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);
    auto avsMessageHeader1 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_1, DIALOG_REQUEST_ID_0);
    std::shared_ptr<AVSDirective> directive1 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader1, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);

    DirectiveHandlerConfiguration handler0Config;
    handler0Config[{NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK}] = BlockingPolicy::BLOCKING;
    auto handler0 = MockDirectiveHandler::create(handler0Config, LONG_HANDLING_TIME_MS);

    DirectiveHandlerConfiguration handler1Config;
    handler1Config[{NAMESPACE_AUDIO_PLAYER, NAME_PLAY}] = BlockingPolicy::NON_BLOCKING;
    auto handler1 = MockDirectiveHandler::create(handler1Config);

    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler0));
    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler1));

    EXPECT_CALL(*(handler0.get()), handleDirectiveImmediately(directive0)).Times(0);
    EXPECT_CALL(*(handler0.get()), preHandleDirective(directive0, _))
        .WillOnce(Invoke(handler0.get(), &MockDirectiveHandler::doPreHandlingFailed));
    EXPECT_CALL(*(handler0.get()), handleDirective(MESSAGE_ID_0)).Times(0);
    EXPECT_CALL(*(handler0.get()), cancelDirective(MESSAGE_ID_0)).Times(0);

    EXPECT_CALL(*(handler1.get()), handleDirectiveImmediately(directive1)).Times(0);
    EXPECT_CALL(*(handler1.get()), preHandleDirective(directive1, _)).Times(0);
    EXPECT_CALL(*(handler1.get()), handleDirective(MESSAGE_ID_1)).Times(0);
    EXPECT_CALL(*(handler1.get()), cancelDirective(MESSAGE_ID_1)).Times(0);

    m_sequencer->setDialogRequestId(DIALOG_REQUEST_ID_0);
    m_sequencer->onDirective(directive0);
    m_sequencer->onDirective(directive1);
    ASSERT_TRUE(handler0->waitUntilPreHandling());
}

/**
 * Send a long-running @c BLOCKING @c AVSDirective followed by a @c NON_BLOCKING directive with the same
 * @c DialogRequestId.  When the first @c AVSDirective is handled, report a failure via @c setFailed().
 * Expect that the first @c AVSDirective will not be cancelled and that the second @c AVSDirective may be
 * dropped before @c preHandleDirective() is called, and that if not, it will be cancelled.
 */
TEST_F(DirectiveSequencerTest, testHandleDirectiveError) {
    auto avsMessageHeader0 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_0, DIALOG_REQUEST_ID_0);
    std::shared_ptr<AVSDirective> directive0 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader0, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);
    auto avsMessageHeader1 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_1, DIALOG_REQUEST_ID_0);
    std::shared_ptr<AVSDirective> directive1 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader1, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);

    DirectiveHandlerConfiguration handler0Config;
    handler0Config[{NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK}] = BlockingPolicy::BLOCKING;
    auto handler0 = MockDirectiveHandler::create(handler0Config, LONG_HANDLING_TIME_MS);

    DirectiveHandlerConfiguration handler1Config;
    handler1Config[{NAMESPACE_AUDIO_PLAYER, NAME_PLAY}] = BlockingPolicy::NON_BLOCKING;
    auto handler1 = MockDirectiveHandler::create(handler1Config);

    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler0));
    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler1));

    EXPECT_CALL(*(handler0.get()), handleDirectiveImmediately(directive0)).Times(0);
    EXPECT_CALL(*(handler0.get()), preHandleDirective(directive0, _)).Times(1);
    EXPECT_CALL(*(handler0.get()), handleDirective(MESSAGE_ID_0))
        .WillOnce(Invoke(handler0.get(), &MockDirectiveHandler::doHandlingFailed));
    EXPECT_CALL(*(handler0.get()), cancelDirective(MESSAGE_ID_0)).Times(0);

    EXPECT_CALL(*(handler1.get()), handleDirectiveImmediately(directive1)).Times(0);
    EXPECT_CALL(*(handler1.get()), preHandleDirective(directive1, _)).Times(AtMost(1));
    EXPECT_CALL(*(handler1.get()), handleDirective(MESSAGE_ID_1)).Times(0);
    EXPECT_CALL(*(handler1.get()), cancelDirective(MESSAGE_ID_1)).Times(AtMost(1));

    m_sequencer->setDialogRequestId(DIALOG_REQUEST_ID_0);
    m_sequencer->onDirective(directive0);
    m_sequencer->onDirective(directive1);
    ASSERT_TRUE(handler0->waitUntilHandling());
}

/**
 * Send a long-running @c BLOCKING @c AVSDirective followed by two @c NON_BLOCKING @c AVSDirectives with the same
 * @c DialogRequestId.  Once they have reached the handling and preHandling stages respectively, call
 * @c addDirectiveHandler(), changing the handlers for the first two @c AVSDirectives.  After the handlers
 * have been changed, trigger completion of the first @c AVSDirective. Expect that the first directive will be
 * completed by its initial handler (the switch was after handleDirective() was called, and there is no trigger to
 * cancel it. Expect that the second directive's first handler will get the preHandleDirective() call and that its
 * second handler will receive the handleDirective().  Expect that the third directive's handler will get calls for
 * preHandleDirective() and cancelDirective().  The cancel is triggered because the second directive's second
 * handler did not recognize the messageId passed in to handleDirective(), and returned false, canceling any
 * subsequent directives with the same dialogRequestId.  Along the way, call @c addDirectiveHandler() while
 * inside cancelDirective() to verify that that operation is refused.
 */
TEST_F(DirectiveSequencerTest, testAddDirectiveHandlersWhileHandlingDirectives) {
    auto avsMessageHeader0 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_0, DIALOG_REQUEST_ID_0);
    std::shared_ptr<AVSDirective> directive0 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader0, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);
    auto avsMessageHeader1 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_1, DIALOG_REQUEST_ID_0);
    std::shared_ptr<AVSDirective> directive1 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader1, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);
    auto avsMessageHeader2 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_TEST, NAME_NON_BLOCKING, MESSAGE_ID_2, DIALOG_REQUEST_ID_0);
    std::shared_ptr<AVSDirective> directive2 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader2, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);

    DirectiveHandlerConfiguration handler0Config;
    handler0Config[{NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK}] = BlockingPolicy::BLOCKING;
    auto handler0 = MockDirectiveHandler::create(handler0Config, LONG_HANDLING_TIME_MS);

    DirectiveHandlerConfiguration handler1Config;
    handler1Config[{NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK}] = BlockingPolicy::BLOCKING;
    auto handler1 = MockDirectiveHandler::create(handler1Config);

    DirectiveHandlerConfiguration handler2Config;
    handler2Config[{NAMESPACE_AUDIO_PLAYER, NAME_PLAY}] = BlockingPolicy::NON_BLOCKING;
    auto handler2 = MockDirectiveHandler::create(handler2Config);

    DirectiveHandlerConfiguration handler3Config;
    handler3Config[{NAMESPACE_AUDIO_PLAYER, NAME_PLAY}] = BlockingPolicy::NON_BLOCKING;
    auto handler3 = MockDirectiveHandler::create(handler3Config);

    DirectiveHandlerConfiguration handler4Config;
    handler4Config[{NAMESPACE_TEST, NAME_NON_BLOCKING}] = BlockingPolicy::NON_BLOCKING;
    auto handler4 = MockDirectiveHandler::create(handler4Config);

    auto cancelDirectiveFunction = [this, &handler1, &handler3, &handler4](const std::string& messageId) {
        ASSERT_TRUE(m_sequencer->removeDirectiveHandler(handler1));
        ASSERT_TRUE(m_sequencer->removeDirectiveHandler(handler3));
        ASSERT_TRUE(m_sequencer->removeDirectiveHandler(handler4));
        handler4->mockCancelDirective(messageId);
    };

    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler0));
    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler2));
    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler4));

    EXPECT_CALL(*(handler0.get()), handleDirectiveImmediately(directive0)).Times(0);
    EXPECT_CALL(*(handler0.get()), preHandleDirective(directive0, _)).Times(1);
    EXPECT_CALL(*(handler0.get()), handleDirective(MESSAGE_ID_0)).Times(1);
    EXPECT_CALL(*(handler0.get()), cancelDirective(MESSAGE_ID_0)).Times(0);

    EXPECT_CALL(*(handler1.get()), handleDirectiveImmediately(directive0)).Times(0);
    EXPECT_CALL(*(handler1.get()), preHandleDirective(directive0, _)).Times(0);
    EXPECT_CALL(*(handler1.get()), handleDirective(MESSAGE_ID_0)).Times(0);
    EXPECT_CALL(*(handler1.get()), cancelDirective(MESSAGE_ID_0)).Times(0);

    EXPECT_CALL(*(handler2.get()), handleDirectiveImmediately(directive1)).Times(0);
    EXPECT_CALL(*(handler2.get()), preHandleDirective(directive1, _)).Times(1);
    EXPECT_CALL(*(handler2.get()), handleDirective(MESSAGE_ID_1)).Times(0);
    EXPECT_CALL(*(handler2.get()), cancelDirective(MESSAGE_ID_1)).Times(0);

    EXPECT_CALL(*(handler3.get()), handleDirectiveImmediately(directive1)).Times(0);
    EXPECT_CALL(*(handler3.get()), preHandleDirective(directive1, _)).Times(0);
    EXPECT_CALL(*(handler3.get()), handleDirective(MESSAGE_ID_1)).Times(1);
    EXPECT_CALL(*(handler3.get()), cancelDirective(MESSAGE_ID_1)).Times(0);

    EXPECT_CALL(*(handler4.get()), handleDirectiveImmediately(directive2)).Times(0);
    EXPECT_CALL(*(handler4.get()), preHandleDirective(directive2, _)).Times(1);
    EXPECT_CALL(*(handler4.get()), handleDirective(MESSAGE_ID_2)).Times(0);
    EXPECT_CALL(*(handler4.get()), cancelDirective(MESSAGE_ID_2)).WillOnce(Invoke(cancelDirectiveFunction));

    m_sequencer->setDialogRequestId(DIALOG_REQUEST_ID_0);
    m_sequencer->onDirective(directive0);
    m_sequencer->onDirective(directive1);
    m_sequencer->onDirective(directive2);

    handler0->waitUntilHandling();
    handler4->waitUntilPreHandling();

    ASSERT_TRUE(m_sequencer->removeDirectiveHandler(handler0));
    ASSERT_TRUE(m_sequencer->removeDirectiveHandler(handler2));
    ASSERT_TRUE(m_sequencer->removeDirectiveHandler(handler4));

    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler1));
    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler3));
    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler4));

    handler0->doHandlingCompleted();
    ASSERT_TRUE(handler4->waitUntilCanceling());
}

/**
 * Send an @c AVSDirective with an non-empty @c DialogRequestId and a @c BLOCKING policy followed by
 * @c HANDLE_IMMEDIATELY and @c NON_BLOCKING @c AVSDirectives with the same @c DialogRequestId. Expect a call to
 * @c preHandleDirective(@c AVSDirective) and a call to @c handleDirective() for the @c AVSDirective that are not
 * @c HANDLE_IMMEDIATELY.  And for the one with @c HANDLE_IMMEDIATELY, only @c handleDirectiveImmediately() is called.
 */
TEST_F(DirectiveSequencerTest, testHandleBlockingThenImmediatelyThenNonBockingOnSameDialogId) {
    auto avsMessageHeader0 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_0, DIALOG_REQUEST_ID_0);
    std::shared_ptr<AVSDirective> directive0 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader0, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);
    auto avsMessageHeader1 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_TEST, NAME_HANDLE_IMMEDIATELY, MESSAGE_ID_1, DIALOG_REQUEST_ID_0);
    std::shared_ptr<AVSDirective> directive1 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader1, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);
    auto avsMessageHeader2 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_2, DIALOG_REQUEST_ID_0);
    std::shared_ptr<AVSDirective> directive2 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader2, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);

    DirectiveHandlerConfiguration handler0Config;
    handler0Config[{NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK}] = BlockingPolicy::BLOCKING;
    auto handler0 = MockDirectiveHandler::create(handler0Config);

    DirectiveHandlerConfiguration handler1Config;
    handler1Config[{NAMESPACE_TEST, NAME_HANDLE_IMMEDIATELY}] = BlockingPolicy::HANDLE_IMMEDIATELY;
    auto handler1 = MockDirectiveHandler::create(handler1Config);

    DirectiveHandlerConfiguration handler2Config;
    handler2Config[{NAMESPACE_AUDIO_PLAYER, NAME_PLAY}] = BlockingPolicy::NON_BLOCKING;
    auto handler2 = MockDirectiveHandler::create(handler2Config);

    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler0));
    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler1));
    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler2));

    // Enforce the sequence.
    InSequence dummy;

    EXPECT_CALL(*(handler0.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler0.get()), preHandleDirective(directive0, _)).Times(1);
    EXPECT_CALL(*(handler0.get()), handleDirective(MESSAGE_ID_0)).Times(1);
    EXPECT_CALL(*(handler0.get()), cancelDirective(_)).Times(0);

    EXPECT_CALL(*(handler1.get()), handleDirectiveImmediately(directive1)).Times(1);
    EXPECT_CALL(*(handler1.get()), preHandleDirective(_, _)).Times(0);
    EXPECT_CALL(*(handler1.get()), handleDirective(_)).Times(0);
    EXPECT_CALL(*(handler1.get()), cancelDirective(_)).Times(0);

    EXPECT_CALL(*(handler2.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler2.get()), preHandleDirective(directive2, _)).Times(1);
    EXPECT_CALL(*(handler2.get()), handleDirective(MESSAGE_ID_2)).Times(1);
    EXPECT_CALL(*(handler2.get()), cancelDirective(_)).Times(0);

    m_sequencer->setDialogRequestId(DIALOG_REQUEST_ID_0);
    m_sequencer->onDirective(directive0);
    ASSERT_TRUE(handler0->waitUntilCompleted());
    m_sequencer->onDirective(directive1);
    m_sequencer->onDirective(directive2);
    ASSERT_TRUE(handler2->waitUntilCompleted());
}

/**
 * Check that the @ DirectiveSequencer does not handle directives when it is disabled
 */
TEST_F(DirectiveSequencerTest, testAddDirectiveAfterDisabled) {
    auto avsMessageHeader =
        std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_0, DIALOG_REQUEST_ID_0);
    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);

    DirectiveHandlerConfiguration handlerConfig;
    handlerConfig[{NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK}] = BlockingPolicy::BLOCKING;
    auto handler = MockDirectiveHandler::create(handlerConfig, LONG_HANDLING_TIME_MS);

    EXPECT_CALL(*handler, handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*handler, preHandleDirective(directive, _)).Times(0);
    EXPECT_CALL(*handler, handleDirective(MESSAGE_ID_2)).Times(0);
    EXPECT_CALL(*handler, cancelDirective(_)).Times(0);

    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler));
    m_sequencer->disable();
    m_sequencer->setDialogRequestId(DIALOG_REQUEST_ID_0);
    ASSERT_FALSE(m_sequencer->onDirective(directive));

    // Tear down method expects the sequencer to be enabled
    m_sequencer->enable();
}

/**
 * Check that the @ DirectiveSequencer.disable() cancel directive being handled
 */
TEST_F(DirectiveSequencerTest, testDisableCancelsDirective) {
    auto avsMessageHeader =
        std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_0, DIALOG_REQUEST_ID_0);
    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);

    DirectiveHandlerConfiguration handlerConfig;
    handlerConfig[{NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK}] = BlockingPolicy::BLOCKING;
    auto handler = MockDirectiveHandler::create(handlerConfig, LONG_HANDLING_TIME_MS);

    EXPECT_CALL(*handler, handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*handler, preHandleDirective(directive, _)).Times(1);
    EXPECT_CALL(*handler, handleDirective(MESSAGE_ID_0)).Times(AtMost(1));
    EXPECT_CALL(*handler, cancelDirective(_)).Times(1);

    // Add directive and wait till prehandling
    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler));
    m_sequencer->setDialogRequestId(DIALOG_REQUEST_ID_0);
    ASSERT_TRUE(m_sequencer->onDirective(directive));
    ASSERT_TRUE(handler->waitUntilPreHandling());

    // Disable
    m_sequencer->disable();
    ASSERT_TRUE(handler->waitUntilCanceling());

    // Tear down method expects the sequencer to be enabled
    m_sequencer->enable();
}

/**
 * Check that the @ DirectiveSequencer can handle directives after being re-enabled
 */
TEST_F(DirectiveSequencerTest, testAddDirectiveAfterReEnabled) {
    auto avsMessageHeader0 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_0, DIALOG_REQUEST_ID_0);
    std::shared_ptr<AVSDirective> directive0 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader0, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);
    auto avsMessageHeader1 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_1, DIALOG_REQUEST_ID_1);
    std::shared_ptr<AVSDirective> ignoredDirective1 = AVSDirective::create(
        "ignoreDirective", avsMessageHeader1, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);
    auto avsMessageHeader2 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_2, DIALOG_REQUEST_ID_2);
    std::shared_ptr<AVSDirective> ignoredDirective2 = AVSDirective::create(
        "anotherIgnored", avsMessageHeader2, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);

    DirectiveHandlerConfiguration handlerConfig;
    handlerConfig[{NAMESPACE_AUDIO_PLAYER, NAME_PLAY}] = BlockingPolicy::NON_BLOCKING;
    auto handler = MockDirectiveHandler::create(handlerConfig);

    // No handle calls are expected
    EXPECT_CALL(*handler, handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*handler, preHandleDirective(_, _)).Times(0);
    EXPECT_CALL(*handler, handleDirective(_)).Times(0);
    EXPECT_CALL(*handler, cancelDirective(_)).Times(0);

    // Except for the ones handling the directive0
    EXPECT_CALL(*handler, preHandleDirective(directive0, _)).Times(1);
    EXPECT_CALL(*handler, handleDirective(MESSAGE_ID_0)).Times(1);

    ASSERT_TRUE(m_sequencer->addDirectiveHandler(handler));
    m_sequencer->disable();

    // Make sure these directives are ignored and never processed
    ASSERT_FALSE(m_sequencer->onDirective(ignoredDirective1));
    ASSERT_FALSE(m_sequencer->onDirective(ignoredDirective2));

    m_sequencer->enable();
    m_sequencer->setDialogRequestId(DIALOG_REQUEST_ID_0);

    ASSERT_TRUE(m_sequencer->onDirective(directive0));
    ASSERT_TRUE(handler->waitUntilCompleted());
}

}  // namespace test
}  // namespace adsl
}  // namespace alexaClientSDK
