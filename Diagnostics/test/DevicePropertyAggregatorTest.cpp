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

#include <future>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/AVS/IndicatorState.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockSpeakerManager.h>
#include <Diagnostics/DevicePropertyAggregator.h>
#include <RegistrationManager/MockCustomerDataManager.h>
#include <Settings/DeviceSettingsManager.h>
#include <acsdkAlertsInterfaces/AlertObserverInterface.h>

namespace alexaClientSDK {
namespace diagnostics {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace acsdkAlertsInterfaces;
using namespace testing;

/// string indicating default alert state.
static const std::string DEFAULT_ALERT_STATE = "IDLE";

/// string indicating default audio content id.
static const std::string DEFAULT_CONTENT_ID = "NONE";

/// string indicating default audio player state value.
static const std::string DEFAULT_AUDIO_PLAYER_STATE = "IDLE";

/// string indicating device context value.
static const std::string DEVICE_CONTEXT_VALUE = "TEST_DEVICE_CONTEXT";

/// Request token used to mock getContext return value.
static const ContextRequestToken MOCK_CONTEXT_REQUEST_TOKEN = 1;

/// string indicating device setting do not disturb disabled
static const std::string DO_NOT_DISTURB_DISABLED = "false";

/// string indicating device setting do not disturb enabled
static const std::string DO_NOT_DISTURB_ENABLED = "true";

/// Settings stub that just set the value immediately.
template <typename ValueT>
class SettingStub : public settings::SettingInterface<ValueT> {
public:
    SetSettingResult setLocalChange(const ValueT& value) override {
        this->m_value = value;
        this->notifyObservers(settings::SettingNotifications::LOCAL_CHANGE);
        return SetSettingResult::ENQUEUED;
    }

    SetSettingResult failLocalChange(const ValueT& value) {
        this->notifyObservers(settings::SettingNotifications::LOCAL_CHANGE_FAILED);
        return SetSettingResult::INTERNAL_ERROR;
    }
    bool setAvsChange(const ValueT& value) override {
        return false;
    }

    bool clearData(const ValueT& value) override {
        return true;
    }

    SettingStub(const ValueT& value) : settings::SettingInterface<ValueT>{value} {
    }
};

class DevicePropertyAggregatorTest : public ::testing::Test {
public:
    void SetUp() override;
    void TearDown() override;

    /**
     * Validates if the value in the @c DevicePropertyAggregator is same as the expected value.
     *
     * @param propertyKey The key of the property to validate.
     * @param expectedValue The expected value to compare against.
     * @return true if property value is equal to the expected value, false otherwise.
     */
    bool validatePropertyValue(const std::string& propertyKey, const std::string& expectedValue);

    /// The @c DevicePropertyAggregator to test.
    std::shared_ptr<DevicePropertyAggregator> m_devicePropertyAggregator;

    /// The @c SpeakerSettings for AVS Volume.
    SpeakerInterface::SpeakerSettings m_avsVolumeSetting;

    /// The @c SpeakerSettings for Alerts Voume.
    SpeakerInterface::SpeakerSettings m_alertsVolumeSetting;

    /// The mock @c ContextManager.
    std::shared_ptr<MockContextManager> m_mockContextManager;

    /// The mock @c SpeakerManager.
    std::shared_ptr<MockSpeakerManager> m_mockSpeakerManager;

    /// The thread to process the @c ContextManager requests on.
    std::thread m_mockContextMangerThread;

    /// The @c SettingsManager.
    std::shared_ptr<settings::DeviceSettingsManager> m_deviceSettingsManager;

    /// The @c SettingStub for the DoNotDisturb setting.
    std::shared_ptr<SettingStub<settings::DoNotDisturbSetting ::ValueType>> m_DNDSetting;

    /// The nummber of values in the propertyMap before adding device settings
    int m_propertyMapSizeBeforeDeviceSetting;
};

void DevicePropertyAggregatorTest::SetUp() {
    m_mockContextManager = std::make_shared<MockContextManager>();
    m_mockSpeakerManager = std::make_shared<MockSpeakerManager>();

    auto customerDataManager = std::make_shared<registrationManager::MockCustomerDataManager>();
    m_deviceSettingsManager = std::make_shared<settings::DeviceSettingsManager>(customerDataManager);

    m_DNDSetting = std::make_shared<SettingStub<settings::DoNotDisturbSetting::ValueType>>(true);
    m_deviceSettingsManager->addSetting<settings::DeviceSettingsIndex::DO_NOT_DISTURB>(m_DNDSetting);

    m_avsVolumeSetting.volume = 10;
    m_avsVolumeSetting.mute = false;

    m_alertsVolumeSetting.volume = 15;
    m_alertsVolumeSetting.mute = true;

    EXPECT_CALL(*m_mockSpeakerManager, getSpeakerSettings(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, _))
        .WillOnce(Invoke([this](ChannelVolumeInterface::Type type, SpeakerInterface::SpeakerSettings* settings) {
            settings->volume = m_avsVolumeSetting.volume;
            settings->mute = m_avsVolumeSetting.mute;
            std::promise<bool> result;
            result.set_value(true);
            return result.get_future();
        }));

    EXPECT_CALL(*m_mockSpeakerManager, getSpeakerSettings(ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME, _))
        .WillOnce(Invoke([this](ChannelVolumeInterface::Type type, SpeakerInterface::SpeakerSettings* settings) {
            settings->volume = m_alertsVolumeSetting.volume;
            settings->mute = m_alertsVolumeSetting.mute;
            std::promise<bool> result;
            result.set_value(true);
            return result.get_future();
        }));

    m_devicePropertyAggregator = DevicePropertyAggregator::create();
    m_devicePropertyAggregator->setContextManager(m_mockContextManager);
    m_devicePropertyAggregator->initializeVolume(m_mockSpeakerManager);
    m_propertyMapSizeBeforeDeviceSetting = m_devicePropertyAggregator->getAllDeviceProperties().size();
    m_devicePropertyAggregator->setDeviceSettingsManager(m_deviceSettingsManager);
}

void DevicePropertyAggregatorTest::TearDown() {
    if (m_mockContextMangerThread.joinable()) {
        m_mockContextMangerThread.join();
    }
    m_mockContextManager.reset();
}

bool DevicePropertyAggregatorTest::validatePropertyValue(
    const std::string& propertyKey,
    const std::string& expectedValue) {
    auto maybePropertyValue = m_devicePropertyAggregator->getDeviceProperty(propertyKey);
    if (maybePropertyValue.hasValue()) {
        return (maybePropertyValue.value() == expectedValue);
    } else {
        return false;
    }
}

/**
 * Test if DEVICE_CONTEXT returns empty Optional if ContextManager was not set.
 */
TEST_F(DevicePropertyAggregatorTest, test_noContextManager_EmptyOptional) {
    std::shared_ptr<DevicePropertyAggregator> devicePropertyAggregator = DevicePropertyAggregator::create();
    ASSERT_FALSE(devicePropertyAggregator->getDeviceProperty(DevicePropertyAggregator::DEVICE_CONTEXT).hasValue());
}

/**
 * Test if property map is initialized for speaker settings.
 */
TEST_F(DevicePropertyAggregatorTest, test_intializePropertyMap) {
    ASSERT_TRUE(validatePropertyValue(DevicePropertyAggregator::AVS_SPEAKER_VOLUME, "10"));
    ASSERT_TRUE(validatePropertyValue(DevicePropertyAggregator::AVS_SPEAKER_MUTE, "false"));
    ASSERT_TRUE(validatePropertyValue(DevicePropertyAggregator::AVS_ALERTS_VOLUME, "15"));
    ASSERT_TRUE(validatePropertyValue(DevicePropertyAggregator::AVS_ALERTS_MUTE, "true"));

    ASSERT_TRUE(validatePropertyValue(DevicePropertyAggregator::AUDIO_PLAYER_STATE, DEFAULT_AUDIO_PLAYER_STATE));
    ASSERT_TRUE(validatePropertyValue(DevicePropertyAggregator::CONTENT_ID, DEFAULT_CONTENT_ID));
    ASSERT_TRUE(validatePropertyValue(DevicePropertyAggregator::ALERT_TYPE_AND_STATE, DEFAULT_ALERT_STATE));
}

/**
 * Test if DEVICE_CONTEXT property returns empty Optional when context fetch fails.
 */
TEST_F(DevicePropertyAggregatorTest, test_getContextFailure_EmptyOptional) {
    auto contextFailureLambda = [this](
                                    std::shared_ptr<ContextRequesterInterface> contextRequester,
                                    const std::string& endpointId,
                                    const std::chrono::milliseconds& timeout) {
        if (m_mockContextMangerThread.joinable()) {
            m_mockContextMangerThread.join();
        }

        m_mockContextMangerThread = std::thread(
            [contextRequester]() { contextRequester->onContextFailure(ContextRequestError::BUILD_CONTEXT_ERROR); });
        return MOCK_CONTEXT_REQUEST_TOKEN;
    };

    EXPECT_CALL(*m_mockContextManager, getContext(_, _, _)).WillOnce(Invoke(contextFailureLambda));

    ASSERT_FALSE(m_devicePropertyAggregator->getDeviceProperty(DevicePropertyAggregator::DEVICE_CONTEXT).hasValue());
}

/**
 * Test if DEVICE_CONTEXT property returns valid value when context fetch succeeds.
 */
TEST_F(DevicePropertyAggregatorTest, test_getContextSuccessful) {
    auto contextSuccessLambda = [this](
                                    std::shared_ptr<ContextRequesterInterface> contextRequester,
                                    const std::string& endpointId,
                                    const std::chrono::milliseconds& timeout) {
        if (m_mockContextMangerThread.joinable()) {
            m_mockContextMangerThread.join();
        }

        m_mockContextMangerThread =
            std::thread([contextRequester]() { contextRequester->onContextAvailable(DEVICE_CONTEXT_VALUE); });
        return MOCK_CONTEXT_REQUEST_TOKEN;
    };

    EXPECT_CALL(*m_mockContextManager, getContext(_, _, _)).WillOnce(Invoke(contextSuccessLambda));

    ASSERT_EQ(
        m_devicePropertyAggregator->getDeviceProperty(DevicePropertyAggregator::DEVICE_CONTEXT), DEVICE_CONTEXT_VALUE);
}
/**
 * Test if to see if DeviceSettingsManager is null there isn't value for a do not disturb.
 */
TEST_F(DevicePropertyAggregatorTest, test_getDoNotDisturbWhenSettingsManagerIsNull) {
    std::shared_ptr<settings::DeviceSettingsManager> deviceSettingManager = nullptr;
    m_devicePropertyAggregator->setDeviceSettingsManager(deviceSettingManager);
    auto value = m_devicePropertyAggregator->getDeviceProperty(DevicePropertyAggregator::DO_NOT_DISTURB);
    ASSERT_FALSE(value.hasValue());
    m_devicePropertyAggregator->setDeviceSettingsManager(m_deviceSettingsManager);
}
/**
 * Test if to see if property map stays the same when setting DoNotDisturb fails
 */
TEST_F(DevicePropertyAggregatorTest, test_getDoNotDisturbWhenSettingFailed) {
    auto changeStatus = m_DNDSetting->failLocalChange(false);
    ASSERT_EQ(changeStatus, SetSettingResult::INTERNAL_ERROR);
    auto value = m_devicePropertyAggregator->getDeviceProperty(DevicePropertyAggregator::DO_NOT_DISTURB);
    ASSERT_EQ(value.value(), DO_NOT_DISTURB_ENABLED);
}
/**
 * Test successful setting and getting doNotDisturb setting
 */
TEST_F(DevicePropertyAggregatorTest, test_getDoNotDisturb) {
    m_deviceSettingsManager->setValue<settings::DeviceSettingsIndex::DO_NOT_DISTURB>(false);
    auto value = m_devicePropertyAggregator->getDeviceProperty(DevicePropertyAggregator::DO_NOT_DISTURB);
    ASSERT_EQ(value.value(), DO_NOT_DISTURB_DISABLED);
    m_deviceSettingsManager->setValue<settings::DeviceSettingsIndex::DO_NOT_DISTURB>(true);
    value = m_devicePropertyAggregator->getDeviceProperty(DevicePropertyAggregator::DO_NOT_DISTURB);
    ASSERT_EQ(value.value(), DO_NOT_DISTURB_ENABLED);
}

/**
 * Test if getDeviceProperty returns empty Optional string.
 */
TEST_F(DevicePropertyAggregatorTest, test_getDevicePropertyForInvalidKey_EmptyOptional) {
    auto maybePropertyValue = m_devicePropertyAggregator->getDeviceProperty("TEST_KEY");
    ASSERT_FALSE(maybePropertyValue.hasValue());
}

/**
 * Test retrieving a Speaker property for a non-intializaed volume returns empty string.
 */
TEST_F(DevicePropertyAggregatorTest, test_getSpeakerSettingsPropertyForNonInitializedVolume_EmptyString) {
    std::shared_ptr<DevicePropertyAggregator> devicePropertyAggregator = DevicePropertyAggregator::create();

    ASSERT_FALSE(validatePropertyValue(DevicePropertyAggregator::AVS_SPEAKER_VOLUME, ""));
    ASSERT_FALSE(validatePropertyValue(DevicePropertyAggregator::AVS_SPEAKER_MUTE, ""));
    ASSERT_FALSE(validatePropertyValue(DevicePropertyAggregator::AVS_ALERTS_VOLUME, ""));
    ASSERT_FALSE(validatePropertyValue(DevicePropertyAggregator::AVS_ALERTS_MUTE, ""));
}

/**
 * Test if get Speaker Settings get updated if the observer method is called.
 */
TEST_F(DevicePropertyAggregatorTest, test_getSpeakerSettingsProperty) {
    SpeakerInterface::SpeakerSettings avsVolumeSettings, alertsVolumeSettings;

    avsVolumeSettings.mute = true;
    avsVolumeSettings.volume = 50;

    alertsVolumeSettings.mute = false;
    alertsVolumeSettings.volume = 80;

    m_devicePropertyAggregator->onSpeakerSettingsChanged(
        SpeakerManagerObserverInterface::Source::LOCAL_API,
        ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME,
        avsVolumeSettings);

    m_devicePropertyAggregator->onSpeakerSettingsChanged(
        SpeakerManagerObserverInterface::Source::LOCAL_API,
        ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME,
        alertsVolumeSettings);

    ASSERT_TRUE(validatePropertyValue(DevicePropertyAggregator::AVS_SPEAKER_VOLUME, "50"));
    ASSERT_TRUE(validatePropertyValue(DevicePropertyAggregator::AVS_SPEAKER_MUTE, "true"));
    ASSERT_TRUE(validatePropertyValue(DevicePropertyAggregator::AVS_ALERTS_VOLUME, "80"));
    ASSERT_TRUE(validatePropertyValue(DevicePropertyAggregator::AVS_ALERTS_MUTE, "false"));
}

/**
 * Test if Connection status gets updated if the observer method is called.
 */
TEST_F(DevicePropertyAggregatorTest, test_getConnectionStatusProperty) {
    m_devicePropertyAggregator->onConnectionStatusChanged(
        ConnectionStatusObserverInterface::Status::PENDING, ConnectionStatusObserverInterface::ChangedReason::SUCCESS);

    ASSERT_TRUE(validatePropertyValue(DevicePropertyAggregator::CONNECTION_STATE, "PENDING"));
}

/**
 * Test if Notification status gets updated if the observer method is called.
 */
TEST_F(DevicePropertyAggregatorTest, test_getNotificationStatusProperty) {
    m_devicePropertyAggregator->onSetIndicator(IndicatorState::ON);

    ASSERT_TRUE(validatePropertyValue(DevicePropertyAggregator::NOTIFICATION_INDICATOR, "ON"));
}

/**
 * Test if the Audio Player Status gets updated if the observer method is called.
 */
TEST_F(DevicePropertyAggregatorTest, test_getAudioPlayerStatusProperty) {
    acsdkAudioPlayerInterfaces::AudioPlayerObserverInterface::Context context;
    context.audioItemId = "abcd";
    context.offset = std::chrono::seconds(5);

    m_devicePropertyAggregator->onPlayerActivityChanged(PlayerActivity::PLAYING, context);

    ASSERT_TRUE(validatePropertyValue(DevicePropertyAggregator::AUDIO_PLAYER_STATE, "PLAYING"));
    ASSERT_TRUE(validatePropertyValue(DevicePropertyAggregator::CONTENT_ID, context.audioItemId));
}

/**
 * Test if the TTS player state gets updated if the observer method is called.
 */
TEST_F(DevicePropertyAggregatorTest, test_getTTSPlayerStateProperty) {
    m_devicePropertyAggregator->onDialogUXStateChanged(DialogUXStateObserverInterface::DialogUXState::LISTENING);

    ASSERT_TRUE(validatePropertyValue(DevicePropertyAggregator::TTS_PLAYER_STATE, "LISTENING"));
}

/**
 * Test if the Alarm status gets updated if the observer method is called.
 */
TEST_F(DevicePropertyAggregatorTest, test_getAlarmStatusProperty) {
    m_devicePropertyAggregator->onAlertStateChange(AlertObserverInterface::AlertInfo(
        "TEST_TOKEN",
        AlertObserverInterface::Type::ALARM,
        AlertObserverInterface::State::STARTED,
        std::chrono::system_clock::now(),
        avsCommon::utils::Optional<AlertObserverInterface::OriginalTime>(),
        avsCommon::utils::Optional<std::string>(),
        "TEST_ALERT_REASON"));

    ASSERT_TRUE(validatePropertyValue(DevicePropertyAggregator::ALERT_TYPE_AND_STATE, "ALARM:STARTED"));
}

/**
 * Test if authorized, registration status is true.
 */
TEST_F(DevicePropertyAggregatorTest, test_getRegistationStatusPropertyTrue) {
    m_devicePropertyAggregator->onAuthStateChange(
        AuthObserverInterface::State::REFRESHED, AuthObserverInterface::Error::SUCCESS);
    ASSERT_TRUE(validatePropertyValue(DevicePropertyAggregator::REGISTRATION_STATUS, "true"));
}

/**
 * Test if de-authorized, registration status is false.
 */
TEST_F(DevicePropertyAggregatorTest, test_getRegistationStatusPropertyFalse) {
    m_devicePropertyAggregator->onAuthStateChange(
        AuthObserverInterface::State::UNINITIALIZED, AuthObserverInterface::Error::SUCCESS);
    ASSERT_TRUE(validatePropertyValue(DevicePropertyAggregator::REGISTRATION_STATUS, "false"));

    m_devicePropertyAggregator->onAuthStateChange(
        AuthObserverInterface::State::EXPIRED, AuthObserverInterface::Error::SUCCESS);
    ASSERT_TRUE(validatePropertyValue(DevicePropertyAggregator::REGISTRATION_STATUS, "false"));
}

}  // namespace test
}  // namespace diagnostics
}  // namespace alexaClientSDK
