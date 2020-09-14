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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYSTATE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYSTATE_H_

#include <cstdint>
#include <string>

#include <AVSCommon/Utils/Timing/TimePoint.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/**
 * This structure represents the state of a capability, including the time that this state was reported and the
 * uncertaintyInMilliseconds.
 */
struct CapabilityState {
    /**
     * The Constructor.
     *
     * @param valuePayload A string representation of the value payload.
     * @param timeOfSample The time at which the property value was recorded.
     * @param uncertaintyInMilliseconds The number of milliseconds that have elapsed since the property value was last
     * confirmed.
     */
    CapabilityState(
        const std::string& valuePayload,
        const utils::timing::TimePoint& timeOfSample = utils::timing::TimePoint::now(),
        const uint32_t uncertaintyInMilliseconds = 0);

    /**
     * Copy constructor.
     *
     * @param other The state to copy from.
     */
    CapabilityState(const CapabilityState& other) = default;

    /**
     * Default constructor.
     *
     * @note Avoid using this method. This was added only to enable the to use @c Optional::value().
     */
    CapabilityState() = default;

    /**
     * Assignment operator.
     *
     * @param other The state to copy from.
     */
    CapabilityState& operator=(const CapabilityState& other) = default;

    /**
     * Equality operators.
     * @param rhs The element to compare to @c this.
     * @return Whether the operation holds.
     */
    /// @{
    inline bool operator==(const CapabilityState& rhs) const;
    inline bool operator!=(const CapabilityState& rhs) const;
    /// @}

    /// A string representation of the value payload.
    std::string valuePayload;

    /// The time at which the property value was recorded.
    utils::timing::TimePoint timeOfSample;

    /// The number of milliseconds that have elapsed since the property value was last confirmed.
    uint32_t uncertaintyInMilliseconds;
};

bool CapabilityState::operator==(const CapabilityState& rhs) const {
    auto thisTime = timeOfSample.getTime_Unix();
    auto rhsTime = rhs.timeOfSample.getTime_Unix();
    return std::tie(valuePayload, thisTime, uncertaintyInMilliseconds) ==
           std::tie(rhs.valuePayload, rhsTime, rhs.uncertaintyInMilliseconds);
}
bool CapabilityState::operator!=(const CapabilityState& rhs) const {
    return !(rhs == *this);
}

inline CapabilityState::CapabilityState(
    const std::string& valuePayload,
    const utils::timing::TimePoint& timeOfSample,
    const uint32_t uncertaintyInMilliseconds) :
        valuePayload(valuePayload),
        timeOfSample(timeOfSample),
        uncertaintyInMilliseconds(uncertaintyInMilliseconds) {
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYSTATE_H_
