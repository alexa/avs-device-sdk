/*
 * MockMediaPlayer.cpp
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

#include "AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h"
#include "AVSCommon/Utils/MediaPlayer/MockMediaPlayer.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace mediaPlayer {
namespace test {

using namespace testing;

std::shared_ptr<NiceMock<MockMediaPlayer>> MockMediaPlayer::create() {
    auto result = std::make_shared<NiceMock<MockMediaPlayer>>();

    ON_CALL(*result.get(), attachmentSetSource(_))
        .WillByDefault(InvokeWithoutArgs(result.get(), &MockMediaPlayer::mockSetSource));
    ON_CALL(*result.get(), urlSetSource(_))
        .WillByDefault(InvokeWithoutArgs(result.get(), &MockMediaPlayer::mockSetSource));
    ON_CALL(*result.get(), streamSetSource(_, _))
        .WillByDefault(InvokeWithoutArgs(result.get(), &MockMediaPlayer::mockSetSource));
    ON_CALL(*result.get(), play(_)).WillByDefault(Invoke(result.get(), &MockMediaPlayer::mockPlay));
    ON_CALL(*result.get(), stop(_)).WillByDefault(Invoke(result.get(), &MockMediaPlayer::mockStop));
    ON_CALL(*result.get(), pause(_)).WillByDefault(Invoke(result.get(), &MockMediaPlayer::mockPause));
    ON_CALL(*result.get(), resume(_)).WillByDefault(Invoke(result.get(), &MockMediaPlayer::mockResume));
    ON_CALL(*result.get(), getOffset(_)).WillByDefault(Invoke(result.get(), &MockMediaPlayer::mockGetOffset));
    return result;
}

MockMediaPlayer::MockMediaPlayer() :
        m_sourceId{ERROR},
        m_offset{MEDIA_PLAYER_INVALID_OFFSET},
        m_play{false},
        m_stop{false},
        m_pause{false},
        m_resume{false},
        m_shutdown{false},
        m_wakePlayPromise{},
        m_wakePlayFuture{m_wakePlayPromise.get_future()},
        m_wakeStopPromise{},
        m_wakeStopFuture{m_wakeStopPromise.get_future()},
        m_wakePausePromise{},
        m_wakePauseFuture{m_wakePausePromise.get_future()},
        m_playerObserver{nullptr} {
}

MockMediaPlayer::~MockMediaPlayer() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_shutdown = true;
    }
    m_wakeTriggerPlay.notify_all();
    m_wakeTriggerStop.notify_all();
    m_wakeTriggerPause.notify_all();
    m_wakeTriggerResume.notify_all();

    if (m_playThread.joinable()) {
        m_playThread.join();
    }
    if (m_playThread_2.joinable()) {
        m_playThread_2.join();
    }
    if (m_stopThread.joinable()) {
        m_stopThread.join();
    }
    if (m_pauseThread.joinable()) {
        m_pauseThread.join();
    }
    if (m_pauseThread_2.joinable()) {
        m_pauseThread_2.join();
    }
    if (m_resumeThread.joinable()) {
        m_resumeThread.join();
    }
}

void MockMediaPlayer::setObserver(
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface> playerObserver) {
    m_playerObserver = playerObserver;
}

MediaPlayerInterface::SourceId MockMediaPlayer::setSource(
    std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader) {
    return attachmentSetSource(attachmentReader);
}

MediaPlayerInterface::SourceId MockMediaPlayer::setSource(const std::string& url, std::chrono::milliseconds offset) {
    return urlSetSource(url);
}

MediaPlayerInterface::SourceId MockMediaPlayer::setSource(std::shared_ptr<std::istream> stream, bool repeat) {
    return streamSetSource(stream, repeat);
}

MediaPlayerInterface::SourceId MockMediaPlayer::mockSetSource() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return ++m_sourceId;
}

bool MockMediaPlayer::mockSetOffset(SourceId id, std::chrono::milliseconds offset) {
    std::lock_guard<std::mutex> lock(m_mutex);
    EXPECT_NE(id, static_cast<SourceId>(ERROR));
    EXPECT_EQ(id, m_sourceId);
    if (ERROR == id || id != m_sourceId) {
        return false;
    }
    m_offset = offset;
    return true;
}

bool MockMediaPlayer::mockPlay(SourceId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    EXPECT_NE(id, static_cast<SourceId>(ERROR));
    EXPECT_EQ(id, m_sourceId);
    if (ERROR == id || id != m_sourceId) {
        return false;
    }
    if (!m_play) {
        m_playThread = std::thread(&MockMediaPlayer::waitForPlay, this, DEFAULT_TIME);
    } else {
        m_wakePlayPromise = std::promise<void>();
        m_playThread_2 = std::thread(&MockMediaPlayer::waitForPlay, this, DEFAULT_TIME);
    }

    m_play = true;
    m_wakeTriggerPlay.notify_one();
    return true;
}

bool MockMediaPlayer::mockStop(SourceId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    EXPECT_NE(id, static_cast<SourceId>(ERROR));
    EXPECT_EQ(id, m_sourceId);
    if (ERROR == id || id != m_sourceId) {
        return false;
    }
    if (!m_stop) {
        m_stopThread = std::thread(&MockMediaPlayer::waitForStop, this, DEFAULT_TIME);
        m_stop = true;
        m_wakeTriggerStop.notify_one();
    }

    return true;
}

bool MockMediaPlayer::mockPause(SourceId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    EXPECT_NE(id, static_cast<SourceId>(ERROR));
    EXPECT_EQ(id, m_sourceId);
    if (ERROR == id || id != m_sourceId) {
        return false;
    }
    if (!m_pause) {
        m_pauseThread = std::thread(&MockMediaPlayer::waitForPause, this, DEFAULT_TIME);
    } else {
        m_wakePausePromise = std::promise<void>();
        m_pauseThread_2 = std::thread(&MockMediaPlayer::waitForPause, this, DEFAULT_TIME);
    }

    m_pause = true;
    m_wakeTriggerPause.notify_one();
    return true;
}

bool MockMediaPlayer::mockResume(SourceId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    EXPECT_NE(id, static_cast<SourceId>(ERROR));
    EXPECT_EQ(id, m_sourceId);
    if (ERROR == id || id != m_sourceId) {
        return false;
    }
    m_resumeThread = std::thread(&MockMediaPlayer::waitForResume, this, DEFAULT_TIME);
    m_resume = true;
    m_wakeTriggerResume.notify_one();
    return true;
}

std::chrono::milliseconds MockMediaPlayer::mockGetOffset(SourceId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    EXPECT_NE(id, static_cast<SourceId>(ERROR));
    EXPECT_EQ(id, m_sourceId);
    if (ERROR == id || id != m_sourceId) {
        return MEDIA_PLAYER_INVALID_OFFSET;
    }
    return m_offset;
}

bool MockMediaPlayer::waitForPlay(const std::chrono::milliseconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakeTriggerPlay.wait_for(lock, duration, [this]() { return (m_play || m_shutdown); })) {
        if (m_playerObserver) {
            m_playerObserver->onPlaybackError(m_sourceId, ErrorType::MEDIA_ERROR_UNKNOWN, "waitForPlay timed out");
        }
        return false;
    }
    m_wakePlayPromise.set_value();
    if (m_playerObserver) {
        m_playerObserver->onPlaybackStarted(m_sourceId);
    }
    return true;
}

bool MockMediaPlayer::waitForStop(const std::chrono::milliseconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakeTriggerStop.wait_for(lock, duration, [this]() { return (m_stop || m_shutdown); })) {
        if (m_playerObserver) {
            m_playerObserver->onPlaybackError(m_sourceId, ErrorType::MEDIA_ERROR_UNKNOWN, "waitForStop timed out");
        }
        return false;
    }
    m_wakeStopPromise.set_value();
    if (m_playerObserver) {
        m_playerObserver->onPlaybackStopped(m_sourceId);
    }
    return true;
}

bool MockMediaPlayer::waitForPause(const std::chrono::milliseconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakeTriggerPause.wait_for(lock, duration, [this]() { return (m_pause || m_shutdown); })) {
        if (m_playerObserver) {
            m_playerObserver->onPlaybackError(m_sourceId, ErrorType::MEDIA_ERROR_UNKNOWN, "waitForPause timed out");
        }
        return false;
    }
    m_wakePausePromise.set_value();
    if (m_playerObserver) {
        m_playerObserver->onPlaybackPaused(m_sourceId);
    }
    return true;
}

bool MockMediaPlayer::waitForResume(const std::chrono::milliseconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakeTriggerResume.wait_for(lock, duration, [this]() { return (m_resume || m_shutdown); })) {
        if (m_playerObserver) {
            m_playerObserver->onPlaybackError(m_sourceId, ErrorType::MEDIA_ERROR_UNKNOWN, "waitForResume timed out");
        }
        return false;
    }
    m_wakeResumePromise.set_value();
    if (m_playerObserver) {
        m_playerObserver->onPlaybackResumed(m_sourceId);
    }
    return true;
}

bool MockMediaPlayer::waitUntilPlaybackStarted(std::chrono::milliseconds timeout) {
    return m_wakePlayFuture.wait_for(timeout) == std::future_status::ready;
}

bool MockMediaPlayer::waitUntilPlaybackFinished(std::chrono::milliseconds timeout) {
    return m_wakeStopFuture.wait_for(timeout) == std::future_status::ready;
}

bool MockMediaPlayer::waitUntilPlaybackPaused(std::chrono::milliseconds timeout) {
    return m_wakePauseFuture.wait_for(timeout) == std::future_status::ready;
}

bool MockMediaPlayer::waitUntilPlaybackResumed(std::chrono::milliseconds timeout) {
    return m_wakeResumeFuture.wait_for(timeout) == std::future_status::ready;
}

void MockMediaPlayer::doShutdown() {
}

}  // namespace test
}  // namespace mediaPlayer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
