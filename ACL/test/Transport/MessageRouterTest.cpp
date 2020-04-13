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

#include "MessageRouterTest.h"

#include <gtest/gtest.h>

namespace alexaClientSDK {
namespace acl {
namespace test {

using namespace alexaClientSDK::avsCommon::sdkInterfaces;

TEST_F(MessageRouterTest, test_getConnectionStatusReturnsDisconnectedBeforeConnect) {
    ASSERT_EQ(m_router->getConnectionStatus().first, ConnectionStatusObserverInterface::Status::DISCONNECTED);
}

TEST_F(MessageRouterTest, test_getConnectionStatusReturnsPendingAfterConnectingStarts) {
    setupStateToPending();
    ASSERT_EQ(m_router->getConnectionStatus().first, ConnectionStatusObserverInterface::Status::PENDING);
}

TEST_F(MessageRouterTest, test_getConnectionStatusReturnsConnectedAfterConnectionEstablished) {
    setupStateToConnected();
    ASSERT_EQ(m_router->getConnectionStatus().first, ConnectionStatusObserverInterface::Status::CONNECTED);
}

TEST_F(MessageRouterTest, test_getConnectionStatusReturnsConnectedAfterDisconnected) {
    m_router->onDisconnected(m_mockTransport, ConnectionStatusObserverInterface::ChangedReason::ACL_DISABLED);
    ASSERT_EQ(m_router->getConnectionStatus().first, ConnectionStatusObserverInterface::Status::DISCONNECTED);
}

TEST_F(MessageRouterTest, test_ensureTheMessageRouterObserverIsInformedOfConnectionPendingAfterConnect) {
    setupStateToPending();

    // wait for the result to propagate by scheduling a task on the client executor
    waitOnMessageRouter(SHORT_TIMEOUT_MS);

    ASSERT_EQ(
        m_mockMessageRouterObserver->getLatestConnectionStatus(), ConnectionStatusObserverInterface::Status::PENDING);
    ASSERT_EQ(
        m_mockMessageRouterObserver->getLatestConnectionChangedReason(),
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);
}

TEST_F(MessageRouterTest, test_ensureTheMessageRouterObserverIsInformedOfNewConnection) {
    setupStateToConnected();

    // wait for the result to propagate by scheduling a task on the client executor
    waitOnMessageRouter(SHORT_TIMEOUT_MS);

    ASSERT_EQ(
        m_mockMessageRouterObserver->getLatestConnectionStatus(), ConnectionStatusObserverInterface::Status::CONNECTED);
    ASSERT_EQ(
        m_mockMessageRouterObserver->getLatestConnectionChangedReason(),
        ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);
}

TEST_F(MessageRouterTest, test_ensureTheMessageRouterObserverIsInformedOfTransportDisconnection) {
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

TEST_F(MessageRouterTest, test_ensureTheMessageRouterObserverIsInformedOfRouterDisconnection) {
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

TEST_F(MessageRouterTest, test_sendIsSuccessfulWhenConnected) {
    setupStateToConnected();

    auto messageRequest = createMessageRequest();

    // Expect to have the message sent to the transport
    EXPECT_CALL(*m_mockTransport, onRequestEnqueued()).Times(1);

    m_router->sendMessage(messageRequest);

    // Since we connected we will be disconnected when the router is destroyed
    EXPECT_CALL(*m_mockTransport, disconnect()).Times(AnyNumber());
}

TEST_F(MessageRouterTest, test_sendFailsWhenDisconnected) {
    auto messageRequest = createMessageRequest();

    // Expect to have the message to be enqueued but the transport is not notified
    EXPECT_CALL(*m_mockTransport, onRequestEnqueued()).Times(0);

    m_router->sendMessage(messageRequest);
}

TEST_F(MessageRouterTest, test_sendFailsWhenPending) {
    // Ensure a transport exists
    initializeMockTransport(m_mockTransport.get());
    m_router->enable();

    auto messageRequest = createMessageRequest();

    // Expect to have the message sent to the transport.
    EXPECT_CALL(*m_mockTransport, onRequestEnqueued()).Times(1);

    m_router->sendMessage(messageRequest);
    waitOnMessageRouter(SHORT_TIMEOUT_MS);
}

TEST_F(MessageRouterTest, test_sendMessageDoesNotSendAfterDisconnected) {
    setupStateToConnected();

    auto messageRequest = createMessageRequest();

    EXPECT_CALL(*m_mockTransport, doShutdown()).Times(AtLeast(1));
    m_router->disable();

    // Expect to have the message sent to the transport
    EXPECT_CALL(*m_mockTransport, onRequestEnqueued()).Times(0);

    m_router->sendMessage(messageRequest);
}

TEST_F(MessageRouterTest, test_disconnectDisconnectsConnectedTransports) {
    setupStateToConnected();

    EXPECT_CALL(*m_mockTransport, doShutdown()).Times(1);

    m_router->disable();
}

TEST_F(MessageRouterTest, test_serverSideDisconnectCreatesANewTransport) {
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

    EXPECT_CALL(*oldTransport.get(), onRequestEnqueued()).Times(0);

    EXPECT_CALL(*newTransport.get(), onRequestEnqueued()).Times(1);

    m_router->sendMessage(messageRequest);

    waitOnMessageRouter(SHORT_TIMEOUT_MS);
}

/**
 * This tests the calling of private method @c receive() for MessageRouterObserver from MessageRouter
 */
TEST_F(MessageRouterTest, test_onReceive) {
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
TEST_F(MessageRouterTest, test_onConnectionStatusChanged) {
    m_mockMessageRouterObserver->reset();
    setupStateToConnected();
    waitOnMessageRouter(SHORT_TIMEOUT_MS);
    ASSERT_TRUE(m_mockMessageRouterObserver->wasNotifiedOfStatusChange());
}

/**
 * Verify that when enable is called with active connection is pending
 * we don't create a new connection.
 */
TEST_F(MessageRouterTest, test_enableTwiceOnPendingTransport) {
    setupStateToPending();
    waitOnMessageRouter(SHORT_TIMEOUT_MS);
    m_mockMessageRouterObserver->reset();

    EXPECT_CALL(*m_mockTransport, connect()).Times(0);

    m_router->enable();

    ASSERT_FALSE(m_mockMessageRouterObserver->wasNotifiedOfStatusChange());
}

/**
 * Verify that if onConnected is called
 * for inactive transport, we don't notify the observers and
 * closing the connection.
 */
TEST_F(MessageRouterTest, test_onConnectedOnInactiveTransport) {
    auto transport = std::make_shared<MockTransport>();
    m_router->onConnected(transport);
    ASSERT_FALSE(m_mockMessageRouterObserver->wasNotifiedOfStatusChange());
}

TEST_F(MessageRouterTest, setAndGetAVSGateway) {
    auto gateway = "Gateway";
    m_router->setAVSGateway(gateway);
    ASSERT_EQ(gateway, m_router->getAVSGateway());
}

}  // namespace test
}  // namespace acl
}  // namespace alexaClientSDK
