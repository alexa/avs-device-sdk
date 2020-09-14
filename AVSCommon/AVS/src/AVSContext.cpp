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

#include "AVSCommon/AVS/AVSContext.h"
#include "AVSCommon/AVS/EventBuilder.h"
#include "AVSCommon/Utils/JSON/JSONGenerator.h"
#include "AVSCommon/Utils/Logger/Logger.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/// Key used to identify the context properties.
static const std::string PROPERTIES_KEY_STRING = "properties";

/// Key used to identify an instance.
static const std::string INSTANCE_KEY_STRING = "instance";

/// Key used to identify the state value.
static const std::string VALUE_KEY_STRING = "value";

/// Key used to identify the time of sample.
static const std::string TIME_OF_SAMPLE_KEY_STRING = "timeOfSample";

/// Key used to identify an the uncertainty in milliseconds related to the time of sample. For more information:
/// https://developer.amazon.com/docs/alexa/alexa-voice-service/reportable-state-properties.html#property-object
static const std::string UNCERTAINTY_KEY_STRING = "uncertaintyInMilliseconds";

/// String to identify log entries originating from this file.
static const std::string TAG("AVSContext");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) utils::logger::LogEntry(TAG, event)

utils::Optional<CapabilityState> AVSContext::getState(const CapabilityTag& identifier) const {
    auto it = m_states.find(identifier);
    return (it != m_states.end()) ? utils::Optional<CapabilityState>(it->second) : utils::Optional<CapabilityState>();
}

std::map<CapabilityTag, CapabilityState> AVSContext::getStates() const {
    return m_states;
}

void AVSContext::addState(const CapabilityTag& identifier, const CapabilityState& state) {
    m_states.insert(std::make_pair(identifier, state));
}

void AVSContext::removeState(const CapabilityTag& identifier) {
    m_states.erase(identifier);
}

std::string AVSContext::toJson() const {
    utils::json::JsonGenerator jsonGenerator;
    jsonGenerator.startArray(PROPERTIES_KEY_STRING);
    for (const auto& element : m_states) {
        auto& identifier = element.first;
        auto& state = element.second;
        if (!state.valuePayload.empty()) {
            jsonGenerator.startArrayElement();
            jsonGenerator.addMember(constants::NAMESPACE_KEY_STRING, identifier.nameSpace);
            jsonGenerator.addMember(constants::NAME_KEY_STRING, identifier.name);
            if (identifier.instance.hasValue()) {
                jsonGenerator.addMember(INSTANCE_KEY_STRING, identifier.instance.value());
            }

            jsonGenerator.addRawJsonMember(VALUE_KEY_STRING, state.valuePayload);
            jsonGenerator.addMember(TIME_OF_SAMPLE_KEY_STRING, state.timeOfSample.getTime_ISO_8601());
            jsonGenerator.addMember(UNCERTAINTY_KEY_STRING, state.uncertaintyInMilliseconds);
            jsonGenerator.finishArrayElement();
        } else {
            ACSDK_DEBUG0(LX(__func__).d("stateIgnored", identifier.nameSpace + "::" + identifier.name));
        }
    }
    jsonGenerator.finishArray();
    ACSDK_DEBUG5(LX(__func__).sensitive("context", jsonGenerator.toString()));
    return jsonGenerator.toString();
}
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
