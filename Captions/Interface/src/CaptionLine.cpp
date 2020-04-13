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

#include <algorithm>
#include <limits>
#include <utility>

#include "Captions/CaptionLine.h"

#include <AVSCommon/Utils/Logger/LoggerUtils.h>
#include <AVSCommon/Utils/String/StringUtils.h>

namespace alexaClientSDK {
namespace captions {

using namespace alexaClientSDK::avsCommon;
using namespace alexaClientSDK::avsCommon::utils::logger;

/// String to identify log entries originating from this file.
static const std::string TAG("CaptionLine");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) LogEntry(TAG, event)

CaptionLine::CaptionLine(const std::string& text, const std::vector<TextStyle>& styles) : text{text}, styles{styles} {
}

CaptionLine CaptionLine::merge(std::vector<CaptionLine> captionLines) {
    CaptionLine result;
    if (captionLines.empty()) {
        return result;
    }

    size_t indexOffset = 0;

    // check for baseline style
    if (captionLines.front().styles.empty() || captionLines.front().styles.front().charIndex != 0) {
        result.styles.emplace_back(TextStyle());
    }

    for (const auto& line : captionLines) {
        result.text.append(line.text);

        // adjust style indices
        for (const auto& style : line.styles) {
            TextStyle tempStyle = style;
            tempStyle.charIndex += indexOffset;
            result.styles.emplace_back(tempStyle);
        }
        indexOffset = line.text.length();
    }
    return result;
}

std::vector<CaptionLine> CaptionLine::splitAtTextIndex(size_t index) {
    std::vector<CaptionLine> result;
    if (index > text.length()) {
        result.emplace_back(CaptionLine(text, styles));
        return result;
    }
    CaptionLine lineOne, lineTwo;

    lineOne.text = text.substr(0, index);

    // remove leading whitespace and measure how many characters are getting removed so that style indices can
    // be later adjusted.
    lineTwo.text = text.substr(index);
    size_t originalLineTwoTextLength = lineTwo.text.length();
    lineTwo.text = utils::string::ltrim(lineTwo.text);
    size_t whitespaceCount = originalLineTwoTextLength - lineTwo.text.length();

    if (styles.empty()) {
        result.emplace_back(CaptionLine(lineOne.text, lineOne.styles));
        result.emplace_back(CaptionLine(lineTwo.text, lineTwo.styles));
        return result;
    }

    // styles need to be in order, by index, for the splitting
    sort(
        styles.begin(), styles.end(), [](const TextStyle& l, const TextStyle& r) { return l.charIndex < r.charIndex; });

    // find the point at which the style should be split; this is the last style to be applied before the index.
    size_t appliedStyleIndex = 0;
    for (auto& style : styles) {
        if (style.charIndex < index) {
            appliedStyleIndex++;
        }
    }

    // Split the styles
    lineOne.styles = {styles.begin(), styles.begin() + appliedStyleIndex};
    lineTwo.styles = {styles.begin() + appliedStyleIndex, styles.end()};

    // determine how much the indices should be adjusted
    size_t indexOffset = 0;
    if (index > std::numeric_limits<size_t>::max() - whitespaceCount) {
        // This is a very unlikely overflow, but if it does, then the character index is set to zero as a safety.
        ACSDK_WARN(LX("lineSplitFailure")
                       .d("reason", "characterAdjustmentAmountWouldOverflow")
                       .d("index", index)
                       .d("whitespaceCount", whitespaceCount));
    } else {
        indexOffset = index + whitespaceCount;
    }

    // adjust the style indices
    for (auto& lineTwoStyle : lineTwo.styles) {
        if (indexOffset > lineTwoStyle.charIndex) {
            lineTwoStyle.charIndex = 0;
        } else {
            lineTwoStyle.charIndex -= indexOffset;
        }
    }

    if (lineTwo.styles.empty() || lineTwo.styles[0].charIndex != 0) {
        // need to carry over the last style from the first line.
        lineTwo.styles.insert(lineTwo.styles.begin(), lineOne.styles.back());

        // The first style will always start at charIndex zero.
        if (!lineTwo.styles.empty()) {
            lineTwo.styles[0].charIndex = 0;
        }
    }

    result.emplace_back(CaptionLine(lineOne.text, lineOne.styles));
    result.emplace_back(CaptionLine(lineTwo.text, lineTwo.styles));
    return result;
}

bool CaptionLine::operator==(const CaptionLine& rhs) const {
    return (text == rhs.text) && (styles == rhs.styles);
}

bool CaptionLine::operator!=(const CaptionLine& rhs) const {
    return !(rhs == *this);
}

std::ostream& operator<<(std::ostream& stream, const CaptionLine& line) {
    stream << "CaptionLine(text:\"" << line.text << "\", styles:[";
    for (auto iter = line.styles.begin(); iter != line.styles.end(); iter++) {
        if (iter != line.styles.begin()) stream << ", ";
        stream << *iter;
    }
    stream << "])";
    return stream;
}

}  // namespace captions
}  // namespace alexaClientSDK
