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

#include <memory>

#include <gtest/gtest.h>

#include "RegistrationManager/CustomerDataManager.h"
#include "AVSCommon/AVS/Initialization/AlexaClientSDKInit.h"
#include "AVSCommon/SDKInterfaces/MockMessageSender.h"

#include "CertifiedSender/CertifiedSender.h"

using namespace ::testing;

namespace alexaClientSDK {
namespace certifiedSender {
namespace test {

class MockConnection : public avsCommon::avs::AbstractConnection {
    MOCK_CONST_METHOD0(isConnected, bool());
};

class MockMessageStorage : public MessageStorageInterface {
public:
    MOCK_METHOD0(createDatabase, bool());
    MOCK_METHOD0(open, bool());
    MOCK_METHOD0(close, void());
    MOCK_METHOD2(store, bool(const std::string& message, int* id));
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
        std::stringstream configuration;
        configuration << CONFIGURATION;
        ASSERT_TRUE(avsCommon::avs::initialization::AlexaClientSDKInit::initialize({&configuration}));

        auto customerDataManager = std::make_shared<registrationManager::CustomerDataManager>();
        auto msgSender = std::make_shared<avsCommon::sdkInterfaces::test::MockMessageSender>();
        m_connection = std::make_shared<MockConnection>();
        m_storage = std::make_shared<MockMessageStorage>();

        EXPECT_CALL(*m_storage, open()).Times(1).WillOnce(Return(true));
        m_certifiedSender = CertifiedSender::create(msgSender, m_connection, m_storage, customerDataManager);
    }

    void TearDown() override {
        if (avsCommon::avs::initialization::AlexaClientSDKInit::isInitialized()) {
            avsCommon::avs::initialization::AlexaClientSDKInit::uninitialize();
        }
        m_connection->removeConnectionStatusObserver(m_certifiedSender);
    }

    /// Class under test.
    std::shared_ptr<CertifiedSender> m_certifiedSender;

    /// Mock message storage layer.
    std::shared_ptr<MockMessageStorage> m_storage;

    /// Pointer to connection. We need to remove certifiedSender as a connection observer or both objects will never
    /// be deleted.
    std::shared_ptr<MockConnection> m_connection;
};

/**
 * Check that @c clearData() method clears the persistent message storage and the current msg queue
 */
TEST_F(CertifiedSenderTest, clearDataTest) {
    EXPECT_CALL(*m_storage, clearDatabase()).Times(1);
    m_certifiedSender->clearData();
}

}  // namespace test
}  // namespace certifiedSender
}  // namespace alexaClientSDK
