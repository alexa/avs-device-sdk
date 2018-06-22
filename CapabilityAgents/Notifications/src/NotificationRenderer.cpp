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

#include <thread>

#include <AVSCommon/Utils/Logger/Logger.h>

#include "Notifications/NotificationRenderer.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace notifications {

using namespace avsCommon::utils::mediaPlayer;

/// String to identify log entries originating from this file.
static const std::string TAG("NotificationsRenderer");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * Emit a NotificationRenderer::State value to a stream.
 *
 * @param stream The stream to emit the value to.
 * @param state The state to emit.
 * @return The stream that was emitted to, allowing chained stream operators.
 */
std::ostream& operator<<(std::ostream& stream, const NotificationRenderer::State state) {
    switch (state) {
        case NotificationRenderer::State::IDLE:
            stream << "IDLE";
            return stream;
        case NotificationRenderer::State::RENDERING_PREFERRED:
            stream << "RENDERING_PREFERRED";
            return stream;
        case NotificationRenderer::State::RENDERING_DEFAULT:
            stream << "RENDERING_DEFAULT";
            return stream;
        case NotificationRenderer::State::CANCELLING:
            stream << "CANCELLING";
            return stream;
        case NotificationRenderer::State::NOTIFYING:
            stream << "NOTIFYING";
            return stream;
    }
    stream << "UNKNOWN: " << static_cast<int>(state);
    return stream;
}

std::shared_ptr<NotificationRenderer> NotificationRenderer::create(std::shared_ptr<MediaPlayerInterface> mediaPlayer) {
    ACSDK_DEBUG5(LX("create"));
    if (!mediaPlayer) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMediaPlayer"));
        return nullptr;
    }
    std::shared_ptr<NotificationRenderer> result(new NotificationRenderer(mediaPlayer));
    mediaPlayer->setObserver(result);
    return result;
}

void NotificationRenderer::addObserver(std::shared_ptr<NotificationRendererObserverInterface> observer) {
    ACSDK_DEBUG5(LX("addObserver"));
    std::lock_guard<std::mutex> lock(m_mutex);
    m_observers.insert(observer);
}

void NotificationRenderer::removeObserver(std::shared_ptr<NotificationRendererObserverInterface> observer) {
    ACSDK_DEBUG5(LX("removeObserver"));
    std::lock_guard<std::mutex> lock(m_mutex);
    m_observers.erase(observer);
}

bool NotificationRenderer::renderNotification(
    std::function<std::unique_ptr<std::istream>()> audioFactory,
    const std::string& url) {
    ACSDK_DEBUG5(LX("renderNotification"));

    if (!audioFactory) {
        ACSDK_ERROR(LX("renderNotificationFailed").d("reason", "nullAudioFactory"));
        return false;
    }

    // There is a small window between the return on onNotificationRenderingFinished() and the transition
    // back to the IDLE state.  If a call to renderNotification is made in that window it will needlessly
    // fail.  We check for that case here and wait if necessary.
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_wakeTrigger.wait(lock, [this]() { return m_state != State::NOTIFYING; });
    }

    // First attempt to render the preferred audio asset.
    if (!setState(State::RENDERING_PREFERRED)) {
        ACSDK_ERROR(LX("renderNotificationFailed").d("reason", "setState(RENDERING_PREFERRED) failed"));
        return false;
    }

    m_audioFactory = audioFactory;
    m_sourceId = m_mediaPlayer->setSource(url);
    if (m_sourceId != MediaPlayerInterface::ERROR && m_mediaPlayer->play(m_sourceId)) {
        ACSDK_DEBUG5(LX("renderNotificationPreferredSuccess").d("sourceId", m_sourceId));
        return true;
    }
    ACSDK_ERROR(LX("playPreferredFailed"));

    // If unable to start rendering the preferred asset, render the default asset, instead.
    if (setState(State::RENDERING_DEFAULT)) {
        m_sourceId = m_mediaPlayer->setSource(m_audioFactory(), false);
        if (m_sourceId != MediaPlayerInterface::ERROR && m_mediaPlayer->play(m_sourceId)) {
            ACSDK_DEBUG5(LX("renderNotificationDefaultSuccess").d("sourceId", m_sourceId));
            return true;
        }
        ACSDK_ERROR(LX("playDefaultFailed"));
    }

    m_sourceId = MediaPlayerInterface::ERROR;
    m_audioFactory = nullptr;
    setState(State::IDLE);
    return false;
}

bool NotificationRenderer::cancelNotificationRendering() {
    ACSDK_DEBUG5(LX("cancelNotificationRendering"));
    if (!setState(State::CANCELLING)) {
        ACSDK_DEBUG5(LX("cancelNotificationRenderingFailed").d("reason", "setState(CANCELLING) failed"));
        return false;
    }
    if (!m_mediaPlayer->stop(m_sourceId)) {
        ACSDK_ERROR(LX("cancelNotificationRenderingFailed").d("reason", "stopFailed"));
        // The state has already transitioned to cancelling, so there is not much to do here
        // but wait for rendering the audio to complete.  Ignore the error and return true.
    }
    return true;
}

