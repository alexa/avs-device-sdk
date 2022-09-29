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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_BLUETOOTH_MOCKBLUETOOTHDEVICE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_BLUETOOTH_MOCKBLUETOOTHDEVICE_H_

#include <unordered_map>
#include <vector>

#include <gmock/gmock.h>

#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/BluetoothServiceInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace bluetooth {
namespace test {

using namespace ::testing;

static constexpr std::size_t PAIRING_PIN_LENGTH_MIN = 4;
static constexpr std::size_t PAIRING_PIN_LENGTH_MAX = 16;

/**
 * Mock class that implements BluetoothDeviceInterface.
 * Please note that MockBluetoothDevice doesn't support sending any @c BluetoothEvent to @c BluetoothEventBus,
 * any @c BluetoothEvent based logic will need to be tested separately.
 */
class MockBluetoothDevice : public BluetoothDeviceInterface {
public:
    std::string getMac() const override;
    std::string getFriendlyName() const override;
    DeviceState getDeviceState() override;
    MockBluetoothDevice::MetaData getDeviceMetaData() override;
    bool isPaired() override;
    std::future<bool> pair() override;
    std::future<bool> unpair() override;
    bool isConnected() override;
    std::future<bool> connect() override;
    std::future<bool> disconnect() override;
    bool setPairingPin(const std::string& pin) override;
    std::vector<std::shared_ptr<services::SDPRecordInterface>> getSupportedServices() override;
    std::shared_ptr<services::BluetoothServiceInterface> getService(std::string uuid) override;
    utils::bluetooth::MediaStreamingState getStreamingState() override;
    bool toggleServiceConnection(bool enabled, std::shared_ptr<services::BluetoothServiceInterface> service) override;

    /// Constructor.
    MockBluetoothDevice(
        const std::string& mac,
        const std::string friendlyName,
        MetaData metaData,
        std::vector<std::shared_ptr<services::BluetoothServiceInterface>> supportedServices);

protected:
    /// Represent the pair status.
    bool m_isPaired;
    /// Represent the connection status.
    bool m_isConnected;
    /// Represent the Bluetooth device mac address.
    const std::string m_mac;
    /// Represent the Bluetooth device friendly name.
    const std::string m_friendlyName;
    /// Represent the Bluetooth device meta data.
    MockBluetoothDevice::MetaData m_metaData;
    /// Represent the Bluetooth device supported Services.
    std::unordered_map<std::string, std::shared_ptr<services::BluetoothServiceInterface>> m_supportedServices;
    /// Represent the Bluetooth device state.
    DeviceState m_deviceState;
};

inline std::string MockBluetoothDevice::getMac() const {
    return m_mac;
}

inline std::string MockBluetoothDevice::getFriendlyName() const {
    return m_friendlyName;
}

inline DeviceState MockBluetoothDevice::getDeviceState() {
    return m_deviceState;
}

inline MockBluetoothDevice::MetaData MockBluetoothDevice::getDeviceMetaData() {
    return m_metaData;
}

inline bool MockBluetoothDevice::isPaired() {
    return m_isPaired;
}

inline std::future<bool> MockBluetoothDevice::pair() {
    std::promise<bool> pairPromise;
    pairPromise.set_value(true);
    m_isPaired = true;
    m_deviceState = DeviceState::PAIRED;
    return pairPromise.get_future();
}

inline std::future<bool> MockBluetoothDevice::unpair() {
    std::promise<bool> pairPromise;
    pairPromise.set_value(true);
    m_isPaired = false;
    m_deviceState = DeviceState::UNPAIRED;
    return pairPromise.get_future();
}

inline bool MockBluetoothDevice::isConnected() {
    return m_isConnected;
}

inline std::future<bool> MockBluetoothDevice::connect() {
    std::promise<bool> connectionPromise;
    connectionPromise.set_value(true);
    m_isConnected = true;
    m_deviceState = DeviceState::CONNECTED;
    return connectionPromise.get_future();
}

inline std::future<bool> MockBluetoothDevice::disconnect() {
    std::promise<bool> connectionPromise;
    connectionPromise.set_value(true);
    m_isConnected = false;
    m_deviceState = DeviceState::DISCONNECTED;
    return connectionPromise.get_future();
}

inline bool MockBluetoothDevice::setPairingPin(const std::string& pin) {
    if (pin.length() < PAIRING_PIN_LENGTH_MIN || pin.length() > PAIRING_PIN_LENGTH_MAX) {
        return false;
    }
    return true;
}

inline std::vector<std::shared_ptr<services::SDPRecordInterface>> MockBluetoothDevice::getSupportedServices() {
    std::vector<std::shared_ptr<services::SDPRecordInterface>> services;
    for (auto service : m_supportedServices) {
        services.push_back(service.second->getRecord());
    }
    return services;
}

inline std::shared_ptr<services::BluetoothServiceInterface> MockBluetoothDevice::getService(std::string uuid) {
    auto serviceIt = m_supportedServices.find(uuid);
    if (serviceIt != m_supportedServices.end()) {
        return serviceIt->second;
    }
    return nullptr;
}

inline utils::bluetooth::MediaStreamingState MockBluetoothDevice::getStreamingState() {
    return alexaClientSDK::avsCommon::utils::bluetooth::MediaStreamingState::IDLE;
}

inline bool MockBluetoothDevice::toggleServiceConnection(
    bool enabled,
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::bluetooth::services::BluetoothServiceInterface> service) {
    // No-op.
    return false;
}

MockBluetoothDevice::MockBluetoothDevice(
    const std::string& mac,
    const std::string friendlyName,
    alexaClientSDK::avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface::MetaData metaData,
    std::vector<std::shared_ptr<services::BluetoothServiceInterface>> supportedServices) :
        m_mac{mac},
        m_friendlyName{friendlyName},
        m_metaData{metaData} {
    m_isPaired = false;
    m_isConnected = false;
    m_deviceState = DeviceState::FOUND;
    for (unsigned int i = 0; i < supportedServices.size(); ++i) {
        auto service = supportedServices[i];
        m_supportedServices.insert({service->getRecord()->getUuid(), service});
    }
}

}  // namespace test
}  // namespace bluetooth
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_BLUETOOTH_MOCKBLUETOOTHDEVICE_H_