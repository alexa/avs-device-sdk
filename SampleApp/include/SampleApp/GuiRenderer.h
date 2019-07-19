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
#include <map>
#include <string>

namespace alexaClientSDK {
namespace sampleApp {

/**
 * A class that implements the TemplateRuntimeObserverInterface.  Instead of rendering the
 * display cards, this class will print out some useful information (e.g. JSON payload)
 * when the renderTemplateCard or renderPlayerInfoCard callbacks are called.
 *
 * It is also used to track the PlayerInfo controls, and provide the PlayerInfo toggle states.
 *
 * @note Due to the payload in RenderTemplate may contain sensitive information, the
 * payload will only be printed if @c ACSDK_EMIT_SENSITIVE_LOGS is ON.
 */
class GuiRenderer : public avsCommon::sdkInterfaces::TemplateRuntimeObserverInterface {
public:
    /**
     * Constructor.
     */
    GuiRenderer();

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

    /// returns the PlayerInfo toggle state
    bool getGuiToggleState(const std::string& toggleName);

    /// String to identify the AVS action SELECTED string.
    static const std::string TOGGLE_ACTION_SELECTED;
    /// String to identify the AVS action DESELECTED string.
    static const std::string TOGGLE_ACTION_DESELECTED;
    /// String to identify the AVS name SHUFFLE string.
    static const std::string TOGGLE_NAME_SHUFFLE;
    /// String to identify the AVS name LOOP string.
    static const std::string TOGGLE_NAME_LOOP;
    /// String to identify the AVS name REPEAT string.
    static const std::string TOGGLE_NAME_REPEAT;
    /// String to identify the AVS name THUMBS_UP string.
    static const std::string TOGGLE_NAME_THUMBSUP;
    /// String to identify the AVS name THUMBS_DOWN string.
    static const std::string TOGGLE_NAME_THUMBSDOWN;

private:
    // Last known toggle states
    std::map<std::string, bool> m_guiToggleStateMap;
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_GUIRENDERER_H_
