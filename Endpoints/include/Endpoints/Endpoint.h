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

#ifndef ALEXA_CLIENT_SDK_ENDPOINTS_INCLUDE_ENDPOINTS_ENDPOINT_H_
#define ALEXA_CLIENT_SDK_ENDPOINTS_INCLUDE_ENDPOINTS_ENDPOINT_H_

#include <list>
#include <mutex>
#include <set>
#include <unordered_map>

#include <AVSCommon/AVS/AVSDiscoveryEndpointAttributes.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace endpoints {

/**
 * Provides an implementation for @c EndpointInterface.
 */
class Endpoint : public avsCommon::sdkInterfaces::endpoints::EndpointInterface {
public:
    /// Alias to improve readability.
    /// @{
    using EndpointIdentifier = avsCommon::sdkInterfaces::endpoints::EndpointIdentifier;
    using EndpointAttributes = avsCommon::avs::AVSDiscoveryEndpointAttributes;
    /// @}

    /**
     * Constructor.
     *
     * @param attributes The endpoint attributes.
     */
    Endpoint(const EndpointAttributes& attributes);

    /**
     * Destructor.
     */
    ~Endpoint();

    /// @name @c EndpointInterface methods.
    /// @{
    EndpointIdentifier getEndpointId() const override;
    EndpointAttributes getAttributes() const override;
    std::vector<avsCommon::avs::CapabilityConfiguration> getCapabilityConfigurations() const override;
    std::unordered_map<
        avsCommon::avs::CapabilityConfiguration,
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface>>
    getCapabilities() const override;
    bool update(const std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointModificationData>&
                    endpointModificationData) override;
    /// @}

    /**
     * Adds the capability configuration and its directive handler to the endpoint.
     *
     * @param capabilityConfiguration The capability agent configuration.
     * @param directiveHandler The @c DirectiveHandler for this capability.
     * @return True if successful, else false.
     */
    bool addCapability(
        const avsCommon::avs::CapabilityConfiguration& capabilityConfiguration,
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> directiveHandler);

    /**
     * Removes the capability configuration from the endpoint.
     *
     * @param capabilityConfiguration The capability agent configuration.
     * @return True if successful, else false.
     */
    bool removeCapability(const avsCommon::avs::CapabilityConfiguration& capabilityConfiguration);

    /**
     * This method is used to add capability configurations for interfaces that don't have any associated directive.
     *
     * @param capabilityConfiguration
     * @return @c true if successful; @c false otherwise.
     */
    bool addCapabilityConfiguration(const avsCommon::avs::CapabilityConfiguration& capabilityConfiguration);

    /**
     * Validate the updated endpoint attributes.
     *
     * @param updatedAttributes The updated endpoint attributes.
     * @return @c true if valid; @c false otherwise.
     */
    bool validateEndpointAttributes(const EndpointAttributes& updatedAttributes);

    /**
     * This method can be used to add other objects that require shutdown during endpoint destruction.
     *
     * @param requireShutdownObjects The list of objects that require explicit shutdown calls.
     */
    void addRequireShutdownObjects(
        const std::list<std::shared_ptr<avsCommon::utils::RequiresShutdown>>& requireShutdownObjects);

private:
    /// Mutex used to synchronize members(m_capabilities and m_attributes) access.
    mutable std::mutex m_mutex;

    /// The endpoint attributes.
    EndpointAttributes m_attributes;

    /// The map of capabilities and the handlers for their directives.
    std::unordered_map<
        avsCommon::avs::CapabilityConfiguration,
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface>>
        m_capabilities;

    /// The list of objects that require explicit shutdown calls.
    std::set<std::shared_ptr<avsCommon::utils::RequiresShutdown>> m_requireShutdownObjects;
};

}  // namespace endpoints
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ENDPOINTS_INCLUDE_ENDPOINTS_ENDPOINT_H_
