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
#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_DEVICESETTINGSMANAGER_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_DEVICESETTINGSMANAGER_H_

#include <string>

#include "Settings/SettingInterface.h"
#include "Settings/SettingsManager.h"

namespace alexaClientSDK {
namespace settings {

/// Type for do not disturb setting.
using DoNotDisturb = SettingInterface<bool>;

/**
 * Enumerates the settings that are kept inside DeviceSettingsManager.
 *
 * @note This enumeration must reflect the order that the settings show up in the DeviceSettingsManager declaration.
 */
enum DeviceSettingsIndex { DO_NOT_DISTURB };

/// The DeviceSettingsManager will manage all common settings to alexa devices.
using DeviceSettingsManager = SettingsManager<DoNotDisturb>;

}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_DEVICESETTINGSMANAGER_H_
