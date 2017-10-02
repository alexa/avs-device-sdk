/*
 * TestMediaPlayer.cpp
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

#include "Integration/TestMediaPlayer.h"

namespace alexaClientSDK {
namespace integration {
namespace test {

/// String to identify log entries originating from this file.
static const std::string TAG("TestMediaPlayer");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

TestMediaPlayer::~TestMediaPlayer() {
}

avsCommon::utils::mediaPlayer::MediaPlayerStatus TestMediaPlayer::setSource(
    std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader) {
    m_attachmentReader = std::move(attachmentReader);
    return avsCommon::utils::mediaPlayer::MediaPlayerStatus::SUCCESS;
}

avsCommon::utils::mediaPlayer::MediaPlayerStatus TestMediaPlayer::setSource(
    std::shared_ptr<std::istream> stream,
    bool repeat) {
    m_istream = stream;
    return avsCommon::utils::mediaPlayer::MediaPlayerStatus::PENDING;
}

avsCommon::utils::mediaPlayer::MediaPlayerStatus TestMediaPlayer::setSource(const std::string& url) {
    return avsCommon::utils::mediaPlayer::MediaPlayerStatus::SUCCESS;
}

avsCommon::utils::mediaPlayer::MediaPlayerStatus TestMediaPlayer::play() {
    if (m_observer) {
        m_observer->onPlaybackStarted();
        m_playbackFinished = true;
        m_timer = std::unique_ptr<avsCommon::utils::timing::Timer>(new avsCommon::utils::timing::Timer);
        // Wait 600 milliseconds before sending onPlaybackFinished.
        m_timer->start(std::chrono::milliseconds(600), [this] {
            if (m_playbackFinished) {
                m_observer->onPlaybackFinished();
                m_playbackFinished = false;
            }
        });
        return avsCommon::utils::mediaPlayer::MediaPlayerStatus::SUCCESS;
    } else {
        return avsCommon::utils::mediaPlayer::MediaPlayerStatus::FAILURE;
    }
}

avsCommon::utils::mediaPlayer::MediaPlayerStatus TestMediaPlayer::stop() {
    if (m_observer && m_playbackFinished) {
        m_observer->onPlaybackFinished();
        m_playbackFinished = false;
        return avsCommon::utils::mediaPlayer::MediaPlayerStatus::SUCCESS;
    } else {
        return avsCommon::utils::mediaPlayer::MediaPlayerStatus::FAILURE;
    }
}

// TODO Add implementation
avsCommon::utils::mediaPlayer::MediaPlayerStatus TestMediaPlayer::pause() {
    return avsCommon::utils::mediaPlayer::MediaPlayerStatus::SUCCESS;
}

// TODO Add implementation
avsCommon::utils::mediaPlayer::MediaPlayerStatus TestMediaPlayer::resume() {
    return avsCommon::utils::mediaPlayer::MediaPlayerStatus::SUCCESS;
}

std::chrono::milliseconds TestMediaPlayer::getOffset() {
    return std::chrono::milliseconds::zero();
}

void TestMediaPlayer::setObserver(
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface> playerObserver) {
    m_observer = playerObserver;
}
}  // namespace test
}  // namespace integration
}  // namespace alexaClientSDK
