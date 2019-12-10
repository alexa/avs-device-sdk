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

#include <AVSCommon/SDKInterfaces/DirectiveSequencerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointRegistrationManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointRegistrationObserverInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesObserverInterface.h>
#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
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
     * @return A pointer to a new @c EndpointRegistrationManager if it succeeds; otherwise, return @c nullptr.
     */
    static std::unique_ptr<EndpointRegistrationManager> create(
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> directiveSequencer,
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate);

    /// @name @c EndpointRegistrationManagerInterface methods.
    /// @{
    std::future<RegistrationResult> registerEndpoint(std::shared_ptr<EndpointInterface> endpoint) override;
    void addObserver(std::shared_ptr<EndpointRegistrationObserverInterface> observer) override;
    void removeObserver(const std::shared_ptr<EndpointRegistrationObserverInterface>& observer) override;
    /// @}

    /**
     * This function will disable any new registration.
     *
     * Subsequent calls to register endpoint will fail.
     * @note This call will block until any ongoing registration finishes.
     */
    void disableRegistration();

private:
    /// Alias for pending registrations.
    using PendingRegistration = std::pair<std::shared_ptr<EndpointInterface>, std::promise<RegistrationResult>>;

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
        void setCallback(std::function<void(RegistrationResult)> callback);

        /// @name CapabilitiesObserverInterface methods.
        /// @{
        void onCapabilitiesStateChange(State newState, Error newError) override;
        /// @}
    private:
        // The function callback.
        std::function<void(RegistrationResult)> m_callback;
    };

    /**
     * Constructor.
     *
     * @param directiveSequencer Object used to route directives sent to this device.
     * @param capabilitiesDelegate Object used to register an endpoint and its capabilities.
     */
    EndpointRegistrationManager(
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> directiveSequencer,
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate);

    /**
     * Execute the endpoint registration.
     *
     * @param endpoint A pointer to the endpoint to be registered.
     */
    void executeEndpointRegistration(const std::shared_ptr<EndpointInterface>& endpoint);

    /**
     * Function used to process changes to the capabilities registration status.
     *
     * @param result The registration result.
     */
    void onCapabilityRegistrationStatusChanged(RegistrationResult result);

    /// Mutex to synchronize access to the variables accessed outside of the executor thread.
    mutable std::mutex m_mutex;

    /// A pointer to the directive sequencer.
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> m_directiveSequencer;

    /// A pointer to the capabilities delegate.
    std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> m_capabilitiesDelegate;

    /// A map for the endpoints currently registered.
    std::unordered_map<EndpointIdentifier, std::shared_ptr<EndpointInterface>> m_endpoints;

    /// A list of observers.
    std::list<std::shared_ptr<EndpointRegistrationObserverInterface>> m_observers;

    /// A list of ongoing registration.
    std::unordered_map<EndpointIdentifier, PendingRegistration> m_pendingRegistrations;

    /// A shared object used to proxy capabilities registration status change.
    std::shared_ptr<CapabilityRegistrationProxy> m_capabilityRegistrationProxy;

    /// A flag indicating whether registration is currently enabled. For now, we only support registering endpoint
    /// before the initial SDK connect().
    bool m_registrationEnabled;

    /// An executor used for performing the registration work asynchronously.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace endpoints
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ENDPOINTS_INCLUDE_ENDPOINTS_ENDPOINTREGISTRATIONMANAGER_H_
