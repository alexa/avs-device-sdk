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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_NOTIFICATIONS_INCLUDE_NOTIFICATIONS_INDICATORSTATE_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_NOTIFICATIONS_INCLUDE_NOTIFICATIONS_INDICATORSTATE_H_

#include <ostream>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace notifications {

/**
 * An enum class used to represent the state of the notification indicator.
 *
 * @note These values shouldn't be changed before evaluating the effect a change will have on existing databases.
 */
enum class IndicatorState { OFF = 0, ON = 1, UNDEFINED };

/**
 * Convert an IndicatorState to an int.
 *
 * @param state The @c IndicatorState to convert.
 * @return The int representation of state
 */
inline int indicatorStateToInt(IndicatorState state) {
    return static_cast<int>(state);
}

/**
 * Convert an int into an @c IndicatorState.
 *
 * @param stateNum The int to convert.
 * @return The IndicatorState representation of stateNum or nullptr if stateNum is invalid.
 */
inline const IndicatorState intToIndicatorState(int stateNum) {
    if (stateNum < 0 || stateNum >= static_cast<int>(IndicatorState::UNDEFINED)) {
        return IndicatorState::UNDEFINED;
    }
    return static_cast<IndicatorState>(stateNum);
}

inline std::ostream& operator<<(std::ostream& stream, IndicatorState state) {
    switch (state) {
        case IndicatorState::ON:
            stream << "ON";
            return stream;
        case IndicatorState::OFF:
            stream << "OFF";
            return stream;
        case IndicatorState::UNDEFINED:
            stream << "UNDEFINED";
            return stream;
    }
    return stream << "UNKNOWN STATE";
}

}  // namespace notifications
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_NOTIFICATIONS_INCLUDE_NOTIFICATIONS_INDICATORSTATE_H_
