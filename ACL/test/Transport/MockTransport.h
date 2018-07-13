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

#ifndef ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKTRANSPORT_H_
#define ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKTRANSPORT_H_

#include <AVSCommon/AVS/MessageRequest.h>

#include "ACL/Transport/TransportInterface.h"

#include <gmock/gmock.h>

#include <memory>

namespace alexaClientSDK {
namespace acl {
namespace transport {
namespace test {

using ::testing::Return;

class MockTransport : public TransportInterface {
public:
    MockTransport() : m_id{sNewId++} {};

    MOCK_METHOD0(doShutdown, void());
    MOCK_METHOD0(connect, bool());
    MOCK_METHOD0(disconnect, void());
    MOCK_METHOD0(isConnected, bool());
    MOCK_METHOD0(isPendingDisconnected, bool());
    MOCK_METHOD1(send, void(std::shared_ptr<avsCommon::avs::MessageRequest>));
    MOCK_METHOD2(onAttachmentReceived, void(const std::string& contextId, const std::string& message));

    const int m_id;

private:
    static int sNewId;
};

int MockTransport::sNewId = 0;

/**
 * Puts the mock transport into a ready to connect state.
 */
void initializeMockTransport(MockTransport* transport) {
    ON_CALL(*transport, connect()).WillByDefault(Return(true));
    ON_CALL(*transport, isConnected()).WillByDefault(Return(false));
}

/**
 * Puts the mock transport into a connected state.
 */
void connectMockTransport(MockTransport* transport) {
    initializeMockTransport(transport);
    ON_CALL(*transport, isConnected()).WillByDefault(Return(true));
}

/**
 * Puts the mock transport into a disconnected state.
 */
void disconnectMockTransport(MockTransport* transport) {
    ON_CALL(*transport, isConnected()).WillByDefault(Return(false));
}

}  // namespace test
}  // namespace transport
}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKTRANSPORT_H_
