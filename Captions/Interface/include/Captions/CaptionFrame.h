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

#ifndef ALEXA_CLIENT_SDK_CAPTIONS_INTERFACE_INCLUDE_CAPTIONS_CAPTIONFRAME_H_
#define ALEXA_CLIENT_SDK_CAPTIONS_INTERFACE_INCLUDE_CAPTIONS_CAPTIONFRAME_H_

#include <chrono>
#include <iostream>
#include <vector>

#include <AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h>

#include "CaptionLine.h"

namespace alexaClientSDK {
namespace captions {

/**
 * A container to represent a single frame of captions, with all the metadata needed to format and display the text.
 */
class CaptionFrame {
public:
    /// Type alias to the media player source ID.
    using MediaPlayerSourceId = avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId;

    /**
     * Get the maximum number of acceptable line wraps that can occur for a single @c CaptionFrame. This is useful as a
     * guard value to prevent accidental infinite loops when calculating line wraps.
     */
    static int getLineWrapLimit();

    /**
     * Constructor
     *
     * @param sourceId The media player source ID. CaptionFrames from the same media source share the same ID.
     * @param duration A number in milliseconds to determine how long the caption should be displayed on the screen.
     * @param delay The amount of time that should pass before this frame is shown on the screen.
     * @param captionTextLines One or more @c CaptionLine objects.
     */
    CaptionFrame(
        MediaPlayerSourceId sourceId = 0,
        std::chrono::milliseconds duration = std::chrono::milliseconds(0),
        std::chrono::milliseconds delay = std::chrono::milliseconds(0),
        const std::vector<CaptionLine>& captionLines = {});

    /**
     * How long the caption text should be displayed on the screen.
     *
     * @return The display duration, in milliseconds.
     */
    std::chrono::milliseconds getDuration() const;

    /**
     * The amount of time that should pass before this frame is shown on the screen.
     *
     * @return The delay, in milliseconds.
     */
    std::chrono::milliseconds getDelay() const;

    /**
     * Getter to return the source ID for this caption content.
     *
     * @return The media source ID.
     */
    MediaPlayerSourceId getSourceId() const;

    /**
     * Getter for the caption text, consisting of one or more pairs of strings of text with the styles present for that
     * text. Each pair represents one styled line of caption text.
     *
     * @return One or more @c CaptionLine objects, each representing one styled line of caption text.
     */
    std::vector<CaptionLine> getCaptionLines() const;

    /**
     * Operator == for @c CaptionFrame.
     *
     * @param rhs The right hand side of the == operation.
     * @return Whether or not this instance and @c rhs are equivalent.
     */
    bool operator==(const CaptionFrame& rhs) const;

    /**
     * Operator != for @c CaptionFrame.
     *
     * @param rhs The right hand side of the != operation.
     * @return Whether or not this instance and @c rhs are not equivalent.
     */
    bool operator!=(const CaptionFrame& rhs) const;

private:
    /// The ID of the media source.
    MediaPlayerSourceId m_id;

    /// How long the caption text should be displayed on the screen.
    std::chrono::milliseconds m_duration;

    /// How long of a delay should be present before being displayed.
    std::chrono::milliseconds m_delay;

    /// The @c CaptionLine objects that compose the entire text for this @c CaptionFrame.
    std::vector<CaptionLine> m_captionLines;
};

/**
 * Write a @c CaptionFrame value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param frame The @c CaptionFrame value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
std::ostream& operator<<(std::ostream& stream, const CaptionFrame& frame);

}  // namespace captions
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPTIONS_INTERFACE_INCLUDE_CAPTIONS_CAPTIONFRAME_H_
