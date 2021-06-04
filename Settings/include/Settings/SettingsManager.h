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

#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGSMANAGER_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGSMANAGER_H_

#include <cstddef>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>

#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/Logger/LogEntry.h>
#include <AVSCommon/Utils/Logger/LoggerUtils.h>
#include <RegistrationManager/CustomerDataHandler.h>

#include "Settings/SettingEventMetadata.h"
#include "Settings/SettingInterface.h"
#include "Settings/SettingStringConversion.h"

namespace alexaClientSDK {
namespace settings {

/**
 * Structure to save a specific setting and its configuration.
 *
 * @tparam SettingsT The type of the setting.
 */
template <typename SettingsT>
struct SettingConfiguration {
    /// The setting configured.
    std::shared_ptr<SettingsT> setting;
    /// The setting metadata.
    avsCommon::utils::Optional<settings::SettingEventMetadata> metadata;
};

/**
 * The @c SettingsManager is responsible for managing settings.
 */
template <typename... SettingsT>
class SettingsManager : public registrationManager::CustomerDataHandler {
public:
    /// The setting type kept at @c index position.
    template <size_t index>
    using SettingType = typename std::tuple_element<index, std::tuple<SettingsT...>>::type;

    /// Define the value type for setting kept on position @c index.
    template <size_t index>
    using ValueType = typename SettingType<index>::ValueType;

    /// Define the observer type for setting kept on position @c index.
    template <size_t index>
    using ObserverType = typename SettingType<index>::ObserverType;

    /// Define the shared_ptr type for the given @c SettingT.
    template <typename SettingT>
    using SettingPointerType = std::shared_ptr<SettingT>;

    /// The tuple holding the settings configuration.
    using SettingConfigurations = std::tuple<SettingConfiguration<SettingsT>...>;

    /// The number of settings supported by this manager.
    static constexpr size_t NUMBER_OF_SETTINGS{sizeof...(SettingsT)};

    /**
     * Settings manager constructor.
     *
     * @param dataManager A dataManager object that will track the CustomerDataHandler.
     * @param settingConfigurations The tuple holding the settings configuration.
     */
    SettingsManager(
        std::shared_ptr<registrationManager::CustomerDataManagerInterface> dataManager,
        SettingConfigurations settingConfigurations);

    /**
     * Settings manager constructor.
     *
     * @deprecated
     * @param dataManager A dataManager object that will track the CustomerDataHandler.
     */
    SettingsManager(std::shared_ptr<registrationManager::CustomerDataManagerInterface> dataManager);

    /**
     * SettingsManager destructor.
     */
    virtual ~SettingsManager() = default;

    /**
     * Enqueue request to set the given settings represented by @c id to the given @c value.
     *
     * @tparam index The index of the target setting.
     * @param value The set value.
     * @return The result of the enqueue operation.
     */
    template <size_t index>
    SetSettingResult setValue(const ValueType<index>& value);

    /**
     * Get the current value of this setting. This will not take into consideration pending @c set requests.
     *
     * @tparam index The index of the target setting.
     * @param defaultValue The value to be returned in case the setting does not exist.
     * @return A pair representing whether the setting exist and what is its value. If the setting doesn't exist,
     * this function will return {false, defaultValue}. If the setting exist, it will return {true, settingValue}.
     */
    template <size_t index>
    std::pair<bool, ValueType<index>> getValue(const ValueType<index>& defaultValue = ValueType<index>()) const;

    /**
     * Get a json representation of the current setting value.
     *
     * @tparam index The index of the target setting.
     * @return The json representation of the given setting. An empty string will be returned if the setting
     * doesn't exist or conversion failed.
     */
    template <size_t index>
    std::string getJsonValue() const;

    /**
     * Register an observer for a given setting.
     *
     * @tparam index The index of the target setting.
     * @param observer The setting observer.
     * @return @c true if it succeed to add the observer, @c false otherwise.
     */
    template <size_t index>
    bool addObserver(std::shared_ptr<ObserverType<index>> observer);

    /**
     * Unregister an observer for a given setting.
     *
     * @tparam index The index of the target setting.
     * @param observer The setting observer.
     */
    template <size_t index>
    void removeObserver(std::shared_ptr<ObserverType<index>> observer);

