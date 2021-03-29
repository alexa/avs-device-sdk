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

#include "AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h"
#include "AVSCommon/Utils/MediaPlayer/MockMediaPlayer.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace mediaPlayer {
namespace test {

using namespace testing;

const static std::chrono::milliseconds WAIT_LOOP_INTERVAL{1};
std::vector<std::shared_ptr<MockMediaPlayer::Source>> MockMediaPlayer::m_sources;
std::atomic<MediaPlayerInterface::SourceId> MockMediaPlayer::m_currentSourceId(MediaPlayerInterface::ERROR);
MediaPlayerInterface::SourceId MockMediaPlayer::m_previousSourceId = MediaPlayerInterface::ERROR;
bool MockMediaPlayer::m_isConcurrentEnabled = false;
std::mutex MockMediaPlayer::m_globalMutex;

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
    ON_CALL(*result.get(), stop(_, _)).WillByDefault(Invoke(result.get(), &MockMediaPlayer::mockStop2));
    ON_CALL(*result.get(), pause(_)).WillByDefault(Invoke(result.get(), &MockMediaPlayer::mockPause));
    ON_CALL(*result.get(), resume(_)).WillByDefault(Invoke(result.get(), &MockMediaPlayer::mockResume));
    ON_CALL(*result.get(), seekTo(_, _, _)).WillByDefault(Invoke(result.get(), &MockMediaPlayer::mockSeek));
    ON_CALL(*result.get(), getOffset(_)).WillByDefault(Invoke(result.get(), &MockMediaPlayer::mockGetOffset));
    ON_CALL(*result.get(), getMediaPlayerState(_)).WillByDefault(Invoke(result.get(), &MockMediaPlayer::mockGetState));
    return result;
}

void MockMediaPlayer::enableConcurrentMediaPlayers() {
    m_isConcurrentEnabled = true;
}

MockMediaPlayer::MockMediaPlayer() : RequiresShutdown{"MockMediaPlayer"} {
    if (m_sources.empty()) {
        // Create a 'source' for sourceId = 0
        mockSetSource();
    }
}

MediaPlayerInterface::SourceId MockMediaPlayer::setSource(
    std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader,
    const avsCommon::utils::AudioFormat* audioFormat,
    const SourceConfig&) {
    return attachmentSetSource(attachmentReader, audioFormat);
}

MediaPlayerInterface::SourceId MockMediaPlayer::setSource(
    std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader,
    std::chrono::milliseconds offsetAdjustment,
    const avsCommon::utils::AudioFormat* audioFormat,
    const SourceConfig&) {
    return attachmentSetSource(attachmentReader, audioFormat);
}

MediaPlayerInterface::SourceId MockMediaPlayer::setSource(
    const std::string& url,
    std::chrono::milliseconds,
    const SourceConfig&,
    bool,
    const PlaybackContext&) {
    return urlSetSource(url);
}

MediaPlayerInterface::SourceId MockMediaPlayer::setSource(
    std::shared_ptr<std::istream> stream,
    bool repeat,
    const SourceConfig&,
    avsCommon::utils::MediaType) {
    return streamSetSource(stream, repeat);
}

void MockMediaPlayer::addObserver(
    std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::test::MockMediaPlayer::observer> playerObserver) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_playerObservers.insert(playerObserver);
}

void MockMediaPlayer::removeObserver(
    std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::test::MockMediaPlayer::observer> removeObserver) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_playerObservers.erase(removeObserver);
}

std::unordered_set<std::shared_ptr<MediaPlayerObserverInterface>> MockMediaPlayer::getObservers() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_playerObservers;
}

void MockMediaPlayer::doShutdown() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_playerObservers.clear();
    }
    {
        std::lock_guard<std::mutex> lock(m_globalMutex);
        m_sources.clear();
        m_isConcurrentEnabled = false;
        m_currentSourceId = MediaPlayerInterface::ERROR;
        m_previousSourceId = MediaPlayerInterface::ERROR;
    }
}

MediaPlayerInterface::SourceId MockMediaPlayer::mockSetSource() {
    std::lock_guard<std::mutex> lock(m_globalMutex);
    SourceId result = m_sources.size();
    m_sources.emplace_back(std::make_shared<Source>(this, result));

    if (!m_isConcurrentEnabled) {
        m_previousSourceId = m_currentSourceId;
        m_currentSourceId = result;
    }

    return result;
}

