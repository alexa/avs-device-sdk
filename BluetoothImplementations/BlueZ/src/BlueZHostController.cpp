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

#include "BlueZ/BlueZHostController.h"

#include <AVSCommon/Utils/Logger/Logger.h>

#include "BlueZ/BlueZConstants.h"
#include "BlueZ/BlueZDeviceManager.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

/// String to identify log entries originating from this file.
static const std::string TAG{"BlueZHostController"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// String to identify name property.
static const std::string ALIAS_PROPERTY = "Alias";

/// String to identify discovering property.
static const std::string DISCOVERABLE_PROPERTY = "Discoverable";

/// The expected size of a MAC address in the format XX:XX:XX:XX:XX:XX.
static const unsigned int MAC_SIZE = 17;

/// String to identify scanning property.
static const std::string SCANNING_PROPERTY = "Discovering";

/// String to identify adapter method to start scanning.
static const std::string START_SCAN = "StartDiscovery";

/// String to identify adapter method to stop scanning.
static const std::string STOP_SCAN = "StopDiscovery";

using namespace avsCommon::utils;

/// A fallback device name.
static const std::string DEFAULT_NAME = "Device";

/**
 * TODO: Move this to MacAddressString class.
 * Utility function to truncate a valid MAC address.
 *
 * @param mac The mac address.
 * @param[out] truncatedMac The truncated MAC address.
 * @return A bool indicating success.
 */
static bool truncate(const std::unique_ptr<MacAddressString>& mac, std::string* truncatedMac) {
    ACSDK_DEBUG5(LX(__func__));

    if (!mac) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullMAC"));
        return false;
    } else if (!truncatedMac) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullTruncatedMAC"));
        return false;
    } else if (mac->getString().length() != MAC_SIZE) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalidMACLength"));
        return false;
    }

    *truncatedMac = mac->getString();

    char X = 'X';
    truncatedMac->at(0) = X;
    truncatedMac->at(1) = X;
    truncatedMac->at(3) = X;
    truncatedMac->at(4) = X;
    truncatedMac->at(6) = X;
    truncatedMac->at(7) = X;
    truncatedMac->at(9) = X;
    truncatedMac->at(10) = X;

    return true;
}

std::unique_ptr<BlueZHostController> BlueZHostController::create(const std::string& adapterObjectPath) {
    ACSDK_DEBUG5(LX(__func__).d("adapterObjectPath", adapterObjectPath));

    if (adapterObjectPath.empty()) {
        ACSDK_ERROR(LX(__func__).d("reason", "emptyAdapterPath").m("Host controller requires adapter to operate!"));
        return nullptr;
    }

    auto hostController = std::unique_ptr<BlueZHostController>(new BlueZHostController(adapterObjectPath));
    if (!hostController->init()) {
        ACSDK_ERROR(LX(__func__).d("reason", "initFailed"));
        return nullptr;
    }

    return hostController;
}

BlueZHostController::BlueZHostController(const std::string& adapterObjectPath) :
        m_adapterObjectPath{adapterObjectPath} {
}

bool BlueZHostController::init() {
    ACSDK_DEBUG5(LX(__func__));

    m_adapter = DBusProxy::create(BlueZConstants::BLUEZ_ADAPTER_INTERFACE, m_adapterObjectPath);
    if (!m_adapter) {
        ACSDK_ERROR(LX(__func__).d("reason", "createAdapterProxyFailed"));
        return false;
    }

    m_adapterProperties = DBusPropertiesProxy::create(m_adapterObjectPath);
    if (!m_adapterProperties) {
        ACSDK_ERROR(LX(__func__).d("reason", "createPropertiesProxyFailed"));
        return false;
    }

    // Get the MAC address.
    std::string mac;
    if (!m_adapterProperties->getStringProperty(
            BlueZConstants::BLUEZ_ADAPTER_INTERFACE, BlueZConstants::BLUEZ_DEVICE_INTERFACE_ADDRESS, &mac)) {
        ACSDK_ERROR(LX(__func__).d("reason", "noMACAddress"));
        return false;
    }

    /*
     * Create a MacAddressString object to validate the MAC string.
     * If this fails, then the MAC address is in an invalid format.
     */
    m_mac = MacAddressString::create(mac);
    if (!m_mac) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalidMAC"));
        return false;
    }

    // Attempt to get the friendlyName of the adapter.
    if (!m_adapterProperties->getStringProperty(
            BlueZConstants::BLUEZ_ADAPTER_INTERFACE, BlueZConstants::BLUEZ_DEVICE_INTERFACE_ALIAS, &m_friendlyName)) {
        ACSDK_ERROR(LX(__func__).d("reason", "noValidFriendlyName").m("Falling back"));

        /*
         * We failed to obtain a friendly name. This is unusual, but doesn't necessarily
         * mean the adapter is inoperational. Use the AVS convention and use a truncated MAC address.
         * If the truncate somehow fails, proceed with a default name.
         */
        std::string truncatedMac;
        m_friendlyName = truncate(m_mac, &truncatedMac) ? truncatedMac : DEFAULT_NAME;
    }

    ACSDK_DEBUG5(LX("adapterProperties").d("mac", m_mac->getString()).d("friendlyName", m_friendlyName));

    return true;
}

std::string BlueZHostController::getMac() const {
    ACSDK_DEBUG5(LX(__func__));

    return m_mac->getString();
}

std::string BlueZHostController::getFriendlyName() const {
    ACSDK_DEBUG5(LX(__func__));

    return m_friendlyName;
}

// Discovery
std::future<bool> BlueZHostController::enterDiscoverableMode() {
    ACSDK_DEBUG5(LX(__func__));

    return setDiscoverable(true);
}

std::future<bool> BlueZHostController::exitDiscoverableMode() {
    ACSDK_DEBUG5(LX(__func__));

    return setDiscoverable(false);
}

std::future<bool> BlueZHostController::setDiscoverable(bool discoverable) {
    ACSDK_DEBUG5(LX(__func__).d("discoverable", discoverable));

    // Synchronous operation for us, but we have to satisfy the interface.
    std::promise<bool> promise;
    bool success = false;

    {
        std::lock_guard<std::mutex> lock(m_adapterMutex);
        success = m_adapterProperties->setProperty(
            BlueZConstants::BLUEZ_ADAPTER_INTERFACE, DISCOVERABLE_PROPERTY, g_variant_new_boolean(discoverable));
    }

    promise.set_value(success);

    if (!success) {
        ACSDK_ERROR(LX(__func__).d("reason", "setAdapterPropertyFailed").d("discoverable", discoverable));
    }

    return promise.get_future();
}

bool BlueZHostController::isDiscoverable() const {
    ACSDK_DEBUG5(LX(__func__));

    bool result = false;
    std::lock_guard<std::mutex> lock(m_adapterMutex);
    m_adapterProperties->getBooleanProperty(BlueZConstants::BLUEZ_ADAPTER_INTERFACE, DISCOVERABLE_PROPERTY, &result);
    return result;
}

// Scanning
std::future<bool> BlueZHostController::startScan() {
    ACSDK_DEBUG5(LX(__func__));

    return changeScanState(true);
}

std::future<bool> BlueZHostController::stopScan() {
    ACSDK_DEBUG5(LX(__func__));

    return changeScanState(false);
}

std::future<bool> BlueZHostController::changeScanState(bool scanning) {
    ACSDK_DEBUG5(LX(__func__).d("scanning", scanning));

    // Synchronous operation for us, but we have to satisfy the interface.
    std::promise<bool> promise;
    ManagedGError error;

    {
        std::lock_guard<std::mutex> lock(m_adapterMutex);
        ManagedGVariant result =
            m_adapter->callMethod(scanning ? START_SCAN : STOP_SCAN, nullptr, error.toOutputParameter());
    }

    if (error.hasError()) {
        ACSDK_ERROR(LX(__func__).d("reason", "callScanMethodFailed").d("error", error.getMessage()));
        promise.set_value(false);
    } else {
        promise.set_value(true);
    }

    return promise.get_future();
}

bool BlueZHostController::isScanning() const {
    ACSDK_DEBUG5(LX(__func__));

    bool result = false;
    std::lock_guard<std::mutex> lock(m_adapterMutex);
    m_adapterProperties->getBooleanProperty(BlueZConstants::BLUEZ_ADAPTER_INTERFACE, SCANNING_PROPERTY, &result);
    return result;
}

void BlueZHostController::onPropertyChanged(const GVariantMapReader& changesMap) {
    char* alias = nullptr;
    bool aliasChanged = changesMap.getCString(ALIAS_PROPERTY.c_str(), &alias);

    std::lock_guard<std::mutex> lock(m_adapterMutex);
    if (aliasChanged) {
        // This should never happen.
        if (!alias) {
            ACSDK_ERROR(LX(__func__).d("reason", "nullAlias"));
        } else {
            ACSDK_DEBUG5(LX("nameChanged").d("oldName", m_friendlyName).d("newName", alias));
            m_friendlyName = alias;
        }
    }
}

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK
