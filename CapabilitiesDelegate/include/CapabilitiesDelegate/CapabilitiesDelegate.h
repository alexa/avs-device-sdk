/*
 * Copyright 2018-2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "CapabilitiesDelegate/DiscoveryEventSenderInterface.h"
#include "CapabilitiesDelegate/Storage/CapabilitiesDelegateStorageInterface.h"

#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesObserverInterface.h>
#include <AVSCommon/SDKInterfaces/PostConnectOperationProviderInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <RegistrationManager/CustomerDataHandler.h>
#include <RegistrationManager/CustomerDataManager.h>

namespace alexaClientSDK {
namespace capabilitiesDelegate {
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
    bool registerEndpoint(
        const avsCommon::avs::AVSDiscoveryEndpointAttributes& endpointAttributes,
        const std::vector<avsCommon::avs::CapabilityConfiguration>& capabilities) override;

    void addCapabilitiesObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesObserverInterface> observer) override;

    void removeCapabilitiesObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesObserverInterface> observer) override;

    void invalidateCapabilities() override;
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
     */
    void setCapabilitiesState(
        avsCommon::sdkInterfaces::CapabilitiesObserverInterface::State newState,
        avsCommon::sdkInterfaces::CapabilitiesObserverInterface::Error newError);

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

    /// The mutex to serialize access to the endpoint config map.
    std::mutex m_endpointMapMutex;

    /// A map from endpoint ID to endpoint configuration.
    std::unordered_map<std::string, std::string> m_endpointIdToConfigMap;

    /// The mutex to synchronize access to the current @c DiscoveryEventSender
    std::mutex m_discoveryEventSenderMutex;

    /// The current post connect publisher.
    std::shared_ptr<DiscoveryEventSenderInterface> m_currentDiscoveryEventSender;
};

}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_CAPABILITIESDELEGATE_H_
