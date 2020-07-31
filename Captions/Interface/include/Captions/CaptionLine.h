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

#ifndef ALEXA_CLIENT_SDK_CAPTIONS_INTERFACE_INCLUDE_CAPTIONS_CAPTIONLINE_H_
#define ALEXA_CLIENT_SDK_CAPTIONS_INTERFACE_INCLUDE_CAPTIONS_CAPTIONLINE_H_

#include <string>
#include <vector>

#include "TextStyle.h"

namespace alexaClientSDK {
namespace captions {

/**
 * A single line of styled caption text.
 */
struct CaptionLine {
    /**
     * Constructor.
     *
     * @param captionText The text content for this line of captions.
     * @param textStyles Zero or more styles with indices that match with the character indices in @c text.
     */
    CaptionLine(const std::string& text = "", const std::vector<TextStyle>& styles = {});

    /**
     * Clefts @c CaptionLine object in twain, at the text index specified. If the second @c CaptionLine starts with
     * whitespace, then it will be removed and the indices will be adjusted. If the index given is greater than the text
     * length, then the returned vector will contain only the current (unmodified) @c CaptionLine object.
     *
     * @param index The text index to determine where the text and style content should be divided.
     * @return One or two @c CaptionLine objects whose unioned content equal the content of this @c CaptionLine object.
     */
    std::vector<CaptionLine> splitAtTextIndex(size_t index);

    /**
     * Joins multiple @c CaptionLine objects into a single @c CaptionLine, adjusting the character indices as needed for
     * the styles. This is the inverse operation of @c CaptionLine::splitAtTextIndex().
     *
     * @param captionLines The caption lines to join together.
     */
    static CaptionLine merge(std::vector<CaptionLine> captionLines);

    /**
     * Operator == for @c CaptionLine.
     *
     * @param rhs The right hand side of the == operation.
     * @return Whether or not this instance and @c rhs are equivalent.
     */
    bool operator==(const CaptionLine& rhs) const;

    /**
     * Operator != for @c CaptionLine.
     *
     * @param rhs The right hand side of the != operation.
     * @return Whether or not this instance and @c rhs are not equivalent.
     */
    bool operator!=(const CaptionLine& rhs) const;

    /// The text content for this line of captions.
    std::string text;

    /// Zero or more @c TextStyles that relate to @c m_text.
    std::vector<TextStyle> styles;
};

/**
 * Write a @c CaptionLine value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param cl The caption line value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
std::ostream& operator<<(std::ostream& stream, const CaptionLine& line);

}  // namespace captions
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPTIONS_INTERFACE_INCLUDE_CAPTIONS_CAPTIONLINE_H_
