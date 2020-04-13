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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MODECONTROLLER_MODECONTROLLEROBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MODECONTROLLER_MODECONTROLLEROBSERVERINTERFACE_H_

#include <AVSCommon/SDKInterfaces/AlexaStateChangeCauseType.h>
#include <AVSCommon/Utils/Timing/TimePoint.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace modeController {

/**
 * This interface is used to observe changes to the mode property of an endpoint
 * that are caused by the @c ModeControllerInteface.
 */
class ModeControllerObserverInterface {
public:
    /**
     * Struct represents the mode of the endpoint
     */
    struct ModeState {
        /// A string to represent mode.
        std::string mode;

        /// The time at which the mode value was recorded.
        avsCommon::utils::timing::TimePoint timeOfSample;

        /// The number of milliseconds that have elapsed since the mode value was last confrimed.
        std::chrono::milliseconds valueUncertainty;
    };

    /**
     * Destructor.
     */
    virtual ~ModeControllerObserverInterface() = default;

    /**
     * Notifies the change in the mode of the endpoint.
     *
     * @param mode The toggle state specified using @c Mode.
     * @param cause The cause for this change specified using @c AlexaStateChangeCauseType.
     */
    virtual void onModeChanged(const ModeState& mode, AlexaStateChangeCauseType cause) = 0;
};

}  // namespace modeController
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MODECONTROLLER_MODECONTROLLEROBSERVERINTERFACE_H_
