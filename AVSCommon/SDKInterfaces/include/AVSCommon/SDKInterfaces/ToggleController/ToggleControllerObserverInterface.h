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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_TOGGLECONTROLLER_TOGGLECONTROLLEROBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_TOGGLECONTROLLER_TOGGLECONTROLLEROBSERVERINTERFACE_H_

#include <AVSCommon/SDKInterfaces/AlexaStateChangeCauseType.h>
#include <AVSCommon/Utils/Timing/TimePoint.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace toggleController {

/**
 * This interface is used to observe changes to the toggle property of an endpoint
 * that are caused by the @c ToggleControllerInteface.
 */
class ToggleControllerObserverInterface {
public:
    /**
     * Struct that represents the toggle state of the endpoint
     */
    struct ToggleState {
        /// A bool to represent toggle state.
        bool toggleState;

        /// The time at which the toggle state was recorded.
        avsCommon::utils::timing::TimePoint timeOfSample;

        /// The number of milliseconds that have elapsed since the toggle state was last confrimed.
        std::chrono::milliseconds valueUncertainty;
    };

    /**
     * Destructor.
     */
    virtual ~ToggleControllerObserverInterface() = default;

    /**
     * Notifies the change in the toggle state of the endpoint.
     *
     * @param toggleState The toggle state specified using @c ToggleState.
     * @param cause The cause for this change specified using @c AlexaStateChangeCauseType.
     */
    virtual void onToggleStateChanged(const ToggleState& toggleState, AlexaStateChangeCauseType cause) = 0;
};

}  // namespace toggleController
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_TOGGLECONTROLLER_TOGGLECONTROLLEROBSERVERINTERFACE_H_
