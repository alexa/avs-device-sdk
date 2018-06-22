/*
 * Copyright 2016-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <functional>
#include <iostream>
#include <random>
#include <sstream>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <unordered_map>

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPost.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpResponseCodes.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "AuthDelegate/AuthDelegate.h"

namespace alexaClientSDK {
namespace authDelegate {

using namespace alexaClientSDK::avsCommon::sdkInterfaces;

/// String to identify log entries originating from this file.
static const std::string TAG("AuthDelegate");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Name of @c ConfigurationNode for AuthDelegate
const std::string CONFIG_KEY_AUTH_DELEGATE = "authDelegate";

/// Name of clientId value in AuthDelegate's @c ConfigurationNode.
const char* CONFIG_KEY_CLIENT_ID = "clientId";

/// Name of clientSecret value in AuthDelegate's @c ConfigurationNode.
const char* CONFIG_KEY_CLIENT_SECRET = "clientSecret";

/// Name of refreshToken value in AuthDelegate's @c ConfigurationNode.
const char* CONFIG_KEY_REFRESH_TOKEN = "refreshToken";

/// Name of lwaURL value in AuthDelegate's @c ConfigurationNode.
const char* CONFIG_KEY_LWA_URL = "lwaUrl";

/// Name of requestTimeout value in AuthDelegate's @c ConfigurationNode.
const char* CONFIG_KEY_REQUEST_TIMEOUT = "requestTimeout";

/// Name of authTokenRefreshHeadStart value in AuthDelegate's @c ConfigurationNode.
const char* CONFIG_KEY_AUTH_TOKEN_REFRESH_HEAD_START = "authTokenRefreshHeadStart";

/// Default value for lwaURL.
static const std::string DEFAULT_LWA_URL = "https://api.amazon.com/auth/o2/token";

/// Default value for requestTimeout.
static const std::chrono::minutes DEFAULT_REQUEST_TIMEOUT = std::chrono::minutes(5);

/// Default value for authTokenRefreshHeadStart.
static const std::chrono::minutes DEFAULT_AUTH_TOKEN_REFRESH_HEAD_START = std::chrono::minutes(10);

/// POST data before 'client_id' that is sent to LWA to refresh the auth token.
static const std::string POST_DATA_UP_TO_CLIENT_ID = "grant_type=refresh_token&client_id=";

/// POST data between 'client_id' and 'refresh_token' that is sent to LWA to refresh the auth token.
static const std::string POST_DATA_BETWEEN_CLIENT_ID_AND_REFRESH_TOKEN = "&refresh_token=";

/// POST data between 'refresh_token' and 'client_secret' that is sent to LWA to refresh the auth token.
static const std::string POST_DATA_BETWEEN_REFRESH_TOKEN_AND_CLIENT_SECRET = "&client_secret=";

/// This is the property name in the JSON which refers to the access token.
static const std::string JSON_KEY_ACCESS_TOKEN = "access_token";

/// This is the property name in the JSON which refers to the refresh token.
static const std::string JSON_KEY_REFRESH_TOKEN = "refresh_token";

/// This is the property name in the JSON which refers to the expiration time.
static const std::string JSON_KEY_EXPIRES_IN = "expires_in";

/// This is the property name in the JSON which refers to the error.
static const std::string JSON_KEY_ERROR = "error";

/**
 * Lookup table for recoverable errors from LWA error codes
 * (@see https://images-na.ssl-images-amazon.com/images/G/01/lwa/dev/docs/website-developer-guide._TTH_.pdf)
 * to AuthObserverInterface::Error codes.
 */
static const std::unordered_map<std::string, AuthObserverInterface::Error> g_recoverableErrorCodeMap = {
    {"invalid_client", AuthObserverInterface::Error::AUTHORIZATION_FAILED},
    {"unauthorized_client", AuthObserverInterface::Error::UNAUTHORIZED_CLIENT},
    {"servererror", AuthObserverInterface::Error::SERVER_ERROR},
};

/**
 * Lookup table for unrecoverable errors from LWA error codes
 * (@see https://images-na.ssl-images-amazon.com/images/G/01/lwa/dev/docs/website-developer-guide._TTH_.pdf)
 * to AuthObserverInterface::Error codes.
 */
static const std::unordered_map<std::string, AuthObserverInterface::Error> g_unrecoverableErrorCodeMap = {
    {"invalid_request", AuthObserverInterface::Error::INVALID_REQUEST},
    {"unsupported_grant_type", AuthObserverInterface::Error::UNSUPPORTED_GRANT_TYPE},
    {"invalid_grant", AuthObserverInterface::Error::AUTHORIZATION_EXPIRED},
};

