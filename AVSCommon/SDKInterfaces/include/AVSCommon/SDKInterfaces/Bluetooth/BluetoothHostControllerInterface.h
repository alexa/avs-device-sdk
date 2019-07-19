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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_BLUETOOTHHOSTCONTROLLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_BLUETOOTHHOSTCONTROLLERINTERFACE_H_

#include <future>
#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace bluetooth {

/**
 * An interface to represent the HostControllerInterface on the local system.
 * This is responsible for Scanning and Discovery.
 */
class BluetoothHostControllerInterface {
public:
    /// Destructor
    virtual ~BluetoothHostControllerInterface() = default;

    /**
     * Getter for the MAC address of the adapter.
     *
     * @return The MAC address of the adapter.
     */
    virtual std::string getMac() const = 0;

    /**
     * Getter for the friendly name of the adapter.
     *
     * @return The friendly name of the adapter.
     */
    virtual std::string getFriendlyName() const = 0;

    /**
     * Getter for the discoverability of the device. This must wait until
     * any prior enterDiscoverableMode or exitDiscoverableMode methods have finished.
     *
     * @return Whether the device is currently discoverable by other devices.
     */
    virtual bool isDiscoverable() const = 0;

    /**
     * Set the adapter to become discoverable.
     *
     * @return Indicates whether the operation was successful.
     */
    virtual std::future<bool> enterDiscoverableMode() = 0;

    /**
     * Set the adapter to become non-discoverable.
     *
     * @return Indicates whether the operation was successful.
     */
    virtual std::future<bool> exitDiscoverableMode() = 0;

    /**
     * Getter for the scanning state of the device. This must wait until
     * any prior startScan or stopScan methods have finished.
     *
     * @return Whether the device is currently scanning for other devices.
     */
    virtual bool isScanning() const = 0;

    /**
     * Set the adapter to start scanning.
     *
     * @return Indicates whether the operation was successful.
     */
    virtual std::future<bool> startScan() = 0;

    /**
     * Set the adapter to stop scanning.
     *
     * @return Indicates whether the operation was successful.
     */
    virtual std::future<bool> stopScan() = 0;
};

}  // namespace bluetooth
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_BLUETOOTHHOSTCONTROLLERINTERFACE_H_
