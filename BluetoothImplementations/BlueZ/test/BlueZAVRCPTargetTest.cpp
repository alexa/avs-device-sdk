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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "BlueZ/BlueZAVRCPTarget.h"
#include "MockDBusProxy.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {
namespace test {

using namespace ::testing;

/**
 * Test fixture.
 */
class BlueZAVRCPTargetTest : public ::testing::Test {
public:
    /// SetUp before each test case.
    virtual void SetUp();

    /// A mock @c DBusProxy object to test with.
    std::shared_ptr<MockDBusProxy> m_mockProxy;

    /// The @c AVRCPTarget instance.
    std::shared_ptr<BlueZAVRCPTarget> m_avrcpTarget;
};

void BlueZAVRCPTargetTest::SetUp() {
    m_mockProxy = std::make_shared<MockDBusProxy>();
    m_avrcpTarget = BlueZAVRCPTarget::create(m_mockProxy);
}

/// Test create() with a null proxy. Should return nullptr.
TEST_F(BlueZAVRCPTargetTest, createNullFails) {
    ASSERT_THAT(BlueZAVRCPTarget::create(nullptr), IsNull());
}

/// Test create() and expect success.
TEST_F(BlueZAVRCPTargetTest, createSucceeds) {
    auto proxy = std::make_shared<MockDBusProxy>();
    ASSERT_THAT(BlueZAVRCPTarget::create(proxy), NotNull());
}

/// Test that play() calls the correct method.
TEST_F(BlueZAVRCPTargetTest, playSuceeds) {
    EXPECT_CALL(*m_mockProxy, callMethod("Play", _, _)).Times(1);
    m_avrcpTarget->play();
}

/// Test that pause() calls the correct method.
TEST_F(BlueZAVRCPTargetTest, pauseSuceeds) {
    EXPECT_CALL(*m_mockProxy, callMethod("Pause", _, _)).Times(1);
    m_avrcpTarget->pause();
}

/// Test that next() calls the correct method.
TEST_F(BlueZAVRCPTargetTest, nextSuceeds) {
    EXPECT_CALL(*m_mockProxy, callMethod("Next", _, _)).Times(1);
    m_avrcpTarget->next();
}

/// Test that previous() calls the correct method.
TEST_F(BlueZAVRCPTargetTest, previousSuceeds) {
    EXPECT_CALL(*m_mockProxy, callMethod("Previous", _, _)).Times(1);
    m_avrcpTarget->previous();
}

}  // namespace test
}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK
