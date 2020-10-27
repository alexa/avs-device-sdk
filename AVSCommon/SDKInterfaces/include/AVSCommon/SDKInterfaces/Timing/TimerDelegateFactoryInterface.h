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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_TIMING_TIMERDELEGATEFACTORYINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_TIMING_TIMERDELEGATEFACTORYINTERFACE_H_

#include <memory>

#include <AVSCommon/SDKInterfaces/Timing/TimerDelegateInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace timing {

/// A factory for creating @c TimerDelegateInterface.
class TimerDelegateFactoryInterface {
public:
    /// Destructor.
    virtual ~TimerDelegateFactoryInterface() = default;

    /**
     * Whether or not this @c TimerDelegateInterface can operate when the system is running in a reduced power mode.
     * Examples of this include an implementation which uses a Real Time Clock.
     *
     * @return A boolean whether this is supported.
     */
    virtual bool supportsLowPowerMode() = 0;

    /**
     * Create an instance of a @c TimerDelegateInterface. This must be non-null.
     *
     * @return A ptr to a @c TimerDelegateInterface.
     */
    virtual std::unique_ptr<TimerDelegateInterface> getTimerDelegate() = 0;
};

}  // namespace timing
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_TIMING_TIMERDELEGATEFACTORYINTERFACE_H_
