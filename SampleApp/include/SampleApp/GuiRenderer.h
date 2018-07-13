/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_GUIRENDERER_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_GUIRENDERER_H_

#include <AVSCommon/SDKInterfaces/TemplateRuntimeObserverInterface.h>

namespace alexaClientSDK {
namespace sampleApp {

/**
 * A simple class that implements the TemplateRuntimeObserverInterface.  Instead of rendering the
 * display cards, this class will print out some useful information (e.g. JSON payload)
 * when the renderTemplateCard or renderPlayerInfoCard callbacks are called.
 *
 * @note Due to the payload in RenderTemplate may contain sensitive information, the
 * payload will only be printed if @c ACSDK_EMIT_SENSITIVE_LOGS is ON.
 */
class GuiRenderer : public avsCommon::sdkInterfaces::TemplateRuntimeObserverInterface {
public:
    /// @name TemplateRuntimeObserverInterface Functions
    /// @{
    void renderTemplateCard(const std::string& jsonPayload, avsCommon::avs::FocusState focusState) override;
    void clearTemplateCard() override;
    void renderPlayerInfoCard(
        const std::string& jsonPayload,
        TemplateRuntimeObserverInterface::AudioPlayerInfo info,
        avsCommon::avs::FocusState focusState) override;
    void clearPlayerInfoCard() override;
    /// @}
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_GUIRENDERER_H_
