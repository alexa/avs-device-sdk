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

#include "acsdk/AlexaLiveViewController/private/AlexaLiveViewControllerCapabilityAgent.h"

#include <chrono>
#include <memory>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <AVSCommon/AVS/EventBuilder.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace alexaLiveViewController {

using namespace alexaClientSDK::avsCommon::avs;
using namespace alexaClientSDK::avsCommon;
using namespace alexaClientSDK::avsCommon::sdkInterfaces;
using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::utils::configuration;
using namespace alexaClientSDK::avsCommon::utils::json;
using namespace alexaLiveViewControllerInterfaces;

/// String to identify log entries originating from this file.
#define TAG "AlexaLiveViewControllerCapabilityAgent"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) logger::LogEntry(TAG, event)

/// The namespace for this capability agent.
static const char* const NAMESPACE = "Alexa.Camera.LiveViewController";

/// The supported version
static const char* const INTERFACE_VERSION = "1.7";

/// The name for StartLiveView directive
static const char* const NAME_STARTLIVEVIEW = "StartLiveView";

/// The name for StopLiveView directive
static const char* const NAME_STOPLIVEVIEW = "StopLiveView";

/// The StartLiveView directive signature.
static const NamespaceAndName START_LIVE_VIEW{NAMESPACE, NAME_STARTLIVEVIEW};

/// The StopLiveView directive signature.
static const NamespaceAndName STOP_LIVE_VIEW{NAMESPACE, NAME_STOPLIVEVIEW};

/// Supported display modes key
static const char* const CONFIGURATION_DISPLAY_MODES_KEY = "supportedDisplayModes";

/// Supported overlay types key
static const char* const CONFIGURATION_OVERLAY_TYPES_KEY = "supportedOverlayTypes";

/// Supported overlay positions key
static const char* const CONFIGURATION_OVERLAY_POSITIONS_KEY = "supportedOverlayPositions";

/// App identifier sent to RTCSessionController App Client.
static const char* const APP_IDENTIFIER = "SmartHome-LiveView";

/// The configuration key
static const char* const CAPABILITY_CONFIGURATION_KEY = "configurations";

/// LiveViewStarted event name.
static const char* const LIVE_VIEW_STARTED_EVENT_NAME = "LiveViewStarted";

/// LiveViewStopped event name.
static const char* const LIVE_VIEW_STOPPED_EVENT_NAME = "LiveViewStopped";

/// Identifier for the SessionId.
static const char* const SESSION_ID_FIELD = "sessionId";

/// Identifier for the Target.
static const char* const TARGET_FIELD = "target";

/// Identifier for a target's endpointId.
static const char* const ENDPOINT_ID_FIELD = "endpointId";

/// Identifier for a target's type.
static const char* const TYPE_FIELD = "type";

/// Identifier for the Role.
static const char* const ROLE_FIELD = "role";

/// Identifier for the participants.
static const char* const PARTICIPANTS_FIELD = "participants";

/// Identifier for the viewers in participants.
static const char* const VIEWERS_FIELD = "viewers";

/// Identifier for the name of a viewer.
static const char* const VIEWER_NAME_FIELD = "name";

/// Identifier for the hasCameraControl property of a viewer.
static const char* const VIEWER_HAS_CAMERA_CONTROL_FIELD = "hasCameraControl";

/// Identifier for the state of a viewer.
static const char* const VIEWER_STATE_FIELD = "state";

/// Identifier for the camera in participants.
static const char* const CAMERA_FIELD = "camera";

/// Identifier for the camera name in participants.
static const char* const CAMERA_NAME_FIELD = "name";

/// Identifier for the camera make in participants.
static const char* const CAMERA_MAKE_FIELD = "make";

/// Identifier for the camera model in participants.
static const char* const CAMERA_MODEL_FIELD = "model";

/// Identifier for the camera capabilities in participants.
static const char* const CAMERA_CAPABILITIES_FIELD = "capabilities";

/// Identifier for the viewerExperience.
static const char* const VIEWER_EXPERIENCE_FIELD = "viewerExperience";

/// Identifier for a viewerExperience's suggestedDisplay.
static const char* const SUGGESTED_DISPLAY_FIELD = "suggestedDisplay";

/// Identifier for a suggestedDisplay's displayMode.
static const char* const DISPLAY_MODE_FIELD = "displayMode";

/// Identifier for a suggestedDisplay's overlayType.
static const char* const OVERLAY_TYPE_FIELD = "overlayType";

/// Identifier for a suggestedDisplay's overlayPosition.
static const char* const OVERLAY_POSITION_FIELD = "overlayPosition";

/// Identifier for a viewerExperience's audioProperties.
static const char* const AUDIO_PROPERTIES_FIELD = "audioProperties";

/// Identifier for an audioProperties' talkMode.
static const char* const TALK_MODE_FIELD = "talkMode";

/// Identifier for an audioProperties' concurrentTwoWayTalk.
static const char* const CONCURRENT_TWO_WAY_TALK_FIELD = "concurrentTwoWayTalk";

/// Identifier for an audioProperties' microphoneState.
static const char* const MICROPHONE_STATE_FIELD = "microphoneState";

/// Identifier for an audioProperties' speakerState.
static const char* const SPEAKER_STATE_FIELD = "speakerState";

/// Identifier for a viewerExperience's liveViewTrigger.
static const char* const LIVE_VIEW_TRIGGER_FIELD = "liveViewTrigger";

/// Identifier for a viewerExperience's idleTimeoutInMilliseconds.
static const char* const IDLE_TIMEOUT_FIELD = "idleTimeoutInMilliseconds";

/**
 * Converts the given string to a @c ConcurrentTwoWayTalkState.
 *
 * @param concurrentTwoWayTalkStateString String representing the concurrentTwoWayTalkState.
 * @return ConcurrentTwoWayTalkState matching the given string.
 */
static ConcurrentTwoWayTalkState concurrentTwoWayTalkFromString(const std::string& concurrentTwoWayTalkStateString) {
    if ("ENABLED" == concurrentTwoWayTalkStateString) {
        return ConcurrentTwoWayTalkState::ENABLED;
    } else if ("DISABLED" == concurrentTwoWayTalkStateString) {
        return ConcurrentTwoWayTalkState::DISABLED;
    } else {
        return ConcurrentTwoWayTalkState::UNKNOWN;
    }
}

/**
 * Converts the given string to a @c TargetType.
 *
 * @param targetTypeString String representing the TargetType.
 * @return TargetType matching the given string.
 */
static Target::TargetType targetTypeFromString(const std::string& targetTypeString) {
    if ("ALEXA_ENDPOINT" == targetTypeString) {
        return Target::TargetType::ALEXA_ENDPOINT;
    } else {
        return Target::TargetType::UNKNOWN;
    }
}

/**
 * Converts the given string to a @c Role.
 *
 * @param roleString String representing the Role.
 * @return Role matching the given string.
 */
static Role roleFromString(const std::string& roleString) {
    if ("CAMERA" == roleString) {
        return Role::CAMERA;
    } else if ("VIEWER" == roleString) {
        return Role::VIEWER;
    } else {
        return Role::UNKNOWN;
    }
}

/**
 * Converts the given string to a @c ViewerState.
 *
 * @param viewerStateString String representing the ViewerState.
 * @return ViewerState matching the given string.
 */
static Viewer::ViewerState viewerStateFromString(const std::string& viewerStateString) {
    if ("CONNECTED" == viewerStateString) {
        return Viewer::ViewerState::CONNECTED;
    } else if ("CONNECTING" == viewerStateString) {
        return Viewer::ViewerState::CONNECTING;
    } else {
        return Viewer::ViewerState::UNKNOWN;
    }
}

