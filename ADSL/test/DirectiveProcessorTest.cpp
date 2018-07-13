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

static const std::string TEST_ATTACHMENT_CONTEXT_ID("TEST_ATTACHMENT_CONTEXT_ID");

/// Namespace and name combination for tests.
#define NAMESPACE_AND_NAME_0_0 NAMESPACE_0, NAME_0

/// Namespace and name combination (changing name this time) for tests.
#define NAMESPACE_AND_NAME_0_1 NAMESPACE_0, NAME_1

/// Namespace and name combination (changing namespace this time) for tests.
#define NAMESPACE_AND_NAME_1_0 NAMESPACE_1, NAME_0

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
    handler0Config[{NAMESPACE_AND_NAME_0_0}] = BlockingPolicy::NON_BLOCKING;
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
 * Register a @c NON_BLOCKING @c DirectiveHandler.  Send an @c AVSDirective that matches the registered handler.
 * Expect that @c preHandleDirective() and @c handleDirective() are called.
 */
TEST_F(DirectiveProcessorTest, testSendNonBlocking) {
    DirectiveHandlerConfiguration handler0Config;
    handler0Config[{NAMESPACE_AND_NAME_0_0}] = BlockingPolicy::NON_BLOCKING;
    std::shared_ptr<MockDirectiveHandler> handler0 = MockDirectiveHandler::create(handler0Config);

    ASSERT_TRUE(m_router->addDirectiveHandler(handler0));

    EXPECT_CALL(*(handler0.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler0.get()), preHandleDirective(m_directive_0_0, _)).Times(1);
    EXPECT_CALL(*(handler0.get()), handleDirective(MESSAGE_ID_0_0)).Times(1);
    EXPECT_CALL(*(handler0.get()), cancelDirective(_)).Times(0);

    m_processor->setDialogRequestId(DIALOG_REQUEST_ID_0);
    ASSERT_TRUE(m_processor->onDirective(m_directive_0_0));

    ASSERT_TRUE(handler0->waitUntilCompleted());
}

/**
 * Test sending a blocking and then a non-blocking directive.  Expect that @c preHandleDirective() and
 * @c handleDirective() is called for each.
 */
TEST_F(DirectiveProcessorTest, testSendBlockingThenNonBlocking) {
    DirectiveHandlerConfiguration handler0Config;
    handler0Config[{NAMESPACE_AND_NAME_0_0}] = BlockingPolicy::BLOCKING;
    std::shared_ptr<MockDirectiveHandler> handler0 = MockDirectiveHandler::create(handler0Config);

    DirectiveHandlerConfiguration handler1Config;
    handler1Config[{NAMESPACE_AND_NAME_0_1}] = BlockingPolicy::NON_BLOCKING;
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
    handler1Config[{NAMESPACE_AND_NAME_0_1}] = BlockingPolicy::NON_BLOCKING;
    std::shared_ptr<MockDirectiveHandler> handler1 = MockDirectiveHandler::create(handler1Config);

    DirectiveHandlerConfiguration handler2Config;
    handler2Config[{NAMESPACE_AND_NAME_1_0}] = BlockingPolicy::NON_BLOCKING;
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
    longRunningHandlerConfig[{NAMESPACE_AND_NAME_0_0}] = BlockingPolicy::BLOCKING;
    auto longRunningHandler =
        MockDirectiveHandler::create(longRunningHandlerConfig, MockDirectiveHandler::DEFAULT_DONE_TIMEOUT_MS);

    DirectiveHandlerConfiguration handler1Config;
    handler1Config[{NAMESPACE_AND_NAME_0_1}] = BlockingPolicy::NON_BLOCKING;
    auto handler1 = MockDirectiveHandler::create(handler1Config);

    DirectiveHandlerConfiguration handler2Config;
    handler2Config[{NAMESPACE_AND_NAME_1_0}] = BlockingPolicy::NON_BLOCKING;
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

}  // namespace test
}  // namespace adsl
}  // namespace alexaClientSDK
