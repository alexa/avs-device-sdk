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

#ifndef ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_BLUEZHOSTCONTROLLER_H_
#define ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_BLUEZHOSTCONTROLLER_H_

#include <string>
#include <memory>
#include <mutex>

#include <gio/gio.h>

#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothHostControllerInterface.h>
#include <AVSCommon/Utils/MacAddressString.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include "BlueZ/BlueZUtils.h"
#include "BlueZ/DBusPropertiesProxy.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

/// An implementation of the @c BluetoothHostControllerInterface using BlueZ.
class BlueZHostController : public avsCommon::sdkInterfaces::bluetooth::BluetoothHostControllerInterface {
public:
    /// @name BluetoothHostControllerInterface Functions
    /// @{
    virtual ~BlueZHostController() = default;

    std::string getMac() const override;
    std::string getFriendlyName() const override;

    bool isDiscoverable() const override;
    std::future<bool> enterDiscoverableMode() override;
    std::future<bool> exitDiscoverableMode() override;

    bool isScanning() const override;
    std::future<bool> startScan() override;
    std::future<bool> stopScan() override;
    /// @}

    /**
     * Creates an instance of the BlueZHostController.
     *
     * @param adapterObjectPath The object path of the adapter.
     * @return An instance if successful, else a nullptr.
     */
    static std::unique_ptr<BlueZHostController> create(const std::string& adapterObjectPath);

    /**
     * A function for BlueZDeviceManager to alert devices when its property has changed. This is to avoid
     * having multiple objects subscribing to DBus events.
     *
     * @param changesMap A map containing the property changes of the device.
     */
    void onPropertyChanged(const GVariantMapReader& changesMap);

private:
    /**
     * Constructor
     *
     * @param adapterObjectPath The object path of the adapter.
     */
    BlueZHostController(const std::string& adapterObjectPath);

    /**
     * Perform any needed initialization.
     *
     * @return A bool indicating the success of the operation.
     */
    bool init();

    /**
     * Helper function to adjust the discoverability of the adapter.
     * In order to satisfy the aysnchronous nature of the interface,
     * a std::package_task is used to construct a future. The function
     * runs on the caller thread.
     *
     * @param discoverable Whether the adapter should be discoverable.
     * @return A bool indicating success of the operation.
     */
    std::future<bool> setDiscoverable(bool discoverable);

    /**
     * Helper function to adjust the scanning state of the adapter.
     * In order to satisfy the aysnchronous nature of the interface,
     * a std::package_task is used to construct a future. The function
     * runs on the caller thread.
     *
     * @param on Whether the adapter should be scanning.
     *
     * @return A bool indicating success of the operation.
     */
    std::future<bool> changeScanState(bool scanning);

    /// The BlueZ object path of the adapter.
    std::string m_adapterObjectPath;

    /// The MAC address of the adapter.
    std::unique_ptr<avsCommon::utils::MacAddressString> m_mac;

    /**
     * A mutex to protect calls to the adapter. The mutex is marked
     * as mutable because we need to lock inside const functions
     * @c isDiscoverable() and @c isScanning().
     */
    mutable std::mutex m_adapterMutex;

    /// The friendly name of the adapter. This is what will appear when devices perform queries.
    std::string m_friendlyName;

    /// A proxy of the Adapter Properties interface.
    std::shared_ptr<DBusPropertiesProxy> m_adapterProperties;

    /// A proxy of the Adapter interface.
    std::shared_ptr<DBusProxy> m_adapter;
};

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_BLUEZHOSTCONTROLLER_H_
