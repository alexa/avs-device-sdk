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

#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <Settings/Types/NetworkInfo.h>

namespace alexaClientSDK {
namespace settings {
namespace test {

using namespace alexaClientSDK::settings::types;

/// A random valid connection type.
static const auto CONNECTION_TYPE = NetworkInfo::ConnectionType::WIFI;

/// A random valid bssid.
static const auto BSSID = "10:00:22:33:44:55";

/// A random valid IP address.
static const auto IP_ADDRESS = "1.2.3.5";

/// A random valid subnet mask.
static const auto SUBNET_MASK = "1.2.3.0/2";

/// A random valid MAC address.
static const auto MAC_ADDRESS = "00:FF:00:FF:00:FF";

/// A random valid DHCP server address.
static const auto DHCP_SERVER_ADDRESS = "200.125.4.0";

/// A random valid connection type.
static const auto ESSID = "essid";

/// A random valid connection type.
static const auto IS_STATIC_IP = false;

TEST(NetworkInfoTest, test_restoreInfoFromEmptyString) {
    NetworkInfo info;
    std::stringstream stream;
    stream << "";
    stream >> std::boolalpha >> info;

    EXPECT_TRUE(stream.fail());
    EXPECT_FALSE(info.getConnectionType().hasValue());
    EXPECT_FALSE(info.getEssid().hasValue());
    EXPECT_FALSE(info.getBssid().hasValue());
    EXPECT_FALSE(info.getIpAddress().hasValue());
    EXPECT_FALSE(info.getSubnetMask().hasValue());
    EXPECT_FALSE(info.getMacAddress().hasValue());
    EXPECT_FALSE(info.getDhcpServerAddress().hasValue());
    EXPECT_FALSE(info.getIsStaticIP().hasValue());
}

TEST(NetworkInfoTest, test_restoreInfoFromEmptyJson) {
    NetworkInfo info;
    std::stringstream stream;
    stream << "{}";
    stream >> std::boolalpha >> info;

    EXPECT_FALSE(stream.fail());
    EXPECT_FALSE(info.getConnectionType().hasValue());
    EXPECT_FALSE(info.getEssid().hasValue());
    EXPECT_FALSE(info.getBssid().hasValue());
    EXPECT_FALSE(info.getIpAddress().hasValue());
    EXPECT_FALSE(info.getSubnetMask().hasValue());
    EXPECT_FALSE(info.getMacAddress().hasValue());
    EXPECT_FALSE(info.getDhcpServerAddress().hasValue());
    EXPECT_FALSE(info.getIsStaticIP().hasValue());
}

TEST(NetworkInfoTest, test_convertFullInfoToStringAndBack) {
    NetworkInfo info;
    info.setConnectionType(NetworkInfo::ConnectionType::WIFI);
    info.setEssid("essid");
    info.setIsStaticIP(false);
    EXPECT_TRUE(info.setBssid("10:00:22:33:44:55"));
    EXPECT_TRUE(info.setIpAddress("1.2.3.5"));
    EXPECT_TRUE(info.setSubnetMask("1.2.3.0/2"));
    EXPECT_TRUE(info.setMacAddress("00:FF:00:FF:00:FF"));
    EXPECT_TRUE(info.setDhcpServerAddress("200.125.4.0"));

    NetworkInfo info2;
    ASSERT_NE(info, info2);

    std::stringstream stream;
    stream << info;
    stream >> info2;
    EXPECT_EQ(info, info2);
}

TEST(NetworkInfoTest, test_outputEmptyNetworkInfo) {
    NetworkInfo info;
    std::stringstream stream;
    stream << info;
    EXPECT_EQ(stream.str(), "{}");
}

TEST(NetworkInfoTest, test_setIpV4Address) {
    NetworkInfo info;
    EXPECT_TRUE(info.setIpAddress("0.0.0.0"));
    EXPECT_TRUE(info.setIpAddress("255.255.255.255"));
    EXPECT_FALSE(info.setIpAddress("255.255.255. 255"));
    EXPECT_FALSE(info.setIpAddress("-255.255.255.255"));
    EXPECT_FALSE(info.setIpAddress("255.255.255.2555"));
    EXPECT_FALSE(info.setIpAddress("255.255.255"));
    EXPECT_FALSE(info.setIpAddress("255.255.255.255.5"));
    EXPECT_FALSE(info.setIpAddress("255.255.255:19"));
    EXPECT_FALSE(info.setIpAddress("255.255.F.19"));
}

TEST(NetworkInfoTest, test_setIpV6Address) {
    NetworkInfo info;
    EXPECT_TRUE(info.setIpAddress("FFFF::1234"));
    EXPECT_TRUE(info.setIpAddress("::FFFF:1234"));
    EXPECT_TRUE(info.setIpAddress("255:0001:FFFF:AAAE:2345:2:44:23"));
    EXPECT_FALSE(info.setIpAddress("FFFF::-1234"));
    EXPECT_FALSE(info.setIpAddress("FFFF::1234::AAAA"));
    EXPECT_FALSE(info.setIpAddress("255:0001:FFFF:AAAE:2345:2:44:23211"));
    EXPECT_FALSE(info.setIpAddress("FFFF::-1234"));
}

TEST(NetworkInfoTest, test_setMacAddress) {
    NetworkInfo info;
    EXPECT_TRUE(info.setMacAddress("00:00:23:11:35:34"));
    EXPECT_TRUE(info.setMacAddress("FF:EE:DD:CC:BB:AA"));
    EXPECT_TRUE(info.setMacAddress("F5:E4:D3:C2:B1:A0"));
    EXPECT_FALSE(info.setMacAddress("F5:E4:D3:C2:B1:A0:"));
    EXPECT_FALSE(info.setMacAddress("F5:E4:D3:C2:B1:A0:0"));
}

TEST(NetworkInfoTest, test_setSubnetMask) {
    NetworkInfo info;
    // IPv4.
    EXPECT_TRUE(info.setSubnetMask("0.0.0.0/20"));
    EXPECT_TRUE(info.setSubnetMask("255.255.255.255/8"));
    EXPECT_TRUE(info.setSubnetMask("255.255.255.0"));
    EXPECT_FALSE(info.setSubnetMask("255.255.255.255/33"));
    EXPECT_FALSE(info.setSubnetMask("255.255.255.255/-2"));
    EXPECT_FALSE(info.setSubnetMask("-255.255.255.255/21"));
    EXPECT_FALSE(info.setSubnetMask("255.255.255.2555/20"));
    EXPECT_FALSE(info.setSubnetMask("255.255.255.255/"));
    EXPECT_FALSE(info.setSubnetMask("255.255.255.255/a"));

    // IPv6.
    EXPECT_TRUE(info.setSubnetMask("FFFF::1234/100"));
    EXPECT_TRUE(info.setSubnetMask("255:0001:FFFF:AAAE:2345:2:44:23/120"));
    EXPECT_FALSE(info.setSubnetMask("255:0001:FFFF:AAAE:2345:2:44:23/250"));
}

TEST(NetworkInfoTest, test_emptyGetters) {
    NetworkInfo info;
    EXPECT_FALSE(info.getConnectionType().hasValue());
    EXPECT_FALSE(info.getEssid().hasValue());
    EXPECT_FALSE(info.getBssid().hasValue());
    EXPECT_FALSE(info.getIpAddress().hasValue());
    EXPECT_FALSE(info.getSubnetMask().hasValue());
    EXPECT_FALSE(info.getMacAddress().hasValue());
    EXPECT_FALSE(info.getDhcpServerAddress().hasValue());
    EXPECT_FALSE(info.getIsStaticIP().hasValue());
}

TEST(NetworkInfoTest, test_gettersAfterSet) {
    NetworkInfo info;
    info.setConnectionType(CONNECTION_TYPE);
    info.setEssid(ESSID);
    info.setIsStaticIP(IS_STATIC_IP);
    EXPECT_TRUE(info.setBssid(BSSID));
    EXPECT_TRUE(info.setIpAddress(IP_ADDRESS));
    EXPECT_TRUE(info.setSubnetMask(SUBNET_MASK));
    EXPECT_TRUE(info.setMacAddress(MAC_ADDRESS));
    EXPECT_TRUE(info.setDhcpServerAddress(DHCP_SERVER_ADDRESS));

    ASSERT_TRUE(info.getConnectionType().hasValue());
    ASSERT_TRUE(info.getEssid().hasValue());
    ASSERT_TRUE(info.getBssid().hasValue());
    ASSERT_TRUE(info.getIpAddress().hasValue());
    ASSERT_TRUE(info.getSubnetMask().hasValue());
    ASSERT_TRUE(info.getMacAddress().hasValue());
    ASSERT_TRUE(info.getDhcpServerAddress().hasValue());
    ASSERT_TRUE(info.getIsStaticIP().hasValue());

    EXPECT_EQ(info.getConnectionType().value(), CONNECTION_TYPE);
    EXPECT_EQ(info.getEssid().value(), ESSID);
    EXPECT_EQ(info.getBssid().value(), BSSID);
    EXPECT_EQ(info.getIpAddress().value(), IP_ADDRESS);
    EXPECT_EQ(info.getSubnetMask().value(), SUBNET_MASK);
    EXPECT_EQ(info.getMacAddress().value(), MAC_ADDRESS);
    EXPECT_EQ(info.getDhcpServerAddress().value(), DHCP_SERVER_ADDRESS);
    EXPECT_EQ(info.getIsStaticIP().value(), IS_STATIC_IP);
}

TEST(NetworkInfoTest, test_gettersAfterReset) {
    NetworkInfo info;
    info.setConnectionType(CONNECTION_TYPE);
    info.setEssid(ESSID);
    info.setIsStaticIP(IS_STATIC_IP);
    EXPECT_TRUE(info.setBssid(BSSID));
    EXPECT_TRUE(info.setIpAddress(IP_ADDRESS));
    EXPECT_TRUE(info.setSubnetMask(SUBNET_MASK));
    EXPECT_TRUE(info.setMacAddress(MAC_ADDRESS));
    EXPECT_TRUE(info.setDhcpServerAddress(DHCP_SERVER_ADDRESS));

    info.resetConnectionType();
    info.resetEssid();
    info.resetBssid();
    info.resetIpAddress();
    info.resetSubnetMask();
    info.resetMacAddress();
    info.resetDhcpServerAddress();
    info.resetIsStaticIP();

    EXPECT_FALSE(info.getConnectionType().hasValue());
    EXPECT_FALSE(info.getEssid().hasValue());
    EXPECT_FALSE(info.getBssid().hasValue());
    EXPECT_FALSE(info.getIpAddress().hasValue());
    EXPECT_FALSE(info.getSubnetMask().hasValue());
    EXPECT_FALSE(info.getMacAddress().hasValue());
    EXPECT_FALSE(info.getDhcpServerAddress().hasValue());
    EXPECT_FALSE(info.getIsStaticIP().hasValue());
}

}  // namespace test
}  // namespace settings
}  // namespace alexaClientSDK
