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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYSEMANTICS_STATESTORANGEMAPPING_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYSEMANTICS_STATESTORANGEMAPPING_H_

#include <string>
#include <vector>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace capabilitySemantics {

/**
 * This class represents a "StatesToRange" type "stateMapping" in a semantic annotation for a capability primitive.
 * @sa https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/capability-primitives.html#semantic-annotation
 *
 * @note This mapping may only be used if the corresponding @c CapabilitySemantics belongs to a @c RangeController
 * capability instance.
 */
class StatesToRangeMapping {
public:
    /**
     * The constructor.
     */
    StatesToRangeMapping();

    /**
     * Adds the specified state identifier to the "states" array of this mapping object. The state identifier
     * represents utterances that correspond to the range specified in @c setRange().
     *
     * @note See the class-level link for supported state identifiers.
     *
     * @param states The identifier of the state to add to the "states" array.
     * @return @c true if the state was successfully added, else @c false.
     */
    bool addState(const std::string& stateId);

    /**
     * Sets the range used in this state mapping.
     *
     * @note The @a minValue and @a maxValue of the range must be within @c minimumValue
     * and @c maximumValue configured in @c configuration.supportedRange for this @c RangeController instance.
     *
     * @param minValue The minimum value of the range mapped to the state(s).
     * @param maxValue The minimum value of the range mapped to the state(s).
     * @return @c true if the value was successfully set, else @c false.
     */
    bool setRange(double minValue, double maxValue);

    /**
     * Checks whether this @c StatesToRangeMapping is valid.
     *
     * @return @c true if valid, else @c false.
     */
    bool isValid() const;

    /**
     * Converts this @c StatesToRangeMapping to a JSON string.
     *
     * @note This follows the AVS discovery message format.
     *
     * @return A JSON string of this @c StatesToRangeMapping.
     */
    std::string toJson() const;

private:
    /**
     * Returns whether the range value for this mapping is initialized.
     *
     * @return @c true if @c setRange() was called with a valid value.
     */
    bool isRangeSet() const;

    /// Indicates an error in construction.
    bool m_isValid;

    /// List of state IDs used in this mapping.
    std::vector<std::string> m_states;

    /// The minimum range value used in this mapping.
    double m_minValue;

    /// The maximum range value used in this mapping.
    double m_maxValue;
};

}  // namespace capabilitySemantics
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYSEMANTICS_STATESTORANGEMAPPING_H_
