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

#include "AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h"
#include "AVSCommon/Utils/MediaPlayer/MockMediaPlayer.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace mediaPlayer {
namespace test {

using namespace testing;

const static std::chrono::milliseconds WAIT_LOOP_INTERVAL{1};

std::shared_ptr<NiceMock<MockMediaPlayer>> MockMediaPlayer::create() {
    auto result = std::make_shared<NiceMock<MockMediaPlayer>>();

    ON_CALL(*result.get(), attachmentSetSource(_, _))
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

MockMediaPlayer::MockMediaPlayer() : RequiresShutdown{"MockMediaPlayer"}, m_playerObserver{nullptr} {
    // Create a 'source' for sourceId = 0
    mockSetSource();
}

MediaPlayerInterface::SourceId MockMediaPlayer::setSource(
    std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader,
    const avsCommon::utils::AudioFormat* audioFormat) {
    return attachmentSetSource(attachmentReader, audioFormat);
}

MediaPlayerInterface::SourceId MockMediaPlayer::setSource(
    const std::string& url,
    std::chrono::milliseconds offset,
    bool repeat) {
    return urlSetSource(url);
}

MediaPlayerInterface::SourceId MockMediaPlayer::setSource(std::shared_ptr<std::istream> stream, bool repeat) {
    return streamSetSource(stream, repeat);
}

void MockMediaPlayer::setObserver(std::shared_ptr<MediaPlayerObserverInterface> playerObserver) {
    m_playerObserver = playerObserver;
}

void MockMediaPlayer::doShutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_playerObserver.reset();
    m_sources.clear();
}

MediaPlayerInterface::SourceId MockMediaPlayer::mockSetSource() {
    std::lock_guard<std::mutex> lock(m_mutex);
    SourceId result = m_sources.size();
    m_sources.emplace_back(std::make_shared<Source>(this, result));
    return result;
}

bool MockMediaPlayer::mockPlay(SourceId sourceId) {
    auto source = getCurrentSource(sourceId);
    if (!source) {
        return false;
    }
    EXPECT_TRUE(source->stopwatch.start());
    source->started.trigger();
    return true;
}

bool MockMediaPlayer::mockPause(SourceId sourceId) {
    auto source = getCurrentSource(sourceId);
    if (!source) {
        return false;
    }
    // Ideally we would EXPECT_TRUE on pause(), however ACSDK-734 doesn't guarantee that will be okay.
    source->stopwatch.pause();
    source->resumed.resetStateReached();
    source->paused.trigger();
    return true;
}

bool MockMediaPlayer::mockResume(SourceId sourceId) {
    auto source = getCurrentSource(sourceId);
    if (!source) {
        return false;
    }
    EXPECT_TRUE(source->stopwatch.resume());
    source->paused.resetStateReached();
    source->resumed.trigger();
    return true;
}

bool MockMediaPlayer::mockStop(SourceId sourceId) {
    auto source = getCurrentSource(sourceId);
    if (!source) {
        return false;
    }

    source->stopwatch.stop();
    source->stopped.trigger();
    return true;
}

bool MockMediaPlayer::mockFinished(SourceId sourceId) {
    auto source = getCurrentSource(sourceId);
    if (!source) {
        return false;
    }
    source->stopwatch.stop();
    source->finished.trigger();
    return true;
}

bool MockMediaPlayer::mockError(SourceId sourceId) {
    auto source = getCurrentSource(sourceId);
    if (!source) {
        return false;
    }
    source->stopwatch.stop();
    source->error.trigger();
    return true;
}

bool MockMediaPlayer::mockSetOffset(SourceId sourceId, std::chrono::milliseconds offset) {
    auto source = getCurrentSource(sourceId);
    if (!source) {
        return false;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    source->offset = offset;
    return true;
}

std::chrono::milliseconds MockMediaPlayer::mockGetOffset(SourceId sourceId) {
    auto source = getCurrentSource(sourceId);
    if (!source) {
        return MEDIA_PLAYER_INVALID_OFFSET;
    }
    return source->stopwatch.getElapsed() + source->offset;
}

bool MockMediaPlayer::waitUntilNextSetSource(const std::chrono::milliseconds timeout) {
    auto originalSourceId = getCurrentSourceId();
    auto timeLimit = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < timeLimit) {
        if (getCurrentSourceId() != originalSourceId) {
            return true;
        }
        std::this_thread::sleep_for(WAIT_LOOP_INTERVAL);
    }
    return false;
}

bool MockMediaPlayer::waitUntilPlaybackStarted(std::chrono::milliseconds timeout) {
    return m_sources[getCurrentSourceId()]->started.wait(timeout);
}

bool MockMediaPlayer::waitUntilPlaybackPaused(std::chrono::milliseconds timeout) {
    return m_sources[getCurrentSourceId()]->paused.wait(timeout);
}

bool MockMediaPlayer::waitUntilPlaybackResumed(std::chrono::milliseconds timeout) {
    return m_sources[getCurrentSourceId()]->resumed.wait(timeout);
}

bool MockMediaPlayer::waitUntilPlaybackStopped(std::chrono::milliseconds timeout) {
    return m_sources[getCurrentSourceId()]->stopped.wait(timeout);
}

bool MockMediaPlayer::waitUntilPlaybackFinished(std::chrono::milliseconds timeout) {
    return m_sources[getCurrentSourceId()]->finished.wait(timeout);
}

bool MockMediaPlayer::waitUntilPlaybackError(std::chrono::milliseconds timeout) {
    return m_sources[getCurrentSourceId()]->error.wait(timeout);
}

MediaPlayerInterface::SourceId MockMediaPlayer::getCurrentSourceId() {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_sources.size() - 1;
}

MockMediaPlayer::SourceState::SourceState(
    Source* source,
    const std::string& name,
    std::function<void(std::shared_ptr<observer>, SourceId)> notifyFunction) :
        m_source{source},
        m_name{name},
        m_notifyFunction{notifyFunction},
        m_stateReached{false},
        m_shutdown{false} {
}

MockMediaPlayer::SourceState::~SourceState() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_shutdown = true;
    }
    m_wake.notify_all();
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void MockMediaPlayer::SourceState::trigger() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_stateReached) {
        return;
    }
    m_thread = std::thread(&MockMediaPlayer::SourceState::notify, this, DEFAULT_TIME);
    m_stateReached = true;
    m_wake.notify_all();
}

