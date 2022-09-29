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

#include <AVSCommon/AVS/CapabilitySemantics/CapabilitySemantics.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include <algorithm>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace capabilitySemantics {

/// The "actionMappings" key.
static const std::string ACTION_MAPPINGS_KEY("actionMappings");

/// The "stateMappings" key.
static const std::string STATE_MAPPINGS_KEY("stateMappings");

/// Empty JSON object.
static const std::string EMPTY_JSON("{}");

/// String to identify log entries originating from this file.
#define TAG "CapabilitySemantics"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) utils::logger::LogEntry(TAG, event)

bool CapabilitySemantics::addActionsToDirectiveMapping(const ActionsToDirectiveMapping& mapping) {
    if (mapping.isValid()) {
        m_actionsDirectiveMappings.push_back(mapping);
        return true;
    } else {
        ACSDK_ERROR(LX("addActionsToDirectiveMappingFailed").d("reason", "invalidMapping"));
        return false;
    }
}

bool CapabilitySemantics::addStatesToValueMapping(const StatesToValueMapping& mapping) {
    if (mapping.isValid()) {
        m_statesValueMappings.push_back(mapping);
        return true;
    } else {
        ACSDK_ERROR(LX("addStatesToValueMappingFailed").d("reason", "invalidMapping"));
        return false;
    }
}

bool CapabilitySemantics::addStatesToRangeMapping(const StatesToRangeMapping& mapping) {
    if (mapping.isValid()) {
        m_statesRangeMappings.push_back(mapping);
        return true;
    } else {
        ACSDK_ERROR(LX("addStatesToRangeMappingFailed").d("reason", "invalidMapping"));
        return false;
    }
}

bool CapabilitySemantics::isValid() const {
    // Semantics requires at least one of 'actionMappings' or 'stateMappings'
    return !m_actionsDirectiveMappings.empty() || !m_statesValueMappings.empty() || !m_statesRangeMappings.empty();
}

std::string CapabilitySemantics::toJson() const {
    if (!isValid()) {
        ACSDK_ERROR(LX("toJsonFailed").d("reason", "invalidCapabilitySemantics"));
        return EMPTY_JSON;
    }

    utils::json::JsonGenerator jsonGenerator;
    if (!m_actionsDirectiveMappings.empty()) {
        std::vector<std::string> actionMappingJsonList;
        for (const auto& mapping : m_actionsDirectiveMappings) {
            actionMappingJsonList.push_back(mapping.toJson());
        }
        jsonGenerator.addMembersArray(ACTION_MAPPINGS_KEY, actionMappingJsonList);
    }
    if (!m_statesValueMappings.empty() || !m_statesRangeMappings.empty()) {
        std::vector<std::string> stateMappingJsonList;
        for (const auto& mapping : m_statesValueMappings) {
            stateMappingJsonList.push_back(mapping.toJson());
        }
        for (const auto& mapping : m_statesRangeMappings) {
            stateMappingJsonList.push_back(mapping.toJson());
        }
        jsonGenerator.addMembersArray(STATE_MAPPINGS_KEY, stateMappingJsonList);
    }
    return jsonGenerator.toString();
}

}  // namespace capabilitySemantics
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
