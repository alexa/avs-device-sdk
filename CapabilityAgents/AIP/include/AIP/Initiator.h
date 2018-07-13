/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AIP_INCLUDE_AIP_INITIATOR_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AIP_INCLUDE_AIP_INITIATOR_H_

namespace alexaClientSDK {
namespace capabilityAgents {
namespace aip {

#include <string>

/**
 * Enumerates the different initiators supported by AVS.
 */
enum class Initiator {
    /// Recognize event was initiated by a press-and-hold action.
    PRESS_AND_HOLD,
    /// Recognize event was initiated by a tap-to-talk action.
    TAP,
    /// Recognize event was initiated by a wakeword action.
    WAKEWORD
};

/**
 * Convert an @c Initiator value to an AVS-compliant string.
 *
 * @param initiator The initiator value to convert to a string.
 * @return The @c std::string conversion of the @c Initiator value.
 */
inline std::string initiatorToString(Initiator initiator) {
    switch (initiator) {
        case Initiator::PRESS_AND_HOLD:
            return "PRESS_AND_HOLD";
        case Initiator::TAP:
            return "TAP";
        case Initiator::WAKEWORD:
            return "WAKEWORD";
    }
    return "Unknown Inititator";
}

}  // namespace aip
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AIP_INCLUDE_AIP_INITIATOR_H_
