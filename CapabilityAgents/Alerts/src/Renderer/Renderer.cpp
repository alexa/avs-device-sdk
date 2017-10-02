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

void Renderer::setObserver(RendererObserverInterface* observer) {
    m_executor.submit([this, observer]() { executeSetObserver(observer); });
}

void Renderer::start(
    const std::string& localAudioFilePath,
    const std::vector<std::string>& urls,
    int loopCount,
    std::chrono::milliseconds loopPause) {
    if (localAudioFilePath.empty() && urls.empty()) {
        ACSDK_ERROR(LX("startFailed").m("both local audio file path and urls are empty."));
        return;
    }

    if (loopCount < 0) {
        ACSDK_ERROR(LX("startInvalidParam").m("loopCount less than zero - adjusting to acceptable minimum."));
        loopCount = 0;
    }

    if (loopPause.count() < 0) {
        ACSDK_ERROR(LX("startInvalidParam").m("loopPause less than zero - adjusting to acceptable minimum."));
        loopPause = std::chrono::milliseconds{0};
    }

    m_executor.submit([this, localAudioFilePath, urls, loopCount, loopPause]() {
        executeStart(localAudioFilePath, urls, loopCount, loopPause);
    });
}

void Renderer::stop() {
    m_executor.submit([this]() { executeStop(); });
}

void Renderer::onPlaybackStarted() {
    m_executor.submit([this]() { executeOnPlaybackStarted(); });
}

void Renderer::onPlaybackFinished() {
    m_executor.submit([this]() { executeOnPlaybackFinished(); });
}

void Renderer::onPlaybackError(const avsCommon::utils::mediaPlayer::ErrorType& type, std::string error) {
    m_executor.submit([this, type, error]() { executeOnPlaybackError(type, error); });
}

Renderer::Renderer(std::shared_ptr<MediaPlayerInterface> mediaPlayer) :
        m_mediaPlayer{mediaPlayer},
        m_observer{nullptr},
        m_nextUrlIndexToRender{0},
        m_loopCount{0},
        m_loopPause{std::chrono::milliseconds{0}},
        m_isRendering{false},
        m_isStopping{false} {
}

void Renderer::executeSetObserver(RendererObserverInterface* observer) {
    ACSDK_DEBUG1(LX("executeSetObserver"));
    m_observer = observer;
}

void Renderer::executeStart(
    const std::string& localAudioFilePath,
    const std::vector<std::string>& urls,
    int loopCount,
    std::chrono::milliseconds loopPause) {
    ACSDK_DEBUG1(LX("executeStart")
                     .d("localAudioFilePath", localAudioFilePath)
                     .d("urls.size", urls.size())
                     .d("loopCount", loopCount)
                     .d("loopPause (ms)", std::chrono::duration_cast<std::chrono::milliseconds>(loopPause).count()));
    m_localAudioFilePath = localAudioFilePath;
    m_urls = urls;
    m_loopCount = loopCount;
    m_loopPause = loopPause;
    ACSDK_DEBUG9(
        LX("executeStart")
            .d("m_urls.size", m_urls.size())
            .d("m_loopCount", m_loopCount)
            .d("m_loopPause (ms)", std::chrono::duration_cast<std::chrono::milliseconds>(m_loopPause).count()));
    m_isStopping = false;

    // TODO : ACSDK-389 to update the local audio to being streams rather than file paths.

    if (urls.empty()) {
        std::unique_ptr<std::ifstream> is(new std::ifstream(m_localAudioFilePath, std::ios::binary));
        if (is->fail()) {
            ACSDK_ERROR(LX("executeStartFailed").d("fileName", m_localAudioFilePath).m("could not open file."));
            return;
        }
        ACSDK_DEBUG9(LX("executeStart").d("setSource", m_localAudioFilePath));
        m_mediaPlayer->setSource(std::move(is), true);
    } else {
        m_nextUrlIndexToRender = 0;
        ACSDK_DEBUG9(LX("executeStart").d("setSource", m_nextUrlIndexToRender));
        m_mediaPlayer->setSource(m_urls[m_nextUrlIndexToRender++]);
    }

    m_mediaPlayer->play();
}

void Renderer::executeStop() {
    ACSDK_DEBUG1(LX("executeStop"));
    m_isStopping = true;
    if (m_isRendering) {
        m_mediaPlayer->stop();
    } else if (m_observer) {
        m_observer->onRendererStateChange(RendererObserverInterface::State::STOPPED);
    }
}

void Renderer::executeOnPlaybackStarted() {
    ACSDK_DEBUG1(LX("executeOnPlaybackStarted"));
    m_isRendering = true;

    if (m_observer) {
        m_observer->onRendererStateChange(RendererObserverInterface::State::STARTED);
    }
}

void Renderer::executeOnPlaybackFinished() {
    ACSDK_DEBUG1(LX("executeOnPlaybackFinished"));
    if (!m_isStopping && !m_urls.empty()) {
        // see if we need to reset the loop, and invoke the pause between loops.
        if (m_nextUrlIndexToRender >= static_cast<int>(m_urls.size()) && m_loopCount > 0) {
            if (m_loopPause.count() > 0) {
                std::this_thread::sleep_for(m_loopPause);
            }

            m_loopCount--;
            m_nextUrlIndexToRender = 0;
        }

        // play the next url in the list
        if (m_nextUrlIndexToRender < static_cast<int>(m_urls.size())) {
            ACSDK_DEBUG9(LX("executeonPlaybackFinished").d("setSource", m_nextUrlIndexToRender));
            m_mediaPlayer->setSource(m_urls[m_nextUrlIndexToRender++]);
            m_mediaPlayer->play();

            return;
        }
    }

    m_isRendering = false;

    if (m_observer) {
        m_observer->onRendererStateChange(RendererObserverInterface::State::STOPPED);
    }
}

void Renderer::executeOnPlaybackError(const avsCommon::utils::mediaPlayer::ErrorType& type, const std::string& error) {
    ACSDK_DEBUG1(LX("executeOnPlaybackError").d("type", type).d("error", error));
    m_isRendering = false;

    if (m_observer) {
        m_observer->onRendererStateChange(RendererObserverInterface::State::ERROR, error);
    }
}

}  // namespace renderer
}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
