
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/SDKInterfaces/MockPowerResourceManager.h>
#include <AVSCommon/Utils/Power/AggregatedPowerResourceManager.h>

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

// Component level.
static PowerResourceManagerInterface::PowerResourceLevel TEST_LEVEL =
    PowerResourceManagerInterface::PowerResourceLevel::STANDBY_LOW;

// Component refCount configuration.
static bool TEST_REFCOUNTED = true;

MATCHER_P(
    HasPowerResourceId,
    /* std::string */ id,
    "") {
    std::shared_ptr<PowerResourceManagerInterface::PowerResourceId> powerResourceId = arg;
    if (!powerResourceId) {
        return false;
    }

    return id == powerResourceId->getResourceId();
}

class AggregatedPowerResourceManagerTest : public ::testing::Test {
public:
    // SetUp.
    void SetUp() override;

    /// PowerResourceManagerInterface Mock.
    std::shared_ptr<NiceMock<MockPowerResourceManager>> m_mockAppPowerManager;

    /// PowerResource under test.
    std::shared_ptr<AggregatedPowerResourceManager> m_powerManager;
};

void AggregatedPowerResourceManagerTest::SetUp() {
    m_mockAppPowerManager = std::make_shared<NiceMock<MockPowerResourceManager>>();
    m_mockAppPowerManager->setDefaultBehavior();

    m_powerManager = AggregatedPowerResourceManager::create(m_mockAppPowerManager);
}

/**
 * Test creating an instance with a nullptr fails.
 */
TEST_F(AggregatedPowerResourceManagerTest, test_factoryMethodWithNullArgument) {
    // Some versions of gcc cannot resolve the overload if calling AggregatedPowerResourceManager::create(nullptr)
    std::shared_ptr<PowerResourceManagerInterface> nullPowerManager = nullptr;
    EXPECT_THAT(AggregatedPowerResourceManager::create(nullPowerManager), IsNull());
}

/**
 * Test creating multiple resources with the same ID fails.
 */
TEST_F(AggregatedPowerResourceManagerTest, test_createMultipleSameId) {
    EXPECT_THAT(m_powerManager->create(TEST_ID, TEST_REFCOUNTED, TEST_LEVEL), NotNull());
    EXPECT_THAT(m_powerManager->create(TEST_ID, TEST_REFCOUNTED, TEST_LEVEL), IsNull());
}

/**
 * Test creating multiple resources with the same level
 * results in one resource from the app PowerResourceManagerInterface perspective.
 */
TEST_F(AggregatedPowerResourceManagerTest, test_createMultipleSameLevel) {
    EXPECT_CALL(*m_mockAppPowerManager, create(_, _, TEST_LEVEL));

    m_powerManager->create(TEST_ID, TEST_REFCOUNTED, TEST_LEVEL);
    m_powerManager->create(TEST_ID + "2", TEST_REFCOUNTED, TEST_LEVEL);
    m_powerManager->create(TEST_ID + "3", TEST_REFCOUNTED, TEST_LEVEL);
}

/**
 * Test creating multiple resources with different level
 * results in one resource from the app PowerResourceManagerInterface perspective.
 */
TEST_F(AggregatedPowerResourceManagerTest, test_createMultipleDifferentLevels) {
    EXPECT_CALL(*m_mockAppPowerManager, create(_, _, TEST_LEVEL));
    EXPECT_CALL(*m_mockAppPowerManager, create(_, _, PowerResourceManagerInterface::PowerResourceLevel::ACTIVE_HIGH));
    EXPECT_CALL(*m_mockAppPowerManager, create(_, _, PowerResourceManagerInterface::PowerResourceLevel::ACTIVE_LOW));

    m_powerManager->create(TEST_ID, TEST_REFCOUNTED, TEST_LEVEL);
    m_powerManager->create(
        TEST_ID + "2", TEST_REFCOUNTED, PowerResourceManagerInterface::PowerResourceLevel::ACTIVE_HIGH);
    m_powerManager->create(
        TEST_ID + "3", TEST_REFCOUNTED, PowerResourceManagerInterface::PowerResourceLevel::ACTIVE_LOW);
}

/**
 * Test acquiring with multiple resources with the same level results in calls
 * to the aggregated resource.
 */
