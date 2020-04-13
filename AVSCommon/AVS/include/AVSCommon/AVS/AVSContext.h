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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_AVSCONTEXT_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_AVSCONTEXT_H_

#include <map>
#include <ostream>
#include <string>

#include "AVSCommon/AVS/CapabilityTag.h"
#include "AVSCommon/AVS/CapabilityState.h"
#include "AVSCommon/Utils/Optional.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/**
 * The @c AVSContext represents a map where the key is the capabilities message identifier, which represents a unique
 * property in the device, and the value is their current state.
 *
 * @note This class is not thread safe.
 */
class AVSContext {
public:
    /// Alias the map of states.
    using States = std::map<CapabilityTag, CapabilityState>;

    /**
     * Constructor.
     */
    AVSContext() = default;

    /**
     * Return a stringified json representation of @c AVSContext value.
     *
     * @return A stringified json following AVS format specification.
     */
    std::string toJson() const;

    /**
     * Get all states available in this context.
     *
     * @return A map with all the states available in this context.
     */
    States getStates() const;

    /**
     * Get state of an specific capability if available.
     *
     * @param identifier The target capability identifier.
     * @return The capability state if available; otherwise, an empty object.
     */
    utils::Optional<CapabilityState> getState(const CapabilityTag& identifier) const;

    /**
     * Add the state for an specific capability.
     *
     * @param identifier The target capability identifier.
     * @param state The state to be saved.
     *
     * @note If the context already has a state for the given capability, this function will overwrite the
     * existing state.
     */
    void addState(const CapabilityTag& identifier, const CapabilityState& state);

    /**
     * Remove the state of an specific capability.
     *
     * @param identifier The target capability identifier.
     */
    void removeState(const CapabilityTag& identifier);

private:
    /// A map of capabilities and their state.
    States m_states;
};

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_AVSCONTEXT_H_
