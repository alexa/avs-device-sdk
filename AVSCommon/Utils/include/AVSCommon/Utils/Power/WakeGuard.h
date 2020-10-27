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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_POWER_WAKEGUARD_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_POWER_WAKEGUARD_H_

#include <memory>

#include "AVSCommon/Utils/Power/PowerResource.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace power {

/**
 * A guard object which implements RAII semantics around @c PowerResource management.
 */
class WakeGuard {
public:
    /**
     * Constructor.
     *
     * @param powerResource The @c PowerResource to obtain.
     */
    WakeGuard(std::shared_ptr<PowerResource> powerResource);

    /// Destructor, releases the underlying resource.
    ~WakeGuard();

private:
    /// The @c PowerResource.
    std::shared_ptr<PowerResource> m_powerResource;
};

}  // namespace power
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_POWER_WAKEGUARD_H_
