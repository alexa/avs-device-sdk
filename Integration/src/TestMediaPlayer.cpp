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

#include <atomic>

#include "Integration/TestMediaPlayer.h"

namespace alexaClientSDK {
namespace integration {
namespace test {

/// String to identify log entries originating from this file.
static const std::string TAG("TestMediaPlayer");

/// Default media player state for reporting all playback eventing
static const avsCommon::utils::mediaPlayer::MediaPlayerState DEFAULT_MEDIA_PLAYER_STATE = {
    std::chrono::milliseconds(0)};

/// A counter used to increment the source id when a new source is set.
static std::atomic<avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId> g_sourceId{0};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

TestMediaPlayer::~TestMediaPlayer() {
}

avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId TestMediaPlayer::setSource(
    std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader,
    const avsCommon::utils::AudioFormat* audioFormat,
    const avsCommon::utils::mediaPlayer::SourceConfig& config) {
    m_attachmentReader = std::move(attachmentReader);
    return ++g_sourceId;
}

avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId TestMediaPlayer::setSource(
    std::shared_ptr<std::istream> stream,
    bool repeat,
    const avsCommon::utils::mediaPlayer::SourceConfig& config,
    avsCommon::utils::MediaType format) {
    m_istream = stream;
    return ++g_sourceId;
}

avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId TestMediaPlayer::setSource(
    const std::string& url,
    std::chrono::milliseconds offset,
    const avsCommon::utils::mediaPlayer::SourceConfig& config,
    bool repeat,
    const avsCommon::utils::mediaPlayer::PlaybackContext& playbackContext) {
    return ++g_sourceId;
}

bool TestMediaPlayer::play(avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId id) {
    if (m_observers.empty()) {
        return false;
    }
    for (const auto& observer : m_observers) {
        observer->onPlaybackStarted(id, DEFAULT_MEDIA_PLAYER_STATE);
    }
    m_playbackFinished = true;
    m_timer = std::unique_ptr<avsCommon::utils::timing::Timer>(new avsCommon::utils::timing::Timer);
    // Wait 600 milliseconds before sending onPlaybackFinished.
    m_timer->start(std::chrono::milliseconds(600), [this, id] {
        for (const auto& observer : m_observers) {
            if (m_playbackFinished) {
                observer->onPlaybackFinished(id, DEFAULT_MEDIA_PLAYER_STATE);
            }
        }
        m_playbackFinished = false;
    });
    return true;
}

bool TestMediaPlayer::stop(avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId id) {
    if (!m_playbackFinished || m_observers.empty()) {
        return false;
    }
    for (const auto& observer : m_observers) {
        if (m_playbackFinished) {
            observer->onPlaybackStopped(id, DEFAULT_MEDIA_PLAYER_STATE);
        }
    }
    m_playbackFinished = false;
    return true;
}

// TODO Add implementation
bool TestMediaPlayer::pause(avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId id) {
    return true;
}

// TODO Add implementation
bool TestMediaPlayer::resume(avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId id) {
    return true;
}

std::chrono::milliseconds TestMediaPlayer::getOffset(avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId id) {
    return std::chrono::milliseconds::zero();
}

void TestMediaPlayer::addObserver(
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface> playerObserver) {
    m_observers.insert(playerObserver);
}

void TestMediaPlayer::removeObserver(
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface> playerObserver) {
    m_observers.erase(playerObserver);
}

uint64_t TestMediaPlayer::getNumBytesBuffered() {
    return 0;
}

avsCommon::utils::Optional<avsCommon::utils::mediaPlayer::MediaPlayerState> TestMediaPlayer::getMediaPlayerState(
    avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId id) {
    auto offset = getOffset(id);
    auto state = avsCommon::utils::mediaPlayer::MediaPlayerState();
    state.offset = offset;
    return avsCommon::utils::Optional<avsCommon::utils::mediaPlayer::MediaPlayerState>(state);
}

}  // namespace test
}  // namespace integration
}  // namespace alexaClientSDK
