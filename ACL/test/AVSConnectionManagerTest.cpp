/*
 * AVSConnectionManagerTest.cpp
 *
 * Copyright 2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "ACL/AVSConnectionManager.h"

namespace alexaClientSDK {
namespace acl {
namespace test {

using namespace ::testing;

/**
 * This class allows us to test MessageRouter interaction.
 */
class MockMessageRouter : public MessageRouterInterface {
public:

    MockMessageRouter() { }

    MOCK_METHOD0(enable, void());
    MOCK_METHOD0(disable, void());
    MOCK_METHOD0(getConnectionStatus, ConnectionStatusObserverInterface::Status());
    MOCK_METHOD1(send, void(std::shared_ptr<avsCommon::avs::MessageRequest> request));
    MOCK_METHOD1(setAVSEndpoint, void(const std::string& avsEndpoint));
    MOCK_METHOD1(setObserver, void(std::shared_ptr<MessageRouterObserverInterface> observer));

};

class AVSConnectionManagerTest : public ::testing::Test {
public:
    AVSConnectionManagerTest() {
        m_messageRouter = std::make_shared<MockMessageRouter>();
        m_avsConnectionManager = AVSConnectionManager::create(m_messageRouter,
                                                              true,
                                                              nullptr,
                                                              nullptr);
    }

protected:
    std::shared_ptr<AVSConnectionManager> m_avsConnectionManager;
    std::shared_ptr<MockMessageRouter> m_messageRouter;
};

} // namespace test
} // namespace acl
} // namespace alexaClientSDK
