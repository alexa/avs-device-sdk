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

#include <array>
#include <cstdio>
#include <functional>
#include <string>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/istreamwrapper.h>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/String/StringUtils.h>

#include "Settings/Types/NetworkInfo.h"

namespace alexaClientSDK {
namespace settings {
namespace types {

/// String to identify log entries originating from this file.
#define TAG "NetworkInfo"

/**
 * Create an error LogEntry using this file's TAG and use "functionNameFailed" as the event string.
 */
#define LX_FAILED() alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, std::string(__func__) + "Failed")

using namespace avsCommon::utils;
using namespace avsCommon::utils::json;

/// Json payload key for connection type.
static const std::string CONNECTION_TYPE_KEY = "connectionType";

/// Json payload key for network name.
static const std::string ESSID_KEY = "ESSID";

/// Json payload key for access point name.
static const std::string BSSID_KEY = "BSSID";

/// Json payload key for IP address.
static const std::string IP_ADDRESS_KEY = "IPAddress";

/// Json payload key for subnet mask.
static const std::string SUBNET_MASK_KEY = "subnetMask";

/// Json payload key for MAC address.
static const std::string MAC_ADDRESS_KEY = "MACAddress";

/// Json payload key for DHCP server address.
static const std::string DHCP_SERVER_ADDRESS_KEY = "DHCPServerAddress";

/// Json payload key for static IP.
static const std::string IS_STATIC_IP_KEY = "staticIP";

/// ConnectionType::ETHERNET string representation.
static const std::string CONNECTION_TYPE_ETHERNET = "ETHERNET";

/// ConnectionType::WIFI string representation.
static const std::string CONNECTION_TYPE_WIFI = "WIFI";

/// IPv4 format is 4 bytes where each byte is represented in decimal and seperated by '.'.
static constexpr char IP_V4_DELIMITER = '.';

/// IPv6 format is 16 bytes where each set of 2 bytes is represented in hexadecimal and seperated by ':'.
static constexpr char IP_V6_DELIMITER = ':';

/// Number of bytes in a MAC48 address.
static constexpr size_t MAC48_NUMBER_OF_BYTES = 6;

/// We assume the MAC address is delimited by ':'.
static constexpr char MAC_ADDRESS_DELIMITER = ':';

/// Number of bytes in an IPv4 address.
static constexpr int IP_V4_NUMBER_OF_BYTES = 4;

/// Number of hextets (2 bytes) in an IPv6 address.
static constexpr int IP_V6_NUMBER_OF_HEXTETS = 8;

/// Subnet mask range separator. Divides the ip portion from the range width.
static constexpr char SUBNET_RANGE_DELIMETER = '/';

/// The double colon used in IPv6 to replace groups of 0s.
static const std::string IP_V6_DOUBLE_COLONS = "::";

Optional<NetworkInfo::ConnectionType> NetworkInfo::getConnectionType() const {
    return m_connectionType;
}

Optional<std::string> NetworkInfo::getEssid() const {
    return m_essid;
}

Optional<std::string> NetworkInfo::getBssid() const {
    return m_bssid;
}

Optional<std::string> NetworkInfo::getIpAddress() const {
    return m_ipAddress;
}

Optional<std::string> NetworkInfo::getSubnetMask() const {
    return m_subnetMask;
}

Optional<std::string> NetworkInfo::getMacAddress() const {
    return m_macAddress;
}

Optional<std::string> NetworkInfo::getDhcpServerAddress() const {
    return m_dhcpServerAddress;
}

Optional<bool> NetworkInfo::getIsStaticIP() const {
    return m_isStaticIP;
}

void NetworkInfo::setConnectionType(const NetworkInfo::ConnectionType& connectionType) {
    m_connectionType.set(connectionType);
}

void NetworkInfo::setEssid(const std::string& essid) {
    m_essid.set(essid);
}

bool NetworkInfo::setBssid(const std::string& bssid) {
    if (!validMacAddress(bssid)) {
        ACSDK_ERROR(LX_FAILED().d("reason", "invalidFormat").sensitive("input", bssid));
        return false;
    }
    m_bssid.set(bssid);
    return true;
}

bool NetworkInfo::setIpAddress(const std::string& ipAddress) {
    if (!validIpAddress(ipAddress)) {
        ACSDK_ERROR(LX_FAILED().d("reason", "invalidFormat").sensitive("input", ipAddress));
        return false;
    }
    m_ipAddress.set(ipAddress);
    return true;
}

bool NetworkInfo::setSubnetMask(const std::string& subnetMask) {
    if (!validSubnetMask(subnetMask)) {
        ACSDK_ERROR(LX_FAILED().d("reason", "invalidFormat").sensitive("input", subnetMask));
        return false;
    }
    m_subnetMask.set(subnetMask);
    return true;
}

bool NetworkInfo::setMacAddress(const std::string& macAddress) {
    if (!validMacAddress(macAddress)) {
        ACSDK_ERROR(LX_FAILED().d("reason", "invalidFormat").sensitive("input", macAddress));
        return false;
    }
    m_macAddress.set(macAddress);
    return true;
}

bool NetworkInfo::setDhcpServerAddress(const std::string& dhcpServerAddress) {
    if (!validIpAddress(dhcpServerAddress)) {
        ACSDK_ERROR(LX_FAILED().d("reason", "invalidFormat").sensitive("input", dhcpServerAddress));
        return false;
    }
    m_dhcpServerAddress.set(dhcpServerAddress);
    return true;
}

void NetworkInfo::setIsStaticIP(bool isStaticIP) {
    m_isStaticIP.set(isStaticIP);
}

void NetworkInfo::resetConnectionType() {
    m_connectionType.reset();
}

void NetworkInfo::resetEssid() {
    m_essid.reset();
}

void NetworkInfo::resetBssid() {
    m_bssid.reset();
}

void NetworkInfo::resetIpAddress() {
    m_ipAddress.reset();
}

void NetworkInfo::resetSubnetMask() {
    m_subnetMask.reset();
}

void NetworkInfo::resetMacAddress() {
    m_macAddress.reset();
}

void NetworkInfo::resetDhcpServerAddress() {
    m_dhcpServerAddress.reset();
}

void NetworkInfo::resetIsStaticIP() {
    m_isStaticIP.reset();
}

bool NetworkInfo::operator==(const NetworkInfo& rhs) const {
    return std::tie(
               m_connectionType,
               m_essid,
               m_bssid,
               m_ipAddress,
               m_subnetMask,
               m_macAddress,
               m_dhcpServerAddress,
               m_isStaticIP) ==
           std::tie(
               rhs.m_connectionType,
               rhs.m_essid,
               rhs.m_bssid,
               rhs.m_ipAddress,
               rhs.m_subnetMask,
               rhs.m_macAddress,
               rhs.m_dhcpServerAddress,
               rhs.m_isStaticIP);
}

bool NetworkInfo::operator!=(const NetworkInfo& rhs) const {
    return !(*this == rhs);
}

/**
 * Checks whether the input has the expected format.
 *
 * @param input The target string that will be tested.
 * @param valueChecker The function that validates the extracted value.
 * @param expectedDelimiter The delimiter used to separate each value.
 * @param hexFormat Whether the values are encoded as hexadecimals.
 * @return @c true if the string represents an IPv4 address; @c false otherwise.
 */
static bool expectedFormat(
    const std::string& input,
    const std::function<bool(unsigned int)>& valueChecker,
    char expectedDelimiter,
    bool hexFormat = false) {
    std::istringstream stream{input};
    if (hexFormat) {
        stream >> std::hex;
    }
    stream >> std::noskipws;

    while (stream.good()) {
        unsigned int value = 0;
        stream >> value;
        if (stream.fail()) {
            ACSDK_DEBUG(LX_FAILED().d("reason", "streamFail").sensitive("input", input).d("position", stream.tellg()));
            return false;
        }

        if (!valueChecker(value)) {
            ACSDK_DEBUG(LX_FAILED().d("reason", "invalidValue").sensitive("value", value));
            return false;
        }

        char delimiter = expectedDelimiter;
        stream >> delimiter;
        if (delimiter != expectedDelimiter) {
            ACSDK_DEBUG(
                LX_FAILED().d("reason", "invalidDelimiter").sensitive("input", input).d("delimiter", delimiter));
            return false;
        }
    }

    return stream.eof();
}

/**
 * Checks whether the input is a valid IPv4 format.
 *
 * @param input The target string that may represent an IPv4 address.
 * @return @c true if the string represents an IPv4 address; @c false otherwise.
 */
static bool validIpV4(const std::string& input) {
    size_t bytes = 0;
    auto valueChecker = [&bytes](unsigned int value) {
        bytes++;
        return value <= std::numeric_limits<uint8_t>::max();
    };
    return expectedFormat(input, valueChecker, IP_V4_DELIMITER) && (bytes == IP_V4_NUMBER_OF_BYTES);
}

/**
 * Checks whether the modified hextet and colon position make a valid IPV6
 * @param hextets amount of hextets given
 * @param doubleColonPos position of the double colon
 * @return
 */
static bool verifyHextetsIPv6(const size_t hextets, const size_t doubleColonPos) {
    return (
        (hextets == IP_V6_NUMBER_OF_HEXTETS) ||
        ((doubleColonPos != std::string::npos) && (hextets < IP_V6_NUMBER_OF_HEXTETS)));
}

/**
 * Checks whether the input is a valid IPv6 format.
 *
 * @param input The target string that may represent an IPv6 address.
 * @return @c true if the string represents an IPv6 address; @c false otherwise.
 */
static bool validIpV6(const std::string& input) {
    size_t doubleColonPos = input.find(IP_V6_DOUBLE_COLONS);
    std::string no2ColonsInput{input};
    if (doubleColonPos != std::string::npos) {
        if ((doubleColonPos == 0) && (input.size() > 1)) {
            // Starts with :: so remove both characters.
            no2ColonsInput.erase(doubleColonPos, 2);
        } else {
            no2ColonsInput.erase(doubleColonPos, 1);
        }
    }

    size_t hextets = 0;
    auto valueChecker = [&hextets](unsigned int value) {
        hextets++;
        return value <= std::numeric_limits<uint16_t>::max();
    };

    if (expectedFormat(no2ColonsInput, valueChecker, IP_V6_DELIMITER, true)) {
        return verifyHextetsIPv6(hextets, doubleColonPos);
    }
    return false;
}

bool NetworkInfo::validMacAddress(const std::string& input) {
    size_t bytes = 0;
    auto valueChecker = [&bytes](unsigned int value) {
        bytes++;
        return value <= std::numeric_limits<uint8_t>::max();
    };
    return expectedFormat(input, valueChecker, MAC_ADDRESS_DELIMITER, true) && (bytes == MAC48_NUMBER_OF_BYTES);
}

bool NetworkInfo::validIpAddress(const std::string& input) {
    return validIpV4(input) || validIpV6(input);
}

bool NetworkInfo::validSubnetMask(const std::string& input) {
    auto separatorIndex = input.find_first_of(SUBNET_RANGE_DELIMETER);
    if (separatorIndex == std::string::npos) {
        return validIpAddress(input);
    }

    auto widthPortion = input.substr(separatorIndex + 1);
    int width = 0;
    if (!avsCommon::utils::string::stringToInt(widthPortion, &width)) {
        ACSDK_DEBUG(LX_FAILED().d("reason", "invalidWidth").sensitive("input", input));
        return false;
    }

    auto ipPortion = input.substr(0, separatorIndex);
    if (validIpV4(ipPortion)) {
        return (width > 0) && (width < IP_V4_NUMBER_OF_BYTES * 8);
    }

    return validIpV6(ipPortion) && (width > 0) && (width < IP_V6_NUMBER_OF_HEXTETS * 16);
}

std::ostream& operator<<(std::ostream& os, const NetworkInfo& info) {
    avsCommon::utils::json::JsonGenerator generator;
    if (info.m_connectionType.hasValue()) {
        std::string typeStr = info.m_connectionType.value() == NetworkInfo::ConnectionType::ETHERNET
                                  ? CONNECTION_TYPE_ETHERNET
                                  : CONNECTION_TYPE_WIFI;
        generator.addMember(CONNECTION_TYPE_KEY, typeStr);
    }

    if (info.m_essid.hasValue()) {
        generator.addMember(ESSID_KEY, info.m_essid.value());
    }

    if (info.m_bssid.hasValue()) {
        generator.addMember(BSSID_KEY, info.m_bssid.value());
    }

    if (info.m_ipAddress.hasValue()) {
        generator.addMember(IP_ADDRESS_KEY, info.m_ipAddress.value());
    }

    if (info.m_subnetMask.hasValue()) {
        generator.addMember(SUBNET_MASK_KEY, info.m_subnetMask.value());
    }

    if (info.m_macAddress.hasValue()) {
        generator.addMember(MAC_ADDRESS_KEY, info.m_macAddress.value());
    }

    if (info.m_dhcpServerAddress.hasValue()) {
        generator.addMember(DHCP_SERVER_ADDRESS_KEY, info.m_dhcpServerAddress.value());
    }

    if (info.m_isStaticIP.hasValue()) {
        generator.addMember(IS_STATIC_IP_KEY, info.m_isStaticIP.value());
    }

    os << generator.toString();
    return os;
}

std::istream& operator>>(std::istream& is, NetworkInfo& info) {
    rapidjson::IStreamWrapper isw(is);
    rapidjson::Document document;
    document.ParseStream(isw);
    if (document.HasParseError()) {
        ACSDK_ERROR(LX_FAILED()
                        .d("offset", document.GetErrorOffset())
                        .d("error", rapidjson::GetParseError_En(document.GetParseError())));
        is.setstate(std::ios_base::failbit);
        return is;
    }

    // Rapidjson leaves the stream with failbit and eofbit set to true. Set EOF only since there was no error.
    is.clear(std::ios_base::eofbit);

    std::string str;
    if (jsonUtils::retrieveValue(document, CONNECTION_TYPE_KEY, &str)) {
        if (str == CONNECTION_TYPE_ETHERNET) {
            info.setConnectionType(NetworkInfo::ConnectionType::ETHERNET);
        } else if (str == CONNECTION_TYPE_WIFI) {
            info.setConnectionType(NetworkInfo::ConnectionType::WIFI);
        } else {
            ACSDK_ERROR(LX_FAILED().d("reason", "invalidConnectionType").sensitive("value", str));
            is.setstate(std::ios_base::failbit);
        }
    }

    if (jsonUtils::retrieveValue(document, ESSID_KEY, &str)) {
        info.setEssid(str);
    }

    if (jsonUtils::retrieveValue(document, BSSID_KEY, &str) && !info.setBssid(str)) {
        ACSDK_ERROR(LX_FAILED().d("reason", "invalidBssid").sensitive("value", str));
        is.setstate(std::ios_base::failbit);
    }

    if (jsonUtils::retrieveValue(document, IP_ADDRESS_KEY, &str) && !info.setIpAddress(str)) {
        ACSDK_ERROR(LX_FAILED().d("reason", "invalidIpAddress").sensitive("value", str));
        is.setstate(std::ios_base::failbit);
    }

    if (jsonUtils::retrieveValue(document, SUBNET_MASK_KEY, &str) && !info.setSubnetMask(str)) {
        ACSDK_ERROR(LX_FAILED().d("reason", "invalidSubnetMask").sensitive("value", str));
        is.setstate(std::ios_base::failbit);
    }

    if (jsonUtils::retrieveValue(document, MAC_ADDRESS_KEY, &str) && !info.setMacAddress(str)) {
        ACSDK_ERROR(LX_FAILED().d("reason", "invalidMacAddress").sensitive("value", str));
        is.setstate(std::ios_base::failbit);
    }

    if (jsonUtils::retrieveValue(document, DHCP_SERVER_ADDRESS_KEY, &str) && !info.setDhcpServerAddress(str)) {
        ACSDK_ERROR(LX_FAILED().d("reason", "invalidDHCPServer").sensitive("value", str));
        is.setstate(std::ios_base::failbit);
    }

    bool isStaticIp = false;
    if (jsonUtils::retrieveValue(document, IS_STATIC_IP_KEY, &isStaticIp)) {
        info.setIsStaticIP(isStaticIp);
    }

    return is;
}

}  // namespace types
}  // namespace settings
}  // namespace alexaClientSDK
