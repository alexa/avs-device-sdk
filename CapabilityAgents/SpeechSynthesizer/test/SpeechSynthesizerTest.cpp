/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/Utils/MediaPlayer/MockMediaPlayer.h>

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
using namespace ::testing;

/// Plenty of time for a test to complete.
static const std::chrono::milliseconds WAIT_TIMEOUT(1000);

/// Time to wait for state change timeout.  This should be set to be longer than STATE_CHANGE_TIMEOUT in
/// SpeechSynthesizer.
static const std::chrono::milliseconds STATE_CHANGE_TIMEOUT(10000);

/// The name of the @c FocusManager channel used by the @c SpeechSynthesizer.
static const std::string CHANNEL_NAME(avsCommon::sdkInterfaces::FocusManagerInterface::DIALOG_CHANNEL_NAME);

/// Namespace for SpeechSynthesizer.
static const std::string NAMESPACE_SPEECH_SYNTHESIZER("SpeechSynthesizer");

/// Name for SpeechSynthesizer directive.
static const std::string NAME_SPEAK("Speak");

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

/// A payload for testing
// clang-format off
static const std::string PAYLOAD_TEST =
        "{"
            "\"url\":\"" + URL_TEST + "\","
            "\"format\":\"" + FORMAT_TEST + "\","
            "\"token\":\""+ TOKEN_TEST + "\""
        "}";
// clang-format on

/// The @c FINISHED state of the @c SpeechSynthesizer.
static const std::string FINISHED_STATE("FINISHED");

/// The @c PLAYING state of the @c SpeechSynthesizer
static const std::string PLAYING_STATE{"PLAYING"};

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

/**
 * MockAttachmentManager
 */
class MockAttachmentManager : public AttachmentManagerInterface {
public:
    MOCK_CONST_METHOD2(generateAttachmentId, std::string(const std::string&, const std::string&));
    MOCK_METHOD1(setAttachmentTimeoutMinutes, bool(std::chrono::minutes timeoutMinutes));
    MOCK_METHOD2(
        createWriter,
        std::unique_ptr<AttachmentWriter>(const std::string& attachmentId, sds::WriterPolicy policy));
    MOCK_METHOD2(
        createReader,
        std::unique_ptr<AttachmentReader>(const std::string& attachmentId, sds::ReaderPolicy policy));
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

    /// The @c DialogUXStateAggregator to test with.
    std::shared_ptr<avsCommon::avs::DialogUXStateAggregator> m_dialogUXStateAggregator;
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
    m_mockContextManager = std::make_shared<NiceMock<MockContextManager>>();
    m_mockFocusManager = std::make_shared<NiceMock<MockFocusManager>>();
    m_mockMessageSender = std::make_shared<NiceMock<MockMessageSender>>();
    m_mockExceptionSender = std::make_shared<NiceMock<MockExceptionEncounteredSender>>();
    m_attachmentManager = std::make_shared<AttachmentManager>(AttachmentManager::AttachmentType::IN_PROCESS);
    m_mockSpeechPlayer = MockMediaPlayer::create();
    m_dialogUXStateAggregator = std::make_shared<avsCommon::avs::DialogUXStateAggregator>();
    m_speechSynthesizer = SpeechSynthesizer::create(
        m_mockSpeechPlayer,
        m_mockMessageSender,
        m_mockFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        m_dialogUXStateAggregator);
    m_mockDirHandlerResult.reset(new MockDirectiveHandlerResult);

    ASSERT_TRUE(m_speechSynthesizer);

    m_speechSynthesizer->addObserver(m_dialogUXStateAggregator);
}

void SpeechSynthesizerTest::TearDown() {
    m_speechSynthesizer->removeObserver(m_dialogUXStateAggregator);
    m_speechSynthesizer->shutdown();
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
 * Test call to handleDirectiveImmediately.
 * Expected result is that @c acquireChannel is called with the correct channel. On focus changed @c FOREGROUND, audio
 * should play. Expect the @c ContextManager @c setState is called when state changes to @c PLAYING.
 */
TEST_F(SpeechSynthesizerTest, testCallingHandleImmediately) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _, NAMESPACE_SPEECH_SYNTHESIZER))
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
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(NAMESPACE_AND_NAME_SPEECH_STATE, PLAYING_STATE_TEST, StateRefreshPolicy::ALWAYS, 0))
        .Times(AtLeast(1))
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));

    m_speechSynthesizer->handleDirectiveImmediately(directive);
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(WAIT_TIMEOUT));
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND);
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
    ASSERT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(WAIT_TIMEOUT));
}

