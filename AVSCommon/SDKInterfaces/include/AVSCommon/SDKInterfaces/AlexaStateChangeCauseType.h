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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ALEXASTATECHANGECAUSETYPE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ALEXASTATECHANGECAUSETYPE_H_

#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * Enumeration used to identify the cause of a property value change.
 */
enum class AlexaStateChangeCauseType {
    // Value changed due to an unknown Alexa interaction.
    ALEXA_INTERACTION,
    // Customer interaction with the application.
    APP_INTERACTION,
    // Customer manual or physical interaction with this device.
    PHYSICAL_INTERACTION,
    // Value change detected during periodic poll.
    PERIODIC_POLL,
    // Value changed due to the rule trigger in application/device.
    RULE_TRIGGER,
    // Value changed due to voice interaction.
    VOICE_INTERACTION
};

/**
 * This function converts the provided @c AlexaStateChangeCauseType to a string.
 *
 * @param cause The @c AlexaStateChangeCauseType to convert to a string.
 * @return The string conversion of the @c AlexaStateChangeCauseType.
 */
inline std::string alexaStateChangeCauseTypeToString(AlexaStateChangeCauseType cause) {
    switch (cause) {
        case AlexaStateChangeCauseType::ALEXA_INTERACTION:
            return "ALEXA_INTERACTION";
        case AlexaStateChangeCauseType::APP_INTERACTION:
            return "APP_INTERACTION";
        case AlexaStateChangeCauseType::PHYSICAL_INTERACTION:
            return "PHYSICAL_INTERACTION";
        case AlexaStateChangeCauseType::PERIODIC_POLL:
            return "PERIODIC_POLL";
        case AlexaStateChangeCauseType::RULE_TRIGGER:
            return "RULE_TRIGGER";
        case AlexaStateChangeCauseType::VOICE_INTERACTION:
            return "VOICE_INTERACTION";
    }
    return "UNKNOWN";
}

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ALEXASTATECHANGECAUSETYPE_H_