bool MockMediaPlayer::mockPlay(SourceId sourceId) {
    {
        std::lock_guard<std::mutex> lock(m_globalMutex);
        if (m_isConcurrentEnabled && isValidSourceId(sourceId)) {
            m_previousSourceId = m_currentSourceId;
            m_currentSourceId = sourceId;
        }
    }
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
    return mockStop2(sourceId, std::chrono::minutes::zero());
}

bool MockMediaPlayer::mockStop2(SourceId sourceId, std::chrono::seconds closePipelineTime) {
    auto source = getCurrentSource(sourceId);
    if (!source) {
        // The AudioPlayer may have set a new MediaPlayer source before stopping
        // the previous Player, so we need to allow a Stop on the player previous to the
        // current player as well.
        source = getPreviousSource(sourceId);
        if (!source) {
            return false;
        }
    }

    source->stopwatch.stop();
    if (closePipelineTime != std::chrono::seconds::zero()) {
        source->stopwatch.reset();
    }
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

bool MockMediaPlayer::mockSeek(SourceId sourceId, std::chrono::milliseconds location, bool fromStart) {
    auto source = getCurrentSource(sourceId);
    if (!source) {
        return false;
    }

    source->stopwatch.stop();
    source->stopwatch.reset();
    source->stopwatch.start();

    mockSetOffset(sourceId, location);
    source->seekComplete.trigger(MediaPlayerState(location));
    return true;
}

bool MockMediaPlayer::mockSetOffset(SourceId sourceId, std::chrono::milliseconds offset) {
    auto source = getCurrentSource(sourceId);
    if (!source) {
        return false;
    }
    std::lock_guard<std::mutex> lock(m_globalMutex);
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

Optional<avsCommon::utils::mediaPlayer::MediaPlayerState> MockMediaPlayer::mockGetState(SourceId sourceId) {
    auto offset = mockGetOffset(sourceId);
    if (offset == MEDIA_PLAYER_INVALID_OFFSET) {
        return Optional<MediaPlayerState>();
    }
    auto state = MediaPlayerState();
    state.offset = offset;
    state.duration = std::chrono::milliseconds(10000000);
    return Optional<MediaPlayerState>(state);
}

void MockMediaPlayer::resetWaitTimer() {
    auto source = getCurrentSource(getCurrentSourceId());
    if (!source) {
        return;
    }
    source->stopwatch.reset();
    source->started.resetStateReached();
}

bool MockMediaPlayer::waitUntilNextSetSource(const std::chrono::milliseconds timeout) {
    auto originalSourceId = getCurrentSourceId();
    if (m_isConcurrentEnabled) {
        std::lock_guard<std::mutex> lock(m_globalMutex);
        originalSourceId = m_sources.size();
    }

    auto timeLimit = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < timeLimit) {
        if (m_isConcurrentEnabled) {
            std::lock_guard<std::mutex> lock(m_globalMutex);
            if (m_sources.size() != originalSourceId) {
                return true;
            }
        } else {
            if (getCurrentSourceId() != originalSourceId) {
                return true;
            }
        }
        std::this_thread::sleep_for(WAIT_LOOP_INTERVAL);
    }
    return false;
}

bool MockMediaPlayer::waitUntilPlaybackStarted(std::chrono::milliseconds timeout) {
    return waitUntilPlaybackStarted(getCurrentSourceId(), timeout);
}

bool MockMediaPlayer::waitUntilPlaybackStarted(SourceId id, std::chrono::milliseconds timeout) {
    return m_sources[id]->started.wait(timeout);
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

bool MockMediaPlayer::waitUntilSeeked(std::chrono::milliseconds timeout) {
    return m_sources[getCurrentSourceId()]->seekComplete.wait(timeout);
}

MediaPlayerInterface::SourceId MockMediaPlayer::getCurrentSourceId() {
    return m_currentSourceId;
}

MediaPlayerInterface::SourceId MockMediaPlayer::getSourceId() {
    std::unique_lock<std::mutex> lock(m_globalMutex);
    for (auto iter = m_sources.rbegin(); iter != m_sources.rend(); ++iter) {
        auto& source = *iter;
        if (source->mockMediaPlayer == this) {
            return source->sourceId;
        }
    }

    return MediaPlayerInterface::ERROR;
}

MediaPlayerInterface::SourceId MockMediaPlayer::getLatestSourceId() {
    std::unique_lock<std::mutex> lock(m_globalMutex);
    return m_sources.size() == 0 ? MediaPlayerInterface::ERROR : m_sources.size() - 1;
}

MockMediaPlayer::SourceState::SourceState(
    Source* source,
    const std::string& name,
    std::function<void(std::shared_ptr<observer>, SourceId, const MediaPlayerState&)> notifyFunction) :
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

void MockMediaPlayer::SourceState::trigger(const MediaPlayerState state) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_stateReached) {
        return;
    }
    auto observers = m_source->mockMediaPlayer->getObservers();
    m_thread = std::thread(&MockMediaPlayer::SourceState::notify, this, observers, DEFAULT_TIME, state);
    m_stateReached = true;
    m_wake.notify_all();
}

void MockMediaPlayer::SourceState::notify(
    const std::unordered_set<std::shared_ptr<MediaPlayerObserverInterface>>& observers,
    const std::chrono::milliseconds timeout,
    const MediaPlayerState state) {
    std::unique_lock<std::mutex> lock(m_mutex);
    auto timedOut = !m_wake.wait_for(lock, timeout, [this]() { return (m_stateReached || m_shutdown); });
    lock.unlock();
    const MediaPlayerState timeoutState = {timeout};
    for (const auto& observer : observers) {
        if (timedOut) {
            observer->onPlaybackError(
                m_source->sourceId,
                ErrorType::MEDIA_ERROR_UNKNOWN,
                m_name + ": wait to notify timed out",
                timeoutState);

            return;
        }
        m_notifyFunction(observer, m_source->sourceId, state);
    }
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
    MediaPlayerInterface::SourceId sourceId,
    const MediaPlayerState& state) {
    observer->onPlaybackStarted(sourceId, state);
}

static void notifyPlaybackPaused(
    std::shared_ptr<MediaPlayerObserverInterface> observer,
    MediaPlayerInterface::SourceId sourceId,
    const MediaPlayerState& state) {
    observer->onPlaybackPaused(sourceId, state);
}

static void notifyPlaybackResumed(
    std::shared_ptr<MediaPlayerObserverInterface> observer,
    MediaPlayerInterface::SourceId sourceId,
    const MediaPlayerState& state) {
    observer->onPlaybackResumed(sourceId, state);
}

static void notifyPlaybackStopped(
    std::shared_ptr<MediaPlayerObserverInterface> observer,
    MediaPlayerInterface::SourceId sourceId,
    const MediaPlayerState& state) {
    observer->onPlaybackStopped(sourceId, state);
}

static void notifyPlaybackFinished(
    std::shared_ptr<MediaPlayerObserverInterface> observer,
    MediaPlayerInterface::SourceId sourceId,
    const MediaPlayerState& state) {
    observer->onPlaybackFinished(sourceId, state);
}

static void notifyPlaybackError(
    std::shared_ptr<MediaPlayerObserverInterface> observer,
    MediaPlayerInterface::SourceId sourceId,
    const MediaPlayerState& state) {
    observer->onPlaybackError(sourceId, ErrorType::MEDIA_ERROR_INTERNAL_SERVER_ERROR, "mock error", state);
}

static void notifySeekComplete(
    std::shared_ptr<MediaPlayerObserverInterface> observer,
    MediaPlayerInterface::SourceId sourceId,
    const MediaPlayerState& state) {
    observer->onSeeked(sourceId, state, state);
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
        seekComplete{this, "seekComplete", notifySeekComplete},
        error{this, "error", notifyPlaybackError} {
}

std::shared_ptr<MockMediaPlayer::Source> MockMediaPlayer::getCurrentSource(SourceId sourceId) {
    std::unique_lock<std::mutex> lock(m_globalMutex);
    if (!isValidSourceId(sourceId) || sourceId != m_currentSourceId) {
        return nullptr;
    }
    return m_sources[static_cast<size_t>(sourceId)];
}

std::shared_ptr<MockMediaPlayer::Source> MockMediaPlayer::getPreviousSource(SourceId sourceId) {
    std::unique_lock<std::mutex> lock(m_globalMutex);
    size_t sourceIndex = static_cast<size_t>(sourceId);
    if (!isValidSourceId(sourceId) || sourceId != m_previousSourceId) {
        return nullptr;
    }
    return m_sources[sourceIndex];
}

/// called in locked state
bool MockMediaPlayer::isValidSourceId(SourceId sourceId) {
    size_t sourceIndex = static_cast<size_t>(sourceId);
    return ERROR != sourceId && sourceIndex > 0 && sourceIndex < m_sources.size();
}

}  // namespace test
}  // namespace mediaPlayer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
