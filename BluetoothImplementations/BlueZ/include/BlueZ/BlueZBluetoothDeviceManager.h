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

#ifndef ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_BLUEZBLUETOOTHDEVICEMANAGER_H_
#define ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_BLUEZBLUETOOTHDEVICEMANAGER_H_

#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceManagerInterface.h>
#include <AVSCommon/Utils/Bluetooth/BluetoothEventBus.h>

#include "BlueZ/BlueZDeviceManager.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

/**
 * BlueZ simplementation of @c BluetoothDeviceManagerInterface. This class is required to allow only one instance
 * of @c BluetoothDeviceManagerInterface in the SDK.
 */
class BlueZBluetoothDeviceManager : public avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceManagerInterface {
public:
    /**
     * Factory method to create a class.
     *
     * @param eventBus @c BluethoothEvent typed @c EventBus to be used for event routing.
     * @return A new instance of BlueZBluetoothDeviceManager on success, nullptr otherwise.
     */
    static std::unique_ptr<BlueZBluetoothDeviceManager> create(
        std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> eventBus);

    virtual ~BlueZBluetoothDeviceManager() override;

    /// @name BluetoothDeviceManagerInterface Functions
    /// @{
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothHostControllerInterface> getHostController() override;
    std::list<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>> getDiscoveredDevices()
        override;
    std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> getEventBus() override;
    ///@}

private:
    /**
     * A constructor
     *
     * @param eventBus event bus to communicate with SDK components
     */
    BlueZBluetoothDeviceManager(std::shared_ptr<BlueZDeviceManager> deviceManager);

    /// Pointer to the internal implementation of BluetoothDeviceManagerInterface
    std::shared_ptr<BlueZDeviceManager> m_deviceManager;
};

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_BLUEZBLUETOOTHDEVICEMANAGER_H_
