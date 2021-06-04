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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ENDPOINTS_ENDPOINTREGISTRATIONOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ENDPOINTS_ENDPOINTREGISTRATIONOBSERVERINTERFACE_H_

#include "AVSCommon/AVS/AVSDiscoveryEndpointAttributes.h"
#include "AVSCommon/AVS/CapabilityConfiguration.h"
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
     * Enumeration of possible registration results.
     */
    enum class RegistrationResult {
        /// Registration succeeded.
        SUCCEEDED,
        /// Registration failed due to some configuration error.
        CONFIGURATION_ERROR,
        /// Registration failed due to internal error.
        INTERNAL_ERROR,
        /// Registration failed because the endpoint is being registered.
        PENDING_REGISTRATION,
        /// Registration failed because the endpoint is being deregistered.
        PENDING_DEREGISTRATION,
        /// Registration failed because the endpoint is being updated.
        PENDING_UPDATE,
        /// Registration failed because the endpoint has already been registered
        ALREADY_REGISTERED
    };

    /**
     * Enumeration of possible update results.
     */
    enum class UpdateResult {
        /// Update Succeeded.
        SUCCEEDED,
        /// Update failed due to some configuration error.
        CONFIGURATION_ERROR,
        /// Update failed due to internal error.
        INTERNAL_ERROR,
        /// Update failed due to the endpoint not being registered yet.
        NOT_REGISTERED,
        /// Update failed because the endpoint is being updated.
        PENDING_UPDATE,
        /// Update failed because the endpoint is being registered.
        PENDING_REGISTRATION,
        /// Update failed because the endpoint is being deregistered.
        PENDING_DEREGISTRATION
    };

    /**
     * Enumeration of possible deregistration results.
     */
    enum class DeregistrationResult {
        /// Deregistration succeeded.
        SUCCEEDED,
        /// Deregistration failed due to the endpoint not being registered yet.
        NOT_REGISTERED,
        /// Deregistration failed due to internal error.
        INTERNAL_ERROR,
        /// Deregistration failed due to some configuration error.
        CONFIGURATION_ERROR,
        /// Deregistration failed because the endpoint is being updated.
        PENDING_UPDATE,
        /// Deregistration failed because the endpoint is being registered.
        PENDING_REGISTRATION,
        /// Deregistration failed because the endpoint is being deregistered.
        PENDING_DEREGISTRATION
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
        const EndpointIdentifier& endpointId,
        const avs::AVSDiscoveryEndpointAttributes& attributes,
        const RegistrationResult result) = 0;

    /**
     * Notifies observer that an existing endpoint update has been processed.
     *
     * @param endpointId The endpoint identifier.
     * @param attributes The endpoint attributes.
     * @param result The final registration result.
     */
    virtual void onEndpointUpdate(
        const EndpointIdentifier& endpointId,
        const avs::AVSDiscoveryEndpointAttributes& attributes,
        const UpdateResult result) = 0;

    /**
     * Notifies observer that an endpoint deregistration has been processed.
     *
     * @param endpointId The endpoint identifier.
     * @param result The final deregistration result.
     */
    virtual void onEndpointDeregistration(const EndpointIdentifier& endpointId, const DeregistrationResult result) = 0;

    /**
     * Notifies observer that an endpoint registration or update has been started.
     *
     * @param endpointId The endpoint identifier.
     * @param attributes The endpoint attributes.
     * @param capabilities The endpoint capabilities.
     */
    virtual void onPendingEndpointRegistrationOrUpdate(
        const EndpointIdentifier& endpointId,
        const avs::AVSDiscoveryEndpointAttributes& attributes,
        const std::vector<avs::CapabilityConfiguration>& capabilities) {
    }
};

