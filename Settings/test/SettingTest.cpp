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

#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <Settings/Setting.h>

#include "Settings/MockSettingProtocol.h"
#include "Settings/MockSettingObserver.h"

namespace alexaClientSDK {
namespace settings {
namespace test {

using namespace testing;

/// Initial value for the setting.
static constexpr bool INIT_VALUE = false;

/// String representation of the initial value.
static const std::string INIT_VALUE_STR = "false";

/// New value for the setting.
static constexpr bool NEW_VALUE = true;

/// String representation of the new value.
static const std::string NEW_VALUE_STR = "true";

/**
 * Setting test class.
 */
class SettingTest : public Test {
protected:
    /**
     * Create protocol object.
     */
    void SetUp() override;

    /**
     * Create setting object.
     *
     * @return The setting object.
     * @note This method will call std::move(m_protocol).
     */
    std::shared_ptr<Setting<bool>> createSetting();

    /// Pointer to a mock protocol.
    std::unique_ptr<MockSettingProtocol> m_protocol;
};

void SettingTest::SetUp() {
    m_protocol = std::unique_ptr<MockSettingProtocol>(new MockSettingProtocol(INIT_VALUE_STR, true, false));
}

std::shared_ptr<Setting<bool>> SettingTest::createSetting() {
    auto setting = Setting<bool>::create(INIT_VALUE, std::move(m_protocol));
    m_protocol = nullptr;
    return setting;
}

/// Test successful create.
TEST_F(SettingTest, testCreate) {
    auto setting = createSetting();
    ASSERT_NE(setting, nullptr);
    EXPECT_EQ(setting->get(), INIT_VALUE);
}

/// Test create initial value when restore value is not available.
TEST_F(SettingTest, testCreateNoRestore) {
    m_protocol = std::unique_ptr<MockSettingProtocol>(new MockSettingProtocol(NEW_VALUE_STR, false, false));
    auto setting = createSetting();
    ASSERT_NE(setting, nullptr);
    EXPECT_EQ(setting->get(), INIT_VALUE);
}

/// Test create with null protocol.
TEST_F(SettingTest, testNullCreate) {
    EXPECT_EQ(Setting<bool>::create(INIT_VALUE, nullptr), nullptr);
}

/// Test avs change.
TEST_F(SettingTest, testAvsChange) {
    auto setting = createSetting();
    ASSERT_NE(setting, nullptr);

    setting->setAvsChange(NEW_VALUE);
    EXPECT_EQ(setting->get(), NEW_VALUE);
}

/// Test avs change revert.
TEST_F(SettingTest, testAvsChangeRevert) {
    m_protocol = std::unique_ptr<MockSettingProtocol>(new MockSettingProtocol(INIT_VALUE_STR, true, true));
    auto setting = createSetting();
    ASSERT_NE(setting, nullptr);

    setting->setAvsChange(NEW_VALUE);
    EXPECT_EQ(setting->get(), INIT_VALUE);
}

/// Test local change.
TEST_F(SettingTest, testLocalChange) {
    auto setting = createSetting();
    ASSERT_NE(setting, nullptr);

    setting->setLocalChange(NEW_VALUE);
    EXPECT_EQ(setting->get(), NEW_VALUE);
}

/// Test local change revert.
TEST_F(SettingTest, testLocalChangeRevert) {
    m_protocol = std::unique_ptr<MockSettingProtocol>(new MockSettingProtocol(INIT_VALUE_STR, true, true));
    auto setting = createSetting();
    ASSERT_NE(setting, nullptr);

    setting->setLocalChange(NEW_VALUE);
    EXPECT_EQ(setting->get(), INIT_VALUE);
}

/// Test observer notification.
TEST_F(SettingTest, testObserverNotificationLocal) {
    auto setting = createSetting();
    ASSERT_NE(setting, nullptr);

    auto observer = std::make_shared<MockSettingObserver<SettingInterface<bool>>>();
    EXPECT_CALL(*observer, onSettingNotification(_, SettingNotifications::LOCAL_CHANGE)).Times(1);

    setting->addObserver(observer);
    setting->setLocalChange(NEW_VALUE);
}

/// Test observer notification.
TEST_F(SettingTest, testObserverNotificationAvs) {
    auto setting = createSetting();
    ASSERT_NE(setting, nullptr);

    auto observer = std::make_shared<MockSettingObserver<SettingInterface<bool>>>();
    EXPECT_CALL(*observer, onSettingNotification(_, SettingNotifications::AVS_CHANGE)).Times(1);

    setting->addObserver(observer);
    setting->setAvsChange(NEW_VALUE);
}

}  // namespace test
}  // namespace settings
}  // namespace alexaClientSDK
