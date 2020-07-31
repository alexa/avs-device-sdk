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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_MIXINGBEHAVIOR_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_MIXINGBEHAVIOR_H_

#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

enum class MixingBehavior {
    /// Indicates that the corresponding Activity is the primary Activity on the AFML Channel
    PRIMARY,

    /// Indicates that the corresponding Activity may duck
    /// If ducking is not possible, the Activity must pause instead
    MAY_DUCK,

    /// Indicates that the corresponding Activity must pause
    MUST_PAUSE,

    /// Indicates that the corresponding Activity must stop
    MUST_STOP,

    /// Indicates that the corresponding Activity may adopt any one of the above behaviors
    UNDEFINED
};

/**
 * This function converts the provided @c MixingBehavior to a string.
 *
 * @param behavior The @c MixingBehavior to convert to a string.
 * @return The string conversion of @c behavior.
 */
inline std::string mixingBehaviorToString(MixingBehavior behavior) {
    switch (behavior) {
        case MixingBehavior::PRIMARY:
            return "PRIMARY";
        case MixingBehavior::MAY_DUCK:
            return "MAY_DUCK";
        case MixingBehavior::MUST_PAUSE:
            return "MUST_PAUSE";
        case MixingBehavior::MUST_STOP:
            return "MUST_STOP";
        case MixingBehavior::UNDEFINED:
            return "UNDEFINED";
    }
    return "UNDEFINED";
}

/**
 * This function reverse maps the provided string to corresponding MixingBehavior Implementation as specified by
 * mixingBehaviorToString
 * @param input string to convert to corresponding MixingBehavior
 * @return @c MixingBehavior that corresponds to the input string. In case of error
 * the API returns MixingBehavior::UNDEFINED
 */
inline MixingBehavior getMixingBehavior(const std::string& input) {
    MixingBehavior behavior = MixingBehavior::UNDEFINED;
    if (mixingBehaviorToString(MixingBehavior::PRIMARY) == input) {
        behavior = MixingBehavior::PRIMARY;
    } else if (mixingBehaviorToString(MixingBehavior::MAY_DUCK) == input) {
        behavior = MixingBehavior::MAY_DUCK;
    } else if (mixingBehaviorToString(MixingBehavior::MUST_PAUSE) == input) {
        behavior = MixingBehavior::MUST_PAUSE;
    } else if (mixingBehaviorToString(MixingBehavior::MUST_STOP) == input) {
        behavior = MixingBehavior::MUST_STOP;
    }

    return behavior;
}

/**
 * Write a @c MixingBehavior value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param behavior The @c MixingBehavior value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const MixingBehavior& behavior) {
    return stream << mixingBehaviorToString(behavior);
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  //  ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_MIXINGBEHAVIOR_H_
