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

#ifndef ALEXA_CLIENT_SDK_CAPTIONS_INTERFACE_INCLUDE_CAPTIONS_CAPTIONPRESENTERINTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPTIONS_INTERFACE_INCLUDE_CAPTIONS_CAPTIONPRESENTERINTERFACE_H_

#include <utility>

#include <AVSCommon/AVS/FocusState.h>

#include "CaptionFrame.h"
#include "CaptionLine.h"
#include "TextStyle.h"

namespace alexaClientSDK {
namespace captions {

/**
 * An interface to measure lines of styled text and handle requests to show or hide a @c CaptionFrame.
 */
class CaptionPresenterInterface {
public:
    /**
     * Destructor.
     */
    virtual ~CaptionPresenterInterface() = default;

    /**
     * Handles a request to show or hide a @c CaptionFrame.
     *
     * @param captionFrame The @c CaptionFrame which is to be acted upon based on the action described by activityType.
     * @param activityType The @c FocusState to indicate whether to bring the caption to the foreground (show), send
     * them to the background (hide).
     */
    virtual void onCaptionActivity(
        const captions::CaptionFrame& captionFrame,
        avsCommon::avs::FocusState activityType) = 0;

    /**
     * Determine the display width of the line of text as it would be displayed on a screen.
     *
     * This function should apply the styles to the text present in the @c CaptionLine and measure the width as it would
     * be displayed on the screen. If the text is too wide to fit on the display, then return true, along with the
     * character index in captionLine of where the text becomes too wide to fit. This function should also return
     * quickly, as it is potentially called many times to find the correct text wrap points.
     *
     * @param captionLine The line of caption text with the styles that apply to that text.
     * application's display and needs to wrap to a new line.
     * @return a pair, the first value is whether a line wrap should occur, the second value indicates zero-indexed
     * character number of where in the captionLine's text the line wrap should occur, which takes effect only if the
     * first value is true. Otherwise return false as the first value.
     */
    virtual std::pair<bool, int> getWrapIndex(const captions::CaptionLine& captionLine) = 0;
};

}  // namespace captions
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPTIONS_INTERFACE_INCLUDE_CAPTIONS_CAPTIONPRESENTERINTERFACE_H_
