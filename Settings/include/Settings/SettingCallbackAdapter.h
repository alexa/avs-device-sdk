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
#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGCALLBACKADAPTER_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGCALLBACKADAPTER_H_

#include <functional>
#include <memory>

#include "SettingsManager.h"
#include "SettingObserverInterface.h"

namespace alexaClientSDK {
namespace settings {

/**
 * Interface for the SettingCallbackAdapter that will allow callbacks to be stored in one single container.
 *
 * @tparam ManagerT The setting manager type that this callback will be added to.
 */
template <typename ManagerT>
class SettingCallbackAdapterInterface {
public:
    /**
     * Add callback to the given manager.
     *
     * @param manager Manager that has the setting to be observed.
     * @return @c true if it succeeds; @false otherwise.
     */
    virtual bool addToManager(ManagerT& manager) = 0;

    /**
     * Remove callback from the given manager.
     *
     * @param manager Manager that has the setting that was being observed.
     */
    virtual void removeFromManager(ManagerT& manager) = 0;
};

/**
 * A SettingCallbackAdapter can be used to register callback functions called when there is a setting notification.
 *
 * This is an example of how to use this adapter.
 *
 * @code
 *   MyClass(std::shared_ptr<DeviceSettingsManager> manager) : m_manager {manager} {
 *      m_adapter = SettingCallbackAdapter<DeviceSettingsManager, id>::create([](bool, SettingNotifications) {
 *          // do something.
 *      });
 *      m_adapter->addToManager(*m_manager);
 *   }
 *
 *   ~MyClass() {
 *      m_adapter->removeFromManager(*m_manager);
 *   }
 * @endcode
 *
 * @tparam ManagerT The setting manager type that this callback will be added to.
 * @tparam id The id of the setting inside the manager.
 */
template <typename ManagerT, size_t id>
class SettingCallbackAdapter
        : public SettingCallbackAdapterInterface<ManagerT>
        , public ManagerT::template ObserverType<id>
        , public std::enable_shared_from_this<SettingCallbackAdapter<ManagerT, id>> {
public:
    /// Setting value type.
    using ValueType = typename ManagerT::template ValueType<id>;

    /// Callback function value type.
    using FunctionType = std::function<void(const ValueType&, SettingNotifications)>;

    /**
     * Creates a SettingCallbackAdapter object.
     *
     * @param callback The function callback that should be called upon notification.
     */
    static std::shared_ptr<SettingCallbackAdapter> create(FunctionType callback);

    /**
     * The virtual destructor.
     */
    virtual ~SettingCallbackAdapter() = default;

    /// @name Functions from @c SettingCallbackAdapterInterface
    /// @{
    bool addToManager(ManagerT& manager) override;
    void removeFromManager(ManagerT& manager) override;
    /// @}

private:
    /**
     * Constructor.
     * @param callback The function callback that should be called upon notification.
     */
    SettingCallbackAdapter(FunctionType callback);

    /// @name SettingObserverInterface function
    /// @{
    virtual void onSettingNotification(const ValueType& value, SettingNotifications notification) override;
    /// @}

    /// Saves the callback method which will be used during the @c onChanged call.
    FunctionType m_callback;
};

template <typename ManagerT, size_t id>
void SettingCallbackAdapter<ManagerT, id>::onSettingNotification(
    const ValueType& value,
    alexaClientSDK::settings::SettingNotifications notification) {
    m_callback(value, notification);
}

template <typename ManagerT, size_t id>
SettingCallbackAdapter<ManagerT, id>::SettingCallbackAdapter(FunctionType callback) : m_callback{callback} {
}

template <typename ManagerT, size_t id>
bool SettingCallbackAdapter<ManagerT, id>::addToManager(ManagerT& manager) {
    /// The template keyword is required because addObserver definition depends on ManagerT.
    return manager.template addObserver<id>(this->shared_from_this());
};

template <typename ManagerT, size_t id>
void SettingCallbackAdapter<ManagerT, id>::removeFromManager(ManagerT& manager) {
    /// The template keyword is required because removeObserver definition depends on ManagerT.
    return manager.template removeObserver<id>(this->shared_from_this());
};

template <typename ManagerT, size_t id>
std::shared_ptr<SettingCallbackAdapter<ManagerT, id>> SettingCallbackAdapter<ManagerT, id>::create(
    SettingCallbackAdapter::FunctionType callback) {
    if (callback) {
        return std::shared_ptr<SettingCallbackAdapter<ManagerT, id>>(
            new SettingCallbackAdapter<ManagerT, id>(callback));
    }
    return nullptr;
}

}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGCALLBACKADAPTER_H_
