/*
 * Config.h
 *
 * Copyright 2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AUTHDELEGATE_INCLUDE_CONFIG_H_
#define ALEXA_CLIENT_SDK_AUTHDELEGATE_INCLUDE_CONFIG_H_

#include <chrono>
#include <memory>
#include <string>
#include "AuthDelegate/HttpPostInterface.h"

namespace alexaClientSDK {
namespace authDelegate {

/// Access to configuration parameters for AuthDelegate
class Config {
public:
    Config();

    virtual ~Config() {};

    /**
     * Get the base URL for contacting LWA
     *
     * @return The LWA endpoint to get tokens from.
     */
    virtual std::string getLwaUrl() const;

    /**
     * Get the client ID to pass to LWA.
     *
     * @return The LWA client ID of this client.
     */
    virtual std::string getClientId() const;

    /**
     * Get the refresh token to pass to LWA.
     *
     * @return The LWA token used to refresh the clients auth token.
     */
    virtual std::string getRefreshToken() const;

    /**
     * Get the client secret to pass to LWA.
     *
     * @return The client secret to pass to LWA.
     */
    virtual std::string getClientSecret() const;

    /**
     * Get how long before an LWA auth token expires to try to refresh it.
     *
     * @param Return head start interval for refreshing the auth token.
     */
    virtual std::chrono::seconds getAuthTokenRefreshHeadStart() const;

    /**
     * Get the max number of seconds to wait for an LWA request to complete.
     *
     * @return The maximum number of seconds to wait on an LWA request.
     */
    virtual std::chrono::seconds getRequestTimeout() const;
protected:
    std::string m_lwaUrl;
    std::string m_clientId;
    std::string m_refreshToken;
    std::string m_clientSecret;
    std::chrono::seconds m_authTokenRefreshHeadStart;
    std::chrono::seconds m_maxAuthTokenTtl;
    std::chrono::seconds m_requestTimeout;
};

} // namespace authDelegate
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_AUTHDELEGATE_INCLUDE_CONFIG_H_
