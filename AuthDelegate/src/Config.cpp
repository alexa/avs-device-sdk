/*
 * Config.cpp
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

#include "AuthDelegate/Config.h"
#include "AuthDelegate/HttpPost.h"

namespace alexaClientSDK {
namespace authDelegate {

static const std::string DEFAULT_LWA_URL = "https://api.amazon.com/auth/o2/token";
static const std::string DEFAULT_CLIENT_ID = "";
static const std::string DEFAULT_REFRESH_TOKEN = "";
static const std::string DEFAULT_CLIENT_SECRET = "";
static const std::chrono::seconds DEFAULT_AUTH_TOKEN_REFRESH_HEAD_START = std::chrono::seconds(10 * 60);
static const std::chrono::seconds DEFAULT_MAX_AUTH_TOKEN_TTL = std::chrono::seconds::max();
static const std::chrono::seconds DEFAULT_REQUEST_TIMEOUT = std::chrono::seconds(5 * 60);

Config::Config() :
        m_lwaUrl(DEFAULT_LWA_URL),
        m_clientId(DEFAULT_CLIENT_ID),
        m_refreshToken(DEFAULT_REFRESH_TOKEN),
        m_clientSecret(DEFAULT_CLIENT_SECRET),
        m_authTokenRefreshHeadStart(DEFAULT_AUTH_TOKEN_REFRESH_HEAD_START),
        m_maxAuthTokenTtl(DEFAULT_MAX_AUTH_TOKEN_TTL),
        m_requestTimeout(DEFAULT_REQUEST_TIMEOUT) {
};

std::string Config::getLwaUrl() const {
    return m_lwaUrl;
}

std::string Config::getClientId() const {
    return m_clientId;
}

std::string Config::getRefreshToken() const {
    return m_refreshToken;
}

std::string Config::getClientSecret() const {
    return m_clientSecret;
}

std::chrono::seconds Config::getAuthTokenRefreshHeadStart() const {
    return m_authTokenRefreshHeadStart;
}

std::chrono::seconds Config::getRequestTimeout() const {
    return m_requestTimeout;
}

} // namespace authDelegate
} // namespace alexaClientSDK
