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
#include <string>

#include <gtest/gtest.h>
#include <rapidjson/document.h>

#include <AVSCommon/AVS/AbstractAVSConnectionManager.h>
#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>

#include "System/SoftwareInfoSender.h"

using namespace testing;

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::avs::attachment::test;
using namespace avsCommon::utils;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::softwareInfo;
using namespace avsCommon::sdkInterfaces::test;

/// This is a string for the namespace we are testing for.
static const std::string NAMESPACE_SYSTEM = "System";

/// This is a string for the name of the System.SoftwareInfo event.
static const std::string NAME_SOFTWARE_INFO = "SoftwareInfo";

/// This is a string for the name of the System.ReportSoftwareInfo directive.
static const std::string NAME_REPORT_SOFTWARE_INFO = "ReportSoftwareInfo";

/// Dummy message ID with which to mock receiving a directive.
static const std::string MESSAGE_ID = "Message-1";

/// Empty dialogRequestId with which to mock receiving a directive.
static const std::string DIALOG_REQUEST_ID = "";

/// Dummy unparsed directive JSON.
static const std::string UNPARSED_DIRECTIVE = "";

/// Header of ReportSoftwareInfo directive.
static const auto REPORT_SOFTWARE_INFO_DIRECTIVE_HEADER =
    std::make_shared<AVSMessageHeader>(NAMESPACE_SYSTEM, NAME_REPORT_SOFTWARE_INFO, MESSAGE_ID, DIALOG_REQUEST_ID);

/// Dummy directive payload.
static const std::string TEST_PAYLOAD = "";

/// Empty attachment ID.
static const std::string ATTACHMENT_CONTEXT_ID = "";

/// Value for timeouts we expect to reach.
static const std::chrono::milliseconds EXPECTED_TIMEOUT(100);

/// Value for timeouts we do not expect to reach.
static const std::chrono::seconds UNEXPECTED_TIMEOUT(5);

/// Max time to wait for two send retries.
static const std::chrono::seconds TWO_RETRIES_TIMEOUT(15);

static const FirmwareVersion FIRST_FIRMWARE_VERSION = 1;
static const FirmwareVersion SECOND_FIRMWARE_VERSION = 2;
static const FirmwareVersion THIRD_FIRMWARE_VERSION = 3;

/**
 *
 */
class MockSoftwareInfoSenderObserver : public SoftwareInfoSenderObserverInterface {
public:
    MOCK_METHOD1(onFirmwareVersionAccepted, void(FirmwareVersion));
};

/**
 * Class with which to mock a connection ot AVS.
 */
class MockConnection : public AbstractAVSConnectionManager {
public:
    MockConnection();

    MOCK_METHOD0(enable, void());
    MOCK_METHOD0(disable, void());
    MOCK_METHOD0(isEnabled, bool());
    MOCK_METHOD0(reconnect, void());
    MOCK_METHOD1(
        addMessageObserver,
        void(std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer));
    MOCK_METHOD1(
        removeMessageObserver,
        void(std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer));

    bool isConnected() const;

    /**
     * Update the connection status.
     *
     * @param status The Connection Status.
     * @param reason The reason the Connection Status changed.
     */
    void updateConnectionStatus(
        ConnectionStatusObserverInterface::Status status,
        ConnectionStatusObserverInterface::ChangedReason reason);
};

MockConnection::MockConnection() : AbstractAVSConnectionManager{} {
}

bool MockConnection::isConnected() const {
    return ConnectionStatusObserverInterface::Status::CONNECTED == m_connectionStatus;
}

void MockConnection::updateConnectionStatus(
    ConnectionStatusObserverInterface::Status status,
    ConnectionStatusObserverInterface::ChangedReason reason) {
    AbstractAVSConnectionManager::updateConnectionStatus(status, reason);
}

