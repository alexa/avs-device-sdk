/*
 * AuthDelegate.h
 *
 * Copyright 2016-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AUTHDELEGATE_INCLUDE_AUTHDELEGATE_AUTH_DELEGATE_H_
#define ALEXA_CLIENT_SDK_AUTHDELEGATE_INCLUDE_AUTHDELEGATE_AUTH_DELEGATE_H_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include "ACL/AuthDelegateInterface.h"
#include "ACL/AuthObserverInterface.h"
#include "AuthDelegate/Config.h"
#include "AuthDelegate/HttpPostInterface.h"

namespace alexaClientSDK {
namespace authDelegate {

/**
 * AuthDelegate provides an implementation of the AuthDelegateInterface. It takes a configuration that
 * specifies LWA 'client ID', 'client Secret', and 'refresh token' values and uses those to keep a
 * valid authorization token available.
 */
class AuthDelegate : public acl::AuthDelegateInterface {
public:

    /**
     * Create an AuthDelegate.
     * This function cannot be called if:
     * <ul>
     * <li>AlexaClientSDKInit::initalize has not been called yet.</li>
     * <li>After AlexaClientSDKInit::uninitialize has been called.</li>
     * </ul>
     *
     * @param config AuthDelegate configuration parameters.  Must not be @c nullptr.
     * @return If successful, returns a new AuthDelegate, othewise @c nullptr.
     */
    static std::unique_ptr<AuthDelegate> create(std::shared_ptr<Config> config);

    /**
     * Create an AuthDelegate.
     * This function cannot be called if:
     * <ul>
     * <li>AlexaClientSDKInit::initalize has not been called yet.</li>
     * <li>After AlexaClientSDKInit::uninitialize has been called.</li>
     * </ul>
     *
     * @param config AuthDelegate configuration parameters.  Must not be @c nullptr.
     * @param httpPost Instance that implement HttpPostInterface. Must not be @c nullptr. The behavior for passing in
     *     @c nullptr is undefined.
     * @return If successful, returns a new AuthDelegate, othewise @c nullptr.
     */
    static std::unique_ptr<AuthDelegate> create(std::shared_ptr<Config> config,
        std::unique_ptr<HttpPostInterface> httpPost);

    /**
     * Deleted copy constructor
     *
     * @param rhs The 'right hand side' to not copy.
     */
    AuthDelegate(const AuthDelegate& rhs) = delete;

    /// AuthDelegate destructor.
    ~AuthDelegate();

    /**
     * Deleted assignment operator
     *
     * @param rhs The 'right hand side' to not copy.
     */
    AuthDelegate& operator=(const AuthDelegate& rhs) = delete;

    void setAuthObserver(std::shared_ptr<acl::AuthObserverInterface> observer) override;

    std::string getAuthToken() override;

private:

    /**
     * AuthDelegate constructor.
     *
     * @param config AuthDelegate configuration parameters.  Must not be @c nullptr, or the behavior is undefined.
     * @param httpPost Instance that implement HttpPostInterface. Must not be @c nullptr, or the behavior is undefined.
     */
    AuthDelegate(std::shared_ptr<Config> config, std::unique_ptr<HttpPostInterface> httpPost);

    /**
     * init() is used by create() to perform initialization after constructio but before returning the
     * AuthDelegate instance so that clients only get access to fully formed instances.
     *
     * @return @c true if initialization is successful.
     */
    bool init();

    /// Method run in its own thread to refresh the auth token and notify for auth state changes.
    void refreshAndNotifyThreadFunction();

    /// Attempt to refresh the auth token.
    void refreshAuthToken();

    /**
     * Process a successful response from a token refresh request to LWA.
     *
     * @param code The response code returned from the LWA request
     * @param body The response body (if any) returned from the LWA request.
     * @return @c true if the auth token was refreshed.
     */
    bool handleLwaResponse(HttpPostInterface::ResponseCode code, const std::string& body);

    /**
     * Parse a value out of an LWA response.
     * TODO: Replace this with a real JSON parser (see ACSDK-58 / ACSDK-61)
     *
     * @param response The LWA response to parse
     * @param prefix The string immediately proceeding the value to parse
     * @param suffix The string immediately succeeding the value to parse
     * @return The substring found between prefix and suffix.
     */
    std::string parseResponseValue(
            const std::string& response,
            const std::string& prefix,
            const std::string& suffix) const;

    /**
     * Determine if the auth token has expired.
     *
     * @return @c true if the auth token has expired.
     */
    bool hasAuthTokenExpired();

    /**
     * Set the authorization state to be reported to our client.  If the state has changed, notify the client.
     *
     * @param newState Our new state.
     */
    void setState(acl::AuthObserverInterface::State newState);

    /**
     * AuthDelegate configuration parameters.
     * Access is not synchronized because it is only accessed by @c m_refreshAndNotifyThread.
     */
    std::shared_ptr<Config> m_config;

    /// Authorization state change observer. Access is synchronized with @c m_mutex.
    std::shared_ptr<acl::AuthObserverInterface> m_observer;

    /// Current state authorization.  Access is synchronized with @c m_mutex.
    acl::AuthObserverInterface::State m_authState;

    /**
     * LWA token is used to refresh the auth token.
     * Access is not synchronized because it is only accessed by @c m_refreshAndNotifyThread.
     */
    std::string m_refreshToken;

    /// Mutex used to serialize access to @c m_authState, @c m_authToken, @c m_observer, and @c m_isStopping.
    std::mutex m_mutex;

    /// Used to wake @c m_refreshAndNotifyThread
    std::condition_variable m_wakeThreadCond;

    /**
     * The most recently received LWA authoriation token.
     * Access is not synchronized because it is only accessed by @c m_refreshAndNotifyThread.
     */
    std::string m_authToken;

    /// @c true if it we want to stop @c m_refreshAndNotifyThread. Access is synchronized with @c m_mutex.
    bool m_isStopping;

    /// Thread for performing token refreshes and observer notifications.
    std::thread m_refreshAndNotifyThread;

    /**
     * Time when the current value of @c m_authToken will expire.
     * Access is not synchronized because it is only accessed by @c m_refreshAndNotifyThread.
     */
    std::chrono::steady_clock::time_point m_expirationTime;

    /**
     * Time when we should next refresh the auth token
     * Access is not synchronized because it is only accessed by @c m_refreshAndNotifyThread.
     */
    std::chrono::steady_clock::time_point m_timeToRefresh;

    /**
     * Time the last token refresh request was sent.
     * Access is not synchronized because it is only accessed by @c m_refreshAndNotifyThread.
     */
    std::chrono::steady_clock::time_point m_requestTime;

    /**
     * Number of times we have retried a refresh request.
     * Access is not synchronized because it is only accessed by @c m_refreshAndNotifyThread.
     */
    int m_retryCount;

    /**
     * HTTP/POST client with which to make LWA requests.
     * Access is not synchronized because it is only accessed by @c m_refreshAndNotifyThread.
     */
    std::unique_ptr<HttpPostInterface> m_HttpPost;
};

} // namespace authDelegate
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_AUTHDELEGATE_INCLUDE_AUTHDELEGATE_AUTH_DELEGATE_H_

