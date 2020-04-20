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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_BLUETOOTHEVENTS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_BLUETOOTHEVENTS_H_

#include "AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceInterface.h"
#include "AVSCommon/SDKInterfaces/Bluetooth/Services/A2DPSourceInterface.h"
#include "AVSCommon/SDKInterfaces/Bluetooth/Services/AVRCPTargetInterface.h"
#include "AVSCommon/Utils/Bluetooth/A2DPRole.h"
#include "AVSCommon/Utils/Bluetooth/MediaStreamingState.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace bluetooth {

/// The different Bluetooth event types.
enum class BluetoothEventType {
    /// Represents when a device is discovered.
    DEVICE_DISCOVERED,

    /// Represents when a device is removed.
    DEVICE_REMOVED,

    /// Represents when the state of a device changes.
    DEVICE_STATE_CHANGED,

    /// Represents when the A2DP streaming state changes.
    STREAMING_STATE_CHANGED,

    /// Represents when a media command has been received.
    MEDIA_COMMAND_RECEIVED,

    /// When the BluetoothDeviceManager has initialized.
    BLUETOOTH_DEVICE_MANAGER_INITIALIZED,

    /// When the scanning state of the host has changed.
    SCANNING_STATE_CHANGED,

    /// Request to connect/disconnect a certain profile
    TOGGLE_A2DP_PROFILE_STATE_CHANGED
};

/// Helper struct to allow enum class to be a key in collections
struct BluetoothEventTypeHash {
    template <typename T>
    std::size_t operator()(T t) const {
        return static_cast<std::size_t>(t);
    }
};

/// Base class for a @c BluetoothEvent.
class BluetoothEvent {
public:
    /**
     * Destructor.
     */
    virtual ~BluetoothEvent() = default;

    /**
     * Get event type
     * @return Event type.
     */
    BluetoothEventType getType() const;

    /**
     * Get @c BluetoothDeviceInterface associated with the event. The value depends on the event type
     * @return @c BluetoothDeviceInterface associated with the event or @c nullptr
     */
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> getDevice() const;

    /**
     * Get @c DeviceState associated with the event
     * @return @c DeviceState associated with the event
     */
    avsCommon::sdkInterfaces::bluetooth::DeviceState getDeviceState() const;

    /**
     * Get @c MediaStreamingState associated with the event
     * @return @c MediaStreamingState associated with the event
     */
    MediaStreamingState getMediaStreamingState() const;

    /**
     * Get @c A2DPRole associated with the event
     * @return @c A2DPRole associated with the event or null if not applicable.
     */
    std::shared_ptr<A2DPRole> getA2DPRole() const;

    /**
     * Get @c MediaCommand associated with the event
     * @return @c MediaCommand associated with the event or null if not applicable
     */
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::services::MediaCommand> getMediaCommand() const;

    /**
     * Get the current scanning state of the host device.
     *
     * @return Whether the device is currently scanning for other devices.
     */
    bool isScanning() const;

    /**
     * Get the current enable/disable state of the A2DP profiles.
     *
     * @return Whether the event requires to enable the A2DP profiles.
     */
    bool isA2DPEnabled() const;

protected:
    /**
     * Constructor
     * @param type Event type
     * @param device @c BluetoothDeviceInterface associated with the event
     * @param deviceState @c DeviceState associated with the event
     * @param mediaStreamingState @c MediaStreamingState associated with the event
     * @param role The A2DP role the AVS device is acting as
     * @param mediaCommand The received MediaCommand if applicable
     */
    BluetoothEvent(
        BluetoothEventType type,
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device = nullptr,
        avsCommon::sdkInterfaces::bluetooth::DeviceState deviceState =
            avsCommon::sdkInterfaces::bluetooth::DeviceState::IDLE,
        MediaStreamingState mediaStreamingState = MediaStreamingState::IDLE,
        std::shared_ptr<A2DPRole> role = nullptr,
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::services::MediaCommand> mediaCommand = nullptr,
        bool scanningState = false,
        bool a2dpEnable = false);

private:
    /// Event type
    BluetoothEventType m_type;

    /// @c BluetoothDeviceInterface associated with the event
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> m_device;

    /// @c DeviceState associated with the event
    avsCommon::sdkInterfaces::bluetooth::DeviceState m_deviceState;

    /// @c MediaStreamingState associated with the event
    MediaStreamingState m_mediaStreamingState;

    /// @c A2DPRole associated with the event
    std::shared_ptr<A2DPRole> m_a2dpRole;

    /// @C MediaCommand that is received
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::services::MediaCommand> m_mediaCommand;

    /// The scanning state of the host device
    bool m_scanningState;

    /// The enable state of the A2DP profiles
    bool m_a2dpEnable;
};

inline BluetoothEvent::BluetoothEvent(
    BluetoothEventType type,
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device,
    avsCommon::sdkInterfaces::bluetooth::DeviceState deviceState,
    MediaStreamingState mediaStreamingState,
    std::shared_ptr<A2DPRole> a2dpRole,
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::services::MediaCommand> mediaCommand,
    bool scanningState,
    bool a2dpEnable) :
        m_type{type},
        m_device{device},
        m_deviceState{deviceState},
        m_mediaStreamingState{mediaStreamingState},
        m_a2dpRole{a2dpRole},
        m_mediaCommand{mediaCommand},
        m_scanningState{scanningState},
        m_a2dpEnable{a2dpEnable} {
}

inline BluetoothEventType BluetoothEvent::getType() const {
    return m_type;
}

inline std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> BluetoothEvent::getDevice()
    const {
    return m_device;
}

inline avsCommon::sdkInterfaces::bluetooth::DeviceState BluetoothEvent::getDeviceState() const {
    return m_deviceState;
}

inline MediaStreamingState BluetoothEvent::getMediaStreamingState() const {
    return m_mediaStreamingState;
}

inline std::shared_ptr<A2DPRole> BluetoothEvent::getA2DPRole() const {
    return m_a2dpRole;
}

inline std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::services::MediaCommand> BluetoothEvent::getMediaCommand()
    const {
    return m_mediaCommand;
}

inline bool BluetoothEvent::isScanning() const {
    return m_scanningState;
}

inline bool BluetoothEvent::isA2DPEnabled() const {
    return m_a2dpEnable;
}

/**
 * Event indicating that a new device was discovered. This must be sent when
 * a new device is discovered with a reference to the @c BluetoothDeviceInterface.
 */
class DeviceDiscoveredEvent : public BluetoothEvent {
public:
    /**
     * Constructor
     * @param device Device associated with the event
     */
    explicit DeviceDiscoveredEvent(
        const std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>& device);
};

inline DeviceDiscoveredEvent::DeviceDiscoveredEvent(
    const std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>& device) :
        BluetoothEvent(
            BluetoothEventType::DEVICE_DISCOVERED,
            device,
            avsCommon::sdkInterfaces::bluetooth::DeviceState::IDLE,
            MediaStreamingState::IDLE) {
}

/**
 * Event indicating that a device is removed from the underlying stack, if applicable.
 * This signifies that the stack is no longer aware of the device. For example,
 * if the stack forgets an unpaired device. This must be sent with a reference
 * to the @c BluetoothDeviceInterface.
 */
class DeviceRemovedEvent : public BluetoothEvent {
public:
    /**
     * Constructor
     * @param device Device associated with the event
     */
    explicit DeviceRemovedEvent(
        const std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>& device);
};

inline DeviceRemovedEvent::DeviceRemovedEvent(
    const std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>& device) :
        BluetoothEvent(
            BluetoothEventType::DEVICE_REMOVED,
            device,
            avsCommon::sdkInterfaces::bluetooth::DeviceState::IDLE,
            MediaStreamingState::IDLE) {
}

/**
 * Event indicating that a device undergoes a state transition. The available states are
 * dictated by @c DeviceState. This must be sent with a reference to the @c BluetoothDeviceInterface.
 */
class DeviceStateChangedEvent : public BluetoothEvent {
public:
    /**
     * Constructor
     * @param device Device associated with the event
     * @param newState New state of the devices
     */
    DeviceStateChangedEvent(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device,
        avsCommon::sdkInterfaces::bluetooth::DeviceState newState);
};