/**
 * Write an @c RegistrationResult value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param registrationResult The @c RegistrationResult value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(
    std::ostream& stream,
    const EndpointRegistrationObserverInterface::RegistrationResult& registrationResult) {
    switch (registrationResult) {
        case EndpointRegistrationObserverInterface::RegistrationResult::SUCCEEDED:
            stream << "SUCCEEDED";
            break;
        case EndpointRegistrationObserverInterface::RegistrationResult::CONFIGURATION_ERROR:
            stream << "CONFIGURATION_ERROR";
            break;
        case EndpointRegistrationObserverInterface::RegistrationResult::INTERNAL_ERROR:
            stream << "INTERNAL_ERROR";
            break;
        case EndpointRegistrationObserverInterface::RegistrationResult::PENDING_REGISTRATION:
            stream << "PENDING_REGISTRATION";
            break;
        case EndpointRegistrationObserverInterface::RegistrationResult::PENDING_DEREGISTRATION:
            stream << "PENDING_DEREGISTRATION";
            break;
        case EndpointRegistrationObserverInterface::RegistrationResult::PENDING_UPDATE:
            stream << "PENDING_UPDATE";
            break;
        case EndpointRegistrationObserverInterface::RegistrationResult::ALREADY_REGISTERED:
            stream << "ALREADY_REGISTERED";
            break;
        default:
            stream << "UNKNOWN";
            break;
    }
    return stream;
}

/**
 * Write an @c DeregistrationResult value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param deregistrationResult The @c DeregistrationResult value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(
    std::ostream& stream,
    const EndpointRegistrationObserverInterface::DeregistrationResult& deregistrationResult) {
    switch (deregistrationResult) {
        case EndpointRegistrationObserverInterface::DeregistrationResult::SUCCEEDED:
            stream << "SUCCEEDED";
            break;
        case EndpointRegistrationObserverInterface::DeregistrationResult::NOT_REGISTERED:
            stream << "NOT_REGISTERED";
            break;
        case EndpointRegistrationObserverInterface::DeregistrationResult::CONFIGURATION_ERROR:
            stream << "CONFIGURATION_ERROR";
            break;
        case EndpointRegistrationObserverInterface::DeregistrationResult::INTERNAL_ERROR:
            stream << "INTERNAL_ERROR";
            break;
        case EndpointRegistrationObserverInterface::DeregistrationResult::PENDING_REGISTRATION:
            stream << "PENDING_REGISTRATION";
            break;
        case EndpointRegistrationObserverInterface::DeregistrationResult::PENDING_DEREGISTRATION:
            stream << "PENDING_DEREGISTRATION";
            break;
        case EndpointRegistrationObserverInterface::DeregistrationResult::PENDING_UPDATE:
            stream << "PENDING_UPDATE";
            break;
        default:
            stream << "UNKNOWN";
            break;
    }
    return stream;
}

/**
 * Write an @c UpdateResult value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param deregistrationResult The @c DeregistrationResult value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(
    std::ostream& stream,
    const EndpointRegistrationObserverInterface::UpdateResult& updateResult) {
    switch (updateResult) {
        case EndpointRegistrationObserverInterface::UpdateResult::SUCCEEDED:
            stream << "SUCCEEDED";
            break;
        case EndpointRegistrationObserverInterface::UpdateResult::NOT_REGISTERED:
            stream << "NOT_REGISTERED";
            break;
        case EndpointRegistrationObserverInterface::UpdateResult::CONFIGURATION_ERROR:
            stream << "CONFIGURATION_ERROR";
            break;
        case EndpointRegistrationObserverInterface::UpdateResult::INTERNAL_ERROR:
            stream << "INTERNAL_ERROR";
            break;
        case EndpointRegistrationObserverInterface::UpdateResult::PENDING_REGISTRATION:
            stream << "PENDING_REGISTRATION";
            break;
        case EndpointRegistrationObserverInterface::UpdateResult::PENDING_DEREGISTRATION:
            stream << "PENDING_DEREGISTRATION";
            break;
        case EndpointRegistrationObserverInterface::UpdateResult::PENDING_UPDATE:
            stream << "PENDING_UPDATE";
        default:
            stream << "UNKNOWN";
            break;
    }
    return stream;
}

}  // namespace endpoints
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ENDPOINTS_ENDPOINTREGISTRATIONOBSERVERINTERFACE_H_
