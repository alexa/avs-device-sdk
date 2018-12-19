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

#include <Settings/DeviceSettingsManager.h>
#include <Settings/SettingsManager.h>
#include <Settings/SettingObserverInterface.h>

namespace alexaClientSDK {
namespace settings {
namespace test {

enum TestSettingId { TEST_ID_INT = 0, TEST_ID_STRING };

/// @defgroup Define initial, new and default values to be used in the test.
/// @{
constexpr int INITIAL_INT_VALUE = 20;
constexpr int NEW_INT_VALUE = -20;
constexpr int DEFAULT_INT_VALUE = 0;
/// @}

/// Settings stub that just set the value immediately.
template <typename ValueT>
class SettingStub : public SettingInterface<ValueT> {
public:
    SetSettingResult setLocalChange(const ValueT& value) override {
        this->m_value = value;
        return SetSettingResult::ENQUEUED;
    }

    SettingStub(const ValueT& value) : SettingInterface<ValueT>{value} {
    }
};

/// Just an empty observer.
template <typename SettingT>
class TestObserver : public SettingT::ObserverType {
public:
    void onSettingNotification(const typename SettingT::ValueType& value, SettingNotifications notification) override {
    }
};

/// @defgroup Define settings types used in the tests.
/// @{
using SettingInt = SettingStub<int>;
using SettingString = SettingStub<std::string>;
/// @}

/// Test class.
class SettingsManagerTest : public testing::Test {
protected:
    /// Settings Manager.
    SettingsManager<SettingInt, SettingString> m_manager;
};

/// Test add settings and setting the setting value.
TEST_F(SettingsManagerTest, testSetExistingSetting) {
    auto setting = std::make_shared<SettingInt>(INITIAL_INT_VALUE);
    auto expectedResult = std::pair<bool, int>{true, NEW_INT_VALUE};

    EXPECT_TRUE((m_manager.addSetting<TEST_ID_INT>(setting)));
    EXPECT_EQ((m_manager.setValue<TEST_ID_INT>(NEW_INT_VALUE)), SetSettingResult::ENQUEUED);
    EXPECT_EQ((m_manager.getValue<TEST_ID_INT>(DEFAULT_INT_VALUE)), expectedResult);
}

/// Test set value for setting that hasn't been registered.
TEST_F(SettingsManagerTest, testSetSettingUnavailable) {
    SettingsManager<SettingInt, SettingString> manager;
    EXPECT_EQ((m_manager.setValue<TEST_ID_INT>(NEW_INT_VALUE)), SetSettingResult::UNAVAILABLE_SETTING);
}

/// Test get value for setting that hasn't been registered.
TEST_F(SettingsManagerTest, testGetExistingSetting) {
    auto setting = std::make_shared<SettingInt>(INITIAL_INT_VALUE);
    auto expectedResult = std::pair<bool, int>{true, INITIAL_INT_VALUE};

    EXPECT_TRUE((m_manager.addSetting<TEST_ID_INT>(setting)));
    EXPECT_EQ((m_manager.getValue<TEST_ID_INT>(DEFAULT_INT_VALUE)), expectedResult);
}

/// Test get value for setting that hasn't been registered.
TEST_F(SettingsManagerTest, testGetSettingUnavailable) {
    auto expectedResult = std::pair<bool, int>{false, DEFAULT_INT_VALUE};
    EXPECT_EQ((m_manager.getValue<TEST_ID_INT>(DEFAULT_INT_VALUE)), expectedResult);
}

/// Test registering a setting that already exists.
TEST_F(SettingsManagerTest, testAddExistingSetting) {
    auto setting1 = std::make_shared<SettingInt>(INITIAL_INT_VALUE);
    auto setting2 = std::make_shared<SettingInt>(INITIAL_INT_VALUE);

    EXPECT_TRUE((m_manager.addSetting<TEST_ID_INT>(setting1)));
    EXPECT_FALSE((m_manager.addSetting<TEST_ID_INT>(setting2)));
}

/// Test addObserver for a setting that exists.
TEST_F(SettingsManagerTest, testAddObserver) {
    auto setting = std::make_shared<SettingInt>(INITIAL_INT_VALUE);
    auto observer = std::make_shared<TestObserver<SettingInt>>();

    EXPECT_TRUE((m_manager.addSetting<TEST_ID_INT>(setting)));
    EXPECT_TRUE((m_manager.addObserver<TEST_ID_INT>(observer)));
}

/// Test addObserver for a setting that doesn't exist.
TEST_F(SettingsManagerTest, testAddObserverFailed) {
    auto observer = std::make_shared<TestObserver<SettingInt>>();

    EXPECT_FALSE((m_manager.addObserver<TEST_ID_INT>(observer)));
}

/// Test addObserver for a setting that exists.
TEST_F(SettingsManagerTest, testRemoveObserver) {
    auto setting = std::make_shared<SettingInt>(INITIAL_INT_VALUE);
    auto observer = std::make_shared<TestObserver<SettingInt>>();

    EXPECT_TRUE((m_manager.addSetting<TEST_ID_INT>(setting)));
    EXPECT_TRUE((m_manager.addObserver<TEST_ID_INT>(observer)));
    m_manager.removeObserver<TEST_ID_INT>(observer);
}

/// Test addObserver for a setting that doesn't exist.
TEST_F(SettingsManagerTest, testRemoveObserverFailed) {
    auto observer = std::make_shared<TestObserver<SettingInt>>();
    m_manager.removeObserver<TEST_ID_INT>(observer);
}

/// Test manager operations for string setting.
TEST_F(SettingsManagerTest, testSetExistingStringSetting) {
    std::string initialValue = "";
    auto setting = std::make_shared<SettingString>(initialValue);

    EXPECT_TRUE((m_manager.addSetting<TEST_ID_STRING>(setting)));
    EXPECT_EQ((m_manager.setValue<TEST_ID_STRING>("test")), SetSettingResult::ENQUEUED);
}

}  // namespace test
}  // namespace settings
}  // namespace alexaClientSDK
