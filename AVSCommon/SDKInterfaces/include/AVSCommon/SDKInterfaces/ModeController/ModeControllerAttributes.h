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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MODECONTROLLER_MODECONTROLLERATTRIBUTES_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MODECONTROLLER_MODECONTROLLERATTRIBUTES_H_

#include <unordered_map>

#include <AVSCommon/AVS/CapabilityResources.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace modeController {

/// This describes the friendly names of a mode used in capability discovery message.
using ModeResources = avsCommon::avs::CapabilityResources;

/**
 * Struct to hold the Mode Controller attributes required for
 * Capability Agent discovery.
 *
 * @see https://developer.amazon.com/docs/alexa/alexa-voice-service/alexa-modecontroller.html#capability-assertion
 */
struct ModeControllerAttributes {
    /**
     * Default constructor.
     *
     * @note Avoid using this method. This was added only to enable the to use @c Optional::value().
     */
    ModeControllerAttributes();

    /**
     *  Constructor to build the @c ModeControllerAttributes using provided values.
     *
     *  @param capabilityResources The capability resources.
     *  @param modes A map of controller modes and mode resources.
     *  @param ordered A boolean indicating the ordering of @c modes.
     */
    ModeControllerAttributes(
        const avsCommon::avs::CapabilityResources& capabilityResources,
        const std::unordered_map<std::string, ModeResources>& modes,
        bool ordered);

    /// A capability resource @c CapabilityResources
    const avsCommon::avs::CapabilityResources capabilityResources;

    /// A map of controller supported modes @c string and their mode resource using @c ModeResources
    const std::unordered_map<std::string, ModeResources> modes;

    /// If @c true indicates that the modes in @c modes are ordered.
    const bool ordered;
};

inline ModeControllerAttributes::ModeControllerAttributes() :
        ModeControllerAttributes(
            avsCommon::avs::CapabilityResources(),
            std::unordered_map<std::string, ModeResources>(),
            false) {
}

inline ModeControllerAttributes::ModeControllerAttributes(
    const avsCommon::avs::CapabilityResources& capabilityResources,
    const std::unordered_map<std::string, ModeResources>& modes,
    bool ordered) :
        capabilityResources{capabilityResources},
        modes{modes},
        ordered{ordered} {
}

}  // namespace modeController
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MODECONTROLLER_MODECONTROLLERATTRIBUTES_H_
