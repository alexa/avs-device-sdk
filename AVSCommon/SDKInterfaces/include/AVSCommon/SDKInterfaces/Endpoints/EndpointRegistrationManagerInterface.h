/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ENDPOINTS_ENDPOINTREGISTRATIONMANAGERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ENDPOINTS_ENDPOINTREGISTRATIONMANAGERINTERFACE_H_

#include <future>
#include <list>
#include <memory>
#include <string>

#include "AVSCommon/AVS/AVSDiscoveryEndpointAttributes.h"
#include "AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h"
#include "AVSCommon/SDKInterfaces/Endpoints/EndpointInterface.h"
#include "AVSCommon/SDKInterfaces/Endpoints/EndpointRegistrationObserverInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace endpoints {

/**
 * Class responsible for managing endpoints handled by this client. This includes:
 *     - Registering the endpoint and its capabilities with AVS (via @c CapabilitiesDelegateInterface).
 *     - Registering endpoint's directive handlers with @c DirectiveSequencerInterface.
 *     - Ensuring endpointId uniqueness across endpoints controlled by this device.
 */
class EndpointRegistrationManagerInterface {
public:
    /// Alias for the registration result type.
    using RegistrationResult = EndpointRegistrationObserverInterface::RegistrationResult;

    /**
     * Destructor
     */
    virtual ~EndpointRegistrationManagerInterface() = default;

    /**
     * Registers an endpoint with AVS and enable control via this client.
     *
     * @param endpoint The endpoint to be registered.
     * @return A future that is set to @c true when the endpoint has been registered and enabled, or that is set to
     * @c false if registration failed.
     * @note This operation is asynchronous. You can also use @c EndpointRegistrationManagerInterface to get notified
     * whenever the operation succeeds or fails.
     */
    virtual std::future<RegistrationResult> registerEndpoint(std::shared_ptr<EndpointInterface> endpoint) = 0;

    /**
     * Adds a registration manager observer to be notified whe a registration has succeeded.
     *
     * @param observer The observer to add.
     */
    virtual void addObserver(std::shared_ptr<EndpointRegistrationObserverInterface> observer) = 0;

    /**
     * Remove a previously registered observer.
     *
     * @param observer The observer to be removed.
     */
    virtual void removeObserver(const std::shared_ptr<EndpointRegistrationObserverInterface>& observer) = 0;
};

}  // namespace endpoints
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ENDPOINTS_ENDPOINTREGISTRATIONMANAGERINTERFACE_H_
