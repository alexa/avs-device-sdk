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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_MEDIADESCRIPTION_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_MEDIADESCRIPTION_H_

#include <unordered_map>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace mediaPlayer {

/// Play behavior key to be included in the additional data.
static std::string PLAY_BEHAVIOR = "playBehavior";

/*
 * An object that contains all playback related information needed from the media CA.
 */
struct MediaDescription {
    /// All additional information to be provided, including PlayBehavior.
    std::unordered_map<std::string, std::string> additionalData;
};

/**
 * Builds an empty Media Description object.
 * @return an empty Media Description object.
 */
inline MediaDescription emptyMediaDescription() {
    return MediaDescription{{}};
}

/**
 * Write a @c MediaDescription value to the given stream.
 *
 * @param stream The stream to write the value to.
 * @param mediaDescription The value to write to the stream as a string.
 * @return The stream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const MediaDescription& mediaDescription) {
    stream << "AdditionalData:{";
    const auto additionalDataCopy = mediaDescription.additionalData;
    for (auto iter = additionalDataCopy.begin(); iter != additionalDataCopy.end(); iter++) {
        stream << "{" << (*iter).first;
        stream << ":" << (*iter).second << "}";
    }
    return stream;
}

}  // namespace mediaPlayer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_MEDIADESCRIPTION_H_
