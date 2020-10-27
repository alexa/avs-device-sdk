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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYSEMANTICS_STATESTOVALUEMAPPING_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYSEMANTICS_STATESTOVALUEMAPPING_H_

#include <string>
#include <vector>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace capabilitySemantics {

/**
 * This class represents a "StatesToValue" type "stateMapping" in a semantic annotation for a capability primitive.
 *
 * @sa https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/capability-primitives.html#semantic-annotation
 */
class StatesToValueMapping {
public:
    /**
     * The constructor.
     */
    StatesToValueMapping();

    /**
     * Adds the specified state identifier to the "states" array of this mapping object. The state identifier
     * represents utterances that correspond to the value specified in @c setValue().
     *
     * @note See the class-level link for supported state identifiers.
     *
     * @param states The identifier of the state to add to the "states" array.
     * @return @c true if the state was successfully added, else @c false.
     */
    bool addState(const std::string& stateId);

    /**
     * Sets a string value used in this state mapping.
     *
     * @note The accepted value depends on the type of the capability interface to which this semantics object belongs.
     * The @a value must match a configured value of the corresponding reportable state property for the capability.
     * @sa https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/reportable-state-properties.html
     *
     * @param value The value mapped to the state(s).
     * @return @c true if the value was successfully set, else @c false.
     */
    bool setValue(const std::string& value);

    /**
     * Sets a floating point value used in this state mapping.
     *
     * @note This @a value type is only supported for @c RangeController capability instances.
     * The @a value must be between the @c minimumValue and @c maximumValue of @c configuration.supportedRange.
     * @sa https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/alexa-rangecontroller.html
     *
     * @param value The value mapped to the state(s).
     * @return @c true if the value was successfully set, else @c false.
     */
    bool setValue(double value);

    /**
     * Checks whether this @c StatesToValueMapping is valid.
     *
     * @return @c true if valid, else @c false.
     */
    bool isValid() const;

    /**
     * Converts this @c StatesToValueMapping to a JSON string.
     *
     * @note This follows the AVS discovery message format.
     *
     * @return A JSON string of this @c StatesToValueMapping.
     */
    std::string toJson() const;

private:
    /**
     * Returns whether the value for this mapping is initialized.
     *
     * @return @c true if one of the @c setValue() methods was called with a valid value.
     */
    bool isValueSet() const;

    /// Indicates an error in construction.
    bool m_isValid;

    /// List of state IDs used in this mapping.
    std::vector<std::string> m_states;

    /**
     * The value used in this mapping.
     * Only one of @c m_stringValue or @c m_doubleValue should be used per instance.
     */
    std::string m_stringValue;

    /**
     * The value used in this mapping.
     * Only one of @c m_stringValue or @c m_doubleValue should be used per instance.
     */
    double m_doubleValue;
};

}  // namespace capabilitySemantics
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYSEMANTICS_STATESTOVALUEMAPPING_H_
