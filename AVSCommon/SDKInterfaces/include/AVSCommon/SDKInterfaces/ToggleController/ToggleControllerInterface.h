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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_TOGGLECONTROLLER_TOGGLECONTROLLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_TOGGLECONTROLLER_TOGGLECONTROLLERINTERFACE_H_

#include <AVSCommon/AVS/AlexaResponseType.h>
#include <AVSCommon/SDKInterfaces/AlexaStateChangeCauseType.h>
#include <AVSCommon/SDKInterfaces/ToggleController/ToggleControllerObserverInterface.h>
#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace toggleController {

/**
 * The ToggleControllerInterface carries out toggle actions on an instance of an endpoint, such as turning it 'ON or
 * 'OFF'.
 *
 * An implementation of the ToggleControllerInterface controls the instance's toggle state and may allow its methods
 * to be called by multiple callers; for example the Alexa Capability Agent or the application’s GUI.
 *
 * @note Implementations of this interface must be thread-safe.
 */
class ToggleControllerInterface {
public:
    /// Alias to improve readability.
    using ToggleState = avsCommon::sdkInterfaces::toggleController::ToggleControllerObserverInterface::ToggleState;

    /**
     * Virtual destructor to assure proper cleanup of derived types.
     */
    virtual ~ToggleControllerInterface() = default;

    /**
     * Set the toggle state of an instance.
     *
     * @param state The desired toggle state of the instance @c true indicates 'ON' and @c false as 'OFF'
     * @param cause The cause type for this action represented using @c AlexaStateChangeCauseType.
     * @return A pair of @c AlexaResponseType and string. For the successful operation, the controller should return
     * a pair with @c AlexaResponseType::SUCCESS with an empty string, otherwise returns a pair with the
     * appropriate reason from @c AlexaResponseType and a description of the error.
     */
    virtual std::pair<avsCommon::avs::AlexaResponseType, std::string> setToggleState(
        bool state,
        AlexaStateChangeCauseType cause) = 0;

    /**
     * Gets the current toggle state of an instance.
     *
     * @return On successful, the instance to return a pair with @c AlexaResponseType::SUCCESS
     * and @c ToggleState otherwise returns a pair with the appropriate reason
     * from @c AlexaResponseType and a empty @c ToggleState.
     */
    virtual std::pair<avsCommon::avs::AlexaResponseType, utils::Optional<ToggleState>> getToggleState() = 0;

    /**
     * Adds a @c ToggleControllerObserverInterface observer.
     *
     * @note If ToggleControllerInterface implementation has configured its instance's property as proactively reported,
     * then it is required to notify observers of @c ToggleControllerObserverInterface for any change in its property
     * state. This includes notifying the value when the device starts, if it is different from the last reported value.
     *
     * @param observer The pointer to the @c ToggleControllerObserverInterface.
     * @return @c true if the object supports observer notification and observer was successfully added; otherwise,
     * return @c false
     */
    virtual bool addObserver(std::shared_ptr<ToggleControllerObserverInterface> observer) = 0;

    /**
     * Removes a observer of @c ToggleControllerObserverInterface.
     *
     * @param observer The pointer to the @c ToggleControllerObserverInterface.
     */
    virtual void removeObserver(const std::shared_ptr<ToggleControllerObserverInterface>& observer) = 0;
};

}  // namespace toggleController
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_TOGGLECONTROLLER_TOGGLECONTROLLERINTERFACE_H_
