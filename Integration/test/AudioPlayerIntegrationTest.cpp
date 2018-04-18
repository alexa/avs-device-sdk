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

/// @file AudioPlayerIntegrationTest.cpp

#include <chrono>
#include <deque>
#include <fstream>
#include <future>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>

#include <gtest/gtest.h>

#include <ADSL/DirectiveSequencer.h>
#include <ADSL/MessageInterpreter.h>
#include <AFML/FocusManager.h>
#include <AIP/AudioInputProcessor.h>
#include <AIP/AudioProvider.h>
#include <AIP/Initiator.h>
#include <AudioPlayer/AudioPlayer.h>
#include <AVSCommon/AVS/Attachment/InProcessAttachmentReader.h>
#include <AVSCommon/AVS/Attachment/InProcessAttachmentWriter.h>
#include <AVSCommon/AVS/BlockingPolicy.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>
#include <AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveHandlerResultInterface.h>
#include <AVSCommon/Utils/Logger/LogEntry.h>
#include <PlaybackController/PlaybackController.h>
#include <PlaybackController/PlaybackRouter.h>
#include <SpeechSynthesizer/SpeechSynthesizer.h>
#include <System/UserInactivityMonitor.h>

#include "Integration/ACLTestContext.h"
#include "Integration/ObservableMessageRequest.h"
#include "Integration/TestDirectiveHandler.h"
#include "Integration/TestExceptionEncounteredSender.h"
#include "Integration/TestMessageSender.h"
#include "Integration/TestSpeechSynthesizerObserver.h"

#ifdef GSTREAMER_MEDIA_PLAYER
#include "MediaPlayer/MediaPlayer.h"
#else
#include "Integration/TestMediaPlayer.h"
#endif

namespace alexaClientSDK {
namespace integration {
namespace test {

using namespace acl;
using namespace adsl;
using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::mediaPlayer;
using namespace contextManager;
using namespace sdkInterfaces;
using namespace avsCommon::utils::sds;
using namespace avsCommon::utils::json;
using namespace capabilityAgents::aip;
using namespace capabilityAgents::playbackController;
using namespace afml;
using namespace capabilityAgents::speechSynthesizer;
using namespace capabilityAgents::system;
#ifdef GSTREAMER_MEDIA_PLAYER
using namespace mediaPlayer;
#endif

// This is a 16 bit 16 kHz little endian linear PCM audio file of "Sing me a song" to be recognized.
static const std::string RECOGNIZE_SING_FILE_NAME = "/recognize_sing_song_test.wav";
// This is a 16 bit 16 kHz little endian linear PCM audio file of "Flashbriefing" to be recognized.
static const std::string RECOGNIZE_FLASHBRIEFING_FILE_NAME = "/recognize_flashbriefing_test.wav";

// This string to be used for PlaybackStarted Directives which use the AudioPlayer namespace.
static const std::string NAME_PLAYBACK_STARTED = "PlaybackStarted";
// This string to be used for PlaybackNearlyFinished Directives which use the AudioPlayer namespace.
static const std::string NAME_PLAYBACK_NEARLY_FINISHED = "PlaybackNearlyFinished";
// This string to be used for PlaybackFinished Directives which use the AudioPlayer namespace.
static const std::string NAME_PLAYBACK_FINISHED = "PlaybackFinished";
// This string to be used for PlaybackStopped Directives which use the AudioPlayer namespace.
static const std::string NAME_PLAYBACK_STOPPED = "PlaybackStopped";
// This string to be used for SynchronizeState Directives.
static const std::string NAME_SYNC_STATE = "SynchronizeState";
// This string to be used for Speak Directives which use the NAMESPACE_SPEECH_SYNTHESIZER namespace.
static const std::string NAME_RECOGNIZE = "Recognize";
// This string to be used for SpeechStarted Directives which use the NAMESPACE_SPEECH_SYNTHESIZER namespace.
static const std::string NAME_SPEECH_STARTED = "SpeechStarted";
// This string to be used for SpeechFinished Directives which use the NAMESPACE_SPEECH_SYNTHESIZER namespace.
static const std::string NAME_SPEECH_FINISHED = "SpeechFinished";

/// The dialog Channel name used in intializing the FocusManager.
static const std::string DIALOG_CHANNEL_NAME = "Dialog";

/// The content Channel name used in intializing the FocusManager.
static const std::string CONTENT_CHANNEL_NAME = "Content";

/// An incorrect Channel name that is never initialized as a Channel.
static const std::string TEST_CHANNEL_NAME = "Test";

/// The priority of the dialog Channel used in intializing the FocusManager.
static const unsigned int DIALOG_CHANNEL_PRIORITY = 100;

/// The priority of the content Channel used in intializing the FocusManager.
static const unsigned int CONTENT_CHANNEL_PRIORITY = 300;

/// The priority of the content Channel used in intializing the FocusManager.
static const unsigned int TEST_CHANNEL_PRIORITY = 400;

/// Sample dialog activity id.
static const std::string DIALOG_ACTIVITY_ID = "dialog";

/// Sample content activity id.
static const std::string CONTENT_ACTIVITY_ID = "content";

/// Sample content activity id.
static const std::string TEST_ACTIVITY_ID = "test";

// This Integer to be used to specify a timeout in seconds.
static const std::chrono::seconds WAIT_FOR_TIMEOUT_DURATION(15);
static const std::chrono::seconds NO_TIMEOUT_DURATION(0);
static const std::chrono::seconds SONG_TIMEOUT_DURATION(120);
/// The compatible encoding for AIP.
static const avsCommon::utils::AudioFormat::Encoding COMPATIBLE_ENCODING =
    avsCommon::utils::AudioFormat::Encoding::LPCM;
/// The compatible endianness for AIP.
static const avsCommon::utils::AudioFormat::Endianness COMPATIBLE_ENDIANNESS =
    avsCommon::utils::AudioFormat::Endianness::LITTLE;
/// The compatible sample rate for AIP.
static const unsigned int COMPATIBLE_SAMPLE_RATE = 16000;
/// The compatible bits per sample for Kitt.ai.
static const unsigned int COMPATIBLE_SAMPLE_SIZE_IN_BITS = 16;
/// The compatible number of channels for Kitt.ai
static const unsigned int COMPATIBLE_NUM_CHANNELS = 1;

/// JSON key to get the event object of a message.
static const std::string JSON_MESSAGE_EVENT_KEY = "event";
/// JSON key to get the directive object of a message.
static const std::string JSON_MESSAGE_DIRECTIVE_KEY = "directive";
/// JSON key to get the header object of a message.
static const std::string JSON_MESSAGE_HEADER_KEY = "header";
/// JSON key to get the namespace value of a header.
static const std::string JSON_MESSAGE_NAMESPACE_KEY = "namespace";
/// JSON key to get the name value of a header.
static const std::string JSON_MESSAGE_NAME_KEY = "name";
/// JSON key to get the messageId value of a header.
static const std::string JSON_MESSAGE_MESSAGE_ID_KEY = "messageId";
/// JSON key to get the dialogRequestId value of a header.
static const std::string JSON_MESSAGE_DIALOG_REQUEST_ID_KEY = "dialogRequestId";
/// JSON key to get the payload object of a message.
static const std::string JSON_MESSAGE_PAYLOAD_KEY = "payload";

/// String to identify log entries originating from this file.
static const std::string TAG("AudioPlayerIntegrationTest");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Path to the AlexaClientSDKConfig.json file (from command line arguments).
static std::string g_configPath;
/// Path to resources (e.g. audio files) for tests (from command line arguments).
static std::string g_inputPath;

/// A test observer that mocks out the ChannelObserverInterface##onFocusChanged() call.
class TestClient : public ChannelObserverInterface {
public:
    /**
     * Constructor.
     */
    TestClient() : m_focusState(FocusState::NONE), m_focusChangeOccurred(false) {
    }

    /**
     * Implementation of the ChannelObserverInterface##onFocusChanged() callback.
     *
     * @param focusState The new focus state of the Channel observer.
     */
    void onFocusChanged(FocusState focusState) override {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_focusState = focusState;
        m_focusChangeOccurred = true;
        m_focusChangedCV.notify_one();
    }

    /**
     * Waits for the ChannelObserverInterface##onFocusChanged() callback.
     *
     * @param timeout The amount of time to wait for the callback.
     * @param focusChanged An output parameter that notifies the caller whether a callback occurred.
     * @return Returns @c true if the callback occurred within the timeout period and @c false otherwise.
     */
    FocusState waitForFocusChange(std::chrono::milliseconds timeout, bool* focusChanged) {
        std::unique_lock<std::mutex> lock(m_mutex);
        bool success = m_focusChangedCV.wait_for(lock, timeout, [this]() { return m_focusChangeOccurred; });

        if (!success) {
            *focusChanged = false;
        } else {
            m_focusChangeOccurred = false;
            *focusChanged = true;
        }
        return m_focusState;
    }

private:
    /// The focus state of the observer.
    FocusState m_focusState;

    /// A lock to guard against focus state changes.
    std::mutex m_mutex;

    /// A condition variable to wait for focus changes.
    std::condition_variable m_focusChangedCV;

    /// A boolean flag so that we can re-use the observer even after a callback has occurred.
    bool m_focusChangeOccurred;
};

class holdToTalkButton {
public:
    bool startRecognizing(std::shared_ptr<AudioInputProcessor> aip, std::shared_ptr<AudioProvider> audioProvider) {
        return aip->recognize(*audioProvider, Initiator::PRESS_AND_HOLD).get();
    }

    bool stopRecognizing(std::shared_ptr<AudioInputProcessor> aip) {
        return aip->stopCapture().get();
    }
};

class AudioPlayerTest : public ::testing::Test {
protected:
    virtual void SetUp() override {
        m_context = ACLTestContext::create(g_configPath);
        ASSERT_TRUE(m_context);

        m_exceptionEncounteredSender = std::make_shared<TestExceptionEncounteredSender>();
        m_dialogUXStateAggregator = std::make_shared<avsCommon::avs::DialogUXStateAggregator>();

        m_directiveSequencer = DirectiveSequencer::create(m_exceptionEncounteredSender);
        m_messageInterpreter = std::make_shared<MessageInterpreter>(
            m_exceptionEncounteredSender, m_directiveSequencer, m_context->getAttachmentManager());

        // Set up connection and connect
        m_avsConnectionManager = std::make_shared<TestMessageSender>(
            m_context->getMessageRouter(), false, m_context->getConnectionStatusObserver(), m_messageInterpreter);
        ASSERT_TRUE(m_avsConnectionManager);

        FocusManager::ChannelConfiguration dialogChannelConfig{DIALOG_CHANNEL_NAME, DIALOG_CHANNEL_PRIORITY};
        FocusManager::ChannelConfiguration contentChannelConfig{CONTENT_CHANNEL_NAME, CONTENT_CHANNEL_PRIORITY};
        FocusManager::ChannelConfiguration testChannelConfig{TEST_CHANNEL_NAME, TEST_CHANNEL_PRIORITY};

        std::vector<FocusManager::ChannelConfiguration> channelConfigurations{
            dialogChannelConfig, contentChannelConfig, testChannelConfig};

        m_focusManager = std::make_shared<FocusManager>(channelConfigurations);

        m_testContentClient = std::make_shared<TestClient>();
        ASSERT_TRUE(m_focusManager->acquireChannel(TEST_CHANNEL_NAME, m_testContentClient, TEST_ACTIVITY_ID));
        bool focusChanged;
        FocusState state;
        state = m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged);
        ASSERT_TRUE(focusChanged);
        ASSERT_EQ(state, FocusState::FOREGROUND);

        m_playbackController = PlaybackController::create(m_context->getContextManager(), m_avsConnectionManager);
        m_playbackRouter = PlaybackRouter::create(m_playbackController);

#ifdef GSTREAMER_MEDIA_PLAYER
        m_speakMediaPlayer =
            MediaPlayer::create(std::make_shared<avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory>());
#else
        m_speakMediaPlayer = std::make_shared<TestMediaPlayer>();
#endif

        m_compatibleAudioFormat.sampleRateHz = COMPATIBLE_SAMPLE_RATE;
        m_compatibleAudioFormat.sampleSizeInBits = COMPATIBLE_SAMPLE_SIZE_IN_BITS;
        m_compatibleAudioFormat.numChannels = COMPATIBLE_NUM_CHANNELS;
        m_compatibleAudioFormat.endianness = COMPATIBLE_ENDIANNESS;
        m_compatibleAudioFormat.encoding = COMPATIBLE_ENCODING;

        size_t nWords = 1024 * 1024;
        size_t wordSize = 2;
        size_t maxReaders = 3;
        size_t bufferSize = AudioInputStream::calculateBufferSize(nWords, wordSize, maxReaders);

        auto m_Buffer = std::make_shared<avsCommon::avs::AudioInputStream::Buffer>(bufferSize);
        auto m_Sds = avsCommon::avs::AudioInputStream::create(m_Buffer, wordSize, maxReaders);
        ASSERT_NE(nullptr, m_Sds);
        m_AudioBuffer = std::move(m_Sds);
        m_AudioBufferWriter =
            m_AudioBuffer->createWriter(avsCommon::avs::AudioInputStream::Writer::Policy::NONBLOCKABLE);
        ASSERT_NE(nullptr, m_AudioBufferWriter);

        // Set up hold to talk button.
        bool alwaysReadable = true;
        bool canOverride = true;
        bool canBeOverridden = true;
        m_HoldToTalkAudioProvider = std::make_shared<AudioProvider>(
            m_AudioBuffer,
            m_compatibleAudioFormat,
            ASRProfile::CLOSE_TALK,
            !alwaysReadable,
            canOverride,
            !canBeOverridden);

        m_holdToTalkButton = std::make_shared<holdToTalkButton>();

        m_userInactivityMonitor = UserInactivityMonitor::create(m_avsConnectionManager, m_exceptionEncounteredSender);
        m_AudioInputProcessor = AudioInputProcessor::create(
            m_directiveSequencer,
            m_avsConnectionManager,
            m_context->getContextManager(),
            m_focusManager,
            m_dialogUXStateAggregator,
            m_exceptionEncounteredSender,
            m_userInactivityMonitor);
        ASSERT_NE(nullptr, m_AudioInputProcessor);
        m_AudioInputProcessor->addObserver(m_dialogUXStateAggregator);

        // Create and register the SpeechSynthesizer.
        m_speechSynthesizer = SpeechSynthesizer::create(
            m_speakMediaPlayer,
            m_avsConnectionManager,
            m_focusManager,
            m_context->getContextManager(),
            m_exceptionEncounteredSender,
            m_dialogUXStateAggregator);
        ASSERT_NE(nullptr, m_speechSynthesizer);
        m_directiveSequencer->addDirectiveHandler(m_speechSynthesizer);
        m_speechSynthesizerObserver = std::make_shared<TestSpeechSynthesizerObserver>();
        m_speechSynthesizer->addObserver(m_speechSynthesizerObserver);
        m_speechSynthesizer->addObserver(m_dialogUXStateAggregator);

#ifdef GSTREAMER_MEDIA_PLAYER
        m_contentMediaPlayer =
            MediaPlayer::create(std::make_shared<avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory>());
#else
        m_contentMediaPlayer = std::make_shared<TestMediaPlayer>();
#endif

        // Create and register the AudioPlayer.
        m_audioPlayer = capabilityAgents::audioPlayer::AudioPlayer::create(
            m_contentMediaPlayer,
            m_avsConnectionManager,
            m_focusManager,
            m_context->getContextManager(),
            m_exceptionEncounteredSender,
            m_playbackRouter);
        ASSERT_NE(nullptr, m_audioPlayer);
        m_directiveSequencer->addDirectiveHandler(m_audioPlayer);

        connect();
    }

    void TearDown() override {
        disconnect();
        // Note that these nullptr checks are needed to avoid segaults if @c SetUp() failed.
        if (m_AudioInputProcessor) {
            m_AudioInputProcessor->shutdown();
        }
        if (m_directiveSequencer) {
            m_directiveSequencer->shutdown();
        }
        if (m_speechSynthesizer) {
            m_speechSynthesizer->shutdown();
        }
        if (m_audioPlayer) {
            m_audioPlayer->shutdown();
        }
        if (m_avsConnectionManager) {
            m_avsConnectionManager->shutdown();
        }
#ifdef GSTREAMER_MEDIA_PLAYER
        if (m_speakMediaPlayer) {
            m_speakMediaPlayer->shutdown();
        }
        if (m_contentMediaPlayer) {
            m_contentMediaPlayer->shutdown();
        }
#endif
        if (m_userInactivityMonitor) {
            m_userInactivityMonitor->shutdown();
        }
        if (m_playbackController) {
            m_playbackController->shutdown();
        }
        if (m_playbackRouter) {
            m_playbackRouter->shutdown();
        }
        m_context.reset();
    }

    /**
     * Connect to AVS.
     */
    void connect() {
        m_avsConnectionManager->enable();
        m_context->waitForConnected();
    }

    /**
     * Disconnect from AVS.
     */
    void disconnect() {
        if (m_avsConnectionManager) {
            m_avsConnectionManager->disable();
            m_context->waitForDisconnected();
        }
    }

    std::string getSentEventName(TestMessageSender::SendParams sendParams) {
        std::string eventString;
        std::string eventHeader;
        std::string eventName;
        jsonUtils::retrieveValue(sendParams.request->getJsonContent(), JSON_MESSAGE_EVENT_KEY, &eventString);
        jsonUtils::retrieveValue(eventString, JSON_MESSAGE_HEADER_KEY, &eventHeader);
        jsonUtils::retrieveValue(eventHeader, JSON_MESSAGE_NAME_KEY, &eventName);
        return eventName;
    }

    bool checkSentEventName(TestMessageSender::SendParams sendParams, std::string expectedName) {
        if (TestMessageSender::SendParams::Type::SEND == sendParams.type) {
            std::string eventName;
            eventName = getSentEventName(sendParams);
            return eventName == expectedName;
        }
        return false;
    }

    std::vector<int16_t> readAudioFromFile(const std::string& fileName, bool* errorOccurred) {
        const int RIFF_HEADER_SIZE = 44;

        std::ifstream inputFile(fileName.c_str(), std::ifstream::binary);
        if (!inputFile.good()) {
            std::cout << "Couldn't open audio file!" << std::endl;
            if (errorOccurred) {
                *errorOccurred = true;
            }
            return {};
        }
        inputFile.seekg(0, std::ios::end);
        int fileLengthInBytes = inputFile.tellg();
        if (fileLengthInBytes <= RIFF_HEADER_SIZE) {
            std::cout << "File should be larger than 44 bytes, which is the size of the RIFF header" << std::endl;
            if (errorOccurred) {
                *errorOccurred = true;
            }
            return {};
        }

        inputFile.seekg(RIFF_HEADER_SIZE, std::ios::beg);

        int numSamples = (fileLengthInBytes - RIFF_HEADER_SIZE) / 2;

        std::vector<int16_t> retVal(numSamples, 0);

        inputFile.read((char*)&retVal[0], numSamples * 2);

        if (inputFile.gcount() != numSamples * 2) {
            std::cout << "Error reading audio file" << std::endl;
            if (errorOccurred) {
                *errorOccurred = true;
            }
            return {};
        }

        inputFile.close();
        if (errorOccurred) {
            *errorOccurred = false;
        }
        return retVal;
    }

    void sendAudioFileAsRecognize(std::string audioFile) {
        // Signal to the AIP to start recognizing.
        ASSERT_NE(nullptr, m_HoldToTalkAudioProvider);
        ASSERT_TRUE(m_holdToTalkButton->startRecognizing(m_AudioInputProcessor, m_HoldToTalkAudioProvider));

        bool error;
        std::string file = g_inputPath + audioFile;
        std::vector<int16_t> audioData = readAudioFromFile(file, &error);
        ASSERT_FALSE(error);
        ASSERT_FALSE(audioData.empty());
        m_AudioBufferWriter->write(audioData.data(), audioData.size());

        // Stop holding the button.
        ASSERT_TRUE(m_holdToTalkButton->stopRecognizing(m_AudioInputProcessor));
    }

    /// Context for running ACL based tests.
    std::unique_ptr<ACLTestContext> m_context;

    std::shared_ptr<TestMessageSender> m_avsConnectionManager;
    std::shared_ptr<TestExceptionEncounteredSender> m_exceptionEncounteredSender;
    std::shared_ptr<PlaybackController> m_playbackController;
    std::shared_ptr<PlaybackRouter> m_playbackRouter;
    std::shared_ptr<TestDirectiveHandler> m_directiveHandler;
    std::shared_ptr<DirectiveSequencerInterface> m_directiveSequencer;
    std::shared_ptr<MessageInterpreter> m_messageInterpreter;
    std::shared_ptr<FocusManager> m_focusManager;
    std::shared_ptr<TestClient> m_testContentClient;
    std::shared_ptr<SpeechSynthesizer> m_speechSynthesizer;
    std::shared_ptr<TestSpeechSynthesizerObserver> m_speechSynthesizerObserver;
    std::shared_ptr<holdToTalkButton> m_holdToTalkButton;
    std::shared_ptr<AudioProvider> m_HoldToTalkAudioProvider;
    avsCommon::utils::AudioFormat m_compatibleAudioFormat;
    std::unique_ptr<AudioInputStream::Writer> m_AudioBufferWriter;
    std::shared_ptr<AudioInputStream> m_AudioBuffer;
    std::shared_ptr<AudioInputProcessor> m_AudioInputProcessor;
    std::shared_ptr<UserInactivityMonitor> m_userInactivityMonitor;
    std::shared_ptr<capabilityAgents::audioPlayer::AudioPlayer> m_audioPlayer;

    FocusState m_focusState;
    std::mutex m_mutex;
    std::condition_variable m_focusChangedCV;
    bool m_focusChangeOccurred;
    std::shared_ptr<avsCommon::avs::DialogUXStateAggregator> m_dialogUXStateAggregator;

#ifdef GSTREAMER_MEDIA_PLAYER
    std::shared_ptr<MediaPlayer> m_speakMediaPlayer;
    std::shared_ptr<MediaPlayer> m_contentMediaPlayer;
#else
    std::shared_ptr<TestMediaPlayer> m_speakMediaPlayer;
    std::shared_ptr<TestMediaPlayer> m_contentMediaPlayer;
#endif
};

/**
 * Test ability for the AudioPlayer to handle one play directive.
 *
 * This test is intended to test the AudioPlayer's ability to handle a short play directive all the way through. To do
 * this, an audio file of "Sing me a song" is sent as a Recognize event. In response, a Play directive is received. The
 * tests then observe that the correct events are sent in order.
 *
 */
TEST_F(AudioPlayerTest, SingASong) {
    // Sing me a song.
    sendAudioFileAsRecognize(RECOGNIZE_SING_FILE_NAME);
    bool focusChanged;
    FocusState state;
    state = m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged);
    ASSERT_TRUE(focusChanged);
    ASSERT_EQ(state, FocusState::BACKGROUND);

    // Recognize.
    TestMessageSender::SendParams sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_RECOGNIZE));

    bool playbackFinishedFound = false;
    bool playbackNearlyFinishedFound = false;
    bool playbackStartedFound = false;

    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    while (TestMessageSender::SendParams::Type::TIMEOUT != sendParams.type && !playbackFinishedFound) {
        if (checkSentEventName(sendParams, NAME_PLAYBACK_STARTED)) {
            playbackStartedFound = true;
            sendParams = m_avsConnectionManager->waitForNext(SONG_TIMEOUT_DURATION);
        } else if (checkSentEventName(sendParams, NAME_PLAYBACK_NEARLY_FINISHED)) {
            playbackNearlyFinishedFound = true;
            sendParams = m_avsConnectionManager->waitForNext(SONG_TIMEOUT_DURATION);
        } else if (checkSentEventName(sendParams, NAME_PLAYBACK_FINISHED)) {
            playbackFinishedFound = true;
        } else {
            sendParams = m_avsConnectionManager->waitForNext(SONG_TIMEOUT_DURATION);
        }
    }
    ASSERT_TRUE(playbackStartedFound);
    ASSERT_TRUE(playbackNearlyFinishedFound);
    ASSERT_TRUE(playbackFinishedFound);

    state = m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged);
    ASSERT_TRUE(focusChanged);
    ASSERT_EQ(state, FocusState::FOREGROUND);

    m_testContentClient->waitForFocusChange(NO_TIMEOUT_DURATION, &focusChanged);
    ASSERT_FALSE(focusChanged);
}

/**
 * Test ability for the AudioPlayer to handle multiple play directives.
 *
 * This test is intended to test the AudioPlayer's ability to handle a group play directive all the way through. To do
 * this, an audio file of "Flashbriefing" is sent as a Recognize event. In response, a Speak, then an undefined number
 * of Play directives, and a final Speak directive is received. The tests then observe that the correct events are sent
 * in order.
 *
 */
TEST_F(AudioPlayerTest, FlashBriefing) {
    // Ask for a flashbriefing.
    sendAudioFileAsRecognize(RECOGNIZE_FLASHBRIEFING_FILE_NAME);

    // Recognize event is sent.
    TestMessageSender::SendParams sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_RECOGNIZE));

    // Speech is handled.
    TestMessageSender::SendParams sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));
    TestMessageSender::SendParams sendFinishedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendFinishedParams, NAME_SPEECH_FINISHED));

    // If no items are in flashbriefing, this section will be skipped. Ensure that at least two items are selected in
    // the Alexa app under Settings -> Flashbriefing.
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    bool hasFlashbriefingItems = false;
    while (TestMessageSender::SendParams::Type::TIMEOUT != sendParams.type &&
           !checkSentEventName(sendParams, NAME_SPEECH_STARTED) &&
           !checkSentEventName(sendParams, NAME_PLAYBACK_STOPPED)) {
        hasFlashbriefingItems = true;
        bool playbackFinishedFound = false;
        bool playbackNearlyFinishedFound = false;
        bool playbackStartedFound = false;

        while (TestMessageSender::SendParams::Type::TIMEOUT != sendParams.type && !playbackFinishedFound) {
            if (checkSentEventName(sendParams, NAME_PLAYBACK_STARTED)) {
                playbackStartedFound = true;
                sendParams = m_avsConnectionManager->waitForNext(SONG_TIMEOUT_DURATION);
            } else if (checkSentEventName(sendParams, NAME_PLAYBACK_NEARLY_FINISHED)) {
                playbackNearlyFinishedFound = true;
                sendParams = m_avsConnectionManager->waitForNext(SONG_TIMEOUT_DURATION);
            } else if (checkSentEventName(sendParams, NAME_PLAYBACK_FINISHED)) {
                playbackFinishedFound = true;
                sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
            } else {
                sendParams = m_avsConnectionManager->waitForNext(SONG_TIMEOUT_DURATION);
            }
        }
        ASSERT_TRUE(playbackStartedFound);
        ASSERT_TRUE(playbackNearlyFinishedFound);
        ASSERT_TRUE(playbackFinishedFound);
    }

    if (hasFlashbriefingItems) {
        // The last speak is then allowed.
        EXPECT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));
        sendFinishedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        EXPECT_TRUE(checkSentEventName(sendFinishedParams, NAME_SPEECH_FINISHED));
    }
}

}  // namespace test
}  // namespace integration
}  // namespace alexaClientSDK

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    if (argc < 3) {
        std::cerr << "USAGE: " << std::string(argv[0]) << " <path_to_AlexaClientSDKConfig.json> <path_to_inputs_folder>"
                  << std::endl;
        return 1;

    } else {
        alexaClientSDK::integration::test::g_configPath = std::string(argv[1]);
        alexaClientSDK::integration::test::g_inputPath = std::string(argv[2]);
        return RUN_ALL_TESTS();
    }
}
