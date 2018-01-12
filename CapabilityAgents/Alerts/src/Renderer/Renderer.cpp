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

/**
 * Local utility function to evaluate if a sourceId returned from the MediaPlayer is ok.
 *
 * @param sourceId The sourceId being tested.
 * @return Whether the sourceId is ok or not.
 */
static bool isSourceIdOk(MediaPlayerInterface::SourceId sourceId) {
    return sourceId != MediaPlayerInterface::ERROR;
}

std::shared_ptr<Renderer> Renderer::create(std::shared_ptr<MediaPlayerInterface> mediaPlayer) {
    if (!mediaPlayer) {
        ACSDK_ERROR(LX("createFailed").m("mediaPlayer parameter was nullptr."));
        return nullptr;
    }

    auto renderer = std::shared_ptr<Renderer>(new Renderer{mediaPlayer});
    mediaPlayer->setObserver(renderer);
    return renderer;
}

void Renderer::setObserver(std::shared_ptr<RendererObserverInterface> observer) {
    m_executor.submit([this, observer]() { executeSetObserver(observer); });
}

void Renderer::start(
    std::function<std::unique_ptr<std::istream>()> audioFactory,
    const std::vector<std::string>& urls,
    int loopCount,
    std::chrono::milliseconds loopPause) {
    {
        auto defaultAudio = audioFactory();
        if ((!defaultAudio || !defaultAudio->good()) && urls.empty()) {
            ACSDK_ERROR(LX("startFailed").m("default audio is bad and urls are empty."));
            return;
        }
    }

    if (loopCount < 0) {
        ACSDK_ERROR(LX("startInvalidParam").m("loopCount less than zero - adjusting to acceptable minimum."));
        loopCount = 0;
    }

    if (loopPause.count() < 0) {
        ACSDK_ERROR(LX("startInvalidParam").m("loopPause less than zero - adjusting to acceptable minimum."));
        loopPause = std::chrono::milliseconds{0};
    }

    m_executor.submit(
        [this, audioFactory, urls, loopCount, loopPause]() { executeStart(audioFactory, urls, loopCount, loopPause); });
}

void Renderer::stop() {
    m_executor.submit([this]() { executeStop(); });
}

void Renderer::onPlaybackStarted(SourceId sourceId) {
    m_executor.submit([this, sourceId]() { executeOnPlaybackStarted(sourceId); });
}

void Renderer::onPlaybackStopped(SourceId sourceId) {
    m_executor.submit([this, sourceId]() { executeOnPlaybackStopped(sourceId); });
}

void Renderer::onPlaybackFinished(SourceId sourceId) {
    m_executor.submit([this, sourceId]() { executeOnPlaybackFinished(sourceId); });
}

void Renderer::onPlaybackError(
    SourceId sourceId,
    const avsCommon::utils::mediaPlayer::ErrorType& type,
    std::string error) {
    m_executor.submit([this, sourceId, type, error]() { executeOnPlaybackError(sourceId, type, error); });
}

Renderer::Renderer(std::shared_ptr<MediaPlayerInterface> mediaPlayer) :
        m_mediaPlayer{mediaPlayer},
        m_observer{nullptr},
        m_nextUrlIndexToRender{0},
        m_loopCount{0},
        m_loopPause{std::chrono::milliseconds{0}},
        m_isStopping{false} {
    resetSourceId();
}

void Renderer::executeSetObserver(std::shared_ptr<RendererObserverInterface> observer) {
    ACSDK_DEBUG1(LX("executeSetObserver"));
    m_observer = observer;
}

void Renderer::executeStart(
    std::function<std::unique_ptr<std::istream>()> audioFactory,
    const std::vector<std::string>& urls,
    int loopCount,
    std::chrono::milliseconds loopPause) {
    ACSDK_DEBUG1(LX("executeStart")
                     .d("urls.size", urls.size())
                     .d("loopCount", loopCount)
                     .d("loopPause (ms)", std::chrono::duration_cast<std::chrono::milliseconds>(loopPause).count()));
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
        m_currentSourceId = m_mediaPlayer->setSource(audioFactory(), true);
    } else {
        m_nextUrlIndexToRender = 0;
        ACSDK_DEBUG9(LX("executeStart").d("setSource", m_nextUrlIndexToRender));
        m_currentSourceId = m_mediaPlayer->setSource(m_urls[m_nextUrlIndexToRender++]);
    }

    ACSDK_INFO(LX("executeStart").d("m_currentSourceId", m_currentSourceId));

    if (!isSourceIdOk(m_currentSourceId)) {
        ACSDK_ERROR(
            LX("executeStartFailed").d("m_currentSourceId", m_currentSourceId).m("SourceId response was invalid."));
        return;
    }

    if (!m_mediaPlayer->play(m_currentSourceId)) {
        ACSDK_ERROR(
            LX("executeStartFailed").d("m_currentSourceId", m_currentSourceId).m("MediaPlayer play request failed."));
        resetSourceId();
    }
}

void Renderer::executeStop() {
    ACSDK_DEBUG1(LX("executeStop"));
    if (m_mediaPlayer->stop(m_currentSourceId)) {
        m_isStopping = true;
    } else {
        std::string errorMessage = "mediaPlayer stop request failed.";
        ACSDK_ERROR(LX("executeStopFailed").d("SourceId", m_currentSourceId).m(errorMessage));
        notifyObserver(RendererObserverInterface::State::ERROR, errorMessage);
    }
}

void Renderer::executeOnPlaybackStarted(SourceId sourceId) {
    ACSDK_DEBUG1(LX("executeOnPlaybackStarted").d("sourceId", sourceId));
    if (m_currentSourceId != sourceId) {
        ACSDK_DEBUG9(LX("executeOnPlaybackStarted")
                         .d("m_currentSourceId", m_currentSourceId)
                         .m("Ignoring - different from expected source id."));
        return;
    }

    notifyObserver(RendererObserverInterface::State::STARTED);
}

void Renderer::executeOnPlaybackStopped(SourceId sourceId) {
    ACSDK_DEBUG1(LX("executeOnPlaybackStopped").d("sourceId", sourceId));
    if (m_currentSourceId != sourceId) {
        ACSDK_DEBUG9(LX("executeOnPlaybackStopped")
                         .d("m_currentSourceId", m_currentSourceId)
                         .m("Ignoring - different from expected source id."));
        return;
    }

    resetSourceId();
    notifyObserver(RendererObserverInterface::State::STOPPED);
    m_observer = nullptr;
}

void Renderer::executeOnPlaybackFinished(SourceId sourceId) {
    ACSDK_DEBUG1(LX("executeOnPlaybackFinished").d("sourceId", sourceId));
    if (m_currentSourceId != sourceId) {
        ACSDK_DEBUG9(LX("executeOnPlaybackFinished")
                         .d("m_currentSourceId", m_currentSourceId)
                         .m("Ignoring - different from expected source id."));
        return;
    }

    RendererObserverInterface::State finalState = RendererObserverInterface::State::STOPPED;

    if (!m_isStopping && !m_urls.empty()) {
        if (renderNextUrl()) {
            return;
        }

        finalState = RendererObserverInterface::State::COMPLETED;
    }

    resetSourceId();
    notifyObserver(finalState);
    m_observer = nullptr;
}

bool Renderer::renderNextUrl() {
    // sanity check.
    if (m_urls.empty()) {
        return false;
    }

    bool shouldRenderNextUrl = true;

    // If we have completed a loop, then update our counters, and determine what to do next.
    if (m_nextUrlIndexToRender >= static_cast<int>(m_urls.size())) {
        ACSDK_DEBUG9(LX("renderNextUrl")
                         .d("loopCount", m_loopCount)
                         .d("nextUrlIndex", m_nextUrlIndexToRender)
                         .m("updating counters."));
        m_loopCount--;
        m_nextUrlIndexToRender = 0;

        if (0 == m_loopCount) {
            shouldRenderNextUrl = false;
        } else if (m_loopPause.count() > 0) {
            std::this_thread::sleep_for(m_loopPause);
        }
    }

    // If we should continue to the next url, let's kick it off.
    if (shouldRenderNextUrl) {
        ACSDK_DEBUG9(LX("renderNextUrl").d("setSource", m_nextUrlIndexToRender));

        std::string url = m_urls[m_nextUrlIndexToRender++];
        m_currentSourceId = m_mediaPlayer->setSource(url);
        if (!isSourceIdOk(m_currentSourceId)) {
            std::string errorMessage = "SourceId response from setSource was invalid.";
            ACSDK_ERROR(LX("renderNextUrl").d("SourceId", m_currentSourceId).sensitive("url", url).m(errorMessage));
            notifyObserver(RendererObserverInterface::State::ERROR, errorMessage);
            return false;
        }
        if (!m_mediaPlayer->play(m_currentSourceId)) {
            std::string errorMessage = "MediaPlayer was unable to play next media item.";
            ACSDK_ERROR(LX("renderNextUrl").d("SourceId", m_currentSourceId).sensitive("url", url).m(errorMessage));
            notifyObserver(RendererObserverInterface::State::ERROR, errorMessage);
            return false;
        }

        ACSDK_DEBUG9(LX("renderNextUrl").m("Next url started successfully"));
    } else {
        ACSDK_DEBUG9(LX("renderNextUrl").m("No more urls to render."));
    }

    return shouldRenderNextUrl;
}

void Renderer::executeOnPlaybackError(
    SourceId sourceId,
    const avsCommon::utils::mediaPlayer::ErrorType& type,
    const std::string& error) {
    ACSDK_DEBUG1(LX("executeOnPlaybackError").d("sourceId", sourceId).d("type", type).d("error", error));
    if (m_currentSourceId != sourceId) {
        ACSDK_DEBUG9(LX("executeOnPlaybackError")
                         .d("m_currentSourceId", m_currentSourceId)
                         .m("Ignoring - different from expected source id."));
        return;
    }

    resetSourceId();
    notifyObserver(RendererObserverInterface::State::ERROR, error);
    m_observer = nullptr;
}

void Renderer::notifyObserver(RendererObserverInterface::State state, const std::string& message) {
    if (m_observer) {
        m_observer->onRendererStateChange(state, message);
    }
}

void Renderer::resetSourceId() {
    m_currentSourceId = MediaPlayerInterface::ERROR;
}

}  // namespace renderer
}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
