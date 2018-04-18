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

#ifndef ALEXA_CLIENT_SDK_DCFDELEGATE_INCLUDE_DCFDELEGATE_DCFDELEGATE_H_
#define ALEXA_CLIENT_SDK_DCFDELEGATE_INCLUDE_DCFDELEGATE_DCFDELEGATE_H_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/AuthObserverInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/DCFDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/DCFObserverInterface.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPutInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace dcfDelegate {
/**
 * DCFDelegate provides an implementation of the DCFDelegateInterface. It allows clients to register capabilities
 * implemented by agents and publish them so that Alexa is aware of the device's capabilities.
 */
class DCFDelegate
        : public avsCommon::sdkInterfaces::DCFDelegateInterface
        , public avsCommon::sdkInterfaces::AuthObserverInterface
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<DCFDelegate> {
public:
    /**
     * Create an DCFDelegate.
     *
     * @param authDelegate The auth delegate instance needed for DCF delegate.
     * @param miscStorage The miscDB instance needed for DCF delegate.
     * @param httpPut The HTTP PUT handler instance needed for DCF delegate.
     * @param configurationRoot The global config object.
     * @param deviceInfo The deviceInfo instance for DCF delegate.
     * @return If successful, returns a new DCFDelegate, otherwise @c nullptr.
     */
    static std::shared_ptr<DCFDelegate> create(
        const std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>& authDelegate,
        const std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface>& miscStorage,
        const std::shared_ptr<avsCommon::utils::libcurlUtils::HttpPutInterface>& httpPut,
        const avsCommon::utils::configuration::ConfigurationNode& configurationRoot,
        std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo);

    /// @name DCFDelegateInterface method overrides.
    /// @{
    bool registerCapability(const std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface>&
                                capabilitiesProvider) override;

    DCFPublishReturnCode publishCapabilities() override;

    void publishCapabilitiesAsyncWithRetries() override;

    void addDCFObserver(std::shared_ptr<avsCommon::sdkInterfaces::DCFObserverInterface> observer) override;

    void removeDCFObserver(std::shared_ptr<avsCommon::sdkInterfaces::DCFObserverInterface> observer) override;
    /// @}

    /// @name AuthObserverInterface method overrides.
    /// @{
    void onAuthStateChange(AuthObserverInterface::State newState, AuthObserverInterface::Error error) override;
    /// @}

    // @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

private:
    /**
     * DCFDelegate constructor.
     *
     * @param authDelegate The auth delegate instance needed for DCF delegate.
     * @param miscStorage The miscDB instance needed for DCF delegate.
     * @param httpPut The HTTP PUT handler instance needed for DCF delegate.
     * @param deviceInfo The deviceInfo instance for DCF delegate.
     */
    DCFDelegate(
        const std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>& authDelegate,
        const std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface>& miscStorage,
        const std::shared_ptr<avsCommon::utils::libcurlUtils::HttpPutInterface>& httpPut,
        const std::shared_ptr<avsCommon::utils::DeviceInfo>& deviceInfo);

    /**
     * Perform initialization after construction but before returning the
     * DCFDelegate instance so that clients only get access to fully formed instances.
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
     * Returns the URL for a DCF publish message given a device Id.
     *
     * @param deviceId The device Id to be put in the DCF URL.
     * @return URL for DCF Publish message.
     */
    std::string getDcfUrl(const std::string& deviceId);

    /**
     * Looks for an override message in the config first and returns that if available.
     * If not available, then it will construct a message from the registered capabilities.
     *
     * @return The message body for DCF.
     */
    std::string getDcfMessageBody();

    /**
     * Gets an auth token from the authDelegate instance available to DCFDelegate.
     * This will wait till the auth token is available.
     *
     * @return The auth token.
     */
    std::string getAuthToken();

    /**
     * Gets the previously sent successful DCF publish message data.
     */
    void getPreviouslySentDCFPublishData();

    /**
     * Returns whether the data/metadata is different from the previous successful message.
     *
     * @return true if the data/metadata is different from the previous successful message, else false.
     */
    bool isDCFPublishDataDifferent();

    /**
     * Returns whether the DCF publish message's capabilities is different from the previous successful message.
     *
     * @return true if the DCF publish message's capabilities is different from the previous successful message, else
     * false.
     */
    bool isDCFPublishMessageDifferent();

    /**
     * Save the current DCF publish message's data.
     *
     * @return true if the DCF publish data was successfully saved, else false.
     */
    bool saveDCFPublishData();

    /**
     * Construct a DCF message from the registered capabilities.
     * You need to have the capability registry mutex locked when calling this function.
     *
     * @return The DCF message constructed from the registered capabilities.
     */
    std::string getDcfMessageBodyFromRegisteredCapabilitiesLocked();

    /**
     * Construct a DCF message from the override message provided in the config.
     *
     * @return The DCF message constructed from the override message provided in the config.
     */
    std::string getDcfMessageBodyFromOverride();

    /**
     * Set the DCFDelegate state to be reported to observers.
     *
     * @param newState The new state.
     * @param newError The error associated with the newState.
     */
    void setDCFState(
        avsCommon::sdkInterfaces::DCFObserverInterface::State newState,
        avsCommon::sdkInterfaces::DCFObserverInterface::Error newError);

    /// Mutex used to serialize access to DCF state and DCF state observers.
    std::mutex m_dcfMutex;

    /// Authorization state change observers. Access is synchronized with @c m_dcfMutex.
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::DCFObserverInterface>> m_dcfObservers;

    /// Current state of DCFDelegate. Access is synchronized with @c m_dcfMutex.
    avsCommon::sdkInterfaces::DCFObserverInterface::State m_dcfState;

    /// Current DCFDelegate error. Access is synchronized with @c m_dcfMutex.
    avsCommon::sdkInterfaces::DCFObserverInterface::Error m_dcfError;

    /// Envelope version of the DCF publish message
    std::string m_envelopeVersion;

    /// DCF endpoint
    std::string m_dcfEndpoint;

    /// The current DCF publish message
    std::string m_dcfPublishMessage;

    /// To serialize the capability config map operations.
    std::mutex m_capabilityMutex;

    /// Auth delegate used to get the access token
    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> m_authDelegate;

    /// misc database
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> m_miscStorage;

    /// HTTP Put handler
    std::shared_ptr<avsCommon::utils::libcurlUtils::HttpPutInterface> m_httpPut;

    /// Overridden DCF message body
    std::string m_overridenDcfPublishMessageBody;

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

    /// Data from previously sent successful DCF Publish message
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

    /// Used to let DCF delegate know that the auth delegate is ready.
    std::condition_variable m_authStatusReady;

    /// To ensure that DCF delegate is waiting for publishing.
    std::mutex m_publishWaitMutex;

    /// To indicate if DCF delegate is being shutdown
    bool m_isDCFDelegateShutdown;

    /// Used to let DCF delegate know that it no longer needs to wait to publish.
    std::condition_variable m_publishWaitDone;

    /// An internal executor that performs execution of callable objects passed to it sequentially but asynchronously.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace dcfDelegate
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_DCFDELEGATE_INCLUDE_DCFDELEGATE_DCFDELEGATE_H_
