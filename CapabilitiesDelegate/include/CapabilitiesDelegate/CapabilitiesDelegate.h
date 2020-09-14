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

#ifndef ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_CAPABILITIESDELEGATE_H_
#define ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_CAPABILITIESDELEGATE_H_

#include <unordered_map>
#include <memory>
#include <mutex>
#include <string>

#include "CapabilitiesDelegate/DiscoveryEventSender.h"
#include "CapabilitiesDelegate/Storage/CapabilitiesDelegateStorageInterface.h"

#include <AVSCommon/AVS/AVSDiscoveryEndpointAttributes.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesObserverInterface.h>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/SDKInterfaces/PostConnectOperationProviderInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <RegistrationManager/CustomerDataHandler.h>
#include <RegistrationManager/CustomerDataManager.h>

namespace alexaClientSDK {
namespace capabilitiesDelegate {

/**
 * Helper struct used to store pending and in-flight endpoint changes.
 */
struct InProcessEndpointsToConfigMapStruct {
    /// A map of pending endpointId to configuration changes.
    /// These endpoints will be sent in a Discovery Event to AVS when next possible.
    std::unordered_map<std::string, std::string> pending;

    /// A map of in-flight endpointId to configuration changes.
    /// These endpoints are currently part of an active Discovery event in-flight to AVS.
    std::unordered_map<std::string, std::string> inFlight;
};

/**
 * CapabilitiesDelegate provides an implementation of the CapabilitiesDelegateInterface. It allows clients to register
 * capabilities implemented by agents and publish them so that Alexa is aware of the device's capabilities.
 */
class CapabilitiesDelegate
        : public avsCommon::sdkInterfaces::CapabilitiesDelegateInterface
        , public avsCommon::utils::RequiresShutdown
        , public avsCommon::sdkInterfaces::PostConnectOperationProviderInterface
        , public DiscoveryStatusObserverInterface
        , public registrationManager::CustomerDataHandler
        , public std::enable_shared_from_this<CapabilitiesDelegate> {
public:
    /**
     * Create an CapabilitiesDelegate.
     *
     * @param authDelegate The auth delegate instance needed for CapabilitiesDelegate.
     * @param storage The storage instance needed for CapabilitiesDelegate.
     * @param customerDataManager Object that will track the CustomerDataHandler.
     * @return If successful, returns a new CapabilitiesDelegate, otherwise @c nullptr.
     */
    static std::shared_ptr<CapabilitiesDelegate> create(
        const std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>& authDelegate,
        const std::shared_ptr<storage::CapabilitiesDelegateStorageInterface>& storage,
        const std::shared_ptr<registrationManager::CustomerDataManager>& customerDataManager);

    /// @name CapabilitiesDelegateInterface method overrides.
    /// @{
    bool addOrUpdateEndpoint(
        const avsCommon::avs::AVSDiscoveryEndpointAttributes& endpointAttributes,
        const std::vector<avsCommon::avs::CapabilityConfiguration>& capabilities) override;

    bool deleteEndpoint(
        const avsCommon::avs::AVSDiscoveryEndpointAttributes& endpointAttributes,
        const std::vector<avsCommon::avs::CapabilityConfiguration>& capabilities) override;

    void addCapabilitiesObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesObserverInterface> observer) override;

    void removeCapabilitiesObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesObserverInterface> observer) override;

    void invalidateCapabilities() override;

    void setMessageSender(
        const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender) override;
    /// @}

    /// @name AlexaEventProcessedObserverInterface method overrides.
    /// @{
    void onAlexaEventProcessedReceived(const std::string& eventCorrelationToken) override;
    /// @}

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /// @name PostConnectOperationInterfaceProvider Functions.
    /// @{
    std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationInterface> createPostConnectOperation() override;
    /// @}

