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
#include <AVSCommon/Utils/Power/PowerResource.h>

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

// Component name.
static std::string PREFIXED_TEST_ID = std::string(PowerResource::PREFIX) + "test";

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

class PowerResourceTest : public ::testing::Test {
public:
    /// SetUp for the test.
    void SetUp() override;

    /// PowerResourceManagerInterface Mock.
    std::shared_ptr<NiceMock<MockPowerResourceManager>> m_powerManagerMock;

    /// PowerResource under test.
    std::shared_ptr<PowerResource> m_powerResource;
};

void PowerResourceTest::SetUp() {
    m_powerManagerMock = std::make_shared<NiceMock<MockPowerResourceManager>>();
    m_powerManagerMock->setDefaultBehavior();

    m_powerResource = PowerResource::create(TEST_ID, m_powerManagerMock, TEST_LEVEL, TEST_REFCOUNTED);
}

/// Test that passing in a nullptr for PowerResourceManagerInterface results in a nullptr.
TEST_F(PowerResourceTest, test_createWithNull) {
    auto noopPowerResource = PowerResource::create(TEST_ID, nullptr, TEST_LEVEL);
    EXPECT_THAT(noopPowerResource, IsNull());
}

/// Test that basic getters work as expected.
TEST_F(PowerResourceTest, test_getters) {
    EXPECT_EQ(m_powerResource->getId(), TEST_ID);
    EXPECT_EQ(m_powerResource->getLevel(), TEST_LEVEL);
    EXPECT_EQ(m_powerResource->isRefCounted(), TEST_REFCOUNTED);
}

/// Test acquire calls the underlying @c PowerResourceManagerInterface.
TEST_F(PowerResourceTest, test_acquire) {
    EXPECT_CALL(*m_powerManagerMock, acquire(HasPowerResourceId(PREFIXED_TEST_ID), _));
    m_powerResource->acquire();
}

/// Test multiple acquire calls reflect changes in refcount and the underlying @c PowerResourceManagerInterface being
/// called.
TEST_F(PowerResourceTest, test_multiAcquire) {
    EXPECT_CALL(*m_powerManagerMock, acquire(HasPowerResourceId(PREFIXED_TEST_ID), _)).Times(2);

    for (unsigned int i = 0; i < 2; i++) {
        m_powerResource->acquire();
    }
}

/// Test destructor releases.
TEST_F(PowerResourceTest, test_destructor_releases) {
    EXPECT_CALL(*m_powerManagerMock, release(HasPowerResourceId(PREFIXED_TEST_ID))).Times(2);
    m_powerResource->acquire();
    m_powerResource->acquire();
    m_powerResource.reset();
}

/// Test release calls the underlying @c PowerResourceManagerInterface.
TEST_F(PowerResourceTest, test_releaseNoAcquireSucceeds) {
    EXPECT_CALL(*m_powerManagerMock, release(HasPowerResourceId(PREFIXED_TEST_ID)));
    m_powerResource->release();
}

/// Test release calls the underlying @c PowerResourceManagerInterface.
TEST_F(PowerResourceTest, test_release) {
    {
        InSequence s;
        EXPECT_CALL(*m_powerManagerMock, acquire(HasPowerResourceId(PREFIXED_TEST_ID), _));
        EXPECT_CALL(*m_powerManagerMock, release(HasPowerResourceId(PREFIXED_TEST_ID)));
    }

    m_powerResource->acquire();
    m_powerResource->release();
}

/// Test multiple releases result in the underlying @c PowerResourceManagerInterface being called, and the appropriate
/// refcount.
TEST_F(PowerResourceTest, test_multiReleaseSymmetricalSucceeds) {
    {
        InSequence s;
        EXPECT_CALL(*m_powerManagerMock, acquire(HasPowerResourceId(PREFIXED_TEST_ID), _)).Times(AnyNumber());
        EXPECT_CALL(*m_powerManagerMock, release(HasPowerResourceId(PREFIXED_TEST_ID))).Times(AnyNumber());
    }

    m_powerResource->acquire();
    m_powerResource->acquire();

    m_powerResource->release();
    m_powerResource->release();
}

/// Test multiple releases result in the underlying @c PowerResourceManagerInterface being called, and the appropriate
/// refcount.
TEST_F(PowerResourceTest, test_multiReleaseAssymmetricalSucceeds) {
    {
        InSequence s;
        EXPECT_CALL(*m_powerManagerMock, acquire(HasPowerResourceId(PREFIXED_TEST_ID), _));
        EXPECT_CALL(*m_powerManagerMock, release(HasPowerResourceId(PREFIXED_TEST_ID))).Times(2);
    }

    m_powerResource->acquire();
    m_powerResource->release();
    m_powerResource->release();
}

/// Test freeze calls underlying @c PowerResourceManagerInterface.
TEST_F(PowerResourceTest, test_freezeMultiSucceeds) {
    {
        InSequence s;
        EXPECT_CALL(*m_powerManagerMock, acquire(HasPowerResourceId(PREFIXED_TEST_ID), _)).Times(AnyNumber());
        // Account for destructor release.
        EXPECT_CALL(*m_powerManagerMock, release(HasPowerResourceId(PREFIXED_TEST_ID))).Times(4);
    }

    m_powerResource->acquire();
    m_powerResource->acquire();
    m_powerResource->freeze();
    EXPECT_TRUE(m_powerResource->isFrozen());
    m_powerResource.reset();
}

/// Test acquiring a frozen resource fails.
TEST_F(PowerResourceTest, test_frozenAcquireFails) {
    {
        InSequence s;
        EXPECT_CALL(*m_powerManagerMock, acquire(HasPowerResourceId(PREFIXED_TEST_ID), _));
        // Account for destructor release.
        EXPECT_CALL(*m_powerManagerMock, release(HasPowerResourceId(PREFIXED_TEST_ID))).Times(2);
    }

    m_powerResource->acquire();
    m_powerResource->freeze();
    EXPECT_TRUE(m_powerResource->isFrozen());

    m_powerResource->acquire();
    EXPECT_TRUE(m_powerResource->isFrozen());
    m_powerResource.reset();
}

