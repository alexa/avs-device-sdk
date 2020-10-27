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

/// @file ShutdownManagerTest.cpp

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "acsdkShutdownManager/ShutdownManager.h"
#include "acsdkShutdownManager/ShutdownNotifier.h"

namespace alexaClientSDK {
namespace acsdkShutdownManager {
namespace test {

using namespace ::testing;
using namespace acsdkShutdownManagerInterfaces;

class ShutdownManagerTest : public ::testing::Test {};

class RequiresShutdownHelper : public avsCommon::utils::RequiresShutdown {
public:
    RequiresShutdownHelper();
    void doShutdown() override;
};

RequiresShutdownHelper::RequiresShutdownHelper() : RequiresShutdown("RequiredsShutdownHelper") {
}

void RequiresShutdownHelper::doShutdown() {
}

class MockRequiresShutdown : public RequiresShutdownHelper {
public:
    MOCK_METHOD0(doShutdown, void());
};

/**
 * Verify the simplest failure case - null ShutdownNotifier.
 */
TEST_F(ShutdownManagerTest, test_simplestFailureCase) {
    auto shutdownManager = ShutdownManager::createShutdownManagerInterface(nullptr);
    ASSERT_FALSE(shutdownManager);
}

/**
 * Verify the simplest success case - no registered instances of RequiresShutdown.
 */
TEST_F(ShutdownManagerTest, test_simplestSuccessCase) {
    auto shutdownNotifier = std::make_shared<ShutdownNotifier>();
    auto shutdownManager = ShutdownManager::createShutdownManagerInterface(shutdownNotifier);
    ASSERT_TRUE(shutdownManager->shutdown());
}

/**
 * Verify the simple success case - multiple RequiresShutdown instances added and called in
 * the reverse of the order that they were added.
 */
TEST_F(ShutdownManagerTest, test_simpleSuccessCase) {
    auto shutdownNotifier = std::make_shared<ShutdownNotifier>();
    auto shutdownManager = ShutdownManager::createShutdownManagerInterface(shutdownNotifier);
    auto requiresShutdown0 = std::make_shared<MockRequiresShutdown>();
    auto requiresShutdown1 = std::make_shared<MockRequiresShutdown>();
    auto requiresShutdown2 = std::make_shared<MockRequiresShutdown>();
    shutdownNotifier->addObserver(requiresShutdown0);
    shutdownNotifier->addObserver(requiresShutdown1);
    shutdownNotifier->addObserver(requiresShutdown2);
    InSequence sequence1;
    EXPECT_CALL(*requiresShutdown2, doShutdown());
    EXPECT_CALL(*requiresShutdown1, doShutdown());
    EXPECT_CALL(*requiresShutdown0, doShutdown());
    ASSERT_TRUE(shutdownManager->shutdown());
}

/**
 * Verify the failure case - RequiresStartup instance added after shutdown started.
 */
TEST_F(ShutdownManagerTest, test_simpleFailureCase) {
    auto shutdownNotifier = std::make_shared<ShutdownNotifier>();
    auto shutdownManager = ShutdownManager::createShutdownManagerInterface(shutdownNotifier);
    auto requiresShutdown0 = std::make_shared<MockRequiresShutdown>();
    auto requiresShutdown1 = std::make_shared<MockRequiresShutdown>();
    auto requiresShutdown2 = std::make_shared<MockRequiresShutdown>();
    auto requiresShutdown3 = std::make_shared<MockRequiresShutdown>();
    auto addRequiresShutdown = [&requiresShutdown3, &shutdownNotifier]() {
        shutdownNotifier->addObserver(requiresShutdown3);
    };
    shutdownNotifier->addObserver(requiresShutdown0);
    shutdownNotifier->addObserver(requiresShutdown1);
    shutdownNotifier->addObserver(requiresShutdown2);
    EXPECT_CALL(*requiresShutdown2, doShutdown());
    EXPECT_CALL(*requiresShutdown1, doShutdown()).WillOnce((Invoke(addRequiresShutdown)));
    EXPECT_CALL(*requiresShutdown0, doShutdown());
    ASSERT_FALSE(shutdownManager->shutdown());
}

}  // namespace test
}  // namespace acsdkShutdownManager
}  // namespace alexaClientSDK
