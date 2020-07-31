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
#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGINTERFACE_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGINTERFACE_H_

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_set>

#include <AVSCommon/Utils/GuardedValue.h>
#include <AVSCommon/Utils/Logger/LogEntry.h>
#include <AVSCommon/Utils/Logger/LoggerUtils.h>

#include "Settings/SetSettingResult.h"
#include "Settings/SettingObserverInterface.h"

namespace alexaClientSDK {
namespace settings {

/**
 * Interface for setting objects. Second template argument is used to restrict usage to the types accepted.
 *
 * The setting implementation only support types that have the following properties:
 * - copy assignable.
 * - Have operator>> and operator<< defined.
 *
 * @tparam ValueT The setting value type.
 */
template <typename ValueT>
class SettingInterface {
public:
    /// Define the setting value type.
    using ValueType = ValueT;

    /// Define the observer type for this setting.
    using ObserverType = SettingObserverInterface<SettingInterface<ValueType>>;

    /**
     * Destructor.
     */
    virtual ~SettingInterface() = default;

    /**
     * Request to set the managed setting to the given @c value. Note that this is an asynchronous operation.
     *
     * @param value The target value.
     * @return The result of the enqueue operation.
     */
    virtual SetSettingResult setLocalChange(const ValueType& value) = 0;

    /**
     * Request to set the managed setting to the given @c value. This should be used when the request came from AVS.
     *
     * @param value The target value.
     * @return @c true if it succeeds to enqueue the request; @false otherwise.
     */
    virtual bool setAvsChange(const ValueType& value) = 0;

    /**
     * Clear the data this @c Setting.
     *
     * @param value The default value for this setting.
     * @return @c true if it data is cleared; @false otherwise.
     */
    virtual bool clearData(const ValueType& value) = 0;

    /**
     * Get the current value of this setting. This will not take into consideration pending @c set requests.
     *
     * @return The setting value.
     */
    inline ValueType get() const;

    /**
     * Get the default value of this setting.
     *
     * @return The setting default value.
     */
    inline ValueType getDefault() const;

    /**
     * Register a setting observer.
     *
     * The observer will be executed every time the setting value changes or a request fails.
     *
     * @param observer The connector to register for calling back whenever the setting notifications.
     * @return @c true if the observer was added, @c false otherwise.
     */
    inline bool addObserver(std::shared_ptr<ObserverType> observer);

    /**
     * Remove a setting observer.
     *
     * @param observer The observer to remove.
     */
    inline void removeObserver(std::shared_ptr<ObserverType>& observer);

protected:
    /**
     * Constructor that initialize @c m_value with the given @c value.
     */
    SettingInterface(const ValueType& value);

    /**
     * Notifies observers of the status of applying a setting and it's current value.
     *
     * @param notification The type of notification to send to the observers.
     */
    void notifyObservers(SettingNotifications notification);

    /// Mutex used to guard observers.
    std::mutex m_observerMutex;

    /// Set of all observers of this setting.
    std::unordered_set<std::shared_ptr<ObserverType>> m_observers;

    /// Alias for GuardedValue with the templated value @c ValueType.
    using GuardedValue = avsCommon::utils::GuardedValue<ValueType>;

    /// Alias to make log entries less verbose.
    using LogEntry = avsCommon::utils::logger::LogEntry;

    /// The setting value. (is_trivially_copyable is not supported on gcc4.8)
    typename std::conditional<std::is_scalar<ValueType>::value, std::atomic<ValueType>, GuardedValue>::type m_value;

    /// The default setting value.
    const ValueType m_defaultValue;
};

template <typename ValueT>
ValueT SettingInterface<ValueT>::get() const {
    return m_value;
}

template <typename ValueT>
ValueT SettingInterface<ValueT>::getDefault() const {
    return m_defaultValue;
}

template <typename ValueT>
bool SettingInterface<ValueT>::addObserver(std::shared_ptr<ObserverType> observer) {
    if (observer) {
        std::lock_guard<std::mutex>{m_observerMutex};
        m_observers.insert(observer);
        return true;
    }
    avsCommon::utils::logger::acsdkError(LogEntry("SettingInterface", "addObserverFailed").d("reason", "nullObserver"));
    return false;
}

template <typename ValueT>
void SettingInterface<ValueT>::notifyObservers(SettingNotifications notification) {
    std::lock_guard<std::mutex>{m_observerMutex};
    for (auto& observer : m_observers) {
        ValueT value = m_value;
        observer->onSettingNotification(value, notification);
    }
}

template <typename ValueT>
void SettingInterface<ValueT>::removeObserver(std::shared_ptr<ObserverType>& observer) {
    std::lock_guard<std::mutex>{m_observerMutex};
    m_observers.erase(observer);
}

template <typename ValueT>
SettingInterface<ValueT>::SettingInterface(const ValueT& value) : m_value(value), m_defaultValue{value} {
}

}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGINTERFACE_H_
