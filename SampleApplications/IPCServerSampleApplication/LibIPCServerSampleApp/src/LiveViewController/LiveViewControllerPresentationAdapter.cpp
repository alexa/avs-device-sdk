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

#include <memory>
#include <string>

#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include <IPCServerSampleApp/LiveViewController/LiveViewControllerPresentationAdapter.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace liveViewController {

using namespace avsCommon::utils::json;
using namespace alexaLiveViewControllerInterfaces;
using namespace presentationOrchestratorInterfaces;

/// String to identify log entries originating from this file.
#define TAG "LiveViewControllerPresentationAdapter"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Interface name for Alexa.Camera.LiveViewController requests.
static const char LIVE_VIEW_CAMERA_INTERFACE_NAME[] = "Alexa.Camera.LiveViewController";

/// Identifier for the SessionId.
static const char* const SESSION_ID_FIELD = "sessionId";

/// Identifier for the Role.
static const char* const ROLE_FIELD = "role";

/// Identifier for the Target.
static const char* const TARGET_FIELD = "target";

/// Identifier for a target's endpointId.
static const char* const ENDPOINT_ID_FIELD = "endpointId";

/// Identifier for a target's type.
static const char* const TYPE_FIELD = "type";

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
 * Adds a @c Target object to a json object.
 *
 * @param targetObject The target object to add the json object.
 * @param jsonGenerator The json generator being used to generate the json object.
 */
static void addTargetObjectToJson(Target targetObject, JsonGenerator& jsonGenerator) {
    jsonGenerator.startObject(TARGET_FIELD);
    jsonGenerator.addMember(ENDPOINT_ID_FIELD, targetObject.endpointId);
    jsonGenerator.addMember(TYPE_FIELD, targetTypeToString(targetObject.type.value()));
    jsonGenerator.finishObject();
}

/**
 * Adds a @c Camera object to a json object.
 *
 * @param cameraObject The camera object to add the json object.
 * @param jsonGenerator The json generator being used to generate the json object.
 */
static void addParticipantsCameraObjectToJson(Camera cameraObject, JsonGenerator& jsonGenerator) {
    jsonGenerator.startObject(CAMERA_FIELD);
    jsonGenerator.addMember(CAMERA_NAME_FIELD, cameraObject.name);
    jsonGenerator.addMember(CAMERA_MAKE_FIELD, cameraObject.make);
    jsonGenerator.addMember(CAMERA_MODEL_FIELD, cameraObject.model.value());
    std::set<std::string> motionCapabilitiesArray;
    for (const auto& motionCapability : cameraObject.capabilities.value()) {
        motionCapabilitiesArray.insert(motionCapabilityToString(motionCapability));
    }
    jsonGenerator.addStringArray(CAMERA_CAPABILITIES_FIELD, motionCapabilitiesArray);
    jsonGenerator.finishObject();
}

/**
 * Adds a @c Participants object to a json object.
 *
 * @param participantsObject The participants object to add to the json object.
 * @param jsonGenerator The json generator being used to generate the json object.
 */
static void addParticipantsObjectToJson(Participants participantsObject, JsonGenerator& jsonGenerator) {
    jsonGenerator.startObject(PARTICIPANTS_FIELD);
    jsonGenerator.startArray(VIEWERS_FIELD);
    for (const auto& viewer : participantsObject.viewers) {
        jsonGenerator.startArrayElement();
        jsonGenerator.addMember(VIEWER_NAME_FIELD, viewer.name);
        jsonGenerator.addMember(VIEWER_HAS_CAMERA_CONTROL_FIELD, viewer.hasCameraControl);
        jsonGenerator.addMember(VIEWER_STATE_FIELD, viewerStateToString(viewer.state));
        jsonGenerator.finishArrayElement();
    }
    jsonGenerator.finishArray();
    addParticipantsCameraObjectToJson(participantsObject.camera, jsonGenerator);
    jsonGenerator.finishObject();
}

/**
 * Adds a @c DisplayProperties object to a json object.
 *
 * @param displayPropertiesObject The display properties object to add to the json object.
 * @param jsonGenerator The json generator being used to generate the json object.
 */
