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

#include <AVSCommon/AVS/CapabilitySemantics/ActionsToDirectiveMapping.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include <algorithm>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace capabilitySemantics {

/// The "@type" key of mapping object JSON.
static const std::string TYPE_KEY("@type");

/// The "ActionsToDirective" value of mapping object JSON.
static const std::string MAPPING_TYPE_VALUE("ActionsToDirective");

/// The "actions" key of mapping object JSON.
static const std::string ACTIONS_KEY("actions");

/// The "directive" key of mapping object JSON.
static const std::string DIRECTIVE_KEY("directive");

/// The "name" key of mapping object JSON.
static const std::string NAME_KEY("name");

/// The "payload" key of mapping object JSON.
static const std::string PAYLOAD_KEY("payload");

/// Empty JSON object.
static const std::string EMPTY_JSON("{}");

/// String to identify log entries originating from this file.
#define TAG "ActionsToDirectiveMapping"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) utils::logger::LogEntry(TAG, event)

ActionsToDirectiveMapping::ActionsToDirectiveMapping() : m_isValid{true} {
}

bool ActionsToDirectiveMapping::addAction(const std::string& actionId) {
    if (actionId.empty()) {
        ACSDK_ERROR(LX("addActionFailed").d("reason", "emptyActionId"));
        m_isValid = false;
        return false;
    }
    if (std::find(m_actions.begin(), m_actions.end(), actionId) != m_actions.end()) {
        ACSDK_ERROR(LX("addActionFailed").d("reason", "duplicateActionId").d("actionId", actionId));
        m_isValid = false;
        return false;
    }
    m_actions.push_back(actionId);
    return true;
}

bool ActionsToDirectiveMapping::setDirective(const std::string& directive, const std::string& payload) {
    if (directive.empty()) {
        ACSDK_ERROR(LX("setDirectiveFailed").d("reason", "emptyDirectiveName"));
        m_isValid = false;
        return false;
    }
    if (payload.empty()) {
        ACSDK_ERROR(LX("setDirectiveFailed").d("reason", "emptyPayload"));
        m_isValid = false;
        return false;
    }
    if (!m_directiveName.empty()) {
        ACSDK_ERROR(LX("setDirectiveFailed").d("directive", directive).d("reason", "directiveAlreadySet"));
        m_isValid = false;
        return false;
    }
    m_directiveName = directive;
    m_directivePayload = payload;
    return true;
}

bool ActionsToDirectiveMapping::isValid() const {
    return m_isValid && !m_actions.empty() && !m_directiveName.empty();
}

std::string ActionsToDirectiveMapping::toJson() const {
    if (!isValid()) {
        ACSDK_ERROR(LX("toJsonFailed").d("reason", "invalidActionsToDirectiveMapping"));
        return EMPTY_JSON;
    }

    utils::json::JsonGenerator jsonGenerator;
    jsonGenerator.addMember(TYPE_KEY, MAPPING_TYPE_VALUE);
    jsonGenerator.addStringArray(ACTIONS_KEY, m_actions);
    jsonGenerator.startObject(DIRECTIVE_KEY);
    jsonGenerator.addMember(NAME_KEY, m_directiveName);
    bool parseSuccess = jsonGenerator.addRawJsonMember(PAYLOAD_KEY, m_directivePayload, true);
    if (!parseSuccess) {
        ACSDK_ERROR(LX("toJsonFailed").d("reason", "payloadNotValidJson"));
        return EMPTY_JSON;
    }
    jsonGenerator.finishObject();  // directive
    return jsonGenerator.toString();
}

}  // namespace capabilitySemantics
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
