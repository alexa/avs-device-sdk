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

#ifndef ACSDKAUTHORIZATION_LWA_LWAAUTHORIZATIONADAPTER_H_
#define ACSDKAUTHORIZATION_LWA_LWAAUTHORIZATIONADAPTER_H_

#include <condition_variable>
#include <thread>

#include <acsdkAuthorization/LWA/LWAAuthorizationConfiguration.h>
#include <acsdkAuthorizationInterfaces/AuthorizationAdapterInterface.h>
#include <acsdkAuthorizationInterfaces/LWA/LWAAuthorizationInterface.h>
#include <acsdkAuthorizationInterfaces/LWA/LWAAuthorizationStorageInterface.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpGetInterface.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPostInterface.h>
#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/RetryTimer.h>

namespace alexaClientSDK {
namespace acsdkAuthorization {
namespace lwa {

/**
 * This class provides functionality for the application to authorize using LWA methods.
 *
 * The class does not currently support reauthorization. If authorization has occurred, then
 * the application should call logout before authorizing.
 *
 * @attention It is the responsibility of the application to acquire the appropriate customer consent. Please
 * refer to @c LWAAuthorizationInterface for more details.
 */
class LWAAuthorizationAdapter
        : public acsdkAuthorizationInterfaces::lwa::LWAAuthorizationInterface
        , public acsdkAuthorizationInterfaces::AuthorizationAdapterInterface
        , public std::enable_shared_from_this<LWAAuthorizationAdapter> {
public:
    /// Destructor
    ~LWAAuthorizationAdapter();

    /**
     * Creates an instance of LWAAuthorizationAdapter.
     *
     * @param configuration The configuration node.
     * @param httpPost An object for making HTTPPost requests.
     * @param deviceInfo Device info.
     * @param storage Persisted storage.
     * @param httpGet An object for making HTTPGet requests.
     * @param adapterId Allows for specifying an alternate adapterId to the default.
     * @return An instance if successful or null.
     */
    static std::shared_ptr<LWAAuthorizationAdapter> create(
        const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configuration,
        std::unique_ptr<avsCommon::utils::libcurlUtils::HttpPostInterface> httpPost,
        const std::shared_ptr<avsCommon::utils::DeviceInfo>& deviceInfo,
        const std::shared_ptr<acsdkAuthorizationInterfaces::lwa::LWAAuthorizationStorageInterface>& storage,
        std::unique_ptr<avsCommon::utils::libcurlUtils::HttpGetInterface> httpGet = nullptr,
        const std::string& adapterId = "");

    /// @name LWAAuthorizationInterface
    /// @{
    bool authorizeUsingCBL(
        const std::shared_ptr<acsdkAuthorizationInterfaces::lwa::CBLAuthorizationObserverInterface>& observer) override;
    bool authorizeUsingCBLWithCustomerProfile(
        const std::shared_ptr<acsdkAuthorizationInterfaces::lwa::CBLAuthorizationObserverInterface>& observer) override;
    /// @}

    /// @name AuthorizationInterface
    /// @{
    std::string getId() override;
    /// @}

    /// @name AuthorizationAdapterInterface
    /// @{
    std::string getAuthToken() override;
    void reset() override;
    void onAuthFailure(const std::string& authToken) override;
    avsCommon::sdkInterfaces::AuthObserverInterface::FullState getState() override;
    std::shared_ptr<acsdkAuthorizationInterfaces::AuthorizationInterface> getAuthorizationInterface() override;
    avsCommon::sdkInterfaces::AuthObserverInterface::FullState onAuthorizationManagerReady(
        const std::shared_ptr<acsdkAuthorizationInterfaces::AuthorizationManagerInterface>& manager) override;
    /// @}

private:
    /// An enum for different token exchange types.
    enum class TokenExchangeMethod {
        /**
         * No active Token Exchange Method. Note: In cases where the refreshToken is retrieved
         * from persisted storage, this will be applicable.
         */
        NONE,
        /// Using the Code Based Linking method.
        CBL
    };

    /**
     * Contains information from a Refresh Token response.
     */
    struct RefreshTokenResponse {
        /// The refresh token.
        std::string refreshToken;

        /// The access token.
        std::string accessToken;

        /// The time the refresh request was made.
        std::chrono::steady_clock::time_point requestTime;

        /// The expiration duration..
        std::chrono::seconds expiration;

        /**
         * Whether or not this refresh token has been exchanged for an access token and verified.
         * Initial responses from LWA will have this set to false.
         */
        bool isRefreshTokenVerified;

        /// Constructor.
        RefreshTokenResponse() :
                refreshToken{""},
                accessToken{""},
                requestTime{std::chrono::steady_clock::now()},
                expiration{std::chrono::seconds::zero()},
                isRefreshTokenVerified{true} {
        }

        /**
         * Returns the a timepoint representing when the token will expire.
         *
         * @return The expiration time.
         */
        std::chrono::steady_clock::time_point getExpirationTime() const {
            return requestTime + expiration;
        }
    };

    /**
     * An enum to track the current state of the @c LWAAuthorizationAdapter.
     */
    enum class FlowState {
        /**
         * Currently waiting for an authorization request.
         */
        IDLE,
        /**
         * No valid refresh token, restart the authorization process by requesting a token from @c LWA.
         * This can take various forms.
         *
         * For CBL:
         * Request a code pair from @c LWA, retrying if required until a valid one is received.
         * Once a valid code pair is acquired, ask the user to authorize by browsing
         * to a verification URL (supplied by LWA with the code pair) and entering the user_code from the
         * code pair.
         *
         * Once a refresh token is obtained, transition to REFFRESHING_TOKEN.
         */
        REQUESTING_TOKEN,
        /**
         * Have a refresh token, and may have a valid access token. Periodically refresh (or acquire) an
         * access token so that if possible, a valid access token is always available.
         */
        REFRESHING_TOKEN,
        /**
         * Transition to this state as a cleanup after a reset() call and the data has been cleared. Transition back to
         * IDLE.
         */
        CLEARING_DATA,
        /**
         * Either a shutdown has been triggered or an unrecoverable error has been encountered. Stop
         * the adapter and prepare for exit.
         */
        STOPPING
    };

    /// Constructor.
    LWAAuthorizationAdapter(
        const std::string& adapterId,
        std::unique_ptr<avsCommon::utils::libcurlUtils::HttpPostInterface> httpPost,
        const std::shared_ptr<acsdkAuthorizationInterfaces::lwa::LWAAuthorizationStorageInterface>& storage,
        std::unique_ptr<avsCommon::utils::libcurlUtils::HttpGetInterface> httpGet);

    /**
     * Initializes.
     *
     * @param configuration The configuration node object.
     * @param devceInfo The devce info object.
     * @return Whether initialization was successful.
     */
    bool init(
        const avsCommon::utils::configuration::ConfigurationNode& configuration,
        const std::shared_ptr<avsCommon::utils::DeviceInfo>& deviceInfo);

    /**
     * Loads persisted data to determine the initialization state.
     *
     * @return The state to start at.
     */
    FlowState retrievePersistedData();

    /**
     * Use a refresh token to request a new access token.
     *
     * @return The response to the request.
     */
    avsCommon::utils::libcurlUtils::HTTPResponse requestRefresh(std::chrono::steady_clock::time_point* requestTime);

    /**
     * Handles the IDLE state as described above.
     *
     * @return The next state to transition to.
     */
    FlowState handleIdle();

    /**
     * Handles the REQUESTING_TOKEN state as described above.
     *
     * @return The next state to transition to.
     */
    FlowState handleRequestingToken();

    /**
     * Handles the REFRESHING_TOKEN state as described above.
     *
     * @return The next state to transition to.
     */
    FlowState handleRefreshingToken();

    /**
     * Handles the CLEARING_DATA state as described above.
     *
     * @return The next state to transition to.
     */
    FlowState handleClearingData();

    /**
     * Handles the STOPPING state as described above.
     *
     * @return The next state to transition to.
     */
    FlowState handleStopping();

    /**
     * Method run in its own thread to handle the authorization flow.
     *
     * @param state The starting state.
     */
    void handleAuthorizationFlow(FlowState state);

    /// @name CBL Methods
    /// @{
    bool authorizeUsingCBLHelper(
        const std::shared_ptr<acsdkAuthorizationInterfaces::lwa::CBLAuthorizationObserverInterface>& observer,
        bool requestCustomerProfile);

    /**
     * Requests a Code Pair from LWA. This will retry. On success, this will notify the
     * CBLAuthorizationObserverInterface of the verification URI and user code.
     *
     * @return The error associated with the request. An empty Optional indicates the request was cancelled due to
     * logout or shutdown. Error::SUCCESS indicates a successful request. Else, the appropriate error will be added.
     */
    avsCommon::utils::Optional<avsCommon::sdkInterfaces::AuthObserverInterface::Error> requestCodePair();

    /**
     * Request a @c device_code, @c user_code pair from @c LWA. Besides alexa::all, if @c m_requestCustomerProfile is
     * false, this will request the userId portion. Otherwise, it will request additionally name and email. This does
     * NOT request postal code.
     *
     * @return The response to the request.
     */
    avsCommon::utils::libcurlUtils::HTTPResponse sendCodePairRequest();

    /**
     * Handle receiving the response to a code pair request.
     *
     * @param response The response to handle.
     * @return The resulting status.
     */
    avsCommon::sdkInterfaces::AuthObserverInterface::Error receiveCodePairResponse(
        const avsCommon::utils::libcurlUtils::HTTPResponse& response);

    /**
     * Makes a request to LWA with the device code and user code received in @c handleRequestingCodePair and waits for a
     * response. On success, LWA will send down a refresh token, access token, expiration, and token type.
     *
     * @return The error associated with the request. An empty Optional indicates the request was cancelled due to
     * logout or shutdown. Error::SUCCESS indicates a successful request. Else, the appropriate error will be added.
     */
    avsCommon::utils::Optional<avsCommon::sdkInterfaces::AuthObserverInterface::Error> exchangeToken(
        RefreshTokenResponse* tokenResponse);

    /**
     * Requests the token from LWA.
     *
     * @param[out] The time the request was made.
     * @return The HTTPResponse.
     */
    avsCommon::utils::libcurlUtils::HTTPResponse sendTokenRequest(std::chrono::steady_clock::time_point* requestTime);
    /// @}

    /**
     * Parses the LWA response and retrieves the refresh token, access token, token type, and expiration.
     *
     * @param response The response.
     * @param expiresImmediately Forces the returned tokenResponse to expire immediately.
     * @param[out] tokenResponse The token response.
     * @return Whether an error occurred.
     */
    avsCommon::sdkInterfaces::AuthObserverInterface::Error receiveTokenResponse(
        const avsCommon::utils::libcurlUtils::HTTPResponse& response,
        bool expiresImmediately,
        struct RefreshTokenResponse* tokenResponse);

    /**
     * A wrapper function that acquires the lock before caling @c shouldStopRetryingLocked();
     *
     * @return If retry loops should stop.
     */
    bool shouldStopRetrying();

    /**
     * Checks if various stages should continue to retry. This will return true
     * if a reset() to clear data or if shutdown is ongoing. The lock must be held before calling this.
     *
     * @return If retry loops should stop.
     */
    bool shouldStopRetryingLocked();

    /**
     * Checks if the adapter is shutting down.
     *
     * @return If the adapter is shutting down.
     */
    bool isShuttingDown();

    /**
     * Stop the adapter.
     */
    void stop();

    /**
     * Stop the current auth attempt by resetting m_authMethod. This will force LWAAuthorizationAdapter to wait for a
     * new Auth operation. The mutex should be locked before calling this method.
     */
    void resetAuthMethodLocked();

    /**
     * A wrapper function that acquires the mutex and then calls @c setRefreshTokenResponseLocked().
     *
     * @param response The response to cache.
     * @param persist Whether to persist the refreshToken.
     */
    void setRefreshTokenResponse(const RefreshTokenResponse& response, bool persist = true);

    /**
     * Cache the refreshToken response and set it as the currently active and valid one. This will contain a valid
     * access token. m_mutex must be held before calling this.
     *
     * @param response The response to cache.
     * @param persist Whether to persist the refreshToken.
     */
    void setRefreshTokenResponseLocked(const RefreshTokenResponse& response, bool persist = true);

    /**
     * Updates the state and reports to @c AuthorizationManagerInterface the new state.
     *
     * @param state The state.
     * @param error The error.
     */
    void updateStateAndNotifyManager(
        avsCommon::sdkInterfaces::AuthObserverInterface::State state,
        avsCommon::sdkInterfaces::AuthObserverInterface::Error error);

    /**
     * A function that gets customer profile information. One reason for failure is that the access token was not
     * generated with the appropriate scopes.
     *
     * @param getCustomerProfile Whether to request the full user profile.
     * @return Whether this was successful.
     */
    bool getCustomerProfile(const std::string& accessToken);

    /// Mutex used to serialize access to various data members.
    std::mutex m_mutex;

    /**
     * HTTP/POST client with which to make @c LWA requests.
     * Access is not synchronized because it is only accessed by @c m_authorizationFlowThread.
     */
    std::unique_ptr<avsCommon::utils::libcurlUtils::HttpPostInterface> m_httpPost;

    /**
     * HTTP/GET client with which to make @c LWA requests.
     * Access is not synchronized because it is only accessed by @c m_authorizationFlowThread.
     */
    std::unique_ptr<avsCommon::utils::libcurlUtils::HttpGetInterface> m_httpGet;

    /// The storage instance used to persist information.
    std::shared_ptr<acsdkAuthorizationInterfaces::lwa::LWAAuthorizationStorageInterface> m_storage;

    /// Configuration parameters
    std::unique_ptr<LWAAuthorizationConfiguration> m_configuration;

    /// The current auth state.
    avsCommon::sdkInterfaces::AuthObserverInterface::State m_authState;

    /// The current error state.
    avsCommon::sdkInterfaces::AuthObserverInterface::Error m_authError;

    /// UserId.
    std::string m_userId;

    /// Condition variable used to wake waits on @c m_authorizationFlowThread.
    std::condition_variable m_wake;

    /// The main thread that this operates on.
    std::thread m_authorizationFlowThread;

    /// @ name CBL
    /// @{
    /// The code expiration time.
    std::chrono::steady_clock::time_point m_codePairExpirationTime;

    /// device-code value returned from a successful code pair request.
    std::string m_deviceCode;

    /// user-code value returned from a successful code pair request.
    std::string m_userCode;

    /// The interval to make token requests to LWA.
    std::chrono::seconds m_tokenRequestInterval;

    /// The observer that will respond to CBL related callbacks.
    std::shared_ptr<acsdkAuthorizationInterfaces::lwa::CBLAuthorizationObserverInterface> m_cblRequester;

    /// Whether to request extended customer profile information such as name and email.
    bool m_requestCustomerProfile;
    /// @}

    /// True if an authorization failure was reported for the current value of m_accessToken.
    bool m_authFailureReported;

    /// The current authorization method that is used.
    TokenExchangeMethod m_authMethod;

    /// The active refresh token state associated.
    RefreshTokenResponse m_refreshTokenResponse;

    /// The instance of @c AuthorizationManagerInterface.
    std::shared_ptr<acsdkAuthorizationInterfaces::AuthorizationManagerInterface> m_manager;

    /// Whether the adapter is shutting down.
    bool m_isShuttingDown;

    /// Whether the adapter is reset and clearing data.
    bool m_isClearingData;

    /// The adapter id.
    const std::string m_adapterId;
};

}  // namespace lwa
}  // namespace acsdkAuthorization
}  // namespace alexaClientSDK

#endif  // ACSDKAUTHORIZATION_LWA_LWAAUTHORIZATIONADAPTER_H_
