/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_BLUETOOTHDEVICEOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_BLUETOOTHDEVICEOBSERVERINTERFACE_H_

#include <string>
#include <unordered_set>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace bluetooth {

/**
 * This interface allows a derived class to know when a bluetooth device is connected or disconnected.
 */
class BluetoothDeviceObserverInterface {
public:
    /**
     * The observable attributes of the bluetooth device.
     */
    struct DeviceAttributes {
        /**
         * Constructor.
         */
        DeviceAttributes() = default;

        /// The name of the active bluetooth device
        std::string name;

        /// The bluetooth services this device supports.
        std::unordered_set<std::string> supportedServices;
    };

    /**
     * Destructor.
     */
    virtual ~BluetoothDeviceObserverInterface() = default;

    /**
     * Used to notify the observer when an active bluetooth device is connected.
     *
     * @param attributes The @c DeviceAttributes of the active bluetooth device.
     */
    virtual void onActiveDeviceConnected(const DeviceAttributes& attributes) = 0;

    /**
     * Used to notify the observer when an active bluetooth device is disconnected.
     *
     * @param attributes The @c DeviceAttributes of the active bluetooth device.
     */
    virtual void onActiveDeviceDisconnected(const DeviceAttributes& attributes) = 0;
};

}  // namespace bluetooth
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_BLUETOOTHDEVICEOBSERVERINTERFACE_H_
