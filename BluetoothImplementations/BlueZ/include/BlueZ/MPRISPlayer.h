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

#ifndef ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_MPRISPLAYER_H_
#define ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_MPRISPLAYER_H_

#include <memory>

#include <gio/gio.h>

#include <AVSCommon/Utils/Bluetooth/BluetoothEventBus.h>
#include "BlueZ/DBusObject.h"
#include "BlueZ/DBusProxy.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

/*
 * A @c DBusObject implementing the org.mpris.MediaPlayer2.Player interface.
 * This object will be registered with BlueZ, and will receive method calls when
 * BlueZ receives an AVRCP command.
 *
 * This object does not fully implement the org.mpris.MediaPlayer2.Player interface.
 * The relevant subset in which BlueZ requires and is supported by the SDK is implemented.
 */
class MPRISPlayer : public DBusObject<MPRISPlayer> {
public:
    /// Destructor.
    virtual ~MPRISPlayer();

    /// The default MPRIS object path for players.
    static const std::string MPRIS_OBJECT_PATH;

    /**
     * Creates an instance of the MPRISPlayer.
     *
     * @param connection A DBusConnection object.
     * @param media The org.bluez.Media1 object.
     * @param eventBus The eventbus to notify of the @c AVRCPCommand.
     * @param playerPath The object path to create the player at.
     * This is defaulted to the path specified by MPRIS, but BlueZ will accept other paths as well.
     *
     * @return An instance of the @c MPRISPlayer if successful, else a nullptr.
     */
    static std::unique_ptr<MPRISPlayer> create(
        std::shared_ptr<DBusConnection> connection,
        std::shared_ptr<DBusProxy> media,
        std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> eventBus,
        const std::string& playerPath = MPRIS_OBJECT_PATH);

private:
    /**
     * Constructor.
     *
     * @param connection A DBusConnection object.
     * @param media The org.bluez.Media1 object.
     * @param eventBus The eventbus to notify of the @c AVRCPCommand.
     * @param playerPath The object to create the player at.
     */
    MPRISPlayer(
        std::shared_ptr<DBusConnection> connection,
        std::shared_ptr<DBusProxy> media,
        std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> eventBus,
        const std::string& playerPath);

    /**
     * Performs initialization.
     *
     * @return Whether the operation was succesful or not.
     */
    bool init();

    /**
     * Register this @c MPRISPlayer with the BlueZ Media object.
     *
     * @return Whether the operation was successful.
     */
    bool registerPlayer();

    /**
     * Unregister this @c MPRISPlayer with the BlueZ Media object.
     *
     * @return Whether the operation was successful.
     */
    bool unregisterPlayer();

    /**
     * The default unsupported method.
     *
     * @param arguments The arguments which this DBus method was called with.
     * @invocation A struct containing data about the method invocation.
     */
    void unsupportedMethod(GVariant* arguments, GDBusMethodInvocation* invocation);

    /**
     * Converts the callback to the corresponding @c AVRCPCommandReceived event.
     *
     * @param arguments The arguments which this DBus method was called with.
     * @invocation A struct containing data about the method invocation.
     */
    void toAVRCPCommand(GVariant* arguments, GDBusMethodInvocation* invocation);

    /**
     * Sends the @c AVRCPCommandRecieved event to @c m_eventBus.
     *
     * @param command The command to send.
     */
    void sendEvent(const avsCommon::sdkInterfaces::bluetooth::services::AVRCPCommand& command);

    /// The DBus object path of the player.
    const std::string m_playerPath;

    /// A Proxy for the Media object.
    std::shared_ptr<DBusProxy> m_media;

    /// The event bus on which to send the @c AVRCPCommand.
    std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> m_eventBus;
};

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_MPRISPLAYER_H_
