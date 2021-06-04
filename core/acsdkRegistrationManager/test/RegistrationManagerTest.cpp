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

#include <memory>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/SDKInterfaces/MockAVSConnectionManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveSequencer.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/Metrics/MockMetricRecorder.h>
#include <RegistrationManager/CustomerDataHandler.h>
#include <RegistrationManager/CustomerDataManager.h>
#include <RegistrationManager/RegistrationManager.h>
#include <RegistrationManager/RegistrationNotifier.h>

namespace alexaClientSDK {
namespace registrationManager {
namespace test {

using namespace registrationManager;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils::metrics::test;
using namespace testing;

class MockRegistrationObserver : public RegistrationObserverInterface {
public:
    MOCK_METHOD0(onLogout, void());
};

class MockCustomerDataHandler : public CustomerDataHandler {
public:
    explicit MockCustomerDataHandler(std::shared_ptr<CustomerDataManager> manager) : CustomerDataHandler{manager} {
    }
    MOCK_METHOD0(clearData, void());
};

/**
 * Test harness for @c RegistrationManager.
 */
class RegistrationManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_directiveSequencer = std::make_shared<MockDirectiveSequencer>();
        m_avsConnectionManager = std::make_shared<MockAVSConnectionManager>();
        m_dataManager = std::make_shared<CustomerDataManager>();
        m_dataHandler = avsCommon::utils::memory::make_unique<MockCustomerDataHandler>(m_dataManager);
        m_metricRecorder = std::make_shared<MockMetricRecorder>();
        m_notifier = RegistrationNotifier::createRegistrationNotifierInterface();
        m_registrationObserver = std::make_shared<MockRegistrationObserver>();
        m_notifier->addObserver(m_registrationObserver);
        m_registrationManager = RegistrationManager::createRegistrationManagerInterface(
            m_dataManager, m_notifier, m_avsConnectionManager, m_directiveSequencer, m_metricRecorder);
    }

    void TearDown() override {
        EXPECT_CALL(*m_directiveSequencer, doShutdown());
        if (m_directiveSequencer) {
            m_directiveSequencer->shutdown();
        }
    }

    /// Used to notify logout to observers.
    std::shared_ptr<RegistrationNotifierInterface> m_notifier;
    /// Used to check if logout disabled the directive sequencer.
    std::shared_ptr<MockDirectiveSequencer> m_directiveSequencer;
    /// Connection manager used during logout.
    std::shared_ptr<MockAVSConnectionManager> m_avsConnectionManager;
    /// Mock data handler to ensure that @c clearData() method is called during logout.
    std::unique_ptr<MockCustomerDataHandler> m_dataHandler;
    /// Data manager is used to call @c clearData() on every dataHandler.
    std::shared_ptr<CustomerDataManager> m_dataManager;
    /// Object under test. It is responsible for implementing logout.
    std::shared_ptr<RegistrationManagerInterface> m_registrationManager;
    /// Mock registration observer used to check if RegistrationObserver is notified after logout.
    std::shared_ptr<MockRegistrationObserver> m_registrationObserver;
    /// Mock Metrics recorder.
    std::shared_ptr<MockMetricRecorder> m_metricRecorder;
};

/**
 * Tests the createRegistrationManager method with various input combinations.
 */
TEST_F(RegistrationManagerTest, test_createRegistrationManagerInterface) {
    std::shared_ptr<RegistrationManagerInterface> manager;

    /// Null DataManager.
    manager = RegistrationManager::createRegistrationManagerInterface(
        nullptr, m_notifier, m_avsConnectionManager, m_directiveSequencer, m_metricRecorder);
    ASSERT_EQ(nullptr, manager);

    /// Null Notifier.
    manager = RegistrationManager::createRegistrationManagerInterface(
        m_dataManager, nullptr, m_avsConnectionManager, m_directiveSequencer, m_metricRecorder);
    ASSERT_EQ(nullptr, manager);

    /// Null ConnectionManager.
    manager = RegistrationManager::createRegistrationManagerInterface(
        m_dataManager, m_notifier, nullptr, m_directiveSequencer, m_metricRecorder);
    ASSERT_EQ(nullptr, manager);

    /// Null DirectiveSequencer.
    manager = RegistrationManager::createRegistrationManagerInterface(
        m_dataManager, m_notifier, m_avsConnectionManager, nullptr, m_metricRecorder);
    ASSERT_EQ(nullptr, manager);

    /// Null Metric Recorder
    manager = RegistrationManager::createRegistrationManagerInterface(
        m_dataManager, m_notifier, m_avsConnectionManager, m_directiveSequencer, m_metricRecorder);
    ASSERT_NE(nullptr, manager);
}

/**
 * Test that logout performs all the following actions:
 * - disable connection manager
 * - disable directive sequencer
 * - clear data handler's data
 * - notify registration observer
 */
TEST_F(RegistrationManagerTest, test_logout) {
    EXPECT_CALL(*m_directiveSequencer, disable());
    EXPECT_CALL(*m_avsConnectionManager, isEnabled());
    EXPECT_CALL(*m_avsConnectionManager, disable());
    EXPECT_CALL(*m_registrationObserver, onLogout());
    EXPECT_CALL(*m_dataHandler, clearData());
#ifdef ACSDK_ENABLE_METRICS_RECORDING
    EXPECT_CALL(*m_metricRecorder, recordMetric(_));
#endif

    m_registrationManager->logout();

    ASSERT_FALSE(m_avsConnectionManager->isEnabled());
}

}  // namespace test
}  // namespace registrationManager
}  // namespace alexaClientSDK
