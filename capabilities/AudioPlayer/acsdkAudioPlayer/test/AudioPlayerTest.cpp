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

/// @file AudioPlayerTest.cpp

#include <chrono>
#include <future>
#include <map>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>

#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MockChannelVolumeInterface.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockFocusManager.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/SDKInterfaces/MockPlaybackRouter.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/MediaPlayer/MockMediaPlayer.h>
#include <AVSCommon/Utils/MediaPlayer/PooledMediaPlayerFactory.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/Metrics/MockMetricRecorder.h>
#include <MockCaptionManager.h>

#include "acsdkAudioPlayer/AudioPlayer.h"

namespace alexaClientSDK {
namespace acsdkAudioPlayer {
namespace test {

using namespace ::testing;
using namespace avsCommon::utils::json;
using namespace avsCommon::utils;
using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils::mediaPlayer;
using namespace avsCommon::utils::memory;
using namespace avsCommon::utils::metrics::test;
using namespace captions::test;
using namespace avsCommon::utils::mediaPlayer::test;
using namespace rapidjson;
using MediaPlayerState = avsCommon::utils::mediaPlayer::MediaPlayerState;

/// Plenty of time for a test to complete.
static std::chrono::milliseconds MY_WAIT_TIMEOUT(1000);

/// Default media player state for reporting all playback offsets
static const MediaPlayerState DEFAULT_MEDIA_PLAYER_STATE = {std::chrono::milliseconds(0)};

// Delay to let events happen / threads catch up
static std::chrono::milliseconds EVENT_PROCESS_DELAY(20);

/// The name of the @c FocusManager channel used by the @c AudioPlayer.
static const std::string CHANNEL_NAME(avsCommon::sdkInterfaces::FocusManagerInterface::CONTENT_CHANNEL_NAME);

/// Namespace for AudioPlayer.
static const std::string NAMESPACE_AUDIO_PLAYER("AudioPlayer");

/// Namespace for Another AudioPlayer.
static const std::string NAMESPACE_AUDIO_PLAYER_2("AudioPlayer_2");

/// Name for AudioPlayer Play directive.
static const std::string NAME_PLAY("Play");

/// Name for AudioPlayer Stop directive.
static const std::string NAME_STOP("Stop");

/// Name for AudioPlayer ClearQueue directive.
static const std::string NAME_CLEARQUEUE("ClearQueue");

/// Name for AudioPlayer UpdateProgressReportInterval directive.
static const std::string NAME_UPDATE_PROGRESS_REPORT_INTERVAL("UpdateProgressReportInterval");

/// The @c NamespaceAndName to send to the @c ContextManager.
static const NamespaceAndName NAMESPACE_AND_NAME_PLAYBACK_STATE{NAMESPACE_AUDIO_PLAYER, "PlaybackState"};

/// Message Id for testing.
static const std::string MESSAGE_ID_TEST("MessageId_Test");

/// Another message Id for testing.
static const std::string MESSAGE_ID_TEST_2("MessageId_Test2");

/// Another message Id for testing.
static const std::string MESSAGE_ID_TEST_3("MessageId_Test3");

/// PlayRequestId for testing.
static const std::string PLAY_REQUEST_ID_TEST("PlayRequestId_Test");

/// Context ID for testing
static const std::string CONTEXT_ID_TEST("ContextId_Test");

/// Context ID for testing
static const std::string CONTEXT_ID_TEST_2("ContextId_Test2");

/// Context ID for testing
static const std::string CONTEXT_ID_TEST_3("ContextId_Test3");

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

/// REPLACE_ALL playBehavior.
static const std::string NAME_REPLACE_ALL("REPLACE_ALL");

/// CLEAR_ALL clearBehavior.
static const std::string NAME_CLEAR_ALL("CLEAR_ALL");

/// audioItemId for testing.
static const std::string AUDIO_ITEM_ID_1("testID1");
static const std::string AUDIO_ITEM_ID_2("testID2");

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

/// The offset in milliseconds returned by the mock media player slightly before the progressReportDelayInMilliseconds.
static const long OFFSET_IN_MILLISECONDS_BEFORE_PROGRESS_REPORT_DELAY{PROGRESS_REPORT_DELAY - 1};

/// The offset in milliseconds returned by the mock media player slightly before the progressReportDelayInMilliseconds.
static const long OFFSET_IN_MILLISECONDS_AFTER_PROGRESS_REPORT_DELAY{PROGRESS_REPORT_DELAY + 1};

/// The offset in milliseconds returned by the mock media player slightly before the
/// progressReportIntervalInMilliseconds.
static const long OFFSET_IN_MILLISECONDS_BEFORE_PROGRESS_REPORT_INTERVAL{PROGRESS_REPORT_INTERVAL - 1};

/// The offset in milliseconds returned by the mock media player slightly before the
/// progressReportIntervalInMilliseconds.
static const long OFFSET_IN_MILLISECONDS_AFTER_PROGRESS_REPORT_INTERVAL{PROGRESS_REPORT_INTERVAL + 1};

/// The time that must elapse in order to get 2.5 interval periods
static const std::chrono::milliseconds TIME_FOR_TWO_AND_A_HALF_INTERVAL_PERIODS{
    std::chrono::milliseconds((2 * PROGRESS_REPORT_INTERVAL) + (PROGRESS_REPORT_INTERVAL / 2))};

/// The time to wait before sending 'onTags()' after the last send.
static const long METADATA_EVENT_DELAY{1001};

static const std::string CAPTION_CONTENT_SAMPLE =
    "WEBVTT\\n"
    "\\n"
    "1\\n"
    "00:00.000 --> 00:01.260\\n"
    "The time is 2:17 PM.";

/// A playRequestor object with type "ALERT"
static const std::string PLAY_REQUESTOR_TYPE_ALERT{"ALERT"};

/// A playRequestor object id.
static const std::string PLAY_REQUESTOR_ID{"12345678"};

/// Payloads for testing.
static std::string createEnqueuePayloadTest(long offsetInMilliseconds, const std::string& audioId = AUDIO_ITEM_ID_1) {
    // clang-format off
    const std::string ENQUEUE_PAYLOAD_TEST =
        "{"
            "\"playBehavior\":\"" + NAME_ENQUEUE + "\","
            "\"audioItem\": {"
                "\"audioItemId\":\"" + audioId + "\","
                "\"stream\": {"
                    "\"url\":\"" + URL_TEST + "\","
                    "\"streamFormat\":\"" + FORMAT_TEST + "\","
                    "\"offsetInMilliseconds\":" + std::to_string(offsetInMilliseconds) + ","
                    "\"expiryTime\":\"" + EXPIRY_TEST + "\","
                    "\"progressReport\": {"
                        "\"progressReportDelayInMilliseconds\":" + std::to_string(PROGRESS_REPORT_DELAY) + ","
                        "\"progressReportIntervalInMilliseconds\":" + std::to_string(PROGRESS_REPORT_INTERVAL) +
                    "},"
                    "\"caption\": {"
                        "\"content\":\"" + CAPTION_CONTENT_SAMPLE + "\","
                        "\"type\":\"WEBVTT\""
                    "},"
                    "\"token\":\"" + TOKEN_TEST + "\","
                    "\"expectedPreviousToken\":\"\""
                "}"
            "}"
        "}";
    // clang-format on

    return ENQUEUE_PAYLOAD_TEST;
}

// clang-format off
static const std::string REPLACE_ALL_PAYLOAD_TEST =
"{"
    "\"playBehavior\":\"" + NAME_REPLACE_ALL + "\","
    "\"audioItem\": {"
        "\"audioItemId\":\"" + AUDIO_ITEM_ID_2 + "\","
        "\"stream\": {"
            "\"url\":\"" + URL_TEST + "\","
            "\"streamFormat\":\"" + FORMAT_TEST + "\","
            "\"offsetInMilliseconds\":" + std::to_string(OFFSET_IN_MILLISECONDS_TEST) + ","
            "\"expiryTime\":\"" + EXPIRY_TEST + "\","
            "\"progressReport\": {"
                "\"progressReportDelayInMilliseconds\":" + std::to_string(PROGRESS_REPORT_DELAY) + ","
                "\"progressReportIntervalInMilliseconds\":" + std::to_string(PROGRESS_REPORT_INTERVAL) +
            "},"
            "\"caption\": {"
                "\"content\":\"" + CAPTION_CONTENT_SAMPLE + "\","
                "\"type\":\"WEBVTT\""
            "},"
            "\"token\":\"" + TOKEN_TEST + "\","
            "\"expectedPreviousToken\":\"\""
        "}"
    "}"
"}";
// clang-format on

static std::string createPayloadWithEndOffset(
    long offset,
    long endOffset,
    const std::string& audioId = AUDIO_ITEM_ID_1) {
    // clang-format off
    static const std::string END_OFFSET_PAYLOAD_TEST =
    "{"
        "\"playBehavior\":\"" + NAME_REPLACE_ALL + "\","
        "\"audioItem\": {"
            "\"audioItemId\":\"" + audioId + "\","
            "\"stream\": {"
                "\"url\":\"" + URL_TEST + "\","
                "\"streamFormat\":\"" + FORMAT_TEST + "\","
                "\"offsetInMilliseconds\":" + std::to_string(offset) + ","
                "\"endOffsetInMilliseconds\":" + std::to_string(endOffset) + ","
                "\"expiryTime\":\"" + EXPIRY_TEST + "\","
                "\"progressReport\": {"
                    "\"progressReportDelayInMilliseconds\":" + std::to_string(PROGRESS_REPORT_DELAY) + ","
                    "\"progressReportIntervalInMilliseconds\":" + std::to_string(PROGRESS_REPORT_INTERVAL) +
                "},"
                "\"caption\": {"
                    "\"content\":\"" + CAPTION_CONTENT_SAMPLE + "\","
                    "\"type\":\"WEBVTT\""
                "},"
                "\"token\":\"" + TOKEN_TEST + "\","
                "\"expectedPreviousToken\":\"\""
            "}"
        "}"
    "}";
    // clang-format on

    return END_OFFSET_PAYLOAD_TEST;
}

// clang-format off
static const std::string PLAY_REQUESTOR_PAYLOAD_TEST =
"{"
    "\"playBehavior\":\"" + NAME_REPLACE_ALL + "\","
    "\"playRequestor\": {"
        "\"type\":\"" + PLAY_REQUESTOR_TYPE_ALERT + "\","
        "\"id\":\"" + PLAY_REQUESTOR_ID + "\""
    "},"
    "\"audioItem\": {"
        "\"audioItemId\":\"" + AUDIO_ITEM_ID_2 + "\","
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

/// Offset JSON key.
static const std::string SEEK_END_OFFSET_KEY = "seekEndOffsetInMilliseconds";

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

/// UPDATE_PROGRESS_REPORT_INTERVAL payload for testing.
// clang-format off
static const std::string UPDATE_PROGRESS_REPORT_INTERVAL_PAYLOAD_TEST =
    "{"
        "\"progressReportIntervalInMilliseconds\": 500"
    "}";
// clang-format on

/// JSON key for the event section of a message.
static const std::string MESSAGE_EVENT_KEY = "event";

/// JSON key for the header section of a message.
static const std::string MESSAGE_HEADER_KEY = "header";

/// JSON key for the name section of a message.
static const std::string MESSAGE_NAME_KEY = "name";

/// JSON key for the token section of a message.
static const std::string MESSAGE_TOKEN_KEY = "token";

/// JSON key for the payload section of a message.
static const std::string MESSAGE_PAYLOAD_KEY = "payload";

/// JSON key for the metadata section of a message.
static const std::string MESSAGE_METADATA_KEY = "metadata";

/// JSON key for "string" type field in metadata section of StreamMetadataExtracted event.
static const std::string MESSAGE_METADATA_STRING_KEY = "StringKey";

/// JSON key for "string" type field in metadata section of StreamMetadataExtracted event.  On whitelist
static const std::string MESSAGE_METADATA_STRING_KEY_WL = "Title";

/// JSON value for "string" type field in metadata section of StreamMetadataExtracted event.
static const std::string MESSAGE_METADATA_STRING_VALUE = "StringValue";

/// JSON value for alternate "string" type field in metadata section of StreamMetadataExtracted event.
static const std::string MESSAGE_METADATA_STRING_VALUE_ALT = "StringValue2";

/// JSON key for "uint" type field in metadata section of StreamMetadataExtracted event.
static const std::string MESSAGE_METADATA_UINT_KEY = "UintKey";

/// JSON value for "uint" type field in metadata section of StreamMetadataExtracted event.
static const std::string MESSAGE_METADATA_UINT_VALUE = "12345";

/// JSON key for "int" type field in metadata section of StreamMetadataExtracted event.
static const std::string MESSAGE_METADATA_INT_KEY = "IntKey";

/// JSON value for "int" type field in metadata section of StreamMetadataExtracted event.
static const std::string MESSAGE_METADATA_INT_VALUE = "67890";

/// JSON key for "double" type field in metadata section of StreamMetadataExtracted event.
static const std::string MESSAGE_METADATA_DOUBLE_KEY = "DoubleKey";

/// JSON value for "double" type field in metadata section of StreamMetadataExtracted event.
static const std::string MESSAGE_METADATA_DOUBLE_VALUE = "3.14";

/// JSON key for "boolean" type field in metadata section of StreamMetadataExtracted event.
static const std::string MESSAGE_METADATA_BOOLEAN_KEY = "BooleanKey";

/// JSON value for "boolean" type field in metadata section of StreamMetadataExtracted event.
static const std::string MESSAGE_METADATA_BOOLEAN_VALUE = "true";

/// JSON key for the playbackAttributes section of a message.
static const std::string MESSAGE_PLAYBACK_ATTRIBUTES_KEY = "playbackAttributes";

/// JSON key for "name" field in playbackAttributes section of message.
static const std::string MESSAGE_PLAYBACK_ATTRIBUTES_NAME_KEY = "name";

/// JSON value for "name" field in playbackAttributes section of message.
static const std::string MESSAGE_PLAYBACK_ATTRIBUTES_NAME_VALUE = "STREAM_NAME_ABSENT";

/// JSON key for "codec" field in playbackAttributes section of message.
static const std::string MESSAGE_PLAYBACK_ATTRIBUTES_CODEC_KEY = "codec";

/// JSON value for "codec" field in playbackAttributes section of message.
static const std::string MESSAGE_PLAYBACK_ATTRIBUTES_CODEC_VALUE = "opus";

/// JSON key for "samplingRateInHertz" field in playbackAttributes section of message.
static const std::string MESSAGE_PLAYBACK_ATTRIBUTES_SAMPLING_RATE_KEY = "samplingRateInHertz";

/// JSON value for "samplingRateInHertz" field in playbackAttributes section of message.
static const long MESSAGE_PLAYBACK_ATTRIBUTES_SAMPLING_RATE_VALUE = 48000;

/// JSON key for "dataRateInBitsPerSecond" field in playbackAttributes section of message.
static const std::string MESSAGE_PLAYBACK_ATTRIBUTES_BITRATE_KEY = "dataRateInBitsPerSecond";

/// JSON value for "dataRateInBitsPerSecond" field in playbackAttributes section of message.
static const long MESSAGE_PLAYBACK_ATTRIBUTES_BITRATE_VALUE = 49000;

/// JSON key for the playbackReports section of a message.
static const std::string MESSAGE_PLAYBACK_REPORTS_KEY = "playbackReports";

/// JSON key for "startOffsetInMilliseconds" field in playbackReports section of message.
static const std::string MESSAGE_PLAYBACK_REPORTS_START_OFFSET_KEY = "startOffsetInMilliseconds";

/// JSON value for "startOffsetInMilliseconds" field in playbackReports section of message.
static const long MESSAGE_PLAYBACK_REPORTS_START_OFFSET_VALUE = 0;

/// JSON key for "endOffsetInMilliseconds" field in playbackReports section of message.
static const std::string MESSAGE_PLAYBACK_REPORTS_END_OFFSET_KEY = "endOffsetInMilliseconds";

/// JSON value for "endOffsetInMilliseconds" field in playbackReports section of message.
static const long MESSAGE_PLAYBACK_REPORTS_END_OFFSET_VALUE = 10000;

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

/// Name of ProgressReportIntervalUpdated event
static const std::string PROGRESS_REPORT_INTERVAL_UPDATED_NAME = "ProgressReportIntervalUpdated";

/// Name of StreamMetadataExtracted event
static const std::string STREAM_METADATA_EXTRACTED_NAME = "StreamMetadataExtracted";

/// Name of SeekComplete event
static const std::string SEEK_COMPLETE_NAME = "PlaybackSeeked";

/// String to identify log entries originating from this file.
static const std::string TAG("AudioPlayerTest");

/// Fingerprint for media player.
static const Fingerprint FINGERPRINT = {"com.audioplayer.test", "DEBUG", "0001"};

/// Key for "fingerprint" in AudioPlayer configurations.
static const std::string FINGERPRINT_KEY = "fingerprint";

/// JSON key for "package" in fingerprint configuration.
static const std::string FINGERPRINT_PACKAGE_KEY = "package";

/// JSON key for "buildType" in fingerprint configuration.
static const std::string FINGERPRINT_BUILD_TYPE_KEY = "buildType";

/// JSON key for "versionNumber" in fingerprint configuration.
static const std::string FINGERPRINT_VERSION_NUMBER_KEY = "versionNumber";

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

class TestAudioPlayerObserver : public acsdkAudioPlayerInterfaces::AudioPlayerObserverInterface {
public:
    TestAudioPlayerObserver() : m_state{PlayerActivity::IDLE} {
    }
    bool waitFor(PlayerActivity activity, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_conditionVariable.wait_for(lock, timeout, [this, activity] { return m_state == activity; });
    }
    void onPlayerActivityChanged(avsCommon::avs::PlayerActivity state, const Context& context) override {
        ACSDK_DEBUG(
            LX("onPlayerActivityChanged")
                .d("state", state)
                .d("audioItemId", context.audioItemId)
                .d("offsetInMs", std::chrono::duration_cast<std::chrono::milliseconds>(context.offset).count()));
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_state = state;
            m_playRequestor = context.playRequestor;
        }
        m_conditionVariable.notify_all();
    }
    PlayRequestor getPlayRequestorObject() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_playRequestor;
    }

private:
    PlayerActivity m_state;
    PlayRequestor m_playRequestor;
    std::mutex m_mutex;
    std::condition_variable m_conditionVariable;
};

class AudioPlayerTest : public ::testing::Test {
public:
    AudioPlayerTest();

