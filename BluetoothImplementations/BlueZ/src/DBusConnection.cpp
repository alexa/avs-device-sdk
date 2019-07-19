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

#include "BlueZ/BlueZUtils.h"
#include "BlueZ/DBusConnection.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

/// String to identify log entries originating from this file.
static const std::string TAG{"DBusConnection"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */

#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

GDBusConnection* DBusConnection::getGDBusConnection() {
    return m_connection;
}

std::unique_ptr<DBusConnection> DBusConnection::create(GBusType connectionType) {
    ManagedGError error;
    GDBusConnection* connection = g_bus_get_sync(connectionType, nullptr, error.toOutputParameter());
    if (error.hasError()) {
        ACSDK_ERROR(LX("createNewFailed").d("reason", error.getMessage()));
        return nullptr;
    }

    g_dbus_connection_set_exit_on_close(connection, false);

    return std::unique_ptr<DBusConnection>(new DBusConnection(connection));
}

unsigned int DBusConnection::subscribeToSignal(
    const char* serviceName,
    const char* interfaceName,
    const char* member,
    const char* firstArgumentFilter,
    GDBusSignalCallback callback,
    gpointer userData) {
    if (nullptr == serviceName) {
        ACSDK_ERROR(LX("subscribeToSignalFailed").d("reason", "serviceName is null"));
        return 0;
    }

    if (nullptr == interfaceName) {
        ACSDK_ERROR(LX("subscribeToSignalFailed").d("reason", "interfaceName is null"));
        return 0;
    }

    if (nullptr == member) {
        ACSDK_ERROR(LX("subscribeToSignalFailed").d("reason", "member is null"));
        return 0;
    }

    if (nullptr == callback) {
        ACSDK_ERROR(LX("subscribeToSignalFailed").d("reason", "callback is null"));
        return 0;
    }

    guint subId = g_dbus_connection_signal_subscribe(
        m_connection,
        serviceName,
        interfaceName,
        member,
        nullptr,
        firstArgumentFilter,
        G_DBUS_SIGNAL_FLAGS_NONE,
        callback,
        userData,
        nullptr);

    if (0 == subId) {
        ACSDK_ERROR(LX("subscribeToSignalFailed").d("reason", "failed to subscribe"));
        return 0;
    }

    ACSDK_DEBUG7(LX("Subscribed to signal")
                     .d("service", serviceName)
                     .d("interface", interfaceName)
                     .d("member", member)
                     .d("result", subId));

    std::lock_guard<std::mutex> guard(m_subscriptionsMutex);
    m_subscriptions.push_back(subId);
    return subId;
}

DBusConnection::DBusConnection(GDBusConnection* connection) : m_connection{connection} {
}

void DBusConnection::close() {
    ACSDK_DEBUG5(LX(__func__));
    if (!m_connection) {
        // Already closed
        return;
    }

    {
        std::lock_guard<std::mutex> guard(m_subscriptionsMutex);

        for (auto subscriptionId : m_subscriptions) {
            g_dbus_connection_signal_unsubscribe(m_connection, subscriptionId);
        }
        m_subscriptions.clear();
    }

    g_dbus_connection_flush_sync(m_connection, nullptr, nullptr);
    g_dbus_connection_close_sync(m_connection, nullptr, nullptr);
    g_object_unref(m_connection);
    m_connection = nullptr;
}

DBusConnection::~DBusConnection() {
    ACSDK_DEBUG7(LX(__func__));
    close();
}

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK
