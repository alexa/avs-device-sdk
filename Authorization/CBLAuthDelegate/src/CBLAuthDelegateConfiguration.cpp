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

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "CBLAuthDelegate/CBLAuthDelegateConfiguration.h"

namespace alexaClientSDK {
namespace authorization {
namespace cblAuthDelegate {

using namespace rapidjson;

/// String to identify log entries originating from this file.
static const std::string TAG("CBLAuthDelegateConfiguration");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Name of @c ConfigurationNode for CBLAuthDelegate
static const std::string CONFIG_KEY_CBL_AUTH_DELEGATE = "cblAuthDelegate";

/// Name of lwaURL value in CBLAuthDelegate's @c ConfigurationNode.
static const std::string CONFIG_KEY_LWA_URL = "lwaUrl";

/// Name of requestTimeout value in CBLAuthDelegate's @c ConfigurationNode.
static const std::string CONFIG_KEY_REQUEST_TIMEOUT = "requestTimeout";

/// Name of accessTokenRefreshHeadStart value in CBLAuthDelegate's @c ConfigurationNode.
static const std::string CONFIG_KEY_ACCESS_TOKEN_REFRESH_HEAD_START = "accessTokenRefreshHeadStart";

/// Name of @c ConfigurationNode for system
static const std::string CONFIG_KEY_SETTINGS = "settings";

/// Name of @c ConfigurationNode for default values under settings.
static const std::string SETTINGS_DEFAULT_SETTINGS_ROOT_KEY = "defaultAVSClientSettings";

/// Name of @c locale value in in settings's @c ConfigurationNode.
static const std::string CONFIG_KEY_LOCALE = "locale";

/// Default value for settings.locale.
static const std::string CONFIG_VALUE_DEFAULT_LOCALE = "en-US";

/// Key for alexa:all values in JSON sent to @c LWA
static const char JSON_KEY_ALEXA_ALL[] = "alexa:all";

/// Key for productID values in JSON sent to @c LWA
static const char JSON_KEY_PRODUCT_ID[] = "productID";

/// Key for productInstanceAttributes values in JSON sent to @c LWA
static const char JSON_KEY_PRODUCT_INSTANCE_ATTRIBUTES[] = "productInstanceAttributes";

/// Key for deviceSerialNumber values in JSON sent to @c LWA
static const char JSON_KEY_DEVICE_SERIAL_NUMBER[] = "deviceSerialNumber";

/// Default value for configured requestTimeout.
static const std::chrono::minutes DEFAULT_REQUEST_TIMEOUT = std::chrono::minutes(1);

/// Default value for configured accessTokenRefreshHeadStart.
static const std::chrono::minutes DEFAULT_ACCESS_TOKEN_REFRESH_HEAD_START = std::chrono::minutes(10);

/// Default for configured base URL for @c LWA requests.
static const std::string DEFAULT_LWA_BASE_URL = "https://api.amazon.com/auth/O2/";

/// Path suffix for URL used in code pair requests to @C LWA.
static const std::string REQUEST_CODE_PAIR_PATH = "create/codepair";

/// Path suffix for URL used in code pair token requests to @c LWA.
static const std::string REQUEST_TOKEN_PATH = "token";

/// Path suffix for URl used in token refresh requests to @c LWA.
static const std::string REFRESH_TOKEN_PATH = "token";

std::unique_ptr<CBLAuthDelegateConfiguration> CBLAuthDelegateConfiguration::create(
    const avsCommon::utils::configuration::ConfigurationNode& configuration,
    const std::shared_ptr<avsCommon::utils::DeviceInfo>& deviceInfo) {
    std::unique_ptr<CBLAuthDelegateConfiguration> instance(new CBLAuthDelegateConfiguration());
    ACSDK_DEBUG5(LX("create"));

    if (instance->init(configuration, deviceInfo)) {
        return instance;
    }

    ACSDK_ERROR(LX("createFailed").d("reason", "initFailed"));
    return nullptr;
}

bool CBLAuthDelegateConfiguration::init(
    const avsCommon::utils::configuration::ConfigurationNode& configurationRoot,
    const std::shared_ptr<avsCommon::utils::DeviceInfo>& deviceInfo) {
    ACSDK_DEBUG5(LX("init"));

    auto configuration = configurationRoot[CONFIG_KEY_CBL_AUTH_DELEGATE];

    if (!configuration) {
        ACSDK_ERROR(LX("initFailed").d("reason", "emptyConfiguration").d("key", CONFIG_KEY_CBL_AUTH_DELEGATE));
        return false;
    }

    m_deviceInfo = deviceInfo;

    configuration.getDuration<std::chrono::seconds>(
        CONFIG_KEY_REQUEST_TIMEOUT, &m_requestTimeout, DEFAULT_REQUEST_TIMEOUT);

    configuration.getDuration<std::chrono::seconds>(
        CONFIG_KEY_ACCESS_TOKEN_REFRESH_HEAD_START,
        &m_accessTokenRefreshHeadStart,
        DEFAULT_ACCESS_TOKEN_REFRESH_HEAD_START);

    configurationRoot[CONFIG_KEY_SETTINGS][SETTINGS_DEFAULT_SETTINGS_ROOT_KEY].getString(
        CONFIG_KEY_LOCALE, &m_locale, CONFIG_VALUE_DEFAULT_LOCALE);

    if (!initScopeData()) {
        ACSDK_ERROR(LX("initFailed").d("reason", "initScopeDataFailed"));
        return false;
    }

    std::string lwaBaseUrl;
    configuration.getString(CONFIG_KEY_LWA_URL, &lwaBaseUrl, DEFAULT_LWA_BASE_URL);
    m_requestCodePairUrl = lwaBaseUrl + REQUEST_CODE_PAIR_PATH;
    m_requestTokenUrl = lwaBaseUrl + REQUEST_TOKEN_PATH;
    m_refreshTokenUrl = lwaBaseUrl + REFRESH_TOKEN_PATH;

    return true;
}

std::string CBLAuthDelegateConfiguration::getClientId() const {
    return m_deviceInfo->getClientId();
}

std::string CBLAuthDelegateConfiguration::getProductId() const {
    return m_deviceInfo->getProductId();
}

std::string CBLAuthDelegateConfiguration::getDeviceSerialNumber() const {
    return m_deviceInfo->getDeviceSerialNumber();
}

std::chrono::seconds CBLAuthDelegateConfiguration::getRequestTimeout() const {
    return m_requestTimeout;
}

std::chrono::seconds CBLAuthDelegateConfiguration::getAccessTokenRefreshHeadStart() const {
    return m_accessTokenRefreshHeadStart;
}

std::string CBLAuthDelegateConfiguration::getLocale() const {
    return m_locale;
}

std::string CBLAuthDelegateConfiguration::getRequestCodePairUrl() const {
    return m_requestCodePairUrl;
}

std::string CBLAuthDelegateConfiguration::getRequestTokenUrl() const {
    return m_requestTokenUrl;
}

std::string CBLAuthDelegateConfiguration::getRefreshTokenUrl() const {
    return m_refreshTokenUrl;
}

std::string CBLAuthDelegateConfiguration::getScopeData() const {
    return m_scopeData;
}

bool CBLAuthDelegateConfiguration::initScopeData() {
    ACSDK_DEBUG5(LX("initScopeData"));

    Document scopeData;
    scopeData.SetObject();
    auto& allocator = scopeData.GetAllocator();
    Value productInstanceAttributes(kObjectType);
    productInstanceAttributes.AddMember(JSON_KEY_DEVICE_SERIAL_NUMBER, getDeviceSerialNumber(), allocator);
    Value alexaColonAll(kObjectType);
    alexaColonAll.AddMember(JSON_KEY_PRODUCT_ID, getProductId(), allocator);
    alexaColonAll.AddMember(JSON_KEY_PRODUCT_INSTANCE_ATTRIBUTES, productInstanceAttributes, allocator);
    scopeData.AddMember(JSON_KEY_ALEXA_ALL, alexaColonAll, allocator);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    if (!scopeData.Accept(writer)) {
        ACSDK_ERROR(LX("initScopeDataFailed").d("reason", "acceptFailed"));
        return false;
    }

    m_scopeData = buffer.GetString();
    ACSDK_DEBUG9(LX("initScopeDataSucceeded").sensitive("scopeData", m_scopeData));

    return true;
}

}  // namespace cblAuthDelegate
}  // namespace authorization
}  // namespace alexaClientSDK
