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

#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_WAKEWORDCONFIRMATIONSETTINGTYPE_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_WAKEWORDCONFIRMATIONSETTINGTYPE_H_

#include <string>
#include <ostream>
#include <istream>

namespace alexaClientSDK {
namespace settings {

/**
 * Defines the values for wake word confirmation setting.
 */
enum class WakeWordConfirmationSettingType {
    /// No wake word confirmation.
    NONE,

    /// A tone will be played for wake word confirmation.
    TONE
};

/**
 * Retrieves the default value of wake word confirmation
 *
 * @return The default value wake word confirmations setting.
 */
inline WakeWordConfirmationSettingType getWakeWordConfirmationDefault() {
    return WakeWordConfirmationSettingType::NONE;
}

/**
 * Write a @c WakeWordConfirmationSettingType value to the given stream.
 *
 * @param stream The stream to write the value to.
 * @param value The value to write to the stream as a string.
 * @return The stream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const WakeWordConfirmationSettingType& value) {
    switch (value) {
        case WakeWordConfirmationSettingType::NONE:
            stream << "NONE";
            return stream;
        case WakeWordConfirmationSettingType::TONE:
            stream << "TONE";
            return stream;
    }

    stream.setstate(std::ios_base::failbit);

    return stream;
}

/**
 * Converts an input string stream value to WakeWordConfirmationSettingType.
 *
 * @param stream The string stream to retrieve the value from.
 * @param [out] value The value to write to.
 * @return The stream that was passed in.
 */
inline std::istream& operator>>(std::istream& is, WakeWordConfirmationSettingType& value) {
    std::string str;
    is >> str;
    if ("NONE" == str) {
        value = WakeWordConfirmationSettingType::NONE;
    } else if ("TONE" == str) {
        value = WakeWordConfirmationSettingType::TONE;
    } else {
        is.setstate(std::ios_base::failbit);
    }
    return is;
}

}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_WAKEWORDCONFIRMATIONSETTINGTYPE_H_
