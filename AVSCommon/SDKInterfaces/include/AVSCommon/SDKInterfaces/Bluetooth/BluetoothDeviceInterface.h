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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_BLUETOOTHDEVICEINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_BLUETOOTHDEVICEINTERFACE_H_

#include <future>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include <AVSCommon/SDKInterfaces/Bluetooth/Services/BluetoothServiceInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/SDPRecordInterface.h>
#include <AVSCommon/Utils/Bluetooth/MediaStreamingState.h>
#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace bluetooth {

/**
 * clang-format off
 * Represents the state of the device. The state diagram is as follows:
 *
 *   +------UNPAIRED-------------+
 *   |                           |
 *   +------UNPAIRED---+         |
 *   V                 |         |
 * FOUND -> PAIRED -> IDLE -> CONNECTED
 *                     ^             |
 *                     +DISCONNECTED-+
 * clang-format on
 */
enum class DeviceState {
    // A device has been discovered.
    FOUND,
    // [Transitional] The device has been unpaired.
    UNPAIRED,
    // [Transitional] The device has successfully paired.
    PAIRED,
    // A paired device.
    IDLE,
    // [Transitional] A device has successfully disconnected.
    DISCONNECTED,
    // A device that has successfully connected.
    CONNECTED
};

/**
 * Converts the @c DeviceState enum to a string.
 *
 * @param The @c DeviceState to convert.
 * @return A string representation of the @c DeviceState.
 */
inline std::string deviceStateToString(DeviceState state) {
    switch (state) {
        case DeviceState::FOUND:
            return "FOUND";
        case DeviceState::UNPAIRED:
            return "UNPAIRED";
        case DeviceState::PAIRED:
            return "PAIRED";
        case DeviceState::IDLE:
            return "IDLE";
        case DeviceState::DISCONNECTED:
            return "DISCONNECTED";
        case DeviceState::CONNECTED:
            return "CONNECTED";
    }

    return "UNKNOWN";
}

/**
 * Overload for the @c DeviceState enum. This will write the @c DeviceState as a string to the provided stream.
 *
 * @param An ostream to send the DeviceState as a string.
 * @param The @c DeviceState to convert.
 * @return The stream.
 */
inline std::ostream& operator<<(std::ostream& stream, const DeviceState state) {
    return stream << deviceStateToString(state);
}

/// Represents a Bluetooth Device.
class BluetoothDeviceInterface {
public:
    /**
     * Struct to represent a Bluetooth device meta data.
     */
    struct MetaData {
        /// The value of undefined class of the Bluetooth device.
        static const int UNDEFINED_CLASS_VALUE = 0;

        utils::Optional<int> vendorId;
        utils::Optional<int> productId;
        int classOfDevice;
        utils::Optional<int> vendorDeviceSigId;
        utils::Optional<std::string> vendorDeviceId;

        /**
         * Constructor
         * @param vendorId The vendor id.
         * @param productId The product id.
         * @param classOfDevice The class of device.
         * @param vendorDeviceSigId The vendor device SIG id.
         * @param vendorDeviceId The vendor device id.
         */
        MetaData(
            utils::Optional<int> vendorId,
            utils::Optional<int> productId,
            int classOfDevice,
            utils::Optional<int> vendorDeviceSigId,
            utils::Optional<std::string> vendorDeviceId) :
                vendorId(vendorId),
                productId(productId),
                classOfDevice(classOfDevice),
                vendorDeviceSigId(vendorDeviceSigId),
                vendorDeviceId(vendorDeviceId) {
        }
    };

    /// Destructor
    virtual ~BluetoothDeviceInterface() = default;

    /**
     * Getter for the MAC address.
     *
     * @return The MAC address of the Bluetooth Device.
     */
    virtual std::string getMac() const = 0;

    /**
     * Getter for the friendly name.
     *
     * @return The friendly name of the Bluetooth Device.
     */
    virtual std::string getFriendlyName() const = 0;

    /**
     * Getter for the @c DeviceState.
     *
     * @return The @c DeviceState of the current device.
     */
    virtual DeviceState getDeviceState() = 0;

    /**
     * Getter for the Bluetooth device metadata.
     *
     * @return the meta data of the Bluetooth device.
     */
    virtual MetaData getDeviceMetaData() = 0;

    /**
     * Getter for the paired state of the device. This should return
     * the state after any pending state changes have been resolved.
     *
     * @return A bool representing whether the device is paired.
     */
    virtual bool isPaired() = 0;

    /**
     * Initiate a pair with this device.
     *
     * @return Indicates whether pairing was successful.
     */
    virtual std::future<bool> pair() = 0;

    /**
     * Initiate an unpair with this device.
     *
     * @return Indicates whether the unpairing was successful.
     */
    virtual std::future<bool> unpair() = 0;

    /**
     * Getter for the paired state of the device. This should return
     * the state after any pending state changes have been resolved.
     *
     * @return A bool representing whether the device is connected.
     */
    virtual bool isConnected() = 0;

    /**
     * Initiate a connect with this device.
     *
     * @return Indicates whether connecting was successful.
     */
    virtual std::future<bool> connect() = 0;

    /**
     * Initiate a disconnect with this device.
     *
     * @return Indicates whether disconnect was successful.
     */
    virtual std::future<bool> disconnect() = 0;

    /**
     * Sets the pairing pin for the current pairing attempt.  PIN length can range from 4 to 16
     * alphanumeric characters, though most devices will only accept numeric characters in the PIN.
     * Expected call flow is:
     * pair() -> PIN request callback -> setPairingPin()
     *
     * @param pin BT pairing pin
     * @return Indicates whether pairing pin was set.
     */
    virtual bool setPairingPin(const std::string& pin) = 0;

    /// @return The Bluetooth Services that this device supports.
    virtual std::vector<std::shared_ptr<services::SDPRecordInterface>> getSupportedServices() = 0;

    /**
     * Get the Bluetooth service that this device supports.
     *
     * @param uuid the uuid of the Bluetooth Service.
     * @return A pointer to an instance of the @c BluetoothServiceInterface if supported, else a nullptr.
     */
    virtual std::shared_ptr<services::BluetoothServiceInterface> getService(std::string uuid) = 0;

    /// @return The current media streaming state of the BluetoothDevice if the device supports A2DP streaming.
    virtual utils::bluetooth::MediaStreamingState getStreamingState() = 0;

    /**
     * Toggle the profile of a device, which restricts the future connection/disconnection.
     *
     * @param enabled True if need to connect the certain profile, false to disconnect.
     * @param service The target profile to toggle.
     * @return A bool indicating success.
     */
    virtual bool toggleServiceConnection(
        bool enabled,
        std::shared_ptr<services::BluetoothServiceInterface> service) = 0;
};

}  // namespace bluetooth
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_BLUETOOTHDEVICEINTERFACE_H_
