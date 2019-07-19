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

#include <string>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Bluetooth/BluetoothEvents.h>
#include "BlueZ/BlueZConstants.h"
#include "BlueZ/BlueZDeviceManager.h"

#include "BlueZ/MPRISPlayer.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

using namespace avsCommon::sdkInterfaces::bluetooth::services;
using namespace avsCommon::utils::bluetooth;

/// String to identify log entries originating from this file.
static const std::string TAG{"MPRISPlayer"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Property indicating whether this player supports the Next functionality.
static const std::string CAN_GO_NEXT = "CanGoNext";

/// Property indicating whether this player supports the Previous functionality.
static const std::string CAN_GO_PREVIOUS = "CanGoPrevious";

/// Property indicating whether this player supports the Play functionality.
static const std::string CAN_PLAY = "CanPlay";

/// Property indicating whether this player supports the Pause functionality.
static const std::string CAN_PAUSE = "CanPause";

/// Property indicating whether this player supports the Seek functionality.
static const std::string CAN_SEEK = "CanSeek";

/// Property indicating whether this player supports the Next functionality.
static const std::string CAN_CONTROL = "CanControl";

/// The Play method.
static const std::string PLAY = "Play";

/// The Pause method.
static const std::string PAUSE = "Pause";

/// The Next method.
static const std::string NEXT = "Next";

/// The Previous method.
static const std::string PREVIOUS = "Previous";

/// The PlayPause method.
static const std::string PLAY_PAUSE = "PlayPause";

/// The Stop method.
static const std::string STOP = "Stop";

/// The Seek method.
static const std::string SEEK = "Seek";

/// The SetPosition method.
static const std::string SET_POSITION = "SetPosition";

/// The OpenUri method.
static const std::string OPEN_URI = "OpenUri";

/// The RegisterPlayer method on the org.bluez.Media1 API.
static const std::string REGISTER_PLAYER = "RegisterPlayer";

/// The UnregisterPlayer method on the org.bluez.Media1 API.
static const std::string UNREGISTER_PLAYER = "UnregisterPlayer";

/// The introspect XML we use to create the DBus object.
// clang-format off
const char INTROSPECT_XML[] = R"(
<!DOCTYPE node PUBLIC -//freedesktop//DTD D-BUS Object Introspection 1.0//EN
    "http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd";>
<node>
<interface name="org.mpris.MediaPlayer2.Player">
    <method name="Next"/>
    <method name="Previous"/>
    <method name="Pause"/>
    <method name="PlayPause"/>
    <method name="Stop"/>
    <method name="Play"/>
    <method name="Seek">
        <arg type="x" direction="in"/>
    </method>
    <method name="SetPosition">
        <arg type="o" direction="in"/>
        <arg type="x" direction="in"/>
    </method>
    <method name="OpenUri">
        <arg type="s" direction="in"/>
    </method>
</interface>
</node>)";

const std::string MPRISPlayer::MPRIS_OBJECT_PATH = "/org/mpris/MediaPlayer2";

std::unique_ptr<MPRISPlayer> MPRISPlayer::create(
    std::shared_ptr<DBusConnection> connection,
    std::shared_ptr<DBusProxy> media,
    std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> eventBus,
    const std::string& playerPath) {
    if (!connection) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullConnection"));
        return nullptr;
    } else if (!media) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullMediaManager"));
        return nullptr;
    } else if (!eventBus) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullEventBus"));
        return nullptr;
    }

    auto mediaPlayer = std::unique_ptr<MPRISPlayer>(new MPRISPlayer(connection, media, eventBus, playerPath));
    if (!mediaPlayer->init()) {
        ACSDK_ERROR(LX(__func__).d("reason", "initFailed"));
        return nullptr;
    }

    return mediaPlayer;
}

MPRISPlayer::MPRISPlayer(
    std::shared_ptr<DBusConnection> connection,
    std::shared_ptr<DBusProxy> media,
    std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> eventBus,
    const std::string& playerPath) :
        DBusObject(
            connection,
            INTROSPECT_XML,
            playerPath,
            {
                {NEXT, &MPRISPlayer::toAVRCPCommand},
                {PREVIOUS, &MPRISPlayer::toAVRCPCommand},
                {PAUSE, &MPRISPlayer::toAVRCPCommand},
                {PLAY, &MPRISPlayer::toAVRCPCommand},
                {PLAY_PAUSE, &MPRISPlayer::unsupportedMethod},
                {STOP, &MPRISPlayer::unsupportedMethod},
                {SEEK, &MPRISPlayer::unsupportedMethod},
                {SET_POSITION, &MPRISPlayer::unsupportedMethod},
                {OPEN_URI, &MPRISPlayer::unsupportedMethod},
            }),
        m_playerPath{playerPath},
        m_media{media},
        m_eventBus{eventBus} {
}

bool MPRISPlayer::init() {
    if (!registerWithDBus()) {
        ACSDK_ERROR(LX(__func__).d("reason", "createDBusObjectFailed"));
        return false;
    }

    return registerPlayer();
}

MPRISPlayer::~MPRISPlayer() {
    ACSDK_DEBUG5(LX(__func__));
    unregisterPlayer();
}

void MPRISPlayer::unsupportedMethod(GVariant* arguments, GDBusMethodInvocation* invocation) {
    ACSDK_WARN(LX(__func__).d("methodName", g_dbus_method_invocation_get_method_name(invocation)));

    g_dbus_method_invocation_return_value(invocation, nullptr);
}

void MPRISPlayer::toAVRCPCommand(GVariant* arguments, GDBusMethodInvocation* invocation) {
    const char* method = g_dbus_method_invocation_get_method_name(invocation);

    ACSDK_DEBUG5(LX(__func__).d("method", method));

    if (PLAY == method) {
        sendEvent(AVRCPCommand::PLAY);
    } else if (PAUSE == method) {
        sendEvent(AVRCPCommand::PAUSE);
    } else if (NEXT == method) {
        sendEvent(AVRCPCommand::NEXT);
    } else if (PREVIOUS == method) {
        sendEvent(AVRCPCommand::PREVIOUS);
    } else {
        ACSDK_ERROR(LX(__func__).d("reason", "unsupported").d("method", method));
    }

    g_dbus_method_invocation_return_value(invocation, nullptr);
}

void MPRISPlayer::sendEvent(const AVRCPCommand& command) {
    ACSDK_DEBUG5(LX(__func__).d("command", command));

    AVRCPCommandReceivedEvent event(command);
    m_eventBus->sendEvent(event);
}

bool MPRISPlayer::registerPlayer() {
    ACSDK_DEBUG5(LX(__func__));

    // Build Dict of properties.
    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));

    // Subset of properties used for AVRCP Commands.
    g_variant_builder_add(&builder, "{sv}", CAN_GO_NEXT.c_str(), g_variant_new("b", TRUE));
    g_variant_builder_add(&builder, "{sv}", CAN_GO_PREVIOUS.c_str(), g_variant_new("b", TRUE));
    g_variant_builder_add(&builder, "{sv}", CAN_PLAY.c_str(), g_variant_new("b", TRUE));
    g_variant_builder_add(&builder, "{sv}", CAN_PAUSE.c_str(), g_variant_new("b", TRUE));
    g_variant_builder_add(&builder, "{sv}", CAN_SEEK.c_str(), g_variant_new("b", FALSE));
    g_variant_builder_add(&builder, "{sv}", CAN_CONTROL.c_str(), g_variant_new("b", TRUE));

    GVariant* properties = g_variant_builder_end(&builder);

    // OBJECT_PATH and ARRAY of STRING:VARIANT (Dictionary).
    auto parameters = g_variant_new("(o@a{sv})", m_playerPath.c_str(), properties);

    ManagedGError error;
    m_media->callMethod(REGISTER_PLAYER, parameters, error.toOutputParameter());

    if (error.hasError()) {
        ACSDK_ERROR(LX(__func__).d("reason", "registerPlayerFailed").d("error", error.getMessage()));
        return false;
    }

    ACSDK_DEBUG(LX("registerPlayerSucceeded").d("path", m_playerPath));

    return true;
}

bool MPRISPlayer::unregisterPlayer() {
    ACSDK_DEBUG5(LX(__func__));

    ManagedGError error;

    // OBJECT_PATH type.
    auto parameters = g_variant_new("(o)", m_playerPath.c_str());
    m_media->callMethod(UNREGISTER_PLAYER, parameters, error.toOutputParameter());

    if (error.hasError()) {
        ACSDK_ERROR(LX(__func__).d("reason", "unregisterPlayerFailed").d("error", error.getMessage()));
        return false;
    }

    ACSDK_DEBUG(LX(__func__).m("unregisterPlayerSucceeded"));

    return true;
}

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK
