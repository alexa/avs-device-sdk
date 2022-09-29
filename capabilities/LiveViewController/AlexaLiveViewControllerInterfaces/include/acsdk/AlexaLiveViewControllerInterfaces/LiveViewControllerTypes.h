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
#ifndef ALEXA_CLIENT_SDK_ALEXALIVEVIEWCONTROLLERINTERFACES_INCLUDE_ACSDK_ALEXALIVEVIEWCONTROLLERINTERFACES_LIVEVIEWCONTROLLERTYPES_H_
#define ALEXA_CLIENT_SDK_ALEXALIVEVIEWCONTROLLERINTERFACES_INCLUDE_ACSDK_ALEXALIVEVIEWCONTROLLERINTERFACES_LIVEVIEWCONTROLLERTYPES_H_

#include <chrono>
#include <set>
#include <string>

#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>

namespace alexaClientSDK {
namespace alexaLiveViewControllerInterfaces {

/**
 * Enumeration of audio state properties. Defines the state of the microphone and speaker when the streaming session
 * starts.
 */
enum class AudioState {
    /**
     * For microphone, this enables the microphone on the viewing device. For speaker, this enables audio from the
     * camera to the viewing device.
     */
    UNMUTED,
    /**
     * For microphone, this mutes the microphone on the viewing device. For speaker, this disables audio from the camera
     * to the viewing device.
     */
    MUTED,
    /**
     * Indicates that camera doesn't support audio communication. In this state, the viewing device doesn't display the
     * microphone and speaker.
     */
    DISABLED,
    /// Any other audio state not covered by the ones above.
    UNKNOWN
};

/**
 * Converts an @c AudioState value to an AVS-compliant string.
 *
 * @param audioState The audio state value to convert to a string.
 * @return The @c std::string conversion of the @c AudioState value.
 */
inline std::string audioStateToString(AudioState audioState) {
    switch (audioState) {
        case AudioState::MUTED:
            return "MUTED";
        case AudioState::UNMUTED:
            return "UNMUTED";
        case AudioState::DISABLED:
            return "DISABLED";
        case AudioState::UNKNOWN:
            return "";
    }
    return "";
}

/**
 * Converts an @c AudioState value to a boolean representing the microphone state.
 *
 * @param audioState The audio state value to convert to the microphone state.
 * @return true if @c audioState is unmuted, false otherwise.
 * @return false
 */
inline bool audioStateToMicrophoneState(AudioState audioState) {
    switch (audioState) {
        case AudioState::UNMUTED:
            return true;
        case AudioState::MUTED:
        case AudioState::DISABLED:
        case AudioState::UNKNOWN:
            return false;
    }
    return false;
}

/**
 * Converts a microphone state in boolean (true if unmuted, false otherwise) to an @c AudioState.
 *
 * @param microphoneState The microphone state value to convert to the audio state.
 * @return AudioState::UNMUTED if @c microphoneState is true, AudioState::MUTED otherwise.
 */
inline AudioState microphoneStateToAudioState(bool microphoneState) {
    if (microphoneState) {
        return AudioState::UNMUTED;
    } else {
        return AudioState::MUTED;
    }
}

/**
 * Enumeration of camera state known by the LiveViewController CA.
 */
enum class CameraState {
    /// Camera is connecting currently which means RTCSC session is being initialized.
    CONNECTING,
    /// Camera is currently connected which means RTCSC is ready to deliver the live camera stream.
    CONNECTED,
    /// Camera is disconnected which means it cannot get the live camera stream anymore.
    DISCONNECTED,
    /// Camera has encountered an error which means it needs to reestablish the RTCSC session.
    ERROR,
    /// Camera is in an unknown state.
    UNKNOWN
};

/**
 * Enumeration of motion capability properties of a camera.
 */
enum class MotionCapability {
    /// Camera supports physical panning.
    PHYSICAL_PAN,
    /// Camera can tilt on the vertical axis.
    PHYSICAL_TILT,
    /// Camera supports optical zoom.
    PHYSICAL_ZOOM,
    /// Motion capability unknown.
    UNKNOWN
};

/**
 * Converts a @c MotionCapability value to an AVS-compliant string.
 *
 * @param motionCapability The motion capability value to convert to a string.
 * @return The @c std::string conversion of the @c MotionCapability value.
 */
inline std::string motionCapabilityToString(MotionCapability motionCapability) {
    switch (motionCapability) {
        case MotionCapability::PHYSICAL_PAN:
            return "PHYSICAL_PAN";
        case MotionCapability::PHYSICAL_TILT:
            return "PHYSICAL_TILT";
        case MotionCapability::PHYSICAL_ZOOM:
            return "PHYSICAL_ZOOM";
        case MotionCapability::UNKNOWN:
            return "";
    }
    return "";
}

/**
 * Enumeration of concurrent two way talk state properties. Indicates whether the camera supports two-way communication
 * with the viewing device.
 */
enum class ConcurrentTwoWayTalkState {
    /// Camera supports two-way audio communication with the viewing device.
    ENABLED,
    /// Camera doesn't support two-way audio communication with the viewing device.
    DISABLED,
    /// Any other concurrent two-way talk state not covered by the ones above.
    UNKNOWN
};

/**
 * Converts a @c ConcurrentTwoWayTalkState value to an AVS-compliant string.
 *
 * @param concurrentTwoWayTalkState The concurrent two-way talk state value to convert to a string.
 * @return The @c std::string conversion of the @c ConcurrentTwoWayTalkState value.
 */
inline std::string concurrentTwoWayTalkStateToString(ConcurrentTwoWayTalkState concurrentTwoWayTalkState) {
    switch (concurrentTwoWayTalkState) {
        case ConcurrentTwoWayTalkState::ENABLED:
            return "ENABLED";
        case ConcurrentTwoWayTalkState::DISABLED:
            return "DISABLED";
        case ConcurrentTwoWayTalkState::UNKNOWN:
            return "";
    }
    return "";
}

/**
 * Enumeration of display mode properties. Defines the supported modes to render the stream.
 */
enum class DisplayMode {
    /// The camera live feed stream displays on the entire screen.
    FULL_SCREEN,
    /// The camera live feed stream displays on top of other streams.
    OVERLAY,
    /// Any other display mode not covered by the ones above.
    UNKNOWN
};

/**
 * Converts a @c DisplayMode value to an AVS-compliant string.
 *
 * @param displayMode The display mode value to convert to a string.
 * @return The @c std::string conversion of the @c DisplayMode value.
 */
inline std::string displayModeToString(DisplayMode displayMode) {
    switch (displayMode) {
        case DisplayMode::FULL_SCREEN:
            return "FULL_SCREEN";
        case DisplayMode::OVERLAY:
            return "OVERLAY";
        case DisplayMode::UNKNOWN:
            return "";
    }
    return "";
}

/**
 * Enumeration of live view trigger properties. Defines the reason that the live view session started.
 */
enum class LiveViewTrigger {
    /// The user started the live view streaming session.
    USER_ACTION,
    /// An automated event, such as a doorbell press, triggered the streaming session.
    AUTOMATED_EVENT,
    /// Any other live view trigger mode not covered by the ones above.
    UNKNOWN
};

/**
 * Converts a @c LiveViewTrigger value to an AVS-compliant string.
 *
 * @param liveViewTrigger The live view trigger value to convert to a string.
 * @return The @c std::string conversion of the @c LiveViewTrigger value.
 */
inline std::string liveViewTriggerToString(LiveViewTrigger liveViewTrigger) {
    switch (liveViewTrigger) {
        case LiveViewTrigger::AUTOMATED_EVENT:
            return "AUTOMATED_EVENT";
        case LiveViewTrigger::USER_ACTION:
            return "USER_ACTION";
        case LiveViewTrigger::UNKNOWN:
            return "";
    }
    return "";
}

/**
 * Enumeration of overlay position properties. Defines the position on the screen to display the overlay.
 */
enum class OverlayPosition {
    /// Directs the viewing device to display the overlay on the top right of the screen.
    TOP_RIGHT,
    /// Directs the viewing device to display the overlay on the top left of the screen.
    TOP_LEFT,
    /// Directs the viewing device to display the overlay on the bottom right of the screen.
    BOTTOM_RIGHT,
    /// Directs the viewing device to display the overlay on the bottom left of the screen.
    BOTTOM_LEFT,
    /// Any other overlay position not covered by the ones above.
    UNKNOWN
};

/**
 * Converts an @c OverlayPosition value to an AVS-compliant string.
 *
 * @param overlayPosition The overlay position value to convert to a string.
 * @return The @c std::string conversion of the @c OverlayPosition value.
 */
inline std::string overlayPositionToString(OverlayPosition overlayPosition) {
    switch (overlayPosition) {
        case OverlayPosition::TOP_RIGHT:
            return "TOP_RIGHT";
        case OverlayPosition::TOP_LEFT:
            return "TOP_LEFT";
        case OverlayPosition::BOTTOM_RIGHT:
            return "BOTTOM_RIGHT";
        case OverlayPosition::BOTTOM_LEFT:
            return "BOTTOM_LEFT";
        case OverlayPosition::UNKNOWN:
            return "";
    }
    return "";
}

/**
 * Enumeration of overlay type properties. Defines the type of overlay supported by the viewing device.
 */
enum class OverlayType {
    /// The viewing device supports picture-in-picture display mode.
    PICTURE_IN_PICTURE,
    /// The viewing device doesn't support overlay mode.
    NONE,
    /// Any other overlay type not covered by the ones above.
    UNKNOWN
};

/**
 * Converts an @c OverlayType value to an AVS-compliant string.
 *
 * @param overlayType The overlay type value to convert to a string.
 * @return The @c std::string conversion of the @c OverlayType value.
 */
inline std::string overlayTypeToString(OverlayType overlayType) {
    switch (overlayType) {
        case OverlayType::PICTURE_IN_PICTURE:
            return "PICTURE_IN_PICTURE";
        case OverlayType::NONE:
            return "NONE";
        case OverlayType::UNKNOWN:
            return "";
    }
    return "";
}

/**
 * Enumeration of role properties. Role property is used to specify the role of the device for the streaming session.
 * The device can act as a security camera or a viewer of a security camera stream.
 */
enum class Role {
    /// The device acts as a camera. There can be one camera per streaming session.
    CAMERA,
    /**
     * The device acts as a viewer of the security camera streaming session. There can be multiple viewing devices in a
     * streaming session.
     */
    VIEWER,
    /// Any other role type not covered by the ones above.
    UNKNOWN
};

/**
 * Converts a @c Role value to an AVS-compliant string.
 *
 * @param role The role value to convert to a string.
 * @return The @c std::string conversion of the @c Role value.
 */
inline std::string roleToString(Role role) {
    switch (role) {
        case Role::CAMERA:
            return "CAMERA";
        case Role::VIEWER:
            return "VIEWER";
        case Role::UNKNOWN:
            return "";
    }
    return "";
}

/**
 * Enumeration of status properties. Indicates the reason for the Alexa.StopLiveView directive being received or the
 * Alexa.LiveViewStopped event being sent in their payloads.
 */
enum class Status {
    /// User requested to stop the live view session
    STOP_LIVE_VIEW_REQUESTED,
    /// Alexa can't find the media source
    MEDIA_SOURCE_NOT_FOUND,
    /// Device isn't authorized to access the live view feature
    UNAUTHORIZED,
    /// Device battery is too low to support the live stream
    BATTERY_LEVEL_TOO_LOW,
    /// Media source is asleep
    MEDIA_SOURCE_ASLEEP,
    /// Media source is turned off
    MEDIA_SOURCE_TURNED_OFF
};

/**
 * Enumeration of talk mode properties. Defines the audio communication supported by the camera.
 */
enum class TalkMode {
    /// Camera doesn't support audio communication
    NO_SUPPORT,
    /**
     * The camera supports two-way communication in the style of a walkie-talkie. The user pushes the microphone on the
     * viewing device to talk, and then releases to listen.
     */
    PRESS_AND_HOLD,
    /**
     * The camera supports two-way audio communication. The user taps the microphone on the viewing device to unmute and
     * talk, and then taps to mute the microphone.
     */
    TAP,
    /// Any other talk mode not covered by the ones above.
    UNKNOWN
};

/**
 * Converts a @c TalkMode value to an AVS-compliant string.
 *
 * @param talkMode The talk mode value to convert to a string.
 * @return The @c std::string conversion of the @c TalkMode value.
 */
inline std::string talkModeToString(TalkMode talkMode) {
    switch (talkMode) {
        case TalkMode::PRESS_AND_HOLD:
            return "PRESS_AND_HOLD";
        case TalkMode::TAP:
            return "TAP";
        case TalkMode::NO_SUPPORT:
            return "NO_SUPPORT";
        case TalkMode::UNKNOWN:
            return "";
    }
    return "";
}

/**
 * AudioProperties object. Defines the audio details of the streaming session.
 */
struct AudioProperties {
    /// Audio communication capability of the camera.
    TalkMode talkMode;
    /// Defines whether the camera supports concurrent two-way communication.
    ConcurrentTwoWayTalkState concurrentTwoWayTalk;
    /// State of the microphone at the start of the streaming session.
    AudioState microphoneState;
    /// State of the speaker at the start of the streaming session.
    AudioState speakerState;
};

/**
 * Camera object. Defines details of the camera, such as name, manufacturer, and camera capabilities.
 */
struct Camera {
    /// Friendly name of the camera, such as "front door". Maximum length is 512 characters.
    std::string name;
    /// Name of the manufacturer of the camera. Maximum length is 512 characters.
    std::string make;
    /// (Optional) Model name of the camera. Maximum length is 512 characters.
    avsCommon::utils::Optional<std::string> model;
    /// (Optional) Motion capabilities of the camera.
    avsCommon::utils::Optional<std::set<MotionCapability>> capabilities;
};

/**
 * DisplayProperties object. Defines details of the requested display experience on the viewing device.
 */
struct DisplayProperties {
    /// Display mode in which to render the live stream
    DisplayMode displayMode;
    /**
     * (Optional) If displayMode is @c OVERLAY, the type of overlay to use to render the live stream.
     * If set to NONE, the displayMode defaults to FULL_SCREEN.
     */
    avsCommon::utils::Optional<OverlayType> overlayType;
    /// (Optional) If displayMode is @c OVERLAY, overlayPosition indicates the position on the screen to display the
    /// overlay.
    avsCommon::utils::Optional<OverlayPosition> overlayPosition;
};

/**
 * Viewer object. Defines details of the Alexa device used to view the streaming session.
 */
struct Viewer {
    /**
     * Enum class for the connection state of the viewing device. In the connected state, the user can see the camera
     * feed.
     */
    enum class ViewerState {
        /// Viewing device is connected, which means the user can see the camera feed.
        CONNECTED,
        /// Viewing device is connecting, which means the user can not see the camera feed yet.
        CONNECTING,
        /// Any other state that is not covered by the viewer states above.
        UNKNOWN
    };

    /// Friendly name of the viewing device, such as "Kitchen Echo Show." Maximum length is 512 characters.
    std::string name;
    /**
     * Indicates whether the viewing device has control over the camera. In a list of viewing devices, one device can
     * control the camera at any one time.
     */
    bool hasCameraControl;
    /**
     * Connection state of the viewing device. In the connected state, the user can see the camera feed.
     */
    ViewerState state;

    /**
     * Comparison with another viewer for inserting into set of viewers.
     *
     * @param anotherViewer Other viewer instance this viewer is compared against.
     *
     * @return true if this viewer should be inserted before @c anotherViewer, false otherwise.
     */
    bool operator<(const Viewer& anotherViewer) const {
        return (name < anotherViewer.name);
    }
};

/**
 * Converts a @c ViewerState value to an AVS-compliant string.
 *
 * @param viewerState The viewer state value to convert to a string.
 * @return The @c std::string conversion of the @c ViewerState value.
 */
inline std::string viewerStateToString(Viewer::ViewerState viewerState) {
    switch (viewerState) {
        case Viewer::ViewerState::CONNECTED:
            return "CONNECTED";
        case Viewer::ViewerState::CONNECTING:
            return "CONNECTING";
        case Viewer::ViewerState::UNKNOWN:
            return "";
    }
    return "";
}

/**
 * Target object. Identifies the endpoint of the viewing device.
 */
struct Target {
    /// Enum class for different types of target endpoints. Currently only ALEXA_ENDPOINT is supported.
    enum class TargetType {
        /// Used to represent an Alexa endpoint.
        ALEXA_ENDPOINT,
        /// Any other endpoint that is not supported at this point.
        UNKNOWN
    };

    /// Identifier of the device.
    std::string endpointId;
    /// (Optional) Type of endpoint. Valid value: ALEXA_ENDPOINT.
    avsCommon::utils::Optional<TargetType> type;
};

/**
 * Converts a @c TargetType value to an AVS-compliant string.
 *
 * @param targetType The target type value to convert to a string.
 * @return The @c std::string conversion of the @c TargetType value.
 */
inline std::string targetTypeToString(Target::TargetType targetType) {
    switch (targetType) {
        case Target::TargetType::ALEXA_ENDPOINT:
            return "ALEXA_ENDPOINT";
        case Target::TargetType::UNKNOWN:
            return "";
    }
    return "";
}

/**
 * Participants object. Contains the camera source and a list of viewing devices in the current streaming session.
 */
struct Participants {
    /// List of the viewing devices. At least one viewing device must be specified.
    std::set<Viewer> viewers;
    /// Camera source of the live feed.
    Camera camera;
};

/**
 * ViewerExperience object. Defines the display and audio properties of the streaming session.
 */
struct ViewerExperience {
    /// Display properties of the live streaming session.
    DisplayProperties suggestedDisplay;
    /// Audio communication properties of the live streaming session.
    AudioProperties audioProperties;
    /// Reason the live view streaming session started.
    LiveViewTrigger liveViewTrigger;
    /**
     * Timeout value in milliseconds. Any user interaction with the viewing device cancels the timer. For example, the
     * user enables the microphone, performs pan, tilt, zoom gestures, or switches between full screen and
     * picture-in-picture. A negative value or zero disables the timer. The default is 15000 milliseconds.
     */
    std::chrono::milliseconds idleTimeoutInMilliseconds;
};

}  // namespace alexaLiveViewControllerInterfaces
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ALEXALIVEVIEWCONTROLLERINTERFACES_INCLUDE_ACSDK_ALEXALIVEVIEWCONTROLLERINTERFACES_LIVEVIEWCONTROLLERTYPES_H_