/// Test harness for @c SoftwareInfoSender class.
class SoftwareInfoSenderTest : public ::testing::Test {
public:
    /// Set up the test harness for running a test.
    void SetUp() override;

protected:
    std::shared_ptr<StrictMock<MockSoftwareInfoSenderObserver>> m_mockObserver;
    /// Mocked connection
    std::shared_ptr<StrictMock<MockConnection>> m_mockConnection;
    /// Mocked MessageSenderInterface
    std::shared_ptr<StrictMock<MockMessageSender>> m_mockMessageSender;
    /// Mocked Exception Encountered Sender.
    std::shared_ptr<StrictMock<MockExceptionEncounteredSender>> m_mockExceptionEncounteredSender;
    /// Mock AttachmentManager for creating directives.
    std::shared_ptr<StrictMock<MockAttachmentManager>> m_mockAttachmentManager;
    /// System.ReportSoftwareInfo directive.
    std::shared_ptr<AVSDirective> m_reportSoftwareInfoDirective;
};

void SoftwareInfoSenderTest::SetUp() {
    m_mockObserver = std::make_shared<StrictMock<MockSoftwareInfoSenderObserver>>();
    m_mockConnection = std::make_shared<StrictMock<MockConnection>>();
    m_mockMessageSender = std::make_shared<StrictMock<MockMessageSender>>();
    m_mockExceptionEncounteredSender = std::make_shared<StrictMock<MockExceptionEncounteredSender>>();
    m_mockAttachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    m_reportSoftwareInfoDirective = AVSDirective::create(
        UNPARSED_DIRECTIVE,
        REPORT_SOFTWARE_INFO_DIRECTIVE_HEADER,
        TEST_PAYLOAD,
        m_mockAttachmentManager,
        ATTACHMENT_CONTEXT_ID);
}

/**
 * Verify that providing an invalid firmware version will cause SoftwareInfoSender::create() to fail.
 */
TEST_F(SoftwareInfoSenderTest, createFailedInvalidFirmwareVersion) {
    ASSERT_FALSE(SoftwareInfoSender::create(
        INVALID_FIRMWARE_VERSION,
        true,
        m_mockObserver,
        m_mockConnection,
        m_mockMessageSender,
        m_mockExceptionEncounteredSender));
}

/**
 * Verify that passing false for sendSoftwareInfoUponConnect will NOT cause SoftwareInfoSender::create() to fail.
 */
TEST_F(SoftwareInfoSenderTest, createSuccessWithsendSoftwareInfoUponConnectFalse) {
    auto softwareInfoSender = SoftwareInfoSender::create(
        FIRST_FIRMWARE_VERSION,
        false,
        m_mockObserver,
        m_mockConnection,
        m_mockMessageSender,
        m_mockExceptionEncounteredSender);
    ASSERT_TRUE(softwareInfoSender);
    softwareInfoSender->shutdown();
}

/**
 * Verify that passing nullptr for observer will NOT cause SoftwareInfoSender::create() to fail.
 */
TEST_F(SoftwareInfoSenderTest, createSuccessWithObserverNull) {
    auto softwareInfoSender = SoftwareInfoSender::create(
        1, true, nullptr, m_mockConnection, m_mockMessageSender, m_mockExceptionEncounteredSender);
    ASSERT_TRUE(softwareInfoSender);
    softwareInfoSender->shutdown();
}

/**
 * Verify that passing nullptr for connection will cause SoftwareInfoSender::create() to fail.
 */
TEST_F(SoftwareInfoSenderTest, createFailedConnectionNull) {
    ASSERT_FALSE(SoftwareInfoSender::create(
        1, true, m_mockObserver, nullptr, m_mockMessageSender, m_mockExceptionEncounteredSender));
}

/**
 * Verify that not providing a @c MessageSender will cause @c SoftwareInfoSender::create() to fail.
 */
TEST_F(SoftwareInfoSenderTest, createFailedMessageSenderNull) {
    ASSERT_FALSE(SoftwareInfoSender::create(
        1, true, m_mockObserver, m_mockConnection, nullptr, m_mockExceptionEncounteredSender));
}

/**
 * Verify that not providing a @c MessageSender will cause @c SoftwareInfoSender::create() to fail.
 */
