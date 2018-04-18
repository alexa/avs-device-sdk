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

#ifndef ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MESSAGEROUTERTEST_H_
#define ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MESSAGEROUTERTEST_H_

#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include <string>

#include "AVSCommon/Utils/Threading/Executor.h"
#include "AVSCommon/Utils/Memory/Memory.h"

#include "MockMessageRouterObserver.h"
#include "MockAuthDelegate.h"
#include "MockTransport.h"
#include "ACL/Transport/MessageRouter.h"
#include "ACL/Transport/MessageConsumerInterface.h"

namespace alexaClientSDK {
namespace acl {
namespace test {

using namespace transport;
using namespace transport::test;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::utils::threading;
using namespace avsCommon::utils::memory;

using namespace ::testing;

class TestableMessageRouter : public MessageRouter {
public:
    TestableMessageRouter(
        std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
        std::shared_ptr<AttachmentManager> attachmentManager,
        std::shared_ptr<TransportFactoryInterface> factory,
        const std::string& avsEndpoint) :
            MessageRouter(authDelegate, attachmentManager, factory, avsEndpoint) {
    }

    /**
     * Check if the underlying executor is in ready state within a given wait time.
     *
     * @param time to wait up to specified milliseconds.
     * @return Whether the underlying executor is ready or not.
     */
    bool isExecutorReady(std::chrono::milliseconds millisecondsToWait) {
        auto future = m_executor.submit([]() { ; });
        auto status = future.wait_for(millisecondsToWait);
        return status == std::future_status::ready;
    }
};

class MockTransportFactory : public TransportFactoryInterface {
public:
    MockTransportFactory(std::shared_ptr<MockTransport> transport) : m_mockTransport{transport} {
    }

    void setMockTransport(std::shared_ptr<MockTransport> transport) {
        m_mockTransport = transport;
    }

private:
    std::shared_ptr<TransportInterface> createTransport(
        std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
        std::shared_ptr<AttachmentManager> attachmentManager,
        const std::string& avsEndpoint,
        std::shared_ptr<MessageConsumerInterface> messageConsumerInterface,
        std::shared_ptr<TransportObserverInterface> transportObserverInterface) override {
        return m_mockTransport;
    }

    std::shared_ptr<MockTransport> m_mockTransport;
};

class MessageRouterTest : public ::testing::Test {
public:
    const std::string AVS_ENDPOINT = "AVS_ENDPOINT";

    MessageRouterTest() :
            m_mockMessageRouterObserver{std::make_shared<MockMessageRouterObserver>()},
            m_mockAuthDelegate{std::make_shared<MockAuthDelegate>()},
            m_attachmentManager{std::make_shared<AttachmentManager>(AttachmentManager::AttachmentType::IN_PROCESS)},
            m_mockTransport{std::make_shared<NiceMock<MockTransport>>()},
            m_transportFactory{std::make_shared<MockTransportFactory>(m_mockTransport)},
            m_router{std::make_shared<TestableMessageRouter>(
                m_mockAuthDelegate,
                m_attachmentManager,
                m_transportFactory,
                AVS_ENDPOINT)} {
        m_router->setObserver(m_mockMessageRouterObserver);
    }

    void TearDown() {
        // Wait on MessageRouter to ensure everything is finished
        waitOnMessageRouter(SHORT_TIMEOUT_MS);
    }

    std::shared_ptr<avsCommon::avs::MessageRequest> createMessageRequest() {
        return std::make_shared<avsCommon::avs::MessageRequest>(MESSAGE);
    }
    void waitOnMessageRouter(std::chrono::milliseconds millisecondsToWait) {
        auto status = m_router->isExecutorReady(millisecondsToWait);

        ASSERT_EQ(true, status);
    }
    void setupStateToPending() {
        initializeMockTransport(m_mockTransport.get());
        m_router->enable();
    }

    void setupStateToConnected() {
        setupStateToPending();
        m_router->onConnected(m_mockTransport);
        connectMockTransport(m_mockTransport.get());
    }

    // Ensure the length of MESSAGE always matches MESSAGE_LENGTH - 1 (one for each char and a null terminator)
    static const std::string MESSAGE;
    static const int MESSAGE_LENGTH;
    static const std::chrono::milliseconds SHORT_TIMEOUT_MS;
    static const std::string CONTEXT_ID;

    std::shared_ptr<MockMessageRouterObserver> m_mockMessageRouterObserver;
    std::shared_ptr<MockAuthDelegate> m_mockAuthDelegate;
    std::shared_ptr<AttachmentManager> m_attachmentManager;
    std::shared_ptr<NiceMock<MockTransport>> m_mockTransport;
    std::shared_ptr<MockTransportFactory> m_transportFactory;
    std::shared_ptr<TestableMessageRouter> m_router;
    // TestableMessageRouter m_router;
};

const std::string MessageRouterTest::MESSAGE = "123456789";
const int MessageRouterTest::MESSAGE_LENGTH = 10;
const std::chrono::milliseconds MessageRouterTest::SHORT_TIMEOUT_MS = std::chrono::milliseconds(1000);
const std::string MessageRouterTest::CONTEXT_ID = "contextIdString";

}  // namespace test
}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MESSAGEROUTERTEST_H_
