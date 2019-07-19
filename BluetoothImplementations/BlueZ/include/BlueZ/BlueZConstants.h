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

#ifndef ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_BLUEZCONSTANTS_H_
#define ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_BLUEZCONSTANTS_H_

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

class BlueZConstants {
public:
    /// Name of the BlueZ service in DBus
    static constexpr auto BLUEZ_SERVICE_NAME = "org.bluez";

    /// Name of the BlueZ interface responsible for media stream delivery
    static constexpr auto BLUEZ_MEDIATRANSPORT_INTERFACE = "org.bluez.MediaTransport1";

    /// Name of the BlueZ interface responsible for general device interactions
    static constexpr auto BLUEZ_DEVICE_INTERFACE = "org.bluez.Device1";

    /// Name of the Device1 interface property containig the MAC address of the device
    static constexpr auto BLUEZ_DEVICE_INTERFACE_ADDRESS = "Address";

    /// Name of the Device1 interface property containig the friendly name of the device
    static constexpr auto BLUEZ_DEVICE_INTERFACE_ALIAS = "Alias";

    /// Name of the BlueZ interface responsible for local bluetooth hardware interactions
    static constexpr auto BLUEZ_ADAPTER_INTERFACE = "org.bluez.Adapter1";

    /// Name of the BlueZ interface responsible for media playback control
    static constexpr auto BLUEZ_MEDIA_INTERFACE = "org.bluez.Media1";

    /// Name of the BlueZ interface responsible for media playback control
    static constexpr auto BLUEZ_MEDIA_PLAYER_INTERFACE = "org.bluez.MediaPlayer1";

    /// Name of the BlueZ interface responsible for pairing agents control
    static constexpr auto BLUEZ_AGENTMANAGER_INTERFACE = "org.bluez.AgentManager1";

    /**
     * Name of the internal DBus interface responsible for keeping a list of registered DBus objects. Used to get a
     * list of known bluetooth hardware and known paired devices from the BlueZ.
     */
    static constexpr auto OBJECT_MANAGER_INTERFACE = "org.freedesktop.DBus.ObjectManager";

    /**
     * Name of the internal DBus interface responsible for accessing the DBus objects' properties. Used to track
     * device and adapter states changes.
     */
    static constexpr auto PROPERTIES_INTERFACE = "org.freedesktop.DBus.Properties";
};

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_BLUEZCONSTANTS_H_
