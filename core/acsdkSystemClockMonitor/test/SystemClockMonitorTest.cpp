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

/// @file SystemClockMonitorTest.cpp

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace alexaClientSDK {
namespace acsdkSystemClockMonitor {
namespace test {

using namespace ::testing;
using namespace acsdkSystemClockMonitorInterfaces;

class MockSystemClockObserver : public SystemClockMonitorObserverInterface {
public:
    MOCK_METHOD0(onSystemClockSynchronized, void());
};

class SystemClockMonitorTest : public ::testing::Test {
public:
    void SetUp() override;
    void TearDown() override;

    /// The @c SystemClockNotifierInterface to relay notifications.
    std::shared_ptr<SystemClockNotifierInterface> m_notifier;

    /// The mock @c SystemClockMonitorObserverInterface to relay notifications.
    std::shared_ptr<MockSystemClockObserver> m_mockObserver;
};

void SystemClockMonitorTest::SetUp() {
    m_notifier = std::make_shared<SystemClockNotifier>();
    m_mockObserver = std::make_shared<NiceMock<MockSystemClockObserver>>();

    m_notifier->addObserver(m_mockObserver);
}

void SystemClockMonitorTest::TearDown() {
    m_mockObserver.reset();
    m_notifier.reset();
}

/**
 * Verify the simplest failure case - null SystemClockNotifier.
 */
TEST(SystemClockMonitorTest, test_createWithNullSystemClockMonitor) {
    auto manager = SystemClockMonitorManager::createSystemClockMonitorInterface(nullptr);
    ASSERT_FALSE(manager);
}

/**
 * Verify the simplest success case - non-null SystemClockNotifier.
 */
TEST(SystemClockMonitorTest, test_create) {
    auto manager = SystemClockMonitorManager::createSystemClockMonitorInterface(m_notifier);
    ASSERT_TRUE(manager);
}

/**
 * Verify the monitor notifies SystemClockNotifier when onSystemClockSynchronized() is called.
 */
TEST(SystemClockMonitorTest, test_getProvidersBeforeStartup) {
    auto manager = SystemClockMonitorManager::createSystemClockMonitorInterface(m_notifier);
    ASSERT_TRUE(manager);

    EXPECT_CALL(*m_mockObserver, onSystemClockSynchronized());
    manager->onSystemClockSynchronized();
}

}  // namespace test
}  // namespace acsdkSystemClockMonitor
}  // namespace alexaClientSDK
