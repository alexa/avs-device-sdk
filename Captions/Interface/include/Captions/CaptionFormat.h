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

#ifndef ALEXA_CLIENT_SDK_CAPTIONS_INTERFACE_INCLUDE_CAPTIONS_CAPTIONFORMAT_H_
#define ALEXA_CLIENT_SDK_CAPTIONS_INTERFACE_INCLUDE_CAPTIONS_CAPTIONFORMAT_H_

#include <ostream>

#include <AVSCommon/Utils/Logger/LoggerUtils.h>

namespace alexaClientSDK {
namespace captions {

/**
 * An enumeration of caption formats supported by the SDK.
 */
enum class CaptionFormat {
    /// WebVTT formatted plain text, see https://www.w3.org/TR/webvtt1/
    WEBVTT,

    /// Unknown or unsupported format.
    UNKNOWN
};

/**
 * Convert an AVS-compliant @c string to a @c CaptionFormat.
 *
 * @param text The AVS-compliant @c string to convert.
 * @return The converted @c CaptionFormat if a match is found, otherwise @c CaptionFormat::UNKNOWN.
 */
inline CaptionFormat avsStringToCaptionFormat(const std::string& text) {
    if (text == "WEBVTT") {
        return CaptionFormat::WEBVTT;
    }
    avsCommon::utils::logger::acsdkWarn(
        avsCommon::utils::logger::LogEntry(__func__, "unknownCaptionFormat").d("formatValue", text));
    return CaptionFormat::UNKNOWN;
}

/**
 * Write a @c CaptionFormat value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param format The @c CaptionFormat value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const CaptionFormat& format) {
    switch (format) {
        case CaptionFormat::WEBVTT:
            stream << "WEBVTT";
            break;
        case CaptionFormat::UNKNOWN:
            stream << "UNKNOWN";
            break;
    }
    return stream;
}

}  // namespace captions
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPTIONS_INTERFACE_INCLUDE_CAPTIONS_CAPTIONFORMAT_H_