TEST_F(SoftwareInfoSenderTest, createFailedExceptionEncounteredSenderNull) {
    ASSERT_FALSE(SoftwareInfoSender::create(1, true, m_mockObserver, m_mockConnection, m_mockMessageSender, nullptr));
}

/**
 * Verify that no SoftwareInfo event or ExceptionEncountered message is sent if @c sendSoftwareInfoOnConnect is
 * @c false and ReportSoftwareInfo directive is not received.
 */
TEST_F(SoftwareInfoSenderTest, noSoftwareInfoEventSentByDefault) {
    EXPECT_CALL(*(m_mockObserver.get()), onFirmwareVersionAccepted(_)).Times(0);
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_)).Times(0);
    EXPECT_CALL(*(m_mockExceptionEncounteredSender.get()), sendExceptionEncountered(_, _, _)).Times(0);

    auto softwareInfoSender = SoftwareInfoSender::create(
        FIRST_FIRMWARE_VERSION,
        false,
        m_mockObserver,
        m_mockConnection,
        m_mockMessageSender,
        m_mockExceptionEncounteredSender);
    ASSERT_TRUE(softwareInfoSender);

    m_mockConnection->updateConnectionStatus(
        ConnectionStatusObserverInterface::Status::PENDING,
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

    m_mockConnection->updateConnectionStatus(
        ConnectionStatusObserverInterface::Status::CONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

    m_mockConnection->updateConnectionStatus(
        ConnectionStatusObserverInterface::Status::PENDING,
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

    m_mockConnection->updateConnectionStatus(
        ConnectionStatusObserverInterface::Status::CONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

    m_mockConnection->updateConnectionStatus(
        ConnectionStatusObserverInterface::Status::DISCONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

    softwareInfoSender->shutdown();
}

/**
 * Verify that no attempt is made to send a SoftwareInfo event or and ExceptionEncounteredEvent if no connection
 * has been established - even if @c sendSoftwareInfoOnConnect is @c true.
 */
TEST_F(SoftwareInfoSenderTest, nothingSentBeforeConnected) {
    EXPECT_CALL(*(m_mockObserver.get()), onFirmwareVersionAccepted(_)).Times(0);
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_)).Times(0);
    EXPECT_CALL(*(m_mockExceptionEncounteredSender.get()), sendExceptionEncountered(_, _, _)).Times(0);

    auto softwareInfoSender = SoftwareInfoSender::create(
        1, true, m_mockObserver, m_mockConnection, m_mockMessageSender, m_mockExceptionEncounteredSender);
    ASSERT_TRUE(softwareInfoSender);

    m_mockConnection->updateConnectionStatus(
        ConnectionStatusObserverInterface::Status::PENDING,
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

    softwareInfoSender->shutdown();
}

/**
 * Verify that one SoftwareInfo event is sent and no ExceptionEncountered message is sent if @c
 * sendSoftwareInfoOnConnect is @c true, a connection is made, and no ReportSoftwareInfo directive is received.
 */
TEST_F(SoftwareInfoSenderTest, softwareInfoSentUponConnectIfSendSetTrueBeforeConnect) {
    std::promise<void> versionAcceptedPromise;
    EXPECT_CALL(*(m_mockObserver.get()), onFirmwareVersionAccepted(_))
        .Times(1)
        .WillOnce(InvokeWithoutArgs([&versionAcceptedPromise]() { versionAcceptedPromise.set_value(); }));

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(1)
        .WillOnce(Invoke([](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT);
        }));

    EXPECT_CALL(*(m_mockExceptionEncounteredSender.get()), sendExceptionEncountered(_, _, _)).Times(0);

    auto softwareInfoSender = SoftwareInfoSender::create(
        1, true, m_mockObserver, m_mockConnection, m_mockMessageSender, m_mockExceptionEncounteredSender);
    ASSERT_TRUE(softwareInfoSender);

    m_mockConnection->updateConnectionStatus(
        ConnectionStatusObserverInterface::Status::PENDING,
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

    m_mockConnection->updateConnectionStatus(
        ConnectionStatusObserverInterface::Status::CONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);
    m_mockConnection->updateConnectionStatus(
        ConnectionStatusObserverInterface::Status::PENDING,
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

    m_mockConnection->updateConnectionStatus(
        ConnectionStatusObserverInterface::Status::CONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

    m_mockConnection->updateConnectionStatus(
        ConnectionStatusObserverInterface::Status::DISCONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

    auto versionAcceptedFuture = versionAcceptedPromise.get_future();
    ASSERT_NE(versionAcceptedFuture.wait_for(UNEXPECTED_TIMEOUT), std::future_status::timeout);

    softwareInfoSender->shutdown();
}

/**
 * Verify that an event is sent if a @c ReportSoftwareInfo directive is received even if @c sendSoftwareInfoOnConnect is
 * @c false.
 */
TEST_F(SoftwareInfoSenderTest, reportSoftwareInfoReceived) {
    std::promise<void> versionAcceptedPromise;
    EXPECT_CALL(*(m_mockObserver.get()), onFirmwareVersionAccepted(_))
        .Times(1)
        .WillOnce(InvokeWithoutArgs([&versionAcceptedPromise]() { versionAcceptedPromise.set_value(); }));

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(1)
        .WillOnce(Invoke([](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT);
        }));

    EXPECT_CALL(*(m_mockExceptionEncounteredSender.get()), sendExceptionEncountered(_, _, _)).Times(0);

    auto softwareInfoSender = SoftwareInfoSender::create(
        FIRST_FIRMWARE_VERSION,
        false,
        m_mockObserver,
        m_mockConnection,
        m_mockMessageSender,
        m_mockExceptionEncounteredSender);
    ASSERT_TRUE(softwareInfoSender);

    m_mockConnection->updateConnectionStatus(
        ConnectionStatusObserverInterface::Status::PENDING,
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

    m_mockConnection->updateConnectionStatus(
        ConnectionStatusObserverInterface::Status::CONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

    softwareInfoSender->handleDirectiveImmediately(m_reportSoftwareInfoDirective);

    auto versionAcceptedFuture = versionAcceptedPromise.get_future();
    ASSERT_NE(versionAcceptedFuture.wait_for(UNEXPECTED_TIMEOUT), std::future_status::timeout);

    softwareInfoSender->shutdown();
}

/**
 * Verify that handling a @c ReportSoftwareInfo directive cancels incomplete handling of any previous
 * ReportSoftwareInfo directive.
 */
TEST_F(SoftwareInfoSenderTest, reportSoftwareInfoCancellsPreviousDirective) {
    // This status causes the first send request to retry, until set to something else.
    std::atomic<MessageRequestObserverInterface::Status> status(
        MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2);

    std::promise<void> versionAcceptedPromise;
    EXPECT_CALL(*(m_mockObserver.get()), onFirmwareVersionAccepted(_))
        .Times(1)
        .WillOnce(InvokeWithoutArgs([&versionAcceptedPromise]() { versionAcceptedPromise.set_value(); }));

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(2)
        .WillRepeatedly(Invoke([&status](std::shared_ptr<MessageRequest> request) {
            std::this_thread::sleep_for(EXPECTED_TIMEOUT);
            request->sendCompleted(status);
        }));

    EXPECT_CALL(*(m_mockExceptionEncounteredSender.get()), sendExceptionEncountered(_, _, _)).Times(0);

    auto softwareInfoSender = SoftwareInfoSender::create(
        FIRST_FIRMWARE_VERSION,
        false,
        m_mockObserver,
        m_mockConnection,
        m_mockMessageSender,
        m_mockExceptionEncounteredSender);
    ASSERT_TRUE(softwareInfoSender);

    m_mockConnection->updateConnectionStatus(
        ConnectionStatusObserverInterface::Status::PENDING,
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

    m_mockConnection->updateConnectionStatus(
        ConnectionStatusObserverInterface::Status::CONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

    softwareInfoSender->handleDirectiveImmediately(m_reportSoftwareInfoDirective);

    status = MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT;

    softwareInfoSender->handleDirectiveImmediately(m_reportSoftwareInfoDirective);

    // Sleep long enough for more than 2 sendMessage() calls to pile up.
    // Yes, it is weak to use a sleep to coordinate the timing of calls.  In
    // this case we are allowing time for calls that we do NOT expect to
    // happen (note that we EXPECT_CALL Time(2) above).  So, there is no
    // trigger we can use to know when to stop waiting.  If the timing
    // is not correct, this test will NOT generate false failures.
    std::this_thread::sleep_for(4 * EXPECTED_TIMEOUT);

    auto versionAcceptedFuture = versionAcceptedPromise.get_future();
    ASSERT_NE(versionAcceptedFuture.wait_for(UNEXPECTED_TIMEOUT), std::future_status::timeout);

    softwareInfoSender->shutdown();
}

/**
 * Verify that notification that the firmware version was accepted by @c AVS is only sent once.
 */
TEST_F(SoftwareInfoSenderTest, delayedReportSoftwareInfoNotifiesOnce) {
    std::promise<void> versionAcceptedPromise;
    EXPECT_CALL(*(m_mockObserver.get()), onFirmwareVersionAccepted(_))
        .Times(1)
        .WillOnce(InvokeWithoutArgs([&versionAcceptedPromise]() { versionAcceptedPromise.set_value(); }));

    std::promise<void> messageSentTwicePromise;
    int sentCounter = 0;
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(2)
        .WillRepeatedly(Invoke([&messageSentTwicePromise, &sentCounter](std::shared_ptr<MessageRequest> request) {
            request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT);
            if (++sentCounter == 2) {
                messageSentTwicePromise.set_value();
            }
        }));

    EXPECT_CALL(*(m_mockExceptionEncounteredSender.get()), sendExceptionEncountered(_, _, _)).Times(0);

    auto softwareInfoSender = SoftwareInfoSender::create(
        FIRST_FIRMWARE_VERSION,
        false,
        m_mockObserver,
        m_mockConnection,
        m_mockMessageSender,
        m_mockExceptionEncounteredSender);
    ASSERT_TRUE(softwareInfoSender);

    m_mockConnection->updateConnectionStatus(
        ConnectionStatusObserverInterface::Status::PENDING,
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

    m_mockConnection->updateConnectionStatus(
        ConnectionStatusObserverInterface::Status::CONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

    softwareInfoSender->handleDirectiveImmediately(m_reportSoftwareInfoDirective);

    auto versionAcceptedFuture = versionAcceptedPromise.get_future();
    ASSERT_NE(versionAcceptedFuture.wait_for(UNEXPECTED_TIMEOUT), std::future_status::timeout);

    softwareInfoSender->handleDirectiveImmediately(m_reportSoftwareInfoDirective);

    auto messageSentTwiceFuture = messageSentTwicePromise.get_future();
    ASSERT_NE(messageSentTwiceFuture.wait_for(UNEXPECTED_TIMEOUT), std::future_status::timeout);

    softwareInfoSender->shutdown();
}

/**
 * Verify that SoftwareInfoSender retries sending.
 */
TEST_F(SoftwareInfoSenderTest, verifySendRetries) {
    std::promise<void> versionAcceptedPromise;
    EXPECT_CALL(*(m_mockObserver.get()), onFirmwareVersionAccepted(_))
        .Times(1)
        .WillOnce(InvokeWithoutArgs([&versionAcceptedPromise]() { versionAcceptedPromise.set_value(); }));

    int callCount = 0;
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(3)
        .WillRepeatedly(Invoke([&callCount](std::shared_ptr<MessageRequest> request) {
            request->sendCompleted(
                ++callCount == 3 ? MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT
                                 : MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2);
        }));

    EXPECT_CALL(*(m_mockExceptionEncounteredSender.get()), sendExceptionEncountered(_, _, _)).Times(0);

    auto softwareInfoSender = SoftwareInfoSender::create(
        1, true, m_mockObserver, m_mockConnection, m_mockMessageSender, m_mockExceptionEncounteredSender);
    ASSERT_TRUE(softwareInfoSender);

    m_mockConnection->updateConnectionStatus(
        ConnectionStatusObserverInterface::Status::PENDING,
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

    m_mockConnection->updateConnectionStatus(
        ConnectionStatusObserverInterface::Status::CONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

    auto versionAcceptedFuture = versionAcceptedPromise.get_future();
    ASSERT_NE(versionAcceptedFuture.wait_for(TWO_RETRIES_TIMEOUT), std::future_status::timeout);

    softwareInfoSender->shutdown();
}

/**
 * Verify that attempting to set an invalid firmware version fails.
 */
TEST_F(SoftwareInfoSenderTest, setInvalidFirmwareVersion) {
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_)).Times(0);
    EXPECT_CALL(*(m_mockExceptionEncounteredSender.get()), sendExceptionEncountered(_, _, _)).Times(0);

    auto softwareInfoSender = SoftwareInfoSender::create(
        FIRST_FIRMWARE_VERSION,
        false,
        m_mockObserver,
        m_mockConnection,
        m_mockMessageSender,
        m_mockExceptionEncounteredSender);
    ASSERT_TRUE(softwareInfoSender);

    m_mockConnection->updateConnectionStatus(
        ConnectionStatusObserverInterface::Status::PENDING,
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

    m_mockConnection->updateConnectionStatus(
        ConnectionStatusObserverInterface::Status::CONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

    ASSERT_FALSE(softwareInfoSender->setFirmwareVersion(INVALID_FIRMWARE_VERSION));

    softwareInfoSender->shutdown();
}

/**
 * Verify that handling a @c ReportSoftwareInfo directive cancels incomplete handling of any previous
 * ReportSoftwareInfo directive.
 */
TEST_F(SoftwareInfoSenderTest, setFirmwareVersionCancellsPreviousSetting) {
    // This status causes the first send request to retry, until set to something else.
    std::atomic<MessageRequestObserverInterface::Status> status(
        MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2);

    std::promise<void> versionAcceptedPromise;
    EXPECT_CALL(*(m_mockObserver.get()), onFirmwareVersionAccepted(THIRD_FIRMWARE_VERSION))
        .Times(1)
        .WillOnce(InvokeWithoutArgs([&versionAcceptedPromise]() { versionAcceptedPromise.set_value(); }));

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(2)
        .WillRepeatedly(Invoke([&status](std::shared_ptr<MessageRequest> request) {
            std::this_thread::sleep_for(EXPECTED_TIMEOUT);
            request->sendCompleted(status);
        }));

    EXPECT_CALL(*(m_mockExceptionEncounteredSender.get()), sendExceptionEncountered(_, _, _)).Times(0);

    auto softwareInfoSender = SoftwareInfoSender::create(
        FIRST_FIRMWARE_VERSION,
        false,
        m_mockObserver,
        m_mockConnection,
        m_mockMessageSender,
        m_mockExceptionEncounteredSender);
    ASSERT_TRUE(softwareInfoSender);

    m_mockConnection->updateConnectionStatus(
        ConnectionStatusObserverInterface::Status::PENDING,
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

    m_mockConnection->updateConnectionStatus(
        ConnectionStatusObserverInterface::Status::CONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

    softwareInfoSender->setFirmwareVersion(SECOND_FIRMWARE_VERSION);

    status = MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT;

    softwareInfoSender->setFirmwareVersion(THIRD_FIRMWARE_VERSION);

    // Sleep long enough for more than 2 sendMessage() calls to pile up.
    // Yes, it is weak to use a sleep to coordinate the timing of calls.  In
    // this case we are allowing time for calls that we do NOT expect to
    // happen (note the EXPECT_CALL Time(2) above).  So, there is no
    // trigger we can use to know when to stop waiting.  If the timing
    // is not correct, this test will NOT generate false failures.
    std::this_thread::sleep_for(4 * EXPECTED_TIMEOUT);

    auto versionAcceptedFuture = versionAcceptedPromise.get_future();
    ASSERT_NE(versionAcceptedFuture.wait_for(UNEXPECTED_TIMEOUT), std::future_status::timeout);

    softwareInfoSender->shutdown();
}

}  // namespace test
}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
