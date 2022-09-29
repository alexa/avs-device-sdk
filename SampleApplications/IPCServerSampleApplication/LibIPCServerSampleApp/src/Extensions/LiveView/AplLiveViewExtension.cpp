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

#include <string>
#include <utility>

#include "IPCServerSampleApp/Extensions/LiveView/AplLiveViewExtension.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace extensions {
namespace liveView {

static const char TAG[] = "AplLiveViewExtension";

/// String to identify log entries originating from this file.
static const std::string SETTING_CAMERA_STATE_NAME = "cameraStateName";
static const std::string COMMAND_EXIT_CAMERA = "ExitCamera";
static const std::string COMMAND_CHANGE_CAMERA_MIC_STATE = "ChangeCameraMicState";
static const std::string EVENTHANDLER_ON_CAMERA_STATE_CHANGED_NAME = "OnCameraStateChanged";
static const std::string EVENTHANDLER_ON_CAMERA_MIC_STATE_CHANGED_NAME = "OnCameraMicStateChanged";
static const std::string EVENTHANDLER_ON_CAMERA_FIRST_FRAME_RENDERED_NAME = "OnCameraFirstFrameRendered";
static const std::string EVENTHANDLER_ON_CAMERA_CLEARED_NAME = "OnCameraCleared";
static const std::string EVENTHANDLER_ON_ASR_PROFILE_CHANGED_NAME = "OnASRProfileChanged";

static const std::string PROPERTY_CAMERA_MIC_STATE = "micOn";
static const std::string PROPERTY_CAMERA_STATE = "cameraState";
static const std::string PROPERTY_CAMERA_FIRST_FRAME_RENDERED = "firstFrameRendered";
static const std::string PROPERTY_ASR_PROFILE = "asrProfile";

/// List of accepted live view camera states.
static const std::vector<std::string> CAMERA_STATES = {"CONNECTING", "CONNECTED", "DISCONNECTED", "ERROR", "UNKNOWN"};

/// List of accepted device asr profiles.
/// https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/audio-hardware-configurations.html
static const std::vector<std::string> ASR_PROFILES = {"CLOSE_TALK", "NEAR_FIELD", "FAR_FIELD"};

AplLiveViewExtension::AplLiveViewExtension(std::shared_ptr<AplLiveViewExtensionObserverInterface> observer) :
        m_observer{std::move(observer)} {
    m_cameraState = apl::LiveMap::create();
    m_cameraState->set(PROPERTY_CAMERA_STATE, "DISCONNECTED");
    m_cameraState->set(PROPERTY_CAMERA_MIC_STATE, false);
    m_cameraState->set(PROPERTY_ASR_PROFILE, "NEAR_FIELD");
    m_cameraState->set(PROPERTY_CAMERA_FIRST_FRAME_RENDERED, false);
}

std::string AplLiveViewExtension::getUri() {
    return URI;
}

apl::Object AplLiveViewExtension::getEnvironment() {
    // No environment for LiveView Extension
    return apl::Object("");
}

std::list<apl::ExtensionCommandDefinition> AplLiveViewExtension::getCommandDefinitions() {
    std::list<apl::ExtensionCommandDefinition> extCmdDefs(
        {apl::ExtensionCommandDefinition(URI, COMMAND_EXIT_CAMERA).allowFastMode(true),
         apl::ExtensionCommandDefinition(URI, COMMAND_CHANGE_CAMERA_MIC_STATE)
             .allowFastMode(true)
             .property(PROPERTY_CAMERA_MIC_STATE, false, true)});
    return extCmdDefs;
}

std::list<apl::ExtensionEventHandler> AplLiveViewExtension::getEventHandlers() {
    std::list<apl::ExtensionEventHandler> extensionEventHandlers(
        {apl::ExtensionEventHandler(URI, EVENTHANDLER_ON_CAMERA_STATE_CHANGED_NAME),
         apl::ExtensionEventHandler(URI, EVENTHANDLER_ON_CAMERA_MIC_STATE_CHANGED_NAME),
         apl::ExtensionEventHandler(URI, EVENTHANDLER_ON_ASR_PROFILE_CHANGED_NAME),
         apl::ExtensionEventHandler(URI, EVENTHANDLER_ON_CAMERA_FIRST_FRAME_RENDERED_NAME),
         apl::ExtensionEventHandler(URI, EVENTHANDLER_ON_CAMERA_CLEARED_NAME)});
    return extensionEventHandlers;
}

std::unordered_map<std::string, apl::LiveObjectPtr> AplLiveViewExtension::getLiveDataObjects() {
    auto liveObjects = std::unordered_map<std::string, apl::LiveObjectPtr>();
    if (!m_cameraStateName.empty()) {
        liveObjects.emplace(m_cameraStateName, m_cameraState);
    }
    return liveObjects;
}

void AplLiveViewExtension::applySettings(const apl::Object& settings) {
    // Reset to defaults
    m_cameraStateName.clear();
    /// Apply @c apl::Content defined settings
    logMessage(apl::LogLevel::kInfo, TAG, __func__, settings.toDebugString());
    if (settings.isMap()) {
        if (settings.has(SETTING_CAMERA_STATE_NAME)) {
            m_cameraStateName = settings.get(SETTING_CAMERA_STATE_NAME).getString();
        }
    }
}

void AplLiveViewExtension::onExtensionEvent(
    const std::string& uri,
    const std::string& name,
    const apl::Object& source,
    const apl::Object& params,
    unsigned int event,
    std::shared_ptr<AplCoreExtensionEventCallbackResultInterface> resultCallback = nullptr) {
    auto eventDebugString = getEventDebugString(uri, name, params);
    logMessage(LOGLEVEL_DEBUG, TAG, __func__, eventDebugString);

    bool succeeded = true;

    if (m_observer) {
        if (COMMAND_EXIT_CAMERA == name) {
            m_observer->handleCameraExitRequest();
        } else if (COMMAND_CHANGE_CAMERA_MIC_STATE == name) {
            if (confirmEventParams(TAG, {PROPERTY_CAMERA_MIC_STATE}, params)) {
                bool micOn = params.get(PROPERTY_CAMERA_MIC_STATE).getBoolean();
                m_cameraState->set(PROPERTY_CAMERA_MIC_STATE, micOn);
                m_observer->handleChangeCameraMicStateRequest(micOn);
            } else {
                succeeded = false;
            }
        } else {
            logMessage(apl::LogLevel::kError, TAG, __func__, "Invalid Command: " + eventDebugString);
            succeeded = false;
        }
    } else {
        logMessage(apl::LogLevel::kError, TAG, __func__, "No Event Observer: " + eventDebugString);
        succeeded = false;
    }

    if (resultCallback) {
        resultCallback->onExtensionEventResult(event, succeeded);
    }
}

void AplLiveViewExtension::setCameraState(const std::string& cameraState) {
    if (m_cameraState->get(PROPERTY_CAMERA_STATE).getString() == cameraState) {
        logMessage(apl::LogLevel::kWarn, TAG, __func__, "Camera State Unchanged");
        return;
    }

    if (std::find(CAMERA_STATES.begin(), CAMERA_STATES.end(), cameraState) == CAMERA_STATES.end()) {
        logMessage(apl::LogLevel::kError, TAG, __func__, "Invalid Camera State: " + cameraState);
        return;
    }

    // If cameraState changes to anything but CONNECTED, always reset first frame rendered
    if (cameraState != "CONNECTED") {
        m_cameraState->set(PROPERTY_CAMERA_FIRST_FRAME_RENDERED, false);
    }

    m_cameraState->set(PROPERTY_CAMERA_STATE, cameraState);

    if (!m_eventHandler) {
        logMessage(apl::LogLevel::kWarn, TAG, __func__, "No Event Handler");
        return;
    }

    auto cameraStateObj = apl::ObjectMap({{PROPERTY_CAMERA_STATE, cameraState}});

    m_eventHandler->invokeExtensionEventHandler(URI, EVENTHANDLER_ON_CAMERA_STATE_CHANGED_NAME, cameraStateObj, false);
}

void AplLiveViewExtension::setCameraMicrophoneState(bool micOn) {
    if (m_cameraState->get(PROPERTY_CAMERA_MIC_STATE).getBoolean() == micOn) {
        logMessage(apl::LogLevel::kWarn, TAG, __func__, "Mic State Unchanged");
        return;
    }
    m_cameraState->set(PROPERTY_CAMERA_MIC_STATE, micOn);

    if (!m_eventHandler) {
        logMessage(apl::LogLevel::kWarn, TAG, __func__, "No Event Handler");
        return;
    }

    auto cameraMicState = apl::ObjectMap({{PROPERTY_CAMERA_MIC_STATE, micOn}});

    m_eventHandler->invokeExtensionEventHandler(
        URI, EVENTHANDLER_ON_CAMERA_MIC_STATE_CHANGED_NAME, cameraMicState, false);
}

void AplLiveViewExtension::setAsrProfile(const std::string& asrProfile) {
    if (m_cameraState->get(PROPERTY_ASR_PROFILE).getString() == asrProfile) {
        logMessage(apl::LogLevel::kWarn, TAG, __func__, "ASR Profile Unchanged");
        return;
    }

    if (std::find(ASR_PROFILES.begin(), ASR_PROFILES.end(), asrProfile) == ASR_PROFILES.end()) {
        logMessage(apl::LogLevel::kError, TAG, __func__, "Invalid ASR Profile: " + asrProfile);
        return;
    }

    m_cameraState->set(PROPERTY_ASR_PROFILE, asrProfile);

    if (!m_eventHandler) {
        logMessage(apl::LogLevel::kWarn, TAG, __func__, "No Event Handler");
        return;
    }

    auto asrProfileObj = apl::ObjectMap({{PROPERTY_ASR_PROFILE, asrProfile}});

    m_eventHandler->invokeExtensionEventHandler(URI, EVENTHANDLER_ON_ASR_PROFILE_CHANGED_NAME, asrProfileObj, false);
}

void AplLiveViewExtension::onCameraFirstFrameRendered() {
    if (m_cameraState->get(PROPERTY_CAMERA_FIRST_FRAME_RENDERED).getBoolean()) {
        logMessage(apl::LogLevel::kWarn, TAG, __func__, "First Frame already rendered");
        return;
    }

    m_cameraState->set(PROPERTY_CAMERA_FIRST_FRAME_RENDERED, true);

    if (!m_eventHandler) {
        logMessage(apl::LogLevel::kWarn, TAG, __func__, "No Event Handler");
        return;
    }

    m_eventHandler->invokeExtensionEventHandler(
        URI, EVENTHANDLER_ON_CAMERA_FIRST_FRAME_RENDERED_NAME, apl::ObjectMap({}), false);
}

void AplLiveViewExtension::onCameraCleared() {
    m_cameraState->set(PROPERTY_CAMERA_FIRST_FRAME_RENDERED, false);

    if (!m_eventHandler) {
        logMessage(apl::LogLevel::kWarn, TAG, __func__, "No Event Handler");
        return;
    }

    m_eventHandler->invokeExtensionEventHandler(URI, EVENTHANDLER_ON_CAMERA_CLEARED_NAME, apl::ObjectMap({}), false);
}

}  // namespace liveView
}  // namespace extensions
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
