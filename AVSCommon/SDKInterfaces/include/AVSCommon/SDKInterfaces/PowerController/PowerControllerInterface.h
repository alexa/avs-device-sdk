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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_POWERCONTROLLER_POWERCONTROLLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_POWERCONTROLLER_POWERCONTROLLERINTERFACE_H_

#include <AVSCommon/AVS/AlexaResponseType.h>
#include <AVSCommon/SDKInterfaces/AlexaStateChangeCauseType.h>
#include <AVSCommon/SDKInterfaces/PowerController/PowerControllerObserverInterface.h>
#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace powerController {

/**
 * The PowerControllerInterface carries out power controller actions such as turning the endpoint ‘ON’ or ‘OFF’.
 *
 * An implementation of the PowerControllerInterface controls the endpoint’s power state and may allow its methods
 * to be called by multiple callers; for example the Alexa Capability Agent or the application’s GUI.
 *
 * @note Implementations of this interface must be thread-safe.
 */
class PowerControllerInterface {
public:
    using PowerState = avsCommon::sdkInterfaces::powerController::PowerControllerObserverInterface::PowerState;

    /**
     * Virtual destructor to assure proper cleanup of derived types.
     */
    virtual ~PowerControllerInterface() = default;

    /**
     * Set the power state of the endpoint.
     *
     * @param powerState The desired power state of the endpoint, @c true indicates 'ON' and @c false as 'OFF'
     * @param cause The cause type for this action represented using @c AlexaStateChangeCauseType.
     * @return A pair of @c AlexaResponseType and string. For the successful operation, the controller should return
     * a pair with @c AlexaResponseType::SUCCESS with an empty string, otherwise returns a pair with the
     * appropriate reason from @c AlexaResponseType and a string with log message that would be logged in
     * cloud for debugging purpose.
     */
    virtual std::pair<avsCommon::avs::AlexaResponseType, std::string> setPowerState(
        bool powerState,
        AlexaStateChangeCauseType cause) = 0;

    /**
     * Get the current power state of the endpoint.
     *
     * @return On successful, the endpoint to return a pair with @c AlexaResponseType::SUCCESS
     * and @c PowerState otherwise returns a pair with the appropriate reason from
     * @c AlexaResponseType and a empty @c PowerState
     */
    virtual std::pair<avsCommon::avs::AlexaResponseType, utils::Optional<PowerState>> getPowerState() = 0;

    /**
     * Adds a @c PowerControllerObserverInterface observer.
     *
     * @note If PowerControllerInterface implementation has configured its instance's property as proactively reported,
     * then it is required to notify observers of @c PowerControllerObserverInterface for any change in its property
     * state. This includes notifying the value when the device starts, if it is different from the last reported value.
     *
     * @param observer The pointer to the @c PowerControllerObserverInterface.
     * @return @c true if the object supports observer notification and observer was successfully added; otherwise,
     * return @c false
     */
    virtual bool addObserver(std::shared_ptr<PowerControllerObserverInterface> observer) = 0;

    /**
     * Removes a observer of @c PowerControllerObserverInterface.
     *
     * @param observer The pointer to the @c PowerControllerObserverInterface.
     */
    virtual void removeObserver(const std::shared_ptr<PowerControllerObserverInterface>& observer) = 0;
};

}  // namespace powerController
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_POWERCONTROLLER_POWERCONTROLLERINTERFACE_H_
