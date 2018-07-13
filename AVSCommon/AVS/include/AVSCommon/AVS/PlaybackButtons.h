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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_PLAYBACKBUTTONS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_PLAYBACKBUTTONS_H_

#include <iostream>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/// Enumeration class for supported playback buttons.
enum class PlaybackButton {
    /// Playback Controller 'Play' button.
    PLAY,

    /// Playback Controller 'Pause' button.
    PAUSE,

    /// Playback Controller 'Next' button.
    NEXT,

    /// Playback Controller 'Previous' button.
    PREVIOUS
};

/**
 * Convert a @c PlaybackButton to  @c std::string.
 *
 * @param button The @c PlaybackButton to convert.
 * @return The representation of @c button.
 */
inline std::string PlaybackButtonToString(PlaybackButton button) {
    switch (button) {
        case PlaybackButton::PLAY:
            return "Play";
        case PlaybackButton::PAUSE:
            return "Pause";
        case PlaybackButton::PREVIOUS:
            return "Previous";
        case PlaybackButton::NEXT:
            return "Next";
    }

    return "unknown playbackButton";
}

/**
 * A macro to be used in operator << overload function to help with the translation
 * of @c Button to string.
 *
 * @param button The @c Button that needs to be translated.
 */
#define ACSDK_PLAYBACK_BUTTON_TO_STRING(button) \
    case PlaybackButton::button: {              \
        return stream << #button;               \
    }

/**
 * Write a @c Button value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param button The @c Button value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const PlaybackButton& button) {
    switch (button) {
        ACSDK_PLAYBACK_BUTTON_TO_STRING(PLAY);
        ACSDK_PLAYBACK_BUTTON_TO_STRING(PAUSE);
        ACSDK_PLAYBACK_BUTTON_TO_STRING(NEXT);
        ACSDK_PLAYBACK_BUTTON_TO_STRING(PREVIOUS);
    }

    return stream;
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

namespace std {
/**
 * @ std::hash() specialization defined to allow @c PlaybackButton to be used as a key in @c std::unordered_map.
 */
template <>
struct hash<alexaClientSDK::avsCommon::avs::PlaybackButton> {
    inline size_t operator()(const alexaClientSDK::avsCommon::avs::PlaybackButton& button) const {
        return std::hash<std::string>{}(PlaybackButtonToString(button));
    }
};
}  // namespace std

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_PLAYBACKBUTTONS_H_
