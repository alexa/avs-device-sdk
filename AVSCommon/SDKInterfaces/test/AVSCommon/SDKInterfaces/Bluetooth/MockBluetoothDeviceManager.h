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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_BLUETOOTH_MOCKBLUETOOTHDEVICEMANAGER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_BLUETOOTH_MOCKBLUETOOTHDEVICEMANAGER_H_

#include <gmock/gmock.h>

#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceManagerInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace bluetooth {
namespace test {

using namespace ::testing;

/**
 * Mock class that implements the BluetoothDeviceManagerInterface.
 */
class MockBluetoothDeviceManager : public BluetoothDeviceManagerInterface {
public:
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothHostControllerInterface> getHostController() override;
    std::list<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>> getDiscoveredDevices()
        override;
    std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> getEventBus() override;

    /**
     * Constructor
     * @param hostcontroller @c BluetoothHostController
     * @param discoveredDevices a list of discovered devices.
     * @param eventBus @c BluetoothEventBus.
     */
    MockBluetoothDeviceManager(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothHostControllerInterface> hostcontroller,
        std::list<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>> discoveredDevices,
        std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> eventBus);

protected:
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothHostControllerInterface> m_hostController;
    std::list<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>> m_discoveredDevices;
    std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> m_eventBus;
};

inline std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothHostControllerInterface>
MockBluetoothDeviceManager::getHostController() {
    return m_hostController;
}

inline std::list<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>>
MockBluetoothDeviceManager::getDiscoveredDevices() {
    return m_discoveredDevices;
}

inline std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> MockBluetoothDeviceManager::getEventBus() {
    return m_eventBus;
}

inline MockBluetoothDeviceManager::MockBluetoothDeviceManager(
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::bluetooth::BluetoothHostControllerInterface>
        hostcontroller,
    std::list<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>>
        discoveredDevices,
    std::shared_ptr<alexaClientSDK::avsCommon::utils::bluetooth::BluetoothEventBus> eventBus) :
        m_hostController{hostcontroller},
        m_discoveredDevices(discoveredDevices),
        m_eventBus(eventBus) {
}

}  // namespace test
}  // namespace bluetooth
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_BLUETOOTH_MOCKBLUETOOTHDEVICEMANAGER_H_
