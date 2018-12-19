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

#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>

#include "ADSL/DirectiveRouter.h"
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

/// Generic MessageId used for tests.
static const std::string MESSAGE_ID_1_0("Message_1_0");

/// Generic MessageId used for test.
static const std::string MESSAGE_ID_2_0("Message_2_0");

/// Generic DialogRequestId used for tests.
static const std::string DIALOG_REQUEST_ID_0("DialogRequestId_0");

/// An unparsed directive for test.
static const std::string UNPARSED_DIRECTIVE("unparsedDirectiveForTest");

/// A paylaod for test;
static const std::string PAYLOAD_TEST("payloadForTest");

/// A generic namespace string for tests.
static const std::string NAMESPACE_0("namespace_0");

/// A generic namespace string for tests.
static const std::string NAMESPACE_1("namespace_1");

/// A generic namespace string for tests.
static const std::string NAMESPACE_2("namespace_2");

/// A generic name string for tests.
static const std::string NAME_0("name_0");

/// A generic name string for tests.
static const std::string NAME_1("name_1");

/// A generic name string for tests.
static const std::string NAME_2("name_2");

/// A generic 'any name' string for tests.
static const std::string NAME_ANY("*");

static const std::string TEST_ATTACHMENT_CONTEXT_ID("TEST_ATTACHMENT_CONTEXT_ID");

/// Namespace and name combination for tests.
#define NAMESPACE_AND_NAME_0_0 NAMESPACE_0, NAME_0

/// Namespace and name combination (changing name this time) for tests.
#define NAMESPACE_AND_NAME_0_1 NAMESPACE_0, NAME_1

/// Namespace and name combination (changing name this time) for tests.
#define NAMESPACE_AND_NAME_0_2 NAMESPACE_0, NAME_2

/// Namespace and name combination ('any name' this time) for tests.
#define NAMESPACE_AND_NAME_0_ANY NAMESPACE_0, NAME_ANY

/// Namespace and name combination (changing namespace this time) for tests.
#define NAMESPACE_AND_NAME_1_0 NAMESPACE_1, NAME_0

/// Namespace and name combination (changing namespace this time) for tests.
#define NAMESPACE_AND_NAME_2_0 NAMESPACE_2, NAME_0

/// Namespace and name combination (changing namespace this time) for tests.
#define NAMESPACE_AND_NAME_2_ANY NAMESPACE_2, NAME_ANY

/// Long timeout we only reach when a concurrency test fails.
static const std::chrono::seconds LONG_TIMEOUT(15);

/**
 * DirectiveRouterTest
 */
class DirectiveRouterTest : public ::testing::Test {
public:
    void SetUp() override;
    void TearDown() override;

    /// A DirectiveRouter instance to test with.
    DirectiveRouter m_router;

    /// AttachmentManager with which to create directives.
    std::shared_ptr<AttachmentManager> m_attachmentManager;

    /// Generic @c AVSDirective for tests.
    std::shared_ptr<AVSDirective> m_directive_0_0;

    /// Generic @c AVSDirective for tests.
    std::shared_ptr<AVSDirective> m_directive_0_1;

    /// Generic @c AVSDirective for tests.
    std::shared_ptr<AVSDirective> m_directive_0_2;

    /// Generic @c AVSDirective for tests.
    std::shared_ptr<AVSDirective> m_directive_1_0;

    /// Generic @c AVSDirective for tests.
    std::shared_ptr<AVSDirective> m_directive_2_0;
};

void DirectiveRouterTest::SetUp() {
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
    auto avsMessageHeader_1_0 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_AND_NAME_1_0, MESSAGE_ID_1_0, DIALOG_REQUEST_ID_0);
    m_directive_1_0 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader_1_0, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);
    auto avsMessageHeader_2_0 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_AND_NAME_2_0, MESSAGE_ID_2_0, DIALOG_REQUEST_ID_0);
    m_directive_2_0 = AVSDirective::create(
        UNPARSED_DIRECTIVE, avsMessageHeader_2_0, PAYLOAD_TEST, m_attachmentManager, TEST_ATTACHMENT_CONTEXT_ID);
}

void DirectiveRouterTest::TearDown() {
    m_router.shutdown();
}

/**
 * Check that an un-registered @c AVDirective will not be routed.
 */
TEST_F(DirectiveRouterTest, testUnroutedDirective) {
    ASSERT_FALSE(m_router.handleDirectiveImmediately(m_directive_0_0));
}

/**
 * Register an @c AVSDirective for routing. Exercise routing via @c handleDirectiveImmediately().
 * Expect that the @c AVSDirective is routed.
 */
