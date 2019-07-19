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

#include <gio/gunixfdlist.h>

#include <AVSCommon/Utils/Logger/Logger.h>

#include "BlueZ/BlueZConstants.h"
#include "BlueZ/DBusProxy.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

/// String to identify log entries originating from this file.
static const std::string TAG{"BlueZUtils"};

static const int PROXY_DEFAULT_TIMEOUT = -1;

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

DBusProxy::DBusProxy(GDBusProxy* proxy, const std::string& objectPath) : m_proxy{proxy}, m_objectPath{objectPath} {
}

DBusProxy::~DBusProxy() {
    ACSDK_DEBUG7(LX(__func__));
    if (m_proxy) {
        g_object_unref(m_proxy);
    }
}

std::shared_ptr<DBusProxy> DBusProxy::create(const std::string& interfaceName, const std::string& objectPath) {
    GError* error = nullptr;
    GDBusProxy* proxy = g_dbus_proxy_new_for_bus_sync(
        G_BUS_TYPE_SYSTEM,
        G_DBUS_PROXY_FLAGS_NONE,
        nullptr,
        BlueZConstants::BLUEZ_SERVICE_NAME,
        objectPath.c_str(),
        interfaceName.c_str(),
        nullptr,
        &error);

    if (!proxy) {
        ACSDK_ERROR(LX("createFailed").d("error", error->message));
        g_error_free(error);
        return nullptr;
    }

    return std::shared_ptr<DBusProxy>(new DBusProxy(proxy, objectPath));
}

ManagedGVariant DBusProxy::callMethod(const std::string& methodName, GVariant* parameters, GError** error) {
    GVariant* tempResult = g_dbus_proxy_call_sync(
        m_proxy, methodName.c_str(), parameters, G_DBUS_CALL_FLAGS_NONE, PROXY_DEFAULT_TIMEOUT, nullptr, error);
    return ManagedGVariant(tempResult);
}

ManagedGVariant DBusProxy::callMethodWithFDList(
    const std::string& methodName,
    GVariant* parameters,
    GUnixFDList** outlist,
    GError** error) {
    GVariant* tempResult = g_dbus_proxy_call_with_unix_fd_list_sync(
        m_proxy,
        methodName.c_str(),
        parameters,
        G_DBUS_CALL_FLAGS_NONE,
        PROXY_DEFAULT_TIMEOUT,
        nullptr,
        outlist,
        nullptr,
        error);
    return ManagedGVariant(tempResult);
}

std::string DBusProxy::getObjectPath() const {
    return m_objectPath;
}

GDBusProxy* DBusProxy::get() {
    return m_proxy;
}

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK
