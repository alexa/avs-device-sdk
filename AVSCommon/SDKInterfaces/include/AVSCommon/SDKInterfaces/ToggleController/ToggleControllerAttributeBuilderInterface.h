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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_TOGGLECONTROLLER_TOGGLECONTROLLERATTRIBUTEBUILDERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_TOGGLECONTROLLER_TOGGLECONTROLLERATTRIBUTEBUILDERINTERFACE_H_

#include <AVSCommon/AVS/CapabilityResources.h>
#include <AVSCommon/AVS/CapabilitySemantics/CapabilitySemantics.h>
#include <AVSCommon/Utils/Optional.h>

#include "ToggleControllerAttributes.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace toggleController {

/**
 * Interface for a toggle controller attribute builder.
 *
 * The builder is responsible for building a @c ToggleControllerAttributes object.
 *
 * @note The following attributes are mandatory and the build will fail if they are missing:
 *   - At least one capability friendly name
 */
class ToggleControllerAttributeBuilderInterface {
public:
    /**
     * Destructor.
     */
    virtual ~ToggleControllerAttributeBuilderInterface() = default;

    /**
     * Configures builder to use capability resources.
     *
     * @note This will overwrite capability resources if it was used previously.
     *
     * @param capabilityResources The capability resources represented using @c CapabilityResources.
     * @return This builder which can be used to nest configuration function calls.
     */
    virtual ToggleControllerAttributeBuilderInterface& withCapabilityResources(
        const avsCommon::avs::CapabilityResources& capabilityResources) = 0;

    /**
     * Configures the builder to use the specified semantics definition.
     * @see https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/alexa-togglecontroller.html
     *
     * @param semantics The @c CapabilitySemantics representing the semantics definition.
     * @return This builder, which can be used to nest configuration function calls.
     */
    virtual ToggleControllerAttributeBuilderInterface& withSemantics(
        const avsCommon::avs::capabilitySemantics::CapabilitySemantics& semantics) = 0;

    /**
     * Builds a @c ToggleControllerAttributes with the configured properties.
     *
     * @return A non optional @c ToggleControllerAttributes if the build succeeds; otherwise, an empty
     * @c ToggleControllerAttributes object.
     */
    virtual utils::Optional<ToggleControllerAttributes> build() = 0;
};

}  // namespace toggleController
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_TOGGLECONTROLLER_TOGGLECONTROLLERATTRIBUTEBUILDERINTERFACE_H_
