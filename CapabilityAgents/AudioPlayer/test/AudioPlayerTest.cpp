/*
 * AudioPlayerTest.cpp
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

/// @file AudioPlayerTest.cpp

#include <chrono>
#include <future>
#include <memory>
#include <map>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/AVS/Attachment/AttachmentManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockFocusManager.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveSequencer.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/AVS/Attachment/AttachmentManager.h>

#include "AudioPlayer/AudioPlayer.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace audioPlayer {
namespace test {

using namespace avsCommon::utils::json;
using namespace avsCommon::utils;
using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils::mediaPlayer;
using namespace ::testing;
using namespace rapidjson;

/// Plenty of time for a test to complete.
static std::chrono::milliseconds WAIT_TIMEOUT(1000);

/// Default time parameter.
static std::chrono::milliseconds DEFAULT_TIME(50);

/// The name of the @c FocusManager channel used by the @c AudioPlayer.
static const std::string CHANNEL_NAME("Content");

/// The activity Id used with the @c FocusManager by @c AudioPlayer.
static const std::string FOCUS_MANAGER_ACTIVITY_ID("AudioPlayer.Play");

/// Namespace for AudioPlayer.
static const std::string NAMESPACE_AUDIO_PLAYER("AudioPlayer");

/// Name for AudioPlayer Play directive.
static const std::string NAME_PLAY("Play");

/// Name for AudioPlayer Stop directive.
static const std::string NAME_STOP("Stop");

/// Name for AudioPlayer ClearQueue directive.
static const std::string NAME_CLEARQUEUE("ClearQueue");

/// The @c NamespaceAndName to send to the @c ContextManager.
static const NamespaceAndName NAMESPACE_AND_NAME_PLAYBACK_STATE{NAMESPACE_AUDIO_PLAYER, "PlaybackState"};

/// Message Id for testing.
static const std::string MESSAGE_ID_TEST("MessageId_Test");

/// Another message Id for testing.
static const std::string MESSAGE_ID_TEST_2("MessageId_Test2");

/// PlayRequestId for testing.
static const std::string PLAY_REQUEST_ID_TEST("PlayRequestId_Test");

/// Context ID for testing
static const std::string CONTEXT_ID_TEST("ContextId_Test");

/// Context ID for testing
static const std::string CONTEXT_ID_TEST_2("ContextId_Test2");

/// Token for testing.
static const std::string TOKEN_TEST("Token_Test");

/// Previous token for testing.
static const std::string PREV_TOKEN_TEST("Prev_Token_Test");

/// Format of the audio.
static const std::string FORMAT_TEST("AUDIO_MPEG");

/// URL for testing.
static const std::string URL_TEST("cid:Test");

/// ENQUEUE playBehavior.
static const std::string NAME_ENQUEUE("ENQUEUE");

/// CLEAR_ALL clearBehavior.
static const std::string NAME_CLEAR_ALL("CLEAR_ALL");

/// audioItemId for testing.
static const std::string AUDIO_ITEM_ID("testID");

/// The @c FINISHED state of the @c AudioPlayer.
static const std::string FINISHED_STATE("FINISHED");

/// The @c PLAYING state of the @c AudioPlayer
static const std::string PLAYING_STATE{"PLAYING"};

/// The @c IDLE state of the @c AudioPlayer
static const std::string IDLE_STATE{"IDLE"};

/// The offset in milliseconds returned by the mock media player.
static const long OFFSET_IN_MILLISECONDS_TEST{100};

/// ExpiryTime for testing. Needs to be in ISO 8601 format.
static const std::string EXPIRY_TEST("481516234248151623421088");

/// progressReportDelayInMilliseconds for testing.
static const long PROGRESS_REPORT_DELAY{200};

/// progressReportIntervalInMilliseconds for testing.
static const long PROGRESS_REPORT_INTERVAL{100};

/// A payload for testing.
// clang-format off
static const std::string ENQUEUE_PAYLOAD_TEST =
        "{"
            "\"playBehavior\":\"" + NAME_ENQUEUE + "\","
            "\"audioItem\": {"
                "\"audioItemId\":\"" + AUDIO_ITEM_ID + "\","
                "\"stream\": {"
                    "\"url\":\"" + URL_TEST + "\","
                    "\"streamFormat\":\"" + FORMAT_TEST + "\","
                    "\"offsetInMilliseconds\":" + std::to_string(OFFSET_IN_MILLISECONDS_TEST) + ","
                    "\"expiryTime\":\"" + EXPIRY_TEST + "\","
                    "\"progressReport\": {"
                        "\"progressReportDelayInMilliseconds\":" + std::to_string(PROGRESS_REPORT_DELAY) + ","
                        "\"progressReportIntervalInMilliseconds\":" + std::to_string(PROGRESS_REPORT_INTERVAL) +
                    "},"
                    "\"token\":\"" + TOKEN_TEST + "\","
                    "\"expectedPreviousToken\":\"\""
                "}"
            "}"
        "}";
// clang-format on

/// Empty payload for testing.
static const std::string EMPTY_PAYLOAD_TEST = "{}";

/// CLEAR_ALL payload for testing.
// clang-format off
static const std::string CLEAR_ALL_PAYLOAD_TEST =
    "{"
        "\"clearBehavior\":\"" + NAME_CLEAR_ALL + "\""
    "}";
// clang-format on

/// Token JSON key.
static const std::string TOKEN_KEY = "token";

/// Offset JSON key.
static const std::string OFFSET_KEY = "offsetInMilliseconds";

/// Player activity JSON key.
static const std::string ACTIVITY_KEY = "playerActivity";

/// The expected state when the @c AudioPlayer is not handling any directive.
// clang-format off
static const std::string IDLE_STATE_TEST =
    "{"
        "\"token\":\"\","
        "\"offsetInMilliseconds\":" + std::to_string(0) + ","
        "\"playerActivity\":\"" + IDLE_STATE + "\""
    "}";
// clang-format on

/// Provide State Token for testing.
static const unsigned int PROVIDE_STATE_TOKEN_TEST{1};

/// JSON key for the event section of a message.
static const std::string MESSAGE_EVENT_KEY = "event";

/// JSON key for the header section of a message.
static const std::string MESSAGE_HEADER_KEY = "header";

/// JSON key for the name section of a message.
static const std::string MESSAGE_NAME_KEY = "name";

/// Name of PlaybackStarted event
static const std::string PLAYBACK_STARTED_NAME = "PlaybackStarted";

/// Name of PlaybackNearlyFinished event
static const std::string PLAYBACK_NEARLY_FINISHED_NAME = "PlaybackNearlyFinished";

/// Name of PlaybackFinished event
static const std::string PLAYBACK_FINISHED_NAME = "PlaybackFinished";

/// Name of PlaybackStopped event
static const std::string PLAYBACK_STOPPED_NAME = "PlaybackStopped";

/// Name of PlaybackPaused event
static const std::string PLAYBACK_PAUSED_NAME = "PlaybackPaused";

/// Name of PlaybackFailed event
static const std::string PLAYBACK_FAILED_NAME = "PlaybackFailed";

/// Name of PlaybackResumed event
static const std::string PLAYBACK_RESUMED_NAME = "PlaybackResumed";

/// Name of PlaybackStutterStarted event
static const std::string PLAYBACK_STUTTER_STARTED_NAME = "PlaybackStutterStarted";

/// Name of PlaybackStutterFinished event
static const std::string PLAYBACK_STUTTER_FINISHED_NAME = "PlaybackStutterFinished";

/// Name of ProgressReportDelayElapsed event
static const std::string PROGRESS_REPORT_DELAY_ELAPSED_NAME = "ProgressReportDelayElapsed";

/// Name of ProgressReportIntervalElapsed event
static const std::string PROGRESS_REPORT_INTERVAL_ELAPSED_NAME = "ProgressReportIntervalElapsed";

/// String to identify log entries originating from this file.
static const std::string TAG("AudioPlayerTest");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

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
    static std::shared_ptr<NiceMock<MockMediaPlayer>> create();

    // 'override' commented out to avoid needless warnings generated because MOCK_METHOD* does not use it.
    void setObserver(
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface> playerObserver) /*override*/;

    MOCK_METHOD1(
        setSource,
        MediaPlayerStatus(std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader));
    MOCK_METHOD2(setSource, MediaPlayerStatus(std::shared_ptr<std::istream> stream, bool repeat));
