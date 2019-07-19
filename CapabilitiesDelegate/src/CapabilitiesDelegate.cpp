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

#include "CapabilitiesDelegate/CapabilitiesDelegate.h"

#include <iomanip>
#include <unordered_map>
#include <unordered_set>

#include <curl/curl.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <ACL/Transport/TransportDefines.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/HTTP/HttpResponseCode.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPutInterface.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPResponse.h>
#include <AVSCommon/Utils/LibcurlUtils/LibcurlUtils.h>

namespace alexaClientSDK {
namespace capabilitiesDelegate {

using namespace avsCommon::avs;
using namespace avsCommon::utils;
using namespace avsCommon::utils::http;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::libcurlUtils;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::storage;
using namespace rapidjson;

/// String to identify log entries originating from this file.
static const std::string TAG("CapabilitiesDelegate");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Name of @c ConfigurationNode for CapabilitiesDelegate
static const std::string CONFIG_KEY_CAPABILITIES_DELEGATE = "capabilitiesDelegate";
/// Name of overridenCapabilitiesPublishMessageBody value in CapabilitiesDelegate's @c ConfigurationNode.
static const std::string CONFIG_KEY_OVERRIDE_CAPABILITIES_API_REQUEST_BODY = "overridenCapabilitiesPublishMessageBody";
/// Name of endpoint value in CapabilitiesDelegate's @c ConfigurationNode.
static const std::string CONFIG_KEY_CAPABILITIES_API_ENDPOINT = "endpoint";

/// Constituents of the Capabilities API URL
/// Capabilities API endpoint
static const std::string DEFAULT_CAPABILITIES_API_ENDPOINT = "https://api.amazonalexa.com";
/// Suffix before the device's place in the URL
static const std::string CAPABILITIES_API_URL_PRE_DEVICE_SUFFIX = "/v1/devices/";
/// Suffix after the device's place in the URL
static const std::string CAPABILITIES_API_URL_POST_DEVICE_SUFFIX = "/capabilities";
/// Device ID for the device that will show up in the URL
static const std::string SELF_DEVICE = "@self";

/// Constants for the Capabilities API message json
/// Content type header key
static const std::string CONTENT_TYPE_HEADER_KEY = "Content-Type";
/// Content type header value
static const std::string CONTENT_TYPE_HEADER_VALUE = "application/json";
/// Content length header key
static const std::string CONTENT_LENGTH_HEADER_KEY = "Content-Length";
/// Auth token header key
static const std::string AUTHORIZATION_HEADER_KEY = "x-amz-access-token";
/// Accept header key
static const std::string ACCEPT_HEADER_KEY = "Accept";
/// Expect header key
static const std::string EXPECT_HEADER_KEY = "Expect";
/// Envelope version key in message body
static const std::string ENVELOPE_VERSION_KEY = "envelopeVersion";
/// Envelope version value in message body
static const std::string ENVELOPE_VERSION_VALUE = "20160207";
/// Capabilities key in message body
static const std::string CAPABILITIES_KEY = "capabilities";

/// Constant for HTTP 400 response json
static const std::string HTTP_RESPONSE_ERROR_VALUE = "message";

/// Separator between the components that make up a capability key
static const std::string CAPABILITY_KEY_SEPARATOR = ".";
/// Separator between header key and value
static const std::string HEADER_KEY_VALUE_SEPARATOR = ":";

/// Constants for prefixes for DB storage
/// Endpoint key
static const std::string DB_KEY_ENDPOINT = "endpoint:";
/// Client id key
static const std::string DB_KEY_CLIENT_ID = "clientId:";
/// Product id key
static const std::string DB_KEY_PRODUCT_ID = "productId:";
/// Envelope version key
static const std::string DB_KEY_ENVELOPE_VERSION = "envelopeVersion:";
/// DSN key
static const std::string DB_KEY_DSN = "deviceSerialNumber:";
/// Message key
static const std::string DB_KEY_PUBLISH_MSG = "publishMsg:";
/// Separator between keys
static const std::string DB_KEY_SEPARATOR = ",";

/// Constants for entries in the capability map
/// Required number of entries in capability map
static const unsigned int CAPABILITY_MAP_REQUIRED_ENTRIES = 3;
/// Maximum number of entries in capability map
static const unsigned int CAPABILITY_MAP_MAXIMUM_ENTRIES = 4;

/// Component name needed for Misc DB
static const std::string COMPONENT_NAME = "capabilitiesDelegate";
/// Capabilities publish message table
static const std::string CAPABILITIES_PUBLISH_TABLE = "capabilitiesPublishMessage";

/**
 * Returns a key for a capability map, which is used as a key to store in the m_capabilityConfigs map.
 * The key consists of the interface type and interface name.
 *
 * @param capabilityMap The capability configuration map of a specific capability.
 * @return a string that is basically <interface type>.<interface name>
 */
static std::string getCapabilityKey(const std::unordered_map<std::string, std::string>& capabilityMap);

/**
 * Helper method for isCapabilityMapCorrectlyFormed() to check if a key is present,
 * and its corresponding value is not empty.
 *
 * @param capabilityMap The capability configuration map of a specific capability.
 * @param findKey The key from capabilityMap that needs to be checked.
 * @param errorEvent Log error event if checks fail.
 * @param errorReasonKey Log error reason key if checks fail.
 * @param errorReasonValue Log error reason value if checks fail.
 * @return True if the checks pass, else false.
 */
static bool capabilityMapFormCheckHelper(
    const std::unordered_map<std::string, std::string>& capabilityMap,
    const std::string& findKey,
    const std::string& errorEvent,
    const std::string& errorReasonKey,
    const std::string& errorReasonValue);

/**
 * Checks a capability map to see
 * - It has all of the required information (type, name, version)
 * - If a configuration string is available, then it is a valid json
 * - There is no other unexpected information
 * Note that, the method will not check to see if the values themselves are valid.
 *
 * @param capabilityMap The capability configuration map of a specific capability.
 * @return True if all the checked conditions pass, else false.
 */
static bool isCapabilityMapCorrectlyFormed(const std::unordered_map<std::string, std::string>& capabilityMap);

/**
 * Get the error message from HTTP response.
 *
 * @return The error message from the HTTP response if the json is valid, else "".
 */
static std::string getErrorMsgFromHttpResponse(const std::string& httpResponse);

/**
 * Get a capability info string from the capability json.
 *
 * @param capabilityJson The capability json.
 * @param jsonMemberKey the member key in the json where the capability info can be found.
 * @return The capability info string if found, else "".
 * @note "" is ok as a return value for this function if there are errors because a null string itself implies an error.
 */
static std::string getCapabilityInfoStringFromJson(
    const rapidjson::Value& capabilityJson,
    const std::string& jsonMemberKey);

/**
 * Get a capability configurations json as a string from the capability json.
 *
 * @param capabilityJson The capability json.
 * @return The capability configurations json as a string if found, else "".
 * @note "" is ok as a return value for this function if there are errors because a null string itself implies an error.
 */
static std::string getCapabilityConfigsStringFromJson(const rapidjson::Value& capabilityJson);

/**
 * Get a string from the json.
 *
 * @param jsonValue The json value object.
 * @param [out] *jsonString The json as a string.
 * @return true if a string was able to be obatined from the json, else false.
 */
static bool getStringFromJson(const rapidjson::Value& jsonValue, std::string* jsonString);

/**
 * Get a json object from a string representing the json.
 *
 * @param jsonString The json as a string.
 * @param allocator The allocator object used to build @c jsonValue. Note: Make sure that the allocator gets destroyed
 *                  only after @c jsonValue has been destroyed.
 * @param [out] *jsonValue The json value object.
 * @return true if the string represents a proper json, else false.
 */
static bool getJsonFromString(
    const std::string& jsonString,
    rapidjson::Value::AllocatorType& allocator,
    rapidjson::Value* jsonValue);

/**
 * Helper method that will log a fail for saving Capabilities API data to misc DB and then attempt to clear the table.
 *
 * @param miscStorage The misc storage handler.
 */
static void logFailedSaveAndClearCapabilitiesPublishTable(const std::shared_ptr<MiscStorageInterface>& miscStorage);

std::string getCapabilityKey(const std::unordered_map<std::string, std::string>& capabilityMap) {
    auto interfaceTypeIterator = capabilityMap.find(CAPABILITY_INTERFACE_TYPE_KEY);
    if (interfaceTypeIterator == capabilityMap.end()) {
        return "";
    }
    auto interfaceType = interfaceTypeIterator->second;

    auto interfaceNameIterator = capabilityMap.find(CAPABILITY_INTERFACE_NAME_KEY);
    if (interfaceNameIterator == capabilityMap.end()) {
        return "";
    }
    auto interfaceName = interfaceNameIterator->second;

    return interfaceType + CAPABILITY_KEY_SEPARATOR + interfaceName;
}

bool capabilityMapFormCheckHelper(
    const std::unordered_map<std::string, std::string>& capabilityMap,
    const std::string& findKey,
    const std::string& errorEvent,
    const std::string& errorReasonKey,
    const std::string& errorReasonValue) {
    auto foundValueIterator = capabilityMap.find(findKey);

    if (foundValueIterator == capabilityMap.end()) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, errorReasonValue));
        return false;
    } else {
        std::string foundValue = foundValueIterator->second;
        if (foundValue.empty()) {
            ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, errorReasonValue));
            return false;
        }
    }

    return true;
}

bool isCapabilityMapCorrectlyFormed(const std::unordered_map<std::string, std::string>& capabilityMap) {
    const std::string errorEvent = "capabilityMapIncorrectlyFormed";
    const std::string errorReasonKey = "reason";

    if (!capabilityMapFormCheckHelper(
            capabilityMap, CAPABILITY_INTERFACE_TYPE_KEY, errorEvent, errorReasonKey, "interfaceTypeNotAvailable")) {
        return false;
    }

    if (!capabilityMapFormCheckHelper(
            capabilityMap, CAPABILITY_INTERFACE_NAME_KEY, errorEvent, errorReasonKey, "interfaceNameNotAvailable")) {
        return false;
    }

    if (!capabilityMapFormCheckHelper(
            capabilityMap,
            CAPABILITY_INTERFACE_VERSION_KEY,
            errorEvent,
            errorReasonKey,
            "interfaceVersionNotAvailable")) {
        return false;
    }

    auto mapSize = capabilityMap.size();
    std::string unexpectedValueErrorReasonValue = "unexpectedValuesFound";

    /// interface type, name and version are the only entries that are required,
    /// and we have checked above that all of them exist.
    if (CAPABILITY_MAP_REQUIRED_ENTRIES == mapSize) {
        return true;
    }

    if (mapSize > CAPABILITY_MAP_MAXIMUM_ENTRIES) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, unexpectedValueErrorReasonValue));
        return false;
    }

    /// the fourth entry (for CAPABILITY_INTERFACE_CONFIGURATIONS_KEY) is optional
    std::string configurationValues;
    auto foundValueIterator = capabilityMap.find(CAPABILITY_INTERFACE_CONFIGURATIONS_KEY);
    if (foundValueIterator == capabilityMap.end()) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, unexpectedValueErrorReasonValue));
        return false;
    }
    configurationValues = foundValueIterator->second;
    if (configurationValues.empty()) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "configurationValuesNotAvailable"));
        return false;
    }

    /// check if the configuration values string is a valid json
    rapidjson::Document configJson;
    if (configJson.Parse(configurationValues).HasParseError()) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "configurationValuesNotValidJson"));
        return false;
    }

    return true;
}

std::string getErrorMsgFromHttpResponse(const std::string& httpResponse) {
    if (httpResponse.empty()) {
        return "";
    }

    std::string errorMsg;
    rapidjson::Document errorMsgJson;
    errorMsgJson.Parse(httpResponse);

    if ((errorMsgJson.HasParseError()) || (!errorMsgJson.HasMember(HTTP_RESPONSE_ERROR_VALUE))) {
        return "";
    }

    return (errorMsgJson.FindMember(HTTP_RESPONSE_ERROR_VALUE))->value.GetString();
}

std::shared_ptr<CapabilitiesDelegate> CapabilitiesDelegate::create(
    const std::shared_ptr<AuthDelegateInterface>& authDelegate,
    const std::shared_ptr<MiscStorageInterface>& miscStorage,
    const std::shared_ptr<HttpPutInterface>& httpPut,
    const std::shared_ptr<registrationManager::CustomerDataManager>& customerDataManager,
    const ConfigurationNode& configurationRoot,
    std::shared_ptr<DeviceInfo> deviceInfo) {
    const std::string errorEvent = "createFailed";
    const std::string errorReasonKey = "reason";

    if (!deviceInfo) {
        deviceInfo = DeviceInfo::create(configurationRoot);
        if (!deviceInfo) {
            ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "deviceInfoNotAvailable"));
            return nullptr;
        }
    }

    std::shared_ptr<CapabilitiesDelegate> instance(
        new CapabilitiesDelegate(authDelegate, miscStorage, httpPut, customerDataManager, deviceInfo));
    if (!(instance->init(configurationRoot))) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "CapabilitiesDelegateInitFailed"));
        return nullptr;
    }
    authDelegate->addAuthObserver(instance);

    return instance;
}

CapabilitiesDelegate::CapabilitiesDelegate(
    const std::shared_ptr<AuthDelegateInterface>& authDelegate,
    const std::shared_ptr<MiscStorageInterface>& miscStorage,
    const std::shared_ptr<HttpPutInterface>& httpPut,
    const std::shared_ptr<registrationManager::CustomerDataManager>& customerDataManager,
    const std::shared_ptr<DeviceInfo>& deviceInfo) :
        RequiresShutdown{"CapabilitiesDelegate"},
        CustomerDataHandler{customerDataManager},
        m_capabilitiesState{CapabilitiesObserverInterface::State::UNINITIALIZED},
        m_capabilitiesError{CapabilitiesObserverInterface::Error::UNINITIALIZED},
        m_authDelegate{authDelegate},
        m_miscStorage{miscStorage},
        m_httpPut{httpPut},
        m_deviceInfo{deviceInfo},
        m_currentAuthState{AuthObserverInterface::State::UNINITIALIZED},
        m_isCapabilitiesDelegateShutdown{false} {
}

void CapabilitiesDelegate::doShutdown() {
    {
        std::lock_guard<std::mutex> lock(m_publishWaitMutex);
        m_isCapabilitiesDelegateShutdown = true;
        m_publishWaitDone.notify_one();
        m_authStatusReady.notify_one();
    }
    m_executor.shutdown();
    m_authDelegate->removeAuthObserver(shared_from_this());
    {
        std::lock_guard<std::mutex> lock(m_capabilitiesMutex);
        m_capabilitiesObservers.clear();
    }
}

void CapabilitiesDelegate::clearData() {
    ACSDK_DEBUG5(LX(__func__));
    bool capabilitiesTableExists = false;
    if (m_miscStorage->tableExists(COMPONENT_NAME, CAPABILITIES_PUBLISH_TABLE, &capabilitiesTableExists)) {
        if (capabilitiesTableExists) {
            if (!m_miscStorage->clearTable(COMPONENT_NAME, CAPABILITIES_PUBLISH_TABLE)) {
                ACSDK_ERROR(LX("clearDataFailed")
                                .d("reason",
                                   "Unable to clear the table " + CAPABILITIES_PUBLISH_TABLE + " for component " +
                                       COMPONENT_NAME + ". Please clear the table for proper future functioning."));
            } else if (!m_miscStorage->deleteTable(COMPONENT_NAME, CAPABILITIES_PUBLISH_TABLE)) {
                ACSDK_ERROR(LX("clearDataFailed")
                                .d("reason",
                                   "Unable to delete the table " + CAPABILITIES_PUBLISH_TABLE + " for component " +
                                       COMPONENT_NAME + ". Please delete the table for proper future functioning."));
            }
        }
    } else {
        ACSDK_ERROR(LX("clearDataFailed")
                        .d("reason",
                           "Unable to check if the table " + CAPABILITIES_PUBLISH_TABLE + " for component " +
                               COMPONENT_NAME + "exists. Please delete the table for proper future functioning."));
    }
}

bool CapabilitiesDelegate::init(const ConfigurationNode& configurationRoot) {
    const std::string errorEvent = "initFailed";
    std::string infoEvent = "CapabilitiesDelegateInit";

#ifdef DEBUG
    std::string capabilitiesDelegateConfigurationMissing = "missingCapabilitiesDelegateConfigurationValue";
    auto capabilitiesDelegateConfiguration = configurationRoot[CONFIG_KEY_CAPABILITIES_DELEGATE];
    if (!capabilitiesDelegateConfiguration) {
        ACSDK_DEBUG0(LX(infoEvent)
                         .d("reason", capabilitiesDelegateConfigurationMissing)
                         .d("key", CONFIG_KEY_CAPABILITIES_DELEGATE));
    } else {
        capabilitiesDelegateConfiguration.getString(
            CONFIG_KEY_CAPABILITIES_API_ENDPOINT, &m_capabilitiesApiEndpoint, DEFAULT_CAPABILITIES_API_ENDPOINT);
        auto overriddenCapabilitiesPublishMsg =
            capabilitiesDelegateConfiguration[CONFIG_KEY_OVERRIDE_CAPABILITIES_API_REQUEST_BODY];
        if (overriddenCapabilitiesPublishMsg) {
            m_overridenCapabilitiesPublishMessageBody = overriddenCapabilitiesPublishMsg.serialize();
        }
    }
#endif
    if (m_capabilitiesApiEndpoint.empty()) {
        m_capabilitiesApiEndpoint = DEFAULT_CAPABILITIES_API_ENDPOINT;
    }

    if (!m_miscStorage->isOpened() && !m_miscStorage->open()) {
        ACSDK_DEBUG0(LX(infoEvent).m("Couldn't open misc database. Creating."));
        if (!m_miscStorage->createDatabase()) {
            ACSDK_ERROR(LX(errorEvent).m("Could not create misc database."));
            return false;
        }
    }

    bool capabilitiesTableExists = false;
    if (!m_miscStorage->tableExists(COMPONENT_NAME, CAPABILITIES_PUBLISH_TABLE, &capabilitiesTableExists)) {
        ACSDK_ERROR(LX(errorEvent).m("Could not get Capabilities table information misc database."));
        return false;
    }

    if (!capabilitiesTableExists) {
        ACSDK_DEBUG0(LX(infoEvent).m(
            "Table for Capabilities doesn't exist in misc database. Creating table " + CAPABILITIES_PUBLISH_TABLE +
            " for component " + COMPONENT_NAME + "."));
        if (!m_miscStorage->createTable(
                COMPONENT_NAME,
                CAPABILITIES_PUBLISH_TABLE,
                MiscStorageInterface::KeyType::STRING_KEY,
                MiscStorageInterface::ValueType::STRING_VALUE)) {
            ACSDK_ERROR(LX(errorEvent)
                            .m("Could not create misc table " + CAPABILITIES_PUBLISH_TABLE + " for component " +
                               COMPONENT_NAME + "."));
            return false;
        }
    }

    return true;
}

bool CapabilitiesDelegate::isCapabilityRegisteredLocked(
    const std::unordered_map<std::string, std::string>& capabilityMap) {
    std::string capabilityKey = getCapabilityKey(capabilityMap);
    return (m_registeredCapabilityConfigs.find(capabilityKey) != m_registeredCapabilityConfigs.end());
}

bool CapabilitiesDelegate::registerCapability(
    const std::shared_ptr<CapabilityConfigurationInterface>& capabilitiesProvider) {
    const std::string errorEvent = "registerCapabilityFailed";
    const std::string errorReasonKey = "reason";

    if (!capabilitiesProvider) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "capabilitiesConfigurationNotAvailable"));
        return false;
    }

    auto capabilities = capabilitiesProvider->getCapabilityConfigurations();
    if (capabilities.empty()) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "capabilitiesNotAvailable"));
        return false;
    }

    std::lock_guard<std::mutex> lock(m_capabilityMutex);
    for (const auto& capability : capabilities) {
        auto capabilityMap = capability->capabilityConfiguration;
        if (!isCapabilityMapCorrectlyFormed(capabilityMap)) {
            ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "capabilityNotDefinedCorrectly"));
            return false;
        }

        if (isCapabilityRegisteredLocked(capabilityMap)) {
            ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "capabilityAlreadyRegistered"));
            return false;
        }

        std::string capabilityKey = getCapabilityKey(capabilityMap);
        bool capabilityAdded = m_registeredCapabilityConfigs
                                   .insert({capabilityKey, std::make_shared<CapabilityConfiguration>(capabilityMap)})
                                   .second;
        if (!capabilityAdded) {
            ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "capabilityRegistryFailed"));
            return false;
        }
    }

    return true;
}

CapabilitiesDelegate::CapabilitiesPublishReturnCode CapabilitiesDelegate::publishCapabilities() {
    std::lock_guard<std::mutex> lock(m_capabilityMutex);
    const std::string errorEvent = "publishCapabilitiesFailed";
    const std::string errorReasonKey = "reason";

    m_capabilitiesPublishMessage = getCapabilitiesPublishMessageBody();
    if (m_capabilitiesPublishMessage.empty()) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "emptyCapabiltiesList"));
        setCapabilitiesState(
            CapabilitiesObserverInterface::State::FATAL_ERROR, CapabilitiesObserverInterface::Error::BAD_REQUEST);
        return CapabilitiesDelegate::CapabilitiesPublishReturnCode::FATAL_ERROR;
    }

    if (!isCapabilitiesPublishDataDifferent()) {
        setCapabilitiesState(
            CapabilitiesObserverInterface::State::SUCCESS, CapabilitiesObserverInterface::Error::SUCCESS);
        return CapabilitiesDelegate::CapabilitiesPublishReturnCode::SUCCESS;
    }

    std::string authToken = getAuthToken();

    if (isShuttingDown()) {
        ACSDK_DEBUG9(LX("publishCapabilitiesFailed").d("reason", "shutdownWhileAuthorizing"));
        setCapabilitiesState(
            CapabilitiesObserverInterface::State::UNINITIALIZED, CapabilitiesObserverInterface::Error::SUCCESS);
        return CapabilitiesDelegate::CapabilitiesPublishReturnCode::FATAL_ERROR;
    }

    const std::string capabilitiesApiUrl = getCapabilitiesApiUrl(SELF_DEVICE);

    const std::vector<std::string> httpHeaderData = {
        CONTENT_TYPE_HEADER_KEY + HEADER_KEY_VALUE_SEPARATOR + CONTENT_TYPE_HEADER_VALUE,
        CONTENT_LENGTH_HEADER_KEY + HEADER_KEY_VALUE_SEPARATOR + std::to_string(m_capabilitiesPublishMessage.size()),
        AUTHORIZATION_HEADER_KEY + HEADER_KEY_VALUE_SEPARATOR + authToken,
        ACCEPT_HEADER_KEY + HEADER_KEY_VALUE_SEPARATOR,
        EXPECT_HEADER_KEY + HEADER_KEY_VALUE_SEPARATOR};

    HTTPResponse httpResponse = m_httpPut->doPut(capabilitiesApiUrl, httpHeaderData, m_capabilitiesPublishMessage);

    switch (httpResponse.code) {
        case HTTPResponseCode::SUCCESS_NO_CONTENT:
            if (!saveCapabilitiesPublishData()) {
                ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "unableToSaveCapabilitiesData"));
                setCapabilitiesState(
                    CapabilitiesObserverInterface::State::FATAL_ERROR,
                    CapabilitiesObserverInterface::Error::UNKNOWN_ERROR);
                return CapabilitiesDelegate::CapabilitiesPublishReturnCode::FATAL_ERROR;
            }
            setCapabilitiesState(
                CapabilitiesObserverInterface::State::SUCCESS, CapabilitiesObserverInterface::Error::SUCCESS);
            return CapabilitiesDelegate::CapabilitiesPublishReturnCode::SUCCESS;
        case HTTPResponseCode::CLIENT_ERROR_BAD_REQUEST:
            ACSDK_ERROR(
                LX(errorEvent).d(errorReasonKey, "badRequest: " + getErrorMsgFromHttpResponse(httpResponse.body)));
            setCapabilitiesState(
                CapabilitiesObserverInterface::State::FATAL_ERROR, CapabilitiesObserverInterface::Error::BAD_REQUEST);
            return CapabilitiesDelegate::CapabilitiesPublishReturnCode::FATAL_ERROR;
        case HTTPResponseCode::CLIENT_ERROR_FORBIDDEN:
            ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "authenticationFailed"));
            setCapabilitiesState(
                CapabilitiesObserverInterface::State::FATAL_ERROR, CapabilitiesObserverInterface::Error::FORBIDDEN);
            m_authDelegate->onAuthFailure(authToken);
            return CapabilitiesDelegate::CapabilitiesPublishReturnCode::FATAL_ERROR;
        case HTTPResponseCode::SERVER_ERROR_INTERNAL:
            ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "internalServiceError"));
            setCapabilitiesState(
                CapabilitiesObserverInterface::State::RETRIABLE_ERROR,
                CapabilitiesObserverInterface::Error::SERVER_INTERNAL_ERROR);
            return CapabilitiesDelegate::CapabilitiesPublishReturnCode::RETRIABLE_ERROR;
        default:
            ACSDK_ERROR(LX(errorEvent)
                            .d("responseCode", httpResponse.code)
                            .d(errorReasonKey, "httpRequestFailed " + httpResponse.serialize()));
            setCapabilitiesState(
                CapabilitiesObserverInterface::State::RETRIABLE_ERROR,
                CapabilitiesObserverInterface::Error::UNKNOWN_ERROR);
            return CapabilitiesDelegate::CapabilitiesPublishReturnCode::RETRIABLE_ERROR;
    }
}

void CapabilitiesDelegate::publishCapabilitiesAsyncWithRetries() {
    m_executor.submit([this]() {
        int retryCount = 0;
        CapabilitiesDelegateInterface::CapabilitiesPublishReturnCode capabilitiesPublishReturnCode =
            publishCapabilities();

        while (CapabilitiesDelegateInterface::CapabilitiesPublishReturnCode::RETRIABLE_ERROR ==
               capabilitiesPublishReturnCode) {
            std::chrono::milliseconds retryBackoff =
                acl::TransportDefines::RETRY_TIMER.calculateTimeToRetry(retryCount++);
            ACSDK_ERROR(LX("capabilitiesPublishFailed").d("reason", "serverError").d("retryCount", retryCount));

            {
                std::unique_lock<std::mutex> lock(m_publishWaitMutex);
                m_publishWaitDone.wait_for(lock, retryBackoff, [this] { return isShuttingDownLocked(); });
            }

            if (isShuttingDown()) {
                capabilitiesPublishReturnCode =
                    CapabilitiesDelegateInterface::CapabilitiesPublishReturnCode::FATAL_ERROR;
            } else {
                capabilitiesPublishReturnCode = publishCapabilities();
            }
        }

        if (CapabilitiesDelegateInterface::CapabilitiesPublishReturnCode::FATAL_ERROR ==
            capabilitiesPublishReturnCode) {
            ACSDK_ERROR(LX("capabilitiesPublishFailed").d("reason", "unableToPublishCapabilities"));
        }
    });
}

std::string CapabilitiesDelegate::getCapabilitiesPublishMessageBodyFromRegisteredCapabilitiesLocked() {
    if (m_registeredCapabilityConfigs.empty()) {
        return "";
    }

    const std::string errorEvent = "getCapabilitiesPublishMessageBodyFromRegisteredCapabilitiesFailed";
    const std::string errorReasonKey = "reason";
    rapidjson::Document capabilitiesPublishMessageBody(kObjectType);
    auto& allocator = capabilitiesPublishMessageBody.GetAllocator();

    capabilitiesPublishMessageBody.AddMember(StringRef(ENVELOPE_VERSION_KEY), ENVELOPE_VERSION_VALUE, allocator);

    rapidjson::Value capabilities(kArrayType);
    for (auto capabilityConfigIterator : m_registeredCapabilityConfigs) {
        auto capabilityMap = (capabilityConfigIterator.second)->capabilityConfiguration;

        /// You should not have been able to register incomplete capabilties. But, checking just in case.
        if (!isCapabilityMapCorrectlyFormed(capabilityMap)) {
            ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "capabilityIsInvalid"));
            return "";
        }

        rapidjson::Value capability(kObjectType);
        auto foundValueIterator = capabilityMap.find(CAPABILITY_INTERFACE_TYPE_KEY);
        if (foundValueIterator != capabilityMap.end()) {
            capability.AddMember(StringRef(CAPABILITY_INTERFACE_TYPE_KEY), foundValueIterator->second, allocator);
        } else {
            ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "capabilityInterfaceTypeNotFound"));
            return "";
        }
        foundValueIterator = capabilityMap.find(CAPABILITY_INTERFACE_NAME_KEY);
        if (foundValueIterator != capabilityMap.end()) {
            capability.AddMember(StringRef(CAPABILITY_INTERFACE_NAME_KEY), foundValueIterator->second, allocator);
        } else {
            ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "capabilityInterfaceNameNotFound"));
            return "";
        }
        foundValueIterator = capabilityMap.find(CAPABILITY_INTERFACE_VERSION_KEY);
        if (foundValueIterator != capabilityMap.end()) {
            capability.AddMember(StringRef(CAPABILITY_INTERFACE_VERSION_KEY), foundValueIterator->second, allocator);
        } else {
            ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "capabilityInterfaceVersionNotFound"));
            return "";
        }
        if (capabilityMap.size() == CAPABILITY_MAP_MAXIMUM_ENTRIES) {
            foundValueIterator = capabilityMap.find(CAPABILITY_INTERFACE_CONFIGURATIONS_KEY);
            if (foundValueIterator != capabilityMap.end()) {
                std::string capabilityConfigurationsString = foundValueIterator->second;
                rapidjson::Value capabilityConfigurationsJson;
                if (getJsonFromString(capabilityConfigurationsString, allocator, &capabilityConfigurationsJson)) {
                    capability.AddMember(
                        StringRef(CAPABILITY_INTERFACE_CONFIGURATIONS_KEY), capabilityConfigurationsJson, allocator);
                } else {
                    ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "capabilityInterfaceConfigurationsIsInvalid"));
                    return "";
                }
            } else {
                ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "capabilityInterfaceConfigurationsNotFound"));
                return "";
            }
        }
        capabilities.PushBack(capability, allocator);
    }
    capabilitiesPublishMessageBody.AddMember(StringRef(CAPABILITIES_KEY), capabilities, allocator);

    std::string capabilitiesPublishMessageString;
    if (!getStringFromJson(capabilitiesPublishMessageBody, &capabilitiesPublishMessageString)) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "unableToGetCapabilitiesPublishMessageBodyString"));
        return "";
    }

    m_envelopeVersion = ENVELOPE_VERSION_VALUE;
    m_capabilityConfigs = m_registeredCapabilityConfigs;

    return capabilitiesPublishMessageString;
}

std::string getCapabilityInfoStringFromJson(const rapidjson::Value& capabilityJson, const std::string& jsonMemberKey) {
    const std::string event = "getCapabilityInfoStringFromJsonFailed";
    const std::string reasonKey = "reason";

    if (!capabilityJson.HasMember(jsonMemberKey)) {
        ACSDK_ERROR(LX(event).d(reasonKey, "jsonMemberNotAvailable: " + jsonMemberKey));
        return "";
    }
    const rapidjson::Value& capabilityTypeJson = (capabilityJson.FindMember(jsonMemberKey))->value;
    if (!capabilityTypeJson.IsString()) {
        ACSDK_ERROR(LX(event).d(reasonKey, "jsonMemberNotString: " + jsonMemberKey));
        return "";
    }
    return capabilityTypeJson.GetString();
}

std::string getCapabilityConfigsStringFromJson(const rapidjson::Value& capabilityJson) {
    const std::string event = "getCapabilityConfigsStringFromJsonFailed";
    const std::string reasonKey = "reason";
    if (!capabilityJson.HasMember(CAPABILITY_INTERFACE_CONFIGURATIONS_KEY)) {
        ACSDK_DEBUG0(LX(event).m("Interface configurations not available"));
        return "";
    }
    const rapidjson::Value& capabilityConfigJson =
        (capabilityJson.FindMember(CAPABILITY_INTERFACE_CONFIGURATIONS_KEY))->value;
    std::string capabilityConfigsString;
    if (!getStringFromJson(capabilityConfigJson, &capabilityConfigsString)) {
        ACSDK_ERROR(LX(event).d(reasonKey, "jsonParsingFailed"));
        return "";
    }
    return capabilityConfigsString;
}