/**
 * Helper function that decides if the HTTP error code is unrecoverable.
 *
 * @param error The HTTP error code.
 * @return @c true if @c error is unrecoverable, @c false otherwise.
 */
static bool isUnrecoverable(const std::string& error) {
    return g_unrecoverableErrorCodeMap.end() != g_unrecoverableErrorCodeMap.find(error);
}

/**
 * Helper function that decides if the Enum error code is unrecoverable.
 *
 * @param error The Enum error code.
 * @return @c true if @c error is unrecoverable, @c false otherwise.
 */
static bool isUnrecoverable(AuthObserverInterface::Error error) {
    for (auto errorCodeMapElement : g_unrecoverableErrorCodeMap) {
        if (errorCodeMapElement.second == error) {
            return true;
        }
    }
    return false;
}

/**
 * Helper function that retrieves the Error enum value.
 *
 * @param error The string in the @c error field of packet body.
 * @return the Error enum code corresponding to @c error. If error is "", returns SUCCESS. If it is an unknown error,
 * returns UNKNOWN_ERROR.
 */
static AuthObserverInterface::Error getErrorCode(const std::string& error) {
    if (error.empty()) {
        return AuthObserverInterface::Error::SUCCESS;
    } else {
        auto errorIterator = g_recoverableErrorCodeMap.find(error);
        if (g_recoverableErrorCodeMap.end() != errorIterator) {
            return errorIterator->second;
        } else {
            errorIterator = g_unrecoverableErrorCodeMap.find(error);
            if (g_unrecoverableErrorCodeMap.end() != errorIterator) {
                return errorIterator->second;
            }
        }
        return AuthObserverInterface::Error::UNKNOWN_ERROR;
    }
}

/**
 * Function to convert the number of times we have already retried to the time to perform the next retry.
 *
 * @param retryCount The number of times we have retried
 * @return The time that the next retry should be attempted
 */
static std::chrono::steady_clock::time_point calculateTimeToRetry(int retryCount) {
    /**
     * Table of retry backoff values based upon page 77 of
     * @see https://images-na.ssl-images-amazon.com/images/G/01/mwsportal/
     * doc/en_US/offamazonpayments/LoginAndPayWithAmazonIntegrationGuide.pdf
     */
    const static std::vector<int> retryBackoffTimes = {
        0,      // Retry 1:  0.00s range with 50% randomization: [ 0.0s.  0.0s]
        1000,   // Retry 2:  1.00s range with 50% randomization: [ 0.5s,  1.5s]
        2000,   // Retry 3:  2.00s range with 50% randomization: [ 1.0s,  3.0s]
        4000,   // Retry 4:  4.00s range with 50% randomization: [ 2.0s,  6.0s]
        10000,  // Retry 5: 10.00s range with 50% randomization: [ 5.0s, 15.0s]
        30000,  // Retry 6: 30.00s range with 50% randomization: [15.0s, 45.0s]
        60000,  // Retry 7: 60.00s range with 50% randomization: [30.0s, 90.0s]
    };

    // Retry Timer Object.
    avsCommon::utils::RetryTimer RETRY_TIMER(retryBackoffTimes);

    return std::chrono::steady_clock::now() + RETRY_TIMER.calculateTimeToRetry(retryCount);
}

std::unique_ptr<AuthDelegate> AuthDelegate::create() {
    return AuthDelegate::create(avsCommon::utils::libcurlUtils::HttpPost::create());
}

std::unique_ptr<AuthDelegate> AuthDelegate::create(
    std::unique_ptr<avsCommon::utils::libcurlUtils::HttpPostInterface> httpPost) {
    if (!avsCommon::avs::initialization::AlexaClientSDKInit::isInitialized()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "sdkNotInitialized"));
        return nullptr;
    }
    std::unique_ptr<AuthDelegate> instance(new AuthDelegate(std::move(httpPost)));
    if (instance->init()) {
        return instance;
    }
    return nullptr;
}

AuthDelegate::AuthDelegate(std::unique_ptr<avsCommon::utils::libcurlUtils::HttpPostInterface> httpPost) :
        m_authState{AuthObserverInterface::State::UNINITIALIZED},
        m_authError{AuthObserverInterface::Error::SUCCESS},
        m_isStopping{false},
        m_expirationTime{std::chrono::time_point<std::chrono::steady_clock>::max()},
        m_retryCount{0},
        m_HttpPost{std::move(httpPost)} {
}

AuthDelegate::~AuthDelegate() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_isStopping = true;
    }
    m_wakeThreadCond.notify_all();
    if (m_refreshAndNotifyThread.joinable()) {
        m_refreshAndNotifyThread.join();
    }
}

