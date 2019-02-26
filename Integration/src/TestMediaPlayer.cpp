/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
    const avsCommon::utils::AudioFormat* audioFormat) {
    m_attachmentReader = std::move(attachmentReader);
    return ++g_sourceId;
}

avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId TestMediaPlayer::setSource(
    std::shared_ptr<std::istream> stream,
    bool repeat) {
    m_istream = stream;
    return ++g_sourceId;
}

avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId TestMediaPlayer::setSource(
    const std::string& url,
    std::chrono::milliseconds offset,
    bool repeat) {
    return ++g_sourceId;
}

bool TestMediaPlayer::play(avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId id) {
    if (m_observer) {
        m_observer->onPlaybackStarted(id);
        m_playbackFinished = true;
        m_timer = std::unique_ptr<avsCommon::utils::timing::Timer>(new avsCommon::utils::timing::Timer);
        // Wait 600 milliseconds before sending onPlaybackFinished.
        m_timer->start(std::chrono::milliseconds(600), [this, id] {
            if (m_playbackFinished) {
                m_observer->onPlaybackFinished(id);
                m_playbackFinished = false;
            }
        });
        return true;
    } else {
        return false;
    }
}

bool TestMediaPlayer::stop(avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId id) {
    if (m_observer && m_playbackFinished) {
        m_observer->onPlaybackStopped(id);
        m_playbackFinished = false;
        return true;
    } else {
        return false;
    }
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

void TestMediaPlayer::setObserver(
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface> playerObserver) {
    m_observer = playerObserver;
}

uint64_t TestMediaPlayer::getNumBytesBuffered() {
    return 0;
}

}  // namespace test
}  // namespace integration
}  // namespace alexaClientSDK