    void SetUp() override;
    void TearDown() override;
    void reSetUp(int numberOfPlayers);

    /// @c MediaPlayerFactory to generate MockMediaPlayers for testing;
    std::unique_ptr<alexaClientSDK::mediaPlayer::PooledMediaPlayerFactory> m_mockFactory;

    /// @c AudioPlayer to test
    std::shared_ptr<AudioPlayer> m_audioPlayer;

    /// @c A test observer to wait for @c AudioPlayer state changes
    std::shared_ptr<TestAudioPlayerObserver> m_testAudioPlayerObserver;

    /// Player to send the audio to.
    std::shared_ptr<MockMediaPlayer> m_mockMediaPlayer;

    /// Another Player to send the audio to.
    std::shared_ptr<MockMediaPlayer> m_mockMediaPlayerTrack2;

    /// Another Player to send the audio to.
    std::shared_ptr<MockMediaPlayer> m_mockMediaPlayerTrack3;

    /// Speaker to send the audio to.
    std::shared_ptr<MockChannelVolumeInterface> m_mockSpeaker;

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

    /// A playback router to notify when @c AudioPlayer becomes active.
    std::shared_ptr<MockPlaybackRouter> m_mockPlaybackRouter;

    /// A mock @c CaptionManager instance to handle captions parsing.
    std::shared_ptr<MockCaptionManager> m_mockCaptionManager;

    /// Attachment manager used to create a reader.
    std::shared_ptr<AttachmentManager> m_attachmentManager;

    /// Map for expected messages testing
    std::map<std::string, int> m_expectedMessages;

    /// Identifier for the currently selected audio source.
    MediaPlayerInterface::SourceId m_sourceId;

    /// The mock @c MetricRecorderInterface
    std::shared_ptr<MockMetricRecorder> m_mockMetricRecorder;

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

    /// Promise to be fulfilled when @c acquireChannel is called.
    std::promise<void> m_wakeReleaseChannelPromise;

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
     *
     * @param offsetInMilliseconds The offset to use in the directive.
     */
    void sendPlayDirective(long offsetInMilliseconds = OFFSET_IN_MILLISECONDS_TEST);

    /**
     * Consolidate code with bad end offset in a play directive.
     * endOffset <= offset
     *
     * @param offset "start" offset in the directive
     * @param endOffset endOffset in the directive
     */
    void badEndOffsetDirective(long offset, long endOffset);

    /**
     * Consolidate code to send Stop directive.
     */
    void sendStopDirective();

    /**
     * Consolidate code to send ClearQueue directive
     */

    void sendClearQueueDirective();

    /**
     * Sends UpdateProgressReportInterval directive.
     */
    void sendUpdateProgressReportIntervalDirective();

    /**
     * Verify that the message name matches the expected name
     *
     * @param request The @c MessageRequest to verify
     * @param expectedName The expected name to find in the json header
     */

    bool verifyMessage(std::shared_ptr<avsCommon::avs::MessageRequest> request, std::string expectedName);

    /**
     * Verify that the sent request matches one in a Map of expectedMessages
     *
     * @param request The @c MessageRequest to verify
     * @param expectedMessages The map of expected messages and a count of how many are seen
     */

    void verifyMessageMap(
        std::shared_ptr<avsCommon::avs::MessageRequest> request,
        std::map<std::string, int>* expectedMessages);

    /**
     * verify that the sent request matches the indexed message in the list
     *
     * @param orderedMessageList The list of expected messages, in order expected
     * @param index The expected message to match
     */
    void verifyMessageOrder2Phase(
        const std::vector<std::string>& orderedMessageList,
        size_t index,
        std::function<void()> trigger1,
        std::function<void()> trigger2);

    /**
     * Verify that the provided state matches the expected state
     *
     * @param jsonState The state to verify
     * @param expectedState The expected state
     */

    void verifyState(const std::string& providedState, const std::string& expectedState);

    /**
     * Verify that the message name matches the expected name and also verify expected tags.
     *
     * @param request The @c MessageRequest to verify
     * @param expectedMessages Map of expected tags and count of how many are seen.
     */
    void verifyTags(
        std::shared_ptr<avsCommon::avs::MessageRequest> request,
        std::map<std::string, int>* expectedMessages,
        bool validateBoolean = true);

    /**
     * Extracts playback attributes from message for verification.
     *
     * @param request The @c MessageRequest to extract.
     * @param actualPlaybackAttributes The @c PlaybackAttributes extracted.
     */
    void extractPlaybackAttributes(
        std::shared_ptr<avsCommon::avs::MessageRequest> request,
        PlaybackAttributes* actualPlaybackAttributes);

    /**
     * Extracts playback reports from message for verification.
     *
     * @param request The @c MessageRequest to extract.
     * @param actualPlaybackReports Pointer to list of @c PlaybackReport extracted.
     */
    void extractPlaybackReports(
        std::shared_ptr<avsCommon::avs::MessageRequest> request,
        std::vector<PlaybackReport>* actualPlaybackReports);

    /**
     * Extracts mediaPlayerState from playback event for verification.
     *
     * @param request The @c MessageRequest to extract.
     * @param actualPlaybackAttributes The @c PlaybackAttributes extracted.
     */
    bool extractMediaPlayerState(
        std::shared_ptr<avsCommon::avs::MessageRequest> request,
        const std::string& expectedState,
        MediaPlayerState* playerState);

    /**
     * Run through test of playing, enqueuing, finish, play
     */
    void testPlayEnqueueFinishPlay();

    /// General purpose mutex.
    std::mutex m_mutex;

    /// Condition variable to wake on a message being sent.
    std::condition_variable m_messageSentTrigger;

    /// Condition variable to wake on MediaPlayer calls. This is used when a MediaPlayer call is expected to occur
    /// without a corresponding change in PlayerActivity.
    std::condition_variable m_mediaPlayerCallTrigger;
};

AudioPlayerTest::AudioPlayerTest() :
        m_wakeSetStatePromise{},
        m_wakeSetStateFuture{m_wakeSetStatePromise.get_future()},
        m_wakeAcquireChannelPromise{},
        m_wakeAcquireChannelFuture{m_wakeAcquireChannelPromise.get_future()},
        m_wakeReleaseChannelPromise{},
        m_wakeReleaseChannelFuture{m_wakeReleaseChannelPromise.get_future()},
        m_wakeSendMessagePromise{},
        m_wakeSendMessageFuture{m_wakeSendMessagePromise.get_future()} {
}

void AudioPlayerTest::SetUp() {
    MockMediaPlayer::enableConcurrentMediaPlayers();

    m_mockContextManager = std::make_shared<NiceMock<MockContextManager>>();
    m_mockFocusManager = std::make_shared<NiceMock<MockFocusManager>>();
    m_mockMessageSender = std::make_shared<NiceMock<MockMessageSender>>();
    m_mockExceptionSender = std::make_shared<NiceMock<MockExceptionEncounteredSender>>();
    m_attachmentManager = std::make_shared<AttachmentManager>(AttachmentManager::AttachmentType::IN_PROCESS);
    m_mockSpeaker = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    m_mockMediaPlayer = MockMediaPlayer::create();
    m_mockPlaybackRouter = std::make_shared<NiceMock<MockPlaybackRouter>>();
    ASSERT_TRUE(m_mockMediaPlayer);
    m_mockMediaPlayerTrack2 = MockMediaPlayer::create();
    ASSERT_TRUE(m_mockMediaPlayerTrack2);
    m_mockMediaPlayerTrack3 = MockMediaPlayer::create();
    ASSERT_TRUE(m_mockMediaPlayerTrack3);
    std::vector<std::shared_ptr<MediaPlayerInterface>> pool = {
        m_mockMediaPlayer, m_mockMediaPlayerTrack2, m_mockMediaPlayerTrack3};
    m_mockFactory = alexaClientSDK::mediaPlayer::PooledMediaPlayerFactory::create(pool, FINGERPRINT);
    m_mockCaptionManager = std::make_shared<NiceMock<MockCaptionManager>>();
    m_mockMetricRecorder = std::make_shared<NiceMock<MockMetricRecorder>>();
    m_audioPlayer = AudioPlayer::create(
        std::move(m_mockFactory),
        m_mockMessageSender,
        m_mockFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        {m_mockSpeaker},
        m_mockCaptionManager,
        m_mockMetricRecorder);

    ASSERT_TRUE(m_audioPlayer);

    m_testAudioPlayerObserver = std::make_shared<TestAudioPlayerObserver>();
    m_audioPlayer->addObserver(m_testAudioPlayerObserver);
    m_mockDirectiveHandlerResult = std::unique_ptr<MockDirectiveHandlerResult>(new MockDirectiveHandlerResult);
}

void AudioPlayerTest::TearDown() {
    if (m_audioPlayer) {
        m_audioPlayer->shutdown();
    }
    m_mockMediaPlayer->shutdown();
    m_mockMediaPlayerTrack2->shutdown();
    m_mockMediaPlayerTrack3->shutdown();
}

void AudioPlayerTest::reSetUp(int numberOfPlayers) {
    ASSERT_LE(numberOfPlayers, 3);
    ASSERT_GE(numberOfPlayers, 1);
    if (m_audioPlayer) {
        m_audioPlayer->shutdown();
    }

    std::vector<std::shared_ptr<MediaPlayerInterface>> pool;
    switch (numberOfPlayers) {
        case 3:
            pool.push_back(m_mockMediaPlayerTrack3);
        case 2:
            pool.push_back(m_mockMediaPlayerTrack2);
        case 1:
            pool.push_back(m_mockMediaPlayer);
    }
    m_mockFactory = alexaClientSDK::mediaPlayer::PooledMediaPlayerFactory::create(pool);
    m_audioPlayer = AudioPlayer::create(
        std::move(m_mockFactory),
        m_mockMessageSender,
        m_mockFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        {m_mockSpeaker},
        m_mockCaptionManager,
        m_mockMetricRecorder);

    ASSERT_TRUE(m_audioPlayer);

    m_testAudioPlayerObserver = std::make_shared<TestAudioPlayerObserver>();
    m_audioPlayer->addObserver(m_testAudioPlayerObserver);
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
    m_wakeReleaseChannelPromise.set_value();
    return returnValue;
}

void AudioPlayerTest::wakeOnSendMessage() {
    m_wakeSendMessagePromise.set_value();
}

void AudioPlayerTest::sendPlayDirective(long offsetInMilliseconds) {
    auto avsMessageHeader =
        std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_TEST, PLAY_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> playDirective = AVSDirective::create(
        "", avsMessageHeader, createEnqueuePayloadTest(offsetInMilliseconds), m_attachmentManager, CONTEXT_ID_TEST);
    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &AudioPlayerTest::wakeOnAcquireChannel));

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted());

    m_audioPlayer->CapabilityAgent::preHandleDirective(playDirective, std::move(m_mockDirectiveHandlerResult));
    m_mockMediaPlayer->waitUntilNextSetSource(MY_WAIT_TIMEOUT);
    m_audioPlayer->onBufferingComplete(m_mockMediaPlayer->getLatestSourceId(), DEFAULT_MEDIA_PLAYER_STATE);
    m_audioPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_EQ(std::future_status::ready, m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));

    m_audioPlayer->onFocusChanged(FocusState::FOREGROUND, avs::MixingBehavior::PRIMARY);

    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));
}

void AudioPlayerTest::badEndOffsetDirective(long offset, long endOffset) {
    ASSERT_LE(endOffset, offset);
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_TEST_2);

    // End offset less than start offset
    std::string endOffsetPayload = createPayloadWithEndOffset(offset, endOffset);
    std::shared_ptr<AVSDirective> playDirective =
        AVSDirective::create("", avsMessageHeader, endOffsetPayload, m_attachmentManager, CONTEXT_ID_TEST_2);

    EXPECT_CALL(*(m_mockExceptionSender.get()), sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_));

    m_audioPlayer->CapabilityAgent::preHandleDirective(playDirective, std::move(m_mockDirectiveHandlerResult));
}

