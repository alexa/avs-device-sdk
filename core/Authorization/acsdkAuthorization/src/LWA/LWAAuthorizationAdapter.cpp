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

#include <algorithm>
#include <functional>
#include <sstream>
#include <unordered_map>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>

#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/HTTP/HttpResponseCode.h>
#include <AVSCommon/Utils/LibcurlUtils/CallbackData.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPost.h>

#include <acsdkAuthorization/LWA/LWAAuthorizationAdapter.h>
#include <acsdkAuthorization/private/Logging.h>

namespace alexaClientSDK {
namespace acsdkAuthorization {
namespace lwa {

using namespace acsdkAuthorizationInterfaces;
using namespace acsdkAuthorizationInterfaces::lwa;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::http;
using namespace avsCommon::utils::libcurlUtils;
using namespace rapidjson;

/// String to identify log entries originating from this file.
#define TAG "LWAAuthorizationAdapter"

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

/// Key for name values in JSON returned by @c LWA
static const char JSON_KEY_NAME[] = "name";

/// Key for user_id values in JSON returned by @c LWA
static const char JSON_KEY_USER_ID[] = "user_id";

/// Key for email values in JSON returned by @c LWA
static const char JSON_KEY_EMAIL[] = "email";

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
static const std::string SCOPE_ALEXA_ALL = "alexa:all";

/// profile scope that requests customer information.
static const std::string SCOPE_PROFILE = "profile";

/// profile:user_id scope that allows tying an access token to a specific account.
static const std::string SCOPE_PROFILE_USER_ID = "profile:user_id";

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

/// Unique identifier for this adapter.
const std::string DEFAULT_ADAPTER_ID = "lwa-adapter";

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
     * Table of retry backoff values
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
        case HTTPResponseCode::SERVER_UNAVAILABLE:
            error = AuthObserverInterface::Error::SERVER_ERROR;
            break;

        case HTTPResponseCode::HTTP_RESPONSE_CODE_UNDEFINED:
        case HTTPResponseCode::SUCCESS_NO_CONTENT:
        case HTTPResponseCode::SUCCESS_CREATED:
        case HTTPResponseCode::SUCCESS_ACCEPTED:
        case HTTPResponseCode::SUCCESS_PARTIAL_CONTENT:
        case HTTPResponseCode::REDIRECTION_MULTIPLE_CHOICES:
        case HTTPResponseCode::REDIRECTION_MOVED_PERMANENTLY:
        case HTTPResponseCode::REDIRECTION_FOUND:
        case HTTPResponseCode::REDIRECTION_SEE_ANOTHER:
        case HTTPResponseCode::REDIRECTION_TEMPORARY_REDIRECT:
        case HTTPResponseCode::REDIRECTION_PERMANENT_REDIRECT:
        case HTTPResponseCode::CLIENT_ERROR_FORBIDDEN:
        case HTTPResponseCode::CLIENT_ERROR_THROTTLING_EXCEPTION:
        case HTTPResponseCode::SERVER_ERROR_NOT_IMPLEMENTED:
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
static AuthObserverInterface::Error parseLWAResponse(
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

/// Write callback function used for CURLOPT_WRITEFUNCTION option in libcurl.
static size_t writeCallback(char* dataBuffer, size_t blockSize, size_t numBlocks, void* dataStream) {
    if (!dataStream) {
        ACSDK_ERROR(LX("writeCallbackFailed").d("reason", "nullDataStream"));
        return 0;
    }

    size_t realSize = blockSize * numBlocks;
    CallbackData* callbackData = reinterpret_cast<CallbackData*>(dataStream);

    return callbackData->appendData(dataBuffer, realSize);
}

/**
 * Perform a GET request.
 *
 * @param url The url.
 * @return An HTTPResponse object representing the result of the request.
 */
static HTTPResponse doGet(const std::string& url) {
    const std::string errorEvent = "doGetFailed";
    const std::string errorReasonKey = "reason";
    HTTPResponse httpResponse;
    CurlEasyHandleWrapper curl;

    if (!curl.setURL(url)) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "unableToSetUrl"));
        return httpResponse;
    }

    if (!curl.setTransferType(CurlEasyHandleWrapper::TransferType::kGET)) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "unableToSetHttpRequestType"));
        return httpResponse;
    }

    CallbackData responseData;
    if (!curl.setWriteCallback(writeCallback, &responseData)) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "unableToSetWriteCallback"));
        return httpResponse;
    }

    CURLcode curlResult = curl.perform();
    if (curlResult != CURLE_OK) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "performFailed").d("result", curl_easy_strerror(curlResult)));
        return httpResponse;
    }

    size_t responseSize = responseData.getSize();
    if (responseSize > 0) {
        std::vector<char> responseBody(responseSize + 1, 0);
        responseData.getData(responseBody.data(), responseSize);
        httpResponse.body = std::string(responseBody.data());
    } else {
        httpResponse.body = "";
    }
    httpResponse.code = curl.getHTTPResponseCode();

    return httpResponse;
}

std::shared_ptr<LWAAuthorizationAdapter> LWAAuthorizationAdapter::create(
    const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configuration,
    std::unique_ptr<avsCommon::utils::libcurlUtils::HttpPostInterface> httpPost,
    const std::shared_ptr<avsCommon::utils::DeviceInfo>& deviceInfo,
    const std::shared_ptr<acsdkAuthorizationInterfaces::lwa::LWAAuthorizationStorageInterface>& storage,
    std::unique_ptr<avsCommon::utils::libcurlUtils::HttpGetInterface> httpGet,
    const std::string& adapterId) {
    if (!configuration || !httpPost || !deviceInfo || !storage) {
        ACSDK_ERROR(LX("createFailed")
                        .d("reason", "nullptr")
                        .d("configurationNull", !configuration)
                        .d("httpPostNull", !httpPost)
                        .d("deviceInfoNull", !deviceInfo)
                        .d("storageNull", !storage));

        return nullptr;
    }

    auto lwaAdapter = std::shared_ptr<LWAAuthorizationAdapter>(new LWAAuthorizationAdapter(
        adapterId.empty() ? DEFAULT_ADAPTER_ID : adapterId, std::move(httpPost), storage, std::move(httpGet)));

    if (!lwaAdapter->init(*configuration, deviceInfo)) {
        return nullptr;
    }

    return lwaAdapter;
}

LWAAuthorizationAdapter::LWAAuthorizationAdapter(
    const std::string& adapterId,
    std::unique_ptr<avsCommon::utils::libcurlUtils::HttpPostInterface> httpPost,
    const std::shared_ptr<acsdkAuthorizationInterfaces::lwa::LWAAuthorizationStorageInterface>& storage,
    std::unique_ptr<avsCommon::utils::libcurlUtils::HttpGetInterface> httpGet) :
        m_httpPost{std::move(httpPost)},
        m_httpGet{std::move(httpGet)},
        m_storage{storage},
        m_authState{AuthObserverInterface::State::UNINITIALIZED},
        m_authError{AuthObserverInterface::Error::SUCCESS},
        m_codePairExpirationTime{},
        m_requestCustomerProfile{false},
        m_authFailureReported{false},
        m_authMethod{TokenExchangeMethod::NONE},
        m_isShuttingDown{false},
        m_isClearingData{false},
        m_adapterId{adapterId} {
}

LWAAuthorizationAdapter::~LWAAuthorizationAdapter() {
    ACSDK_DEBUG5(LX("~LWAAuthorizationAdapter"));
    stop();
}

bool LWAAuthorizationAdapter::shouldStopRetrying() {
    ACSDK_DEBUG5(LX("shouldStopRetrying"));
    std::lock_guard<std::mutex> lock(m_mutex);
    return shouldStopRetryingLocked();
}

bool LWAAuthorizationAdapter::shouldStopRetryingLocked() {
    ACSDK_DEBUG5(LX("shouldStopRetryingLocked"));
    return m_isClearingData || m_isShuttingDown;
}

LWAAuthorizationAdapter::FlowState LWAAuthorizationAdapter::retrievePersistedData() {
    ACSDK_DEBUG5(LX("retrievePersistedData"));

    std::unique_lock<std::mutex> lock(m_mutex);

    RefreshTokenResponse refreshTokenResponse;
    bool storageTokenSucceeded = m_storage->getRefreshToken(&refreshTokenResponse.refreshToken);
    bool userIdSucceeded = m_storage->getUserId(&m_userId);
    if (!userIdSucceeded) {
        ACSDK_INFO(LX("retrievePersistedData").m("noUserId"));
        m_userId.clear();
    }

    if (!storageTokenSucceeded) {
        // Not authorized, wait for authorization request.
        ACSDK_INFO(LX("retrievePersistedData").m("noRefreshToken"));
        return FlowState::IDLE;
    } else {
        setRefreshTokenResponseLocked(refreshTokenResponse);

        m_authState = AuthObserverInterface::State::AUTHORIZING;
        m_authError = AuthObserverInterface::Error::SUCCESS;
        return FlowState::REFRESHING_TOKEN;
    }
}

LWAAuthorizationAdapter::FlowState LWAAuthorizationAdapter::handleIdle() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_wake.wait(
        lock, [this] { return m_isShuttingDown || m_isClearingData || (TokenExchangeMethod::NONE != m_authMethod); });

    if (m_isClearingData) {
        return FlowState::CLEARING_DATA;
    } else if (m_isShuttingDown) {
        return FlowState::STOPPING;
    }

    return FlowState::REQUESTING_TOKEN;
}

AuthObserverInterface::Error LWAAuthorizationAdapter::receiveCodePairResponse(const HTTPResponse& response) {
    ACSDK_DEBUG5(LX("receiveCodePairResponse").d("code", response.code).sensitive("body", response.body));

    Document document;
    auto result = parseLWAResponse(response, &document);

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

    // Retain legacy behavior of using predetermined value.
    m_tokenRequestInterval = MIN_TOKEN_REQUEST_INTERVAL;

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

    m_cblRequester->onRequestAuthorization(verificationUri, m_userCode);

    return result;
}

HTTPResponse LWAAuthorizationAdapter::sendCodePairRequest() {
    ACSDK_DEBUG5(LX("sendCodePairRequest"));

    std::string scope = SCOPE_ALEXA_ALL;
    if (m_requestCustomerProfile) {
        scope = scope + " " + SCOPE_PROFILE;
    } else {
        scope = scope + " " + SCOPE_PROFILE_USER_ID;
    }

    const std::vector<std::pair<std::string, std::string>> postData = {
        {POST_KEY_RESPONSE_TYPE, POST_VALUE_DEVICE_CODE},
        {POST_KEY_CLIENT_ID, m_configuration->getClientId()},
        {POST_KEY_SCOPE, scope},
        {POST_KEY_SCOPE_DATA, m_configuration->getScopeData()},
    };
    const std::vector<std::string> headerLines = {HEADER_LINE_URLENCODED,
                                                  HEADER_LINE_LANGUAGE_PREFIX + m_configuration->getLocale()};

    return m_httpPost->doPost(
        m_configuration->getRequestCodePairUrl(), headerLines, postData, m_configuration->getRequestTimeout());
}

avsCommon::utils::Optional<avsCommon::sdkInterfaces::AuthObserverInterface::Error> LWAAuthorizationAdapter::
    requestCodePair() {
    ACSDK_DEBUG5(LX("requestCodePair"));

    int retryCount = 0;
    while (!shouldStopRetrying()) {
        auto error = receiveCodePairResponse(sendCodePairRequest());
        Optional<AuthObserverInterface::Error> result(error);

        switch (error) {
            case AuthObserverInterface::Error::SUCCESS:
            case AuthObserverInterface::Error::UNAUTHORIZED_CLIENT:
            case AuthObserverInterface::Error::INVALID_REQUEST:
            case AuthObserverInterface::Error::INVALID_VALUE:
            case AuthObserverInterface::Error::UNSUPPORTED_GRANT_TYPE:
            case AuthObserverInterface::Error::INTERNAL_ERROR:
            case AuthObserverInterface::Error::INVALID_CBL_CLIENT_ID:
                return result;
            // Retry Errors
            case AuthObserverInterface::Error::UNKNOWN_ERROR:
            case AuthObserverInterface::Error::AUTHORIZATION_FAILED:
            case AuthObserverInterface::Error::SERVER_ERROR:
            case AuthObserverInterface::Error::AUTHORIZATION_EXPIRED:
            case AuthObserverInterface::Error::INVALID_CODE_PAIR:
            case AuthObserverInterface::Error::AUTHORIZATION_PENDING:
            case AuthObserverInterface::Error::SLOW_DOWN:
                break;
        }

        std::unique_lock<std::mutex> lock(m_mutex);
        m_wake.wait_until(lock, calculateTimeToRetry(retryCount++), [this] { return shouldStopRetryingLocked(); });
    }

    return Optional<AuthObserverInterface::Error>();
}

avsCommon::utils::libcurlUtils::HTTPResponse LWAAuthorizationAdapter::sendTokenRequest(
    std::chrono::steady_clock::time_point* requestTime) {
    ACSDK_DEBUG5(LX("sendTokenRequest"));

    const std::vector<std::pair<std::string, std::string>> postData = {{POST_KEY_GRANT_TYPE, POST_VALUE_DEVICE_CODE},
                                                                       {POST_KEY_DEVICE_CODE, m_deviceCode},
                                                                       {POST_KEY_USER_CODE, m_userCode}};
    const std::vector<std::string> headerLines = {HEADER_LINE_URLENCODED};

    if (requestTime) {
        *requestTime = std::chrono::steady_clock::now();
    } else {
        ACSDK_ERROR(LX("sendTokenRequestFailed").d("reason", "nullRequestTime"));
    }

    return m_httpPost->doPost(
        m_configuration->getRequestTokenUrl(), headerLines, postData, m_configuration->getRequestTimeout());
}

AuthObserverInterface::Error LWAAuthorizationAdapter::receiveTokenResponse(
    const avsCommon::utils::libcurlUtils::HTTPResponse& response,
    bool expiresImmediately,
    struct RefreshTokenResponse* tokenResponse) {
    ACSDK_DEBUG5(LX("receiveTokenResponse").d("code", response.code));

    if (!tokenResponse) {
        ACSDK_ERROR(LX("receiveTokenResponseFailed").d("reason", "nullTokenResponse"));
        return AuthObserverInterface::Error::UNKNOWN_ERROR;
    }

    Document document;
    auto result = parseLWAResponse(response, &document);

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
                        .d("token_type", tokenType)
                        .d("expires_in", expiresInSeconds));
        return AuthObserverInterface::Error::UNKNOWN_ERROR;
    }

    // This is used to make the initial access token expire immediately so that we also verify the refresh token
    // before reporting the @c REFRESHED state.
    if (expiresImmediately) {
        expiresInSeconds = 0;
    }

    tokenResponse->refreshToken = refreshToken;
    tokenResponse->accessToken = accessToken;
    tokenResponse->expiration = std::chrono::seconds(expiresInSeconds);

    return AuthObserverInterface::Error::SUCCESS;
}

Optional<AuthObserverInterface::Error> LWAAuthorizationAdapter::exchangeToken(RefreshTokenResponse* tokenResponse) {
    ACSDK_DEBUG5(LX("exchangeToken"));

    if (!tokenResponse) {
        ACSDK_ERROR(LX("exchangeTokenFailed").d("reason", "nullTokenResponse"));
        return Optional<AuthObserverInterface::Error>(AuthObserverInterface::Error::INTERNAL_ERROR);
    }

    auto interval = m_tokenRequestInterval;

    while (!shouldStopRetrying()) {
        // If the code pair expired, stop.  The application needs to restart authorization.
        if (std::chrono::steady_clock::now() >= m_codePairExpirationTime) {
            return Optional<AuthObserverInterface::Error>(AuthObserverInterface::Error::INVALID_CODE_PAIR);
        }

        m_cblRequester->onCheckingForAuthorization();
        std::chrono::steady_clock::time_point requestTime;
        HTTPResponse httpResponse = sendTokenRequest(&requestTime);
        tokenResponse->requestTime = requestTime;
        auto error = receiveTokenResponse(httpResponse, true, tokenResponse);
        Optional<AuthObserverInterface::Error> result(error);

        switch (error) {
            case AuthObserverInterface::Error::SUCCESS:
                // Since we receive the refreshToken from the cloud, we have no guarantee
                // it will work.
                tokenResponse->isRefreshTokenVerified = false;
            /* FALL-THROUGH */
            case AuthObserverInterface::Error::AUTHORIZATION_FAILED:
            case AuthObserverInterface::Error::UNAUTHORIZED_CLIENT:
            case AuthObserverInterface::Error::INVALID_REQUEST:
            case AuthObserverInterface::Error::INVALID_VALUE:
            case AuthObserverInterface::Error::AUTHORIZATION_EXPIRED:
            case AuthObserverInterface::Error::UNSUPPORTED_GRANT_TYPE:
            case AuthObserverInterface::Error::INVALID_CODE_PAIR:
            case AuthObserverInterface::Error::INTERNAL_ERROR:
            case AuthObserverInterface::Error::INVALID_CBL_CLIENT_ID:
                return result;
            /// Retriable errors.
            case AuthObserverInterface::Error::SLOW_DOWN:
                interval = std::min(interval * TOKEN_REQUEST_SLOW_DOWN_FACTOR, MAX_TOKEN_REQUEST_INTERVAL);
            /* FALL-THROUGH */
            case AuthObserverInterface::Error::UNKNOWN_ERROR:
            case AuthObserverInterface::Error::SERVER_ERROR:
            case AuthObserverInterface::Error::AUTHORIZATION_PENDING:
                break;
        }

        std::unique_lock<std::mutex> lock(m_mutex);
        m_wake.wait_for(lock, interval, [this] { return shouldStopRetryingLocked(); });
    }

    return Optional<AuthObserverInterface::Error>();
}

LWAAuthorizationAdapter::FlowState LWAAuthorizationAdapter::handleRequestingToken() {
    ACSDK_DEBUG5(LX("handleRequestingToken"));

    updateStateAndNotifyManager(AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS);

    TokenExchangeMethod authMethod;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        authMethod = m_authMethod;
    }

    switch (authMethod) {
        case TokenExchangeMethod::CBL: {
            auto result = requestCodePair();
            if (result.hasValue() && AuthObserverInterface::Error::SUCCESS == result.value()) {
                RefreshTokenResponse tokenResponse;
                result = exchangeToken(&tokenResponse);
                if (result.hasValue() && AuthObserverInterface::Error::SUCCESS == result.value()) {
                    getCustomerProfile(tokenResponse.accessToken);
                    setRefreshTokenResponse(tokenResponse);
                    return FlowState::REFRESHING_TOKEN;
                }
            }

            // Request was stopped.
            if (!result.hasValue()) {
                updateStateAndNotifyManager(
                    AuthObserverInterface::State::UNINITIALIZED, AuthObserverInterface::Error::SUCCESS);
            } else {
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    resetAuthMethodLocked();
                }

                updateStateAndNotifyManager(AuthObserverInterface::State::UNRECOVERABLE_ERROR, result.value());
            }

            return FlowState::IDLE;
        }
        case TokenExchangeMethod::NONE:
            ACSDK_ERROR(LX("handleRequestingTokenFailed").d("reason", "noAuthMethod"));
            return FlowState::IDLE;
    }

    ACSDK_ERROR(LX("handleRequestingTokenFailed").d("reason", "missingEnumHandler"));
    return FlowState::IDLE;
}

void LWAAuthorizationAdapter::resetAuthMethodLocked() {
    ACSDK_DEBUG5(LX("resetAuthMethodLocked"));
    m_authMethod = TokenExchangeMethod::NONE;
}

bool LWAAuthorizationAdapter::getCustomerProfile(const std::string& accessToken) {
    ACSDK_DEBUG5(LX("getCustomerProfile"));
    std::string url = m_configuration->getCustomerProfileUrl() + "?access_token=" + accessToken;
    HTTPResponse response;
    if (m_httpGet) {
        response = m_httpGet->doGet(url, {});
    } else {
        ACSDK_DEBUG0(LX("getCustomerProfileFailed").d("reason", "usingFallbackGetLogic"));
        response = doGet(url);
    }

    ACSDK_INFO(LX("getCustomerProfile").sensitive("code", response.code).sensitive("body", response.body));

    Document document;
    auto result = parseLWAResponse(response, &document);

    if (result != AuthObserverInterface::Error::SUCCESS) {
        ACSDK_ERROR(LX("getCustomerProfileFailed").d("result", result));
        return false;
    }

    auto it = document.FindMember(JSON_KEY_USER_ID);
    if (it != document.MemberEnd() && it->value.IsString()) {
        m_userId = it->value.GetString();
    }

    if (m_userId.empty()) {
        ACSDK_ERROR(LX("getCustomerProfileFailed").d("reason", "emptyUserId"));
    }

    if (m_requestCustomerProfile) {
        std::string name, email;

        it = document.FindMember(JSON_KEY_NAME);
        if (it != document.MemberEnd() && it->value.IsString()) {
            name = it->value.GetString();
        }

        it = document.FindMember(JSON_KEY_EMAIL);
        if (it != document.MemberEnd() && it->value.IsString()) {
            email = it->value.GetString();
        }

        if (name.empty() && email.empty()) {
            ACSDK_ERROR(LX("getCustomerProfileFailed").d("reason", "emptyNameAndEmail"));
        } else {
            // If there is some data available, notify observer.
            m_cblRequester->onCustomerProfileAvailable({name, email});
        }
    }

    return true;
}

avsCommon::utils::libcurlUtils::HTTPResponse LWAAuthorizationAdapter::requestRefresh(
    std::chrono::steady_clock::time_point* requestTime) {
    ACSDK_DEBUG5(LX("requestRefresh"));

    const std::vector<std::pair<std::string, std::string>> postData = {
        {POST_KEY_GRANT_TYPE, POST_VALUE_REFRESH_TOKEN},
        {POST_KEY_REFRESH_TOKEN, m_refreshTokenResponse.refreshToken},
        {POST_KEY_CLIENT_ID, m_configuration->getClientId()}};
    const std::vector<std::string> headerLines = {HEADER_LINE_URLENCODED};

    // Don't wait for this request so long that we would be late to notify our observer if the current token expires.
    auto now = std::chrono::steady_clock::now();
    if (requestTime) {
        *requestTime = now;
    } else {
        ACSDK_ERROR(LX("requestRefreshFailed").d("reason", "nullRequestTime"));
    }
    auto timeout = m_configuration->getRequestTimeout();

    if (AuthObserverInterface::State::REFRESHED == m_authState) {
        auto timeUntilExpired =
            std::chrono::duration_cast<std::chrono::seconds>(m_refreshTokenResponse.getExpirationTime() - now);
        if ((timeout > timeUntilExpired) && (timeUntilExpired > std::chrono::seconds::zero())) {
            timeout = timeUntilExpired;
        }
    }

    return m_httpPost->doPost(m_configuration->getRequestTokenUrl(), headerLines, postData, timeout);
}

LWAAuthorizationAdapter::FlowState LWAAuthorizationAdapter::handleRefreshingToken() {
    ACSDK_DEBUG5(LX("handleRefreshingToken"));

    int retryCount = 0;
    std::chrono::steady_clock::time_point nextRefresh =
        m_refreshTokenResponse.getExpirationTime() - m_configuration->getAccessTokenRefreshHeadStart();

    while (!shouldStopRetrying()) {
        std::unique_lock<std::mutex> lock(m_mutex);

        /*
         * This logic checks to see if the nextRefresh calculated above will be longer than the token expiration.
         * If so, then it effectively just takes the min of getExpirationTime() and nextRefresh. Will leave legacy
         * logic as is.
         */
        bool isAboutToExpire =
            (AuthObserverInterface::State::REFRESHED == m_authState &&
             m_refreshTokenResponse.getExpirationTime() < nextRefresh);

        auto nextActionTime = (isAboutToExpire ? m_refreshTokenResponse.getExpirationTime() : nextRefresh);

        m_wake.wait_until(lock, nextActionTime, [this] { return m_authFailureReported || shouldStopRetryingLocked(); });

        if (shouldStopRetryingLocked()) {
            break;
        }

        auto nextState = m_authState;

        if (m_authFailureReported) {
            m_authFailureReported = false;
            isAboutToExpire = false;
        }

        if (isAboutToExpire) {
            ACSDK_DEBUG0(LX("handleRefreshingTokenFailed").d("reason", "aboutToExpire"));
            m_refreshTokenResponse.accessToken.clear();
            lock.unlock();
            nextState = AuthObserverInterface::State::EXPIRED;
        } else {
            lock.unlock();

            std::chrono::steady_clock::time_point requestTime;
            auto httpResponse = requestRefresh(&requestTime);

            RefreshTokenResponse newRefreshTokenResponse;
            newRefreshTokenResponse.requestTime = requestTime;
            auto result = receiveTokenResponse(httpResponse, false, &newRefreshTokenResponse);
            auto error = result;

            switch (result) {
                case AuthObserverInterface::Error::SUCCESS:
                    retryCount = 0;
                    nextState = AuthObserverInterface::State::REFRESHED;
                    setRefreshTokenResponse(newRefreshTokenResponse);
                    nextRefresh =
                        m_refreshTokenResponse.getExpirationTime() - m_configuration->getAccessTokenRefreshHeadStart();
                    break;
                case AuthObserverInterface::Error::UNKNOWN_ERROR:
                case AuthObserverInterface::Error::SERVER_ERROR:
                case AuthObserverInterface::Error::AUTHORIZATION_PENDING:
                case AuthObserverInterface::Error::SLOW_DOWN:
                    nextRefresh = calculateTimeToRetry(retryCount++);
                    break;
                case AuthObserverInterface::Error::INVALID_REQUEST:
                    if (!m_refreshTokenResponse.isRefreshTokenVerified) {
                        error = AuthObserverInterface::Error::INVALID_CBL_CLIENT_ID;
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
                    resetAuthMethodLocked();
                    updateStateAndNotifyManager(AuthObserverInterface::State::UNRECOVERABLE_ERROR, error);
                    return FlowState::IDLE;
                }
            }
        }
        updateStateAndNotifyManager(nextState, AuthObserverInterface::Error::SUCCESS);
    }

    return FlowState::IDLE;
}

void LWAAuthorizationAdapter::stop() {
    ACSDK_DEBUG5(LX("stop"));
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_isShuttingDown = true;
    }

    m_wake.notify_one();
    if (m_authorizationFlowThread.joinable()) {
        m_authorizationFlowThread.join();
    }

    // Clear here just in case any requesters still want the token.
    RefreshTokenResponse reset;
    setRefreshTokenResponse(reset, false);
}

LWAAuthorizationAdapter::FlowState LWAAuthorizationAdapter::handleStopping() {
    ACSDK_DEBUG5(LX("handleStopping"));
    return FlowState::STOPPING;
}

void LWAAuthorizationAdapter::updateStateAndNotifyManager(
    avsCommon::sdkInterfaces::AuthObserverInterface::State state,
    avsCommon::sdkInterfaces::AuthObserverInterface::Error error) {
    ACSDK_DEBUG5(LX("updateStateAndNotifyManager").d("state", state).d("error", error));

    std::string userId;
    std::shared_ptr<AuthorizationManagerInterface> manager;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (state != m_authState) {
            m_authState = state;
            m_authError = error;
        } else {
            ACSDK_DEBUG5(LX("updateStateAndNotifyManagerFailed").d("reason", "sameState").d("state", state));
            return;
        }

        userId = m_userId;
        manager = m_manager;
    }

    if (manager) {
        manager->reportStateChange({state, error}, m_adapterId, userId);
    } else {
        ACSDK_WARN(LX("updateStateAndNotifyManagerFailed").d("reason", "nullManager"));
    }
}

void LWAAuthorizationAdapter::setRefreshTokenResponse(const RefreshTokenResponse& response, bool persist) {
    ACSDK_DEBUG5(LX("setRefreshTokenResponse"));
    std::lock_guard<std::mutex> lock(m_mutex);
    setRefreshTokenResponseLocked(response, persist);
}

void LWAAuthorizationAdapter::setRefreshTokenResponseLocked(const RefreshTokenResponse& response, bool persist) {
    ACSDK_DEBUG5(LX("setRefreshTokenResponseLocked"));

    m_refreshTokenResponse = response;

    if (persist) {
        if (!m_storage->setRefreshToken(m_refreshTokenResponse.refreshToken)) {
            ACSDK_ERROR(LX("failedToPersistNewRefreshToken"));
        }

        if (!m_storage->setUserId(m_userId)) {
            ACSDK_ERROR(LX("failedToPersistNewRefreshToken"));
        }
    }
}

LWAAuthorizationAdapter::FlowState LWAAuthorizationAdapter::handleClearingData() {
    ACSDK_DEBUG5(LX("handleClearingData"));

    updateStateAndNotifyManager(
        avsCommon::sdkInterfaces::AuthObserverInterface::State::UNINITIALIZED,
        avsCommon::sdkInterfaces::AuthObserverInterface::Error::SUCCESS);

    m_isClearingData = false;

    return FlowState::IDLE;
}

bool LWAAuthorizationAdapter::isShuttingDown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    ACSDK_DEBUG5(LX("isShuttingDown").d("shuttingDown", m_isShuttingDown));
    return m_isShuttingDown;
}

void LWAAuthorizationAdapter::handleAuthorizationFlow(FlowState state) {
    ACSDK_DEBUG5(LX("handleAuthorizationFlow"));

    while (!isShuttingDown()) {
        auto nextFlowState = FlowState::STOPPING;
        switch (state) {
            case FlowState::IDLE:
                nextFlowState = handleIdle();
                break;
            case FlowState::REQUESTING_TOKEN:
                nextFlowState = handleRequestingToken();
                break;
            case FlowState::REFRESHING_TOKEN:
                nextFlowState = handleRefreshingToken();
                break;
            case FlowState::CLEARING_DATA:
                nextFlowState = handleClearingData();
                break;
            case FlowState::STOPPING:
                nextFlowState = handleStopping();
                break;
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_isClearingData) {
            nextFlowState = FlowState::CLEARING_DATA;
        }

        state = nextFlowState;
    }
}

bool LWAAuthorizationAdapter::init(
    const avsCommon::utils::configuration::ConfigurationNode& configuration,
    const std::shared_ptr<avsCommon::utils::DeviceInfo>& deviceInfo) {
    ACSDK_DEBUG5(LX("init"));

    m_configuration = LWAAuthorizationConfiguration::create(configuration, deviceInfo);
    if (!m_configuration) {
        ACSDK_ERROR(LX("initFailed").d("reason", "createAuthorizationConfigurationFailed"));
        return false;
    }

    if (!m_storage->openOrCreate()) {
        ACSDK_ERROR(LX("initFailed").d("reason", "accessDatabaseFailed"));
        return false;
    }

    m_authorizationFlowThread =
        std::thread(&LWAAuthorizationAdapter::handleAuthorizationFlow, this, retrievePersistedData());

    return true;
}

