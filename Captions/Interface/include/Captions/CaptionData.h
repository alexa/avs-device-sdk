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

#ifndef ALEXA_CLIENT_SDK_CAPTIONS_INTERFACE_INCLUDE_CAPTIONS_CAPTIONDATA_H_
#define ALEXA_CLIENT_SDK_CAPTIONS_INTERFACE_INCLUDE_CAPTIONS_CAPTIONDATA_H_

#include <string>
#include <utility>

#include "CaptionFormat.h"

namespace alexaClientSDK {
namespace captions {

/**
 * A container for un-processed caption content and metadata of a particular caption format.
 */
struct CaptionData {
    /**
     * Constructor.
     *
     * @param format The caption format enum value.
     * @param content The caption content.
     */
    CaptionData(const CaptionFormat format = CaptionFormat::UNKNOWN, const std::string& content = "") :
            format{format},
            content{content} {
    }

    /**
     * Indicates if the data contained in this struct is valid caption data.
     *
     * @return true if this object contains valid caption data.
     */
    bool isValid() const;

    /**
     * Operator == to compare @c CaptionData values.
     *
     * @param rhs The right hand side of the == operation.
     * @return Whether or not this instance and @c rhs are equivalent.
     */
    bool operator==(const CaptionData& rhs) const {
        return (format == rhs.format) && (content == rhs.content);
    }

    /**
     * Operator != for @c CaptionData.
     *
     * @param rhs The right hand side of the != operation.
     * @return Whether or not this instance and @c rhs are not equivalent.
     */
    bool operator!=(const CaptionData& rhs) const {
        return !(rhs == *this);
    }

    /// The format of the un-processed caption content.
    CaptionFormat format;

    /// The un-processed caption content.
    std::string content;
};

}  // namespace captions
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPTIONS_INTERFACE_INCLUDE_CAPTIONS_CAPTIONDATA_H_
