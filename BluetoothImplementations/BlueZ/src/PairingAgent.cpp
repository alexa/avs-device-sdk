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

#include "BlueZ/PairingAgent.h"

#include <iostream>
#include <string>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <BlueZ/BlueZConstants.h>
#include <BlueZ/BlueZDeviceManager.h>

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

/// String to identify log entries originating from this file.
static const std::string TAG{"PairingAgent"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The path we register the PairingAgent object under.
static const std::string AGENT_OBJECT_PATH = "/ACSDK/Bluetooth/Agent";

/**
 * The io_capability of the pairing agent.
 *
 * This is used for devices without a screen or display,
 * and tells BlueZ to use Simple Secure Pairing.
 */
static const std::string CAPABILITY = "NoInputNoOutput";

/// org.bluez.Agent1.Release method name.
static const std::string RELEASE = "Release";

/// org.bluez.Agent1.RequestPinCode method name.
static const std::string REQUEST_PIN_CODE = "RequestPinCode";

/// org.bluez.Agent1.DisplayPinCode method name.
static const std::string DISPLAY_PIN_CODE = "DisplayPinCode";

/// org.bluez.Agent1.RequestPasskey method name.
static const std::string REQUEST_PASSKEY = "RequestPasskey";

/// org.bluez.Agent1.DisplayPasskey method name.
static const std::string DISPLAY_PASSKEY = "DisplayPasskey";

/// org.bluez.Agent1.RequestConfirmation method name.
static const std::string REQUEST_CONFIRMATION = "RequestConfirmation";

/// org.bluez.Agent1.RequestAuthorization method name.
static const std::string REQUEST_AUTHORIZATION = "RequestAuthorization";

/// org.bluez.Agent1.AuthorizeService method name.
static const std::string AUTHORIZE_SERVICE = "AuthorizeService";

/// org.bluez.Agent1.Cancel method name.
static const std::string CANCEL = "Cancel";

/// BlueZ Identifier.
static const std::string BLUEZ_OBJECT_PATH = "/org/bluez";

/// Default Passkey.
const uint32_t DEFAULT_PASSKEY = 0;

/// Default PinCode.
const char* DEFAULT_PIN_CODE = "0000";

// The introspect XML we ue to create the DBus object.
// clang-format off
const char INTROSPECT_XML[] = R"(
<!DOCTYPE node PUBLIC -//freedesktop//DTD D-BUS Object Introspection 1.0//EN
    "http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd";>
<node>
<interface name="org.bluez.Agent1">
    <method name="Release">
    </method>
    <method name="RequestPinCode">
        <arg type="s" direction="out"/>
        <arg name="device" type="o" direction="in"/>
        </method>
    <method name="DisplayPinCode">
        <arg type="s" direction="out"/>
        <arg name="device" type="o" direction="in"/>
        <arg name="pincode" type="s" direction="in"/>
    </method>
    <method name="RequestPasskey">
        <arg type="u" direction="out"/>
        <arg name="device" type="o" direction="in"/>
    </method>
    <method name="DisplayPasskey">
        <arg name="device" type="o" direction="in"/>
        <arg name="passkey" type="u" direction="in"/>
        <arg name="entered" type="q" direction="in"/>
    </method>
    <method name="RequestConfirmation">
        <arg name="device" type="o" direction="in"/>
        <arg name="passkey" type="u" direction="in"/>
    </method>
    <method name="RequestAuthorization">
        <arg name="device" type="o" direction="in"/>
    </method>
    <method name="AuthorizeService">
        <arg name="device" type="o" direction="in"/>
        <arg name="uuid" type="s" direction="in"/>
    </method>
    <method name="Cancel">
    </method>
</interface>
</node>)";
// clang-format on

std::unique_ptr<PairingAgent> PairingAgent::create(std::shared_ptr<DBusConnection> connection) {
    if (!connection) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullConnection"));
        return nullptr;
    }

    auto pairingAgent = std::unique_ptr<PairingAgent>(new PairingAgent(connection));
    if (!pairingAgent->init()) {
        ACSDK_ERROR(LX(__func__).d("reason", "initFailed"));
        return nullptr;
    }

    return pairingAgent;
}

PairingAgent::PairingAgent(std::shared_ptr<DBusConnection> connection) :
        DBusObject(
            connection,
            INTROSPECT_XML,
            AGENT_OBJECT_PATH,
            {
                {RELEASE, &PairingAgent::release},
                {REQUEST_PIN_CODE, &PairingAgent::requestPinCode},
                {DISPLAY_PIN_CODE, &PairingAgent::displayPinCode},
                {REQUEST_PASSKEY, &PairingAgent::requestPasskey},
                {DISPLAY_PASSKEY, &PairingAgent::displayPasskey},
                {REQUEST_CONFIRMATION, &PairingAgent::requestConfirmation},
                {REQUEST_AUTHORIZATION, &PairingAgent::requestAuthorization},
                {AUTHORIZE_SERVICE, &PairingAgent::authorizeService},
                {CANCEL, &PairingAgent::cancel},
            }) {
}

