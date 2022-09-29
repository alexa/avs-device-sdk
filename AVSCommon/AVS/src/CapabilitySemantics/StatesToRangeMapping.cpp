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

#include <AVSCommon/AVS/CapabilitySemantics/StatesToRangeMapping.h>
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

/// The "StatesToRange" mapping type.
static const std::string MAPPING_TYPE_VALUE("StatesToRange");

/// The "states" key of mapping object JSON.
static const std::string STATES_KEY("states");

/// The "range" key of mapping object JSON.
static const std::string RANGE_KEY("range");

/// The "minimumValue" key of mapping object JSON.
static const std::string MIN_VALUE_KEY("minimumValue");

/// The "maximumValue" key of mapping object JSON.
static const std::string MAX_VALUE_KEY("maximumValue");

/// Indicates @c m_minValue or @c m_maxValue is not initialized.
static constexpr double UNINITIALIZED_DOUBLE = std::numeric_limits<double>::min();

/// Empty JSON object.
static const std::string EMPTY_JSON("{}");

/// String to identify log entries originating from this file.
#define TAG "StatesToRangeMapping"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) utils::logger::LogEntry(TAG, event)

StatesToRangeMapping::StatesToRangeMapping() :
        m_isValid{true},
        m_minValue{UNINITIALIZED_DOUBLE},
        m_maxValue{UNINITIALIZED_DOUBLE} {
}

bool StatesToRangeMapping::addState(const std::string& stateId) {
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

bool StatesToRangeMapping::setRange(double minValue, double maxValue) {
    if (isRangeSet()) {
        ACSDK_ERROR(LX("setRangeFailed").d("reason", "rangeAlreadySet"));
        m_isValid = false;
        return false;
    }
    if (minValue > maxValue) {
        ACSDK_ERROR(LX("setRangeFailed").d("reason", "minGreaterThanMax"));
        m_isValid = false;
        return false;
    }
    m_minValue = minValue;
    m_maxValue = maxValue;
    return true;
}

bool StatesToRangeMapping::isValid() const {
    return m_isValid && isRangeSet() && !m_states.empty();
}

std::string StatesToRangeMapping::toJson() const {
    if (!isValid()) {
        ACSDK_ERROR(LX("toJsonFailed").d("reason", "invalidStatesToRangeMapping"));
        return EMPTY_JSON;
    }

    utils::json::JsonGenerator jsonGenerator;
    jsonGenerator.addMember(TYPE_KEY, MAPPING_TYPE_VALUE);
    jsonGenerator.addStringArray(STATES_KEY, m_states);
    jsonGenerator.startObject(RANGE_KEY);
    jsonGenerator.addMember(MIN_VALUE_KEY, m_minValue);
    jsonGenerator.addMember(MAX_VALUE_KEY, m_maxValue);
    jsonGenerator.finishObject();
    return jsonGenerator.toString();
}

bool StatesToRangeMapping::isRangeSet() const {
    return m_minValue != UNINITIALIZED_DOUBLE && m_maxValue != UNINITIALIZED_DOUBLE;
}

}  // namespace capabilitySemantics
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
