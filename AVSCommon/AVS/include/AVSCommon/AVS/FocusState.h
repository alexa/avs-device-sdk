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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_FOCUSSTATE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_FOCUSSTATE_H_

#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/**
 * An enum class used to specify the levels of focus that a Channel can have.
 */
enum class FocusState {
    /// Represents the highest focus a Channel can have.
    FOREGROUND,

    /// Represents the intermediate level focus a Channel can have.
    BACKGROUND,

    /// This focus is used to represent when a Channel is not being used or when an observer should stop.
    NONE
};

/**
 * This function converts the provided @c FocusState to a string.
 *
 * @param state The @c FocusState to convert to a string.
 * @return The string conversion of @c state.
 */
inline std::string focusStateToString(FocusState state) {
    switch (state) {
        case FocusState::FOREGROUND:
            return "FOREGROUND";
        case FocusState::BACKGROUND:
            return "BACKGROUND";
        case FocusState::NONE:
            return "NONE";
    }
    return "Unknown State";
}

/**
 * Write a @c FocusState value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param state The @c FocusState value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const FocusState& state) {
    return stream << focusStateToString(state);
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_FOCUSSTATE_H_