inline DeviceStateChangedEvent::DeviceStateChangedEvent(
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device,
    avsCommon::sdkInterfaces::bluetooth::DeviceState newState) :
        BluetoothEvent(BluetoothEventType::DEVICE_STATE_CHANGED, device, newState, MediaStreamingState::IDLE) {
}

/**
 * Event indicating that a device's streaming state has changed. This refers to the
 * A2DP profile. This must be sent on transitions between @c MediaStreamingState.
 */
class MediaStreamingStateChangedEvent : public BluetoothEvent {
public:
    /**
     * Constructor
     * @param newState New media streaming state
     * @param role The A2DPRole associated with the device running the SDK
     * @param device The peer device that is connected and whose streaming status changed
     */
    explicit MediaStreamingStateChangedEvent(
        MediaStreamingState newState,
        A2DPRole role,
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device);
};

inline MediaStreamingStateChangedEvent::MediaStreamingStateChangedEvent(
    MediaStreamingState newState,
    A2DPRole role,
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device) :
        BluetoothEvent(
            BluetoothEventType::STREAMING_STATE_CHANGED,
            device,
            avsCommon::sdkInterfaces::bluetooth::DeviceState::IDLE,
            newState,
            std::make_shared<A2DPRole>(role)){};

/**
 * Event indicating that an AVRCP command was received.
 */
class MediaCommandReceivedEvent : public BluetoothEvent {
public:
    explicit MediaCommandReceivedEvent(avsCommon::sdkInterfaces::bluetooth::services::MediaCommand command);
};

inline MediaCommandReceivedEvent::MediaCommandReceivedEvent(
    avsCommon::sdkInterfaces::bluetooth::services::MediaCommand command) :
        BluetoothEvent(
            BluetoothEventType::MEDIA_COMMAND_RECEIVED,
            nullptr,
            avsCommon::sdkInterfaces::bluetooth::DeviceState::IDLE,
            MediaStreamingState::IDLE,
            nullptr,
            std::make_shared<avsCommon::sdkInterfaces::bluetooth::services::MediaCommand>(command)) {
}

/**
 * Event indicating that the BluetoothDeviceManager has finished initialization. This should only be sent once.
 */
class BluetoothDeviceManagerInitializedEvent : public BluetoothEvent {
public:
    explicit BluetoothDeviceManagerInitializedEvent();
};

inline BluetoothDeviceManagerInitializedEvent::BluetoothDeviceManagerInitializedEvent() :
        BluetoothEvent(BluetoothEventType::BLUETOOTH_DEVICE_MANAGER_INITIALIZED) {
}

/**
 * Event indicating that the scanning state on the host device has changed.
 */
class ScanningStateChangedEvent : public BluetoothEvent {
public:
    explicit ScanningStateChangedEvent(bool isScanning);
};

inline ScanningStateChangedEvent::ScanningStateChangedEvent(bool isScanning) :
        BluetoothEvent(
            BluetoothEventType::SCANNING_STATE_CHANGED,
            nullptr,
            avsCommon::sdkInterfaces::bluetooth::DeviceState::IDLE,
            MediaStreamingState::IDLE,
            nullptr,
            nullptr,
            isScanning) {
}

/**
 * Event indicating that the request to disconnect/connect certain profiles on devices
 */
class ToggleA2DPProfileStateChangedEvent : public BluetoothEvent {
public:
    explicit ToggleA2DPProfileStateChangedEvent(bool a2dpEnable);
};

inline ToggleA2DPProfileStateChangedEvent::ToggleA2DPProfileStateChangedEvent(bool a2dpEnable) :
        BluetoothEvent(
            BluetoothEventType::TOGGLE_A2DP_PROFILE_STATE_CHANGED,
            nullptr,
            avsCommon::sdkInterfaces::bluetooth::DeviceState::IDLE,
            MediaStreamingState::IDLE,
            nullptr,
            nullptr,
            false,
            a2dpEnable) {
}

}  // namespace bluetooth
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_BLUETOOTHEVENTS_H_
