/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <Settings/SettingEventMetadata.h>
#include <Settings/SettingEventSender.h>

#include <AVSCommon/SDKInterfaces/MockAVSConnectionManager.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/Utils/PromiseFuturePair.h>

namespace alexaClientSDK {
namespace settings {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils;
using namespace settings;

using namespace ::testing;

using ::testing::MatchesRegex;

/// The metadata for the event messages used in this test.
static const SettingEventMetadata METADATA = {"NAMESPACE", "CHANGEDEVENT", "REPORTEVENT", "SETTING"};

/// The expected changed event format.

// clang-format off
static const std::string EXPECTED_CHANGED_EVENT = R"(\{"event":\{"header":\{"namespace":"NAMESPACE","name":"CHANGEDEVENT","messageId":".*-.*-.*-.*-.*"\},"payload":\{"SETTING":true\}\}\})";
// clang-format on

/// The expected report event format.
// clang-format off
static const std::string EXPECTED_REPORT_EVENT = R"(\{"event":\{"header":\{"namespace":"NAMESPACE","name":"REPORTEVENT","messageId":".*-.*-.*-.*-.*"\},"payload":\{"SETTING":true\}\}\})";
// clang-format on

/// Table with the retry times on subsequent retries.
static const std::vector<int> RETRY_TABLE = {
    500,   // Retry 1: 0.5s
    1000,  // Retry 2: 1s
    1500   // Retry 3: 1.5s
};

///
static const auto MAX_RETRY_WAIT = std::chrono::seconds(4);

/**
 *  Test harness for @c SettingEventSender class.
 */
class SettingEventSenderTest : public Test {
public:
    /// Set up the test harness for running a test.
    void SetUp() override;

protected:
    /// An instance of the @c SettingEventSender to be tested.
    std::shared_ptr<SettingEventSender> m_sender;

    /// Mock of @c MessageSenderInterface.
    std::shared_ptr<MockMessageSender> m_mockMessageSender;

    /// Mock of @c AVSConnectionManagerInterface.
    std::shared_ptr<MockAVSConnectionManager> m_mockAVSConnectionManager;
};

void SettingEventSenderTest::SetUp() {
    m_mockMessageSender = std::make_shared<NiceMock<MockMessageSender>>();
    m_mockAVSConnectionManager = std::make_shared<NiceMock<MockAVSConnectionManager>>();

    m_sender = SettingEventSender::create(METADATA, m_mockMessageSender, RETRY_TABLE);

    ASSERT_NE(m_sender, nullptr);
}

/**
 * Test create validation.
 */
TEST_F(SettingEventSenderTest, test_createValidation) {
    m_sender = SettingEventSender::create(METADATA, nullptr);

    ASSERT_EQ(m_sender, nullptr);
}

/**
 * Test to verify if the changed event is sent as expected.
 */
TEST_F(SettingEventSenderTest, test_sendChangedEvent) {
    PromiseFuturePair<std::string> setValuePromise;

    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .WillOnce(Invoke([&setValuePromise](std::shared_ptr<MessageRequest> request) {
            setValuePromise.setValue(request->getJsonContent());
            request->sendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS);
        }));

    m_sender->sendChangedEvent("true");
    ASSERT_TRUE(setValuePromise.waitFor(std::chrono::seconds(1)));
    EXPECT_THAT(setValuePromise.getValue(), MatchesRegex(EXPECTED_CHANGED_EVENT));
}

/**
 * Test to verify if the report event is sent as expected.
 */
TEST_F(SettingEventSenderTest, test_sendReportEvent) {
    PromiseFuturePair<std::string> setValuePromise;

    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .WillOnce(Invoke([&setValuePromise](std::shared_ptr<MessageRequest> request) {
            setValuePromise.setValue(request->getJsonContent());
            request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS);
        }));

    m_sender->sendReportEvent("true");

    ASSERT_TRUE(setValuePromise.waitFor(std::chrono::seconds(1)));
    EXPECT_THAT(setValuePromise.getValue(), MatchesRegex(EXPECTED_REPORT_EVENT));
}

/**
 * Test validation for incorrect JSON value passed.
 */
TEST_F(SettingEventSenderTest, test_invalidJSONValue) {
    const std::string invalidJSONValue = "TRUE";

    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(0);

    ASSERT_FALSE(m_sender->sendReportEvent(invalidJSONValue).get());
}

/**
 * Test sending of events will block until a response is received
 */
TEST_F(SettingEventSenderTest, testSlow_blockingSend) {
    PromiseFuturePair<std::shared_ptr<MessageRequest>> firstRequest;
    PromiseFuturePair<std::shared_ptr<MessageRequest>> secondRequest;

    {
        InSequence dummy;

        EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
            .WillOnce(
                Invoke([&firstRequest](std::shared_ptr<MessageRequest> request) { firstRequest.setValue(request); }));

        EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
            .WillOnce(Invoke([&secondRequest](std::shared_ptr<MessageRequest> request) {
                secondRequest.setValue(request);
                request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS);
            }));
    }

    std::thread sendThread([this]() {
        m_sender->sendReportEvent("true");
        m_sender->sendReportEvent("false");
    });

    ASSERT_TRUE(firstRequest.waitFor(std::chrono::seconds(1)));
    EXPECT_THAT(firstRequest.getValue()->getJsonContent(), MatchesRegex(EXPECTED_REPORT_EVENT));

    // There is no HTTP response on the first request, second request should not be sent out.
    ASSERT_FALSE(secondRequest.waitFor(std::chrono::seconds(1)));

    // Respond to the first request
    firstRequest.getValue()->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS);

    sendThread.join();
}

/**
 * Test retries will give up after N attempts
 */
TEST_F(SettingEventSenderTest, testSlow_maxRetries) {
    std::atomic<unsigned long> attempts;
    unsigned long maxAttempts = RETRY_TABLE.size();
    PromiseFuturePair<void> retryDone;

    attempts = 0;
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .WillRepeatedly(Invoke([&attempts, &retryDone, maxAttempts](std::shared_ptr<MessageRequest> request) {
            request->sendCompleted(MessageRequestObserverInterface::Status::THROTTLED);
            attempts++;
            if (maxAttempts == attempts) {
                retryDone.setValue();
            }
        }));

    ASSERT_FALSE(m_sender->sendReportEvent("true").get());

    ASSERT_TRUE(retryDone.waitFor(MAX_RETRY_WAIT));
}

/**
 * Test retry on server internal error HTTP response
 */
TEST_F(SettingEventSenderTest, testSlow_retryOnInternalError) {
    std::atomic<unsigned long> attempts;
    unsigned long maxAttempts = RETRY_TABLE.size();
    PromiseFuturePair<void> retryDone;

    attempts = 0;
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .WillRepeatedly(Invoke([&attempts, &retryDone, maxAttempts](std::shared_ptr<MessageRequest> request) {
            request->sendCompleted(MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2);
            attempts++;
            if (maxAttempts == attempts) {
                retryDone.setValue();
            }
        }));

    ASSERT_FALSE(m_sender->sendReportEvent("true").get());

    ASSERT_TRUE(retryDone.waitFor(MAX_RETRY_WAIT));
}

/**
 * Test retry on server internal error HTTP response and will stop after success
 */
TEST_F(SettingEventSenderTest, testSlow_retryStopAfterSuccess) {
    std::atomic<unsigned long> attempts;
    unsigned long maxAttempts = RETRY_TABLE.size();
    PromiseFuturePair<void> retryDone;

    attempts = 0;
    auto handleSendMessageError = [&attempts, &retryDone, maxAttempts](std::shared_ptr<MessageRequest> request) {
        request->sendCompleted(MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2);
        attempts++;
        if (maxAttempts == attempts) {
            retryDone.setValue();
        }
    };

    auto handleSendMessageSuccess = [&attempts, &retryDone, maxAttempts](std::shared_ptr<MessageRequest> request) {
        request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS);
        attempts++;
        if (maxAttempts == attempts) {
            retryDone.setValue();
        }
    };

    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .WillOnce(Invoke(handleSendMessageError))
        .WillOnce(Invoke(handleSendMessageSuccess))
        .WillRepeatedly(Invoke(handleSendMessageSuccess));

    ASSERT_TRUE(m_sender->sendReportEvent("true").get());
    ASSERT_FALSE(retryDone.waitFor(MAX_RETRY_WAIT));
    ASSERT_LE(attempts, maxAttempts);
}

/**
 * Test no retry on non-connected HTTP response
 */
TEST_F(SettingEventSenderTest, testSlow_noRetryOnNonConnected) {
    std::atomic<unsigned long> attempts;
    unsigned long maxAttempts = RETRY_TABLE.size();
    PromiseFuturePair<void> retryDone;

    attempts = 0;
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .WillRepeatedly(Invoke([&attempts, &retryDone, maxAttempts](std::shared_ptr<MessageRequest> request) {
            request->sendCompleted(MessageRequestObserverInterface::Status::NOT_CONNECTED);
            attempts++;
            if (maxAttempts == attempts) {
                retryDone.setValue();
            }
        }));

    ASSERT_FALSE(m_sender->sendReportEvent("true").get());

    ASSERT_FALSE(retryDone.waitFor(MAX_RETRY_WAIT));
}

/**
 * Test cancellation of retries
 */
TEST_F(SettingEventSenderTest, testSlow_cancelRetry) {
    std::atomic<unsigned long> attempts;
    unsigned long maxAttempts = RETRY_TABLE.size();
    PromiseFuturePair<void> retryDone;

    attempts = 0;
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .WillRepeatedly(Invoke([this, &attempts, &retryDone, maxAttempts](std::shared_ptr<MessageRequest> request) {
            // On the second attempt, cancel the send.
            if (attempts == 1) {
                m_sender->cancel();
            }

            request->sendCompleted(MessageRequestObserverInterface::Status::THROTTLED);
            attempts++;

            if (maxAttempts == attempts) {
                retryDone.setValue();
            }
        }));

    ASSERT_FALSE(m_sender->sendReportEvent("true").get());

    ASSERT_FALSE(retryDone.waitFor(MAX_RETRY_WAIT));

    // Verify that retry attempts is only 2 since it's canceled.
    ASSERT_EQ(attempts, 2u);
}

}  // namespace test
}  // namespace settings
}  // namespace alexaClientSDK