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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_DEVICEINFO_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_DEVICEINFO_H_

#include <string>

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
     * @return If successful, returns a new DeviceInfo, otherwise @c nullptr.
     */
    static std::unique_ptr<DeviceInfo> create(
        const std::string& clientId,
        const std::string& productId,
        const std::string& deviceSerialNumber);

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
     */
    DeviceInfo(const std::string& clientId, const std::string& productId, const std::string& deviceSerialNumber);

    /// Client ID
    std::string m_clientId;

    /// Product ID
    std::string m_productId;

    /// DSN
    std::string m_deviceSerialNumber;
};

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_DEVICEINFO_H_
