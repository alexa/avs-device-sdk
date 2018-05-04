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
#include "BlueZ/DBusObjectBase.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

/// String to identify log entries originating from this file.
static const std::string TAG{"DBusObjectBase"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

DBusObjectBase::~DBusObjectBase() {
    ACSDK_DEBUG7(LX(__func__));
    unregisterObject();
}

DBusObjectBase::DBusObjectBase(
    std::shared_ptr<DBusConnection> connection,
    const std::string& xmlInterfaceIntrospection,
    const std::string& objectPath,
    GDBusInterfaceMethodCallFunc methodCallFunc) :
        m_xmlInterfaceIntrospection{xmlInterfaceIntrospection},
        m_registrationId{0},
        m_interfaceVtable{methodCallFunc, nullptr, nullptr},
        m_connection{connection},
        m_objectPath{objectPath} {
}
void DBusObjectBase::onMethodCalledInternal(const gchar* methodName) {
    ACSDK_DEBUG7(LX(__func__).d("methodName", methodName));
}

void DBusObjectBase::unregisterObject() {
    if (m_registrationId > 0) {
        g_dbus_connection_unregister_object(m_connection->getGDBusConnection(), m_registrationId);
        m_registrationId = 0;
    }
}

bool DBusObjectBase::registerWithDBus() {
    ACSDK_DEBUG5(LX(__func__));

    if (m_registrationId > 0) {
        return true;
    }

    ManagedGError error;
    GDBusNodeInfo* data = g_dbus_node_info_new_for_xml(m_xmlInterfaceIntrospection.c_str(), error.toOutputParameter());
    if (error.hasError()) {
        ACSDK_ERROR(LX("Failed to register object").d("error", error.getMessage()));
        return false;
    }

    GDBusInterfaceInfo* interfaceInfo = data->interfaces[0];

    m_registrationId = g_dbus_connection_register_object(
        m_connection->getGDBusConnection(),
        m_objectPath.c_str(),
        interfaceInfo,
        &m_interfaceVtable,
        this,
        nullptr,
        nullptr);
    ACSDK_DEBUG5(LX("Object registered").d("Object path", m_objectPath).d("Interface", interfaceInfo->name));
    return m_registrationId > 0;
}

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK
