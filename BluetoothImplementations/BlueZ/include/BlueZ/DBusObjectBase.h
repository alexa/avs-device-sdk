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

#ifndef ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_DBUSOBJECTBASE_H_
#define ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_DBUSOBJECTBASE_H_

#include <memory>
#include <string>

#include "BlueZ/DBusConnection.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

/**
 * Base class for objects exposed to DBus
 */
class DBusObjectBase {
public:
    /**
     * Destructor
     */
    virtual ~DBusObjectBase();

    /**
     * Registers an object with DBus
     *
     * @return true if succeeded, false otherwise
     */
    bool registerWithDBus();

    /**
     * Unregisters the object from DBus
     */
    void unregisterObject();

protected:
    /**
     * A constructor.
     *
     * @param connection @c DBusConnection connection to register this object with
     * @param xmlInterfaceIntrospection XML containing the description of the interfaces this object implements. See
     * <a href="https://dbus.freedesktop.org/doc/dbus-api-design.html"/>
     * @param objectPath Path to register the object to
     * @param methodCallFunc @c GDBusInterfaceMethodCallFunc callback that handles the incoming method calls
     */
    DBusObjectBase(
        std::shared_ptr<DBusConnection> connection,
        const std::string& xmlInterfaceIntrospection,
        const std::string& objectPath,
        GDBusInterfaceMethodCallFunc methodCallFunc);

    /**
     * Method used solely for logging the call of the object's method
     *
     * @param methodName the called method name
     */
    void onMethodCalledInternal(const char* methodName);

private:
    /// XML interface description to be used for object registration
    const std::string m_xmlInterfaceIntrospection;

    /// The ID of the object registered to DBus. 0 if object is not registered.
    unsigned int m_registrationId;

    /// @c GDBusInterfaceVTable object containing the reference to interface description parsed from the XML
    GDBusInterfaceVTable m_interfaceVtable;

    /// A DBus connection to be used to register the object
    std::shared_ptr<DBusConnection> m_connection;

    /// A path to be registered object with
    const std::string m_objectPath;
};

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_DBUSOBJECTBASE_H_
