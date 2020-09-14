/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_SERVICES_AVRCPTARGETINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_SERVICES_AVRCPTARGETINTERFACE_H_

#include <ostream>
#include <string>

#include "AVSCommon/SDKInterfaces/Bluetooth/Services/BluetoothServiceInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace bluetooth {
namespace services {

// TODO: Move to own enum file.
/// An Enum representing Media commands.
enum class MediaCommand {
    /// A Play command.
    PLAY,

    /// A Pause command.
    PAUSE,

    /// A Next command.
    NEXT,

    /// A Previous command. If issued at the beginning of a song, the previous track will be selected.
    PREVIOUS,

    /// A Play/Pause command.
    PLAY_PAUSE
};

/**
 * Converts the @c MediaCommand enum to a string.
 *
 * @param cmd The @c MediaCommand to convert.
 * @return A string representation of the @c MediaCommand.
 */
inline std::string commandToString(MediaCommand cmd) {
    switch (cmd) {
        case MediaCommand::PLAY:
            return "PLAY";
        case MediaCommand::PAUSE:
            return "PAUSE";
        case MediaCommand::NEXT:
            return "NEXT";
        case MediaCommand::PREVIOUS:
            return "PREVIOUS";
        case MediaCommand::PLAY_PAUSE:
            return "PLAY_PAUSE";
    }

    return "UNKNOWN";
}

/**
 * Overload for the @c MediaCommand enum. This will write the @c MediaCommand as a string to the provided stream.
 *
 * @param stream An ostream to send the DeviceState as a string.
 * @param cmd The @c MediaCommand to convert.
 * @return The stream.
 */
inline std::ostream& operator<<(std::ostream& stream, const MediaCommand cmd) {
    return stream << commandToString(cmd);
}

/// Used to implement an instance of AVRCPTarget (Audio/Video Remote Control Profile).
class AVRCPTargetInterface : public BluetoothServiceInterface {
public:
    /// The Service UUID.
    static constexpr const char* UUID = "0000110c-0000-1000-8000-00805f9b34fb";

    /// The Service Name.
    static constexpr const char* NAME = "A/V_RemoteControlTarget";

    /**
     * Sends a play command to the device supporting AVRCPTarget.
     *
     * @return A boolean indicating the success of the function.
     */
    virtual bool play() = 0;

    /**
     * Sends a pause command to the device supporting AVRCPTarget.
     *
     * @return A boolean indicating the success of the function.
     */
    virtual bool pause() = 0;

    /**
     * Sends a next command to the device supporting AVRCPTarget.
     *
     * @return A boolean indicating the success of the function.
     */
    virtual bool next() = 0;

    /**
     * Sends a previous command to the device supporting AVRCPTarget.
     *
     * @return A boolean indicating the success of the function.
     */
    virtual bool previous() = 0;

    /**
     * Destructor.
     */
    virtual ~AVRCPTargetInterface() = default;
};

}  // namespace services
}  // namespace bluetooth
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  //  ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_SERVICES_AVRCPTARGETINTERFACE_H_
