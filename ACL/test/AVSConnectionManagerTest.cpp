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

/// @file AVSConnectionManagerTest.cpp

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/Utils/Network/MockInternetConnectionMonitor.h>
#include "ACL/AVSConnectionManager.h"

namespace alexaClientSDK {
namespace acl {
namespace test {

using namespace ::testing;
using namespace alexaClientSDK::avsCommon::avs::initialization;
using namespace alexaClientSDK::avsCommon::sdkInterfaces;
using namespace avsCommon::utils::network::test;

/// This class allows us to test MessageObserver interaction
class MockMessageObserver : public MessageObserverInterface {
public:
    MockMessageObserver() {
    }
    MOCK_METHOD2(receive, void(const std::string& contextId, const std::string& message));
};

/// This class allows us to test ConnectionStatusObserver interaction
class MockConnectionStatusObserver : public ConnectionStatusObserverInterface {
public:
    MockConnectionStatusObserver() {
    }
    MOCK_METHOD2(
        onConnectionStatusChanged,
        void(
            ConnectionStatusObserverInterface::Status status,
            ConnectionStatusObserverInterface::ChangedReason reason));
};

/**
 * This class allows us to test MessageRouter interaction.
 */
class MockMessageRouter : public MessageRouterInterface {
public:
    MockMessageRouter() : MessageRouterInterface{"MockMessageRouter"} {
    }

    MOCK_METHOD0(enable, void());
    MOCK_METHOD0(disable, void());
    MOCK_METHOD0(doShutdown, void());
    MOCK_METHOD0(getConnectionStatus, MessageRouterInterface::ConnectionStatus());
    MOCK_METHOD1(sendMessage, void(std::shared_ptr<avsCommon::avs::MessageRequest> request));
    MOCK_METHOD1(setAVSGateway, void(const std::string& avsGateway));
    MOCK_METHOD0(getAVSGateway, std::string());
    MOCK_METHOD0(onWakeConnectionRetry, void());
    MOCK_METHOD0(onWakeVerifyConnectivity, void());
    MOCK_METHOD1(setObserver, void(std::shared_ptr<MessageRouterObserverInterface> observer));
};

/// Test harness for @c AVSConnectionManager class
class AVSConnectionManagerTest : public ::testing::Test {
public:
    void SetUp() override;
    void TearDown() override;

    std::shared_ptr<AVSConnectionManager> m_avsConnectionManager;
    std::shared_ptr<MockMessageRouter> m_messageRouter;
    std::shared_ptr<MockConnectionStatusObserver> m_observer;
    std::shared_ptr<MockMessageObserver> m_messageObserver;
    std::shared_ptr<MockInternetConnectionMonitor> m_mockConnectionMonitor;
};

void AVSConnectionManagerTest::SetUp() {
    AlexaClientSDKInit::initialize(std::vector<std::shared_ptr<std::istream>>());
    m_messageRouter = std::make_shared<MockMessageRouter>();
    m_observer = std::make_shared<MockConnectionStatusObserver>();
    m_messageObserver = std::make_shared<MockMessageObserver>();
    m_mockConnectionMonitor = std::make_shared<MockInternetConnectionMonitor>();
    m_avsConnectionManager = AVSConnectionManager::create(
        m_messageRouter,
        true,
        std::unordered_set<std::shared_ptr<ConnectionStatusObserverInterface>>(),
        std::unordered_set<std::shared_ptr<MessageObserverInterface>>(),
        m_mockConnectionMonitor);

    EXPECT_THAT(m_avsConnectionManager, NotNull());
}

void AVSConnectionManagerTest::TearDown() {
    AlexaClientSDKInit::uninitialize();
}

/**
 * Test @c create with valid messageRouter, ConnectionStatusObserver, MessageObservers
 */
TEST_F(AVSConnectionManagerTest, test_create) {
    EXPECT_CALL(*m_messageRouter, setObserver(_)).Times(1);
    EXPECT_CALL(*m_messageRouter, enable()).Times(1);
    ASSERT_NE(nullptr, m_avsConnectionManager->create(m_messageRouter, true, {m_observer}, {m_messageObserver}));
}

/**
 * Test @c create with different combinations of messageRouter, ConnectionStatusObserver, MessageObservers,
 * InternetConnectionMonitor.
 */
TEST_F(AVSConnectionManagerTest, test_createWithNullMessageRouterAndObservers) {
    ASSERT_EQ(nullptr, m_avsConnectionManager->create(nullptr, true, {m_observer}, {m_messageObserver}));
    ASSERT_EQ(nullptr, m_avsConnectionManager->create(m_messageRouter, true, {nullptr}, {m_messageObserver}));
    ASSERT_EQ(nullptr, m_avsConnectionManager->create(m_messageRouter, true, {m_observer}, {nullptr}));
    ASSERT_NE(
        nullptr, m_avsConnectionManager->create(m_messageRouter, true, {m_observer}, {m_messageObserver}, nullptr));
    ASSERT_NE(
        nullptr,
        m_avsConnectionManager->create(
            m_messageRouter, true, {m_observer}, {m_messageObserver}, m_mockConnectionMonitor));

    std::shared_ptr<MockConnectionStatusObserver> validConnectionStatusObserver;
    validConnectionStatusObserver = std::make_shared<MockConnectionStatusObserver>();
    ASSERT_EQ(
        nullptr,
        m_avsConnectionManager->create(
            m_messageRouter, true, {m_observer, nullptr, validConnectionStatusObserver}, {m_messageObserver}));
    std::shared_ptr<MockMessageObserver> validMessageObserver;
    validMessageObserver = std::make_shared<MockMessageObserver>();
    ASSERT_EQ(
        nullptr,
        m_avsConnectionManager->create(
            m_messageRouter, true, {m_observer}, {m_messageObserver, nullptr, validMessageObserver}));
    ASSERT_EQ(nullptr, m_avsConnectionManager->create(m_messageRouter, true, {nullptr}, {nullptr}));
    // create should pass with empty set of ConnectionStatusObservers
    ASSERT_NE(
        nullptr,
        m_avsConnectionManager->create(
            m_messageRouter,
            true,
            std::unordered_set<std::shared_ptr<ConnectionStatusObserverInterface>>(),
            {validMessageObserver}));
    // create should pass with empty set of MessageObservers
    ASSERT_NE(
        nullptr,
        m_avsConnectionManager->create(
            m_messageRouter,
            true,
            {validConnectionStatusObserver},
            std::unordered_set<std::shared_ptr<MessageObserverInterface>>()));
    // create should pass with valid messageRouter, ConnectionStatusObservers and MessageObservers
    ASSERT_NE(
        nullptr,
        m_avsConnectionManager->create(m_messageRouter, true, {validConnectionStatusObserver}, {validMessageObserver}));
}

/**
 * Test addConnectionStatusObserver with a @c nullptr observer, expecting no errors.
 */
TEST_F(AVSConnectionManagerTest, test_addConnectionStatusObserverNull) {
    EXPECT_CALL(*m_messageRouter, getConnectionStatus()).Times(0);
    m_avsConnectionManager->addConnectionStatusObserver(nullptr);
}

/**
 * Test with addConnectionStatusObserver with MockConnectionStatusObserver.
 */
TEST_F(AVSConnectionManagerTest, test_addConnectionStatusObserverValid) {
    EXPECT_CALL(*m_observer, onConnectionStatusChanged(_, _)).Times(1);
    m_avsConnectionManager->addConnectionStatusObserver(m_observer);
}

/**
 * Test removeConnectionStatusObserver with a @c nullptr observer, expecting no errors.
 */
TEST_F(AVSConnectionManagerTest, test_removeConnectionStatusObserverNull) {
    m_avsConnectionManager->removeConnectionStatusObserver(nullptr);
}

/**
 * Test addMessageObserver with a @c nullptr observer, expecting no errors.
 */
TEST_F(AVSConnectionManagerTest, test_addMessageObserverNull) {
    m_avsConnectionManager->addMessageObserver(nullptr);
}

/**
 * Test removeMessageObserver with a @c nullptr observer, expecting no errors.
 */
TEST_F(AVSConnectionManagerTest, test_removeMessageObserverNull) {
    m_avsConnectionManager->removeMessageObserver(nullptr);
}

/**
 * Test enable and disable function of AVSConnectionManager
 */
TEST_F(AVSConnectionManagerTest, test_enableAndDisableFunction) {
    EXPECT_CALL(*m_messageRouter, enable()).Times(1);
    m_avsConnectionManager->enable();
    ASSERT_TRUE(m_avsConnectionManager->isEnabled());
    EXPECT_CALL(*m_messageRouter, disable()).Times(1);
    m_avsConnectionManager->disable();
    ASSERT_FALSE(m_avsConnectionManager->isEnabled());
}

/**
 * Tests sendMessage with a @c nullptr request, expecting no errors.
 */
TEST_F(AVSConnectionManagerTest, test_sendMessageRequest) {
    EXPECT_CALL(*m_messageRouter, sendMessage(_)).Times(1);
    m_avsConnectionManager->sendMessage(nullptr);
    EXPECT_CALL(*m_messageRouter, sendMessage(_)).Times(1);
    std::shared_ptr<avsCommon::avs::MessageRequest> messageRequest;
    messageRequest = std::make_shared<avsCommon::avs::MessageRequest>("Test message");
    m_avsConnectionManager->sendMessage(messageRequest);
}

/**
 * Test setAVSGateway and expect a call to messageRouter's setAVSGateway.
 */
TEST_F(AVSConnectionManagerTest, test_setAVSGateway) {
    EXPECT_CALL(*m_messageRouter, setAVSGateway(_)).Times(1);
    m_avsConnectionManager->setAVSGateway("AVSGateway");
}

/**
 * Test getAVSGateway and expect a call to messageRouter's getAVSGateway.
 */
TEST_F(AVSConnectionManagerTest, getAVSGatewayTest) {
    auto gateway = "AVSGateway";
    EXPECT_CALL(*m_messageRouter, getAVSGateway()).Times(1).WillOnce(Return(gateway));
    ASSERT_EQ(gateway, m_avsConnectionManager->getAVSGateway());
}

/**
 * Test that onConnectionStatusChanged(false) results in prompting MessageRouter to verify the connection.
 */
TEST_F(AVSConnectionManagerTest, test_enabledOnConnectStatusChangedToFalse) {
    // Create a new MessageRouter so we don't get residual calls to m_messageRouter from SetUp().
    auto messageRouter = std::make_shared<MockMessageRouter>();

    {
        InSequence dummy;
        EXPECT_CALL(*messageRouter, enable());
        EXPECT_CALL(*messageRouter, onWakeVerifyConnectivity());
    }

    m_avsConnectionManager = AVSConnectionManager::create(
        messageRouter,
        true,
        std::unordered_set<std::shared_ptr<ConnectionStatusObserverInterface>>(),
        std::unordered_set<std::shared_ptr<MessageObserverInterface>>());
    m_avsConnectionManager->onConnectionStatusChanged(false);
    // Explicitly reset so we control when destructor is called and can set expectations accordingly.
    m_avsConnectionManager.reset();
}

/**
 * Test that onConnectionStatusChanged(true) results in prompting MessageRouter to reconnect.
 */
TEST_F(AVSConnectionManagerTest, test_enabledOnConnectStatusChangedToTrue) {
    // Create a new MessageRouter so we don't get residual calls to m_messageRouter from SetUp().
    auto messageRouter = std::make_shared<MockMessageRouter>();

    {
        InSequence dummy;
        EXPECT_CALL(*messageRouter, enable());
        EXPECT_CALL(*messageRouter, onWakeConnectionRetry());
    }

    m_avsConnectionManager = AVSConnectionManager::create(
        messageRouter,
        true,
        std::unordered_set<std::shared_ptr<ConnectionStatusObserverInterface>>(),
        std::unordered_set<std::shared_ptr<MessageObserverInterface>>());
    m_avsConnectionManager->onConnectionStatusChanged(true);
    // Explicitly reset so we control when destructor is called and can set expectations accordingly.
    m_avsConnectionManager.reset();
}

/**
 * Test that onConnectionStatusChanged() results in no reconnect attempts or connectivity checks.
 */
TEST_F(AVSConnectionManagerTest, test_disabledOnConnectStatusChanged) {
    // Create a new MessageRouter so we don't get residual calls to m_messageRouter from SetUp().
    auto messageRouter = std::make_shared<MockMessageRouter>();

    {
        InSequence dummy;
        EXPECT_CALL(*messageRouter, enable()).Times(0);
        EXPECT_CALL(*messageRouter, disable()).Times(0);
        EXPECT_CALL(*messageRouter, onWakeVerifyConnectivity()).Times(0);
        EXPECT_CALL(*messageRouter, onWakeConnectionRetry()).Times(0);
    }

    m_avsConnectionManager = AVSConnectionManager::create(
        messageRouter,
        false,
        std::unordered_set<std::shared_ptr<ConnectionStatusObserverInterface>>(),
        std::unordered_set<std::shared_ptr<MessageObserverInterface>>());
    m_avsConnectionManager->onConnectionStatusChanged(true);
    m_avsConnectionManager->onConnectionStatusChanged(false);
    // Explicitly reset so we control when destructor is called and can set expectations accordingly.
    m_avsConnectionManager.reset();
}

}  // namespace test
}  // namespace acl
}  // namespace alexaClientSDK

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
