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

#include <AVSCommon/AVS/CapabilitySemantics/StatesToValueMapping.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include <algorithm>
#include <limits>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace capabilitySemantics {

/// The "@type" key of mapping object JSON.
static const std::string TYPE_KEY("@type");

/// The "StatesToValue" mapping type.
static const std::string MAPPING_TYPE_VALUE("StatesToValue");

/// The "states" key of mapping object JSON.
static const std::string STATES_KEY("states");

/// The "value" key of mapping object JSON.
static const std::string VALUE_KEY("value");

/// Indicates @c m_doubleValue  is not initialized.
static constexpr double UNINITIALIZED_DOUBLE = std::numeric_limits<double>::min();

/// Indicates @c m_stringValue is not initialized.
static std::string UNINITIALIZED_STRING = "";

/// Empty JSON object.
static const std::string EMPTY_JSON("{}");

/// String to identify log entries originating from this file.
#define TAG "StatesToValueMapping"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) utils::logger::LogEntry(TAG, event)

StatesToValueMapping::StatesToValueMapping() :
        m_isValid{true},
        m_stringValue{UNINITIALIZED_STRING},
        m_doubleValue{UNINITIALIZED_DOUBLE} {
}

bool StatesToValueMapping::addState(const std::string& stateId) {
    if (stateId.empty()) {
        ACSDK_ERROR(LX("addStateFailed").d("reason", "emptyStateId"));
        m_isValid = false;
        return false;
    }
    if (std::find(m_states.begin(), m_states.end(), stateId) != m_states.end()) {
        ACSDK_ERROR(LX("addStateFailed").d("reason", "duplicateStateId").d("stateId", stateId));
        m_isValid = false;
        return false;
    }
    m_states.push_back(stateId);
    return true;
}

bool StatesToValueMapping::setValue(const std::string& value) {
    if (isValueSet()) {
        ACSDK_ERROR(LX("setValueFailed").d("reason", "valueAlreadySet"));
        m_isValid = false;
        return false;
    }
    if (value.empty()) {
        ACSDK_ERROR(LX("setValueFailed").d("reason", "emptyValue"));
        m_isValid = false;
        return false;
    }
    m_stringValue = value;
    return true;
}

bool StatesToValueMapping::setValue(double value) {
    if (isValueSet()) {
        ACSDK_ERROR(LX("setValueFailed").d("reason", "valueAlreadySet"));
        m_isValid = false;
        return false;
    }
    m_doubleValue = value;
    return true;
}

bool StatesToValueMapping::isValid() const {
    return m_isValid && isValueSet() && !m_states.empty();
}

std::string StatesToValueMapping::toJson() const {
    if (!isValid()) {
        ACSDK_ERROR(LX("toJsonFailed").d("reason", "invalidStatesToValueMapping"));
        return EMPTY_JSON;
    }

    utils::json::JsonGenerator jsonGenerator;
    jsonGenerator.addMember(TYPE_KEY, MAPPING_TYPE_VALUE);
    jsonGenerator.addStringArray(STATES_KEY, m_states);
    if (m_stringValue != UNINITIALIZED_STRING) {
        jsonGenerator.addMember(VALUE_KEY, m_stringValue);
    } else {
        jsonGenerator.addMember(VALUE_KEY, m_doubleValue);
    }
    return jsonGenerator.toString();
}

bool StatesToValueMapping::isValueSet() const {
    return m_stringValue != UNINITIALIZED_STRING || m_doubleValue != UNINITIALIZED_DOUBLE;
}

}  // namespace capabilitySemantics
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
