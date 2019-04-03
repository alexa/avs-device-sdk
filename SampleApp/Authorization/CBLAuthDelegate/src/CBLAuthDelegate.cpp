/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <algorithm>
#include <functional>
#include <iostream>
#include <random>
#include <sstream>
#include <unordered_map>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/HTTP/HttpResponseCode.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPost.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "CBLAuthDelegate/CBLAuthDelegate.h"

namespace alexaClientSDK {
namespace authorization {
namespace cblAuthDelegate {

using namespace alexaClientSDK::avsCommon::sdkInterfaces;
using namespace alexaClientSDK::avsCommon::utils::http;
using namespace alexaClientSDK::avsCommon::utils::libcurlUtils;
using namespace alexaClientSDK::registrationManager;
using namespace rapidjson;

/// String to identify log entries originating from this file.
static const std::string TAG("CBLAuthDelegate");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Key for user_code values in JSON returned by @c LWA
static const char JSON_KEY_USER_CODE[] = "user_code";

/// Key for device_code values in JSON returned by @c LWA
static const char JSON_KEY_DEVICE_CODE[] = "device_code";

/// Key for verification_uri values in JSON returned by @c LWA
static const char JSON_KEY_VERIFICATION_URI[] = "verification_uri";

/// Key for expires_in values in JSON returned by @c LWA
static const char JSON_KEY_EXPIRES_IN[] = "expires_in";

/// Key for interval values in JSON returned by @c LWA
static const char JSON_KEY_INTERVAL[] = "interval";

/// Key for interval values in JSON returned by @c LWA
static const char JSON_KEY_TOKEN_TYPE[] = "token_type";

/// Key for access_token values in JSON returned by @c LWA
static const char JSON_KEY_ACCESS_TOKEN[] = "access_token";

/// Key for refresh_token values in JSON returned by @c LWA
static const char JSON_KEY_REFRESH_TOKEN[] = "refresh_token";

/// Key for error values in JSON returned by @c LWA
static const char JSON_KEY_ERROR[] = "error";

/// Expected token_type value returned from token requests to @c LWA.
static const std::string JSON_VALUE_BEARER = "bearer";

/// response_type key in POST requests to @c LWA.
static const std::string POST_KEY_RESPONSE_TYPE = "response_type";

/// client_id key in POST requests to @c LWA.
static const std::string POST_KEY_CLIENT_ID = "client_id";

/// scope key in POST requests to @c LWA.
static const std::string POST_KEY_SCOPE = "scope";

/// scope_data key in POST requests to @c LWA.
static const std::string POST_KEY_SCOPE_DATA = "scope_data";

/// grant_type key in POST requests to @c LWA.
static const std::string POST_KEY_GRANT_TYPE = "grant_type";

/// device_code key in POST requests to @c LWA.
static const std::string POST_KEY_DEVICE_CODE = "device_code";

/// user_code key in POST requests to @c LWA.
static const std::string POST_KEY_USER_CODE = "user_code";

/// refresh_token key in POST requests to @c LWA.
static const std::string POST_KEY_REFRESH_TOKEN = "refresh_token";

/// refresh_token value in POST requests to @c LWA.
static const std::string POST_VALUE_REFRESH_TOKEN = "refresh_token";

/// device_code value in POST requests to @c LWA.
static const std::string POST_VALUE_DEVICE_CODE = "device_code";

/// alexa:all value in POST requests to @c LWA.
static const std::string POST_VALUE_ALEXA_ALL = "alexa:all";

/// HTTP Header line specifying URL encoded data
static const std::string HEADER_LINE_URLENCODED = "Content-Type: application/x-www-form-urlencoded";

/// Prefix of HTTP header line specifying language.
static const std::string HEADER_LINE_LANGUAGE_PREFIX = "Accept-Language: ";

/// Min time to wait between attempt to poll for a token while authentication is pending.
static const std::chrono::seconds MIN_TOKEN_REQUEST_INTERVAL = std::chrono::seconds(5);

/// Max time to wait between attempt to poll for a token while authentication is pending.
static const std::chrono::seconds MAX_TOKEN_REQUEST_INTERVAL = std::chrono::seconds(60);

/// Scale factor to apply to interval between token poll requests when a 'slow_down' response is received.
static const int TOKEN_REQUEST_SLOW_DOWN_FACTOR = 2;

/// Map error names from @c LWA to @c AuthObserverInterface::Error values.
static const std::unordered_map<std::string, AuthObserverInterface::Error> g_nameToErrorMap = {
    {"authorization_pending", AuthObserverInterface::Error::AUTHORIZATION_PENDING},
    {"invalid_client", AuthObserverInterface::Error::INVALID_VALUE},
    {"invalid_code_pair", AuthObserverInterface::Error::INVALID_CODE_PAIR},
    {"invalid_grant", AuthObserverInterface::Error::AUTHORIZATION_EXPIRED},
    {"invalid_request", AuthObserverInterface::Error::INVALID_REQUEST},
    {"InvalidValue", AuthObserverInterface::Error::INVALID_VALUE},
    {"servererror", AuthObserverInterface::Error::SERVER_ERROR},
    {"slow_down", AuthObserverInterface::Error::SLOW_DOWN},
    {"unauthorized_client", AuthObserverInterface::Error::UNAUTHORIZED_CLIENT},
    {"unsupported_grant_type", AuthObserverInterface::Error::UNSUPPORTED_GRANT_TYPE}};

/**
 * Helper function to convert from @c LWA error names to @c AuthObserverInterface::Error values.
 *
 * @param error The string in the @c error field returned by @c LWA
 * @return the Error enum code corresponding to @c error. If error is "", returns SUCCESS. If it is an unknown error,
 * returns UNKNOWN_ERROR.
 */
static AuthObserverInterface::Error getErrorCode(const std::string& error) {
    if (error.empty()) {
        return AuthObserverInterface::Error::SUCCESS;
    } else {
        auto it = g_nameToErrorMap.find(error);
        if (it != g_nameToErrorMap.end()) {
            return it->second;
        }
        ACSDK_ERROR(LX("getErrorCodeFailed").d("reason", "unknownError").d("error", error));
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

/**
 * Map an HTTP status code to an @c AuthObserverInterface::Error value.
 *
 * @param code The code to map from.
 * @return The value the code was mapped to.
 */
static AuthObserverInterface::Error mapHTTPCodeToError(long code) {
    AuthObserverInterface::Error error = AuthObserverInterface::Error::INTERNAL_ERROR;
    switch (code) {
        case HTTPResponseCode::SUCCESS_OK:
            error = AuthObserverInterface::Error::SUCCESS;
            break;

        case HTTPResponseCode::CLIENT_ERROR_BAD_REQUEST:
            error = AuthObserverInterface::Error::INVALID_REQUEST;
            break;

        case HTTPResponseCode::SERVER_ERROR_INTERNAL:
            error = AuthObserverInterface::Error::SERVER_ERROR;
            break;

        case HTTPResponseCode::HTTP_RESPONSE_CODE_UNDEFINED:
        case HTTPResponseCode::SUCCESS_NO_CONTENT:
        default:
            error = AuthObserverInterface::Error::UNKNOWN_ERROR;
            break;
    }
    ACSDK_DEBUG5(LX("mapHTTPStatusToError").d("code", code).d("error", error));
    return error;
}

/**
 * Perform common parsing of an @c LWA response.
 *
 * @param response The response to parse.
 * @param document The document to populate from the body of the response.
 * @return The status from the initial parsing of the response.
 */
AuthObserverInterface::Error parseLWAResponse(
    const avsCommon::utils::libcurlUtils::HTTPResponse& response,
    rapidjson::Document* document) {
    if (!document) {
        ACSDK_CRITICAL(LX("parseLWAResponseFailed").d("reason", "nullDocument"));
        return AuthObserverInterface::Error::INTERNAL_ERROR;
    }

    auto result = mapHTTPCodeToError(response.code);

    if (document->Parse(response.body.c_str()).HasParseError()) {
        ACSDK_ERROR(LX("parseLWAResponseFailed")
                        .d("reason", "parseJsonFailed")
                        .d("position", document->GetErrorOffset())
                        .d("error", GetParseError_En(document->GetParseError()))
                        .sensitive("body", response.body));
        if (AuthObserverInterface::Error::SUCCESS == result) {
            result = AuthObserverInterface::Error::UNKNOWN_ERROR;
        }
        return result;
    }

    if (result != AuthObserverInterface::Error::SUCCESS) {
        std::string error;
        auto it = document->FindMember(JSON_KEY_ERROR);
        if (it != document->MemberEnd() && it->value.IsString()) {
            error = it->value.GetString();
            if (!error.empty()) {
                result = getErrorCode(error);
                ACSDK_DEBUG5(LX("errorInLwaResponseBody").d("error", error).d("errorCode", result));
            }
        }
    }

    return result;
}

std::unique_ptr<CBLAuthDelegate> CBLAuthDelegate::create(
    const avsCommon::utils::configuration::ConfigurationNode& configuration,
    std::shared_ptr<CustomerDataManager> customerDataManager,
    std::shared_ptr<CBLAuthDelegateStorageInterface> storage,
    std::shared_ptr<CBLAuthRequesterInterface> authRequester,
    std::shared_ptr<HttpPostInterface> httpPost,
    std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo) {
    ACSDK_DEBUG5(LX("create"));

    if (!avsCommon::avs::initialization::AlexaClientSDKInit::isInitialized()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "sdkNotInitialized"));
        return nullptr;
    }
    if (!customerDataManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullCustomerDataManager"));
        return nullptr;
    }
    if (!storage) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullStorage"));
        return nullptr;
    }
    if (!authRequester) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullAuthRequester"));
        return nullptr;
    }
    if (!httpPost) {
        httpPost = HttpPost::create();
        if (!httpPost) {
            ACSDK_ERROR(LX("createFailed").d("reason", "nullptrHttPost"));
            return nullptr;
        }
    }
    if (!deviceInfo) {
        deviceInfo = avsCommon::utils::DeviceInfo::create(configuration);
        if (!deviceInfo) {
            ACSDK_ERROR(LX("createFailed").d("reason", "nullptrDeviceInfo"));
            return nullptr;
        }
    }

    std::unique_ptr<CBLAuthDelegate> instance(
        new CBLAuthDelegate(customerDataManager, storage, authRequester, httpPost));

    if (instance->init(configuration, deviceInfo)) {
        return instance;
    }
    return nullptr;
}

CBLAuthDelegate::~CBLAuthDelegate() {
    stop();
}

void CBLAuthDelegate::addAuthObserver(std::shared_ptr<AuthObserverInterface> observer) {
    ACSDK_DEBUG5(LX("addAuthObserver").d("observer", observer.get()));

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!observer) {
        ACSDK_WARN(LX("addAuthObserverFailed").d("reason", "nullObserver"));
        return;
    }
    if (!m_observers.insert(observer).second) {
        ACSDK_WARN(LX("addAuthObserverFailed").d("reason", "observerAlreadyAdded"));
        return;
    }
    observer->onAuthStateChange(m_authState, m_authError);
}

void CBLAuthDelegate::removeAuthObserver(std::shared_ptr<AuthObserverInterface> observer) {
    ACSDK_DEBUG5(LX("removeAuthObserver").d("observer", observer.get()));

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!observer) {
        ACSDK_WARN(LX("removeAuthObserverFailed").d("reason", "nullObserver"));
    } else if (m_observers.erase(observer) == 0) {
        ACSDK_WARN(LX("removeAuthObserverFailed").d("reason", "observerNotAdded"));
    }
}

std::string CBLAuthDelegate::getAuthToken() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_accessToken;
}

void CBLAuthDelegate::onAuthFailure(const std::string& token) {
    ACSDK_DEBUG0(LX("onAuthFailure").sensitive("token", token));

    std::lock_guard<std::mutex> lock(m_mutex);
    if (token.empty() || token == m_accessToken) {
        ACSDK_DEBUG9(LX("onAuthFailure").m("setting m_authFailureReported"));
        m_authFailureReported = true;
        m_wake.notify_one();
    }
}

void CBLAuthDelegate::clearData() {
    ACSDK_DEBUG3(LX("clearData"));
    stop();
    m_storage->clear();
}

CBLAuthDelegate::CBLAuthDelegate(
    std::shared_ptr<CustomerDataManager> customerDataManager,
    std::shared_ptr<CBLAuthDelegateStorageInterface> storage,
    std::shared_ptr<CBLAuthRequesterInterface> authRequester,
    std::shared_ptr<HttpPostInterface> httpPost) :
        CustomerDataHandler{customerDataManager},
        m_storage{storage},
        m_httpPost{httpPost},
        m_authRequester{authRequester},
        m_isStopping{false},
        m_authState{AuthObserverInterface::State::UNINITIALIZED},
        m_authError{AuthObserverInterface::Error::SUCCESS},
        m_tokenExpirationTime{std::chrono::time_point<std::chrono::steady_clock>::max()},
        m_retryCount{0},
        m_newRefreshToken{false},
        m_authFailureReported{false} {
    ACSDK_DEBUG5(LX("CBLAuthDelegate"));
}

bool CBLAuthDelegate::init(
    const avsCommon::utils::configuration::ConfigurationNode& configuration,
    const std::shared_ptr<avsCommon::utils::DeviceInfo>& deviceInfo) {
    ACSDK_DEBUG5(LX("init"));

    m_configuration = CBLAuthDelegateConfiguration::create(configuration, deviceInfo);
    if (!m_configuration) {
        ACSDK_ERROR(LX("initFailed").d("reason", "createCBLAuthDelegateConfigurationFailed"));
        return false;
    }

    if (!m_storage->open()) {
        ACSDK_DEBUG5(LX("init").m("openStorageFailed"));
        if (!m_storage->createDatabase()) {
            ACSDK_ERROR(LX("initFailed").d("reason", "createDatabaseFailed"));
            return false;
        }
    }

    m_authorizationFlowThread = std::thread(&CBLAuthDelegate::handleAuthorizationFlow, this);

    return true;
}

void CBLAuthDelegate::stop() {
    ACSDK_DEBUG5(LX("stop"));

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_isStopping = true;
    }
    m_wake.notify_one();
    if (m_authorizationFlowThread.joinable()) {
        m_authorizationFlowThread.join();
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_accessToken.clear();
    }
}

void CBLAuthDelegate::handleAuthorizationFlow() {
    ACSDK_DEBUG5(LX("handleAuthorizationFlow"));

    auto flowState = FlowState::STARTING;
    while (!isStopping()) {
        auto nextFlowState = FlowState::STOPPING;
        switch (flowState) {
            case FlowState::STARTING:
                nextFlowState = handleStarting();
                break;
            case FlowState::REQUESTING_CODE_PAIR:
                nextFlowState = handleRequestingCodePair();
                break;
            case FlowState::REQUESTING_TOKEN:
                nextFlowState = handleRequestingToken();
                break;
            case FlowState::REFRESHING_TOKEN:
                nextFlowState = handleRefreshingToken();
                break;
            case FlowState::STOPPING:
                nextFlowState = handleStopping();
                break;
        }
        flowState = nextFlowState;
    }
}

CBLAuthDelegate::FlowState CBLAuthDelegate::handleStarting() {
    ACSDK_DEBUG5(LX("handleStarting"));

    if (m_storage->getRefreshToken(&m_refreshToken)) {
        return FlowState::REFRESHING_TOKEN;
    }

    ACSDK_DEBUG0(LX("getRefreshTokenFailed"));
    return FlowState::REQUESTING_CODE_PAIR;
}

CBLAuthDelegate::FlowState CBLAuthDelegate::handleRequestingCodePair() {
    ACSDK_DEBUG5(LX("handleRequestingCodePair"));

    m_retryCount = 0;
    while (!isStopping()) {
        auto result = receiveCodePairResponse(requestCodePair());
        switch (result) {
            case AuthObserverInterface::Error::SUCCESS:
                return FlowState::REQUESTING_TOKEN;
            case AuthObserverInterface::Error::UNKNOWN_ERROR:
            case AuthObserverInterface::Error::AUTHORIZATION_FAILED:
            case AuthObserverInterface::Error::SERVER_ERROR:
            case AuthObserverInterface::Error::AUTHORIZATION_EXPIRED:
            case AuthObserverInterface::Error::INVALID_CODE_PAIR:
            case AuthObserverInterface::Error::AUTHORIZATION_PENDING:
            case AuthObserverInterface::Error::SLOW_DOWN:
                break;
            case AuthObserverInterface::Error::UNAUTHORIZED_CLIENT:
            case AuthObserverInterface::Error::INVALID_REQUEST:
            case AuthObserverInterface::Error::INVALID_VALUE:
            case AuthObserverInterface::Error::UNSUPPORTED_GRANT_TYPE:
            case AuthObserverInterface::Error::INTERNAL_ERROR:
            case AuthObserverInterface::Error::INVALID_CBL_CLIENT_ID: {
                setAuthState(AuthObserverInterface::State::UNRECOVERABLE_ERROR);
                return FlowState::STOPPING;
            }
        }

        std::unique_lock<std::mutex> lock(m_mutex);
        m_wake.wait_until(lock, calculateTimeToRetry(m_retryCount++), [this] { return m_isStopping; });
    }

    return FlowState::STOPPING;
}

CBLAuthDelegate::FlowState CBLAuthDelegate::handleRequestingToken() {
    ACSDK_DEBUG5(LX("handleRequestingToken"));

    auto interval = MIN_TOKEN_REQUEST_INTERVAL;

    while (!isStopping()) {
        // If the code pair expired, stop.  The application needs to restart authorization.
        if (std::chrono::steady_clock::now() >= m_codePairExpirationTime) {
            setAuthError(AuthObserverInterface::Error::INVALID_CODE_PAIR);
            setAuthState(AuthObserverInterface::State::UNRECOVERABLE_ERROR);
            return FlowState::STOPPING;
        }

        m_authRequester->onCheckingForAuthorization();
        auto result = receiveTokenResponse(requestToken(), true);
        switch (result) {
            case AuthObserverInterface::Error::SUCCESS:
                m_newRefreshToken = true;
                return FlowState::REFRESHING_TOKEN;
            case AuthObserverInterface::Error::UNKNOWN_ERROR:
            case AuthObserverInterface::Error::SERVER_ERROR:
            case AuthObserverInterface::Error::AUTHORIZATION_PENDING:
                break;
            case AuthObserverInterface::Error::SLOW_DOWN:
                interval = std::min(interval * TOKEN_REQUEST_SLOW_DOWN_FACTOR, MAX_TOKEN_REQUEST_INTERVAL);
                break;
            case AuthObserverInterface::Error::AUTHORIZATION_FAILED:
            case AuthObserverInterface::Error::UNAUTHORIZED_CLIENT:
            case AuthObserverInterface::Error::INVALID_REQUEST:
            case AuthObserverInterface::Error::INVALID_VALUE:
            case AuthObserverInterface::Error::AUTHORIZATION_EXPIRED:
            case AuthObserverInterface::Error::UNSUPPORTED_GRANT_TYPE:
            case AuthObserverInterface::Error::INVALID_CODE_PAIR:
            case AuthObserverInterface::Error::INTERNAL_ERROR:
            case AuthObserverInterface::Error::INVALID_CBL_CLIENT_ID: {
                setAuthState(AuthObserverInterface::State::UNRECOVERABLE_ERROR);
                return FlowState::STOPPING;
            }
        }

        std::unique_lock<std::mutex> lock(m_mutex);
        m_wake.wait_for(lock, interval, [this] { return m_isStopping; });
    }

    return FlowState::STOPPING;
}

CBLAuthDelegate::FlowState CBLAuthDelegate::handleRefreshingToken() {
    ACSDK_DEBUG5(LX("handleRefreshingToken"));

    m_retryCount = 0;

    while (!isStopping()) {
        std::unique_lock<std::mutex> lock(m_mutex);
        bool isAboutToExpire =
            (AuthObserverInterface::State::REFRESHED == m_authState && m_tokenExpirationTime < m_timeToRefresh);

        auto nextActionTime = (isAboutToExpire ? m_tokenExpirationTime : m_timeToRefresh);

        m_wake.wait_until(lock, nextActionTime, [this] { return m_authFailureReported || m_isStopping; });

        if (m_isStopping) {
            break;
        }

        auto nextState = m_authState;

        if (m_authFailureReported) {
            m_authFailureReported = false;
            isAboutToExpire = false;
        }

        if (isAboutToExpire) {
            m_accessToken.clear();
            lock.unlock();
            nextState = AuthObserverInterface::State::EXPIRED;
        } else {
            bool newRefreshToken = m_newRefreshToken;
            m_newRefreshToken = false;
            lock.unlock();
            auto result = receiveTokenResponse(requestRefresh(), false);
            switch (result) {
                case AuthObserverInterface::Error::SUCCESS:
                    m_retryCount = 0;
                    nextState = AuthObserverInterface::State::REFRESHED;
                    break;
                case AuthObserverInterface::Error::UNKNOWN_ERROR:
                case AuthObserverInterface::Error::SERVER_ERROR:
                case AuthObserverInterface::Error::AUTHORIZATION_PENDING:
                case AuthObserverInterface::Error::SLOW_DOWN:
                    m_timeToRefresh = calculateTimeToRetry(m_retryCount++);
                    break;
                case AuthObserverInterface::Error::INVALID_REQUEST:
                    if (newRefreshToken) {
                        setAuthError(AuthObserverInterface::Error::INVALID_CBL_CLIENT_ID);
                    }
                // Falls through
                case AuthObserverInterface::Error::AUTHORIZATION_FAILED:
                case AuthObserverInterface::Error::UNAUTHORIZED_CLIENT:
                case AuthObserverInterface::Error::INVALID_VALUE:
                case AuthObserverInterface::Error::AUTHORIZATION_EXPIRED:
                case AuthObserverInterface::Error::UNSUPPORTED_GRANT_TYPE:
                case AuthObserverInterface::Error::INVALID_CODE_PAIR:
                case AuthObserverInterface::Error::INTERNAL_ERROR:
                case AuthObserverInterface::Error::INVALID_CBL_CLIENT_ID: {
                    setAuthState(AuthObserverInterface::State::UNRECOVERABLE_ERROR);
                    return FlowState::STOPPING;
                }
            }
        }
        setAuthState(nextState);
    }

    return FlowState::STOPPING;
}

CBLAuthDelegate::FlowState CBLAuthDelegate::handleStopping() {
    ACSDK_DEBUG5(LX("handleStopping"));
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isStopping = true;
    return FlowState::STOPPING;
}

HTTPResponse CBLAuthDelegate::requestCodePair() {
    ACSDK_DEBUG5(LX("requestCodePair"));

    const std::vector<std::pair<std::string, std::string>> postData = {
        {POST_KEY_RESPONSE_TYPE, POST_VALUE_DEVICE_CODE},
        {POST_KEY_CLIENT_ID, m_configuration->getClientId()},
        {POST_KEY_SCOPE, POST_VALUE_ALEXA_ALL},
        {POST_KEY_SCOPE_DATA, m_configuration->getScopeData()},
    };
    const std::vector<std::string> headerLines = {HEADER_LINE_URLENCODED,
                                                  HEADER_LINE_LANGUAGE_PREFIX + m_configuration->getLocale()};

    return m_httpPost->doPost(
        m_configuration->getRequestCodePairUrl(), headerLines, postData, m_configuration->getRequestTimeout());
}

avsCommon::utils::libcurlUtils::HTTPResponse CBLAuthDelegate::requestToken() {
    ACSDK_DEBUG5(LX("requestToken"));

    const std::vector<std::pair<std::string, std::string>> postData = {{POST_KEY_GRANT_TYPE, POST_VALUE_DEVICE_CODE},
                                                                       {POST_KEY_DEVICE_CODE, m_deviceCode},
                                                                       {POST_KEY_USER_CODE, m_userCode}};
    const std::vector<std::string> headerLines = {HEADER_LINE_URLENCODED};

    m_requestTime = std::chrono::steady_clock::now();

    return m_httpPost->doPost(
        m_configuration->getRequestTokenUrl(), headerLines, postData, m_configuration->getRequestTimeout());
}

avsCommon::utils::libcurlUtils::HTTPResponse CBLAuthDelegate::requestRefresh() {
    ACSDK_DEBUG5(LX("requestRefresh"));

    const std::vector<std::pair<std::string, std::string>> postData = {
        {POST_KEY_GRANT_TYPE, POST_VALUE_REFRESH_TOKEN},
        {POST_KEY_REFRESH_TOKEN, m_refreshToken},
        {POST_KEY_CLIENT_ID, m_configuration->getClientId()}};
    const std::vector<std::string> headerLines = {HEADER_LINE_URLENCODED};

    // Don't wait for this request so long that we would be late to notify our observer if the token expires.
    m_requestTime = std::chrono::steady_clock::now();
    auto timeout = m_configuration->getRequestTimeout();
    if (AuthObserverInterface::State::REFRESHED == m_authState) {
        auto timeUntilExpired = std::chrono::duration_cast<std::chrono::seconds>(m_tokenExpirationTime - m_requestTime);
        if ((timeout > timeUntilExpired) && (timeUntilExpired > std::chrono::seconds::zero())) {
            timeout = timeUntilExpired;
        }
    }

    return m_httpPost->doPost(m_configuration->getRequestTokenUrl(), headerLines, postData, timeout);
}

AuthObserverInterface::Error CBLAuthDelegate::receiveCodePairResponse(const HTTPResponse& response) {
    ACSDK_DEBUG5(LX("receiveCodePairResponse").d("code", response.code).sensitive("body", response.body));

    Document document;
    auto result = parseLWAResponse(response, &document);
    setAuthError(result);

    if (result != AuthObserverInterface::Error::SUCCESS) {
        ACSDK_DEBUG0(LX("receiveCodePairResponseFailed").d("result", result));
        return result;
    }

    auto it = document.FindMember(JSON_KEY_USER_CODE);
    if (it != document.MemberEnd() && it->value.IsString()) {
        m_userCode = it->value.GetString();
    }

    it = document.FindMember(JSON_KEY_DEVICE_CODE);
    if (it != document.MemberEnd() && it->value.IsString()) {
        m_deviceCode = it->value.GetString();
    }

    std::string verificationUri;
    it = document.FindMember(JSON_KEY_VERIFICATION_URI);
    if (it != document.MemberEnd() && it->value.IsString()) {
        verificationUri = it->value.GetString();
    }

    int64_t expiresInSeconds = 0;
    it = document.FindMember(JSON_KEY_EXPIRES_IN);
    if (it != document.MemberEnd() && it->value.IsUint64()) {
        expiresInSeconds = it->value.GetUint64();
    }

    int64_t intervalSeconds = 0;
    it = document.FindMember(JSON_KEY_INTERVAL);
    if (it != document.MemberEnd() && it->value.IsUint64()) {
        intervalSeconds = it->value.GetUint64();
    }

    if (m_userCode.empty() || m_deviceCode.empty() || verificationUri.empty() || 0 == expiresInSeconds) {
        ACSDK_ERROR(LX("receiveCodePairResponseFailed")
                        .d("reason", "missing or InvalidResponseProperty")
                        .d("user_code", m_userCode)
                        .sensitive("device_code", m_deviceCode)
                        .d("verification_uri", verificationUri)
                        .d("expiresIn", expiresInSeconds)
                        .d("interval", intervalSeconds));
        return AuthObserverInterface::Error::UNKNOWN_ERROR;
    }

    m_codePairExpirationTime = std::chrono::steady_clock::now() + std::chrono::seconds(expiresInSeconds);

    m_authRequester->onRequestAuthorization(verificationUri, m_userCode);

    return result;
}

AuthObserverInterface::Error CBLAuthDelegate::receiveTokenResponse(
    const avsCommon::utils::libcurlUtils::HTTPResponse& response,
    bool expiresImmediately) {
    ACSDK_DEBUG5(LX("receiveTokenResponse").d("code", response.code).sensitive("body", response.body));

    Document document;
    auto result = parseLWAResponse(response, &document);
    setAuthError(result);

    if (result != AuthObserverInterface::Error::SUCCESS) {
        ACSDK_DEBUG0(LX("receiveTokenResponseFailed").d("result", result));
        return result;
    }

    std::string accessToken;
    auto it = document.FindMember(JSON_KEY_ACCESS_TOKEN);
    if (it != document.MemberEnd() && it->value.IsString()) {
        accessToken = it->value.GetString();
    }

    std::string refreshToken;
    it = document.FindMember(JSON_KEY_REFRESH_TOKEN);
    if (it != document.MemberEnd() && it->value.IsString()) {
        refreshToken = it->value.GetString();
    }

    std::string tokenType;
    it = document.FindMember(JSON_KEY_TOKEN_TYPE);
    if (it != document.MemberEnd() && it->value.IsString()) {
        tokenType = it->value.GetString();
    }

    int64_t expiresInSeconds = 0;
    it = document.FindMember(JSON_KEY_EXPIRES_IN);
    if (it != document.MemberEnd() && it->value.IsUint64()) {
        expiresInSeconds = it->value.GetUint64();
    }

    if (accessToken.empty() || refreshToken.empty() || tokenType != JSON_VALUE_BEARER || 0 == expiresInSeconds) {
        ACSDK_ERROR(LX("receiveTokenResponseFailed")
                        .d("reason", "missing or InvalidResponseProperty")
                        .sensitive("access_token", accessToken)
                        .d("refresh_token", refreshToken)
                        .d("token_type", tokenType)
                        .d("expiresIn", expiresInSeconds));
        return AuthObserverInterface::Error::UNKNOWN_ERROR;
    }

    // This is used to make the initial access token expire immediately so that we also verify the refresh token
    // before reporting the @c REFRESHED state.
    if (expiresImmediately) {
        expiresInSeconds = 0;
    }

    setRefreshToken(refreshToken);
    m_tokenExpirationTime = m_requestTime + std::chrono::seconds(expiresInSeconds);
    m_timeToRefresh = m_tokenExpirationTime - m_configuration->getAccessTokenRefreshHeadStart();
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_accessToken = accessToken;
    }

    return AuthObserverInterface::Error::SUCCESS;
}

void CBLAuthDelegate::setAuthState(AuthObserverInterface::State newAuthState) {
    ACSDK_DEBUG5(LX("setAuthState").d("newAuthState", newAuthState));

    std::lock_guard<std::mutex> lock(m_mutex);
    if (newAuthState == m_authState) {
        return;
    }
    m_authState = newAuthState;

    if (!m_observers.empty()) {
        ACSDK_DEBUG9(LX("callingOnAuthStateChange").d("state", m_authState).d("error", m_authError));
        for (auto observer : m_observers) {
            observer->onAuthStateChange(m_authState, m_authError);
        }
    }
}

void CBLAuthDelegate::setAuthError(AuthObserverInterface::Error authError) {
    ACSDK_DEBUG5(LX("setAuthError").d("authError", authError));

    std::lock_guard<std::mutex> lock(m_mutex);
    m_authError = authError;
}

void CBLAuthDelegate::setRefreshToken(const std::string& refreshToken) {
    ACSDK_DEBUG5(LX("setRefreshToken").sensitive("refreshToken", refreshToken));

    m_refreshToken = refreshToken;
    if (!m_storage->setRefreshToken(refreshToken)) {
        ACSDK_ERROR(LX("failedToPersistNewRefreshToken"));
    }
}

void CBLAuthDelegate::clearRefreshToken() {
    ACSDK_DEBUG5(LX("clearRefreshToken"));

    m_refreshToken.clear();
    if (!m_storage->clearRefreshToken()) {
        ACSDK_ERROR(LX("FailedToPersistClearedRefreshToken"));
    }
}

bool CBLAuthDelegate::isStopping() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_isStopping;
}

}  // namespace cblAuthDelegate
}  // namespace authorization
}  // namespace alexaClientSDK
