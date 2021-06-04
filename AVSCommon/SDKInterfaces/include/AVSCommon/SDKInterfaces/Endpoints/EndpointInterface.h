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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ENDPOINTS_ENDPOINTINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ENDPOINTS_ENDPOINTINTERFACE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "AVSCommon/AVS/CapabilityConfiguration.h"
#include "AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h"
#include "AVSCommon/AVS/AVSDiscoveryEndpointAttributes.h"
#include "AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h"
#include "AVSCommon/SDKInterfaces/Endpoints/EndpointModificationData.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace endpoints {

/**
 * The interface class for the Endpoint Object.
 */
class EndpointInterface {
public:
    /**
     * Destructor.
     */
    virtual ~EndpointInterface() = default;

    /**
     * The getter for the endpoint identifier.
     *
     * @return The endpoint identifier.
     */
    virtual EndpointIdentifier getEndpointId() const = 0;

    /**
     * The getter for the endpoint attributes.
     *
     * @return The @c EndpointAttributes structure.
     */
    virtual avs::AVSDiscoveryEndpointAttributes getAttributes() const = 0;

    /**
     * The getter for the set of @c CapabilityConfigurations supported by this endpoint.
     *
     * @return The set of @c CapabilityConfiguration supported by the endpoint.
     */
    virtual std::vector<avsCommon::avs::CapabilityConfiguration> getCapabilityConfigurations() const = 0;

    /**
     * The getter for the capabilities that are supported by this endpoint.
     *
     * @return The capabilities supported by the endpoint represented by a map of capability configurations and their
     * respective directive handlers.
     */
    virtual std::unordered_map<
        avsCommon::avs::CapabilityConfiguration,
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface>>
    getCapabilities() const = 0;

    /**
     * Update the Endpoint Object.
     *
     * @param endpointModificationData A pointer to @c EndpointModificationData used to update the endpoint.
     * @return True if successful, else false.
     *
     * @note The implementation of this method should keep thread safe.
     */
    virtual bool update(const std::shared_ptr<EndpointModificationData>& endpointModificationData) = 0;
};

}  // namespace endpoints
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ENDPOINTS_ENDPOINTINTERFACE_H_
