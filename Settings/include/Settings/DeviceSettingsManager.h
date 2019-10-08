/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_DEVICESETTINGSMANAGER_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_DEVICESETTINGSMANAGER_H_

#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include "Settings/SettingInterface.h"
#include "Settings/SettingsManager.h"
#include "Settings/SpeechConfirmationSettingType.h"
#include "Settings/WakeWordConfirmationSettingType.h"

namespace alexaClientSDK {
namespace settings {

/// Alias for the locale type.
using Locale = std::string;

/// Alias for locales enabled in the device.
using DeviceLocales = std::vector<Locale>;

/// Alias for the wake word type.
using WakeWord = std::string;

/// Alias for a set of wake words type.
using WakeWords = std::set<WakeWord>;

/// Type for do not disturb setting.
using DoNotDisturbSetting = SettingInterface<bool>;

/// Type for wake word confirmation setting.
using WakeWordConfirmationSetting = SettingInterface<WakeWordConfirmationSettingType>;

/// Type for end of speech confirmation setting.
using SpeechConfirmationSetting = SettingInterface<SpeechConfirmationSettingType>;

/// Type for time zone setting.
using TimeZoneSetting = SettingInterface<std::string>;

/// Type for wake words.
using WakeWordsSetting = SettingInterface<WakeWords>;

/// Type for locale.
using LocalesSetting = SettingInterface<DeviceLocales>;

/**
 * Enumerates the settings that are kept inside DeviceSettingsManager.
 *
 * @note This enumeration must reflect the order that the settings show up in the DeviceSettingsManager declaration.
 */
enum DeviceSettingsIndex { DO_NOT_DISTURB, WAKEWORD_CONFIRMATION, SPEECH_CONFIRMATION, TIMEZONE, WAKE_WORDS, LOCALE };

/// The DeviceSettingsManager will manage all common settings to alexa devices.
using DeviceSettingsManager = SettingsManager<
    DoNotDisturbSetting,
    WakeWordConfirmationSetting,
    SpeechConfirmationSetting,
    TimeZoneSetting,
    WakeWordsSetting,
    LocalesSetting>;

}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_DEVICESETTINGSMANAGER_H_
