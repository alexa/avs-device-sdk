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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ENDPOINTS_ENDPOINTREGISTRATIONOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ENDPOINTS_ENDPOINTREGISTRATIONOBSERVERINTERFACE_H_

#include "AVSCommon/AVS/AVSDiscoveryEndpointAttributes.h"
#include "AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace endpoints {

/**
 * Interface that can be implemented in order to receive notifications about changes in endpoints registration.
 */
class EndpointRegistrationObserverInterface {
public:
    /**
     * Enumeration of possible registration result.
     */
    enum class RegistrationResult {
        /// Registration succeeded.
        SUCCEEDED,
        /// Registration failed because endpoint has been enqueued for registration.
        PENDING_REGISTRATION,
        /// Registration failed because endpoint has already been registered.
        ALREADY_REGISTERED,
        /// Registration failed due to some configuration error.
        CONFIGURATION_ERROR,
        /// We currently only support endpoint registration before the client has connected to AVS.
        REGISTRATION_DISABLED,
        /// Registration failed due to internal error.
        INTERNAL_ERROR
    };

    /**
     * Destructor.
     */
    virtual ~EndpointRegistrationObserverInterface() = default;

    /**
     * Notifies observer that a new endpoint registration has been processed.
     *
     * @param endpointId The endpoint identifier.
     * @param attributes The endpoint attributes.
     * @param result The final registration result.
     */
    virtual void onEndpointRegistration(
        const EndpointIdentifier endpointId,
        const avs::AVSDiscoveryEndpointAttributes& attributes,
        const RegistrationResult result) = 0;
};

}  // namespace endpoints
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ENDPOINTS_ENDPOINTREGISTRATIONOBSERVERINTERFACE_H_
