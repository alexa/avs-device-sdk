/*
 * DirectiveRouterTest.cpp
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

#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "ADSL/DirectiveRouter.h"
#include "MockAttachmentManager.h"
#include "MockDirectiveHandler.h"
#include "MockDirectiveHandlerResult.h"

using namespace ::testing;

namespace alexaClientSDK {
namespace adsl {
namespace test {

using namespace avsCommon;

/// Generic messageId used for tests.
static const std::string MESSAGE_ID_0_0("Message_0_0");

/// Generic MessageId used for test.
static const std::string MESSAGE_ID_0_1("Message_0_1");

/// Generic MessageId used for tests.
static const std::string MESSAGE_ID_1_0("Message_1_0");

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

/// A generic name string for tests.
static const std::string NAME_0("name_0");

/// A generic name string for tests.
static const std::string NAME_1("name_1");

/// Namespace and name combination for tests.
#define NAMESPACE_AND_NAME_0_0 NAMESPACE_0, NAME_0

/// Namespace and name combination (changing name this time) for tests.
#define NAMESPACE_AND_NAME_0_1 NAMESPACE_0, NAME_1

/// Namespace and name combination (changing namespace this time) for tests.
#define NAMESPACE_AND_NAME_1_0 NAMESPACE_1, NAME_0

/// Long timeout we only reach when a concurrency test fails.
static const std::chrono::seconds LONG_TIMEOUT(15);

/**
 * DirectiveRouterTest
 */
class DirectiveRouterTest : public ::testing::Test {
public:
    void SetUp() override;

    /// A DirectiveRouter instance to test with.
    DirectiveRouter m_router;

    /// Mock AttachmentManager with which to create directives.
    std::shared_ptr<AttachmentManagerInterface> m_attachmentManager;

    /// Generic mock @c DirectiveHandler for tests.
    std::shared_ptr<MockDirectiveHandler> m_handler0;

    /// Generic mock @c DirectiveHandler for tests.
    std::shared_ptr<MockDirectiveHandler> m_handler1;

    /// Generic mock @c DirectiveHandler for tests.
    std::shared_ptr<MockDirectiveHandler> m_handler2;

    /// Generic @c AVSDirective for tests.
    std::shared_ptr<AVSDirective> m_directive_0_0;

    /// Generic @c AVSDirective for tests.
    std::shared_ptr<AVSDirective> m_directive_0_1;

    /// Generic @c AVSDirective for tests.
    std::shared_ptr<AVSDirective> m_directive_1_0;
};

void DirectiveRouterTest::SetUp() {
    m_attachmentManager = std::make_shared<NiceMock<MockAttachmentManager>>();

    m_handler0 = MockDirectiveHandler::create();
    m_handler1 = MockDirectiveHandler::create();
    m_handler2 = MockDirectiveHandler::create();

    auto avsMessageHeader_0_0 = std::make_shared<AVSMessageHeader>(
            NAMESPACE_AND_NAME_0_0, MESSAGE_ID_0_0, DIALOG_REQUEST_ID_0);
    m_directive_0_0 = AVSDirective::create(
            UNPARSED_DIRECTIVE, avsMessageHeader_0_0, PAYLOAD_TEST, m_attachmentManager);
    auto avsMessageHeader_0_1 = std::make_shared<AVSMessageHeader>(
            NAMESPACE_AND_NAME_0_1, MESSAGE_ID_0_1, DIALOG_REQUEST_ID_0);
    m_directive_0_1 = AVSDirective::create(
            UNPARSED_DIRECTIVE, avsMessageHeader_0_1, PAYLOAD_TEST, m_attachmentManager);
    auto avsMessageHeader_1_0 = std::make_shared<AVSMessageHeader>(
            NAMESPACE_AND_NAME_1_0, MESSAGE_ID_1_0, DIALOG_REQUEST_ID_0);
    m_directive_1_0 = AVSDirective::create(
            UNPARSED_DIRECTIVE, avsMessageHeader_1_0, PAYLOAD_TEST, m_attachmentManager);
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
    ASSERT_TRUE(m_router.addDirectiveHandlers({
            {{NAMESPACE_AND_NAME_0_0}, {m_handler0, BlockingPolicy::NON_BLOCKING}}
    }));

    EXPECT_CALL(*(m_handler0.get()), handleDirectiveImmediately(m_directive_0_0)).Times(1);
    EXPECT_CALL(*(m_handler0.get()), preHandleDirective(_, _)).Times(0);
    EXPECT_CALL(*(m_handler0.get()), handleDirective(_)).Times(0);
    EXPECT_CALL(*(m_handler0.get()), cancelDirective(_)).Times(0);
    EXPECT_CALL(*(m_handler0.get()), onDeregistered()).Times(1);

    ASSERT_TRUE(m_router.handleDirectiveImmediately(m_directive_0_0));
}

