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

/// @file StartupManagerTest.cpp

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "acsdkStartupManager/StartupManager.h"
#include "acsdkStartupManager/StartupNotifier.h"

namespace alexaClientSDK {
namespace acsdkStartupManager {
namespace test {

using namespace ::testing;
using namespace acsdkStartupManagerInterfaces;

class StartupManagerTest : public ::testing::Test {};

class MockRequiresStartup : public RequiresStartupInterface {
public:
    MOCK_METHOD0(startup, bool());
};

static auto returnTrue = []() { return true; };
static auto returnFalse = []() { return false; };

/**
 * Verify the simplest failure case - null StartupNotifier.
 */
TEST_F(StartupManagerTest, test_simplestFailureCase) {
    auto startupManager = StartupManager::createStartupManagerInterface(nullptr);
    ASSERT_FALSE(startupManager);
}

/**
 * Verify the simplest success case - no startup observers.
 */
TEST_F(StartupManagerTest, test_simplestSuccessCase) {
    auto startupNotifier = std::make_shared<StartupNotifier>();
    auto startupManager = StartupManager::createStartupManagerInterface(startupNotifier);

    ASSERT_TRUE(startupManager->startup());
}

/**
 * Verify the simple success case of notifying an observer of startup and the observer returning true.
 */
TEST_F(StartupManagerTest, test_simpleSuccessCase) {
    auto startupNotifier = std::make_shared<StartupNotifier>();
    auto startupManager = StartupManager::createStartupManagerInterface(startupNotifier);
    auto requiresStartup = std::make_shared<MockRequiresStartup>();

    EXPECT_CALL(*requiresStartup, startup()).WillOnce(Invoke(returnTrue));
    startupNotifier->addObserver(requiresStartup);
    ASSERT_TRUE(startupManager->startup());
}

/**
 * Verify the complex success case of notifying multiple observers of startup and the observers returning true.
 */
TEST_F(StartupManagerTest, test_complexSuccessCase) {
    auto startupNotifier = std::make_shared<StartupNotifier>();
    auto startupManager = StartupManager::createStartupManagerInterface(startupNotifier);
    auto requiresStartup0 = std::make_shared<MockRequiresStartup>();
    auto requiresStartup1 = std::make_shared<MockRequiresStartup>();
    auto requiresStartup2 = std::make_shared<MockRequiresStartup>();

    EXPECT_CALL(*requiresStartup0, startup()).WillOnce(Invoke(returnTrue));
    EXPECT_CALL(*requiresStartup1, startup()).WillOnce(Invoke(returnTrue));
    EXPECT_CALL(*requiresStartup2, startup()).WillOnce(Invoke(returnTrue));
    startupNotifier->addObserver(requiresStartup0);
    startupNotifier->addObserver(requiresStartup1);
    startupNotifier->addObserver(requiresStartup2);
    ASSERT_TRUE(startupManager->startup());
}

/**
 * Verify the simple failure case - notifying one observer of startup and the observer returning false.
 */
TEST_F(StartupManagerTest, test_simpleFailCase) {
    auto startupNotifier = std::make_shared<StartupNotifier>();
    auto startupManager = StartupManager::createStartupManagerInterface(startupNotifier);
    auto requiresStartup = std::make_shared<MockRequiresStartup>();

    EXPECT_CALL(*requiresStartup, startup()).WillOnce(Invoke(returnFalse));
    startupNotifier->addObserver(requiresStartup);
    ASSERT_FALSE(startupManager->startup());
}

/**
 * Verify the complex failure case of notifying multiple observers of startup and some of the observers returning false.
 */
TEST_F(StartupManagerTest, test_complexFailureCase) {
    auto startupNotifier = std::make_shared<StartupNotifier>();
    auto startupManager = StartupManager::createStartupManagerInterface(startupNotifier);
    auto requiresStartup0 = std::make_shared<MockRequiresStartup>();
    auto requiresStartup1 = std::make_shared<MockRequiresStartup>();
    auto requiresStartup2 = std::make_shared<MockRequiresStartup>();
    auto requiresStartup3 = std::make_shared<MockRequiresStartup>();
    auto requiresStartup4 = std::make_shared<MockRequiresStartup>();

    EXPECT_CALL(*requiresStartup0, startup()).WillRepeatedly(Invoke(returnTrue));
    EXPECT_CALL(*requiresStartup1, startup()).WillRepeatedly(Invoke(returnFalse));
    EXPECT_CALL(*requiresStartup2, startup()).WillRepeatedly(Invoke(returnTrue));
    EXPECT_CALL(*requiresStartup3, startup()).WillRepeatedly(Invoke(returnFalse));
    EXPECT_CALL(*requiresStartup4, startup()).WillRepeatedly(Invoke(returnTrue));
    startupNotifier->addObserver(requiresStartup0);
    startupNotifier->addObserver(requiresStartup1);
    startupNotifier->addObserver(requiresStartup2);
    startupNotifier->addObserver(requiresStartup3);
    startupNotifier->addObserver(requiresStartup4);
    ASSERT_FALSE(startupManager->startup());
}

}  // namespace test
}  // namespace acsdkStartupManager
}  // namespace alexaClientSDK
