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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_BLUETOOTHEVENTS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_BLUETOOTHEVENTS_H_

#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/A2DPSourceInterface.h>

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
    STREAMING_STATE_CHANGED
};

/// Helper struct to allow enum class to be a key in collections
struct BluetoothEventTypeHash {
    template <typename T>
    std::size_t operator()(T t) const {
        return static_cast<std::size_t>(t);
    }
};

/// An Enum representing the current state of the stream.
enum class MediaStreamingState {
    /// Currently not streaming.
    IDLE,
    /// Currently acquiring the stream.
    PENDING,
    /// Currently streaming.
    ACTIVE
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
     * Get @c BluetoothDeviceInterface assoctiated with the event. The value depends on the event type
     * @return @c BluetoothDeviceInterface assoctiated with the event or @c nullptr
     */
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> getDevice() const;

    /**
     * Get @c DeviceState associated with the event
     * @return @c DeviceState associated with the event
     */
    avsCommon::sdkInterfaces::bluetooth::DeviceState getDeviceState() const;

    /**
     * Get @c MediaStreamingState assoctiated with the event
     * @return @c MediaStreamingState assoctiated with the event
     */
    MediaStreamingState getMediaStreamingState() const;

protected:
    /**
     * Constructor
     * @param type Event type
     * @param device @c BluetoothDeviceInterface assoctiated with the event
     * @param deviceState @c DeviceState associated with the event
     * @param mediaStreamingState @c MediaStreamingState assoctiated with the event
     */
    BluetoothEvent(
        BluetoothEventType type,
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device = nullptr,
        avsCommon::sdkInterfaces::bluetooth::DeviceState deviceState =
            avsCommon::sdkInterfaces::bluetooth::DeviceState::IDLE,
        MediaStreamingState mediaStreamingState = MediaStreamingState::IDLE);

private:
    /// Event type
    BluetoothEventType m_type;

    /// @c BluetoothDeviceInterface assoctiated with the event
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> m_device;

    /// @c DeviceState associated with the event
    avsCommon::sdkInterfaces::bluetooth::DeviceState m_deviceState;

    /// @c MediaStreamingState assoctiated with the event
    MediaStreamingState m_mediaStreamingState;
};

inline BluetoothEvent::BluetoothEvent(
    BluetoothEventType type,
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device,
    avsCommon::sdkInterfaces::bluetooth::DeviceState deviceState,
    MediaStreamingState mediaStreamingState) :
        m_type{type},
        m_device{device},
        m_deviceState{deviceState},
        m_mediaStreamingState{mediaStreamingState} {
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
     * @param device Device assoctiated with the event
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
     * @param device Device assoctiated with the event
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
     * @param device Device assoctiated with the event
     * @param newState New media streaming state
     */
    explicit MediaStreamingStateChangedEvent(MediaStreamingState newState);
};

inline MediaStreamingStateChangedEvent::MediaStreamingStateChangedEvent(MediaStreamingState newState) :
        BluetoothEvent(
            BluetoothEventType::STREAMING_STATE_CHANGED,
            nullptr,
            avsCommon::sdkInterfaces::bluetooth::DeviceState::IDLE,
            newState) {
}

}  // namespace bluetooth
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_BLUETOOTHEVENTS_H_
