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

/// Tag for find the AudioItemId in the payload of the RenderPlayerInfo directive
static const std::string AUDIO_ITEM_ID_TAG("audioItemId");

/// Tag for find the template type in the payload of the RenderTemplate directive
static const std::string TEMPLATE_TYPE_TAG("type");

/// Tag for find the title node in the payload of the RenderTemplate directive
static const std::string TITLE_NODE("title");

/// Tag for find the maintitle in the title node of the RenderTemplate directive
static const std::string MAIN_TITLE_TAG("mainTitle");

static const std::string RENDER_TEMPLATE_HEADER =
    "##############################################################################\n"
    "#     RenderTemplateCard                                                      \n"
    "#-----------------------------------------------------------------------------\n";

static const std::string RENDER_TEMPLATE_CLEARED =
    "##############################################################################\n"
    "#     RenderTemplateCard - Cleared                                            \n"
    "##############################################################################\n";

static const std::string RENDER_FOOTER =
    "##############################################################################\n";

static const std::string RENDER_PLAYER_INFO_HEADER =
    "##############################################################################\n"
    "#     RenderPlayerInfoCard                                                    \n"
    "#-----------------------------------------------------------------------------\n";

static const std::string RENDER_PLAYER_INFO_CLEARED =
    "##############################################################################\n"
    "#     RenderPlayerInfoCard - Cleared                                          \n"
    "##############################################################################\n";

void GuiRenderer::renderTemplateCard(const std::string& jsonPayload, avsCommon::avs::FocusState focusState) {
    rapidjson::Document payload;
    rapidjson::ParseResult result = payload.Parse(jsonPayload);
    if (!result) {
        return;
    }

    std::string templateType;
    if (!avsCommon::utils::json::jsonUtils::retrieveValue(payload, TEMPLATE_TYPE_TAG, &templateType)) {
        return;
    }

    rapidjson::Value::ConstMemberIterator titleNode;
    if (!avsCommon::utils::json::jsonUtils::findNode(payload, TITLE_NODE, &titleNode)) {
        return;
    }

    std::string mainTitle;
    if (!avsCommon::utils::json::jsonUtils::retrieveValue(titleNode->value, MAIN_TITLE_TAG, &mainTitle)) {
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
        return;
    }

    std::string audioItemId;
    if (!avsCommon::utils::json::jsonUtils::retrieveValue(payload, AUDIO_ITEM_ID_TAG, &audioItemId)) {
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
    buffer += RENDER_FOOTER;
    ConsolePrinter::simplePrint(buffer);
}

void GuiRenderer::clearPlayerInfoCard() {
    ConsolePrinter::simplePrint(RENDER_PLAYER_INFO_CLEARED);
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