TEST_F(DirectiveRouterTest, testSettingADirectiveHandler) {
    DirectiveHandlerConfiguration handler0Config;
    handler0Config[{NAMESPACE_AND_NAME_0_0}] = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, false);
    std::shared_ptr<MockDirectiveHandler> handler0 = MockDirectiveHandler::create(handler0Config);
    ASSERT_TRUE(m_router.addDirectiveHandler(handler0));

    EXPECT_CALL(*(handler0.get()), handleDirectiveImmediately(m_directive_0_0)).Times(1);
    EXPECT_CALL(*(handler0.get()), preHandleDirective(_, _)).Times(0);
    EXPECT_CALL(*(handler0.get()), handleDirective(_)).Times(0);
    EXPECT_CALL(*(handler0.get()), cancelDirective(_)).Times(0);
    EXPECT_CALL(*(handler0.get()), onDeregistered()).Times(1);

    ASSERT_TRUE(m_router.handleDirectiveImmediately(m_directive_0_0));
}

/**
 * Register @c AVSDirectives to be routed to different handlers. Exercise routing via @c preHandleDirective().
 * Expect that the @c AVSDirectives make it to their registered handler.
 */
TEST_F(DirectiveRouterTest, testRegisteringMultipeHandler) {
    DirectiveHandlerConfiguration handler0Config;
    auto audioNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, false);
    handler0Config[{NAMESPACE_AND_NAME_0_0}] = audioNonBlockingPolicy;
    std::shared_ptr<MockDirectiveHandler> handler0 = MockDirectiveHandler::create(handler0Config);

    DirectiveHandlerConfiguration handler1Config;
    handler1Config[{NAMESPACE_AND_NAME_0_1}] = audioNonBlockingPolicy;
    std::shared_ptr<MockDirectiveHandler> handler1 = MockDirectiveHandler::create(handler1Config);

    DirectiveHandlerConfiguration handler2Config;
    handler2Config[{NAMESPACE_AND_NAME_0_ANY}] = audioNonBlockingPolicy;
    std::shared_ptr<MockDirectiveHandler> handler2 = MockDirectiveHandler::create(handler2Config);

    DirectiveHandlerConfiguration handler3Config;
    handler3Config[{NAMESPACE_AND_NAME_1_0}] = audioNonBlockingPolicy;
    std::shared_ptr<MockDirectiveHandler> handler3 = MockDirectiveHandler::create(handler3Config);

    DirectiveHandlerConfiguration handler4Config;
    handler4Config[{NAMESPACE_AND_NAME_2_ANY}] = audioNonBlockingPolicy;
    std::shared_ptr<MockDirectiveHandler> handler4 = MockDirectiveHandler::create(handler4Config);

    ASSERT_TRUE(m_router.addDirectiveHandler(handler0));
    ASSERT_TRUE(m_router.addDirectiveHandler(handler1));
    ASSERT_TRUE(m_router.addDirectiveHandler(handler2));
    ASSERT_TRUE(m_router.addDirectiveHandler(handler3));
    ASSERT_TRUE(m_router.addDirectiveHandler(handler4));

    EXPECT_CALL(*(handler0.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler0.get()), preHandleDirective(m_directive_0_0, _)).Times(1);
    EXPECT_CALL(*(handler0.get()), handleDirective(_)).Times(0);
    EXPECT_CALL(*(handler0.get()), cancelDirective(_)).Times(0);
    EXPECT_CALL(*(handler0.get()), onDeregistered()).Times(1);

    EXPECT_CALL(*(handler1.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler1.get()), preHandleDirective(m_directive_0_1, _)).Times(1);
    EXPECT_CALL(*(handler1.get()), handleDirective(_)).Times(0);
    EXPECT_CALL(*(handler1.get()), cancelDirective(_)).Times(0);
    EXPECT_CALL(*(handler1.get()), onDeregistered()).Times(1);

    // Test that wildcard handler2 will get a directive, while other handlers exist within the same namespace
    EXPECT_CALL(*(handler2.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler2.get()), preHandleDirective(m_directive_0_2, _)).Times(1);
    EXPECT_CALL(*(handler2.get()), handleDirective(_)).Times(0);
    EXPECT_CALL(*(handler2.get()), cancelDirective(_)).Times(0);
    EXPECT_CALL(*(handler2.get()), onDeregistered()).Times(1);

    EXPECT_CALL(*(handler3.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler3.get()), preHandleDirective(m_directive_1_0, _)).Times(1);
    EXPECT_CALL(*(handler3.get()), handleDirective(_)).Times(0);
    EXPECT_CALL(*(handler3.get()), cancelDirective(_)).Times(0);
    EXPECT_CALL(*(handler3.get()), onDeregistered()).Times(1);

    // Test that wildcard handler4 will get a directive, when no other handlers exist within the same namespace
    EXPECT_CALL(*(handler4.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler4.get()), preHandleDirective(m_directive_2_0, _)).Times(1);
    EXPECT_CALL(*(handler4.get()), handleDirective(_)).Times(0);
    EXPECT_CALL(*(handler4.get()), cancelDirective(_)).Times(0);
    EXPECT_CALL(*(handler4.get()), onDeregistered()).Times(1);

    ASSERT_TRUE(m_router.preHandleDirective(m_directive_0_0, nullptr));
    ASSERT_TRUE(m_router.preHandleDirective(m_directive_0_1, nullptr));
    ASSERT_TRUE(m_router.preHandleDirective(m_directive_0_2, nullptr));
    ASSERT_TRUE(m_router.preHandleDirective(m_directive_1_0, nullptr));
    ASSERT_TRUE(m_router.preHandleDirective(m_directive_2_0, nullptr));
}

/**
 * Register @c AVSDirectives to be routed to different handlers. Then update the registration by clearing it and
 * replacing it with a new configuration where one of the handlers is removed, one is changed, and one is not changed.
 * Exercise routing via @c handleDirective(). Expect that the @c AVSDirectives are delivered to the last handler they
 * were last assigned to and that false and a @c BlockingPolicy of NONE is returned for the directive whose handler
 * was removed.
 */
TEST_F(DirectiveRouterTest, testRemovingChangingAndNotChangingHandlers) {
    DirectiveHandlerConfiguration handler0Config;
    auto audioBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, true);
    auto audioNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, false);
    handler0Config[{NAMESPACE_AND_NAME_0_0}] = audioNonBlockingPolicy;
    std::shared_ptr<MockDirectiveHandler> handler0 = MockDirectiveHandler::create(handler0Config);

    DirectiveHandlerConfiguration handler1Config;
    handler1Config[{NAMESPACE_AND_NAME_0_1}] = audioNonBlockingPolicy;
    std::shared_ptr<MockDirectiveHandler> handler1 = MockDirectiveHandler::create(handler1Config);

    DirectiveHandlerConfiguration handler2Config;
    handler2Config[{NAMESPACE_AND_NAME_1_0}] = audioNonBlockingPolicy;
    std::shared_ptr<MockDirectiveHandler> handler2 = MockDirectiveHandler::create(handler2Config);

    DirectiveHandlerConfiguration handler3Config;
    handler3Config[{NAMESPACE_AND_NAME_1_0}] = audioBlockingPolicy;
    std::shared_ptr<MockDirectiveHandler> handler3 = MockDirectiveHandler::create(handler3Config);

    EXPECT_CALL(*(handler0.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler0.get()), preHandleDirective(_, _)).Times(0);
    EXPECT_CALL(*(handler0.get()), handleDirective(_)).Times(0);
    EXPECT_CALL(*(handler0.get()), cancelDirective(_)).Times(0);
    EXPECT_CALL(*(handler0.get()), onDeregistered()).Times(1);

    EXPECT_CALL(*(handler1.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler1.get()), preHandleDirective(_, _)).Times(0);
    EXPECT_CALL(*(handler1.get()), handleDirective(MESSAGE_ID_0_1)).WillOnce(Return(true));
    EXPECT_CALL(*(handler1.get()), cancelDirective(_)).Times(0);
    EXPECT_CALL(*(handler1.get()), onDeregistered()).Times(2);

    EXPECT_CALL(*(handler2.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler2.get()), preHandleDirective(_, _)).Times(0);
    EXPECT_CALL(*(handler2.get()), handleDirective(_)).Times(0);
    EXPECT_CALL(*(handler2.get()), cancelDirective(_)).Times(0);
    EXPECT_CALL(*(handler2.get()), onDeregistered()).Times(1);

    EXPECT_CALL(*(handler3.get()), handleDirective(MESSAGE_ID_1_0)).WillOnce(Return(true));

    ASSERT_TRUE(m_router.addDirectiveHandler(handler0));
    ASSERT_TRUE(m_router.addDirectiveHandler(handler1));
    ASSERT_TRUE(m_router.addDirectiveHandler(handler2));

    ASSERT_TRUE(m_router.removeDirectiveHandler(handler0));
    ASSERT_TRUE(m_router.removeDirectiveHandler(handler1));
    ASSERT_TRUE(m_router.removeDirectiveHandler(handler2));

    ASSERT_FALSE(m_router.removeDirectiveHandler(handler0));
    ASSERT_FALSE(m_router.removeDirectiveHandler(handler1));
    ASSERT_FALSE(m_router.removeDirectiveHandler(handler2));

    ASSERT_TRUE(m_router.addDirectiveHandler(handler1));
    ASSERT_TRUE(m_router.addDirectiveHandler(handler3));

    auto policy = m_router.getPolicy(m_directive_0_0);
    ASSERT_FALSE(m_router.handleDirective(m_directive_0_0));
    ASSERT_FALSE(policy.isValid());
    policy = m_router.getPolicy(m_directive_0_1);
    ASSERT_TRUE(m_router.handleDirective(m_directive_0_1));
    ASSERT_EQ(policy, audioNonBlockingPolicy);
    policy = m_router.getPolicy(m_directive_1_0);
    ASSERT_TRUE(m_router.handleDirective(m_directive_1_0));
    ASSERT_EQ(policy, audioBlockingPolicy);
}

/**
 * Register two @c AVSDirectives to be routed to different handlers with different blocking policies. Configure the
 * mock handlers to return false from @c handleDirective(). Exercise routing via handleDirective(). Expect that
 * @c DirectiveRouter::handleDirective() returns @c false and BlockingPolicy::nonePolicy() to indicate failure.
 */
TEST_F(DirectiveRouterTest, testResultOfHandleDirectiveFailure) {
    DirectiveHandlerConfiguration handler0Config;
    handler0Config[{NAMESPACE_AND_NAME_0_0}] = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, false);
    std::shared_ptr<MockDirectiveHandler> handler0 = MockDirectiveHandler::create(handler0Config);

    DirectiveHandlerConfiguration handler1Config;
    handler1Config[{NAMESPACE_AND_NAME_0_1}] = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, true);
    std::shared_ptr<MockDirectiveHandler> handler1 = MockDirectiveHandler::create(handler1Config);

    ASSERT_TRUE(m_router.addDirectiveHandler(handler0));
    ASSERT_TRUE(m_router.addDirectiveHandler(handler1));

    EXPECT_CALL(*(handler0.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler0.get()), preHandleDirective(_, _)).Times(0);
    EXPECT_CALL(*(handler0.get()), handleDirective(MESSAGE_ID_0_0)).WillOnce(Return(false));
    EXPECT_CALL(*(handler0.get()), cancelDirective(_)).Times(0);
    EXPECT_CALL(*(handler0.get()), onDeregistered()).Times(1);

    EXPECT_CALL(*(handler1.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler1.get()), preHandleDirective(_, _)).Times(0);
    EXPECT_CALL(*(handler1.get()), handleDirective(MESSAGE_ID_0_1)).WillOnce(Return(false));
    EXPECT_CALL(*(handler1.get()), cancelDirective(_)).Times(0);
    EXPECT_CALL(*(handler1.get()), onDeregistered()).Times(1);

    ASSERT_FALSE(m_router.handleDirective(m_directive_0_0));
    ASSERT_FALSE(m_router.handleDirective(m_directive_0_1));
}

/**
 * Register an @c AVSDirective for handling.  Invoke preHandleDirective() on a new thread and block its return
 * until a subsequent invocation of @c handleDirective() has started. Expect the blocked call to preHandleDirective()
 * to complete quickly.
 */
TEST_F(DirectiveRouterTest, testHandlerMethodsCanRunConcurrently) {
    DirectiveHandlerConfiguration handler0Config;
    handler0Config[{NAMESPACE_AND_NAME_0_0}] = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, true);
    std::shared_ptr<MockDirectiveHandler> handler0 = MockDirectiveHandler::create(handler0Config);

    ASSERT_TRUE(m_router.addDirectiveHandler(handler0));

    std::promise<void> waker;
    auto sleeper = waker.get_future();

    auto sleeperFunction = [&sleeper]() {
        ASSERT_EQ(sleeper.wait_for(LONG_TIMEOUT), std::future_status::ready)
            << "ERROR: Timeout reached while waiting for concurrent handler.";
    };

    auto wakerFunction = [&waker]() {
        waker.set_value();
        return true;
    };

    EXPECT_CALL(*(handler0.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler0.get()), preHandleDirective(m_directive_0_0, _)).WillOnce(InvokeWithoutArgs(sleeperFunction));
    EXPECT_CALL(*(handler0.get()), handleDirective(MESSAGE_ID_0_0)).WillOnce(InvokeWithoutArgs(wakerFunction));
    EXPECT_CALL(*(handler0.get()), cancelDirective(_)).Times(0);
    EXPECT_CALL(*(handler0.get()), onDeregistered()).Times(1);

    std::thread sleeperThread([this]() { ASSERT_TRUE(m_router.preHandleDirective(m_directive_0_0, nullptr)); });
    ASSERT_TRUE(m_router.handleDirective(m_directive_0_0));
    auto policy = m_router.getPolicy(m_directive_0_0);
    ASSERT_EQ(policy, BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, true));
    sleeperThread.join();
}

}  // namespace test
}  // namespace adsl
}  // namespace alexaClientSDK
