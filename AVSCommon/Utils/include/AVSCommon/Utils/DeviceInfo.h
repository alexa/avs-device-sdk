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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_DEVICEINFO_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_DEVICEINFO_H_

#include <string>

#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

/**
 * Class that defines a device's identification information.
 */
class DeviceInfo {
public:
    /**
     * Create a DeviceInfo.
     *
     * @param configurationRoot The @c ConfiguratioNode to read device info from.
     * @return If successful, returns a new DeviceInfo, otherwise @c nullptr.
     */
    static std::shared_ptr<DeviceInfo> createFromConfiguration(
        const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationRoot);

    /**
     * Create a DeviceInfo.
     *
     * @deprecated
     * @param configurationRoot The global config object.
     * @return If successful, returns a new DeviceInfo, otherwise @c nullptr.
     */
    static std::unique_ptr<DeviceInfo> create(
        const avsCommon::utils::configuration::ConfigurationNode& configurationRoot);

    /**
     * Create a DeviceInfo.
     *
     * @param clientId Client Id.
     * @param productId Product Id.
     * @param deviceSerialNumber DSN.
     * @param manufacturerName The manufacturer name.
     * @param description The description of the device.
     * @param friendlyName AVS friendly name for device
     * @param deviceType The device type.
     * @param registrationKey The Registration key value.
     * @param productIdKey The Product Id Key value.
     * @return If successful, returns a new DeviceInfo, otherwise @c nullptr.
     */
    static std::unique_ptr<DeviceInfo> create(
        const std::string& clientId,
        const std::string& productId,
        const std::string& deviceSerialNumber,
        const std::string& manufacturerName,
        const std::string& description,
        const std::string& friendlyName = "",
        const std::string& deviceType = "",
        const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId = "",
        const std::string& registrationKey = "",
        const std::string& productIdKey = "");

    /**
     * Gets the client Id.
     *
     * @return Client Id.
     */
    std::string getClientId() const;

    /**
     * Gets the product Id.
     *
     * @return Product Id.
     */
    std::string getProductId() const;

    /**
     * Gets the device serial number.
     *
     * @return Device serial number.
     */
    std::string getDeviceSerialNumber() const;

    /**
     * Gets the device manufacturer name.
     *
     * @return Device manufacturer name.
     */
    std::string getManufacturerName() const;

    /**
     * Gets a description of the device.
     *
     * @return The device description.
     */
    std::string getDeviceDescription() const;

    /**
     * Gets the device's AVS friendly name.
     *
     * @return AVS friendly name.
     */
    std::string getFriendlyName() const;

    /**
     * Gets the deviceType.
     *
     * @return device Type.
     */
    std::string getDeviceType() const;

    /**
     * Gets the device default endpoint id.
     *
     * @return A string representing the default endpoint identifier.
     */
    sdkInterfaces::endpoints::EndpointIdentifier getDefaultEndpointId() const;

    /**
     * Gets the device's registration key value.
     *
     * @return registration key.
     */
    std::string getRegistrationKey() const;

    /**
     * Gets the product id key value.
     *
     * @return product id key.
     */
    std::string getProductIdKey() const;

    /**
     * Operator == for @c DeviceInfo
     *
     * @param rhs The right hand side of the == operation.
     * @return Whether or not this instance and @c rhs are equivalent.
     */
    bool operator==(const DeviceInfo& rhs);

    /**
     * Operator != for @c DeviceInfo
     *
     * @param rhs The right hand side of the != operation.
     * @return Whether or not this instance and @c rhs are not equivalent.
     */
    bool operator!=(const DeviceInfo& rhs);

private:
    /**
     * DeviceInfo constructor.
     *
     * @param clientId Client Id.
     * @param productId Product Id.
     * @param deviceSerialNumber DSN.
     * @param manufacturerName The manufacturer name.
     * @param description The description of the device.
     * @param endpointId The device endpoint id.
     * @param registrationKey The Registration key value.
     * @param productIdKey The Product Id Key value.
     */
    DeviceInfo(
        const std::string& clientId,
        const std::string& productId,
        const std::string& deviceSerialNumber,
        const std::string& manufacturerName,
        const std::string& description,
        const std::string& friendlyName,
        const std::string& deviceType,
        const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
        const std::string& registrationKey,
        const std::string& productIdKey);

    /// Client ID
    std::string m_clientId;

    /// Product ID
    std::string m_productId;

    /// DSN
    std::string m_deviceSerialNumber;

    /// The device manufacturer.
    std::string m_manufacturer;

    /// The device description.
    std::string m_deviceDescription;

    /// AVS Friendly name for the device
    std::string m_friendlyName;

    /// Device Type
    std::string m_deviceType;

    /// Device default endpoint id.
    sdkInterfaces::endpoints::EndpointIdentifier m_defaultEndpointId;

    /// Registration key.
    std::string m_registrationKey;

    /// Product ID key.
    std::string m_productIdKey;
};

inline std::string DeviceInfo::getManufacturerName() const {
    return m_manufacturer;
}

inline std::string DeviceInfo::getDeviceDescription() const {
    return m_deviceDescription;
}

inline sdkInterfaces::endpoints::EndpointIdentifier DeviceInfo::getDefaultEndpointId() const {
    return m_defaultEndpointId;
}

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_DEVICEINFO_H_
