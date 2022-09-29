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

#include "AVSCommon/Utils/DeviceInfo.h"

#include "AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h"
#include "AVSCommon/Utils/Logger/Logger.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

/// String to identify log entries originating from this file.
#define TAG "DeviceInfo"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

using namespace avsCommon::sdkInterfaces::endpoints;

/// Name of @c ConfigurationNode for DeviceInfo
const std::string CONFIG_KEY_DEVICE_INFO = "deviceInfo";
/// Name of clientId value in DeviceInfo's @c ConfigurationNode.
const std::string CONFIG_KEY_CLIENT_ID = "clientId";
/// Name of registration key value in DeviceInfo's @c ConfigurationNode.
const static std::string CONFIG_KEY_REGISTRATION_KEY = "registrationKey";
/// Name of productId key value in DeviceInfo's @c ConfigurationNode.
const std::string CONFIG_KEY_PRODUCT_ID_KEY = "productIdKey";
/// Name of registration value in DeviceInfo's @c ConfigurationNode.
const static std::string CONFIG_KEY_REGISTRATION = "registration";
/// Name of productId value in DeviceInfo's @c ConfigurationNode.
const std::string CONFIG_KEY_PRODUCT_ID = "productId";
/// Name of deviceSerialNumber value in DeviceInfo's @c ConfigurationNode.
const std::string CONFIG_KEY_DSN = "deviceSerialNumber";
/// DeviceType in DeviceInfo's @c ConfigurationNode.
const std::string CONFIG_KEY_DEVICE_TYPE = "deviceType";
/// Name of friendlyName value in DeviceInfo's @c ConfigurationNode.
const std::string CONFIG_KEY_FRIENDLY_NAME = "friendlyName";
/// Key for the device manufacturer name in DeviceInfo's @c ConfigurationNode.
const std::string CONFIG_KEY_MANUFACTURER_NAME = "manufacturerName";
/// Key for the device description in DeviceInfo's @c ConfigurationNode.
const std::string CONFIG_KEY_DESCRIPTION = "description";
/// String used to join attributes in the generation of the default endpoint id.
const std::string ENDPOINT_ID_CONCAT = "::";

/**
 * Generate an endpoint identifier to the device per public documentation:
 * @see https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/alexa-discovery.html
 *
 * @param clientId The client ID of the device maintaining the HTTP/2 connection, generated during product registration.
 * @param productId  The product ID of the device maintaining the HTTP/2 connection.
 * @param deviceSerialNumber The serial number of the device maintaining the HTTP/2 connection.
 * @return The default endpoint identifier for this client.
 */
inline EndpointIdentifier generateEndpointId(
    const std::string& clientId,
    const std::string& productId,
    const std::string& deviceSerialNumber) {
    return clientId + ENDPOINT_ID_CONCAT + productId + ENDPOINT_ID_CONCAT + deviceSerialNumber;
}

std::shared_ptr<DeviceInfo> DeviceInfo::createFromConfiguration(
    const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationRoot) {
    if (!configurationRoot) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullConfigurationRoot"));
        return nullptr;
    }
    return create(*configurationRoot);
}

std::unique_ptr<DeviceInfo> DeviceInfo::create(
    const avsCommon::utils::configuration::ConfigurationNode& configurationRoot) {
    std::string clientId;
    std::string productId;
    std::string deviceSerialNumber;
    std::string manufacturerName;
    std::string description;
    std::string friendlyName;
    std::string deviceType;
    std::string registrationKey;
    std::string productIdKey;

    const std::string errorEvent = "createFailed";
    const std::string errorReasonKey = "reason";
    const std::string errorValueKey = "key";

    auto deviceInfoConfiguration = configurationRoot[CONFIG_KEY_DEVICE_INFO];
    if (!deviceInfoConfiguration) {
        ACSDK_ERROR(LX(errorEvent)
                        .d(errorReasonKey, "missingDeviceInfoConfiguration")
                        .d(errorValueKey, CONFIG_KEY_DEVICE_INFO));
        return nullptr;
    }

    if (!deviceInfoConfiguration.getString(CONFIG_KEY_CLIENT_ID, &clientId)) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "missingClientId").d(errorValueKey, CONFIG_KEY_CLIENT_ID));
        return nullptr;
    }

    if (!deviceInfoConfiguration.getString(CONFIG_KEY_PRODUCT_ID, &productId)) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "missingProductId").d(errorValueKey, CONFIG_KEY_PRODUCT_ID));
        return nullptr;
    }

    if (!deviceInfoConfiguration.getString(CONFIG_KEY_DSN, &deviceSerialNumber)) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "missingDeviceSerialNumber").d(errorValueKey, CONFIG_KEY_DSN));
        return nullptr;
    }

    if (!deviceInfoConfiguration.getString(CONFIG_KEY_DEVICE_TYPE, &deviceType)) {
        ACSDK_INFO(LX(__func__).d("result", "skipDeviceType").d(errorValueKey, CONFIG_KEY_DEVICE_TYPE));
    }

    if (!deviceInfoConfiguration.getString(CONFIG_KEY_FRIENDLY_NAME, &friendlyName)) {
        ACSDK_INFO(LX(__func__).d("result", "skipFriendlyName").d(errorValueKey, CONFIG_KEY_FRIENDLY_NAME));
    }

    if (!deviceInfoConfiguration.getString(CONFIG_KEY_MANUFACTURER_NAME, &manufacturerName)) {
        ACSDK_ERROR(
            LX(errorEvent).d(errorReasonKey, "missingManufacturerName").d(errorValueKey, CONFIG_KEY_MANUFACTURER_NAME));
        return nullptr;
    }

    if (!deviceInfoConfiguration.getString(CONFIG_KEY_DESCRIPTION, &description)) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "missingDescription").d(errorValueKey, CONFIG_KEY_DESCRIPTION));
        return nullptr;
    }

    if (!deviceInfoConfiguration.getString(CONFIG_KEY_REGISTRATION_KEY, &registrationKey)) {
        ACSDK_INFO(LX(__func__).d("result", "skipRegistrationKey").d(errorValueKey, CONFIG_KEY_REGISTRATION_KEY));
        registrationKey = CONFIG_KEY_REGISTRATION;
    }

    if (!deviceInfoConfiguration.getString(CONFIG_KEY_PRODUCT_ID_KEY, &productIdKey)) {
        ACSDK_INFO(LX(__func__).d("result", "skipProductIdKey").d(errorValueKey, CONFIG_KEY_PRODUCT_ID_KEY));
        productIdKey = CONFIG_KEY_PRODUCT_ID;
    }

    return create(
        clientId,
        productId,
        deviceSerialNumber,
        manufacturerName,
        description,
        friendlyName,
        deviceType,
        "",
        registrationKey,
        productIdKey);
}

std::unique_ptr<DeviceInfo> DeviceInfo::create(
    const std::string& clientId,
    const std::string& productId,
    const std::string& deviceSerialNumber,
    const std::string& manufacturerName,
    const std::string& description,
    const std::string& friendlyName,
    const std::string& deviceType,
    const EndpointIdentifier& endpointId,
    const std::string& registrationKey,
    const std::string& productIdKey) {
    const std::string errorEvent = "createFailed";
    const std::string errorReasonKey = "reason";
    const std::string errorValueKey = "key";

    if (clientId.empty()) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "missingClientId").d(errorValueKey, CONFIG_KEY_CLIENT_ID));
        return nullptr;
    }

    if (productId.empty()) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "missingProductId").d(errorValueKey, CONFIG_KEY_PRODUCT_ID));
        return nullptr;
    }

    if (deviceSerialNumber.empty()) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "missingDeviceSerialNumber").d(errorValueKey, CONFIG_KEY_DSN));
        return nullptr;
    }

    if (manufacturerName.empty()) {
        ACSDK_ERROR(
            LX(errorEvent).d(errorReasonKey, "missingManufacturerName").d(errorValueKey, CONFIG_KEY_MANUFACTURER_NAME));
        return nullptr;
    }

    if (description.empty()) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "missingDescription").d(errorValueKey, CONFIG_KEY_DESCRIPTION));
        return nullptr;
    }

    auto defaultEndpointId =
        endpointId.empty() ? generateEndpointId(clientId, productId, deviceSerialNumber) : endpointId;

    auto defaultRegistrationKey = registrationKey.empty() ? CONFIG_KEY_REGISTRATION : registrationKey;

    auto defaultProductIdKey = productIdKey.empty() ? CONFIG_KEY_PRODUCT_ID : productIdKey;

    std::unique_ptr<DeviceInfo> instance(new DeviceInfo(
        clientId,
        productId,
        deviceSerialNumber,
        manufacturerName,
        description,
        friendlyName,
        deviceType,
        defaultEndpointId,
        defaultRegistrationKey,
        defaultProductIdKey));

    return instance;
}

std::string DeviceInfo::getClientId() const {
    return m_clientId;
}

std::string DeviceInfo::getProductId() const {
    return m_productId;
}

std::string DeviceInfo::getDeviceSerialNumber() const {
    return m_deviceSerialNumber;
}

std::string DeviceInfo::getFriendlyName() const {
    return m_friendlyName;
}

std::string DeviceInfo::getDeviceType() const {
    return m_deviceType;
}

std::string DeviceInfo::getRegistrationKey() const {
    return m_registrationKey;
}

std::string DeviceInfo::getProductIdKey() const {
    return m_productIdKey;
}

bool DeviceInfo::operator==(const DeviceInfo& rhs) {
    return std::tie(
               m_clientId,
               m_productId,
               m_deviceSerialNumber,
               m_manufacturer,
               m_deviceDescription,
               m_friendlyName,
               m_deviceType,
               m_defaultEndpointId) ==
           std::tie(
               rhs.m_clientId,
               rhs.m_productId,
               rhs.m_deviceSerialNumber,
               rhs.m_manufacturer,
               rhs.m_deviceDescription,
               rhs.m_friendlyName,
               rhs.m_deviceType,
               rhs.m_defaultEndpointId);
}

bool DeviceInfo::operator!=(const DeviceInfo& rhs) {
    return !(*this == rhs);
}

DeviceInfo::DeviceInfo(
    const std::string& clientId,
    const std::string& productId,
    const std::string& deviceSerialNumber,
    const std::string& manufacturerName,
    const std::string& description,
    const std::string& friendlyName,
    const std::string& deviceType,
    const EndpointIdentifier& endpointId,
    const std::string& registrationKey,
    const std::string& productIdKey) :
        m_clientId{clientId},
        m_productId{productId},
        m_deviceSerialNumber{deviceSerialNumber},
        m_manufacturer{manufacturerName},
        m_deviceDescription{description},
        m_friendlyName{friendlyName},
        m_deviceType{deviceType},
        m_defaultEndpointId{endpointId},
        m_registrationKey{registrationKey},
        m_productIdKey{productIdKey} {
}

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
