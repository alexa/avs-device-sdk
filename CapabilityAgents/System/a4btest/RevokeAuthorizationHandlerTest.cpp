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

#include <chrono>
#include <condition_variable>
#include <thread>

#include <gtest/gtest.h>
#include <rapidjson/document.h>

#include <ADSL/DirectiveSequencer.h>
#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockRevokeAuthorizationObserver.h>

#include "System/RevokeAuthorizationHandler.h"

using namespace testing;

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {
namespace test {

using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::avs;

/// This is a string for the namespace we are testing for.
static const std::string REVOKE_NAMESPACE = "System";

/// This is a string for the correct name the RevokeAuthorization directive uses.
static const std::string REVOKE_DIRECTIVE_NAME = "RevokeAuthorization";

/// This is the full payload expected to come from AVS.
static const std::string REVOKE_PAYLOAD = "{}";

/// This is the string for the message ID used in the directive.
static const std::string REVOKE_MESSAGE_ID = "ABC123DEF";

/// This is a short delay tests can use when waiting for a directive.
static const std::chrono::nanoseconds SHORT_DIRECTIVE_DELAY =
    std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(50));

/// This is the condition variable to be used to control the exit of the test case.
std::condition_variable exitTrigger;

/**
 * Helper function that will notify the exit trigger.
 */
static void notifyExit() {
    exitTrigger.notify_all();
}

/**
 * Helper function to construct a directive.
 */
static std::shared_ptr<AVSDirective> createDirective() {
    auto revokeDirectiveHeader =
        std::make_shared<AVSMessageHeader>(REVOKE_NAMESPACE, REVOKE_DIRECTIVE_NAME, REVOKE_MESSAGE_ID);
    auto attachmentManager = std::make_shared<StrictMock<attachment::test::MockAttachmentManager>>();
    return AVSDirective::create("", revokeDirectiveHeader, REVOKE_PAYLOAD, attachmentManager, "");
}

/// Test harness for @c RevokeAuthorizationHandler class.
class RevokeAuthorizationHandlerTest : public ::testing::Test {
public:
    /// Set up the test harness for running a test.
    void SetUp() override;

protected:
    /// Mocked Revoke Authorization Observer. Note that we make it a strict mock to ensure we test the flow completely.
    std::shared_ptr<StrictMock<MockRevokeAuthorizationObserver>> m_mockRevokeAuthorizationObserver;
    /// Mocked Exception Encountered Sender. Note that we make it a strict mock to ensure we test the flow completely.
    std::shared_ptr<StrictMock<MockExceptionEncounteredSender>> m_mockExceptionEncounteredSender;
};

void RevokeAuthorizationHandlerTest::SetUp() {
    // Create strict mocks, to ensure no unexpected calls are made.
    m_mockRevokeAuthorizationObserver = std::make_shared<StrictMock<MockRevokeAuthorizationObserver>>();
    m_mockExceptionEncounteredSender = std::make_shared<StrictMock<MockExceptionEncounteredSender>>();
}

/**
 * This case tests if @c RevokeAuthorizationHandler basic create function works properly
 */
TEST_F(RevokeAuthorizationHandlerTest, createSuccessfully) {
    ASSERT_NE(nullptr, RevokeAuthorizationHandler::create(m_mockExceptionEncounteredSender));
}

/**
 * This case tests if possible @c nullptr parameters passed to @c RevokeAuthorizationHandler::create are handled
 * properly.
 */
TEST_F(RevokeAuthorizationHandlerTest, createWithError) {
    ASSERT_EQ(nullptr, RevokeAuthorizationHandler::create(nullptr));
}

/**
 * This case tests if a directive is handled properly and passed to the registered observer.
 * It uses the directive sequencer to ensure getCapabilities properly identifies the namespace/directive name.
 */
TEST_F(RevokeAuthorizationHandlerTest, handleDirectiveProperly) {
    auto revokeHandler = RevokeAuthorizationHandler::create(m_mockExceptionEncounteredSender);
    ASSERT_NE(nullptr, revokeHandler);

    // Add our mock observer to verify observers are called.
    ASSERT_TRUE(revokeHandler->addObserver(m_mockRevokeAuthorizationObserver));

    // Sanity check that the same observer isn't added twice.
    ASSERT_FALSE(revokeHandler->addObserver(m_mockRevokeAuthorizationObserver));

    auto directiveSequencer = adsl::DirectiveSequencer::create(m_mockExceptionEncounteredSender);
    directiveSequencer->addDirectiveHandler(revokeHandler);

    std::mutex exitMutex;
    std::unique_lock<std::mutex> exitLock(exitMutex);
    EXPECT_CALL(*m_mockRevokeAuthorizationObserver, onRevokeAuthorization()).WillOnce(InvokeWithoutArgs(notifyExit));
    directiveSequencer->onDirective(createDirective());
    ASSERT_EQ(std::cv_status::no_timeout, exitTrigger.wait_for(exitLock, SHORT_DIRECTIVE_DELAY));
    directiveSequencer->shutdown();
}

/**
 * This case tests if handleDirectiveImmediately handles the directive properly.
 */
TEST_F(RevokeAuthorizationHandlerTest, handleDirectiveImmediatelyProperly) {
    auto revokeHandler = RevokeAuthorizationHandler::create(m_mockExceptionEncounteredSender);
    ASSERT_NE(nullptr, revokeHandler);

    // Add our mock observer to verify observers are called.
    ASSERT_TRUE(revokeHandler->addObserver(m_mockRevokeAuthorizationObserver));

    EXPECT_CALL(*m_mockRevokeAuthorizationObserver, onRevokeAuthorization());
    revokeHandler->handleDirectiveImmediately(createDirective());
}

/**
 * This case tests if handleDirectiveImmediately handles a @c nullptr directive correctly and does not notify observers.
 */
TEST_F(RevokeAuthorizationHandlerTest, handleDirectiveImmediatelyNullDirective) {
    auto revokeHandler = RevokeAuthorizationHandler::create(m_mockExceptionEncounteredSender);
    ASSERT_NE(nullptr, revokeHandler);

    // Add our strict mock observer to verify observers are not called. Any call to a strict mock results in failure.
    ASSERT_TRUE(revokeHandler->addObserver(m_mockRevokeAuthorizationObserver));

    revokeHandler->handleDirectiveImmediately(nullptr);
}

/**
 * This case tests if handleDirective handles a @c nullptr DirectiveInfo correctly and does not notify observers.
 */
TEST_F(RevokeAuthorizationHandlerTest, handleDirectiveNullDirectiveInfo) {
    auto revokeHandler = RevokeAuthorizationHandler::create(m_mockExceptionEncounteredSender);
    ASSERT_NE(nullptr, revokeHandler);

    // Add our strict mock observer to verify observers are not called. Any call to a strict mock results in failure.
    ASSERT_TRUE(revokeHandler->addObserver(m_mockRevokeAuthorizationObserver));

    revokeHandler->handleDirective(nullptr);
}

/**
 * This case tests if cancelDirective handles a @c nullptr DirectiveInfo safely.
 */
TEST_F(RevokeAuthorizationHandlerTest, cancelDirectiveNullDirectiveInfo) {
    auto revokeHandler = RevokeAuthorizationHandler::create(m_mockExceptionEncounteredSender);
    ASSERT_NE(nullptr, revokeHandler);

    // Add our strict mock observer to verify observers are not called. Any call to a strict mock results in failure.
    ASSERT_TRUE(revokeHandler->addObserver(m_mockRevokeAuthorizationObserver));

    revokeHandler->cancelDirective(nullptr);
}

/**
 * This case tests when a registered observer is removed, it does not receive notifications.
 */
TEST_F(RevokeAuthorizationHandlerTest, removeObserverSuccessfully) {
    auto revokeHandler = RevokeAuthorizationHandler::create(m_mockExceptionEncounteredSender);
    ASSERT_NE(nullptr, revokeHandler);

    // Add our mock observer to the handler.
    revokeHandler->addObserver(m_mockRevokeAuthorizationObserver);

    auto directiveSequencer = adsl::DirectiveSequencer::create(m_mockExceptionEncounteredSender);
    directiveSequencer->addDirectiveHandler(revokeHandler);

    // Remove our mock observer so that it should not be notified.
    ASSERT_TRUE(revokeHandler->removeObserver(m_mockRevokeAuthorizationObserver));

    // Sanity check that we can safely attempt to remove it again.
    ASSERT_FALSE(revokeHandler->removeObserver(m_mockRevokeAuthorizationObserver));

    std::mutex exitMutex;
    std::unique_lock<std::mutex> exitLock(exitMutex);
    // No EXPECT defined, so test should fail if a call is made.
    directiveSequencer->onDirective(createDirective());
    std::this_thread::sleep_for(SHORT_DIRECTIVE_DELAY);
    directiveSequencer->shutdown();
}

/**
 * Test to verify the preHandleDirective method doesn't really take action, as there is no pre-handle
 * work supported at this time.
 */
TEST_F(RevokeAuthorizationHandlerTest, preHandleDirectiveTest) {
    auto revokeHandler = RevokeAuthorizationHandler::create(m_mockExceptionEncounteredSender);
    ASSERT_NE(nullptr, revokeHandler);

    revokeHandler->preHandleDirective(nullptr);
}

/**
 * Test to verify the addObserver method successfully ignores @c nullptr inputs.
 */
TEST_F(RevokeAuthorizationHandlerTest, addObserverIgnoreNullPtr) {
    auto revokeHandler = RevokeAuthorizationHandler::create(m_mockExceptionEncounteredSender);
    ASSERT_NE(nullptr, revokeHandler);

    // Ensure the nullptr wasn't added.
    ASSERT_FALSE(revokeHandler->addObserver(nullptr));
}

/**
 * Test to verify the removeObserver method successfully ignores @c nullptr inputs.
 */
TEST_F(RevokeAuthorizationHandlerTest, removeObserverIgnoreNullPtr) {
    auto revokeHandler = RevokeAuthorizationHandler::create(m_mockExceptionEncounteredSender);
    ASSERT_NE(nullptr, revokeHandler);

    // Ensure the nullptr wasn't added.
    ASSERT_FALSE(revokeHandler->removeObserver(nullptr));
}

}  // namespace test
}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
