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

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include "SampleApp/ConsolePrinter.h"
#include "SampleApp/GuiRenderer.h"

namespace alexaClientSDK {
namespace sampleApp {

using namespace avsCommon::utils::json;

/// Tag for find the AudioItemId in the payload of the RenderPlayerInfo directive
static const std::string AUDIO_ITEM_ID_TAG("audioItemId");

/// Tag for find the template type in the payload of the RenderTemplate directive
static const std::string TEMPLATE_TYPE_TAG("type");

/// Tag for find the title node in the payload of the RenderTemplate directive
static const std::string TITLE_NODE("title");

/// Tag for find the controls node in the payload of the RenderTemplate directive
static const std::string CONTROLS_NODE("controls");

/// Tag for find the controls name key in the payload of the RenderTemplate directive
static const std::string CONTROLS_NAME_KEY("name");

/// Tag for find the controls selected key in the payload of the RenderTemplate directive
static const std::string CONTROLS_SELECTED_KEY("selected");

/// Tag for find the maintitle in the title node of the RenderTemplate directive
static const std::string MAIN_TITLE_TAG("mainTitle");

// String to identify the AVS name SKIP_FORWARD string.
static const std::string BUTTON_NAME_SKIPFORWARD("SKIP_FORWARD");

// String to identify the AVS name SKIP_BACKWARD string.
static const std::string BUTTON_NAME_SKIPBACKWARD("SKIP_BACKWARD");

/// Begin a RenderTemplate UI block
static const std::string RENDER_TEMPLATE_HEADER =
    "##############################################################################\n"
    "#     RenderTemplateCard                                                      \n"
    "#-----------------------------------------------------------------------------\n";

/// RenderTemplate UI has been cleared by AVS
static const std::string RENDER_TEMPLATE_CLEARED =
    "##############################################################################\n"
    "#     RenderTemplateCard - Cleared                                            \n"
    "##############################################################################\n";

/// Generic Separator
static const std::string RENDER_FOOTER =
    "##############################################################################\n";

/// Begin a PlayerInfo UI block
static const std::string RENDER_PLAYER_INFO_HEADER =
    "##############################################################################\n"
    "#     RenderPlayerInfoCard                                                    \n"
    "#-----------------------------------------------------------------------------\n";

/// Begin a PlayerInfo UI GUI controls block
static const std::string RENDER_PLAYER_CONTROLS_HEADER =
    "##############################################################################\n"
    "#     GUI Playback Controls                                                   \n"
    "#-----------------------------------------------------------------------------\n";

/// PlayerInfo UI has been cleared by AVS
static const std::string RENDER_PLAYER_INFO_CLEARED =
    "##############################################################################\n"
    "#     RenderPlayerInfoCard - Cleared                                          \n"
    "##############################################################################\n";

/// utility for bool to state string
static std::string boolToSelectedString(bool selected) {
    return selected ? GuiRenderer::TOGGLE_ACTION_SELECTED : GuiRenderer::TOGGLE_ACTION_DESELECTED;
}

GuiRenderer::GuiRenderer() {
    // initialize map
    m_guiToggleStateMap.insert(std::make_pair(TOGGLE_NAME_SHUFFLE, false));
    m_guiToggleStateMap.insert(std::make_pair(TOGGLE_NAME_LOOP, false));
    m_guiToggleStateMap.insert(std::make_pair(TOGGLE_NAME_REPEAT, false));
    m_guiToggleStateMap.insert(std::make_pair(TOGGLE_NAME_THUMBSDOWN, false));
    m_guiToggleStateMap.insert(std::make_pair(TOGGLE_NAME_THUMBSUP, false));
}

const std::string GuiRenderer::TOGGLE_ACTION_SELECTED = "SELECTED";

const std::string GuiRenderer::TOGGLE_ACTION_DESELECTED = "DESELECTED";

const std::string GuiRenderer::TOGGLE_NAME_SHUFFLE = "SHUFFLE";

const std::string GuiRenderer::TOGGLE_NAME_LOOP = "LOOP";

const std::string GuiRenderer::TOGGLE_NAME_REPEAT = "REPEAT";

const std::string GuiRenderer::TOGGLE_NAME_THUMBSUP = "THUMBS_UP";

const std::string GuiRenderer::TOGGLE_NAME_THUMBSDOWN = "THUMBS_DOWN";

void GuiRenderer::renderTemplateCard(const std::string& jsonPayload, avsCommon::avs::FocusState focusState) {
    rapidjson::Document payload;
    rapidjson::ParseResult result = payload.Parse(jsonPayload);
    if (!result) {
        ConsolePrinter::simplePrint("ERROR: Template JSON payload not parseable");
        return;
    }

    std::string templateType;
    if (!jsonUtils::retrieveValue(payload, TEMPLATE_TYPE_TAG, &templateType)) {
        ConsolePrinter::simplePrint("ERROR: Template JSON payload has no template type field");
        return;
    }

    rapidjson::Value::ConstMemberIterator titleNode;
    if (!jsonUtils::findNode(payload, TITLE_NODE, &titleNode)) {
        ConsolePrinter::simplePrint("ERROR: Template JSON payload has no title field");
        return;
    }

    std::string mainTitle;
    if (!jsonUtils::retrieveValue(titleNode->value, MAIN_TITLE_TAG, &mainTitle)) {
        ConsolePrinter::simplePrint("ERROR: Template JSON payload has no main title field");
        return;
    }

    // Storing the output in a single buffer so that the display is continuous.
    std::string buffer;
    buffer += RENDER_TEMPLATE_HEADER;
    buffer += "# Focus State         : " + focusStateToString(focusState) + "\n";
    buffer += "# Template Type       : " + templateType + "\n";
    buffer += "# Main Title          : " + mainTitle + "\n";
    buffer += RENDER_FOOTER;

#ifdef ACSDK_EMIT_SENSITIVE_LOGS
    buffer += jsonPayload + "\n";
    buffer += RENDER_FOOTER;

#endif

    ConsolePrinter::simplePrint(buffer);
}

void GuiRenderer::clearTemplateCard() {
    ConsolePrinter::simplePrint(RENDER_TEMPLATE_CLEARED);
}

void GuiRenderer::renderPlayerInfoCard(
    const std::string& jsonPayload,
    TemplateRuntimeObserverInterface::AudioPlayerInfo info,
    avsCommon::avs::FocusState focusState) {
    rapidjson::Document payload;
    rapidjson::ParseResult result = payload.Parse(jsonPayload);
    if (!result) {
        ConsolePrinter::simplePrint("ERROR: PlayerInfo JSON payload not parseable");
        return;
    }

    std::string audioItemId;
    if (!jsonUtils::retrieveValue(payload, AUDIO_ITEM_ID_TAG, &audioItemId)) {
        ConsolePrinter::simplePrint("ERROR: PlayerInfo JSON payload has no audio item field");
        return;
    }

    // Storing the output in a single buffer so that the display is continuous.
    std::string buffer;
    buffer += RENDER_PLAYER_INFO_HEADER;
    buffer += "# Focus State         : " + focusStateToString(focusState) + "\n";
    buffer += "# AudioItemId         : " + audioItemId + "\n";
    buffer += "# Audio state         : " + playerActivityToString(info.audioPlayerState) + "\n";
    buffer += "# Offset milliseconds : " + std::to_string(info.offset.count()) + "\n";
    buffer += RENDER_FOOTER;
    buffer += jsonPayload + "\n";
    buffer += RENDER_PLAYER_CONTROLS_HEADER;

    // Add GUI Playback Controller Button command info based on PlayerInfo payload
    rapidjson::Value::ConstMemberIterator controlsNode;
    if (jsonUtils::findNode(payload, CONTROLS_NODE, &controlsNode)) {
        const rapidjson::Value& controls = payload[CONTROLS_NODE];
        for (rapidjson::SizeType i = 0; i < controls.Size(); i++) {
            std::string controlName;
            if (!jsonUtils::retrieveValue(controls[i], CONTROLS_NAME_KEY, &controlName)) {
                ConsolePrinter::simplePrint("ERROR: PlayerInfo JSON payload has controls field");
                break;
            }

            bool controlSelected;
            if (!jsonUtils::retrieveValue(controls[i], CONTROLS_SELECTED_KEY, &controlSelected)) {
                ConsolePrinter::simplePrint("ERROR: PlayerInfo JSON payload has controls selected field");
                break;
            } else if (BUTTON_NAME_SKIPFORWARD == controlName) {
                buffer += "# Press '5' for a '" + BUTTON_NAME_SKIPFORWARD + "' button press.\n";
            } else if (BUTTON_NAME_SKIPBACKWARD == controlName) {
                buffer += "# Press '6' for a '" + BUTTON_NAME_SKIPBACKWARD + "' button press.\n";
            } else if (TOGGLE_NAME_SHUFFLE == controlName) {
                m_guiToggleStateMap[controlName] = controlSelected;
                buffer += "# Press '7' for a '" + TOGGLE_NAME_SHUFFLE + "' toggle press.     " + TOGGLE_NAME_SHUFFLE +
                          " state: " + boolToSelectedString(controlSelected) + "\n";
            } else if (TOGGLE_NAME_LOOP == controlName) {
                m_guiToggleStateMap[controlName] = controlSelected;
                buffer += "# Press '8' for a '" + TOGGLE_NAME_LOOP + "' toggle press.        " + TOGGLE_NAME_LOOP +
                          " state: " + boolToSelectedString(controlSelected) + "\n";
            } else if (TOGGLE_NAME_REPEAT == controlName) {
                m_guiToggleStateMap[controlName] = controlSelected;
                buffer += "# Press '9' for a '" + TOGGLE_NAME_REPEAT + "' toggle Pressess.   " + TOGGLE_NAME_REPEAT +
                          " state: " + boolToSelectedString(controlSelected) + "\n";
            } else if (TOGGLE_NAME_THUMBSDOWN == controlName) {
                m_guiToggleStateMap[controlName] = controlSelected;
                buffer += "# Press '-' for a '" + TOGGLE_NAME_THUMBSDOWN + "' toggle press. " + TOGGLE_NAME_THUMBSDOWN +
                          " state: " + boolToSelectedString(controlSelected) + "\n";
            } else if (TOGGLE_NAME_THUMBSUP == controlName) {
                m_guiToggleStateMap[controlName] = controlSelected;
                buffer += "# Press '+' for a '" + TOGGLE_NAME_THUMBSUP + "' toggle press.   " + TOGGLE_NAME_THUMBSUP +
                          " state: " + boolToSelectedString(controlSelected) + "\n";
            }
        }
    }
    buffer += RENDER_FOOTER;
    ConsolePrinter::simplePrint(buffer);
}

void GuiRenderer::clearPlayerInfoCard() {
    ConsolePrinter::simplePrint(RENDER_PLAYER_INFO_CLEARED);
}

bool GuiRenderer::getGuiToggleState(const std::string& toggleName) {
    return m_guiToggleStateMap.find(toggleName)->second;
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
