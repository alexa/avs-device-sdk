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

#ifndef ACSDK_APLCAPABILITYCOMMONINTERFACES_APLTIMEOUTTYPE_H_
#define ACSDK_APLCAPABILITYCOMMONINTERFACES_APLTIMEOUTTYPE_H_

#include <string>

#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace aplCapabilityCommonInterfaces {

/**
 * Strongly-typed timeoutType as defined in the API specification
 * https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/presentation-apl.html#renderdocument
 */
enum class APLTimeoutType { SHORT, TRANSIENT, LONG };

static inline alexaClientSDK::avsCommon::utils::Optional<APLTimeoutType> convertToTimeoutType(
    const std::string& timeoutTypeStr) {
    alexaClientSDK::avsCommon::utils::Optional<APLTimeoutType> timeoutType;

    if (timeoutTypeStr == "SHORT") {
        timeoutType.set(APLTimeoutType::SHORT);
    } else if (timeoutTypeStr == "LONG") {
        timeoutType.set(APLTimeoutType::LONG);
    } else if (timeoutTypeStr == "TRANSIENT") {
        timeoutType.set(APLTimeoutType::TRANSIENT);
    }

    return timeoutType;
};

}  // namespace aplCapabilityCommonInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_APLCAPABILITYCOMMONINTERFACES_APLTIMEOUTTYPE_H_