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
#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGOBSERVERINTERFACE_H_

#include <functional>
#include <memory>
#include <ostream>

namespace alexaClientSDK {
namespace settings {

/**
 * Enumerate the type of notifications.
 */
enum class SettingNotifications {
    /// The setting that was changed locally is being applied.
    LOCAL_CHANGE_IN_PROGRESS,
    /// The setting that was changed via cloud is being applied.
    AVS_CHANGE_IN_PROGRESS,
    /// Setting value changed due to a local change.
    LOCAL_CHANGE,
    /// Setting value changed due to a change requested via cloud.
    AVS_CHANGE,
    /// Local request failed.
    LOCAL_CHANGE_FAILED,
    /// AVS request failed.
    AVS_CHANGE_FAILED,
    /// Local request cancelled due to a new request.
    LOCAL_CHANGE_CANCELLED,
    /// AVS request cancelled due to a new request.
    AVS_CHANGE_CANCELLED
};

/**
 * Write a @c SettingNotifications value to the given stream.
 *
 * @param stream The stream to write the value to.
 * @param value The value to write to the stream as a string.
 * @return The stream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const SettingNotifications& value) {
    switch (value) {
        case SettingNotifications::LOCAL_CHANGE_IN_PROGRESS:
            stream << "LOCAL_CHANGE_IN_PROGRESS";
            return stream;
        case SettingNotifications::AVS_CHANGE_IN_PROGRESS:
            stream << "AVS_CHANGE_IN_PROGRESS";
            return stream;
        case SettingNotifications::LOCAL_CHANGE:
            stream << "LOCAL_CHANGE";
            return stream;
        case SettingNotifications::AVS_CHANGE:
            stream << "AVS_CHANGE";
            return stream;
        case SettingNotifications::LOCAL_CHANGE_FAILED:
            stream << "LOCAL_CHANGE_FAILED";
            return stream;
        case SettingNotifications::AVS_CHANGE_FAILED:
            stream << "AVS_CHANGE_FAILED";
            return stream;
        case SettingNotifications::LOCAL_CHANGE_CANCELLED:
            stream << "LOCAL_CHANGE_CANCELLED";
            return stream;
        case SettingNotifications::AVS_CHANGE_CANCELLED:
            stream << "AVS_CHANGE_CANCELLED";
            return stream;
    }

    stream.setstate(std::ios_base::failbit);
    return stream;
}

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
