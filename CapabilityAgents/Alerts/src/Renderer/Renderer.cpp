/*
 * Renderer.cpp
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "Alerts/Renderer/Renderer.h"

//#include "Alerts/Storage/AlertStorageInterface.h"
#include "AVSCommon/Utils/Logger/Logger.h"

#include <fstream>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {
namespace renderer {

using namespace avsCommon::utils::logger;
using namespace avsCommon::utils::mediaPlayer;

/// String to identify log entries originating from this file.
static const std::string TAG("Renderer");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<Renderer> Renderer::create(std::shared_ptr<MediaPlayerInterface> mediaPlayer) {
    if (!mediaPlayer) {
        ACSDK_ERROR(LX("createFailed").m("mediaPlayer parameter was nullptr."));
        return nullptr;
    }

    auto renderer = std::shared_ptr<Renderer>(new Renderer{mediaPlayer});
    mediaPlayer->setObserver(renderer);
    return renderer;
}

Renderer::Renderer(std::shared_ptr<MediaPlayerInterface> mediaPlayer) :
        m_mediaPlayer{mediaPlayer}, m_observer{nullptr}, m_loopCount{0}, m_loopPauseInMilliseconds{0},
        m_isRendering{false} {

}

void Renderer::setObserver(RendererObserverInterface* observer) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_observer = observer;
}

void Renderer::start(const std::string & localAudioFilePath,
                     const std::vector<std::string> & urls,
                     int loopCount, std::chrono::milliseconds loopPauseInMilliseconds) {
    if (localAudioFilePath.empty() && urls.empty()) {
        ACSDK_ERROR(LX("startFailed").m("both local audio file path and urls are empty."));
        return;
    }

    if (loopCount < 0) {
        ACSDK_ERROR(LX("startInvalidParam").m("loopCount less than zero - adjusting to acceptable minimum."));
        loopCount = 0;
    }

    if (loopPauseInMilliseconds.count() < 0) {
        ACSDK_ERROR(LX("startInvalidParam")
                .m("loopPauseInMilliseconds less than zero - adjusting to acceptable minimum."));
        loopPauseInMilliseconds = std::chrono::milliseconds::zero();
    }

    std::unique_lock<std::mutex> lock(m_mutex);
    m_localAudioFilePath = localAudioFilePath;
    m_urls = urls;
    m_loopCount = loopCount;
    m_loopPauseInMilliseconds = loopPauseInMilliseconds;
    lock.unlock();

    m_executor.submit([this] () { executeStart(); });
}

void Renderer::stop() {
    m_executor.submit([this] () { executeStop(); });
}

void Renderer::executeStart() {
    std::unique_lock<std::mutex> lock(m_mutex);
    std::string localAudioFilePathCopy = m_localAudioFilePath;
    lock.unlock();

    // TODO : ACSDK-389 to update the local audio to being streams rather than file paths.

    std::unique_ptr<std::ifstream> is(new std::ifstream(localAudioFilePathCopy, std::ios::binary));

    m_mediaPlayer->setSource(std::move(is), true);
    m_mediaPlayer->play();
}

void Renderer::executeStop() {
    std::unique_lock<std::mutex> lock(m_mutex);
    bool isRenderingCopy = m_isRendering;
    RendererObserverInterface* observerCopy = m_observer;
    lock.unlock();

    if (isRenderingCopy) {
        m_mediaPlayer->stop();
    } else {
        if (observerCopy) {
            observerCopy->onRendererStateChange(RendererObserverInterface::State::STOPPED);
        }
    }
}

void Renderer::onPlaybackStarted() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_isRendering = true;
    RendererObserverInterface* observerCopy = m_observer;
    lock.unlock();

    if (observerCopy) {
        observerCopy->onRendererStateChange(RendererObserverInterface::State::STARTED);
    }
}

void Renderer::onPlaybackFinished() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_isRendering = false;
    RendererObserverInterface* observerCopy = m_observer;
    lock.unlock();

    if (observerCopy) {
        observerCopy->onRendererStateChange(RendererObserverInterface::State::STOPPED);
    }
}

void Renderer::onPlaybackError(std::string error) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_isRendering = false;
    RendererObserverInterface* observerCopy = m_observer;
    lock.unlock();

    if (observerCopy) {
        observerCopy->onRendererStateChange(RendererObserverInterface::State::ERROR, error);
    }
}

} // namespace renderer
} // namespace alerts
} // namespace capabilityAgents
} // namespace alexaClientSDK
