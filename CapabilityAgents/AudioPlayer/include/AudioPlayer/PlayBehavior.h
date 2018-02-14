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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AUDIOPLAYER_INCLUDE_AUDIOPLAYER_PLAYBEHAVIOR_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AUDIOPLAYER_INCLUDE_AUDIOPLAYER_PLAYBEHAVIOR_H_

#include <ostream>

#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace audioPlayer {

/// Used to determine how a client must handle queueing and playback of a stream.
enum class PlayBehavior {
    /**
     * Immediately begin playback of the stream returned with the @c Play directive, and replace current and enqueued
     * streams.
     */
    REPLACE_ALL,

    /// Adds a stream to the end of the current queue.
    ENQUEUE,

    /// Replace all streams in the queue. This does not impact the currently playing stream.
    REPLACE_ENQUEUED
};

/**
 * Convert a @c PlayBehavior to an AVS-compliant @c std::string.
 *
 * @param playBehavior The @c PlayBehavior to convert.
 * @return The AVS-compliant string representation of @c playBehavior.
 */
inline std::string playBehaviorToString(PlayBehavior playBehavior) {
    switch (playBehavior) {
        case PlayBehavior::REPLACE_ALL:
            return "REPLACE_ALL";
        case PlayBehavior::ENQUEUE:
            return "ENQUEUE";
        case PlayBehavior::REPLACE_ENQUEUED:
            return "REPLACE_ENQUEUED";
    }
    return "unknown PlayBehavior";
}

/**
 * Convert an AVS-compliant @c string to a @c PlayBehavior.
 *
 * @param text The string to convert.
 * @param[out] playBehavior The converted @c PlayBehavior.
 * @return @c true if the string converted succesfully, else @c false.
 */
inline bool stringToPlayBehavior(const std::string& text, PlayBehavior* playBehavior) {
    if (nullptr == playBehavior) {
        return false;
    } else if (playBehaviorToString(PlayBehavior::REPLACE_ALL) == text) {
        *playBehavior = PlayBehavior::REPLACE_ALL;
        return true;
    } else if (playBehaviorToString(PlayBehavior::ENQUEUE) == text) {
        *playBehavior = PlayBehavior::ENQUEUE;
        return true;
    } else if (playBehaviorToString(PlayBehavior::REPLACE_ENQUEUED) == text) {
        *playBehavior = PlayBehavior::REPLACE_ENQUEUED;
        return true;
    }
    return false;
}

/**
 * Write a @c PlayBehavior value to an @c ostream.
 *
 * @param stream The stream to write the value to.
 * @param playBehavior The @c PlayBehavior value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const PlayBehavior& playBehavior) {
    return stream << playBehaviorToString(playBehavior);
}

/**
 * Convert a @c PlayBehavior to a @c rapidjson::Value.
 *
 * @param documentNode The @c rapidjson::Value to write to.
 * @param playBehavior The @c PlayBehavior to convert.
 * @return @c true if conversion is successful, else @c false.
 */
inline bool convertToValue(const rapidjson::Value& documentNode, PlayBehavior* playBehavior) {
    std::string text;
    if (!avsCommon::utils::json::jsonUtils::convertToValue(documentNode, &text)) {
        return false;
    }
    return stringToPlayBehavior(text, playBehavior);
}

}  // namespace audioPlayer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AUDIOPLAYER_INCLUDE_AUDIOPLAYER_PLAYBEHAVIOR_H_
