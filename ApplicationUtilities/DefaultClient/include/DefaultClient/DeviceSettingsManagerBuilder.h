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

#ifndef ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_DEVICESETTINGSMANAGERBUILDER_H_
#define ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_DEVICESETTINGSMANAGERBUILDER_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <tuple>

#include <ACL/AVSConnectionManager.h>
#include <AVSCommon/SDKInterfaces/LocaleAssetsManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/SystemTimeZoneInterface.h>
#include <acsdkDoNotDisturb/DoNotDisturbCapabilityAgent.h>
#include <RegistrationManager/CustomerDataManager.h>
#include <Settings/CloudControlledSettingProtocol.h>
#include <Settings/DeviceControlledSettingProtocol.h>
#include <Settings/DeviceSettingsManager.h>
#include <Settings/Setting.h>
#include <Settings/SettingEventMetadata.h>
#include <Settings/SettingEventSender.h>
#include <Settings/SettingsManagerBuilderBase.h>
#include <Settings/SharedAVSSettingProtocol.h>
#include <Settings/Storage/DeviceSettingStorageInterface.h>

namespace alexaClientSDK {
namespace defaultClient {

/**
 * The builder for @c DeviceSettingsManager.
 */
class DeviceSettingsManagerBuilder : public settings::SettingsManagerBuilderBase<settings::DeviceSettingsManager> {
public:
    /**
     * Constructor.
     *
     * @param settingStorage The storage used for settings.
     * @param messageSender Sender used to send events related to this setting changes.
     * @param connectionManager The ACL connection manager.
     * @param dataManager A dataManager object that will track the CustomerDataHandler.
     */
    DeviceSettingsManagerBuilder(
        std::shared_ptr<settings::storage::DeviceSettingStorageInterface> settingStorage,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<acl::AVSConnectionManager> connectionManager,
        std::shared_ptr<registrationManager::CustomerDataManager> dataManager);

    /**
     * Configures do not disturb setting.
     *
     * @param dndCA The do not disturb capability agent which is actually responsible for building the setting.
     *
     * @return This builder to allow nested calls.
     */
    DeviceSettingsManagerBuilder& withDoNotDisturbSetting(
        const std::shared_ptr<capabilityAgents::doNotDisturb::DoNotDisturbCapabilityAgent>& dndCA);

    /**
     * Configures alarm volume ramp setting.
     *
     * @return This builder to allow nested calls.
     */
    DeviceSettingsManagerBuilder& withAlarmVolumeRampSetting();

    /**
     * Configures wake word confirmation setting.
     *
     * @return This builder to allow nested calls.
     */
    DeviceSettingsManagerBuilder& withWakeWordConfirmationSetting();

    /**
     * Configures speech confirmation setting.
     *
     * @return This builder to allow nested calls.
     */
    DeviceSettingsManagerBuilder& withSpeechConfirmationSetting();

    /**
     * Configures time zone setting.
     *
     * @param systemTimeZone The system timezone is an optional parameter responsible for validating / applying
     * timezone changes system wide.
     * @return This builder to allow nested calls.
     */
    DeviceSettingsManagerBuilder& withTimeZoneSetting(
        std::shared_ptr<avsCommon::sdkInterfaces::SystemTimeZoneInterface> systemTimeZone = nullptr);

    /**
     * Configures locale setting.
     *
     * @param localeAssetsManager The locale assets manager is responsible for validating / applying locale changes.
     * @return This builder to allow nested calls.
     */
    DeviceSettingsManagerBuilder& withLocaleSetting(
        std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface> localeAssetsManager);

    /**
     * Configures locale and wake words setting.
     *
     * @param localeAssetsManager The locale assets manager is responsible for validating / applying locale
     * related changes.
     * @return This builder to allow nested calls.
     */
    DeviceSettingsManagerBuilder& withLocaleAndWakeWordsSettings(
        std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface> localeAssetsManager);

    /**
     * Configures network info setting.
     *
     * @return This builder to allow nested calls.
     */
    DeviceSettingsManagerBuilder& withNetworkInfoSetting();

    /**
     * Gets the setting for the given @c index.
     *
     * @tparam index The setting index.
     * @return A pointer for the setting kept in @c index if the setting has been built; @c nullptr otherwise.
     * @note This function should be used after @c build() has been called.
     */
    template <size_t index>
    std::shared_ptr<SettingType<index>> getSetting() const;

    /**
     * Gets the setting configuration for the given @c index.
     *
     * @tparam index The setting index.
     * @return The setting configuration. An empty setting will be returned if the setting wasn't configured.
     * @note This function should be used after @c build() has been called.
     */
    template <size_t index>
    settings::SettingConfiguration<SettingType<index>> getConfiguration() const;

    /**
     * Builds a @c DeviceSettingsManager with the settings previously configured.
     *
     * @return A pointer to a new DeviceSettingsManager if all settings were successfully built; @c nullptr otherwise.
     */
    std::unique_ptr<settings::DeviceSettingsManager> build() override;

private:
    /**
     * Builds a setting that follows the given synchronization protocol.
     *
     * @tparam index The setting index.
     * @tparam ProtocolT The type of the setting protocol.
     * @param metadata The setting event metadata.
     * @param defaultValue The setting default value.
     * @param applyFn Function responsible for validating and applying a new setting value.
     * @return This builder to allow nested calls.
     */
    template <size_t index, class ProtocolT>
    DeviceSettingsManagerBuilder& withSynchronizedSetting(
        const settings::SettingEventMetadata& metadata,
        const ValueType<index>& defaultValue,
        std::function<bool(const ValueType<index>&)> applyFn = std::function<bool(const ValueType<index>&)>());

    /// The setting storage used to build persistent settings.
    std::shared_ptr<settings::storage::DeviceSettingStorageInterface> m_settingStorage;

    /// The message sender used to build settings that are synchronized with AVS.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /// The connection manager that manages the connection with AVS.
    std::shared_ptr<acl::AVSConnectionManager> m_connectionManager;

    /// The dataManager object that will track the CustomerDataHandler.
    std::shared_ptr<registrationManager::CustomerDataManager> m_dataManager;

    /// Flag that indicates if there was any configuration error.
    bool m_foundError;
};

template <size_t index>
settings::SettingConfiguration<DeviceSettingsManagerBuilder::SettingType<index>> DeviceSettingsManagerBuilder::
    getConfiguration() const {
    return std::get<index>(m_settingConfigs);
}

template <size_t index>
std::shared_ptr<DeviceSettingsManagerBuilder::SettingType<index>> DeviceSettingsManagerBuilder::getSetting() const {
    return std::get<index>(m_settingConfigs).setting;
}

}  // namespace defaultClient
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_DEVICESETTINGSMANAGERBUILDER_H_
