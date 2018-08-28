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
    std::unique_ptr<std::istream> defaultAudio = audioFactory();
    if (!defaultAudio) {
        ACSDK_ERROR(LX("startFailed").m("default audio is nullptr"));
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
        m_numberOfStreamsRenderedThisLoop{0},
        m_remainingLoopCount{0},
        m_directiveLoopCount{0},
        m_loopPause{std::chrono::milliseconds{0}},
        m_isStopping{false} {
    resetSourceId();
}

void Renderer::executeSetObserver(std::shared_ptr<RendererObserverInterface> observer) {
    ACSDK_DEBUG1(LX("executeSetObserver"));
    m_observer = observer;
}
bool Renderer::shouldPlayDefault() {
    ACSDK_DEBUG9(LX("shouldPlayDefault"));
    return m_urls.empty();
}

bool Renderer::shouldMediaPlayerRepeat() {
    ACSDK_DEBUG9(LX("shouldMediaPlayerRepeat"));
    return m_urls.empty() && (0 == m_directiveLoopCount) && (0 == m_loopPause.count());
}

bool Renderer::isLastSource() {
    ACSDK_DEBUG9(LX("isLastSource"));
    return isLastSourceInLoop() && m_remainingLoopCount <= 0;
}

bool Renderer::isLastSourceInLoop() {
    ACSDK_DEBUG9(LX("isLastSourceInLoop"));
    return m_numberOfStreamsRenderedThisLoop >= static_cast<int>(m_urls.size());
}

bool Renderer::shouldRenderNext() {
    ACSDK_DEBUG9(LX("shouldRenderNext"));

    if (shouldMediaPlayerRepeat()) {
        return false;
    }

    if (m_directiveLoopCount > 0) {
        return (m_remainingLoopCount > 0);
    }

    if (shouldPlayDefault() && (0 != m_loopPause.count())) {
        return true;
    }
    return false;
}

bool Renderer::shouldPause() {
    ACSDK_DEBUG9(LX("shouldPause"));
    if (0 == m_loopPause.count()) {
        return false;
    }

    if (m_directiveLoopCount == 0) {
        return true;
    }

    if (m_remainingLoopCount > 0) {
        return true;
    }

    return false;
}
void Renderer::pause() {
    ACSDK_DEBUG9(LX("pause"));
    std::this_thread::sleep_for(m_loopPause);
}

void Renderer::play() {
    ACSDK_DEBUG9(LX("play"));
    if (shouldPlayDefault()) {
        m_currentSourceId = m_mediaPlayer->setSource(m_defaultAudioFactory(), shouldMediaPlayerRepeat());
    } else {
        m_currentSourceId = m_mediaPlayer->setSource(m_urls[m_numberOfStreamsRenderedThisLoop]);
    }
    if (!isSourceIdOk(m_currentSourceId)) {
        ACSDK_ERROR(
            LX("executeStartFailed").d("m_currentSourceId", m_currentSourceId).m("SourceId response was invalid."));
        return;
    }

    if (!m_mediaPlayer->play(m_currentSourceId)) {
        const std::string errorMessage{"MediaPlayer play request failed."};
        ACSDK_ERROR(LX("executeStartFailed").d("m_currentSourceId", m_currentSourceId).m(errorMessage));
        handlePlaybackError(errorMessage);
    }
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
    m_remainingLoopCount = loopCount;
    m_directiveLoopCount = loopCount;
    m_loopPause = loopPause;
    m_defaultAudioFactory = audioFactory;

    ACSDK_DEBUG9(
        LX("executeStart")
            .d("m_urls.size", m_urls.size())
            .d("m_remainingLoopCount", m_remainingLoopCount)
            .d("m_loopPause (ms)", std::chrono::duration_cast<std::chrono::milliseconds>(m_loopPause).count()));
    m_isStopping = false;

    m_numberOfStreamsRenderedThisLoop = 0;

    play();
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

    ++m_numberOfStreamsRenderedThisLoop;

    if (!m_isStopping && shouldRenderNext()) {
        if (renderNextAudioAsset()) {
            return;
        }

        finalState = RendererObserverInterface::State::COMPLETED;
    }

    resetSourceId();
    notifyObserver(finalState);
    m_observer = nullptr;
}

bool Renderer::renderNextAudioAsset() {
    // If we have completed a loop, then update our counters, and determine what to do next.  If the URLs aren't
    // reachable, m_urls will be empty.
    if (isLastSourceInLoop()) {
        m_remainingLoopCount--;
        m_numberOfStreamsRenderedThisLoop = 0;
        ACSDK_DEBUG5(LX("renderNextAudioAsset")
                         .d("remainingLoopCount", m_remainingLoopCount)
                         .d("nextAudioIndex", m_numberOfStreamsRenderedThisLoop)
                         .m("Preparing the audio loop counters."));

        if (shouldPause()) {
            pause();
        }
    }

    if (!shouldRenderNext()) {
        return false;
    }

    play();

    return true;
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

    // This will cause a retry (through Renderer::start) using the same code paths as before, except in this case the
    // urls to render will be empty.
    handlePlaybackError(error);
}

void Renderer::notifyObserver(RendererObserverInterface::State state, const std::string& message) {
    if (m_observer) {
        m_observer->onRendererStateChange(state, message);
    }
}

void Renderer::resetSourceId() {
    m_currentSourceId = MediaPlayerInterface::ERROR;
}

void Renderer::handlePlaybackError(const std::string& error) {
    resetSourceId();

    notifyObserver(RendererObserverInterface::State::ERROR, error);
    m_observer = nullptr;
}

}  // namespace renderer
}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
