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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_MEDIAPLAYER_MOCKMEDIAPLAYER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_MEDIAPLAYER_MOCKMEDIAPLAYER_H_

#include <unordered_set>

#include <gmock/gmock.h>

#include "AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h"
#include "AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h"
#include "AVSCommon/Utils/MediaPlayer/SourceConfig.h"
#include "AVSCommon/Utils/RequiresShutdown.h"
#include "AVSCommon/Utils/Timing/Stopwatch.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace mediaPlayer {
namespace test {

/// Default time parameter.
static const std::chrono::milliseconds DEFAULT_TIME(50);

/**
 * Interface to add virtual functions to MediaPlayerInterface to allow for mocking / EXPECT
 * of polymorphic methods.
 */
class MockMediaPlayerHelper : public MediaPlayerInterface {
public:
    /**
     * Variant of setSource() taking an attachment reader.
     *
     * @param attachmentReader The attachment from which to read audio data.
     * @param audioFormat The audioFormat to be used when playing raw PCM data.
     * @return The SourceId used by MediaPlayerInterface to identify this @c setSource() request.
     */
    virtual SourceId attachmentSetSource(
        std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader,
        const avsCommon::utils::AudioFormat* audioFormat) = 0;
    /**
     * Variant of setSource() taking an istream from which to read audio data.
     *
     * @param stream The stream from which to read audio data.
     * @param repeat Whether or not to repeat playing the audio.
     * @return The SourceId used by MediaPlayerInterface to identify this @c setSource() request.
     */
    virtual SourceId streamSetSource(std::shared_ptr<std::istream> stream, bool repeat) = 0;
    /**
     * Variant of setSource() taking a URL with which to fetch audio data.
     *
     * @param url The URL with which to fetch the audio data.
     * @return The SourceId used by MediaPlayerInterface to identify this @c setSource() request.
     */
    virtual SourceId urlSetSource(const std::string& url) = 0;
};

/// A mock MediaPlayer for unit tests.
class MockMediaPlayer
        : public MockMediaPlayerHelper
        , public RequiresShutdown {
public:
    using observer = avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface;
    using MediaPlayerState = avsCommon::utils::mediaPlayer::MediaPlayerState;

    /**
     * Creates an instance of the @c MockMediaPlayer.
     *
     * @return An instance of the @c MockMediaPlayer.
     */
    static std::shared_ptr<testing::NiceMock<MockMediaPlayer>> create();

    /**
     * Configures the MockMediaPlayer to support concurrent MediaPlayers.
     * Concurrent players differ  in that they don't assume that
     * the last Player with 'setSource' called is the 'current' player.
     * Instead, the last player on which 'play' was called is the current player.
     * This is used in tests involving pre-buffering, where multiple players may have had 'setSource' called.
     * The enabled state will persist until 'doShutdown' is called
     */
    static void enableConcurrentMediaPlayers();

    MockMediaPlayer();

    // 'override' commented out below to avoid needless warnings generated because MOCK_METHOD* does not use it.

    /// @name MediaPlayerInterface overrides
    /// @{
    SourceId setSource(
        std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader,
        const avsCommon::utils::AudioFormat* audioFormat = nullptr,
        const SourceConfig& config = emptySourceConfig()) /*override*/;
    SourceId setSource(
        const std::string& url,
        std::chrono::milliseconds offset = std::chrono::milliseconds::zero(),
        const SourceConfig& config = emptySourceConfig(),
        bool repeat = false) /*override*/;
    SourceId setSource(
        std::shared_ptr<std::istream> stream,
        bool repeat = false,
        const SourceConfig& config = emptySourceConfig(),
        avsCommon::utils::MediaType format = avsCommon::utils::MediaType::UNKNOWN) /*override*/;
    void addObserver(std::shared_ptr<observer> playerObserver) /*override*/;
    void removeObserver(std::shared_ptr<observer> playerObserver) /*override*/;
    /// @}

    MOCK_METHOD1(play, bool(SourceId));
    MOCK_METHOD1(pause, bool(SourceId));
    MOCK_METHOD1(resume, bool(SourceId));
    MOCK_METHOD1(stop, bool(SourceId));
    MOCK_METHOD1(getOffset, std::chrono::milliseconds(SourceId));
    MOCK_METHOD0(getNumBytesBuffered, uint64_t());
    MOCK_METHOD1(
        getMediaPlayerState,
        avsCommon::utils::Optional<avsCommon::utils::mediaPlayer::MediaPlayerState>(SourceId));
    MOCK_METHOD0(getPlaybackAttributes, avsCommon::utils::Optional<PlaybackAttributes>());
    MOCK_METHOD0(getPlaybackReports, std::vector<PlaybackReport>());

    /// @name RequiresShutdown overrides
    /// @{
    void doShutdown() /*override*/;
    /// @}

    MOCK_METHOD2(
        attachmentSetSource,
        SourceId(
            std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader,
            const avsCommon::utils::AudioFormat* audioFormat));
    MOCK_METHOD2(streamSetSource, SourceId(std::shared_ptr<std::istream> stream, bool repeat));
    MOCK_METHOD1(urlSetSource, SourceId(const std::string& url));

    /**
     * This is a mock method which will generate a new SourceId.
     *
     * @return @c SUCCESS.
     */
    SourceId mockSetSource();

    /**
     * This is a mock method which will send the @c onPlaybackStarted() notification to the observer.
     *
     * @return Whether the operation was successful.
     */
    bool mockPlay(SourceId sourceId);

    /**
     * This is a mock method which will send the @c onPlaybackPaused() notification to the observer.
     *
     * @return Whether the operation was successful.
     */
    bool mockPause(SourceId sourceId);

    /**
     * This is a mock method which will send the @c onPlaybackResumed() notification to the observer.
     *
     * @return Whether the operation was successful.
     */
    bool mockResume(SourceId sourceId);

    /**
     * This is a mock method which will send the @c onPlaybackStopped() notification to the observer.
     *
     * @return Whether the operation was successful.
     */
    bool mockStop(SourceId sourceId);

    /**
     * This is a mock method which will send the @c onPlaybackFinished() notification to the observer.
     *
     * @return Whether the operation was successful.
     */
    bool mockFinished(SourceId sourceId);

    /**
     * This is a mock method which will send the @c onPlaybackError() notification to the observer.
     *
     * @return Whether the operation was successful.
     */
    bool mockError(SourceId sourceId);

    /**
     * This is a mock method which validates @c sourceId and if valid, sets the offset.
     */
    bool mockSetOffset(SourceId sourceId, std::chrono::milliseconds offset);

    /**
     * This is a mock method which validates the id in a @c getOffset() call.
     */
    std::chrono::milliseconds mockGetOffset(SourceId id);

    /**
     * This is a mock method which gives the @c sourceId and the offset through @c getOffset()
     */
    avsCommon::utils::Optional<avsCommon::utils::mediaPlayer::MediaPlayerState> mockGetState(SourceId id);

    /**
     * Reset the timer used in the following waitXXX methods.
     * Used to test resuming
     */
    void resetWaitTimer();

    /**
     * Waits for the next call to @c setSource().
     * When concurrentMediaPlayers are enabled, this function will wait for the next call to any instance's @c
     * setSource()
     *
     * @param timeout The maximum time to wait.
     * @return @c true if setSource was called within @c timeout milliseconds else @c false.
     */
    bool waitUntilNextSetSource(const std::chrono::milliseconds timeout = std::chrono::milliseconds(DEFAULT_TIME));

    /**
     * Waits for the current source to reach the playback started state.
     * This is only valid when concurrentMediaPlayers are not enabled.
     *
     * @param timeout The maximum time to wait.
     * @return @c true if the state was reached within @c timeout milliseconds else @c false.
     */
    bool waitUntilPlaybackStarted(const std::chrono::milliseconds timeout = std::chrono::milliseconds(DEFAULT_TIME));
    /**
     * Waits for a source to reach the playback started state.
     *
     * @param timeout The maximum time to wait.
     * @param is The SourceId of the track waiting to Play
     * @return @c true if the state was reached within @c timeout milliseconds else @c false.
     */
    bool waitUntilPlaybackStarted(
        SourceId id,
        const std::chrono::milliseconds timeout = std::chrono::milliseconds(DEFAULT_TIME));

    /**
     * Waits for the current source to reach the playback paused state.
     *
     * @param timeout The maximum time to wait.
     * @return @c true if the state was reached within @c timeout milliseconds else @c false.
     */
    bool waitUntilPlaybackPaused(const std::chrono::milliseconds timeout = std::chrono::milliseconds(DEFAULT_TIME));

    /**
     * Waits for the current source to reach the playback resumed state.
     *
     * @param timeout The maximum time to wait.
     * @return @c true if the state was reached within @c timeout milliseconds else @c false.
     */
    bool waitUntilPlaybackResumed(const std::chrono::milliseconds timeout = std::chrono::milliseconds(DEFAULT_TIME));

    /**
     * Waits for the current source to reach the playback stopped state.
     *
     * @param timeout The maximum time to wait.
     * @return @c true if the state was reached within @c timeout milliseconds else @c false.
     */
    bool waitUntilPlaybackStopped(const std::chrono::milliseconds timeout = std::chrono::milliseconds(DEFAULT_TIME));

    /**
     * Waits for the current source to reach the playback finished state.
     *
     * @param timeout The maximum time to wait.
     * @return @c true if the state was reached within @c timeout milliseconds else @c false.
     */
    bool waitUntilPlaybackFinished(const std::chrono::milliseconds timeout = std::chrono::milliseconds(DEFAULT_TIME));

    /**
     * Waits for the current source to reach the playback error state.
     *
     * @param timeout The maximum time to wait.
     * @return @c true if the state was reached within @c timeout milliseconds else @c false.
     */
    bool waitUntilPlaybackError(const std::chrono::milliseconds timeout = std::chrono::milliseconds(DEFAULT_TIME));

    /**
     * Get the SourceId for the media MockMediaPlayer is currently playing.
     *
     * @return The SourceId for the media MockMediaPlayer is currently playing.
     */
    SourceId getCurrentSourceId();

    /**
     * Get the SourceId for this MediaPlayer instance
     *
     * @return the SourceId for this MediaPlayer instance
     */
    SourceId getSourceId();

    /**
     * Get the SourceId for the most recent MediaPlayer.setSource()
     *
     * @return the SourceId for the MediaPlayer instance
     */
    SourceId getLatestSourceId();

    /**
     * Get the list of current observers.
     *
     * @return List of current observers to this.
     */
    std::unordered_set<std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface>> getObservers()
        const;

private:
    struct Source;

    /**
     * Track whether or not a particular MediaPlayer state has been reached for a particular source.
     */
    class SourceState {
    public:
        /**
         * Constructor.  Initializes the state to unreached.
         *
         * @param source The source whose state is being tracked.
         * @param name Name of the state to be tracked (for logging)
         * @param notifyFunction The function to call to notify our observer that this state has been reached.
         */
        SourceState(
            Source* source,
            const std::string& name,
            std::function<void(std::shared_ptr<observer>, SourceId, const MediaPlayerState&)> notifyFunction);

        /**
         * Destructor.  If necessary, wakes and joins m_thread (cancelling any non-started notification)
         * so that m_thread can be destroyed.
         */
        ~SourceState();

        /**
         * Trigger the transition to reaching this state.
         * @note This method returns immediately (possibly before the notification has been delivered).
         */
        void trigger();

        /**
         * Wait until this state is reached.
         *
         * @param timeout How long to wait to reach this state before giving up.
         * @return Whether or not this state was reached.
         */
        bool wait(std::chrono::milliseconds timeout);

        /**
         * Reset this state to unreached.
         */
        void resetStateReached();

    private:
        /**
         * Make notification callback to report reaching this state.
         *
         * @param observers List of observers to notify
         * @param timeout How long to wait before giving up on actually reaching the state and delivering
         * an @c onPlaybackFailed() callback, instead.
         */
        void notify(
            const std::unordered_set<std::shared_ptr<MediaPlayerObserverInterface>>& observer,
            std::chrono::milliseconds timeout);

        /// The source whose state we are tracking.
        Source* m_source;

        /// Name of this state (for logging).
        std::string m_name;

        /// The function to use to deliver this notification.
        std::function<void(std::shared_ptr<observer>, SourceId, const MediaPlayerState&)> m_notifyFunction;

        /// Used to serialize access to some data members.
        std::mutex m_mutex;

        /// Whether or not this state has been reached. Access synchronized with @c mutex.
        bool m_stateReached;

        /// Used to wake threads waiting to reach this state.
        std::condition_variable m_wake;

        /// Thread used to make observer callbacks asynchronously.
        std::thread m_thread;

        /// Whether or not this instance is shutting down. Access synchronized with @c mutex.
        bool m_shutdown;
    };

    /**
     * Object to track the states for a given source.
     */
    struct Source {
    public:
        /**
         * Constructor.  Initializes this source such that all states have not been reached.
         *
         * @param player The @c MockMediaPlayer this source was added to.
         * @param id The @c SourceId that this source was assigned.
         */
        Source(MockMediaPlayer* player, SourceId id);

        /// The @c MockMediaPlayer this source was added to.
        MockMediaPlayer* mockMediaPlayer;

        /// The @c SourceId that this source was assigned.
        SourceId sourceId;

        /// Offset to report for @c getOffset() calls for this source.
        std::chrono::milliseconds offset;

        /// Tracks if playbackStarted state has been reached.
        SourceState started;

        /// Tracks if playbackPaused state has been reached.
        SourceState paused;

        /// Tracks if playbackResumed state has been reached.
        SourceState resumed;

        /// Tracks if playbackStopped state has been reached.
        SourceState stopped;

        /// Tracks if playbackFinished state has been reached.
        SourceState finished;

        /// Tracks if playbackError state has been reached.
        SourceState error;

        /// Tracks how far mocked playback has progressed, using elapsed real time.
        avsCommon::utils::timing::Stopwatch stopwatch;
    };

    /**
     * Get the current @c Source, but only if it matches the specified @c SourceId.
     *
     * @param The @c SourceId to match with the current source.
     * @return The current @c Source, or nullptr if the current source does not match @c sourceId.
     */
    std::shared_ptr<Source> getCurrentSource(SourceId sourceId);

    /**
     * Get the @c Source previous to the current, but only if it matches the specified @c SourceId.
     *
     * @param The @c SourceId to match with the previous source.
     * @return The current @c Source, or nullptr if the previous source does not match @c sourceId.
     */
    std::shared_ptr<Source> getPreviousSource(SourceId sourceId);

    /**
     * Returns true if sourceId is one that was returned by a call to 'setSource'.
     *
     * @param sourceId sourceId to validate
     * @return true if valid
     */
    bool isValidSourceId(SourceId sourceId);

    /// Serialize access to instance data
    mutable std::mutex m_mutex;

    /// Serialize access to m_sources and global ids.
    static std::mutex m_globalMutex;

    /// All sources fed to this MediaPlayer.  Static so its global across many instances
    static std::vector<std::shared_ptr<Source>> m_sources;

    /// The Current SourceID
    static std::atomic<SourceId> m_currentSourceId;

    /// The Previous SourceID (or MediaPlayerInterface::ERROR, if there is no previous)
    static SourceId m_previousSourceId;

    /// Concurrent mode
    static bool m_isConcurrentEnabled;

    /// The player observers to be notified of the media player state changes.
    std::unordered_set<std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface>> m_playerObservers;
};

}  // namespace test
}  // namespace mediaPlayer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_MEDIAPLAYER_MOCKMEDIAPLAYER_H_
