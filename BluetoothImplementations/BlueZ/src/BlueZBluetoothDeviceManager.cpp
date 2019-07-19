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

#include <AVSCommon/Utils/Logger/Logger.h>

#include "BlueZ/BlueZBluetoothDeviceManager.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

using namespace avsCommon::sdkInterfaces::bluetooth;
using namespace avsCommon::utils::bluetooth;

/// String to identify log entries originating from this file.
static const std::string TAG{"BlueZBluetoothDeviceManager"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<BluetoothHostControllerInterface> BlueZBluetoothDeviceManager::getHostController() {
    return m_deviceManager->getHostController();
}

std::list<std::shared_ptr<BluetoothDeviceInterface>> BlueZBluetoothDeviceManager::getDiscoveredDevices() {
    return m_deviceManager->getDiscoveredDevices();
}

std::unique_ptr<BlueZBluetoothDeviceManager> BlueZBluetoothDeviceManager::create(
    std::shared_ptr<BluetoothEventBus> eventBus) {
    auto deviceManager = BlueZDeviceManager::create(eventBus);
    if (!deviceManager) {
        return nullptr;
    }
    return std::unique_ptr<BlueZBluetoothDeviceManager>(new BlueZBluetoothDeviceManager(deviceManager));
}

BlueZBluetoothDeviceManager::BlueZBluetoothDeviceManager(std::shared_ptr<BlueZDeviceManager> deviceManager) :
        m_deviceManager{deviceManager} {
}

BlueZBluetoothDeviceManager::~BlueZBluetoothDeviceManager() {
    ACSDK_DEBUG5(LX(__func__));
    m_deviceManager->shutdown();
};

std::shared_ptr<BluetoothEventBus> BlueZBluetoothDeviceManager::getEventBus() {
    return m_deviceManager->getEventBus();
}

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK
