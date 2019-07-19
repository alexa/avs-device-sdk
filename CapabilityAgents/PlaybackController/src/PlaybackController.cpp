/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <iostream>

#include "PlaybackController/PlaybackController.h"
#include "PlaybackController/PlaybackMessageRequest.h"

#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/EventBuilder.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace playbackController {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;

/// PlaybackController capability constants
/// PlaybackController interface type
static const std::string PLAYBACKCONTROLLER_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";
/// PlaybackController interface name
static const std::string PLAYBACKCONTROLLER_CAPABILITY_INTERFACE_NAME = "PlaybackController";
/// PlaybackController interface version
static const std::string PLAYBACKCONTROLLER_CAPABILITY_INTERFACE_VERSION = "1.1";

/// String to identify log entries originating from this file.
static const std::string TAG("PlaybackController");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// String to identify the AVS namespace of the event we send.
static const std::string PLAYBACK_CONTROLLER_NAMESPACE = "PlaybackController";

/**
 * Creates the PlaybackController capability configuration.
 *
 * @return The PlaybackController capability configuration.
 */
static std::shared_ptr<CapabilityConfiguration> getPlaybackControllerCapabilityConfiguration();

std::shared_ptr<PlaybackController> PlaybackController::create(
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<MessageSenderInterface> messageSender) {
    if (!contextManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullContextManager"));
        return nullptr;
    }
    if (!messageSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMessageSender"));
        return nullptr;
    }

    return std::shared_ptr<PlaybackController>(new PlaybackController(contextManager, messageSender));
}

void PlaybackController::doShutdown() {
    m_executor.shutdown();
    m_messageSender.reset();
    m_contextManager.reset();
}

void PlaybackController::handleCommand(const PlaybackCommand& command) {
    auto task = [this, &command]() {
        ACSDK_DEBUG9(LX("buttonPressedExecutor").d("Button", command));

        if (m_commands.empty()) {
            ACSDK_DEBUG9(LX("buttonPressedExecutor").m("Queue is empty, call getContext()."));
            m_contextManager->getContext(shared_from_this());
        }
        m_commands.push(&command);
    };

    ACSDK_DEBUG9(LX("buttonPressed").d("Button", command));
    m_executor.submit(task);
}

void PlaybackController::onButtonPressed(PlaybackButton button) {
    auto& command = PlaybackCommand::buttonToCommand(button);
    handleCommand(command);
}

void PlaybackController::onTogglePressed(PlaybackToggle toggle, bool action) {
    auto& command = PlaybackCommand::toggleToCommand(toggle, action);
    handleCommand(command);
}

void PlaybackController::messageSent(
    const PlaybackCommand& command,
    MessageRequestObserverInterface::Status messageStatus) {
    if (MessageRequestObserverInterface::Status::SUCCESS == messageStatus) {
        ACSDK_DEBUG(LX("messageSentSucceeded").d("ButtonPressed", command));
    } else {
        ACSDK_ERROR(LX("messageSentFailed").d("ButtonPressed", command).d("error", messageStatus));
    }
}

void PlaybackController::onContextAvailable(const std::string& jsonContext) {
    auto task = [this, jsonContext]() {
        ACSDK_DEBUG9(LX("onContextAvailableExecutor"));

        if (m_commands.empty()) {
            // The queue shouldn't be empty, log a warning message and return here.
            ACSDK_WARN(LX("onContextAvailableExecutor").m("Queue is empty, return."));
            return;
        }

        auto& command = *m_commands.front();
        m_commands.pop();

        auto msgIdAndJsonEvent = buildJsonEventString(
            PLAYBACK_CONTROLLER_NAMESPACE, command.getEventName(), "", command.getEventPayload(), jsonContext);
        m_messageSender->sendMessage(
            std::make_shared<PlaybackMessageRequest>(command, msgIdAndJsonEvent.second, shared_from_this()));
        if (!m_commands.empty()) {
            ACSDK_DEBUG9(LX("onContextAvailableExecutor").m("Queue is not empty, call getContext()."));
            m_contextManager->getContext(shared_from_this());
        }
    };

    ACSDK_DEBUG9(LX("onContextAvailable"));
    m_executor.submit(task);
}

void PlaybackController::onContextFailure(const ContextRequestError error) {
    auto task = [this, error]() {
        if (m_commands.empty()) {
            // The queue shouldn't be empty, log a warning message and return here.
            ACSDK_WARN(LX("onContextFailureExecutor").m("Queue is empty, return."));
            return;
        }

        auto& command = m_commands.front();
        m_commands.pop();

        ACSDK_ERROR(LX("contextRetrievalFailed").d("ButtonPressed", command).d("error", error));

        if (!m_commands.empty()) {
            m_contextManager->getContext(shared_from_this());
        }
    };

    ACSDK_DEBUG9(LX("onContextFailure"));
    m_executor.submit(task);
}

PlaybackController::PlaybackController(
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<MessageSenderInterface> messageSender) :
        RequiresShutdown{"PlaybackController"},
        m_messageSender{messageSender},
        m_contextManager{contextManager} {
    m_capabilityConfigurations.insert(getPlaybackControllerCapabilityConfiguration());
}

std::shared_ptr<CapabilityConfiguration> getPlaybackControllerCapabilityConfiguration() {
    std::unordered_map<std::string, std::string> configMap;
    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, PLAYBACKCONTROLLER_CAPABILITY_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, PLAYBACKCONTROLLER_CAPABILITY_INTERFACE_NAME});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, PLAYBACKCONTROLLER_CAPABILITY_INTERFACE_VERSION});

    return std::make_shared<CapabilityConfiguration>(configMap);
}

std::unordered_set<std::shared_ptr<CapabilityConfiguration>> PlaybackController::getCapabilityConfigurations() {
    return m_capabilityConfigurations;
}

}  // namespace playbackController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
