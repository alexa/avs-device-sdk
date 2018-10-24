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

#ifndef ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_BLUEZBLUETOOTHDEVICE_H_
#define ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_BLUEZBLUETOOTHDEVICE_H_

#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <gio/gio.h>

#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/Bluetooth/SDPRecords.h>
#include "BlueZ/DBusPropertiesProxy.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

class BlueZDeviceManager;

/// A BlueZ implementation of the @c BluetoothDeviceInterface.
class BlueZBluetoothDevice
        : public avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface
        , public std::enable_shared_from_this<BlueZBluetoothDevice> {
public:
    enum class BlueZDeviceState {
        /// A device has been discovered.
        FOUND,
        /// [Transitional] The device has been unpaired.
        UNPAIRED,
        /// [Transitional] The device has successfully paired.
        PAIRED,
        /// A paired device.
        IDLE,
        /// [Transitional] A device has successfully disconnected.
        DISCONNECTED,
        /// A device that has successfully connected.
        CONNECTED,
        /**
         * When we fail a connect call as a result of the device not being correctly authenticated.
         * BlueZ will contiously attempt to connect, and toggle the connect property
         * on and off. This represents that state.
         */
        CONNECTION_FAILED
    };

    /// @name BluetoothDeviceInterface Functions
    /// @{
    ~BlueZBluetoothDevice() override;

    std::string getMac() const override;
    std::string getFriendlyName() const override;
    avsCommon::sdkInterfaces::bluetooth::DeviceState getDeviceState() override;

    bool isPaired() override;
    std::future<bool> pair() override;
    std::future<bool> unpair() override;

    bool isConnected() override;
    std::future<bool> connect() override;
    std::future<bool> disconnect() override;

    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::services::SDPRecordInterface>>
    getSupportedServices() override;
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::services::A2DPSinkInterface> getA2DPSink() override;
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::services::A2DPSourceInterface> getA2DPSource() override;
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::services::AVRCPTargetInterface> getAVRCPTarget() override;
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::services::AVRCPControllerInterface> getAVRCPController()
        override;
    /// @}

    /**
     * Creates an instance of the BlueZBluetoothDevice.
     *
     * @param mac The MAC address.
     * @param objectPath The DBus object path.
     * @param deviceManager The associated deviceManager.
     * @return A pointer to a BlueZBluetoothDevice if successful, else a nullptr.
     */
    static std::shared_ptr<BlueZBluetoothDevice> create(
        const std::string& mac,
        const std::string& objectPath,
        std::shared_ptr<BlueZDeviceManager> deviceManager);

    /**
     * Gets the DBus object path of the device.
     *
     * @return The DBus objectpath of the device.
     *
     */
    std::string getObjectPath() const;

    /**
     * A function for BlueZDeviceManager to alert the BlueZ device when its property has changed. This is to avoid
     * having multiple objects subscribing to DBus events.
     *
     * @param changesMap A map containing the property changes of the device.
     */
    void onPropertyChanged(const GVariantMapReader& changesMap);

private:
    /// Constructor.
    BlueZBluetoothDevice(
        const std::string& mac,
        const std::string& objectPath,
        std::shared_ptr<BlueZDeviceManager> deviceManager);

    /**
     * Initialization function.
     *
     * @return Whether the operation was successful.
     */
    bool init();

    /**
     * Query BlueZ and update @c m_friendlyName of the device.
     *
     * @return Whether the operation was successful. The return does not incidate whether the friendlyName changed.
     */
    bool updateFriendlyName();

    /**
     * Helper function to extract and parse the supported services for this device from the BlueZ device property.
     *
     * @return A set of supported services. Returns empty if none were found.
     */
    std::unordered_set<std::string> getServiceUuids();

    /**
     * Helper function to extract and parse the supported services for this device based on a variant.
     *
     * @param array A GVariant containg the array of uuids.
     * due to limitations from C functions that utilize this param.
     * @return A set of supported services.
     */
    std::unordered_set<std::string> getServiceUuids(GVariant* array);

    /**
     * Creates @c BluetoothServiceInterface for supported services based on the uuids.
     *
     * @return Whether the operation was successful.
     */
    bool initializeServices(const std::unordered_set<std::string>& uuids);

    /**
     * Pair with this device.
     *
     * @return Whether the operation was successful.
     */
    bool executePair();

    /**
     * Unpair with this device. BlueZ doesn't expose an unpair method, so we must request that
     * the BlueZDeviceManager delete the device.
     *
     * @return Whether the operation was successful.
     */
    bool executeUnpair();

    /**
     * Connect with this device.
     *
     * @return Whether the operation was successful.
     */
    bool executeConnect();

    /**
     * Disconnect with this device.
     *
     * @return Whether the operation was successful.
     */
    bool executeDisconnect();

    /**
     * Helper function to check if paired. This uses @c m_deviceState and does not
     * query the stack.
     *
     * @return Whether the device is paired.
     */
    bool executeIsPaired();

    /**
     * Helper function to check if connected. This uses @c m_deviceState and does not
     * query the stack.
     *
     * @return Whether the device is connected.
     */
    bool executeIsConnected();

    /**
     * Queries BlueZ for the value of the property as reported by the adapter.
     *
     * @param name The name of the property.
     * @param[out] value The value of the property.
     * @return A bool indicating success.
     */
    bool queryDeviceProperty(const std::string& name, bool* value);

    /**
     * A function to convert the BlueZDeviceState to the normal DeviceState.
     */
    avsCommon::sdkInterfaces::bluetooth::DeviceState convertToDeviceState(BlueZDeviceState bluezDeviceState);

    /**
     * Transition to new state and optionally notify listeners.
     *
     * @param newState The new state to transition to.
     * @param sendEvent Whether to alert listeners.
     */
    void transitionToState(BlueZDeviceState newState, bool sendEvent);

    /**
     * Helper function to check if a service exists in the @c m_servicesMap.
     *
     * @param uuid The service UUID to check.
     * @return A bool indicating whether it exists.
     */
    bool serviceExists(const std::string& uuid);

    /**
     * Helper function to insert service into @c m_servicesMap.
     *
     * @param service The service to insert.
     * @return bool Indicates whether insertion was successful.
     */
    bool insertService(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::services::BluetoothServiceInterface> service);

    /**
     * Helper function to insert service into @c m_servicesMap.
     *
     * @tparam ServiceType The type of the @c BluetoothServiceInterface.
     * @return The instance of the service if successful, else nullptr.
     */
    template <typename ServiceType>
    std::shared_ptr<ServiceType> getService();

    /// Proxy to interact with the org.bluez.Device1 interface.
    std::shared_ptr<DBusProxy> m_deviceProxy;

    /// Proxy to interact with the org.bluez.Device1 interface.
    std::shared_ptr<DBusPropertiesProxy> m_propertiesProxy;

    /// The MAC address.
    const std::string m_mac;

    /// The DBus object path.
    const std::string m_objectPath;

    /// The friendly name.
    std::string m_friendlyName;

    /// Mutex to protect access to @c m_servicesMap.
    std::mutex m_servicesMapMutex;

    /// A map of UUID to services.
    std::unordered_map<
        std::string,
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::services::BluetoothServiceInterface>>
        m_servicesMap;

    /// The current state of the device.
    BlueZDeviceState m_deviceState;

    /// The associated @c BlueZDeviceManager.
    std::shared_ptr<BlueZDeviceManager> m_deviceManager;

    /// An executor used for serializing requests on the Device's own thread of execution.
    avsCommon::utils::threading::Executor m_executor;
};

/**
 * Converts the @c DeviceState enum to a string.
 *
 * @param The @c DeviceState to convert.
 * @return A string representation of the @c BlueZDeviceState.
 */
inline std::string deviceStateToString(BlueZBluetoothDevice::BlueZDeviceState state) {
    switch (state) {
        case BlueZBluetoothDevice::BlueZDeviceState::FOUND:
            return "FOUND";
        case BlueZBluetoothDevice::BlueZDeviceState::UNPAIRED:
            return "UNPAIRED";
        case BlueZBluetoothDevice::BlueZDeviceState::PAIRED:
            return "PAIRED";
        case BlueZBluetoothDevice::BlueZDeviceState::IDLE:
            return "IDLE";
        case BlueZBluetoothDevice::BlueZDeviceState::DISCONNECTED:
            return "DISCONNECTED";
        case BlueZBluetoothDevice::BlueZDeviceState::CONNECTED:
            return "CONNECTED";
        case BlueZBluetoothDevice::BlueZDeviceState::CONNECTION_FAILED:
            return "CONNECTION_FAILED";
    }

    return "UNKNOWN";
}

/**
 * Overload for the @c BlueZDeviceState enum. This will write the @c BlueZDeviceState as a string to the provided
 * stream.
 *
 * @param An ostream to send the DeviceState as a string.
 * @param The @c DeviceState to convert.
 * @return The stream.
 */
inline std::ostream& operator<<(std::ostream& stream, const BlueZBluetoothDevice::BlueZDeviceState& state) {
    return stream << deviceStateToString(state);
}

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_BLUEZBLUETOOTHDEVICE_H_
