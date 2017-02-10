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

/**
 * 'invalid_request' Error Code from LWA (@see https://images-na.ssl-images-amazon.com/images/G/01/lwa/dev/
 * docs/website-developer-guide._TTH_.pdf)
 */
static const std::string ERROR_CODE_INVALID_REQUEST = "invalid_request";
/**
 * 'unsupported_grant_type' Error Code from LWA (@see https://images-na.ssl-images-amazon.com/images/G/01/lwa/dev/
 * docs/website-developer-guide._TTH_.pdf)
 */
static const std::string ERROR_CODE_UNSUPPORTED_GRANT_TYPE = "unsupported_grant_type";
/**
 * 'invalid_grant' Error Code from LWA (@see https://images-na.ssl-images-amazon.com/images/G/01/lwa/dev/
 * docs/website-developer-guide._TTH_.pdf)
 */
static const std::string ERROR_CODE_INVALID_GRANT = "invalid_grant";

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
            m_observer->onAuthStateChange(m_authState);
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
    // TODO: Revisit this logic: ACSDK-71

    std::function<bool()> isStopping = [this] {
        return m_isStopping;
    };

    while (true) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (AuthObserverInterface::State::REFRESHED == m_authState && m_expirationTime < m_timeToRefresh) {
            if (m_wakeThreadCond.wait_until(lock, m_expirationTime, isStopping)) {
                break;
            }
            m_authToken.clear();
            lock.unlock();
            setState(AuthObserverInterface::State::EXPIRED);
        } else {
            if (m_wakeThreadCond.wait_until(lock, m_timeToRefresh, isStopping)) {
                break;
            }
            lock.unlock();
            refreshAuthToken();
        }
    }
}

void AuthDelegate::refreshAuthToken() {
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

    if (handleLwaResponse(code, body)) {
        m_retryCount = 0;
        setState(AuthObserverInterface::State::REFRESHED);
    } else {
        m_timeToRefresh = calculateTimeToRetry(m_retryCount++);
    }
}

bool AuthDelegate::handleLwaResponse(HttpPostInterface::ResponseCode code, const std::string& body) {

    if (HttpPostInterface::ResponseCode::SUCCESS_OK == code) {
        // TODO replace with a real JSON parser once we pick one: ACSDK-58
        auto authToken = parseResponseValue(body, "access_token\":\"", "\"");
        auto refreshToken = parseResponseValue(body, "refresh_token\":\"", "\"");
        auto expiresInStr = parseResponseValue(body, "expires_in\":", "}");

        long expiresIn = 0;
        try {
            expiresIn = std::stol(expiresInStr);
        } catch (std::logic_error error) {
            std::cerr << "strtol('" << expiresInStr << "') failed: " << error.what() << std::endl;
            return false;
        }

        if (authToken.empty() || refreshToken.empty() || expiresIn <= 0) {
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

            return false;
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
        setState(acl::AuthObserverInterface::State::REFRESHED);
        return true;
    }

    if (HttpPostInterface::ResponseCode::CLIENT_ERROR_BAD_REQUEST == code) {
        // TODO replace with a real JSON parser once we pick one: ACSDK-58
        auto error = parseResponseValue(body, "\"error\":\"", "\"");
        if (ERROR_CODE_INVALID_REQUEST == error || ERROR_CODE_UNSUPPORTED_GRANT_TYPE == error||
                ERROR_CODE_INVALID_GRANT == error) {
            std::cerr << "The LWA response body indicated an unrecoverable error: " << error << std::endl;
            // TODO: ACSDK-90 Improve AuthObserverInterface to specify an error when reporting UNRECOVERABLE_ERROR.
            setState(acl::AuthObserverInterface::State::UNRECOVERABLE_ERROR);
            // No need to continue retrying, so stop this thread.
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_isStopping = true;
            }
        }
    }

    return false;
}

// TODO: Ditch this once we have integated a JSON parser: ACSDK-58
std::string AuthDelegate::parseResponseValue(
        const std::string& response,
        const std::string& prefix,
        const std::string& suffix) const {

    auto valueStart = response.find(prefix);
    if (response.npos == valueStart) {
        return "";
    }
    valueStart += prefix.length();
    auto valueEnd = response.find(suffix, valueStart);
    if (response.npos == valueEnd) {
        return "";
    }
    return response.substr(valueStart, valueEnd - valueStart);
}

bool AuthDelegate::hasAuthTokenExpired() {
    return std::chrono::steady_clock::now() >= m_expirationTime;
}

void AuthDelegate::setState(AuthObserverInterface::State newState) {
    if (m_authState != newState) {
        m_authState = newState;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_observer) {
                m_observer->onAuthStateChange(m_authState);
            }
        }
    }
}

} // namespace authDelegate
} // namespace alexaClientSDK
