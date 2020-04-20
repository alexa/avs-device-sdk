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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_RANGECONTROLLER_RANGECONTROLLEROBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_RANGECONTROLLER_RANGECONTROLLEROBSERVERINTERFACE_H_

#include <AVSCommon/SDKInterfaces/AlexaStateChangeCauseType.h>
#include <AVSCommon/Utils/Timing/TimePoint.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace rangeController {

/**
 * This interface is used to observe changes to the range property of an endpoint
 * that are caused by the @c RangeControllerInteface.
 */
class RangeControllerObserverInterface {
public:
    /**
     * Destructor.
     */
    virtual ~RangeControllerObserverInterface() = default;

    /**
     * Struct represents the range state of the endpoint.
     */
    struct RangeState {
        /// A double to represent the range value of instance.
        double value;

        /// The time at which the range value was recorded.
        avsCommon::utils::timing::TimePoint timeOfSample;

        /// The number of milliseconds that have elapsed since the range value was last confrimed.
        std::chrono::milliseconds valueUncertainty;
    };

    /**
     * Notifies the change in the range value of the endpoint.
     *
     * @param rangeState The toggle state specified using @c RangeState.
     * @param cause The cause for this change specified using @c AlexaStateChangeCauseType.
     */
    virtual void onRangeChanged(const RangeState& rangeState, AlexaStateChangeCauseType cause) = 0;
};

}  // namespace rangeController
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_RANGECONTROLLER_RANGECONTROLLEROBSERVERINTERFACE_H_