bool LWAAuthorizationAdapter::authorizeUsingCBLHelper(
    const std::shared_ptr<CBLAuthorizationObserverInterface>& observer,
    bool requestCustomerProfile) {
    if (!observer) {
        ACSDK_ERROR(LX("authorizeUsingCBLHelperFailed").d("reason", "nullObserver"));
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_manager) {
        ACSDK_ERROR(LX("authorizeUsingCBLHelperFailed").d("reason", "nullManager"));
        return false;
    }

    if (TokenExchangeMethod::NONE != m_authMethod) {
        ACSDK_INFO(LX("authorizeUsingCBLHelperFailed").d("reason", "authorizationInProgress"));
        return false;
    }

    if (AuthObserverInterface::State::UNINITIALIZED == m_authState ||
        AuthObserverInterface::State::UNRECOVERABLE_ERROR == m_authState) {
        m_cblRequester = observer;
        m_authMethod = TokenExchangeMethod::CBL;
        m_requestCustomerProfile = requestCustomerProfile;
        m_wake.notify_one();
    } else {
        ACSDK_INFO(LX("authorizeUsingCBLHelperFailed").d("reason", "invalidState").d("m_authState", m_authState));
        return false;
    }

    return true;
}

bool LWAAuthorizationAdapter::authorizeUsingCBL(const std::shared_ptr<CBLAuthorizationObserverInterface>& observer) {
    return authorizeUsingCBLHelper(observer, false);
}

bool LWAAuthorizationAdapter::authorizeUsingCBLWithCustomerProfile(
    const std::shared_ptr<CBLAuthorizationObserverInterface>& observer) {
    return authorizeUsingCBLHelper(observer, true);
}

std::string LWAAuthorizationAdapter::getId() {
    // m_adapterId is const, no need to lock.
    ACSDK_DEBUG5(LX("getId").d("id", m_adapterId));
    return m_adapterId;
}

std::string LWAAuthorizationAdapter::getAuthToken() {
    std::lock_guard<std::mutex> lock(m_mutex);
    ACSDK_DEBUG5(LX("getAuthToken"));

    return m_refreshTokenResponse.accessToken;
}

void LWAAuthorizationAdapter::reset() {
    ACSDK_DEBUG5(LX("reset"));

    std::lock_guard<std::mutex> lock(m_mutex);
    m_storage->clear();
    m_userId.clear();
    m_refreshTokenResponse = RefreshTokenResponse();
    m_codePairExpirationTime = std::chrono::steady_clock::time_point();
    m_deviceCode.clear();
    m_userCode.clear();
    m_tokenRequestInterval = std::chrono::seconds::zero();
    m_authFailureReported = false;
    resetAuthMethodLocked();
    m_isClearingData = true;
    m_wake.notify_one();
}

void LWAAuthorizationAdapter::onAuthFailure(const std::string& authToken) {
    ACSDK_DEBUG0(LX("onAuthFailure"));

    std::lock_guard<std::mutex> lock(m_mutex);
    if (authToken.empty() || authToken == m_refreshTokenResponse.accessToken) {
        ACSDK_DEBUG9(LX("onAuthFailure").m("setting m_authFailureReported"));
        m_authFailureReported = true;
        m_wake.notify_one();
    }
}

AuthObserverInterface::FullState LWAAuthorizationAdapter::getState() {
    ACSDK_DEBUG5(LX("getState"));

    std::lock_guard<std::mutex> lock(m_mutex);
    return {m_authState, m_authError};
}

std::shared_ptr<AuthorizationInterface> LWAAuthorizationAdapter::getAuthorizationInterface() {
    ACSDK_DEBUG5(LX("getAuthorizationInterface"));
    return shared_from_this();
}

AuthObserverInterface::FullState LWAAuthorizationAdapter::onAuthorizationManagerReady(
    const std::shared_ptr<AuthorizationManagerInterface>& manager) {
    ACSDK_DEBUG5(LX("onAuthorizationManagerReady"));

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!manager) {
        ACSDK_ERROR(LX("onAuthorizationManagerReadyFailed").d("reason", "nullManager"));
    } else {
        m_manager = manager;
    }

    return {m_authState, m_authError};
}

}  // namespace lwa
}  // namespace acsdkAuthorization
}  // namespace alexaClientSDK