/**
 * Converts the given string to a @c DisplayMode.
 *
 * @param displayModeString String representing the DisplayMode.
 * @return DisplayMode matching the given string.
 */
static DisplayMode displayModeFromString(const std::string& displayModeString) {
    if ("FULL_SCREEN" == displayModeString) {
        return DisplayMode::FULL_SCREEN;
    } else if ("OVERLAY" == displayModeString) {
        return DisplayMode::OVERLAY;
    } else {
        return DisplayMode::UNKNOWN;
    }
}

/**
 * Converts the given string to a @c OverlayPosition.
 *
 * @param overlayPositionString String representing the OverlayPosition.
 * @return OverlayPosition matching the given string.
 */
static OverlayPosition overlayPositionFromString(const std::string& overlayPositionString) {
    if ("TOP_RIGHT" == overlayPositionString) {
        return OverlayPosition::TOP_RIGHT;
    } else if ("TOP_LEFT" == overlayPositionString) {
        return OverlayPosition::TOP_LEFT;
    } else if ("BOTTOM_RIGHT" == overlayPositionString) {
        return OverlayPosition::BOTTOM_RIGHT;
    } else if ("BOTTOM_LEFT" == overlayPositionString) {
        return OverlayPosition::BOTTOM_LEFT;
    } else {
        return OverlayPosition::UNKNOWN;
    }
}

/**
 * Converts the given string to a @c OverlayType.
 *
 * @param overlayTypeString String representing the OverlayType.
 * @return OverlayType matching the given string.
 */
static OverlayType overlayTypeFromString(const std::string& overlayTypeString) {
    if ("PICTURE_IN_PICTURE" == overlayTypeString) {
        return OverlayType::PICTURE_IN_PICTURE;
    } else if ("NONE" == overlayTypeString) {
        return OverlayType::NONE;
    } else {
        return OverlayType::UNKNOWN;
    }
}

/**
 * Converts the given string to a @c TalkMode.
 *
 * @param talkModeString String representing the TalkMode.
 * @return TalkMode matching the given string.
 */
static TalkMode talkModeFromString(const std::string& talkModeString) {
    if ("PRESS_AND_HOLD" == talkModeString) {
        return TalkMode::PRESS_AND_HOLD;
    } else if ("TAP" == talkModeString) {
        return TalkMode::TAP;
    } else if ("NO_SUPPORT" == talkModeString) {
        return TalkMode::NO_SUPPORT;
    } else {
        return TalkMode::UNKNOWN;
    }
}

/**
 * Converts the given string to a @c AudioState.
 *
 * @param audioStateString String representing the AudioState.
 * @return AudioState matching the given string.
 */
static AudioState audioStateFromString(const std::string& audioStateString) {
    if ("MUTED" == audioStateString) {
        return AudioState::MUTED;
    } else if ("UNMUTED" == audioStateString) {
        return AudioState::UNMUTED;
    } else if ("DISABLED" == audioStateString) {
        return AudioState::DISABLED;
    } else {
        return AudioState::UNKNOWN;
    }
}

/**
 * Converts the given string to a @c LiveViewTrigger.
 *
 * @param liveViewTriggerString String representing the LiveViewTrigger.
 * @return LiveViewTrigger matching the given string.
 */
static LiveViewTrigger liveViewTriggerFromString(const std::string& liveViewTriggerString) {
    if ("AUTOMATED_EVENT" == liveViewTriggerString) {
        return LiveViewTrigger::AUTOMATED_EVENT;
    } else if ("USER_ACTION" == liveViewTriggerString) {
        return LiveViewTrigger::USER_ACTION;
    } else {
        return LiveViewTrigger::UNKNOWN;
    }
}

/**
 * Converts the given string to a @c MotionCapability.
 *
 * @param motionCapabilityString String representing the MotionCapability.
 * @return MotionCapability matching the given string.
 */
static MotionCapability motionCapabilityFromString(const std::string& motionCapabilityString) {
    if ("PHYSICAL_PAN" == motionCapabilityString) {
        return MotionCapability::PHYSICAL_PAN;
    } else if ("PHYSICAL_TILT" == motionCapabilityString) {
        return MotionCapability::PHYSICAL_TILT;
    } else if ("PHYSICAL_ZOOM" == motionCapabilityString) {
        return MotionCapability::PHYSICAL_ZOOM;
    } else {
        return MotionCapability::UNKNOWN;
    }
}

/**
 * Helper function to convert the display modes to strings and add it to the Configuration json.
 *
 * @param @c displayModes The set of display modes.
 * @param @c jsonGenerator JsonGenerator object used to build the json string
 */
static void addDisplayModesArrayToConfiguration(
    const std::set<DisplayMode>& displayModes,
    avsCommon::utils::json::JsonGenerator* jsonGenerator) {
    std::set<std::string> displayModesArray;
    for (const auto& displayMode : displayModes) {
        displayModesArray.insert(displayModeToString(displayMode));
    }
    jsonGenerator->addStringArray(CONFIGURATION_DISPLAY_MODES_KEY, displayModesArray);
}

/**
 * Helper function to convert the overlay types to strings and add it to the Configuration json.
 *
 * @param @c overlayTypes The set of overlay types.
 * @param @c jsonGenerator JsonGenerator object used to build the json string
 */
static void addOverlayTypesArrayToConfiguration(
    const std::set<OverlayType>& overlayTypes,
    avsCommon::utils::json::JsonGenerator* jsonGenerator) {
    std::set<std::string> overlayTypesArray;
    for (const auto& overlayType : overlayTypes) {
        overlayTypesArray.insert(overlayTypeToString(overlayType));
    }
    jsonGenerator->addStringArray(CONFIGURATION_OVERLAY_TYPES_KEY, overlayTypesArray);
}

/**
 * Helper function to convert the overlay positions to strings and add it to the Configuration json.
 *
 * @param @c overlayPositions The set of overlay positions.
 * @param @c jsonGenerator JsonGenerator object used to build the json string
 */
static void addOverlayPositionsArrayToConfiguration(
    const std::set<OverlayPosition>& overlayPositions,
    avsCommon::utils::json::JsonGenerator* jsonGenerator) {
    std::set<std::string> overlayPositionsArray;
    for (const auto& overlayPosition : overlayPositions) {
        overlayPositionsArray.insert(overlayPositionToString(overlayPosition));
    }
    jsonGenerator->addStringArray(CONFIGURATION_OVERLAY_POSITIONS_KEY, overlayPositionsArray);
}

std::shared_ptr<AlexaLiveViewControllerCapabilityAgent> AlexaLiveViewControllerCapabilityAgent::create(
    EndpointIdentifier endpointId,
    std::shared_ptr<LiveViewControllerInterface> liveViewController,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<AlexaInterfaceMessageSenderInterface> responseSender,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender) {
    if (endpointId.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "emptyEndpointId"));
        return nullptr;
    }
    if (!liveViewController) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullLiveViewController"));
        return nullptr;
    }
    if (!messageSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMessageSender"));
        return nullptr;
    }
    if (!contextManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullContextManager"));
        return nullptr;
    }
    if (!responseSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullResponseSender"));
        return nullptr;
    }
    if (!exceptionSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionSender"));
        return nullptr;
    }

    auto liveViewControllerCapabilityAgent =
        std::shared_ptr<AlexaLiveViewControllerCapabilityAgent>(new AlexaLiveViewControllerCapabilityAgent(
            endpointId,
            std::move(liveViewController),
            std::move(messageSender),
            std::move(contextManager),
            std::move(responseSender),
            std::move(exceptionSender)));

    if (!liveViewControllerCapabilityAgent) {
        ACSDK_ERROR(LX("createFailed").d("reason", "instantiationFailed"));
        return nullptr;
    }

    return liveViewControllerCapabilityAgent;
}

AlexaLiveViewControllerCapabilityAgent::AlexaLiveViewControllerCapabilityAgent(
    EndpointIdentifier endpointId,
    std::shared_ptr<LiveViewControllerInterface> liveViewController,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<AlexaInterfaceMessageSenderInterface> responseSender,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender) :
        CapabilityAgent{NAMESPACE, exceptionSender},
        RequiresShutdown{"AlexaLiveViewControllerCapabilityAgent"},
        m_endpointId{std::move(endpointId)},
        m_liveViewController{std::move(liveViewController)},
        m_messageSender{std::move(messageSender)},
        m_contextManager{std::move(contextManager)},
        m_responseSender{std::move(responseSender)},
        m_concurrentTwoWayTalk{ConcurrentTwoWayTalkState::DISABLED} {
    m_executor = std::make_shared<avsCommon::utils::threading::Executor>();
    m_appInfo = rtc::native::AppInfo{APP_IDENTIFIER};
}

void AlexaLiveViewControllerCapabilityAgent::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    ACSDK_DEBUG9(LX(__func__));
    if (!directive) {
        ACSDK_ERROR(LX("handleDirectiveImmediatelyFailed").d("reason", "nullDirectiveInfo"));
        return;
    }
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void AlexaLiveViewControllerCapabilityAgent::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX(__func__));
    // do nothing.
}

void AlexaLiveViewControllerCapabilityAgent::handleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX(__func__));
    if (!info || !info->directive) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }

    ACSDK_DEBUG5(LX("handleDirective")
                     .d("name", info->directive->getName())
                     .d("messageId", info->directive->getMessageId())
                     .d("correlationToken", info->directive->getCorrelationToken()));

    if (info->directive->getName() == NAME_STARTLIVEVIEW) {
        handleStartLiveView(info);
    } else if (info->directive->getName() == NAME_STOPLIVEVIEW) {
        handleStopLiveView(info);
    } else {
        handleUnknownDirective(info, ExceptionErrorType::UNSUPPORTED_OPERATION);
    }
}

void AlexaLiveViewControllerCapabilityAgent::handleStartLiveView(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX(__func__));

    m_executor->submit([this, info]() {
        ACSDK_DEBUG9(LX("handleStartLiveViewInExecutor").sensitive("payload", info->directive->getPayload()));

        // Process the StartLiveView directive and send LiveViewStarted event
        auto startLiveViewRequest = parseStartLiveViewDirectivePayload(info);
        if (!startLiveViewRequest) {
            ACSDK_ERROR(LX("handleStartLiveViewInExecutor").d("reason", "unableToParseDirectivePayload"));
            handleUnknownDirective(info, ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }

        // Initiate the RtcscAppClient
        executeInstantiateRtcscAppClient();

        // We only allow one active session at time with the Rtcsc Client, so disconnect the current session if active.
        if (!m_lastSessionId.empty() && m_lastSessionId != m_currentSessionId) {
            ACSDK_DEBUG5(LX("handleStartLiveViewInExecutor").d("interrupt session", "session id changed"));
            executeDisconnectRtcscSession(
                m_lastSessionId, rtc::native::RtcscAppDisconnectCode::HIGHER_PRIORITY_SESSION_INTERRUPTED);
        }
        m_lastSessionId = m_currentSessionId;

        LiveViewControllerInterface::Response result;
        result = m_liveViewController->start(std::move(startLiveViewRequest));
        executeStartLiveViewDirective(info);
        executeSetHandlingCompleted(info);
        executeSendResponseEvent(info, result);
    });
}

std::unique_ptr<LiveViewControllerInterface::StartLiveViewRequest> AlexaLiveViewControllerCapabilityAgent::
    parseStartLiveViewDirectivePayload(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX(__func__));

    // Initialize the StartLiveViewRequest
    auto startLiveViewRequest = std::unique_ptr<LiveViewControllerInterface::StartLiveViewRequest>(
        new LiveViewControllerInterface::StartLiveViewRequest());

    // Parse the StartLiveView directive
    rapidjson::Document payload;
    rapidjson::ParseResult parseResult = payload.Parse(info->directive->getPayload());
    if (!parseResult) {
        ACSDK_ERROR(LX("handleStartLiveViewInExecutor")
                        .d("reason", rapidjson::GetParseError_En(parseResult.Code()))
                        .d("offset", parseResult.Offset())
                        .d("messageId", info->directive->getMessageId()));
        sendExceptionEncounteredAndReportFailed(
            info, "Unable to parse payload", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return nullptr;
    }

    // SessionId
    std::string sessionId;
    if (!jsonUtils::retrieveValue(payload, SESSION_ID_FIELD, &sessionId)) {
        ACSDK_ERROR(LX("parseStartLiveViewDirectivePayload").d("reason", "NoSessionId"));
        sendExceptionEncounteredAndReportFailed(info, "missing sessionId");
        return nullptr;
    }
    m_currentSessionId = sessionId;
    startLiveViewRequest->sessionId = sessionId;

    // Target object
    rapidjson::Value::ConstMemberIterator target;
    std::string targetParsedPayload;
    if (jsonUtils::findNode(payload, TARGET_FIELD, &target) &&
        jsonUtils::convertToValue(target->value, &targetParsedPayload)) {
        rapidjson::Document doc;
        doc.Parse(targetParsedPayload);

        if (doc.HasMember(ENDPOINT_ID_FIELD) && doc[ENDPOINT_ID_FIELD].IsString()) {
            m_targetEndpointId = doc[ENDPOINT_ID_FIELD].GetString();
        } else {
            ACSDK_ERROR(LX("parseStartLiveViewDirectivePayload").d("reason", "NoEndpointId"));
            sendExceptionEncounteredAndReportFailed(info, "missing EndpointId");
            return nullptr;
        }

        if (doc.HasMember(TYPE_FIELD) && doc[TYPE_FIELD].IsString()) {
            m_targetType = doc[TYPE_FIELD].GetString();
        } else {
            ACSDK_ERROR(LX("parseStartLiveViewDirectivePayload").m("NoType"));
            sendExceptionEncounteredAndReportFailed(info, "missing Type");
            return nullptr;
        }
    } else {
        ACSDK_ERROR(LX("parseStartLiveViewDirectivePayload").d("reason", "NoTarget"));
        sendExceptionEncounteredAndReportFailed(info, "missing Target");
        return nullptr;
    }
    Target targetObject;
    targetObject.endpointId = m_targetEndpointId;
    targetObject.type = targetTypeFromString(m_targetType);
    startLiveViewRequest->target = targetObject;

    // Role value
    std::string role;
    if (!jsonUtils::retrieveValue(payload, ROLE_FIELD, &role)) {
        ACSDK_ERROR(LX("parseStartLiveViewDirectivePayload").d("reason", "NoRole"));
        sendExceptionEncounteredAndReportFailed(info, "missing Role");
        return nullptr;
    } else if (roleFromString(role) != Role::VIEWER) {
        ACSDK_ERROR(LX("parseStartLiveViewDirectivePayload").d("reason", "only supporting viewer role"));
        sendExceptionEncounteredAndReportFailed(info, "only supporting viewer role");
        return nullptr;
    }
    startLiveViewRequest->role = roleFromString(role);

    // Participants object
    rapidjson::Value::ConstMemberIterator participantsIterator;
    std::string participantsPayload;
    std::set<Viewer> viewers;
    std::set<MotionCapability> motionCapabilities;
    Camera camera;
    Viewer viewer;
    if (jsonUtils::findNode(payload, PARTICIPANTS_FIELD, &participantsIterator) &&
        jsonUtils::convertToValue(participantsIterator->value, &participantsPayload)) {
        rapidjson::Document doc;
        doc.Parse(participantsPayload);

        // Parse viewers array
        if (doc.HasMember(VIEWERS_FIELD) && doc[VIEWERS_FIELD].IsArray()) {
            for (auto& viewerObject : doc[VIEWERS_FIELD].GetArray()) {
                if (!viewerObject.IsObject()) {
                    continue;
                }

                // Parse name
                if (viewerObject.HasMember(VIEWER_NAME_FIELD) && viewerObject[VIEWER_NAME_FIELD].IsString()) {
                    viewer.name = viewerObject[VIEWER_NAME_FIELD].GetString();
                } else {
                    ACSDK_ERROR(LX("parseStartLiveViewDirectivePayload").d("reason", "NoName"));
                    sendExceptionEncounteredAndReportFailed(info, "missing Name");
                    return nullptr;
                }

                // Parse hasCameraControl
                if (viewerObject.HasMember(VIEWER_HAS_CAMERA_CONTROL_FIELD) &&
                    viewerObject[VIEWER_HAS_CAMERA_CONTROL_FIELD].IsBool()) {
                    viewer.hasCameraControl = viewerObject[VIEWER_HAS_CAMERA_CONTROL_FIELD].GetBool();
                } else {
                    ACSDK_ERROR(LX("parseStartLiveViewDirectivePayload").d("reason", "NoHasCameraControl"));
                    sendExceptionEncounteredAndReportFailed(info, "missing HasCameraControl");
                    return nullptr;
                }

                // Parse state
                if (viewerObject.HasMember(VIEWER_STATE_FIELD) && viewerObject[VIEWER_STATE_FIELD].IsString()) {
                    viewer.state = viewerStateFromString(viewerObject[VIEWER_STATE_FIELD].GetString());
                } else {
                    ACSDK_ERROR(LX("parseStartLiveViewDirectivePayload").d("reason", "NoState"));
                    sendExceptionEncounteredAndReportFailed(info, "missing State");
                    return nullptr;
                }

                viewers.emplace(viewer);
            }
        } else {
            ACSDK_ERROR(LX("parseStartLiveViewDirectivePayload").d("reason", "NoViewers"));
            sendExceptionEncounteredAndReportFailed(info, "missing Viewers");
            return nullptr;
        }

        // Parse camera object
        if (doc.HasMember(CAMERA_FIELD) && doc[CAMERA_FIELD].IsObject()) {
            auto cameraObject = doc[CAMERA_FIELD].GetObject();

            // Parse name
            if (cameraObject.HasMember(CAMERA_NAME_FIELD) && cameraObject[CAMERA_NAME_FIELD].IsString()) {
                camera.name = cameraObject[CAMERA_NAME_FIELD].GetString();
            } else {
                ACSDK_ERROR(LX("parseStartLiveViewDirectivePayload").d("reason", "NoName"));
                sendExceptionEncounteredAndReportFailed(info, "missing Name");
                return nullptr;
            }

            // Parse make
            if (cameraObject.HasMember(CAMERA_MAKE_FIELD) && cameraObject[CAMERA_MAKE_FIELD].IsString()) {
                camera.make = cameraObject[CAMERA_MAKE_FIELD].GetString();
            } else {
                ACSDK_ERROR(LX("parseStartLiveViewDirectivePayload").d("reason", "NoMake"));
                sendExceptionEncounteredAndReportFailed(info, "missing Make");
                return nullptr;
            }

            // Parse model
            if (cameraObject.HasMember(CAMERA_MODEL_FIELD) && cameraObject[CAMERA_MODEL_FIELD].IsString()) {
                camera.model.set(cameraObject[CAMERA_MODEL_FIELD].GetString());
            } else {
                ACSDK_WARN(LX("parseStartLiveViewDirectivePayload").d("reason", "NoModel"));
            }

            // Parse capabilities
            if (cameraObject.HasMember(CAMERA_CAPABILITIES_FIELD) &&
                cameraObject[CAMERA_CAPABILITIES_FIELD].IsArray()) {
                for (auto& motionCapability : cameraObject[CAMERA_CAPABILITIES_FIELD].GetArray()) {
                    if (!motionCapability.IsString()) {
                        continue;
                    }
                    motionCapabilities.emplace(motionCapabilityFromString(motionCapability.GetString()));
                }
                camera.capabilities = motionCapabilities;
            } else {
                ACSDK_WARN(LX("parseStartLiveViewDirectivePayload").d("reason", "NoCapabilities"));
            }
        } else {
            ACSDK_ERROR(LX("parseStartLiveViewDirectivePayload").d("reason", "NoCamera"));
            sendExceptionEncounteredAndReportFailed(info, "missing Camera");
            return nullptr;
        }
    } else {
        ACSDK_ERROR(LX("parseStartLiveViewDirectivePayload").d("reason", "NoParticipants"));
        sendExceptionEncounteredAndReportFailed(info, "missing Participants");
        return nullptr;
    }
    Participants participantsObject;
    participantsObject.viewers = viewers;
    participantsObject.camera = camera;
    startLiveViewRequest->participants = participantsObject;

    // ViewerExperience object
    rapidjson::Value::ConstMemberIterator viewerExperienceIterator;
    std::string viewerExperiencePayload;
    DisplayProperties suggestedDisplay;
    AudioProperties audioProperties;
    LiveViewTrigger liveViewTrigger;
    int64_t idleTimeoutInMilliseconds;
    m_concurrentTwoWayTalk = ConcurrentTwoWayTalkState::DISABLED;
    if (jsonUtils::findNode(payload, VIEWER_EXPERIENCE_FIELD, &viewerExperienceIterator) &&
        jsonUtils::convertToValue(viewerExperienceIterator->value, &viewerExperiencePayload)) {
        rapidjson::Document doc;
        doc.Parse(viewerExperiencePayload);

        // Parse suggested display
        if (doc.HasMember(SUGGESTED_DISPLAY_FIELD) && doc[SUGGESTED_DISPLAY_FIELD].IsObject()) {
            auto suggestedDisplayObject = doc[SUGGESTED_DISPLAY_FIELD].GetObject();

            // Parse display mode
            if (suggestedDisplayObject.HasMember(DISPLAY_MODE_FIELD) &&
                suggestedDisplayObject[DISPLAY_MODE_FIELD].IsString()) {
                suggestedDisplay.displayMode =
                    displayModeFromString(suggestedDisplayObject[DISPLAY_MODE_FIELD].GetString());
            } else {
                ACSDK_ERROR(LX("parseStartLiveViewDirectivePayload").d("reason", "NoDisplayMode"));
                sendExceptionEncounteredAndReportFailed(info, "missing DisplayMode");
                return nullptr;
            }
            // Parse overlay type
            if (suggestedDisplayObject.HasMember(OVERLAY_TYPE_FIELD) &&
                suggestedDisplayObject[OVERLAY_TYPE_FIELD].IsString()) {
                suggestedDisplay.overlayType.set(
                    overlayTypeFromString(suggestedDisplayObject[OVERLAY_TYPE_FIELD].GetString()));
            } else {
                ACSDK_WARN(LX("parseStartLiveViewDirectivePayload").d("reason", "NoOverlayType"));
            }
            // Parse overlay position
            if (suggestedDisplayObject.HasMember(OVERLAY_POSITION_FIELD) &&
                suggestedDisplayObject[OVERLAY_POSITION_FIELD].IsString()) {
                suggestedDisplay.overlayPosition.set(
                    overlayPositionFromString(suggestedDisplayObject[OVERLAY_POSITION_FIELD].GetString()));
            } else {
                ACSDK_WARN(LX("parseStartLiveViewDirectivePayload").d("reason", "NoOverlayPosition"));
            }
        } else {
            ACSDK_ERROR(LX("parseStartLiveViewDirectivePayload").d("reason", "NoAudioProperties"));
            sendExceptionEncounteredAndReportFailed(info, "missing AudioProperties");
            return nullptr;
        }

        // Parse audio properties
        if (doc.HasMember(AUDIO_PROPERTIES_FIELD) && doc[AUDIO_PROPERTIES_FIELD].IsObject()) {
            auto audioPropertiesObject = doc[AUDIO_PROPERTIES_FIELD].GetObject();

            // Parse talk mode
            if (audioPropertiesObject.HasMember(TALK_MODE_FIELD) && audioPropertiesObject[TALK_MODE_FIELD].IsString()) {
                audioProperties.talkMode = talkModeFromString(audioPropertiesObject[TALK_MODE_FIELD].GetString());
            } else {
                ACSDK_ERROR(LX("parseStartLiveViewDirectivePayload").d("reason", "NoTalkMode"));
                sendExceptionEncounteredAndReportFailed(info, "missing TalkMode");
                return nullptr;
            }

            // Parse concurrent two-way talk
            if (audioPropertiesObject.HasMember(CONCURRENT_TWO_WAY_TALK_FIELD) &&
                audioPropertiesObject[CONCURRENT_TWO_WAY_TALK_FIELD].IsString()) {
                m_concurrentTwoWayTalk =
                    concurrentTwoWayTalkFromString(audioPropertiesObject[CONCURRENT_TWO_WAY_TALK_FIELD].GetString());
                audioProperties.concurrentTwoWayTalk = m_concurrentTwoWayTalk;
            } else {
                ACSDK_ERROR(LX("parseStartLiveViewDirectivePayload").d("reason", "NoConcurrentTwoWayTalk"));
                sendExceptionEncounteredAndReportFailed(info, "missing ConcurrentTwoWayTalk");
                return nullptr;
            }

            // Parse microphone state
            if (audioPropertiesObject.HasMember(MICROPHONE_STATE_FIELD) &&
                audioPropertiesObject[MICROPHONE_STATE_FIELD].IsString()) {
                audioProperties.microphoneState =
                    audioStateFromString(audioPropertiesObject[MICROPHONE_STATE_FIELD].GetString());
            } else {
                ACSDK_ERROR(LX("parseStartLiveViewDirectivePayload").d("reason", "NoMicrophoneState"));
                sendExceptionEncounteredAndReportFailed(info, "missing MicrophoneState");
                return nullptr;
            }

            // Parse speaker state
            if (audioPropertiesObject.HasMember(SPEAKER_STATE_FIELD) &&
                audioPropertiesObject[SPEAKER_STATE_FIELD].IsString()) {
                audioProperties.speakerState =
                    audioStateFromString(audioPropertiesObject[SPEAKER_STATE_FIELD].GetString());
            } else {
                ACSDK_ERROR(LX("parseStartLiveViewDirectivePayload").d("reason", "NoSpeakerState"));
                sendExceptionEncounteredAndReportFailed(info, "missing SpeakerState");
                return nullptr;
            }
        } else {
            ACSDK_ERROR(LX("parseStartLiveViewDirectivePayload").d("reason", "NoAudioProperties"));
            sendExceptionEncounteredAndReportFailed(info, "missing AudioProperties");
            return nullptr;
        }

        // Parse LiveViewTrigger
        if (doc.HasMember(LIVE_VIEW_TRIGGER_FIELD) && doc[LIVE_VIEW_TRIGGER_FIELD].IsString()) {
            liveViewTrigger = liveViewTriggerFromString(doc[LIVE_VIEW_TRIGGER_FIELD].GetString());
        } else {
            ACSDK_ERROR(LX("parseStartLiveViewDirectivePayload").d("reason", "NoLiveViewTrigger"));
            sendExceptionEncounteredAndReportFailed(info, "missing LiveViewTrigger");
            return nullptr;
        }

        // Parse IdleTimeoutInMilliseconds
        if (!jsonUtils::retrieveValue(doc, IDLE_TIMEOUT_FIELD, &idleTimeoutInMilliseconds)) {
            ACSDK_ERROR(LX("parseStartLiveViewDirectivePayload").m("NoIdleTimeoutInMilliseconds"));
            sendExceptionEncounteredAndReportFailed(info, "missing IdleTimeoutInMilliseconds");
            return nullptr;
        }
    } else {
        ACSDK_ERROR(LX("parseStartLiveViewDirectivePayload").d("reason", "NoViewerExperience"));
        sendExceptionEncounteredAndReportFailed(info, "missing ViewerExperience");
        return nullptr;
    }
    ViewerExperience viewerExperienceObject;
    viewerExperienceObject.suggestedDisplay = suggestedDisplay;
    viewerExperienceObject.audioProperties = audioProperties;
    viewerExperienceObject.liveViewTrigger = liveViewTrigger;
    viewerExperienceObject.idleTimeoutInMilliseconds = std::chrono::milliseconds(idleTimeoutInMilliseconds);
    startLiveViewRequest->viewerExperience = viewerExperienceObject;

    // Set the StartLiveView request
    return startLiveViewRequest;
}

