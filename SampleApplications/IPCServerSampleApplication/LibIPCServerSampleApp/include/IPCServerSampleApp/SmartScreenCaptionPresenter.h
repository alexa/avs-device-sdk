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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_SMARTSCREENCAPTIONPRESENTER_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_SMARTSCREENCAPTIONPRESENTER_H_

#include "IPCServerSampleApp/RenderCaptionsInterface.h"
#include <Captions/CaptionPresenterInterface.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

class SmartScreenCaptionPresenter : public captions::CaptionPresenterInterface {
public:
    explicit SmartScreenCaptionPresenter(std::shared_ptr<RenderCaptionsInterface> renderCaptionsInterface);

    /// @name CaptionPresenterInterface methods
    /// @{
    void onCaptionActivity(const captions::CaptionFrame& captionFrame, avsCommon::avs::FocusState focusState) override;
    std::pair<bool, int> getWrapIndex(const captions::CaptionLine& captionLine) override;
    ///@}

private:
    /// Pointer to the GUI Client interface
    std::shared_ptr<RenderCaptionsInterface> m_renderCaptionsInterface;

    rapidjson::Document convertCaptionFrameToJson(const captions::CaptionFrame& captionFrame);
    rapidjson::Value convertCaptionLineToJson(
        const captions::CaptionLine& captionLine,
        rapidjson::Document::AllocatorType& allocator);
    rapidjson::Value convertTextStyleToJson(
        captions::TextStyle textStyle,
        rapidjson::Document::AllocatorType& allocator);
    rapidjson::Value convertStyleToJson(captions::Style style, rapidjson::Document::AllocatorType& allocator);

    rapidjson::Value convertCaptionLinesToJson(
        const std::vector<captions::CaptionLine>& captionLines,
        rapidjson::Document::AllocatorType& allocator);
};

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_SMARTSCREENCAPTIONPRESENTER_H_
