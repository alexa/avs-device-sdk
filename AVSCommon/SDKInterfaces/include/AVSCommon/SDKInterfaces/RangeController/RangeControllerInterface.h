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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_RANGECONTROLLER_RANGECONTROLLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_RANGECONTROLLER_RANGECONTROLLERINTERFACE_H_

#include <AVSCommon/AVS/AlexaResponseType.h>
#include <AVSCommon/SDKInterfaces/AlexaStateChangeCauseType.h>
#include <AVSCommon/SDKInterfaces/RangeController/RangeControllerObserverInterface.h>
#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace rangeController {

/**
 * The RangeControllerInterface carries out range actions on an instance of an endpoint.
 *
 * An implementation of the RangeControllerInterface controls the instance's range  and may allow its methods
 * to be called by multiple callers; for example the Alexa Capability Agent or the application’s GUI.
 *
 * @note Implementations of this interface must be thread-safe.
 */
class RangeControllerInterface {
public:
    /// Alias to improve readability.
    using RangeState = avsCommon::sdkInterfaces::rangeController::RangeControllerObserverInterface::RangeState;

    /**
     * Virtual destructor to assure proper cleanup of derived types.
     */
    virtual ~RangeControllerInterface() = default;

    /**
     * Struct to represent the configuration of the range controller.
     */
    struct RangeControllerConfiguration {
        /// The minimum value of range.
        double minimumValue;

        /// The maximum value of range.
        double maximumValue;

        /// This represents the value to change when moving through the range.
        double precision;
    };

    /**
     * Gets the controller configuration as defined in @c RangeControllerConfiguration
     *
     * @return @c RangeControllerConfiguration
     */
    virtual RangeControllerConfiguration getConfiguration() = 0;

    /**
     * Set the range of the instance.
     *
     * @param range The desired range value of the instance.
     * @param cause The cause type for this action represented using @c AlexaStateChangeCauseType.
     * @return A pair of @c AlexaResponseType and string. For the successful operation, the controller should return
     * a pair with @c AlexaResponseType::SUCCESS with an empty string, otherwise returns a pair with the
     * appropriate reason from @c AlexaResponseType and a description of the error.
     */
    virtual std::pair<avsCommon::avs::AlexaResponseType, std::string> setRangeValue(
        double range,
        AlexaStateChangeCauseType cause) = 0;

    /**
     * Adjust the range of an instance.
     *
     * @param deltaRange The desired delta range of the instance.
     * @param cause The cause type for this action represented using @c AlexaStateChangeCauseType.
     * @return A pair of @c AlexaResponseType and string. For the successful operation, the controller should return
     * a pair with @c AlexaResponseType::SUCCESS with an empty string, otherwise returns a pair with the
     * appropriate reason from @c AlexaResponseType and a description of the error.
     */
    virtual std::pair<avsCommon::avs::AlexaResponseType, std::string> adjustRangeValue(
        double deltaRange,
        AlexaStateChangeCauseType cause) = 0;

    /**
     * Gets the current range state of the instance.
     *
     * @return On successful, the instance to return a pair with @c AlexaResponseType::SUCCESS
     * and @c RangeState otherwise returns a pair with the appropriate reason
     * from @c AlexaResponseType and a empty @c RangeState.
     */
    virtual std::pair<avsCommon::avs::AlexaResponseType, utils::Optional<RangeState>> getRangeState() = 0;

    /**
     * Adds a @c RangeControllerObserverInterface observer.
     *
     * @note If RangeControllerInterface implementation has configured its instance's property as proactively reported,
     * then it is required to notify observers of @c RangeControllerObserverInterface for any change in its property
     * state. This includes notifying the value when the device starts, if it is different from the last reported value.
     *
     * @param observer The pointer to the @c RangeControllerObserverInterface.
     * @return @c true if the object supports observer notification and observer was successfully added; otherwise,
     * return @c false
     */
    virtual bool addObserver(std::shared_ptr<RangeControllerObserverInterface> observer) = 0;

    /**
     * Removes a observer of @c RangeControllerObserverInterface.
     *
     * @param observer The pointer to the @c RangeControllerObserverInterface.
     */
    virtual void removeObserver(const std::shared_ptr<RangeControllerObserverInterface>& observer) = 0;
};

}  // namespace rangeController
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_RANGECONTROLLER_RANGECONTROLLERINTERFACE_H_
