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

#include <istream>
#include <memory>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "AVSCommon/AVS/Attachment/MockAttachmentManager.h"
#include "AVSCommon/AVS/Initialization/AlexaClientSDKInit.h"
#include "AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h"
#include "AVSCommon/Utils/Memory/Memory.h"
#include "RegistrationManager/RegistrationManager.h"
#include "RegistrationManager/CustomerDataManager.h"

namespace alexaClientSDK {
namespace registrationManager {
namespace test {

using avsCommon::utils::memory::make_unique;

class MockMessageRouter : public acl::MessageRouterInterface {
public:
    MockMessageRouter() : MessageRouterInterface{"MockMessageRouter"} {
    }

    MOCK_METHOD0(enable, void());
    MOCK_METHOD0(disable, void());
    MOCK_METHOD0(doShutdown, void());
    MOCK_METHOD0(getConnectionStatus, acl::MessageRouterInterface::ConnectionStatus());
    MOCK_METHOD1(sendMessage, void(std::shared_ptr<avsCommon::avs::MessageRequest> request));
    MOCK_METHOD1(setAVSEndpoint, void(const std::string& avsEndpoint));
    MOCK_METHOD1(setObserver, void(std::shared_ptr<acl::MessageRouterObserverInterface> observer));
};

class MockRegistrationObserver : public RegistrationObserverInterface {
public:
    MOCK_METHOD0(onLogout, void());
};

class MockCustomerDataHandler : public CustomerDataHandler {
public:
    MockCustomerDataHandler(std::shared_ptr<CustomerDataManager> manager) : CustomerDataHandler{manager} {
    }
    MOCK_METHOD0(clearData, void());
};

class RegistrationManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        avsCommon::avs::initialization::AlexaClientSDKInit::initialize(std::vector<std::istream*>());

        m_messageRouter = std::make_shared<MockMessageRouter>();
        EXPECT_CALL(*m_messageRouter, setObserver(testing::_));
        EXPECT_CALL(*m_messageRouter, enable());
        m_avsConnectionManager = acl::AVSConnectionManager::create(m_messageRouter, true);

        auto exceptionEncounteredSender =
            std::make_shared<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender>();
        m_directiveSequencer = adsl::DirectiveSequencer::create(exceptionEncounteredSender);

        m_dataManager = std::make_shared<CustomerDataManager>();
        m_dataHandler = make_unique<MockCustomerDataHandler>(m_dataManager);

        m_registrationManager =
            make_unique<RegistrationManager>(m_directiveSequencer, m_avsConnectionManager, m_dataManager);
        m_registrationObserver = std::make_shared<MockRegistrationObserver>();
        m_registrationManager->addObserver(m_registrationObserver);
    }

    void TearDown() override {
        if (m_directiveSequencer) {
            m_directiveSequencer->shutdown();
        }
    }

    /// Connection manager used during logout.
    std::shared_ptr<acl::AVSConnectionManager> m_avsConnectionManager;
    /// Mock message router.
    std::shared_ptr<MockMessageRouter> m_messageRouter;
    /// Used to check if logout disabled the directive sequencer.
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> m_directiveSequencer;
    /// Mock data handler to ensure that @c clearData() method is called during logout.
    std::unique_ptr<MockCustomerDataHandler> m_dataHandler;
    /// Data manager is used to call @c clearData() on every dataHandler.
    std::shared_ptr<CustomerDataManager> m_dataManager;
    /// Object under test. It is responsible for implementing logout.
    std::unique_ptr<RegistrationManager> m_registrationManager;
    /// Mock registration observer used to check if RegistrationObserver is notified after logout.
    std::shared_ptr<MockRegistrationObserver> m_registrationObserver;
};

/**
 * Test that logout performsa all the following actions:
 * - disable connection manager
 * - disable directive sequencer
 * - clear data handler's data
 * - notify registration observer
 */
TEST_F(RegistrationManagerTest, testLogout) {
    EXPECT_CALL(*m_messageRouter, disable());
    EXPECT_CALL(*m_registrationObserver, onLogout());
    EXPECT_CALL(*m_dataHandler, clearData());

    m_registrationManager->logout();

    ASSERT_FALSE(m_avsConnectionManager->isEnabled());

    // Check that directive sequencer is not processing directives
    std::string context{"context"};
    auto header = std::make_shared<avsCommon::avs::AVSMessageHeader>("namespace", "name", "messageid", "requestid");
    auto attachmentManager = std::make_shared<avsCommon::avs::attachment::test::MockAttachmentManager>();
    std::shared_ptr<avsCommon::avs::AVSDirective> directive =
        avsCommon::avs::AVSDirective::create("unparsed", header, "payload", attachmentManager, context);
    ASSERT_FALSE(m_directiveSequencer->onDirective(directive));
}

}  // namespace test
}  // namespace registrationManager
}  // namespace alexaClientSDK
