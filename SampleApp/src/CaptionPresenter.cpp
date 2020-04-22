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

#include "SampleApp/CaptionPresenter.h"
#include "SampleApp/ConsolePrinter.h"

namespace alexaClientSDK {
namespace sampleApp {

using namespace avsCommon::utils;
using namespace alexaClientSDK::avsCommon::avs;

void CaptionPresenter::onCaptionActivity(const captions::CaptionFrame& captionFrame, FocusState focus) {
    // Note: due to the nature of console-driven text output, two concessions are being made:
    //  - The only type of focus that is handled is FOREGROUND. BACKGROUND and NONE should also be handled.
    //  - Each CaptionLine object in the CaptionFrame contains style information which should be handled according to
    //    the presentation needs of the application, and in a way which matches with CaptionPresenter::getWrapIndex.
    if (FocusState::FOREGROUND == focus) {
        std::vector<std::string> captionText;
        for (auto& line : captionFrame.getCaptionLines()) {
            captionText.emplace_back(line.text);
        }
        ConsolePrinter::captionsPrint(captionText);
    }
}

std::pair<bool, int> CaptionPresenter::getWrapIndex(const captions::CaptionLine& captionLine) {
    // This is a simplistic implementation that relies on the fixed-width characters of a console output.
    // A "real" implementation would apply the styles and measure the width of the text as shown on a screen to
    // determine if the text should wrap.

    // This lineWidth value is artificially small to demonstrate the line wrapping functionality.
    const size_t lineWidth = 30;
    if (captionLine.text.length() > lineWidth) {
        return std::make_pair(true, lineWidth);
    }
    return std::make_pair(false, 0);
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
