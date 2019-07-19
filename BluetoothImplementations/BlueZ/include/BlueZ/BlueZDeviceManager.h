/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_BLUEZDEVICEMANAGER_H_
#define ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_BLUEZDEVICEMANAGER_H_

#include <list>
#include <map>
#include <mutex>

#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothHostControllerInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/A2DPSourceInterface.h>
#include <AVSCommon/Utils/Bluetooth/BluetoothEventBus.h>
#include <AVSCommon/Utils/Bluetooth/BluetoothEvents.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

#include "BlueZ/BlueZHostController.h"
#include "BlueZ/BlueZUtils.h"
#include "BlueZ/MPRISPlayer.h"
#include "BlueZ/PairingAgent.h"

#include <gio/gio.h>

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

class MediaEndpoint;
class BlueZBluetoothDevice;

/**
 * Internal BlueZ implementation of @c BluetoothDeviceManagerInterface.
 */
class BlueZDeviceManager
        : public avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceManagerInterface
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<BlueZDeviceManager> {
public:
    /**
     * Factory method to create a class.
     *
     * @param eventBus @c BluethoothEvent typed @c EventBus to be used for event routing.
     * @return A new instance of BlueZDeviceManager on success, nullptr otherwise.
     */
    static std::shared_ptr<BlueZDeviceManager> create(
        std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> eventBus);

    /**
     * Destructor.
     */
    virtual ~BlueZDeviceManager() override;

    /// @name BluetoothDeviceManagerInterface Functions
    /// @{

    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothHostControllerInterface> getHostController() override;
    std::list<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>> getDiscoveredDevices()
        override;

    ///@}

    // @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /**
     * Get the @c BluetoothEventBus used by this device manager to post bluetooth related events.
     *
     * @return A @c BluetoothEventBus object associated with the device manager.
     */
    std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> getEventBus();

    /**
     * Get the SINK @c MediaEndpoint associated with the device manager
     *
     * @return @c MediaEndpoint instance
     */
    std::shared_ptr<MediaEndpoint> getMediaEndpoint();

    /**
     * Get the DBus object path of the current bluetooth hardware adapter used by this device manager.
     *
     * @return DBus object path of the current bluetooth hardware adapter used by this device manager.
     */
    std::string getAdapterPath() const;

private:
    /**
     * A constructor
     *
     * @param eventBus event bus to communicate with SDK components
     */
    explicit BlueZDeviceManager(const std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus>& eventBus);

    /**
     * Method used to initialize a fresh @c BlueZDeviceManager object before returning it by @c create() function.
     *
     * @return true on success
     */
    bool init();

    /**
     * Initialize A2DP streaming related components.
     *
     * @return true on success
     */
    bool initializeMedia();

    /**
     * Finalize A2DP streaming related components.
     *
     * @return true on success
     */
    bool finalizeMedia();

    /**
     * Helper method to create a @c BlueZBluetoothDevice class instance from DBus object provided by BlueZ.
     *
     * @param objectPath DBus object path of the device
     * @param dbusObject GLib variant containing the DBus object with the device description
     * @return Pointer to a new instance of @c BlueZBluetoothDevice
     */
    std::shared_ptr<BlueZBluetoothDevice> addDeviceFromDBusObject(const char* objectPath, GVariant* dbusObject);

    /**
     * DBus callback called when BlueZ service has a new interface implemented by an object in a DBus object tree.
     *
     * @param conn DBus connection
     * @param sender_name The sender service name
     * @param object_path Path of the sender object
     * @param interface_name Name of the interface that sends the signal
     * @param signal_name Name of the signal. "InterfacesAdded" in this case.
     * @param parameters Parameters of the signal, containing the description of all added interfaces.
     * @param data User data associated with signal
     */
    static void interfacesAddedCallback(
        GDBusConnection* conn,
        const gchar* sender_name,
        const gchar* object_path,
        const gchar* interface_name,
        const gchar* signal_name,
        GVariant* parameters,
        gpointer data);

    /**
     * DBus callback called when BlueZ service loses an interface implementation
     *
     * @param conn DBus connection
     * @param sender_name The sender service name
     * @param object_path Path of the sender object
     * @param interface_name Name of the interface that sends the signal
     * @param signal_name Name of the signal. "InterfacesRemoved" in this case.
     * @param parameters Parameters of the signal, containing the description of the removed interfaces.
     * @param data User data associated with signal
     */
    static void interfacesRemovedCallback(
        GDBusConnection* conn,
        const gchar* sender_name,
        const gchar* object_path,
        const gchar* interface_name,
        const gchar* signal_name,
        GVariant* parameters,
        gpointer data);

    /**
     * DBus callback called when BlueZ service has properties changed in one of its objects.
     *
     * @param conn DBus connection
     * @param sender_name The sender service name
     * @param object_path Path of the sender object
     * @param interface_name Name of the interface that sends the signal
     * @param signal_name Name of the signal. "PropertiesChanged" in this case.
     * @param parameters Parameters of the signal, containing the description of all properties changed
     * @param data User data associated with signal
     */
    static void propertiesChangedCallback(
        GDBusConnection* conn,
        const gchar* sender_name,
        const gchar* object_path,
        const gchar* interface_name,
        const gchar* signal_name,
        GVariant* parameters,
        gpointer data);

    /**
     * Add device to the list of known devices. This method is thread safe.
     *
     * @param devicePath Object path of the device.
     * @param device Device to be added.
     */
    void addDevice(const char* devicePath, std::shared_ptr<BlueZBluetoothDevice> device);

    /**
     * Remove device from the list of know devices. This method is thread safe.
     *
     * @param devicePath Object path of the device.
     */
    void removeDevice(const char* devicePath);

    /**
     * Notifies @c EventBus listeners of a new device added.
     *
     * @param device Pointer to a device.
     */
    void notifyDeviceAdded(std::shared_ptr<BlueZBluetoothDevice> device);

    /**
     * Handles the new interface being added to BlueZ objects.
     *
     * @param objectPath Object path for the object implementing new interfaces.
     * @param interfacesChangedMap Variant with the interfaces changes.
     */
    void onInterfaceAdded(const char* objectPath, ManagedGVariant& interfacesChangedMap);

    /**
     * Handles the removal of the DBus interfaces. In our case it also means that the object is deleted.
     *
     * @param objectPath Path of the object.
     */
    void onInterfaceRemoved(const char* objectPath);

    /**
     * Handles the property values changes in the Adapter.
     *
     * @param path DBus path of the object whose properties are changed.
     * @param changesMap Variant containing the changes description.
     */
    void onAdapterPropertyChanged(const std::string& path, const GVariantMapReader& changesMap);

    /**
     * Handles the property values changes in BlueZ objects.
     *
     * @param path DBus path of the object whose properties are changed.
     * @param changesMap Variant containing the changes description.
     */
    void onDevicePropertyChanged(const std::string& path, const GVariantMapReader& changesMap);

    /**
     * Handles the A2DP media stream properties changes.
     *
     * @param path Path of the device owning the media stream.
     * @param changesMap Variant with the changes description.
     */
    void onMediaStreamPropertyChanged(const std::string& path, const GVariantMapReader& changesMap);

    /**
     * Initializes the @c BluezHostController instance.
     *
     * @return A new @c BluezHostController instance.
     */
    std::shared_ptr<BlueZHostController> initializeHostController();

    /**
     * Handles the object's properties changes.
     *
     * @param propertyOwner Interface owning the properties being changed.
     * @param objectPath DBus path to the object implementing the interface.
     * @param changesMap Variant containing changes description.
     */
    void onPropertiesChanged(
        const std::string& propertyOwner,
        const std::string& objectPath,
        const GVariantMapReader& changesMap);

    /**
     * Retrieves the current state from BlueZ service and updates internal state accordingly.
     */
    bool getStateFromBlueZ();

    /**
     * Get known device by its DBus object path.
     *
     * @param path Object path of the device.
     * @return An instance of @c BlueZBluetoothDevice or nullptr if none found.
     */
    std::shared_ptr<BlueZBluetoothDevice> getDeviceByPath(const std::string& path) const;

    /// Thread procedure to setup and handle GLib events
    void mainLoopThread();

    /// DBus object path of the hardware bluetooth adapter used by device manager
    std::string m_adapterPath;

    /// DBus proxy for ObjectManager interface of the BlueZ service
    std::shared_ptr<DBusProxy> m_objectManagerProxy;

    /// DBus proxy for Media1 interface of the BlueZ service
    std::shared_ptr<DBusProxy> m_mediaProxy;

    /// List of known devices.
    std::map<std::string, std::shared_ptr<BlueZBluetoothDevice>> m_devices;

    /// SINK media endpoint used for audio streaming.
    std::shared_ptr<MediaEndpoint> m_mediaEndpoint;

    /// Pairing agent used for device pairing.
    std::shared_ptr<PairingAgent> m_pairingAgent;

    /// DBus MediaPlayer used for AVRCPTarget.
    std::shared_ptr<MPRISPlayer> m_mediaPlayer;

    /// The EventBus to communicate with SDK components.
    std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> m_eventBus;

    // TODO Having a separate thread for event listening is wasteful. Can optimize with a BlueZ bus listener.
    /// Event loop to listen for signals.
    GMainLoop* m_eventLoop;

    /// GLib context to run event loop in.
    GMainContext* m_workerContext;

    /// DBus connection
    std::shared_ptr<DBusConnection> m_connection;

    /// Current streaming state
    avsCommon::utils::bluetooth::MediaStreamingState m_streamingState;

    /// Mutex to synchronize known device list access.
    mutable std::mutex m_devicesMutex;

    /// A host controller instance used by the device manager.
    std::shared_ptr<BlueZHostController> m_hostController;

    /// Promise to hold the result of a glib's main loop thread initialization.
    std::promise<bool> m_mainLoopInitPromise;

    /// Thread to run event listener on.
    std::thread m_eventThread;
};

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_BLUEZDEVICEMANAGER_H_
