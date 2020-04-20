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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_CAPTIONPRESENTER_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_CAPTIONPRESENTER_H_

#include <Captions/CaptionPresenterInterface.h>

namespace alexaClientSDK {
namespace sampleApp {

/**
 * Sample implementation of @c CaptionPresenterInterface that measures text and prints captions content to console.
 */
class CaptionPresenter : public captions::CaptionPresenterInterface {
public:
    /// @name CaptionPresenterInterface methods
    /// @{
    void onCaptionActivity(const captions::CaptionFrame& captionFrame, avsCommon::avs::FocusState focusState) override;
    std::pair<bool, int> getWrapIndex(const captions::CaptionLine& captionLine) override;
    ///@}
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_CAPTIONPRESENTER_H_