void AudioPlayerTest::sendStopDirective() {
    auto avsStopMessageHeader =
        std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_STOP, MESSAGE_ID_TEST, PLAY_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> stopDirective =
        AVSDirective::create("", avsStopMessageHeader, EMPTY_PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    m_audioPlayer->CapabilityAgent::preHandleDirective(stopDirective, std::move(m_mockDirectiveHandlerResult));
    m_audioPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

void AudioPlayerTest::sendClearQueueDirective() {
    auto avsClearMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_AUDIO_PLAYER, NAME_CLEARQUEUE, MESSAGE_ID_TEST, PLAY_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> clearQueueDirective =
        AVSDirective::create("", avsClearMessageHeader, CLEAR_ALL_PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    m_audioPlayer->CapabilityAgent::preHandleDirective(clearQueueDirective, std::move(m_mockDirectiveHandlerResult));
    m_audioPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

void AudioPlayerTest::sendUpdateProgressReportIntervalDirective() {
    auto header = std::make_shared<AVSMessageHeader>(
        NAMESPACE_AUDIO_PLAYER, NAME_UPDATE_PROGRESS_REPORT_INTERVAL, MESSAGE_ID_TEST, PLAY_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        "", header, UPDATE_PROGRESS_REPORT_INTERVAL_PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);
    m_audioPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_audioPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

bool AudioPlayerTest::verifyMessage(std::shared_ptr<avsCommon::avs::MessageRequest> request, std::string expectedName) {
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

    return (requestName == expectedName);
}

void AudioPlayerTest::verifyMessageMap(
    std::shared_ptr<avsCommon::avs::MessageRequest> request,
    std::map<std::string, int>* expectedMessages) {
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
        expectedMessages->at(requestName) = expectedMessages->at(requestName) + 1;
    }
}

void AudioPlayerTest::verifyState(const std::string& providedState, const std::string& expectedState) {
    rapidjson::Document providedStateParsed;
    providedStateParsed.Parse(providedState);

    rapidjson::Document expectedStateParsed;
    expectedStateParsed.Parse(expectedState);

    EXPECT_EQ(providedStateParsed, expectedStateParsed);
}

void AudioPlayerTest::verifyTags(
    std::shared_ptr<avsCommon::avs::MessageRequest> request,
    std::map<std::string, int>* expectedMessages,
    bool validateBoolean) {
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
        expectedMessages->at(requestName) = expectedMessages->at(requestName) + 1;
    }

    auto payload = event->value.FindMember(MESSAGE_PAYLOAD_KEY);
    EXPECT_NE(payload, event->value.MemberEnd());

    auto metadata = payload->value.FindMember(MESSAGE_METADATA_KEY);
    if (metadata == payload->value.MemberEnd()) {
        return;
    }

    std::string metadata_string_value;
    jsonUtils::retrieveValue(metadata->value, MESSAGE_METADATA_STRING_KEY, &metadata_string_value);

    if (expectedMessages->find(metadata_string_value) != expectedMessages->end()) {
        expectedMessages->at(metadata_string_value) = expectedMessages->at(metadata_string_value) + 1;
    }

    metadata_string_value = "";
    jsonUtils::retrieveValue(metadata->value, MESSAGE_METADATA_STRING_KEY_WL, &metadata_string_value);

    if (expectedMessages->find(metadata_string_value) != expectedMessages->end()) {
        expectedMessages->at(metadata_string_value) = expectedMessages->at(metadata_string_value) + 1;
    }

    std::string metadata_uint_value;
    jsonUtils::retrieveValue(metadata->value, MESSAGE_METADATA_UINT_KEY, &metadata_uint_value);

    if (expectedMessages->find(metadata_uint_value) != expectedMessages->end()) {
        expectedMessages->at(metadata_uint_value) = expectedMessages->at(metadata_uint_value) + 1;
    }

    std::string metadata_int_value;
    jsonUtils::retrieveValue(metadata->value, MESSAGE_METADATA_INT_KEY, &metadata_int_value);

    if (expectedMessages->find(metadata_int_value) != expectedMessages->end()) {
        expectedMessages->at(metadata_int_value) = expectedMessages->at(metadata_int_value) + 1;
    }

    std::string metadata_double_value;
    jsonUtils::retrieveValue(metadata->value, MESSAGE_METADATA_DOUBLE_KEY, &metadata_double_value);

    if (expectedMessages->find(metadata_double_value) != expectedMessages->end()) {
        expectedMessages->at(metadata_double_value) = expectedMessages->at(metadata_double_value) + 1;
    }

    if (validateBoolean) {
        bool metadata_boolean_value = false;
        jsonUtils::retrieveValue(metadata->value, MESSAGE_METADATA_BOOLEAN_KEY, &metadata_boolean_value);
        ASSERT_TRUE(metadata_boolean_value);
    }
}

void AudioPlayerTest::extractPlaybackAttributes(
    std::shared_ptr<avsCommon::avs::MessageRequest> request,
    PlaybackAttributes* actualPlaybackAttributes) {
    rapidjson::Document document;
    document.Parse(request->getJsonContent().c_str());
    EXPECT_FALSE(document.HasParseError())
        << "rapidjson detected a parsing error at offset:" + std::to_string(document.GetErrorOffset()) +
               ", error message: " + GetParseError_En(document.GetParseError());

    auto event = document.FindMember(MESSAGE_EVENT_KEY);
    EXPECT_NE(event, document.MemberEnd());

    auto payload = event->value.FindMember(MESSAGE_PAYLOAD_KEY);
    EXPECT_NE(payload, event->value.MemberEnd());

    auto playbackAttributes = payload->value.FindMember(MESSAGE_PLAYBACK_ATTRIBUTES_KEY);
    EXPECT_NE(playbackAttributes, payload->value.MemberEnd());

    std::string name;
    jsonUtils::retrieveValue(playbackAttributes->value, MESSAGE_PLAYBACK_ATTRIBUTES_NAME_KEY, &name);
    actualPlaybackAttributes->name = name;

    std::string codec;
    jsonUtils::retrieveValue(playbackAttributes->value, MESSAGE_PLAYBACK_ATTRIBUTES_CODEC_KEY, &codec);
    actualPlaybackAttributes->codec = codec;

    int64_t samplingRate;
    jsonUtils::retrieveValue(playbackAttributes->value, MESSAGE_PLAYBACK_ATTRIBUTES_SAMPLING_RATE_KEY, &samplingRate);
    actualPlaybackAttributes->samplingRateInHertz = (long)samplingRate;

    int64_t bitrate;
    jsonUtils::retrieveValue(playbackAttributes->value, MESSAGE_PLAYBACK_ATTRIBUTES_BITRATE_KEY, &bitrate);
    actualPlaybackAttributes->dataRateInBitsPerSecond = (long)bitrate;
}

void AudioPlayerTest::extractPlaybackReports(
    std::shared_ptr<avsCommon::avs::MessageRequest> request,
    std::vector<PlaybackReport>* actualPlaybackReports) {
    rapidjson::Document document;
    document.Parse(request->getJsonContent().c_str());
    EXPECT_FALSE(document.HasParseError())
        << "rapidjson detected a parsing error at offset:" + std::to_string(document.GetErrorOffset()) +
               ", error message: " + GetParseError_En(document.GetParseError());

    auto event = document.FindMember(MESSAGE_EVENT_KEY);
    EXPECT_NE(event, document.MemberEnd());

    auto payload = event->value.FindMember(MESSAGE_PAYLOAD_KEY);
    EXPECT_NE(payload, event->value.MemberEnd());

    auto playbackReports = payload->value.FindMember(MESSAGE_PLAYBACK_REPORTS_KEY);
    if (playbackReports == payload->value.MemberEnd()) {
        return;
    }

    for (const auto& playbackReport : playbackReports->value.GetArray()) {
        PlaybackReport actualPlaybackReport;

        int64_t startOffset;
        jsonUtils::retrieveValue(playbackReport, MESSAGE_PLAYBACK_REPORTS_START_OFFSET_KEY, &startOffset);
        actualPlaybackReport.startOffset = std::chrono::milliseconds(startOffset);

        int64_t endOffset;
        jsonUtils::retrieveValue(playbackReport, MESSAGE_PLAYBACK_REPORTS_END_OFFSET_KEY, &endOffset);
        actualPlaybackReport.endOffset = std::chrono::milliseconds(endOffset);

        auto playbackAttributes = playbackReport.FindMember(MESSAGE_PLAYBACK_ATTRIBUTES_KEY);
        EXPECT_NE(playbackAttributes, playbackReport.MemberEnd());

        std::string name;
        jsonUtils::retrieveValue(playbackAttributes->value, MESSAGE_PLAYBACK_ATTRIBUTES_NAME_KEY, &name);
        actualPlaybackReport.playbackAttributes.name = name;

        std::string codec;
        jsonUtils::retrieveValue(playbackAttributes->value, MESSAGE_PLAYBACK_ATTRIBUTES_CODEC_KEY, &codec);
        actualPlaybackReport.playbackAttributes.codec = codec;

        int64_t samplingRate;
        jsonUtils::retrieveValue(
            playbackAttributes->value, MESSAGE_PLAYBACK_ATTRIBUTES_SAMPLING_RATE_KEY, &samplingRate);
        actualPlaybackReport.playbackAttributes.samplingRateInHertz = (long)samplingRate;

        int64_t bitrate;
        jsonUtils::retrieveValue(playbackAttributes->value, MESSAGE_PLAYBACK_ATTRIBUTES_BITRATE_KEY, &bitrate);
        actualPlaybackReport.playbackAttributes.dataRateInBitsPerSecond = (long)bitrate;

        actualPlaybackReports->push_back(actualPlaybackReport);
    }
}

bool AudioPlayerTest::extractMediaPlayerState(
    std::shared_ptr<avsCommon::avs::MessageRequest> request,
    const std::string& expectedState,
    MediaPlayerState* playerState) {
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

    if (expectedState == requestName) {
        auto payload = event->value.FindMember(MESSAGE_PAYLOAD_KEY);
        EXPECT_NE(payload, event->value.MemberEnd());

        uint64_t offset = 0;
        // Commented out untile AudioPlayer 1.6, when this will be correct
        // jsonUtils::retrieveValue(payload->value, SEEK_END_OFFSET_KEY, &offset);
        jsonUtils::retrieveValue(payload->value, OFFSET_KEY, &offset);
        playerState->offset = std::chrono::milliseconds(offset);
        return true;
    }
    return false;
}

/**
 * Test create() with nullptrs
 */

TEST_F(AudioPlayerTest, test_createWithNullPointers) {
    std::shared_ptr<AudioPlayer> testAudioPlayer;
    std::vector<std::shared_ptr<MediaPlayerInterface>> pool = {
        m_mockMediaPlayer, m_mockMediaPlayerTrack2, m_mockMediaPlayerTrack3};

    testAudioPlayer = AudioPlayer::create(
        nullptr,
        m_mockMessageSender,
        m_mockFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        {m_mockSpeaker},
        m_mockCaptionManager,
        m_mockMetricRecorder);
    EXPECT_EQ(testAudioPlayer, nullptr);

    m_mockFactory = alexaClientSDK::mediaPlayer::PooledMediaPlayerFactory::create(pool);
    testAudioPlayer = AudioPlayer::create(
        std::move(m_mockFactory),
        nullptr,
        m_mockFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        {m_mockSpeaker},
        m_mockCaptionManager,
        m_mockMetricRecorder);
    EXPECT_EQ(testAudioPlayer, nullptr);

    m_mockFactory = alexaClientSDK::mediaPlayer::PooledMediaPlayerFactory::create(pool);
    testAudioPlayer = AudioPlayer::create(
        std::move(m_mockFactory),
        m_mockMessageSender,
        nullptr,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        {m_mockSpeaker},
        m_mockCaptionManager,
        m_mockMetricRecorder);
    EXPECT_EQ(testAudioPlayer, nullptr);

    m_mockFactory = alexaClientSDK::mediaPlayer::PooledMediaPlayerFactory::create(pool);
    testAudioPlayer = AudioPlayer::create(
        std::move(m_mockFactory),
        m_mockMessageSender,
        m_mockFocusManager,
        nullptr,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        {m_mockSpeaker},
        m_mockCaptionManager,
        m_mockMetricRecorder);
    EXPECT_EQ(testAudioPlayer, nullptr);

    m_mockFactory = alexaClientSDK::mediaPlayer::PooledMediaPlayerFactory::create(pool);
    testAudioPlayer = AudioPlayer::create(
        std::move(m_mockFactory),
        m_mockMessageSender,
        m_mockFocusManager,
        m_mockContextManager,
        nullptr,
        m_mockPlaybackRouter,
        {m_mockSpeaker},
        m_mockCaptionManager,
        m_mockMetricRecorder);
    EXPECT_EQ(testAudioPlayer, nullptr);

    m_mockFactory = alexaClientSDK::mediaPlayer::PooledMediaPlayerFactory::create(pool);
    testAudioPlayer = AudioPlayer::create(
        std::move(m_mockFactory),
        m_mockMessageSender,
        m_mockFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        nullptr,
        {m_mockSpeaker},
        m_mockCaptionManager,
        m_mockMetricRecorder);
    EXPECT_EQ(testAudioPlayer, nullptr);

    m_mockFactory = alexaClientSDK::mediaPlayer::PooledMediaPlayerFactory::create(pool);
    testAudioPlayer = AudioPlayer::create(
        std::move(m_mockFactory),
        m_mockMessageSender,
        m_mockFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        {},
        m_mockCaptionManager,
        m_mockMetricRecorder);
    EXPECT_EQ(testAudioPlayer, nullptr);
}

/**
 * Test transition from Idle to Playing
 */

TEST_F(AudioPlayerTest, test_transitionFromIdleToPlaying) {
    EXPECT_CALL(*(m_mockMediaPlayer.get()), play(_)).Times(AtLeast(1));
    sendPlayDirective();
}

/**
 * Test transition from Playing to Stopped with Stop Directive
 */

TEST_F(AudioPlayerTest, test_transitionFromPlayingToStopped) {
    sendPlayDirective();

    EXPECT_CALL(*(m_mockMediaPlayer.get()), stop(_)).Times(AtLeast(1));

    // now send Stop directive
    sendStopDirective();
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::STOPPED, MY_WAIT_TIMEOUT));
}

/**
 * Test transition from Playing to Stopped with ClearQueue.CLEAR_ALL Directive
 */

TEST_F(AudioPlayerTest, test_transitionFromPlayingToStoppedWithClear) {
    sendPlayDirective();

    EXPECT_CALL(*(m_mockMediaPlayer.get()), stop(_)).Times(AtLeast(1));

    sendClearQueueDirective();

    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::STOPPED, MY_WAIT_TIMEOUT));
}

/**
 * Test transition from Stopped to Playing after issuing second Play directive
 */

TEST_F(AudioPlayerTest, test_transitionFromStoppedToPlaying) {
    sendPlayDirective();

    EXPECT_CALL(*(m_mockMediaPlayer.get()), stop(_)).Times(AtLeast(1));
    sendClearQueueDirective();
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::STOPPED, MY_WAIT_TIMEOUT));
    m_audioPlayer->onFocusChanged(FocusState::NONE, MixingBehavior::MUST_STOP);

    // Verify previous media player unused
    EXPECT_CALL(*(m_mockMediaPlayer.get()), play(_)).Times(0);
    EXPECT_CALL(*(m_mockMediaPlayerTrack2.get()), play(_)).Times(AtLeast(1));
    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _)).Times(1).WillOnce(Return(true));
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_TEST_2);
    std::shared_ptr<AVSDirective> playDirective = AVSDirective::create(
        "",
        avsMessageHeader,
        createEnqueuePayloadTest(OFFSET_IN_MILLISECONDS_TEST, AUDIO_ITEM_ID_2),
        m_attachmentManager,
        CONTEXT_ID_TEST_2);
    m_audioPlayer->CapabilityAgent::preHandleDirective(playDirective, std::move(m_mockDirectiveHandlerResult));
    m_audioPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST_2);
    m_audioPlayer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));
}

/**
 * Test transition from Stopped to Playing after issuing second Play directive, resuming
 */

TEST_F(AudioPlayerTest, testTransitionFromStoppedToResumePlaying) {
    // send a play directive
    sendPlayDirective();

    // send clear Queue directive
    EXPECT_CALL(*(m_mockMediaPlayer.get()), stop(_)).Times(AtLeast(1));
    sendClearQueueDirective();

    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::STOPPED, MY_WAIT_TIMEOUT));
    m_audioPlayer->onFocusChanged(FocusState::NONE, MixingBehavior::MUST_STOP);

    // re-init attachment manager - resuming is not normally done with attachments...
    m_attachmentManager = std::make_shared<AttachmentManager>(AttachmentManager::AttachmentType::IN_PROCESS);
    m_mockMediaPlayer->resetWaitTimer();

    EXPECT_CALL(*(m_mockMediaPlayerTrack2.get()), play(_)).Times(1);
    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _)).Times(1).WillOnce(Return(true));

    // Enqueue next track
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_TEST_2);

    std::shared_ptr<AVSDirective> playDirective = AVSDirective::create(
        "",
        avsMessageHeader,
        createEnqueuePayloadTest(OFFSET_IN_MILLISECONDS_TEST),
        m_attachmentManager,
        CONTEXT_ID_TEST);
    m_audioPlayer->CapabilityAgent::preHandleDirective(playDirective, std::move(m_mockDirectiveHandlerResult));
    m_audioPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST_2);
    m_audioPlayer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));
}

/**
 * Test transition to next track, when next track has been enqueued
 */

TEST_F(AudioPlayerTest, testTransitionFromPlayingToPlayingNextEnqueuedTrack) {
    // send a play directive
    sendPlayDirective();
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));

    std::this_thread::sleep_for(std::chrono::milliseconds(EVENT_PROCESS_DELAY));

    // Verify that the media player for the enqueued track is used
    EXPECT_CALL(*(m_mockMediaPlayer.get()), play(_)).Times(0);
    EXPECT_CALL(*(m_mockMediaPlayerTrack2.get()), play(_)).Times(AtLeast(1));
    EXPECT_CALL(*(m_mockMediaPlayerTrack3.get()), play(_)).Times(0);

    // Enqueue next track
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_TEST_2);
    std::shared_ptr<AVSDirective> playDirective = AVSDirective::create(
        "",
        avsMessageHeader,
        createEnqueuePayloadTest(OFFSET_IN_MILLISECONDS_TEST, AUDIO_ITEM_ID_2),
        m_attachmentManager,
        CONTEXT_ID_TEST_2);

    m_audioPlayer->CapabilityAgent::preHandleDirective(playDirective, std::move(m_mockDirectiveHandlerResult));
    m_audioPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST_2);

    std::this_thread::sleep_for(std::chrono::milliseconds(EVENT_PROCESS_DELAY));

    // Now, send a REPLACE_ALL directive that has the same ID as the enqueued track
    avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_TEST_3);
    playDirective =
        AVSDirective::create("", avsMessageHeader, REPLACE_ALL_PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST_3);
    m_audioPlayer->CapabilityAgent::preHandleDirective(playDirective, std::move(m_mockDirectiveHandlerResult));
    m_audioPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST_3);

    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::STOPPED, MY_WAIT_TIMEOUT));
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));
}

/**
 * Test transition from Playing to Paused when focus changes to Dialog channel
 */

TEST_F(AudioPlayerTest, test_transitionFromPlayingToPaused) {
    sendPlayDirective();

    EXPECT_CALL(*(m_mockMediaPlayer.get()), pause(_)).Times(AtLeast(1));

    // simulate focus change
    m_audioPlayer->onFocusChanged(FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PAUSED, MY_WAIT_TIMEOUT));
}

/**
 * Test transition from Paused to Stopped on ClearQueue.CLEAR_ALL directive
 */
TEST_F(AudioPlayerTest, test_transitionFromPausedToStopped) {
    sendPlayDirective();

    EXPECT_CALL(*(m_mockMediaPlayer.get()), stop(_)).Times(AtLeast(1));

    // simulate focus change in order to pause
    m_audioPlayer->onFocusChanged(FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PAUSED, MY_WAIT_TIMEOUT));

    // clear queue directive must stop music
    sendClearQueueDirective();
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::STOPPED, MY_WAIT_TIMEOUT));
}

/**
 * Test transition from Paused to Playing after resume
 */

TEST_F(AudioPlayerTest, test_resumeAfterPaused) {
    sendPlayDirective();

    EXPECT_CALL(*(m_mockMediaPlayer.get()), stop(_)).Times(AtLeast(1));

    // simulate focus change in order to pause
    m_audioPlayer->onFocusChanged(FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PAUSED, MY_WAIT_TIMEOUT));

    // verify mediaplayer resumes on foreground
    EXPECT_CALL(*(m_mockMediaPlayer.get()), resume(_)).Times(AtLeast(1));
    m_audioPlayer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));
}

/**
 * Test @c provideState while IDLE
 */

TEST_F(AudioPlayerTest, test_callingProvideStateWhenIdle) {
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

    m_audioPlayer->provideState(NAMESPACE_AND_NAME_PLAYBACK_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test @c onPlaybackError and expect a PlaybackFailed message
 */

TEST_F(AudioPlayerTest, test_onPlaybackError) {
    m_expectedMessages.insert({PLAYBACK_STARTED_NAME, 0});
    m_expectedMessages.insert({PLAYBACK_FAILED_NAME, 0});
    m_expectedMessages.insert({PLAYBACK_STOPPED_NAME, 0});

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            std::lock_guard<std::mutex> lock(m_mutex);
            verifyMessageMap(request, &m_expectedMessages);
            m_messageSentTrigger.notify_one();
        }));

    sendPlayDirective();

    m_audioPlayer->onPlaybackError(
        m_mockMediaPlayer->getCurrentSourceId(),
        ErrorType::MEDIA_ERROR_UNKNOWN,
        "TEST_ERROR",
        DEFAULT_MEDIA_PLAYER_STATE);

    std::unique_lock<std::mutex> lock(m_mutex);

    bool result;

    result = m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [this] {
        for (auto messageStatus : m_expectedMessages) {
            if (messageStatus.second == 0) {
                return false;
            }
        }
        return true;
    });
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::STOPPED, MY_WAIT_TIMEOUT));
    ASSERT_TRUE(result);
}

/**
 * Test @c onPlaybackError and expect a PlaybackFailed message
 */

TEST_F(AudioPlayerTest, test_onPlaybackError_Stopped) {
    m_expectedMessages.insert({PLAYBACK_STARTED_NAME, 0});
    m_expectedMessages.insert({PLAYBACK_STOPPED_NAME, 0});
    // we don't want to see this, as it shouldn't be send when stopped
    m_expectedMessages.insert({PLAYBACK_FAILED_NAME, -1});

    // Return offset that's greater than 500ms so that no PlaybackFailed event is sent
    EXPECT_CALL(*(m_mockMediaPlayer.get()), getOffset(_)).WillRepeatedly(Return(std::chrono::milliseconds(600)));

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            std::lock_guard<std::mutex> lock(m_mutex);
            verifyMessageMap(request, &m_expectedMessages);
            m_messageSentTrigger.notify_one();
        }));

    sendPlayDirective();
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));

    sendStopDirective();
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::STOPPED, MY_WAIT_TIMEOUT));

    m_audioPlayer->onPlaybackError(
        m_mockMediaPlayer->getCurrentSourceId(),
        ErrorType::MEDIA_ERROR_UNKNOWN,
        "TEST_ERROR",
        DEFAULT_MEDIA_PLAYER_STATE);

    std::unique_lock<std::mutex> lock(m_mutex);

    bool result;

    result = m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [this] {
        for (auto messageStatus : m_expectedMessages) {
            if (messageStatus.second == 0) {
                return false;
            }
        }
        return true;
    });
    ASSERT_TRUE(result);
}

/**
 * Test @c onPlaybackError during pre-buffering
 */

TEST_F(AudioPlayerTest, testPrebufferOnPlaybackError) {
    m_expectedMessages.insert({PLAYBACK_FAILED_NAME, 0});

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            std::lock_guard<std::mutex> lock(m_mutex);
            verifyMessageMap(request, &m_expectedMessages);
            m_messageSentTrigger.notify_one();
        }));

    sendPlayDirective();
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));

    bool result;

    std::this_thread::sleep_for(std::chrono::milliseconds(EVENT_PROCESS_DELAY));

    // Verify that the media player for the enqueued track is used
    EXPECT_CALL(*(m_mockMediaPlayer.get()), play(_)).Times(0);
    // zero because we should error out before play is called
    EXPECT_CALL(*(m_mockMediaPlayerTrack2.get()), play(_)).Times(0);

    // Enqueue next track
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_TEST_2);
    std::shared_ptr<AVSDirective> playDirective = AVSDirective::create(
        "",
        avsMessageHeader,
        createEnqueuePayloadTest(OFFSET_IN_MILLISECONDS_TEST, AUDIO_ITEM_ID_2),
        m_attachmentManager,
        CONTEXT_ID_TEST_2);
    m_audioPlayer->CapabilityAgent::preHandleDirective(playDirective, std::move(m_mockDirectiveHandlerResult));
    std::this_thread::sleep_for(std::chrono::milliseconds(EVENT_PROCESS_DELAY));

    // Send error for track 2 while track 1 is playing
    m_audioPlayer->onPlaybackError(
        m_mockMediaPlayerTrack2->getSourceId(),
        ErrorType::MEDIA_ERROR_UNKNOWN,
        "TEST_ERROR",
        DEFAULT_MEDIA_PLAYER_STATE);

    // now 'play' track 2
    m_audioPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST_2);
    std::this_thread::sleep_for(std::chrono::milliseconds(EVENT_PROCESS_DELAY));

    {
        // verify error not sent
        std::unique_lock<std::mutex> lock(m_mutex);

        result = m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [this] {
            for (auto messageStatus : m_expectedMessages) {
                if (messageStatus.second == 0) {
                    return false;
                }
            }
            return true;
        });
        ASSERT_FALSE(result);
    }

    // verify error sent
    std::unique_lock<std::mutex> lock(m_mutex);
    // Second track enqueue, but had an error loading.  now advance by finishing playing
    m_audioPlayer->onPlaybackFinished(m_mockMediaPlayer->getSourceId(), DEFAULT_MEDIA_PLAYER_STATE);

    result = m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [this] {
        for (auto messageStatus : m_expectedMessages) {
            if (messageStatus.second == 0) {
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

TEST_F(AudioPlayerTest, test_onPlaybackPaused) {
    m_expectedMessages.insert({PLAYBACK_STARTED_NAME, 0});
    m_expectedMessages.insert({PLAYBACK_PAUSED_NAME, 0});

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            std::lock_guard<std::mutex> lock(m_mutex);
            verifyMessageMap(request, &m_expectedMessages);
            m_messageSentTrigger.notify_one();
        }));

    sendPlayDirective();

    m_audioPlayer->onFocusChanged(FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PAUSED, MY_WAIT_TIMEOUT));

    std::unique_lock<std::mutex> lock(m_mutex);
    bool result;
    result = m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [this] {
        for (auto messageStatus : m_expectedMessages) {
            if (messageStatus.second == 0) {
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

TEST_F(AudioPlayerTest, test_onPlaybackResumed) {
    m_expectedMessages.insert({PLAYBACK_STARTED_NAME, 0});
    m_expectedMessages.insert({PLAYBACK_RESUMED_NAME, 0});

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            std::lock_guard<std::mutex> lock(m_mutex);
            verifyMessageMap(request, &m_expectedMessages);
            m_messageSentTrigger.notify_one();
        }));

    sendPlayDirective();
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));

    bool result;

    m_audioPlayer->onPlaybackResumed(m_mockMediaPlayer->getCurrentSourceId(), DEFAULT_MEDIA_PLAYER_STATE);

    std::unique_lock<std::mutex> lock(m_mutex);

    result = m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [this] {
        for (auto messageStatus : m_expectedMessages) {
            if (messageStatus.second == 0) {
                return false;
            }
        }
        return true;
    });

    ASSERT_TRUE(result);
}

/**
 * Test @c onPlaybackFinished and expect a PLAYBACK_NEARLY_FINISHED_NAME and a PLAYBACK_FINISHED_NAME message
 */
TEST_F(AudioPlayerTest, test_onPlaybackFinished_bufferCompleteAfterStarted) {
    m_expectedMessages.insert({PLAYBACK_STARTED_NAME, 0});
    m_expectedMessages.insert({PLAYBACK_NEARLY_FINISHED_NAME, 0});
    m_expectedMessages.insert({PLAYBACK_FINISHED_NAME, 0});

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            std::lock_guard<std::mutex> lock(m_mutex);
            verifyMessageMap(request, &m_expectedMessages);
            m_messageSentTrigger.notify_one();
        }));

    sendPlayDirective();
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));

    m_audioPlayer->onPlaybackFinished(m_mockMediaPlayer->getCurrentSourceId(), DEFAULT_MEDIA_PLAYER_STATE);

    std::unique_lock<std::mutex> lock(m_mutex);
    bool result;

    result = m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [this] {
        for (auto messageStatus : m_expectedMessages) {
            if (messageStatus.second == 0) {
                return false;
            }
        }
        return true;
    });

    ASSERT_TRUE(result);
}

/**
 * Test @c onPlaybackFinished and expect a PLAYBACK_NEARLY_FINISHED_NAME and a PLAYBACK_FINISHED_NAME message
 */
TEST_F(AudioPlayerTest, test_onPlaybackFinished_bufferCompleteBeforeStarted) {
    m_expectedMessages.insert({PLAYBACK_STARTED_NAME, 0});
    m_expectedMessages.insert({PLAYBACK_NEARLY_FINISHED_NAME, 0});
    m_expectedMessages.insert({PLAYBACK_FINISHED_NAME, 0});

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            std::lock_guard<std::mutex> lock(m_mutex);
            verifyMessageMap(request, &m_expectedMessages);
            m_messageSentTrigger.notify_one();
        }));

    auto avsMessageHeader =
        std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_TEST, PLAY_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> playDirective = AVSDirective::create(
        "",
        avsMessageHeader,
        createEnqueuePayloadTest(OFFSET_IN_MILLISECONDS_TEST),
        m_attachmentManager,
        CONTEXT_ID_TEST);
    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &AudioPlayerTest::wakeOnAcquireChannel));

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted());

    m_audioPlayer->CapabilityAgent::preHandleDirective(playDirective, std::move(m_mockDirectiveHandlerResult));
    m_audioPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_EQ(std::future_status::ready, m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));

    m_audioPlayer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);

    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));

    m_audioPlayer->onBufferingComplete(m_mockMediaPlayer->getCurrentSourceId(), DEFAULT_MEDIA_PLAYER_STATE);
    m_audioPlayer->onPlaybackFinished(m_mockMediaPlayer->getCurrentSourceId(), DEFAULT_MEDIA_PLAYER_STATE);

    bool result;
    std::unique_lock<std::mutex> lock(m_mutex);

    result = m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [this] {
        for (auto messageStatus : m_expectedMessages) {
            if (messageStatus.second == 0) {
                return false;
            }
        }
        return true;
    });

    ASSERT_TRUE(result);
}

/**
 * Test @c onPlaybackFinished with playbackAttributes.
 */

