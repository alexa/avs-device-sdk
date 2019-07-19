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
#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGCALLBACKS_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGCALLBACKS_H_

#include <functional>
#include <memory>
#include <vector>

#include "SettingObserverInterface.h"
#include "SettingCallbackAdapter.h"

namespace alexaClientSDK {
namespace settings {

/**
 * The SettingCallbacks allow easy management of callbacks to monitor multiple settings with one object only.
 *
 * This object can be used to register multiple callbacks that are member functions, lambdas and static functions. E.g.:
 *
 * @code
 *
 * MyClass(std::shared_ptr<DeviceSettingManagers> manager) : m_callbacks {manager} {
 *     m_callbacks.add<ASCENDING_ALARM>([](bool enable, SettingNotifications notification){
 *         // do something...
 *     });
 *     m_callbacks.add<TIMEZONE>(staticFunction);
 *     m_callbacks.add<WAKEWORD_CONFIRMATION>(std::bind(this, &MyClass::onWakewordConfirmation, std::placeholder::_1));
 * }
 *
 * ~MyClass() {
 *     // Observers get removed during destruction.
 * }
 * @endcode
 *
 * If you would like to listen notifications related to only one setting, see @c SettingEventCallback class or extend
 * the @c SettingObserverInterface class.
 *
 * @tparam SettingsT Settings type enumeration.
 */
template <typename ManagerT>
class SettingCallbacks {
public:
    /// Setting value type.
    template <size_t id>
    using ValueType = typename ManagerT::template ValueType<id>;

    /// Callback function value type.
    template <size_t id>
    using FunctionType = std::function<void(const ValueType<id>&, SettingNotifications)>;

    /**
     * The destructor.
     */
    virtual ~SettingCallbacks();

    /**
     * Create a SettingCallbacks object.
     *
     * This method allow user to use static / lambda functions as the callbacks.
     *
     * @param manager The object that manages all settings.
     * @param callback The callback function for @c SettingT.
     * @param otherCallbacks The callback functions for the rest of the settings.
     * @return Pointer to new object if it succeeds to add all callbacks, @c nullptr otherwise.
     */
    static std::shared_ptr<SettingCallbacks<ManagerT>> create(std::shared_ptr<ManagerT> manager);

    /**
     * Add a callback function to the setting with @c id.
     * @tparam id The property id.
     * @param callback The callback function.
     * @return @c true if it succeeds; @c false otherwise.
     */
    template <size_t id>
    bool add(FunctionType<id> callback);

    /**
     * Disconnects the observed setting from all observed settings. This function also gets called by the destructor.
     *
     * @param manager The object that manages all settings.
     * @param observer The observer object to be disconnected.
     * @return @true if it succeeds, @c false otherwise.
     */
    void removeAll();

private:
    /**
     * Constructor.
     */
    SettingCallbacks(std::shared_ptr<ManagerT> manager);

    /// Keep a pointer to settings manager.
    std::shared_ptr<ManagerT> m_manager;

    /// Store all the callbacks that were successfully registered.
    std::vector<std::shared_ptr<SettingCallbackAdapterInterface<ManagerT>>> m_callbacks;
};

template <typename ManagerT>
SettingCallbacks<ManagerT>::~SettingCallbacks() {
    removeAll();
}

template <typename ManagerT>
std::shared_ptr<SettingCallbacks<ManagerT>> SettingCallbacks<ManagerT>::create(std::shared_ptr<ManagerT> manager) {
    if (manager) {
        return std::shared_ptr<SettingCallbacks<ManagerT>>(new SettingCallbacks<ManagerT>(manager));
    }
    return nullptr;
}

template <typename ManagerT>
template <size_t id>
bool SettingCallbacks<ManagerT>::add(SettingCallbacks::FunctionType<id> callback) {
    auto wrapper = SettingCallbackAdapter<ManagerT, id>::create(callback);
    if (wrapper) {
        if (wrapper->addToManager(*m_manager)) {
            m_callbacks.push_back(wrapper);
            return true;
        }
    }
    return false;
}

template <typename ManagerT>
void SettingCallbacks<ManagerT>::removeAll() {
    for (auto& wrapper : m_callbacks) {
        wrapper->removeFromManager(*m_manager);
    }
    m_callbacks.clear();
}

template <typename ManagerT>
SettingCallbacks<ManagerT>::SettingCallbacks(std::shared_ptr<ManagerT> manager) : m_manager{manager} {
}

}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGCALLBACKS_H_
