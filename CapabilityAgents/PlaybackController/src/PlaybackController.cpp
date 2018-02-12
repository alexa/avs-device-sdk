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

#include <AVSCommon/AVS/EventBuilder.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace playbackController {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;

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

/// String to identify the AVS name of the event on the 'Play' button pressed.
static const std::string PLAYBACK_PLAY_NAME = "PlayCommandIssued";
/// String to identify the AVS name of the event on the 'Pause' button pressed.
static const std::string PLAYBACK_PAUSE_NAME = "PauseCommandIssued";
/// String to identify the AVS name of the event on the 'Next' button pressed.
static const std::string PLAYBACK_NEXT_NAME = "NextCommandIssued";
/// String to identify the AVS name of the event on the 'Previous' button pressed.
static const std::string PLAYBACK_PREVIOUS_NAME = "PreviousCommandIssued";

/**
 * A macro to be used in buttonToMessageName function to help with the translation
 * of @c Button to its AVS event name.
 *
 * @param button The @c Button that needs to be translated.
 */
#define ACSDK_PLAYBACK_BUTTON_TO_NAME(button) \
    case PlaybackButton::button:              \
        return ACSDK_CONCATENATE(ACSDK_CONCATENATE(PLAYBACK_, button), _NAME);

/**
 * Utility function to convert the Button into the name in the event message.
 *
 * @param button The @c Button for the message.
 * @return The name of the event message associated with the @c Button.
 */
static std::string buttonToMessageName(PlaybackButton button) {
    switch (button) {
        ACSDK_PLAYBACK_BUTTON_TO_NAME(PLAY);
        ACSDK_PLAYBACK_BUTTON_TO_NAME(PAUSE);
        ACSDK_PLAYBACK_BUTTON_TO_NAME(NEXT);
        ACSDK_PLAYBACK_BUTTON_TO_NAME(PREVIOUS);
    }
    return "UNKNOWN";
}

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

void PlaybackController::onButtonPressed(PlaybackButton button) {
    auto task = [this, button]() {
        ACSDK_DEBUG9(LX("buttonPressedExecutor").d("Button", button));

        if (m_buttons.empty()) {
            ACSDK_DEBUG9(LX("buttonPressedExecutor").m("Queue is empty, call getContext()."));
            m_contextManager->getContext(shared_from_this());
        }
        m_buttons.push(button);
    };

    ACSDK_DEBUG9(LX("buttonPressed").d("Button", button));
    m_executor.submit(task);
}

void PlaybackController::messageSent(PlaybackButton button, MessageRequestObserverInterface::Status messageStatus) {
    if (MessageRequestObserverInterface::Status::SUCCESS == messageStatus) {
        ACSDK_DEBUG(LX("messageSentSucceeded").d("ButtonPressed", button));
    } else {
        ACSDK_ERROR(LX("messageSentFailed").d("ButtonPressed", button).d("error", messageStatus));
    }
}

void PlaybackController::onContextAvailable(const std::string& jsonContext) {
    auto task = [this, jsonContext]() {
        ACSDK_DEBUG9(LX("onContextAvailableExecutor"));

        if (m_buttons.empty()) {
            // The queue shouldn't be empty, log a warning message and return here.
            ACSDK_WARN(LX("onContextAvailableExecutor").m("Queue is empty, return."));
            return;
        }

        auto button = m_buttons.front();
        m_buttons.pop();

        auto msgIdAndJsonEvent =
            buildJsonEventString(PLAYBACK_CONTROLLER_NAMESPACE, buttonToMessageName(button), "", "{}", jsonContext);
        m_messageSender->sendMessage(
            std::make_shared<PlaybackMessageRequest>(button, msgIdAndJsonEvent.second, shared_from_this()));

        if (!m_buttons.empty()) {
            ACSDK_DEBUG9(LX("onContextAvailableExecutor").m("Queue is not empty, call getContext()."));
            m_contextManager->getContext(shared_from_this());
        }
    };

    ACSDK_DEBUG9(LX("onContextAvailable"));
    m_executor.submit(task);
}

void PlaybackController::onContextFailure(const ContextRequestError error) {
    auto task = [this, error]() {
        if (m_buttons.empty()) {
            // The queue shouldn't be empty, log a warning message and return here.
            ACSDK_WARN(LX("onContextFailureExecutor").m("Queue is empty, return."));
            return;
        }

        auto button = m_buttons.front();
        m_buttons.pop();

        ACSDK_ERROR(LX("contextRetrievalFailed").d("ButtonPressed", button).d("error", error));

        if (!m_buttons.empty()) {
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
}

}  // namespace playbackController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
