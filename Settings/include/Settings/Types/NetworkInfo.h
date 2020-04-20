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
#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_TYPES_NETWORKINFO_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_TYPES_NETWORKINFO_H_

#include <istream>
#include <ostream>
#include <string>

#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace settings {
namespace types {

/**
 * Class that represents a set of network information.
 */
class NetworkInfo {
public:
    /**
     * Device network connection types.
     */
    enum class ConnectionType {
        /// Represents a wired connection.
        ETHERNET,
        /// Represents a wireless connection.
        WIFI
    };

    template <typename T>
    using Optional = avsCommon::utils::Optional<T>;

    /**
     * Get the network connection type.
     *
     * @return The network connection type if available, an empty @c Optional otherwise.
     */
    Optional<ConnectionType> getConnectionType() const;

    /**
     * Get the name of the network.
     *
     * @return The name of the network if available, an empty @c Optional otherwise.
     */
    Optional<std::string> getEssid() const;

    /**
     * Get the physical name of the access point.
     *
     * @return The physical name of the access point if available, an empty @c Optional otherwise.
     */
    Optional<std::string> getBssid() const;

    /**
     * Get the IP address value of the device in this network.
     *
     * @return The IP address value of the device in this network if available, an empty @c Optional otherwise.
     */
    Optional<std::string> getIpAddress() const;

    /**
     * Get the network subnet mask.
     *
     * @return The network subnet mask if available, an empty @c Optional otherwise.
     */
    Optional<std::string> getSubnetMask() const;

    /**
     * Get the device network physical address(MAC).
     *
     * @return The device network physical address(MAC) if available, an empty @c Optional otherwise.
     */
    Optional<std::string> getMacAddress() const;

    /**
     * Get the DHCP server address.
     *
     * @return The DHCP server address if available, an empty @c Optional otherwise.
     */
    Optional<std::string> getDhcpServerAddress() const;

    /**
     * Get whether this network uses static IP.
     *
     * @return Whether this network uses static IP if information is available, an empty @c Optional otherwise.
     */
    Optional<bool> getIsStaticIP() const;

    /**
     * Set the network connection type.
     *
     * @param connectionType The network connection type the device is connected to.
     */
    void setConnectionType(const ConnectionType& connectionType);

    /**
     * Set the name of the network.
     *
     * @param essid The name of the network.
     */
    void setEssid(const std::string& essid);

    /**
     * Set the physical name of the access point.
     *
     * @param bssid The physical name of the access point.
     * @note The string should be formatted as a set of octets (in hexadecimal format) separated by ':'.
     * @return @c true if the bssid has been set; @c false otherwise.
     */
    bool setBssid(const std::string& bssid);

    /**
     * Set the IP address value.
     *
     * @param ipAddress The IP address. The value can be either IPv6 or IPv4.
     * @return @c true if the ip address has been set; @c false otherwise.
     */
    bool setIpAddress(const std::string& ipAddress);

    /**
     * Set the network subnet mask.
     *
     * @param subnetMask The network subnet mask using IPv6 or IPv4 mask format.
     * @return @c true if the subnet mask has been set; @c false otherwise.
     */
    bool setSubnetMask(const std::string& subnetMask);

    /**
     * Set the physical address(MAC) of the device.
     *
     * @param macAddress The device MAC address.
     * @note The string should be formatted as a set of octets (in hexadecimal format) separated by ':'.
     * @return @c true if the MAC address has been set; @c false otherwise.
     */
    bool setMacAddress(const std::string& macAddress);

    /**
     * Set the DHCP server address known to the device.
     *
     * @param dhcpServerAddress The address of the DHCP server using IPv6 or IPv4 mask format.
     * @return @c true if the HHCP serer address has been set; @c false otherwise.
     */
    bool setDhcpServerAddress(const std::string& dhcpServerAddress);

    /**
     * Set whether the IP address is static or not.
     *
     * @param isStaticIP Whether the network uses static IP or not.
     */
    void setIsStaticIP(bool isStaticIP);

    /**
     * Reset the network connection type information.
     */
    void resetConnectionType();

    /**
     * Reset the name of the network.
     */
    void resetEssid();

    /**
     * Reset the physical name of the access point.
     */
    void resetBssid();

    /**
     * Reset the IP address value.
     */
    void resetIpAddress();

    /**
     * Reset the network subnet mask.
     */
    void resetSubnetMask();

    /**
     * Reset the physical address(MAC) of the device.
     */
    void resetMacAddress();

    /**
     * Reset the DHCP server address known to the device.
     */
    void resetDhcpServerAddress();

    /**
     * Reset whether the IP address is static or not.
     */
    void resetIsStaticIP();

    /**
     * Equality operator.
     */
    bool operator==(const NetworkInfo& rhs) const;
    bool operator!=(const NetworkInfo& rhs) const;

private:
    /**
     * Check if the given @c input string is a valid MAC address (EUI-48 or EUI-64).
     *
     * @param input The input string that should be formatted as a MAC address.
     * @return @c true if @c input has the expected format; false otherwise.
     */
    bool validMacAddress(const std::string& input);

    /**
     * Check if the given @c input string is a valid IP address (IPv4 or IPv6).
     *
     * @param input The input string that should be formatted as an IP address.
     * @return @c true if @c input has the expected format; false otherwise.
     */
    bool validIpAddress(const std::string& input);

    /**
     * Check if the given @c input string is a valid subnet mask range (IPv4 or IPv6).
     *
     * @param input The input string that should be formatted as a subnet mask.
     * @return @c true if @c input has the expected format; false otherwise.
     */
    bool validSubnetMask(const std::string& input);

    /// Current network connection type.
    Optional<ConnectionType> m_connectionType;

    /// Name of the network the device is connected to.
    Optional<std::string> m_essid;

    /// Physical name of the access point.
    Optional<std::string> m_bssid;

    /// IP address value of the device.
    Optional<std::string> m_ipAddress;

    /// Subnet mask used by the device.
    Optional<std::string> m_subnetMask;

    /// Physical address(MAC) of the device
    Optional<std::string> m_macAddress;

    /// DHCP server address known to the device.
    Optional<std::string> m_dhcpServerAddress;

    /// Device IP address is static or not.
    Optional<bool> m_isStaticIP;

    /// Friend output function.
    friend std::ostream& operator<<(std::ostream& os, const NetworkInfo& info);
};

/**
 * Write a @c NetworkInfo value to the given stream in json format.
 *
 * @param stream The stream to write the value to.
 * @param info The network info to write to the stream as a string.
 * @return The stream that was passed in and written to.
 */
std::ostream& operator<<(std::ostream& os, const NetworkInfo& info);

/**
 * Converts an input string stream value formatted as json document to NetworkInfo.
 *
 * @param stream The string stream to retrieve the value from.
 * @param [out] info The network info to write to.
 * @return The stream that was passed in.
 */
std::istream& operator>>(std::istream& is, NetworkInfo& info);

}  // namespace types
}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_TYPES_NETWORKINFO_H_