/**
 * Tests preHandleDirective and HandleDirective
 * Call preHandle with a valid SPEAK directive. Then call handleDirective. Expected result is that @c acquireChannel
 * is called with the correct channel. On focus changed @c FOREGROUND, audio should play. Expect the @c ContextManager
 * @c setState is called when state changes to @c PLAYING.
 */
TEST_F(SpeechSynthesizerTest, testCallingHandle) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _, NAMESPACE_SPEECH_SYNTHESIZER))
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

    m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirHandlerResult));
    m_speechSynthesizer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(WAIT_TIMEOUT));
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND);
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
    ASSERT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(WAIT_TIMEOUT));
}

/**
 * Tests cancelDirective.
 * Call preHandle with a valid SPEAK directive. Then call cancelDirective. Expect that neither @c setState nor
 * @c sendMessage are called since handle was never called to start playing audio.
 */
TEST_F(SpeechSynthesizerTest, testCallingCancel) {
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
 * @c ContextManager @c setState is called when the state changes to @c PLAYING and then to @c FINISHED.
 * Expect @c sendMessage is called only once. On cancel, message should not be sent to AVS.
 */
TEST_F(SpeechSynthesizerTest, testCallingCancelAfterHandle) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _, NAMESPACE_SPEECH_SYNTHESIZER))
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
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
    EXPECT_CALL(*(m_mockFocusManager.get()), releaseChannel(CHANNEL_NAME, _))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnReleaseChannel));

    m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirHandlerResult));
    m_speechSynthesizer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(WAIT_TIMEOUT));
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND);
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();
    ASSERT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(WAIT_TIMEOUT));
    m_speechSynthesizer->CapabilityAgent::cancelDirective(MESSAGE_ID_TEST);
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStopped());
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
    ASSERT_TRUE(std::future_status::ready == m_wakeReleaseChannelFuture.wait_for(WAIT_TIMEOUT));
}

/**
 * Testing provideState.
 * Call @c provideState and expect that setState is called.
 */
TEST_F(SpeechSynthesizerTest, testCallingProvideStateWhenNotPlaying) {
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), getOffset(_)).Times(0);
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(NAMESPACE_AND_NAME_SPEECH_STATE, IDLE_STATE_TEST, StateRefreshPolicy::NEVER, PROVIDE_STATE_TOKEN_TEST))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));

    m_speechSynthesizer->provideState(NAMESPACE_AND_NAME_SPEECH_STATE, PROVIDE_STATE_TOKEN_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
}

/**
 * Testing provideState when playing.
 * Call @c provideState after the state of the @c SpeechSynthesizer has changed to @c PLAYING.
 * Expect @c getOffset is called. Expect @c setState is called when state changes and when state is
 * requested via @c provideState.
 */
TEST_F(SpeechSynthesizerTest, testCallingProvideStateWhenPlaying) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _, NAMESPACE_SPEECH_SYNTHESIZER))
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
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(NAMESPACE_AND_NAME_SPEECH_STATE, FINISHED_STATE_TEST, StateRefreshPolicy::NEVER, 0))
        .Times(1)
        .WillOnce(Return(SetStateResult::SUCCESS));
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
    EXPECT_CALL(*(m_mockFocusManager.get()), releaseChannel(CHANNEL_NAME, _))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnReleaseChannel));

    m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirHandlerResult));
    m_speechSynthesizer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(WAIT_TIMEOUT));
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND);
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();
    ASSERT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(WAIT_TIMEOUT));
    m_speechSynthesizer->provideState(NAMESPACE_AND_NAME_SPEECH_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
    m_speechSynthesizer->CapabilityAgent::cancelDirective(MESSAGE_ID_TEST);
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStopped());
    ASSERT_TRUE(std::future_status::ready == m_wakeReleaseChannelFuture.wait_for(WAIT_TIMEOUT));
}

/**
 * Testing barge-in via @c handleDirectiveImmediately when audio is playing back.
 * Call @c handleDirective. Once playback started notification is received, call @c handleDirectiveImmediately.
 */
TEST_F(SpeechSynthesizerTest, testBargeInWhilePlaying) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    auto avsMessageHeader2 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST_2);
    std::shared_ptr<AVSDirective> directive2 =
        AVSDirective::create("", avsMessageHeader2, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST_2);

    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _, NAMESPACE_SPEECH_SYNTHESIZER))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnAcquireChannel));
    EXPECT_CALL(
        *(m_mockSpeechPlayer.get()),
        attachmentSetSource(A<std::shared_ptr<avsCommon::avs::attachment::AttachmentReader>>(), nullptr))
        .Times(AtLeast(1));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), play(_)).Times(AtLeast(1));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), getOffset(_))
        .Times(1)
        .WillOnce(Return(OFFSET_IN_CHRONO_MILLISECONDS_TEST));
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
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
    EXPECT_CALL(*(m_mockFocusManager.get()), releaseChannel(CHANNEL_NAME, _))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnReleaseChannel));

    m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirHandlerResult));
    m_speechSynthesizer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(WAIT_TIMEOUT));
    m_wakeAcquireChannelPromise = std::promise<void>();
    m_wakeAcquireChannelFuture = m_wakeAcquireChannelPromise.get_future();
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND);
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();
    ASSERT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(WAIT_TIMEOUT));
    m_speechSynthesizer->handleDirectiveImmediately(directive2);
    m_speechSynthesizer->CapabilityAgent::cancelDirective(MESSAGE_ID_TEST);
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStopped());
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
    ASSERT_TRUE(std::future_status::ready == m_wakeReleaseChannelFuture.wait_for(WAIT_TIMEOUT));
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(WAIT_TIMEOUT));
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
TEST_F(SpeechSynthesizerTest, testNotCallStopTwice) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    auto avsMessageHeader2 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST_2);
    std::shared_ptr<AVSDirective> directive2 =
        AVSDirective::create("", avsMessageHeader2, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST_2);

    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _, NAMESPACE_SPEECH_SYNTHESIZER))
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
        setState(NAMESPACE_AND_NAME_SPEECH_STATE, FINISHED_STATE_TEST, StateRefreshPolicy::NEVER, 0))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
    EXPECT_CALL(*(m_mockFocusManager.get()), releaseChannel(CHANNEL_NAME, _))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnReleaseChannel));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), stop(_))
        .Times(2)
        .WillOnce(Invoke([this](avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId id) {
            wakeOnStopped();
            m_speechSynthesizer->onPlaybackStopped(id);
            return true;
        }))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*(m_mockDirHandlerResult.get()), setCompleted()).Times(AtLeast(0));

    // send Speak directive and getting focus and wait until playback started
    m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirHandlerResult));
    m_speechSynthesizer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(WAIT_TIMEOUT));
    m_wakeAcquireChannelPromise = std::promise<void>();
    m_wakeAcquireChannelFuture = m_wakeAcquireChannelPromise.get_future();
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND);
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();
    ASSERT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(WAIT_TIMEOUT));
    m_wakeSendMessagePromise = std::promise<void>();
    m_wakeSendMessageFuture = m_wakeSendMessagePromise.get_future();

    // cancel directive, this should result in calling stop()
    m_speechSynthesizer->CapabilityAgent::cancelDirective(MESSAGE_ID_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeStoppedFuture.wait_for(WAIT_TIMEOUT));

    // goes to background, this should not result in calling the 2nd stop()
    m_speechSynthesizer->onFocusChanged(FocusState::BACKGROUND);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();

    /*
     * onPlaybackStopped, this will result in an error with reason=nullptrDirectiveInfo.  But this shouldn't break the
     * SpeechSynthesizer
     */
    ASSERT_TRUE(std::future_status::ready == m_wakeReleaseChannelFuture.wait_for(WAIT_TIMEOUT));
    m_wakeReleaseChannelPromise = std::promise<void>();
    m_wakeReleaseChannelFuture = m_wakeReleaseChannelPromise.get_future();

    // send second speak directive and make sure it working
    m_speechSynthesizer->handleDirectiveImmediately(directive2);
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(WAIT_TIMEOUT));
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();
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
TEST_F(SpeechSynthesizerTest, testMediaPlayerFailedToStop) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    auto avsMessageHeader2 =
        std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST_2);
    std::shared_ptr<AVSDirective> directive2 =
        AVSDirective::create("", avsMessageHeader2, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST_2);

    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _, NAMESPACE_SPEECH_SYNTHESIZER))
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
        setState(NAMESPACE_AND_NAME_SPEECH_STATE, FINISHED_STATE_TEST, StateRefreshPolicy::NEVER, 0))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSetState));
    EXPECT_CALL(*(m_mockMessageSender.get()), sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnSendMessage));
    EXPECT_CALL(*(m_mockFocusManager.get()), releaseChannel(CHANNEL_NAME, _))
        .Times(AtLeast(1))
        .WillRepeatedly(InvokeWithoutArgs(this, &SpeechSynthesizerTest::wakeOnReleaseChannel));
    EXPECT_CALL(*(m_mockSpeechPlayer.get()), stop(_))
        .Times(2)
        .WillOnce(Invoke([this](avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId id) {
            wakeOnStopped();
            return false;
        }))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*(m_mockDirHandlerResult.get()), setFailed(_)).Times(AtLeast(0));

    // send Speak directive and getting focus and wait until playback started
    m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirHandlerResult));
    m_speechSynthesizer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(WAIT_TIMEOUT));
    m_wakeAcquireChannelPromise = std::promise<void>();
    m_wakeAcquireChannelFuture = m_wakeAcquireChannelPromise.get_future();
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND);
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();
    ASSERT_TRUE(std::future_status::ready == m_wakeSendMessageFuture.wait_for(WAIT_TIMEOUT));
    m_wakeSendMessagePromise = std::promise<void>();
    m_wakeSendMessageFuture = m_wakeSendMessagePromise.get_future();

    // cancel directive, this should result in calling stop()
    m_speechSynthesizer->CapabilityAgent::cancelDirective(MESSAGE_ID_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeStoppedFuture.wait_for(WAIT_TIMEOUT));

    // goes to background, this should not result in calling the 2nd stop()
    ASSERT_TRUE(std::future_status::ready == m_wakeReleaseChannelFuture.wait_for(WAIT_TIMEOUT));
    m_speechSynthesizer->onFocusChanged(FocusState::BACKGROUND);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();

    /*
     * onPlaybackStopped, this will result in an error with reason=nullptrDirectiveInfo.  But this shouldn't break the
     * SpeechSynthesizer
     */
    ASSERT_TRUE(std::future_status::ready == m_wakeReleaseChannelFuture.wait_for(WAIT_TIMEOUT));
    m_wakeReleaseChannelPromise = std::promise<void>();
    m_wakeReleaseChannelFuture = m_wakeReleaseChannelPromise.get_future();

    // send second speak directive and make sure it working
    m_speechSynthesizer->handleDirectiveImmediately(directive2);
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(WAIT_TIMEOUT));
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();
    ASSERT_TRUE(m_mockSpeechPlayer->waitUntilPlaybackStarted());
}

/**
 * Testing SpeechSynthesizer will call stop() if the SpeechSynthesizer experienced a state change timeout to PLAYING
 * state.
 */
TEST_F(SpeechSynthesizerTest, testSetStateTimeout) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PAYLOAD_TEST, m_attachmentManager, CONTEXT_ID_TEST);

    EXPECT_CALL(*(m_mockFocusManager.get()), acquireChannel(CHANNEL_NAME, _, NAMESPACE_SPEECH_SYNTHESIZER))
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

    // Send Speak directive and getting focus and wait until state change timeout.
    m_speechSynthesizer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirHandlerResult));
    m_speechSynthesizer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeAcquireChannelFuture.wait_for(WAIT_TIMEOUT));
    m_wakeAcquireChannelPromise = std::promise<void>();
    m_wakeAcquireChannelFuture = m_wakeAcquireChannelPromise.get_future();
    m_speechSynthesizer->onFocusChanged(FocusState::FOREGROUND);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetFailedFuture.wait_for(STATE_CHANGE_TIMEOUT));

    // Upon getting onPlaybackedStarted, expect state to be updated, but SpeechStarted event will not be sent.
    m_speechSynthesizer->onPlaybackStarted(m_mockSpeechPlayer->getCurrentSourceId());
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();

    // Upon getting onPlaybackStopped, expect state to be updated, but SpeechFinished event will not be sent.
    m_speechSynthesizer->onPlaybackStopped(m_mockSpeechPlayer->getCurrentSourceId());
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();

    ASSERT_TRUE(std::future_status::ready == m_wakeReleaseChannelFuture.wait_for(WAIT_TIMEOUT));
    m_speechSynthesizer->onFocusChanged(FocusState::BACKGROUND);
}

}  // namespace test
}  // namespace speechSynthesizer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
