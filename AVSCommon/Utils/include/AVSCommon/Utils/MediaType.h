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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIATYPE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIATYPE_H_

#include <iostream>
#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

/**
 * An enum class used to represent the file type of audio data.
 */
enum class MediaType {
    /// Mpeg type audio data.
    MPEG,
    /// Wav type audio data.
    WAV,
    /// Audio data type not known
    UNKNOWN
};

/**
 * Convert a string file type to @c MediaType value.
 *
 * @param mimetype The string representation of a file's mimetype.
 *
 * @return @c MediaType value of file
 */
inline MediaType MimeTypeToMediaType(const std::string& mimetype) {
    if (mimetype == "audio/mpeg" || mimetype == "audio/x-mpeg") {
        return MediaType::MPEG;
    } else if (mimetype == "audio/wav" || mimetype == "audio/x-wav") {
        return MediaType::WAV;
    }

    return MediaType::UNKNOWN;
}

/**
 * Write an @c MediaType value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param mediatype The @c MediaType value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const MediaType& mediatype) {
    switch (mediatype) {
        case MediaType::MPEG:
            stream << "MPEG";
            break;
        case MediaType::WAV:
            stream << "WAV";
            break;
        case MediaType::UNKNOWN:
        default:
            stream << "UNKNOWN";
            break;
    }
    return stream;
}

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIATYPE_H_
