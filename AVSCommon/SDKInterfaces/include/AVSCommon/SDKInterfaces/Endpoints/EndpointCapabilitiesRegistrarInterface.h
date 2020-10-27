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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ENDPOINTS_ENDPOINTCAPABILITIESREGISTRARINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ENDPOINTS_ENDPOINTCAPABILITIESREGISTRARINTERFACE_H_

#include <memory>

#include "AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h"
#include "AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace endpoints {

/**
 * Class to register capabilities with an endpoint being built.
 *
 * @note Capabilities cannot be added to an endpoint that has already been built.
 *
 */
class EndpointCapabilitiesRegistrarInterface {
public:
    /**
     * Destructor.
     */
    virtual ~EndpointCapabilitiesRegistrarInterface() = default;

    /**
     * Adds a capability.
     *
     * @param configuration The capability configuration.
     * @param directiveHandler The handler for the directives in the given namespace.
     * @return This registrar which can be used to nest configuration function calls.
     */
    virtual EndpointCapabilitiesRegistrarInterface& withCapability(
        const avs::CapabilityConfiguration& configuration,
        std::shared_ptr<DirectiveHandlerInterface> directiveHandler) = 0;

    /**
     * Adds a capability.
     *
     * @param configuration The object used to retrieve the capability configurations.
     * @param directiveHandler The handler for the directives in the given namespace.
     * @deprecated CapabilityConfigurationInterface is deprecated.
     * @return This registrar which can be used to nest configuration function calls.
     */
    virtual EndpointCapabilitiesRegistrarInterface& withCapability(
        const std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface>& configurationInterface,
        std::shared_ptr<DirectiveHandlerInterface> directiveHandler) = 0;

    /**
     * Adds a capability configuration that doesn't have any associated @c DirectiveHandler.
     *
     * @param configurationInterface The object used to retrieve the capability configurations.
     * @deprecated CapabilityConfigurationInterface is deprecated.
     * @return This registrar which can be used to nest configuration function calls.
     */
    virtual EndpointCapabilitiesRegistrarInterface& withCapabilityConfiguration(
        const std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface>& configurationInterface) = 0;
};

}  // namespace endpoints
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ENDPOINTS_ENDPOINTCAPABILITIESREGISTRARINTERFACE_H_
