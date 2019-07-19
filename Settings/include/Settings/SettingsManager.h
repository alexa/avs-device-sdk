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

#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGSMANAGER_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGSMANAGER_H_

#include <cstddef>
#include <memory>
#include <mutex>
#include <utility>

#include <AVSCommon/Utils/Logger/LogEntry.h>
#include <AVSCommon/Utils/Logger/LoggerUtils.h>

#include "SettingInterface.h"

namespace alexaClientSDK {
namespace settings {

/**
 * The @c SettingsManager is responsible for managing settings.
 */
template <typename... SettingsT>
class SettingsManager {
public:
    /**
     * Define the setting type kept on position @c index.
     *
     * Explanation:
     *   - tuple definition: This allow us to use @c std::get<> to get a specific index of @c SettingsT.
     *   - Pointers: Since a setting type can be abstract, we use pointers.
     *   - decltype: Gets the reference type of the "object" in @c index position.
     *   - decay: Extract the type from the reference returned by decltype.
     *
     * @tparam index The index of the target setting.
     */
    template <size_t index>
    using SettingType = typename std::decay<decltype(*std::get<index>(std::tuple<SettingsT*...>()))>::type;

    /// Define the value type for setting kept on position @c index.
    template <size_t index>
    using ValueType = typename SettingType<index>::ValueType;

    /// Define the observer type for setting kept on position @c index.
    template <size_t index>
    using ObserverType = typename SettingType<index>::ObserverType;

    /// Define the shared_ptr type for the given @c SettingT.
    template <typename SettingT>
    using SettingPointerType = std::shared_ptr<SettingT>;

    /**
     * Settings manager constructor.
     */
    SettingsManager() = default;

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
    std::pair<bool, ValueType<index>> getValue(const ValueType<index>& defaultValue) const;

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

private:
    /// Alias to make log entries less verbose.
    using LogEntry = avsCommon::utils::logger::LogEntry;

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
};

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

}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGSMANAGER_H_
