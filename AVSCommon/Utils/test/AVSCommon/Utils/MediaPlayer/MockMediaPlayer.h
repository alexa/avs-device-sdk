/*
 * MockMediaPlayer.h
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_MEDIAPLAYER_MOCKMEDIAPLAYER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_MEDIAPLAYER_MOCKMEDIAPLAYER_H_

#include "AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h"
#include <gmock/gmock.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace mediaPlayer {
namespace test {

/// Default time parameter.
static const std::chrono::milliseconds DEFAULT_TIME(50);

/// A mock MediaPlayer for unit tests.
class MockMediaPlayer : public MediaPlayerInterface {
public:
    /// Constructor.
    MockMediaPlayer();

    /// Destructor.
    ~MockMediaPlayer();

    /**
     * Creates an instance of the @c MockMediaPlayer.
     *
     * @return An instance of the @c MockMediaPlayer.
     */
    static std::shared_ptr<testing::NiceMock<MockMediaPlayer>> create();

    // 'override' commented out to avoid needless warnings generated because MOCK_METHOD* does not use it.
    void setObserver(
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface> playerObserver) /*override*/;

    SourceId setSource(std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader) /*override*/;
    MOCK_METHOD1(
        attachmentSetSource,
        SourceId(std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader));
    SourceId setSource(
        const std::string& url,
        std::chrono::milliseconds offset = std::chrono::milliseconds::zero()) /*override*/;
    MOCK_METHOD2(streamSetSource, SourceId(std::shared_ptr<std::istream> stream, bool repeat));
    SourceId setSource(std::shared_ptr<std::istream> stream, bool repeat) /*override*/;
    void doShutdown() /*override*/;
    MOCK_METHOD1(urlSetSource, SourceId(const std::string& url));

    MOCK_METHOD1(play, bool(SourceId));
    MOCK_METHOD1(stop, bool(SourceId));
    MOCK_METHOD1(pause, bool(SourceId));
    MOCK_METHOD1(resume, bool(SourceId));
    MOCK_METHOD1(getOffset, std::chrono::milliseconds(SourceId));

    /**
     * This is a mock method which will generate a new SourceId.
     *
     * @return @c SUCCESS.
     */
    SourceId mockSetSource();

    /**
     * This is a mock method which validates the id in a @c setOffset() call.
     */
    bool mockSetOffset(SourceId id, std::chrono::milliseconds offset);

    /**
     * This is a mock method which will signal to @c waitForPlay to send the play started notification to the observer.
     *
     * @return @c SUCCESS.
     */
    bool mockPlay(SourceId id);

    /**
     * This is a mock method which will signal to @c waitForStop to send the play finished notification to the observer.
     *
     * @return @c SUCCESS.
     */
    bool mockStop(SourceId id);

    /**
     * This is a mock method which will signal to @c waitForPause to send the play finished notification to the
     * observer.
     *
     * @return @c SUCCESS.
     */
    bool mockPause(SourceId id);

    /**
     * This is a mock method which will signal to @c waitForResume to send the play finished notification to the
     * observer.
     *
     * @return @c SUCCESS.
     */
    bool mockResume(SourceId id);

    /**
     * This is a mock method which validates the id in a @c getOffset() call.
     */
    std::chrono::milliseconds mockGetOffset(SourceId id);

    /**
     * Waits for play to be called. It notifies the observer that play has started.
     *
     * @param duration Time to wait for a play to be called before notifying observer that an error occurred.
     * @return @c true if play was called within the timeout duration else @c false.
     */
    bool waitForPlay(const std::chrono::milliseconds duration = std::chrono::milliseconds(DEFAULT_TIME));

    /**
     * Waits for stop to be called. It notifies the observer that play has finished.
     *
     * @param duration Time to wait for a stop to be called before notifying observer that an error occurred.
     * @return @c true if stop was called within the timeout duration else @c false.
     */
    bool waitForStop(const std::chrono::milliseconds duration = std::chrono::milliseconds(DEFAULT_TIME));

    /**
     * Waits for pause to be called. It notifies the observer that play has been paused.
     *
     * @param duration Time to wait for a pause to be called before notifying observer that an error occurred.
     * @return @c true if pause was called within the timeout duration else @c false.
     */
    bool waitForPause(const std::chrono::milliseconds duration = std::chrono::milliseconds(DEFAULT_TIME));

    /**
     * Waits for resume to be called. It notifies the observer that play should resume.
     *
     * @param duration Time to wait for a resume to be called before notifying observer that an error occurred.
     * @return @c true if resume was called within the timeout duration else @c false.
     */
    bool waitForResume(const std::chrono::milliseconds duration = std::chrono::milliseconds(DEFAULT_TIME));

    /**
     * Waits for the promise @c m_wakePlayPromise to be fulfilled and the future to be notified of call to @c play.
     *
     * @param timeout The duration to wait for the future to be ready.
     * @return @c true if @c play was called within the @c timeout else @c false.
     */
    bool waitUntilPlaybackStarted(const std::chrono::milliseconds timeout = std::chrono::milliseconds(DEFAULT_TIME));

    /**
     * Waits for the promise @c m_wakeStopPromise to be fulfilled and the future to be notified of call to @c stop.
     *
     * @param timeout The duration to wait for the future to be ready.
     * @return @c true if @c stop was called within the @c timeout else @c false.
     */
    bool waitUntilPlaybackFinished(const std::chrono::milliseconds timeout = std::chrono::milliseconds(DEFAULT_TIME));

    /**
     * Waits for the promise @c m_wakePausePromise to be fulfilled and the future to be notified of call to @c pause.
     *
     * @param timeout The duration to wait for the future to be ready.
     * @return @c true if @c pause was called within the @c timeout else @c false.
     */
    bool waitUntilPlaybackPaused(const std::chrono::milliseconds timeout = std::chrono::milliseconds(DEFAULT_TIME));

    /**
     * Waits for the promise @c m_wakeResumePromise to be fulfilled and the future to be notified of call to @c resume.
     *
     * @param timeout The duration to wait for the future to be ready.
     * @return @c true if @c resume was called within the @c timeout else @c false.
     */
    bool waitUntilPlaybackResumed(const std::chrono::milliseconds timeout = std::chrono::milliseconds(DEFAULT_TIME));

    /// Condition variable to wake the @ waitForPlay.
    std::condition_variable m_wakeTriggerPlay;

    /// Condition variable to wake the @ waitForStop.
    std::condition_variable m_wakeTriggerStop;

    /// Condition variable to wake the @ waitForPause.
    std::condition_variable m_wakeTriggerPause;

    /// Condition variable to wake the @ waitForResume.
    std::condition_variable m_wakeTriggerResume;

    /// mutex to protect @c m_lastId, @c m_playId, @c m_stopId, @c m_pauseId, @c m_resumeId and @c m_shutdown.
    std::mutex m_mutex;

    /// Identifier for the currently selected audio source.
    SourceId m_sourceId;

    /// The current offset of the mock media player.
    std::chrono::milliseconds m_offset;

    /// Flag to indicate @c play was called.
    bool m_play;

    /// Flag to indicate @c stop was called.
    bool m_stop;

    /// Flag to indicate @c pause was called
    bool m_pause;

    /// Flag to indicate @c resume was called
    bool m_resume;

    /// Flag to indicate when MockMediaPlayer is shutting down.
    bool m_shutdown;

    /// Thread to run @c waitForPlay asynchronously.
    std::thread m_playThread;

    /// Second thread to run @c waitForPlay asynchronously, to test returning to the PLAYING state
    std::thread m_playThread_2;

    /// Thread to run @c waitForStop asynchronously.
    std::thread m_stopThread;

    /// Thread to run @c waitForPause asynchronously.
    std::thread m_pauseThread;

    /// Second thread to run @c waitForPause asynchronously, to test returning to the PAUSED state
    std::thread m_pauseThread_2;

    /// Thread to run @c waitForResume asynchronously.
    std::thread m_resumeThread;

    /// Promise to be fulfilled when @c play is called.
    std::promise<void> m_wakePlayPromise;

    /// Future to notify when @c play is called.
    std::future<void> m_wakePlayFuture;

    /// Promise to be fulfilled when @c stop is called.
    std::promise<void> m_wakeStopPromise;

    /// Future to notify when @c stop is called.
    std::future<void> m_wakeStopFuture;

    /// Promise to be fulfilled when @c pause is called.
    std::promise<void> m_wakePausePromise;

    /// Future to notify when @c pause is called.
    std::future<void> m_wakePauseFuture;

    /// Promise to be fulfilled when @c resume is called.
    std::promise<void> m_wakeResumePromise;

    /// Future to notify when @c resume is called.
    std::future<void> m_wakeResumeFuture;

    /// The player observer to be notified of the media player state changes.
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface> m_playerObserver;
};

}  // namespace test
}  // namespace mediaPlayer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_MEDIAPLAYER_MOCKMEDIAPLAYER_H_
