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

#include <AVSCommon/Utils/Logger/Logger.h>
#include "PlaybackController/PlaybackRouter.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace playbackController {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::avs;

/// String to identify log entries originating from this file.
static const std::string TAG("PlaybackRouter");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<PlaybackRouter> PlaybackRouter::create(std::shared_ptr<PlaybackHandlerInterface> defaultHandler) {
    ACSDK_DEBUG9(LX("create").m("called"));

    if (nullptr == defaultHandler) {
        ACSDK_ERROR(LX("createFailed").d("reason", "null defaultHandler"));
        return nullptr;
    }

    auto playbackRouter = std::shared_ptr<PlaybackRouter>(new PlaybackRouter(defaultHandler));
    return playbackRouter;
}

void PlaybackRouter::buttonPressed(PlaybackButton button) {
    ACSDK_DEBUG9(LX("buttonPressed").d("button", button));
    std::unique_lock<std::mutex> lock(m_handlerMutex);

    if (!m_handler) {
        ACSDK_ERROR(LX("buttonPressedFailed").m("called but handler is not set"));
        return;
    }
    auto observer = m_handler;
    lock.unlock();

    observer->onButtonPressed(button);
}

void PlaybackRouter::togglePressed(PlaybackToggle toggle, bool action) {
    ACSDK_DEBUG9(LX("togglePressed").d("toggle", toggle).d("action", action));
    std::unique_lock<std::mutex> lock(m_handlerMutex);

    if (!m_handler) {
        ACSDK_ERROR(LX("togglePressedFailed").m("called but handler is not set"));
        return;
    }
    auto observer = m_handler;
    lock.unlock();

    observer->onTogglePressed(toggle, action);
}

void PlaybackRouter::switchToDefaultHandler() {
    ACSDK_DEBUG9(LX("switchToDefaultHandler"));

    setHandler(m_defaultHandler);
}

PlaybackRouter::PlaybackRouter(std::shared_ptr<PlaybackHandlerInterface> defaultHandler) :
        RequiresShutdown{"PlaybackRouter"},
        m_handler{defaultHandler},
        m_defaultHandler{defaultHandler} {
}

void PlaybackRouter::doShutdown() {
    std::lock_guard<std::mutex> lock(m_handlerMutex);

    m_handler.reset();
    m_defaultHandler.reset();
}

void PlaybackRouter::setHandler(
    std::shared_ptr<PlaybackHandlerInterface> handler,
    std::shared_ptr<LocalPlaybackHandlerInterface> localHandler) {
    ACSDK_DEBUG9(LX("setHandler").d("handler", handler));
    std::lock_guard<std::mutex> guard(m_handlerMutex);

    if (!handler) {
        ACSDK_ERROR(LX("setHandler").d("handler", handler));
        return;
    }

    m_handler = handler;
    m_localHandler = localHandler;
}

void PlaybackRouter::useDefaultHandlerWith(std::shared_ptr<LocalPlaybackHandlerInterface> localHandler) {
    ACSDK_DEBUG9(LX("useDefaultHandlerWith"));
    setHandler(m_defaultHandler, localHandler);
}

bool PlaybackRouter::localOperation(LocalPlaybackHandlerInterface::PlaybackOperation op) {
    ACSDK_DEBUG9(LX("localOperation"));

    bool useFallback = true;
    {
        std::lock_guard<std::mutex> guard(m_handlerMutex);

        if (m_localHandler) {
            useFallback = !m_localHandler->localOperation(op);
            ACSDK_DEBUG(LX("localOperation").d("usingFallback", useFallback));
        }
    }

    if (useFallback) {
        switch (op) {
            case LocalPlaybackHandlerInterface::PlaybackOperation::STOP_PLAYBACK:
            case LocalPlaybackHandlerInterface::PlaybackOperation::PAUSE_PLAYBACK:
                buttonPressed(PlaybackButton::PAUSE);
                break;
            case LocalPlaybackHandlerInterface::PlaybackOperation::RESUME_PLAYBACK:
                buttonPressed(PlaybackButton::PLAY);
                break;
            default:
                return false;
        }
    }

    return true;
}

bool PlaybackRouter::localSeekTo(std::chrono::milliseconds location, bool fromStart) {
    ACSDK_DEBUG9(LX("localSeekTo").d("location", location.count()).d("fromStart", fromStart));
    std::lock_guard<std::mutex> guard(m_handlerMutex);
    if (m_localHandler) {
        return m_localHandler->localSeekTo(location, fromStart);
    }
    return false;
}

}  // namespace playbackController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
