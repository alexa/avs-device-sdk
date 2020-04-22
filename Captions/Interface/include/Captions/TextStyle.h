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

#ifndef ALEXA_CLIENT_SDK_CAPTIONS_INTERFACE_INCLUDE_CAPTIONS_TEXTSTYLE_H_
#define ALEXA_CLIENT_SDK_CAPTIONS_INTERFACE_INCLUDE_CAPTIONS_TEXTSTYLE_H_

#include <cstdint>
#include <iostream>

namespace alexaClientSDK {
namespace captions {

/**
 * Container used to describe basic text markup, such as bold, italic, etc.
 */
struct Style {
    /**
     * Constructor.
     *
     * @param bold whether the bolded text appearance should be applied to the text.
     * @param italic whether the italic text appearance should be applied to the text.
     * @param underline whether the underlined text appearance should be applied to the text.
     */
    Style(bool bold = false, bool italic = false, bool underline = false);

    /**
     * Operator == to compare @c Style values.
     *
     * @param rhs The right hand side of the == operation.
     * @return Whether or not this style and @c rhs are equivalent.
     */
    bool operator==(const Style& rhs) const;

    /**
     * Operator == to compare @c Style values.
     *
     * @param rhs The right hand side of the == operation.
     * @return Whether or not this style and @c rhs are equivalent.
     */
    bool operator!=(const Style& rhs) const;

    /// Indicates whether the bolded text appearance should be applied to the text.
    bool m_bold;

    /// Indicates whether the italic text appearance should be applied to the text.
    bool m_italic;

    /// Indicates whether the underlined text appearance should be applied to the text.
    bool m_underline;
};

/**
 * Write a @c Style value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param s The @c Style value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
std::ostream& operator<<(std::ostream& stream, const Style& style);

/**
 * Container to describe style text that should be applied starting at a given index.
 */
struct TextStyle {
    /**
     * Constructor.
     * @param index The character index from which the @c style should be applied.
     * @param style The styles that are active and should be applied from this @c index forward.
     */
    TextStyle(size_t index = 0, const Style& style = Style());

    /// The character index from which the @c activeStyle should be applied.
    size_t charIndex;

    /// The styles that are active and should be applied from this @c charIndex forward.
    Style activeStyle;

    /**
     * Operator == to compare @c TextStyle values.
     *
     * @param rhs The right hand side of the == operation.
     * @return Whether or not this text style and @c rhs are equivalent.
     */
    bool operator==(const TextStyle& rhs) const;

    /**
     * Operator != to compare @c TextStyle values.
     *
     * @param rhs The right hand side of the != operation.
     * @return Whether or not this text style and @c rhs are not equivalent.
     */
    bool operator!=(const TextStyle& rhs) const;
};

/**
 * Write a @c TextStyle value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param s The @c TextStyle value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
std::ostream& operator<<(std::ostream& stream, const TextStyle& textStyle);

}  // namespace captions
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_CAPTIONS_INTERFACE_INCLUDE_CAPTIONS_TEXTSTYLE_H_
