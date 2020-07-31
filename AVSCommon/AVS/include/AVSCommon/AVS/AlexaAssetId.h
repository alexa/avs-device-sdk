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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ALEXAASSETID_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ALEXAASSETID_H_

#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace resources {

/// This represents the Alexa asset identifier.
using AlexaAssetId = std::string;

/**
 * String constants for the asset identifier.
 * @see https://developer.amazon.com/docs/alexa/device-apis/resources-and-assets.html#global-alexa-catalog
 */

/// Asset identifier for device with friendly name "Shower".
static const AlexaAssetId ASSET_ALEXA_DEVICENAME_SHOWER = "Alexa.DeviceName.Shower";

/// Asset identifier for the supported Alexa friendly names synonymous with "Washer".
static const AlexaAssetId ASSET_ALEXA_DEVICENAME_WASHER = "Alexa.DeviceName.Washer";

/// Asset identifier for the supported Alexa friendly names synonymous with "Router".
static const AlexaAssetId ASSET_ALEXA_DEVICENAME_ROUTER = "Alexa.DeviceName.Router";

///  Asset identifier for the supported Alexa friendly names synonymous with "Fan".
static const AlexaAssetId ASSET_ALEXA_DEVICENAME_FAN = "Alexa.DeviceName.Fan";

/// Asset identifier for the supported Alexa friendly names synonymous with "Air Purifier".
static const AlexaAssetId ASSET_ALEXA_DEVICENAME_AIRPURIFIER = "Alexa.DeviceName.AirPurifier";

/// Asset identifier for the supported Alexa friendly names synonymous with "Space Heater".
static const AlexaAssetId ASSET_ALEXA_DEVICENAME_SPACEHEATER = "Alexa.DeviceName.SpaceHeater";

/// Asset identifier for the supported Alexa friendly names synonymous with "Rain Head".
static const AlexaAssetId ASSET_ALEXA_SHOWER_RAINHEAD = "Alexa.Shower.RainHead";

/// Asset identifier for the supported Alexa friendly names synonymous with "Handheld Shower".
static const AlexaAssetId ASSET_ALEXA_SHOWER_HANDHELD = "Alexa.Shower.HandHeld";

/// Asset identifier for the supported Alexa friendly names synonymous with "Water Temperature".
static const AlexaAssetId ASSET_ALEXA_SETTING_WATERTEMPERATURE = "Alexa.Setting.WaterTemperature";

/// Asset identifier for the supported Alexa friendly names synonymous with "Temperature".
static const AlexaAssetId ASSET_ALEXA_SETTING_TEMPERATURE = "Alexa.Setting.Temperature";

/// Asset identifier for the supported Alexa friendly names synonymous with "Wash Cycle".
static const AlexaAssetId ASSET_ALEXA_SETTING_WASHCYCLE = "Alexa.Setting.WashCycle";

/// Asset identifier for the supported Alexa friendly names synonymous with "2.4G Guest Wi-Fi".
static const AlexaAssetId ASSET_ALEXA_SETTING_2GGUESTWIFI = "Alexa.Setting.2GGuestWiFi";

/// Asset identifier for the supported Alexa friendly names synonymous with "5G Guest Wi-Fi".
static const AlexaAssetId ASSET_ALEXA_SETTING_5GGUESTWIFI = "Alexa.Setting.5GGuestWiFi";

/// Asset identifier for the supported Alexa friendly names synonymous with "Guest Wi-fi".
static const AlexaAssetId ASSET_ALEXA_SETTING_GUESTWIFI = "Alexa.Setting.GuestWiFi";

/// Asset identifier for the supported Alexa friendly names synonymous with "Auto" or "Automatic".
static const AlexaAssetId ASSET_ALEXA_SETTING_AUTO = "Alexa.Setting.Auto";

/// Asset identifier for the supported Alexa friendly names synonymous with "Night".
static const AlexaAssetId ASSET_ALEXA_SETTING_NIGHT = "Alexa.Setting.Night";

/// Asset identifier for the supported Alexa friendly names synonymous with "Quiet".
static const AlexaAssetId ASSET_ALEXA_SETTING_QUIET = "Alexa.Setting.Quiet";

/// Asset identifier for the supported Alexa friendly names synonymous with "Oscillate".
static const AlexaAssetId ASSET_ALEXA_SETTING_OSCILLATE = "Alexa.Setting.Oscillate";

/// Asset identifier for the supported Alexa friendly names synonymous with "Fan Speed".
static const AlexaAssetId ASSET_ALEXA_SETTING_FANSPEED = "Alexa.Setting.FanSpeed";

/// Asset identifier for the supported Alexa friendly names synonymous with "Preset".
static const AlexaAssetId ASSET_ALEXA_SETTING_PRESET = "Alexa.Setting.Preset";

/// Asset identifier for the supported Alexa friendly names synonymous with "Mode".
static const AlexaAssetId ASSET_ALEXA_SETTING_MODE = "Alexa.Setting.Mode";

/// Asset identifier for the supported Alexa friendly names synonymous with "Direction".
static const AlexaAssetId ASSET_ALEXA_SETTING_DIRECTION = "Alexa.Setting.Direction";

/// Asset identifier for the supported Alexa friendly names synonymous with "Delicates".
static const AlexaAssetId ASSET_ALEXA_VALUE_DELICATE = "Alexa.Value.Delicate";

/// Asset identifier for the supported Alexa friendly names synonymous with "Quick Wash".
static const AlexaAssetId ASSET_ALEXA_VALUE_QUICKWASH = "Alexa.Value.QuickWash";

/// Asset identifier for the supported Alexa friendly names synonymous with "Maximum".
static const AlexaAssetId ASSET_ALEXA_VALUE_MAXIMUM = "Alexa.Value.Maximum";

/// Asset identifier for the supported Alexa friendly names synonymous with "Minimum".
static const AlexaAssetId ASSET_ALEXA_VALUE_MINIMUM = "Alexa.Value.Minimum";

/// Asset identifier for the supported Alexa friendly names synonymous with "High".
static const AlexaAssetId ASSET_ALEXA_VALUE_HIGH = "Alexa.Value.High";

/// Asset identifier for the supported Alexa friendly names synonymous with "Low".
static const AlexaAssetId ASSET_ALEXA_VALUE_LOW = "Alexa.Value.Low";

/// Asset identifier for the supported Alexa friendly names synonymous with "Medium".
static const AlexaAssetId ASSET_ALEXA_VALUE_MEDIUM = "Alexa.Value.Medium";

}  // namespace resources

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ALEXAASSETID_H_