TEST_F(AudioPlayerTest, testOnPlaybackFinishedWithPlaybackAttributes) {
    PlaybackAttributes expectedPlaybackAttributes = {MESSAGE_PLAYBACK_ATTRIBUTES_NAME_VALUE,
                                                     MESSAGE_PLAYBACK_ATTRIBUTES_CODEC_VALUE,
                                                     MESSAGE_PLAYBACK_ATTRIBUTES_SAMPLING_RATE_VALUE,
                                                     MESSAGE_PLAYBACK_ATTRIBUTES_BITRATE_VALUE};
    EXPECT_CALL(*(m_mockMediaPlayer.get()), getPlaybackAttributes())
        .WillRepeatedly(Return(avsCommon::utils::Optional<PlaybackAttributes>(expectedPlaybackAttributes)));

    PlaybackAttributes* actualPlaybackAttributes = new PlaybackAttributes();
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(
            Invoke([this, actualPlaybackAttributes](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
                std::lock_guard<std::mutex> lock(m_mutex);
                extractPlaybackAttributes(request, actualPlaybackAttributes);
                m_messageSentTrigger.notify_one();
            }));

    sendPlayDirective();

    m_audioPlayer->onPlaybackFinished(m_mockMediaPlayer->getCurrentSourceId(), DEFAULT_MEDIA_PLAYER_STATE);

    std::unique_lock<std::mutex> lock(m_mutex);

    bool result;

    result =
        m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [expectedPlaybackAttributes, actualPlaybackAttributes] {
            if (actualPlaybackAttributes->name != expectedPlaybackAttributes.name ||
                actualPlaybackAttributes->codec != expectedPlaybackAttributes.codec ||
                actualPlaybackAttributes->samplingRateInHertz != expectedPlaybackAttributes.samplingRateInHertz ||
                actualPlaybackAttributes->dataRateInBitsPerSecond !=
                    expectedPlaybackAttributes.dataRateInBitsPerSecond) {
                return false;
            }

            return true;
        });

    ASSERT_TRUE(result);
}

/**
 * Test @c onPlaybackStopped with playbackReports.
 */

TEST_F(AudioPlayerTest, testOnPlaybackStoppedWithPlaybackReports) {
    PlaybackAttributes expectedPlaybackAttributes = {MESSAGE_PLAYBACK_ATTRIBUTES_NAME_VALUE,
                                                     MESSAGE_PLAYBACK_ATTRIBUTES_CODEC_VALUE,
                                                     MESSAGE_PLAYBACK_ATTRIBUTES_SAMPLING_RATE_VALUE,
                                                     MESSAGE_PLAYBACK_ATTRIBUTES_BITRATE_VALUE};
    PlaybackReport expectedPlaybackReport = {std::chrono::milliseconds(MESSAGE_PLAYBACK_REPORTS_START_OFFSET_VALUE),
                                             std::chrono::milliseconds(MESSAGE_PLAYBACK_REPORTS_END_OFFSET_VALUE),
                                             expectedPlaybackAttributes};
    EXPECT_CALL(*(m_mockMediaPlayer.get()), getPlaybackReports())
        .WillRepeatedly(Return(std::vector<PlaybackReport>{expectedPlaybackReport}));

    std::vector<PlaybackReport>* actualPlaybackReports = new std::vector<PlaybackReport>();
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this, actualPlaybackReports](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            std::lock_guard<std::mutex> lock(m_mutex);
            extractPlaybackReports(request, actualPlaybackReports);
            m_messageSentTrigger.notify_one();
        }));

    sendPlayDirective();

    m_audioPlayer->onPlaybackStopped(m_mockMediaPlayer->getCurrentSourceId(), DEFAULT_MEDIA_PLAYER_STATE);

    std::unique_lock<std::mutex> lock(m_mutex);

    bool result;

    result = m_messageSentTrigger.wait_for(
        lock, MY_WAIT_TIMEOUT, [expectedPlaybackReport, expectedPlaybackAttributes, actualPlaybackReports] {
            if (actualPlaybackReports->size() != 1) {
                return false;
            }

            PlaybackAttributes actualPlaybackAttributes = actualPlaybackReports->at(0).playbackAttributes;
            if (actualPlaybackReports->at(0).startOffset != expectedPlaybackReport.startOffset ||
                actualPlaybackReports->at(0).endOffset != expectedPlaybackReport.endOffset ||
                actualPlaybackAttributes.name != expectedPlaybackAttributes.name ||
                actualPlaybackAttributes.codec != expectedPlaybackAttributes.codec ||
                actualPlaybackAttributes.samplingRateInHertz != expectedPlaybackAttributes.samplingRateInHertz ||
                actualPlaybackAttributes.dataRateInBitsPerSecond !=
                    expectedPlaybackAttributes.dataRateInBitsPerSecond) {
                return false;
            }

            return true;
        });

    ASSERT_TRUE(result);
}

/**
 * Test @c onBufferUnderrun and expect a PlaybackStutterStarted message
 */

TEST_F(AudioPlayerTest, test_onBufferUnderrun) {
    m_expectedMessages.insert({PLAYBACK_STARTED_NAME, 0});
    m_expectedMessages.insert({PLAYBACK_STUTTER_STARTED_NAME, 0});

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            std::lock_guard<std::mutex> lock(m_mutex);
            verifyMessageMap(request, &m_expectedMessages);
            m_messageSentTrigger.notify_one();
        }));

    sendPlayDirective();

    m_audioPlayer->onBufferUnderrun(m_mockMediaPlayer->getCurrentSourceId(), DEFAULT_MEDIA_PLAYER_STATE);

    std::unique_lock<std::mutex> lock(m_mutex);

    bool result;

    result = m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [this] {
        for (auto messageStatus : m_expectedMessages) {
            if (messageStatus.second == 0) {
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

TEST_F(AudioPlayerTest, testTimer_onBufferRefilled) {
    m_expectedMessages.insert({PLAYBACK_STARTED_NAME, 0});
    m_expectedMessages.insert({PLAYBACK_STUTTER_FINISHED_NAME, 0});

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            std::lock_guard<std::mutex> lock(m_mutex);
            verifyMessageMap(request, &m_expectedMessages);
            m_messageSentTrigger.notify_one();
        }));

    sendPlayDirective();

    m_audioPlayer->onBufferRefilled(m_mockMediaPlayer->getCurrentSourceId(), DEFAULT_MEDIA_PLAYER_STATE);

    std::unique_lock<std::mutex> lock(m_mutex);

    bool result;

    result = m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [this] {
        for (auto messageStatus : m_expectedMessages) {
            if (messageStatus.second == 0) {
                return false;
            }
        }
        return true;
    });

    ASSERT_TRUE(result);
}

/**
 * Test @c onTags and expect valid JSON.
 * Build a vector of tags and pass to Observer (onTags).
 * Observer will use the vector of tags and build a valid JSON object
 * "StreamMetadataExtracted Event". This JSON object is verified in verifyTags.
 * Verify that metadata not on whitelist is removed, and not sent
 */

TEST_F(AudioPlayerTest, test_onTags_filteredOut) {
    sendPlayDirective();

    m_expectedMessages.insert({STREAM_METADATA_EXTRACTED_NAME, 0});
    m_expectedMessages.insert({MESSAGE_METADATA_STRING_VALUE, 0});
    m_expectedMessages.insert({MESSAGE_METADATA_UINT_VALUE, 0});
    m_expectedMessages.insert({MESSAGE_METADATA_DOUBLE_VALUE, 0});

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .WillRepeatedly(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            if (!m_mockMediaPlayer->waitUntilPlaybackStopped(std::chrono::milliseconds(0))) {
                std::lock_guard<std::mutex> lock(m_mutex);
                verifyTags(request, &m_expectedMessages);
                m_messageSentTrigger.notify_one();
            }
        }));

    std::unique_ptr<AudioPlayer::VectorOfTags> ptrToVectorOfTags = make_unique<AudioPlayer::VectorOfTags>();
    auto vectorOfTags = ptrToVectorOfTags.get();

    // Populate vector with dummy tags
    AudioPlayer::TagKeyValueType stringTag, uintTag, intTag, doubleTag, booleanTag;
    stringTag.key = std::string(MESSAGE_METADATA_STRING_KEY);
    stringTag.value = std::string(MESSAGE_METADATA_STRING_VALUE);
    stringTag.type = AudioPlayer::TagType::STRING;
    vectorOfTags->push_back(stringTag);

    uintTag.key = std::string(MESSAGE_METADATA_UINT_KEY);
    uintTag.value = std::string(MESSAGE_METADATA_UINT_VALUE);
    uintTag.type = AudioPlayer::TagType::UINT;
    vectorOfTags->push_back(uintTag);

    intTag.key = std::string(MESSAGE_METADATA_INT_KEY);
    intTag.value = std::string(MESSAGE_METADATA_INT_VALUE);
    intTag.type = AudioPlayer::TagType::INT;
    vectorOfTags->push_back(intTag);

    doubleTag.key = std::string(MESSAGE_METADATA_DOUBLE_KEY);
    doubleTag.value = std::string(MESSAGE_METADATA_DOUBLE_VALUE);
    doubleTag.type = AudioPlayer::TagType::DOUBLE;
    vectorOfTags->push_back(doubleTag);

    booleanTag.key = std::string(MESSAGE_METADATA_BOOLEAN_KEY);
    booleanTag.value = std::string(MESSAGE_METADATA_BOOLEAN_VALUE);
    booleanTag.type = AudioPlayer::TagType::BOOLEAN;
    vectorOfTags->push_back(booleanTag);

    m_audioPlayer->onTags(
        m_mockMediaPlayer->getCurrentSourceId(), std::move(ptrToVectorOfTags), DEFAULT_MEDIA_PLAYER_STATE);

    std::unique_lock<std::mutex> lock(m_mutex);

    auto result = m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [this] {
        for (auto messageStatus : m_expectedMessages) {
            if (messageStatus.second == 0) {
                return false;
            }
        }
        return true;
    });
    ASSERT_FALSE(result);
}

/**
 * Test @c onTags and expect valid JSON.
 * Build a vector of tags and pass to Observer (onTags).
 * Observer will use the vector of tags and build a valid JSON object
 * "StreamMetadataExtracted Event". This JSON object is verified in verifyTags.
 * Send data on whitelist
 */

TEST_F(AudioPlayerTest, test_onTags_filteredIn) {
    sendPlayDirective();
    m_expectedMessages.insert({STREAM_METADATA_EXTRACTED_NAME, 0});
    m_expectedMessages.insert({MESSAGE_METADATA_STRING_VALUE, 0});
    m_expectedMessages.insert({MESSAGE_METADATA_UINT_VALUE, -1});
    m_expectedMessages.insert({MESSAGE_METADATA_DOUBLE_VALUE, -1});

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            if (!m_mockMediaPlayer->waitUntilPlaybackStopped(std::chrono::milliseconds(0))) {
                std::lock_guard<std::mutex> lock(m_mutex);
                verifyTags(request, &m_expectedMessages, false);
                m_messageSentTrigger.notify_one();
            }
        }));

    std::unique_ptr<AudioPlayer::VectorOfTags> ptrToVectorOfTags = make_unique<AudioPlayer::VectorOfTags>();
    auto vectorOfTags = ptrToVectorOfTags.get();

    // Populate vector with dummy tags
    AudioPlayer::TagKeyValueType stringTag, uintTag, intTag, doubleTag, booleanTag;
    stringTag.key = std::string(MESSAGE_METADATA_STRING_KEY_WL);
    stringTag.value = std::string(MESSAGE_METADATA_STRING_VALUE);
    stringTag.type = AudioPlayer::TagType::STRING;
    vectorOfTags->push_back(stringTag);

    uintTag.key = std::string(MESSAGE_METADATA_UINT_KEY);
    uintTag.value = std::string(MESSAGE_METADATA_UINT_VALUE);
    uintTag.type = AudioPlayer::TagType::UINT;
    vectorOfTags->push_back(uintTag);

    intTag.key = std::string(MESSAGE_METADATA_INT_KEY);
    intTag.value = std::string(MESSAGE_METADATA_INT_VALUE);
    intTag.type = AudioPlayer::TagType::INT;
    vectorOfTags->push_back(intTag);

    doubleTag.key = std::string(MESSAGE_METADATA_DOUBLE_KEY);
    doubleTag.value = std::string(MESSAGE_METADATA_DOUBLE_VALUE);
    doubleTag.type = AudioPlayer::TagType::DOUBLE;
    vectorOfTags->push_back(doubleTag);

    booleanTag.key = std::string(MESSAGE_METADATA_BOOLEAN_KEY);
    booleanTag.value = std::string(MESSAGE_METADATA_BOOLEAN_VALUE);
    booleanTag.type = AudioPlayer::TagType::BOOLEAN;
    vectorOfTags->push_back(booleanTag);

    m_audioPlayer->onTags(
        m_mockMediaPlayer->getCurrentSourceId(), std::move(ptrToVectorOfTags), DEFAULT_MEDIA_PLAYER_STATE);

    std::unique_lock<std::mutex> lock(m_mutex);

    auto result = m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [this] {
        for (auto messageStatus : m_expectedMessages) {
            if (messageStatus.second == 0) {
                return false;
            }
        }
        return true;
    });
    ASSERT_TRUE(result);
}

/**
 * Test @c onTags and expect valid JSON.
 * Build a vector of tags and pass to Observer (onTags).
 * Observer will use the vector of tags and build a valid JSON object
 * "StreamMetadataExtracted Event". This JSON object is verified in verifyTags.
 * Send data on whitelist
 * make sure event not sent too fast
 */