    /**
     * Register a new setting to be managed.
     *
     * @tparam index The index of the target setting.
     * @param setting The setting object.
     * @return @c true if it succeeds; @c false if manager already has a setting object from the same type.
     */
    template <size_t index>
    bool addSetting(std::shared_ptr<SettingType<index>> setting);

    /**
     * Unregister a new setting to be managed.
     *
     * @tparam index The index of the target setting.
     * @param setting The setting object.
     */
    template <size_t index>
    void removeSetting(std::shared_ptr<SettingType<index>> setting);

    /**
     * Checks if a setting is available.
     *
     * @tparam index The index of the target setting.
     * @return @c true if the setting is available, @c false otherwise.
     */
    template <size_t index>
    bool hasSetting();

    /**
     * Gets the settings configuration.
     *
     * @return the settings configuration.
     */
    SettingConfigurations getConfigurations() const;

    /**
     * Gets the setting for the given @c index.
     *
     * @tparam index The setting index.
     * @return A pointer for the setting kept in @c index if the setting has been built; @c nullptr otherwise.
     * @note This function should be used after @c build() has been called.
     */
    template <size_t index>
    std::shared_ptr<SettingType<index>> getSetting() const;

    /// @name CustomerDataHandler Functions
    /// @{
    void clearData() override;
    /// @}

private:
    /// Alias to make log entries less verbose.
    using LogEntry = avsCommon::utils::logger::LogEntry;

    /**
     * ClearData on the setting when @c CustomerDataHandler triggers clearData callback.
     *
     * @note This function is an no-op for the case when index is the size of SettingsT.
     *
     */
    template <size_t index>
    typename std::enable_if<index >= NUMBER_OF_SETTINGS, void>::type doClearData() {
    }

    /**
     * ClearData on the setting when @c CustomerDataHandler triggers clearData callback.
     *
     * @note This is a recursive function for iterating through a tuple.
     *
     */

    template <size_t index>
    typename std::enable_if<index <= NUMBER_OF_SETTINGS - 1, void>::type doClearData() {
        auto& setting = std::get<index>(m_settings);
        if (setting) {
            if (!setting->clearData(setting->getDefault())) {
                avsCommon::utils::logger::acsdkError(LogEntry("SettingManager", "clearDataFailed")
                                                         .d("reason", "invalidSetting")
                                                         .d("settingIndex", index));
            }
        } else {
            avsCommon::utils::logger::acsdkDebug0(
                LogEntry("SettingManager", __func__).d("reason", "invalidSetting").d("settingIndex", index));
        }
        doClearData<index + 1>();
    }

    /**
     * Delete copy constructor.
     */
    SettingsManager(const SettingsManager&) = delete;

    /**
     * Delete operator=.
     */
    SettingsManager& operator=(const SettingsManager&) = delete;

    /// Mutex for settings.
    mutable std::mutex m_mutex;

    // A tuple with all the settings supported by this manager.
    std::tuple<SettingPointerType<SettingsT>...> m_settings;

