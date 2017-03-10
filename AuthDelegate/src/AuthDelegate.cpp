/*
 * AuthDelegate.cpp
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

#include <chrono>
#include <iostream>
#include <random>
#include <stdexcept>
#include <sstream>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <unordered_map>
#include "curl/curl.h"
#include "AuthDelegate/AuthDelegate.h"
#include "AuthDelegate/HttpPost.h"
#include "AVSUtils/Initialization/AlexaClientSDKInit.h"

namespace alexaClientSDK {

using acl::AuthObserverInterface;

namespace authDelegate {

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
static bool isUnrecoverable(const std::string & error) {
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
        if(errorCodeMapElement.second == error) {
            return true;
        }
    }
    return false;
}

/**
 * Helper function that retrieves the Error enum value.
 *
 * @param error The string in the @c error field of packet body.
 * @return the Error enum code corresponding to @c error. If error is "", returns NO_ERROR. If it is an unknown error,
 * returns UNKNOWN_ERROR.
 */
static AuthObserverInterface::Error getErrorCode(const std::string & error) {
    if (error.empty()) {
        return AuthObserverInterface::Error::NO_ERROR;
    } else {
        auto errorIterator = g_recoverableErrorCodeMap.find(error);
        if(g_recoverableErrorCodeMap.end() != errorIterator) {
            return errorIterator->second;
        } else {
            errorIterator = g_unrecoverableErrorCodeMap.find(error);
            if(g_unrecoverableErrorCodeMap.end() != errorIterator) {
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
    static const int retryBackoffTimes[] = {
        0,     // Retry 1:  0.00s range with 0.5 randomization: [ 0.0s.  0.0s]
        1000,  // Retry 2:  1.00s range with 0.5 randomization: [ 0.5s,  1.5s]
        2000,  // Retry 3:  2.00s range with 0.5 randomization: [ 1.0s,  3.0s]
        4000,  // Retry 4:  5.00s range with 0.5 randomization: [ 2.0s,  6.0s]
        10000, // Retry 5: 10.00s range with 0.5 randomization: [ 5.0s, 15.0s]
        30000, // Retry 6: 20.00s range with 0.5 randomization: [15.0s, 45.0s]
        60000, // Retry 7: 60.00s range with 0.5 randomization: [30.0s, 90.0s]
    };
    /// Scale of range (relative to table entry) to select a random value from.
    static const double RETRY_RANDOMIZATION_FACTOR = 0.5;
    /// Factor to multiply table value by when selecting low end of random values.
    static const double RETRY_DECREASE_FACTOR = 1 - RETRY_RANDOMIZATION_FACTOR;
    /// Factor to multiply table value by when selecting high end of random values.
    static const double RETRY_INCREASE_FACTOR = 1 + RETRY_RANDOMIZATION_FACTOR;

    // Cap count to the size of the table
    static const int retryTableSize =
        (sizeof(retryBackoffTimes) / sizeof(retryBackoffTimes[0]));
    if (retryCount < 0) {
        retryCount = 0;
    } else if (retryCount >= retryTableSize) {
        retryCount = retryTableSize - 1;
    }

    // Calculate randomized interval to the next retry.
    std::mt19937 generator(static_cast<unsigned>(std::time(nullptr)));
    std::uniform_int_distribution<int> distribution(
        static_cast<int>(retryBackoffTimes[retryCount] * RETRY_DECREASE_FACTOR),
        static_cast<int>(retryBackoffTimes[retryCount] * RETRY_INCREASE_FACTOR));
    auto delayMs = std::chrono::milliseconds(distribution(generator));
    // TODO: convert to debug log: ACSDK-57
    std::cerr << "calculateTimeToRetry() delayMs=" << delayMs.count() << std::endl;

    return std::chrono::steady_clock::now() + delayMs;
}

std::unique_ptr<AuthDelegate> AuthDelegate::create(std::shared_ptr<Config> config) {
    return AuthDelegate::create(config, HttpPost::create());
}

std::unique_ptr<AuthDelegate> AuthDelegate::create(std::shared_ptr<Config> config,
        std::unique_ptr<HttpPostInterface> httpPost) {
    if (!avsUtils::initialization::AlexaClientSDKInit::isInitialized()) {
        std::cerr<<"Alexa Client SDK not initialized, aborting";
        return nullptr;
    }
    std::unique_ptr<AuthDelegate> instance(new AuthDelegate(config, std::move(httpPost)));
    if (instance->init()) {
        return instance;
    }
    return nullptr;
}

AuthDelegate::AuthDelegate(std::shared_ptr<Config> config, std::unique_ptr<HttpPostInterface> httpPost):
    m_config{config},
    m_authState{AuthObserverInterface::State::UNINITIALIZED},
    m_authError{AuthObserverInterface::Error::NO_ERROR},
    m_isStopping{false},
    m_expirationTime{std::chrono::time_point<std::chrono::steady_clock>::max()},
    m_retryCount{0},
    m_HttpPost{std::move(httpPost)}
{
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

void AuthDelegate::setAuthObserver(std::shared_ptr<AuthObserverInterface> observer) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (observer != m_observer) {
        m_observer = observer;
        if (m_observer) {
            m_observer->onAuthStateChange(m_authState, m_authError);
        }
    }
}

std::string AuthDelegate::getAuthToken() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_authToken;
}

bool AuthDelegate::init() {
    if (!m_config) {
        std::cerr << "ERROR: AuthDelegate::create() provided nullptr config." << std::endl;
        return false;
    }

    if (m_config->getClientId().empty()) {
        std::cerr << "ERROR: Config with empty clientID." << std::endl;
        return false;
    }
    if (m_config->getClientSecret().empty()) {
        std::cerr << "ERROR: Config with empty clientSecret." << std::endl;
        return false;
    }

    if (m_config->getRefreshToken().empty()) {
        std::cerr << "ERROR: Config with empty refreshToken." << std::endl;
        return false;
    }
    m_refreshToken = m_config->getRefreshToken();

    if (m_config->getLwaUrl().empty()) {
        std::cerr << "ERROR: Config with empty lwaUrl." << std::endl;
        return false;
    }
    if (!m_HttpPost) {
        std::cerr << "ERROR: m_HttpPost can't be nullptr." << std::endl;
        return false;
    }
    m_refreshAndNotifyThread = std::thread(&AuthDelegate::refreshAndNotifyThreadFunction, this);
    return true;
}

void AuthDelegate::refreshAndNotifyThreadFunction() {
    std::function<bool()> isStopping = [this] {
        return m_isStopping;
    };

    while (true) {
        std::unique_lock<std::mutex> lock(m_mutex);
        bool isAboutToExpire = (AuthObserverInterface::State::REFRESHED == m_authState &&
                m_expirationTime < m_timeToRefresh);
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
            if (AuthObserverInterface::Error::NO_ERROR == refreshAuthToken()) {
                nextState = AuthObserverInterface::State::REFRESHED;
            }
        }
        setState(nextState);
    }
}

AuthObserverInterface::Error AuthDelegate::refreshAuthToken() {
    // Don't wait for this request so long that we would be late to notify our observer if the token expires.
    m_requestTime = std::chrono::steady_clock::now();
    auto timeout = m_config->getRequestTimeout();
    if (AuthObserverInterface::State::REFRESHED == m_authState) {
        auto timeUntilExpired = std::chrono::duration_cast<std::chrono::seconds>(m_expirationTime - m_requestTime);
        if (timeout > timeUntilExpired) {
            timeout = timeUntilExpired;
        }
    }

    std::ostringstream postData;
    postData
        << POST_DATA_UP_TO_CLIENT_ID << m_config->getClientId()
        << POST_DATA_BETWEEN_CLIENT_ID_AND_REFRESH_TOKEN << m_refreshToken
        << POST_DATA_BETWEEN_REFRESH_TOKEN_AND_CLIENT_SECRET << m_config->getClientSecret();

    std::string body;
    auto code = m_HttpPost->doPost(m_config->getLwaUrl(), postData.str(), timeout, body);
    auto newError = handleLwaResponse(code, body);

    if (AuthObserverInterface::Error::NO_ERROR == newError) {
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
        std::cerr << "Error in JSON at position " 
            << document.GetErrorOffset() 
            << ": " 
            << GetParseError_En(document.GetParseError()) << std::endl;
        return AuthObserverInterface::Error::UNKNOWN_ERROR;
    }

    if (HttpPostInterface::HTTP_RESPONSE_CODE_SUCCESS_OK == code) {
        std::string authToken;
        std::string refreshToken;
        uint64_t expiresIn = 0;

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
            expiresIn = expiresInIterator->value.GetUint64();
        }

        if (authToken.empty() || refreshToken.empty() || expiresIn == 0) {
            std::cerr
                    << "handleResponse() failed: authToken.empty()='"
                    << authToken.empty()
                    << "' refreshToken.empty()='"
                    << refreshToken.empty()
                    << "' (expiresIn<=0)="
                    << (expiresIn <= 0)
                    << std::endl;

#ifdef DEBUG
            // TODO: Private data and must be logged at or below DEBUG level when we integrate the logging lib ACSDK-57.
            std::cerr << "response='" << body << "'" << std::endl;
#endif

            return AuthObserverInterface::Error::UNKNOWN_ERROR;
        }

#ifdef DEBUG
        // TODO: Private data and must be logged at or below DEBUG level when we integrate the logging lib ACSDK-57.
        if (m_refreshToken != refreshToken) {
            std::cout << "RefreshToken updated to: '" << refreshToken << "'" << std::endl;
        }
#endif

        m_refreshToken = refreshToken;
        m_expirationTime = m_requestTime + std::chrono::seconds(expiresIn);
        m_timeToRefresh = m_expirationTime - m_config->getAuthTokenRefreshHeadStart();
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_authToken = authToken;
        }
        return AuthObserverInterface::Error::NO_ERROR;

    } else {
        std::string error;
        auto errorIterator = document.FindMember(JSON_KEY_ERROR.c_str());
        if (errorIterator != document.MemberEnd() && errorIterator->value.IsString()) {
            error = errorIterator->value.GetString();
        }
        if (!error.empty()) {
            // If `error` field is non-empty in the body, it means that we encountered an error.
            if (isUnrecoverable(error)) {
                std::cerr << "The LWA response body indicated an unrecoverable error: " << error << std::endl;
            } else {
                std::cerr << "The LWA response body indicated a recoverable or unknown error: " << error << std::endl;
            }
            return getErrorCode(error);
        } else {
            std::cerr << "The LWA response encountered an unknown response body." << std::endl;
#ifdef DEBUG
            // TODO: Private data and must be logged at or below DEBUG level when we integrate the logging lib ACSDK-57.
            std::cerr << "Message body: " << body << std::endl;
#endif
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
        std::cerr << "The thread is stopping due to the unrecoverable error." << std::endl;
        newState = AuthObserverInterface::State::UNRECOVERABLE_ERROR;
        m_isStopping = true;
    }

    if (m_authState != newState) {
        m_authState = newState;
        if (m_observer) {
            m_observer->onAuthStateChange(m_authState, m_authError);
        }
    }
}

} // namespace authDelegate
} // namespace alexaClientSDK
