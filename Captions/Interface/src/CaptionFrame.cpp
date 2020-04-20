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

#include "Captions/CaptionFrame.h"

namespace alexaClientSDK {
namespace captions {

/// The maximum number of acceptable line wraps that can occur for a single @c CaptionFrame.
static const int LINE_WRAP_LIMIT = 200;

CaptionFrame::CaptionFrame(
    MediaPlayerSourceId id,
    std::chrono::milliseconds duration,
    std::chrono::milliseconds delay,
    const std::vector<CaptionLine>& captionLines) :
        m_id{id},
        m_duration{duration},
        m_delay{delay},
        m_captionLines{captionLines} {
}

CaptionFrame::MediaPlayerSourceId CaptionFrame::getSourceId() const {
    return m_id;
}

std::chrono::milliseconds CaptionFrame::getDuration() const {
    return m_duration;
}

std::chrono::milliseconds CaptionFrame::getDelay() const {
    return m_delay;
}

std::vector<CaptionLine> CaptionFrame::getCaptionLines() const {
    return m_captionLines;
}

int CaptionFrame::getLineWrapLimit() {
    return LINE_WRAP_LIMIT;
}

bool CaptionFrame::operator==(const CaptionFrame& rhs) const {
    return (m_id == rhs.getSourceId()) && (m_duration == rhs.getDuration()) && (m_delay == rhs.getDelay()) &&
           (m_captionLines == rhs.getCaptionLines());
}

bool CaptionFrame::operator!=(const CaptionFrame& rhs) const {
    return !(rhs == *this);
}

std::ostream& operator<<(std::ostream& stream, const CaptionFrame& frame) {
    stream << "CaptionFrame(id:" << frame.getSourceId() << ", duration:" << frame.getDuration().count()
           << ", delay:" << frame.getDelay().count() << ", lines:[";
    const auto captionLinesCopy = frame.getCaptionLines();
    for (auto iter = captionLinesCopy.begin(); iter != captionLinesCopy.end(); iter++) {
        if (iter != captionLinesCopy.begin()) stream << ", ";
        stream << *iter;
    }
    stream << "])";
    return stream;
}

}  // namespace captions
}  // namespace alexaClientSDK
