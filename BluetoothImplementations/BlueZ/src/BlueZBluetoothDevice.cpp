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

#include <AVSCommon/Utils/Bluetooth/SDPRecords.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include "BlueZ/BlueZA2DPSink.h"
#include "BlueZ/BlueZA2DPSource.h"
#include "BlueZ/BlueZAVRCPController.h"
#include "BlueZ/BlueZAVRCPTarget.h"
#include "BlueZ/BlueZConstants.h"
#include "BlueZ/BlueZDeviceManager.h"

#include "BlueZ/BlueZBluetoothDevice.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

using namespace avsCommon::utils;
using namespace avsCommon::utils::bluetooth;
using namespace avsCommon::sdkInterfaces::bluetooth;
using namespace avsCommon::sdkInterfaces::bluetooth::services;

/// String to identify log entries originating from this file.
static const std::string TAG{"BlueZBluetoothDevice"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The Name property that BlueZ uses.
static const std::string BLUEZ_DEVICE_PROPERTY_ALIAS = "Alias";

/// The UUID property that BlueZ uses.
static const std::string BLUEZ_DEVICE_PROPERTY_UUIDS = "UUIDs";

/// An BlueZ error indicating when an object no longer exists.
static const std::string BLUEZ_ERROR_NOTFOUND = "org.bluez.Error.DoesNotExist";

/// A BlueZ connect error indicating authentication was rejected.
static const std::string BLUEZ_ERROR_RESOURCE_UNAVAILABLE = "org.bluez.Error.Failed: Resource temporarily unavailable";

/// BlueZ org.bluez.Device1 method to pair.
static const std::string BLUEZ_DEVICE_METHOD_PAIR = "Pair";

/// BlueZ org.bluez.Device1 method to connect.
static const std::string BLUEZ_DEVICE_METHOD_CONNECT = "Connect";

/// BlueZ org.bluez.Device1 method to disconnect.
static const std::string BLUEZ_DEVICE_METHOD_DISCONNECT = "Disconnect";

/// BlueZ org.bluez.Device1 paired property.
static const std::string BLUEZ_DEVICE_PROPERTY_PAIRED = "Paired";

/// BlueZ org.bluez.Device1 connected property.
static const std::string BLUEZ_DEVICE_PROPERTY_CONNECTED = "Connected";

/// BlueZ org.bluez.Adapter1 method to remove device.
static const std::string BLUEZ_ADAPTER_REMOVE_DEVICE = "RemoveDevice";

/// The Media Control interface on the DBus object.
static const std::string MEDIA_CONTROL_INTERFACE = "org.bluez.MediaControl1";

std::shared_ptr<BlueZBluetoothDevice> BlueZBluetoothDevice::create(
    const std::string& mac,
    const std::string& objectPath,
    std::shared_ptr<BlueZDeviceManager> deviceManager) {
    ACSDK_DEBUG5(LX(__func__));

    if (!g_variant_is_object_path(objectPath.c_str())) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalidObjectPath").d("objPath", objectPath));
        return nullptr;
    }

    auto device = std::shared_ptr<BlueZBluetoothDevice>(new BlueZBluetoothDevice(mac, objectPath, deviceManager));

    if (!device->init()) {
        ACSDK_ERROR(LX(__func__).d("reason", "initFailed"));
        return nullptr;
    }

    return device;
}

BlueZBluetoothDevice::BlueZBluetoothDevice(
    const std::string& mac,
    const std::string& objectPath,
    std::shared_ptr<BlueZDeviceManager> deviceManager) :
        m_mac{mac},
        m_objectPath{objectPath},
        m_deviceState{BlueZDeviceState::FOUND},
        m_deviceManager{deviceManager} {
}

std::string BlueZBluetoothDevice::getMac() const {
    ACSDK_DEBUG5(LX(__func__));

    return m_mac;
}

std::string BlueZBluetoothDevice::getFriendlyName() const {
    ACSDK_DEBUG5(LX(__func__));

    return m_friendlyName;
}

bool BlueZBluetoothDevice::updateFriendlyName() {
    ACSDK_DEBUG5(LX(__func__));

    if (!m_propertiesProxy->getStringProperty(
            BlueZConstants::BLUEZ_DEVICE_INTERFACE, BLUEZ_DEVICE_PROPERTY_ALIAS, &m_friendlyName)) {
        ACSDK_ERROR(LX(__func__).d("reason", "getNameFailed"));
        return false;
    }

    return true;
}

BlueZBluetoothDevice::~BlueZBluetoothDevice() {
    ACSDK_DEBUG5(LX(__func__).d("mac", getMac()));
    m_executor.shutdown();

    {
        std::lock_guard<std::mutex> lock(m_servicesMapMutex);

        for (auto& entry : m_servicesMap) {
            entry.second->cleanup();
        }
        m_servicesMap.clear();
    }
}

bool BlueZBluetoothDevice::init() {
    ACSDK_DEBUG5(LX(__func__).d("path", m_objectPath));

    m_deviceProxy = DBusProxy::create(BlueZConstants::BLUEZ_DEVICE_INTERFACE, m_objectPath);
    if (!m_deviceProxy) {
        ACSDK_ERROR(LX(__func__).d("reason", "createDeviceProxyFailed"));
        return false;
    }

    m_propertiesProxy = DBusPropertiesProxy::create(m_objectPath);
    if (!m_propertiesProxy) {
        ACSDK_ERROR(LX(__func__).d("reason", "createPropertyProxyFailed"));
        return false;
    }

    updateFriendlyName();

    bool isPaired = false;
    if (queryDeviceProperty(BLUEZ_DEVICE_PROPERTY_PAIRED, &isPaired) && isPaired) {
        ACSDK_DEBUG5(LX(__func__).m("deviceIsPaired"));
        m_deviceState = BlueZDeviceState::IDLE;
    }

    // Parse UUIDs and find versions.
    if (!initializeServices(getServiceUuids())) {
        ACSDK_ERROR(LX(__func__).d("reason", "initializeServicesFailed"));
        return false;
    }

    return true;
}

bool BlueZBluetoothDevice::initializeServices(const std::unordered_set<std::string>& uuids) {
    ACSDK_DEBUG5(LX(__func__));

    for (const auto& uuid : uuids) {
        ACSDK_DEBUG9(LX(__func__).d("supportedUUID", uuid));

        // BlueZ does not provide the version of the service.
        if (A2DPSourceInterface::UUID == uuid && !serviceExists(uuid)) {
            ACSDK_DEBUG5(LX(__func__).d("supports", A2DPSourceInterface::NAME));
            auto a2dpSource = BlueZA2DPSource::create(m_deviceManager);
            if (!a2dpSource) {
                ACSDK_ERROR(LX(__func__).d("reason", "createA2DPFailed"));
                return false;
            } else {
                a2dpSource->setup();
                insertService(a2dpSource);
            }
        } else if (AVRCPTargetInterface::UUID == uuid && !serviceExists(uuid)) {
            ACSDK_DEBUG5(LX(__func__).d("supports", AVRCPTargetInterface::NAME));
            auto mediaControlProxy = DBusProxy::create(MEDIA_CONTROL_INTERFACE, m_objectPath);
            if (!mediaControlProxy) {
                ACSDK_ERROR(LX(__func__).d("reason", "nullMediaControlProxy"));
                return false;
            }

            auto avrcpTarget = BlueZAVRCPTarget::create(mediaControlProxy);
            if (!avrcpTarget) {
                ACSDK_ERROR(LX(__func__).d("reason", "createAVRCPTargetFailed"));
                return false;
            } else {
                avrcpTarget->setup();
                insertService(avrcpTarget);
            }
        } else if (A2DPSinkInterface::UUID == uuid && !serviceExists(uuid)) {
            ACSDK_DEBUG5(LX(__func__).d("supports", A2DPSinkInterface::NAME));
            auto a2dpSink = BlueZA2DPSink::create();
            if (!a2dpSink) {
                ACSDK_ERROR(LX(__func__).d("reason", "createA2DPSinkFailed"));
                return false;
            } else {
                a2dpSink->setup();
                insertService(a2dpSink);
            }
        } else if (AVRCPControllerInterface::UUID == uuid && !serviceExists(uuid)) {
            ACSDK_DEBUG5(LX(__func__).d("supports", AVRCPControllerInterface::NAME));
            auto avrcpController = BlueZAVRCPController::create();
            if (!avrcpController) {
                ACSDK_ERROR(LX(__func__).d("reason", "createAVRCPControllerFailed"));
                return false;
            } else {
                avrcpController->setup();
                insertService(avrcpController);
            }
        }
    }

    return true;
}

bool BlueZBluetoothDevice::isPaired() {
    ACSDK_DEBUG5(LX(__func__));

    auto future = m_executor.submit([this] { return executeIsPaired(); });

    if (future.valid()) {
        return future.get();
    } else {
        ACSDK_ERROR(LX(__func__).d("reason", "invalidFuture").d("action", "defaultingFalse"));
        return false;
    }
}

bool BlueZBluetoothDevice::executeIsPaired() {
    ACSDK_DEBUG5(LX(__func__));

    return BlueZDeviceState::UNPAIRED != m_deviceState && BlueZDeviceState::FOUND != m_deviceState;
}

std::future<bool> BlueZBluetoothDevice::pair() {
    ACSDK_DEBUG5(LX(__func__));

    return m_executor.submit([this] { return executePair(); });
}

bool BlueZBluetoothDevice::executePair() {
    ACSDK_DEBUG5(LX(__func__));

    ManagedGError error;
    m_deviceProxy->callMethod(BLUEZ_DEVICE_METHOD_PAIR, nullptr, error.toOutputParameter());

    if (error.hasError()) {
        ACSDK_ERROR(LX(__func__).d("error", error.getMessage()));
        return false;
    }
    return true;
}

std::future<bool> BlueZBluetoothDevice::unpair() {
    ACSDK_DEBUG5(LX(__func__));

    return m_executor.submit([this] { return executeUnpair(); });
}

bool BlueZBluetoothDevice::executeUnpair() {
    ACSDK_DEBUG5(LX(__func__));

    ManagedGError error;

    auto adapterProxy = DBusProxy::create(BlueZConstants::BLUEZ_ADAPTER_INTERFACE, m_deviceManager->getAdapterPath());

    if (!adapterProxy) {
        ACSDK_ERROR(LX(__func__).d("error", "createAdapterProxyFailed"));
        return false;
    }

    adapterProxy->callMethod(
        BLUEZ_ADAPTER_REMOVE_DEVICE, g_variant_new("(o)", m_objectPath.c_str()), error.toOutputParameter());

    if (error.hasError()) {
        std::string errorMsg = error.getMessage();
        // Treat as success if you can't find the device anymore.
        if (std::string::npos != errorMsg.find(BLUEZ_ERROR_NOTFOUND)) {
            return true;
        }
        ACSDK_ERROR(LX(__func__).d("error", errorMsg));
        return false;
    }
    return true;
}

std::string BlueZBluetoothDevice::getObjectPath() const {
    ACSDK_DEBUG5(LX(__func__));

    return m_objectPath;
}

std::unordered_set<std::string> BlueZBluetoothDevice::getServiceUuids(GVariant* array) {
    ACSDK_DEBUG5(LX(__func__));

    std::unordered_set<std::string> uuids;

    if (!array) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullArray"));
        return uuids;
    } else if (!g_variant_is_of_type(array, G_VARIANT_TYPE_ARRAY)) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalidType").d("type", g_variant_get_type_string(array)));
        return uuids;
    }

    GVariantTupleReader arrayReader(array);
    arrayReader.forEach([&uuids](GVariant* variant) {
        if (!variant) {
            ACSDK_ERROR(LX("iteratingArrayFailed").d("reason", "nullVariant"));
            return false;
        }
        // Do not free, this is not allocated.
        const gchar* temp = g_variant_get_string(variant, NULL);
        std::string uuid(temp);
        ACSDK_DEBUG5(LX(__func__).d("uuid", uuid));
        uuids.insert(uuid);
        return true;
    });

    return uuids;
}

std::unordered_set<std::string> BlueZBluetoothDevice::getServiceUuids() {
    ACSDK_DEBUG5(LX(__func__));

    // DBus returns this as (a{v},). We have to drill into the tuple to retrieve the array.
    ManagedGVariant uuidsTuple;
    if (!m_propertiesProxy->getVariantProperty(
            BlueZConstants::BLUEZ_DEVICE_INTERFACE, BLUEZ_DEVICE_PROPERTY_UUIDS, &uuidsTuple)) {
        ACSDK_ERROR(LX(__func__).d("reason", "getVariantPropertyFailed"));
        return std::unordered_set<std::string>();
    }

    GVariantTupleReader tupleReader(uuidsTuple);
    ManagedGVariant array = tupleReader.getVariant(0).unbox();

    if (!array.hasValue()) {
        // The format isn't what we were expecting. Print the original tuple for debugging.
        ACSDK_ERROR(LX(__func__).d("reason", "unexpectedVariantFormat").d("variant", uuidsTuple.dumpToString(false)));
        return std::unordered_set<std::string>();
    }

    return getServiceUuids(array.get());
}

bool BlueZBluetoothDevice::isConnected() {
    ACSDK_DEBUG5(LX(__func__));

    auto future = m_executor.submit([this] { return executeIsConnected(); });

    if (future.valid()) {
        return future.get();
    } else {
        ACSDK_ERROR(LX(__func__).d("reason", "invalidFuture").d("action", "defaultingFalse"));
        return false;
    }
}

bool BlueZBluetoothDevice::executeIsConnected() {
    ACSDK_DEBUG5(LX(__func__));

    return BlueZDeviceState::CONNECTED == m_deviceState;
}

std::future<bool> BlueZBluetoothDevice::connect() {
    ACSDK_DEBUG5(LX(__func__));

    return m_executor.submit([this] { return executeConnect(); });
}

bool BlueZBluetoothDevice::executeConnect() {
    ACSDK_DEBUG5(LX(__func__));

    // If already connected, don't attempt to connect again.
    // On 5.37, a generic "Failed" error is thrown.
    if (executeIsConnected()) {
        return true;
    }

    ManagedGError error;
    m_deviceProxy->callMethod(BLUEZ_DEVICE_METHOD_CONNECT, nullptr, error.toOutputParameter());

    if (error.hasError()) {
        std::string errStr = error.getMessage() ? error.getMessage() : "";
        ACSDK_ERROR(LX(__func__).d("error", errStr));

        // This indicates an issue with authentication, likely the other device has unpaired.
        if (std::string::npos != errStr.find(BLUEZ_ERROR_RESOURCE_UNAVAILABLE)) {
            transitionToState(BlueZDeviceState::CONNECTION_FAILED, false);
        }
        return false;
    }

    /*
     * If the current state is CONNECTION_FAILED, another Connected = true property changed signal may not appear.
     * We'll transition to the CONNECTED state directly here. If that signal does come, we simply
     * ignore it because there's no transition when you're already CONNECTED and you see a Connected = true.
     */
    if (BlueZDeviceState::CONNECTION_FAILED == m_deviceState) {
        transitionToState(BlueZDeviceState::CONNECTED, true);
    }

    return true;
}

std::future<bool> BlueZBluetoothDevice::disconnect() {
    ACSDK_DEBUG5(LX(__func__));

    return m_executor.submit([this] { return executeDisconnect(); });
}

bool BlueZBluetoothDevice::executeDisconnect() {
    ACSDK_DEBUG5(LX(__func__));
    ManagedGError error;
    m_deviceProxy->callMethod(BLUEZ_DEVICE_METHOD_DISCONNECT, nullptr, error.toOutputParameter());

    if (error.hasError()) {
        ACSDK_ERROR(LX(__func__).d("error", error.getMessage()));
        return false;
    }

    return true;
}

std::vector<std::shared_ptr<SDPRecordInterface>> BlueZBluetoothDevice::getSupportedServices() {
    ACSDK_DEBUG5(LX(__func__));

    std::vector<std::shared_ptr<SDPRecordInterface>> services;

    {
        std::lock_guard<std::mutex> lock(m_servicesMapMutex);

        for (auto& it : m_servicesMap) {
            services.push_back(it.second->getRecord());
        }
    }

    return services;
}

bool BlueZBluetoothDevice::serviceExists(const std::string& uuid) {
    std::lock_guard<std::mutex> lock(m_servicesMapMutex);
    return m_servicesMap.count(uuid) != 0;
}

bool BlueZBluetoothDevice::insertService(std::shared_ptr<BluetoothServiceInterface> service) {
    ACSDK_DEBUG5(LX(__func__));

    if (!service) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullService"));
        return false;
    }

    std::shared_ptr<SDPRecordInterface> record = service->getRecord();

    if (!record) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullRecord"));
        return false;
    }

    ACSDK_DEBUG5(LX(__func__).d("serviceUuid", record->getUuid()));

    bool success = false;
    {
        std::lock_guard<std::mutex> lock(m_servicesMapMutex);
        success = m_servicesMap.insert({record->getUuid(), service}).second;
    }

    if (!success) {
        ACSDK_ERROR(LX(__func__).d("reason", "serviceAlreadyExists"));
    }

    return success;
}

template <typename ServiceType>
std::shared_ptr<ServiceType> BlueZBluetoothDevice::getService() {
    ACSDK_DEBUG5(LX(__func__).d("uuid", ServiceType::UUID));

    std::shared_ptr<ServiceType> service = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_servicesMapMutex);
        auto it = m_servicesMap.find(ServiceType::UUID);
        if (it == m_servicesMap.end()) {
            ACSDK_DEBUG(LX(__func__).d("reason", "serviceNotFound"));
        } else {
            // We completely control the types these are going to be, so avoid the overhead of dynamic_pointer_cast.
            service = std::static_pointer_cast<ServiceType>(it->second);
        }
    }

    return service;
}

std::shared_ptr<A2DPSinkInterface> BlueZBluetoothDevice::getA2DPSink() {
    ACSDK_DEBUG5(LX(__func__));

    return getService<A2DPSinkInterface>();
}

std::shared_ptr<A2DPSourceInterface> BlueZBluetoothDevice::getA2DPSource() {
    ACSDK_DEBUG5(LX(__func__));
    return getService<A2DPSourceInterface>();
}

std::shared_ptr<AVRCPTargetInterface> BlueZBluetoothDevice::getAVRCPTarget() {
    ACSDK_DEBUG5(LX(__func__));

    return getService<AVRCPTargetInterface>();
}

std::shared_ptr<AVRCPControllerInterface> BlueZBluetoothDevice::getAVRCPController() {
    ACSDK_DEBUG5(LX(__func__));

    return getService<AVRCPControllerInterface>();
}

DeviceState BlueZBluetoothDevice::getDeviceState() {
    ACSDK_DEBUG5(LX(__func__));

    return m_executor.submit([this] { return convertToDeviceState(m_deviceState); }).get();
}

avsCommon::sdkInterfaces::bluetooth::DeviceState BlueZBluetoothDevice::convertToDeviceState(
    BlueZDeviceState bluezDeviceState) {
    switch (bluezDeviceState) {
        case BlueZDeviceState::FOUND:
            return DeviceState::FOUND;
        case BlueZDeviceState::UNPAIRED:
            return DeviceState::UNPAIRED;
        case BlueZDeviceState::PAIRED:
            return DeviceState::PAIRED;
        case BlueZDeviceState::CONNECTION_FAILED:
        case BlueZDeviceState::IDLE:
            return DeviceState::IDLE;
        case BlueZDeviceState::DISCONNECTED:
            return DeviceState::DISCONNECTED;
        case BlueZDeviceState::CONNECTED:
            return DeviceState::CONNECTED;
    }

    ACSDK_ERROR(LX(__func__)
                    .d("reason", "noConversionFound")
                    .d("bluezDeviceState", bluezDeviceState)
                    .d("defaulting", DeviceState::FOUND));

    return DeviceState::FOUND;
}

bool BlueZBluetoothDevice::queryDeviceProperty(const std::string& name, bool* value) {
    ACSDK_DEBUG5(LX(__func__).d("name", name));

    if (!value) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullValue"));
        return false;
    } else if (!m_propertiesProxy) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullPropertiesProxy"));
        return false;
    }

    return m_propertiesProxy->getBooleanProperty(BlueZConstants::BLUEZ_DEVICE_INTERFACE, name.c_str(), value);
}

void BlueZBluetoothDevice::transitionToState(BlueZDeviceState newState, bool sendEvent) {
    ACSDK_DEBUG5(LX(__func__).d("oldState", m_deviceState).d("newState", newState).d("sendEvent", sendEvent));

    m_deviceState = newState;
    if (!m_deviceManager) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullDeviceManager"));
        return;
    } else if (!m_deviceManager->getEventBus()) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullEventBus"));
        return;
    }

    if (sendEvent) {
        m_deviceManager->getEventBus()->sendEvent(
            DeviceStateChangedEvent(shared_from_this(), convertToDeviceState(newState)));
    }
}

// TODO ACSDK-1398: Refactor this with a proper state machine.
void BlueZBluetoothDevice::onPropertyChanged(const GVariantMapReader& changesMap) {
    ACSDK_DEBUG5(LX(__func__).d("values", g_variant_print(changesMap.get(), true)));

    gboolean paired = false;
    bool pairedChanged = changesMap.getBoolean(BLUEZ_DEVICE_PROPERTY_PAIRED.c_str(), &paired);

    gboolean connected = false;
    bool connectedChanged = changesMap.getBoolean(BLUEZ_DEVICE_PROPERTY_CONNECTED.c_str(), &connected);

    // Changes to the friendlyName on the device will be saved on a new connect.
    char* alias = nullptr;
    bool aliasChanged = changesMap.getCString(BLUEZ_DEVICE_PROPERTY_ALIAS.c_str(), &alias);
    std::string aliasStr;

    if (aliasChanged) {
        // This should never happen. If it does, don't update.
        if (!alias) {
            ACSDK_ERROR(LX(__func__).d("reason", "nullAlias"));
            aliasChanged = false;
        } else {
            aliasStr = alias;
        }
    }

    // This is used for checking connectedness.
    bool a2dpSourceAvailable = false;
    bool a2dpSinkAvailable = false;

    /*
     * It's not guaranteed all services will be available at construction time.
     * If any become available at a later time, initialize them.
     */
    ManagedGVariant uuidsVariant = changesMap.getVariant(BLUEZ_DEVICE_PROPERTY_UUIDS.c_str());
    std::unordered_set<std::string> uuids;

    if (uuidsVariant.hasValue()) {
        auto uuids = getServiceUuids(uuidsVariant.get());
        initializeServices(uuids);

        a2dpSourceAvailable = (uuids.count(A2DPSourceInterface::UUID) > 0);
        a2dpSinkAvailable = (uuids.count(A2DPSinkInterface::UUID) > 0);
    }

    m_executor.submit([this,
                       pairedChanged,
                       paired,
                       connectedChanged,
                       connected,
                       a2dpSourceAvailable,
                       a2dpSinkAvailable,
                       aliasChanged,
                       aliasStr] {

        if (aliasChanged) {
            ACSDK_DEBUG5(LX("nameChanged").d("oldName", m_friendlyName).d("newName", aliasStr));
            m_friendlyName = aliasStr;
        }

        switch (m_deviceState) {
            case BlueZDeviceState::FOUND: {
                if (pairedChanged && paired) {
                    transitionToState(BlueZDeviceState::PAIRED, true);
                    transitionToState(BlueZDeviceState::IDLE, true);

                    /*
                     * A connect signal doesn't always mean a device is connected by the BluetoothDeviceInterface
                     * definition. This sequence has been observed:
                     *
                     * 1) Pairing (BlueZ sends Connect = true).
                     * 2) Pair Successful.
                     * 3) Connect multimedia services.
                     * 4) Connect multimedia services successful (BlueZ sends Paired = true, UUIDs = [array of
                     * uuids]).
                     *
                     * Thus we will use the combination of Connect, Paired, and the availability of certain UUIDs to
                     * determine connectedness.
                     */
                    bool isConnected = false;
                    if (queryDeviceProperty(BLUEZ_DEVICE_PROPERTY_CONNECTED, &isConnected) && isConnected &&
                        (a2dpSourceAvailable || a2dpSinkAvailable)) {
                        transitionToState(BlueZDeviceState::CONNECTED, true);
                    }
                }
                break;
            }
            case BlueZDeviceState::IDLE: {
                if (connectedChanged && connected) {
                    transitionToState(BlueZDeviceState::CONNECTED, true);
                } else if (pairedChanged && !paired) {
                    transitionToState(BlueZDeviceState::UNPAIRED, true);
                    transitionToState(BlueZDeviceState::FOUND, true);
                }
                break;
            }
            case BlueZDeviceState::CONNECTED: {
                if (pairedChanged && !paired) {
                    transitionToState(BlueZDeviceState::UNPAIRED, true);
                    transitionToState(BlueZDeviceState::FOUND, true);
                } else if (connectedChanged && !connected) {
                    transitionToState(BlueZDeviceState::DISCONNECTED, true);
                    transitionToState(BlueZDeviceState::IDLE, true);
                }
                break;
            }
            case BlueZDeviceState::UNPAIRED:
            case BlueZDeviceState::PAIRED:
            case BlueZDeviceState::DISCONNECTED: {
                ACSDK_ERROR(LX("onPropertyChanged").d("reason", "invalidState").d("state", m_deviceState));
                break;
            }
            case BlueZDeviceState::CONNECTION_FAILED: {
                if (pairedChanged && !paired) {
                    transitionToState(BlueZDeviceState::UNPAIRED, true);
                    transitionToState(BlueZDeviceState::FOUND, true);
                }
            }
        }
    });
}

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK
