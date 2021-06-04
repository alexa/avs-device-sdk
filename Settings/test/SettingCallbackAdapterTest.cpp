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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <RegistrationManager/MockCustomerDataManager.h>
#include <Settings/DeviceSettingsManager.h>
#include <Settings/SettingCallbackAdapter.h>

namespace alexaClientSDK {
namespace settings {
namespace test {

using namespace testing;

/// Define initial values for timezone.
const std::string INIT_TIMEZONE = "Canada/Eastern";

/// Define updated values for each timezone.
const std::string NEW_TIMEZONE = "Canada/Pacific";

/// Just an alias to make the name shorter.
using TestEventCallback = SettingCallbackAdapter<DeviceSettingsManager, DeviceSettingsIndex::TIMEZONE>;

/**
 * Stub a setting for test purpose.
 */
class TimezoneSettingStub : public TimeZoneSetting {
public:
    /// Set the value synchronously for simplicity.
    SetSettingResult setLocalChange(const std::string& value) override;

    /// Just a stub that doesn't do anything.
    bool setAvsChange(const std::string& value) override;

    /// Just a stub that doesn't do anything.
    bool clearData(const std::string& value) override;

    /// Build setting object.
    explicit TimezoneSettingStub(const std::string& value);
};

SetSettingResult TimezoneSettingStub::setLocalChange(const std::string& value) {
    this->m_value = value;
    this->notifyObservers(SettingNotifications::LOCAL_CHANGE);
    return SetSettingResult::ENQUEUED;
}

TimezoneSettingStub::TimezoneSettingStub(const std::string& value) : TimeZoneSetting{value} {
}

bool TimezoneSettingStub::setAvsChange(const std::string& value) {
    return false;
}

bool TimezoneSettingStub::clearData(const std::string& value) {
    return true;
}

/**
 * Test class used to set-up tear down tests.
 */
class SettingCallbackAdapterTest : public Test {
protected:
    /**
     * Set up the objects common for most tests.
     */
    void SetUp() override;

    /**
     * Tear down the objects common for most tests.
     */
    void TearDown() override;

    /// Device manager object.
    std::shared_ptr<DeviceSettingsManager> m_manager;
    /// Stub for the timezone setting.
    std::shared_ptr<TimezoneSettingStub> m_timezone;
};

void SettingCallbackAdapterTest::SetUp() {
    auto customerDataManager = std::make_shared<NiceMock<registrationManager::MockCustomerDataManager>>();
    DeviceSettingManagerSettingConfigurations settingConfigs;

    m_manager = std::make_shared<DeviceSettingsManager>(customerDataManager, settingConfigs);
    m_timezone = std::make_shared<TimezoneSettingStub>(INIT_TIMEZONE);
    ASSERT_TRUE(m_manager->addSetting<DeviceSettingsIndex::TIMEZONE>(m_timezone));
}

void SettingCallbackAdapterTest::TearDown() {
    m_timezone.reset();
    m_manager.reset();
}

/// Used to test static callback.
std::string globalTimezone = INIT_TIMEZONE;

/// Define a static function to be used as callback.
static void staticCallback(const std::string& newValue, SettingNotifications notification) {
    globalTimezone = newValue;
    EXPECT_EQ(notification, SettingNotifications::LOCAL_CHANGE);
}

/**
 * Observer class that register callback member function.
 */
class ObserverClass {
public:
    /**
     * Constructor.
     *
     * @param manager The device manager used to register the callbacks.
     */
    explicit ObserverClass(std::shared_ptr<DeviceSettingsManager>& manager);

    /**
     * Destructor.
     */
    ~ObserverClass();

    /**
     * Callback method for timezone. We don't care about the notification type, so we just omit it.
     *
     * @param value The new timezone value.
     */
    void onTimezone(const std::string& value);

    /// Adapter for timezone callback.
    std::shared_ptr<TestEventCallback> m_timezoneAdapter;

    /// Pointer to settings manager.
    std::shared_ptr<DeviceSettingsManager> m_manager;

    /// The timezone value reported.
    std::string m_timezoneValue;
};

ObserverClass::ObserverClass(std::shared_ptr<alexaClientSDK::settings::DeviceSettingsManager>& manager) :
        m_timezoneAdapter{
            TestEventCallback::create(std::bind(&ObserverClass::onTimezone, this, std::placeholders::_1))},
        m_manager{manager},
        m_timezoneValue{INIT_TIMEZONE} {
    EXPECT_TRUE(m_timezoneAdapter->addToManager(*m_manager));
}

ObserverClass::~ObserverClass() {
    m_timezoneAdapter->removeFromManager(*m_manager);
}

void ObserverClass::onTimezone(const std::string& value) {
    m_timezoneValue = value;
}

/// Test callback for lambda functions.
TEST_F(SettingCallbackAdapterTest, test_lambdaCallback) {
    std::string value = INIT_TIMEZONE;
    auto callback = [&value](const std::string& newValue, SettingNotifications) { value = newValue; };

    auto callbackAdapter = TestEventCallback::create(callback);
    callbackAdapter->addToManager(*m_manager);

    m_timezone->setLocalChange(NEW_TIMEZONE);
    EXPECT_EQ(value, NEW_TIMEZONE);
}

/// Test callback for static functions.
TEST_F(SettingCallbackAdapterTest, test_staticCallback) {
    auto callbackAdapter = TestEventCallback::create(staticCallback);
    callbackAdapter->addToManager(*m_manager);

    m_timezone->setLocalChange(NEW_TIMEZONE);
    EXPECT_EQ(globalTimezone, NEW_TIMEZONE);
}

/// Test callback for member functions.
TEST_F(SettingCallbackAdapterTest, test_memberCallback) {
    ObserverClass observer{m_manager};
    m_timezone->setLocalChange(NEW_TIMEZONE);
    EXPECT_EQ(observer.m_timezoneValue, NEW_TIMEZONE);
}

}  // namespace test
}  // namespace settings
}  // namespace alexaClientSDK
