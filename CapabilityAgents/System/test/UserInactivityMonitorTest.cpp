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

/// @file UserInactivityMonitorTest.cpp

#include <gtest/gtest.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockUserInactivityMonitorObserver.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <ADSL/DirectiveSequencer.h>

#include "System/UserInactivityMonitor.h"

using namespace testing;

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {
namespace test {

using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::avs;
using namespace avsCommon::utils::json;
using namespace rapidjson;
using ::testing::InSequence;

/// This is a string for the namespace we are testing for.
static const std::string USER_INACTIVITY_RESET_NAMESPACE = "System";

/// This is a string for the correct name the endpointing directive uses.
static const std::string USER_INACTIVITY_RESET_NAME = "ResetUserInactivity";

/// This is the string for the message ID used in the directive.
static const std::string USER_INACTIVITY_MESSAGE_ID = "ABC123DEF";
static const std::string USER_INACTIVITY_PAYLOAD_KEY = "inactiveTimeInSeconds";
static const std::chrono::milliseconds USER_INACTIVITY_REPORT_PERIOD{20};

/// This is the condition variable to be used to control the exit of the test case.
std::condition_variable exitTrigger;

/**
 * Check if message request has errors.
 *
 * @param messageRequest The message requests to be checked.
 * @return @c true if parsing the JSON has error, otherwise @c false.
 */
static bool checkMessageRequest(std::shared_ptr<MessageRequest> messageRequest) {
    rapidjson::Document jsonContent(rapidjson::kObjectType);
    if (jsonContent.Parse(messageRequest->getJsonContent()).HasParseError()) {
        return false;
    }
    rapidjson::Value::ConstMemberIterator eventNode;
    if (!jsonUtils::findNode(jsonContent, "event", &eventNode)) {
        return false;
    }
    rapidjson::Value::ConstMemberIterator payloadNode;
    if (!jsonUtils::findNode(eventNode->value, "payload", &payloadNode)) {
        return false;
    }
    rapidjson::Value::ConstMemberIterator inactivityNode;
    if (!jsonUtils::findNode(payloadNode->value, USER_INACTIVITY_PAYLOAD_KEY, &inactivityNode)) {
        return false;
    }
    // The payload should be a long integer.
    return inactivityNode->value.IsUint64();
}

/**
 * Check if message request has errors.
 *
 * @param messageRequest The message requests to be checked.
 * @return @c true if parsing the JSON has error, otherwise @c false.
 */
static bool checkMessageRequestAndReleaseTrigger(std::shared_ptr<MessageRequest> messageRequest) {
    auto returnValue = checkMessageRequest(messageRequest);
    exitTrigger.notify_all();
    return returnValue;
}

/// Test harness for @c UserInactivityMonitor class.
class UserInactivityMonitorTest : public ::testing::Test {
public:
    /// Set up the test harness for running a test.
    void SetUp() override;

protected:
    /// Mocked Message Sender. Note that we make it a strict mock to ensure we test the flow completely.
    std::shared_ptr<StrictMock<MockMessageSender>> m_mockMessageSender;
    /// Mocked Exception Encountered Sender. Note that we make it a strict mock to ensure we test the flow completely.
    std::shared_ptr<StrictMock<MockExceptionEncounteredSender>> m_mockExceptionEncounteredSender;
};

void UserInactivityMonitorTest::SetUp() {
    m_mockMessageSender = std::make_shared<StrictMock<MockMessageSender>>();
    m_mockExceptionEncounteredSender = std::make_shared<StrictMock<MockExceptionEncounteredSender>>();
}

/**
 * This case tests if @c UserInactivityMonitor basic create function works properly
 */
TEST_F(UserInactivityMonitorTest, createSuccessfully) {
    std::mutex exitMutex;
    std::unique_lock<std::mutex> exitLock(exitMutex);
    EXPECT_CALL(*m_mockMessageSender, sendMessage(ResultOf(&checkMessageRequestAndReleaseTrigger, Eq(true))));

    auto userInactivityMonitor = UserInactivityMonitor::create(
        m_mockMessageSender, m_mockExceptionEncounteredSender, USER_INACTIVITY_REPORT_PERIOD);
    ASSERT_NE(nullptr, userInactivityMonitor);

    exitTrigger.wait_for(exitLock, USER_INACTIVITY_REPORT_PERIOD + USER_INACTIVITY_REPORT_PERIOD);
}

/**
 * This case tests if possible @c nullptr parameters passed to @c UserInactivityMonitor::create are handled properly.
 */
TEST_F(UserInactivityMonitorTest, createWithError) {
    ASSERT_EQ(nullptr, UserInactivityMonitor::create(m_mockMessageSender, nullptr));
    ASSERT_EQ(nullptr, UserInactivityMonitor::create(nullptr, m_mockExceptionEncounteredSender));
    ASSERT_EQ(nullptr, UserInactivityMonitor::create(nullptr, nullptr));
}

/**
 * This case tests if a directive is handled properly.
 */
TEST_F(UserInactivityMonitorTest, handleDirectiveProperly) {
    std::mutex exitMutex;
    std::unique_lock<std::mutex> exitLock(exitMutex);
    std::promise<void> notifyObserverPromise1;
    std::promise<void> notifyObserverPromise2;
    auto notifyObserverFuture1 = notifyObserverPromise1.get_future();
    auto notifyObserverFuture2 = notifyObserverPromise2.get_future();

    EXPECT_CALL(*m_mockMessageSender, sendMessage(ResultOf(&checkMessageRequestAndReleaseTrigger, Eq(true))));

    auto userInactivityMonitor = UserInactivityMonitor::create(
        m_mockMessageSender, m_mockExceptionEncounteredSender, USER_INACTIVITY_REPORT_PERIOD);
    ASSERT_NE(nullptr, userInactivityMonitor);

    // let's verify that observers are notified when the UserInactivityReport Event is sent.
    auto userInactivityObserver1 = std::make_shared<MockUserInactivityMonitorObserver>();
    auto userInactivityObserver2 = std::make_shared<MockUserInactivityMonitorObserver>();
    EXPECT_CALL(*(userInactivityObserver1.get()), onUserInactivityReportSent())
        .WillOnce(InvokeWithoutArgs([&notifyObserverPromise1] { notifyObserverPromise1.set_value(); }));
    ;
    EXPECT_CALL(*(userInactivityObserver2.get()), onUserInactivityReportSent())
        .WillOnce(InvokeWithoutArgs([&notifyObserverPromise2] { notifyObserverPromise2.set_value(); }));
    ;
    userInactivityMonitor->addObserver(userInactivityObserver1);
    userInactivityMonitor->addObserver(userInactivityObserver2);

    auto directiveSequencer = adsl::DirectiveSequencer::create(m_mockExceptionEncounteredSender);
    directiveSequencer->addDirectiveHandler(userInactivityMonitor);

    auto userInactivityDirectiveHeader = std::make_shared<AVSMessageHeader>(
        USER_INACTIVITY_RESET_NAMESPACE, USER_INACTIVITY_RESET_NAME, USER_INACTIVITY_MESSAGE_ID);
    auto attachmentManager = std::make_shared<StrictMock<attachment::test::MockAttachmentManager>>();
    std::shared_ptr<AVSDirective> userInactivityDirective =
        AVSDirective::create("", userInactivityDirectiveHeader, "", attachmentManager, "");

    directiveSequencer->onDirective(userInactivityDirective);
    exitTrigger.wait_for(exitLock, USER_INACTIVITY_REPORT_PERIOD + USER_INACTIVITY_REPORT_PERIOD);
    notifyObserverFuture1.wait_for(USER_INACTIVITY_REPORT_PERIOD + USER_INACTIVITY_REPORT_PERIOD);
    notifyObserverFuture2.wait_for(USER_INACTIVITY_REPORT_PERIOD + USER_INACTIVITY_REPORT_PERIOD);

    directiveSequencer->shutdown();
}

/**
 * This case tests if multiple requests are being sent up to AVS.
 */
TEST_F(UserInactivityMonitorTest, sendMultipleReports) {
    InSequence s;
    std::mutex exitMutex;
    std::unique_lock<std::mutex> exitLock(exitMutex);
    int repetitionCount = 3;
    EXPECT_CALL(*m_mockMessageSender, sendMessage(ResultOf(&checkMessageRequest, Eq(true)))).Times(repetitionCount - 1);
    EXPECT_CALL(*m_mockMessageSender, sendMessage(ResultOf(&checkMessageRequestAndReleaseTrigger, Eq(true)))).Times(1);
    auto userInactivityMonitor = UserInactivityMonitor::create(
        m_mockMessageSender, m_mockExceptionEncounteredSender, USER_INACTIVITY_REPORT_PERIOD);
    ASSERT_NE(nullptr, userInactivityMonitor);

    exitTrigger.wait_for(exitLock, repetitionCount * USER_INACTIVITY_REPORT_PERIOD + USER_INACTIVITY_REPORT_PERIOD);
}

/**
 * Verify that timeSinceUserInactivity works as expected.
 */
TEST_F(UserInactivityMonitorTest, verifyInactivityTime) {
    auto userInactivityMonitor = UserInactivityMonitor::create(m_mockMessageSender, m_mockExceptionEncounteredSender);
    ASSERT_NE(nullptr, userInactivityMonitor);

    // we should strongly expect that zero seconds have passed!
    auto timeInactive = userInactivityMonitor->timeSinceUserActivity();
    ASSERT_EQ(0, timeInactive.count());

    // now test for a small millisecond delta.
    std::this_thread::sleep_for(USER_INACTIVITY_REPORT_PERIOD);
    timeInactive = userInactivityMonitor->timeSinceUserActivity();
    auto msPassed = std::chrono::duration_cast<std::chrono::milliseconds>(timeInactive);
    ASSERT_GE(msPassed.count(), 0);
}

/**
 * This case tests if multiple requests are being sent up to AVS with a reset during the process.
 */
TEST_F(UserInactivityMonitorTest, sendMultipleReportsWithReset) {
    InSequence s;
    std::mutex exitMutex;
    std::unique_lock<std::mutex> exitLock(exitMutex);
    int repetitionCount = 5;
    EXPECT_CALL(*m_mockMessageSender, sendMessage(ResultOf(&checkMessageRequest, Eq(true))))
        .Times(AtLeast(repetitionCount - 1));
    EXPECT_CALL(*m_mockMessageSender, sendMessage(ResultOf(&checkMessageRequestAndReleaseTrigger, Eq(true)))).Times(1);

    auto userInactivityMonitor = UserInactivityMonitor::create(
        m_mockMessageSender, m_mockExceptionEncounteredSender, USER_INACTIVITY_REPORT_PERIOD);
    ASSERT_NE(nullptr, userInactivityMonitor);

    auto directiveSequencer = adsl::DirectiveSequencer::create(m_mockExceptionEncounteredSender);
    directiveSequencer->addDirectiveHandler(userInactivityMonitor);

    auto userInactivityDirectiveHeader = std::make_shared<AVSMessageHeader>(
        USER_INACTIVITY_RESET_NAMESPACE, USER_INACTIVITY_RESET_NAME, USER_INACTIVITY_MESSAGE_ID);
    auto attachmentManager = std::make_shared<StrictMock<attachment::test::MockAttachmentManager>>();
    std::shared_ptr<AVSDirective> userInactivityDirective =
        AVSDirective::create("", userInactivityDirectiveHeader, "", attachmentManager, "");

    std::this_thread::sleep_for(2 * USER_INACTIVITY_REPORT_PERIOD + USER_INACTIVITY_REPORT_PERIOD / 2);
    directiveSequencer->onDirective(userInactivityDirective);

    exitTrigger.wait_for(exitLock, repetitionCount * USER_INACTIVITY_REPORT_PERIOD + USER_INACTIVITY_REPORT_PERIOD);
    directiveSequencer->shutdown();
}

}  // namespace test
}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
