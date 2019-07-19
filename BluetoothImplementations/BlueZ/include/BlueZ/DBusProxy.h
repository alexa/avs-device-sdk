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

#ifndef ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_DBUSPROXY_H_
#define ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_DBUSPROXY_H_

#include <memory>
#include <string>

#include "BlueZUtils.h"

#include <gio/gunixfdlist.h>

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

/**
 * A wrapper class for DBus proxy objects.
 */
class DBusProxy {
public:
    virtual ~DBusProxy();

    /**
     * Factory method to create a new instance of the DBus proxy. DBus proxies are shortcuts that bind to a specific
     * interface of an object and simplifies operations on that object by avoiding providing the object and its
     * interface during the subsequent calls.
     *
     * @param interfaceName Name of the DBus interface to be proxified
     * @param objectPath Object path to an object who's interface is to be proxified
     * @return A new instance of @c DBusProxy on success, nullptr otherwise.
     */
    static std::shared_ptr<DBusProxy> create(const std::string& interfaceName, const std::string& objectPath);

    /**
     * Calls a method of the proxy and returns the its result.
     *
     * @param methodName Name of the method to invoke
     * @param parameters A @c GVariant* containing the tuple with the parameters for the method or nullptr if method
     * does not have any parameters
     * @param[out] error A pointer to a @c GError* variable that will receive the error returned by method invocation
     * @return The result @c GVariant* returned by method invocation
     */
    virtual ManagedGVariant callMethod(
        const std::string& methodName,
        GVariant* parameters = nullptr,
        GError** error = nullptr);

    /**
     * Calls a method of the proxy and returns the its result along with the list of linux file descriptors associated
     * with it.
     *
     * @param methodName Name of the method to invoke
     * @param parameters A @c GVariant* containing the tuple with the parameters for the method or nullptr if method
     * does not have any parameters
     * @param[out] outlist A pointer to @c GUnixFDList* variable that recieves a list of linux file descriptor list
     * associated with result returned by the method call
     * @param[out] error A pointer to a @c GError* variable that will receive the error returned by method invocation
     * @return The result @c GVariant* returned by method invocation
     */
    virtual ManagedGVariant callMethodWithFDList(
        const std::string& methodName,
        GVariant* parameters = nullptr,
        GUnixFDList** outlist = nullptr,
        GError** error = nullptr);

    /**
     * Get the path of the object being proxified
     *
     * @return A string containing the object path
     */
    virtual std::string getObjectPath() const;

    /**
     * Get the raw @c GDBusProxy* value associated with the object
     *
     * @return A pointer to @c GDBusProxy object associated with the this instance. This pointer is valid as long as
     * @c DBusProxy object is valid.
     */
    virtual GDBusProxy* get();

protected:
    /**
     * Constructor that initializes the object with an existing @c GDBusProxy* value. It also receives object path for
     * the object being proxified to keep it for later use.
     *
     * @param proxy An existing @c GDBusProxy* value
     * @param objectPath Object path of an object being proxified
     */
    DBusProxy(GDBusProxy* proxy, const std::string& objectPath);

private:
    /// A @c GDBusProxy* value associated with an object
    GDBusProxy* m_proxy;

    /// Object path of an object being proxified
    const std::string m_objectPath;
};

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_DBUSPROXY_H_
