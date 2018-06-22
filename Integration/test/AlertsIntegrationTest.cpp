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

/// @file AlertsIntegrationTest.cpp
#include <gtest/gtest.h>
#include <string>
#include <future>
#include <fstream>
#include <chrono>
#include <deque>
#include <mutex>
#include <unordered_map>
#include <iostream>

#include "ACL/Transport/HTTP2MessageRouter.h"
#include "ACL/Transport/PostConnectSynchronizer.h"
#include "ADSL/DirectiveSequencer.h"
#include "ADSL/MessageInterpreter.h"
#include "AFML/FocusManager.h"
#include "AIP/AudioInputProcessor.h"
#include "AIP/AudioProvider.h"
#include "AIP/Initiator.h"
#include "AVSCommon/AVS/Attachment/AttachmentManager.h"
#include "AVSCommon/AVS/Attachment/InProcessAttachmentReader.h"
#include "AVSCommon/AVS/Attachment/InProcessAttachmentWriter.h"
#include "AVSCommon/AVS/BlockingPolicy.h"
#include "AVSCommon/AVS/Initialization/AlexaClientSDKInit.h"
#include "AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h"
#include "AVSCommon/SDKInterfaces/DirectiveHandlerResultInterface.h"
#include "AVSCommon/Utils/JSON/JSONUtils.h"
#include "AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h"
#include "AVSCommon/Utils/Logger/LogEntry.h"
#include "Alerts/AlertObserverInterface.h"
#include "Alerts/AlertsCapabilityAgent.h"
#include "Alerts/Storage/SQLiteAlertStorage.h"
#include "Audio/AlertsAudioFactory.h"
#include "AuthDelegate/AuthDelegate.h"
#include "CertifiedSender/CertifiedSender.h"
#include "CertifiedSender/SQLiteMessageStorage.h"
#include "ContextManager/ContextManager.h"
#include "Integration/AuthObserver.h"
#include "Integration/ClientMessageHandler.h"
#include "Integration/ConnectionStatusObserver.h"
#include "Integration/ObservableMessageRequest.h"
#include "Integration/TestAlertObserver.h"
#include "Integration/TestDirectiveHandler.h"
#include "Integration/TestExceptionEncounteredSender.h"
#include "Integration/TestMessageSender.h"
#include "Integration/TestSpeechSynthesizerObserver.h"
#include "RegistrationManager/CustomerDataManager.h"
#include "SpeechSynthesizer/SpeechSynthesizer.h"
#include "System/UserInactivityMonitor.h"

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
using namespace authDelegate;
using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::avs::initialization;
using namespace avsCommon::utils::mediaPlayer;
using namespace certifiedSender;
using namespace contextManager;
using namespace sdkInterfaces;
using namespace avsCommon::utils::sds;
using namespace avsCommon::utils::json;
using namespace capabilityAgents::aip;
using namespace afml;
using namespace capabilityAgents::speechSynthesizer;
using namespace capabilityAgents::system;
#ifdef GSTREAMER_MEDIA_PLAYER
using namespace mediaPlayer;
#endif

// This is a 16 bit 16 kHz little endian linear PCM audio file of "Joke" to be recognized.
static const std::string RECOGNIZE_JOKE_AUDIO_FILE_NAME = "/recognize_joke_test.wav";
// This is a 16 bit 16 kHz little endian linear PCM audio file of "What's up" to be recognized.
static const std::string RECOGNIZE_WHATS_UP_AUDIO_FILE_NAME = "/recognize_whats_up_test.wav";
// This is a 16 bit 16 kHz little endian linear PCM audio file of "What's up" to be recognized.
static const std::string RECOGNIZE_WEATHER_AUDIO_FILE_NAME = "/recognize_weather_test.wav";
// This is a 16 bit 16 kHz little endian linear PCM audio file of "Set a timer for 5 seconds" to be recognized.
static const std::string RECOGNIZE_TIMER_AUDIO_FILE_NAME = "/recognize_timer_test.wav";
// This is a 16 bit 16 kHz little endian linear PCM audio file of "Set a timer for 10 seconds" to be recognized.
static const std::string RECOGNIZE_LONG_TIMER_AUDIO_FILE_NAME = "/recognize_long_timer_test.wav";
// This is a 16 bit 16 kHz little endian linear PCM audio file of "Set a timer for 15 seconds" to be recognized.
static const std::string RECOGNIZE_VERY_LONG_TIMER_AUDIO_FILE_NAME = "/recognize_very_long_timer_test.wav";
// This is a 16 bit 16 kHz little endian linear PCM audio file of "Stop" to be recognized.
static const std::string RECOGNIZE_STOP_AUDIO_FILE_NAME = "/recognize_stop_timer_test.wav";
// This is a 16 bit 16 kHz little endian linear PCM audio file of "Cancel the timer" to be recognized.
static const std::string RECOGNIZE_CANCEL_TIMER_AUDIO_FILE_NAME = "/recognize_cancel_timer_test.wav";
// This string to be used for Speak Directives which use the NAMESPACE_SPEECH_SYNTHESIZER namespace.
static const std::string NAME_RECOGNIZE = "Recognize";
// This string to be used for SetAlertFailed Directives which use the NAMESPACE_ALERTS namespace.
static const std::string NAME_SET_ALERT_FAILED = "SetAlertFailed";
// This string to be used for AlertStopped Directives which use the NAMESPACE_ALERTS namespace.
static const std::string NAME_ALERT_STOPPED = "AlertStopped";
// This string to be used for AlertEnteredBackground Directives which use the NAMESPACE_ALERTS namespace.
static const std::string NAME_ALERT_ENTERED_BACKGROUND = "AlertEnteredBackground";
// This string to be used for AlertEnteredForeground Directives which use the NAMESPACE_ALERTS namespace.
static const std::string NAME_ALERT_ENTERED_FOREGROUND = "AlertEnteredForeground";
// This string to be used for DeleteAlertSucceeded Directives which use the NAMESPACE_ALERTS namespace.
static const std::string NAME_DELETE_ALERT_SUCCEEDED = "DeleteAlertSucceeded";
// This string to be used for DeleteAlertFailed Directives which use the NAMESPACE_ALERTS namespace.
static const std::string NAME_DELETE_ALERT_FAILED = "DeleteAlertFailed";
// This string to be used for AlertStarted Directives which use the NAMESPACE_ALERTS namespace.
static const std::string NAME_ALERT_STARTED = "AlertStarted";
// This string to be used for SpeechStarted Directives which use the NAMESPACE_SPEECH_SYNTHESIZER namespace.
static const std::string NAME_SPEECH_STARTED = "SpeechStarted";
// This string to be used for SpeechFinished Directives which use the NAMESPACE_SPEECH_SYNTHESIZER namespace.
static const std::string NAME_SPEECH_FINISHED = "SpeechFinished";
// This string to be used for AlertStarted Directives which use the NAMESPACE_ALERTS namespace.
static const std::string NAME_SET_ALERT_SUCCEEDED = "SetAlertSucceeded";
// Sample dialog activity id.
static const std::string DIALOG_ACTIVITY_ID = "Dialog";
// Sample content activity id.
static const std::string CONTENT_ACTIVITY_ID = "Content";
/// Sample alerts activity id.
static const std::string ALERTS_ACTIVITY_ID = "Alerts";
// This Integer to be used to specify a timeout in seconds.
static const std::chrono::seconds WAIT_FOR_TIMEOUT_DURATION(25);
// This Integer to be used to specify a short timeout in seconds.
static const std::chrono::seconds SHORT_TIMEOUT_DURATION(5);
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
static const std::string TAG("AlertsIntegrationTest");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::string configPath;
std::string inputPath;

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
     * @return Returns @c true if the callback occured within the timeout period and @c false otherwise.
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

class AlertsTest : public ::testing::Test {
protected:
    virtual void SetUp() override {
        std::ifstream infile(configPath);
        ASSERT_TRUE(infile.good());
        ASSERT_TRUE(AlexaClientSDKInit::initialize({&infile}));
        m_authObserver = std::make_shared<AuthObserver>();
        m_authDelegate = AuthDelegate::create();
        m_authDelegate->addAuthObserver(m_authObserver);
        m_attachmentManager = std::make_shared<avsCommon::avs::attachment::AttachmentManager>(
            AttachmentManager::AttachmentType::IN_PROCESS);
        m_connectionStatusObserver = std::make_shared<ConnectionStatusObserver>();
        bool isEnabled = false;
        m_messageRouter = std::make_shared<HTTP2MessageRouter>(m_authDelegate, m_attachmentManager);
        m_exceptionEncounteredSender = std::make_shared<TestExceptionEncounteredSender>();
        m_dialogUXStateAggregator = std::make_shared<avsCommon::avs::DialogUXStateAggregator>();

        m_directiveSequencer = DirectiveSequencer::create(m_exceptionEncounteredSender);
        m_messageInterpreter = std::make_shared<MessageInterpreter>(
            m_exceptionEncounteredSender, m_directiveSequencer, m_attachmentManager);

        // Set up connection and connect
        m_avsConnectionManager = std::make_shared<TestMessageSender>(
            m_messageRouter, isEnabled, m_connectionStatusObserver, m_messageInterpreter);
        ASSERT_NE(nullptr, m_avsConnectionManager);

        m_focusManager = std::make_shared<FocusManager>(FocusManager::DEFAULT_AUDIO_CHANNELS);
        m_testContentClient = std::make_shared<TestClient>();
        ASSERT_TRUE(m_focusManager->acquireChannel(
            FocusManager::CONTENT_CHANNEL_NAME, m_testContentClient, CONTENT_ACTIVITY_ID));
        bool focusChanged = false;
        ASSERT_EQ(
            m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::FOREGROUND);
        ASSERT_TRUE(focusChanged);

        m_testDialogClient = std::make_shared<TestClient>();

        m_contextManager = ContextManager::create();
        ASSERT_NE(nullptr, m_contextManager);
        PostConnectObject::init(m_contextManager);

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
            m_contextManager,
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
            m_contextManager,
            m_exceptionEncounteredSender,
            m_dialogUXStateAggregator);
        ASSERT_NE(nullptr, m_speechSynthesizer);
        m_directiveSequencer->addDirectiveHandler(m_speechSynthesizer);
        m_speechSynthesizerObserver = std::make_shared<TestSpeechSynthesizerObserver>();
        m_speechSynthesizer->addObserver(m_speechSynthesizerObserver);
        m_speechSynthesizer->addObserver(m_dialogUXStateAggregator);

#ifdef GSTREAMER_MEDIA_PLAYER
        m_rendererMediaPlayer = MediaPlayer::create(nullptr);
#else
        m_rendererMediaPlayer = std::make_shared<TestMediaPlayer>();
#endif
        m_alertRenderer = renderer::Renderer::create(m_rendererMediaPlayer);

        auto alertsAudioFactory = std::make_shared<applicationUtilities::resources::audio::AlertsAudioFactory>();

        m_alertStorage = capabilityAgents::alerts::storage::SQLiteAlertStorage::create(
            avsCommon::utils::configuration::ConfigurationNode::getRoot(), alertsAudioFactory);

        m_alertObserver = std::make_shared<TestAlertObserver>();

        auto messageStorage =
            SQLiteMessageStorage::create(avsCommon::utils::configuration::ConfigurationNode::getRoot());

        m_customerDataManager = std::make_shared<registrationManager::CustomerDataManager>();

        m_certifiedSender = CertifiedSender::create(
            m_avsConnectionManager,
            m_avsConnectionManager->getConnectionManager(),
            std::move(messageStorage),
            m_customerDataManager);

        m_alertsAgent = AlertsCapabilityAgent::create(
            m_avsConnectionManager,
            m_certifiedSender,
            m_focusManager,
            m_contextManager,
            m_exceptionEncounteredSender,
            m_alertStorage,
            alertsAudioFactory,
            m_alertRenderer,
            m_customerDataManager);
        ASSERT_NE(m_alertsAgent, nullptr);
        m_alertsAgent->addObserver(m_alertObserver);
        m_alertsAgent->onLocalStop();
        m_alertsAgent->removeAllAlerts();
        m_directiveSequencer->addDirectiveHandler(m_alertsAgent);

        m_avsConnectionManager->addConnectionStatusObserver(m_alertsAgent);

        connect();
    }

    void TearDown() override {
        disconnect();
        m_AudioInputProcessor->shutdown();
        m_directiveSequencer->shutdown();
        m_speechSynthesizer->shutdown();
        if (m_alertsAgent) {
            m_alertsAgent->onLocalStop();
            m_alertsAgent->removeAllAlerts();
            m_alertsAgent->shutdown();
        }
        m_certifiedSender->shutdown();
        m_avsConnectionManager->shutdown();
        AlexaClientSDKInit::uninitialize();
    }

    /**
     * Connect to AVS.
     */
    void connect() {
        ASSERT_TRUE(m_authObserver->waitFor(AuthObserver::State::REFRESHED)) << "Retrieving the auth token timed out.";
        m_avsConnectionManager->enable();
        ASSERT_TRUE(m_connectionStatusObserver->waitFor(ConnectionStatusObserverInterface::Status::CONNECTED))
            << "Connecting timed out.";
    }

    /**
     * Disconnect from AVS.
     */
    void disconnect() {
        m_avsConnectionManager->disable();
        ASSERT_TRUE(m_connectionStatusObserver->waitFor(ConnectionStatusObserverInterface::Status::DISCONNECTED))
            << "Connecting timed out.";
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

        // Put audio onto the SDS saying "Tell me a joke".
        bool error = false;
        std::string file = inputPath + audioFile;
        std::vector<int16_t> audioData = readAudioFromFile(file, &error);
        ASSERT_FALSE(error);
        ASSERT_FALSE(audioData.empty());
        m_AudioBufferWriter->write(audioData.data(), audioData.size());

        // Stop holding the button.
        ASSERT_TRUE(m_holdToTalkButton->stopRecognizing(m_AudioInputProcessor));
    }

    std::shared_ptr<AuthObserver> m_authObserver;
    std::shared_ptr<AuthDelegate> m_authDelegate;
    std::shared_ptr<ConnectionStatusObserver> m_connectionStatusObserver;
    std::shared_ptr<MessageRouter> m_messageRouter;
    std::shared_ptr<TestMessageSender> m_avsConnectionManager;
    std::shared_ptr<CertifiedSender> m_certifiedSender;
    std::shared_ptr<TestExceptionEncounteredSender> m_exceptionEncounteredSender;
    std::shared_ptr<TestDirectiveHandler> m_directiveHandler;
    std::shared_ptr<DirectiveSequencerInterface> m_directiveSequencer;
    std::shared_ptr<MessageInterpreter> m_messageInterpreter;
    std::shared_ptr<ContextManager> m_contextManager;
    std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> m_attachmentManager;
    std::shared_ptr<FocusManager> m_focusManager;
    std::shared_ptr<TestClient> m_testContentClient;
    std::shared_ptr<TestClient> m_testDialogClient;
    std::shared_ptr<TestAlertObserver> m_AlertsAgentObserver;
    std::shared_ptr<SpeechSynthesizer> m_speechSynthesizer;
    std::shared_ptr<AlertsCapabilityAgent> m_alertsAgent;
    std::shared_ptr<TestSpeechSynthesizerObserver> m_speechSynthesizerObserver;
    std::shared_ptr<capabilityAgents::alerts::storage::SQLiteAlertStorage> m_alertStorage;
    std::shared_ptr<renderer::RendererInterface> m_alertRenderer;
    std::shared_ptr<TestAlertObserver> m_alertObserver;
    std::shared_ptr<holdToTalkButton> m_holdToTalkButton;
    std::shared_ptr<AudioProvider> m_HoldToTalkAudioProvider;
    avsCommon::utils::AudioFormat m_compatibleAudioFormat;
    std::unique_ptr<AudioInputStream::Writer> m_AudioBufferWriter;
    std::shared_ptr<AudioInputStream> m_AudioBuffer;
    std::shared_ptr<AudioInputProcessor> m_AudioInputProcessor;
    std::shared_ptr<UserInactivityMonitor> m_userInactivityMonitor;
    std::shared_ptr<registrationManager::CustomerDataManager> m_customerDataManager;

    FocusState m_focusState;
    std::mutex m_mutex;
    std::condition_variable m_focusChangedCV;
    bool m_focusChangeOccurred;
    std::shared_ptr<avsCommon::avs::DialogUXStateAggregator> m_dialogUXStateAggregator;

#ifdef GSTREAMER_MEDIA_PLAYER
    std::shared_ptr<MediaPlayer> m_speakMediaPlayer;
    std::shared_ptr<MediaPlayer> m_rendererMediaPlayer;
#else
    std::shared_ptr<TestMediaPlayer> m_speakMediaPlayer;
    std::shared_ptr<TestMediaPlayer> m_rendererMediaPlayer;
#endif
};

/**
 * Test when one timer is stopped locally
 *
 * Set a 5 second timer, ensure it goes off, then use local stop and make sure the timer is stopped.
 */
TEST_F(AlertsTest, handleOneTimerWithLocalStop) {
    // Write audio to SDS saying "Set a timer for 5 seconds"
    sendAudioFileAsRecognize(RECOGNIZE_TIMER_AUDIO_FILE_NAME);
    TestMessageSender::SendParams sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_RECOGNIZE));

    // Speech is handled.
    TestMessageSender::SendParams sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));
    TestMessageSender::SendParams sendFinishedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendFinishedParams, NAME_SPEECH_FINISHED));

    // SetAlertSucceeded Event is sent
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_SET_ALERT_SUCCEEDED));

    ASSERT_EQ(m_alertObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION).state, AlertObserverInterface::State::READY);

    // AlertStarted Event is sent.
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_ALERT_STARTED));

    ASSERT_EQ(m_alertObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION).state, AlertObserverInterface::State::STARTED);

    // The test channel client has been notified the content channel has been backgrounded.
    bool focusChanged = false;
    ASSERT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::BACKGROUND);
    ASSERT_TRUE(focusChanged);

    // Locally stop the alarm.
    m_alertsAgent->onLocalStop();

    ASSERT_EQ(m_alertObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION).state, AlertObserverInterface::State::STOPPED);

    // AlertStopped Event is sent.
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_ALERT_STOPPED));

    // Low priority Test client gets back permission to the test channel
    EXPECT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::FOREGROUND);
    EXPECT_TRUE(focusChanged);
}

/**
 * Test when one timer is stopped verbally
 *
 * Set two second timer, ensure they go off, then stop both timers.
 */
TEST_F(AlertsTest, handleMultipleTimersWithLocalStop) {
    // Write audio to SDS saying "Set a timer for 15 seconds".
    sendAudioFileAsRecognize(RECOGNIZE_VERY_LONG_TIMER_AUDIO_FILE_NAME);
    TestMessageSender::SendParams sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_RECOGNIZE));

    // The test channel client has been notified the content channel has been backgrounded.
    bool focusChanged = false;
    ASSERT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::BACKGROUND);
    ASSERT_TRUE(focusChanged);

    // Speech is handled.
    TestMessageSender::SendParams sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));
    TestMessageSender::SendParams sendFinishedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendFinishedParams, NAME_SPEECH_FINISHED));

    ASSERT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::FOREGROUND);
    ASSERT_TRUE(focusChanged);

    // Write audio to SDS saying "Set a timer for 5 seconds".
    sendAudioFileAsRecognize(RECOGNIZE_TIMER_AUDIO_FILE_NAME);
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_RECOGNIZE));

    ASSERT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::BACKGROUND);
    ASSERT_TRUE(focusChanged);

    ASSERT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::FOREGROUND);
    ASSERT_TRUE(focusChanged);

    // SetAlertSucceeded Event is sent
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_SET_ALERT_SUCCEEDED));

    // Speech is handled.
    sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));
    sendFinishedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendFinishedParams, NAME_SPEECH_FINISHED));

    // SetAlertSucceeded Event is sent.
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_SET_ALERT_SUCCEEDED));

    // AlertStarted Event is sent.
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_ALERT_STARTED));

    // The test channel client has been notified the content channel has been backgrounded.
    ASSERT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::BACKGROUND);
    ASSERT_TRUE(focusChanged);

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // Locally stop the alarm.
    m_focusManager->stopForegroundActivity();

    // AlertStopped Event is sent.
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_ALERT_STOPPED));

    // AlertStarted Event is sent.
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_ALERT_STARTED));

    // The test channel client has been notified the content channel has been backgrounded.
    ASSERT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::BACKGROUND);
    ASSERT_TRUE(focusChanged);

    std::this_thread::sleep_for(std::chrono::milliseconds(600));

    // Locally stop the second alarm.
    m_focusManager->stopForegroundActivity();

    // AlertStopped Event is sent.
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_ALERT_STOPPED));

    // Low priority Test client gets back permission to the test channel.
    EXPECT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::FOREGROUND);
    EXPECT_TRUE(focusChanged);
}

/**
 * Test when the Alerts channel is acqired by a different client when an alert is active
 *
 * Set a 5 second timer, ensure it goes off, then have a test client acquire the Alerts channel. Ensure that the alert
 * is stopped.
 */
TEST_F(AlertsTest, stealChannelFromActiveAlert) {
    // Write audio to SDS saying "Set a timer for 5 seconds"
    sendAudioFileAsRecognize(RECOGNIZE_TIMER_AUDIO_FILE_NAME);
    TestMessageSender::SendParams sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_RECOGNIZE));

    // Speech is handled.
    TestMessageSender::SendParams sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));
    TestMessageSender::SendParams sendFinishedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendFinishedParams, NAME_SPEECH_FINISHED));

    // SetAlertSucceeded Event is sent
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_SET_ALERT_SUCCEEDED));

    // AlertStarted Event is sent.
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_ALERT_STARTED));
    ASSERT_EQ(m_alertObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION).state, AlertObserverInterface::State::READY);
    ASSERT_EQ(m_alertObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION).state, AlertObserverInterface::State::STARTED);

    // The test channel client has been notified the content channel has been backgrounded.
    bool focusChanged = false;
    ASSERT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::BACKGROUND);
    ASSERT_TRUE(focusChanged);

    // Steal the alerts channel.
    ASSERT_TRUE(
        m_focusManager->acquireChannel(FocusManager::ALERTS_CHANNEL_NAME, m_testDialogClient, ALERTS_ACTIVITY_ID));

    // AlertStopped Event is sent.
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_ALERT_STOPPED));

    ASSERT_EQ(m_alertObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION).state, AlertObserverInterface::State::STOPPED);

    // Release the alerts channel.
    m_focusManager->releaseChannel(FocusManager::ALERTS_CHANNEL_NAME, m_testDialogClient);

    // Low priority Test client gets back permission to the test channel.
    EXPECT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::FOREGROUND);
    EXPECT_TRUE(focusChanged);
}

/**
 * Test when a disconnect and reconnect happens while an alert is active
 *
 * Set a 5 second timer, then call disconnect, wait for the alert to become active and reconnect.
 * Locally stop the alert and ensure AlertStopped is sent.
 */
TEST_F(AlertsTest, DisconnectAndReconnectBeforeLocalStop) {
    // Write audio to SDS saying "Set a timer for 5 seconds"
    sendAudioFileAsRecognize(RECOGNIZE_TIMER_AUDIO_FILE_NAME);
    TestMessageSender::SendParams sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_RECOGNIZE));

    // Speech is handled.
    TestMessageSender::SendParams sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));
    TestMessageSender::SendParams sendFinishedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendFinishedParams, NAME_SPEECH_FINISHED));

    // SetAlertSucceeded Event is sent
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_SET_ALERT_SUCCEEDED));

    // allow time for the certified sender to send the message ok.
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    disconnect();

    // Wait for the alarm to go off.
    std::this_thread::sleep_for(std::chrono::milliseconds(6000));

    // The test channel client has been notified the content channel has been backgrounded.
    bool focusChanged = false;
    ASSERT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::BACKGROUND);
    ASSERT_TRUE(focusChanged);

    connect();

    // allow time for the other components to become aware of the connection status change.
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // Locally stop the alarm.
    m_alertsAgent->onLocalStop();

    // AlertStarted Event is sent.
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_ALERT_STARTED));
    // AlertStopped Event is sent.
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_ALERT_STOPPED));

    // Low priority Test client gets back permission to the test channel
    EXPECT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::FOREGROUND);
    EXPECT_TRUE(focusChanged);

    m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged);
    ASSERT_FALSE(focusChanged);
}

/**
 * Test when a disconnect and reconnect happens before  an alert is active
 *
 * Set a 5 second timer, then call disconnect then reconnect. Once the alert is active, locally stop the alert
 * and ensure AlertStopped is sent.
 */
TEST_F(AlertsTest, DisconnectAndReconnect) {
    // Write audio to SDS saying "Set a timer for 5 seconds"
    sendAudioFileAsRecognize(RECOGNIZE_TIMER_AUDIO_FILE_NAME);
    TestMessageSender::SendParams sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_RECOGNIZE));

    // Speech is handled.
    TestMessageSender::SendParams sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));
    TestMessageSender::SendParams sendFinishedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendFinishedParams, NAME_SPEECH_FINISHED));

    // SetAlertSucceeded Event is sent
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_SET_ALERT_SUCCEEDED));
    disconnect();

    // Wait for the alarm to go off.
    std::this_thread::sleep_for(std::chrono::milliseconds(6000));

    // The test channel client has been notified the content channel has been backgrounded.
    bool focusChanged = false;

    EXPECT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::BACKGROUND);
    EXPECT_TRUE(focusChanged);

    // Locally stop the alarm.
    m_alertsAgent->onLocalStop();

    connect();

    // AlertStopped Event is sent.
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_FALSE(checkSentEventName(sendParams, NAME_ALERT_STOPPED));

    // Low priority Test client gets back permission to the test channel
    EXPECT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::FOREGROUND);
    EXPECT_TRUE(focusChanged);
}

/**
 * Test when all alerts are cleared from storage before an alert is active
 *
 * Set a 5 second timer, then call removeAllAlerts. Wait and ensure that the alert does not become active and no
 * events are sent for it.
 */
TEST_F(AlertsTest, RemoveAllAlertsBeforeAlertIsActive) {
    // Write audio to SDS saying "Set a timer for 5 seconds"
    sendAudioFileAsRecognize(RECOGNIZE_TIMER_AUDIO_FILE_NAME);
    TestMessageSender::SendParams sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_RECOGNIZE));

    // Speech is handled.
    TestMessageSender::SendParams sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));
    TestMessageSender::SendParams sendFinishedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendFinishedParams, NAME_SPEECH_FINISHED));

    bool focusChanged = false;
    FocusState state;
    state = m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged);
    ASSERT_TRUE(focusChanged);
    ASSERT_EQ(state, FocusState::BACKGROUND);

    state = m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged);
    ASSERT_TRUE(focusChanged);
    ASSERT_EQ(state, FocusState::FOREGROUND);

    // SetAlertSucceeded Event is sent
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_SET_ALERT_SUCCEEDED));

    // Remove all alerts.
    m_alertsAgent->removeAllAlerts();

    // AlertStarted Event is not sent.
    sendParams = m_avsConnectionManager->waitForNext(SHORT_TIMEOUT_DURATION);
    ASSERT_FALSE(checkSentEventName(sendParams, NAME_ALERT_STARTED));

    // AlertStopped Event is not sent.
    sendParams = m_avsConnectionManager->waitForNext(SHORT_TIMEOUT_DURATION);
    ASSERT_FALSE(checkSentEventName(sendParams, NAME_ALERT_STOPPED));

    // Focus has not changed.
    focusChanged = false;
    state = m_testContentClient->waitForFocusChange(SHORT_TIMEOUT_DURATION, &focusChanged);
    ASSERT_FALSE(focusChanged);
    ASSERT_EQ(state, FocusState::FOREGROUND);
}

/**
 * Test when an alert is canceled before it is due
 *
 * Set a 10 second timer, then send audio of "Cancel the timer" as a recognize event. Ensure the timer does not go off
 * and the DeleteAlertSucceeded event is sent.
 */
TEST_F(AlertsTest, cancelAlertBeforeItIsActive) {
    // Write audio to SDS saying "Set a timer for 10 seconds"
    sendAudioFileAsRecognize(RECOGNIZE_LONG_TIMER_AUDIO_FILE_NAME);
    TestMessageSender::SendParams sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_RECOGNIZE));

    // Speech is handled.
    TestMessageSender::SendParams sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));
    TestMessageSender::SendParams sendFinishedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendFinishedParams, NAME_SPEECH_FINISHED));

    // SetAlertSucceeded Event is sent
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_SET_ALERT_SUCCEEDED));

    // Write audio to SDS sying "Cancel the timer"
    sendAudioFileAsRecognize(RECOGNIZE_CANCEL_TIMER_AUDIO_FILE_NAME);

    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_RECOGNIZE));

    // DeleteAlertSucceeded Event is sent.
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_DELETE_ALERT_SUCCEEDED));

    // Speech is handled.
    sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));
    sendFinishedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendFinishedParams, NAME_SPEECH_FINISHED));

    // Low priority Test client gets back permission to the test channel.
    bool focusChanged = false;
    ASSERT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::BACKGROUND);

    focusChanged = false;
    ASSERT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::FOREGROUND);

    // AlertStarted Event is not sent.
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_FALSE(checkSentEventName(sendParams, NAME_ALERT_STARTED));
}

/**
 * Test when the storage is removed before an alert is set
 *
 * Close the storage before asking for a 5 second timer. SetAlertFailed and DeleteAlertFailed events are then sent.
 */
TEST_F(AlertsTest, RemoveStorageBeforeAlarmIsSet) {
    m_alertStorage->close();

    // Write audio to SDS saying "Set a timer for 5 seconds"
    sendAudioFileAsRecognize(RECOGNIZE_LONG_TIMER_AUDIO_FILE_NAME);
    TestMessageSender::SendParams sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_RECOGNIZE));

    // Speech is handled.
    TestMessageSender::SendParams sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));
    TestMessageSender::SendParams sendFinishedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendFinishedParams, NAME_SPEECH_FINISHED));

    bool focusChanged = false;
    ASSERT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::BACKGROUND);
    ASSERT_TRUE(focusChanged);

    ASSERT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::FOREGROUND);
    ASSERT_TRUE(focusChanged);

    // SetAlertFailed Event is sent
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_SET_ALERT_FAILED));

    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    if (checkSentEventName(sendParams, NAME_SPEECH_STARTED)) {
        sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        if (checkSentEventName(sendParams, NAME_SPEECH_FINISHED)) {
            sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        }
    }
    // DeleteAlertFailed is sent.
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_DELETE_ALERT_FAILED));

    ASSERT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::BACKGROUND);
    ASSERT_TRUE(focusChanged);

    // Low priority Test client gets back permission to the test channel.
    ASSERT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::FOREGROUND);
    ASSERT_TRUE(focusChanged);
}

/**
 * Test when an alert is active and the user barges in and gets one speak in response
 *
 * Set a 5 second timer and wait until it is active. Send a recognize event asking for joke and see that the alert goes
 * into the background. When the speak is complete, the alert is forgrounded and can be locally stopped.
 */
TEST_F(AlertsTest, UserShortUnrelatedBargeInOnActiveTimer) {
    // Write audio to SDS saying "Set a timer for 5 seconds"
    sendAudioFileAsRecognize(RECOGNIZE_TIMER_AUDIO_FILE_NAME);
    TestMessageSender::SendParams sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_RECOGNIZE));

    // Speech is handled.
    TestMessageSender::SendParams sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));
    TestMessageSender::SendParams sendFinishedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendFinishedParams, NAME_SPEECH_FINISHED));

    // SetAlertSucceeded Event is sent
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_SET_ALERT_SUCCEEDED));

    // AlertStarted Event is sent.
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_ALERT_STARTED));

    // The test channel client has been notified the content channel has been backgrounded.
    bool focusChanged = false;
    ASSERT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::BACKGROUND);
    ASSERT_TRUE(focusChanged);

    // Write audio to SDS sying "Tell me a joke"
    sendAudioFileAsRecognize(RECOGNIZE_JOKE_AUDIO_FILE_NAME);
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);

    if (getSentEventName(sendParams) == NAME_ALERT_ENTERED_BACKGROUND) {
        sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    } else {
        ASSERT_TRUE(checkSentEventName(sendParams, NAME_RECOGNIZE));
        sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        ASSERT_TRUE(checkSentEventName(sendParams, NAME_ALERT_ENTERED_BACKGROUND));
    }

    // Speech is handled.
    sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));
    sendFinishedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendFinishedParams, NAME_SPEECH_FINISHED));

    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_ALERT_ENTERED_FOREGROUND));

    std::this_thread::sleep_for(std::chrono::milliseconds(600));

    // Locally stop the alarm.
    m_alertsAgent->onLocalStop();

    // AlertStopped Event is sent.
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    EXPECT_TRUE(checkSentEventName(sendParams, NAME_ALERT_STOPPED));

    // Low priority Test client gets back permission to the test channel
    EXPECT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::FOREGROUND);
    EXPECT_TRUE(focusChanged);
}

/**
 * Test when an alert is active and the user barges in and gets multiple speaks in response
 *
 * Set a 5 second timer and wait until it is active. Send a recognize event asking "what's up" and see that the alert
 * goes into the background. When all the speaks are complete, the alert is forgrounded and can be locally stopped.
 */
TEST_F(AlertsTest, DISABLED_UserLongUnrelatedBargeInOnActiveTimer) {
    // Write audio to SDS saying "Set a timer for 5 seconds"
    sendAudioFileAsRecognize(RECOGNIZE_TIMER_AUDIO_FILE_NAME);
    TestMessageSender::SendParams sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_RECOGNIZE));

    // Speech is handled.
    TestMessageSender::SendParams sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));
    TestMessageSender::SendParams sendFinishedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendFinishedParams, NAME_SPEECH_FINISHED));

    // SetAlertSucceeded Event is sent
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_SET_ALERT_SUCCEEDED));

    // Wait for the alarm to go off.
    std::this_thread::sleep_for(std::chrono::milliseconds(6000));

    // AlertStarted Event is sent.
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_ALERT_STARTED));

    // The test channel client has been notified the content channel has been backgrounded.
    bool focusChanged = false;
    ASSERT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::BACKGROUND);
    ASSERT_TRUE(focusChanged);

    // Write audio to SDS sying "What's up"
    sendAudioFileAsRecognize(RECOGNIZE_WHATS_UP_AUDIO_FILE_NAME);

    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    if (getSentEventName(sendParams) == NAME_ALERT_ENTERED_BACKGROUND) {
        sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        EXPECT_TRUE(checkSentEventName(sendParams, NAME_RECOGNIZE));
    } else {
        EXPECT_TRUE(checkSentEventName(sendParams, NAME_RECOGNIZE));
        sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        ASSERT_TRUE(checkSentEventName(sendParams, NAME_ALERT_ENTERED_BACKGROUND));
    }

    // Speech is handled.
    sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));

    sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_FINISHED));
    sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_ALERT_ENTERED_FOREGROUND));
    sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);

    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_ALERT_ENTERED_BACKGROUND));

    while (checkSentEventName(sendStartedParams, NAME_ALERT_ENTERED_BACKGROUND)) {
        sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));
        sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_FINISHED));
        sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_ALERT_ENTERED_FOREGROUND));
        sendStartedParams = m_avsConnectionManager->waitForNext(SHORT_TIMEOUT_DURATION);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(600));

    // Locally stop the alarm.
    m_alertsAgent->onLocalStop();

    sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_ALERT_STOPPED));

    // Low priority Test client gets back permission to the test channel
    EXPECT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::FOREGROUND);
    EXPECT_TRUE(focusChanged);
}

/**
 * Test when the user is speaking to Alexa and an alert becomes active
 *
 * Set a 5 second timer then start a recognize event using a hold to talk initiator but do not call stopCapture until
 * the alert has become active in the background. Once the alert is active, call stopCapture and see that is is in the
 * foreground before locally stopping it.
 */
TEST_F(AlertsTest, UserSpeakingWhenAlertShouldBeActive) {
    // Write audio to SDS saying "Set a timer for 10 seconds"
    sendAudioFileAsRecognize(RECOGNIZE_LONG_TIMER_AUDIO_FILE_NAME);
    TestMessageSender::SendParams sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_RECOGNIZE));

    // Speech is handled.
    TestMessageSender::SendParams sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));
    TestMessageSender::SendParams sendFinishedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendFinishedParams, NAME_SPEECH_FINISHED));

    // SetAlertSucceeded Event is sent.
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_SET_ALERT_SUCCEEDED));

    // Signal to the AIP to start recognizing.
    ASSERT_NE(nullptr, m_HoldToTalkAudioProvider);
    ASSERT_TRUE(m_holdToTalkButton->startRecognizing(m_AudioInputProcessor, m_HoldToTalkAudioProvider));

    // Put audio onto the SDS saying "Tell me a joke".
    bool error = false;
    std::string file = inputPath + RECOGNIZE_WEATHER_AUDIO_FILE_NAME;
    std::vector<int16_t> audioData = readAudioFromFile(file, &error);
    ASSERT_FALSE(error);
    ASSERT_FALSE(audioData.empty());
    m_AudioBufferWriter->write(audioData.data(), audioData.size());

    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_RECOGNIZE));

    // The test channel client has been notified the content channel has been backgrounded.
    bool focusChanged = false;
    ASSERT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::BACKGROUND);
    ASSERT_TRUE(focusChanged);

    // AlertStarted Event is sent.
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_ALERT_STARTED));

    // Stop holding the button.
    ASSERT_TRUE(m_holdToTalkButton->stopRecognizing(m_AudioInputProcessor));

    // Speech is handled.
    sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));

    sendFinishedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    if (getSentEventName(sendParams) == NAME_SPEECH_FINISHED) {
        sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        ASSERT_TRUE(checkSentEventName(sendParams, NAME_ALERT_ENTERED_FOREGROUND));
    } else {
        ASSERT_TRUE(checkSentEventName(sendFinishedParams, NAME_SPEECH_FINISHED));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(800));

    // Locally stop the alarm.
    m_alertsAgent->onLocalStop();

    // Low priority Test client gets back permission to the test channel
    ASSERT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::FOREGROUND);
    ASSERT_TRUE(focusChanged);
}

TEST_F(AlertsTest, handleOneTimerWithVocalStop) {
    // Write audio to SDS saying "Set a timer for 5 seconds"
    sendAudioFileAsRecognize(RECOGNIZE_TIMER_AUDIO_FILE_NAME);
    TestMessageSender::SendParams sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_RECOGNIZE));

    TestMessageSender::SendParams sendStartedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));

    // SpeechFinished is sent here.
    TestMessageSender::SendParams sendFinishedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendFinishedParams, NAME_SPEECH_FINISHED));

    // SetAlertSucceeded Event is sent
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_SET_ALERT_SUCCEEDED));

    // AlertStarted Event is sent.
    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_ALERT_STARTED));

    ASSERT_EQ(m_alertObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION).state, AlertObserverInterface::State::READY);
    ASSERT_EQ(m_alertObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION).state, AlertObserverInterface::State::STARTED);

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // The test channel client has been notified the content channel has been backgrounded.
    bool focusChanged = false;
    ASSERT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::BACKGROUND);
    ASSERT_TRUE(focusChanged);

    // Write audio to SDS sying "Stop"
    sendAudioFileAsRecognize(RECOGNIZE_STOP_AUDIO_FILE_NAME);

    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    if (getSentEventName(sendParams) == NAME_ALERT_ENTERED_BACKGROUND) {
        sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        EXPECT_TRUE(checkSentEventName(sendParams, NAME_RECOGNIZE));
    } else {
        EXPECT_TRUE(checkSentEventName(sendParams, NAME_RECOGNIZE));
        sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        ASSERT_TRUE(checkSentEventName(sendParams, NAME_ALERT_ENTERED_BACKGROUND));
    }

    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_DELETE_ALERT_SUCCEEDED));

    sendParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendParams, NAME_ALERT_STOPPED));

    ASSERT_EQ(
        m_alertObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION).state,
        AlertObserverInterface::State::FOCUS_ENTERED_BACKGROUND);
    ASSERT_EQ(m_alertObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION).state, AlertObserverInterface::State::STOPPED);

    // Low priority Test client gets back permission to the test channel
    EXPECT_EQ(
        m_testContentClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION, &focusChanged), FocusState::FOREGROUND);
    EXPECT_TRUE(focusChanged);
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
        alexaClientSDK::integration::test::configPath = std::string(argv[1]);
        alexaClientSDK::integration::test::inputPath = std::string(argv[2]);
        return RUN_ALL_TESTS();
    }
}
