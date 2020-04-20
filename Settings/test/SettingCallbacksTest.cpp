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

#include <RegistrationManager/CustomerDataManager.h>
#include <Settings/DeviceSettingsManager.h>
#include <Settings/SettingsManager.h>
#include <Settings/SettingCallbacks.h>
#include <Settings/SpeechConfirmationSettingType.h>
#include <Settings/WakeWordConfirmationSettingType.h>

namespace alexaClientSDK {
namespace settings {
namespace test {

using namespace testing;

/// @defgroup Define initial values for each setting type.
/// @{
constexpr auto INIT_ALARM_VOLUME_RAMP = types::AlarmVolumeRampTypes::NONE;
constexpr WakeWordConfirmationSettingType INIT_WAKEWORD_CONFIRMATION = WakeWordConfirmationSettingType::NONE;
const std::string INIT_TIMEZONE = "Canada/Eastern";
/// @}

/// @defgroup Define updated values for each setting type.
/// @{
constexpr auto NEW_ALARM_VOLUME_RAMP = types::AlarmVolumeRampTypes::ASCENDING;
const std::string NEW_TIMEZONE = "Canada/Pacific";
/// @}

/**
 * Stub a setting for test purpose.
 *
 * @tparam SettingT The setting type to be extended.
 */
template <typename SettingT>
class SettingStub : public SettingT {
public:
    /// Set the value synchronously for simplicity.
    SetSettingResult setLocalChange(const typename SettingT::ValueType& value);

    /// Just a stub that doesn't do anything.
    bool setAvsChange(const typename SettingT::ValueType& value);

    /// Just a stub that doesn't do anything.
    bool clearData(const typename SettingT::ValueType& value);

    /// Build setting object.
    SettingStub(const typename SettingT::ValueType& value);
};

template <typename SettingT>
SetSettingResult SettingStub<SettingT>::setLocalChange(const typename SettingT::ValueType& value) {
    this->m_value = value;
    this->notifyObservers(SettingNotifications::LOCAL_CHANGE);
    return SetSettingResult::ENQUEUED;
}

template <typename SettingT>
bool SettingStub<SettingT>::setAvsChange(const typename SettingT::ValueType& value) {
    return false;
}

template <typename SettingT>
bool SettingStub<SettingT>::clearData(const typename SettingT::ValueType& value) {
    return true;
}

template <typename SettingT>
SettingStub<SettingT>::SettingStub(const typename SettingT::ValueType& value) : SettingT{value} {
}

/**
 * Test class used to set-up tear down tests.
 */
class SettingCallbacksTest : public Test {
protected:
    /**
     * Set up the objects common for most tests.
     */
    void SetUp() override;

    /**
     * Tear down the objects common for most tests.
     */
    void TearDown() override;

    /// The device settings manager.
    std::shared_ptr<DeviceSettingsManager> m_manager;
    /// Alarm Volume Ramp setting stub.
    std::shared_ptr<SettingStub<AlarmVolumeRampSetting>> m_alarmVolumeRamp;
    /// Wake word confirmation setting stub.
    std::shared_ptr<SettingStub<WakeWordConfirmationSetting>> m_wwConfirmation;
    /// Timezone setting stub.
    std::shared_ptr<SettingStub<TimeZoneSetting>> m_timezone;
};

void SettingCallbacksTest::SetUp() {
    auto customerDataManager = std::make_shared<registrationManager::CustomerDataManager>();
    m_manager = std::make_shared<DeviceSettingsManager>(customerDataManager);
    m_alarmVolumeRamp = std::make_shared<SettingStub<AlarmVolumeRampSetting>>(INIT_ALARM_VOLUME_RAMP);
    m_wwConfirmation = std::make_shared<SettingStub<WakeWordConfirmationSetting>>(INIT_WAKEWORD_CONFIRMATION);
    m_timezone = std::make_shared<SettingStub<TimeZoneSetting>>(INIT_TIMEZONE);
    ASSERT_TRUE(m_manager->addSetting<DeviceSettingsIndex::ALARM_VOLUME_RAMP>(m_alarmVolumeRamp));
    ASSERT_TRUE(m_manager->addSetting<DeviceSettingsIndex::WAKEWORD_CONFIRMATION>(m_wwConfirmation));
    ASSERT_TRUE(m_manager->addSetting<DeviceSettingsIndex::TIMEZONE>(m_timezone));
}

void SettingCallbacksTest::TearDown() {
    m_manager->removeSetting<DeviceSettingsIndex::TIMEZONE>(m_timezone);
    m_manager->removeSetting<DeviceSettingsIndex::WAKEWORD_CONFIRMATION>(m_wwConfirmation);
    m_manager->removeSetting<DeviceSettingsIndex::ALARM_VOLUME_RAMP>(m_alarmVolumeRamp);
    m_timezone.reset();
    m_wwConfirmation.reset();
    m_alarmVolumeRamp.reset();
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
 * Observer class that register callback member functions.
 */
class ObserverClass {
public:
    /**
     * Constructor.
     *
     * @param manager The device manager used to register the callbacks.
     */
    ObserverClass(std::shared_ptr<DeviceSettingsManager>& manager);

    /**
     * Destructor.
     */
    ~ObserverClass() = default;

    /**
     * Callback method for alarm volume ramp. We don't care about the notification type, so we just omit it.
     *
     * @param value The new alarm volume ramp value.
     */
    void onAlarmVolumeRamp(const types::AlarmVolumeRampTypes& value);

    /**
     * Callback method for wakeword confirmation. We don't care about the notification type, so we just omit it.
     *
     * @param value The new wakeword confirmation value.
     */
    void onWakewordConfirmation(const WakeWordConfirmationSettingType& value);

    /**
     * Callback method for timezone. We don't care about the notification type, so we just omit it.
     *
     * @param value The new timezone value.
     */
    void onTimezone(const std::string& value);

    /// The device setting manager.
    std::shared_ptr<DeviceSettingsManager> m_manager;

    /// The callback wrapper.
    std::shared_ptr<SettingCallbacks<DeviceSettingsManager>> m_callbacks;

    /// The alarm volume ramp value.
    types::AlarmVolumeRampTypes m_alarmVolumeRampValue;

    /// The wakeword confirmation value.
    WakeWordConfirmationSettingType m_wakewordConfirmationValue;

    /// The timezone value.
    std::string m_timezoneValue;
};

ObserverClass::ObserverClass(std::shared_ptr<alexaClientSDK::settings::DeviceSettingsManager>& manager) :
        m_manager{manager},
        m_callbacks{SettingCallbacks<DeviceSettingsManager>::create(manager)},
        m_alarmVolumeRampValue{INIT_ALARM_VOLUME_RAMP},
        m_wakewordConfirmationValue{INIT_WAKEWORD_CONFIRMATION},
        m_timezoneValue{INIT_TIMEZONE} {
    EXPECT_TRUE((m_callbacks->add<DeviceSettingsIndex::ALARM_VOLUME_RAMP>(
        std::bind(&ObserverClass::onAlarmVolumeRamp, this, std::placeholders::_1))));
    EXPECT_TRUE((m_callbacks->add<DeviceSettingsIndex::WAKEWORD_CONFIRMATION>(
        std::bind(&ObserverClass::onWakewordConfirmation, this, std::placeholders::_1))));
    EXPECT_TRUE((m_callbacks->add<DeviceSettingsIndex::TIMEZONE>(
        std::bind(&ObserverClass::onTimezone, this, std::placeholders::_1))));
}

void ObserverClass::onAlarmVolumeRamp(const types::AlarmVolumeRampTypes& value) {
    m_alarmVolumeRampValue = value;
}

void ObserverClass::onWakewordConfirmation(const WakeWordConfirmationSettingType& value) {
    m_wakewordConfirmationValue = value;
}

void ObserverClass::onTimezone(const std::string& value) {
    m_timezoneValue = value;
}

/// Test callback for a mix of lambda and static functions.
TEST_F(SettingCallbacksTest, test_lambdaAndStaticCallbacks) {
    auto alarmVolumeRamp = INIT_ALARM_VOLUME_RAMP;
    auto alarmVolumeRampCallback = [&alarmVolumeRamp](
                                       const types::AlarmVolumeRampTypes& newValue, SettingNotifications) {
        alarmVolumeRamp = newValue;
    };

    auto callbacks = SettingCallbacks<DeviceSettingsManager>::create(m_manager);
    callbacks->add<DeviceSettingsIndex::ALARM_VOLUME_RAMP>(alarmVolumeRampCallback);
    callbacks->add<DeviceSettingsIndex::TIMEZONE>(staticCallback);

    m_manager->setValue<DeviceSettingsIndex::ALARM_VOLUME_RAMP>(NEW_ALARM_VOLUME_RAMP);
    m_manager->setValue<DeviceSettingsIndex::TIMEZONE>(NEW_TIMEZONE);

    EXPECT_EQ(globalTimezone, NEW_TIMEZONE);
    EXPECT_EQ(alarmVolumeRamp, NEW_ALARM_VOLUME_RAMP);
}

/// Test callback for member functions.
TEST_F(SettingCallbacksTest, test_memberCallback) {
    ObserverClass observer{m_manager};
    m_manager->setValue<DeviceSettingsIndex::ALARM_VOLUME_RAMP>(NEW_ALARM_VOLUME_RAMP);
    m_manager->setValue<DeviceSettingsIndex::TIMEZONE>(NEW_TIMEZONE);
    EXPECT_EQ(observer.m_alarmVolumeRampValue, NEW_ALARM_VOLUME_RAMP);
    EXPECT_EQ(observer.m_wakewordConfirmationValue, INIT_WAKEWORD_CONFIRMATION);
    EXPECT_EQ(observer.m_timezoneValue, NEW_TIMEZONE);
}

}  // namespace test
}  // namespace settings
}  // namespace alexaClientSDK
