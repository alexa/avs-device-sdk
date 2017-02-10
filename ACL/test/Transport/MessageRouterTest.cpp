/**
 * MessageRouterTest.cpp
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

TEST_F(MessageRouterTest, getConnectionStatusReturnsDisconnectedBeforeConnect) {
    ASSERT_EQ(m_router.getConnectionStatus(), ConnectionStatus::DISCONNECTED);
}

TEST_F(MessageRouterTest, getConnectionStatusReturnsPendingAfterConnectingStarts) {
    setupStateToPending();
    ASSERT_EQ(m_router.getConnectionStatus(), ConnectionStatus::PENDING);
}

TEST_F(MessageRouterTest, getConnectionStatusReturnsConnectedAfterConnectionEstablished) {
    setupStateToConnected();
    ASSERT_EQ(m_router.getConnectionStatus(), ConnectionStatus::CONNECTED);
}

TEST_F(MessageRouterTest, getConnectionStatusReturnsConnectedAfterDisconnected) {
    m_router.onDisconnected(ConnectionChangedReason::ACL_DISABLED);
    ASSERT_EQ(m_router.getConnectionStatus(), ConnectionStatus::DISCONNECTED);
}

TEST_F(MessageRouterTest, ensureTheMessageRouterObserverIsInformedOfConnectionPendingAfterConnect) {
    setupStateToPending();

    // wait for the result to propogate by scheduling a task on the client executor
    waitOnExecutor(m_receiveExecutor, SHORT_TIMEOUT_MS);

    ASSERT_EQ(m_mockMessageRouterObserver->getLatestConnectionStatus(), ConnectionStatus::PENDING);
    ASSERT_EQ(m_mockMessageRouterObserver->getLatestConnectionChangedReason(), ConnectionChangedReason::ACL_CLIENT_REQUEST);
}

TEST_F(MessageRouterTest, ensureTheMessageRouterObserverIsInformedOfNewConnection) {
    setupStateToConnected();

    // wait for the result to propogate by scheduling a task on the client executor
    waitOnExecutor(m_receiveExecutor, SHORT_TIMEOUT_MS);

    ASSERT_EQ(m_mockMessageRouterObserver->getLatestConnectionStatus(), ConnectionStatus::CONNECTED);
    ASSERT_EQ(m_mockMessageRouterObserver->getLatestConnectionChangedReason(), ConnectionChangedReason::ACL_CLIENT_REQUEST);
}

TEST_F(MessageRouterTest, ensureTheMessageRouterObserverIsInformedOfTransportDisconnection) {
    setupStateToConnected();

    auto reason = ConnectionChangedReason::ACL_DISABLED;
    disconnectMockTransport(m_mockTransport.get());
    m_router.onDisconnected(reason);

    // wait for the result to propogate by scheduling a task on the client executor
    waitOnExecutor(m_receiveExecutor, SHORT_TIMEOUT_MS);

    ASSERT_EQ(m_mockMessageRouterObserver->getLatestConnectionStatus(), ConnectionStatus::DISCONNECTED);
    ASSERT_EQ(m_mockMessageRouterObserver->getLatestConnectionChangedReason(), reason);
}

TEST_F(MessageRouterTest, ensureTheMessageRouterObserverIsInformedOfRouterDisconnection) {
    setupStateToConnected();

    m_router.disable();

    // wait for the result to propogate by scheduling a task on the client executor
    waitOnExecutor(m_receiveExecutor, SHORT_TIMEOUT_MS);

    ASSERT_EQ(m_mockMessageRouterObserver->getLatestConnectionStatus(), ConnectionStatus::DISCONNECTED);
    ASSERT_EQ(m_mockMessageRouterObserver->getLatestConnectionChangedReason(),
              ConnectionChangedReason::ACL_CLIENT_REQUEST);
}

TEST_F(MessageRouterTest, ensureTheMessageRouterObserverIsInformedOfReceivedMessages) {
    auto message = createMessage();

    // receive it
    m_router.onMessageReceived(message);

    // wait for the result to propogate by scheduling a task on the client executor
    waitOnExecutor(m_receiveExecutor, SHORT_TIMEOUT_MS);

    // verify the observer received the same message
    auto receivedMessage = m_mockMessageRouterObserver->getLatestMessage();
    ASSERT_EQ(receivedMessage->getJSONContent(), MESSAGE);

    // unsure if this is the best way to verify this, but it works
    char line[MESSAGE_LENGTH];
    receivedMessage->getAttachment()->getline(line, MESSAGE_LENGTH);
    std::string receivedAttachmentString(line);
    ASSERT_EQ(receivedAttachmentString, MESSAGE);
}

TEST_F(MessageRouterTest, sendIsSuccessfulWhenConnected) {
    setupStateToConnected();

    auto messageRequest = createMessageRequest();

    // Expect to have the message sent to the transport
    EXPECT_CALL(*m_mockTransport, send(messageRequest)).Times(1);

    m_router.send(messageRequest);
    waitOnExecutor(m_sendExecutor, SHORT_TIMEOUT_MS);

    // Since we connected we will be disconnected when the router is destroyed
    EXPECT_CALL(*m_mockTransport, disconnect()).Times(AnyNumber());
}

TEST_F(MessageRouterTest, sendFailsWhenDisconnected) {
    auto messageRequest = createMessageRequest();

    // Expect to have the message sent to the transport
    EXPECT_CALL(*m_mockTransport, send(messageRequest)).Times(0);

    m_router.send(messageRequest);
    waitOnExecutor(m_sendExecutor, SHORT_TIMEOUT_MS);
}

TEST_F(MessageRouterTest, sendFailsWhenPending) {
    // Ensure a transport exists
    initializeMockTransport(m_mockTransport.get());
    m_router.enable();

    auto messageRequest = createMessageRequest();

    // Expect to have the message sent to the transport
    EXPECT_CALL(*m_mockTransport, send(messageRequest)).Times(0);

    m_router.send(messageRequest);
    waitOnExecutor(m_sendExecutor, SHORT_TIMEOUT_MS);
    waitOnExecutor(m_receiveExecutor, SHORT_TIMEOUT_MS);
}

TEST_F(MessageRouterTest, sendMessageDoesNotSendAfterDisconnected) {
    setupStateToConnected();

    auto messageRequest = createMessageRequest();

    EXPECT_CALL(*m_mockTransport, disconnect()).Times(AtLeast(1));
    m_router.disable();

    // Expect to have the message sent to the transport
    EXPECT_CALL(*m_mockTransport, send(messageRequest)).Times(0);

    m_router.send(messageRequest);
    waitOnExecutor(m_sendExecutor, SHORT_TIMEOUT_MS);
}

TEST_F(MessageRouterTest, disconnectDisconnectsConnectedTransports) {
    setupStateToConnected();

    EXPECT_CALL(*m_mockTransport, disconnect()).Times(1);

    m_router.disable();
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

    m_router.setMockTransport(newTransport);

    // Reset the MessageRouterObserver, there should be no interactions with the observer
    m_router.onServerSideDisconnect();

    waitOnExecutor(m_receiveExecutor, SHORT_TIMEOUT_MS);

    ASSERT_EQ(m_mockMessageRouterObserver->getLatestConnectionStatus(), ConnectionStatus::PENDING);
    ASSERT_EQ(
            m_mockMessageRouterObserver->getLatestConnectionChangedReason(),
            ConnectionChangedReason::SERVER_SIDE_DISCONNECT);

    // mock the new transports connection
    connectMockTransport(newTransport.get());
    m_router.onConnected();

    waitOnExecutor(m_receiveExecutor, SHORT_TIMEOUT_MS);

    ASSERT_EQ(m_mockMessageRouterObserver->getLatestConnectionStatus(), ConnectionStatus::CONNECTED);
    ASSERT_EQ(
            m_mockMessageRouterObserver->getLatestConnectionChangedReason(),
            ConnectionChangedReason::ACL_CLIENT_REQUEST);

    // mock the old transport disconnecting completely
    disconnectMockTransport(oldTransport.get());
    m_router.onDisconnected(ConnectionChangedReason::ACL_CLIENT_REQUEST);

    auto messageRequest = createMessageRequest();

    EXPECT_CALL(*oldTransport.get(), send(messageRequest)).Times(0);

    EXPECT_CALL(*newTransport.get(), send(messageRequest)).Times(1);

    m_router.send(messageRequest);

    waitOnExecutor(m_sendExecutor, SHORT_TIMEOUT_MS);

}

} // namespace acl
} // namespace alexaClientSDK