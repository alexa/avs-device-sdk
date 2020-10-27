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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYSEMANTICS_CAPABILITYSEMANTICS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYSEMANTICS_CAPABILITYSEMANTICS_H_

#include <string>
#include <vector>

#include "ActionsToDirectiveMapping.h"
#include "StatesToRangeMapping.h"
#include "StatesToValueMapping.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace capabilitySemantics {

/**
 * This class represents the 'semantics' object of a capability primitive definition.
 * @see https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/capability-primitives.html#semantic-annotation
 */
class CapabilitySemantics {
public:
    /**
     * The default constructor.
     */
    CapabilitySemantics() = default;

    /**
     * Adds an @c ActionsToDirective type "actionMapping" to this semantics definition.
     *
     * @note A specific action ID may be used in only one 'actionMappings' object added with this method, but this
     * method does not provide the validation.
     *
     * @param mapping The ActionsToDirective mapping represented as an @c ActionsToDirectiveMapping.
     * @return @c true if adding the mapping was successful, else @c false.
     */
    bool addActionsToDirectiveMapping(const ActionsToDirectiveMapping& mapping);

    /**
     * Adds a @c StatesToValue type "stateMapping" to this semantics definition.
     *
     * @note A specific state ID may be used in only one 'stateMappings' object added with either this method or
     * @c addStatesToRangeMapping(), but this method does not provide the validation.
     *
     * @param mapping The StatesToValue mapping represented as an @c StatesToValueMapping.
     * @return @c true if adding the mapping was successful, else @c false.
     */
    bool addStatesToValueMapping(const StatesToValueMapping& mapping);

    /**
     * Adds a @c StatesToRange type "stateMapping" to this semantics definition.
     *
     * @note A specific state ID may be used in only one 'stateMappings' object added with either this method or
     * @c addStatesToValueMapping(), but this method does not provide the validation.
     *
     * @param mapping The StatesToRange mapping represented as an @c StatesToRangeMapping.
     * @return @c true if adding the mapping was successful, else @c false.
     */
    bool addStatesToRangeMapping(const StatesToRangeMapping& mapping);

    /**
     * Checks if the @c CapabilitySemantics is valid.
     *
     * @return @c true if valid, else @c false.
     */
    bool isValid() const;

    /**
     * Converts this semantics object to a JSON string.
     *
     * @note This follows the AVS discovery message format.
     *
     * @return A JSON string representation of this @c CapabilitySemantics.
     */
    std::string toJson() const;

private:
    /// Vector holding the @c ActionsToDirectiveMapping action mappings.
    std::vector<ActionsToDirectiveMapping> m_actionsDirectiveMappings;

    /// Vector holding the @c StatesToValueMapping state mappings.
    std::vector<StatesToValueMapping> m_statesValueMappings;

    /// Vector holding the @c StatesToRangeMapping state mappings.
    std::vector<StatesToRangeMapping> m_statesRangeMappings;
};

}  // namespace capabilitySemantics
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYSEMANTICS_CAPABILITYSEMANTICS_H_
