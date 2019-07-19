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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_AUTHORIZATION_CBLAUTHDELEGATE_INCLUDE_CBLAUTHDELEGATE_CBLAUTHDELEGATE_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_AUTHORIZATION_CBLAUTHDELEGATE_INCLUDE_CBLAUTHDELEGATE_CBLAUTHDELEGATE_H_

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>

#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/AuthObserverInterface.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPostInterface.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/RetryTimer.h>
#include <RegistrationManager/CustomerDataHandler.h>

#include "CBLAuthDelegate/CBLAuthDelegateConfiguration.h"
#include "CBLAuthDelegate/CBLAuthDelegateStorageInterface.h"
#include "CBLAuthDelegate/CBLAuthRequesterInterface.h"

namespace alexaClientSDK {
namespace authorization {
namespace cblAuthDelegate {

/**
 * CBLAuthDelegate provides an implementation of the @c AuthDelegateInterface using the Code-Based Linking
 * authorization process.
 * @see https://developer.amazon.com/docs/alexa-voice-service/code-based-linking-other-platforms.html
 */
class CBLAuthDelegate
        : public avsCommon::sdkInterfaces::AuthDelegateInterface
        , public registrationManager::CustomerDataHandler {
public:
    /**
     * Create a CBLAuthDelegate.
     * This function cannot be called if:
     * <ul>
     * <li>AlexaClientSDKInit::initialize has not been called yet.</li>
     * <li>After AlexaClientSDKInit::un-initialize has been called.</li>
     * </ul>
     *
     * @param configuration The ConfigurationNode containing the configuration parameters for the new instance.
     * @param customerDataManager The CustomerDataManager instance this instance should register with.
     * @param storage The object used to persist the new CBLAuthDelegate's state.
     * @param authRequester Observer used to tell the user to browse to a URL and enter a 'user code'.
     * @param httpPost Instance that implements HttpPostInterface. If nullptr, a default implementation.
     * @param deviceInfo The deviceInfo instance.
     * will be provided.
     */
    static std::unique_ptr<CBLAuthDelegate> create(
        const avsCommon::utils::configuration::ConfigurationNode& configuration,
        std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
        std::shared_ptr<CBLAuthDelegateStorageInterface> storage,
        std::shared_ptr<CBLAuthRequesterInterface> authRequester,
        std::shared_ptr<avsCommon::utils::libcurlUtils::HttpPostInterface> httpPost = nullptr,
        std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo = nullptr);

    /**
     * Deleted copy constructor
     *
     * @param rhs The 'right hand side' to not copy.
     */
    CBLAuthDelegate(const CBLAuthDelegate& rhs) = delete;

    /**
     * Destructor
     */
    ~CBLAuthDelegate();

    /**
     * Deleted assignment operator
     *
     * @param rhs The 'right hand side' to not copy.
     */
    CBLAuthDelegate& operator=(const CBLAuthDelegate& rhs) = delete;

    /// @name AuthDelegateInterface methods
    /// @{
    void addAuthObserver(std::shared_ptr<avsCommon::sdkInterfaces::AuthObserverInterface> observer) override;
    void removeAuthObserver(std::shared_ptr<avsCommon::sdkInterfaces::AuthObserverInterface> observer) override;
    std::string getAuthToken() override;
    void onAuthFailure(const std::string& token) override;
    /// @}

    /// @name CustomerDataHandler methods
    /// @{
    void clearData() override;
    /// @}

private:
    /**
     * The Code-Based Linking authorization process has several stages.  This enum is used to track
     * which stage of the process this @c CBLAuthDelegate is in.
     */
    enum class FlowState {
        /// Initialization: if there is an existing refresh token, transition to @c REFRESHING_TOKEN, if not
        /// transition to @c REQUESTING_CODE_PAIR.
        STARTING,
        /// No valid refresh token, restart the authorization process by requesting a code pair from @c LWA,
        /// retrying if required. Once a valid code pair is acquired, ask the user to authorize by browsing
        /// to a verification URL (supplied by LWA with the code pair) and entering the user_code from the
        /// code pair.  Then transition to @c REQUESTING_TOKEN.
        REQUESTING_CODE_PAIR,
        /// Have received a code pair from @c LWA and are waiting for the user to authenticate and enter
        /// the user code. Wait for the user by polling @c LWA for an access token using the device code
        /// and user code. Waiting stops when an access token is received or the code pair expires. If an
        /// access token is acquired, transition to @c REFRESHING_TOKEN. If the code pair expires before
        /// an access token is acquired, transition back to @c REQUESTING_CODE_PAIR.
        REQUESTING_TOKEN,
        /// Have a refresh token, and may have a valid access token. Periodically refresh (or acquire) an
        /// access token so that if possible, a valid access token is always available. If the refresh
        /// token becomes invalid, transition back to the @c REQUESTING_CODE_PAIR state.
        REFRESHING_TOKEN,
        /// Either a shutdown has been triggered or an unrecoverable error has been encountered. Stop
        /// the process of acquiring an access token.
        STOPPING
    };

    /**
     * CBLAuthDelegate constructor.
     *
     * @param customerDataManager The CustomerDataManager instance this instance should register with.
     * @param storage The object used to persist the new CBLAuthDelegate's state.
     * @param authRequester Observer used to tell the user to authenticate the client for access to AVS
     * by browsing to a URL (supplied by @c LWA) and enter a 'user code' (also provided by @c LWA).
     * @param httpPost Instance that implements HttpPostInterface. Must not be @c nullptr. The behavior
     * for passing in @c nullptr is undefined.
     */
    CBLAuthDelegate(
        std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
        std::shared_ptr<CBLAuthDelegateStorageInterface> storage,
        std::shared_ptr<CBLAuthRequesterInterface> authRequester,
        std::shared_ptr<avsCommon::utils::libcurlUtils::HttpPostInterface> httpPost);

    /**
     * init() is used by create() to perform initialization after construction but before returning the
     * AuthDelegate instance so that clients only get access to fully formed instances.
     *
     * @param configuration The ConfigurationNode containing the configuration parameters for the new instance.
     * @param deviceInfo The deviceInfo instance.
     * @return @c true if initialization is successful.
     */
    bool init(
        const avsCommon::utils::configuration::ConfigurationNode& configuration,
        const std::shared_ptr<avsCommon::utils::DeviceInfo>& deviceInfo);

    /**
     * Stop trying to create or refresh an access token.
     */
    void stop();

    /**
     * Method run in its own thread to handle the authorization flow.
     */
    void handleAuthorizationFlow();

    /**
     * Handle the @c STARTING @c FlowState
     *
     * @return Return the @c FlowState to transition to.
     */
    FlowState handleStarting();

    /**
     * Handle the @c REQUESTING_CODE_PAIR @c FlowState
     *
     * @return Return the @c FlowState to transition to.
     */
    FlowState handleRequestingCodePair();

    /**
     * Handle the @c REQUESTING_TOKEN @c FlowState
     *
     * @return Return the @c FlowState to transition to.
     */
    FlowState handleRequestingToken();

    /**
     * Handle the @c REFRESHING_TOKEN @c FlowState
     *
     * @return Return the @c FlowState to transition to.
     */
    FlowState handleRefreshingToken();

    /**
     * Handle the @c STOPPING @c FlowState
     *
     * @return Return the @c FlowState to transition to.
     */
    FlowState handleStopping();

    /**
     * Request a @c device_code, @c user_code pair from @c LWA.
     *
     * @return The response to the request.
     */
    avsCommon::utils::libcurlUtils::HTTPResponse requestCodePair();

    /**
     * Use a code pair to request an access token.
     *
     * @return The response to the request.
     */
    avsCommon::utils::libcurlUtils::HTTPResponse requestToken();

    /**
     * Use a refresh token to request a new access token.
     *
     * @return The response to the request.
     */
    avsCommon::utils::libcurlUtils::HTTPResponse requestRefresh();

    /**
     * Handle receiving the response to a code pair request.
     *
     * @param response The response to handle.
     * @return The resulting status.
     */
    avsCommon::sdkInterfaces::AuthObserverInterface::Error receiveCodePairResponse(
        const avsCommon::utils::libcurlUtils::HTTPResponse& response);

    /**
     * Handle receiving the response to a request for an access token.
     *
     * @param response The response to handle.
     * @param expiresImmediately Whether or not any access token received should expire immediately.
     * This is used to force immediate refresh of the initial access token (acquired by @c handleRequestingToken())
     * to validate the refresh token in combination with the client ID to avoid the confusing user experience
     * that results if the access token fail to refresh (typically an hour) after initial authoriation.
     * @return The resulting status.
     */
    avsCommon::sdkInterfaces::AuthObserverInterface::Error receiveTokenResponse(
        const avsCommon::utils::libcurlUtils::HTTPResponse& response,
        bool expiresImmediately);

    /**
     * Set the authorization state to be reported to observers.
     *
     * @param newState The new state.
     */
    void setAuthState(avsCommon::sdkInterfaces::AuthObserverInterface::State newState);

    /**
     * Set the authorization error to be reported to observers.
     *
     * @param error The latest error status.
     */
    void setAuthError(avsCommon::sdkInterfaces::AuthObserverInterface::Error error);

    /**
     * Set the current refresh token, and save it to storage.
     *
     * @param refreshToken The new refresh token value.
     */
    void setRefreshToken(const std::string& refreshToken);

    /**
     * Clear the current refresh token, and clear any old one from storage, too.
     */
    void clearRefreshToken();

    /**
     * Determine whether @c threadFunction() should exit.
     *
     * @return Whether @c threadFunction() should exit.
     */
    bool isStopping();

    /**
     * Object used to persist CBLAuthDelegate state.
     * Access is not synchronized because it is only accessed by @c m_thread.
     */
    std::shared_ptr<CBLAuthDelegateStorageInterface> m_storage;

    /**
     * HTTP/POST client with which to make @c LWA requests.
     * Access is not synchronized because it is only accessed by @c m_thread.
     */
    std::shared_ptr<avsCommon::utils::libcurlUtils::HttpPostInterface> m_httpPost;

    /// Observer to receive notifications from this instance of CBLAuthDelegate.
    std::shared_ptr<CBLAuthRequesterInterface> m_authRequester;

    /// Configuration parameters
    std::unique_ptr<CBLAuthDelegateConfiguration> m_configuration;

    /// Whether or not @c threadFunction() is stopping.
    bool m_isStopping;

    /// Mutex used to serialize access to various data members.
    std::mutex m_mutex;

    /// Authorization state change observers. Access is synchronized with @c m_mutex.
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::AuthObserverInterface>> m_observers;

    /// The most recently received LWA authorization token.
    std::string m_accessToken;

    /// Current state of authorization. Access is synchronized with @c m_mutex.
    avsCommon::sdkInterfaces::AuthObserverInterface::State m_authState;

    /// Current authorization error. Access is synchronized with @c m_mutex.
    avsCommon::sdkInterfaces::AuthObserverInterface::Error m_authError;

    /// device-code value returned from a successful code pair request.
    std::string m_deviceCode;

    /// user-code value returned from a successful code pair request.
    std::string m_userCode;

    /// Thread for processing the Code-Based Linking authorization flow.
    std::thread m_authorizationFlowThread;

    /// Point in time when the last received pair of @c device_code, @c user_code will expire.
    std::chrono::steady_clock::time_point m_codePairExpirationTime;

    /// Time when the current value of @c m_accessToken will expire.
    std::chrono::steady_clock::time_point m_tokenExpirationTime;

    /// LWA refresh token used to refresh the access token.
    std::string m_refreshToken;

    /// Condition variable used to wake waits on @c m_thread.
    std::condition_variable m_wake;

    /// Time when we should next refresh the access token.
    std::chrono::steady_clock::time_point m_timeToRefresh;

    /// Time the last token refresh request was sent.
    std::chrono::steady_clock::time_point m_requestTime;

    /// Number of time an access token refresh has been attempted.
    int m_retryCount;

    /// True if the refresh token has not yet been used to create an access token.
    bool m_newRefreshToken;

    /// True if an authorization failure was reported for the current value of m_accessToken.
    bool m_authFailureReported;
};

}  // namespace cblAuthDelegate
}  // namespace authorization
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_AUTHORIZATION_CBLAUTHDELEGATE_INCLUDE_CBLAUTHDELEGATE_CBLAUTHDELEGATE_H_
