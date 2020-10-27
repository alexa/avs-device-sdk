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

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/SDKInterfaces/MockPowerResourceManager.h>
#include <AVSCommon/Utils/Power/WakeGuard.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace power {
namespace test {

using namespace ::testing;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;

// Component name.
static std::string TEST_ID = "test";

MATCHER_P(
    ContainsPowerResourceId,
    /* std::string */ id,
    "") {
    std::shared_ptr<PowerResourceManagerInterface::PowerResourceId> powerResourceId = arg;
    if (!powerResourceId) {
        return false;
    }

    return std::string::npos != powerResourceId->getResourceId().find(id);
}

/// Test that the constructor with a nullptr doesn't segfault.
TEST(WakeGuardTest, test_constructWithNull) {
    WakeGuard wake(nullptr);
}

TEST(WakeGuardTest, test_createAcquires) {
    auto powerManagerMock = std::make_shared<NiceMock<MockPowerResourceManager>>();
    powerManagerMock->setDefaultBehavior();
    auto powerResource = PowerResource::create(TEST_ID, powerManagerMock);

    EXPECT_CALL(*powerManagerMock, acquire(ContainsPowerResourceId(TEST_ID), _));

    auto wake = WakeGuard(powerResource);
}

TEST(WakeGuardTest, test_destructorReleases) {
    auto powerManagerMock = std::make_shared<NiceMock<MockPowerResourceManager>>();
    powerManagerMock->setDefaultBehavior();
    auto powerResource = PowerResource::create(TEST_ID, powerManagerMock);

    EXPECT_CALL(*powerManagerMock, release(ContainsPowerResourceId(TEST_ID)));

    { auto wake = WakeGuard(powerResource); }
}

}  // namespace test
}  // namespace power
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
