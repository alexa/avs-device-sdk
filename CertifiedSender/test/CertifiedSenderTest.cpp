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

#include <RegistrationManager/CustomerDataManager.h>
#include <AVSCommon/AVS/AbstractAVSConnectionManager.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/Utils/PromiseFuturePair.h>

#include "CertifiedSender/CertifiedSender.h"

using namespace ::testing;

namespace alexaClientSDK {
namespace certifiedSender {
namespace test {

/// A sample message
static const std::string TEST_MESSAGE = "TEST_MESSAGE";

/// A sample message URI.
static const std::string TEST_URI = "TEST_URI";

/// Timeout used in test
static const auto TEST_TIMEOUT = std::chrono::seconds(5);

class MockConnection : public avsCommon::avs::AbstractAVSConnectionManager {
    MOCK_METHOD0(enable, void());
    MOCK_METHOD0(disable, void());
    MOCK_METHOD0(isEnabled, bool());
    MOCK_METHOD0(reconnect, void());
    MOCK_CONST_METHOD0(isConnected, bool());
    MOCK_METHOD0(onWakeConnectionRetry, void());
    MOCK_METHOD0(onWakeVerifyConnectivity, void());
    MOCK_METHOD1(
        addMessageObserver,
        void(std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer));
    MOCK_METHOD1(
        removeMessageObserver,
        void(std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer));
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

        m_customerDataManager = std::make_shared<registrationManager::CustomerDataManager>();
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

    /// Class under test.
    std::shared_ptr<CertifiedSender> m_certifiedSender;

    /// Mock message storage layer.
    std::shared_ptr<MockMessageStorage> m_storage;

    /// Pointer to connection. We need to remove certifiedSender as a connection observer or both objects will never
    /// be deleted.
    std::shared_ptr<MockConnection> m_connection;

    /// The pointer to the customer data manager
    std::shared_ptr<registrationManager::CustomerDataManager> m_customerDataManager;

    /// The mock message sender instance.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockMessageSender> m_mockMessageSender;
};

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
            .WillOnce(Invoke([](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
                ASSERT_EQ(request->getJsonContent(), "testMessage_2");
                request->sendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS);
            }));
        EXPECT_CALL(*m_storage, erase(2)).WillOnce(Invoke([&allRequestsSent](int messageId) {
            allRequestsSent.setValue(true);
            return true;
        }));
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

}  // namespace test
}  // namespace certifiedSender
}  // namespace alexaClientSDK
