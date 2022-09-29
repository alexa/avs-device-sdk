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

#include "acsdk/VisualCharacteristics/private/VisualCharacteristicsSerializer.h"

#include <AVSCommon/Utils/Logger/Logger.h>

#include "acsdk/VisualCharacteristics/private/VCConfigParser.h"

namespace alexaClientSDK {
namespace visualCharacteristics {

using namespace visualCharacteristicsInterfaces;

/// String to identify log entries originating from this file.
#define TAG "VisualCharacteristicsSerializer"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<VisualCharacteristicsSerializerInterface> VisualCharacteristicsSerializer::create() {
    std::shared_ptr<VisualCharacteristicsSerializer> visualCharacteristicsSerializer(
        new VisualCharacteristicsSerializer());

    return visualCharacteristicsSerializer;
}

VisualCharacteristicsSerializer::VisualCharacteristicsSerializer() {
}

bool VisualCharacteristicsSerializer::serializeInteractionModes(
    const std::vector<InteractionMode>& interactionModes,
    std::string& serializedJson) {
    ACSDK_DEBUG5(LX(__func__));
    return VCConfigParser::serializeInteractionMode(interactionModes, serializedJson);
}

bool VisualCharacteristicsSerializer::serializeWindowTemplate(
    const std::vector<WindowTemplate>& windowTemplates,
    std::string& serializedJson) {
    ACSDK_DEBUG5(LX(__func__));
    return VCConfigParser::serializeWindowTemplate(windowTemplates, serializedJson);
}

bool VisualCharacteristicsSerializer::serializeDisplayCharacteristics(
    const DisplayCharacteristics& display,
    std::string& serializedJson) {
    ACSDK_DEBUG5(LX(__func__));
    return VCConfigParser::serializeDisplayCharacteristics(display, serializedJson);
}

}  // namespace visualCharacteristics
}  // namespace alexaClientSDK
