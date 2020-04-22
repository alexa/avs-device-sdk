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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_RANGECONTROLLER_RANGECONTROLLERATTRIBUTES_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_RANGECONTROLLER_RANGECONTROLLERATTRIBUTES_H_

#include <AVSCommon/AVS/CapabilityResources.h>
#include <AVSCommon/AVS/AlexaUnitOfMeasure.h>
#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace rangeController {

/// This describes the friendly names of a preset used in capability discovery message.
using PresetResources = avsCommon::avs::CapabilityResources;

/**
 * Struct to hold the Range Controller attributes required for
 * Capability Agent discovery.
 *
 * @see https://developer.amazon.com/docs/alexa/alexa-voice-service/alexa-rangecontroller.html#capability-assertion
 */
struct RangeControllerAttributes {
    /**
     * Default constructor.
     *
     * @note Avoid using this method. This was added only to enable the to use @c Optional::value().
     */
    RangeControllerAttributes();

    /**
     *  Constructor to build the @c RangeControllerAttributes using provided values.
     *
     *  @param capabilityResources The capability resources.
     *  @param unitOfMeasure The unit of measure of range defined using @c AlexaUnitOfMeasure
     *  @param presets presets of the range controller.
     */
    RangeControllerAttributes(
        const avsCommon::avs::CapabilityResources& capabilityResources,
        const utils::Optional<avsCommon::avs::resources::AlexaUnitOfMeasure>& unitOfMeasure,
        const std::vector<std::pair<double, avsCommon::sdkInterfaces::rangeController::PresetResources>>& presets);

    /// A capability resource @c CapabilityResources
    const avsCommon::avs::CapabilityResources capabilityResources;

    /// The unit of measure used for the range value.
    const utils::Optional<avsCommon::avs::resources::AlexaUnitOfMeasure> unitOfMeasure;

    /**
     * A vector of a pair defining the value of a preset using @c double and its preset resources using @c
     * PresetResources.
     */
    std::vector<std::pair<double, avsCommon::sdkInterfaces::rangeController::PresetResources>> presets;
};

inline RangeControllerAttributes::RangeControllerAttributes() {
}

inline RangeControllerAttributes::RangeControllerAttributes(
    const avsCommon::avs::CapabilityResources& capabilityResources,
    const utils::Optional<avsCommon::avs::resources::AlexaUnitOfMeasure>& unitOfMeasure,
    const std::vector<std::pair<double, avsCommon::sdkInterfaces::rangeController::PresetResources>>& presets) :
        capabilityResources{capabilityResources},
        unitOfMeasure{unitOfMeasure},
        presets{presets} {
}

}  // namespace rangeController
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_RANGECONTROLLER_RANGECONTROLLERATTRIBUTES_H_
