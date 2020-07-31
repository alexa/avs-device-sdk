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
#ifndef ALEXA_CLIENT_SDK_ENDPOINTS_INCLUDE_ENDPOINTS_ENDPOINTREGISTRATIONMANAGER_H_
#define ALEXA_CLIENT_SDK_ENDPOINTS_INCLUDE_ENDPOINTS_ENDPOINTREGISTRATIONMANAGER_H_

#include <functional>
#include <future>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesObserverInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveSequencerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointRegistrationManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointRegistrationObserverInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace endpoints {

/**
 * Class responsible for managing endpoints that are registered with AVS and that can be controlled by this client.
 */
class EndpointRegistrationManager : public avsCommon::sdkInterfaces::endpoints::EndpointRegistrationManagerInterface {
public:
    /// @name Aliases to improve readability.
    /// @{
    using EndpointIdentifier = avsCommon::sdkInterfaces::endpoints::EndpointIdentifier;
    using EndpointInterface = avsCommon::sdkInterfaces::endpoints::EndpointInterface;
    using EndpointRegistrationObserverInterface =
        avsCommon::sdkInterfaces::endpoints::EndpointRegistrationObserverInterface;
    /// @}

    /**
     * Destructor.
     */
    ~EndpointRegistrationManager();

    /**
     * Create an @c EndpointRegistrationManager.
     *
     * @param directiveSequencer Object used to route directives sent to this device.
     * @param capabilitiesDelegate Object used to register an endpoint and its capabilities.
     * @param defaultEndpointId The @c EndpointIdentifier of the default endpoint.
     * @return A pointer to a new @c EndpointRegistrationManager if it succeeds; otherwise, return @c nullptr.
     */
    static std::unique_ptr<EndpointRegistrationManager> create(
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> directiveSequencer,
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate,
        const EndpointIdentifier& defaultEndpointId);

    /**
     * Wait for all pending registrations and deregistrations to be enqueued for publishing.
     */
    void waitForPendingRegistrationsToEnqueue();

    /// @name @c EndpointRegistrationManagerInterface methods.
    /// @{
    std::future<RegistrationResult> registerEndpoint(std::shared_ptr<EndpointInterface> endpoint) override;
    std::future<DeregistrationResult> deregisterEndpoint(const EndpointIdentifier& endpointId) override;
    void addObserver(std::shared_ptr<EndpointRegistrationObserverInterface> observer) override;
    void removeObserver(const std::shared_ptr<EndpointRegistrationObserverInterface>& observer) override;
    /// @}

private:
    /// Alias for pending registrations.
    using PendingRegistration = std::pair<std::shared_ptr<EndpointInterface>, std::promise<RegistrationResult>>;

    /// Alias for pending deregistrations.
    using PendingDeregistration = std::pair<std::shared_ptr<EndpointInterface>, std::promise<DeregistrationResult>>;

    /**
     * Class used to observe changes to the capabilities registration.
     */
    class CapabilityRegistrationProxy : public avsCommon::sdkInterfaces::CapabilitiesObserverInterface {
    public:
        /**
         * Sets the callback function.
         *
         * @param callback The callback function to be used to notify of a registration change.
         */
        void setCallback(
            std::function<void(
                const std::pair<RegistrationResult, std::vector<EndpointIdentifier>>& addedOrUpdatedEndpoints,
                const std::pair<DeregistrationResult, std::vector<EndpointIdentifier>>& deletedEndpoints)> callback);

        /// @name CapabilitiesObserverInterface methods.
        /// @{
        void onCapabilitiesStateChange(
            State newState,
            Error newError,
            const std::vector<EndpointIdentifier>& addedOrUpdatedEndpointIds,
            const std::vector<EndpointIdentifier>& deletedEndpointIds) override;
        /// @}
    private:
        // The function callback.
        std::function<void(
            const std::pair<RegistrationResult, std::vector<EndpointIdentifier>>& addedOrUpdatedEndpoints,
            const std::pair<DeregistrationResult, std::vector<EndpointIdentifier>>& deletedEndpoints)>
            m_callback;
    };

    /**
     * Constructor.
     *
     * @param directiveSequencer Object used to route directives sent to this device.
     * @param capabilitiesDelegate Object used to register an endpoint and its capabilities.
     * @param defaultEndpointId The @c EndpointIdentifier of the default endpoint.
     */
    EndpointRegistrationManager(
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> directiveSequencer,
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate,
        const EndpointIdentifier& defaultEndpointId);

    /**
     * Execute the endpoint registration.
     *
     * @param endpoint A pointer to the endpoint to be registered.
     */
    void executeRegisterEndpoint(const std::shared_ptr<EndpointInterface>& endpoint);

    /**
     * Execute the endpoint deregistration.
     *
     * @param endpoint A pointer to the endpoint to be registered.
     */
    void executeDeregisterEndpoint(const std::shared_ptr<EndpointInterface>& endpoint);

    /**
     * Adds capability directive handlers for an endpoint.
     *
     * @param endpoint The endpoint whose capabilities need to be added.
     * @param[out] handlersAdded The out-parameter where the resulting handlers added will be stored.
     * @return Whether adding capabilities succeeded.
     */
    bool addCapabilities(
        const std::shared_ptr<EndpointInterface>& endpoint,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface>>* handlersAdded);

    /**
     * Adds capability directive handlers for an endpoint.
     *
     * @param endpoint The endpoint whose capabilities need to be added.
     * @return Whether adding capabilities succeeded.
     */
    bool addCapabilities(const std::shared_ptr<EndpointInterface>& endpoint);

    /**
     * Removes capability directive handlers from an endpoint.
     *
     * @param endpoint The endpoint whose capabilities need to be removed.
     * @param[out] handlersAdded The out-parameter where the resulting handlers removed will be stored.
     * @return Whether removing capabilities succeeded.
     */
    bool removeCapabilities(
        const std::shared_ptr<EndpointInterface>& endpoint,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface>>* handlersRemoved);

    /**
     * Removes capability directive handlers from an endpoint.
     *
     * @param endpoint The endpoint whose capabilities need to be added.
     * @return Whether removing capabilities succeeded.
     */
    bool removeCapabilities(const std::shared_ptr<EndpointInterface>& endpoint);

    /**
     * Updates registered endpoints (m_endpoints) with newly added/updated endpoints.
     * @note If registration failed, the previous endpoint (if it existed) will be restored.
     *
     * @param addedOrUpdatedEndpoints A pair consisting of the RegistrationResult for the new endpoints
     * as well as a vector of the newly added or updated @c EndpointIdentifiers.
     */
    void updateAddedOrUpdatedEndpoints(
        const std::pair<RegistrationResult, std::vector<EndpointIdentifier>>& addedOrUpdatedEndpoints);

    /**
     * Updates registered endpoints (m_endpoints) by removing the newly deleted endpoints.
     * @note If deregistration failed, the previous endpoint (if it existed) will be restored.
     *
     * @param deletedEndpoints A pair consisting of the DeregistrationResult for the new endpoints
     * as well as a vector of the newly deleted @c EndpointIdentifiers.
     */
    void removeDeletedEndpoints(
        const std::pair<DeregistrationResult, std::vector<EndpointIdentifier>>& deletedEndpoints);

    /**
     * Function used to process changes to the capabilities registration status.
     *
     * @param addedOrUpdatedEndpoints A pair consisting of the RegistrationResult for the new endpoints
     * as well as a vector of the newly added or updated @c EndpointIdentifiers.
     * @param deletedEndpoints A pair consisting of the DeregistrationResult for the new endpoints
     * as well as a vector of the newly deleted @c EndpointIdentifiers.
     * @param result The registration result.
     */
    void onCapabilityRegistrationStatusChanged(
        const std::pair<RegistrationResult, std::vector<EndpointIdentifier>>& addedOrUpdatedEndpoints,
        const std::pair<DeregistrationResult, std::vector<EndpointIdentifier>>& deletedEndpoints);

    /// Mutex to synchronize access to observers.
    mutable std::mutex m_observersMutex;

    /// A list of observers.
    std::list<std::shared_ptr<EndpointRegistrationObserverInterface>> m_observers;

    /// A pointer to the directive sequencer.
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> m_directiveSequencer;

    /// A pointer to the capabilities delegate.
    std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> m_capabilitiesDelegate;

    /// Mutex to synchronize access the various maps of endpoints.
    mutable std::mutex m_endpointsMutex;

    /// A map for the endpoints currently registered.
    std::unordered_map<EndpointIdentifier, std::shared_ptr<EndpointInterface>> m_endpoints;

    /// A list of ongoing registration.
    std::unordered_map<EndpointIdentifier, PendingRegistration> m_pendingRegistrations;

    /// A list of ongoing deregistration.
    std::unordered_map<EndpointIdentifier, PendingDeregistration> m_pendingDeregistrations;

    /// The @c EndpointIdentifier of the default endpoint. Once registered, it cannot be modified
    /// or deleted.
    const EndpointIdentifier m_defaultEndpointId;

    /// A shared object used to proxy capabilities registration status change.
    std::shared_ptr<CapabilityRegistrationProxy> m_capabilityRegistrationProxy;

    /// An executor used for performing the registration work asynchronously.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace endpoints
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ENDPOINTS_INCLUDE_ENDPOINTS_ENDPOINTREGISTRATIONMANAGER_H_