/// Test releasing a frozen resource fails.
TEST_F(PowerResourceTest, test_frozenReleaseFails) {
    auto strictPowerManager = std::make_shared<StrictMock<MockPowerResourceManager>>();
    strictPowerManager->setDefaultBehavior();

    EXPECT_CALL(*strictPowerManager, create(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(*strictPowerManager, close(_)).Times(AnyNumber());

    m_powerResource = PowerResource::create(TEST_ID, strictPowerManager, TEST_LEVEL, TEST_REFCOUNTED);
    EXPECT_CALL(*strictPowerManager, acquire(HasPowerResourceId(PREFIXED_TEST_ID), _));
    // freeze() releases once.
    // Account for destructor release.
    EXPECT_CALL(*strictPowerManager, release(HasPowerResourceId(PREFIXED_TEST_ID))).Times(2);

    m_powerResource->acquire();
    m_powerResource->freeze();
    EXPECT_TRUE(m_powerResource->isFrozen());

    m_powerResource->release();
    EXPECT_TRUE(m_powerResource->isFrozen());
    m_powerResource.reset();
}

/// Test multiple freeze calls keeps resource frozen and maintains the refcount.
TEST_F(PowerResourceTest, test_multipleFreezeIdempotent) {
    // Account for destructor release.
    EXPECT_CALL(*m_powerManagerMock, release(HasPowerResourceId(PREFIXED_TEST_ID))).Times(2);

    m_powerResource->acquire();
    m_powerResource->freeze();
    EXPECT_TRUE(m_powerResource->isFrozen());

    m_powerResource->freeze();
    EXPECT_TRUE(m_powerResource->isFrozen());
    m_powerResource.reset();
}

/// Test thaw acquires as many refCounts as available.
TEST_F(PowerResourceTest, test_thawMulti) {
    // Initial acquire + 2x thaw().
    EXPECT_CALL(*m_powerManagerMock, acquire(HasPowerResourceId(PREFIXED_TEST_ID), _)).Times(4);

    m_powerResource->acquire();
    m_powerResource->acquire();
    m_powerResource->freeze();
    m_powerResource->thaw();
}

/// Test acquiring a thawed resource succeeds.
TEST_F(PowerResourceTest, test_thawAcquireSucceeds) {
    EXPECT_CALL(*m_powerManagerMock, acquire(HasPowerResourceId(PREFIXED_TEST_ID), _));

    m_powerResource->freeze();
    m_powerResource->thaw();
    m_powerResource->acquire();

    EXPECT_FALSE(m_powerResource->isFrozen());
}

/// Test releasing a thawed resource succeeds.
TEST_F(PowerResourceTest, test_thawReleaseSucceeds) {
    {
        InSequence s;
        EXPECT_CALL(*m_powerManagerMock, acquire(HasPowerResourceId(PREFIXED_TEST_ID), _));
        EXPECT_CALL(*m_powerManagerMock, release(HasPowerResourceId(PREFIXED_TEST_ID)));
        EXPECT_CALL(*m_powerManagerMock, acquire(HasPowerResourceId(PREFIXED_TEST_ID), _));
        EXPECT_CALL(*m_powerManagerMock, release(HasPowerResourceId(PREFIXED_TEST_ID)));
    }

    m_powerResource->acquire();
    m_powerResource->freeze();
    m_powerResource->thaw();
    m_powerResource->release();

    EXPECT_FALSE(m_powerResource->isFrozen());
}

/// Test multiple thaws results.
TEST_F(PowerResourceTest, test_multipleThawIdempotent) {
    {
        InSequence s;
        EXPECT_CALL(*m_powerManagerMock, acquire(HasPowerResourceId(PREFIXED_TEST_ID), _));
        EXPECT_CALL(*m_powerManagerMock, release(HasPowerResourceId(PREFIXED_TEST_ID)));
        // Single Thaw
        EXPECT_CALL(*m_powerManagerMock, acquire(HasPowerResourceId(PREFIXED_TEST_ID), _));
        // Account for destructor.
        EXPECT_CALL(*m_powerManagerMock, release(HasPowerResourceId(PREFIXED_TEST_ID)));
    }

    m_powerResource->acquire();
    m_powerResource->freeze();

    m_powerResource->thaw();
    EXPECT_FALSE(m_powerResource->isFrozen());

    m_powerResource->thaw();
    EXPECT_FALSE(m_powerResource->isFrozen());
    m_powerResource.reset();
}

/// Test that acquire for a non-refCounted resource results in a refCount of 1.
TEST_F(PowerResourceTest, test_notRefCountedMultipleAcquireSuccceeds) {
    m_powerResource = PowerResource::create(TEST_ID, m_powerManagerMock, TEST_LEVEL, !TEST_REFCOUNTED);

    EXPECT_CALL(*m_powerManagerMock, acquire(HasPowerResourceId(PREFIXED_TEST_ID), _)).Times(2);

    m_powerResource->acquire();
    m_powerResource->acquire();
}

/// Test that in a none refCounted resource release works.
TEST_F(PowerResourceTest, test_notRefCountedReleaseSucceeds) {
    m_powerResource = PowerResource::create(TEST_ID, m_powerManagerMock, TEST_LEVEL, !TEST_REFCOUNTED);

    EXPECT_CALL(*m_powerManagerMock, release(HasPowerResourceId(PREFIXED_TEST_ID))).Times(2);

    m_powerResource->acquire();
    m_powerResource->acquire();
    m_powerResource->release();
    m_powerResource->release();
}

}  // namespace test
}  // namespace power
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
