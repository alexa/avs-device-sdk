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
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/Utils/WaitEvent.h>
#include <RegistrationManager/CustomerDataManager.h>

#include "Settings/DeviceSettingsManager.h"
#include "Settings/MockSetting.h"
#include "Settings/SettingsManager.h"
#include "Settings/SettingObserverInterface.h"

namespace alexaClientSDK {
namespace settings {
namespace test {

enum TestSettingId { TEST_ID_INT = 0, TEST_ID_STRING, TEST_ID_CHAR };

/// @defgroup Define initial, new and default values to be used in the test.
/// @{
constexpr int INITIAL_INT_VALUE = 20;
constexpr int NEW_INT_VALUE = -20;
constexpr int DEFAULT_INT_VALUE = 0;
constexpr char DEFAULT_CHAR_VALUE = 'a';
/// @}

/// General timeout for tests to fail.
static const std::chrono::seconds TEST_TIMEOUT(10);

/// Settings stub that just set the value immediately.
template <typename ValueT>
class SettingStub : public SettingInterface<ValueT> {
public:
    SetSettingResult setLocalChange(const ValueT& value) override {
        this->m_value = value;
        return SetSettingResult::ENQUEUED;
    }

    bool setAvsChange(const ValueT& value) override {
        return false;
    }

    bool clearData(const ValueT& value) override {
        return true;
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
using SettingBool = MockSetting<bool>;
/// @}

/// Test class.
class SettingsManagerTest : public testing::Test {
public:
    void SetUp() override;

protected:
    /// Settings Manager.
    std::shared_ptr<SettingsManager<SettingInt, SettingString, SettingBool>> m_manager;
};

void SettingsManagerTest::SetUp() {
    auto customerDataManager = std::make_shared<registrationManager::CustomerDataManager>();
    m_manager = std::make_shared<SettingsManager<SettingInt, SettingString, SettingBool>>(customerDataManager);
}

/// Test add settings and setting the setting value.
TEST_F(SettingsManagerTest, test_setExistingSetting) {
    auto setting = std::make_shared<SettingInt>(INITIAL_INT_VALUE);
    auto expectedResult = std::pair<bool, int>{true, NEW_INT_VALUE};

    EXPECT_TRUE((m_manager->addSetting<TEST_ID_INT>(setting)));
    EXPECT_EQ((m_manager->setValue<TEST_ID_INT>(NEW_INT_VALUE)), SetSettingResult::ENQUEUED);
    EXPECT_EQ((m_manager->getValue<TEST_ID_INT>(DEFAULT_INT_VALUE)), expectedResult);
}

/// Test set value for setting that hasn't been registered.
TEST_F(SettingsManagerTest, test_setSettingUnavailable) {
    auto customerDataManager = std::make_shared<registrationManager::CustomerDataManager>();
    SettingsManager<SettingInt, SettingString> manager{customerDataManager};
    EXPECT_EQ((m_manager->setValue<TEST_ID_INT>(NEW_INT_VALUE)), SetSettingResult::UNAVAILABLE_SETTING);
}

/// Test get value for setting that hasn't been registered.
TEST_F(SettingsManagerTest, test_getExistingSetting) {
    auto setting = std::make_shared<SettingInt>(INITIAL_INT_VALUE);
    auto expectedResult = std::pair<bool, int>{true, INITIAL_INT_VALUE};

    EXPECT_TRUE((m_manager->addSetting<TEST_ID_INT>(setting)));
    EXPECT_EQ((m_manager->getValue<TEST_ID_INT>(DEFAULT_INT_VALUE)), expectedResult);
}

/// Test get value for setting that hasn't been registered.
TEST_F(SettingsManagerTest, test_getSettingUnavailable) {
    auto expectedResult = std::pair<bool, int>{false, DEFAULT_INT_VALUE};
    EXPECT_EQ((m_manager->getValue<TEST_ID_INT>(DEFAULT_INT_VALUE)), expectedResult);
}

/// Test registering a setting that already exists.
TEST_F(SettingsManagerTest, test_addExistingSetting) {
    auto setting1 = std::make_shared<SettingInt>(INITIAL_INT_VALUE);
    auto setting2 = std::make_shared<SettingInt>(INITIAL_INT_VALUE);

    EXPECT_TRUE((m_manager->addSetting<TEST_ID_INT>(setting1)));
    EXPECT_FALSE((m_manager->addSetting<TEST_ID_INT>(setting2)));
}

/// Test addObserver for a setting that exists.
TEST_F(SettingsManagerTest, test_addObserver) {
    auto setting = std::make_shared<SettingInt>(INITIAL_INT_VALUE);
    auto observer = std::make_shared<TestObserver<SettingInt>>();

    EXPECT_TRUE((m_manager->addSetting<TEST_ID_INT>(setting)));
    EXPECT_TRUE((m_manager->addObserver<TEST_ID_INT>(observer)));
}

/// Test addObserver for a setting that doesn't exist.
TEST_F(SettingsManagerTest, test_addObserverFailed) {
    auto observer = std::make_shared<TestObserver<SettingInt>>();

    EXPECT_FALSE((m_manager->addObserver<TEST_ID_INT>(observer)));
}

/// Test addObserver for a setting that exists.
TEST_F(SettingsManagerTest, test_removeObserver) {
    auto setting = std::make_shared<SettingInt>(INITIAL_INT_VALUE);
    auto observer = std::make_shared<TestObserver<SettingInt>>();

    EXPECT_TRUE((m_manager->addSetting<TEST_ID_INT>(setting)));
    EXPECT_TRUE((m_manager->addObserver<TEST_ID_INT>(observer)));
    m_manager->removeObserver<TEST_ID_INT>(observer);
}

/// Test addObserver for a setting that doesn't exist.
TEST_F(SettingsManagerTest, test_removeObserverFailed) {
    auto observer = std::make_shared<TestObserver<SettingInt>>();
    m_manager->removeObserver<TEST_ID_INT>(observer);
}

/// Test manager operations for string setting.
TEST_F(SettingsManagerTest, test_setExistingStringSetting) {
    std::string initialValue = "";
    auto setting = std::make_shared<SettingString>(initialValue);

    EXPECT_TRUE((m_manager->addSetting<TEST_ID_STRING>(setting)));
    EXPECT_EQ((m_manager->setValue<TEST_ID_STRING>("test")), SetSettingResult::ENQUEUED);
}

/// Test manager getting a clearData callback will call clearData on the setting.
TEST_F(SettingsManagerTest, test_clearDataInSettingManagerCallsClearDataInSetting) {
    auto setting = std::make_shared<SettingBool>(DEFAULT_CHAR_VALUE);
    EXPECT_TRUE((m_manager->addSetting<TEST_ID_CHAR>(setting)));

    avsCommon::utils::WaitEvent waitEvent;
    EXPECT_CALL(*setting, clearData(DEFAULT_CHAR_VALUE)).WillOnce(testing::InvokeWithoutArgs([&waitEvent] {
        waitEvent.wakeUp();
        return true;
    }));

    m_manager->clearData();

    // Wait till last expectation is met.
    ASSERT_TRUE(waitEvent.wait(TEST_TIMEOUT));
}

}  // namespace test
}  // namespace settings
}  // namespace alexaClientSDK
