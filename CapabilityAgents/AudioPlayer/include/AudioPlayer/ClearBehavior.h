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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AUDIOPLAYER_INCLUDE_AUDIOPLAYER_CLEARBEHAVIOR_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AUDIOPLAYER_INCLUDE_AUDIOPLAYER_CLEARBEHAVIOR_H_

#include <ostream>

#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace audioPlayer {

/// Used to determine clear queue behavior.
enum class ClearBehavior {
    /// Clears the queue and continues to play the currently playing stream.
    CLEAR_ENQUEUED,

    /// Clears the entire playback queue and stops the currently playing stream (if applicable).
    CLEAR_ALL
};

/**
 * Convert a @c ClearBehavior to an AVS-compliant @c std::string.
 *
 * @param clearBehavior The @c ClearBehavior to convert.
 * @return The AVS-compliant string representation of @c clearBehavior.
 */
inline std::string clearBehaviorToString(ClearBehavior clearBehavior) {
    switch (clearBehavior) {
        case ClearBehavior::CLEAR_ENQUEUED:
            return "CLEAR_ENQUEUED";
        case ClearBehavior::CLEAR_ALL:
            return "CLEAR_ALL";
    }
    return "unknown ClearBehavior";
}

/**
 * Convert an AVS-compliant @c string to a @c ClearBehavior.
 *
 * @param text The string to convert.
 * @param[out] clearBehavior The converted @c ClearBehavior.
 * @return @c true if the string converted succesfully, else @c false.
 */
inline bool stringToClearBehavior(const std::string& text, ClearBehavior* clearBehavior) {
    if (nullptr == clearBehavior) {
        return false;
    } else if (clearBehaviorToString(ClearBehavior::CLEAR_ENQUEUED) == text) {
        *clearBehavior = ClearBehavior::CLEAR_ENQUEUED;
        return true;
    } else if (clearBehaviorToString(ClearBehavior::CLEAR_ALL) == text) {
        *clearBehavior = ClearBehavior::CLEAR_ALL;
        return true;
    }
    return false;
}

/**
 * Write a @c ClearBehavior value to an @c ostream.
 *
 * @param stream The stream to write the value to.
 * @param clearBehavior The @c ClearBehavior value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const ClearBehavior& clearBehavior) {
    return stream << clearBehaviorToString(clearBehavior);
}

/**
 * Convert a @c ClearBehavior to a @c rapidjson::Value.
 *
 * @param documentNode The @c rapidjson::Value to write to.
 * @param clearBehavior The @c ClearBehavior to convert.
 * @return @c true if conversion is successful, else @c false.
 */
inline bool convertToValue(const rapidjson::Value& documentNode, ClearBehavior* clearBehavior) {
    std::string text;
    if (!avsCommon::utils::json::jsonUtils::convertToValue(documentNode, &text)) {
        return false;
    }
    return stringToClearBehavior(text, clearBehavior);
}

}  // namespace audioPlayer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AUDIOPLAYER_INCLUDE_AUDIOPLAYER_CLEARBEHAVIOR_H_
