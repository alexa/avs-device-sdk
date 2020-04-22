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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MODECONTROLLER_MODECONTROLLERATTRIBUTEBUILDERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MODECONTROLLER_MODECONTROLLERATTRIBUTEBUILDERINTERFACE_H_

#include <AVSCommon/AVS/CapabilityResources.h>
#include <AVSCommon/Utils/Optional.h>

#include "ModeControllerAttributes.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace modeController {

/**
 * Interface for a mode controller attribute builder.
 *
 * The builder is responsible for building a @c ModeControllerAttributes object.
 *
 * @note The attribute builder will fail if following conditions are not met:
 *   - CapabilityResources with at least one friendly name.
 *   - At least one mode with mode resources that contains atleast one friendly name.
 */
class ModeControllerAttributeBuilderInterface {
public:
    /**
     * Destructor.
     */
    virtual ~ModeControllerAttributeBuilderInterface() = default;

    /**
     * Configures builder to use capability resources.
     *
     * @note This will overwrite capability resources if it was used previously.
     *
     * @param capabilityResources The capability resources represented using @c CapabilityResources.
     * @return This builder which can be used to nest configuration function calls.
     */
    virtual ModeControllerAttributeBuilderInterface& withCapabilityResources(
        const avsCommon::avs::CapabilityResources& capabilityResources) = 0;

    /**
     * Add a mode with mode name and its mode resources.
     *
     * @note By default modes added using this method are considered as not ordered. However, to enforce
     * ordering of the modes as the way these are inserted into, call @c setOrdered with @c true.
     *
     * @note Calling this with the same mode name shall overwrite the previous value.
     *
     * @param mode A mode of controller.
     * @param modeResources  The mode resources represeneted using @c ModeResources.
     * @return This builder which can be used to nest configuration function calls.
     */
    virtual ModeControllerAttributeBuilderInterface& addMode(
        const std::string& mode,
        const ModeResources& modeResources) = 0;

    /**
     * Configures builders about the order of the modes in the controller.
     *
     * @note The order here means how the modes are organized in the mode controller.
     * By setting this to @c true, you enable the Alexa to send the @c adjustMode directive.
     * @see https://developer.amazon.com/docs/alexa/alexa-voice-service/alexa-modecontroller.html#capability-assertion
     *
     * @note Calling this again will overrite the previous provided value.
     *
     * @param ordered The ordering of the modes in the controller.
     *
     * @return This builder which can be used to nest configuration function calls.
     */
    virtual ModeControllerAttributeBuilderInterface& setOrdered(bool ordered) = 0;

    /**
     * Builds a @c ModeControllerAttributes with the configured properties.
     *
     * @return A non optional @c ModeControllerAttributes if the build succeeds; otherwise, an empty
     * @c ModeControllerAttributes object.
     */
    virtual utils::Optional<ModeControllerAttributes> build() = 0;
};

}  // namespace modeController
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MODECONTROLLER_MODECONTROLLERATTRIBUTEBUILDERINTERFACE_H_