bool getStringFromJson(const rapidjson::Value& jsonValue, std::string* jsonString) {
    const std::string event = "getStringFromJsonFailed";
    const std::string reasonKey = "reason";
    if (!jsonString) {
        ACSDK_ERROR(LX(event).m("jsonString is nullptr."));
        return false;
    }
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!jsonValue.Accept(writer)) {
        ACSDK_ERROR(LX(event).d(reasonKey, "writerRefusedJson"));
        return false;
    }
    const char* bufferData = buffer.GetString();
    if (!bufferData) {
        ACSDK_ERROR(LX(event).d(reasonKey, "nullptrJsonBufferString"));
        return false;
    }
    *jsonString = std::string(bufferData);
    return true;
}

bool getJsonFromString(
    const std::string& jsonString,
    rapidjson::Value::AllocatorType& allocator,
    rapidjson::Value* jsonValue) {
    const std::string event = "getJsonFromStringFailed";
    const std::string reasonKey = "reason";
    if (!jsonValue) {
        ACSDK_ERROR(LX(event).m("jsonValue is nullptr."));
        return false;
    }
    if (jsonString.empty()) {
        ACSDK_ERROR(LX(event).d(reasonKey, "jsonStringEmpty"));
        return false;
    }
    rapidjson::Document jsonDoc;
    if (jsonDoc.Parse(jsonString).HasParseError()) {
        ACSDK_ERROR(LX(event).d(reasonKey, "jsonStringNotValidJson"));
        return false;
    }
    *jsonValue = rapidjson::Value(jsonDoc, allocator);

    return true;
}

std::string CapabilitiesDelegate::getCapabilitiesPublishMessageBodyFromOverride() {
    if (m_overridenCapabilitiesPublishMessageBody.empty()) {
        return "";
    }

    const std::string errorEvent = "getCapabilitiesPublishMessageBodyFromOverrideFailed";
    const std::string errorReasonKey = "reason";

    std::string envelopeVersion;
    std::unordered_map<std::string, std::shared_ptr<CapabilityConfiguration>> capabilityConfigs;

    rapidjson::Document overridenCapabilitiesPublishMessageJson;
    overridenCapabilitiesPublishMessageJson.Parse(m_overridenCapabilitiesPublishMessageBody);

    if (overridenCapabilitiesPublishMessageJson.HasParseError()) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "Error parsing the message"));
        return "";
    }

    unsigned int numOfMembersInJson = 2;
    if (overridenCapabilitiesPublishMessageJson.MemberCount() != numOfMembersInJson) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "The message has incorrect number of components"));
        return "";
    }

    if ((!overridenCapabilitiesPublishMessageJson.HasMember(ENVELOPE_VERSION_KEY)) ||
        (!overridenCapabilitiesPublishMessageJson.HasMember(CAPABILITIES_KEY))) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "The message does not have the required components"));
        return "";
    }

    const rapidjson::Value& envelopeVersionJson =
        (overridenCapabilitiesPublishMessageJson.FindMember(ENVELOPE_VERSION_KEY))->value;
    if (!envelopeVersionJson.IsString()) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "Envelope version value is not a string"));
        return "";
    }
    envelopeVersion = envelopeVersionJson.GetString();

    const rapidjson::Value& capabiltiesJson =
        (overridenCapabilitiesPublishMessageJson.FindMember(CAPABILITIES_KEY))->value;
    if (!capabiltiesJson.IsArray()) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "Capabilities list is not an array"));
        return "";
    }
    for (rapidjson::SizeType capabilityIndx = 0; capabilityIndx < capabiltiesJson.Size(); capabilityIndx++) {
        const rapidjson::Value& capabilityJson = capabiltiesJson[capabilityIndx];
        std::unordered_map<std::string, std::string> capabilityMap;

        unsigned int numOfMembers = capabilityJson.MemberCount();
        if ((numOfMembers < CAPABILITY_MAP_REQUIRED_ENTRIES) || (numOfMembers > CAPABILITY_MAP_MAXIMUM_ENTRIES)) {
            ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "Capability has incorrect number of components"));
            return "";
        }

        std::string capabilityType = getCapabilityInfoStringFromJson(capabilityJson, CAPABILITY_INTERFACE_TYPE_KEY);
        if (capabilityType.empty()) {
            ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "Capability does not have an appropriate interface type"));
            return "";
        }
        capabilityMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, capabilityType});

        std::string capabilityName = getCapabilityInfoStringFromJson(capabilityJson, CAPABILITY_INTERFACE_NAME_KEY);
        if (capabilityName.empty()) {
            ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "Capability does not have an appropriate interface name"));
            return "";
        }
        capabilityMap.insert({CAPABILITY_INTERFACE_NAME_KEY, capabilityName});

        std::string capabilityVersion =
            getCapabilityInfoStringFromJson(capabilityJson, CAPABILITY_INTERFACE_VERSION_KEY);
        if (capabilityVersion.empty()) {
            ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "Capability does not have an appropriate interface version"));
            return "";
        }
        capabilityMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, capabilityVersion});

        if (numOfMembers == CAPABILITY_MAP_MAXIMUM_ENTRIES) {
            std::string capabilityConfigurations = getCapabilityConfigsStringFromJson(capabilityJson);
            if (capabilityConfigurations.empty()) {
                ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "Error in processing capability configurations"));
                return "";
            }
            capabilityMap.insert({CAPABILITY_INTERFACE_CONFIGURATIONS_KEY, capabilityConfigurations});
        }

        std::string capabilityKey = getCapabilityKey(capabilityMap);
        bool capabilityAdded =
            capabilityConfigs.insert({capabilityKey, std::make_shared<CapabilityConfiguration>(capabilityMap)}).second;
        if (!capabilityAdded) {
            ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "Error in processing message"));
            return "";
        }
    }

    m_envelopeVersion = envelopeVersion;
    m_capabilityConfigs = capabilityConfigs;

    return m_overridenCapabilitiesPublishMessageBody;
}

std::string CapabilitiesDelegate::getCapabilitiesPublishMessageBody() {
    std::string capabilitiesMessageBody = getCapabilitiesPublishMessageBodyFromOverride();

    if (capabilitiesMessageBody.empty()) {
        capabilitiesMessageBody = getCapabilitiesPublishMessageBodyFromRegisteredCapabilitiesLocked();
    }

    return capabilitiesMessageBody;
}

std::string CapabilitiesDelegate::getCapabilitiesApiUrl(const std::string& deviceId) {
    return m_capabilitiesApiEndpoint + CAPABILITIES_API_URL_PRE_DEVICE_SUFFIX + deviceId +
           CAPABILITIES_API_URL_POST_DEVICE_SUFFIX;
}

void CapabilitiesDelegate::addCapabilitiesObserver(std::shared_ptr<CapabilitiesObserverInterface> observer) {
    ACSDK_DEBUG5(LX("addCapabilitiesObserver").d("observer", observer.get()));

    std::lock_guard<std::mutex> lock(m_capabilitiesMutex);
    if (!observer) {
        ACSDK_ERROR(LX("addCapabilitiesObserverFailed").d("reason", "nullObserver"));
        return;
    }
    if (!m_capabilitiesObservers.insert(observer).second) {
        ACSDK_WARN(LX("addCapabilitiesObserverFailed").d("reason", "observerAlreadyAdded"));
        return;
    }
    observer->onCapabilitiesStateChange(m_capabilitiesState, m_capabilitiesError);
}

void CapabilitiesDelegate::removeCapabilitiesObserver(std::shared_ptr<CapabilitiesObserverInterface> observer) {
    ACSDK_DEBUG5(LX("removeCapabilitiesObserver").d("observer", observer.get()));

    std::lock_guard<std::mutex> lock(m_capabilitiesMutex);
    if (!observer) {
        ACSDK_ERROR(LX("removeCapabilitiesObserverFailed").d("reason", "nullObserver"));
    } else if (m_capabilitiesObservers.erase(observer) == 0) {
        ACSDK_WARN(LX("removeCapabilitiesObserverFailed").d("reason", "observerNotAdded"));
    }
}

void CapabilitiesDelegate::setCapabilitiesState(
    CapabilitiesObserverInterface::State newCapabilitiesState,
    CapabilitiesObserverInterface::Error newCapabilitiesError) {
    ACSDK_DEBUG5(LX("setCapabilitiesState").d("newCapabilitiesState", newCapabilitiesState));

    std::lock_guard<std::mutex> lock(m_capabilitiesMutex);
    if ((newCapabilitiesState == m_capabilitiesState) && (newCapabilitiesError == m_capabilitiesError)) {
        return;
    }
    m_capabilitiesState = newCapabilitiesState;
    m_capabilitiesError = newCapabilitiesError;

    if (!m_capabilitiesObservers.empty()) {
        ACSDK_DEBUG9(
            LX("callingOnCapabilitiesStateChange").d("state", m_capabilitiesState).d("error", m_capabilitiesError));
        for (auto& observer : m_capabilitiesObservers) {
            observer->onCapabilitiesStateChange(m_capabilitiesState, m_capabilitiesError);
        }
    }
}

void CapabilitiesDelegate::onAuthStateChange(
    AuthObserverInterface::State newState,
    AuthObserverInterface::Error newError) {
    std::lock_guard<std::mutex> lock(m_authStatusMutex);
    m_currentAuthState = newState;
    if ((AuthObserverInterface::State::REFRESHED == m_currentAuthState)) {
        m_authStatusReady.notify_one();
    }
}

std::string CapabilitiesDelegate::getAuthToken() {
    {
        std::unique_lock<std::mutex> lock(m_authStatusMutex);
        m_authStatusReady.wait(lock, [this]() {
            return (isShuttingDown() || (AuthObserverInterface::State::REFRESHED == m_currentAuthState));
        });

        if (isShuttingDown()) {
            ACSDK_DEBUG9(LX("getAuthTokenFailed").d("reason", "shutdownWhileWaitingForToken"));
            return "";
        }
    }

    return m_authDelegate->getAuthToken();
}

void CapabilitiesDelegate::getPreviouslySentCapabilitiesPublishData() {
    const std::string dbKeysPrefix = DB_KEY_ENDPOINT + m_capabilitiesApiEndpoint + DB_KEY_SEPARATOR;

    std::string previousClientId;
    std::string previousProductId;
    std::string previousDeviceSerialNumber;
    m_miscStorage->get(COMPONENT_NAME, CAPABILITIES_PUBLISH_TABLE, dbKeysPrefix + DB_KEY_CLIENT_ID, &previousClientId);
    m_miscStorage->get(
        COMPONENT_NAME, CAPABILITIES_PUBLISH_TABLE, dbKeysPrefix + DB_KEY_PRODUCT_ID, &previousProductId);
    m_miscStorage->get(
        COMPONENT_NAME, CAPABILITIES_PUBLISH_TABLE, dbKeysPrefix + DB_KEY_DSN, &previousDeviceSerialNumber);

    if (!previousClientId.empty() && !previousProductId.empty() && !previousDeviceSerialNumber.empty()) {
        m_previousDeviceInfo = DeviceInfo::create(previousClientId, previousProductId, previousDeviceSerialNumber);
    } else {
        m_previousDeviceInfo.reset();
    }

    m_previousEnvelopeVersion = "";
    m_miscStorage->get(
        COMPONENT_NAME, CAPABILITIES_PUBLISH_TABLE, dbKeysPrefix + DB_KEY_ENVELOPE_VERSION, &m_previousEnvelopeVersion);

    std::string previousCapabilitiesPublishMsgStr;
    m_miscStorage->get(
        COMPONENT_NAME,
        CAPABILITIES_PUBLISH_TABLE,
        dbKeysPrefix + DB_KEY_PUBLISH_MSG,
        &previousCapabilitiesPublishMsgStr);
    if (!previousCapabilitiesPublishMsgStr.empty()) {
        rapidjson::Document previousCapabilitiesPublishMsgJson;
        previousCapabilitiesPublishMsgJson.Parse(previousCapabilitiesPublishMsgStr);

        for (rapidjson::SizeType capabilityIndx = 0;
             capabilityIndx < previousCapabilitiesPublishMsgJson[CAPABILITIES_KEY].Size();
             capabilityIndx++) {
            const rapidjson::Value& capabilityJson =
                previousCapabilitiesPublishMsgJson[CAPABILITIES_KEY][capabilityIndx];
            std::unordered_map<std::string, std::string> capabilityMap;

            std::string capabilityType = (capabilityJson.FindMember(CAPABILITY_INTERFACE_TYPE_KEY))->value.GetString();
            capabilityMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, capabilityType});

            std::string capabilityName = (capabilityJson.FindMember(CAPABILITY_INTERFACE_NAME_KEY))->value.GetString();
            capabilityMap.insert({CAPABILITY_INTERFACE_NAME_KEY, capabilityName});

            std::string capabilityVersion =
                (capabilityJson.FindMember(CAPABILITY_INTERFACE_VERSION_KEY))->value.GetString();
            capabilityMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, capabilityVersion});

            std::string capabilityConfigurations = getCapabilityConfigsStringFromJson(capabilityJson);
            if (!capabilityConfigurations.empty()) {
                capabilityMap.insert({CAPABILITY_INTERFACE_CONFIGURATIONS_KEY, capabilityConfigurations});
            }

            std::string capabilityKey = getCapabilityKey(capabilityMap);
            m_previousCapabilityConfigs.insert(
                {capabilityKey, std::make_shared<CapabilityConfiguration>(capabilityMap)});
        }
    }
}

