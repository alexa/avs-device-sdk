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

void PlaybackRouter::setHandler(std::shared_ptr<PlaybackHandlerInterface> handler) {
    ACSDK_DEBUG9(LX("setHandler").d("handler", handler));
    std::lock_guard<std::mutex> guard(m_handlerMutex);

    if (!handler) {
        ACSDK_ERROR(LX("setHandler").d("handler", handler));
        return;
    }

    m_handler = handler;
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

}  // namespace playbackController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
