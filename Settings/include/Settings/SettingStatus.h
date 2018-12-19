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

#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGSTATUS_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGSTATUS_H_

namespace alexaClientSDK {
namespace settings {

/**
 * Specifies the state of a setting.
 */
enum class SettingStatus {
    /// The setting has either not been set and not been persisted in a storage or the setting is in an unknown status.
    NOT_AVAILABLE,

    /// The setting has been set locally but not synchronized with AVS.
    LOCAL_CHANGE_IN_PROGRESS,

    /// AVS request is in progress.
    AVS_CHANGE_IN_PROGRESS,

    /// The setting has been set locally and synchronized with AVS.
    SYNCHRONIZED
};

/**
 * This function converts the provided @c SettingStatus to a string.
 *
 * @param status The @c SettingStatus to convert to a string.
 * @return The string conversion of the @c SettingStatus.
 */
inline std::string settingStatusToString(SettingStatus status) {
    switch (status) {
        case SettingStatus::LOCAL_CHANGE_IN_PROGRESS:
            return "LOCAL_CHANGE_IN_PROGRESS";
        case SettingStatus::AVS_CHANGE_IN_PROGRESS:
            return "AVS_CHANGE_IN_PROGRESS";
        case SettingStatus::SYNCHRONIZED:
            return "SYNCHRONIZED";
        case SettingStatus::NOT_AVAILABLE:
            return "NOT_AVAILABLE";
    }
    return "UNKNOWN";
}

/**
 * This function parses a string and converts it to its corresponding @c SettingStatus.
 *
 * @param statusString The string to be converted to @c SettingStatus.
 * @return The @c SettingStatus corresponding to the string provided.
 */
inline SettingStatus stringToSettingStatus(const std::string& statusString) {
    if ("LOCAL_CHANGE_IN_PROGRESS" == statusString) {
        return SettingStatus::LOCAL_CHANGE_IN_PROGRESS;
    } else if ("SYNCHRONIZED" == statusString) {
        return SettingStatus::SYNCHRONIZED;
    } else if ("AVS_CHANGE_IN_PROGRESS" == statusString) {
        return SettingStatus::AVS_CHANGE_IN_PROGRESS;
    }
    return SettingStatus::NOT_AVAILABLE;
}

}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGSTATUS_H_
