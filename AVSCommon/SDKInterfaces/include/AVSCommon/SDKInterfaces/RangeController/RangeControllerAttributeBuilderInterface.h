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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_RANGECONTROLLER_RANGECONTROLLERATTRIBUTEBUILDERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_RANGECONTROLLER_RANGECONTROLLERATTRIBUTEBUILDERINTERFACE_H_

#include <AVSCommon/AVS/CapabilityResources.h>
#include <AVSCommon/Utils/Optional.h>

#include "RangeControllerAttributes.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace rangeController {

/**
 * Interface for a range controller attribute builder.
 *
 * The builder is responsible for building a @c RangeControllerAttributes object.
 *
 * @note The following attributes are mandatory and the build will fail if they are missing:
 *   - At least one capability friendly name.
 *   - If presets are available, then their should be preset resources with at least one friendly name for that preset.
 */
class RangeControllerAttributeBuilderInterface {
public:
    /**
     * Destructor.
     */
    virtual ~RangeControllerAttributeBuilderInterface() = default;

    /**
     * Configures builder to use capability resources.
     *
     * @note This will overwrite capability resources if it was used previously.
     *
     * @param capabilityResources The capability resources represented using @c CapabilityResources.
     * @return This builder which can be used to nest configuration function calls.
     */
    virtual RangeControllerAttributeBuilderInterface& withCapabilityResources(
        const avsCommon::avs::CapabilityResources& capabilityResources) = 0;

    /**
     * Configures builder to use a unit of measure used for instance of range controller.
     *
     * @note This will overwrite unitOfMeasure if it was used previously.
     *
     * @param unitOfMeasure The unit of measure of range controller.
     * @return This builder which can be used to nest configuration function calls.
     */
    virtual RangeControllerAttributeBuilderInterface& withUnitOfMeasure(
        const avsCommon::avs::resources::AlexaUnitOfMeasure& unitOfMeasure) = 0;

    /**
     * Adds a preset with preset value and its friendly names defined using @c FriendlyNames.
     *
     * @note Calling this with the same preset value shall overwrite the previous value.
     *
     * @param preset The value of a preset and its preset resources represeneted using @c PresetResources.
     * @return This builder which can be used to nest configuration function calls.
     */
    virtual RangeControllerAttributeBuilderInterface& addPreset(
        const std::pair<double, avsCommon::sdkInterfaces::rangeController::PresetResources>& preset) = 0;

    /**
     * Builds a @c RangeControllerAttributes with the configured properties.
     *
     * @return A non optional @c RangeControllerAttributes if the build succeeds; otherwise, an empty
     * @c RangeControllerAttributes object.
     */
    virtual utils::Optional<RangeControllerAttributes> build() = 0;
};

}  // namespace rangeController
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_RANGECONTROLLER_RANGECONTROLLERATTRIBUTEBUILDERINTERFACE_H_
