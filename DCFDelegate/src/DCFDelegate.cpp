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

#include "DCFDelegate/DCFDelegate.h"

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
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPutInterface.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpResponseCodes.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPResponse.h>
#include <AVSCommon/Utils/LibcurlUtils/LibcurlUtils.h>

namespace alexaClientSDK {
namespace dcfDelegate {

using namespace avsCommon::avs;
using namespace avsCommon::utils;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::libcurlUtils;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::storage;
using namespace rapidjson;

/// String to identify log entries originating from this file.
static const std::string TAG("DCFDelegate");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Name of @c ConfigurationNode for DCFDelegate
static const std::string CONFIG_KEY_DCF_DELEGATE = "dcfDelegate";
/// Name of overridenDcfPublishMessageBody value in DCFDelegate's @c ConfigurationNode.
static const std::string CONFIG_KEY_OVERRIDE_DCF_REQUEST_BODY = "overridenDcfPublishMessageBody";
/// Name of endpoint value in DCFDelegate's @c ConfigurationNode.
static const std::string CONFIG_KEY_DCF_ENDPOINT = "endpoint";

/// Constituents of the DCF URL
/// DCF endpoint
static const std::string DEFAULT_DCF_ENDPOINT = "https://api.amazonalexa.com";
/// Suffix before the device's place in the URL
static const std::string DCF_URL_PRE_DEVICE_SUFFIX = "/v1/devices/";
/// Suffix after the device's place in the URL
static const std::string DCF_URL_POST_DEVICE_SUFFIX = "/capabilities";
/// Device ID for the device that will show up in the URL
static const std::string SELF_DEVICE = "@self";

/// Constants for the DCF publish message json
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
static const std::string COMPONENT_NAME = "dcfDelegate";
/// DCF Publish message table
static const std::string DCF_PUBLISH_TABLE = "dcfPublishMessage";

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
 * @return The error message from the HTTP response if the json is valid, else ""
 */
static std::string getErrorMsgFromHttpResponse(const std::string& httpResponse);

/**
 * Get a capability info string from the capability json.
 *
 * @param capabilityJson The capability json
 * @param jsonMemberKey the member key in the json where the capability info can be found
 * @return The capability info string if found, else "" ("" is ok as a return for errors because a null string itself is
 * also an error)
 */
static std::string getCapabilityInfoStringFromJson(
    const rapidjson::Value& capabilityJson,
    const std::string& jsonMemberKey);

/**
 * Helper method that will log a fail for saving DCF publish data to misc DB and then attempt to clear the table.
 *
 * @param miscStorage The misc storage handler.
 */
static void logFailedSaveAndClearDCFPublishTable(const std::shared_ptr<MiscStorageInterface>& miscStorage);

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

std::shared_ptr<DCFDelegate> DCFDelegate::create(
    const std::shared_ptr<AuthDelegateInterface>& authDelegate,
    const std::shared_ptr<MiscStorageInterface>& miscStorage,
    const std::shared_ptr<HttpPutInterface>& httpPut,
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

    std::shared_ptr<DCFDelegate> instance(new DCFDelegate(authDelegate, miscStorage, httpPut, deviceInfo));
    if (!(instance->init(configurationRoot))) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "DCFDelegateInitFailed"));
        return nullptr;
    }
    authDelegate->addAuthObserver(instance);

    return instance;
}

DCFDelegate::DCFDelegate(
    const std::shared_ptr<AuthDelegateInterface>& authDelegate,
    const std::shared_ptr<MiscStorageInterface>& miscStorage,
    const std::shared_ptr<HttpPutInterface>& httpPut,
    const std::shared_ptr<DeviceInfo>& deviceInfo) :
        RequiresShutdown{"DCFDelegate"},
        m_dcfState{DCFObserverInterface::State::UNINITIALIZED},
        m_dcfError{DCFObserverInterface::Error::UNINITIALIZED},
        m_authDelegate{authDelegate},
        m_miscStorage{miscStorage},
        m_httpPut{httpPut},
        m_deviceInfo{deviceInfo},
        m_currentAuthState{AuthObserverInterface::State::UNINITIALIZED},
        m_isDCFDelegateShutdown{false} {
}

void DCFDelegate::doShutdown() {
    {
        std::lock_guard<std::mutex> lock(m_publishWaitMutex);
        m_isDCFDelegateShutdown = true;
        m_publishWaitDone.notify_one();
    }
    m_executor.shutdown();
    m_authDelegate->removeAuthObserver(shared_from_this());
    {
        std::lock_guard<std::mutex> lock(m_dcfMutex);
        m_dcfObservers.clear();
    }
}

bool DCFDelegate::init(const ConfigurationNode& configurationRoot) {
    const std::string errorEvent = "initFailed";
    std::string infoEvent = "DCFDelegateInit";

#ifdef DEBUG
    std::string dcfDelegateConfigurationMissing = "missingDCFDelegateConfigurationValue";
    auto dcfDelegateConfiguration = configurationRoot[CONFIG_KEY_DCF_DELEGATE];
    if (!dcfDelegateConfiguration) {
        ACSDK_INFO(LX(infoEvent).d("reason", dcfDelegateConfigurationMissing).d("key", CONFIG_KEY_DCF_DELEGATE));
    } else {
        dcfDelegateConfiguration.getString(CONFIG_KEY_DCF_ENDPOINT, &m_dcfEndpoint, DEFAULT_DCF_ENDPOINT);
        auto overriddenDcfPublishMsg = dcfDelegateConfiguration[CONFIG_KEY_OVERRIDE_DCF_REQUEST_BODY];
        if (overriddenDcfPublishMsg) {
            m_overridenDcfPublishMessageBody = overriddenDcfPublishMsg.serialize();
        }
    }
#endif
    if (m_dcfEndpoint.empty()) {
        m_dcfEndpoint = DEFAULT_DCF_ENDPOINT;
    }

    if (!m_miscStorage->open()) {
        ACSDK_INFO(LX(infoEvent).m("Couldn't open misc database. Creating."));
        if (!m_miscStorage->createDatabase()) {
            ACSDK_ERROR(LX(errorEvent).m("Could not create misc database."));
            return false;
        }
    }

    bool dcfTableExists = false;
    if (!m_miscStorage->tableExists(COMPONENT_NAME, DCF_PUBLISH_TABLE, &dcfTableExists)) {
        ACSDK_ERROR(LX(errorEvent).m("Could not get DCF table information misc database."));
        return false;
    }

    if (!dcfTableExists) {
        ACSDK_INFO(LX(infoEvent).m(
            "Table for DCF doesn't exist in misc database. Creating table " + DCF_PUBLISH_TABLE + " for component " +
            COMPONENT_NAME + "."));
        if (!m_miscStorage->createTable(
                COMPONENT_NAME,
                DCF_PUBLISH_TABLE,
                MiscStorageInterface::KeyType::STRING_KEY,
                MiscStorageInterface::ValueType::STRING_VALUE)) {
            ACSDK_ERROR(
                LX(errorEvent)
                    .m("Could not create misc table " + DCF_PUBLISH_TABLE + " for component " + COMPONENT_NAME + "."));
            return false;
        }
    }

    return true;
}

bool DCFDelegate::isCapabilityRegisteredLocked(const std::unordered_map<std::string, std::string>& capabilityMap) {
    std::string capabilityKey = getCapabilityKey(capabilityMap);
    return (m_registeredCapabilityConfigs.find(capabilityKey) != m_registeredCapabilityConfigs.end());
}

bool DCFDelegate::registerCapability(const std::shared_ptr<CapabilityConfigurationInterface>& capabilitiesProvider) {
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

DCFDelegate::DCFPublishReturnCode DCFDelegate::publishCapabilities() {
    std::lock_guard<std::mutex> lock(m_capabilityMutex);
    const std::string errorEvent = "publishCapabilitiesFailed";
    const std::string errorReasonKey = "reason";

    m_dcfPublishMessage = getDcfMessageBody();
    if (m_dcfPublishMessage.empty()) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "emptyCapabiltiesList"));
        setDCFState(DCFObserverInterface::State::FATAL_ERROR, DCFObserverInterface::Error::BAD_REQUEST);
        return DCFDelegate::DCFPublishReturnCode::FATAL_ERROR;
    }

    if (!isDCFPublishDataDifferent()) {
        setDCFState(DCFObserverInterface::State::SUCCESS, DCFObserverInterface::Error::SUCCESS);
        return DCFDelegate::DCFPublishReturnCode::SUCCESS;
    }

    std::string authToken = getAuthToken();
    if (authToken.empty()) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "getAuthTokenFailed"));
        setDCFState(DCFObserverInterface::State::FATAL_ERROR, DCFObserverInterface::Error::FORBIDDEN);
        return DCFDelegate::DCFPublishReturnCode::FATAL_ERROR;
    }

    const std::string dcfUrl = getDcfUrl(SELF_DEVICE);

    const std::vector<std::string> httpHeaderData = {
        CONTENT_TYPE_HEADER_KEY + HEADER_KEY_VALUE_SEPARATOR + CONTENT_TYPE_HEADER_VALUE,
        CONTENT_LENGTH_HEADER_KEY + HEADER_KEY_VALUE_SEPARATOR + std::to_string(m_dcfPublishMessage.size()),
        AUTHORIZATION_HEADER_KEY + HEADER_KEY_VALUE_SEPARATOR + authToken,
        ACCEPT_HEADER_KEY + HEADER_KEY_VALUE_SEPARATOR,
        EXPECT_HEADER_KEY + HEADER_KEY_VALUE_SEPARATOR};

    HTTPResponse httpResponse = m_httpPut->doPut(dcfUrl, httpHeaderData, m_dcfPublishMessage);

    switch (httpResponse.code) {
        case HTTPResponseCode::SUCCESS_NO_CONTENT:
            if (!saveDCFPublishData()) {
                ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "unableToSaveDCFData"));
                setDCFState(DCFObserverInterface::State::FATAL_ERROR, DCFObserverInterface::Error::UNKNOWN_ERROR);
                return DCFDelegate::DCFPublishReturnCode::FATAL_ERROR;
            }
            setDCFState(DCFObserverInterface::State::SUCCESS, DCFObserverInterface::Error::SUCCESS);
            return DCFDelegate::DCFPublishReturnCode::SUCCESS;
        case HTTPResponseCode::BAD_REQUEST:
            ACSDK_ERROR(
                LX(errorEvent).d(errorReasonKey, "badRequest: " + getErrorMsgFromHttpResponse(httpResponse.body)));
            setDCFState(DCFObserverInterface::State::FATAL_ERROR, DCFObserverInterface::Error::BAD_REQUEST);
            return DCFDelegate::DCFPublishReturnCode::FATAL_ERROR;
        case HTTPResponseCode::FORBIDDEN:
            ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "authenticationFailed"));
            setDCFState(DCFObserverInterface::State::FATAL_ERROR, DCFObserverInterface::Error::FORBIDDEN);
            return DCFDelegate::DCFPublishReturnCode::FATAL_ERROR;
        case HTTPResponseCode::SERVER_INTERNAL_ERROR:
            ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "internalServiceError"));
            setDCFState(
                DCFObserverInterface::State::RETRIABLE_ERROR, DCFObserverInterface::Error::SERVER_INTERNAL_ERROR);
            return DCFDelegate::DCFPublishReturnCode::RETRIABLE_ERROR;
        default:
            ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "httpRequestFailed " + httpResponse.serialize()));
            setDCFState(DCFObserverInterface::State::FATAL_ERROR, DCFObserverInterface::Error::UNKNOWN_ERROR);
            return DCFDelegate::DCFPublishReturnCode::FATAL_ERROR;
    }
}

void DCFDelegate::publishCapabilitiesAsyncWithRetries() {
    m_executor.submit([this]() {
        int retryCount = 0;
        DCFDelegateInterface::DCFPublishReturnCode dcfPublishReturnCode = publishCapabilities();

        while (DCFDelegateInterface::DCFPublishReturnCode::RETRIABLE_ERROR == dcfPublishReturnCode) {
            std::chrono::milliseconds retryBackoff =
                acl::TransportDefines::RETRY_TIMER.calculateTimeToRetry(retryCount++);
            ACSDK_ERROR(LX("dcfPublishFailed").d("reason", "serverError").d("retryCount", retryCount));

            std::unique_lock<std::mutex> lock(m_publishWaitMutex);
            m_publishWaitDone.wait_for(lock, retryBackoff, [this] { return m_isDCFDelegateShutdown; });

            if (m_isDCFDelegateShutdown) {
                dcfPublishReturnCode = DCFDelegateInterface::DCFPublishReturnCode::FATAL_ERROR;
            } else {
                dcfPublishReturnCode = publishCapabilities();
            }
        }

        if (DCFDelegateInterface::DCFPublishReturnCode::FATAL_ERROR == dcfPublishReturnCode) {
            ACSDK_ERROR(LX("dcfPublishFailed").d("reason", "unableToPublishCapabilities"));
        }
    });
}

std::string DCFDelegate::getDcfMessageBodyFromRegisteredCapabilitiesLocked() {
    if (m_registeredCapabilityConfigs.empty()) {
        return "";
    }

    rapidjson::Document dcfMessageBody(kObjectType);
    auto& allocator = dcfMessageBody.GetAllocator();

    dcfMessageBody.AddMember(StringRef(ENVELOPE_VERSION_KEY), ENVELOPE_VERSION_VALUE, allocator);

    rapidjson::Value capabilities(kArrayType);
    for (auto capabilityConfigIterator : m_registeredCapabilityConfigs) {
        auto capabilityMap = (capabilityConfigIterator.second)->capabilityConfiguration;

        /// You should not have been able to register incomplete capabilties. But, checking just in case.
        if (!isCapabilityMapCorrectlyFormed(capabilityMap)) {
            return "";
        }

        rapidjson::Value capability(kObjectType);
        auto foundValueIterator = capabilityMap.find(CAPABILITY_INTERFACE_TYPE_KEY);
        if (foundValueIterator != capabilityMap.end()) {
            capability.AddMember(StringRef(CAPABILITY_INTERFACE_TYPE_KEY), foundValueIterator->second, allocator);
        }
        foundValueIterator = capabilityMap.find(CAPABILITY_INTERFACE_NAME_KEY);
        if (foundValueIterator != capabilityMap.end()) {
            capability.AddMember(StringRef(CAPABILITY_INTERFACE_NAME_KEY), foundValueIterator->second, allocator);
        }
        foundValueIterator = capabilityMap.find(CAPABILITY_INTERFACE_VERSION_KEY);
        if (foundValueIterator != capabilityMap.end()) {
            capability.AddMember(StringRef(CAPABILITY_INTERFACE_VERSION_KEY), foundValueIterator->second, allocator);
        }
        if (capabilityMap.size() == CAPABILITY_MAP_MAXIMUM_ENTRIES) {
            foundValueIterator = capabilityMap.find(CAPABILITY_INTERFACE_CONFIGURATIONS_KEY);
            if (foundValueIterator != capabilityMap.end()) {
                capability.AddMember(
                    StringRef(CAPABILITY_INTERFACE_CONFIGURATIONS_KEY), foundValueIterator->second, allocator);
            }
        }
        capabilities.PushBack(capability, allocator);
    }
    dcfMessageBody.AddMember(StringRef(CAPABILITIES_KEY), capabilities, allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!dcfMessageBody.Accept(writer)) {
        ACSDK_ERROR(
            LX("getDcfMessageBodyFromRegisteredCapabilitiesFailed").d("reason", "writerRefusedDcfMessageBodyJson"));
        return "";
    }

    m_envelopeVersion = ENVELOPE_VERSION_VALUE;
    m_capabilityConfigs = m_registeredCapabilityConfigs;

    const char* bufferData = buffer.GetString();
    if (!bufferData) {
        ACSDK_ERROR(LX("getDcfMessageBodyFromRegisteredCapabilitiesFailed")
                        .d("reason", "nullptrDcfMessageBodyJsonBufferString"));
        return "";
    }

    return std::string(bufferData);
}

std::string getCapabilityInfoStringFromJson(const rapidjson::Value& capabilityJson, const std::string& jsonMemberKey) {
    if (!capabilityJson.HasMember(jsonMemberKey)) {
        return "";
    }
    const rapidjson::Value& capabilityTypeJson = (capabilityJson.FindMember(jsonMemberKey))->value;
    if (!capabilityTypeJson.IsString()) {
        return "";
    }
    return capabilityTypeJson.GetString();
}

std::string DCFDelegate::getDcfMessageBodyFromOverride() {
    if (m_overridenDcfPublishMessageBody.empty()) {
        return "";
    }

    const std::string errorEvent = "getDcfMessageBodyFromOverrideFailed";
    const std::string errorReasonKey = "reason";

    std::string envelopeVersion;
    std::unordered_map<std::string, std::shared_ptr<CapabilityConfiguration>> capabilityConfigs;

    rapidjson::Document overridenDcfPublishMessageJson;
    overridenDcfPublishMessageJson.Parse(m_overridenDcfPublishMessageBody);

    if (overridenDcfPublishMessageJson.HasParseError()) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "Error parsing the message"));
        return "";
    }

    unsigned int numOfMembersInJson = 2;
    if (overridenDcfPublishMessageJson.MemberCount() != numOfMembersInJson) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "The message has incorrect number of components"));
        return "";
    }

    if ((!overridenDcfPublishMessageJson.HasMember(ENVELOPE_VERSION_KEY)) ||
        (!overridenDcfPublishMessageJson.HasMember(CAPABILITIES_KEY))) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "The message does not have the required components"));
        return "";
    }

    const rapidjson::Value& envelopeVersionJson =
        (overridenDcfPublishMessageJson.FindMember(ENVELOPE_VERSION_KEY))->value;
    if (!envelopeVersionJson.IsString()) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "Envelope version value is not a string"));
        return "";
    }
    envelopeVersion = envelopeVersionJson.GetString();

    const rapidjson::Value& capabiltiesJson = (overridenDcfPublishMessageJson.FindMember(CAPABILITIES_KEY))->value;
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
            if (!capabilityJson.HasMember(CAPABILITY_INTERFACE_CONFIGURATIONS_KEY)) {
                ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "Capability has an unexpected component"));
                return "";
            }
            const rapidjson::Value& capabilityConfigJson =
                (capabilityJson.FindMember(CAPABILITY_INTERFACE_CONFIGURATIONS_KEY))->value;
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            if (!capabilityConfigJson.Accept(writer)) {
                ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "Error in processing capability configuration"));
                return "";
            }
            const char* bufferData = buffer.GetString();
            if (!bufferData) {
                ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "Error in processing capability configuration"));
                return "";
            }
            capabilityMap.insert({CAPABILITY_INTERFACE_CONFIGURATIONS_KEY, std::string(bufferData)});
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

    return m_overridenDcfPublishMessageBody;
}

std::string DCFDelegate::getDcfMessageBody() {
    std::string dcfMessageBody = getDcfMessageBodyFromOverride();

    if (dcfMessageBody.empty()) {
        dcfMessageBody = getDcfMessageBodyFromRegisteredCapabilitiesLocked();
    }

    return dcfMessageBody;
}

std::string DCFDelegate::getDcfUrl(const std::string& deviceId) {
    return m_dcfEndpoint + DCF_URL_PRE_DEVICE_SUFFIX + deviceId + DCF_URL_POST_DEVICE_SUFFIX;
}

void DCFDelegate::addDCFObserver(std::shared_ptr<DCFObserverInterface> observer) {
    ACSDK_DEBUG5(LX("addDCFObserver").d("observer", observer.get()));

    std::lock_guard<std::mutex> lock(m_dcfMutex);
    if (!observer) {
        ACSDK_ERROR(LX("addDCFObserverFailed").d("reason", "nullObserver"));
        return;
    }
    if (!m_dcfObservers.insert(observer).second) {
        ACSDK_WARN(LX("addDCFObserverFailed").d("reason", "observerAlreadyAdded"));
        return;
    }
    observer->onDCFStateChange(m_dcfState, m_dcfError);
}

void DCFDelegate::removeDCFObserver(std::shared_ptr<DCFObserverInterface> observer) {
    ACSDK_DEBUG5(LX("removeDCFObserver").d("observer", observer.get()));

    std::lock_guard<std::mutex> lock(m_dcfMutex);
    if (!observer) {
        ACSDK_ERROR(LX("removeDCFObserverFailed").d("reason", "nullObserver"));
    } else if (m_dcfObservers.erase(observer) == 0) {
        ACSDK_WARN(LX("removeDCFObserverFailed").d("reason", "observerNotAdded"));
    }
}

void DCFDelegate::setDCFState(DCFObserverInterface::State newDCFState, DCFObserverInterface::Error newDCFError) {
    ACSDK_DEBUG5(LX("setDCFState").d("newDCFState", newDCFState));

    std::lock_guard<std::mutex> lock(m_dcfMutex);
    if ((newDCFState == m_dcfState) && (newDCFError == m_dcfError)) {
        return;
    }
    m_dcfState = newDCFState;
    m_dcfError = newDCFError;

    if (!m_dcfObservers.empty()) {
        ACSDK_DEBUG9(LX("callingOnDCFStateChange").d("state", m_dcfState).d("error", m_dcfError));
        for (auto& observer : m_dcfObservers) {
            observer->onDCFStateChange(m_dcfState, m_dcfError);
        }
    }
}

void DCFDelegate::onAuthStateChange(AuthObserverInterface::State newState, AuthObserverInterface::Error error) {
    std::lock_guard<std::mutex> lock(m_authStatusMutex);
    m_currentAuthState = newState;
    if ((AuthObserverInterface::State::REFRESHED == m_currentAuthState) ||
        (AuthObserverInterface::State::UNRECOVERABLE_ERROR == m_currentAuthState)) {
        m_authStatusReady.notify_one();
    }
}

std::string DCFDelegate::getAuthToken() {
    {
        std::unique_lock<std::mutex> lock(m_authStatusMutex);
        m_authStatusReady.wait(lock, [this]() {
            return (
                (AuthObserverInterface::State::REFRESHED == m_currentAuthState) ||
                (AuthObserverInterface::State::UNRECOVERABLE_ERROR == m_currentAuthState));
        });
    }

    if (AuthObserverInterface::State::UNRECOVERABLE_ERROR == m_currentAuthState) {
        ACSDK_ERROR(LX("getAuthTokenFailed").d("reason", "Unrecoverable error by auth delegate"));
        return "";
    }

    return m_authDelegate->getAuthToken();
}

void DCFDelegate::getPreviouslySentDCFPublishData() {
    const std::string dbKeysPrefix = DB_KEY_ENDPOINT + m_dcfEndpoint + DB_KEY_SEPARATOR;

    std::string previousClientId;
    std::string previousProductId;
    std::string previousDeviceSerialNumber;
    m_miscStorage->get(COMPONENT_NAME, DCF_PUBLISH_TABLE, dbKeysPrefix + DB_KEY_CLIENT_ID, &previousClientId);
    m_miscStorage->get(COMPONENT_NAME, DCF_PUBLISH_TABLE, dbKeysPrefix + DB_KEY_PRODUCT_ID, &previousProductId);
    m_miscStorage->get(COMPONENT_NAME, DCF_PUBLISH_TABLE, dbKeysPrefix + DB_KEY_DSN, &previousDeviceSerialNumber);
    m_previousDeviceInfo = DeviceInfo::create(previousClientId, previousProductId, previousDeviceSerialNumber);

    m_previousEnvelopeVersion = "";
    m_miscStorage->get(
        COMPONENT_NAME, DCF_PUBLISH_TABLE, dbKeysPrefix + DB_KEY_ENVELOPE_VERSION, &m_previousEnvelopeVersion);

    std::string previousDCFPublishMsgStr;
    m_miscStorage->get(COMPONENT_NAME, DCF_PUBLISH_TABLE, dbKeysPrefix + DB_KEY_PUBLISH_MSG, &previousDCFPublishMsgStr);
    if (!previousDCFPublishMsgStr.empty()) {
        rapidjson::Document previousDCFPublishMsgJson;
        previousDCFPublishMsgJson.Parse(previousDCFPublishMsgStr);

        for (rapidjson::SizeType capabilityIndx = 0;
             capabilityIndx < previousDCFPublishMsgJson[CAPABILITIES_KEY].Size();
             capabilityIndx++) {
            const rapidjson::Value& capabilityJson = previousDCFPublishMsgJson[CAPABILITIES_KEY][capabilityIndx];
            std::unordered_map<std::string, std::string> capabilityMap;

            std::string capabilityType = (capabilityJson.FindMember(CAPABILITY_INTERFACE_TYPE_KEY))->value.GetString();
            capabilityMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, capabilityType});

            std::string capabilityName = (capabilityJson.FindMember(CAPABILITY_INTERFACE_NAME_KEY))->value.GetString();
            capabilityMap.insert({CAPABILITY_INTERFACE_NAME_KEY, capabilityName});

            std::string capabilityVersion =
                (capabilityJson.FindMember(CAPABILITY_INTERFACE_VERSION_KEY))->value.GetString();
            capabilityMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, capabilityVersion});

            if (capabilityJson.HasMember(CAPABILITY_INTERFACE_CONFIGURATIONS_KEY)) {
                std::string capabilityConfigs =
                    (capabilityJson.FindMember(CAPABILITY_INTERFACE_CONFIGURATIONS_KEY))->value.GetString();
                capabilityMap.insert({CAPABILITY_INTERFACE_CONFIGURATIONS_KEY, capabilityConfigs});
            }

            std::string capabilityKey = getCapabilityKey(capabilityMap);
            m_previousCapabilityConfigs.insert(
                {capabilityKey, std::make_shared<CapabilityConfiguration>(capabilityMap)});
        }
    }
}

bool DCFDelegate::isDCFPublishDataDifferent() {
    getPreviouslySentDCFPublishData();

    if ((!m_previousDeviceInfo) || (*m_previousDeviceInfo != *m_deviceInfo)) {
        return true;
    }

    if (m_previousEnvelopeVersion != m_envelopeVersion) {
        return true;
    }

    return isDCFPublishMessageDifferent();
}

bool DCFDelegate::isDCFPublishMessageDifferent() {
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

void logFailedSaveAndClearDCFPublishTable(const std::shared_ptr<MiscStorageInterface>& miscStorage) {
    ACSDK_ERROR(
        LX("saveDCFPublishDataFailed")
            .d("reason", "Unable to update the table " + DCF_PUBLISH_TABLE + " for component " + COMPONENT_NAME + "."));
    ACSDK_INFO(LX("saveDCFPublishDataInfo")
                   .m("Clearing table " + DCF_PUBLISH_TABLE + " for component " + COMPONENT_NAME + "."));
    if (!miscStorage->clearTable(COMPONENT_NAME, DCF_PUBLISH_TABLE)) {
        ACSDK_ERROR(LX("saveDCFPublishDataFailed")
                        .d("reason",
                           "Unable to clear the table " + DCF_PUBLISH_TABLE + " for component " + COMPONENT_NAME +
                               ". Please clear the table for proper future functioning."));
    }
}

bool DCFDelegate::saveDCFPublishData() {
    const std::string dbKeysPrefix = DB_KEY_ENDPOINT + m_dcfEndpoint + DB_KEY_SEPARATOR;

    if ((!m_previousDeviceInfo) || (m_previousDeviceInfo->getClientId() != m_deviceInfo->getClientId())) {
        if (!m_miscStorage->put(
                COMPONENT_NAME, DCF_PUBLISH_TABLE, dbKeysPrefix + DB_KEY_CLIENT_ID, m_deviceInfo->getClientId())) {
            logFailedSaveAndClearDCFPublishTable(m_miscStorage);
            return false;
        }
    }

    if ((!m_previousDeviceInfo) || (m_previousDeviceInfo->getProductId() != m_deviceInfo->getProductId())) {
        if (!m_miscStorage->put(
                COMPONENT_NAME, DCF_PUBLISH_TABLE, dbKeysPrefix + DB_KEY_PRODUCT_ID, m_deviceInfo->getProductId())) {
            logFailedSaveAndClearDCFPublishTable(m_miscStorage);
            return false;
        }
    }

    if ((!m_previousDeviceInfo) ||
        (m_previousDeviceInfo->getDeviceSerialNumber() != m_deviceInfo->getDeviceSerialNumber())) {
        if (!m_miscStorage->put(
                COMPONENT_NAME, DCF_PUBLISH_TABLE, dbKeysPrefix + DB_KEY_DSN, m_deviceInfo->getDeviceSerialNumber())) {
            logFailedSaveAndClearDCFPublishTable(m_miscStorage);
            return false;
        }
    }

    if ((m_previousEnvelopeVersion.empty()) || (m_previousEnvelopeVersion != m_envelopeVersion)) {
        if (!m_miscStorage->put(
                COMPONENT_NAME, DCF_PUBLISH_TABLE, dbKeysPrefix + DB_KEY_ENVELOPE_VERSION, m_envelopeVersion)) {
            logFailedSaveAndClearDCFPublishTable(m_miscStorage);
            return false;
        }
    }

    if (m_previousCapabilityConfigs.empty() || isDCFPublishMessageDifferent()) {
        if (!m_miscStorage->put(
                COMPONENT_NAME, DCF_PUBLISH_TABLE, dbKeysPrefix + DB_KEY_PUBLISH_MSG, m_dcfPublishMessage)) {
            logFailedSaveAndClearDCFPublishTable(m_miscStorage);
            return false;
        }
    }

    return true;
}

}  // namespace dcfDelegate
}  // namespace alexaClientSDK
