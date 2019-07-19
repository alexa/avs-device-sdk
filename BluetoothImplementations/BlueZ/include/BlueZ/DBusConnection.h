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

#ifndef ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_DBUSCONNECTION_H_
#define ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_DBUSCONNECTION_H_

#include <memory>
#include <mutex>
#include <vector>

#include <gio/gio.h>

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

/**
 * A wrapper around DBus connection object.
 */
class DBusConnection {
public:
    /**
     * Destructor.
     */
    ~DBusConnection();

    /**
     * Connects to DBus and returns a new instance of @c DBusConnection.
     *
     * @param connectionType A @c GBusType of the connection.
     * @return A pointer to @c DBusConnection instance or nullptr in case of error.
     */
    static std::unique_ptr<DBusConnection> create(GBusType connectionType = G_BUS_TYPE_SYSTEM);

    /**
     * Subscribes a callback to DBus signal
     *
     * @param serviceName DBus service name. E.g.: "org.bluez"
     * @param interfaceName Name of the DBus interface to listen to. E.g.: "org.freedesktop.DBus.ObjectManager"
     * @param member Name of the interface member (event name). E.g.: "InterfacesAdded"
     * @param firstArgumentFilter A string to filter the first argument of the event callback with. Null if no filter
     * applied. For BlueZ the first argument could contain the ObjectPath of the object that is being added/removed or
     * changed.
     * @param callback Callback to be called by DBus
     * @param userData A pointer to be passed to a callback
     * @return id of the callback created or 0 on error.
     * @remarks @c DBusConnection internally keeps list of all subscriptions made. They all will be closed in
     * the destructor. For most of the cases we don't need the unsubscribing in runtime, so this is left undone now.
     */
    unsigned int subscribeToSignal(
        const char* serviceName,
        const char* interfaceName,
        const char* member,
        const char* firstArgumentFilter,
        GDBusSignalCallback callback,
        gpointer userData);

    /**
     * Get @c GDBusConnection pointer
     *
     * @return A pointer to @c GDBusConnection.
     */
    GDBusConnection* getGDBusConnection();

    /**
     * Closes the connection. All the subscriptions are closed automatically.
     */
    void close();

private:
    /**
     * Private constructor used in create() method.
     *
     * @param connection Raw @c GDBusConnection pointer to attach to.
     */
    explicit DBusConnection(GDBusConnection* connection);

    /// Raw @c GDBusConnection* pointer used for operations
    GDBusConnection* m_connection;

    /// Mutex to guard subscriptions
    std::mutex m_subscriptionsMutex;

    /// Vector of ids of all signal subscriptions made for this connection
    std::vector<guint> m_subscriptions;
};

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_DBUSCONNECTION_H_
