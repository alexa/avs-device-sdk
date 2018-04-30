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

#include "AVSCommon/Utils/DeviceInfo.h"

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

/// String to identify log entries originating from this file.
static const std::string TAG("DeviceInfo");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Name of @c ConfigurationNode for DeviceInfo
const std::string CONFIG_KEY_DEVICE_INFO = "deviceInfo";
/// Name of clientId value in DeviceInfo's @c ConfigurationNode.
const std::string CONFIG_KEY_CLIENT_ID = "clientId";
/// Name of productId value in DeviceInfo's @c ConfigurationNode.
const std::string CONFIG_KEY_PRODUCT_ID = "productId";
/// Name of deviceSerialNumber value in DeviceInfo's @c ConfigurationNode.
const std::string CONFIG_KEY_DSN = "deviceSerialNumber";

std::unique_ptr<DeviceInfo> DeviceInfo::create(
    const avsCommon::utils::configuration::ConfigurationNode& configurationRoot) {
    std::string clientId;
    std::string productId;
    std::string deviceSerialNumber;
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

    return create(clientId, productId, deviceSerialNumber);
}

std::unique_ptr<DeviceInfo> DeviceInfo::create(
    const std::string& clientId,
    const std::string& productId,
    const std::string& deviceSerialNumber) {
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

    std::unique_ptr<DeviceInfo> instance(new DeviceInfo(clientId, productId, deviceSerialNumber));

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

bool DeviceInfo::operator==(const DeviceInfo& rhs) {
    if (getClientId() != rhs.getClientId()) {
        return false;
    }
    if (getProductId() != rhs.getProductId()) {
        return false;
    }
    if (getDeviceSerialNumber() != rhs.getDeviceSerialNumber()) {
        return false;
    }

    return true;
}

bool DeviceInfo::operator!=(const DeviceInfo& rhs) {
    return !(*this == rhs);
}

DeviceInfo::DeviceInfo(
    const std::string& clientId,
    const std::string& productId,
    const std::string& deviceSerialNumber) :
        m_clientId{clientId},
        m_productId{productId},
        m_deviceSerialNumber{deviceSerialNumber} {
}

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
