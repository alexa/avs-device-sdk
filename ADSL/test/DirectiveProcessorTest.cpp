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

// @file DirectiveProcessorTest.cpp

#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>

#include "ADSL/DirectiveProcessor.h"
#include "MockDirectiveHandler.h"

using namespace ::testing;

namespace alexaClientSDK {
namespace adsl {
namespace test {

using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;

/// Generic messageId used for tests.
static const std::string MESSAGE_ID_0_0("Message_0_0");

/// Generic MessageId used for test.
static const std::string MESSAGE_ID_0_1("Message_0_1");

/// Generic MessageId used for test.
static const std::string MESSAGE_ID_0_2("Message_0_2");

/// Generic MessageId used for test.
static const std::string MESSAGE_ID_0_3("Message_0_3");

/// Generic MessageId used for tests.
static const std::string MESSAGE_ID_1_0("Message_1_0");

/// Generic DialogRequestId used for tests.
static const std::string DIALOG_REQUEST_ID_0("DialogRequestId_0");

/// Generic DialogRequestId used for tests.
static const std::string DIALOG_REQUEST_ID_1("DialogRequestId_1");

/// An unparsed directive for test.
static const std::string UNPARSED_DIRECTIVE("unparsedDirectiveForTest");

/// A paylaod for test;
static const std::string PAYLOAD_TEST("payloadForTest");

/// A generic namespace string for tests.
static const std::string NAMESPACE_0("namespace_0");

/// A generic namespace string for tests.
static const std::string NAMESPACE_1("namespace_1");

/// A generic name string for tests.
static const std::string NAME_0("name_0");

/// A generic name string for tests.
static const std::string NAME_1("name_1");

/// A generic name string for tests.
static const std::string NAME_2("name_2");

/// A generic name string for tests.
static const std::string NAME_3("name_3");

static const std::string TEST_ATTACHMENT_CONTEXT_ID("TEST_ATTACHMENT_CONTEXT_ID");

/// Namespace and name combination for tests.
#define NAMESPACE_AND_NAME_0_0 NAMESPACE_0, NAME_0

/// Namespace and name combination (changing name this time) for tests.
#define NAMESPACE_AND_NAME_0_1 NAMESPACE_0, NAME_1

/// Namespace and name combination (changing name this time) for tests.
#define NAMESPACE_AND_NAME_0_2 NAMESPACE_0, NAME_2

/// Namespace and name combination (changing name this time) for tests.
#define NAMESPACE_AND_NAME_0_3 NAMESPACE_0, NAME_3

/// Namespace and name combination (changing namespace this time) for tests.
#define NAMESPACE_AND_NAME_1_0 NAMESPACE_1, NAME_0

/// An InteractionModel.NewDialogRequest v1.0 directive. The v1.0 directive has a dialogRequestID in the header and
/// payload.
// clang-format off
static const std::string NEW_DIALOG_REQUEST_DIRECTIVE_V0 = R"delim(
{
	"directive": {
		"header": {
			"namespace": "InteractionModel",
			"name": "NewDialogRequest",
			"messageId": "2120215c-d803-4800-8773-e9505d16354a",
			"dialogRequestId": ")delim" + DIALOG_REQUEST_ID_0 + R"delim("
		},
		"payload": {
			"dialogRequestId": ")delim" + DIALOG_REQUEST_ID_0 + R"delim("
		}
	}
})delim";
// clang-format on

/// A null context ID string
static const std::string NO_CONTEXT = "";

/// The NewDialogRequest directive signature.
static const NamespaceAndName NEW_DIALOG_REQUEST_SIGNATURE{"InteractionModel", "NewDialogRequest"};

/// Test timeout.
static const auto TEST_TIMEOUT = std::chrono::seconds(5);

/**
 * DirectiveRouterTest
 */
class DirectiveProcessorTest : public ::testing::Test {
public:
    void SetUp() override;

    /// A DirectiveRouter instance to test with.
    std::shared_ptr<DirectiveRouter> m_router;

    /// A DirectiveProcessor instance to test with.
    std::shared_ptr<DirectiveProcessor> m_processor;

    /// AttachmentManager with which to create directives.
    std::shared_ptr<AttachmentManager> m_attachmentManager;

    /// Generic @c AVSDirective for tests.
    std::shared_ptr<AVSDirective> m_directive_0_0;

    /// Generic @c AVSDirective for tests.
    std::shared_ptr<AVSDirective> m_directive_0_1;

    /// Generic @c AVSDirective for tests.
    std::shared_ptr<AVSDirective> m_directive_0_2;

    /// Generic @c AVSDirective for tests.
    std::shared_ptr<AVSDirective> m_directive_0_3;

    /// Generic @c AVSDirective for tests.
    std::shared_ptr<AVSDirective> m_directive_1_0;
};

void DirectiveProcessorTest::SetUp() {
    m_router = std::make_shared<DirectiveRouter>();
    m_processor = std::make_shared<DirectiveProcessor>(m_router.get());
    m_attachmentManager = std::make_shared<AttachmentManager>(AttachmentManager::AttachmentType::IN_PROCESS);

    auto avsMessageHeader_0_0 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_AND_NAME_0_0, MESSAGE_ID_0_0, DIALOG_REQUEST_ID_0);
    m_directive_0_0 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader_0_0, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);

    auto avsMessageHeader_0_1 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_AND_NAME_0_1, MESSAGE_ID_0_1, DIALOG_REQUEST_ID_0);
    m_directive_0_1 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader_0_1, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);

    auto avsMessageHeader_0_2 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_AND_NAME_0_2, MESSAGE_ID_0_2, DIALOG_REQUEST_ID_0);
    m_directive_0_2 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader_0_2, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);

    auto avsMessageHeader_0_3 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_AND_NAME_0_3, MESSAGE_ID_0_3, DIALOG_REQUEST_ID_0);
    m_directive_0_3 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader_0_3, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);

    auto avsMessageHeader_1_0 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_AND_NAME_1_0, MESSAGE_ID_1_0, DIALOG_REQUEST_ID_1);
    m_directive_1_0 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader_1_0, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);
}

/**
 * Send a nullptr @c AVSDirective.  Expect that it is ignored and a failure status (false) is returned.
 */
TEST_F(DirectiveProcessorTest, testNullptrDirective) {
    ASSERT_FALSE(m_processor->onDirective(nullptr));
}

/**
 * Register a @c DirectiveHandler and set the @c dialogRequestId.  Send a @c AVSDirective that the handler
 * registered for, but with a @c dialogRequestId that does not match.  Expect that @c onDirective()
 * returns true (because the handler was registered) but that none of the handler methods are called
 * (because directives with the wrong @c dialogRequestID are dropped).
 */
TEST_F(DirectiveProcessorTest, testWrongDialogRequestId) {
    DirectiveHandlerConfiguration handler0Config;
    handler0Config[{NAMESPACE_AND_NAME_0_0}] = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, false);
    std::shared_ptr<MockDirectiveHandler> handler0 = MockDirectiveHandler::create(handler0Config);

    ASSERT_TRUE(m_router->addDirectiveHandler(handler0));

    EXPECT_CALL(*(handler0.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler0.get()), preHandleDirective(m_directive_0_0, _)).Times(0);
    EXPECT_CALL(*(handler0.get()), handleDirective(MESSAGE_ID_0_0)).Times(0);
    EXPECT_CALL(*(handler0.get()), cancelDirective(_)).Times(0);

    m_processor->setDialogRequestId(DIALOG_REQUEST_ID_1);
    ASSERT_TRUE(m_processor->onDirective(m_directive_0_0));
}

/**
 * Register an @c AUDIO_NON_BLOCKING @c DirectiveHandler.  Send an @c AVSDirective that matches the registered handler.
 * Expect that @c preHandleDirective() and @c handleDirective() are called.
 */
TEST_F(DirectiveProcessorTest, testSendNonBlocking) {
    DirectiveHandlerConfiguration handler0Config;
    handler0Config[{NAMESPACE_AND_NAME_0_0}] = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, false);
    std::shared_ptr<MockDirectiveHandler> handler0 = MockDirectiveHandler::create(handler0Config);

    ASSERT_TRUE(m_router->addDirectiveHandler(handler0));

    EXPECT_CALL(*(handler0.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler0.get()), preHandleDirective(m_directive_0_0, _)).Times(1);
    EXPECT_CALL(*(handler0.get()), handleDirective(MESSAGE_ID_0_0)).Times(1);
    EXPECT_CALL(*(handler0.get()), cancelDirective(_)).Times(0);

    m_processor->setDialogRequestId(DIALOG_REQUEST_ID_0);
    ASSERT_TRUE(m_processor->onDirective(m_directive_0_0));

    ASSERT_TRUE(handler0->waitUntilCompleted(std::chrono::milliseconds(1500000)));
}

/**
 * Test sending a blocking and then a non-blocking directive.  Expect that @c preHandleDirective() and
 * @c handleDirective() is called for each.
 */
TEST_F(DirectiveProcessorTest, testSendBlockingThenNonBlocking) {
    DirectiveHandlerConfiguration handler0Config;
    handler0Config[{NAMESPACE_AND_NAME_0_0}] = BlockingPolicy(BlockingPolicy::MEDIUMS_AUDIO_AND_VISUAL, true);
    std::shared_ptr<MockDirectiveHandler> handler0 = MockDirectiveHandler::create(handler0Config);

    DirectiveHandlerConfiguration handler1Config;
    handler1Config[{NAMESPACE_AND_NAME_0_1}] = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, false);
    std::shared_ptr<MockDirectiveHandler> handler1 = MockDirectiveHandler::create(handler1Config);

    ASSERT_TRUE(m_router->addDirectiveHandler(handler0));
    ASSERT_TRUE(m_router->addDirectiveHandler(handler1));

    EXPECT_CALL(*(handler0.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler0.get()), preHandleDirective(m_directive_0_0, _)).Times(1);
    EXPECT_CALL(*(handler0.get()), handleDirective(MESSAGE_ID_0_0)).Times(1);
    EXPECT_CALL(*(handler0.get()), cancelDirective(_)).Times(0);

    EXPECT_CALL(*(handler1.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler1.get()), preHandleDirective(m_directive_0_1, _)).Times(1);
    EXPECT_CALL(*(handler1.get()), handleDirective(MESSAGE_ID_0_1)).Times(1);
    EXPECT_CALL(*(handler1.get()), cancelDirective(_)).Times(0);

    m_processor->setDialogRequestId(DIALOG_REQUEST_ID_0);
    ASSERT_TRUE(m_processor->onDirective(m_directive_0_0));
    ASSERT_TRUE(m_processor->onDirective(m_directive_0_1));

    ASSERT_TRUE(handler1->waitUntilCompleted());
}

/**
 * Register a handler for two @c AVSDirectives.  Send an @c AVSDirective for which no handler is registered.
 * Then send an @c AVSDirective with the same @c dialogRequestId.  Expect that the first @c AVSDirective will
 * be dropped (and @c onDirective() will return false) because there is no handler.  Expect the second directive
 * to be handled, including @c preHandleDirective() and @c handleDirective() calls. After @c handleDirective()
 * has been called, set the @c dialogRequestId and send an @c AVSDirective for which the other handler was
 * registered. Expect that the last directive is handled as well.
 */
TEST_F(DirectiveProcessorTest, testOnUnregisteredDirective) {
    DirectiveHandlerConfiguration handler1Config;
    handler1Config[{NAMESPACE_AND_NAME_0_1}] = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, false);
    std::shared_ptr<MockDirectiveHandler> handler1 = MockDirectiveHandler::create(handler1Config);

    DirectiveHandlerConfiguration handler2Config;
    handler2Config[{NAMESPACE_AND_NAME_1_0}] = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, false);
    std::shared_ptr<MockDirectiveHandler> handler2 = MockDirectiveHandler::create(handler2Config);

    ASSERT_TRUE(m_router->addDirectiveHandler(handler1));
    ASSERT_TRUE(m_router->addDirectiveHandler(handler2));

    EXPECT_CALL(*(handler1.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler1.get()), preHandleDirective(m_directive_0_1, _)).Times(1);
    EXPECT_CALL(*(handler1.get()), handleDirective(MESSAGE_ID_0_1)).Times(1);
    EXPECT_CALL(*(handler1.get()), cancelDirective(_)).Times(0);

    EXPECT_CALL(*(handler2.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler2.get()), preHandleDirective(m_directive_1_0, _)).Times(1);
    EXPECT_CALL(*(handler2.get()), handleDirective(MESSAGE_ID_1_0)).Times(1);
    EXPECT_CALL(*(handler2.get()), cancelDirective(_)).Times(0);

    m_processor->setDialogRequestId(DIALOG_REQUEST_ID_0);
    ASSERT_FALSE(m_processor->onDirective(m_directive_0_0));
    ASSERT_TRUE(m_processor->onDirective(m_directive_0_1));
    ASSERT_TRUE(handler1->waitUntilCompleted());

    m_processor->setDialogRequestId(DIALOG_REQUEST_ID_1);
    ASSERT_TRUE(m_processor->onDirective(m_directive_1_0));

    ASSERT_TRUE(handler2->waitUntilCompleted());
}

/**
 * Register a long running and @c BLOCKING @c DirectiveHandler for one @c AVSDirective and a short
 * running @c DirectiveHandler for two more @c AVSDirectives.  Send the long running @c AVSDirective
 * followed by a short running one.  Wait until the long running @c AVSDirective has entered
 * handleDirective() and the short running @c AVSDirective has entered preHandleDirective().
 * then change the @c dialogRequestId() and send a final @c AVSDirective with the new
 * @c dialogRequestId().  Expect the first two @c AVSDirectives to be cancelled and expect the
 * final @c AVSDirective to be processed normally.
 */
TEST_F(DirectiveProcessorTest, testSetDialogRequestIdCancelsOutstandingDirectives) {
    DirectiveHandlerConfiguration longRunningHandlerConfig;
    longRunningHandlerConfig[{NAMESPACE_AND_NAME_0_0}] = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, true);
    auto longRunningHandler =
        MockDirectiveHandler::create(longRunningHandlerConfig, MockDirectiveHandler::DEFAULT_DONE_TIMEOUT_MS);

    DirectiveHandlerConfiguration handler1Config;
    auto audioNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, false);
    handler1Config[{NAMESPACE_AND_NAME_0_1}] = audioNonBlockingPolicy;
    auto handler1 = MockDirectiveHandler::create(handler1Config);

    DirectiveHandlerConfiguration handler2Config;
    handler2Config[{NAMESPACE_AND_NAME_1_0}] = audioNonBlockingPolicy;
    auto handler2 = MockDirectiveHandler::create(handler2Config);

    ASSERT_TRUE(m_router->addDirectiveHandler(longRunningHandler));
    ASSERT_TRUE(m_router->addDirectiveHandler(handler1));
    ASSERT_TRUE(m_router->addDirectiveHandler(handler2));

    EXPECT_CALL(*(longRunningHandler.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(longRunningHandler.get()), preHandleDirective(_, _)).Times(1);
    EXPECT_CALL(*(longRunningHandler.get()), handleDirective(_)).Times(1);
    EXPECT_CALL(*(longRunningHandler.get()), cancelDirective(_)).Times(1);

    EXPECT_CALL(*(handler1.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler1.get()), preHandleDirective(m_directive_0_1, _)).Times(1);
    EXPECT_CALL(*(handler1.get()), handleDirective(MESSAGE_ID_0_1)).Times(0);
    EXPECT_CALL(*(handler1.get()), cancelDirective(_)).Times(1);

    EXPECT_CALL(*(handler2.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler2.get()), preHandleDirective(m_directive_1_0, _)).Times(1);
    EXPECT_CALL(*(handler2.get()), handleDirective(MESSAGE_ID_1_0)).Times(1);
    EXPECT_CALL(*(handler2.get()), cancelDirective(_)).Times(0);

    m_processor->setDialogRequestId(DIALOG_REQUEST_ID_0);
    ASSERT_TRUE(m_processor->onDirective(m_directive_0_0));
    ASSERT_TRUE(m_processor->onDirective(m_directive_0_1));
    ASSERT_TRUE(longRunningHandler->waitUntilHandling());
    ASSERT_TRUE(handler1->waitUntilPreHandling());
    m_processor->setDialogRequestId(DIALOG_REQUEST_ID_1);
    ASSERT_TRUE(m_processor->onDirective(m_directive_1_0));
    ASSERT_TRUE(handler2->waitUntilCompleted());
}

TEST_F(DirectiveProcessorTest, testAddDirectiveWhileDisabled) {
    m_processor->disable();
    ASSERT_FALSE(m_processor->onDirective(m_directive_0_0));
}

TEST_F(DirectiveProcessorTest, testAddDirectiveAfterReEnabled) {
    m_processor->disable();
    ASSERT_FALSE(m_processor->onDirective(m_directive_0_0));

    m_processor->enable();
    ASSERT_TRUE(m_processor->onDirective(m_directive_0_0));
}

/**
 * Verify that an @c AVSDirective using @c MEDIUMS_AUDIO_AND_VISUAL
 *  is blocking @c MEDIUM_AUDIO but not blocking @c MEDIUMS_NONE.
 */
TEST_F(DirectiveProcessorTest, testAudioAndVisualIsBlockingAudio) {
    auto& audioAndVisualBlockingDirective = m_directive_0_0;
    DirectiveHandlerConfiguration audioAndVisualBlockingHandlerConfig;
    auto audioBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUMS_AUDIO_AND_VISUAL, true);
    audioAndVisualBlockingHandlerConfig[{NAMESPACE_AND_NAME_0_0}] = audioBlockingPolicy;
    auto audioAndVisualBlockingHandler = MockDirectiveHandler::create(audioAndVisualBlockingHandlerConfig);
    ASSERT_TRUE(m_router->addDirectiveHandler(audioAndVisualBlockingHandler));

    auto& audioNonBlockingDirective = m_directive_0_1;
    DirectiveHandlerConfiguration audioNonBlockingHandlerConfig;
    auto audioNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, false);
    audioNonBlockingHandlerConfig[{NAMESPACE_AND_NAME_0_1}] = audioNonBlockingPolicy;
    auto audioNonBlockingHandler = MockDirectiveHandler::create(audioNonBlockingHandlerConfig);
    ASSERT_TRUE(m_router->addDirectiveHandler(audioNonBlockingHandler));

    auto& noneMediumsNoBlockingDirective = m_directive_0_2;
    DirectiveHandlerConfiguration handler3Config;
    auto noneMediums = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    handler3Config[{NAMESPACE_AND_NAME_0_2}] = noneMediums;
    auto noneMediumsNonBlockingHandler = MockDirectiveHandler::create(handler3Config);
    ASSERT_TRUE(m_router->addDirectiveHandler(noneMediumsNonBlockingHandler));

    // On audioAndVisualBlockingDirective, expect preHandle and handle. Then cancel, as it wasn't completed.
    EXPECT_CALL(*(audioAndVisualBlockingHandler.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(audioAndVisualBlockingHandler.get()), preHandleDirective(audioAndVisualBlockingDirective, _))
        .Times(1);
    EXPECT_CALL(*(audioAndVisualBlockingHandler.get()), handleDirective(MESSAGE_ID_0_0))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(*(audioAndVisualBlockingHandler.get()), cancelDirective(_)).Times(1);

    // On audioNonBlockingDirective, expect only preHandle as it's blocked.
    EXPECT_CALL(*(audioNonBlockingHandler.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(audioNonBlockingHandler.get()), preHandleDirective(audioNonBlockingDirective, _)).Times(1);
    EXPECT_CALL(*(audioNonBlockingHandler.get()), handleDirective(MESSAGE_ID_0_1)).Times(0);
    EXPECT_CALL(*(audioNonBlockingHandler.get()), cancelDirective(_)).Times(1);

    // On noneMediumsNoBlockingDirective, expect completion.
    EXPECT_CALL(*(noneMediumsNonBlockingHandler.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(noneMediumsNonBlockingHandler.get()), preHandleDirective(noneMediumsNoBlockingDirective, _)).Times(1);
    EXPECT_CALL(*(noneMediumsNonBlockingHandler.get()), handleDirective(MESSAGE_ID_0_2)).Times(1);
    EXPECT_CALL(*(noneMediumsNonBlockingHandler.get()), cancelDirective(_)).Times(0);

    m_processor->setDialogRequestId(DIALOG_REQUEST_ID_0);

    ASSERT_TRUE(m_processor->onDirective(audioAndVisualBlockingDirective));
    ASSERT_TRUE(m_processor->onDirective(audioNonBlockingDirective));
    ASSERT_TRUE(m_processor->onDirective(noneMediumsNoBlockingDirective));

    noneMediumsNonBlockingHandler->waitUntilCompleted();
}

/**
 * Verify that a blocking @c AVSDirective is not blocking
 * a future @c AVSDirective on a different @c Medium.
 */
TEST_F(DirectiveProcessorTest, testDifferentMediums) {
    auto& audioBlockingDirective = m_directive_0_0;
    DirectiveHandlerConfiguration audioBlockingHandlerConfig;
    audioBlockingHandlerConfig[{NAMESPACE_AND_NAME_0_0}] = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, true);
    auto audioBlockingHandler = MockDirectiveHandler::create(audioBlockingHandlerConfig);
    ASSERT_TRUE(m_router->addDirectiveHandler(audioBlockingHandler));

    auto& visualBlockingDirective = m_directive_0_1;
    DirectiveHandlerConfiguration visualBlockingHandlerConfig;
    visualBlockingHandlerConfig[{NAMESPACE_AND_NAME_0_1}] = BlockingPolicy(BlockingPolicy::MEDIUM_VISUAL, true);
    auto visualBlockingHandler = MockDirectiveHandler::create(visualBlockingHandlerConfig);
    ASSERT_TRUE(m_router->addDirectiveHandler(visualBlockingHandler));

    // On audioBlockingDirective, expect preHandle and handle. Then cancel as it wasn't completed.
    EXPECT_CALL(*(audioBlockingHandler.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(audioBlockingHandler.get()), preHandleDirective(audioBlockingDirective, _)).Times(1);
    EXPECT_CALL(*(audioBlockingHandler.get()), handleDirective(MESSAGE_ID_0_0)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*(audioBlockingHandler.get()), cancelDirective(_)).Times(1);

    // On visualBlockingDirective, expect completion.
    EXPECT_CALL(*(visualBlockingHandler.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(visualBlockingHandler.get()), preHandleDirective(visualBlockingDirective, _)).Times(1);
    EXPECT_CALL(*(visualBlockingHandler.get()), handleDirective(MESSAGE_ID_0_1)).Times(1);
    EXPECT_CALL(*(visualBlockingHandler.get()), cancelDirective(_)).Times(0);

    m_processor->setDialogRequestId(DIALOG_REQUEST_ID_0);

    ASSERT_TRUE(m_processor->onDirective(audioBlockingDirective));
    ASSERT_TRUE(m_processor->onDirective(visualBlockingDirective));

    visualBlockingHandler->waitUntilCompleted();
}

/**
 * Verify that when @c MEDIUM_AUDIO and @c MEDIUM_VISUAL
 * have been blocked by two different @c AVSDirectives,
 * When one of the blocking has been completed, only its @c Medium is
 * released.
 */
TEST_F(DirectiveProcessorTest, testReleaseOneMedium) {
    // 4 directives: blocking audio, blocking visual, using audio and using visual
    auto& audioBlockingDirective = m_directive_0_0;
    DirectiveHandlerConfiguration audioBlockingHandlerConfig;
    audioBlockingHandlerConfig[{NAMESPACE_AND_NAME_0_0}] = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, true);
    auto audioBlockingHandler = MockDirectiveHandler::create(audioBlockingHandlerConfig);
    ASSERT_TRUE(m_router->addDirectiveHandler(audioBlockingHandler));

    auto& visualBlockingDirective = m_directive_0_1;
    DirectiveHandlerConfiguration visualBlockingHandlerConfig;
    visualBlockingHandlerConfig[{NAMESPACE_AND_NAME_0_1}] = BlockingPolicy(BlockingPolicy::MEDIUM_VISUAL, true);
    auto visualBlockingHandler = MockDirectiveHandler::create(visualBlockingHandlerConfig);
    ASSERT_TRUE(m_router->addDirectiveHandler(visualBlockingHandler));

    auto& audioBlockingDirective2 = m_directive_0_2;
    DirectiveHandlerConfiguration audioBlockingHandler2Config;
    audioBlockingHandler2Config[{NAMESPACE_AND_NAME_0_2}] = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, true);
    auto audioBlockingHandler2 = MockDirectiveHandler::create(audioBlockingHandler2Config);
    ASSERT_TRUE(m_router->addDirectiveHandler(audioBlockingHandler2));

    auto& visualBlockingDirective2 = m_directive_0_3;
    DirectiveHandlerConfiguration visualBlockingHandler2Config;
    visualBlockingHandler2Config[{NAMESPACE_AND_NAME_0_3}] = BlockingPolicy(BlockingPolicy::MEDIUM_VISUAL, true);
    auto visualBlockingHandler2 = MockDirectiveHandler::create(visualBlockingHandler2Config);
    ASSERT_TRUE(m_router->addDirectiveHandler(visualBlockingHandler2));

    // On audioBlockingDirective, expect preHandle and handle. Then cancel as it wasn't completed.
    EXPECT_CALL(*(audioBlockingHandler.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(audioBlockingHandler.get()), preHandleDirective(audioBlockingDirective, _)).Times(1);
    EXPECT_CALL(*(audioBlockingHandler.get()), handleDirective(MESSAGE_ID_0_0)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*(audioBlockingHandler.get()), cancelDirective(_)).Times(1);

    // On visualBlockingDirective, expect completion.
    EXPECT_CALL(*(visualBlockingHandler.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(visualBlockingHandler.get()), preHandleDirective(visualBlockingDirective, _)).Times(1);
    EXPECT_CALL(*(visualBlockingHandler.get()), handleDirective(MESSAGE_ID_0_1)).Times(1);
    EXPECT_CALL(*(visualBlockingHandler.get()), cancelDirective(_)).Times(0);

    // On audioBlockingDirective2, expect preHandle and handle. Then cancel as it wasn't completed because it's blocked.
    EXPECT_CALL(*(audioBlockingHandler2.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(audioBlockingHandler2.get()), preHandleDirective(audioBlockingDirective2, _)).Times(1);
    EXPECT_CALL(*(audioBlockingHandler2.get()), handleDirective(MESSAGE_ID_0_2)).Times(0);
    EXPECT_CALL(*(audioBlockingHandler2.get()), cancelDirective(_)).Times(1);

    // On visualBlockingDirective2, expect completion.
    EXPECT_CALL(*(visualBlockingHandler2.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(visualBlockingHandler2.get()), preHandleDirective(visualBlockingDirective2, _)).Times(1);
    EXPECT_CALL(*(visualBlockingHandler2.get()), handleDirective(MESSAGE_ID_0_3)).Times(1);
    EXPECT_CALL(*(visualBlockingHandler2.get()), cancelDirective(_)).Times(0);

    m_processor->setDialogRequestId(DIALOG_REQUEST_ID_0);

    ASSERT_TRUE(m_processor->onDirective(audioBlockingDirective));
    ASSERT_TRUE(m_processor->onDirective(visualBlockingDirective));
    ASSERT_TRUE(m_processor->onDirective(audioBlockingDirective2));
    ASSERT_TRUE(m_processor->onDirective(visualBlockingDirective2));

    visualBlockingHandler->waitUntilCompleted();
    visualBlockingHandler2->waitUntilCompleted();
}

/**
 * Verify that a blocked directive with isBlocking=true on the queue is blocking
 * subsequent directives using the same @c Medium.
 */
TEST_F(DirectiveProcessorTest, TestBlockingQueuedDirectivIsBlocking) {
    auto& audioBlockingDirective = m_directive_0_0;
    DirectiveHandlerConfiguration audioBlockingHandlerConfig;
    audioBlockingHandlerConfig[{NAMESPACE_AND_NAME_0_0}] = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, true);
    auto audioBlockingHandler =
        MockDirectiveHandler::create(audioBlockingHandlerConfig, MockDirectiveHandler::DEFAULT_DONE_TIMEOUT_MS);
    ASSERT_TRUE(m_router->addDirectiveHandler(audioBlockingHandler));

    auto& audioAndVisualBlocking = m_directive_0_1;
    DirectiveHandlerConfiguration audioAndVisualBlockingHandlerConfig;
    audioAndVisualBlockingHandlerConfig[{NAMESPACE_AND_NAME_0_1}] =
        BlockingPolicy(BlockingPolicy::MEDIUMS_AUDIO_AND_VISUAL, true);
    auto audioAndVisualBlockingHandler = MockDirectiveHandler::create(audioAndVisualBlockingHandlerConfig);
    ASSERT_TRUE(m_router->addDirectiveHandler(audioAndVisualBlockingHandler));

    auto& visualBlockingDirective = m_directive_0_2;
    DirectiveHandlerConfiguration visualBlockingHandlerConfig;
    visualBlockingHandlerConfig[{NAMESPACE_AND_NAME_0_2}] = BlockingPolicy(BlockingPolicy::MEDIUM_VISUAL, true);
    auto visualBlockingHandler = MockDirectiveHandler::create(visualBlockingHandlerConfig);
    ASSERT_TRUE(m_router->addDirectiveHandler(visualBlockingHandler));

    /*
     * create a none mediums directive to make sure visualBlockingDirective was waiting in the
     * queue after audioAndVisualBlocking
     */
    auto& noneMediumsDirective = m_directive_0_3;
    DirectiveHandlerConfiguration noneMediumsHandlerConfig;
    noneMediumsHandlerConfig[{NAMESPACE_AND_NAME_0_3}] = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    auto noneMediumsHandler = MockDirectiveHandler::create(noneMediumsHandlerConfig);
    ASSERT_TRUE(m_router->addDirectiveHandler(noneMediumsHandler));

    EXPECT_CALL(*(audioBlockingHandler.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(audioBlockingHandler.get()), preHandleDirective(audioBlockingDirective, _)).Times(1);
    EXPECT_CALL(*(audioBlockingHandler.get()), cancelDirective(_)).Times(0);

    EXPECT_CALL(*(audioAndVisualBlockingHandler.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(audioAndVisualBlockingHandler.get()), preHandleDirective(audioAndVisualBlocking, _)).Times(1);
    EXPECT_CALL(*(audioAndVisualBlockingHandler.get()), cancelDirective(_)).Times(0);

    EXPECT_CALL(*(visualBlockingHandler.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(visualBlockingHandler.get()), preHandleDirective(visualBlockingDirective, _)).Times(1);
    EXPECT_CALL(*(visualBlockingHandler.get()), cancelDirective(_)).Times(0);

    EXPECT_CALL(*(noneMediumsHandler.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(noneMediumsHandler.get()), preHandleDirective(noneMediumsDirective, _)).Times(1);
    EXPECT_CALL(*(noneMediumsHandler.get()), cancelDirective(_)).Times(0);

    /*
     * Expect the directives to be handles in the following order:
     * 1. audioBlockingDirective - arrived first
     * 2. noneMediumsHandler - not bloecked by any of the others.
     * 3. audioAndVisualBlocking - waiting for audioBlockingDirective to complete.
     * 4. visualBlockingHandler - waiting for audioAndVisualBlocking to complete
     */
    ::testing::Sequence s1;
    EXPECT_CALL(*(audioBlockingHandler.get()), handleDirective(MESSAGE_ID_0_0)).Times(1).InSequence(s1);
    EXPECT_CALL(*(noneMediumsHandler.get()), handleDirective(MESSAGE_ID_0_3)).Times(1).InSequence(s1);
    EXPECT_CALL(*(audioAndVisualBlockingHandler.get()), handleDirective(MESSAGE_ID_0_1)).Times(1).InSequence(s1);
    EXPECT_CALL(*(visualBlockingHandler.get()), handleDirective(MESSAGE_ID_0_2)).Times(1).InSequence(s1);

    m_processor->setDialogRequestId(DIALOG_REQUEST_ID_0);

    ASSERT_TRUE(m_processor->onDirective(audioBlockingDirective));
    audioBlockingHandler->waitUntilHandling();

    ASSERT_TRUE(m_processor->onDirective(audioAndVisualBlocking));
    audioAndVisualBlockingHandler->waitUntilPreHandling();

    ASSERT_TRUE(m_processor->onDirective(visualBlockingDirective));
    visualBlockingHandler->waitUntilPreHandling();

    // wait until noneMediumsDirective is handled to make sure we have visited the procesingLoop with all of them
    ASSERT_TRUE(m_processor->onDirective(noneMediumsDirective));
    noneMediumsHandler->waitUntilHandling();

    // Now, audioBlockingDirective can be completed.
    audioBlockingHandler->doHandlingCompleted();

    audioAndVisualBlockingHandler->waitUntilCompleted();
    visualBlockingHandler->waitUntilCompleted();
}

/**
 * Verify that a blocked directive with isBlocking=false on the queue is NOT blocking
 * subsequent directives using the same @c Medium.
 */
TEST_F(DirectiveProcessorTest, TestNonBlockingQueuedDirectivIsNotBlocking) {
    auto& audioBlockingDirective = m_directive_0_0;
    DirectiveHandlerConfiguration audioBlockingHandlerConfig;
    audioBlockingHandlerConfig[{NAMESPACE_AND_NAME_0_0}] = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, true);
    auto audioBlockingHandler =
        MockDirectiveHandler::create(audioBlockingHandlerConfig, MockDirectiveHandler::DEFAULT_DONE_TIMEOUT_MS);
    ASSERT_TRUE(m_router->addDirectiveHandler(audioBlockingHandler));

    auto& audioAndVisualNonBlocking = m_directive_0_1;
    DirectiveHandlerConfiguration audioAndVisualNonBlockingHandlerConfig;
    audioAndVisualNonBlockingHandlerConfig[{NAMESPACE_AND_NAME_0_1}] =
        BlockingPolicy(BlockingPolicy::MEDIUMS_AUDIO_AND_VISUAL, false);
    auto audioAndVisualNonBlockingHandler = MockDirectiveHandler::create(audioAndVisualNonBlockingHandlerConfig);
    ASSERT_TRUE(m_router->addDirectiveHandler(audioAndVisualNonBlockingHandler));

    auto& visualBlockingDirective = m_directive_0_2;
    DirectiveHandlerConfiguration visualBlockingHandlerConfig;
    visualBlockingHandlerConfig[{NAMESPACE_AND_NAME_0_2}] = BlockingPolicy(BlockingPolicy::MEDIUM_VISUAL, true);
    auto visualBlockingHandler = MockDirectiveHandler::create(visualBlockingHandlerConfig);
    ASSERT_TRUE(m_router->addDirectiveHandler(visualBlockingHandler));

    // Expect completion on all the directives.
    EXPECT_CALL(*(audioBlockingHandler.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(audioBlockingHandler.get()), preHandleDirective(audioBlockingDirective, _)).Times(1);
    EXPECT_CALL(*(audioBlockingHandler.get()), cancelDirective(_)).Times(0);

    EXPECT_CALL(*(audioAndVisualNonBlockingHandler.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(audioAndVisualNonBlockingHandler.get()), preHandleDirective(audioAndVisualNonBlocking, _)).Times(1);
    EXPECT_CALL(*(audioAndVisualNonBlockingHandler.get()), cancelDirective(_)).Times(0);

    EXPECT_CALL(*(visualBlockingHandler.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(visualBlockingHandler.get()), preHandleDirective(visualBlockingDirective, _)).Times(1);
    EXPECT_CALL(*(visualBlockingHandler.get()), cancelDirective(_)).Times(0);

    /*
     * Expect the third directive to be handled before the second, because it is neither blocked by the first one
     *  - no common mediums. and nor by the second, queued directive, since the second is not a blocking directive.
     */
    ::testing::Sequence s1;
    EXPECT_CALL(*(audioBlockingHandler.get()), handleDirective(MESSAGE_ID_0_0)).Times(1).InSequence(s1);
    EXPECT_CALL(*(visualBlockingHandler.get()), handleDirective(MESSAGE_ID_0_2)).Times(1).InSequence(s1);
    EXPECT_CALL(*(audioAndVisualNonBlockingHandler.get()), handleDirective(MESSAGE_ID_0_1)).Times(1).InSequence(s1);

    m_processor->setDialogRequestId(DIALOG_REQUEST_ID_0);

    ASSERT_TRUE(m_processor->onDirective(audioBlockingDirective));
    audioBlockingHandler->waitUntilHandling();

    ASSERT_TRUE(m_processor->onDirective(audioAndVisualNonBlocking));
    ASSERT_TRUE(m_processor->onDirective(visualBlockingDirective));

    // make sure audioAndVisualNonBlocking has been enqueued.
    audioAndVisualNonBlockingHandler->waitUntilPreHandling();

    /*
     * visualBlockingDirective should be handled as it's not blocked by audioBlockingDirective and
     * audioAndVisualNonBlocking is not blocking.
     */
    visualBlockingHandler->waitUntilHandling();

    // Now, audioBlockingDirective can be completed.
    audioBlockingHandler->doHandlingCompleted();

    audioAndVisualNonBlockingHandler->waitUntilCompleted();
    visualBlockingHandler->waitUntilCompleted();
}

/**
 * Test if the workaround in @c DirectiveProcessor allows the handling of  InteractionModel.NewDialogRequest v1.0
 * directive.
 * TODO: ACSDK-2218: This test should be removed when the workaround is removed.
 */
TEST_F(DirectiveProcessorTest, testNewDialogRequestHandling) {
    auto directivePair = AVSDirective::create(NEW_DIALOG_REQUEST_DIRECTIVE_V0, nullptr, NO_CONTEXT);
    std::shared_ptr<AVSDirective> newDialogRequestDirective = std::move(directivePair.first);

    DirectiveHandlerConfiguration handler0Config;
    handler0Config[NEW_DIALOG_REQUEST_SIGNATURE] = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    std::shared_ptr<MockDirectiveHandler> handler0 = MockDirectiveHandler::create(handler0Config);

    ASSERT_TRUE(m_router->addDirectiveHandler(handler0));

    // The NewDialogRequest directive should be handled by the capability agent.
    EXPECT_CALL(*(handler0.get()), handleDirective(_)).Times(1);

    // Set a different dialogRequestID from the one in the directive.
    m_processor->setDialogRequestId(DIALOG_REQUEST_ID_1);

    // Handle the directive.
    ASSERT_TRUE(m_processor->onDirective(newDialogRequestDirective));

    // Directive should have been handled by CA.
    ASSERT_TRUE(handler0->waitUntilCompleted(TEST_TIMEOUT));
}

}  // namespace test
}  // namespace adsl
}  // namespace alexaClientSDK
