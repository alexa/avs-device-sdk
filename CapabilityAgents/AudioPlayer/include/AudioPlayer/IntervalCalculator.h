/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AUDIOPLAYER_INCLUDE_AUDIOPLAYER_INTERVALCALCULATOR_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AUDIOPLAYER_INCLUDE_AUDIOPLAYER_INTERVALCALCULATOR_H_

#include <chrono>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace audioPlayer {

/**
 * Calculate the time relative to an offet for a given interval period.  Error cases are negative intervals or offsets
 * and null pointers for the return value.  Additionally, an interval of 0 is illegal.
 *
 * @param interval The period (starting from 0) that the interval occurs at.
 * @param offset The time that has elapsed so far
 * @param [out] intervalStart A pointer to the return value.  It will contain the relative time from the offset when the
 * next interval will start.
 * @return True if successful, false if there is an error.
 */
bool getIntervalStart(
    const std::chrono::milliseconds& interval,
    const std::chrono::milliseconds& offset,
    std::chrono::milliseconds* intervalStart);

}  // namespace audioPlayer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AUDIOPLAYER_INCLUDE_AUDIOPLAYER_INTERVALCALCULATOR_H_
