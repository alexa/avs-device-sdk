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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_BLUETOOTHDEVICEMANAGERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_BLUETOOTHDEVICEMANAGERINTERFACE_H_

#include <list>
#include <memory>

#include "AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceInterface.h"
#include "AVSCommon/SDKInterfaces/Bluetooth/BluetoothHostControllerInterface.h"
#include "AVSCommon/Utils/Bluetooth/BluetoothEventBus.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace bluetooth {

/**
 * This component is a starting point of any platform specific implementation of bluetooth functionality. It is
 * responsible for ownership of @c BluetoothDeviceInterface objects and @c BluetootHostController objects.
 *
 */
class BluetoothDeviceManagerInterface {
public:
    /**
     * Destructor
     */
    virtual ~BluetoothDeviceManagerInterface() = default;

    /**
     * Get @c BluetoothHostControllerInterface instance
     * @return Pointer to a @c BluetoothHostControllerInterface instance
     */
    virtual std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothHostControllerInterface>
    getHostController() = 0;

    /**
     * Get a list of devices the Host Controller is aware of. This list must contain:
     *
     * i) Paired devices.
     * ii) Devices found during the scanning process.
     */
    virtual std::list<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>>
    getDiscoveredDevices() = 0;

    /**
     * Get the @c BluetoothEventBus used by this device manager to post bluetooth related events.
     *
     * @return A @c BluetoothEventBus object associated with the device manager.
     */
    virtual std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> getEventBus() = 0;
};

}  // namespace bluetooth
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_BLUETOOTHDEVICEMANAGERINTERFACE_H_