void NotificationRenderer::onPlaybackStarted(SourceId sourceId) {
    ACSDK_DEBUG5(LX("onPlaybackStarted").d("sourceId", sourceId));
    if (sourceId != m_sourceId) {
        ACSDK_ERROR(LX("onPlaybackStartedFailed").d("reason", "unexpectedSourceId").d("expected", m_sourceId));
        return;
    }
    if (State::IDLE == m_state || State::NOTIFYING == m_state) {
        ACSDK_ERROR(LX("onPlaybackStartedFailed").d("reason", "unexpectedState").d("state", m_state));
    }
}

void NotificationRenderer::onPlaybackStopped(SourceId sourceId) {
    ACSDK_DEBUG5(LX("onPlaybackStopped").d("sourceId", sourceId));
    if (sourceId != m_sourceId) {
        ACSDK_ERROR(LX("onPlaybackStoppedFailed").d("reason", "unexpectedSourceId").d("expected", m_sourceId));
        return;
    }
    onRenderingFinished(sourceId);
}

void NotificationRenderer::onPlaybackFinished(SourceId sourceId) {
    ACSDK_DEBUG5(LX("onPlaybackFinished").d("sourceId", sourceId));
    if (sourceId != m_sourceId) {
        ACSDK_ERROR(LX("onPlaybackFinishedFailed").d("reason", "unexpectedSourceId").d("expected", m_sourceId));
        return;
    }
    onRenderingFinished(sourceId);
}

void NotificationRenderer::onPlaybackError(SourceId sourceId, const ErrorType& type, std::string error) {
    ACSDK_DEBUG5(LX("onPlaybackError").d("sourceId", sourceId).d("type", type).d("error", error));

    if (sourceId != m_sourceId) {
        ACSDK_ERROR(LX("onPlaybackErrorFailed").d("reason", "unexpectedSourceId").d("expected", m_sourceId));
        return;
    }

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        switch (m_state) {
            case State::IDLE:
            case State::NOTIFYING:
                ACSDK_ERROR(LX("onPlaybackErrorFailed").d("reason", "unexpectedState"));
                return;
            case State::RENDERING_DEFAULT:
            case State::CANCELLING:
                lock.unlock();
                onRenderingFinished(sourceId);
                return;
            case State::RENDERING_PREFERRED:
                // Rendering the preferred audio asset failed. Render the default audio asset, instead.
                if (!setStateLocked(State::RENDERING_DEFAULT)) {
                    return;
                }
                break;
        }
    }

    // Calling m_mediaPlayer->setSource() or m_mediaPlayer->play() will deadlock if called from a
    // MediaPlayerObserverInterface callback.  We need a separate thread to kick off rendering the default audio.
    m_renderFallbackFuture = std::async(std::launch::async, [this, sourceId]() {
        m_sourceId = m_mediaPlayer->setSource(m_audioFactory(), false);
        if (m_sourceId != MediaPlayerInterface::ERROR && m_mediaPlayer->play(m_sourceId)) {
            return;
        }
        ACSDK_ERROR(LX("playDefaultAudioFailed"));
        onRenderingFinished(sourceId);
    });
}

NotificationRenderer::NotificationRenderer(std::shared_ptr<MediaPlayerInterface> mediaPlayer) :
        m_mediaPlayer{mediaPlayer},
        m_state{State::IDLE},
        m_sourceId{MediaPlayerInterface::ERROR} {
}

void NotificationRenderer::onRenderingFinished(SourceId sourceId) {
    std::unordered_set<std::shared_ptr<NotificationRendererObserverInterface>> localObservers;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        localObservers = m_observers;
        if (!setStateLocked(State::NOTIFYING)) {
            return;
        }
    }
    for (auto observer : localObservers) {
        observer->onNotificationRenderingFinished();
    }
    setState(State::IDLE);
}

bool NotificationRenderer::setState(State newState) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return setStateLocked(newState);
}

bool NotificationRenderer::setStateLocked(State newState) {
    bool result = true;
    if (newState == m_state) {
        result = false;
    } else {
        switch (newState) {
            case State::IDLE:
                result = (State::RENDERING_PREFERRED != m_state) && (State::RENDERING_DEFAULT != m_state);
                break;
            case State::RENDERING_PREFERRED:
                result = State::IDLE == m_state;
                break;
            case State::RENDERING_DEFAULT:
                result = State::RENDERING_PREFERRED == m_state;
                break;
            case State::CANCELLING:
                result = (State::RENDERING_DEFAULT == m_state) || (State::RENDERING_PREFERRED == m_state);
                break;
            case State::NOTIFYING:
                result = m_state != State::IDLE;
                break;
        }
    }
    if (result) {
        ACSDK_DEBUG5(LX("setStateSuccess").d("state", m_state).d("newState", newState));
        m_state = newState;
        m_wakeTrigger.notify_all();
    } else {
        ACSDK_ERROR(LX("setStateFailed").d("state", m_state).d("newState", newState));
    }
    return result;
}

}  // namespace notifications
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
