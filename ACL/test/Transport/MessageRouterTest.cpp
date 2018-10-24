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

#include "MessageRouterTest.h"

#include <gtest/gtest.h>

namespace alexaClientSDK {
namespace acl {
namespace test {

using namespace alexaClientSDK::avsCommon::sdkInterfaces;

TEST_F(MessageRouterTest, getConnectionStatusReturnsDisconnectedBeforeConnect) {
    ASSERT_EQ(m_router->getConnectionStatus().first, ConnectionStatusObserverInterface::Status::DISCONNECTED);
}

TEST_F(MessageRouterTest, getConnectionStatusReturnsPendingAfterConnectingStarts) {
    setupStateToPending();
    ASSERT_EQ(m_router->getConnectionStatus().first, ConnectionStatusObserverInterface::Status::PENDING);
}

TEST_F(MessageRouterTest, getConnectionStatusReturnsConnectedAfterConnectionEstablished) {
    setupStateToConnected();
    ASSERT_EQ(m_router->getConnectionStatus().first, ConnectionStatusObserverInterface::Status::CONNECTED);
}

TEST_F(MessageRouterTest, getConnectionStatusReturnsConnectedAfterDisconnected) {
    m_router->onDisconnected(m_mockTransport, ConnectionStatusObserverInterface::ChangedReason::ACL_DISABLED);
    ASSERT_EQ(m_router->getConnectionStatus().first, ConnectionStatusObserverInterface::Status::DISCONNECTED);
}

TEST_F(MessageRouterTest, ensureTheMessageRouterObserverIsInformedOfConnectionPendingAfterConnect) {
    setupStateToPending();

    // wait for the result to propagate by scheduling a task on the client executor
    waitOnMessageRouter(SHORT_TIMEOUT_MS);

    ASSERT_EQ(
        m_mockMessageRouterObserver->getLatestConnectionStatus(), ConnectionStatusObserverInterface::Status::PENDING);
    ASSERT_EQ(
        m_mockMessageRouterObserver->getLatestConnectionChangedReason(),
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);
}

TEST_F(MessageRouterTest, ensureTheMessageRouterObserverIsInformedOfNewConnection) {
    setupStateToConnected();

    // wait for the result to propagate by scheduling a task on the client executor
    waitOnMessageRouter(SHORT_TIMEOUT_MS);

    ASSERT_EQ(
        m_mockMessageRouterObserver->getLatestConnectionStatus(), ConnectionStatusObserverInterface::Status::CONNECTED);
    ASSERT_EQ(
        m_mockMessageRouterObserver->getLatestConnectionChangedReason(),
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);
}

TEST_F(MessageRouterTest, ensureTheMessageRouterObserverIsInformedOfTransportDisconnection) {
    setupStateToConnected();

    auto reason = ConnectionStatusObserverInterface::ChangedReason::ACL_DISABLED;
    disconnectMockTransport(m_mockTransport.get());
    m_router->onDisconnected(m_mockTransport, reason);

    // wait for the result to propagate by scheduling a task on the client executor
    waitOnMessageRouter(SHORT_TIMEOUT_MS);

    ASSERT_EQ(
        m_mockMessageRouterObserver->getLatestConnectionStatus(), ConnectionStatusObserverInterface::Status::PENDING);
    ASSERT_EQ(m_mockMessageRouterObserver->getLatestConnectionChangedReason(), reason);
}

TEST_F(MessageRouterTest, ensureTheMessageRouterObserverIsInformedOfRouterDisconnection) {
    setupStateToConnected();

    m_router->disable();

    // wait for the result to propagate by scheduling a task on the client executor
    waitOnMessageRouter(SHORT_TIMEOUT_MS);

    ASSERT_EQ(
        m_mockMessageRouterObserver->getLatestConnectionStatus(),
        ConnectionStatusObserverInterface::Status::DISCONNECTED);
    ASSERT_EQ(
        m_mockMessageRouterObserver->getLatestConnectionChangedReason(),
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);
}

TEST_F(MessageRouterTest, sendIsSuccessfulWhenConnected) {
    setupStateToConnected();

    auto messageRequest = createMessageRequest();

    // Expect to have the message sent to the transport
    EXPECT_CALL(*m_mockTransport, send(messageRequest)).Times(1);

    m_router->sendMessage(messageRequest);

    // Since we connected we will be disconnected when the router is destroyed
    EXPECT_CALL(*m_mockTransport, disconnect()).Times(AnyNumber());
}

TEST_F(MessageRouterTest, sendFailsWhenDisconnected) {
    auto messageRequest = createMessageRequest();

    // Expect to have the message sent to the transport
    EXPECT_CALL(*m_mockTransport, send(messageRequest)).Times(0);

    m_router->sendMessage(messageRequest);
}

TEST_F(MessageRouterTest, sendFailsWhenPending) {
    // Ensure a transport exists
    initializeMockTransport(m_mockTransport.get());
    m_router->enable();

    auto messageRequest = createMessageRequest();

    // Expect to have the message sent to the transport.
    EXPECT_CALL(*m_mockTransport, send(messageRequest)).Times(1);

    m_router->sendMessage(messageRequest);
    waitOnMessageRouter(SHORT_TIMEOUT_MS);
}

TEST_F(MessageRouterTest, sendMessageDoesNotSendAfterDisconnected) {
    setupStateToConnected();

    auto messageRequest = createMessageRequest();

    EXPECT_CALL(*m_mockTransport, doShutdown()).Times(AtLeast(1));
    m_router->disable();

    // Expect to have the message sent to the transport
    EXPECT_CALL(*m_mockTransport, send(messageRequest)).Times(0);

    m_router->sendMessage(messageRequest);
}

TEST_F(MessageRouterTest, disconnectDisconnectsConnectedTransports) {
    setupStateToConnected();

    EXPECT_CALL(*m_mockTransport, doShutdown()).Times(1);

    m_router->disable();
}

TEST_F(MessageRouterTest, serverSideDisconnectCreatesANewTransport) {
    /*
     * This test is difficult to setup in a nice way. The idea is to replace the original
     * transport with a new one, call onServerSideDisconnect to make it the new active
     * transport, and then send a message. The message should be sent on the new transport.
     */
    setupStateToConnected();

    auto oldTransport = m_mockTransport;

    auto newTransport = std::make_shared<NiceMock<MockTransport>>();
    initializeMockTransport(newTransport.get());

    m_transportFactory->setMockTransport(newTransport);

    // Reset the MessageRouterObserver, there should be no interactions with the observer
    m_router->onServerSideDisconnect(oldTransport);

    waitOnMessageRouter(SHORT_TIMEOUT_MS);

    ASSERT_EQ(
        m_mockMessageRouterObserver->getLatestConnectionStatus(), ConnectionStatusObserverInterface::Status::PENDING);
    ASSERT_EQ(
        m_mockMessageRouterObserver->getLatestConnectionChangedReason(),
        ConnectionStatusObserverInterface::ChangedReason::SERVER_SIDE_DISCONNECT);

    // mock the new transports connection
    connectMockTransport(newTransport.get());
    m_router->onConnected(newTransport);

    waitOnMessageRouter(SHORT_TIMEOUT_MS);

    ASSERT_EQ(
        m_mockMessageRouterObserver->getLatestConnectionStatus(), ConnectionStatusObserverInterface::Status::CONNECTED);
    ASSERT_EQ(
        m_mockMessageRouterObserver->getLatestConnectionChangedReason(),
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

    // mock the old transport disconnecting completely
    disconnectMockTransport(oldTransport.get());
    m_router->onDisconnected(oldTransport, ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

    auto messageRequest = createMessageRequest();

    EXPECT_CALL(*oldTransport.get(), send(messageRequest)).Times(0);

    EXPECT_CALL(*newTransport.get(), send(messageRequest)).Times(1);

    m_router->sendMessage(messageRequest);

    waitOnMessageRouter(SHORT_TIMEOUT_MS);
}

/**
 * This tests the calling of private method @c receive() for MessageRouterObserver from MessageRouter
 */
TEST_F(MessageRouterTest, onReceiveTest) {
    m_mockMessageRouterObserver->reset();
    m_router->consumeMessage(CONTEXT_ID, MESSAGE);
    waitOnMessageRouter(SHORT_TIMEOUT_MS);
    ASSERT_TRUE(m_mockMessageRouterObserver->wasNotifiedOfReceive());
    ASSERT_EQ(CONTEXT_ID, m_mockMessageRouterObserver->getAttachmentContextId());
    ASSERT_EQ(MESSAGE, m_mockMessageRouterObserver->getLatestMessage());
}

/**
 * This tests the calling of private method @c onConnectionStatusChanged()
 * for MessageRouterObserver from MessageRouter
 */
TEST_F(MessageRouterTest, onConnectionStatusChangedTest) {
    m_mockMessageRouterObserver->reset();
    setupStateToConnected();
    waitOnMessageRouter(SHORT_TIMEOUT_MS);
    ASSERT_TRUE(m_mockMessageRouterObserver->wasNotifiedOfStatusChange());
}
}  // namespace test
}  // namespace acl
}  // namespace alexaClientSDK