bool CapabilitiesDelegate::isCapabilitiesPublishDataDifferent() {
    getPreviouslySentCapabilitiesPublishData();

    if ((!m_previousDeviceInfo) || (*m_previousDeviceInfo != *m_deviceInfo)) {
        return true;
    }

    if (m_previousEnvelopeVersion != m_envelopeVersion) {
        return true;
    }

    return isCapabilitiesPublishMessageDifferent();
}

bool CapabilitiesDelegate::isCapabilitiesPublishMessageDifferent() {
    if (m_previousCapabilityConfigs.size() != m_capabilityConfigs.size()) {
        return true;
    }

    for (const auto& capConfigIterator : m_capabilityConfigs) {
        std::string capConfigKey = capConfigIterator.first;
        std::unordered_map<std::string, std::string> currCapConfigMap =
            (capConfigIterator.second)->capabilityConfiguration;

        auto prevCapConfigIterator = m_previousCapabilityConfigs.find(capConfigKey);
        if (prevCapConfigIterator == m_previousCapabilityConfigs.end()) {
            return true;
        }

        std::unordered_map<std::string, std::string> prevCapConfigMap =
            (prevCapConfigIterator->second)->capabilityConfiguration;

        CapabilityConfiguration currCapConfig(currCapConfigMap);
        CapabilityConfiguration prevCapConfig(prevCapConfigMap);

        if (currCapConfig != prevCapConfigMap) {
            return true;
        }
    }

    return false;
}

void logFailedSaveAndClearCapabilitiesPublishTable(const std::shared_ptr<MiscStorageInterface>& miscStorage) {
    ACSDK_ERROR(
        LX("saveCapabilitiesPublishDataFailed")
            .d("reason",
               "Unable to update the table " + CAPABILITIES_PUBLISH_TABLE + " for component " + COMPONENT_NAME + "."));
    ACSDK_INFO(LX("saveCapabilitiesPublishDataInfo")
                   .m("Clearing table " + CAPABILITIES_PUBLISH_TABLE + " for component " + COMPONENT_NAME + "."));
    if (!miscStorage->clearTable(COMPONENT_NAME, CAPABILITIES_PUBLISH_TABLE)) {
        ACSDK_ERROR(LX("saveCapabilitiesPublishDataFailed")
                        .d("reason",
                           "Unable to clear the table " + CAPABILITIES_PUBLISH_TABLE + " for component " +
                               COMPONENT_NAME + ". Please clear the table for proper future functioning."));
    }
}

bool CapabilitiesDelegate::saveCapabilitiesPublishData() {
    const std::string dbKeysPrefix = DB_KEY_ENDPOINT + m_capabilitiesApiEndpoint + DB_KEY_SEPARATOR;

    if ((!m_previousDeviceInfo) || (m_previousDeviceInfo->getClientId() != m_deviceInfo->getClientId())) {
        if (!m_miscStorage->put(
                COMPONENT_NAME,
                CAPABILITIES_PUBLISH_TABLE,
                dbKeysPrefix + DB_KEY_CLIENT_ID,
                m_deviceInfo->getClientId())) {
            logFailedSaveAndClearCapabilitiesPublishTable(m_miscStorage);
            return false;
        }
    }

    if ((!m_previousDeviceInfo) || (m_previousDeviceInfo->getProductId() != m_deviceInfo->getProductId())) {
        if (!m_miscStorage->put(
                COMPONENT_NAME,
                CAPABILITIES_PUBLISH_TABLE,
                dbKeysPrefix + DB_KEY_PRODUCT_ID,
                m_deviceInfo->getProductId())) {
            logFailedSaveAndClearCapabilitiesPublishTable(m_miscStorage);
            return false;
        }
    }

    if ((!m_previousDeviceInfo) ||
        (m_previousDeviceInfo->getDeviceSerialNumber() != m_deviceInfo->getDeviceSerialNumber())) {
        if (!m_miscStorage->put(
                COMPONENT_NAME,
                CAPABILITIES_PUBLISH_TABLE,
                dbKeysPrefix + DB_KEY_DSN,
                m_deviceInfo->getDeviceSerialNumber())) {
            logFailedSaveAndClearCapabilitiesPublishTable(m_miscStorage);
            return false;
        }
    }

    if ((m_previousEnvelopeVersion.empty()) || (m_previousEnvelopeVersion != m_envelopeVersion)) {
        if (!m_miscStorage->put(
                COMPONENT_NAME,
                CAPABILITIES_PUBLISH_TABLE,
                dbKeysPrefix + DB_KEY_ENVELOPE_VERSION,
                m_envelopeVersion)) {
            logFailedSaveAndClearCapabilitiesPublishTable(m_miscStorage);
            return false;
        }
    }

    if (m_previousCapabilityConfigs.empty() || isCapabilitiesPublishMessageDifferent()) {
        if (!m_miscStorage->put(
                COMPONENT_NAME,
                CAPABILITIES_PUBLISH_TABLE,
                dbKeysPrefix + DB_KEY_PUBLISH_MSG,
                m_capabilitiesPublishMessage)) {
            logFailedSaveAndClearCapabilitiesPublishTable(m_miscStorage);
            return false;
        }
    }

    return true;
}

bool CapabilitiesDelegate::isShuttingDown() {
    std::lock_guard<std::mutex> lock(m_publishWaitMutex);
    return isShuttingDownLocked();
}

bool CapabilitiesDelegate::isShuttingDownLocked() {
    return m_isCapabilitiesDelegateShutdown;
}

void CapabilitiesDelegate::invalidateCapabilities() {
    if (!m_miscStorage->clearTable(COMPONENT_NAME, CAPABILITIES_PUBLISH_TABLE)) {
        ACSDK_ERROR(LX("invalidateCapabilities")
                        .d("reason",
                           "Unable to clear the table " + CAPABILITIES_PUBLISH_TABLE + " for component " +
                               COMPONENT_NAME + ". Please clear the table for proper future functioning."));
    }
}

}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK
