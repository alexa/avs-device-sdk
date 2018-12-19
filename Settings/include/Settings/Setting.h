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
#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTING_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTING_H_

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include <AVSCommon/Utils/Logger/LogEntry.h>
#include <AVSCommon/Utils/Logger/LoggerUtils.h>

#include "Settings/SetSettingResult.h"
#include "Settings/SettingInterface.h"
#include "Settings/SettingProtocolInterface.h"
#include "Settings/SettingStatus.h"
#include "Settings/SettingStringConversion.h"

namespace alexaClientSDK {
namespace settings {

/**
 * The setting class implements the setting interface for the given value.
 *
 * This class provide methods to set value through local request (@c set method) and through AVS request (@c
 * setAvsChange). This setting should also be able to persist the current value as well as revert to previous value if
 * a transaction fails.
 *
 * The setting can be configured to use multiple avs synchronization protocols. See @c SettingProtocolInterface.
 *
 * @tparam ValueT The type of the value used to represent a given setting.
 */
template <typename ValueT>
class Setting : public SettingInterface<ValueT> {
public:
    /// Define an alias for setting value type.
    using ValueType = ValueT;

    /**
     * Create a @c Setting object.
     *
     * @param defaultValue The default value to be used if no value was found in the storage.
     * @param settingProtocol The avs protocol used to persist and synchronize values with avs.
     * @param applyValueFn Function responsible for validating and applying a new setting value to the device.
     * @note The @c applyValueFn should succeed if there is no value change.
     * @return A pointer to the new object if it succeeds; @c nullptr otherwise.
     */
    static std::shared_ptr<Setting<ValueT>> create(
        const ValueType& defaultValue,
        std::unique_ptr<SettingProtocolInterface> settingProtocol,
        std::function<bool(const ValueType&)> applyValueFn = std::function<bool(const ValueType&)>());

    /**
     * Request to set the managed setting to the given @c value. This should be used when the request came from AVS.
     *
     * @param value The target value.
     * @return @c true if it succeeds to enqueue the request; @false otherwise.
     */
    bool setAvsChange(const ValueType& value);

    /// @name SettingInterface methods.
    /// @{
    SetSettingResult setLocalChange(const ValueType& value) override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param value The default value to be used if no value was found in the storage.
     * @param applyValueFn Function responsible for applying a new setting value on the device.
     * @param settingProtocol The avs protocol used to persist and synchronize values with avs.
     */
    Setting(
        const ValueType& value,
        std::function<bool(const ValueType&)> applyValueFn,
        std::unique_ptr<SettingProtocolInterface> settingProtocol);

    /**
     * Implement the set value logic. This is a common path between avs / local value change.
     *
     * @param value The target value.
     * @return A pair where @c first represents whether the value could be applied and @c second is a string
     * representation of @c m_value by the end of the operation.
     */
    std::pair<bool, std::string> handleSetValue(const ValueType& value);

    /**
     * Restore function called during setting initialization. This should restore the persisted value.
     *
     * @return @true if it succeeds; false otherwise.
     */
    bool restore();

    /// Function used to apply the new setting to the device. For example, for timezone, this function changes the
    /// device timezone.
    std::function<bool(const ValueType&)> m_applyFunction;

    /// The protocol object used to synchronize changes with AVS and database.
    std::unique_ptr<SettingProtocolInterface> m_protocol;

    /// Old value used in case the protocol fails and the old value has to be restored.
    ValueType m_oldValue;

    /// Mutex used to ensure that all callbacks are synchronized.
    std::mutex m_mutex;

    /// Alias to make log entries less verbose.
    using LogEntry = avsCommon::utils::logger::LogEntry;
};

template <typename ValueT>
std::shared_ptr<Setting<ValueT>> Setting<ValueT>::create(
    const ValueType& defaultValue,
    std::unique_ptr<SettingProtocolInterface> settingProtocol,
    std::function<bool(const ValueType&)> applyValueFn) {
    if (!settingProtocol) {
        avsCommon::utils::logger::acsdkError(LogEntry("Setting", "createFailed").d("reason", "nullSettingProtocol"));
        return nullptr;
    }

    auto setting =
        std::shared_ptr<Setting<ValueT>>(new Setting(defaultValue, applyValueFn, std::move(settingProtocol)));

    if (!setting->restore()) {
        avsCommon::utils::logger::acsdkError(LogEntry("Setting", "createFailed").d("reason", "restoreValueFailed"));
        return nullptr;
    }

    return setting;
}

template <typename ValueT>
std::pair<bool, std::string> Setting<ValueT>::handleSetValue(const ValueType& value) {
    bool ok;
    std::string valueStr;
    std::tie(ok, valueStr) = toSettingString<ValueT>(value);
    if (ok && (!this->m_applyFunction || this->m_applyFunction(value))) {
        avsCommon::utils::logger::acsdkInfo(LogEntry("Setting", __func__).d("value", valueStr));
        m_oldValue = this->m_value;
        this->m_value = value;
        return {true, valueStr};
    }
    avsCommon::utils::logger::acsdkError(LogEntry("Setting", "setValueFailed").d("reason", "applyFailed"));
    return {false, toSettingString<ValueType>(this->m_value).second};
}

template <typename ValueT>
bool Setting<ValueT>::setAvsChange(const ValueType& value) {
    auto executeSet = [this, value] {
        std::lock_guard<std::mutex> lock{m_mutex};
        return this->handleSetValue(value);
    };
    auto revertChange = [this] {
        std::lock_guard<std::mutex> lock{m_mutex};
        auto oldValue = m_oldValue;
        return this->handleSetValue(oldValue).second;
    };
    auto notifyObservers = [this](SettingNotifications notification) { this->notifyObservers(notification); };
    return m_protocol->avsChange(executeSet, revertChange, notifyObservers);
}

template <typename ValueT>
SetSettingResult Setting<ValueT>::setLocalChange(const ValueType& value) {
    auto executeSet = [this, value] {
        std::lock_guard<std::mutex> lock{m_mutex};
        return this->handleSetValue(value);
    };
    auto revertChange = [this] {
        std::lock_guard<std::mutex> lock{m_mutex};
        auto oldValue = m_oldValue;
        return this->handleSetValue(oldValue).second;
    };
    auto notifyObservers = [this](SettingNotifications notification) { this->notifyObservers(notification); };
    return m_protocol->localChange(executeSet, revertChange, notifyObservers);
}

template <typename ValueT>
bool Setting<ValueT>::restore() {
    auto applyChange = [this](const std::string& databaseValue) -> std::pair<bool, std::string> {
        bool convertOk, setOk;
        ValueT value;
        std::string valueStr;
        if (databaseValue.empty()) {
            convertOk = true;
            value = this->m_value;
        } else {
            std::tie(convertOk, value) = fromSettingString<ValueT>(databaseValue, this->m_value);
        }
        std::lock_guard<std::mutex> lock{m_mutex};
        std::tie(setOk, valueStr) = handleSetValue(value);
        return {convertOk && setOk, valueStr};
    };
    auto notifyObservers = [this](SettingNotifications notification) { this->notifyObservers(notification); };
    return m_protocol->restoreValue(applyChange, notifyObservers);
}

template <typename ValueT>
Setting<ValueT>::Setting(
    const ValueType& value,
    std::function<bool(const ValueType&)> applyValueFn,
    std::unique_ptr<SettingProtocolInterface> settingProtocol) :
        SettingInterface<ValueType>{value},
        m_applyFunction{applyValueFn},
        m_protocol{std::move(settingProtocol)},
        m_oldValue{value} {
}

}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTING_H_