    /// @name DiscoveryObserverInterface methods
    /// @{
    void onDiscoveryCompleted(
        const std::unordered_map<std::string, std::string>& addOrUpdateReportEndpoints,
        const std::unordered_map<std::string, std::string>& deleteReportEndpoints) override;
    void onDiscoveryFailure(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status status) override;
    /// @}

    /// @name AVSGatewayManagerInterface methods
    /// @{
    void onAVSGatewayChanged(const std::string& avsGateway) override;
    /// @}

    /// @name CustomerDataHandler Functions
    /// @{
    void clearData() override;
    /// @}

    /// @name ConnectionStatusObserverInterface Functions
    /// @{
    void onConnectionStatusChanged(const Status status, const ChangedReason reason) override;
    /// @}

    /**
     * Adds the event sender used to send Discovery events.
     */
    void addDiscoveryEventSender(const std::shared_ptr<DiscoveryEventSenderInterface>& discoveryEventSender);

private:
    /**
     * CapabilitiesDelegate constructor.
     *
     * @param authDelegate The auth delegate instance needed for CapabilitiesDelegate.
     * @param storage The storage instance needed for CapabilitiesDelegate.
     * @param customerDataManager Object that will track the CustomerDataHandler.
     */
    CapabilitiesDelegate(
        const std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>& authDelegate,
        const std::shared_ptr<storage::CapabilitiesDelegateStorageInterface>& storage,
        const std::shared_ptr<registrationManager::CustomerDataManager>& customerDataManager);

    /**
     * Perform initialization after construction but before returning the
     * CapabilitiesDelegate instance so that clients only get access to fully formed instances.
     *
     * @return @c true if initialization is successful.
     */
    bool init();

    /**
     * Gets an auth token from the authDelegate instance available to CapabilitiesDelegate.
     * This will wait till the auth token is available.
     *
     * @return The auth token.
     */
    std::string getAuthToken();

    /**
     * Set the CapabilitiesDelegate state to be reported to observers.
     *
     * @param newState The new state.
     * @param newError The error associated with the newState.
     * @param addOrUpdateReportEndpoints The list of @c EndpointIdentifier of endpoints that were intended to
     * be added or updated.
     * @param deleteReportEndpoints The list of @c EndpointIdentifier of endpoints that were intended for
     * deletion.
     */
    void setCapabilitiesState(
        const avsCommon::sdkInterfaces::CapabilitiesObserverInterface::State newState,
        const avsCommon::sdkInterfaces::CapabilitiesObserverInterface::Error newError,
        const std::vector<avsCommon::sdkInterfaces::endpoints::EndpointIdentifier>& addOrUpdateReportEndpoints,
        const std::vector<avsCommon::sdkInterfaces::endpoints::EndpointIdentifier>& deleteReportEndpoints);

    /**
     * Updates the storage with the AddOrUpdateReport and DeleteReport endpoints.
     * @note: This method is called after successfully publishing Discovery events and the endpoints used in the
     * discovery events are updated in the storage.
     *
     * @param addOrUpdateReportEndpoints The map of AddOrUpdateReport endpoints.
     * @param deleteReportEndpoints The map of DeleteReport endpoints.
     * @return True if successful, else false.
     */
    bool updateEndpointConfigInStorage(
        const std::unordered_map<std::string, std::string>& addOrUpdateReportEndpoints,
        const std::unordered_map<std::string, std::string>& deleteReportEndpoints);

    /**
     * Resets the discovery event sender.
     */
    void resetDiscoveryEventSender();

    /**
     * Executes sending the CapabilitiesDelegate's pending endpoints.
     */
    void executeSendPendingEndpoints();

    /**
     * Whether @c CapabilitiesDelegate is shutting down.
     */
    bool isShuttingDown();

    /**
     * Adds stale endpoints from the database to m_deleteEndpoints.pending list. Stale endpoints are endpoints
     * stored in the database, but are not registered or pending registration. @c m_endpointsMutex must be locked
     * to call this method.
     *
     * @param storedEndpointConfig The reference to the stored map of endpointId to configuration.
     */
    void addStaleEndpointsToPendingDeleteLocked(std::unordered_map<std::string, std::string>* storedEndpointConfig);

    /**
     * Filters m_addOrUpdate.pending endpoints to remove those that are already in the database, and therefore
     * do not need to be sent in an addOrUpdateReport. @c m_endpointsMutex must be locked to call this method.
     *
     * @param storedEndpointConfig The reference to the stored map of endpointId to configuration. This map
     * is filtered to remove endpoints that do not need to be registered to AVS.
     */
    void filterUnchangedPendingAddOrUpdateEndpointsLocked(
        std::unordered_map<std::string, std::string>* storedEndpointConfig);

    /**
     * Moves in-flight endpoints to pending for re-try purposes (e.g. on re-connect). @c m_endpointsMutex must
     * be locked to call this method.
     */
    void moveInFlightEndpointsToPendingLocked();

    /**
     * Moves in-flight endpoints to pending for re-try purposes (e.g. on re-connect).
     */
    void moveInFlightEndpointsToPending();

    /**
     * Moves in-flight endpoints into m_endpoints, e.g. when Discovery events completed successfully.
     */
    void moveInFlightEndpointsToRegisteredEndpoints();

    /// Mutex used to serialize access to Capabilities state and Capabilities state observers.
    std::mutex m_observerMutex;

    /// Authorization state change observers. Access is synchronized with @c m_capabilitiesMutex.
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesObserverInterface>>
        m_capabilitiesObservers;

    /// Current state of CapabilitiesDelegate. Access is synchronized with @c m_capabilitiesMutex.
    avsCommon::sdkInterfaces::CapabilitiesObserverInterface::State m_capabilitiesState;

    /// Current CapabilitiesDelegate error. Access is synchronized with @c m_capabilitiesMutex.
    avsCommon::sdkInterfaces::CapabilitiesObserverInterface::Error m_capabilitiesError;

    /// Auth delegate used to get the access token
    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> m_authDelegate;

    /// The reference to the @c CapabilitiesDelegateStorageInterface.
    std::shared_ptr<storage::CapabilitiesDelegateStorageInterface> m_capabilitiesDelegateStorage;

    /// The mutex to serialize access to m_isConnected.
    std::mutex m_isConnectedMutex;

    /// Flag indicating latest reported connection status. True if SDK is connected to the AVS and ready,
    /// false otherwise.
    bool m_isConnected;

    /// The mutex to serialize operations related to pending and in-flight endpoints.
    std::mutex m_endpointsMutex;

    /// A struct containing the in-flight and pending endpoints for Discovery.addOrUpdateReport event.
    InProcessEndpointsToConfigMapStruct m_addOrUpdateEndpoints;

    /// A struct containing the in-flight and pending endpoints Discovery.deleteReport event.
    InProcessEndpointsToConfigMapStruct m_deleteEndpoints;

    /// A map of endpointId to configuration for currently registered endpoints.
    std::unordered_map<std::string, std::string> m_endpoints;

    /// The mutex to serialize operations related to @c m_currentDiscoveryEventSender.
    std::mutex m_currentDiscoveryEventSenderMutex;

    /// The current @c DiscoveryEventSenderInterface.
    std::shared_ptr<DiscoveryEventSenderInterface> m_currentDiscoveryEventSender;

    /// The mutex to synchronize access to the @c MessageSenderInterface.
    std::mutex m_messageSenderMutex;

    /// Object that can send messages to AVS.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /// The mutex to synchronize access to @c m_isShuttingDown.
    std::mutex m_isShuttingDownMutex;

    /// Whether or not the @c CapabilitiesDelegate is shutting down.
    bool m_isShuttingDown;

    /**
     * @c Executor which queues up operations from asynchronous API calls.
     *
     * @note This declaration needs to come *after* the Executor Thread Variables above so that the thread shuts down
     *     before the Executor Thread Variables are destroyed.
     */
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_CAPABILITIESDELEGATE_H_
