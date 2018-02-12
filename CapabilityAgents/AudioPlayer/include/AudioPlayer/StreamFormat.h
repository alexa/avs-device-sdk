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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AUDIOPLAYER_INCLUDE_AUDIOPLAYER_STREAMFORMAT_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AUDIOPLAYER_INCLUDE_AUDIOPLAYER_STREAMFORMAT_H_

#include <ostream>

#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace audioPlayer {

/// Specifies the format of a binary audio attachment in a @c Play directive.
enum class StreamFormat {
    /// Audio is in mpeg format.
    AUDIO_MPEG,
    /// Audio is an unknown/unsupported format.
    UNKNOWN
};

/**
 * Convert a @c StreamFormat to an AVS-compliant @c std::string.
 *
 * @param streamFormat The @c StreamFormat to convert.
 * @return The AVS-compliant string representation of @c streamFormat.
 */
inline std::string streamFormatToString(StreamFormat streamFormat) {
    switch (streamFormat) {
        case StreamFormat::AUDIO_MPEG:
            return "AUDIO_MPEG";
        case StreamFormat::UNKNOWN:
            break;
    }
    return "unknown StreamFormat";
}

/**
 * Convert an AVS-compliant @c string to a @c StreamFormat.
 *
 * @param text The string to convert.
 * @param[out] streamFormat The converted @c StreamFormat.
 * @return @c true if the string converted succesfully, else @c false.
 */
inline bool stringToStreamFormat(const std::string& text, StreamFormat* streamFormat) {
    if (nullptr == streamFormat) {
        return false;
    } else if (text == streamFormatToString(StreamFormat::AUDIO_MPEG)) {
        *streamFormat = StreamFormat::AUDIO_MPEG;
        return true;
    }
    return false;
}

/**
 * Write a @c StreamFormat value to an @c ostream.
 *
 * @param stream The stream to write the value to.
 * @param streamFormat The @c StreamFormat value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const StreamFormat& streamFormat) {
    return stream << streamFormatToString(streamFormat);
}

/**
 * Convert a @c StreamFormat to a @c rapidjson::Value.
 *
 * @param documentNode The @c rapidjson::Value to write to.
 * @param streamFormat The @c StreamFormat to convert.
 * @return @c true if conversion is successful, else @c false.
 */
inline bool convertToValue(const rapidjson::Value& documentNode, StreamFormat* streamFormat) {
    std::string text;
    if (!avsCommon::utils::json::jsonUtils::convertToValue(documentNode, &text)) {
        return false;
    }
    return stringToStreamFormat(text, streamFormat);
}

}  // namespace audioPlayer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AUDIOPLAYER_INCLUDE_AUDIOPLAYER_STREAMFORMAT_H_
