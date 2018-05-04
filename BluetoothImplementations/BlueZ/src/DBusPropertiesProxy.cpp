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

#include <AVSCommon/Utils/Logger/Logger.h>

#include "BlueZ/BlueZConstants.h"
#include "BlueZ/BlueZDeviceManager.h"
#include "BlueZ/DBusPropertiesProxy.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

/// String to identify log entries originating from this file.
static const std::string TAG{"DBusPropertiesProxy"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

DBusPropertiesProxy::DBusPropertiesProxy(GDBusProxy* proxy, const std::string& objectPath) :
        DBusProxy(proxy, objectPath) {
}

std::shared_ptr<DBusPropertiesProxy> DBusPropertiesProxy::create(const std::string& objectPath) {
    GError* error = nullptr;
    GDBusProxy* proxy = g_dbus_proxy_new_for_bus_sync(
        G_BUS_TYPE_SYSTEM,
        G_DBUS_PROXY_FLAGS_NONE,
        nullptr,
        BlueZConstants::BLUEZ_SERVICE_NAME,
        objectPath.c_str(),
        BlueZConstants::PROPERTIES_INTERFACE,
        nullptr,
        &error);

    if (error) {
        ACSDK_ERROR(LX("createFailed").d("error", error->message));
        g_error_free(error);
        return nullptr;
    }

    return std::shared_ptr<DBusPropertiesProxy>(new DBusPropertiesProxy(proxy, objectPath));
}

bool DBusPropertiesProxy::getBooleanProperty(const std::string& interface, const std::string& property, bool* result) {
    if (nullptr == result) {
        ACSDK_ERROR(LX("getBooleanPropertyFailed").d("reason", "result is null"));
        return false;
    }
    ManagedGError error;
    ManagedGVariant varResult =
        callMethod("Get", g_variant_new("(ss)", interface.c_str(), property.c_str()), error.toOutputParameter());

    if (error.hasError()) {
        ACSDK_ERROR(LX(__func__)
                        .d("Failed to get boolean property", property)
                        .d("error", error.getMessage())
                        .d("interface", interface)
                        .d("property", property)
                        .d("path", getObjectPath()));
        return false;
    }

    GVariantTupleReader tupleReader(varResult);
    ManagedGVariant unboxed = tupleReader.getVariant(0).unbox();
    *result = static_cast<bool>(g_variant_get_boolean(unboxed.get()));
    return true;
}

bool DBusPropertiesProxy::getVariantProperty(
    const std::string& interface,
    const std::string& property,
    ManagedGVariant* result) {
    ACSDK_DEBUG5(LX(__func__));

    if (nullptr == result) {
        ACSDK_ERROR(LX("getVariantPropertyFailed").d("reason", "result is null"));
        return false;
    }

    ManagedGError error;
    ManagedGVariant varResult =
        callMethod("Get", g_variant_new("(ss)", interface.c_str(), property.c_str()), error.toOutputParameter());

    if (error.hasError()) {
        ACSDK_ERROR(LX("getVariantPropertyFailed")
                        .d("Failed to get variant property", property)
                        .d("error", error.getMessage()));
        return false;
    }

    result->swap(varResult);
    return true;
}

bool DBusPropertiesProxy::getStringProperty(
    const std::string& interface,
    const std::string& property,
    std::string* result) {
    ACSDK_DEBUG9(LX(__func__).d("object", getObjectPath()).d("interface", interface).d("property", property));

    if (nullptr == result) {
        ACSDK_ERROR(LX("getStringPropertyFailed").d("reason", "result is null"));
        return false;
    }

    ManagedGError error;
    ManagedGVariant varResult =
        callMethod("Get", g_variant_new("(ss)", interface.c_str(), property.c_str()), error.toOutputParameter());

    if (error.hasError()) {
        ACSDK_ERROR(
            LX("getStringPropertyFailed").d("Failed to get string property", property).d("error", error.getMessage()));
        return false;
    }

    GVariantTupleReader tupleReader(varResult);
    ManagedGVariant unboxed = tupleReader.getVariant(0).unbox();
    *result = g_variant_get_string(unboxed.get(), nullptr);
    return true;
}

bool DBusPropertiesProxy::setProperty(const std::string& interface, const std::string& property, GVariant* value) {
    if (nullptr == value) {
        ACSDK_ERROR(LX("setPropertyFailed").d("reason", "value is null"));
        return false;
    }

    ManagedGError error;
    ManagedGVariant varResult = callMethod(
        "Set", g_variant_new("(ssv)", interface.c_str(), property.c_str(), value), error.toOutputParameter());

    if (error.hasError()) {
        ACSDK_ERROR(LX("setPropertyFailed").d("Failed to set property value", property).d("error", error.getMessage()));
        return false;
    }
    return true;
}

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK
