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

#include <chrono>
#include <future>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/AVS/Attachment/AttachmentManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockFocusManager.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveSequencer.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockPowerResourceManager.h>
#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/Utils/MediaPlayer/MockMediaPlayer.h>
#include <AVSCommon/Utils/Metrics/MockMetricRecorder.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <MockCaptionManager.h>

#include "SpeechSynthesizer/SpeechSynthesizer.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace speechSynthesizer {
namespace test {

using namespace avsCommon::utils;
using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils::mediaPlayer;
using namespace avsCommon::utils::mediaPlayer::test;
using namespace avsCommon::utils::metrics;
using namespace captions::test;
using namespace ::testing;
using MediaPlayerState = avsCommon::utils::mediaPlayer::MediaPlayerState;
using PowerResourceLevel = PowerResourceManagerInterface::PowerResourceLevel;

/// Plenty of time for a test to complete.
static const std::chrono::milliseconds MY_WAIT_TIMEOUT(1000);

/// Default media player state for all playback events
static const MediaPlayerState DEFAULT_MEDIA_PLAYER_STATE = {std::chrono::milliseconds(0)};

/// Time to wait for state change timeout.  This should be set to be longer than STATE_CHANGE_TIMEOUT in
/// SpeechSynthesizer.
static const std::chrono::milliseconds STATE_CHANGE_TIMEOUT(10000);

/// The name of the @c FocusManager channel used by the @c SpeechSynthesizer.
static const std::string CHANNEL_NAME(avsCommon::sdkInterfaces::FocusManagerInterface::DIALOG_CHANNEL_NAME);

/// Namespace for SpeechSynthesizer.
static const std::string NAMESPACE_SPEECH_SYNTHESIZER("SpeechSynthesizer");

/// Name for SpeechSynthesizer directive.
static const std::string NAME_SPEAK("Speak");

/// The name of the event to send to the AVS server once audio starting playing.
static std::string SPEECH_STARTED_EVENT_NAME{"SpeechStarted"};

/// The name of the event to send to the AVS server once audio finishes playing.
static std::string SPEECH_FINISHED_EVENT_NAME{"SpeechFinished"};

/// The name of the event to send to the AVS server once audio playing has been interrupted.
static std::string SPEECH_INTERRUPTED_EVENT_NAME{"SpeechInterrupted"};

/// Wrong name for testing.
static const std::string NAME_RECOGNIZE("Recognize");

/// The @c NamespaceAndName to send to the @c ContextManager.
static const NamespaceAndName NAMESPACE_AND_NAME_SPEECH_STATE{NAMESPACE_SPEECH_SYNTHESIZER, "SpeechState"};

/// Message Id for testing.
static const std::string MESSAGE_ID_TEST("MessageId_Test");

/// Message Id for testing.
static const std::string MESSAGE_ID_TEST_2("MessageId_Test_2");

/// DialogRequestId for testing.
static const std::string DIALOG_REQUEST_ID_TEST("DialogRequestId_Test");

/// Token for testing.
static const std::string TOKEN_TEST("Token_Test");

/// Format of the audio.
static const std::string FORMAT_TEST("AUDIO_MPEG");

/// URL for testing.
static const std::string URL_TEST("cid:Test");

/// Context ID for testing
static const std::string CONTEXT_ID_TEST("ContextId_Test");

/// Context ID for testing
static const std::string CONTEXT_ID_TEST_2("ContextId_Test_2");

static const std::string CAPTION_CONTENT_SAMPLE =
    "WEBVTT\\n"
    "\\n"
    "1\\n"
    "00:00.000 --> 00:01.260\\n"
    "The time is 2:17 PM.";

/// A payload for testing
// clang-format off
static const std::string PAYLOAD_TEST =
        "{"
            "\"url\":\"" + URL_TEST + "\","
            "\"format\":\"" + FORMAT_TEST + "\","
            "\"token\":\""+ TOKEN_TEST + "\","
            "\"caption\": {"
                "\"content\":\"" + CAPTION_CONTENT_SAMPLE + "\","
                "\"type\":\"WEBVTT\""
            "}"
        "}";
// clang-format on

/// A payload for testing with single audio analyzer entry
// clang-format off
static const std::string PAYLOAD_TEST_SINGLE_ANALYZER =
        "{"
            "\"url\":\"" + URL_TEST + "\","
            "\"format\":\"" + FORMAT_TEST + "\","
            "\"token\":\""+ TOKEN_TEST + "\","
            "\"caption\": {"
                "\"content\":\"" + CAPTION_CONTENT_SAMPLE + "\","
                "\"type\":\"WEBVTT\""
            "},"
            "\"analyzers\":[{\"interface\":\"analyzername\", \"enabled\":\"YES\"}]"
        "}";
// clang-format on

/// A payload for testing with multiple audio analyzer entry
// clang-format off
static const std::string PAYLOAD_TEST_MULTIPLE_ANALYZER =
        "{"
            "\"url\":\"" + URL_TEST + "\","
            "\"format\":\"" + FORMAT_TEST + "\","
            "\"token\":\""+ TOKEN_TEST + "\","
            "\"caption\": {"
                "\"content\":\"" + CAPTION_CONTENT_SAMPLE + "\","
                "\"type\":\"WEBVTT\""
            "},"
            "\"analyzers\":["
                "{\"interface\":\"analyzername1\", \"enabled\":\"YES\"},"
                "{\"interface\":\"analyzername2\", \"enabled\":\"NO\"}"
            "]"
        "}";
// clang-format on

/// The @c FINISHED state of the @c SpeechSynthesizer.
static const std::string FINISHED_STATE("FINISHED");

/// The @c PLAYING state of the @c SpeechSynthesizer
static const std::string PLAYING_STATE{"PLAYING"};

/// The @c INTERRUPTED state of the @c SpeechSynthesizer
static const std::string INTERRUPTED_STATE{"INTERRUPTED"};

/// The offset in milliseconds returned by the mock media player.
static const long OFFSET_IN_MILLISECONDS_TEST{100};

/// An std::chrono::milliseconds representation of the offset.
static const std::chrono::milliseconds OFFSET_IN_CHRONO_MILLISECONDS_TEST{100};

/// The expected state when the @c SpeechSynthesizer is in @c PLAYING state.
// clang-format off
static const std::string PLAYING_STATE_TEST =
    "{"
        "\"token\":\"" + TOKEN_TEST + "\","
        "\"offsetInMilliseconds\":" + std::to_string(OFFSET_IN_MILLISECONDS_TEST) + ","
        "\"playerActivity\":\"" + PLAYING_STATE + "\""
    "}";
// clang-format on

/// The expected state when the @c SpeechSynthesizer is in @c FINISHED state.
// clang-format off
static const std::string FINISHED_STATE_TEST =
    "{"
        "\"token\":\"" + TOKEN_TEST + "\","
        "\"offsetInMilliseconds\":" + std::to_string(0) + ","
        "\"playerActivity\":\"" + FINISHED_STATE + "\""
    "}";
// clang-format on

/// The expected state when the @c SpeechSynthesizer is in @c INTERRUPTED state.
// clang-format off
static const std::string INTERRUPTED_STATE_TEST =
    "{"
        "\"token\":\"" + TOKEN_TEST + "\","
        "\"offsetInMilliseconds\":" + std::to_string(OFFSET_IN_MILLISECONDS_TEST) + ","
        "\"playerActivity\":\"" + INTERRUPTED_STATE + "\""
    "}";
// clang-format on

/// The expected state when the @c SpeechSynthesizer is not handling any directive.
// clang-format off
static const std::string IDLE_STATE_TEST =
    "{"
        "\"token\":\"\","
        "\"offsetInMilliseconds\":" + std::to_string(0) + ","
        "\"playerActivity\":\"" + FINISHED_STATE + "\""
    "}";
// clang-format on

/// Provide State Token for testing.
static const unsigned int PROVIDE_STATE_TOKEN_TEST{1};

/// Component name for power resource management.
static const std::string COMPONENT_NAME("SpeechSynthesizer");

/**
 * Store useful information about a mock Speak Directive.
 */
struct SpeakTestInfo {
    /// The payload content.
    const std::string payload;
    /// The message id.
    const std::string messageId;
    /// The directive token.
    const std::string token;
};

static SpeakTestInfo generateSpeakInfo(PlayBehavior playBehavior) {
    json::JsonGenerator generator;
    static int id = 0;
    std::string idStr = "_" + std::to_string(id++);
    std::string token = TOKEN_TEST + idStr;
    generator.addMember("url", URL_TEST + idStr);
    generator.addMember("format", FORMAT_TEST);
    generator.addMember("playBehavior", playBehaviorToString(playBehavior));
    generator.addMember("token", token);
    return SpeakTestInfo{generator.toString(), (MESSAGE_ID_TEST + idStr), token};
}

static std::string generatePlayingState(const SpeakTestInfo& info) {
    return std::string(PLAYING_STATE_TEST).replace(PLAYING_STATE_TEST.find(TOKEN_TEST), TOKEN_TEST.size(), info.token);
}

static std::string generateFinishedState(const SpeakTestInfo& info) {
    return std::string(FINISHED_STATE_TEST)
        .replace(FINISHED_STATE_TEST.find(TOKEN_TEST), TOKEN_TEST.size(), info.token);
}

static std::string generateInterruptedState(const SpeakTestInfo& info) {
    return std::string(INTERRUPTED_STATE_TEST)
        .replace(INTERRUPTED_STATE_TEST.find(TOKEN_TEST), TOKEN_TEST.size(), info.token);
}

class MockSpeechSynthesizerObserver : public SpeechSynthesizerObserverInterface {
public:
    MOCK_METHOD4(
        onStateChanged,
        void(
            SpeechSynthesizerObserverInterface::SpeechSynthesizerState state,
            const mediaPlayer::MediaPlayerInterface::SourceId mediaSourceId,
            const Optional<mediaPlayer::MediaPlayerState>& mediaPlayerState,
            const std::vector<audioAnalyzer::AudioAnalyzerState>& audioAnalyzerState));
};

class SpeechSynthesizerTest : public ::testing::Test {
public:
    SpeechSynthesizerTest();

    void SetUp() override;
    void TearDown() override;

    /// @c SpeechSynthesizer to test
    std::shared_ptr<SpeechSynthesizer> m_speechSynthesizer;

    /// Player to send the audio to.
    std::shared_ptr<MockMediaPlayer> m_mockSpeechPlayer;

    /// @c ContextManager to provide state and update state.
    std::shared_ptr<MockContextManager> m_mockContextManager;

    /// Mock SpeechSynthesizerObserver for testing.
    std::shared_ptr<MockSpeechSynthesizerObserver> m_mockSpeechSynthesizerObserver;

    /**
     * Fulfills the @c m_wakeSetStatePromise. This is invoked in response to a @c setState call.
     *
     * @return @c SUCCESS.
     */
    SetStateResult wakeOnSetState();

    /// Promise to be fulfilled when @c setState is called.
    std::promise<void> m_wakeSetStatePromise;

    /// Future to notify when @c setState is called.
    std::future<void> m_wakeSetStateFuture;

    /// @c FocusManager to request focus to the DIALOG channel.
    std::shared_ptr<MockFocusManager> m_mockFocusManager;

    /**
     * Fulfills the @c m_wakeAcquireChannelPromise. This is invoked in response to a @c acquireChannel call.
     *
     * @return @c true
     */
    bool wakeOnAcquireChannel();

    /// Promise to be fulfilled when @c acquireChannel is called.
    std::promise<void> m_wakeAcquireChannelPromise;

    /// Future to notify when @c acquireChannel is called.
    std::future<void> m_wakeAcquireChannelFuture;

    /**
     * Fulfills the @c m_wakeReleaseChannelPromise. This is invoked in response to a @c releaseChannel call.
     *
     * @return @c true
     */
    std::future<bool> wakeOnReleaseChannel();

    /// Promise to be fulfilled when @c releaseChannel is called.
    std::promise<void> m_wakeReleaseChannelPromise;

    /// Future to notify when @c releaseChannel is called.
    std::future<void> m_wakeReleaseChannelFuture;

    /// A directive handler result to send the result to.
    std::unique_ptr<MockDirectiveHandlerResult> m_mockDirHandlerResult;

    /**
     * Fulfills the @c m_wakeSetCompletedPromise. This is invoked in response to a @c setCompleted call.
     */
    void wakeOnSetCompleted();

    /// Promise to be fulfilled when @c setCompleted is called.
    std::promise<void> m_wakeSetCompletedPromise;

    /// Future to notify when @c setCompleted is called.
    std::future<void> m_wakeSetCompletedFuture;

    /**
     * Fulfills the @c m_wakeSetFailedPromise. This is invoked in response to a @c setFailed call.
     */
    void wakeOnSetFailed();

    /// Promise to be fulfilled when @c setFailed is called.
    std::promise<void> m_wakeSetFailedPromise;

    /// Future to notify when @c setFailed is called.
    std::future<void> m_wakeSetFailedFuture;

    /// A message sender used to send events to AVS.
    std::shared_ptr<MockMessageSender> m_mockMessageSender;

    /**
     * Fulfills the @c m_wakeSendMessagePromise. This is invoked in response to a @c sendMessage call.
     */
    void wakeOnSendMessage();

    /// Promise to be fulfilled when @c sendMessage is called.
    std::promise<void> m_wakeSendMessagePromise;

    /// Future to notify when @c sendMessage is called.
    std::future<void> m_wakeSendMessageFuture;

    /**
     * Fulfills the @c m_wakeStoppedPromise. This is invoked in response to a @c stop call.
     */
    void wakeOnStopped();

    /// Promise to be fulfilled when @c stop is called.
    std::promise<void> m_wakeStoppedPromise;

    /// Future to notify when @c stop is called.
    std::future<void> m_wakeStoppedFuture;

    /// An exception sender used to send exception encountered events to AVS.
    std::shared_ptr<MockExceptionEncounteredSender> m_mockExceptionSender;

    /// Attachment manager used to create a reader.
    std::shared_ptr<AttachmentManager> m_attachmentManager;

    /// Metric recorder used to record metric.
    std::shared_ptr<MetricRecorderInterface> m_metricRecorder;

    /// The @c DialogUXStateAggregator to test with.
    std::shared_ptr<avsCommon::avs::DialogUXStateAggregator> m_dialogUXStateAggregator;

    /// A mock @c CaptionManager instance to handle captions parsing.
    std::shared_ptr<MockCaptionManager> m_mockCaptionManager;

    /// The mock @c PowerResourceManagerInterface
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockPowerResourceManager> m_mockPowerResourceManager;

    /**
     * Setup speech synthesizer to have a pending speech directive.
     *
     * @param resultHandler the result handler for the new speech directive.
     * @param info Information used to generate the new speech directive.
     * @return @c true if it succeed; @c false otherwise.
     */
    bool setupPendingSpeech(std::unique_ptr<DirectiveHandlerResultInterface> resultHandler, const SpeakTestInfo& info);

    /**
     * Setup speech synthesizer to have an active speech directive.
     *
     * @param resultHandler the result handler for the new speech directive.
     * @param info Information used to generate and activate the new speech directive.
     * @return @c true if it succeed; @c false otherwise.
     */
    bool setupActiveSpeech(std::unique_ptr<DirectiveHandlerResultInterface> resultHandler, const SpeakTestInfo& info);
};

SpeechSynthesizerTest::SpeechSynthesizerTest() :
        m_wakeSetStatePromise{},
        m_wakeSetStateFuture{m_wakeSetStatePromise.get_future()},
        m_wakeAcquireChannelPromise{},
        m_wakeAcquireChannelFuture{m_wakeAcquireChannelPromise.get_future()},
        m_wakeReleaseChannelPromise{},
        m_wakeReleaseChannelFuture{m_wakeReleaseChannelPromise.get_future()},
        m_wakeSetCompletedPromise{},
        m_wakeSetCompletedFuture{m_wakeSetCompletedPromise.get_future()},
        m_wakeSetFailedPromise{},
        m_wakeSetFailedFuture{m_wakeSetFailedPromise.get_future()},
        m_wakeSendMessagePromise{},
        m_wakeSendMessageFuture{m_wakeSendMessagePromise.get_future()},
        m_wakeStoppedPromise{},
        m_wakeStoppedFuture{m_wakeStoppedPromise.get_future()} {
}

void SpeechSynthesizerTest::SetUp() {
    m_metricRecorder = std::make_shared<NiceMock<avsCommon::utils::metrics::test::MockMetricRecorder>>();
    m_mockContextManager = std::make_shared<NiceMock<MockContextManager>>();
    m_mockFocusManager = std::make_shared<NiceMock<MockFocusManager>>();
    m_mockMessageSender = std::make_shared<NiceMock<MockMessageSender>>();
    m_mockExceptionSender = std::make_shared<NiceMock<MockExceptionEncounteredSender>>();
    m_attachmentManager = std::make_shared<AttachmentManager>(AttachmentManager::AttachmentType::IN_PROCESS);
    m_mockSpeechPlayer = MockMediaPlayer::create();
    m_dialogUXStateAggregator = std::make_shared<avsCommon::avs::DialogUXStateAggregator>();
    m_mockCaptionManager = std::make_shared<NiceMock<MockCaptionManager>>();
    m_mockPowerResourceManager = std::make_shared<avsCommon::sdkInterfaces::test::MockPowerResourceManager>();
    m_mockSpeechSynthesizerObserver = std::make_shared<MockSpeechSynthesizerObserver>();
    m_speechSynthesizer = SpeechSynthesizer::create(
        m_mockSpeechPlayer,
        m_mockMessageSender,
        m_mockFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        m_metricRecorder,
        m_dialogUXStateAggregator,
        m_mockCaptionManager,
        m_mockPowerResourceManager);
    m_mockDirHandlerResult.reset(new MockDirectiveHandlerResult);

    ASSERT_TRUE(m_speechSynthesizer);

    m_speechSynthesizer->addObserver(m_dialogUXStateAggregator);
}

void SpeechSynthesizerTest::TearDown() {
    m_speechSynthesizer->shutdown();
    m_mockSpeechPlayer->shutdown();
}

SetStateResult SpeechSynthesizerTest::wakeOnSetState() {
    m_wakeSetStatePromise.set_value();
    return SetStateResult::SUCCESS;
}

bool SpeechSynthesizerTest::wakeOnAcquireChannel() {
    m_wakeAcquireChannelPromise.set_value();
    return true;
}

std::future<bool> SpeechSynthesizerTest::wakeOnReleaseChannel() {
    std::promise<bool> releaseChannelSuccess;
    std::future<bool> returnValue = releaseChannelSuccess.get_future();
    releaseChannelSuccess.set_value(true);
    m_wakeReleaseChannelPromise.set_value();
    return returnValue;
}

void SpeechSynthesizerTest::wakeOnSetCompleted() {
    m_wakeSetCompletedPromise.set_value();
}

void SpeechSynthesizerTest::wakeOnSetFailed() {
    m_wakeSetFailedPromise.set_value();
}

void SpeechSynthesizerTest::wakeOnSendMessage() {
    m_wakeSendMessagePromise.set_value();
}

void SpeechSynthesizerTest::wakeOnStopped() {
    m_wakeStoppedPromise.set_value();
}

/**
 * Match request by the event content. This does a simple string search.
 *
 * @param request The request to be checked.
 * @param expectedContent The content that should be included in the request.
 * @return If the content was found or not.
 */
static bool matchEvent(std::shared_ptr<MessageRequest> request, const std::string& expectedContent) {
    return request && (request->getJsonContent().find(expectedContent) != std::string::npos);
}

MATCHER(IsStartedEvent, "") {
    return matchEvent(arg, SPEECH_STARTED_EVENT_NAME);
}
MATCHER(IsFinishedEvent, "") {
    return matchEvent(arg, SPEECH_FINISHED_EVENT_NAME);
}
MATCHER(IsInterruptedEvent, "") {
    return matchEvent(arg, SPEECH_INTERRUPTED_EVENT_NAME);
}

/**
 * Test call to handleDirectiveImmediately.
 * Expected result is that @c acquireChannel is called with the correct channel. On focus changed @c FOREGROUND, audio
 * should play. Expect the @c ContextManager @c setState is called when state changes to @c PLAYING.
 */
TEST_F(SpeechSynthesizerTest, test_callingHandleImmediately) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnAcquireChannel));
    EXPECT_CALL(
        *(m_mockSpeechPlayer.get()),
        attachmentSetSource(A<std::shared_ptr<avsCommon::avs::attachment::AttachmentReader>>(), nullptr))
        .Times(AtLeast(1));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), play(_)).Times(AtLeast(1));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), getOffset(_))
        .Times(1)
        .WillOnce(Return(OFFSET_IN_CHRONO_MILLISECONDS_TEST));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), getMediaPlayerState(_)).Times(AtLeast(2));
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(NAMESPACE_AND_NAME_SPEECH_STATE, PLAYING_STATE_TEST, StateRefreshPolicy::ALWAYS, 0))
        .Times(AtLeast(1))
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
    EXPECT_CALL(*m_mockCaptionManager, onCaption(_, _)).Times(1);
    EXPECT_CALL(*m_mockPowerResourceManager, acquirePowerResource(COMPONENT_NAME, PowerResourceLevel::ACTIVE_HIGH))
        .Times(AtLeast(1));

    std::vector<audioAnalyzer::AudioAnalyzerState> data;
    EXPECT_CALL(
        *m_mockSpeechSynthesizerObserver,
        onStateChanged(SpeechSynthesizerObserverInterface::SpeechSynthesizerState::GAINING_FOCUS, _, _, _))
        .Times(1);
    EXPECT_CALL(
        *m_mockSpeechSynthesizerObserver,
        onStateChanged(SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING, _, _, Eq(data)))
        .Times(1);

    m_speechSynthesizer->addObserver(m_mockSpeechSynthesizerObserver);
    m_speechSynthesizer->handleDirectiveImmediately(directive);
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    ASSERT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Tests preHandleDirective and HandleDirective
 * Call preHandle with a valid SPEAK directive. Then call handleDirective. Expected result is that @c acquireChannel
 * is called with the correct channel. On focus changed @c FOREGROUND, audio should play. Expect the @c ContextManager
 * @c setState is called when state changes to @c PLAYING.
 */
TEST_F(SpeechSynthesizerTest, test_callingHandle) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnAcquireChannel));
    EXPECT_CALL(
        *(m_mockSpeechPlayer.get()),
        attachmentSetSource(A<std::shared_ptr<avsCommon::avs::attachment::AttachmentReader>>(), nullptr))
        .Times(AtLeast(1));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), play(_)).Times(AtLeast(1));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), getOffset(_))
        .Times(1)
        .WillOnce(Return(OFFSET_IN_CHRONO_MILLISECONDS_TEST));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), getMediaPlayerState(_)).Times(AtLeast(2));
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(NAMESPACE_AND_NAME_SPEECH_STATE, PLAYING_STATE_TEST, StateRefreshPolicy::ALWAYS, 0))
        .Times(AtLeast(1))
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
    EXPECT_CALL(*(m_mockDirHandlerResult.get()), setFailed(_))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetFailed));
    EXPECT_CALL(*m_mockCaptionManager, onCaption(_, _)).Times(1);
    EXPECT_CALL(*m_mockPowerResourceManager, acquirePowerResource(COMPONENT_NAME, PowerResourceLevel::ACTIVE_HIGH))
        .Times(AtLeast(1));

    m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirHandlerResult));
    m_speechSynthesizer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    ASSERT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Tests cancelDirective.
 * Call preHandle with a valid SPEAK directive. Then call cancelDirective. Expect that neither @c setState nor
 * @c sendMessage are called since handle was never called to start playing audio.
 */
TEST_F(SpeechSynthesizerTest, test_callingCancel) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    EXPECT_CALL(*(m_mockContextManager.get()), setState(_, _, _, _)).Times(0);
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_)).Times(0);

    m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirHandlerResult));
    m_speechSynthesizer->CapabilityAgent::cancelDirective(MESSAGE_ID_TEST);
}

/**
 * Testing cancelDirective after calling
 * Call preHandle with a valid SPEAK directive. Then call handleDirective. Expected result is that @c acquireChannel
 * is called once. On Focus Changed to foreground, audio should play. Call cancel directive. Expect the
 * @c ContextManager @c setState is called when the state changes to @c PLAYING and then to @c INTERRUPTED.
 * Expect @c sendMessage is called twice (SpeechStarted and SpeechInterrupted).
 */
TEST_F(SpeechSynthesizerTest, test_callingCancelAfterHandle) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnAcquireChannel));
    EXPECT_CALL(
        *(m_mockSpeechPlayer.get()),
        attachmentSetSource(A<std::shared_ptr<avsCommon::avs::attachment::AttachmentReader>>(), nullptr))
        .Times(AtLeast(1));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), play(_)).Times(AtLeast(1));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), getOffset(_))
        .Times(AtLeast(2))
        .WillRepeatedly(Return(OFFSET_IN_CHRONO_MILLISECONDS_TEST));
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(NAMESPACE_AND_NAME_SPEECH_STATE, PLAYING_STATE_TEST, StateRefreshPolicy::ALWAYS, 0))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(NAMESPACE_AND_NAME_SPEECH_STATE, INTERRUPTED_STATE_TEST, StateRefreshPolicy::NEVER, 0))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(IsStartedEvent()))
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
    EXPECT_CALL(*(m_mockFocusManager.get()), releaseChannel(CHANNEL_NAME, _))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnReleaseChannel));
    EXPECT_CALL(*(m_mockDirHandlerResult.get()), setFailed(_))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetFailed));
    EXPECT_CALL(*m_mockPowerResourceManager, acquirePowerResource(COMPONENT_NAME, PowerResourceLevel::ACTIVE_HIGH))
        .Times(AtLeast(1));

    m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirHandlerResult));
    m_speechSynthesizer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();
    ASSERT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(MY_WAIT_TIMEOUT));
    m_wakeSendMessagePromise = std::promise<void>();
    m_wakeSendMessageFuture = m_wakeSendMessagePromise.get_future();
    m_speechSynthesizer->CapabilityAgent::cancelDirective(MESSAGE_ID_TEST);
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(IsInterruptedEvent()))
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
    EXPECT_CALL(*m_mockPowerResourceManager, releasePowerResource(COMPONENT_NAME)).Times(AtLeast(1));
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStopped());
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    ASSERT_TRUE(std::future_status::ready == m_wakeReleaseChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    ASSERT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Testing provideState.
 * Call @c provideState and expect that setState is called.
 */
TEST_F(SpeechSynthesizerTest, test_callingProvideStateWhenNotPlaying) {
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), getOffset(_)).Times(0);
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(NAMESPACE_AND_NAME_SPEECH_STATE, IDLE_STATE_TEST, StateRefreshPolicy::NEVER, PROVIDE_STATE_TOKEN_TEST))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));

    m_speechSynthesizer->provideState(NAMESPACE_AND_NAME_SPEECH_STATE, PROVIDE_STATE_TOKEN_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Testing provideState when playing.
 * Call @c provideState after the state of the @c SpeechSynthesizer has changed to @c PLAYING.
 * Expect @c getOffset is called. Expect @c setState is called when state changes and when state is
 * requested via @c provideState.
 */
TEST_F(SpeechSynthesizerTest, test_callingProvideStateWhenPlaying) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnAcquireChannel));
    EXPECT_CALL(
        *(m_mockSpeechPlayer.get()),
        attachmentSetSource(A<std::shared_ptr<avsCommon::avs::attachment::AttachmentReader>>(), nullptr))
        .Times(AtLeast(1));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), play(_)).Times(AtLeast(1));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), getOffset(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(OFFSET_IN_CHRONO_MILLISECONDS_TEST));
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(NAMESPACE_AND_NAME_SPEECH_STATE, PLAYING_STATE_TEST, StateRefreshPolicy::ALWAYS, 0))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(
            NAMESPACE_AND_NAME_SPEECH_STATE, PLAYING_STATE_TEST, StateRefreshPolicy::ALWAYS, PROVIDE_STATE_TOKEN_TEST))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
    EXPECT_CALL(*m_mockPowerResourceManager, acquirePowerResource(COMPONENT_NAME, PowerResourceLevel::ACTIVE_HIGH))
        .Times(AtLeast(1));

    m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirHandlerResult));
    m_speechSynthesizer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();
    ASSERT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(MY_WAIT_TIMEOUT));
    m_speechSynthesizer->provideState(NAMESPACE_AND_NAME_SPEECH_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Testing barge-in via @c handleDirectiveImmediately when audio is playing back.
 * Call @c handleDirective. Once playback started notification is received, call @c handleDirectiveImmediately.
 */
TEST_F(SpeechSynthesizerTest, testTimer_bargeInWhilePlaying) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    auto avsMessageHeader2 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST_2);
    std::shared_ptr<AVSDirective> directive2 =
        AVSDirective::create("", avsMessageHeader2, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST_2);

    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnAcquireChannel));
    EXPECT_CALL(
        *(m_mockSpeechPlayer.get()),
        attachmentSetSource(A<std::shared_ptr<avsCommon::avs::attachment::AttachmentReader>>(), nullptr))
        .Times(AtLeast(1));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), play(_)).Times(AtLeast(1));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), getOffset(_))
        .Times(AtLeast(2))
        .WillRepeatedly(Return(OFFSET_IN_CHRONO_MILLISECONDS_TEST));
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(NAMESPACE_AND_NAME_SPEECH_STATE, PLAYING_STATE_TEST, StateRefreshPolicy::ALWAYS, 0))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(NAMESPACE_AND_NAME_SPEECH_STATE, INTERRUPTED_STATE_TEST, StateRefreshPolicy::NEVER, 0))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(IsStartedEvent()))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
    EXPECT_CALL(*(m_mockDirHandlerResult.get()), setFailed(_))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetFailed));
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(IsInterruptedEvent()))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
    EXPECT_CALL(*(m_mockFocusManager.get()), releaseChannel(CHANNEL_NAME, _))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnReleaseChannel));
    EXPECT_CALL(*m_mockPowerResourceManager, acquirePowerResource(COMPONENT_NAME, PowerResourceLevel::ACTIVE_HIGH))
        .Times(AtLeast(1));

    m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirHandlerResult));
    m_speechSynthesizer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_wakeAcquireChannelPromise = std::promise<void>();
    m_wakeAcquireChannelFuture = m_wakeAcquireChannelPromise.get_future();
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();
    ASSERT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(MY_WAIT_TIMEOUT));
    m_wakeSendMessagePromise = std::promise<void>();
    m_wakeSendMessageFuture = m_wakeSendMessagePromise.get_future();

    EXPECT_CALL(*m_mockPowerResourceManager, releasePowerResource(COMPONENT_NAME)).Times(AtLeast(1));
    m_speechSynthesizer->CapabilityAgent::cancelDirective(MESSAGE_ID_TEST);
    m_speechSynthesizer->handleDirectiveImmediately(directive2);
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStopped());
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    ASSERT_TRUE(std::future_status::ready == m_wakeReleaseChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    ASSERT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Testing SpeechSynthesizer won't be calling stop() in @c MediaPlayer twice.
 * Call preHandle with a valid SPEAK directive. Then call handleDirective. Expected result is that @c acquireChannel
 * is called once. On Focus Changed to foreground, audio should play.
 * Call cancel directive. Expect the stop() to be called once.
 * Call onFocusChanged, expect the stop() to not be called again.
 * Expect when handleDirectiveImmediately with a valid SPEAK directive is called, @c SpeechSynthesizer
 * will react correctly.
 */
TEST_F(SpeechSynthesizerTest, testTimer_notCallStopTwice) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    auto avsMessageHeader2 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST_2);
    std::shared_ptr<AVSDirective> directive2 =
        AVSDirective::create("", avsMessageHeader2, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST_2);

    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnAcquireChannel));
    EXPECT_CALL(
        *(m_mockSpeechPlayer.get()),
        attachmentSetSource(A<std::shared_ptr<avsCommon::avs::attachment::AttachmentReader>>(), nullptr))
        .Times(AtLeast(1));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), play(_)).Times(AtLeast(1));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), getOffset(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(OFFSET_IN_CHRONO_MILLISECONDS_TEST));
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(NAMESPACE_AND_NAME_SPEECH_STATE, PLAYING_STATE_TEST, StateRefreshPolicy::ALWAYS, 0))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(NAMESPACE_AND_NAME_SPEECH_STATE, INTERRUPTED_STATE_TEST, StateRefreshPolicy::NEVER, 0))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(IsStartedEvent()))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(IsInterruptedEvent())).Times(AtLeast(1));
    EXPECT_CALL(*(m_mockFocusManager.get()), releaseChannel(CHANNEL_NAME, _))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnReleaseChannel));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), stop(_))
        .Times(1)
        .WillOnce(Invoke([this](avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId id) {
            wakeOnStopped();
            m_speechSynthesizer->onPlaybackStopped(id, DEFAULT_MEDIA_PLAYER_STATE);
            return true;
        }))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*(m_mockDirHandlerResult.get()), setCompleted()).Times(AtLeast(0));
    EXPECT_CALL(*m_mockPowerResourceManager, acquirePowerResource(COMPONENT_NAME, PowerResourceLevel::ACTIVE_HIGH))
        .Times(AtLeast(1));

    // send Speak directive and getting focus and wait until playback started
    m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirHandlerResult));
    m_speechSynthesizer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_wakeAcquireChannelPromise = std::promise<void>();
    m_wakeAcquireChannelFuture = m_wakeAcquireChannelPromise.get_future();
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();
    ASSERT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(MY_WAIT_TIMEOUT));
    m_wakeSendMessagePromise = std::promise<void>();
    m_wakeSendMessageFuture = m_wakeSendMessagePromise.get_future();

    EXPECT_CALL(*m_mockPowerResourceManager, releasePowerResource(COMPONENT_NAME)).Times(AtLeast(1));
    // cancel directive, this should result in calling stop()
    m_speechSynthesizer->CapabilityAgent::cancelDirective(MESSAGE_ID_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeStoppedFuture.wait_for(MY_WAIT_TIMEOUT));

    // goes to background, this should not result in calling the 2nd stop()
    m_speechSynthesizer->onFocusChanged(FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();

    /*
     * onPlaybackStopped, this will result in an error with reason=nullptrDirectiveInfo.  But this shouldn't break the
     * SpeechSynthesizer
     */
    ASSERT_TRUE(std::future_status::ready == m_wakeReleaseChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_wakeReleaseChannelPromise = std::promise<void>();
    m_wakeReleaseChannelFuture = m_wakeReleaseChannelPromise.get_future();

    EXPECT_CALL(*m_mockPowerResourceManager, acquirePowerResource(COMPONENT_NAME, PowerResourceLevel::ACTIVE_HIGH))
        .Times(AtLeast(1));
    // send second speak directive and make sure it working
    m_speechSynthesizer->handleDirectiveImmediately(directive2);
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
}

/**
 * Testing executeCancel() completes execution before onFocusChanged() is called.
 *
 * The directive that was cancelled should never play. The second speech should play without any problem.
 */
TEST_F(SpeechSynthesizerTest, testSlow_callingCancelBeforeOnFocusChanged) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    auto avsMessageHeader2 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST_2);
    std::shared_ptr<AVSDirective> directive2 =
        AVSDirective::create("", avsMessageHeader2, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST_2);

    EXPECT_CALL(*m_mockFocusManager, acquireChannel(CHANNEL_NAME, _))
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnAcquireChannel));
    m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirHandlerResult));
    m_speechSynthesizer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_wakeAcquireChannelPromise = std::promise<void>();
    m_wakeAcquireChannelFuture = m_wakeAcquireChannelPromise.get_future();

    // Expect speech synthesizer to release the focus since it is no longer needed.
    EXPECT_CALL(*(m_mockFocusManager.get()), releaseChannel(CHANNEL_NAME, _))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnReleaseChannel));
    m_speechSynthesizer->CapabilityAgent::cancelDirective(MESSAGE_ID_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeReleaseChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_wakeReleaseChannelPromise = std::promise<void>();
    m_wakeReleaseChannelFuture = m_wakeReleaseChannelPromise.get_future();

    // FocusManager might still be processing the initial acquire focus.
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    m_speechSynthesizer->onFocusChanged(FocusState::NONE, MixingBehavior::MUST_STOP);

    // Expect the next directive to start playing.
    EXPECT_CALL(*m_mockFocusManager, acquireChannel(CHANNEL_NAME, _))
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnAcquireChannel));
    EXPECT_CALL(
        *m_mockContextManager,
        setState(NAMESPACE_AND_NAME_SPEECH_STATE, PLAYING_STATE_TEST, StateRefreshPolicy::ALWAYS, 0))
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
    EXPECT_CALL(
        *m_mockSpeechPlayer,
        attachmentSetSource(A<std::shared_ptr<avsCommon::avs::attachment::AttachmentReader>>(), nullptr));
    EXPECT_CALL(*m_mockSpeechPlayer, play(_));
    EXPECT_CALL(*m_mockSpeechPlayer, getOffset(_)).WillRepeatedly(Return(OFFSET_IN_CHRONO_MILLISECONDS_TEST));
    EXPECT_CALL(*m_mockPowerResourceManager, acquirePowerResource(COMPONENT_NAME, PowerResourceLevel::ACTIVE_HIGH))
        .Times(AtLeast(1));

    // send second speak directive and make sure it working
    m_speechSynthesizer->handleDirectiveImmediately(directive2);
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
}

/**
 * Testing executeCancel() completes execution before executeStateChange() is called.
 */
TEST_F(SpeechSynthesizerTest, test_callingCancelBeforeOnExecuteStateChanged) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    auto avsMessageHeader2 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST_2);
    std::shared_ptr<AVSDirective> directive2 =
        AVSDirective::create("", avsMessageHeader2, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST_2);

    EXPECT_CALL(*m_mockFocusManager, acquireChannel(CHANNEL_NAME, _))
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnAcquireChannel));

    m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirHandlerResult));
    m_speechSynthesizer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_wakeAcquireChannelPromise = std::promise<void>();
    m_wakeAcquireChannelFuture = m_wakeAcquireChannelPromise.get_future();

    m_speechSynthesizer->CapabilityAgent::cancelDirective(MESSAGE_ID_TEST);
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);

    // Expect the next directive to start playing.
    EXPECT_CALL(*m_mockFocusManager, acquireChannel(CHANNEL_NAME, _))
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnAcquireChannel));
    EXPECT_CALL(
        *m_mockContextManager,
        setState(NAMESPACE_AND_NAME_SPEECH_STATE, PLAYING_STATE_TEST, StateRefreshPolicy::ALWAYS, 0))
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
    EXPECT_CALL(
        *m_mockSpeechPlayer,
        attachmentSetSource(A<std::shared_ptr<avsCommon::avs::attachment::AttachmentReader>>(), nullptr));
    EXPECT_CALL(*m_mockSpeechPlayer, play(_));
    EXPECT_CALL(*m_mockSpeechPlayer, getOffset(_)).WillRepeatedly(Return(OFFSET_IN_CHRONO_MILLISECONDS_TEST));
    EXPECT_CALL(*m_mockPowerResourceManager, acquirePowerResource(COMPONENT_NAME, PowerResourceLevel::ACTIVE_HIGH))
        .Times(AtLeast(1));

    // send second speak directive and make sure it working
    m_speechSynthesizer->handleDirectiveImmediately(directive2);
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
}

/**
 * Testing SpeechSynthesizer will continue to function properly if stop() in @c MediaPlayer returned with an error.
 * Call preHandle with a valid SPEAK directive. Then call handleDirective. Expected result is that @c acquireChannel
 * is called once. On Focus Changed to foreground, audio should play.
 * Call cancel directive. Expect the stop() to be called once, and we force MediaPlayer to return an error.
 * Expect when handleDirectiveImmediately with a valid SPEAK directive is called, @c SpeechSynthesizer
 * will react correctly.
 */
TEST_F(SpeechSynthesizerTest, test_mediaPlayerFailedToStop) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    auto avsMessageHeader2 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST_2);
    std::shared_ptr<AVSDirective> directive2 =
        AVSDirective::create("", avsMessageHeader2, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST_2);

    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnAcquireChannel));
    EXPECT_CALL(
        *(m_mockSpeechPlayer.get()),
        attachmentSetSource(A<std::shared_ptr<avsCommon::avs::attachment::AttachmentReader>>(), nullptr))
        .Times(AtLeast(1));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), play(_)).Times(AtLeast(1));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), getOffset(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(OFFSET_IN_CHRONO_MILLISECONDS_TEST));
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(NAMESPACE_AND_NAME_SPEECH_STATE, PLAYING_STATE_TEST, StateRefreshPolicy::ALWAYS, 0))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(NAMESPACE_AND_NAME_SPEECH_STATE, INTERRUPTED_STATE_TEST, StateRefreshPolicy::NEVER, 0))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(IsStartedEvent()))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
    EXPECT_CALL(*(m_mockFocusManager.get()), releaseChannel(CHANNEL_NAME, _))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnReleaseChannel));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), stop(_))
        .WillOnce(Invoke([this](avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId id) {
            wakeOnStopped();
            return false;
        }));
    EXPECT_CALL(*(m_mockDirHandlerResult.get()), setFailed(_)).Times(AtLeast(0));
    EXPECT_CALL(*m_mockPowerResourceManager, acquirePowerResource(COMPONENT_NAME, PowerResourceLevel::ACTIVE_HIGH))
        .Times(AtLeast(1));

    // send Speak directive and getting focus and wait until playback started
    m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirHandlerResult));
    m_speechSynthesizer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_wakeAcquireChannelPromise = std::promise<void>();
    m_wakeAcquireChannelFuture = m_wakeAcquireChannelPromise.get_future();
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();
    ASSERT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(MY_WAIT_TIMEOUT));
    m_wakeSendMessagePromise = std::promise<void>();
    m_wakeSendMessageFuture = m_wakeSendMessagePromise.get_future();

    EXPECT_CALL(*m_mockPowerResourceManager, releasePowerResource(COMPONENT_NAME)).Times(AtLeast(1));
    // cancel directive, this should result in calling stop()
    m_speechSynthesizer->CapabilityAgent::cancelDirective(MESSAGE_ID_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeStoppedFuture.wait_for(MY_WAIT_TIMEOUT));

    // goes to background, this should not result in calling the 2nd stop()
    ASSERT_TRUE(std::future_status::ready == m_wakeReleaseChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_speechSynthesizer->onFocusChanged(FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();

    /*
     * onPlaybackStopped, this will result in an error with reason=nullptrDirectiveInfo.  But this shouldn't break the
     * SpeechSynthesizer
     */
    ASSERT_TRUE(std::future_status::ready == m_wakeReleaseChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_wakeReleaseChannelPromise = std::promise<void>();
    m_wakeReleaseChannelFuture = m_wakeReleaseChannelPromise.get_future();

    EXPECT_CALL(*m_mockPowerResourceManager, acquirePowerResource(COMPONENT_NAME, PowerResourceLevel::ACTIVE_HIGH))
        .Times(AtLeast(1));
    // send second speak directive and make sure it working
    m_speechSynthesizer->handleDirectiveImmediately(directive2);
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
}

/**
 * Test SpeechSynthesizer shutdown when speech is playing and @c MediaPlayerInterface.stop() fails (ACSDK-1859).
 *
 * Expected result is that shutdown should succeeded no matter the @c stop return.
 */
TEST_F(SpeechSynthesizerTest, testTimer_mediaPlayerAlwaysFailToStop) {
    auto speechSynthesizer = SpeechSynthesizer::create(
        m_mockSpeechPlayer,
        m_mockMessageSender,
        m_mockFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        m_metricRecorder,
        m_dialogUXStateAggregator,
        m_mockCaptionManager);

    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    EXPECT_CALL(*m_mockFocusManager, acquireChannel(_, _))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnAcquireChannel));
    EXPECT_CALL(*m_mockSpeechPlayer, attachmentSetSource(_, _)).Times(AtLeast(1));
    EXPECT_CALL(*m_mockSpeechPlayer, play(_)).Times(AtLeast(1));
    EXPECT_CALL(*m_mockSpeechPlayer, getOffset(_)).WillRepeatedly(Return(OFFSET_IN_CHRONO_MILLISECONDS_TEST));
    EXPECT_CALL(*m_mockContextManager, setState(_, _, _, _)).Times(AtLeast(1));
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_));
    EXPECT_CALL(*m_mockFocusManager, releaseChannel(_, _)).Times(AtLeast(1));
    EXPECT_CALL(*m_mockSpeechPlayer, stop(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockDirHandlerResult, setFailed(_));

    // send Speak directive and getting focus and wait until playback started
    speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirHandlerResult));
    speechSynthesizer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    EXPECT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    speechSynthesizer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());

    speechSynthesizer->shutdown();
    speechSynthesizer.reset();
}

/**
 * Testing SpeechSynthesizer will call stop() if the SpeechSynthesizer experienced a state change timeout to PLAYING
 * state.
 */
TEST_F(SpeechSynthesizerTest, testSlow_setStateTimeout) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnAcquireChannel));
    EXPECT_CALL(
        *(m_mockSpeechPlayer.get()),
        attachmentSetSource(A<std::shared_ptr<avsCommon::avs::attachment::AttachmentReader>>(), nullptr))
        .Times(AtLeast(1));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), play(_)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), getOffset(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(OFFSET_IN_CHRONO_MILLISECONDS_TEST));
    EXPECT_CALL(*(m_mockExceptionSender.get()), sendExceptionEncountered(_, _, _)).Times(1);
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(NAMESPACE_AND_NAME_SPEECH_STATE, PLAYING_STATE_TEST, StateRefreshPolicy::ALWAYS, 0))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(NAMESPACE_AND_NAME_SPEECH_STATE, FINISHED_STATE_TEST, StateRefreshPolicy::NEVER, 0))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_)).Times(0);
    EXPECT_CALL(*(m_mockFocusManager.get()), releaseChannel(CHANNEL_NAME, _))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnReleaseChannel));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), stop(_)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*(m_mockDirHandlerResult.get()), setFailed(_))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetFailed));
    EXPECT_CALL(*m_mockPowerResourceManager, acquirePowerResource(COMPONENT_NAME, PowerResourceLevel::ACTIVE_HIGH))
        .Times(AtLeast(1));
    EXPECT_CALL(*m_mockPowerResourceManager, releasePowerResource(COMPONENT_NAME)).Times(AtLeast(1));

    // Send Speak directive and getting focus and wait until state change timeout.
    m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirHandlerResult));
    m_speechSynthesizer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_wakeAcquireChannelPromise = std::promise<void>();
    m_wakeAcquireChannelFuture = m_wakeAcquireChannelPromise.get_future();
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetFailedFuture.wait_for(STATE_CHANGE_TIMEOUT));

    // Upon getting onPlaybackedStarted, expect state to be updated, but SpeechStarted event will not be sent.
    m_speechSynthesizer->onPlaybackStarted(m_mockSpeechPlayer->getCurrentSourceId(), DEFAULT_MEDIA_PLAYER_STATE);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();

    // Upon getting onPlaybackStopped, expect state to be updated, but SpeechFinished event will not be sent.
    m_speechSynthesizer->onPlaybackStopped(m_mockSpeechPlayer->getCurrentSourceId(), DEFAULT_MEDIA_PLAYER_STATE);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();

    ASSERT_TRUE(std::future_status::ready == m_wakeReleaseChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_speechSynthesizer->onFocusChanged(FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE);
}

/**
 * Testing changing focus state to NONE (local stop) during a speak.
 * Expect @c setFailed to be called so any subsequent directives with the same dialogRequestId will be dropped.
 */
TEST_F(SpeechSynthesizerTest, test_givenPlayingStateFocusBecomesNone) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnAcquireChannel));
    EXPECT_CALL(
        *(m_mockSpeechPlayer.get()),
        attachmentSetSource(A<std::shared_ptr<avsCommon::avs::attachment::AttachmentReader>>(), nullptr))
        .Times(AtLeast(1));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), play(_)).Times(AtLeast(1));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), getOffset(_))
        .Times(AtLeast(2))
        .WillRepeatedly(Return(OFFSET_IN_CHRONO_MILLISECONDS_TEST));
    EXPECT_CALL(*(m_mockDirHandlerResult.get()), setFailed(_))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetFailed));
    EXPECT_CALL(*(m_mockDirHandlerResult.get()), setCompleted()).Times(0);
    EXPECT_CALL(*m_mockPowerResourceManager, acquirePowerResource(COMPONENT_NAME, PowerResourceLevel::ACTIVE_HIGH))
        .Times(AtLeast(1));

    m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirHandlerResult));
    m_speechSynthesizer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    EXPECT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    EXPECT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
    EXPECT_CALL(*m_mockPowerResourceManager, releasePowerResource(COMPONENT_NAME)).Times(AtLeast(1));
    m_speechSynthesizer->onFocusChanged(FocusState::NONE, MixingBehavior::MUST_STOP);
    EXPECT_TRUE(std::future_status::ready == m_wakeSetFailedFuture.wait_for(STATE_CHANGE_TIMEOUT));
}

/**
 * Testing SpeechSynthesizer will call setFailed() if the SpeechSynthesizer got a onPlaybackStopped() callback while
 * it is in PLAYING state.
 */
TEST_F(SpeechSynthesizerTest, testTimer_onPlayedStopped) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnAcquireChannel));
    EXPECT_CALL(
        *(m_mockSpeechPlayer.get()),
        attachmentSetSource(A<std::shared_ptr<avsCommon::avs::attachment::AttachmentReader>>(), nullptr))
        .Times(AtLeast(1));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), play(_)).Times(AtLeast(1));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), getOffset(_)).WillRepeatedly(Return(OFFSET_IN_CHRONO_MILLISECONDS_TEST));
    EXPECT_CALL(*(m_mockDirHandlerResult.get()), setFailed(_))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetFailed));
    EXPECT_CALL(*(m_mockDirHandlerResult.get()), setCompleted()).Times(0);
    EXPECT_CALL(*m_mockPowerResourceManager, acquirePowerResource(COMPONENT_NAME, PowerResourceLevel::ACTIVE_HIGH))
        .Times(AtLeast(1));

    m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirHandlerResult));
    m_speechSynthesizer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    EXPECT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    EXPECT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
    EXPECT_CALL(*m_mockPowerResourceManager, releasePowerResource(COMPONENT_NAME)).Times(AtLeast(1));
    m_speechSynthesizer->onPlaybackStopped(m_mockSpeechPlayer->getCurrentSourceId(), DEFAULT_MEDIA_PLAYER_STATE);
    EXPECT_TRUE(std::future_status::ready == m_wakeSetFailedFuture.wait_for(STATE_CHANGE_TIMEOUT));
}

bool SpeechSynthesizerTest::setupActiveSpeech(
    std::unique_ptr<DirectiveHandlerResultInterface> resultHandler,
    const SpeakTestInfo& info) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, info.messageId, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, info.payload, m_attachmentManager, CONTEXT_ID_TEST);

    EXPECT_CALL(*m_mockFocusManager, acquireChannel(CHANNEL_NAME, _))
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnAcquireChannel));
    EXPECT_CALL(
        *m_mockSpeechPlayer,
        attachmentSetSource(A<std::shared_ptr<avsCommon::avs::attachment::AttachmentReader>>(), nullptr));
    EXPECT_CALL(*m_mockSpeechPlayer, play(_)).Times(AtLeast(1));
    EXPECT_CALL(*m_mockSpeechPlayer, getOffset(_)).WillRepeatedly(Return(OFFSET_IN_CHRONO_MILLISECONDS_TEST));
    EXPECT_CALL(
        *m_mockContextManager,
        setState(NAMESPACE_AND_NAME_SPEECH_STATE, generatePlayingState(info), StateRefreshPolicy::ALWAYS, 0))
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
    EXPECT_CALL(*m_mockMessageSender, sendMessage(IsStartedEvent()))
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
    EXPECT_CALL(*m_mockPowerResourceManager, acquirePowerResource(COMPONENT_NAME, PowerResourceLevel::ACTIVE_HIGH))
        .Times(AtLeast(1));
    EXPECT_CALL(*m_mockPowerResourceManager, releasePowerResource(COMPONENT_NAME)).Times(AtLeast(1));

    m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(resultHandler));
    m_speechSynthesizer->CapabilityAgent::handleDirective(info.messageId);
    EXPECT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    EXPECT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
    EXPECT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    EXPECT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(MY_WAIT_TIMEOUT));

    // Reset promises.
    m_wakeSendMessagePromise = std::promise<void>();
    m_wakeSendMessageFuture = m_wakeSendMessagePromise.get_future();
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();
    m_wakeAcquireChannelPromise = std::promise<void>();
    m_wakeAcquireChannelFuture = m_wakeAcquireChannelPromise.get_future();

    return !Test::HasFailure();
}

bool SpeechSynthesizerTest::setupPendingSpeech(
    std::unique_ptr<DirectiveHandlerResultInterface> resultHandler,
    const SpeakTestInfo& info) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, info.messageId, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, info.payload, m_attachmentManager, CONTEXT_ID_TEST);
    EXPECT_CALL(*m_mockFocusManager, acquireChannel(CHANNEL_NAME, _))
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnAcquireChannel));
    EXPECT_CALL(*m_mockPowerResourceManager, acquirePowerResource(COMPONENT_NAME, PowerResourceLevel::ACTIVE_HIGH))
        .Times(AtLeast(1));
    EXPECT_CALL(*m_mockPowerResourceManager, releasePowerResource(COMPONENT_NAME)).Times(AtLeast(1));

    m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(resultHandler));
    m_speechSynthesizer->CapabilityAgent::handleDirective(info.messageId);

    EXPECT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_wakeAcquireChannelPromise = std::promise<void>();
    m_wakeAcquireChannelFuture = m_wakeAcquireChannelPromise.get_future();
    return !Test::HasFailure();
}

/**
 * Test SpeechSynthesizer REPLACE_ALL when there is no active directive.
 *
 * Expect the speech synthesizer to play the new speech and go to idle after.
 */
TEST_F(SpeechSynthesizerTest, test_replaceAllWithEmptyQueue) {
    auto mockActiveResultHandler = std::unique_ptr<MockDirectiveHandlerResult>(new MockDirectiveHandlerResult());
    auto info = generateSpeakInfo(PlayBehavior::REPLACE_ALL);
    EXPECT_CALL(*mockActiveResultHandler, setCompleted())
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetCompleted));
    EXPECT_TRUE(setupActiveSpeech(std::move(mockActiveResultHandler), info));

    EXPECT_CALL(
        *m_mockContextManager,
        setState(NAMESPACE_AND_NAME_SPEECH_STATE, generateFinishedState(info), StateRefreshPolicy::NEVER, 0))
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
    EXPECT_CALL(*m_mockMessageSender, sendMessage(IsFinishedEvent()))
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));

    m_mockSpeechPlayer->mockFinished(m_mockSpeechPlayer->getCurrentSourceId());
    EXPECT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(MY_WAIT_TIMEOUT));
    EXPECT_TRUE(std::future_status::ready == m_wakeSetCompletedFuture.wait_for(MY_WAIT_TIMEOUT));
    EXPECT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test SpeechSynthesizer REPLACE_ALL when the queue has one speak directive that hasn't started yet.
 *
 * Expect the speech synthesizer to cancel the enqueued directive and play the new speech.
 */
TEST_F(SpeechSynthesizerTest, test_replaceAllWithNonEmptyQueue) {
    {
        SCOPED_TRACE("Setup Queue");
        auto mockEnqueuedResultHandler = std::unique_ptr<MockDirectiveHandlerResult>(new MockDirectiveHandlerResult());
        auto pending = generateSpeakInfo(PlayBehavior::ENQUEUE);
        ASSERT_TRUE(setupPendingSpeech(std::move(mockEnqueuedResultHandler), pending));
    }

    auto mockResultHandler = std::unique_ptr<MockDirectiveHandlerResult>(new MockDirectiveHandlerResult());
    EXPECT_CALL(*mockResultHandler, setCompleted())
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetCompleted));
    auto speak = generateSpeakInfo(PlayBehavior::REPLACE_ALL);
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, speak.messageId, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, speak.payload, m_attachmentManager, CONTEXT_ID_TEST);

    {
        SCOPED_TRACE("Setup Expectations");
        EXPECT_CALL(*m_mockFocusManager, acquireChannel(CHANNEL_NAME, _))
            .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnAcquireChannel));
        EXPECT_CALL(
            *m_mockSpeechPlayer,
            attachmentSetSource(A<std::shared_ptr<avsCommon::avs::attachment::AttachmentReader>>(), nullptr));
        EXPECT_CALL(*m_mockSpeechPlayer, play(_)).Times(AtLeast(1));
        EXPECT_CALL(*m_mockSpeechPlayer, getOffset(_)).WillRepeatedly(Return(OFFSET_IN_CHRONO_MILLISECONDS_TEST));
        EXPECT_CALL(
            *m_mockContextManager,
            setState(NAMESPACE_AND_NAME_SPEECH_STATE, generatePlayingState(speak), StateRefreshPolicy::ALWAYS, 0))
            .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
        EXPECT_CALL(*m_mockMessageSender, sendMessage(IsStartedEvent()))
            .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
    }

    {
        SCOPED_TRACE("Test Directive Handling");
        m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(mockResultHandler));
        m_speechSynthesizer->CapabilityAgent::handleDirective(speak.messageId);
        EXPECT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    }

    {
        SCOPED_TRACE("Check Speech Playback");
        m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
        EXPECT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
        EXPECT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
        EXPECT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(MY_WAIT_TIMEOUT));
        m_wakeSendMessagePromise = std::promise<void>();
        m_wakeSendMessageFuture = m_wakeSendMessagePromise.get_future();
        m_wakeSetStatePromise = std::promise<void>();
        m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();
    }

    {
        SCOPED_TRACE("Check Speech Playback");
        EXPECT_CALL(
            *m_mockContextManager,
            setState(NAMESPACE_AND_NAME_SPEECH_STATE, generateFinishedState(speak), StateRefreshPolicy::NEVER, 0))
            .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
        EXPECT_CALL(*m_mockMessageSender, sendMessage(IsFinishedEvent()))
            .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
        m_mockSpeechPlayer->mockFinished(m_mockSpeechPlayer->getCurrentSourceId());

        EXPECT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(MY_WAIT_TIMEOUT));
        EXPECT_TRUE(std::future_status::ready == m_wakeSetCompletedFuture.wait_for(MY_WAIT_TIMEOUT));
        EXPECT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    }
}

/**
 * Test SpeechSynthesizer REPLACE_ALL when there is an ongoing speech.
 *
 * Expect the speech synthesizer to cancel the active speech, send an interrupted event and play the new speech.
 */
TEST_F(SpeechSynthesizerTest, test_replaceAllStopActiveSpeech) {
    auto active = generateSpeakInfo(PlayBehavior::ENQUEUE);
    {
        SCOPED_TRACE("Setup Queue");
        auto mockEnqueuedResultHandler = std::unique_ptr<MockDirectiveHandlerResult>(new MockDirectiveHandlerResult());
        EXPECT_CALL(*mockEnqueuedResultHandler, setFailed(_));
        ASSERT_TRUE(setupActiveSpeech(std::move(mockEnqueuedResultHandler), active));
    }

    auto mockResultHandler = std::unique_ptr<MockDirectiveHandlerResult>(new MockDirectiveHandlerResult());
    EXPECT_CALL(*mockResultHandler, setCompleted())
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetCompleted));
    auto speak = generateSpeakInfo(PlayBehavior::REPLACE_ALL);
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, speak.messageId, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, speak.payload, m_attachmentManager, CONTEXT_ID_TEST);

    {
        SCOPED_TRACE("Setup Expectations");
        // Interrupted event.
        EXPECT_CALL(*m_mockSpeechPlayer, stop(_));
        EXPECT_CALL(
            *m_mockContextManager,
            setState(NAMESPACE_AND_NAME_SPEECH_STATE, generateInterruptedState(active), StateRefreshPolicy::NEVER, 0));
        EXPECT_CALL(*m_mockMessageSender, sendMessage(IsInterruptedEvent()));

        // New directive handling.
        EXPECT_CALL(*m_mockFocusManager, acquireChannel(CHANNEL_NAME, _))
            .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnAcquireChannel));
        EXPECT_CALL(
            *m_mockSpeechPlayer,
            attachmentSetSource(A<std::shared_ptr<avsCommon::avs::attachment::AttachmentReader>>(), nullptr));
        EXPECT_CALL(*m_mockSpeechPlayer, play(_)).Times(AtLeast(1));
        EXPECT_CALL(*m_mockSpeechPlayer, getOffset(_))
            .Times(AtLeast(2))
            .WillRepeatedly(Return(OFFSET_IN_CHRONO_MILLISECONDS_TEST));
        EXPECT_CALL(
            *m_mockContextManager,
            setState(NAMESPACE_AND_NAME_SPEECH_STATE, generatePlayingState(speak), StateRefreshPolicy::ALWAYS, 0))
            .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
        EXPECT_CALL(*m_mockMessageSender, sendMessage(IsStartedEvent()))
            .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
        EXPECT_CALL(*m_mockPowerResourceManager, acquirePowerResource(COMPONENT_NAME, PowerResourceLevel::ACTIVE_HIGH))
            .Times(AtLeast(1));
    }

    {
        SCOPED_TRACE("Test Directive Handling");
        m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(mockResultHandler));
        m_speechSynthesizer->CapabilityAgent::handleDirective(speak.messageId);
        EXPECT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    }

    {
        SCOPED_TRACE("Check Speech Playback");
        m_speechSynthesizer->onFocusChanged(FocusState::NONE, MixingBehavior::MUST_STOP);
        m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
        EXPECT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
        EXPECT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
        EXPECT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(MY_WAIT_TIMEOUT));
        m_wakeSendMessagePromise = std::promise<void>();
        m_wakeSendMessageFuture = m_wakeSendMessagePromise.get_future();
        m_wakeSetStatePromise = std::promise<void>();
        m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();
    }

    {
        SCOPED_TRACE("Check Speech Playback");
        EXPECT_CALL(
            *m_mockContextManager,
            setState(NAMESPACE_AND_NAME_SPEECH_STATE, generateFinishedState(speak), StateRefreshPolicy::NEVER, 0))
            .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
        EXPECT_CALL(*m_mockMessageSender, sendMessage(IsFinishedEvent()))
            .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
        EXPECT_CALL(*m_mockPowerResourceManager, releasePowerResource(COMPONENT_NAME)).Times(AtLeast(1));
        m_mockSpeechPlayer->mockFinished(m_mockSpeechPlayer->getCurrentSourceId());

        EXPECT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(MY_WAIT_TIMEOUT));
        EXPECT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    }
}

/**
 * Test SpeechSynthesizer ENQUEUE play behavior when there is already an active directive.
 *
 * Expect the speech synthesizer to finish playing the first directive and play the enqueued directive right after.
 */
TEST_F(SpeechSynthesizerTest, test_enqueueWithActiveSpeech) {
    auto firstDirective = generateSpeakInfo(PlayBehavior::ENQUEUE);
    {
        SCOPED_TRACE("Setup First");
        auto mockEnqueuedResultHandler = std::unique_ptr<MockDirectiveHandlerResult>(new MockDirectiveHandlerResult());
        EXPECT_CALL(*mockEnqueuedResultHandler, setCompleted());
        ASSERT_TRUE(setupActiveSpeech(std::move(mockEnqueuedResultHandler), firstDirective));
    }

    auto mockResultHandler = std::unique_ptr<MockDirectiveHandlerResult>(new MockDirectiveHandlerResult());
    EXPECT_CALL(*mockResultHandler, setCompleted());
    auto secondDirective = generateSpeakInfo(PlayBehavior::ENQUEUE);
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, secondDirective.messageId, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, secondDirective.payload, m_attachmentManager, CONTEXT_ID_TEST);
    {
        SCOPED_TRACE("Add Second");
        m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(mockResultHandler));
    }

    {
        SCOPED_TRACE("Finish First");
        EXPECT_CALL(
            *m_mockContextManager,
            setState(
                NAMESPACE_AND_NAME_SPEECH_STATE, generateFinishedState(firstDirective), StateRefreshPolicy::NEVER, 0));
        EXPECT_CALL(*m_mockMessageSender, sendMessage(IsFinishedEvent()))
            .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
        m_mockSpeechPlayer->mockFinished(m_mockSpeechPlayer->getCurrentSourceId());
        EXPECT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(MY_WAIT_TIMEOUT));
    }

    // Reset promises.
    m_wakeSendMessagePromise = std::promise<void>();
    m_wakeSendMessageFuture = m_wakeSendMessagePromise.get_future();

    {
        // Start new speech speech.
        SCOPED_TRACE("Start Second");
        EXPECT_CALL(*m_mockFocusManager, acquireChannel(CHANNEL_NAME, _))
            .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnAcquireChannel));
        m_speechSynthesizer->CapabilityAgent::handleDirective(secondDirective.messageId);
        EXPECT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));

        EXPECT_CALL(
            *m_mockSpeechPlayer,
            attachmentSetSource(A<std::shared_ptr<avsCommon::avs::attachment::AttachmentReader>>(), nullptr));
        EXPECT_CALL(*m_mockSpeechPlayer, play(_));
        EXPECT_CALL(*m_mockSpeechPlayer, getOffset(_)).WillRepeatedly(Return(OFFSET_IN_CHRONO_MILLISECONDS_TEST));
        EXPECT_CALL(
            *m_mockContextManager,
            setState(
                NAMESPACE_AND_NAME_SPEECH_STATE, generatePlayingState(secondDirective), StateRefreshPolicy::ALWAYS, 0))
            .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
        EXPECT_CALL(*m_mockMessageSender, sendMessage(IsStartedEvent()))
            .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
        EXPECT_CALL(*m_mockPowerResourceManager, acquirePowerResource(COMPONENT_NAME, PowerResourceLevel::ACTIVE_HIGH))
            .Times(AtLeast(1));

        m_speechSynthesizer->onFocusChanged(FocusState::NONE, MixingBehavior::MUST_STOP);
        m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
        EXPECT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(MY_WAIT_TIMEOUT));
        EXPECT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    }

    // Reset promises.
    m_wakeAcquireChannelPromise = std::promise<void>();
    m_wakeAcquireChannelFuture = m_wakeAcquireChannelPromise.get_future();
    m_wakeSendMessagePromise = std::promise<void>();
    m_wakeSendMessageFuture = m_wakeSendMessagePromise.get_future();
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();

    {
        SCOPED_TRACE("Finish Second");
        EXPECT_CALL(
            *m_mockContextManager,
            setState(
                NAMESPACE_AND_NAME_SPEECH_STATE, generateFinishedState(secondDirective), StateRefreshPolicy::NEVER, 0))
            .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
        EXPECT_CALL(*m_mockMessageSender, sendMessage(IsFinishedEvent()))
            .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
        EXPECT_CALL(*m_mockPowerResourceManager, releasePowerResource(COMPONENT_NAME)).Times(AtLeast(1));
        m_mockSpeechPlayer->mockFinished(m_mockSpeechPlayer->getCurrentSourceId());
        EXPECT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(MY_WAIT_TIMEOUT));
        EXPECT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    }
}

/**
 * Test SpeechSynthesizer REPLACE_ENQUEUED play behavior when there is one directive playing and one in the queue.
 *
 * Expect the speech synthesizer to finish playing the first directive, skip the second and play the third directive.
 */
TEST_F(SpeechSynthesizerTest, test_replaceEnqueuedWithAnotherEnqueuedItem) {
    auto firstDirective = generateSpeakInfo(PlayBehavior::ENQUEUE);
    {
        SCOPED_TRACE("Setup First");
        auto mockEnqueuedResultHandler = std::unique_ptr<MockDirectiveHandlerResult>(new MockDirectiveHandlerResult());
        EXPECT_CALL(*mockEnqueuedResultHandler, setCompleted());
        ASSERT_TRUE(setupActiveSpeech(std::move(mockEnqueuedResultHandler), firstDirective));
    }

    {
        SCOPED_TRACE("Add Second");
        auto secondDirective = generateSpeakInfo(PlayBehavior::ENQUEUE);
        auto secondMessageHeader = std::make_shared<AVSMessageHeader>(
            NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, secondDirective.messageId, DIALOG_REQUEST_ID_TEST);
        std::shared_ptr<AVSDirective> directive = AVSDirective::create(
            "", secondMessageHeader, secondDirective.payload, m_attachmentManager, CONTEXT_ID_TEST);
        m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, nullptr);
    }

    auto mockResultHandler = std::unique_ptr<MockDirectiveHandlerResult>(new MockDirectiveHandlerResult());
    EXPECT_CALL(*mockResultHandler, setCompleted());
    auto thirdDirective = generateSpeakInfo(PlayBehavior::REPLACE_ENQUEUED);
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, thirdDirective.messageId, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, thirdDirective.payload, m_attachmentManager, CONTEXT_ID_TEST);
    {
        SCOPED_TRACE("Add Third");
        m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(mockResultHandler));
        m_speechSynthesizer->CapabilityAgent::handleDirective(thirdDirective.messageId);
    }

    {
        SCOPED_TRACE("Finish First");
        EXPECT_CALL(
            *m_mockContextManager,
            setState(
                NAMESPACE_AND_NAME_SPEECH_STATE, generateFinishedState(firstDirective), StateRefreshPolicy::NEVER, 0));
        EXPECT_CALL(*m_mockMessageSender, sendMessage(IsFinishedEvent()));
        // New speech.
        EXPECT_CALL(*m_mockFocusManager, acquireChannel(CHANNEL_NAME, _))
            .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnAcquireChannel));
        m_mockSpeechPlayer->mockFinished(m_mockSpeechPlayer->getCurrentSourceId());

        EXPECT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
        m_wakeAcquireChannelPromise = std::promise<void>();
        m_wakeAcquireChannelFuture = m_wakeAcquireChannelPromise.get_future();
    }

    {
        // Start new speech speech.
        SCOPED_TRACE("Start Second");
        EXPECT_CALL(
            *m_mockSpeechPlayer,
            attachmentSetSource(A<std::shared_ptr<avsCommon::avs::attachment::AttachmentReader>>(), nullptr));
        EXPECT_CALL(*m_mockSpeechPlayer, play(_));
        EXPECT_CALL(*m_mockSpeechPlayer, getOffset(_)).WillRepeatedly(Return(OFFSET_IN_CHRONO_MILLISECONDS_TEST));
        EXPECT_CALL(
            *m_mockContextManager,
            setState(
                NAMESPACE_AND_NAME_SPEECH_STATE, generatePlayingState(thirdDirective), StateRefreshPolicy::ALWAYS, 0))
            .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
        EXPECT_CALL(*m_mockMessageSender, sendMessage(IsStartedEvent()))
            .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
        EXPECT_CALL(*m_mockPowerResourceManager, acquirePowerResource(COMPONENT_NAME, PowerResourceLevel::ACTIVE_HIGH))
            .Times(AtLeast(1));

        m_speechSynthesizer->onFocusChanged(FocusState::NONE, MixingBehavior::MUST_STOP);
        m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
        EXPECT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(MY_WAIT_TIMEOUT));
        EXPECT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    }

    // Reset promises.
    m_wakeSendMessagePromise = std::promise<void>();
    m_wakeSendMessageFuture = m_wakeSendMessagePromise.get_future();
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();

    {
        SCOPED_TRACE("Finish Second");
        EXPECT_CALL(
            *m_mockContextManager,
            setState(
                NAMESPACE_AND_NAME_SPEECH_STATE, generateFinishedState(thirdDirective), StateRefreshPolicy::NEVER, 0))
            .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
        EXPECT_CALL(*m_mockMessageSender, sendMessage(IsFinishedEvent()))
            .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
        EXPECT_CALL(*m_mockPowerResourceManager, releasePowerResource(COMPONENT_NAME)).Times(AtLeast(1));
        m_mockSpeechPlayer->mockFinished(m_mockSpeechPlayer->getCurrentSourceId());
        EXPECT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(MY_WAIT_TIMEOUT));
        EXPECT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    }
}

/**
 * Test call to test audio analyzer config parsing logic.
 */
TEST_F(SpeechSynthesizerTest, test_parsingSingleAnalyzerConfig) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST_SINGLE_ANALYZER, m_attachmentManager, CONTEXT_ID_TEST);

    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnAcquireChannel));
    EXPECT_CALL(
        *(m_mockSpeechPlayer.get()),
        attachmentSetSource(A<std::shared_ptr<avsCommon::avs::attachment::AttachmentReader>>(), nullptr))
        .Times(AtLeast(1));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), play(_)).Times(AtLeast(1));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), getOffset(_))
        .Times(1)
        .WillOnce(Return(OFFSET_IN_CHRONO_MILLISECONDS_TEST));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), getMediaPlayerState(_)).Times(AtLeast(2));
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(NAMESPACE_AND_NAME_SPEECH_STATE, PLAYING_STATE_TEST, StateRefreshPolicy::ALWAYS, 0))
        .Times(AtLeast(1))
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
    EXPECT_CALL(*m_mockCaptionManager, onCaption(_, _)).Times(1);
    EXPECT_CALL(*m_mockPowerResourceManager, acquirePowerResource(COMPONENT_NAME, PowerResourceLevel::ACTIVE_HIGH))
        .Times(AtLeast(1));
    std::vector<audioAnalyzer::AudioAnalyzerState> data;
    data.push_back(audioAnalyzer::AudioAnalyzerState("analyzername", "YES"));
    EXPECT_CALL(
        *m_mockSpeechSynthesizerObserver,
        onStateChanged(SpeechSynthesizerObserverInterface::SpeechSynthesizerState::GAINING_FOCUS, _, _, _))
        .Times(1);
    EXPECT_CALL(
        *m_mockSpeechSynthesizerObserver,
        onStateChanged(SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING, _, _, Eq(data)))
        .Times(1);

    m_speechSynthesizer->addObserver(m_mockSpeechSynthesizerObserver);
    m_speechSynthesizer->handleDirectiveImmediately(directive);
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    ASSERT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(MY_WAIT_TIMEOUT));
}

TEST_F(SpeechSynthesizerTest, test_parsingMultipleAnalyzerConfig) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        "", avsMessageHeader, PAYLOAD_TEST_MULTIPLE_ANALYZER, m_attachmentManager, CONTEXT_ID_TEST);

    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnAcquireChannel));
    EXPECT_CALL(
        *(m_mockSpeechPlayer.get()),
        attachmentSetSource(A<std::shared_ptr<avsCommon::avs::attachment::AttachmentReader>>(), nullptr))
        .Times(AtLeast(1));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), play(_)).Times(AtLeast(1));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), getOffset(_))
        .Times(1)
        .WillOnce(Return(OFFSET_IN_CHRONO_MILLISECONDS_TEST));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), getMediaPlayerState(_)).Times(AtLeast(2));
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(NAMESPACE_AND_NAME_SPEECH_STATE, PLAYING_STATE_TEST, StateRefreshPolicy::ALWAYS, 0))
        .Times(AtLeast(1))
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
    EXPECT_CALL(*m_mockCaptionManager, onCaption(_, _)).Times(1);
    EXPECT_CALL(*m_mockPowerResourceManager, acquirePowerResource(COMPONENT_NAME, PowerResourceLevel::ACTIVE_HIGH))
        .Times(AtLeast(1));
    std::vector<audioAnalyzer::AudioAnalyzerState> data;
    data.push_back(audioAnalyzer::AudioAnalyzerState("analyzername1", "YES"));
    data.push_back(audioAnalyzer::AudioAnalyzerState("analyzername2", "NO"));
    EXPECT_CALL(
        *m_mockSpeechSynthesizerObserver,
        onStateChanged(SpeechSynthesizerObserverInterface::SpeechSynthesizerState::GAINING_FOCUS, _, _, _))
        .Times(1);
    EXPECT_CALL(
        *m_mockSpeechSynthesizerObserver,
        onStateChanged(SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING, _, _, Eq(data)))
        .Times(1);

    m_speechSynthesizer->addObserver(m_mockSpeechSynthesizerObserver);
    m_speechSynthesizer->handleDirectiveImmediately(directive);
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(MY_WAIT_TIMEOUT));
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    ASSERT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(MY_WAIT_TIMEOUT));
}

}  // namespace test
}  // namespace speechSynthesizer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
