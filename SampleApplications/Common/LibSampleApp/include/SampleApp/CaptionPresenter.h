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

#ifndef SAMPLEAPP_CAPTIONPRESENTER_H_
#define SAMPLEAPP_CAPTIONPRESENTER_H_

#include <Captions/CaptionManagerInterface.h>
#include <Captions/CaptionPresenterInterface.h>

namespace alexaClientSDK {
namespace sampleApp {

/**
 * Sample implementation of @c CaptionPresenterInterface that measures text and prints captions content to console.
 */
class CaptionPresenter : public captions::CaptionPresenterInterface {
public:
    /**
     * Factory method that returns a new instance of @c CaptionPresenterInterface.
     *
     * @param captionManager The @c CaptionManagerInterface to which we should add this new @c
     * CaptionPresenterInterface. If null or if disabled, the CaptionPresenter is still created to satisfy manufactory
     * exports, but will not be added to the CaptionManager.
     * @return Pointer to a new @c CaptionPresenterInterface.
     */
    static std::shared_ptr<captions::CaptionPresenterInterface> createCaptionPresenterInterface(
        const std::shared_ptr<captions::CaptionManagerInterface>& captionManager);

    /// @name CaptionPresenterInterface methods
    /// @{
    void onCaptionActivity(const captions::CaptionFrame& captionFrame, avsCommon::avs::FocusState focusState) override;
    std::pair<bool, int> getWrapIndex(const captions::CaptionLine& captionLine) override;
    ///@}
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // SAMPLEAPP_CAPTIONPRESENTER_H_
