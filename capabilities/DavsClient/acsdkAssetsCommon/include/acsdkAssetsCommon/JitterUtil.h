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

#ifndef ACSDKASSETSCOMMON_JITTERUTIL_H_
#define ACSDKASSETSCOMMON_JITTERUTIL_H_

#include <chrono>

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {
namespace jitterUtil {

/**
 * Provides a time delay off baseValue with some jitteriness
 * @param baseValue value to base off of
 * @param jitterFactor factor of jitter from baseValue, 0 > and < 1
 * @return
 */
std::chrono::milliseconds jitter(std::chrono::milliseconds baseValue, float jitterFactor = 0.2);

/**
 * Provides a time delay off baseValue with some jitteriness and 2x exponential back-off
 * @param baseValue value to base off of
 * @param jitterFactor exponentially increases baseValue by this amount with jitter
 * @return
 */
std::chrono::milliseconds expJitter(std::chrono::milliseconds baseValue, float jitterFactor = 0.2);

}  // namespace jitterUtil
}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETSCOMMON_JITTERUTIL_H_