#ifdef __clang__
// Remove warnings when compiling with clang.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
#endif

    MOCK_METHOD1(setSource, MediaPlayerStatus(const std::string& url));

#ifdef __clang__
#pragma clang diagnostic pop
#endif
    MOCK_METHOD0(play, MediaPlayerStatus());
    MOCK_METHOD0(stop, MediaPlayerStatus());
    MOCK_METHOD0(pause, MediaPlayerStatus());
    MOCK_METHOD0(resume, MediaPlayerStatus());
    MOCK_METHOD0(getOffset, std::chrono::milliseconds());
    MOCK_METHOD0(getOffsetInMilliseconds, int64_t());
    MOCK_METHOD1(setOffset, MediaPlayerStatus(std::chrono::milliseconds offset));

    /**
     * This is a mock method which will signal to @c waitForPlay to send the play started notification to the observer.
     *
     * @return @c SUCCESS.
     */
    MediaPlayerStatus mockPlay();

    /**
     * This is a mock method which will signal to @c waitForStop to send the play finished notification to the observer.
     *
     * @return @c SUCCESS.
     */
    MediaPlayerStatus mockStop();

    /**
     * This is a mock method which will signal to @c waitForPause to send the play finished notification to the
     * observer.
     *
     * @return @c SUCCESS.
     */
    MediaPlayerStatus mockPause();

    /**
     * This is a mock method which will signal to @c waitForResume to send the play finished notification to the
     * observer.
     *
     * @return @c SUCCESS.
     */
    MediaPlayerStatus mockResume();

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

    /// mutex to protect @c m_play, @c m_stop and @c m_shutdown.
    std::mutex m_mutex;

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

std::shared_ptr<NiceMock<MockMediaPlayer>> MockMediaPlayer::create() {
    auto result = std::make_shared<NiceMock<MockMediaPlayer>>();

    ON_CALL(*result.get(), play()).WillByDefault(Invoke(result.get(), &MockMediaPlayer::mockPlay));
    ON_CALL(*result.get(), stop()).WillByDefault(Invoke(result.get(), &MockMediaPlayer::mockStop));
    ON_CALL(*result.get(), pause()).WillByDefault(Invoke(result.get(), &MockMediaPlayer::mockPause));
    ON_CALL(*result.get(), resume()).WillByDefault(Invoke(result.get(), &MockMediaPlayer::mockResume));
    return result;
}

MockMediaPlayer::MockMediaPlayer() :
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
    if (m_resumeThread.joinable()) {
        m_resumeThread.join();
    }
}

void MockMediaPlayer::setObserver(
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface> playerObserver) {
    m_playerObserver = playerObserver;
}

MediaPlayerStatus MockMediaPlayer::mockPlay() {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_play) {
        m_playThread = std::thread(&MockMediaPlayer::waitForPlay, this, DEFAULT_TIME);
    } else {
        m_wakePlayPromise = std::promise<void>();
        m_playThread_2 = std::thread(&MockMediaPlayer::waitForPlay, this, DEFAULT_TIME);
    }

    m_play = true;
    m_wakeTriggerPlay.notify_one();
    return MediaPlayerStatus::SUCCESS;
}

MediaPlayerStatus MockMediaPlayer::mockStop() {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_stop) {
        m_stopThread = std::thread(&MockMediaPlayer::waitForStop, this, DEFAULT_TIME);
        m_stop = true;
        m_wakeTriggerStop.notify_one();
    }

    return MediaPlayerStatus::SUCCESS;
}

MediaPlayerStatus MockMediaPlayer::mockPause() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_pauseThread = std::thread(&MockMediaPlayer::waitForPause, this, DEFAULT_TIME);
    m_pause = true;
    m_wakeTriggerPause.notify_one();
    return MediaPlayerStatus::SUCCESS;
}

MediaPlayerStatus MockMediaPlayer::mockResume() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_resumeThread = std::thread(&MockMediaPlayer::waitForResume, this, DEFAULT_TIME);
    m_resume = true;
    m_wakeTriggerResume.notify_one();
    return MediaPlayerStatus::SUCCESS;
}

bool MockMediaPlayer::waitForPlay(const std::chrono::milliseconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakeTriggerPlay.wait_for(lock, duration, [this]() { return (m_play || m_shutdown); })) {
        if (m_playerObserver) {
            m_playerObserver->onPlaybackError(ErrorType::MEDIA_ERROR_UNKNOWN, "waitForPlay timed out");
        }
        return false;
    }
    m_wakePlayPromise.set_value();
    if (m_playerObserver) {
        m_playerObserver->onPlaybackStarted();
    }
    return true;
}

bool MockMediaPlayer::waitForStop(const std::chrono::milliseconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakeTriggerStop.wait_for(lock, duration, [this]() { return (m_stop || m_shutdown); })) {
        if (m_playerObserver) {
            m_playerObserver->onPlaybackError(ErrorType::MEDIA_ERROR_UNKNOWN, "waitForStop timed out");
        }
        return false;
    }
    m_wakeStopPromise.set_value();
    if (m_playerObserver) {
        m_playerObserver->onPlaybackFinished();
    }
    return true;
}

bool MockMediaPlayer::waitForPause(const std::chrono::milliseconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakeTriggerPause.wait_for(lock, duration, [this]() { return (m_pause || m_shutdown); })) {
        if (m_playerObserver) {
            m_playerObserver->onPlaybackError(ErrorType::MEDIA_ERROR_UNKNOWN, "waitForPause timed out");
        }
        return false;
    }
    m_wakePausePromise.set_value();
    if (m_playerObserver) {
        m_playerObserver->onPlaybackPaused();
    }
    return true;
}

bool MockMediaPlayer::waitForResume(const std::chrono::milliseconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakeTriggerResume.wait_for(lock, duration, [this]() { return (m_resume || m_shutdown); })) {
        if (m_playerObserver) {
            m_playerObserver->onPlaybackError(ErrorType::MEDIA_ERROR_UNKNOWN, "waitForResume timed out");
        }
        return false;
    }
    m_wakeResumePromise.set_value();
    if (m_playerObserver) {
        m_playerObserver->onPlaybackResumed();
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

class AudioPlayerTest : public ::testing::Test {
public:
    AudioPlayerTest();

    void SetUp() override;
    void TearDown() override;

    /// @c AudioPlayer to test
    std::shared_ptr<AudioPlayer> m_audioPlayer;

    /// Player to send the audio to.
    std::shared_ptr<MockMediaPlayer> m_mockMediaPlayer;

    /// @c ContextManager to provide state and update state.
    std::shared_ptr<MockContextManager> m_mockContextManager;

    /// @c FocusManager to request focus to the DIALOG channel.
    std::shared_ptr<MockFocusManager> m_mockFocusManager;

    /// A directive handler result to send the result to.
    std::unique_ptr<MockDirectiveHandlerResult> m_mockDirectiveHandlerResult;

    /// A message sender used to send events to AVS.
    std::shared_ptr<MockMessageSender> m_mockMessageSender;

    /// An exception sender used to send exception encountered events to AVS.
    std::shared_ptr<MockExceptionEncounteredSender> m_mockExceptionSender;

    /// Attachment manager used to create a reader.
    std::shared_ptr<AttachmentManager> m_attachmentManager;

    /// Map for expected messages testing
    std::map<std::string, bool> m_expectedMessages;

    /**
     * This is invoked in response to a @c setState call.
     *
     * @return @c SUCCESS.
     */
    SetStateResult wakeOnSetState();

    /// Promise to be fulfilled when @c setState is called.
    std::promise<void> m_wakeSetStatePromise;

    /// Future to notify when @c setState is called.
    std::future<void> m_wakeSetStateFuture;

    /**
     * This is invoked in response to a @c acquireChannel call.
     *
     * @return @c true
     */

    bool wakeOnAcquireChannel();

    /// Promise to be fulfilled when @c acquireChannel is called.
    std::promise<void> m_wakeAcquireChannelPromise;

    /// Future to notify when @c acquireChannel is called.
    std::future<void> m_wakeAcquireChannelFuture;

    /**
     * This is invoked in response to a @c releaseChannel call.
     *
     * @return @c true
     */
    std::future<bool> wakeOnReleaseChannel();

    /// Future to notify when @c releaseChannel is called.
    std::future<void> m_wakeReleaseChannelFuture;

    /**
     * Fulfills the @c m_wakeSendMessagePromise. This is invoked in response to a @c sendMessage call.
     */
    void wakeOnSendMessage();

    /// Promise to be fulfilled when @c sendMessage is called.
    std::promise<void> m_wakeSendMessagePromise;

    /// Future to notify when @c sendMessage is called.
    std::future<void> m_wakeSendMessageFuture;

    /**
     * Consolidate code to send Play directive.
     */
    void sendPlayDirective();

    /**
     * Consolidate code to send ClearQueue directive
     */

    void sendClearQueueDirective();

    /**
     * Verify that the message name matches the expected name
     *
     * @param request The @c MessageRequest to verify
     * @param expectedName The expected name to find in the json header
     */

    void verifyMessage(
        std::shared_ptr<avsCommon::avs::MessageRequest> request,
        std::map<std::string, bool>* expectedMessages);

    /**
     * Verify that the providede state matches the expected state
     *
     * @param jsonState The state to verify
     * @param expectedState The expected state
     */

    void verifyState(const std::string& providedState, const std::string& expectedState);

    /// Condition variable to wake on a message being sent
    std::condition_variable messageSentTrigger;

    /// Mutex for messages
    std::mutex messageMutex;
};

AudioPlayerTest::AudioPlayerTest() :
        m_wakeSetStatePromise{},
        m_wakeSetStateFuture{m_wakeSetStatePromise.get_future()},
        m_wakeAcquireChannelPromise{},
        m_wakeAcquireChannelFuture{m_wakeAcquireChannelPromise.get_future()},
        m_wakeSendMessagePromise{},
        m_wakeSendMessageFuture{m_wakeSendMessagePromise.get_future()} {
}

void AudioPlayerTest::SetUp() {
    m_mockContextManager = std::make_shared<NiceMock<MockContextManager>>();
    m_mockFocusManager = std::make_shared<NiceMock<MockFocusManager>>();
    m_mockMessageSender = std::make_shared<NiceMock<MockMessageSender>>();
    m_mockExceptionSender = std::make_shared<NiceMock<MockExceptionEncounteredSender>>();
    m_attachmentManager = std::make_shared<AttachmentManager>(AttachmentManager::AttachmentType::IN_PROCESS);
    m_mockMediaPlayer = MockMediaPlayer::create();
    m_audioPlayer = AudioPlayer::create(
        m_mockMediaPlayer,
        m_mockMessageSender,
        m_mockFocusManager,
        m_mockContextManager,
        m_attachmentManager,
        m_mockExceptionSender);
    m_mockDirectiveHandlerResult.reset(new MockDirectiveHandlerResult);

    ASSERT_TRUE(m_audioPlayer);
}

void AudioPlayerTest::TearDown() {
    m_audioPlayer->shutdown();
}

SetStateResult AudioPlayerTest::wakeOnSetState() {
    m_wakeSetStatePromise.set_value();
    return SetStateResult::SUCCESS;
}

bool AudioPlayerTest::wakeOnAcquireChannel() {
    m_wakeAcquireChannelPromise.set_value();
    return true;
}

std::future<bool> AudioPlayerTest::wakeOnReleaseChannel() {
    std::promise<bool> releaseChannelSuccess;
    std::future<bool> returnValue = releaseChannelSuccess.get_future();
    releaseChannelSuccess.set_value(true);
    return returnValue;
}

void AudioPlayerTest::wakeOnSendMessage() {
    m_wakeSendMessagePromise.set_value();
}

void AudioPlayerTest::sendPlayDirective() {
    auto avsMessageHeader =
        std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_TEST, PLAY_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> playDirective =
        AVSDirective::create("", avsMessageHeader, ENQUEUE_PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);
    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _, FOCUS_MANAGER_ACTIVITY_ID))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &AudioPlayerTest::wakeOnAcquireChannel));

    m_audioPlayer->CapabilityAgent::preHandleDirective(playDirective, std::move(m_mockDirectiveHandlerResult));
    m_audioPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(WAIT_TIMEOUT));

    m_audioPlayer->onFocusChanged(FocusState::FOREGROUND);
}

