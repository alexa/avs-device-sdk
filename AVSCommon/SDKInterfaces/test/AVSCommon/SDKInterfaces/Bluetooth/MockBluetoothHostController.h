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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_BLUETOOTH_MOCKBLUETOOTHHOSTCONTROLLER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_BLUETOOTH_MOCKBLUETOOTHHOSTCONTROLLER_H_

#include <gmock/gmock.h>

#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothHostControllerInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace bluetooth {
namespace test {

static const std::string MOCK_MAC_ADDRESS = "XX:XX:XX:XX";
static const std::string MOCK_FRIENDLY_NAME = "MockBluetoothHostControllerName";

/**
 * Mock class that implements BluetoothHostControllerInterface
 */
class MockBluetoothHostController : public BluetoothHostControllerInterface {
public:
    std::string getMac() const override;
    std::string getFriendlyName() const override;
    bool isDiscoverable() const override;
    bool isScanning() const override;
    std::future<bool> startScan() override;
    std::future<bool> stopScan() override;
    std::future<bool> enterDiscoverableMode() override;
    std::future<bool> exitDiscoverableMode() override;

protected:
    bool m_isDiscoverable;
    bool m_isScanning;
};

inline std::future<bool> MockBluetoothHostController::startScan() {
    std::promise<bool> scanPromise;
    scanPromise.set_value(true);
    m_isScanning = true;
    return scanPromise.get_future();
}

inline std::future<bool> MockBluetoothHostController::stopScan() {
    std::promise<bool> scanPromise;
    scanPromise.set_value(true);
    m_isScanning = false;
    return scanPromise.get_future();
}

std::future<bool> MockBluetoothHostController::enterDiscoverableMode() {
    std::promise<bool> discoverPromise;
    discoverPromise.set_value(true);
    m_isDiscoverable = true;
    return discoverPromise.get_future();
}

std::future<bool> MockBluetoothHostController::exitDiscoverableMode() {
    std::promise<bool> discoverPromise;
    discoverPromise.set_value(true);
    m_isDiscoverable = false;
    return discoverPromise.get_future();
}

bool MockBluetoothHostController::isScanning() const {
    return m_isScanning;
}

bool MockBluetoothHostController::isDiscoverable() const {
    return m_isDiscoverable;
}

std::string MockBluetoothHostController::getMac() const {
    return MOCK_MAC_ADDRESS;
}

std::string MockBluetoothHostController::getFriendlyName() const {
    return MOCK_FRIENDLY_NAME;
}

}  // namespace test
}  // namespace bluetooth
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_BLUETOOTH_MOCKBLUETOOTHHOSTCONTROLLER_H_