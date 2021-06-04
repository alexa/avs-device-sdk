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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ENDPOINTS_ENDPOINTMODIFICATIONDATA_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ENDPOINTS_ENDPOINTMODIFICATIONDATA_H_

#include <list>
#include <memory>
#include <vector>

#include <AVSCommon/AVS/AVSDiscoveryEndpointAttributes.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace endpoints {

/**
 * A struct to contain all changes to be made to a given endpoint.
 */
struct EndpointModificationData {
    /// The @c EndpointIdentifier of the given endpoint.
    EndpointIdentifier endpointIdentifier;

    /// The new @c AVSDiscoveryEndpointAttribute.
    /// @note This updatedEndpointAttributes will completely replace the current existing
    /// @c AVSDiscoveryEndpointAttributes of the given endpoint.
    utils::Optional<avs::AVSDiscoveryEndpointAttributes> updatedEndpointAttributes;

    /// The list of modified existing @c CapabilityConfiguration.
    std::vector<avs::CapabilityConfiguration> updatedConfigurations;

    /// The list of capabilities needed to add.
    std::vector<std::pair<avs::CapabilityConfiguration, std::shared_ptr<DirectiveHandlerInterface>>> capabilitiesToAdd;

    /// The list of capabilities needed to remove.
    std::vector<avs::CapabilityConfiguration> capabilitiesToRemove;

    /// The list of newly added capabilities which implement the @c RequireShutdown needed to shut down.
    /// @note This list should be a sub list of capabilitiesToAdd.
    std::list<std::shared_ptr<avsCommon::utils::RequiresShutdown>> capabilitiesToShutDown;

    EndpointModificationData(
        const EndpointIdentifier& endpointIdentifier,
        utils::Optional<avs::AVSDiscoveryEndpointAttributes> updatedEndpointAttributes,
        const std::vector<avs::CapabilityConfiguration>& updatedConfigurations,
        const std::vector<std::pair<avs::CapabilityConfiguration, std::shared_ptr<DirectiveHandlerInterface>>>&
            capabilitiesToAdd,
        const std::vector<avs::CapabilityConfiguration>& capabilitiesToRemove,
        const std::list<std::shared_ptr<avsCommon::utils::RequiresShutdown>>& capabilitiesToShutDown) :
            endpointIdentifier(endpointIdentifier),
            updatedEndpointAttributes(updatedEndpointAttributes),
            updatedConfigurations(updatedConfigurations),
            capabilitiesToAdd(capabilitiesToAdd),
            capabilitiesToRemove(capabilitiesToRemove),
            capabilitiesToShutDown(capabilitiesToShutDown) {
    }
};

}  // namespace endpoints
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ENDPOINTS_ENDPOINTMODIFICATIONDATA_H_