TEST_F(AudioPlayerTest, test_onTags_filteredIn_rateCheck) {
    sendPlayDirective();

    m_expectedMessages.insert({STREAM_METADATA_EXTRACTED_NAME, 0});
    m_expectedMessages.insert({MESSAGE_METADATA_STRING_VALUE, 0});
    m_expectedMessages.insert({MESSAGE_METADATA_UINT_VALUE, -1});
    m_expectedMessages.insert({MESSAGE_METADATA_DOUBLE_VALUE, -1});

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            if (!m_mockMediaPlayer->waitUntilPlaybackStopped(std::chrono::milliseconds(0))) {
                std::lock_guard<std::mutex> lock(m_mutex);
                verifyTags(request, &m_expectedMessages, false);
                m_messageSentTrigger.notify_one();
            }
        }));

    std::unique_ptr<AudioPlayer::VectorOfTags> ptrToVectorOfTags = make_unique<AudioPlayer::VectorOfTags>();
    auto vectorOfTags = ptrToVectorOfTags.get();

    // Populate vector with dummy tags
    AudioPlayer::TagKeyValueType stringTag, uintTag, intTag, doubleTag, booleanTag;
    stringTag.key = std::string(MESSAGE_METADATA_STRING_KEY_WL);
    stringTag.value = std::string(MESSAGE_METADATA_STRING_VALUE);
    stringTag.type = AudioPlayer::TagType::STRING;
    vectorOfTags->push_back(stringTag);

    uintTag.key = std::string(MESSAGE_METADATA_UINT_KEY);
    uintTag.value = std::string(MESSAGE_METADATA_UINT_VALUE);
    uintTag.type = AudioPlayer::TagType::UINT;
    vectorOfTags->push_back(uintTag);

    intTag.key = std::string(MESSAGE_METADATA_INT_KEY);
    intTag.value = std::string(MESSAGE_METADATA_INT_VALUE);
    intTag.type = AudioPlayer::TagType::INT;
    vectorOfTags->push_back(intTag);

    doubleTag.key = std::string(MESSAGE_METADATA_DOUBLE_KEY);
    doubleTag.value = std::string(MESSAGE_METADATA_DOUBLE_VALUE);
    doubleTag.type = AudioPlayer::TagType::DOUBLE;
    vectorOfTags->push_back(doubleTag);

    booleanTag.key = std::string(MESSAGE_METADATA_BOOLEAN_KEY);
    booleanTag.value = std::string(MESSAGE_METADATA_BOOLEAN_VALUE);
    booleanTag.type = AudioPlayer::TagType::BOOLEAN;
    vectorOfTags->push_back(booleanTag);

    std::unique_ptr<AudioPlayer::VectorOfTags> ptrToVectorOfTags1 =
        make_unique<AudioPlayer::VectorOfTags>(vectorOfTags->begin(), vectorOfTags->end());
    m_audioPlayer->onTags(
        m_mockMediaPlayer->getCurrentSourceId(), std::move(ptrToVectorOfTags1), DEFAULT_MEDIA_PLAYER_STATE);

    {
        std::unique_lock<std::mutex> lock(m_mutex);

        auto result = m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [this] {
            for (auto messageStatus : m_expectedMessages) {
                if (messageStatus.second == 0) {
                    return false;
                }
            }
            return true;
        });

        ASSERT_TRUE(result);
    }
    m_expectedMessages.clear();
    m_expectedMessages.insert({STREAM_METADATA_EXTRACTED_NAME, 0});
    m_expectedMessages.insert({MESSAGE_METADATA_STRING_VALUE_ALT, 0});
    m_expectedMessages.insert({MESSAGE_METADATA_UINT_VALUE, -1});
    m_expectedMessages.insert({MESSAGE_METADATA_DOUBLE_VALUE, -1});

    for (auto iter = vectorOfTags->begin(); iter != vectorOfTags->end(); ++iter) {
        if (iter->key == MESSAGE_METADATA_STRING_KEY_WL) {
            iter->value = MESSAGE_METADATA_STRING_VALUE_ALT;
            break;
        }
    }

    std::unique_ptr<AudioPlayer::VectorOfTags> ptrToVectorOfTags2 =
        make_unique<AudioPlayer::VectorOfTags>(vectorOfTags->begin(), vectorOfTags->end());
    m_audioPlayer->onTags(
        m_mockMediaPlayer->getCurrentSourceId(), std::move(ptrToVectorOfTags2), DEFAULT_MEDIA_PLAYER_STATE);

    {
        std::unique_lock<std::mutex> lock(m_mutex);

        auto result = m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [this] {
            for (auto messageStatus : m_expectedMessages) {
                if (messageStatus.second == 0) {
                    return false;
                }
            }
            return true;
        });

        ASSERT_FALSE(result);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(METADATA_EVENT_DELAY));

    m_audioPlayer->onTags(
        m_mockMediaPlayer->getCurrentSourceId(), std::move(ptrToVectorOfTags), DEFAULT_MEDIA_PLAYER_STATE);

    {
        std::unique_lock<std::mutex> lock(m_mutex);

        auto result = m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [this] {
            for (auto messageStatus : m_expectedMessages) {
                if (messageStatus.second == 0) {
                    return false;
                }
            }
            return true;
        });

        ASSERT_TRUE(result);
    }
}

/**
 * Test @c onTags and expect valid JSON.
 * Build a vector of tags and pass to Observer (onTags).
 * Observer will use the vector of tags and build a valid JSON object
 * "StreamMetadataExtracted Event". This JSON object is verified in verifyTags.
 * Send data on whitelist
 * make sure duplicate not sent
 */

TEST_F(AudioPlayerTest, test_onTags_filteredIn_duplicateCheck) {
    sendPlayDirective();

    m_expectedMessages.insert({STREAM_METADATA_EXTRACTED_NAME, 0});
    m_expectedMessages.insert({MESSAGE_METADATA_STRING_VALUE, 0});
    m_expectedMessages.insert({MESSAGE_METADATA_UINT_VALUE, -1});
    m_expectedMessages.insert({MESSAGE_METADATA_DOUBLE_VALUE, -1});

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            if (!m_mockMediaPlayer->waitUntilPlaybackStopped(std::chrono::milliseconds(0))) {
                std::lock_guard<std::mutex> lock(m_mutex);
                verifyTags(request, &m_expectedMessages, false);
                m_messageSentTrigger.notify_one();
            }
        }));

    std::unique_ptr<AudioPlayer::VectorOfTags> ptrToVectorOfTags = make_unique<AudioPlayer::VectorOfTags>();
    auto vectorOfTags = ptrToVectorOfTags.get();

    // Populate vector with dummy tags
    AudioPlayer::TagKeyValueType stringTag, uintTag, intTag, doubleTag, booleanTag;
    stringTag.key = std::string(MESSAGE_METADATA_STRING_KEY_WL);
    stringTag.value = std::string(MESSAGE_METADATA_STRING_VALUE);
    stringTag.type = AudioPlayer::TagType::STRING;
    vectorOfTags->push_back(stringTag);

    uintTag.key = std::string(MESSAGE_METADATA_UINT_KEY);
    uintTag.value = std::string(MESSAGE_METADATA_UINT_VALUE);
    uintTag.type = AudioPlayer::TagType::UINT;
    vectorOfTags->push_back(uintTag);

    intTag.key = std::string(MESSAGE_METADATA_INT_KEY);
    intTag.value = std::string(MESSAGE_METADATA_INT_VALUE);
    intTag.type = AudioPlayer::TagType::INT;
    vectorOfTags->push_back(intTag);

    doubleTag.key = std::string(MESSAGE_METADATA_DOUBLE_KEY);
    doubleTag.value = std::string(MESSAGE_METADATA_DOUBLE_VALUE);
    doubleTag.type = AudioPlayer::TagType::DOUBLE;
    vectorOfTags->push_back(doubleTag);

    booleanTag.key = std::string(MESSAGE_METADATA_BOOLEAN_KEY);
    booleanTag.value = std::string(MESSAGE_METADATA_BOOLEAN_VALUE);
    booleanTag.type = AudioPlayer::TagType::BOOLEAN;
    vectorOfTags->push_back(booleanTag);

    std::unique_ptr<AudioPlayer::VectorOfTags> ptrToVectorOfTags1 =
        make_unique<AudioPlayer::VectorOfTags>(vectorOfTags->begin(), vectorOfTags->end());
    m_audioPlayer->onTags(
        m_mockMediaPlayer->getCurrentSourceId(), std::move(ptrToVectorOfTags1), DEFAULT_MEDIA_PLAYER_STATE);

    {
        std::unique_lock<std::mutex> lock(m_mutex);

        auto result = m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [this] {
            for (auto messageStatus : m_expectedMessages) {
                if (messageStatus.second == 0) {
                    return false;
                }
            }
            return true;
        });

        ASSERT_TRUE(result);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(METADATA_EVENT_DELAY));

    m_expectedMessages.clear();
    m_expectedMessages.insert({STREAM_METADATA_EXTRACTED_NAME, 0});
    m_expectedMessages.insert({MESSAGE_METADATA_STRING_VALUE, 0});
    m_expectedMessages.insert({MESSAGE_METADATA_UINT_VALUE, -1});
    m_expectedMessages.insert({MESSAGE_METADATA_DOUBLE_VALUE, -1});

    std::unique_ptr<AudioPlayer::VectorOfTags> ptrToVectorOfTags2 =
        make_unique<AudioPlayer::VectorOfTags>(vectorOfTags->begin(), vectorOfTags->end());
    m_audioPlayer->onTags(
        m_mockMediaPlayer->getCurrentSourceId(), std::move(ptrToVectorOfTags2), DEFAULT_MEDIA_PLAYER_STATE);

    {
        std::unique_lock<std::mutex> lock(m_mutex);

        auto result = m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [this] {
            for (auto messageStatus : m_expectedMessages) {
                if (messageStatus.second == 0) {
                    return false;
                }
            }
            return true;
        });

        ASSERT_FALSE(result);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(METADATA_EVENT_DELAY));

    m_expectedMessages.clear();
    m_expectedMessages.insert({STREAM_METADATA_EXTRACTED_NAME, 0});
    m_expectedMessages.insert({MESSAGE_METADATA_STRING_VALUE_ALT, 0});
    m_expectedMessages.insert({MESSAGE_METADATA_UINT_VALUE, -1});
    m_expectedMessages.insert({MESSAGE_METADATA_DOUBLE_VALUE, -1});

    for (auto iter = vectorOfTags->begin(); iter != vectorOfTags->end(); ++iter) {
        if (iter->key == MESSAGE_METADATA_STRING_KEY_WL) {
            iter->value = MESSAGE_METADATA_STRING_VALUE_ALT;
            break;
        }
    }
    m_audioPlayer->onTags(
        m_mockMediaPlayer->getCurrentSourceId(), std::move(ptrToVectorOfTags), DEFAULT_MEDIA_PLAYER_STATE);

    {
        std::unique_lock<std::mutex> lock(m_mutex);

        auto result = m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [this] {
            for (auto messageStatus : m_expectedMessages) {
                if (messageStatus.second == 0) {
                    return false;
                }
            }
            return true;
        });

        ASSERT_TRUE(result);
    }
}

/**
 * Test @c cancelDirective
 * Expect the @c handleDirective call to the cancelled directive returns false
 */

TEST_F(AudioPlayerTest, test_cancelDirective) {
    sendPlayDirective();

    m_audioPlayer->CapabilityAgent::cancelDirective(MESSAGE_ID_TEST);

    ASSERT_FALSE(m_audioPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST));
}

/**
 * Test focus change to NONE in IDLE state
 * Expect nothing to happen
 */

TEST_F(AudioPlayerTest, test_focusChangeToNoneInIdleState) {
    // switching to FocusState::NONE should cause no change
    m_audioPlayer->onFocusChanged(FocusState::NONE, MixingBehavior::MUST_STOP);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::IDLE, MY_WAIT_TIMEOUT));
}

/**
 * Test focus change from FOREGROUND to BACKGROUND in IDLE state
 * Expect a call to pause(). This call is intended to go through MediaPlayer and cause nothing to happen
 * due to a lack of a queued AudioItem.
 */

TEST_F(AudioPlayerTest, test_focusChangeFromForegroundToBackgroundInIdleState) {
    m_audioPlayer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    // ensure AudioPlayer is IDLE
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::IDLE, MY_WAIT_TIMEOUT));

    m_audioPlayer->onFocusChanged(FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE);
    // ensure AudioPlayer is still IDLE
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::IDLE, MY_WAIT_TIMEOUT));
}

/**
 * Test focus change in from NONE to BACKGROUND while IDLE.
 * Expect a call to pause. This isn't an expected state during normal execution.
 */

TEST_F(AudioPlayerTest, test_focusChangeFromNoneToBackgroundInIdleState) {
    m_audioPlayer->onFocusChanged(FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE);
}

/**
 * Test focus changes in PLAYING state
 * Expect to pause when switching to BACKGROUND and to stop when switching to NONE
 */

TEST_F(AudioPlayerTest, test_focusChangesInPlayingState) {
    sendPlayDirective();

    // already in FOREGROUND, expect no change
    m_audioPlayer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));

    // expect to pause in BACKGROUND
    EXPECT_CALL(*(m_mockMediaPlayer.get()), pause(_)).Times(1);
    m_audioPlayer->onFocusChanged(FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PAUSED, MY_WAIT_TIMEOUT));

    // expect to resume when switching back to FOREGROUND
    EXPECT_CALL(*(m_mockMediaPlayer.get()), resume(_)).Times(1);
    m_audioPlayer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));

    // expect to stop when changing focus to NONE
    EXPECT_CALL(*(m_mockMediaPlayer.get()), stop(_)).Times(1);
    m_audioPlayer->onFocusChanged(FocusState::NONE, MixingBehavior::MUST_STOP);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::STOPPED, MY_WAIT_TIMEOUT));
}

/**
 * Test focus changes in STOPPED state
 * Expect to remain in STOPPED state when switching to FOREGROUND (because there are no queued AudioItems) and
 * to transition to PAUSED when switching to BACKGROUND.
 */

TEST_F(AudioPlayerTest, test_focusChangesInStoppedState) {
    sendPlayDirective();

    // push AudioPlayer into stopped state
    EXPECT_CALL(*(m_mockMediaPlayer.get()), stop(_)).Times(AtLeast(1));
    m_audioPlayer->onFocusChanged(FocusState::NONE, MixingBehavior::MUST_STOP);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::STOPPED, MY_WAIT_TIMEOUT));

    m_audioPlayer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::STOPPED, MY_WAIT_TIMEOUT));

    m_audioPlayer->onFocusChanged(FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE);
    ASSERT_FALSE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PAUSED, MY_WAIT_TIMEOUT));
}

/**
 * Test focus changes in PAUSED state
 * Expect to resume when switching to FOREGROUND, expect nothing when switching to BACKGROUND, expect stop when
 * switching to NONE
 */
TEST_F(AudioPlayerTest, test_focusChangesInPausedState) {
    sendPlayDirective();

    // push AudioPlayer into paused state
    EXPECT_CALL(*(m_mockMediaPlayer.get()), pause(_)).Times(1);
    m_audioPlayer->onFocusChanged(FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PAUSED, MY_WAIT_TIMEOUT));

    // expect a resume when switching back to FOREGROUND
    EXPECT_CALL(*(m_mockMediaPlayer.get()), resume(_)).Times(1);
    m_audioPlayer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));

    // return to paused state
    EXPECT_CALL(*(m_mockMediaPlayer.get()), pause(_)).Times(1);
    m_audioPlayer->onFocusChanged(FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PAUSED, MY_WAIT_TIMEOUT));

    // expect nothing to happen when switching to BACKGROUND from BACKGROUND
    m_audioPlayer->onFocusChanged(FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PAUSED, MY_WAIT_TIMEOUT));

    // expect stop when switching to NONE focus
    EXPECT_CALL(*(m_mockMediaPlayer.get()), stop(_)).Times(1);
    m_audioPlayer->onFocusChanged(FocusState::NONE, MixingBehavior::MUST_STOP);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::STOPPED, MY_WAIT_TIMEOUT));
}

/**
 * Test focus changes in BUFFER_UNDERRUN state
 * Expect nothing to happen when switching to FOREGROUND, expect to pause when switching to BACKGROUND, expect to stop
 * when switching to NONE
 */

TEST_F(AudioPlayerTest, test_focusChangesInBufferUnderrunState) {
    sendPlayDirective();

    // push AudioPlayer into buffer underrun state
    m_audioPlayer->onBufferUnderrun(m_mockMediaPlayer->getCurrentSourceId(), DEFAULT_MEDIA_PLAYER_STATE);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::BUFFER_UNDERRUN, MY_WAIT_TIMEOUT));

    // nothing happens, AudioPlayer already in FOREGROUND
    m_audioPlayer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::BUFFER_UNDERRUN, MY_WAIT_TIMEOUT));

    // expect to pause if pushed to BACKGROUND
    EXPECT_CALL(*(m_mockMediaPlayer.get()), pause(_)).Times(1);
    m_audioPlayer->onFocusChanged(FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PAUSED, MY_WAIT_TIMEOUT));

    // back to FOREGROUND and buffer underrun state
    EXPECT_CALL(*(m_mockMediaPlayer.get()), resume(_)).Times(1);
    m_audioPlayer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));
    m_audioPlayer->onBufferUnderrun(m_mockMediaPlayer->getCurrentSourceId(), DEFAULT_MEDIA_PLAYER_STATE);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::BUFFER_UNDERRUN, MY_WAIT_TIMEOUT));

    // expect stop when switching to NONE focus
    EXPECT_CALL(*(m_mockMediaPlayer.get()), stop(_)).Times(1);
    m_audioPlayer->onFocusChanged(FocusState::NONE, MixingBehavior::MUST_STOP);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::STOPPED, MY_WAIT_TIMEOUT));
}

/**
 * Test an immediate focus change to background after play() has been called
 * Expect that pause() is called when @c AudioPlayer is pushed into background
 */

TEST_F(AudioPlayerTest, test_focusChangeToBackgroundBeforeOnPlaybackStarted) {
    EXPECT_CALL(*(m_mockMediaPlayer.get()), play(_)).Times(1);
    sendPlayDirective();

    // send clear queue directive and move to NONE focus
    EXPECT_CALL(*(m_mockMediaPlayer.get()), stop(_)).Times(AtLeast(1));

    sendClearQueueDirective();

    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::STOPPED, MY_WAIT_TIMEOUT));
    m_audioPlayer->onFocusChanged(FocusState::NONE, MixingBehavior::MUST_STOP);

    // send a second Play directive and move to foreground
    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*(m_mockMediaPlayerTrack2.get()), play(_)).Times(1);
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_TEST_2);

    std::shared_ptr<AVSDirective> playDirective = AVSDirective::create(
        "",
        avsMessageHeader,
        createEnqueuePayloadTest(OFFSET_IN_MILLISECONDS_TEST, AUDIO_ITEM_ID_2),
        m_attachmentManager,
        CONTEXT_ID_TEST_2);

    m_audioPlayer->CapabilityAgent::preHandleDirective(playDirective, std::move(m_mockDirectiveHandlerResult));
    m_audioPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST_2);

    m_audioPlayer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));

    m_audioPlayer->onFocusChanged(FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PAUSED, MY_WAIT_TIMEOUT));
}

/**
 * Test @c onPlaybackError and expect AudioPlayer to change to STOPPED state and that it would go back to PLAYING state
 * when a new REPLACE_ALL Play directive comes in.
 */

TEST_F(AudioPlayerTest, test_playAfterOnPlaybackError) {
    EXPECT_CALL(*(m_mockMediaPlayer.get()), getOffset(_))
        .WillRepeatedly(Return(m_mockMediaPlayer->getOffset(m_mockMediaPlayer->getCurrentSourceId())));
    sendPlayDirective();
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));
    EXPECT_CALL(*(m_mockFocusManager.get()), releaseChannel(CHANNEL_NAME, _))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &AudioPlayerTest::wakeOnReleaseChannel));
    m_audioPlayer->onPlaybackError(
        m_mockMediaPlayer->getCurrentSourceId(),
        ErrorType::MEDIA_ERROR_UNKNOWN,
        "TEST_ERROR",
        DEFAULT_MEDIA_PLAYER_STATE);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::STOPPED, MY_WAIT_TIMEOUT));
    ASSERT_EQ(std::future_status::ready, m_wakeReleaseChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_audioPlayer->onFocusChanged(FocusState::NONE, MixingBehavior::MUST_STOP);

    // send a REPLACE_ALL Play directive to see if AudioPlayer can still play the new item
    EXPECT_CALL(*(m_mockMediaPlayerTrack2.get()), play(_)).Times(1);
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_TEST_2);

    std::shared_ptr<AVSDirective> playDirective =
        AVSDirective::create("", avsMessageHeader, REPLACE_ALL_PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST_2);

    m_wakeAcquireChannelPromise = std::promise<void>();
    m_wakeAcquireChannelFuture = m_wakeAcquireChannelPromise.get_future();
    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &AudioPlayerTest::wakeOnAcquireChannel));
    m_audioPlayer->CapabilityAgent::preHandleDirective(playDirective, std::move(m_mockDirectiveHandlerResult));
    m_audioPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST_2);
    ASSERT_EQ(std::future_status::ready, m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_audioPlayer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));
}

/**
 * Test play directive calls @c CaptionManager.onCaption()
 */
TEST_F(AudioPlayerTest, test_playCallsCaptionManager) {
    EXPECT_CALL(*(m_mockCaptionManager.get()), onCaption(_, _)).Times(1);
    sendPlayDirective();
}

/**
 * Test play directive parses caption payload.
 */
TEST_F(AudioPlayerTest, test_playParsesCaptionPayload) {
    captions::CaptionData expectedCaptionData = captions::CaptionData(
        captions::CaptionFormat::WEBVTT, "WEBVTT\n\n1\n00:00.000 --> 00:01.260\nThe time is 2:17 PM.");
    EXPECT_CALL(*(m_mockCaptionManager.get()), onCaption(_, expectedCaptionData)).Times(1);
    sendPlayDirective();
}

/**
 * Test @c onPlaybackStarted calls the @c PlaybackRouter
 */
TEST_F(AudioPlayerTest, test_playbackStartedSwitchesHandler) {
    EXPECT_CALL(*m_mockPlaybackRouter, useDefaultHandlerWith(_));
    sendPlayDirective();
}

/**
 * Test to verify that ProgressReportDelayElapsed Event is sent correctly.  This test is timing sensitive.
 */
TEST_F(AudioPlayerTest, test_progressReportDelayElapsed) {
    m_expectedMessages.insert({PROGRESS_REPORT_DELAY_ELAPSED_NAME, 0});

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            std::lock_guard<std::mutex> lock(m_mutex);
            verifyMessageMap(request, &m_expectedMessages);
            m_messageSentTrigger.notify_one();
        }));

    sendPlayDirective(OFFSET_IN_MILLISECONDS_BEFORE_PROGRESS_REPORT_DELAY);

    std::this_thread::sleep_for(std::chrono::milliseconds(PROGRESS_REPORT_DELAY));

    std::unique_lock<std::mutex> lock(m_mutex);
    auto result = m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [this] {
        for (auto messageStatus : m_expectedMessages) {
            if (messageStatus.second != 1) {
                return false;
            }
        }
        return true;
    });
    ASSERT_TRUE(result);
}

/**
 * Test to verify that ProgressReportDelayElapsed Event is not sent when the delay is less than the offset.  This test
 * is timing sensitive.
 */
TEST_F(AudioPlayerTest, test_progressReportDelayElapsedDelayLessThanOffset) {
    m_expectedMessages.insert({PROGRESS_REPORT_DELAY_ELAPSED_NAME, 0});

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            std::lock_guard<std::mutex> lock(m_mutex);
            verifyMessageMap(request, &m_expectedMessages);
            m_messageSentTrigger.notify_one();
        }));

    sendPlayDirective(OFFSET_IN_MILLISECONDS_AFTER_PROGRESS_REPORT_DELAY);

    std::this_thread::sleep_for(std::chrono::milliseconds(PROGRESS_REPORT_DELAY));

    std::unique_lock<std::mutex> lock(m_mutex);
    auto result = m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [this] {
        for (auto messageStatus : m_expectedMessages) {
            if (messageStatus.second != 0) {
                return false;
            }
        }
        return true;
    });
    ASSERT_TRUE(result);
}

/**
 * Test to verify that ProgressReportIntervalElapsed Event is sent when the interval is less than the offset.  There
 * will be a ProgressReportIntervalElapsed Event at 100, 200 and 300 ms.  This test is timing sensitive.
 */
TEST_F(AudioPlayerTest, testTimer_progressReportIntervalElapsed) {
    m_expectedMessages.insert({PROGRESS_REPORT_INTERVAL_ELAPSED_NAME, 0});

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            std::lock_guard<std::mutex> lock(m_mutex);
            verifyMessageMap(request, &m_expectedMessages);
            m_messageSentTrigger.notify_one();
        }));

    sendPlayDirective(OFFSET_IN_MILLISECONDS_BEFORE_PROGRESS_REPORT_INTERVAL);

    std::this_thread::sleep_for(TIME_FOR_TWO_AND_A_HALF_INTERVAL_PERIODS);

    std::unique_lock<std::mutex> lock(m_mutex);
    auto result = m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [this] {
        for (auto messageStatus : m_expectedMessages) {
            if (messageStatus.second != 3) {
                return false;
            }
        }
        return true;
    });
    ASSERT_TRUE(result);
}

/**
 * Test to verify that ProgressReportIntervalElapsed Event is sent when the interval is less than the offset.  There
 * will be a ProgressReportIntervalElapsed Event at 200 and 300 ms.  This test is timing sensitive.
 */
TEST_F(AudioPlayerTest, test_progressReportIntervalElapsedIntervalLessThanOffset) {
    m_expectedMessages.insert({PROGRESS_REPORT_INTERVAL_ELAPSED_NAME, 0});

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            std::lock_guard<std::mutex> lock(m_mutex);
            verifyMessageMap(request, &m_expectedMessages);
            m_messageSentTrigger.notify_one();
        }));

    sendPlayDirective(OFFSET_IN_MILLISECONDS_AFTER_PROGRESS_REPORT_INTERVAL);

    std::this_thread::sleep_for(TIME_FOR_TWO_AND_A_HALF_INTERVAL_PERIODS);

    std::unique_lock<std::mutex> lock(m_mutex);
    auto result = m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [this] {
        for (auto messageStatus : m_expectedMessages) {
            if (messageStatus.second != 2) {
                return false;
            }
        }
        return true;
    });
    ASSERT_TRUE(result);
}

/**
 * Test when @c AudioPlayer goes to BACKGROUND focus that it changes to PAUSED state.  And when another PLAY directive
 * with REPLACE_ALL behavior comes in, that it would go to STOPPED state, and will not start playing again until the
 * focus goes back to FOREGROUND.
 */

TEST_F(AudioPlayerTest, testSlow_playOnlyAfterForegroundFocus) {
    EXPECT_CALL(*(m_mockMediaPlayer.get()), getOffset(_))
        .WillRepeatedly(Return(m_mockMediaPlayer->getOffset(m_mockMediaPlayer->getCurrentSourceId())));
    sendPlayDirective();
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));
    m_audioPlayer->onPlaybackStarted(m_mockMediaPlayer->getCurrentSourceId(), DEFAULT_MEDIA_PLAYER_STATE);
    m_audioPlayer->onFocusChanged(FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PAUSED, MY_WAIT_TIMEOUT));

    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &AudioPlayerTest::wakeOnAcquireChannel));

    // send a REPLACE_ALL Play directive
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_TEST_2);

    std::shared_ptr<AVSDirective> playDirective =
        AVSDirective::create("", avsMessageHeader, REPLACE_ALL_PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST_2);

    m_wakeAcquireChannelPromise = std::promise<void>();
    m_audioPlayer->CapabilityAgent::preHandleDirective(playDirective, std::move(m_mockDirectiveHandlerResult));
    EXPECT_CALL(*(m_mockMediaPlayerTrack2.get()), play(_)).Times(0);
    m_audioPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST_2);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::STOPPED, MY_WAIT_TIMEOUT));

    // Now check play() is only called when focus to back to FOREGROUND
    EXPECT_CALL(*(m_mockMediaPlayerTrack2.get()), play(_)).Times(1);
    m_audioPlayer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));
}

/**
 * Test when @c AudioPlayer goes to BACKGROUND focus that it changes to PAUSED state.  And when another PLAY directive
 * with REPLACE_ALL behavior comes in, if a Race condition occurs causing the focus change to NONE to execute while
 * still in PAUSED state, that a FOREGROUND change will still play
 */

TEST_F(AudioPlayerTest, testSlow_focusChangeRaceOnPlay) {
    EXPECT_CALL(*(m_mockMediaPlayer.get()), getOffset(_))
        .WillRepeatedly(Return(m_mockMediaPlayer->getOffset(m_mockMediaPlayer->getCurrentSourceId())));
    // play item, then pause, as if barged in
    sendPlayDirective();
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));
    m_audioPlayer->onPlaybackStarted(m_mockMediaPlayer->getCurrentSourceId(), DEFAULT_MEDIA_PLAYER_STATE);
    m_audioPlayer->onFocusChanged(FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PAUSED, MY_WAIT_TIMEOUT));

    m_wakeAcquireChannelPromise = std::promise<void>();
    m_wakeAcquireChannelFuture = m_wakeAcquireChannelPromise.get_future();
    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &AudioPlayerTest::wakeOnAcquireChannel));

    // send a REPLACE_ALL Play directive
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_TEST_2);

    std::shared_ptr<AVSDirective> playDirective =
        AVSDirective::create("", avsMessageHeader, REPLACE_ALL_PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST_2);

    // Send new directive while pasued.
    m_audioPlayer->CapabilityAgent::preHandleDirective(playDirective, std::move(m_mockDirectiveHandlerResult));
    m_audioPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST_2);

    // wait for acquireChannel to be called.  At this point, executePlay will have called executeStop()
    ASSERT_EQ(std::future_status::ready, m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));

    // Now simulate the focus change of BACKGROUD->NONE->BACKGROUND, before the onPlaybackStopped seen in the race
    // condition
    m_audioPlayer->onFocusChanged(FocusState::NONE, MixingBehavior::MUST_STOP);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::STOPPED, MY_WAIT_TIMEOUT));
    m_audioPlayer->onFocusChanged(FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE);

    // We really want to be able to 'waitFor' the focus to change back to BACKGROUND, but there is no way to do that,
    // so we''ll sleep a bit, to givve it a chance
    std::this_thread::sleep_for(std::chrono::milliseconds(EVENT_PROCESS_DELAY));

    // Verify play() is called on FOREGROUND, proving that the queue was not cleared by focus transition
    EXPECT_CALL(*(m_mockMediaPlayerTrack2.get()), play(_)).Times(1);
    m_audioPlayer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));
}

/**
 * Test when @c AudioPlayer starts to play but loses focus before the onPlaybackStarted callback is received.
 * After onPlaybackStarted is received, playback should stop.
 */
TEST_F(AudioPlayerTest, testTimer_playbackStartedCallbackAfterFocusLost) {
    EXPECT_CALL(*(m_mockMediaPlayer.get()), getOffset(_))
        .WillRepeatedly(Return(m_mockMediaPlayer->getOffset(m_mockMediaPlayer->getCurrentSourceId())));

    auto avsMessageHeader =
        std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_TEST, PLAY_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> playDirective = AVSDirective::create(
        "",
        avsMessageHeader,
        createEnqueuePayloadTest(OFFSET_IN_MILLISECONDS_TEST),
        m_attachmentManager,
        CONTEXT_ID_TEST);
    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &AudioPlayerTest::wakeOnAcquireChannel));

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted());

    m_audioPlayer->CapabilityAgent::preHandleDirective(playDirective, std::move(m_mockDirectiveHandlerResult));
    m_audioPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    {
        // Enforce the sequence.
        InSequence dummy;

        std::promise<void> playCalledPromise;
        std::future<void> playCalled = playCalledPromise.get_future();

        // Override play() behavior in MockMediaPlayer. We don't want play() to automatically call onPlaybackStarted.
        ON_CALL(*m_mockMediaPlayer, play(_)).WillByDefault(InvokeWithoutArgs([&playCalledPromise] {
            playCalledPromise.set_value();
            return true;
        }));
        EXPECT_CALL(*m_mockMediaPlayer, stop(_)).Times(1);

        // Wait for acquireFocus().
        ASSERT_EQ(std::future_status::ready, m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
        m_audioPlayer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);

        ASSERT_THAT(playCalled.wait_for(MY_WAIT_TIMEOUT), Ne(std::future_status::timeout));

        m_audioPlayer->onFocusChanged(FocusState::NONE, MixingBehavior::MUST_STOP);
        m_audioPlayer->onPlaybackStarted(m_mockMediaPlayer->getSourceId(), DEFAULT_MEDIA_PLAYER_STATE);

        // Make sure stop() will be called, but STOPPED state is already true
        ASSERT_FALSE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));
    }
}

/**
 * This test sets up a sequence of 1 REPLACE track, followed by 3 ENQUEUE tracks,
 * then tests that they are all played in sequence.
 * It is called from tests that set up different numbers of MediaPlayers in a Factory Pool
 * to ensure everything works smoothly.
 */
void AudioPlayerTest::testPlayEnqueueFinishPlay() {
    // send a play directive
    sendPlayDirective();
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));

    std::this_thread::sleep_for(std::chrono::milliseconds(EVENT_PROCESS_DELAY));

    // Enqueue 3 tracks
    for (int i = 0; i < 3; i++) {
        std::string msgId = MESSAGE_ID_TEST + std::to_string(i);
        auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, msgId);
        std::shared_ptr<AVSDirective> playDirective = AVSDirective::create(
            "",
            avsMessageHeader,
            createEnqueuePayloadTest(OFFSET_IN_MILLISECONDS_TEST, AUDIO_ITEM_ID_1 + std::to_string(i)),
            m_attachmentManager,
            CONTEXT_ID_TEST + std::to_string(i));
        m_audioPlayer->CapabilityAgent::preHandleDirective(playDirective, std::move(m_mockDirectiveHandlerResult));
        m_audioPlayer->CapabilityAgent::handleDirective(msgId);
    }

    m_audioPlayer->onPlaybackFinished(m_mockMediaPlayer->getCurrentSourceId(), DEFAULT_MEDIA_PLAYER_STATE);

    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::FINISHED, MY_WAIT_TIMEOUT));
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));

    m_audioPlayer->onPlaybackFinished(m_mockMediaPlayer->getCurrentSourceId(), DEFAULT_MEDIA_PLAYER_STATE);

    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::FINISHED, MY_WAIT_TIMEOUT));
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));

    m_audioPlayer->onPlaybackFinished(m_mockMediaPlayer->getCurrentSourceId(), DEFAULT_MEDIA_PLAYER_STATE);

    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::FINISHED, MY_WAIT_TIMEOUT));
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));

    m_audioPlayer->onPlaybackFinished(m_mockMediaPlayer->getCurrentSourceId(), DEFAULT_MEDIA_PLAYER_STATE);
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::FINISHED, MY_WAIT_TIMEOUT));
}

TEST_F(AudioPlayerTest, test1PlayerPool_PlayEnqueueFinishPlay) {
    reSetUp(1);

    testPlayEnqueueFinishPlay();
}

TEST_F(AudioPlayerTest, test2PlayerPool_PlayEnqueueFinishPlay) {
    reSetUp(2);

    testPlayEnqueueFinishPlay();
}

TEST_F(AudioPlayerTest, test3PlayerPool_PlayEnqueueFinishPlay) {
    reSetUp(3);

    testPlayEnqueueFinishPlay();
}

/**
 * Test the playRequestor Object can be parsed by the AudioPlayer and reported to its observers via the
 * AudioPlayerObserverInterface.
 */
TEST_F(AudioPlayerTest, testPlayRequestor) {
    auto avsMessageHeader =
        std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_TEST, PLAY_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> playDirective =
        AVSDirective::create("", avsMessageHeader, PLAY_REQUESTOR_PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);
    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &AudioPlayerTest::wakeOnAcquireChannel));

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted());

    m_audioPlayer->CapabilityAgent::preHandleDirective(playDirective, std::move(m_mockDirectiveHandlerResult));
    m_audioPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_EQ(std::future_status::ready, m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));

    m_audioPlayer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);

    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));

    auto playRequestor = m_testAudioPlayerObserver->getPlayRequestorObject();
    EXPECT_EQ(playRequestor.type, PLAY_REQUESTOR_TYPE_ALERT);
    EXPECT_EQ(playRequestor.id, PLAY_REQUESTOR_ID);
}

/**
 * Test that when UpdateProgressReportInterval directive is sent then onProgressReportIntervalUpdated event is called.
 */
TEST_F(AudioPlayerTest, testUpdateProgressReportInterval) {
    sendPlayDirective();

    m_expectedMessages.insert({PROGRESS_REPORT_INTERVAL_UPDATED_NAME, 0});

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            std::lock_guard<std::mutex> lock(m_mutex);
            verifyMessageMap(request, &m_expectedMessages);
            m_messageSentTrigger.notify_one();
        }));

    sendUpdateProgressReportIntervalDirective();

    std::unique_lock<std::mutex> lock(m_mutex);

    bool result;

    result = m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [this] {
        for (auto messageStatus : m_expectedMessages) {
            if (messageStatus.second == 0) {
                return false;
            }
        }
        return true;
    });

    ASSERT_TRUE(result);
}

void AudioPlayerTest::verifyMessageOrder2Phase(
    const std::vector<std::string>& orderedMessageList,
    size_t index,
    std::function<void()> trigger1,
    std::function<void()> trigger2) {
    size_t nextIndex = 0;

    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(
            Invoke([this, orderedMessageList, &nextIndex](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (nextIndex < orderedMessageList.size()) {
                    if (verifyMessage(request, orderedMessageList.at(nextIndex))) {
                        if (nextIndex < orderedMessageList.size()) {
                            nextIndex++;
                        }
                    }
                }
                m_messageSentTrigger.notify_one();
            }));

    trigger1();

    {
        bool phase2 = false;
        bool result;
        std::unique_lock<std::mutex> lock(m_mutex);
        result = m_messageSentTrigger.wait_for(
            lock, MY_WAIT_TIMEOUT, [orderedMessageList, &nextIndex, &index, &phase2, &trigger2] {
                if (nextIndex == index && !phase2) {
                    phase2 = true;
                    trigger2();
                } else if (nextIndex == orderedMessageList.size()) {
                    return true;
                }
                return false;
            });

        ASSERT_TRUE(result);
    }
}

TEST_F(AudioPlayerTest, testTimer_playbackFinishedMessageOrder_1Player) {
    reSetUp(1);

    std::vector<std::string> expectedMessages;

    expectedMessages.push_back(PLAYBACK_STARTED_NAME);
    expectedMessages.push_back(PROGRESS_REPORT_DELAY_ELAPSED_NAME);
    expectedMessages.push_back(PLAYBACK_NEARLY_FINISHED_NAME);
    expectedMessages.push_back(PLAYBACK_FINISHED_NAME);

    verifyMessageOrder2Phase(
        expectedMessages,
        2,
        [this] { sendPlayDirective(); },
        [this] {
            m_audioPlayer->onPlaybackFinished(m_mockMediaPlayer->getCurrentSourceId(), DEFAULT_MEDIA_PLAYER_STATE);
        });
}

TEST_F(AudioPlayerTest, testTimer_playbackFinishedMessageOrder_2Players) {
    reSetUp(2);

    std::vector<std::string> expectedMessages;

    expectedMessages.push_back(PLAYBACK_STARTED_NAME);
    expectedMessages.push_back(PLAYBACK_NEARLY_FINISHED_NAME);
    expectedMessages.push_back(PROGRESS_REPORT_DELAY_ELAPSED_NAME);
    expectedMessages.push_back(PLAYBACK_FINISHED_NAME);

    verifyMessageOrder2Phase(
        expectedMessages,
        3,
        [this] { sendPlayDirective(); },
        [this] {
            m_audioPlayer->onPlaybackFinished(m_mockMediaPlayer->getCurrentSourceId(), DEFAULT_MEDIA_PLAYER_STATE);
        });
}

TEST_F(AudioPlayerTest, testTimer_playbackStoppedMessageOrder_1Player) {
    reSetUp(1);

    std::vector<std::string> expectedMessages;

    expectedMessages.push_back(PLAYBACK_STARTED_NAME);
    expectedMessages.push_back(PROGRESS_REPORT_DELAY_ELAPSED_NAME);
    expectedMessages.push_back(PLAYBACK_STOPPED_NAME);

    verifyMessageOrder2Phase(
        expectedMessages,
        2,
        [this] { sendPlayDirective(); },
        [this] {
            m_audioPlayer->onPlaybackStopped(m_mockMediaPlayer->getCurrentSourceId(), DEFAULT_MEDIA_PLAYER_STATE);
        });
}

TEST_F(AudioPlayerTest, testTimer_playbackStoppedMessageOrder_2Players) {
    reSetUp(2);

    std::vector<std::string> expectedMessages;

    expectedMessages.push_back(PLAYBACK_STARTED_NAME);
    expectedMessages.push_back(PLAYBACK_NEARLY_FINISHED_NAME);
    expectedMessages.push_back(PROGRESS_REPORT_DELAY_ELAPSED_NAME);
    expectedMessages.push_back(PLAYBACK_STOPPED_NAME);

    verifyMessageOrder2Phase(
        expectedMessages,
        3,
        [this] { sendPlayDirective(); },
        [this] {
            m_audioPlayer->onPlaybackStopped(m_mockMediaPlayer->getCurrentSourceId(), DEFAULT_MEDIA_PLAYER_STATE);
        });
}

TEST_F(AudioPlayerTest, test_publishedCapabiltiesContainsFingerprint) {
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> caps =
        m_audioPlayer->getCapabilityConfigurations();
    auto cap = *caps.begin();

    auto configuration = cap->additionalConfigurations.find(CAPABILITY_INTERFACE_CONFIGURATIONS_KEY);
    ASSERT_NE(configuration, cap->additionalConfigurations.end());

    JsonGenerator expectedConfigurations;
    expectedConfigurations.startObject(FINGERPRINT_KEY);
    expectedConfigurations.addMember(FINGERPRINT_PACKAGE_KEY, FINGERPRINT.package);
    expectedConfigurations.addMember(FINGERPRINT_BUILD_TYPE_KEY, FINGERPRINT.buildType);
    expectedConfigurations.addMember(FINGERPRINT_VERSION_NUMBER_KEY, FINGERPRINT.versionNumber);

    ASSERT_EQ(expectedConfigurations.toString(), configuration->second);
}

TEST_F(AudioPlayerTest, test_localStop) {
    sendPlayDirective();

    EXPECT_CALL(*(m_mockMediaPlayer.get()), stop(_)).Times(AtLeast(1));

    // now send Stop directive
    ASSERT_TRUE(m_audioPlayer->localOperation(LocalPlaybackHandlerInterface::PlaybackOperation::STOP_PLAYBACK));
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::STOPPED, MY_WAIT_TIMEOUT));
}

TEST_F(AudioPlayerTest, test_localPause) {
    sendPlayDirective();

    EXPECT_CALL(*(m_mockMediaPlayer.get()), stop(_, _)).Times(AtLeast(1));

    ASSERT_TRUE(m_audioPlayer->localOperation(LocalPlaybackHandlerInterface::PlaybackOperation::PAUSE_PLAYBACK));
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::STOPPED, MY_WAIT_TIMEOUT));
}

TEST_F(AudioPlayerTest, test_localResumeAfterPaused) {
    sendPlayDirective();

    // pause playback
    ASSERT_TRUE(m_audioPlayer->localOperation(LocalPlaybackHandlerInterface::PlaybackOperation::PAUSE_PLAYBACK));
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::STOPPED, MY_WAIT_TIMEOUT));

    // verify mediaplayer resumes
    EXPECT_CALL(*(m_mockMediaPlayer.get()), play(_)).Times(AtLeast(1));
    m_mockMediaPlayer->resetWaitTimer();

    ASSERT_TRUE(m_audioPlayer->localOperation(LocalPlaybackHandlerInterface::PlaybackOperation::RESUME_PLAYBACK));
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));
}

TEST_F(AudioPlayerTest, test_localSeekTo) {
    sendPlayDirective();

    EXPECT_CALL(*(m_mockMediaPlayer.get()), seekTo(_, _, _)).Times(AtLeast(1));

    std::chrono::milliseconds position = std::chrono::milliseconds::zero();
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this, &position](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            std::lock_guard<std::mutex> lock(m_mutex);
            MediaPlayerState state;
            // Commented out untile AudioPlayer 1.6, when this will be correct
            //            if (extractMediaPlayerState(request, SEEK_COMPLETE_NAME, &state)) {
            if (extractMediaPlayerState(request, PLAYBACK_STARTED_NAME, &state)) {
                position = state.offset;
            }
            m_messageSentTrigger.notify_one();
        }));

    auto pos = std::chrono::milliseconds(5000);
    m_audioPlayer->localSeekTo(pos, true);

    {
        bool result;
        std::unique_lock<std::mutex> lock(m_mutex);
        result = m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [&position, &pos] {
            if (position == pos) {
                return true;
            }
            return false;
        });

        ASSERT_TRUE(result);
    }
}

TEST_F(AudioPlayerTest, test_localSeekToWhileLocalStopped) {
    sendPlayDirective();

    EXPECT_CALL(*(m_mockMediaPlayer.get()), seekTo(_, _, _)).Times(AtLeast(1));

    std::chrono::milliseconds position = std::chrono::milliseconds::zero();
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this, &position](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            std::lock_guard<std::mutex> lock(m_mutex);
            MediaPlayerState state;
            // Commented out untile AudioPlayer 1.6, when this will be correct
            //            if (extractMediaPlayerState(request, SEEK_COMPLETE_NAME, &state)) {
            if (extractMediaPlayerState(request, PLAYBACK_STOPPED_NAME, &state)) {
                position = state.offset;
            }
            m_messageSentTrigger.notify_one();
        }));

    // pause playback
    ASSERT_TRUE(m_audioPlayer->localOperation(LocalPlaybackHandlerInterface::PlaybackOperation::PAUSE_PLAYBACK));
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::STOPPED, MY_WAIT_TIMEOUT));

    auto pos = std::chrono::milliseconds(5000);
    m_audioPlayer->localSeekTo(pos, true);

    {
        bool result;
        std::unique_lock<std::mutex> lock(m_mutex);
        result = m_messageSentTrigger.wait_for(lock, MY_WAIT_TIMEOUT, [&position, &pos] {
            if (position == pos) {
                return true;
            }
            return false;
        });

        ASSERT_TRUE(result);
    }

    EXPECT_CALL(*(m_mockMediaPlayer.get()), play(_)).Times(AtLeast(1));
    m_mockMediaPlayer->resetWaitTimer();

    ASSERT_TRUE(m_audioPlayer->localOperation(LocalPlaybackHandlerInterface::PlaybackOperation::RESUME_PLAYBACK));
    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));
}

TEST_F(AudioPlayerTest, DISABLED_test_endOffset) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_TEST_2);

    std::string endOffsetPayload = createPayloadWithEndOffset(100L, 1500L);
    std::shared_ptr<AVSDirective> playDirective =
        AVSDirective::create("", avsMessageHeader, endOffsetPayload, m_attachmentManager, CONTEXT_ID_TEST_2);
    m_audioPlayer->CapabilityAgent::preHandleDirective(playDirective, std::move(m_mockDirectiveHandlerResult));
    m_mockMediaPlayer->waitUntilNextSetSource(MY_WAIT_TIMEOUT);
    m_audioPlayer->onBufferingComplete(m_mockMediaPlayer->getLatestSourceId(), DEFAULT_MEDIA_PLAYER_STATE);
    m_audioPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST_2);

    m_audioPlayer->onFocusChanged(FocusState::FOREGROUND, avs::MixingBehavior::PRIMARY);

    ASSERT_TRUE(m_testAudioPlayerObserver->waitFor(PlayerActivity::PLAYING, MY_WAIT_TIMEOUT));
}

TEST_F(AudioPlayerTest, test_badEndOffset) {
    // End offset less than start offset
    badEndOffsetDirective(100L, 50L);
}

TEST_F(AudioPlayerTest, test_badEndOffsetEqualValue) {
    // End offset equal to start offset
    badEndOffsetDirective(100L, 100L);
}

}  // namespace test
}  // namespace acsdkAudioPlayer
}  // namespace alexaClientSDK
