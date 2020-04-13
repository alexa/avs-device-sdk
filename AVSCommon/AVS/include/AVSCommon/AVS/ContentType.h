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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CONTENTTYPE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CONTENTTYPE_H_

#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

enum class ContentType {
    /// Indicates that the corresponding Activity is mixable with other channels
    /// Such Activities may duck upon receiving FocusState::BACKGROUND focus
    MIXABLE,

    /// Indicates that the corresponding Activity is not mixable with other channels
    /// Such Activities must pause upon receiving FocusState::BACKGROUND focus
    NONMIXABLE,

    /// Indicates that the corresponding ContentType was undefined/unitialized
    UNDEFINED,

    /// Indicates the Number of @c ContentType enumerations
    NUM_CONTENT_TYPE
};

/**
 * This function converts the provided @c ContentType to a string.
 *
 * @param contentType The @c ContentType to convert to a string.
 * @return The string conversion of @c contentType.
 */
inline std::string contentTypeToString(ContentType contentType) {
    switch (contentType) {
        case ContentType::MIXABLE:
            return "MIXABLE";
        case ContentType::NONMIXABLE:
            return "NONMIXABLE";
        case ContentType::UNDEFINED:
            return "UNDEFINED";
        default:
            return "UNDEFINED";
    }
}

/**
 * Write a @c ContentType value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param contenType The @c ContentType value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const ContentType& contentType) {
    return stream << contentTypeToString(contentType);
}
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CONTENTTYPE_H_