bool PairingAgent::init() {
    if (!registerWithDBus()) {
        return false;
    }

    m_agentManager = DBusProxy::create(BlueZConstants::BLUEZ_AGENTMANAGER_INTERFACE, BLUEZ_OBJECT_PATH);
    if (!m_agentManager) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullAgentManager"));
        return false;
    }

    return registerAgent() && requestDefaultAgent();
}

PairingAgent::~PairingAgent() {
    ACSDK_DEBUG5(LX(__func__));
    unregisterAgent();
}

void PairingAgent::release(GVariant* arguments, GDBusMethodInvocation* invocation) {
    ACSDK_DEBUG5(LX(__func__));
    g_dbus_method_invocation_return_value(invocation, nullptr);
}

void PairingAgent::requestPinCode(GVariant* arguments, GDBusMethodInvocation* invocation) {
    ACSDK_DEBUG5(LX(__func__));
    ACSDK_INFO(LX(__func__).d("pinCode", DEFAULT_PIN_CODE));

    // STRING type.
    auto parameters = g_variant_new("(s)", DEFAULT_PIN_CODE);
    g_dbus_method_invocation_return_value(invocation, parameters);
}

void PairingAgent::displayPinCode(GVariant* arguments, GDBusMethodInvocation* invocation) {
    ACSDK_DEBUG5(LX(__func__));
    g_dbus_method_invocation_return_value(invocation, nullptr);
}

void PairingAgent::requestPasskey(GVariant* arguments, GDBusMethodInvocation* invocation) {
    ACSDK_DEBUG5(LX(__func__));
    ACSDK_INFO(LX(__func__).d("passKey", DEFAULT_PASSKEY));

    // UINT32 type.
    auto parameters = g_variant_new("(u)", DEFAULT_PASSKEY);
    g_dbus_method_invocation_return_value(invocation, parameters);
}

void PairingAgent::displayPasskey(GVariant* arguments, GDBusMethodInvocation* invocation) {
    ACSDK_DEBUG5(LX(__func__));
    g_dbus_method_invocation_return_value(invocation, nullptr);
}

void PairingAgent::requestConfirmation(GVariant* arguments, GDBusMethodInvocation* invocation) {
    ACSDK_DEBUG5(LX(__func__));
    g_dbus_method_invocation_return_value(invocation, nullptr);
}

void PairingAgent::requestAuthorization(GVariant* arguments, GDBusMethodInvocation* invocation) {
    ACSDK_DEBUG5(LX(__func__));
    g_dbus_method_invocation_return_value(invocation, nullptr);
}

void PairingAgent::authorizeService(GVariant* arguments, GDBusMethodInvocation* invocation) {
    ACSDK_DEBUG5(LX(__func__));
    g_dbus_method_invocation_return_value(invocation, nullptr);
}

void PairingAgent::cancel(GVariant* arguments, GDBusMethodInvocation* invocation) {
    ACSDK_DEBUG5(LX(__func__));
    g_dbus_method_invocation_return_value(invocation, nullptr);
}

bool PairingAgent::requestDefaultAgent() {
    ACSDK_DEBUG5(LX(__func__));
    ManagedGError error;

    // OBJECT_PATH type.
    auto parameters = g_variant_new("(o)", AGENT_OBJECT_PATH.c_str());
    m_agentManager->callMethod("RequestDefaultAgent", parameters, error.toOutputParameter());

    if (error.hasError()) {
        ACSDK_ERROR(LX(__func__).d("reason", "requestDefaultAgentFailed").d("error", error.getMessage()));
        return false;
    }

    ACSDK_DEBUG5(LX(__func__).m("requestDefaultAgentSuccessful"));
    return true;
}

bool PairingAgent::registerAgent() {
    ACSDK_DEBUG5(LX(__func__));
    ManagedGError error;

    // OBJECT_PATH & STRING type.
    auto parameters = g_variant_new("(os)", AGENT_OBJECT_PATH.c_str(), CAPABILITY.c_str());
    m_agentManager->callMethod("RegisterAgent", parameters, error.toOutputParameter());

    if (error.hasError()) {
        ACSDK_ERROR(LX(__func__).d("reason", "registerAgentFailed").d("error", error.getMessage()));
        return false;
    }

    ACSDK_DEBUG5(LX(__func__).m("registerAgentDone"));
    return true;
}

bool PairingAgent::unregisterAgent() {
    ACSDK_DEBUG5(LX(__func__));
    ManagedGError error;

    // OBJECT_PATH type.
    auto parameters = g_variant_new("(o)", AGENT_OBJECT_PATH.c_str());
    m_agentManager->callMethod("UnregisterAgent", parameters, error.toOutputParameter());

    if (error.hasError()) {
        ACSDK_ERROR(LX(__func__).d("reason", "unregisterAgentFailed").d("error", error.getMessage()));
        return false;
    }

    ACSDK_DEBUG5(LX(__func__).m("unregisterAgentDone"));
    return true;
}

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK
