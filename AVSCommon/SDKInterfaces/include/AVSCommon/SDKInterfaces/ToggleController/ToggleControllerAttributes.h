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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_TOGGLECONTROLLER_TOGGLECONTROLLERATTRIBUTES_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_TOGGLECONTROLLER_TOGGLECONTROLLERATTRIBUTES_H_

#include <AVSCommon/AVS/CapabilityResources.h>
#include <AVSCommon/AVS/CapabilitySemantics/CapabilitySemantics.h>
#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace toggleController {

/**
 * The Toggle Controller attributes containing the capability resources required for
 * Capability Agent discovery.
 *
 * @see https://developer.amazon.com/docs/alexa/alexa-voice-service/alexa-togglecontroller.html#capability-assertion
 */
struct ToggleControllerAttributes {
    /**
     * Default constructor.
     *
     * @note Avoid using this method. This was added only to enable the to use @c Optional::value().
     */
    ToggleControllerAttributes() = default;

    /**
     * Constructor to build the @c ToggleControllerAttributes using provided values.
     *
     * @param capabilityResources The capability resources.
     */
    ToggleControllerAttributes(const avsCommon::avs::CapabilityResources& capabilityResources);

    /**
     * Constructor to build the @c ToggleControllerAttributes using provided values.
     *
     * @param capabilityResources The capability resources.
     * @param semantics The optional semantics definition.
     */
    ToggleControllerAttributes(
        const avsCommon::avs::CapabilityResources& capabilityResources,
        avsCommon::utils::Optional<avsCommon::avs::capabilitySemantics::CapabilitySemantics> semantics);

    /// The capability resources as @c CapabilityResources
    const avsCommon::avs::CapabilityResources capabilityResources;

    /// A semantics definition as an @c Optional @c CapabilitySemantics
    avsCommon::utils::Optional<avsCommon::avs::capabilitySemantics::CapabilitySemantics> semantics;
};

inline ToggleControllerAttributes::ToggleControllerAttributes(
    const avsCommon::avs::CapabilityResources& capabilityResources) :
        capabilityResources{capabilityResources},
        semantics{avsCommon::utils::Optional<avsCommon::avs::capabilitySemantics::CapabilitySemantics>()} {
}

inline ToggleControllerAttributes::ToggleControllerAttributes(
    const avsCommon::avs::CapabilityResources& capabilityResources,
    avsCommon::utils::Optional<avsCommon::avs::capabilitySemantics::CapabilitySemantics> semantics) :
        capabilityResources{capabilityResources},
        semantics{semantics} {
}

}  // namespace toggleController
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_TOGGLECONTROLLER_TOGGLECONTROLLERATTRIBUTES_H_