void AudioPlayerTest::sendClearQueueDirective() {
    auto avsClearMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_AUDIO_PLAYER, NAME_CLEARQUEUE, MESSAGE_ID_TEST, PLAY_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> clearQueueDirective =
        AVSDirective::create("", avsClearMessageHeader, CLEAR_ALL_PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    m_audioPlayer->CapabilityAgent::preHandleDirective(clearQueueDirective, std::move(m_mockDirectiveHandlerResult));
    m_audioPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

void AudioPlayerTest::verifyMessage(
    std::shared_ptr<avsCommon::avs::MessageRequest> request,
    std::map<std::string, bool>* expectedMessages) {
    rapidjson::Document document;
    document.Parse(request->getJsonContent().c_str());
    EXPECT_FALSE(document.HasParseError())
        << "rapidjson detected a parsing error at offset:" + std::to_string(document.GetErrorOffset()) +
               ", error message: " + GetParseError_En(document.GetParseError());

    auto event = document.FindMember(MESSAGE_EVENT_KEY);
    EXPECT_NE(event, document.MemberEnd());

    auto header = event->value.FindMember(MESSAGE_HEADER_KEY);
    EXPECT_NE(header, event->value.MemberEnd());

    std::string requestName;
    jsonUtils::retrieveValue(header->value, MESSAGE_NAME_KEY, &requestName);

    if (expectedMessages->find(requestName) != expectedMessages->end()) {
        expectedMessages->at(requestName) = true;
    }
}

void AudioPlayerTest::verifyState(const std::string& providedState, const std::string& expectedState) {
    rapidjson::Document providedStateParsed;
    providedStateParsed.Parse(providedState);

    rapidjson::Document expectedStateParsed;
    expectedStateParsed.Parse(expectedState);

    EXPECT_EQ(providedStateParsed, expectedStateParsed);
}

/**
 * Test create() with nullptrs
 */

TEST_F(AudioPlayerTest, testCreateWithNullPointers) {
    std::shared_ptr<AudioPlayer> testAudioPlayer;

    testAudioPlayer = AudioPlayer::create(
        m_mockMediaPlayer,
        nullptr,
        m_mockFocusManager,
        m_mockContextManager,
        m_attachmentManager,
        m_mockExceptionSender);
    EXPECT_EQ(testAudioPlayer, nullptr);

    testAudioPlayer = AudioPlayer::create(
        m_mockMediaPlayer,
        m_mockMessageSender,
        nullptr,
        m_mockContextManager,
        m_attachmentManager,
        m_mockExceptionSender);
    EXPECT_EQ(testAudioPlayer, nullptr);

    testAudioPlayer = AudioPlayer::create(
        m_mockMediaPlayer,
        m_mockMessageSender,
        m_mockFocusManager,
        nullptr,
        m_attachmentManager,
        m_mockExceptionSender);
    EXPECT_EQ(testAudioPlayer, nullptr);

    testAudioPlayer = AudioPlayer::create(
        m_mockMediaPlayer,
        m_mockMessageSender,
        m_mockFocusManager,
        m_mockContextManager,
        nullptr,
        m_mockExceptionSender);
    EXPECT_EQ(testAudioPlayer, nullptr);

    testAudioPlayer = AudioPlayer::create(
        m_mockMediaPlayer, m_mockMessageSender, m_mockFocusManager, m_mockContextManager, m_attachmentManager, nullptr);
    EXPECT_EQ(testAudioPlayer, nullptr);
}

/**
 * Test transition from Idle to Playing
 */

TEST_F(AudioPlayerTest, testTransitionFromIdleToPlaying) {
    EXPECT_CALL(*(m_mockMediaPlayer.get()), play()).Times(AtLeast(1));
    sendPlayDirective();

    ASSERT_TRUE(m_mockMediaPlayer->waitUntilPlaybackStarted());
}

/**
 * Test transition from Playing to Stopped with Stop Directive
 */

TEST_F(AudioPlayerTest, testTransitionFromPlayingToStopped) {
    sendPlayDirective();

    EXPECT_CALL(*(m_mockMediaPlayer.get()), stop()).Times(AtLeast(1));

    // now send Stop directive
    auto avsStopMessageHeader =
        std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_STOP, MESSAGE_ID_TEST, PLAY_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> stopDirective =
        AVSDirective::create("", avsStopMessageHeader, EMPTY_PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    m_audioPlayer->CapabilityAgent::preHandleDirective(stopDirective, std::move(m_mockDirectiveHandlerResult));
    m_audioPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    ASSERT_TRUE(m_mockMediaPlayer->waitUntilPlaybackFinished());
}

/**
 * Test transition from Playing to Stopped with ClearQueue.CLEAR_ALL Directive
 */

TEST_F(AudioPlayerTest, testTransitionFromPlayingToStoppedWithClear) {
    sendPlayDirective();

    EXPECT_CALL(*(m_mockMediaPlayer.get()), stop()).Times(AtLeast(1));

    sendClearQueueDirective();

    ASSERT_TRUE(m_mockMediaPlayer->waitUntilPlaybackFinished());
}

/**
 * Test transition from Stopped to Playing after issuing second Play directive
 */

TEST_F(AudioPlayerTest, testTransitionFromStoppedToPlaying) {
    sendPlayDirective();

    EXPECT_CALL(*(m_mockMediaPlayer.get()), stop()).Times(AtLeast(1));

    sendClearQueueDirective();

    EXPECT_CALL(*(m_mockMediaPlayer.get()), play()).Times(AtLeast(1));

    /// send a second Play directive
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_TEST_2);

    std::shared_ptr<AVSDirective> playDirective =
        AVSDirective::create("", avsMessageHeader, ENQUEUE_PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST_2);

    ASSERT_TRUE(m_mockMediaPlayer->waitUntilPlaybackStarted());

    m_audioPlayer->CapabilityAgent::preHandleDirective(playDirective, std::move(m_mockDirectiveHandlerResult));
    m_audioPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST_2);

    ASSERT_TRUE(m_mockMediaPlayer->waitUntilPlaybackStarted());

    m_audioPlayer->onFocusChanged(FocusState::FOREGROUND);
}

/**
 * Test transition from Playing to Paused when focus changes to Dialog channel
 */

TEST_F(AudioPlayerTest, testTransitionFromPlayingToPaused) {
    sendPlayDirective();

    EXPECT_CALL(*(m_mockMediaPlayer.get()), pause()).Times(AtLeast(1));

    // simulate focus change
    m_audioPlayer->onFocusChanged(FocusState::BACKGROUND);

    ASSERT_TRUE(m_mockMediaPlayer->waitUntilPlaybackPaused());
}

/**
 * Test transition from Paused to Stopped on ClearQueue.CLEAR_ALL directive
 */

TEST_F(AudioPlayerTest, testTransitionFromPausedToStopped) {
    sendPlayDirective();

    EXPECT_CALL(*(m_mockMediaPlayer.get()), stop()).Times(AtLeast(1));

    // simulate focus change in order to pause
    m_audioPlayer->onFocusChanged(FocusState::BACKGROUND);

    ASSERT_TRUE(m_mockMediaPlayer->waitUntilPlaybackPaused());

    sendClearQueueDirective();
}

/**
 * Test transition from Paused to Playing after resume
 */

TEST_F(AudioPlayerTest, testResumeAfterPaused) {
    sendPlayDirective();

    EXPECT_CALL(*(m_mockMediaPlayer.get()), stop()).Times(AtLeast(1));

    // simulate focus change in order to pause
    m_audioPlayer->onFocusChanged(FocusState::BACKGROUND);

    ASSERT_TRUE(m_mockMediaPlayer->waitUntilPlaybackPaused());

    EXPECT_CALL(*(m_mockMediaPlayer.get()), resume()).Times(AtLeast(1));
    m_audioPlayer->onFocusChanged(FocusState::FOREGROUND);
}

/**
 * Test @c provideState while IDLE
 */

TEST_F(AudioPlayerTest, testCallingProvideStateWhenIdle) {
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(NAMESPACE_AND_NAME_PLAYBACK_STATE, _, StateRefreshPolicy::NEVER, PROVIDE_STATE_TOKEN_TEST))
        .Times(1)
        .WillOnce(DoAll(
            // need to include all four arguments, but only care about jsonState
            Invoke([this](
                       const avs::NamespaceAndName& namespaceAndName,
                       const std::string& jsonState,
                       const avs::StateRefreshPolicy& refreshPolicy,
                       const unsigned int stateRequestToken) { verifyState(jsonState, IDLE_STATE_TEST); }),
            InvokeWithoutArgs(this, &AudioPlayerTest::wakeOnSetState)));

    m_audioPlayer->provideState(PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
}

/**
 * Test @c onPlaybackError and expect a PlaybackFailed message
 */

TEST_F(AudioPlayerTest, testOnPlaybackError) {
    m_expectedMessages.insert({PLAYBACK_STARTED_NAME, false});
    m_expectedMessages.insert({PLAYBACK_NEARLY_FINISHED_NAME, false});
    m_expectedMessages.insert({PLAYBACK_FAILED_NAME, false});

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            if (!m_mockMediaPlayer->m_stop) {
                std::lock_guard<std::mutex> lock(messageMutex);
                verifyMessage(request, &m_expectedMessages);
                messageSentTrigger.notify_one();
            }
        }));

    sendPlayDirective();

    m_audioPlayer->onPlaybackError(ErrorType::MEDIA_ERROR_UNKNOWN, "TEST_ERROR");

    std::unique_lock<std::mutex> lock(messageMutex);

    bool result;

    result = messageSentTrigger.wait_for(lock, WAIT_TIMEOUT, [this] {
        for (auto messageStatus : m_expectedMessages) {
            if (!messageStatus.second) {
                return false;
            }
        }
        return true;
    });

    ASSERT_TRUE(result);
}

/**
 * Test @c onPlaybackPaused and expect a PlaybackPaused message
 */

TEST_F(AudioPlayerTest, testOnPlaybackPaused) {
    m_expectedMessages.insert({PLAYBACK_STARTED_NAME, false});
    m_expectedMessages.insert({PLAYBACK_NEARLY_FINISHED_NAME, false});
    m_expectedMessages.insert({PLAYBACK_PAUSED_NAME, false});

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            if (!m_mockMediaPlayer->m_stop) {
                std::lock_guard<std::mutex> lock(messageMutex);
                verifyMessage(request, &m_expectedMessages);
                messageSentTrigger.notify_one();
            }
        }));

    sendPlayDirective();

    m_audioPlayer->onPlaybackPaused();

    std::unique_lock<std::mutex> lock(messageMutex);

    bool result;

    result = messageSentTrigger.wait_for(lock, WAIT_TIMEOUT, [this] {
        for (auto messageStatus : m_expectedMessages) {
            if (!messageStatus.second) {
                return false;
            }
        }
        return true;
    });

    ASSERT_TRUE(result);
}

/**
 * Test @c onPlaybackResumed and expect a PlaybackResumed message
 */

TEST_F(AudioPlayerTest, testOnPlaybackResumed) {
    m_expectedMessages.insert({PLAYBACK_STARTED_NAME, false});
    m_expectedMessages.insert({PLAYBACK_NEARLY_FINISHED_NAME, false});
    m_expectedMessages.insert({PLAYBACK_RESUMED_NAME, false});

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            if (!m_mockMediaPlayer->m_stop) {
                std::lock_guard<std::mutex> lock(messageMutex);
                verifyMessage(request, &m_expectedMessages);
                messageSentTrigger.notify_one();
            }
        }));

    sendPlayDirective();

    m_audioPlayer->onPlaybackResumed();

    std::unique_lock<std::mutex> lock(messageMutex);

    bool result;

    result = messageSentTrigger.wait_for(lock, WAIT_TIMEOUT, [this] {
        for (auto messageStatus : m_expectedMessages) {
            if (!messageStatus.second) {
                return false;
            }
        }
        return true;
    });

    ASSERT_TRUE(result);
}

/**
 * Test @c onBufferUnderrun and expect a PlaybackStutterStarted message
 */

TEST_F(AudioPlayerTest, testOnBufferUnderrun) {
    m_expectedMessages.insert({PLAYBACK_STARTED_NAME, false});
    m_expectedMessages.insert({PLAYBACK_NEARLY_FINISHED_NAME, false});
    m_expectedMessages.insert({PLAYBACK_STUTTER_STARTED_NAME, false});

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            if (!m_mockMediaPlayer->m_stop) {
                std::lock_guard<std::mutex> lock(messageMutex);
                verifyMessage(request, &m_expectedMessages);
                messageSentTrigger.notify_one();
            }
        }));

    sendPlayDirective();

    m_audioPlayer->onBufferUnderrun();

    std::unique_lock<std::mutex> lock(messageMutex);

    bool result;

    result = messageSentTrigger.wait_for(lock, WAIT_TIMEOUT, [this] {
        for (auto messageStatus : m_expectedMessages) {
            if (!messageStatus.second) {
                return false;
            }
        }
        return true;
    });

    ASSERT_TRUE(result);
}

/**
 * Test @c onBufferRefilled and expect a PlaybackStutterFinished message
 */

TEST_F(AudioPlayerTest, testOnBufferRefilled) {
    m_expectedMessages.insert({PLAYBACK_STARTED_NAME, false});
    m_expectedMessages.insert({PLAYBACK_NEARLY_FINISHED_NAME, false});
    m_expectedMessages.insert({PLAYBACK_STUTTER_FINISHED_NAME, false});

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            if (!m_mockMediaPlayer->m_stop) {
                std::lock_guard<std::mutex> lock(messageMutex);
                verifyMessage(request, &m_expectedMessages);
                messageSentTrigger.notify_one();
            }
        }));

    sendPlayDirective();

    m_audioPlayer->onBufferRefilled();

    std::unique_lock<std::mutex> lock(messageMutex);

    bool result;

    result = messageSentTrigger.wait_for(lock, WAIT_TIMEOUT, [this] {
        for (auto messageStatus : m_expectedMessages) {
            if (!messageStatus.second) {
                return false;
            }
        }
        return true;
    });

    ASSERT_TRUE(result);
}

/**
 * Test @c cancelDirective
 * Expect the @c handleDirective call to the cancelled directive returns false
 */

TEST_F(AudioPlayerTest, testCancelDirective) {
    sendPlayDirective();

    m_audioPlayer->CapabilityAgent::cancelDirective(MESSAGE_ID_TEST);

    ASSERT_FALSE(m_audioPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST));
}

}  // namespace test
}  // namespace audioPlayer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
