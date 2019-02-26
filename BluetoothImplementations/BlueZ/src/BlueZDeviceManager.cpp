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

#include <algorithm>
#include <cstring>
#include <string>
#include <unordered_map>

#include <AVSCommon/Utils/Bluetooth/BluetoothEvents.h>
#include <AVSCommon/Utils/Bluetooth/SDPRecords.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h>

#include "BlueZ/BlueZBluetoothDevice.h"
#include "BlueZ/BlueZConstants.h"
#include "BlueZ/BlueZDeviceManager.h"
#include "BlueZ/BlueZHostController.h"
#include "BlueZ/MediaEndpoint.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

using namespace avsCommon::sdkInterfaces::bluetooth;
using namespace avsCommon::sdkInterfaces::bluetooth::services;
using namespace avsCommon::utils;
using namespace avsCommon::utils::bluetooth;

/// MediaTransport1 interface property "state"
static const char* MEDIATRANSPORT_PROPERTY_STATE = "State";

/// DBus object root path
static const char* OBJECT_PATH_ROOT = "/";

/// BlueZ codec id for SBC
static const int A2DP_CODEC_SBC = 0x00;

/// Support all available capabilities for this byte
static const int SBC_CAPS_ALL = 0xff;

/// Minimum bitpool size supported
static const int SBC_BITPOOL_MIN = 2;

/// Maxmimum bitpool size supported
static const int SBC_BITPOOL_MAX = 64;

/**
 * DBus object path for the SINK media endpoint
 */
static const char* DBUS_ENDPOINT_PATH_SINK = "/com/amazon/alexa/sdk/sinkendpoint";

/**
 * BlueZ A2DP streaming state when audio data is streaming from the device, but we still did not acquire the file
 * descriptor.
 */
static const std::string STATE_PENDING = "pending";

/**
 * BlueZ A2DP streaming state when no audio data is streaming from the device.
 */
static const std::string STATE_IDLE = "idle";

/**
 * BlueZ A2DP streaming state when audio data is streaming from the device and we are reading it.
 */
static const std::string STATE_ACTIVE = "active";

/// String to identify log entries originating from this file.
static const std::string TAG{"BlueZDeviceManager"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<BlueZDeviceManager> BlueZDeviceManager::create(
    std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> eventBus) {
    ACSDK_DEBUG5(LX(__func__));

    if (!eventBus) {
        ACSDK_ERROR(LX("createFailed").d("reason", "eventBus is nullptr"));
        return nullptr;
    }

    std::shared_ptr<BlueZDeviceManager> deviceManager(new BlueZDeviceManager(eventBus));
    if (!deviceManager->init()) {
        return nullptr;
    }

    return deviceManager;
}

bool BlueZDeviceManager::init() {
    ACSDK_DEBUG5(LX(__func__));

    ACSDK_DEBUG5(LX("Creating connection..."));
    m_connection = DBusConnection::create();
    if (nullptr == m_connection) {
        ACSDK_ERROR(LX("initFailed").d("reason", "Failed to create DBus connection"));
        return false;
    }

    ACSDK_DEBUG5(LX("Creating ObjectManagerProxy..."));
    // Create ObjectManager proxy used to find Adapter to use and a list of known Devices
    m_objectManagerProxy = DBusProxy::create(BlueZConstants::OBJECT_MANAGER_INTERFACE, OBJECT_PATH_ROOT);
    if (nullptr == m_objectManagerProxy) {
        ACSDK_ERROR(LX("initFailed").d("Failed to create ObjectManager proxy", ""));
        return false;
    }

    ACSDK_DEBUG5(LX("Retrieving BlueZ state..."));
    if (!getStateFromBlueZ()) {
        return false;
    }

    ACSDK_DEBUG5(LX("Initializing Host Controller..."));
    m_hostController = initializeHostController();
    if (!m_hostController) {
        ACSDK_ERROR(LX("initFailed").d("reason", "nullHostController"));
        return false;
    }

    m_mediaProxy = DBusProxy::create(BlueZConstants::BLUEZ_MEDIA_INTERFACE, m_adapterPath);
    if (nullptr == m_mediaProxy) {
        ACSDK_ERROR(LX("initializeMediaFailed").d("reason", "Failed to create Media proxy"));
        return false;
    }

    m_workerContext = g_main_context_new();
    if (nullptr == m_workerContext) {
        ACSDK_ERROR(LX("initFailed").d("reason", "Failed to create glib main context"));
        return false;
    }

    m_eventLoop = g_main_loop_new(m_workerContext, false);
    if (nullptr == m_eventLoop) {
        ACSDK_ERROR(LX("initFailed").d("reason", "Failed to create glib main loop"));
        return false;
    }

    m_eventThread = std::thread(&BlueZDeviceManager::mainLoopThread, this);

    if (!m_mainLoopInitPromise.get_future().get()) {
        ACSDK_ERROR(LX("initFailed").d("reason", "failed to initialize glib main loop"));
        return false;
    }

    ACSDK_DEBUG5(LX("BlueZDeviceManager initialized..."));

    BluetoothDeviceManagerInitializedEvent event;
    m_eventBus->sendEvent(event);

    return true;
}

bool BlueZDeviceManager::initializeMedia() {
    // Create Media interface proxy to register MediaEndpoint
    m_mediaEndpoint = std::make_shared<MediaEndpoint>(m_connection, DBUS_ENDPOINT_PATH_SINK);

    if (!m_mediaEndpoint->registerWithDBus()) {
        ACSDK_ERROR(LX("initializeMediaFailed").d("reason", "registerEndpointFailed"));
        return false;
    }

    ManagedGError error;

    // Creating a dictionary
    GVariantBuilder* b;
    GVariantBuilder* capBuilder;
    capBuilder = g_variant_builder_new(G_VARIANT_TYPE("ay"));

    // Channel Modes: Mono DualChannel Stereo JointStereo
    // Frequencies: 16Khz 32Khz 44.1Khz 48Khz
    g_variant_builder_add(capBuilder, "y", SBC_CAPS_ALL);

    // Subbands: 4 8
    // Blocks: 4 8 12 16
    // Allocation mode: both
    g_variant_builder_add(capBuilder, "y", SBC_CAPS_ALL);

    // Bitpool Range: 2-64
    g_variant_builder_add(capBuilder, "y", SBC_BITPOOL_MIN);
    g_variant_builder_add(capBuilder, "y", SBC_BITPOOL_MAX);

    GVariant* caps = g_variant_builder_end(capBuilder);

    b = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));

    std::string a2dpSinkUuid = A2DPSinkInterface::UUID;
    std::transform(a2dpSinkUuid.begin(), a2dpSinkUuid.end(), a2dpSinkUuid.begin(), ::toupper);

    g_variant_builder_add(b, "{sv}", "UUID", g_variant_new_string(a2dpSinkUuid.c_str()));
    g_variant_builder_add(b, "{sv}", "Codec", g_variant_new_byte(A2DP_CODEC_SBC));
    g_variant_builder_add(b, "{sv}", "Capabilities", caps);

    // Second parameter
    GVariant* parameters = g_variant_builder_end(b);

    m_mediaProxy->callMethod(
        "RegisterEndpoint", g_variant_new("(o@a{sv})", DBUS_ENDPOINT_PATH_SINK, parameters), error.toOutputParameter());

    if (error.hasError()) {
        ACSDK_ERROR(LX("initializeMediaFailed").d("Failed to register MediaEndpoint", ""));
        return false;
    }

    return true;
}

std::shared_ptr<BlueZBluetoothDevice> BlueZDeviceManager::getDeviceByPath(const std::string& path) const {
    {
        std::lock_guard<std::mutex> guard(m_devicesMutex);
        auto iter = m_devices.find(path);
        if (iter != m_devices.end()) {
            return iter->second;
        }
    }

    ACSDK_ERROR(LX("getDeviceByPathFailed").d("reason", "deviceNotFound").d("path", path));

    return nullptr;
}

void BlueZDeviceManager::onMediaStreamPropertyChanged(const std::string& path, const GVariantMapReader& changesMap) {
    const std::string FD_KEY = "/fd";

    // Get device path without the /fd<number>
    auto pos = path.rfind(FD_KEY);
    if (std::string::npos == pos) {
        ACSDK_ERROR(LX(__func__).d("reason", "unexpectedPath").d("path", path));
        return;
    }

    std::string devicePath = path.substr(0, pos);

    auto device = getDeviceByPath(devicePath);
    if (!device) {
        ACSDK_ERROR(LX(__func__).d("reason", "deviceDoesNotExist").d("path", devicePath));
        return;
    }

    auto mediaTransportProperties = DBusPropertiesProxy::create(path);
    if (!mediaTransportProperties) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullPropertiesProxy").d("path", path));
        return;
    }

    std::string uuid;
    if (!mediaTransportProperties->getStringProperty(BlueZConstants::BLUEZ_MEDIATRANSPORT_INTERFACE, "UUID", &uuid)) {
        ACSDK_ERROR(LX(__func__).d("reason", "getPropertyFailed"));
        return;
    }

    std::transform(uuid.begin(), uuid.end(), uuid.begin(), ::tolower);
    ACSDK_DEBUG5(LX(__func__).d("mediaStreamUuid", uuid));

    char* newStateStr;
    avsCommon::utils::bluetooth::MediaStreamingState newState;
    if (changesMap.getCString(MEDIATRANSPORT_PROPERTY_STATE, &newStateStr)) {
        ACSDK_DEBUG5(LX("Media transport state changed").d("newState", newStateStr));

        if (STATE_ACTIVE == newStateStr) {
            newState = avsCommon::utils::bluetooth::MediaStreamingState::ACTIVE;
        } else if (STATE_PENDING == newStateStr) {
            newState = avsCommon::utils::bluetooth::MediaStreamingState::PENDING;
        } else if (STATE_IDLE == newStateStr) {
            newState = avsCommon::utils::bluetooth::MediaStreamingState::IDLE;
        } else {
            ACSDK_ERROR(LX("onMediaStreamPropertyChangedFailed").d("Unknown state", newStateStr));
            return;
        }
    }

    if (A2DPSourceInterface::UUID == uuid) {
        auto sink = device->getA2DPSink();
        if (!sink) {
            ACSDK_ERROR(LX(__func__).d("reason", "nullSink"));
            return;
        }

        MediaStreamingStateChangedEvent event(newState, A2DPRole::SOURCE, device);
        m_eventBus->sendEvent(event);
        return;
    } else if (A2DPSinkInterface::UUID == uuid) {
        if (path != m_mediaEndpoint->getStreamingDevicePath()) {
            ACSDK_DEBUG5(LX(__func__)
                             .d("reason", "pathMismatch")
                             .d("path", path)
                             .d("streamingDevicePath", m_mediaEndpoint->getStreamingDevicePath()));
            return;
        }

        if (m_streamingState == newState) {
            return;
        }

        m_streamingState = newState;
        m_mediaEndpoint->onMediaTransportStateChanged(newState, path);

        MediaStreamingStateChangedEvent event(newState, bluetooth::A2DPRole::SINK, device);
        m_eventBus->sendEvent(event);
    }
}

void BlueZDeviceManager::onDevicePropertyChanged(const std::string& path, const GVariantMapReader& changesMap) {
    ACSDK_DEBUG7(LX(__func__).d("path", path));
    std::shared_ptr<BlueZBluetoothDevice> device = getDeviceByPath(path);
    if (!device) {
        ACSDK_ERROR(LX("onDevicePropertyChangedFailed").d("reason", "device not found"));
        return;
    }

    device->onPropertyChanged(changesMap);
    ACSDK_DEBUG7(LX(__func__).d("finished", "ok"));
}

void BlueZDeviceManager::onAdapterPropertyChanged(const std::string& path, const GVariantMapReader& changesMap) {
    ACSDK_DEBUG7(LX(__func__).d("path", path));
    if (!m_hostController) {
        ACSDK_ERROR(LX("onAdapterPropertyChangedFailed").d("reason", "nullHostController"));
        return;
    }

    m_hostController->onPropertyChanged(changesMap);
}

std::string BlueZDeviceManager::getAdapterPath() const {
    return m_adapterPath;
}

void BlueZDeviceManager::interfacesAddedCallback(
    GDBusConnection* conn,
    const gchar* sender_name,
    const gchar* object_path,
    const gchar* interface_name,
    const gchar* signal_name,
    GVariant* parameters,
    gpointer deviceManager) {
    ACSDK_DEBUG5(LX(__func__));

    if (nullptr == parameters) {
        ACSDK_ERROR(LX("interfacesAddedCallbackFailed").d("reason", "parameters are null"));
        return;
    }

    if (nullptr == deviceManager) {
        ACSDK_ERROR(LX("interfacesAddedCallbackFailed").d("reason", "deviceManager is null"));
        return;
    }

    GVariantTupleReader tupleReader(parameters);
    char* addedObjectPath = tupleReader.getObjectPath(0);
    ManagedGVariant interfacesChangedMap = tupleReader.getVariant(1);

    static_cast<BlueZDeviceManager*>(deviceManager)->onInterfaceAdded(addedObjectPath, interfacesChangedMap);
}

void BlueZDeviceManager::interfacesRemovedCallback(
    GDBusConnection* conn,
    const gchar* sender_name,
    const gchar* object_path,
    const gchar* interface_name,
    const gchar* signal_name,
    GVariant* variant,
    gpointer deviceManager) {
    ACSDK_DEBUG5(LX(__func__));

    char* interfaceRemovedPath;

    if (nullptr == variant) {
        ACSDK_ERROR(LX("interfacesRemovedCallbackFailed").d("reason", "variant is null"));
        return;
    }

    if (nullptr == deviceManager) {
        ACSDK_ERROR(LX("interfacesRemovedCallbackFailed").d("reason", "deviceManager is null"));
        return;
    }

    g_variant_get(variant, "(oas)", &interfaceRemovedPath, NULL);

    static_cast<BlueZDeviceManager*>(deviceManager)->onInterfaceRemoved(interfaceRemovedPath);
}

void BlueZDeviceManager::propertiesChangedCallback(
    GDBusConnection* conn,
    const gchar* sender_name,
    const gchar* object_path,
    const gchar* interface_name,
    const gchar* signal_name,
    GVariant* prop,
    gpointer deviceManager) {
    ACSDK_DEBUG5(LX(__func__));

    if (nullptr == prop) {
        ACSDK_ERROR(LX("propertiesChangedCallbackFailed").d("reason", "variant is null"));
        return;
    }

    if (nullptr == object_path) {
        ACSDK_ERROR(LX("propertiesChangedCallbackFailed").d("reason", "object_path is null"));
        return;
    }

    if (nullptr == deviceManager) {
        ACSDK_ERROR(LX("propertiesChangedCallbackFailed").d("reason", "deviceManager is null"));
        return;
    }

    ACSDK_DEBUG7(LX("Properties changed").d("objectPath", object_path));

    ACSDK_DEBUG9(LX("Details").d("", g_variant_print(prop, true)));

    char* propertyOwner;

    GVariantTupleReader tupleReader(prop);
    propertyOwner = tupleReader.getCString(0);
    ManagedGVariant propertyMapVariant = tupleReader.getVariant(1);

    GVariantMapReader mapReader(propertyMapVariant);
    static_cast<BlueZDeviceManager*>(deviceManager)->onPropertiesChanged(propertyOwner, object_path, mapReader);
}

void BlueZDeviceManager::onPropertiesChanged(
    const std::string& propertyOwner,
    const std::string& objectPath,
    const GVariantMapReader& changesMap) {
    if (BlueZConstants::BLUEZ_MEDIATRANSPORT_INTERFACE == propertyOwner) {
        onMediaStreamPropertyChanged(objectPath, changesMap);
    } else if (BlueZConstants::BLUEZ_DEVICE_INTERFACE == propertyOwner) {
        onDevicePropertyChanged(objectPath, changesMap);
    } else if (BlueZConstants::BLUEZ_ADAPTER_INTERFACE == propertyOwner) {
        onAdapterPropertyChanged(objectPath, changesMap);
    }
}

void BlueZDeviceManager::onInterfaceAdded(const char* objectPath, ManagedGVariant& interfacesChangedMap) {
    ACSDK_DEBUG7(LX(__func__).d("path", objectPath));
    ACSDK_DEBUG9(LX(__func__).d("Details", g_variant_print(interfacesChangedMap.get(), true)));

    GVariantMapReader mapReader(interfacesChangedMap.get());
    ManagedGVariant deviceInterfaceObject = mapReader.getVariant(BlueZConstants::BLUEZ_DEVICE_INTERFACE);
    if (deviceInterfaceObject.get() != nullptr) {
        std::shared_ptr<BlueZBluetoothDevice> device = addDeviceFromDBusObject(objectPath, deviceInterfaceObject.get());
        notifyDeviceAdded(device);
    }
}

void BlueZDeviceManager::onInterfaceRemoved(const char* objectPath) {
    ACSDK_DEBUG7(LX(__func__));
    removeDevice(objectPath);
}

void BlueZDeviceManager::addDevice(const char* devicePath, std::shared_ptr<BlueZBluetoothDevice> device) {
    ACSDK_DEBUG7(LX(__func__));
    if (nullptr == devicePath) {
        ACSDK_ERROR(LX("addDeviceFailed").d("reason", "devicePath is null"));
    }
    if (nullptr == device) {
        ACSDK_ERROR(LX("addDeviceFailed").d("reason", "device is null"));
    }

    {
        std::lock_guard<std::mutex> guard(m_devicesMutex);
        m_devices[devicePath] = device;
    }

    ACSDK_DEBUG7(
        LX("Device added").d("path", devicePath).d("mac", device->getMac()).d("alias", device->getFriendlyName()));
}

void BlueZDeviceManager::notifyDeviceAdded(std::shared_ptr<BlueZBluetoothDevice> device) {
    ACSDK_DEBUG7(LX(__func__));
    avsCommon::utils::bluetooth::DeviceDiscoveredEvent event(device);
    m_eventBus->sendEvent(event);
}

void BlueZDeviceManager::removeDevice(const char* devicePath) {
    if (nullptr == devicePath) {
        ACSDK_ERROR(LX("removeDeviceFailed").d("reason", "devicePath is null"));
    }
    ACSDK_DEBUG5(LX("Removing device").d("device path", devicePath));
    std::shared_ptr<BluetoothDeviceInterface> device;
    {
        std::lock_guard<std::mutex> guard(m_devicesMutex);
        auto it = m_devices.find(devicePath);
        if (it != m_devices.end()) {
            device = it->second;
            m_devices.erase(it);
        }
    }

    if (device) {
        avsCommon::utils::bluetooth::DeviceRemovedEvent event(device);
        m_eventBus->sendEvent(event);
    }
}

void BlueZDeviceManager::doShutdown() {
    ACSDK_DEBUG5(LX(__func__));

    {
        std::lock_guard<std::mutex> guard(m_devicesMutex);

        // Disconnect every device.
        for (auto iter : m_devices) {
            std::shared_ptr<BlueZBluetoothDevice> device = iter.second;
            device->disconnect().get();
        }

        m_devices.clear();
    }

    // Destroy all objects requiring m_connection first.
    finalizeMedia();
    m_pairingAgent.reset();
    m_mediaPlayer.reset();

    m_connection->close();
    if (m_eventLoop) {
        g_main_loop_quit(m_eventLoop);
    }

    if (m_eventThread.joinable()) {
        m_eventThread.join();
    }
}

BlueZDeviceManager::~BlueZDeviceManager() {
    ACSDK_DEBUG5(LX(__func__));
}

BlueZDeviceManager::BlueZDeviceManager(
    const std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus>& eventBus) :
        RequiresShutdown{"BlueZDeviceManager"},
        m_eventBus{eventBus},
        m_streamingState{avsCommon::utils::bluetooth::MediaStreamingState::IDLE} {
}

bool BlueZDeviceManager::getStateFromBlueZ() {
    ManagedGError error;
    ManagedGVariant managedObjectsVar =
        m_objectManagerProxy->callMethod("GetManagedObjects", nullptr, error.toOutputParameter());

    if (error.hasError()) {
        ACSDK_ERROR(LX("initializeKnownDevicesFailed").d("error", error.getMessage()));
        return false;
    }

    GVariantTupleReader resultReader(managedObjectsVar);
    ManagedGVariant managedObjectsMap = resultReader.getVariant(0);
    GVariantMapReader mapReader(managedObjectsMap, true);

    mapReader.forEach([this](char* objectPath, GVariant* dbusObject) -> bool {
        GVariantMapReader supportedInterfacesMap(dbusObject);

        // Check for Adapter if none found yet
        if (m_adapterPath.empty()) {
            ManagedGVariant adapterInterfaceVar =
                supportedInterfacesMap.getVariant(BlueZConstants::BLUEZ_ADAPTER_INTERFACE);
            if (adapterInterfaceVar.hasValue()) {
                ACSDK_DEBUG3(LX("Found bluetooth adapter").d("Path", objectPath));
                m_adapterPath = objectPath;
            }
        }

        ManagedGVariant deviceInterfaceVar = supportedInterfacesMap.getVariant(BlueZConstants::BLUEZ_DEVICE_INTERFACE);
        if (deviceInterfaceVar.hasValue()) {
            // Found a known device
            auto device = addDeviceFromDBusObject(objectPath, deviceInterfaceVar.get());
        }

        return true;
    });

    return true;
}

std::shared_ptr<BlueZBluetoothDevice> BlueZDeviceManager::addDeviceFromDBusObject(
    const char* objectPath,
    GVariant* dbusObject) {
    if (nullptr == objectPath) {
        ACSDK_ERROR(LX("addDeviceFromDBusObjectFailed").d("reason", "objectPath is null"));
    }
    if (nullptr == dbusObject) {
        ACSDK_ERROR(LX("addDeviceFromDBusObjectFailed").d("reason", "dbusObject is null"));
    }

    GVariantMapReader deviceMapReader(dbusObject);
    char* macAddress = nullptr;

    if (!deviceMapReader.getCString(BlueZConstants::BLUEZ_DEVICE_INTERFACE_ADDRESS, &macAddress)) {
        // No mac address - ignore the device
        return nullptr;
    }

    std::shared_ptr<BlueZBluetoothDevice> knownDevice =
        BlueZBluetoothDevice::create(macAddress, objectPath, shared_from_this());
    if (knownDevice) {
        addDevice(objectPath, knownDevice);
    }

    return knownDevice;
}

std::list<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>> BlueZDeviceManager::
    getDiscoveredDevices() {
    ACSDK_DEBUG5(LX(__func__));
    std::list<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>> newList;

    std::lock_guard<std::mutex> guard(m_devicesMutex);

    for (const auto& it : m_devices) {
        newList.push_back(
            std::static_pointer_cast<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>(it.second));
    }

    return newList;
}

std::shared_ptr<BlueZHostController> BlueZDeviceManager::initializeHostController() {
    return BlueZHostController::create(m_adapterPath);
}

std::shared_ptr<MediaEndpoint> BlueZDeviceManager::getMediaEndpoint() {
    return m_mediaEndpoint;
}

std::shared_ptr<BluetoothHostControllerInterface> BlueZDeviceManager::getHostController() {
    return m_hostController;
}

std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> BlueZDeviceManager::getEventBus() {
    return m_eventBus;
}

bool BlueZDeviceManager::finalizeMedia() {
    ManagedGError error;

    m_mediaProxy->callMethod(
        "UnregisterEndpoint", g_variant_new("(o)", DBUS_ENDPOINT_PATH_SINK), error.toOutputParameter());

    if (error.hasError()) {
        ACSDK_ERROR(LX("finalizeMediaFailed").d("reason", "Failed to unregister MediaEndpoint"));
        return false;
    }

    m_mediaEndpoint.reset();

    return true;
}

void BlueZDeviceManager::mainLoopThread() {
    g_main_context_push_thread_default(m_workerContext);

    // Connect signals.
    ACSDK_DEBUG5(LX("Connecting signals..."));

    do {
        // New interface added. Used to track new devices found by BlueZ
        int subscriptionId = m_connection->subscribeToSignal(
            BlueZConstants::BLUEZ_SERVICE_NAME,
            BlueZConstants::OBJECT_MANAGER_INTERFACE,
            "InterfacesAdded",
            nullptr,
            BlueZDeviceManager::interfacesAddedCallback,
            this);

        if (0 == subscriptionId) {
            ACSDK_ERROR(LX("initFailed").d("reason", "failed to subscribe to InterfacesAdded signal"));
            m_mainLoopInitPromise.set_value(false);
            break;
        }

        // Track device removal
        subscriptionId = m_connection->subscribeToSignal(
            BlueZConstants::BLUEZ_SERVICE_NAME,
            BlueZConstants::OBJECT_MANAGER_INTERFACE,
            "InterfacesRemoved",
            nullptr,
            BlueZDeviceManager::interfacesRemovedCallback,
            this);

        if (0 == subscriptionId) {
            ACSDK_ERROR(LX("initFailed").d("reason", "failed to subscribe to InterfacesRemoved signal"));
            m_mainLoopInitPromise.set_value(false);
            break;
        }

        // Track properties changes
        subscriptionId = m_connection->subscribeToSignal(
            BlueZConstants::BLUEZ_SERVICE_NAME,
            BlueZConstants::PROPERTIES_INTERFACE,
            "PropertiesChanged",
            nullptr,
            BlueZDeviceManager::propertiesChangedCallback,
            this);

        if (0 == subscriptionId) {
            ACSDK_ERROR(LX("initFailed").d("reason", "failed to subscribe to PropertiesChanged signal"));
            m_mainLoopInitPromise.set_value(false);
            break;
        }

        ACSDK_DEBUG5(LX("init").m("Initializing Bluetooth Media"));

        if (!initializeMedia()) {
            ACSDK_ERROR(LX("initFailed").d("reason", "initBluetoothMediaFailed"));
            m_mainLoopInitPromise.set_value(false);
            break;
        }

        ACSDK_DEBUG5(LX("init").m("Initializing Pairing Agent"));

        m_pairingAgent = PairingAgent::create(m_connection);
        if (!m_pairingAgent) {
            ACSDK_ERROR(LX("initFailed").d("reason", "initPairingAgentFailed"));
            m_mainLoopInitPromise.set_value(false);
            break;
        }

        ACSDK_DEBUG5(LX("init").m("Initializing MRPIS Player"));

        m_mediaPlayer = MPRISPlayer::create(m_connection, m_mediaProxy, m_eventBus);
        if (!m_mediaPlayer) {
            ACSDK_ERROR(LX("initFailed").d("reason", "initMediaPlayerFailed"));
            m_mainLoopInitPromise.set_value(false);
            break;
        }

        m_mainLoopInitPromise.set_value(true);

        g_main_loop_run(m_eventLoop);
    } while (false);

    g_main_loop_unref(m_eventLoop);
    g_main_context_pop_thread_default(m_workerContext);
    g_main_context_unref(m_workerContext);
}

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK
