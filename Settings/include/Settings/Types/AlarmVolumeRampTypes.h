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

#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_TYPES_ALARMVOLUMERAMPTYPES_H__
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_TYPES_ALARMVOLUMERAMPTYPES_H__

#include <istream>
#include <ostream>
#include <string>

namespace alexaClientSDK {
namespace settings {
namespace types {

/**
 * Defines the values for alarm volume ramp setting.
 */
enum class AlarmVolumeRampTypes {
    /// Alarm should start with final volume.
    NONE,

    /// Alarm will include a fade-in period.
    ASCENDING
};

/**
 * Retrieves the default value of alarm volume ramp.
 *
 * @return The default value ramp.
 */
constexpr AlarmVolumeRampTypes getAlarmVolumeRampDefault() {
    return AlarmVolumeRampTypes::NONE;
}

/**
 * Convert @c AlarmVolumeRampTypes to boolean logic since the UI experience is enable / disable.
 *
 * @param volumeRamp The setting value.
 * @return Whether the setting value is enabled (ASCENDING) or not (NONE).
 */
constexpr bool isEnabled(AlarmVolumeRampTypes volumeRamp) {
    return volumeRamp != AlarmVolumeRampTypes::NONE;
}

/**
 * Convert boolean to @c AlarmVolumeRampTypes.
 *
 * @param enabled Whether fade-in is enabled or not.
 * @return The corresponding setting value.
 */
constexpr AlarmVolumeRampTypes toAlarmRamp(bool enabled) {
    return enabled ? AlarmVolumeRampTypes::ASCENDING : AlarmVolumeRampTypes::NONE;
}

/**
 * Write a @c AlarmVolumeRampTypes value to the given stream.
 *
 * @param stream The stream to write the value to.
 * @param value The value to write to the stream as a string.
 * @return The stream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const AlarmVolumeRampTypes& value) {
    switch (value) {
        case AlarmVolumeRampTypes::NONE:
            stream << "NONE";
            return stream;
        case AlarmVolumeRampTypes::ASCENDING:
            stream << "ASCENDING";
            return stream;
    }

    stream.setstate(std::ios_base::failbit);
    return stream;
}

/**
 * Converts an input string stream value to AlarmVolumeRampTypes.
 *
 * @param stream The string stream to retrieve the value from.
 * @param [out] value The value to write to.
 * @return The stream that was passed in.
 */
inline std::istream& operator>>(std::istream& is, AlarmVolumeRampTypes& value) {
    std::string str;
    is >> str;
    if ("NONE" == str) {
        value = AlarmVolumeRampTypes::NONE;
    } else if ("ASCENDING" == str) {
        value = AlarmVolumeRampTypes::ASCENDING;
    } else {
        is.setstate(std::ios_base::failbit);
    }
    return is;
}

}  // namespace types
}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_TYPES_ALARMVOLUMERAMPTYPES_H__