void MockMediaPlayer::SourceState::notify(const std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(m_mutex);
    auto observer = m_source->mockMediaPlayer->m_playerObserver;
    if (!m_wake.wait_for(lock, timeout, [this]() { return (m_stateReached || m_shutdown); })) {
        if (observer) {
            lock.unlock();
            observer->onPlaybackError(
                m_source->sourceId, ErrorType::MEDIA_ERROR_UNKNOWN, m_name + ": wait to notify timed out");
        }
        return;
    }
    if (observer) {
        m_notifyFunction(observer, m_source->sourceId);
    }
    return;
}

bool MockMediaPlayer::SourceState::wait(const std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wake.wait_for(lock, timeout, [this]() { return (m_stateReached || m_shutdown); })) {
        return false;
    }
    return m_stateReached;
}

void MockMediaPlayer::SourceState::resetStateReached() {
    if (m_thread.joinable()) {
        m_thread.join();
    }
    std::unique_lock<std::mutex> lock(m_mutex);
    m_stateReached = false;
}

static void notifyPlaybackStarted(
    std::shared_ptr<MediaPlayerObserverInterface> observer,
    MediaPlayerInterface::SourceId sourceId) {
    observer->onPlaybackStarted(sourceId);
}

static void notifyPlaybackPaused(
    std::shared_ptr<MediaPlayerObserverInterface> observer,
    MediaPlayerInterface::SourceId sourceId) {
    observer->onPlaybackPaused(sourceId);
}

static void notifyPlaybackResumed(
    std::shared_ptr<MediaPlayerObserverInterface> observer,
    MediaPlayerInterface::SourceId sourceId) {
    observer->onPlaybackResumed(sourceId);
}

static void notifyPlaybackStopped(
    std::shared_ptr<MediaPlayerObserverInterface> observer,
    MediaPlayerInterface::SourceId sourceId) {
    observer->onPlaybackStopped(sourceId);
}

static void notifyPlaybackFinished(
    std::shared_ptr<MediaPlayerObserverInterface> observer,
    MediaPlayerInterface::SourceId sourceId) {
    observer->onPlaybackFinished(sourceId);
}

static void notifyPlaybackError(
    std::shared_ptr<MediaPlayerObserverInterface> observer,
    MediaPlayerInterface::SourceId sourceId) {
    observer->onPlaybackError(sourceId, ErrorType::MEDIA_ERROR_INTERNAL_SERVER_ERROR, "mock error");
}

MockMediaPlayer::Source::Source(MockMediaPlayer* player, SourceId id) :
        mockMediaPlayer{player},
        sourceId{id},
        offset{MEDIA_PLAYER_INVALID_OFFSET},
        started{this, "started", notifyPlaybackStarted},
        paused{this, "paused", notifyPlaybackPaused},
        resumed{this, "resumed", notifyPlaybackResumed},
        stopped{this, "stopped", notifyPlaybackStopped},
        finished{this, "finished", notifyPlaybackFinished},
        error{this, "error", notifyPlaybackError} {
}

std::shared_ptr<MockMediaPlayer::Source> MockMediaPlayer::getCurrentSource(SourceId sourceId) {
    if (ERROR == sourceId || sourceId != getCurrentSourceId()) {
        return nullptr;
    }
    return m_sources[static_cast<size_t>(sourceId)];
}

}  // namespace test
}  // namespace mediaPlayer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
