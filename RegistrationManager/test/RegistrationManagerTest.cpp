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

#include <AVSCommon/SDKInterfaces/MockAVSConnectionManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveSequencer.h>
#include <AVSCommon/Utils/Memory/Memory.h>

#include "RegistrationManager/RegistrationManager.h"
#include "RegistrationManager/CustomerDataManager.h"

namespace alexaClientSDK {
namespace registrationManager {
namespace test {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace testing;

using avsCommon::utils::memory::make_unique;

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
        m_directiveSequencer = std::make_shared<MockDirectiveSequencer>();
        m_avsConnectionManager = std::make_shared<MockAVSConnectionManager>();
        m_dataManager = std::make_shared<CustomerDataManager>();
        m_dataHandler = make_unique<MockCustomerDataHandler>(m_dataManager);
        m_registrationManager.reset(
            new RegistrationManager(m_directiveSequencer, m_avsConnectionManager, m_dataManager));
        m_registrationObserver = std::make_shared<MockRegistrationObserver>();
        m_registrationManager->addObserver(m_registrationObserver);
    }

    void TearDown() override {
        if (m_directiveSequencer) {
            m_directiveSequencer->shutdown();
        }
    }

    /// Used to check if logout disabled the directive sequencer.
    std::shared_ptr<MockDirectiveSequencer> m_directiveSequencer;
    /// Connection manager used during logout.
    std::shared_ptr<MockAVSConnectionManager> m_avsConnectionManager;
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
 * Test that logout performs all the following actions:
 * - disable connection manager
 * - disable directive sequencer
 * - clear data handler's data
 * - notify registration observer
 */
TEST_F(RegistrationManagerTest, testLogout) {
    EXPECT_CALL(*m_directiveSequencer, disable());
    EXPECT_CALL(*m_avsConnectionManager, disable());
    EXPECT_CALL(*m_registrationObserver, onLogout());
    EXPECT_CALL(*m_dataHandler, clearData());

    m_registrationManager->logout();

    ASSERT_FALSE(m_avsConnectionManager->isEnabled());
}

}  // namespace test
}  // namespace registrationManager
}  // namespace alexaClientSDK