static void addDisplayPropertiesObjectToJson(
    const DisplayProperties& displayPropertiesObject,
    JsonGenerator& jsonGenerator) {
    jsonGenerator.startObject(SUGGESTED_DISPLAY_FIELD);
    jsonGenerator.addMember(DISPLAY_MODE_FIELD, displayModeToString(displayPropertiesObject.displayMode));
    jsonGenerator.addMember(OVERLAY_TYPE_FIELD, overlayTypeToString(displayPropertiesObject.overlayType.value()));
    jsonGenerator.addMember(
        OVERLAY_POSITION_FIELD, overlayPositionToString(displayPropertiesObject.overlayPosition.value()));
    jsonGenerator.finishObject();
}

/**
 * Adds an @c AudioProperties object to a json object.
 *
 * @param audioPropertiesObject The audio properties object to add to the json object.
 * @param jsonGenerator The json generator being used to generate the json object.
 */
static void addAudioPropertiesObjectToJson(const AudioProperties& audioPropertiesObject, JsonGenerator& jsonGenerator) {
    jsonGenerator.startObject(AUDIO_PROPERTIES_FIELD);
    jsonGenerator.addMember(TALK_MODE_FIELD, talkModeToString(audioPropertiesObject.talkMode));
    jsonGenerator.addMember(
        CONCURRENT_TWO_WAY_TALK_FIELD, concurrentTwoWayTalkStateToString(audioPropertiesObject.concurrentTwoWayTalk));
    jsonGenerator.addMember(MICROPHONE_STATE_FIELD, audioStateToString(audioPropertiesObject.microphoneState));
    jsonGenerator.addMember(SPEAKER_STATE_FIELD, audioStateToString(audioPropertiesObject.speakerState));
    jsonGenerator.finishObject();
}

/**
 * Adds an @c ViewerExperience object to a json object.
 *
 * @param viewerExperienceObject The viewer experience object to add to the json object.
 * @param jsonGenerator The json generator being used to generate the json object.
 */
static void addViewerExperienceObjectToJson(
    const ViewerExperience& viewerExperienceObject,
    JsonGenerator& jsonGenerator) {
    jsonGenerator.startObject(VIEWER_EXPERIENCE_FIELD);
    addDisplayPropertiesObjectToJson(viewerExperienceObject.suggestedDisplay, jsonGenerator);
    addAudioPropertiesObjectToJson(viewerExperienceObject.audioProperties, jsonGenerator);
    jsonGenerator.addMember(LIVE_VIEW_TRIGGER_FIELD, liveViewTriggerToString(viewerExperienceObject.liveViewTrigger));
    jsonGenerator.addMember(IDLE_TIMEOUT_FIELD, viewerExperienceObject.idleTimeoutInMilliseconds.count());
    jsonGenerator.finishObject();
}

static std::string serializeStartLiveViewPayload(
    std::unique_ptr<LiveViewControllerInterface::StartLiveViewRequest> startLiveViewRequest) {
    JsonGenerator jsonGenerator;
    jsonGenerator.addMember(SESSION_ID_FIELD, startLiveViewRequest->sessionId);
    addTargetObjectToJson(startLiveViewRequest->target, jsonGenerator);
    jsonGenerator.addMember(ROLE_FIELD, roleToString(startLiveViewRequest->role));
    addParticipantsObjectToJson(startLiveViewRequest->participants, jsonGenerator);
    addViewerExperienceObjectToJson(startLiveViewRequest->viewerExperience, jsonGenerator);
    return jsonGenerator.toString();
}

static std::string cameraStateToString(CameraState cameraState) {
    switch (cameraState) {
        case CameraState::CONNECTED:
            return "CONNECTED";
        case CameraState::CONNECTING:
            return "CONNECTING";
        case CameraState::DISCONNECTED:
            return "DISCONNECTED";
        case CameraState::ERROR:
            return "ERROR";
        case CameraState::UNKNOWN:
            return "";
    }
    return "";
}

std::shared_ptr<LiveViewControllerPresentationAdapter> LiveViewControllerPresentationAdapter::create(
    const std::shared_ptr<ipc::IPCHandlerRegistrationInterface>& ipcHandlerRegistrar,
    const std::shared_ptr<AplClientBridge>& aplClientBridge) {
    if (!ipcHandlerRegistrar) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullIPCHandlerRegistrar"));
        return nullptr;
    }

    if (!aplClientBridge) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullAplClientBridge"));
        return nullptr;
    }

    auto liveViewControllerPresentationAdapter =
        std::shared_ptr<LiveViewControllerPresentationAdapter>(new LiveViewControllerPresentationAdapter());

    liveViewControllerPresentationAdapter->initialize(ipcHandlerRegistrar, aplClientBridge);

    return liveViewControllerPresentationAdapter;
}

LiveViewControllerPresentationAdapter::LiveViewControllerPresentationAdapter() {
}

void LiveViewControllerPresentationAdapter::initialize(
    const std::shared_ptr<ipc::IPCHandlerRegistrationInterface>& ipcHandlerRegistrar,
    const std::shared_ptr<AplClientBridge>& aplClientBridge) {
    m_liveViewCameraIPCHandler = ipc::LiveViewCameraHandler::create(ipcHandlerRegistrar, shared_from_this());
    m_aplLiveViewExtension = std::make_shared<extensions::liveView::AplLiveViewExtension>(shared_from_this());
    // Init ASR Profile as CLOSE_TALK
    handleASRProfileChanged(alexaClientSDK::capabilityAgents::aip::ASRProfile::CLOSE_TALK);
    // Register the extension with the APL client bridge.
    aplClientBridge->registerSharedExtension(m_aplLiveViewExtension);
}

void LiveViewControllerPresentationAdapter::onPresentationAvailable(
    PresentationRequestToken id,
    std::shared_ptr<PresentationInterface> presentation) {
    ACSDK_DEBUG5(LX(__func__).d("id", id));

    m_executor.submit([this, id, presentation]() {
        if (id == m_startLiveViewRequestToken) {
            m_presentation = presentation;
            bool cameraTalkSupported =
                TalkMode::NO_SUPPORT != m_startLiveViewRequest->viewerExperience.audioProperties.talkMode;
            // The camera mic should be enabled on int if the request is UNMUTED, supports TWO_WAY_TALK,
            // AND the device is not using a CLOSE_TALK ASR Profile
            bool micInitEnabled =
                alexaClientSDK::capabilityAgents::aip::ASRProfile::CLOSE_TALK != m_asrProfile &&
                AudioState::UNMUTED == m_startLiveViewRequest->viewerExperience.audioProperties.microphoneState &&
                ConcurrentTwoWayTalkState::ENABLED ==
                    m_startLiveViewRequest->viewerExperience.audioProperties.concurrentTwoWayTalk;
            std::string startLiveViewPayload = serializeStartLiveViewPayload(std::move(m_startLiveViewRequest));
            // Init camera render
            m_liveViewCameraIPCHandler->renderCamera(startLiveViewPayload);
            // Init camera UI mic state, will handle the call to toggle the mic once initialized
            executeSetCameraUIMicState(micInitEnabled);
            // Set self as HoldToTalk handler if ASR Profile is CLOSE_TALK and the camera actually supports talk
            if (alexaClientSDK::capabilityAgents::aip::ASRProfile::CLOSE_TALK == m_asrProfile && cameraTalkSupported) {
                m_interactionManager->overrideHoldToTalkSpeechHandler(shared_from_this());
            }
        }
    });
}

void LiveViewControllerPresentationAdapter::onPresentationStateChanged(
    PresentationRequestToken id,
    PresentationState newState) {
    ACSDK_DEBUG5(LX(__func__).d("id", id).d("newState", newState));

    m_executor.submit([this, id, newState] {
        if (id != m_startLiveViewRequestToken) {
            ACSDK_WARN(LX(__func__)
                           .d("reason", "state change does not correspond to the last presentation")
                           .d("id", id)
                           .d("m_startLiveViewRequestToken", m_startLiveViewRequestToken));
            return;
        }

        switch (newState) {
            case PresentationState::FOREGROUND:
                // fall-through
            case PresentationState::FOREGROUND_UNFOCUSED:
                break;
            case PresentationState::BACKGROUND:
                executeDismissPresentation();
                break;
            case PresentationState::NONE:
                if (m_presentation) {
                    m_presentation.reset();
                }
                executeOnPresentationDismissed();
                break;
        }
    });
}

bool LiveViewControllerPresentationAdapter::onNavigateBack(PresentationRequestToken id) {
    /* NO OP: Let PO manage the back navigation */
    return false;
}

void LiveViewControllerPresentationAdapter::cameraMicrophoneStateChanged(const std::string& message) {
    bool enabled = false;
    if (!jsonUtils::retrieveValue(message, ipc::CAMERA_ENABLED_TAG, &enabled)) {
        ACSDK_ERROR(LX(__func__).d("reason", "enabledNotFound"));
        return;
    }
    handleSetCameraMicrophoneState(enabled);
}

void LiveViewControllerPresentationAdapter::cameraFirstFrameRendered(const std::string& message) {
    m_executor.submit([this]() {
        if (m_aplLiveViewExtension) {
            m_aplLiveViewExtension->onCameraFirstFrameRendered();
        }
    });
}

void LiveViewControllerPresentationAdapter::windowIdReport(const std::string& message) {
    m_executor.submit([this, message]() {
        rapidjson::Document payload;
        if (!jsonUtils::parseJSON(message, &payload)) {
            ACSDK_ERROR(LX(__func__).d("reason", "invalid message string"));
            return;
        }

        std::string liveViewCameraWindowId;
        if (!jsonUtils::retrieveValue(message, ipc::CAMERA_WINDOW_ID_TAG, &liveViewCameraWindowId)) {
            ACSDK_WARN(LX(__func__).d("reason", "live view camera window ID not found"));
        } else {
            m_liveViewCameraWindowId = liveViewCameraWindowId;
        }
    });
}

LiveViewControllerInterface::Response LiveViewControllerPresentationAdapter::start(
    std::unique_ptr<LiveViewControllerInterface::StartLiveViewRequest> request) {
    m_executor.submit([this]() {
        if (m_presentationOrchestratorClient) {
            PresentationOptions poOptions;
            poOptions.presentationLifespan = PresentationLifespan::SHORT;
            poOptions.interfaceName = LIVE_VIEW_CAMERA_INTERFACE_NAME;
            poOptions.timeout = PresentationInterface::getTimeoutDisabled();

            m_startLiveViewRequestToken = m_presentationOrchestratorClient->requestWindow(
                m_liveViewCameraWindowId, poOptions, shared_from_this());
        }
    });

    m_startLiveViewRequest = std::move(request);

    return {};
}

LiveViewControllerInterface::Response LiveViewControllerPresentationAdapter::stop() {
    dismissPresentation();
    return {};
}

LiveViewControllerInterface::Response LiveViewControllerPresentationAdapter::setCameraState(CameraState cameraState) {
    m_executor.submit([this, cameraState]() {
        auto cameraStateStr = cameraStateToString(cameraState);
        m_liveViewCameraIPCHandler->setCameraState(cameraStateStr);

        if (m_aplLiveViewExtension) {
            m_aplLiveViewExtension->setCameraState(cameraStateStr);
        }
    });

    return {};
}

Configuration LiveViewControllerPresentationAdapter::getConfiguration() {
    const static std::set<DisplayMode> supportedDisplayModes = {DisplayMode::FULL_SCREEN};
    const static std::set<OverlayType> supportedOverlayTypes = {OverlayType::NONE};
    const static std::set<OverlayPosition> supportedOverlayPositions = {OverlayPosition::TOP_RIGHT};

    return Configuration(supportedDisplayModes, supportedOverlayTypes, supportedOverlayPositions);
}

void LiveViewControllerPresentationAdapter::handleCameraExitRequest() {
    dismissPresentation();
}

void LiveViewControllerPresentationAdapter::handleChangeCameraMicStateRequest(bool micOn) {
    handleSetCameraMicrophoneState(micOn);
}

void LiveViewControllerPresentationAdapter::handleSetCameraMicrophoneState(bool enabled) {
    m_executor.submit([this, enabled]() { executeNotifyMicrophoneStateChanged(enabled); });
}

void LiveViewControllerPresentationAdapter::setPresentationOrchestrator(
    std::shared_ptr<PresentationOrchestratorClientInterface> poClient) {
    m_executor.submit([this, poClient]() { m_presentationOrchestratorClient = std::move(poClient); });
}

void LiveViewControllerPresentationAdapter::setInteractionManager(
    std::shared_ptr<common::InteractionManager> interactionManager) {
    m_executor.submit([this, interactionManager]() { m_interactionManager = std::move(interactionManager); });
}

bool LiveViewControllerPresentationAdapter::addObserver(std::weak_ptr<LiveViewControllerObserverInterface> observer) {
    m_notifier.addWeakPtrObserver(observer);
    return true;
}

void LiveViewControllerPresentationAdapter::removeObserver(
    std::weak_ptr<LiveViewControllerObserverInterface> observer) {
    m_notifier.removeWeakPtrObserver(observer);
}

void LiveViewControllerPresentationAdapter::onASRProfileChanged(const std::string& profile) {
    handleASRProfileChanged(capabilityAgents::aip::getASRProfile(profile));
}

void LiveViewControllerPresentationAdapter::onStateChanged(State state) {
    // No-op
}

void LiveViewControllerPresentationAdapter::handleASRProfileChanged(
    alexaClientSDK::capabilityAgents::aip::ASRProfile profile) {
    m_executor.submit([this, profile]() {
        m_asrProfile = profile;
        if (m_aplLiveViewExtension) {
            m_aplLiveViewExtension->setAsrProfile(asrProfileToString(profile));
        }
    });
}

void LiveViewControllerPresentationAdapter::dismissPresentation() {
    m_executor.submit([this]() { executeDismissPresentation(); });
}

void LiveViewControllerPresentationAdapter::executeDismissPresentation() {
    if (m_presentation) {
        m_presentation->dismiss();
    }
}

void LiveViewControllerPresentationAdapter::executeOnPresentationDismissed() {
    ACSDK_DEBUG9(LX(__func__));
    m_interactionManager->overrideHoldToTalkSpeechHandler(nullptr);

    m_liveViewCameraIPCHandler->clearCamera();

    if (m_aplLiveViewExtension) {
        m_aplLiveViewExtension->onCameraCleared();
    }
    m_notifier.notifyObservers(
        [](const std::shared_ptr<LiveViewControllerObserverInterface>& observer) { observer->onLiveViewCleared(); });
}

void LiveViewControllerPresentationAdapter::executeNotifyMicrophoneStateChanged(bool enabled) {
    ACSDK_DEBUG9(LX(__func__));
    m_notifier.notifyObservers([enabled](const std::shared_ptr<LiveViewControllerObserverInterface>& observer) {
        observer->onMicrophoneStateChanged(microphoneStateToAudioState(enabled));
    });
}

void LiveViewControllerPresentationAdapter::executeSetCameraUIMicState(bool enabled) {
    if (m_aplLiveViewExtension) {
        m_aplLiveViewExtension->setCameraMicrophoneState(enabled);
    }
}

std::future<bool> LiveViewControllerPresentationAdapter::notifyOfWakeWord(
    capabilityAgents::aip::AudioProvider wakeWordAudioProvider,
    avsCommon::avs::AudioInputStream::Index beginIndex,
    avsCommon::avs::AudioInputStream::Index endIndex,
    std::string keyword,
    std::chrono::steady_clock::time_point startOfSpeechTimestamp,
    std::shared_ptr<const std::vector<char>> KWDMetadata) {
    // No-op
    return m_executor.submit([]() { return false; });
}

std::future<bool> LiveViewControllerPresentationAdapter::notifyOfTapToTalk(
    capabilityAgents::aip::AudioProvider tapToTalkAudioProvider,
    avsCommon::avs::AudioInputStream::Index beginIndex,
    std::chrono::steady_clock::time_point startOfSpeechTimestamp) {
    // No-op
    return m_executor.submit([]() { return false; });
}

std::future<bool> LiveViewControllerPresentationAdapter::notifyOfHoldToTalkStart(
    capabilityAgents::aip::AudioProvider holdToTalkAudioProvider,
    std::chrono::steady_clock::time_point startOfSpeechTimestamp,
    avsCommon::avs::AudioInputStream::Index beginIndex) {
    ACSDK_DEBUG5(LX(__func__));
    return m_executor.submit([this]() {
        executeSetCameraUIMicState(true);
        executeNotifyMicrophoneStateChanged(true);
        return true;
    });
}

std::future<bool> LiveViewControllerPresentationAdapter::notifyOfHoldToTalkEnd() {
    ACSDK_DEBUG5(LX(__func__));
    return m_executor.submit([this]() {
        executeSetCameraUIMicState(false);
        executeNotifyMicrophoneStateChanged(false);
        return true;
    });
}

std::future<bool> LiveViewControllerPresentationAdapter::notifyOfTapToTalkEnd() {
    // No-op
    return m_executor.submit([]() { return false; });
}

}  // namespace liveViewController
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK