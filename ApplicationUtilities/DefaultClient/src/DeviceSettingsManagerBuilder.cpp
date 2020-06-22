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

#include <AIP/AudioInputProcessor.h>
#include <acsdkAlerts/AlertsCapabilityAgent.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <Settings/Types/LocaleWakeWordsSetting.h>
#include <System/TimeZoneHandler.h>
#include <System/LocaleHandler.h>

#include "DefaultClient/DeviceSettingsManagerBuilder.h"

/// String to identify log entries originating from this file.
static const std::string TAG("SettingsManagerBuilder");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace defaultClient {

using namespace settings;

/// The default timezone setting.
static const std::string DEFAULT_TIMEZONE = "Etc/GMT";

/// The key in our config file to find the root of settings for this database.
static const std::string SETTINGS_CONFIGURATION_ROOT_KEY = "deviceSettings";

/// The key to find the default timezone configuration.
static const std::string DEFAULT_TIMEZONE_CONFIGURATION_KEY = "defaultTimezone";

/// Network info setting events metadata.
static const SettingEventMetadata NETWORK_INFO_METADATA = {"System",
                                                           "NetworkInfoChanged",
                                                           "NetworkInfoReport",
                                                           "networkInfo"};

template <typename PointerT>
static inline bool checkPointer(const std::shared_ptr<PointerT>& pointer, const std::string& variableName) {
    if (!pointer) {
        ACSDK_ERROR(LX("checkPointerFailed").d("variable", variableName));
        return false;
    }
    return true;
}

DeviceSettingsManagerBuilder::DeviceSettingsManagerBuilder(
    std::shared_ptr<storage::DeviceSettingStorageInterface> settingStorage,
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<acl::AVSConnectionManager> connectionManager,
    std::shared_ptr<registrationManager::CustomerDataManager> dataManager) :
        m_settingStorage{settingStorage},
        m_messageSender{messageSender},
        m_connectionManager{connectionManager},
        m_dataManager{dataManager},
        m_foundError{false} {
    m_foundError =
        !(checkPointer(settingStorage, "settingStorage") && checkPointer(messageSender, "messageSender") &&
          checkPointer(connectionManager, "connectionManager"));
}

template <size_t index>
bool addSetting(DeviceSettingsManagerBuilder builder, DeviceSettingsManager& manager) {
    auto setting = builder.getSetting<index>();
    return addSetting<index - 1>(builder, manager) && (!setting || manager.addSetting<index>(setting));
}

template <>
bool addSetting<0>(DeviceSettingsManagerBuilder builder, DeviceSettingsManager& manager) {
    auto setting = builder.getSetting<0>();
    return !setting || manager.addSetting<0>(setting);
}

std::unique_ptr<DeviceSettingsManager> DeviceSettingsManagerBuilder::build() {
    if (m_foundError) {
        ACSDK_ERROR(LX("buildFailed").d("reason", "settingConstructionFailed"));
        return nullptr;
    }

    std::unique_ptr<DeviceSettingsManager> manager{new DeviceSettingsManager(m_dataManager)};
    if (!addSetting<NUMBER_OF_SETTINGS - 1>(*this, *manager)) {
        ACSDK_ERROR(LX("buildFailed").d("reason", "addSettingFailed"));
        return nullptr;
    }
    return manager;
}

DeviceSettingsManagerBuilder& DeviceSettingsManagerBuilder::withAlarmVolumeRampSetting() {
    return withSynchronizedSetting<DeviceSettingsIndex::ALARM_VOLUME_RAMP, SharedAVSSettingProtocol>(
        acsdkAlerts::AlertsCapabilityAgent::getAlarmVolumeRampMetadata(), types::getAlarmVolumeRampDefault());
}

DeviceSettingsManagerBuilder& DeviceSettingsManagerBuilder::withWakeWordConfirmationSetting() {
    return withSynchronizedSetting<DeviceSettingsIndex::WAKEWORD_CONFIRMATION, SharedAVSSettingProtocol>(
        capabilityAgents::aip::AudioInputProcessor::getWakeWordConfirmationMetadata(),
        getWakeWordConfirmationDefault());
}

DeviceSettingsManagerBuilder& DeviceSettingsManagerBuilder::withSpeechConfirmationSetting() {
    return withSynchronizedSetting<DeviceSettingsIndex::SPEECH_CONFIRMATION, SharedAVSSettingProtocol>(
        capabilityAgents::aip::AudioInputProcessor::getSpeechConfirmationMetadata(), getSpeechConfirmationDefault());
}

DeviceSettingsManagerBuilder& DeviceSettingsManagerBuilder::withTimeZoneSetting(
    std::shared_ptr<avsCommon::sdkInterfaces::SystemTimeZoneInterface> systemTimeZone) {
    std::function<bool(const TimeZoneSetting::ValueType&)> applyFunction;
    if (systemTimeZone) {
        applyFunction = [systemTimeZone](const TimeZoneSetting::ValueType& value) {
            return systemTimeZone->setTimezone(value);
        };
    }

    std::string defaultTimezone = DEFAULT_TIMEZONE;
    auto settingsConfig =
        avsCommon::utils::configuration::ConfigurationNode::getRoot()[SETTINGS_CONFIGURATION_ROOT_KEY];
    if (settingsConfig) {
        settingsConfig.getString(DEFAULT_TIMEZONE_CONFIGURATION_KEY, &defaultTimezone, DEFAULT_TIMEZONE);
    }

    return withSynchronizedSetting<DeviceSettingsIndex::TIMEZONE, SharedAVSSettingProtocol>(
        capabilityAgents::system::TimeZoneHandler::getTimeZoneMetadata(), defaultTimezone, applyFunction);
}

DeviceSettingsManagerBuilder& DeviceSettingsManagerBuilder::withLocaleSetting(
    std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface> localeAssetsManager) {
    // TODO: LocaleWakeWordsSetting should accept nullptr as wakeWordEventSender if wake words are disabled.
    auto dummySender = SettingEventSender::create(SettingEventMetadata(), m_messageSender);
    auto localeMetadata = capabilityAgents::system::LocaleHandler::getLocaleEventsMetadata();
    auto localeEventSender = SettingEventSender::create(localeMetadata, m_messageSender);
    auto setting = types::LocaleWakeWordsSetting::create(
        std::move(localeEventSender), std::move(dummySender), m_settingStorage, localeAssetsManager);

    if (!setting) {
        ACSDK_ERROR(LX("createLocaleWakeWordsSettingFailed").d("reason", "cannotAddSetting"));
        m_foundError = true;
        return *this;
    } else {
        std::get<DeviceSettingsIndex::LOCALE>(m_settingConfigs) =
            SettingConfiguration<LocalesSetting>{setting, localeMetadata};
    }

    m_connectionManager->addConnectionStatusObserver(setting);
    return *this;
}

DeviceSettingsManagerBuilder& DeviceSettingsManagerBuilder::withLocaleAndWakeWordsSettings(
    std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface> localeAssetsManager) {
    auto localeMetadata = capabilityAgents::system::LocaleHandler::getLocaleEventsMetadata();
    auto wakeWordsMetadata = capabilityAgents::aip::AudioInputProcessor::getWakeWordsEventsMetadata();
    auto localeEventSender = SettingEventSender::create(localeMetadata, m_messageSender);
    auto wakeWordsEventSender = SettingEventSender::create(wakeWordsMetadata, m_messageSender);
    auto setting = types::LocaleWakeWordsSetting::create(
        std::move(localeEventSender), std::move(wakeWordsEventSender), m_settingStorage, localeAssetsManager);

    if (!setting) {
        ACSDK_ERROR(LX("createLocaleWakeWordsSettingFailed").d("reason", "cannotAddSetting"));
        m_foundError = true;
        return *this;
    } else {
        std::get<DeviceSettingsIndex::LOCALE>(m_settingConfigs) =
            SettingConfiguration<LocalesSetting>{setting, localeMetadata};
        std::get<DeviceSettingsIndex::WAKE_WORDS>(m_settingConfigs) =
            SettingConfiguration<WakeWordsSetting>{setting, wakeWordsMetadata};
    }

    m_connectionManager->addConnectionStatusObserver(setting);
    return *this;
}

DeviceSettingsManagerBuilder& DeviceSettingsManagerBuilder::withNetworkInfoSetting() {
    return withSynchronizedSetting<DeviceSettingsIndex::NETWORK_INFO, DeviceControlledSettingProtocol>(
        NETWORK_INFO_METADATA, types::NetworkInfo());
}

DeviceSettingsManagerBuilder& DeviceSettingsManagerBuilder::withDoNotDisturbSetting(
    const std::shared_ptr<capabilityAgents::doNotDisturb::DoNotDisturbCapabilityAgent>& dndCA) {
    if (!dndCA) {
        ACSDK_ERROR(LX("withDNDSettingFailed").d("reason", "nullCA"));
        m_foundError = true;
        return *this;
    }

    std::get<DeviceSettingsIndex::DO_NOT_DISTURB>(m_settingConfigs) = SettingConfiguration<DoNotDisturbSetting>{
        dndCA->getDoNotDisturbSetting(), dndCA->getDoNotDisturbEventsMetadata()};
    return *this;
}

template <size_t index, class ProtocolT>
DeviceSettingsManagerBuilder& DeviceSettingsManagerBuilder::withSynchronizedSetting(
    const SettingEventMetadata& metadata,
    const ValueType<index>& defaultValue,
    std::function<bool(const ValueType<index>&)> applyFn) {
    auto eventSender = SettingEventSender::create(metadata, m_connectionManager);
    auto protocol = ProtocolT::create(metadata, std::move(eventSender), m_settingStorage, m_connectionManager);
    auto setting = Setting<ValueType<index>>::create(defaultValue, std::move(protocol), applyFn);

    if (!setting) {
        m_foundError = true;
        return *this;
    }
    std::get<index>(m_settingConfigs) = SettingConfiguration<SettingType<index>>{setting, metadata};
    return *this;
}

}  // namespace defaultClient
}  // namespace alexaClientSDK