    /// A tuple with all setting configurations.
    SettingConfigurations m_settingConfigs;
};

template <typename... SettingsT>
SettingsManager<SettingsT...>::SettingsManager(
    std::shared_ptr<registrationManager::CustomerDataManagerInterface> dataManager,
    SettingConfigurations settingConfiguration) :
        CustomerDataHandler{dataManager},
        m_settingConfigs{settingConfiguration} {
}

template <typename... SettingsT>
SettingsManager<SettingsT...>::SettingsManager(
    std::shared_ptr<registrationManager::CustomerDataManagerInterface> dataManager) :
        CustomerDataHandler{dataManager} {
}

template <typename... SettingsT>
template <size_t index>
std::pair<bool, typename SettingsManager<SettingsT...>::template ValueType<index>> SettingsManager<
    SettingsT...>::getValue(const ValueType<index>& defaultValue) const {
    std::lock_guard<std::mutex> lock{m_mutex};
    auto& setting = std::get<index>(m_settings);
    if (setting) {
        return {true, setting->get()};
    }

    avsCommon::utils::logger::acsdkError(
        LogEntry("SettingManager", "getValueFailed").d("reason", "invalidSetting").d("settingIndex", index));
    return {false, defaultValue};
}

template <typename... SettingsT>
template <size_t index>
std::string SettingsManager<SettingsT...>::getJsonValue() const {
    std::lock_guard<std::mutex> lock{m_mutex};
    auto& setting = std::get<index>(m_settings);
    if (setting) {
        auto result = toSettingString<ValueType<index>>(setting->get());
        if (!result.first) {
            avsCommon::utils::logger::acsdkError(LogEntry("SettingManager", "getStringValueFailed")
                                                     .d("reason", "toSettingStringFailed")
                                                     .d("settingIndex", index));
            return std::string();
        }
        return result.second;
    }

    avsCommon::utils::logger::acsdkDebug0(
        LogEntry("SettingManager", __func__).d("result", "noSettingAvailable").d("settingIndex", index));
    return std::string();
}

template <typename... SettingsT>
template <size_t index>
SetSettingResult SettingsManager<SettingsT...>::setValue(const ValueType<index>& value) {
    std::lock_guard<std::mutex> lock{m_mutex};
    auto& setting = std::get<index>(m_settings);
    if (setting) {
        return setting->setLocalChange(value);
    }

    avsCommon::utils::logger::acsdkError(
        LogEntry("SettingManager", "setValueFailed").d("reason", "invalidSetting").d("settingIndex", index));
    return SetSettingResult::UNAVAILABLE_SETTING;
}

template <typename... SettingsT>
template <size_t index>
bool SettingsManager<SettingsT...>::addObserver(std::shared_ptr<ObserverType<index>> observer) {
    std::lock_guard<std::mutex> lock{m_mutex};
    auto& setting = std::get<index>(m_settings);
    if (setting && observer) {
        return setting->addObserver(observer);
    }

    avsCommon::utils::logger::acsdkError(
        LogEntry("SettingManager", "addObserverFailed").d("reason", "invalidSetting").d("settingIndex", index));
    return false;
}

template <typename... SettingsT>
template <size_t index>
void SettingsManager<SettingsT...>::removeObserver(std::shared_ptr<ObserverType<index>> observer) {
    std::lock_guard<std::mutex> lock{m_mutex};
    auto& setting = std::get<index>(m_settings);
    if (setting && observer) {
        setting->removeObserver(observer);
        return;
    }

    avsCommon::utils::logger::acsdkError(
        LogEntry("SettingManager", "removeObserverFailed").d("reason", "invalidSetting").d("settingIndex", index));
}

template <typename... SettingsT>
template <size_t index>
bool SettingsManager<SettingsT...>::addSetting(std::shared_ptr<SettingType<index>> newSetting) {
    std::lock_guard<std::mutex> lock{m_mutex};
    auto& setting = std::get<index>(m_settings);
    if (!setting && newSetting) {
        setting.swap(newSetting);
        return true;
    }

    avsCommon::utils::logger::acsdkError(
        LogEntry("SettingManager", "addSettingFailed").d("reason", "invalidSetting").d("settingIndex", index));
    return false;
}

template <typename... SettingsT>
template <size_t index>
void SettingsManager<SettingsT...>::removeSetting(std::shared_ptr<SettingType<index>> oldSetting) {
    std::lock_guard<std::mutex> lock{m_mutex};
    auto& setting = std::get<index>(m_settings);
    if (setting == oldSetting) {
        setting.reset();
    } else {
        avsCommon::utils::logger::acsdkError(
            LogEntry("SettingManager", "removeSettingFailed").d("reason", "invalidSetting").d("settingIndex", index));
    }
}

template <typename... SettingsT>
template <size_t index>
bool SettingsManager<SettingsT...>::hasSetting() {
    std::lock_guard<std::mutex> lock{m_mutex};
    return std::get<index>(m_settings);
}

template <typename... SettingsT>
void SettingsManager<SettingsT...>::clearData() {
    std::lock_guard<std::mutex> lock{m_mutex};
    doClearData<0>();
}

template <typename... SettingsT>
std::tuple<SettingConfiguration<SettingsT>...> SettingsManager<SettingsT...>::getConfigurations() const {
    return m_settingConfigs;
}

template <typename... SettingsT>
template <size_t index>
std::shared_ptr<typename SettingsManager<SettingsT...>::template SettingType<index>> SettingsManager<
    SettingsT...>::getSetting() const {
    return std::get<index>(m_settingConfigs).setting;
}

}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGSMANAGER_H_
