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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_AUTHORIZATION_CBLAUTHDELEGATE_INCLUDE_CBLAUTHDELEGATE_CBLAUTHDELEGATECONFIGURATION_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_AUTHORIZATION_CBLAUTHDELEGATE_INCLUDE_CBLAUTHDELEGATE_CBLAUTHDELEGATECONFIGURATION_H_

#include <chrono>
#include <memory>
#include <string>

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>

namespace alexaClientSDK {
namespace authorization {
namespace cblAuthDelegate {

/**
 * Class encapsulating configuration parameters for a CBLAuthDelegate.
 */
class CBLAuthDelegateConfiguration {
public:
    /**
     * Create a CBLAuthDelegateConfiguration instance
     *
     * @param configuration The ConfigurationNode containing the configuration parameters for the new instance.
     * @param deviceInfo The deviceInfo instance.
     * @return the CBLAuthDelegateConfiguration instance if successful, else a nullptr.
     */
    static std::unique_ptr<CBLAuthDelegateConfiguration> create(
        const avsCommon::utils::configuration::ConfigurationNode& configuration,
        const std::shared_ptr<avsCommon::utils::DeviceInfo>& deviceInfo);

    /**
     * Get the client ID.
     *
     * @return The client ID.
     */
    std::string getClientId() const;

    /**
     * Get the product ID.
     *
     * @return The product ID.
     */
    std::string getProductId() const;

    /**
     * Get the device serial number.
     *
     * @return The device serial number.
     */
    std::string getDeviceSerialNumber() const;

    /**
     * Get the request timeout.
     *
     * @return The request timeout.
     */
    std::chrono::seconds getRequestTimeout() const;

    /**
     * Get the duration before access token expiration to start refreshing the token.
     *
     * @return The duration before access token expiration to start refreshing the token.
     */
    std::chrono::seconds getAccessTokenRefreshHeadStart() const;

    /**
     * Get the locale to use when prompting the user to authenticate the client to access AVS.
     *
     * @return The locale to use when prompting the user to authenticate the client to access AVS.
     */
    std::string getLocale() const;

    /**
     * Get the URL to use when requesting a code pair from @c LWA.
     *
     * @return The URL to use when requesting a code pair from @c LWA.
     */
    std::string getRequestCodePairUrl() const;

    /**
     * Get the URL to use when requesting an initial access token from @c LWA.
     *
     * @return The URL to use when requesting an initial access token from @c LWA.
     */
    std::string getRequestTokenUrl() const;

    /**
     * Get the URL to use when refreshing an access token.
     *
     * @return The URL to use when refreshing an access token.
     */
    std::string getRefreshTokenUrl() const;

    /**
     * Get (as text) the 'scope_data' JSON object to send to @c LWA when requesting a code pair.
     *
     * @return The 'scope_data' JSON object to send to @c LWA when requesting a code pair.
     */
    std::string getScopeData() const;

private:
    /**
     * Initialize the new instance.
     *
     * @param root The root level node of the raw configuration from which to populate this instance.
     * @param deviceInfo The deviceInfo instance.
     * @return Whether or not the operation was successful.
     */
    bool init(
        const avsCommon::utils::configuration::ConfigurationNode& configuration,
        const std::shared_ptr<avsCommon::utils::DeviceInfo>& deviceInfo);

    /**
     * Initialize m_scopeData.
     *
     * @return Whether or not the initialization succeeded.
     */
    bool initScopeData();

    /// Device info
    std::shared_ptr<avsCommon::utils::DeviceInfo> m_deviceInfo;

    /// How long to wait for a response from @c LWA.
    std::chrono::seconds m_requestTimeout;

    /// How far ahead of auth token expiration to start making requests to refresh the auth token.
    std::chrono::seconds m_accessTokenRefreshHeadStart;

    /// Locale to pass in code pair requests to @c LWA.
    std::string m_locale;

    /// Base URL for requesting a code pair.
    std::string m_requestCodePairUrl;

    /// Base URL for requesting an auth token.
    std::string m_requestTokenUrl;

    /// Base URL for refreshing an auth token.
    std::string m_refreshTokenUrl;

    /// JSON string for scope_data value passed to @c LWA in code pair requests.
    std::string m_scopeData;
};

}  // namespace cblAuthDelegate
}  // namespace authorization
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_AUTHORIZATION_CBLAUTHDELEGATE_INCLUDE_CBLAUTHDELEGATE_CBLAUTHDELEGATECONFIGURATION_H_