void AuthDelegate::addAuthObserver(std::shared_ptr<AuthObserverInterface> observer) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!observer) {
        return;
    }

    if (!m_observers.insert(observer).second) {
        return;
    }

    observer->onAuthStateChange(m_authState, m_authError);
}

void AuthDelegate::removeAuthObserver(std::shared_ptr<AuthObserverInterface> observer) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!observer) {
        return;
    }
    m_observers.erase(observer);
}

std::string AuthDelegate::getAuthToken() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_authToken;
}

bool AuthDelegate::init() {
    auto configuration = avsCommon::utils::configuration::ConfigurationNode::getRoot()[CONFIG_KEY_AUTH_DELEGATE];
    if (!configuration) {
        ACSDK_ERROR(LX("initFailed").d("reason", "missingConfigurationValue").d("key", CONFIG_KEY_AUTH_DELEGATE));
        return false;
    }

    if (!configuration.getString(CONFIG_KEY_CLIENT_ID, &m_clientId) || m_clientId.empty()) {
        ACSDK_ERROR(LX("initFailed").d("reason", "missingConfigurationValue").d("key", CONFIG_KEY_CLIENT_ID));
        return false;
    }

    if (!configuration.getString(CONFIG_KEY_CLIENT_SECRET, &m_clientSecret) || m_clientSecret.empty()) {
        ACSDK_ERROR(LX("initFailed").d("reason", "missingConfigurationValue").d("key", CONFIG_KEY_CLIENT_SECRET));
        return false;
    }

    if (!configuration.getString(CONFIG_KEY_REFRESH_TOKEN, &m_refreshToken) || m_refreshToken.empty()) {
        ACSDK_ERROR(LX("initFailed").d("reason", "missingConfigurationValue").d("key", CONFIG_KEY_REFRESH_TOKEN));
        return false;
    }

    configuration.getString(CONFIG_KEY_LWA_URL, &m_lwaUrl, DEFAULT_LWA_URL);

    configuration.getDuration<std::chrono::seconds>(
        CONFIG_KEY_REQUEST_TIMEOUT, &m_requestTimeout, DEFAULT_REQUEST_TIMEOUT);

    configuration.getDuration<std::chrono::seconds>(
        CONFIG_KEY_AUTH_TOKEN_REFRESH_HEAD_START, &m_authTokenRefreshHeadStart, DEFAULT_AUTH_TOKEN_REFRESH_HEAD_START);

    if (!m_HttpPost) {
        ACSDK_ERROR(LX("initFailed").d("reason", "nullptrHttPost"));
        return false;
    }

    m_refreshAndNotifyThread = std::thread(&AuthDelegate::refreshAndNotifyThreadFunction, this);
    return true;
}

void AuthDelegate::refreshAndNotifyThreadFunction() {
    std::function<bool()> isStopping = [this] { return m_isStopping; };

    while (true) {
        std::unique_lock<std::mutex> lock(m_mutex);
        bool isAboutToExpire =
            (AuthObserverInterface::State::REFRESHED == m_authState && m_expirationTime < m_timeToRefresh);
        auto nextActionTime = (isAboutToExpire ? m_expirationTime : m_timeToRefresh);
        auto nextState = m_authState;

        // Wait for an appropriate amount of time.
        if (m_wakeThreadCond.wait_until(lock, nextActionTime, isStopping)) {
            break;
        }
        if (isAboutToExpire) {
            m_authToken.clear();
            nextState = AuthObserverInterface::State::EXPIRED;
            lock.unlock();
        } else {
            lock.unlock();
            if (AuthObserverInterface::Error::SUCCESS == refreshAuthToken()) {
                nextState = AuthObserverInterface::State::REFRESHED;
            }
        }
        setState(nextState);
    }
}

AuthObserverInterface::Error AuthDelegate::refreshAuthToken() {
    // Don't wait for this request so long that we would be late to notify our observer if the token expires.
    m_requestTime = std::chrono::steady_clock::now();
    auto timeout = m_requestTimeout;
    if (AuthObserverInterface::State::REFRESHED == m_authState) {
        auto timeUntilExpired = std::chrono::duration_cast<std::chrono::seconds>(m_expirationTime - m_requestTime);
        if (timeout > timeUntilExpired) {
            timeout = timeUntilExpired;
        }
    }

    std::ostringstream postData;
    postData << POST_DATA_UP_TO_CLIENT_ID << m_clientId << POST_DATA_BETWEEN_CLIENT_ID_AND_REFRESH_TOKEN
             << m_refreshToken << POST_DATA_BETWEEN_REFRESH_TOKEN_AND_CLIENT_SECRET << m_clientSecret;

    std::string body;
    auto code = m_HttpPost->doPost(m_lwaUrl, postData.str(), timeout, body);
    auto newError = handleLwaResponse(code, body);

    if (AuthObserverInterface::Error::SUCCESS == newError) {
        m_retryCount = 0;
    } else {
        m_timeToRefresh = calculateTimeToRetry(m_retryCount++);
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_authError = newError;
    }
    return newError;
}

AuthObserverInterface::Error AuthDelegate::handleLwaResponse(long code, const std::string& body) {
    rapidjson::Document document;
    if (document.Parse(body.c_str()).HasParseError()) {
        ACSDK_ERROR(LX("handleLwaResponseFailed")
                        .d("reason", "parseJsonFailed")
                        .d("position", document.GetErrorOffset())
                        .d("error", GetParseError_En(document.GetParseError())));
        return AuthObserverInterface::Error::UNKNOWN_ERROR;
    }

    if (HTTPResponseCode::SUCCESS_OK == code) {
        std::string authToken;
        std::string refreshToken;
        uint64_t expiresInSeconds = 0;

        auto authTokenIterator = document.FindMember(JSON_KEY_ACCESS_TOKEN.c_str());
        if (authTokenIterator != document.MemberEnd() && authTokenIterator->value.IsString()) {
            authToken = authTokenIterator->value.GetString();
        }

        auto refreshTokenIterator = document.FindMember(JSON_KEY_REFRESH_TOKEN.c_str());
        if (refreshTokenIterator != document.MemberEnd() && refreshTokenIterator->value.IsString()) {
            refreshToken = refreshTokenIterator->value.GetString();
        }

        auto expiresInIterator = document.FindMember(JSON_KEY_EXPIRES_IN.c_str());
        if (expiresInIterator != document.MemberEnd() && expiresInIterator->value.IsUint64()) {
            expiresInSeconds = expiresInIterator->value.GetUint64();
        }

        if (authToken.empty() || refreshToken.empty() || expiresInSeconds == 0) {
            ACSDK_ERROR(LX("handleLwaResponseFailed")
                            .d("authTokenEmpty", authToken.empty())
                            .d("refreshTokenEmpty", refreshToken.empty())
                            .d("expiresInSeconds", expiresInSeconds));
            ACSDK_DEBUG(LX("handleLwaresponseFailed").d("body", body));
            return AuthObserverInterface::Error::UNKNOWN_ERROR;
        }

        ACSDK_DEBUG(LX("handleLwaResponseSucceeded")
                        .sensitive("refreshToken", refreshToken)
                        .sensitive("authToken", authToken)
                        .d("expiresInSeconds", expiresInSeconds));

        m_refreshToken = refreshToken;
        m_expirationTime = m_requestTime + std::chrono::seconds(expiresInSeconds);
        m_timeToRefresh = m_expirationTime - m_authTokenRefreshHeadStart;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_authToken = authToken;
        }
        return AuthObserverInterface::Error::SUCCESS;

    } else {
        std::string error;
        auto errorIterator = document.FindMember(JSON_KEY_ERROR.c_str());
        if (errorIterator != document.MemberEnd() && errorIterator->value.IsString()) {
            error = errorIterator->value.GetString();
        }
        if (!error.empty()) {
            // If `error` field is non-empty in the body, it means that we encountered an error.
            ACSDK_ERROR(LX("handleLwaResponseFailed").d("error", error).d("isUnrecoverable", isUnrecoverable(error)));
            return getErrorCode(error);
        } else {
            ACSDK_ERROR(LX("handleLwaResponseFailed").d("error", "errorNotFoundInResponseBody"));
            return AuthObserverInterface::Error::UNKNOWN_ERROR;
        }
    }
}

bool AuthDelegate::hasAuthTokenExpired() {
    return std::chrono::steady_clock::now() >= m_expirationTime;
}

void AuthDelegate::setState(AuthObserverInterface::State newState) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (isUnrecoverable(m_authError)) {
        ACSDK_ERROR(LX("threadStopping").d("reason", "encounteredUnrecoverableError"));
        newState = AuthObserverInterface::State::UNRECOVERABLE_ERROR;
        m_isStopping = true;
    }

    if (m_authState != newState) {
        m_authState = newState;
        for (auto observer : m_observers) {
            ACSDK_DEBUG(LX("onAuthStateChangeCalled").d("state", (int)m_authState).d("error", (int)m_authError));
            observer->onAuthStateChange(m_authState, m_authError);
        }
    }
}

}  // namespace authDelegate
}  // namespace alexaClientSDK
