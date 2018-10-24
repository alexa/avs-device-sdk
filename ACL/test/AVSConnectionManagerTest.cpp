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

/// @file AVSConnectionManagerTest.cpp

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include "ACL/AVSConnectionManager.h"

namespace alexaClientSDK {
namespace acl {
namespace test {

using namespace ::testing;
using namespace alexaClientSDK::avsCommon::avs::initialization;
using namespace alexaClientSDK::avsCommon::sdkInterfaces;

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
    MOCK_METHOD1(setAVSEndpoint, void(const std::string& avsEndpoint));
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
};

void AVSConnectionManagerTest::SetUp() {
    AlexaClientSDKInit::initialize(std::vector<std::shared_ptr<std::istream>>());
    m_messageRouter = std::make_shared<MockMessageRouter>();
    m_observer = std::make_shared<MockConnectionStatusObserver>();
    m_messageObserver = std::make_shared<MockMessageObserver>();
    m_avsConnectionManager = AVSConnectionManager::create(
        m_messageRouter,
        true,
        std::unordered_set<std::shared_ptr<ConnectionStatusObserverInterface>>(),
        std::unordered_set<std::shared_ptr<MessageObserverInterface>>());
}

void AVSConnectionManagerTest::TearDown() {
    AlexaClientSDKInit::uninitialize();
}

/**
 * Test @c create with valid messageRouter, ConnectionStatusObserver, MessageObservers
 */
TEST_F(AVSConnectionManagerTest, createTest) {
    EXPECT_CALL(*m_messageRouter, setObserver(_)).Times(1);
    EXPECT_CALL(*m_messageRouter, enable()).Times(1);
    ASSERT_NE(nullptr, m_avsConnectionManager->create(m_messageRouter, true, {m_observer}, {m_messageObserver}));
}

/**
 * Test @c create with different combinations of messageRouter, ConnectionStatusObserver, MessageObservers
 */
TEST_F(AVSConnectionManagerTest, createWithNullMessageRouterAndObservers) {
    ASSERT_EQ(nullptr, m_avsConnectionManager->create(nullptr, true, {m_observer}, {m_messageObserver}));
    ASSERT_EQ(nullptr, m_avsConnectionManager->create(m_messageRouter, true, {nullptr}, {m_messageObserver}));
    ASSERT_EQ(nullptr, m_avsConnectionManager->create(m_messageRouter, true, {m_observer}, {nullptr}));
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
TEST_F(AVSConnectionManagerTest, addConnectionStatusObserverNull) {
    EXPECT_CALL(*m_messageRouter, getConnectionStatus()).Times(0);
    m_avsConnectionManager->addConnectionStatusObserver(nullptr);
}

/**
 * Test with addConnectionStatusObserver with MockConnectionStatusObserver.
 */
TEST_F(AVSConnectionManagerTest, addConnectionStatusObserverValid) {
    EXPECT_CALL(*m_observer, onConnectionStatusChanged(_, _)).Times(1);
    m_avsConnectionManager->addConnectionStatusObserver(m_observer);
}

/**
 * Test removeConnectionStatusObserver with a @c nullptr observer, expecting no errors.
 */
TEST_F(AVSConnectionManagerTest, removeConnectionStatusObserverNull) {
    m_avsConnectionManager->removeConnectionStatusObserver(nullptr);
}

/**
 * Test addMessageObserver with a @c nullptr observer, expecting no errors.
 */
TEST_F(AVSConnectionManagerTest, addMessageObserverNull) {
    m_avsConnectionManager->addMessageObserver(nullptr);
}

/**
 * Test removeMessageObserver with a @c nullptr observer, expecting no errors.
 */
TEST_F(AVSConnectionManagerTest, removeMessageObserverNull) {
    m_avsConnectionManager->removeMessageObserver(nullptr);
}

/**
 * Test enable and disable function of AVSConnectionManager
 */
TEST_F(AVSConnectionManagerTest, enableAndDisableFunction) {
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
TEST_F(AVSConnectionManagerTest, sendMessageRequestTest) {
    EXPECT_CALL(*m_messageRouter, sendMessage(_)).Times(1);
    m_avsConnectionManager->sendMessage(nullptr);
    EXPECT_CALL(*m_messageRouter, sendMessage(_)).Times(1);
    std::shared_ptr<avsCommon::avs::MessageRequest> messageRequest;
    messageRequest = std::make_shared<avsCommon::avs::MessageRequest>("Test message");
    m_avsConnectionManager->sendMessage(messageRequest);
}

/**
 * Test setAVSEndpoint and expect a call to messageRouter's setAVSEndpoint.
 */
TEST_F(AVSConnectionManagerTest, setAVSEndpointTest) {
    EXPECT_CALL(*m_messageRouter, setAVSEndpoint(_)).Times(1);
    m_avsConnectionManager->setAVSEndpoint("AVSEndpoint");
}
}  // namespace test
}  // namespace acl
}  // namespace alexaClientSDK

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
