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

#ifndef ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_DBUSPROPERTIESPROXY_H_
#define ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_DBUSPROPERTIESPROXY_H_

#include <memory>
#include <string>

#include "DBusProxy.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

/**
 * A special case of DBus proxy that binds to properties of a specific object
 *
 * See <a href="https://developer.gnome.org/glib/stable/gvariant-format-strings.html">GVariant Format Strings</a>
 * for type strings explanation.
 */
class DBusPropertiesProxy : public DBusProxy {
public:
    /**
     * A factory method to create a new instance
     *
     * @param objectPath
     * @return New instance of @c DBusPropertiesProxy on success, nullptr otherwise.
     */
    static std::shared_ptr<DBusPropertiesProxy> create(const std::string& objectPath);

    /**
     * Get value of the interface's boolean property
     *
     * @param interface Name of the interface to query
     * @param property Name of the property to get
     * @param[out] result A pointer to a bool variable that receives the property value
     * @return true if property was found, false otherwise.
     */
    bool getBooleanProperty(const std::string& interface, const std::string& property, bool* result);

    /**
     * Get value of the interface's string property
     *
     * @param interface Name of the interface to query
     * @param property Name of the property to get
     * @param[out] result A reference a to std::string variable that receives the property value
     * @return true if property was found, false otherwise.
     */
    bool getStringProperty(const std::string& interface, const std::string& property, std::string* result);

    /**
     * Get value of the interface's variant property
     *
     * @param interface Name of the interface to query
     * @param property Name of the property to get
     * @param[out] result A reference to @c ManagedGVariant variable that receives the property value
     * @return true if property was found, false otherwise.
     */
    bool getVariantProperty(const std::string& interface, const std::string& property, ManagedGVariant* result);

    /**
     * Set value of the interface's property
     *
     * @param interface Name of the interface owning the property
     * @param property Name of the property to set
     * @param value A @ GVariant* value to set
     * @return true if property was found and its value has been changed, false otherwise.
     */
    bool setProperty(const std::string& interface, const std::string& property, GVariant* value);

private:
    /**
     * Private constructor
     *
     * @param proxy A pointer to @c GDBusProxy to use.
     * @param objectPath Object path of the object whose properties to listen to
     */
    explicit DBusPropertiesProxy(GDBusProxy* proxy, const std::string& objectPath);
};

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_DBUSPROPERTIESPROXY_H_
