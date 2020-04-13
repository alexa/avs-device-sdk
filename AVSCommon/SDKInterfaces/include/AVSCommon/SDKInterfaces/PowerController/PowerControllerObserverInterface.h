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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_POWERCONTROLLER_POWERCONTROLLEROBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_POWERCONTROLLER_POWERCONTROLLEROBSERVERINTERFACE_H_

#include <chrono>

#include <AVSCommon/SDKInterfaces/AlexaStateChangeCauseType.h>
#include <AVSCommon/Utils/Timing/TimePoint.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace powerController {

/**
 * This interface is used to observe changes to the power state property of an endpoint
 * that are caused by the @c PowerControllerInteface.
 */
class PowerControllerObserverInterface {
public:
    /**
     * Struct that represents the power state of the endpoint.
     */
    struct PowerState {
        /// Represents power state, @c true indicates 'ON' and @c false as 'OFF'
        bool powerState;

        /// Represents time at which the power state value was recorded.
        avsCommon::utils::timing::TimePoint timeOfSample;

        /// The number of milliseconds that have elapsed since the power state value was last confrimed.
        std::chrono::milliseconds valueUncertainty;
    };

    /**
     * Destructor.
     */
    virtual ~PowerControllerObserverInterface() = default;

    /**
     * Notifies the change in the power state property of the endpoint.
     *
     * @param powerState The power state specified using @c PowerState.
     * @param cause The cause for this change specified using @c AlexaStateChangeCauseType.
     */
    virtual void onPowerStateChanged(const PowerState& powerState, AlexaStateChangeCauseType cause) = 0;
};

}  // namespace powerController
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_POWERCONTROLLER_POWERCONTROLLEROBSERVERINTERFACE_H_