TEST_F(AggregatedPowerResourceManagerTest, test_acquireMultipleSameLevel) {
    EXPECT_CALL(*m_mockAppPowerManager, acquire(_, _)).Times(3);

    auto resource1 = m_powerManager->create(TEST_ID, TEST_REFCOUNTED, TEST_LEVEL);
    auto resource2 = m_powerManager->create(TEST_ID + "2", TEST_REFCOUNTED, TEST_LEVEL);
    auto resource3 = m_powerManager->create(TEST_ID + "3", TEST_REFCOUNTED, TEST_LEVEL);

    m_powerManager->acquire(resource1);
    m_powerManager->acquire(resource2);
    m_powerManager->acquire(resource3);
}

/**
 * Test acquiring with multiple resources with different levels results in calls
 * to the aggregated resource.
 */
TEST_F(AggregatedPowerResourceManagerTest, test_acquireMultipleDifferentLevels) {
    EXPECT_CALL(*m_mockAppPowerManager, acquire(_, _)).Times(3);

    auto resource1 = m_powerManager->create(TEST_ID, TEST_REFCOUNTED, TEST_LEVEL);
    auto resource2 = m_powerManager->create(
        TEST_ID + "2", TEST_REFCOUNTED, PowerResourceManagerInterface::PowerResourceLevel::ACTIVE_HIGH);
    auto resource3 = m_powerManager->create(
        TEST_ID + "3", TEST_REFCOUNTED, PowerResourceManagerInterface::PowerResourceLevel::ACTIVE_LOW);

    m_powerManager->acquire(resource1);
    m_powerManager->acquire(resource2);
    m_powerManager->acquire(resource3);
}

/**
 * Test acquiring with a nullptr.
 */
TEST_F(AggregatedPowerResourceManagerTest, test_acquireNull) {
    m_powerManager.reset();

    auto strictPowerManager = std::make_shared<StrictMock<MockPowerResourceManager>>();
    m_powerManager = AggregatedPowerResourceManager::create(strictPowerManager);

    m_powerManager->acquire(nullptr);
}

/**
 * Test acquiring multiple refcounted resources results in multiple acquire.
 */
TEST_F(AggregatedPowerResourceManagerTest, test_acquireRefCounted) {
    EXPECT_CALL(*m_mockAppPowerManager, acquire(_, _)).Times(3);

    auto resource1 = m_powerManager->create(TEST_ID, TEST_REFCOUNTED, TEST_LEVEL);

    m_powerManager->acquire(resource1);
    m_powerManager->acquire(resource1);
    m_powerManager->acquire(resource1);
}

/**
 * Test acquiring multiple non-ref counted resources results in just one acquire.
 */
TEST_F(AggregatedPowerResourceManagerTest, test_acquireNonRefCounted) {
    EXPECT_CALL(*m_mockAppPowerManager, acquire(_, _));

    auto resource1 = m_powerManager->create(TEST_ID, false, TEST_LEVEL);

    m_powerManager->acquire(resource1);
    m_powerManager->acquire(resource1);
    m_powerManager->acquire(resource1);
}

/**
 * Test release with multiple resources with the same level results in calls
 * to the aggregated resource.
 */
TEST_F(AggregatedPowerResourceManagerTest, test_releaseMultipleSameLevel) {
    EXPECT_CALL(*m_mockAppPowerManager, release(_)).Times(3);

    auto resource1 = m_powerManager->create(TEST_ID, TEST_REFCOUNTED, TEST_LEVEL);
    auto resource2 = m_powerManager->create(TEST_ID + "2", TEST_REFCOUNTED, TEST_LEVEL);
    auto resource3 = m_powerManager->create(TEST_ID + "3", TEST_REFCOUNTED, TEST_LEVEL);

    m_powerManager->acquire(resource1);
    m_powerManager->acquire(resource2);
    m_powerManager->acquire(resource3);

    m_powerManager->release(resource1);
    m_powerManager->release(resource2);
    m_powerManager->release(resource3);
}

/**
 * Test release with multiple resources with different levels results in calls
 * to the aggregated resource.
 */
TEST_F(AggregatedPowerResourceManagerTest, test_releaseMultipleDifferentLevels) {
    EXPECT_CALL(*m_mockAppPowerManager, release(_)).Times(3);

    auto resource1 = m_powerManager->create(TEST_ID, TEST_REFCOUNTED, TEST_LEVEL);
    auto resource2 = m_powerManager->create(
        TEST_ID + "2", TEST_REFCOUNTED, PowerResourceManagerInterface::PowerResourceLevel::ACTIVE_HIGH);
    auto resource3 = m_powerManager->create(
        TEST_ID + "3", TEST_REFCOUNTED, PowerResourceManagerInterface::PowerResourceLevel::ACTIVE_LOW);

    m_powerManager->acquire(resource1);
    m_powerManager->acquire(resource2);
    m_powerManager->acquire(resource3);

    m_powerManager->release(resource1);
    m_powerManager->release(resource2);
    m_powerManager->release(resource3);
}

/**
 * Test release with a nullptr.
 */
TEST_F(AggregatedPowerResourceManagerTest, test_releaseNull) {
    m_powerManager.reset();

    auto strictPowerManager = std::make_shared<StrictMock<MockPowerResourceManager>>();
    m_powerManager = AggregatedPowerResourceManager::create(strictPowerManager);

    m_powerManager->release(nullptr);
}

/**
 * Test releasing multiple refcounted resources results in multiple release.
 */
TEST_F(AggregatedPowerResourceManagerTest, test_releaseRefCounted) {
    EXPECT_CALL(*m_mockAppPowerManager, release(_)).Times(3);

    auto resource1 = m_powerManager->create(TEST_ID, TEST_REFCOUNTED, TEST_LEVEL);

    m_powerManager->acquire(resource1);
    m_powerManager->acquire(resource1);
    m_powerManager->acquire(resource1);

    m_powerManager->release(resource1);
    m_powerManager->release(resource1);
    m_powerManager->release(resource1);
}

/**
 * Test releasing multiple non-ref counted resources results in just one acquire.
 */
TEST_F(AggregatedPowerResourceManagerTest, test_releaseNonRefCounted) {
    EXPECT_CALL(*m_mockAppPowerManager, acquire(_, _));

    auto resource1 = m_powerManager->create(TEST_ID, false, TEST_LEVEL);

    m_powerManager->acquire(resource1);
    m_powerManager->acquire(resource1);
    m_powerManager->acquire(resource1);

    m_powerManager->release(resource1);
    m_powerManager->release(resource1);
    m_powerManager->release(resource1);
}

/**
 * Test closing a power resource results in releasing remaining refCount.
 */
TEST_F(AggregatedPowerResourceManagerTest, test_close) {
    EXPECT_CALL(*m_mockAppPowerManager, release(_)).Times(2);
    auto resource1 = m_powerManager->create(TEST_ID, TEST_REFCOUNTED, TEST_LEVEL);

    m_powerManager->acquire(resource1);
    m_powerManager->acquire(resource1);
    m_powerManager->close(resource1);
}

/**
 * Test calling close will cleanup the aggregated resource if there are no more resources of that level.
 */
TEST_F(AggregatedPowerResourceManagerTest, test_closeCleanup) {
    EXPECT_CALL(*m_mockAppPowerManager, close(_));
    auto resource1 = m_powerManager->create(TEST_ID, TEST_REFCOUNTED, TEST_LEVEL);
    m_powerManager->close(resource1);
}

/**
 * Test ensuring that acquirePowerResource calls the underlying application PowerResourceManagerInterface methods.
 */
TEST_F(AggregatedPowerResourceManagerTest, test_acquirePowerResourceLegacy) {
    EXPECT_CALL(*m_mockAppPowerManager, acquirePowerResource(TEST_ID, TEST_LEVEL)).Times(0);
    m_powerManager->acquirePowerResource(TEST_ID, TEST_LEVEL);
}

/**
 * Test ensuring that releasePowerResource calls the underlying application PowerResourceManagerInterface methods.
 */
TEST_F(AggregatedPowerResourceManagerTest, test_releasePowerResourceLegacy) {
    EXPECT_CALL(*m_mockAppPowerManager, releasePowerResource(TEST_ID)).Times(0);
    m_powerManager->releasePowerResource(TEST_ID);
}

/**
 * Test ensuring that isPowerResourceAcquired calls the underlying application PowerResourceManagerInterface methods.
 */
TEST_F(AggregatedPowerResourceManagerTest, test_isPowerResourceAcquiredLegacy) {
    EXPECT_CALL(*m_mockAppPowerManager, isPowerResourceAcquired(TEST_ID)).Times(0);
    m_powerManager->isPowerResourceAcquired(TEST_ID);
}

}  // namespace test
}  // namespace power
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