/**
 * Register @c AVSDirectives to be routed to different handlers. Exercise routing via @c preHandleDirective().
 * Expect that the @c AVSDirectives make it to their registered handler.
 */
TEST_F(DirectiveRouterTest, testRegisteringMultipeHandler) {
    ASSERT_TRUE(m_router.addDirectiveHandlers({
          {{NAMESPACE_AND_NAME_0_0}, {m_handler0, BlockingPolicy::NON_BLOCKING}},
          {{NAMESPACE_AND_NAME_0_1}, {m_handler1, BlockingPolicy::NON_BLOCKING}},
          {{NAMESPACE_AND_NAME_1_0}, {m_handler2, BlockingPolicy::NON_BLOCKING}}
    }));

    EXPECT_CALL(*(m_handler0.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(m_handler0.get()), preHandleDirective(m_directive_0_0, _)).Times(1);
    EXPECT_CALL(*(m_handler0.get()), handleDirective(_)).Times(0);
    EXPECT_CALL(*(m_handler0.get()), cancelDirective(_)).Times(0);
    EXPECT_CALL(*(m_handler0.get()), onDeregistered()).Times(1);

    EXPECT_CALL(*(m_handler1.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(m_handler1.get()), preHandleDirective(m_directive_0_1, _)).Times(1);
    EXPECT_CALL(*(m_handler1.get()), handleDirective(_)).Times(0);
    EXPECT_CALL(*(m_handler1.get()), cancelDirective(_)).Times(0);
    EXPECT_CALL(*(m_handler1.get()), onDeregistered()).Times(1);

    EXPECT_CALL(*(m_handler2.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(m_handler2.get()), preHandleDirective(m_directive_1_0, _)).Times(1);
    EXPECT_CALL(*(m_handler2.get()), handleDirective(_)).Times(0);
    EXPECT_CALL(*(m_handler2.get()), cancelDirective(_)).Times(0);
    EXPECT_CALL(*(m_handler2.get()), onDeregistered()).Times(1);

    ASSERT_TRUE(m_router.preHandleDirective(m_directive_0_0, nullptr));
    ASSERT_TRUE(m_router.preHandleDirective(m_directive_0_1, nullptr));
    ASSERT_TRUE(m_router.preHandleDirective(m_directive_1_0, nullptr));
}

/**
 * Register @c AVSDirectives to be routed to different handlers. Then update the registration by clearing it and
 * replacing it with a new configuration where one of the handlers was removed, one is changed, and one is not changed.
 * Exercise routing via @c handleDirective(). Expect that the @c AVSDirectives are delivered to the last handler they
 * were last assigned to and that false and a @c BlockingPolicy of NONE is returned for the directive whose handler
 * was removed.
 */
TEST_F(DirectiveRouterTest, testRemovingChangingAndNotChangingHandlers) {
    DirectiveHandlerConfiguration config1 = {
          {{NAMESPACE_AND_NAME_0_0}, {m_handler0, BlockingPolicy::NON_BLOCKING}},
          {{NAMESPACE_AND_NAME_0_1}, {m_handler1, BlockingPolicy::NON_BLOCKING}},
          {{NAMESPACE_AND_NAME_1_0}, {m_handler2, BlockingPolicy::NON_BLOCKING}}
    };
    DirectiveHandlerConfiguration config2 = {
          {{NAMESPACE_AND_NAME_0_1}, {m_handler1, BlockingPolicy::NON_BLOCKING}},
          {{NAMESPACE_AND_NAME_1_0}, {m_handler0, BlockingPolicy::BLOCKING}}
    };

    EXPECT_CALL(*(m_handler0.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(m_handler0.get()), preHandleDirective(_, _)).Times(0);
    EXPECT_CALL(*(m_handler0.get()), handleDirective(MESSAGE_ID_1_0)).WillOnce(Return(true));
    EXPECT_CALL(*(m_handler0.get()), cancelDirective(_)).Times(0);
    EXPECT_CALL(*(m_handler0.get()), onDeregistered()).Times(2);

    EXPECT_CALL(*(m_handler1.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(m_handler1.get()), preHandleDirective(_, _)).Times(0);
    EXPECT_CALL(*(m_handler1.get()), handleDirective(MESSAGE_ID_0_1)).WillOnce(Return(true));
    EXPECT_CALL(*(m_handler1.get()), cancelDirective(_)).Times(0);
    EXPECT_CALL(*(m_handler1.get()), onDeregistered()).Times(2);

    EXPECT_CALL(*(m_handler2.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(m_handler2.get()), preHandleDirective(_, _)).Times(0);
    EXPECT_CALL(*(m_handler2.get()), handleDirective(_)).Times(0);
    EXPECT_CALL(*(m_handler2.get()), cancelDirective(_)).Times(0);
    EXPECT_CALL(*(m_handler2.get()), onDeregistered()).Times(1);

    ASSERT_TRUE(m_router.addDirectiveHandlers(config1));
    ASSERT_FALSE(m_router.addDirectiveHandlers(config2));
    ASSERT_TRUE(m_router.removeDirectiveHandlers(config1));
    ASSERT_FALSE(m_router.removeDirectiveHandlers(config2));
    ASSERT_TRUE(m_router.addDirectiveHandlers(config2));

    auto policy = BlockingPolicy::NONE;
    ASSERT_FALSE(m_router.handleDirective(m_directive_0_0, &policy));
    ASSERT_EQ(policy, BlockingPolicy::NONE);
    ASSERT_TRUE(m_router.handleDirective(m_directive_0_1, &policy));
    ASSERT_EQ(policy, BlockingPolicy::NON_BLOCKING);
    ASSERT_TRUE(m_router.handleDirective(m_directive_1_0, &policy));
    ASSERT_EQ(policy, BlockingPolicy::BLOCKING);
}

/**
 * Register two @c AVSDirectives to be routed to different handlers with different blocking policies. Configure the
 * mock handlers to return false from @c handleDirective(). Exercise routing via handleDirective(). Expect that
 * @c DirectiveRouter::handleDirective() returns @c false and BlockingPolicy::NONE to indicate failure.
 */
TEST_F(DirectiveRouterTest, testResultOfHandleDirectiveFailure) {
    ASSERT_TRUE(m_router.addDirectiveHandlers({
          {{NAMESPACE_AND_NAME_0_0}, {m_handler0, BlockingPolicy::NON_BLOCKING}},
          {{NAMESPACE_AND_NAME_0_1}, {m_handler1, BlockingPolicy::BLOCKING}}
    }));

    EXPECT_CALL(*(m_handler0.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(m_handler0.get()), preHandleDirective(_, _)).Times(0);
    EXPECT_CALL(*(m_handler0.get()), handleDirective(MESSAGE_ID_0_0)).WillOnce(Return(false));
    EXPECT_CALL(*(m_handler0.get()), cancelDirective(_)).Times(0);
    EXPECT_CALL(*(m_handler0.get()), onDeregistered()).Times(1);

    EXPECT_CALL(*(m_handler1.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(m_handler1.get()), preHandleDirective(_, _)).Times(0);
    EXPECT_CALL(*(m_handler1.get()), handleDirective(MESSAGE_ID_0_1)).WillOnce(Return(false));
    EXPECT_CALL(*(m_handler1.get()), cancelDirective(_)).Times(0);
    EXPECT_CALL(*(m_handler1.get()), onDeregistered()).Times(1);

    auto policy = BlockingPolicy::NONE;
    ASSERT_FALSE(m_router.handleDirective(m_directive_0_0, &policy));
    ASSERT_EQ(policy, BlockingPolicy::NONE);
    ASSERT_FALSE(m_router.handleDirective(m_directive_0_1, &policy));
    ASSERT_EQ(policy, BlockingPolicy::NONE);
}

/**
 * Register an @c AVSDirective for handling.  Invoke preHandleDirective() on a new thread and block its return
 * until a subsequent invocation of @c handleDirective() has started. Expect the blocked call to preHandleDirective()
 * to complete quickly.
 */
TEST_F(DirectiveRouterTest, testHandlerMethodsCanRunConcurrently) {
    ASSERT_TRUE(m_router.addDirectiveHandlers({
            {{NAMESPACE_AND_NAME_0_0}, {m_handler0, BlockingPolicy::BLOCKING}}
    }));

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

    EXPECT_CALL(*(m_handler0.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(m_handler0.get()), preHandleDirective(m_directive_0_0, _))
            .WillOnce(InvokeWithoutArgs(sleeperFunction));
    EXPECT_CALL(*(m_handler0.get()), handleDirective(MESSAGE_ID_0_0))
            .WillOnce(InvokeWithoutArgs(wakerFunction));
    EXPECT_CALL(*(m_handler0.get()), cancelDirective(_)).Times(0);
    EXPECT_CALL(*(m_handler0.get()), onDeregistered()).Times(1);

    std::thread sleeperThread([this]() {
        ASSERT_TRUE(m_router.preHandleDirective(m_directive_0_0, nullptr));
    });
    auto policy = BlockingPolicy::NONE;
    ASSERT_TRUE(m_router.handleDirective(m_directive_0_0, &policy));
    ASSERT_EQ(policy, BlockingPolicy::BLOCKING);
    sleeperThread.join();
}

} // namespace test
} // namespace adsl
} // namespace alexaClientSDK
