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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MODECONTROLLER_MODECONTROLLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MODECONTROLLER_MODECONTROLLERINTERFACE_H_

#include <AVSCommon/AVS/AlexaResponseType.h>
#include <AVSCommon/SDKInterfaces/AlexaStateChangeCauseType.h>
#include <AVSCommon/SDKInterfaces/ModeController/ModeControllerObserverInterface.h>
#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace modeController {

/**
 * The ModeControllerInterface carries out mode actions on an instance of an endpoint.
 *
 * An implementation of the ModeControllerInterface controls the instance's mode and may allow its methods
 * to be called by multiple callers; for example the Alexa Capability Agent or the application’s GUI.
 *
 * @note Implementations of this interface must be thread-safe.
 */
class ModeControllerInterface {
public:
    /// Alias to improve readability.
    using ModeState = avsCommon::sdkInterfaces::modeController::ModeControllerObserverInterface::ModeState;

    /**
     * Virtual destructor to assure proper cleanup of derived types.
     */
    virtual ~ModeControllerInterface() = default;

    /**
     * The configuration of the mode controller that represents the supported modes as
     * a vector of @c string
     */
    using ModeControllerConfiguration = std::vector<std::string>;

    /**
     * Gets the controller configuration as defined @c ModeControllerConfiguration
     *
     * @return @c ModeControllerConfiguration
     */
    virtual ModeControllerConfiguration getConfiguration() = 0;

    /**
     * Set the mode of the instance.
     *
     * @param mode The desired mode of the instance.
     * @param cause The cause type for this action represented using @c AlexaStateChangeCauseType.
     * @return A pair of @c AlexaResponseType and string. For the successful operation, the controller should return
     * a pair with @c AlexaResponseType::SUCCESS with an empty string, otherwise returns a pair with the
     * appropriate reason from @c AlexaResponseType and a description of the error.
     */
    virtual std::pair<avsCommon::avs::AlexaResponseType, std::string> setMode(
        const std::string& mode,
        AlexaStateChangeCauseType cause) = 0;

    /**
     * Adjust the mode of an instance.
     *
     * @param modeDelta The delta by which the controller mode should be changed (only applicable if the
     * mode controller is ordered).
     * @param cause The appropriate @c AlexaStateChangeCauseType for this change.
     * @return A pair of @c AlexaResponseType and string. For the successful operation, the controller should return
     * a pair of @c AlexaResponseType::SUCCESS with an empty string, otherwise it returns pair with the appropriate
     * reason from @c AlexaResponseType and a description of the error.
     */
    virtual std::pair<avsCommon::avs::AlexaResponseType, std::string> adjustMode(
        int modeDelta,
        AlexaStateChangeCauseType cause) = 0;

    /**
     * Gets the current mode of the instance.
     *
     * @return On successful, the instance to return a pair with @c AlexaResponseType::SUCCESS
     * and @c ModeState otherwise returns a pair with the appropriate reason
     * from @c AlexaResponseType and an empty @c ModeState.
     */
    virtual std::pair<avsCommon::avs::AlexaResponseType, utils::Optional<ModeState>> getMode() = 0;

    /**
     * Adds a @c ModeControllerObserverInterface observer
     *
     * @note If ModeControllerInterface implementation has configured its instance's property as proactively reported,
     * then it is required to notify observers of @c ModeControllerObserverInterface for any change in its property
     * state. This includes notifying the value when the device starts, if it is different from the last reported value.
     *
     * @param observer The pointer to the @c ModeControllerObserverInterface.
     * @return @c true if the object supports observer notification and observer was successfully added; otherwise,
     * return @c false
     */
    virtual bool addObserver(std::shared_ptr<ModeControllerObserverInterface> observer) = 0;

    /**
     * Removes an observer.
     *
     * @param observer The pointer to the @c ModeControllerObserverInterface.
     */
    virtual void removeObserver(const std::shared_ptr<ModeControllerObserverInterface>& observer) = 0;
};

}  // namespace modeController
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MODECONTROLLER_MODECONTROLLERINTERFACE_H_