void AlexaLiveViewControllerCapabilityAgent::executeStartLiveViewDirective(const std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX(__func__));
    m_lastDisplayedDirective = info;

    // Register with RTCSC App Client
    if (!m_rtcscAppClient) {
        ACSDK_ERROR(LX("executeStartLiveViewDirective").d("reason", "Null rtcscAppClient"));
        sendExceptionEncounteredAndReportFailed(info, "Null rtcscAppClient", ExceptionErrorType::INTERNAL_ERROR);
        m_lastDisplayedDirective.reset();
        return;
    }

    auto errorCode = m_rtcscAppClient->registerAppClientListener(m_appInfo, this);
    if (errorCode != rtc::native::RtcscErrorCode::SUCCESS) {
        ACSDK_ERROR(LX(__func__).d("registerAppClientListener RtcscErrorCode", rtc::native::toString(errorCode)));
        const std::string exceptionMessage = "registerAppClientListener results in " + rtc::native::toString(errorCode);
        sendExceptionEncounteredAndReportFailed(info, exceptionMessage, ExceptionErrorType::INTERNAL_ERROR);
        m_lastDisplayedDirective.reset();
        return;
    }

    // Render live view
    executeRenderLiveView();
}

void AlexaLiveViewControllerCapabilityAgent::executeRenderLiveView() {
    ACSDK_DEBUG9(LX(__func__));

    if (hasActiveLiveView()) {
        executeRenderLiveViewCallbacks(false);
    }
}

void AlexaLiveViewControllerCapabilityAgent::executeRenderLiveViewCallbacks(bool isClearLiveView) {
    ACSDK_DEBUG9(LX(__func__));
    if (!isClearLiveView) {
        m_liveViewController->setCameraState(CameraState::CONNECTING);

        // Send the LiveViewStartedEvent
        rapidjson::Document targetPayload(rapidjson::kObjectType);
        targetPayload.AddMember(
            rapidjson::StringRef(ENDPOINT_ID_FIELD), m_targetEndpointId, targetPayload.GetAllocator());
        targetPayload.AddMember(rapidjson::StringRef(TYPE_FIELD), m_targetType, targetPayload.GetAllocator());

        rapidjson::Document liveViewStartedPayload(rapidjson::kObjectType);
        liveViewStartedPayload.AddMember(
            rapidjson::StringRef(SESSION_ID_FIELD), m_lastSessionId, liveViewStartedPayload.GetAllocator());
        liveViewStartedPayload.AddMember(
            rapidjson::StringRef(TARGET_FIELD), targetPayload, liveViewStartedPayload.GetAllocator());

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        if (liveViewStartedPayload.Accept(writer)) {
            executeSendLiveViewEvent(LIVE_VIEW_STARTED_EVENT_NAME, buffer.GetString());
        } else {
            ACSDK_ERROR(LX(__func__).d("reason", "writerRefusedJsonObject"));
        }

    } else {
        executeDisconnectRtcscSession(m_lastSessionId, rtc::native::RtcscAppDisconnectCode::USER_TERMINATED_SESSION);
        m_lastDisplayedDirective = nullptr;

        // Send the LiveViewStopped event
        rapidjson::Document targetPayload(rapidjson::kObjectType);
        targetPayload.AddMember(
            rapidjson::StringRef(ENDPOINT_ID_FIELD), m_targetEndpointId, targetPayload.GetAllocator());
        targetPayload.AddMember(rapidjson::StringRef(TYPE_FIELD), m_targetType, targetPayload.GetAllocator());

        rapidjson::Document liveViewStoppedPayload(rapidjson::kObjectType);
        liveViewStoppedPayload.AddMember(
            rapidjson::StringRef(SESSION_ID_FIELD), m_lastSessionId, liveViewStoppedPayload.GetAllocator());
        liveViewStoppedPayload.AddMember(
            rapidjson::StringRef(TARGET_FIELD), targetPayload, liveViewStoppedPayload.GetAllocator());

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        if (liveViewStoppedPayload.Accept(writer)) {
            executeSendLiveViewEvent(LIVE_VIEW_STOPPED_EVENT_NAME, buffer.GetString());
        } else {
            ACSDK_ERROR(LX(__func__).d("reason", "writerRefusedJsonObject"));
        }
        m_targetType = {};
        m_targetEndpointId = {};
    }
}

void AlexaLiveViewControllerCapabilityAgent::handleStopLiveView(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX(__func__));

    m_executor->submit([this, info]() {
        ACSDK_DEBUG9(LX("handleStopLiveViewInExecutor").sensitive("payload", info->directive->getPayload()));
        if (!validateStopLiveViewDirectivePayload(info)) {
            ACSDK_ERROR(LX("handleStopLiveViewInExecutor").d("reason", "invalid StopLiveView directive"));
            return;
        }

        LiveViewControllerInterface::Response result;
        result = m_liveViewController->stop();
        executeStopLiveViewDirective(info);
        executeSetHandlingCompleted(info);
        executeSendResponseEvent(info, result);
    });
}

bool AlexaLiveViewControllerCapabilityAgent::validateStopLiveViewDirectivePayload(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX(__func__));
    // Parse the StopLiveView directive
    rapidjson::Document payload;
    rapidjson::ParseResult parseResult = payload.Parse(info->directive->getPayload());
    if (!parseResult) {
        ACSDK_ERROR(LX("validateStopLiveViewDirectivePayload")
                        .d("reason", rapidjson::GetParseError_En(parseResult.Code()))
                        .d("offset", parseResult.Offset())
                        .d("messageId", info->directive->getMessageId()));
        sendExceptionEncounteredAndReportFailed(
            info, "Unable to parse payload", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return false;
    }

    /// Parse target object
    rapidjson::Value::ConstMemberIterator target;
    std::string targetParsedPayload;
    std::string targetEndpointId;
    std::string targetType;
    if (jsonUtils::findNode(payload, TARGET_FIELD, &target) &&
        jsonUtils::convertToValue(target->value, &targetParsedPayload)) {
        rapidjson::Document doc;
        doc.Parse(targetParsedPayload);

        if (doc.HasMember(ENDPOINT_ID_FIELD) && doc[ENDPOINT_ID_FIELD].IsString()) {
            targetEndpointId = doc[ENDPOINT_ID_FIELD].GetString();
        } else {
            ACSDK_WARN(LX("validateStopLiveViewDirectivePayload").d("reason", "NoTargetEndpointId"));
        }

        if (targetEndpointId != m_targetEndpointId) {
            ACSDK_WARN(LX("validateStopLiveViewDirectivePayload")
                           .d("reason", "mismatchedEndpointId")
                           .d("expectedEndpointId", m_targetEndpointId)
                           .d("receivedEndpointId", targetEndpointId));
        }

        if (doc.HasMember(TYPE_FIELD) && doc[TYPE_FIELD].IsString()) {
            targetType = doc[TYPE_FIELD].GetString();
        } else {
            ACSDK_WARN(LX("validateStopLiveViewDirectivePayload").m("NoTargetType"));
        }

        if (!targetType.empty() && targetType != m_targetType) {
            ACSDK_WARN(LX("validateStopLiveViewDirectivePayload")
                           .d("reason", "mismatchedType")
                           .d("expectedType", m_targetType)
                           .d("receivedType", targetType));
        }
    } else {
        ACSDK_WARN(LX("validateStopLiveViewDirectivePayload").d("reason", "NoTarget"));
    }

    return true;
}

void AlexaLiveViewControllerCapabilityAgent::executeStopLiveViewDirective(const std::shared_ptr<DirectiveInfo> info) {
    executeClearLiveView();
}

void AlexaLiveViewControllerCapabilityAgent::executeClearLiveView() {
    ACSDK_DEBUG9(LX(__func__));
    if (hasActiveLiveView()) {
        executeRenderLiveViewCallbacks(true);
    }
}

void AlexaLiveViewControllerCapabilityAgent::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX(__func__));
    if (!info || !info->directive) {
        ACSDK_ERROR(LX("cancelDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }
    removeDirective(info);
}

DirectiveHandlerConfiguration AlexaLiveViewControllerCapabilityAgent::getConfiguration() const {
    ACSDK_DEBUG9(LX(__func__));
    DirectiveHandlerConfiguration configuration;

    configuration[START_LIVE_VIEW] = BlockingPolicy(BlockingPolicy::MEDIUM_VISUAL, true);
    configuration[STOP_LIVE_VIEW] = BlockingPolicy(BlockingPolicy::MEDIUMS_AUDIO_AND_VISUAL, true);

    return configuration;
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> AlexaLiveViewControllerCapabilityAgent::
    getCapabilityConfigurations() {
    auto additionalConfigurations = CapabilityConfiguration::AdditionalConfigurations();
    auto config = m_liveViewController->getConfiguration();
    auto configurationJson = buildLiveViewControllerConfigurationJson(config);
    if (!configurationJson.empty()) {
        additionalConfigurations[CAPABILITY_CONFIGURATION_KEY] = configurationJson;
    }
    CapabilityConfiguration configuration{CapabilityConfiguration::ALEXA_INTERFACE_TYPE,
                                          NAMESPACE,
                                          INTERFACE_VERSION,
                                          Optional<std::string>(),  // instance
                                          Optional<CapabilityConfiguration::Properties>(),
                                          additionalConfigurations};
    return {std::make_shared<alexaClientSDK::avsCommon::avs::CapabilityConfiguration>(configuration)};
}

void AlexaLiveViewControllerCapabilityAgent::doShutdown() {
    ACSDK_DEBUG9(LX(__func__));
    if (!m_executor->isShutdown()) {
        m_executor->shutdown();
    }
    m_liveViewController.reset();
    m_messageSender.reset();
    m_contextManager.reset();
    m_responseSender.reset();
}

void AlexaLiveViewControllerCapabilityAgent::removeDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX(__func__));
    if (info->directive && info->result) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
    }
}

void AlexaLiveViewControllerCapabilityAgent::executeSetHandlingCompleted(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX(__func__));
    if (info && info->result) {
        info->result->setCompleted();
        removeDirective(info);
    }
}

void AlexaLiveViewControllerCapabilityAgent::handleUnknownDirective(
    std::shared_ptr<DirectiveInfo> info,
    avsCommon::avs::ExceptionErrorType type) {
    ACSDK_ERROR(LX("requestedToHandleUnknownDirective")
                    .d("reason", "unknownDirective")
                    .d("namespace", info->directive->getNamespace())
                    .d("name", info->directive->getName()));

    m_executor->submit([this, info, type] {
        const std::string exceptionMessage =
            "unexpected directive " + info->directive->getNamespace() + ":" + info->directive->getName();

        sendExceptionEncounteredAndReportFailed(info, exceptionMessage, type);
    });
}

void AlexaLiveViewControllerCapabilityAgent::executeSendResponseEvent(
    const std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
    const alexaLiveViewControllerInterfaces::LiveViewControllerInterface::Response& result) {
    switch (result.type) {
        case LiveViewControllerInterface::Response::Type::SUCCESS: {
            m_responseSender->sendResponseEvent(
                info->directive->getInstance(),
                info->directive->getCorrelationToken(),
                AVSMessageEndpoint(m_endpointId));
            break;
        }
        case LiveViewControllerInterface::Response::Type::FAILED_BATTERY_TOO_LOW:
            sendAlexaErrorResponse(
                info, AlexaInterfaceMessageSenderInterface::ErrorResponseType::ENDPOINT_LOW_POWER, result.errorMessage);
            break;
        case LiveViewControllerInterface::Response::Type::FAILED_UNAUTHORIZED:
            sendAlexaErrorResponse(
                info,
                AlexaInterfaceMessageSenderInterface::ErrorResponseType::INVALID_AUTHORIZATION_CREDENTIAL,
                result.errorMessage);
            break;
        case LiveViewControllerInterface::Response::Type::FAILED_MEDIA_SOURCE_NOT_FOUND:
        case LiveViewControllerInterface::Response::Type::FAILED_MEDIA_SOURCE_ASLEEP:
        case LiveViewControllerInterface::Response::Type::FAILED_MEDIA_SOURCE_TURNED_OFF:
        case LiveViewControllerInterface::Response::Type::FAILED_INTERNAL_ERROR:
            sendAlexaErrorResponse(
                info, AlexaInterfaceMessageSenderInterface::ErrorResponseType::INTERNAL_ERROR, result.errorMessage);
            break;
    }
}

void AlexaLiveViewControllerCapabilityAgent::sendAlexaErrorResponse(
    const std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
    const avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface::ErrorResponseType alexaErrorResponseType,
    const std::string& responseMessage) {
    m_responseSender->sendErrorResponseEvent(
        info->directive->getInstance(),
        info->directive->getCorrelationToken(),
        AVSMessageEndpoint(m_endpointId),
        alexaErrorResponseType,
        responseMessage);
}

std::string AlexaLiveViewControllerCapabilityAgent::buildLiveViewControllerConfigurationJson(
    const Configuration& configuration) {
    avsCommon::utils::json::JsonGenerator jsonGenerator;

    if (configuration.supportedDisplayModes.empty()) {
        ACSDK_ERROR(LX(__func__).d("reason", "EmptyDisplayModesArray"));
        return "";
    } else {
        addDisplayModesArrayToConfiguration(configuration.supportedDisplayModes, &jsonGenerator);
    }

    if (configuration.supportedOverlayTypes.empty()) {
        ACSDK_ERROR(LX(__func__).d("reason", "EmptyOverlayTypesArray"));
        return "";
    } else {
        addOverlayTypesArrayToConfiguration(configuration.supportedOverlayTypes, &jsonGenerator);
    }

    if (configuration.supportedOverlayPositions.empty()) {
        ACSDK_ERROR(LX(__func__).d("reason", "EmptyOverlayPositionsArray"));
        return "";
    } else {
        addOverlayPositionsArrayToConfiguration(configuration.supportedOverlayPositions, &jsonGenerator);
    }

    ACSDK_DEBUG9(LX(__func__).sensitive("configuration", jsonGenerator.toString()));
    return jsonGenerator.toString();
}

void AlexaLiveViewControllerCapabilityAgent::executeSendLiveViewEvent(
    const std::string& eventName,
    const std::string& payload) {
    m_executor->submit([this, eventName, payload]() {
        auto msgIdAndJsonEvent = avsCommon::avs::buildJsonEventString(NAMESPACE, eventName, "", payload);
        auto userEventMessage = std::make_shared<avsCommon::avs::MessageRequest>(msgIdAndJsonEvent.second);
        ACSDK_DEBUG9(LX("Sending event to AVS").d("namespace", NAMESPACE).d("name", eventName));
        m_messageSender->sendMessage(userEventMessage);
    });
}

void AlexaLiveViewControllerCapabilityAgent::setRtcscAppClient(
    const std::shared_ptr<rtc::native::RtcscAppClientInterface>& rtcscAppClient) {
    ACSDK_WARN(LX(__func__).d("reason", "should be called in test only"));
    m_rtcscAppClient = rtcscAppClient;
}

void AlexaLiveViewControllerCapabilityAgent::executeInstantiateRtcscAppClient() {
    if (!m_rtcscAppClient) {
        m_rtcscAppClient = rtc::native::RtcscAppClientInterface::getInstance();
    }
}

void AlexaLiveViewControllerCapabilityAgent::executeDisconnectRtcscSession(
    const std::string& sessionId,
    rtc::native::RtcscAppDisconnectCode disconnectCode) {
    ACSDK_DEBUG9(LX(__func__).d("sessionId", sessionId).d("disconnectCode", rtc::native::toString(disconnectCode)));
    if (m_rtcscAppClient) {
        auto result = m_rtcscAppClient->disconnectSession(sessionId, disconnectCode);
        if (result != rtc::native::RtcscErrorCode::SUCCESS) {
            ACSDK_WARN(LX(__func__).d("reason", toString(result)));
        }
    }
}

bool AlexaLiveViewControllerCapabilityAgent::hasActiveLiveView() {
    /**
     * Checks if there is an active StartLiveView directive by comparing the namespace and name of the last displayed
     * against Alexa.Camera.LiveViewController.StartLiveView.
     */
    return m_lastDisplayedDirective && m_lastDisplayedDirective->directive->getNamespace() == NAMESPACE &&
           m_lastDisplayedDirective->directive->getName() == NAME_STARTLIVEVIEW;
}

void AlexaLiveViewControllerCapabilityAgent::setMicrophoneState(bool enabled) {
    ACSDK_DEBUG9(LX(__func__).d("micEnabled", enabled));

    m_executor->submit([this, enabled]() {
        if (!m_rtcscAppClient) {
            ACSDK_ERROR(LX("setMicrophoneStateFailed").d("reason", "Null rtcscAppClient"));
            return;
        }

        auto result = m_rtcscAppClient->setLocalAudioState(m_lastSessionId, enabled);
        if (result != rtc::native::RtcscErrorCode::SUCCESS) {
            ACSDK_ERROR(LX("setMicrophoneStateInExecutor")
                            .d("reason", "setLocalAudioStateFailed")
                            .d("result", toString(result)));
        }
        if (m_concurrentTwoWayTalk != ConcurrentTwoWayTalkState::ENABLED) {
            result = m_rtcscAppClient->setRemoteAudioState(m_lastSessionId, !enabled);
            if (result != rtc::native::RtcscErrorCode::SUCCESS) {
                ACSDK_ERROR(LX("setMicrophoneStateInExecutor")
                                .d("reason", "setRemoteAudioState")
                                .d("result", toString(result)));
            }
        }
    });
}

void AlexaLiveViewControllerCapabilityAgent::onSessionAvailable(const std::string& sessionId) {
    ACSDK_DEBUG9(LX(__func__).d("sessionId", sessionId));

    m_executor->submit([this, sessionId]() {
        if (!hasActiveLiveView()) {
            ACSDK_WARN(LX(__func__).d("onSessionAvailableFailedInExecutor", "No active live view directive"));
            return;
        }
        if (sessionId != m_lastSessionId) {
            ACSDK_WARN(LX("onSessionAvailableFailedInExecutor")
                           .d("reason", "Mismatched sessionIds from LiveViewController and RTCSessionController")
                           .d("current SessionId", m_lastSessionId)
                           .d("received SessionId", sessionId));
            return;
        }

        if (!m_rtcscAppClient) {
            ACSDK_ERROR(LX("onSessionAvailableFailedInExecutor").d("reason", "Null rtcscAppClient"));
            return;
        }

        auto result = m_rtcscAppClient->signalReadyForSession(sessionId);

        if (result != rtc::native::RtcscErrorCode::SUCCESS) {
            ACSDK_WARN(LX("onSessionAvailableFailedInExecutor").d("reason", toString(result)));
        }
    });
}

void AlexaLiveViewControllerCapabilityAgent::onSessionRemoved(const std::string& sessionId) {
    ACSDK_DEBUG9(LX(__func__).d("sessionId", sessionId));

    m_executor->submit([this, sessionId]() {
        if (!hasActiveLiveView()) {
            // Unregister as an RTC client listener when a session has been removed, and we have no active live view
            // directive.
            ACSDK_DEBUG9(LX("onSessionRemovedInExecutor").d("unregistering app listener", sessionId));
            if (m_rtcscAppClient) {
                auto result = m_rtcscAppClient->unregisterAppClientListener(m_appInfo);
                ACSDK_DEBUG9(LX("onSessionRemovedInExecutor").d("rtcscUnregisterCode", rtc::native::toString(result)));
                if (result != rtc::native::RtcscErrorCode::SUCCESS) {
                    ACSDK_WARN(LX("onSessionRemovedInExecutor").d("reason", toString(result)));
                }
                m_rtcscAppClient.reset();
                rtc::native::RtcscAppClientInterface::releaseInstance();
            }
        }
        if (m_lastSessionId == sessionId) {
            ACSDK_DEBUG9(LX("onSessionRemovedInExecutor").d("reset lastSessionId", sessionId));
            m_lastSessionId = {};
            m_currentSessionId = {};
        }
    });
}

void AlexaLiveViewControllerCapabilityAgent::onError(
    rtc::native::RtcscErrorCode errorCode,
    const std::string& errorMessage,
    const rtc::native::Optional<std::string>& sessionId) {
    ACSDK_DEBUG9(LX(__func__).d("errorCode", rtc::native::toString(errorCode)).d("errorMessage", errorMessage));
    m_executor->submit([this, sessionId] {
        if (!hasActiveLiveView()) {
            ACSDK_WARN(LX("onErrorInExecutor").d("reason", "No active live view directive"));
            return;
        }
        if (sessionId.hasValue() && sessionId.value() != m_lastSessionId) {
            ACSDK_WARN(LX("onErrorInExecutor")
                           .d("reason", "Mismatched sessionIds from LiveViewController and RTCSessionController")
                           .d("current SessionId", m_lastSessionId)
                           .d("received SessionId", sessionId.value()));
            return;
        }
        m_liveViewController->setCameraState(CameraState::ERROR);
    });
}

void AlexaLiveViewControllerCapabilityAgent::onSessionStateChanged(
    const std::string& sessionId,
    rtc::native::SessionState sessionState) {
    ACSDK_DEBUG9(LX(__func__).d("sessionState", rtc::native::toString(sessionState)).d("sessionId", sessionId));
    // No-op
}

void AlexaLiveViewControllerCapabilityAgent::onMediaStatusChanged(
    const std::string& sessionId,
    rtc::native::MediaSide mediaSide,
    rtc::native::MediaType mediaType,
    bool enabled) {
    ACSDK_DEBUG9(LX(__func__)
                     .d("mediaSide", rtc::native::toString(mediaSide))
                     .d("mediaType", rtc::native::toString(mediaType))
                     .d("sessionId", sessionId));
    // No-op
}

void AlexaLiveViewControllerCapabilityAgent::onVideoEffectChanged(
    const std::string& sessionId,
    rtc::native::VideoEffect currentVideoEffect,
    int videoEffectDurationMs) {
    ACSDK_DEBUG9(LX(__func__)
                     .d("sessionId", sessionId)
                     .d("currentVideoEffect", rtc::native::toString(currentVideoEffect))
                     .d("videoEffectDurationMs", videoEffectDurationMs));
    // No-op
}

void AlexaLiveViewControllerCapabilityAgent::onMediaConnectionStateChanged(
    const std::string& sessionId,
    rtc::native::MediaConnectionState state) {
    m_executor->submit([this, sessionId, state] {
        ACSDK_DEBUG9(LX("onMediaConnectionStateChangedInExecutor").d("state", rtc::native::toString(state)));
        if (!hasActiveLiveView()) {
            ACSDK_WARN(LX("onMediaConnectionStateChangedInExecutor").d("reason", "No active live view directive"));
            return;
        }
        if (sessionId != m_lastSessionId) {
            ACSDK_WARN(LX("onMediaConnectionStateChangedInExecutor")
                           .d("reason", "Mismatched sessionIds from LiveViewController and RTCSessionController")
                           .d("current SessionId", m_lastSessionId)
                           .d("received SessionId", sessionId));
            return;
        }
        switch (state) {
            case rtc::native::MediaConnectionState::CONNECTING:
                m_liveViewController->setCameraState(CameraState::CONNECTING);
                break;
            case rtc::native::MediaConnectionState::CONNECTED:
                m_liveViewController->setCameraState(CameraState::CONNECTED);
                // TODO: RTCSC Client to provide API for setting mic state on init.
                // For now, always init with mic MUTED, and wait for call from LVC UI to unmute
                setMicrophoneState(false);
                break;
            case rtc::native::MediaConnectionState::DISCONNECTED:
                m_liveViewController->setCameraState(CameraState::DISCONNECTED);
                executeClearLiveView();
                break;
            case rtc::native::MediaConnectionState::UNKNOWN:
                m_liveViewController->setCameraState(CameraState::UNKNOWN);
                break;
        }
    });
}

void AlexaLiveViewControllerCapabilityAgent::onFirstFrameReceived(
    const std::string& sessionId,
    rtc::native::MediaType mediaType) {
    ACSDK_DEBUG9(LX(__func__).d("sessionId", sessionId).d("mediaType", rtc::native::toString(mediaType)));
    // No-op
}

void AlexaLiveViewControllerCapabilityAgent::onFirstFrameRendered(
    const std::string& sessionId,
    rtc::native::MediaSide mediaSide) {
    ACSDK_DEBUG9(LX(__func__).d("sessionId", sessionId).d("mediaSide", rtc::native::toString(mediaSide)));
    m_executor->submit([this, sessionId] {
        if (!hasActiveLiveView()) {
            ACSDK_WARN(LX("onFirstFrameRenderedInExecutor").d("reason", "No active live view directive"));
            return;
        }
        if (sessionId != m_lastSessionId) {
            ACSDK_WARN(LX("onFirstFrameRenderedInExecutor")
                           .d("reason", "Mismatched sessionIds from LiveViewController and RTCSessionController")
                           .d("current SessionId", m_lastSessionId)
                           .d("received SessionId", sessionId));
            return;
        }
    });
}

void AlexaLiveViewControllerCapabilityAgent::onMicrophoneStateChanged(
    alexaLiveViewControllerInterfaces::AudioState microphoneState) {
    setMicrophoneState(audioStateToMicrophoneState(microphoneState));
}

void AlexaLiveViewControllerCapabilityAgent::onLiveViewCleared() {
    m_executor->submit([this]() { executeClearLiveView(); });
}

}  // namespace alexaLiveViewController
}  // namespace alexaClientSDK
