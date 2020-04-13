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

// @file DeviceInfoTest.cpp

#include <gtest/gtest.h>

#include "AVSCommon/Utils/DeviceInfo.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace test {

using namespace ::testing;

/// A Test client Id.
static const std::string TEST_CLIENT_ID = "TEST_CLIENT_ID";

/// A Test product Id.
static const std::string TEST_PRODUCT_ID = "TEST_PRODUCT_ID";

/// A Test serial number.
static const std::string TEST_SERIAL_NUMBER = "TEST_SERIAL_NUMBER";

/// A Test manufacturer name.
static const std::string TEST_MANUFACTURER_NAME = "TEST_MANUFACTURER_NAME";

/// A Test description.
static const std::string TEST_DESCRIPTION = "TEST_DESCRIPTION";

/// A Test friendly name.
static const std::string TEST_FRIENDLY_NAME = "TEST_FRIENDLY_NAME";

/// A Test device type.
static const std::string TEST_DEVICE_TYPE = "TEST_DEVICE_TYPE";

/// A Test endpoint identifier name.
static const sdkInterfaces::endpoints::EndpointIdentifier TEST_ENDPOINT_IDENTIFER = "TEST_ENDPOINT_IDENTIFER";

/// A Test registration key name.
static const std::string TEST_REGISTRATION_KEY = "TEST_REGISTRATION_KEY";

/// A Test product id key.
static const std::string TEST_PRODUCT_ID_KEY = "TEST_PRODUCT_ID_KEY";

/// A Default registration key name.
static const std::string DEFAULT_REGISTRATION_KEY = "registration";

/// A default product id key name.
static const std::string DEFAULT_PRODUCT_ID_KEY = "productId";

/**
 * Class for testing the DeviceInfo class
 */
class DeviceInfoTest : public ::testing::Test {};

TEST_F(DeviceInfoTest, test_buildDefault) {
    auto deviceInfo = DeviceInfo::create(
        TEST_CLIENT_ID, TEST_PRODUCT_ID, TEST_SERIAL_NUMBER, TEST_MANUFACTURER_NAME, TEST_DESCRIPTION);
    ASSERT_NE(deviceInfo, nullptr);
    ASSERT_EQ(deviceInfo->getClientId(), TEST_CLIENT_ID);
    ASSERT_EQ(deviceInfo->getProductId(), TEST_PRODUCT_ID);
    ASSERT_EQ(deviceInfo->getDeviceSerialNumber(), TEST_SERIAL_NUMBER);
    ASSERT_EQ(deviceInfo->getManufacturerName(), TEST_MANUFACTURER_NAME);
    ASSERT_EQ(deviceInfo->getDeviceDescription(), TEST_DESCRIPTION);

    // validate these missing values are default values
    ASSERT_EQ(deviceInfo->getRegistrationKey(), DEFAULT_REGISTRATION_KEY);
    ASSERT_EQ(deviceInfo->getProductIdKey(), DEFAULT_PRODUCT_ID_KEY);
}

TEST_F(DeviceInfoTest, test_buildCustomKeys) {
    auto deviceInfo = DeviceInfo::create(
        TEST_CLIENT_ID,
        TEST_PRODUCT_ID,
        TEST_SERIAL_NUMBER,
        TEST_MANUFACTURER_NAME,
        TEST_DESCRIPTION,
        TEST_FRIENDLY_NAME,
        TEST_DEVICE_TYPE,
        TEST_ENDPOINT_IDENTIFER,
        TEST_REGISTRATION_KEY,
        TEST_PRODUCT_ID_KEY);
    ASSERT_NE(deviceInfo, nullptr);
    ASSERT_EQ(deviceInfo->getFriendlyName(), TEST_FRIENDLY_NAME);
    ASSERT_EQ(deviceInfo->getDeviceType(), TEST_DEVICE_TYPE);
    ASSERT_EQ(deviceInfo->getDefaultEndpointId(), TEST_ENDPOINT_IDENTIFER);
    ASSERT_EQ(deviceInfo->getRegistrationKey(), TEST_REGISTRATION_KEY);
    ASSERT_EQ(deviceInfo->getProductIdKey(), TEST_PRODUCT_ID_KEY);

    deviceInfo = DeviceInfo::create(
        TEST_CLIENT_ID,
        TEST_PRODUCT_ID,
        TEST_SERIAL_NUMBER,
        TEST_MANUFACTURER_NAME,
        TEST_DESCRIPTION,
        "",
        "",
        "",
        "",
        "");
    ASSERT_NE(deviceInfo, nullptr);
    ASSERT_EQ(deviceInfo->getRegistrationKey(), DEFAULT_REGISTRATION_KEY);
    ASSERT_EQ(deviceInfo->getProductIdKey(), DEFAULT_PRODUCT_ID_KEY);
}

TEST_F(DeviceInfoTest, test_buildEmptyStringsInvalid) {
    auto deviceInfo =
        DeviceInfo::create("", TEST_PRODUCT_ID, TEST_SERIAL_NUMBER, TEST_MANUFACTURER_NAME, TEST_DESCRIPTION);
    ASSERT_EQ(deviceInfo, nullptr);

    deviceInfo = DeviceInfo::create(TEST_CLIENT_ID, "", TEST_SERIAL_NUMBER, TEST_MANUFACTURER_NAME, TEST_DESCRIPTION);
    ASSERT_EQ(deviceInfo, nullptr);

    deviceInfo = DeviceInfo::create(TEST_CLIENT_ID, TEST_PRODUCT_ID, "", TEST_MANUFACTURER_NAME, TEST_DESCRIPTION);
    ASSERT_EQ(deviceInfo, nullptr);

    deviceInfo = DeviceInfo::create(TEST_CLIENT_ID, TEST_PRODUCT_ID, TEST_SERIAL_NUMBER, "", TEST_DESCRIPTION);
    ASSERT_EQ(deviceInfo, nullptr);

    deviceInfo = DeviceInfo::create(TEST_CLIENT_ID, TEST_PRODUCT_ID, TEST_SERIAL_NUMBER, TEST_MANUFACTURER_NAME, "");
    ASSERT_EQ(deviceInfo, nullptr);
}

}  // namespace test
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK