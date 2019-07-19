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
#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGOBSERVERINTERFACE_H_

#include <functional>
#include <memory>

namespace alexaClientSDK {
namespace settings {

/**
 * Enumerate the type of notifications.
 */
enum class SettingNotifications {
    /// Setting value changed due to a local change.
    LOCAL_CHANGE,
    /// Setting value changed due to a change requested via cloud.
    AVS_CHANGE,
    /// Local request failed.
    LOCAL_CHANGE_FAILED,
    /// AVS request failed.
    AVS_CHANGE_FAILED
};

/**
 * Base definition of SettingObserver.
 *
 * @tparam SettingT The setting type to be observed.
 */
template <typename SettingT>
class SettingObserverInterface {
public:
    /**
     * The virtual destructor.
     */
    virtual ~SettingObserverInterface() = default;

    /**
     * Function called when the observed setting is called.
     *
     * @param value The current value of the setting. For @c LOCAL_CHANGE and @c AVS_CHANGE, the value should be the
     * same as the one requested.
     * @param notification The notification type.
     */
    virtual void onSettingNotification(
        const typename SettingT::ValueType& value,
        SettingNotifications notification) = 0;

    /// Only objects from SettingT can call the value change notification.
    friend SettingT;
};

}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGOBSERVERINTERFACE_H_
