/**
 * AudioInputProcessorTest.cpp
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

/// @file AudioInputProcessorTest.cpp

#include <gtest/gtest.h>

#include <AVSCommon/SDKInterfaces/MockDirectiveSequencer.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockFocusManager.h>
#include <AVSCommon/MockExceptionEncounteredSender.h>
#include <AVSUtils/Memory/Memory.h>

#include "AIP/AudioInputProcessor.h"
#include "MockObserver.h"

using namespace testing;

namespace alexaClientSDK {
namespace capabilityAgent {
namespace aip {
namespace test {

/// The name of the @c FocusManager channel used by @c AudioInputProvider.
static const std::string CHANNEL_NAME = "Dialog";

/// The activityId string used with @c FocusManager by @c AudioInputProvider.
static const std::string ACTIVITY_ID = "SpeechRecognizer.Recognize";

/// The namespace for this capability agent.
static const std::string NAMESPACE = "SpeechRecognizer";

/// The StopCapture directive signature.
static const avsCommon::avs::NamespaceAndName STOP_CAPTURE{NAMESPACE, "StopCapture"};

/// The ExpectSpeech directive signature.
static const avsCommon::avs::NamespaceAndName EXPECT_SPEECH{NAMESPACE, "ExpectSpeech"};

/// The directives @c AudioInputProcessor should handle.
avsCommon::avs::NamespaceAndName DIRECTIVES[] = {STOP_CAPTURE, EXPECT_SPEECH};

/// The SpeechRecognizer context state signature.
static const avsCommon::avs::NamespaceAndName RECOGNIZER_STATE{NAMESPACE, "RecognizerState"};

/// Number of directives @c AudioInputProcessor should handle.
static const size_t NUM_DIRECTIVES = sizeof(DIRECTIVES) / sizeof(*DIRECTIVES);

/// The @c BlockingPolicy for all @c AudioInputProcessor directives.
static const auto BLOCKING_POLICY = avsCommon::avs::BlockingPolicy::NON_BLOCKING;

/// Sample rate for audio input stream.
static const unsigned int SAMPLE_RATE_HZ = 16000;

/// Sample size for audio input stream.
static const unsigned int SAMPLE_SIZE_IN_BITS = 16;

/// Number of channels in audio input stream.
static const unsigned int NUM_CHANNELS = 1;

/// Number of seconds of audio to hold in the SDS circular buffer.
static const size_t SDS_SECONDS = 1;

/// Number of words to hold in the SDS circular buffer.
static const size_t SDS_WORDS = SDS_SECONDS * SAMPLE_RATE_HZ;

/// Number of bytes per word in the SDS circular buffer.
static const size_t SDS_WORDSIZE = SAMPLE_SIZE_IN_BITS / 8;

/// Maximum number of readers to support in the SDS circular buffer.
static const size_t SDS_MAXREADERS = 2;

/// Boolean value to indicate an AudioProvider is always readable.
static const bool ALWAYS_READABLE = true;

/// Boolean value to indicate an AudioProvider can override another AudioProvider.
static const bool CAN_OVERRIDE = true;

/// Boolean value to indicate an AudioProvider can be overridden by another AudioProvider.
static const bool CAN_BE_OVERRIDDEN = true;

/// Test harness for @c AudioInputProcessor class.
class AudioInputProcessorTest: public ::testing::Test {
public:
    /// Set up the test harness for running a test.
    void SetUp() override;

    /// Clean up the test harness after running a test.
    void TearDown() override;

protected:
    /// The mock @c DirectiveSequencerInterface.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockDirectiveSequencer> m_mockDirectiveSequencer;

    /// The mock @c MessageSenderInterface.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockMessageSender> m_mockMessageSender;

    /// The mock @c ContextManagerInterface.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockContextManager> m_mockContextManager;

    /// The mock @c FocusManagerInterface.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockFocusManager> m_mockFocusManager;

    /// The mock @c ExceptionEncounteredSenderInterface.
    std::shared_ptr<avsCommon::MockExceptionEncounteredSender> m_mockExceptionEncounteredSender;

    /// The @c Buffer backing @c m_stream.
    std::shared_ptr<avsCommon::utils::sds::InProcessSDS::Buffer> m_buffer;

    /// The @c SharedDataStream to test with.
    std::shared_ptr<avsCommon::utils::sds::InProcessSDS> m_stream;

    /// The format of @c m_stream;
    avsCommon::AudioFormat m_format;

    /// The @c AudioProvider to test with.
    std::unique_ptr<AudioProvider> m_audioProvider;

    /// The @c AudioInputProcessor to test.
    std::shared_ptr<AudioInputProcessor> m_audioInputProcessor;

    /// The mock @c ObserverInterface.
    std::shared_ptr<MockObserver> m_mockObserver;
};

void AudioInputProcessorTest::SetUp() {
    m_mockDirectiveSequencer = std::make_shared<avsCommon::sdkInterfaces::test::MockDirectiveSequencer>();
    m_mockMessageSender = std::make_shared<avsCommon::sdkInterfaces::test::MockMessageSender>();
    m_mockContextManager = std::make_shared<avsCommon::sdkInterfaces::test::MockContextManager>();
    m_mockFocusManager = std::make_shared<avsCommon::sdkInterfaces::test::MockFocusManager>();
    m_mockExceptionEncounteredSender = std::make_shared<avsCommon::MockExceptionEncounteredSender>();
    size_t bufferSize =
            avsCommon::utils::sds::InProcessSDS::calculateBufferSize(SDS_WORDS, SDS_WORDSIZE, SDS_MAXREADERS);
    m_buffer = std::make_shared<avsCommon::utils::sds::InProcessSDS::Buffer>(bufferSize);
    m_stream = avsCommon::utils::sds::InProcessSDS::create(m_buffer);
    ASSERT_NE(m_stream, nullptr);
    m_format = {
            avsCommon::AudioFormat::Encoding::LPCM,
            avsCommon::AudioFormat::Endianness::LITTLE,
            SAMPLE_RATE_HZ,
            SAMPLE_SIZE_IN_BITS,
            NUM_CHANNELS };
    m_audioProvider = avsUtils::memory::make_unique<AudioProvider>(
            m_stream,
            m_format,
            ASRProfile::NEAR_FIELD,
            ALWAYS_READABLE,
            CAN_OVERRIDE,
            CAN_BE_OVERRIDDEN);
    EXPECT_CALL(*m_mockContextManager, setStateProvider(RECOGNIZER_STATE, Ne(nullptr))).Times(1);
    m_audioInputProcessor = AudioInputProcessor::create(
            m_mockDirectiveSequencer,
            m_mockMessageSender,
            m_mockContextManager,
            m_mockFocusManager,
            m_mockExceptionEncounteredSender,
            *m_audioProvider);
    EXPECT_NE(m_audioInputProcessor, nullptr);
    m_mockObserver = std::make_shared<MockObserver>();
    ASSERT_NE(m_mockObserver, nullptr);
    m_audioInputProcessor->addObserver(m_mockObserver);

}

void AudioInputProcessorTest::TearDown() {
    EXPECT_CALL(*m_mockFocusManager, releaseChannel("Dialog", _)).Times(AtLeast(0));
    EXPECT_CALL(*m_mockObserver, onStateChanged(AudioInputProcessor::State::IDLE)).Times(AtLeast(0));
    m_audioInputProcessor->resetState().wait();
}

TEST_F(AudioInputProcessorTest, create) {
    // Verify it fails to create with nullptr for any of the first four parameters.
    m_audioInputProcessor = AudioInputProcessor::create(
            nullptr,
            m_mockMessageSender,
            m_mockContextManager,
            m_mockFocusManager,
            m_mockExceptionEncounteredSender,
            *m_audioProvider);
    EXPECT_EQ(m_audioInputProcessor, nullptr);
    m_audioInputProcessor = AudioInputProcessor::create(
            m_mockDirectiveSequencer,
            nullptr,
            m_mockContextManager,
            m_mockFocusManager,
            m_mockExceptionEncounteredSender,
            *m_audioProvider);
    EXPECT_EQ(m_audioInputProcessor, nullptr);
    m_audioInputProcessor = AudioInputProcessor::create(
            m_mockDirectiveSequencer,
            m_mockMessageSender,
            nullptr,
            m_mockFocusManager,
            m_mockExceptionEncounteredSender,
            *m_audioProvider);
    EXPECT_EQ(m_audioInputProcessor, nullptr);
    m_audioInputProcessor = AudioInputProcessor::create(
            m_mockDirectiveSequencer,
            m_mockMessageSender,
            m_mockContextManager,
            nullptr,
            m_mockExceptionEncounteredSender,
            *m_audioProvider);
    EXPECT_EQ(m_audioInputProcessor, nullptr);
    m_audioInputProcessor = AudioInputProcessor::create(
            m_mockDirectiveSequencer,
            m_mockMessageSender,
            m_mockContextManager,
            m_mockFocusManager,
            nullptr,
            *m_audioProvider);
    EXPECT_EQ(m_audioInputProcessor, nullptr);

    // Verify it does *not* fail to create with nullptr for the final parameter.
    EXPECT_CALL(*m_mockContextManager, setStateProvider(RECOGNIZER_STATE, Ne(nullptr))).Times(1);
    m_audioInputProcessor = AudioInputProcessor::create(
            m_mockDirectiveSequencer,
            m_mockMessageSender,
            m_mockContextManager,
            m_mockFocusManager,
            m_mockExceptionEncounteredSender,
            AudioProvider::null());
    EXPECT_NE(m_audioInputProcessor, nullptr);
}

TEST_F(AudioInputProcessorTest, getConfiguration) {
    auto configuration = m_audioInputProcessor->getConfiguration();
    EXPECT_EQ(configuration.size(), NUM_DIRECTIVES);
    avsCommon::avs::HandlerAndPolicy handlerAndPolicy{m_audioInputProcessor, BLOCKING_POLICY};
    for (auto namespaceAndName: DIRECTIVES) {
        auto directive = configuration.find(namespaceAndName);
        EXPECT_NE(directive, configuration.end());
        if (configuration.end() == directive) {
            continue;
        }
        EXPECT_EQ(directive->second, handlerAndPolicy);
    }
}

TEST_F(AudioInputProcessorTest, addRemoveObserver) {
    // Nothing to assert directly here, but make sure these run without crashing.

    // Null pointer detection.
    m_audioInputProcessor->addObserver(nullptr);
    m_audioInputProcessor->removeObserver(nullptr);

    // Add/remove single observer.
    auto observer = std::make_shared<MockObserver>();
    m_audioInputProcessor->addObserver(observer);
    m_audioInputProcessor->removeObserver(observer);

    // Add multiple observers.
    auto observer2 = std::make_shared<MockObserver>();
    m_audioInputProcessor->addObserver(observer);
    m_audioInputProcessor->addObserver(observer2);

    // Remove both observers (out of order).
    m_audioInputProcessor->removeObserver(observer);
    m_audioInputProcessor->removeObserver(observer2);

    // Try to re-remove an observer which is no longer registered.
    m_audioInputProcessor->removeObserver(observer);
}

TEST_F(AudioInputProcessorTest, recognizeNullStream) {
    // Verify null stream is detected/rejected.
    auto result = m_audioInputProcessor->recognize(AudioProvider::null(), Initiator::PRESS_AND_HOLD);
    ASSERT_TRUE(result.valid());
    ASSERT_FALSE(result.get());
}

TEST_F(AudioInputProcessorTest, recognizePressAndHoldWithDefaults) {
    std::mutex mutex;
    std::condition_variable conditionVariable;
    bool done = false;

    EXPECT_CALL(*m_mockContextManager, getContext(_))
        .WillOnce(InvokeWithoutArgs([this] { m_audioInputProcessor->onContextAvailable(""); }));
    EXPECT_CALL(*m_mockObserver, onStateChanged(AudioInputProcessor::State::RECOGNIZING));
    EXPECT_CALL(*m_mockFocusManager, acquireChannel("Dialog", _, "SpeechRecognizer.Recognize"))
        .WillOnce(InvokeWithoutArgs([this] {
            m_audioInputProcessor->onFocusChanged(avsCommon::sdkInterfaces::FocusState::FOREGROUND);
            return true;}));
    EXPECT_CALL(*m_mockDirectiveSequencer, setDialogRequestId(_));
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .WillOnce(InvokeWithoutArgs([&] {
            std::lock_guard<std::mutex> lock(mutex);
            done = true;
            conditionVariable.notify_one(); }));
    auto result = m_audioInputProcessor->recognize(*m_audioProvider, Initiator::PRESS_AND_HOLD);
    ASSERT_TRUE(result.valid());
    ASSERT_TRUE(result.get());
    std::unique_lock<std::mutex> lock(mutex);
    ASSERT_TRUE(conditionVariable.wait_for(lock, std::chrono::seconds(10), [&done] { return done; } ));
}

} // namespace test
} // namespace aip
} // namespace capabilityAgent
} // namespace alexaClientSDK
