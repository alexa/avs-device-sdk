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
#include <queue>

#include <gtest/gtest.h>

#include <AVSCommon/AVS/AbstractAVSConnectionManager.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>
#include <AVSCommon/SDKInterfaces/MessageRequestObserverInterface.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/Utils/PromiseFuturePair.h>
#include <RegistrationManager/MockCustomerDataManager.h>

#include "CertifiedSender/CertifiedSender.h"

using namespace ::testing;

namespace alexaClientSDK {
namespace certifiedSender {
namespace test {

using namespace avsCommon::sdkInterfaces;

/// A sample message
static const std::string TEST_MESSAGE = "TEST_MESSAGE";

/// A sample message URI.
static const std::string TEST_URI = "TEST_URI";

/// Timeout used in test
static const auto TEST_TIMEOUT = std::chrono::seconds(5);

/// Very long timeout used in test
static const auto LONG_TEST_TIMEOUT = std::chrono::seconds(20);

class MockConnection : public avsCommon::avs::AbstractAVSConnectionManager {
    MOCK_METHOD0(enable, void());
    MOCK_METHOD0(disable, void());
    MOCK_METHOD0(isEnabled, bool());
    MOCK_METHOD0(reconnect, void());
    MOCK_CONST_METHOD0(isConnected, bool());
    MOCK_METHOD0(onWakeConnectionRetry, void());
    MOCK_METHOD1(
        addMessageObserver,
        void(std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer));
    MOCK_METHOD1(
        removeMessageObserver,
        void(std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer));
    MOCK_METHOD1(sendMessage, void(std::shared_ptr<avsCommon::avs::MessageRequest> request));
    MOCK_METHOD1(setAVSGateway, void(const std::string& avsGateway));
    MOCK_CONST_METHOD0(getAVSGateway, std::string());
};

class MockMessageStorage : public MessageStorageInterface {
public:
    MOCK_METHOD0(createDatabase, bool());
    MOCK_METHOD0(open, bool());
    MOCK_METHOD0(close, void());
    MOCK_METHOD2(store, bool(const std::string& message, int* id));
    MOCK_METHOD3(store, bool(const std::string& message, const std::string& uriPathExtension, int* id));
    MOCK_METHOD1(load, bool(std::queue<StoredMessage>* messageContainer));
    MOCK_METHOD1(erase, bool(int messageId));
    MOCK_METHOD0(clearDatabase, bool());
    virtual ~MockMessageStorage() = default;
};

class CertifiedSenderTest : public ::testing::Test {
public:
protected:
    void SetUp() override {
        static const std::string CONFIGURATION = R"({
            "certifiedSender" : {
                "databaseFilePath":"database.db"
            }
        })";
        auto configuration = std::shared_ptr<std::stringstream>(new std::stringstream());
        (*configuration) << CONFIGURATION;
        ASSERT_TRUE(avsCommon::avs::initialization::AlexaClientSDKInit::initialize({configuration}));

        m_customerDataManager = std::make_shared<NiceMock<registrationManager::MockCustomerDataManager>>();
        m_mockMessageSender = std::make_shared<avsCommon::sdkInterfaces::test::MockMessageSender>();
        m_connection = std::make_shared<MockConnection>();
        m_storage = std::make_shared<MockMessageStorage>();

        EXPECT_CALL(*m_storage, open()).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*m_storage, load(_)).Times(1).WillOnce(Return(true));
        m_certifiedSender =
            CertifiedSender::create(m_mockMessageSender, m_connection, m_storage, m_customerDataManager);
    }

    void TearDown() override {
        if (avsCommon::avs::initialization::AlexaClientSDKInit::isInitialized()) {
            avsCommon::avs::initialization::AlexaClientSDKInit::uninitialize();
        }
        m_certifiedSender->shutdown();
    }

    /**
     * Utility function to test that messages that receive a non-retryable status response are not retried.
     * @param status The (non-retryable) status.
     * @return Whether the test succeeded.
     */
    bool testNotRetryable(MessageRequestObserverInterface::Status status);

    /**
     * Utility function to test that messages that receive a retryable status response are retried.
     *
     * @note A long timeout must be used because the first retry will happen only 10s after the first attempt.
     * @param status The (retryable) status.
     * @return Whether the test succeeded.
     */
    bool testRetryable(MessageRequestObserverInterface::Status status);

    /// Class under test.
    std::shared_ptr<CertifiedSender> m_certifiedSender;

    /// Mock message storage layer.
    std::shared_ptr<MockMessageStorage> m_storage;

    /// Pointer to connection. We need to remove certifiedSender as a connection observer or both objects will never
    /// be deleted.
    std::shared_ptr<MockConnection> m_connection;

    /// The pointer to the customer data manager
    std::shared_ptr<registrationManager::MockCustomerDataManager> m_customerDataManager;

    /// The mock message sender instance.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockMessageSender> m_mockMessageSender;
};

bool CertifiedSenderTest::testNotRetryable(MessageRequestObserverInterface::Status status) {
    avsCommon::utils::PromiseFuturePair<bool> requestSent;

    std::stringstream messageStream;
    messageStream << "TestNotRetryableMessage-" << status;
    std::string message = messageStream.str();

    {
        InSequence s;
        EXPECT_CALL(*m_storage, store(_, _, _)).WillOnce(Return(true));

        EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
            .Times(1)
            .WillOnce(Invoke([status, &requestSent, message](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
                EXPECT_EQ(request->getJsonContent(), message);
                request->sendCompleted(status);
                requestSent.setValue(true);
            }));

        EXPECT_CALL(*m_storage, erase(_)).WillOnce(Return(true));
    }

    m_certifiedSender->sendJSONMessage(message, TEST_URI);

    /// wait for requests to get sent out.
    return requestSent.waitFor(TEST_TIMEOUT);
}

bool CertifiedSenderTest::testRetryable(MessageRequestObserverInterface::Status status) {
    avsCommon::utils::PromiseFuturePair<bool> requestSent;

    std::stringstream messageStream;
    messageStream << "TestRetryableMessage-" << status;
    std::string message = messageStream.str();

    {
        InSequence s;

        EXPECT_CALL(*m_storage, store(_, _, _)).WillOnce(Return(true));

        EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
            .Times(1)
            .WillOnce(Invoke([status, message](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
                EXPECT_EQ(request->getJsonContent(), message);
                request->sendCompleted(status);
            }));

        EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
            .Times(1)
            .WillOnce(Invoke([&requestSent, message](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
                EXPECT_EQ(request->getJsonContent(), message);
                request->sendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS);
                requestSent.setValue(true);
            }));

        EXPECT_CALL(*m_storage, erase(_)).WillOnce(Return(true));
    }

    m_certifiedSender->sendJSONMessage(message, TEST_URI);

    /// wait for requests to get sent out.
    return requestSent.waitFor(LONG_TEST_TIMEOUT);
}

/**
 * Check that @c clearData() method clears the persistent message storage and the current msg queue
 */
TEST_F(CertifiedSenderTest, test_clearData) {
    EXPECT_CALL(*m_storage, clearDatabase()).Times(1);
    m_certifiedSender->clearData();
}

/**
 * Tests various failure scenarios for the init method.
 */
TEST_F(CertifiedSenderTest, test_initFailsWhenStorageMethodsFail) {
    /// Test if the init method fails when createDatabase on storage fails.
    {
        EXPECT_CALL(*m_storage, open()).Times(1).WillOnce(Return(false));
        EXPECT_CALL(*m_storage, createDatabase()).Times(1).WillOnce(Return(false));
        EXPECT_CALL(*m_storage, load(_)).Times(0);
        auto certifiedSender =
            CertifiedSender::create(m_mockMessageSender, m_connection, m_storage, m_customerDataManager);
        ASSERT_EQ(certifiedSender, nullptr);
    }

    /// Test if the init method fails when load from storage fails.
    {
        EXPECT_CALL(*m_storage, open()).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*m_storage, load(_)).Times(1).WillOnce(Return(false));
        auto certifiedSender =
            CertifiedSender::create(m_mockMessageSender, m_connection, m_storage, m_customerDataManager);
        ASSERT_EQ(certifiedSender, nullptr);
    }
}

/**
 * Tests if the stored messages get sent when a connection is established.
 */
TEST_F(CertifiedSenderTest, testTimer_storedMessagesGetSent) {
    EXPECT_CALL(*m_storage, open()).Times(1).WillOnce(Return(true));

    /// Return messages from storage.
    EXPECT_CALL(*m_storage, load(_))
        .Times(1)
        .WillOnce(Invoke([](std::queue<MessageStorageInterface::StoredMessage>* storedMessages) {
            storedMessages->push(MessageStorageInterface::StoredMessage(1, "testMessage_1"));
            storedMessages->push(MessageStorageInterface::StoredMessage(2, "testMessage_2"));
            return true;
        }));

    avsCommon::utils::PromiseFuturePair<bool> allRequestsSent;
    {
        InSequence s;

        EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
            .Times(1)
            .WillOnce(Invoke([](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
                ASSERT_EQ(request->getJsonContent(), "testMessage_1");
                request->sendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS);
            }));
        EXPECT_CALL(*m_storage, erase(1)).WillOnce(Return(true));

        EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
            .Times(1)
            .WillOnce(Invoke([&allRequestsSent](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
                ASSERT_EQ(request->getJsonContent(), "testMessage_2");
                request->sendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS);
                allRequestsSent.setValue(true);
            }));
        EXPECT_CALL(*m_storage, erase(2)).WillOnce(Return(true));
    }

    auto certifiedSender = CertifiedSender::create(m_mockMessageSender, m_connection, m_storage, m_customerDataManager);

    std::static_pointer_cast<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>(certifiedSender)
        ->onConnectionStatusChanged(
            avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status::CONNECTED,
            avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason::SUCCESS);

    /// wait for requests to get sent out.
    EXPECT_TRUE(allRequestsSent.waitFor(TEST_TIMEOUT));

    /// Cleanup
    certifiedSender->shutdown();
}

/**
 * Verify that a message with a URI specified will be sent out by the sender with the URI.
 */
TEST_F(CertifiedSenderTest, testTimer_SendMessageWithURI) {
    avsCommon::utils::PromiseFuturePair<std::shared_ptr<avsCommon::avs::MessageRequest>> requestSent;

    EXPECT_CALL(*m_storage, store(_, TEST_URI, _)).WillOnce(Return(true));

    {
        InSequence s;
        EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
            .WillOnce(Invoke([&requestSent](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
                requestSent.setValue(request);
                request->sendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS);
            }));

        EXPECT_CALL(*m_storage, erase(_)).WillOnce(Return(true));
    }

    std::static_pointer_cast<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>(m_certifiedSender)
        ->onConnectionStatusChanged(
            avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status::CONNECTED,
            avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason::SUCCESS);

    m_certifiedSender->sendJSONMessage(TEST_MESSAGE, TEST_URI);

    EXPECT_TRUE(requestSent.waitFor(TEST_TIMEOUT));
    EXPECT_EQ(requestSent.getValue()->getJsonContent(), TEST_MESSAGE);
    EXPECT_EQ(requestSent.getValue()->getUriPathExtension(), TEST_URI);
}

/**
 * Tests if messages are re-submitted when the response is a re-tryable response.
 */
TEST_F(CertifiedSenderTest, testTimer_retryableResponsesAreRetried) {
    std::static_pointer_cast<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>(m_certifiedSender)
        ->onConnectionStatusChanged(
            avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status::CONNECTED,
            avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason::SUCCESS);

    {
        SCOPED_TRACE(MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2);
        EXPECT_TRUE(testRetryable(MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2));
    }
    {
        SCOPED_TRACE(MessageRequestObserverInterface::Status::THROTTLED);
        EXPECT_TRUE(testRetryable(MessageRequestObserverInterface::Status::THROTTLED));
    }
    {
        SCOPED_TRACE(MessageRequestObserverInterface::Status::PENDING);
        EXPECT_TRUE(testRetryable(MessageRequestObserverInterface::Status::PENDING));
    }
    {
        SCOPED_TRACE(MessageRequestObserverInterface::Status::NOT_CONNECTED);
        EXPECT_TRUE(testRetryable(MessageRequestObserverInterface::Status::NOT_CONNECTED));
    }
    {
        SCOPED_TRACE(MessageRequestObserverInterface::Status::NOT_SYNCHRONIZED);
        EXPECT_TRUE(testRetryable(MessageRequestObserverInterface::Status::NOT_SYNCHRONIZED));
    }
    {
        SCOPED_TRACE(MessageRequestObserverInterface::Status::TIMEDOUT);
        EXPECT_TRUE(testRetryable(MessageRequestObserverInterface::Status::TIMEDOUT));
    }
    {
        SCOPED_TRACE(MessageRequestObserverInterface::Status::PROTOCOL_ERROR);
        EXPECT_TRUE(testRetryable(MessageRequestObserverInterface::Status::PROTOCOL_ERROR));
    }
    {
        SCOPED_TRACE(MessageRequestObserverInterface::Status::INTERNAL_ERROR);
        EXPECT_TRUE(testRetryable(MessageRequestObserverInterface::Status::INTERNAL_ERROR));
    }
    {
        SCOPED_TRACE(MessageRequestObserverInterface::Status::REFUSED);
        EXPECT_TRUE(testRetryable(MessageRequestObserverInterface::Status::REFUSED));
    }
    {
        SCOPED_TRACE(MessageRequestObserverInterface::Status::INVALID_AUTH);
        EXPECT_TRUE(testRetryable(MessageRequestObserverInterface::Status::INVALID_AUTH));
    }
}

/**
 * Tests if messages are discarded when the response is a non-retryable response.
 */
TEST_F(CertifiedSenderTest, testTimer_nonRetryableResponsesAreNotRetried) {
    std::static_pointer_cast<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>(m_certifiedSender)
        ->onConnectionStatusChanged(
            avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status::CONNECTED,
            avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason::SUCCESS);

    {
        SCOPED_TRACE(MessageRequestObserverInterface::Status::SUCCESS);
        EXPECT_TRUE(testNotRetryable(MessageRequestObserverInterface::Status::SUCCESS));
    }
    {
        SCOPED_TRACE(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
        EXPECT_TRUE(testNotRetryable(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED));
    }
    {
        SCOPED_TRACE(MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT);
        EXPECT_TRUE(testNotRetryable(MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT));
    }
    {
        SCOPED_TRACE(MessageRequestObserverInterface::Status::CANCELED);
        EXPECT_TRUE(testNotRetryable(MessageRequestObserverInterface::Status::CANCELED));
    }
    {
        SCOPED_TRACE(MessageRequestObserverInterface::Status::SERVER_OTHER_ERROR);
        EXPECT_TRUE(testNotRetryable(MessageRequestObserverInterface::Status::SERVER_OTHER_ERROR));
    }
    {
        SCOPED_TRACE(MessageRequestObserverInterface::Status::BAD_REQUEST);
        EXPECT_TRUE(testNotRetryable(MessageRequestObserverInterface::Status::BAD_REQUEST));
    }
}

}  // namespace test
}  // namespace certifiedSender
}  // namespace alexaClientSDK
