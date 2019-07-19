/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/AuthObserverInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesObserverInterface.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPutInterface.h>
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
        , public avsCommon::sdkInterfaces::AuthObserverInterface
        , public avsCommon::utils::RequiresShutdown
        , public registrationManager::CustomerDataHandler
        , public std::enable_shared_from_this<CapabilitiesDelegate> {
public:
    /**
     * Create an CapabilitiesDelegate.
     *
     * @param authDelegate The auth delegate instance needed for CapabilitiesDelegate.
     * @param miscStorage The miscDB instance needed for CapabilitiesDelegate.
     * @param httpPut The HTTP PUT handler instance needed for CapabilitiesDelegate.
     * @param customerDataManager Object that will track the CustomerDataHandler.
     * @param configurationRoot The global config object.
     * @param deviceInfo The deviceInfo instance for CapabilitiesDelegate.
     * @return If successful, returns a new CapabilitiesDelegate, otherwise @c nullptr.
     */
    static std::shared_ptr<CapabilitiesDelegate> create(
        const std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>& authDelegate,
        const std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface>& miscStorage,
        const std::shared_ptr<avsCommon::utils::libcurlUtils::HttpPutInterface>& httpPut,
        const std::shared_ptr<registrationManager::CustomerDataManager>& customerDataManager,
        const avsCommon::utils::configuration::ConfigurationNode& configurationRoot,
        std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo);

    /// @name CapabilitiesDelegateInterface method overrides.
    /// @{
    bool registerCapability(const std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface>&
                                capabilitiesProvider) override;

    CapabilitiesPublishReturnCode publishCapabilities() override;

    void publishCapabilitiesAsyncWithRetries() override;

    void addCapabilitiesObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesObserverInterface> observer) override;

    void removeCapabilitiesObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesObserverInterface> observer) override;

    void invalidateCapabilities() override;
    /// @}

    /// @name AuthObserverInterface method overrides.
    /// @{
    void onAuthStateChange(AuthObserverInterface::State newState, AuthObserverInterface::Error newError) override;
    /// @}

    // @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /// @name CustomerDataHandler Functions
    /// @{
    void clearData() override;
    /// @}

private:
    /**
     * CapabilitiesDelegate constructor.
     *
     * @param authDelegate The auth delegate instance needed for CapabilitiesDelegate.
     * @param miscStorage The miscDB instance needed for CapabilitiesDelegate.
     * @param httpPut The HTTP PUT handler instance needed for CapabilitiesDelegate.
     * @param customerDataManager Object that will track the CustomerDataHandler.
     * @param deviceInfo The deviceInfo instance for CapabilitiesDelegate.
     */
    CapabilitiesDelegate(
        const std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>& authDelegate,
        const std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface>& miscStorage,
        const std::shared_ptr<avsCommon::utils::libcurlUtils::HttpPutInterface>& httpPut,
        const std::shared_ptr<registrationManager::CustomerDataManager>& customerDataManager,
        const std::shared_ptr<avsCommon::utils::DeviceInfo>& deviceInfo);

    /**
     * Perform initialization after construction but before returning the
     * CapabilitiesDelegate instance so that clients only get access to fully formed instances.
     *
     * @param configurationRoot The global config object.
     * @return @c true if initialization is successful.
     */
    bool init(const avsCommon::utils::configuration::ConfigurationNode& configurationRoot);

    /**
     * Returns if a capability (given its configuration map) has been registered.
     * You need to have the capability registry mutex locked when calling this function.
     *
     * @param capabilityMap The capability configuration map of a specific capability.
     * @return true if a capability has been registered, else false.
     */
    bool isCapabilityRegisteredLocked(const std::unordered_map<std::string, std::string>& capabilityMap);

    /**
     * Returns the URL for a Capabilities API message given a device Id.
     *
     * @param deviceId The device Id to be put in the capabilities API URL.
     * @return URL for the Capabilities API message.
     */
    std::string getCapabilitiesApiUrl(const std::string& deviceId);

    /**
     * Looks for an override message in the config first and returns that if available.
     * If not available, then it will construct a message from the registered capabilities.
     *
     * @return The message body for the Capabilities API.
     */
    std::string getCapabilitiesPublishMessageBody();

    /**
     * Gets an auth token from the authDelegate instance available to CapabilitiesDelegate.
     * This will wait till the auth token is available.
     *
     * @return The auth token.
     */
    std::string getAuthToken();

    /**
     * Gets the previously sent successful Capabilities API message data.
     */
    void getPreviouslySentCapabilitiesPublishData();

    /**
     * Returns whether the data/metadata is different from the previous successful message.
     *
     * @return true if the data/metadata is different from the previous successful message, else false.
     */
    bool isCapabilitiesPublishDataDifferent();

    /**
     * Returns whether the Capabilities API message's capabilities is different from the previous successful message.
     *
     * @return true if the Capabilities API message's capabilities is different from the previous successful message,
     * else false.
     */
    bool isCapabilitiesPublishMessageDifferent();

    /**
     * Save the current Capabilities API message's data.
     *
     * @return true if the Capabilities API data was successfully saved, else false.
     */
    bool saveCapabilitiesPublishData();

    /**
     * Construct a Capabilities API message from the registered capabilities.
     * You need to have the capability registry mutex locked when calling this function.
     *
     * @return The Capabilities API message constructed from the registered capabilities.
     */
    std::string getCapabilitiesPublishMessageBodyFromRegisteredCapabilitiesLocked();

    /**
     * Construct a Capabilities API message from the override message provided in the config.
     *
     * @return The Capabilities API message constructed from the override message provided in the config.
     */
    std::string getCapabilitiesPublishMessageBodyFromOverride();

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
     * Return whether or not this instance is shutting down.
     *
     * @return Whether or not this instance is shutting down.
     */
    bool isShuttingDown();

    /**
     * Return whether or not this instance is shutting down.
     * @note This must be called with @c m_publishWaitMutex acquired.
     *
     * @return Whether or not this instance is shutting down.
     */
    bool isShuttingDownLocked();

    /// Mutex used to serialize access to Capabilities state and Capabilities state observers.
    std::mutex m_capabilitiesMutex;

    /// Authorization state change observers. Access is synchronized with @c m_capabilitiesMutex.
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesObserverInterface>>
        m_capabilitiesObservers;

    /// Current state of CapabilitiesDelegate. Access is synchronized with @c m_capabilitiesMutex.
    avsCommon::sdkInterfaces::CapabilitiesObserverInterface::State m_capabilitiesState;

    /// Current CapabilitiesDelegate error. Access is synchronized with @c m_capabilitiesMutex.
    avsCommon::sdkInterfaces::CapabilitiesObserverInterface::Error m_capabilitiesError;

    /// Envelope version of the Capabilities API message
    std::string m_envelopeVersion;

    /// Capabilities API endpoint
    std::string m_capabilitiesApiEndpoint;

    /// The current Capabilities API message
    std::string m_capabilitiesPublishMessage;

    /// To serialize the capability config map operations.
    std::mutex m_capabilityMutex;

    /// Auth delegate used to get the access token
    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> m_authDelegate;

    /// misc database
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> m_miscStorage;

    /// HTTP Put handler
    std::shared_ptr<avsCommon::utils::libcurlUtils::HttpPutInterface> m_httpPut;

    /// Overridden Capabilities API message body
    std::string m_overridenCapabilitiesPublishMessageBody;

    /// DeviceInfo
    std::shared_ptr<avsCommon::utils::DeviceInfo> m_deviceInfo;

    /// A map of the capability key (consisting of the interface type and name)  and the capability config map.
    std::unordered_map<std::string, std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> m_capabilityConfigs;

    /// Map of the capability key (consisting of the interface type and name) and the capability config map registered
    /// by capabilties.
    std::unordered_map<std::string, std::shared_ptr<avsCommon::avs::CapabilityConfiguration>>
        m_registeredCapabilityConfigs;

    /// The authDelegate's auth status.
    AuthObserverInterface::State m_currentAuthState;

    /// Data from previously sent successful Capabilities API message
    /// Previous DeviceInfo
    std::shared_ptr<avsCommon::utils::DeviceInfo> m_previousDeviceInfo;
    /// Previous envelope version
    std::string m_previousEnvelopeVersion;
    /// Map of previous capabilities. This is a map of the capability key (consisting of the interface type and name)
    /// and the capability config map
    std::unordered_map<std::string, std::shared_ptr<avsCommon::avs::CapabilityConfiguration>>
        m_previousCapabilityConfigs;

    /// To ensure that auth delegate is ready.
    std::mutex m_authStatusMutex;

    /// Used to let CapabilitiesDelegate know that the auth delegate is ready.
    std::condition_variable m_authStatusReady;

    /// To ensure that CapabilitiesDelegate is waiting for publishing.
    std::mutex m_publishWaitMutex;

    /// To indicate if CapabilitiesDelegate is being shutdown
    bool m_isCapabilitiesDelegateShutdown;

    /// Used to let CapabilitiesDelegate know that it no longer needs to wait to publish.
    std::condition_variable m_publishWaitDone;

    /// An internal executor that performs execution of callable objects passed to it sequentially but asynchronously.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_CAPABILITIESDELEGATE_H_
