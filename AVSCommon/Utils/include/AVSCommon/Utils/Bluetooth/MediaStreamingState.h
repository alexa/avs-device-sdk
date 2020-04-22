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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_MEDIASTREAMINGSTATE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_MEDIASTREAMINGSTATE_H_

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace bluetooth {

/// An Enum representing the current state of the stream.
enum class MediaStreamingState {
    /// Currently not streaming.
    IDLE,
    /// Currently acquiring the stream.
    PENDING,
    /// Currently streaming.
    ACTIVE
};

/**
 * Converts the @c MediaStreamingState enum to a string.
 *
 * @param The @c MediaStreamingState to convert.
 * @return A string representation of the @c MediaStreamingState.
 */
inline std::string mediaStreamingStateToString(MediaStreamingState value) {
    switch (value) {
        case MediaStreamingState::IDLE:
            return "IDLE";
        case MediaStreamingState::PENDING:
            return "PENDING";
        case MediaStreamingState::ACTIVE:
            return "ACTIVE";
        default:
            return "UNKNOWN";
    }
}

/**
 * Overload for the @c MediaStreamingState enum. This will write the @c MediaStreamingState as a string to the
 * provided stream.
 *
 * @param An ostream to send the @c MediaStreamingState as a string.
 * @param The @c MediaStreamingState to convert.
 * @return The stream.
 */
inline std::ostream& operator<<(std::ostream& stream, const MediaStreamingState state) {
    return stream << mediaStreamingStateToString(state);
}

}  // namespace bluetooth
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_MEDIASTREAMINGSTATE_H_
