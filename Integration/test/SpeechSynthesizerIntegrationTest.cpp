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

/// @file SpeechSynthesizerIntegrationTest.cpp
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
#include "ACL/Transport/PostConnectObject.h"
#include "ADSL/DirectiveSequencer.h"
#include "ADSL/MessageInterpreter.h"
#include "AFML/FocusManager.h"
#include "AuthDelegate/AuthDelegate.h"
#include "AVSCommon/AVS/Attachment/AttachmentManager.h"
#include "AVSCommon/AVS/Attachment/InProcessAttachmentReader.h"
#include "AVSCommon/AVS/Attachment/InProcessAttachmentWriter.h"
#include "AVSCommon/AVS/BlockingPolicy.h"
#include "AVSCommon/Utils/JSON/JSONUtils.h"
#include "AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h"
#include "AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h"
#include "AVSCommon/SDKInterfaces/DirectiveHandlerResultInterface.h"
#include "AVSCommon/AVS/Initialization/AlexaClientSDKInit.h"
#include "ContextManager/ContextManager.h"
#include "Integration/AuthObserver.h"
#include "Integration/ClientMessageHandler.h"
#include "Integration/ConnectionStatusObserver.h"
#include "Integration/ObservableMessageRequest.h"
#include "Integration/TestMessageSender.h"
#include "Integration/TestSpeechSynthesizerObserver.h"
#include "AVSCommon/Utils/Logger/LogEntry.h"
#include "SpeechSynthesizer/SpeechSynthesizer.h"
#include "Integration/TestDirectiveHandler.h"
#include "Integration/TestExceptionEncounteredSender.h"

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
using namespace contextManager;
using namespace capabilityAgents::speechSynthesizer;
using namespace sdkInterfaces;
using namespace avsCommon::utils::sds;
using namespace avsCommon::utils::json;
using namespace afml;
#ifdef GSTREAMER_MEDIA_PLAYER
using namespace mediaPlayer;
#endif

/**
 * This string specifies a Recognize event using the specified profile.
 *
 * CLOSE_TALK performs end-of-speech detection on the client, so no directive is sent from AVS to stop recording.
 * NEAR_FIELD performs end-of-speech detection in AVS, so a directive is sent from AVS to stop recording.
 */
// clang-format off
#define RECOGNIZE_EVENT_JSON(PROFILE, DIALOG_REQUEST_ID )         \
    "{"                                                           \
        "\"event\":{"                                             \
            "\"payload\":{"                                       \
                "\"format\":\"AUDIO_L16_RATE_16000_CHANNELS_1\"," \
                "\"profile\":\"" #PROFILE "\""                    \
            "},"                                                  \
            "\"header\":{"                                        \
                "\"dialogRequestId\":\"" DIALOG_REQUEST_ID "\","  \
                "\"messageId\":\"messageId123\","                 \
                "\"name\":\"Recognize\","                         \
                "\"namespace\":\"SpeechRecognizer\""              \
            "}"                                                   \
        "},"                                                      \
        "\"context\":[{"                                          \
            "\"payload\":{"                                       \
                "\"activeAlerts\":[],"                            \
                "\"allAlerts\":[]"                                \
            "},"                                                  \
            "\"header\":{"                                        \
                "\"name\":\"AlertsState\","                       \
                "\"namespace\":\"Alerts\""                        \
            "}"                                                   \
        "},"                                                      \
        "{"                                                       \
            "\"payload\":{"                                       \
                "\"playerActivity\":\"IDLE\","                    \
                "\"offsetInMilliseconds\":0,"                     \
                "\"token\":\"\""                                  \
            "},"                                                  \
            "\"header\":{"                                        \
                "\"name\":\"PlaybackState\","                     \
                "\"namespace\":\"AudioPlayer\""                   \
            "}"                                                   \
        "},"                                                      \
        "{"                                                       \
            "\"payload\":{"                                       \
                "\"muted\":false,"                                \
                "\"volume\":0"                                    \
            "},"                                                  \
            "\"header\":{"                                        \
                "\"name\":\"VolumeState\","                       \
                "\"namespace\":\"Speaker\""                       \
            "}"                                                   \
        "},"                                                      \
        "{"                                                       \
            "\"payload\":{"                                       \
                "\"playerActivity\":\"FINISHED\","                \
                "\"offsetInMilliseconds\":0,"                     \
                "\"token\":\"\""                                  \
            "},"                                                  \
            "\"header\":{"                                        \
                "\"name\":\"SpeechState\","                       \
                "\"namespace\":\"SpeechSynthesizer\""             \
            "}"                                                   \
        "}]"                                                      \
    "}"
// clang-format on

// This is a 16 bit 16 kHz little endian linear PCM audio file of "Joke" to be recognized.
static const std::string RECOGNIZE_JOKE_AUDIO_FILE_NAME = "/recognize_joke_test.wav";
// This is a 16 bit 16 kHz little endian linear PCM audio file of "Wikipedia" to be recognized.
static const std::string RECOGNIZE_WIKI_AUDIO_FILE_NAME = "/recognize_wiki_test.wav";
// This is a 16 bit 16 kHz little endian linear PCM audio file of "Lions" to be recognized.
static const std::string RECOGNIZE_LIONS_AUDIO_FILE_NAME = "/recognize_lions_test.wav";
// This is a 16 bit 16 kHz little endian linear PCM audio file of "Flashbriefing" to be recognized.
static const std::string RECOGNIZE_FLASHBRIEFING_AUDIO_FILE_NAME = "/recognize_flashbriefing_test.wav";
// This is a 16 bit 16 kHz little endian linear PCM audio file of "What's up" to be recognized.
static const std::string RECOGNIZE_WHATS_UP_AUDIO_FILE_NAME = "/recognize_whats_up_test.wav";
// This is a 16 bit 16 kHz little endian linear PCM audio file of "Volume up" to be recognized.
static const std::string RECOGNIZE_VOLUME_UP_AUDIO_FILE_NAME = "/recognize_volume_up_test.wav";
// String to be used as a basic DialogRequestID.
#define FIRST_DIALOG_REQUEST_ID "DialogRequestID123"
// String to be used as a DialogRequestID when the first has already been used.
#define SECOND_DIALOG_REQUEST_ID "DialogRequestID456"
// This string specifies a Recognize event using the CLOSE_TALK profile and uses the first DialogRequestID.
static const std::string CT_FIRST_RECOGNIZE_EVENT_JSON = RECOGNIZE_EVENT_JSON(CLOSE_TALK, FIRST_DIALOG_REQUEST_ID);
// This string specifies a Recognize event using the CLOSE_TALK profile and uses the first DialogRequestID.
static const std::string CT_FIRST_RECOGNIZE_EVENT_JSON_NEAR = RECOGNIZE_EVENT_JSON(NEAR_FIELD, FIRST_DIALOG_REQUEST_ID);
// This string specifies a Recognize event using the CLOSE_TALK profile and uses the second DialogRequestID.
static const std::string CT_SECOND_RECOGNIZE_EVENT_JSON = RECOGNIZE_EVENT_JSON(CLOSE_TALK, SECOND_DIALOG_REQUEST_ID);
// This string to be used for Speak Directives which use the NAMESPACE_SPEECH_SYNTHESIZER namespace.
static const std::string NAME_SPEAK = "Speak";
// This string to be used for Speak Directives which use the NAMESPACE_SPEECH_SYNTHESIZER namespace.
static const std::string NAME_RECOGNIZE = "Recognize";
// This string to be used for AdjustVolume Directives which use the NAMESPACE_SPEAKER namespace.
static const std::string NAME_ADJUST_VOLUME = "AdjustVolume";
// This string to be used for ExpectSpeech Directives which use the NAMESPACE_SPEECH_RECOGNIZER namespace.
static const std::string NAME_EXPECT_SPEECH = "ExpectSpeech";
// This string to be used for SetMute Directives which use the NAMESPACE_SPEAKER namespace.
static const std::string NAME_SET_MUTE = "SetMute";
// This string to be used for SpeechStarted Directives which use the NAMESPACE_SPEECH_SYNTHESIZER namespace.
static const std::string NAME_SPEECH_STARTED = "SpeechStarted";
// This string to be used for SpeechFinished Directives which use the NAMESPACE_SPEECH_SYNTHESIZER namespace.
static const std::string NAME_SPEECH_FINISHED = "SpeechFinished";
// This String to be used to register the SpeechRecognizer namespace to a DirectiveHandler.
static const std::string NAMESPACE_SPEECH_RECOGNIZER = "SpeechRecognizer";
// This String to be used to register the SpeechSynthesizer namespace to a DirectiveHandler.
static const std::string NAMESPACE_SPEECH_SYNTHESIZER = "SpeechSynthesizer";
// This String to be used to register the Speaker namespace to a DirectiveHandler.
static const std::string NAMESPACE_SPEAKER = "Speaker";
// This pair connects a Speak name and SpeechSynthesizer namespace for use in DirectiveHandler registration.
static const NamespaceAndName SPEAK_PAIR = {NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK};
// This pair connects a ExpectSpeech name and SpeechRecognizer namespace for use in DirectiveHandler registration.
static const NamespaceAndName EXPECT_SPEECH_PAIR = {NAMESPACE_SPEECH_RECOGNIZER, NAME_EXPECT_SPEECH};
// This pair connects a SetMute name and Speaker namespace for use in DirectiveHandler registration.
static const NamespaceAndName SET_MUTE_PAIR = {NAMESPACE_SPEAKER, NAME_SET_MUTE};
// This pair connects a SetMute name and Speaker namespace for use in DirectiveHandler registration.
static const NamespaceAndName ADJUST_VOLUME_PAIR = {NAMESPACE_SPEAKER, NAME_ADJUST_VOLUME};
// Sample dialog activity id.
static const std::string DIALOG_ACTIVITY_ID = "Dialog";
/// Sample alerts activity id.
static const std::string ALERTS_ACTIVITY_ID = "Alerts";
// This Integer to be used to specify a timeout in seconds for a directive to reach the DirectiveHandler.
static const std::chrono::seconds WAIT_FOR_TIMEOUT_DURATION(15);
// This Integer to be used to specify a timeout in seconds for AuthDelegate to wait for LWA response.
static const std::chrono::seconds SEND_EVENT_TIMEOUT_DURATION(20);
// This Integer to be used to specify a timeout in seconds for a directive to reach the DirectiveHandler.
static const std::chrono::seconds DIRECTIVE_TIMEOUT_DURATION(7);
// This Integer to be used when it is expected the duration will timeout.
static const std::chrono::seconds WANTING_TIMEOUT_DURATION(1);
// This Integer to be used to specify a timeout in seconds for the Media Player to finish playing.
static const std::chrono::seconds WAIT_FOR_MEDIA_PLAYER_TIMEOUT_DURATION(60);
// This Integer to be used to specify number of Speak Directives to validate in test handleMultipleConsecutiveSpeaks.
// Although we anticipate four Speak Directives, we validate only three Speak Directives.
// Validating three Speak Directives helps keep the test short.
static const unsigned int NUMBER_OF_SPEAK_DIRECTIVES_TO_VALIDATE = 3;

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
static const std::string TAG("SpeechSynthesizerIntegrationTest");

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
    TestClient() : m_focusState(FocusState::NONE) {
    }

    /**
     * Implementation of the ChannelObserverInterface##onFocusChanged() callback.
     *
     * @param focusState The new focus state of the Channel observer.
     */
    void onFocusChanged(FocusState focusState) override {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.push_back(focusState);
        m_focusState = focusState;
        m_wakeTrigger.notify_all();
    }

    /**
     * Waits for the ChannelObserverInterface##onFocusChanged() callback.
     *
     * @param timeout The amount of time to wait for the callback.
     * @param focusChanged An output parameter that notifies the caller whether a callback occurred.
     * @return Returns @c true if the callback occured within the timeout period and @c false otherwise.
     */
    FocusState waitForFocusChange(std::chrono::milliseconds timeout) {
        FocusState ret;
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_wakeTrigger.wait_for(lock, timeout, [this]() { return !m_queue.empty(); })) {
            ret = m_focusState;
            return ret;
        }
        ret = m_queue.front();
        m_queue.pop_front();
        return ret;
    }

    FocusState getCurrentFocusState() {
        return m_focusState;
    }

private:
    /// The focus state of the observer.
    FocusState m_focusState;

    /// A lock to guard against focus state changes.
    std::mutex m_mutex;

    /// Trigger to wake up waitForNext calls.
    std::condition_variable m_wakeTrigger;
    /// Queue of received focus states that have not been waited on.
    std::deque<FocusState> m_queue;
};

class SpeechSynthesizerTest : public ::testing::Test {
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
        m_clientMessageHandler = std::make_shared<ClientMessageHandler>(m_attachmentManager);
        bool isEnabled = false;
        m_messageRouter = std::make_shared<HTTP2MessageRouter>(m_authDelegate, m_attachmentManager);
        m_exceptionEncounteredSender = std::make_shared<TestExceptionEncounteredSender>();
        m_dialogUXStateAggregator = std::make_shared<avsCommon::avs::DialogUXStateAggregator>();

        DirectiveHandlerConfiguration config;
        config[SET_MUTE_PAIR] = BlockingPolicy::BLOCKING;
        config[ADJUST_VOLUME_PAIR] = BlockingPolicy::BLOCKING;
        config[EXPECT_SPEECH_PAIR] = BlockingPolicy::BLOCKING;
        m_directiveHandler = std::make_shared<TestDirectiveHandler>(config);

        m_directiveSequencer = DirectiveSequencer::create(m_exceptionEncounteredSender);
        m_messageInterpreter = std::make_shared<MessageInterpreter>(
            m_exceptionEncounteredSender, m_directiveSequencer, m_attachmentManager);

        m_contextManager = ContextManager::create();
        ASSERT_NE(nullptr, m_contextManager);
        PostConnectObject::init(m_contextManager);

        // Set up connection and connect
        m_avsConnectionManager = std::make_shared<TestMessageSender>(
            m_messageRouter, isEnabled, m_connectionStatusObserver, m_messageInterpreter);
        ASSERT_NE(nullptr, m_avsConnectionManager);
        connect();

        m_focusManager = std::make_shared<FocusManager>(FocusManager::DEFAULT_AUDIO_CHANNELS);
        m_testClient = std::make_shared<TestClient>();
        ASSERT_TRUE(
            m_focusManager->acquireChannel(FocusManager::ALERTS_CHANNEL_NAME, m_testClient, ALERTS_ACTIVITY_ID));
        ASSERT_EQ(m_testClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION), FocusState::FOREGROUND);

#ifdef GSTREAMER_MEDIA_PLAYER
        m_mediaPlayer =
            MediaPlayer::create(std::make_shared<avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory>());
#else
        m_mediaPlayer = std::make_shared<TestMediaPlayer>();
#endif

        // Create and register the SpeechSynthesizer.
        m_speechSynthesizer = SpeechSynthesizer::create(
            m_mediaPlayer,
            m_avsConnectionManager,
            m_focusManager,
            m_contextManager,
            m_exceptionEncounteredSender,
            m_dialogUXStateAggregator);
        m_directiveSequencer->addDirectiveHandler(m_speechSynthesizer);
        m_speechSynthesizerObserver = std::make_shared<TestSpeechSynthesizerObserver>();
        m_speechSynthesizer->addObserver(m_speechSynthesizerObserver);
        m_speechSynthesizer->addObserver(m_dialogUXStateAggregator);

        ASSERT_TRUE(m_directiveSequencer->addDirectiveHandler(m_directiveHandler));
    }

    /**
     * Send and event to AVS. Blocks until a status is received.
     *
     * @param message The message to send.
     * @param expectStatus The status to expect from the call to send the message.
     * @param timeout How long to wait for a result from delivering the message.
     */
    void sendEvent(
        const std::string& jsonContent,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader,
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status expectedStatus,
        std::chrono::seconds timeout) {
        auto messageRequest = std::make_shared<ObservableMessageRequest>(jsonContent, attachmentReader);
        m_avsConnectionManager->sendMessage(messageRequest);
        ASSERT_TRUE(messageRequest->waitFor(expectedStatus, timeout));
    }

    /**
     * Function to setup a message and send it to AVS.
     *
     * @param json A JSON string containing the message to send.
     * @param expectStatus The status to expect from the call to send the message.
     * @param timeout How long to wait for a result from delivering the message.
     */
    void setupMessageAndSend(
        const std::string& json,
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status expectedStatus,
        std::chrono::seconds timeout) {
        sendEvent(json, nullptr, expectedStatus, timeout);
    }

    /**
     * Function to setup a message with an attachment and send it to AVS.
     *
     * @param json A JSON string containing the message to send.
     * @param file Name of the file to read the attachment from.
     * @param expectStatus The status to expect from the call to send the message.
     * @param timeout How long to wait for a result from delivering the message.
     */
    void setupMessageWithAttachmentAndSend(
        const std::string& json,
        std::string& file,
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status expectedStatus,
        std::chrono::seconds timeout) {
        auto is = std::make_shared<std::ifstream>(file, std::ios::binary);
        ASSERT_TRUE(is->is_open());

        const int mbBytes = 1024 * 1024;

        std::vector<char> localBuffer(mbBytes);

        auto bufferSize = InProcessSDS::calculateBufferSize(localBuffer.size());
        auto buffer = std::make_shared<InProcessSDSTraits::Buffer>(bufferSize);
        std::shared_ptr<InProcessSDS> sds = InProcessSDS::create(buffer);

        auto attachmentWriter = InProcessAttachmentWriter::create(sds);

        while (*is) {
            is->read(localBuffer.data(), mbBytes);
            size_t numBytesRead = is->gcount();
            AttachmentWriter::WriteStatus writeStatus = AttachmentWriter::WriteStatus::OK;
            attachmentWriter->write(localBuffer.data(), numBytesRead, &writeStatus);

            // write status should be either OK or CLOSED
            bool writeStatusOk =
                (AttachmentWriter::WriteStatus::OK == writeStatus ||
                 AttachmentWriter::WriteStatus::CLOSED == writeStatus);
            ASSERT_TRUE(writeStatusOk);
        }

        attachmentWriter->close();

        std::shared_ptr<InProcessAttachmentReader> attachmentReader =
            InProcessAttachmentReader::create(ReaderPolicy::NONBLOCKING, sds);
        ASSERT_NE(attachmentReader, nullptr);

        sendEvent(json, attachmentReader, expectedStatus, std::chrono::seconds(timeout));
    }

    /**
     * Waits for the ChannelObserverInterface##onFocusChanged() callback.
     *
     * @param timeout The amount of time to wait for the callback.
     * @param focusChanged An output parameter that notifies the caller whether a callback occurred.
     * @return Returns @c true if the callback occured within the timeout period and @c false otherwise.
     */
    FocusState waitForFocusChange(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_focusChanged.wait_for(lock, timeout, [this]() { return m_focusChangeOccurred; });

        return m_focusState;
    }

    void TearDown() override {
        disconnect();
        m_speechSynthesizer->removeObserver(m_dialogUXStateAggregator);
        m_speechSynthesizer->shutdown();
        m_directiveSequencer->shutdown();
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

    bool checkSentEventName(TestMessageSender::SendParams sendParams, std::string expectedName) {
        if (TestMessageSender::SendParams::Type::SEND == sendParams.type) {
            std::string eventString;
            std::string eventHeader;
            std::string eventName;
            jsonUtils::retrieveValue(sendParams.request->getJsonContent(), JSON_MESSAGE_EVENT_KEY, &eventString);
            jsonUtils::retrieveValue(eventString, JSON_MESSAGE_HEADER_KEY, &eventHeader);
            jsonUtils::retrieveValue(eventHeader, JSON_MESSAGE_NAME_KEY, &eventName);
            return eventName == expectedName;
        }
        return false;
    }

    std::shared_ptr<AuthObserver> m_authObserver;
    std::shared_ptr<AuthDelegate> m_authDelegate;
    std::shared_ptr<ConnectionStatusObserver> m_connectionStatusObserver;
    std::shared_ptr<MessageRouter> m_messageRouter;
    std::shared_ptr<TestMessageSender> m_avsConnectionManager;
    std::shared_ptr<TestExceptionEncounteredSender> m_exceptionEncounteredSender;
    std::shared_ptr<TestDirectiveHandler> m_directiveHandler;
    std::shared_ptr<DirectiveSequencerInterface> m_directiveSequencer;
    std::shared_ptr<MessageInterpreter> m_messageInterpreter;
    std::shared_ptr<ContextManager> m_contextManager;
    std::shared_ptr<TestSpeechSynthesizerObserver> m_speechSynthesizerObserver;
    std::shared_ptr<SpeechSynthesizer> m_speechSynthesizer;
    std::shared_ptr<avsCommon::avs::DialogUXStateAggregator> m_dialogUXStateAggregator;
    std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> m_attachmentManager;
    std::shared_ptr<ClientMessageHandler> m_clientMessageHandler;
    std::shared_ptr<FocusManager> m_focusManager;
    std::shared_ptr<TestClient> m_testClient;
    FocusState m_focusState;
    std::mutex m_mutex;
    std::condition_variable m_focusChanged;
    bool m_focusChangeOccurred;

#ifdef GSTREAMER_MEDIA_PLAYER
    std::shared_ptr<MediaPlayer> m_mediaPlayer;
#else
    std::shared_ptr<TestMediaPlayer> m_mediaPlayer;
#endif
};

/**
 * Test ability for the SpeechSynthesizer to handle one Speak directive.
 *
 * This test is intended to test the SpeechSynthesizer's ability to receive one directive, play it using a MediaPlayer
 * then return to a finished state.
 *
 */
TEST_F(SpeechSynthesizerTest, handleOneSpeech) {
    // SpeechSynthesizerObserverInterface defaults to a FINISHED state.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);

    // Send audio of "Joke" that will prompt SetMute and Speak.
    m_directiveSequencer->setDialogRequestId(FIRST_DIALOG_REQUEST_ID);
    std::string file = inputPath + RECOGNIZE_JOKE_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(
        CT_FIRST_RECOGNIZE_EVENT_JSON,
        file,
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS,
        SEND_EVENT_TIMEOUT_DURATION);

    TestMessageSender::SendParams sendRecognizeParams = m_avsConnectionManager->waitForNext(DIRECTIVE_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendRecognizeParams, NAME_RECOGNIZE));

    // Wait for the directive to route through to our handler.
    TestDirectiveHandler::DirectiveParams params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_EQ(params.type, TestDirectiveHandler::DirectiveParams::Type::PREHANDLE);
    params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_EQ(params.type, TestDirectiveHandler::DirectiveParams::Type::HANDLE);

    // Unblock the queue so SpeechSynthesizer can do its work.
    params.result->setCompleted();

    // SpeechSynthesizer is now playing.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::GAINING_FOCUS);

    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING);

    // Check that SS grabs the channel focus by seeing that the test client has been backgrounded.
    ASSERT_EQ(m_testClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION), FocusState::BACKGROUND);

    // SpeechStarted was sent.
    TestMessageSender::SendParams sendStartedParams = m_avsConnectionManager->waitForNext(DIRECTIVE_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));

    // Media Player has finished.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);

    // SpeechFinished is sent here.
    TestMessageSender::SendParams sendFinishedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendFinishedParams, NAME_SPEECH_FINISHED));

    // Alerts channel regains the foreground.
    ASSERT_EQ(m_testClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION), FocusState::FOREGROUND);
}

/**
 * Test ability for the SpeechSynthesizer to handle multiple consecutive Speak directives.
 *
 * This test is intended to test the SpeechSynthesizer's ability to receive multiple directives, play each using a
 * MediaPlayer then return to a finished state and notify the DirectiveSequencer that the directive was handled. This is
 * done by sending a Recognize event to AVS with audio of "What's up?" which returns four sets of SetMute and Speak.
 *
 */
TEST_F(SpeechSynthesizerTest, handleMultipleConsecutiveSpeaks) {
    // SpeechSynthesizerObserverInterface defaults to a FINISHED state.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);

    // Send audio of "What's up" that will prompt four sets of SetMute and Speak.
    m_directiveSequencer->setDialogRequestId(FIRST_DIALOG_REQUEST_ID);
    std::string file = inputPath + RECOGNIZE_WHATS_UP_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(
        CT_FIRST_RECOGNIZE_EVENT_JSON,
        file,
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS,
        SEND_EVENT_TIMEOUT_DURATION);

    TestMessageSender::SendParams sendRecognizeParams = m_avsConnectionManager->waitForNext(DIRECTIVE_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendRecognizeParams, NAME_RECOGNIZE));

    for (unsigned int x = 0; x < NUMBER_OF_SPEAK_DIRECTIVES_TO_VALIDATE; ++x) {
        // Each iteration, remove the blocking setMute directive.
        TestDirectiveHandler::DirectiveParams params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        while (params.type != TestDirectiveHandler::DirectiveParams::Type::HANDLE) {
            ASSERT_NE(params.type, TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);
            params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        }
        params.result->setCompleted();

        // SpeechSynthesizer is now speaking.
        ASSERT_EQ(
            m_speechSynthesizerObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION),
            SpeechSynthesizerObserverInterface::SpeechSynthesizerState::GAINING_FOCUS);

        ASSERT_EQ(
            m_speechSynthesizerObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION),
            SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING);

        // Check that SS grabs the channel focus by seeing that the test client has been backgrounded.
        ASSERT_EQ(m_testClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION), FocusState::BACKGROUND);

        // SpeechStarted has sent.
        TestMessageSender::SendParams sendStartedParams =
            m_avsConnectionManager->waitForNext(DIRECTIVE_TIMEOUT_DURATION);
        ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));

        // Media Player has finished.
        ASSERT_EQ(
            m_speechSynthesizerObserver->waitForNext(WAIT_FOR_MEDIA_PLAYER_TIMEOUT_DURATION),
            SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);

        // SpeechFinished was sent.
        TestMessageSender::SendParams sendFinishedParams =
            m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        ASSERT_TRUE(checkSentEventName(sendFinishedParams, NAME_SPEECH_FINISHED));

        // Alerts channel regains the foreground.
        ASSERT_EQ(m_testClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION), FocusState::FOREGROUND);
    }
}

/**
 * Test ability for the SpeechSynthesizer to handle one Speak directive.
 *
 * This test is intended to test the SpeechSynthesizer's ability to receive one directive, play it using a MediaPlayer
 * then return to a finished state. Once the Speak reaches the SpeechSynthesizer, the dialogRequestID is changed and
 * all directives are cancelled.
 *
 */
TEST_F(SpeechSynthesizerTest, bargeInOnOneSpeech) {
    // SpeechSynthesizerObserverInterface defaults to a FINISHED state.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);

    // Send audio of "Joke" that will prompt SetMute and Speak.
    m_directiveSequencer->setDialogRequestId(FIRST_DIALOG_REQUEST_ID);
    std::string file = inputPath + RECOGNIZE_JOKE_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(
        CT_FIRST_RECOGNIZE_EVENT_JSON,
        file,
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS,
        SEND_EVENT_TIMEOUT_DURATION);

    TestMessageSender::SendParams sendRecognizeParams = m_avsConnectionManager->waitForNext(DIRECTIVE_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendRecognizeParams, NAME_RECOGNIZE));

    // Wait for the directive to route through to our handler.
    TestDirectiveHandler::DirectiveParams params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_EQ(params.type, TestDirectiveHandler::DirectiveParams::Type::PREHANDLE);
    params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_EQ(params.type, TestDirectiveHandler::DirectiveParams::Type::HANDLE);

    // Unblock the queue so SpeechSynthesizer can do its work.
    params.result->setCompleted();

    // SpeechSynthesizer is now speaking.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::GAINING_FOCUS);

    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING);

    // Change the dialogRequestID to cancel the queued directives.
    m_directiveSequencer->setDialogRequestId(SECOND_DIALOG_REQUEST_ID);

    // Check that SS grabs the channel focus by seeing that the test client has been backgrounded.
    ASSERT_EQ(m_testClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION), FocusState::BACKGROUND);

    // SpeechStarted was sent.
    TestMessageSender::SendParams sendStartedParams = m_avsConnectionManager->waitForNext(DIRECTIVE_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));

    // SpeechSynthesizer has finished.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);

    // No SpeechFinished was sent.
    TestMessageSender::SendParams sendFinishedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_FALSE(checkSentEventName(sendFinishedParams, NAME_SPEECH_FINISHED));

    // Alerts channel regains the foreground.
    ASSERT_EQ(m_testClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION), FocusState::FOREGROUND);
}

/**
 * Test ability for the SpeechSynthesizer to handle a barge in at the begining of consucutive speaks.
 *
 * This test is intended to test the SpeechSynthesizer's ability to receive multiple directives, play each using a
 * MediaPlayer then return to a finished state and notify the DirectiveSequencer that the directive was handled. This is
 * done by sending a Recognize event to AVS with audio of "What's up?" which returns four sets of SetMute and Speak.
 * Once the first Speak reaches the SpeechSynthesizer, the dialogRequestID is changed and all directives are cancelled.
 *
 */
TEST_F(SpeechSynthesizerTest, bargeInOnMultipleSpeaksAtTheBeginning) {
    // SpeechSynthesizerObserverInterface defaults to a FINISHED state.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);

    // Send audio of "What's Up" that will prompt four sets of SetMute and Speak.
    m_directiveSequencer->setDialogRequestId(FIRST_DIALOG_REQUEST_ID);
    std::string file = inputPath + RECOGNIZE_WHATS_UP_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(
        CT_FIRST_RECOGNIZE_EVENT_JSON,
        file,
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS,
        SEND_EVENT_TIMEOUT_DURATION);

    TestMessageSender::SendParams sendRecognizeParams = m_avsConnectionManager->waitForNext(DIRECTIVE_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendRecognizeParams, NAME_RECOGNIZE));

    // Wait for the directive to route through to our handler.
    TestDirectiveHandler::DirectiveParams params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_NE(params.type, TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);

    // While blocked, change the dialogRequestID.
    m_directiveSequencer->setDialogRequestId(SECOND_DIALOG_REQUEST_ID);

    while (params.type != TestDirectiveHandler::DirectiveParams::Type::TIMEOUT) {
        if (params.type == TestDirectiveHandler::DirectiveParams::Type::HANDLE) {
            params.result->setCompleted();
        }
        params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    }

    // SpeechSynthesizer is still finished.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WANTING_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);

    // Check that the test client is still in the foreground.
    ASSERT_EQ(m_testClient->waitForFocusChange(WANTING_TIMEOUT_DURATION), FocusState::FOREGROUND);

    // SpeechStarted is not sent.
    TestMessageSender::SendParams canceledSendStartedParams =
        m_avsConnectionManager->waitForNext(WANTING_TIMEOUT_DURATION);
    ASSERT_FALSE(checkSentEventName(canceledSendStartedParams, NAME_SPEECH_STARTED));

    // Media Player has not changed.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WANTING_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);

    // SpeechFinished is not sent.
    TestMessageSender::SendParams canceledSendFinishedParams =
        m_avsConnectionManager->waitForNext(WANTING_TIMEOUT_DURATION);
    ASSERT_FALSE(checkSentEventName(canceledSendFinishedParams, NAME_SPEECH_FINISHED));

    // Alerts channel regains the foreground.
    ASSERT_EQ(m_testClient->waitForFocusChange(WANTING_TIMEOUT_DURATION), FocusState::FOREGROUND);
}

/**
 * Test ability for the SpeechSynthesizer to handle a barge in in the middle of consucutive speaks.
 *
 * This test is intended to test the SpeechSynthesizer's ability to receive multiple directives, play each using a
 * MediaPlayer then return to a finished state and notify the DirectiveSequencer that the directive was handled. This is
 * done by sending a Recognize event to AVS with audio of "What's up?" which returns four sets of SetMute and Speak.
 * While the Speak directives are being handled, the dialogRequestID is changed and all directives are cancelled.
 *
 */
TEST_F(SpeechSynthesizerTest, bargeInOnMultipleSpeaksInTheMiddle) {
    // SpeechSynthesizerObserverInterface defaults to a FINISHED state.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);

    // Send audio of "What's up" that will prompt four sets of SetMute and Speak.
    m_directiveSequencer->setDialogRequestId(FIRST_DIALOG_REQUEST_ID);
    std::string file = inputPath + RECOGNIZE_WHATS_UP_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(
        CT_FIRST_RECOGNIZE_EVENT_JSON,
        file,
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS,
        SEND_EVENT_TIMEOUT_DURATION);

    TestMessageSender::SendParams sendRecognizeParams = m_avsConnectionManager->waitForNext(DIRECTIVE_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendRecognizeParams, NAME_RECOGNIZE));

    // Wait for the directive to route through to our handler.
    TestDirectiveHandler::DirectiveParams params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    bool handleFound = false;
    while (params.type != TestDirectiveHandler::DirectiveParams::Type::TIMEOUT && !handleFound) {
        if (params.type == TestDirectiveHandler::DirectiveParams::Type::HANDLE) {
            handleFound = true;
        } else {
            params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        }
    }
    ASSERT_TRUE(handleFound);

    // Unblock the queue so SS can do its work.
    params.result->setCompleted();

    // SpeechSynthesizer is now speaking.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::GAINING_FOCUS);

    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING);

    // Check that SS grabs the channel focus by seeing that the test client has been backgrounded.
    ASSERT_EQ(m_testClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION), FocusState::BACKGROUND);

    // SpeechStarted is sent.
    TestMessageSender::SendParams sendStartedParams = m_avsConnectionManager->waitForNext(DIRECTIVE_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));

    // Media Player has finished.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);

    // SpeechFinished is sent here.
    TestMessageSender::SendParams sendFinishedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendFinishedParams, NAME_SPEECH_FINISHED));

    // Alerts channel regains the foreground.
    ASSERT_EQ(m_testClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION), FocusState::FOREGROUND);

    // While blocked, change the dialogRequestID to cancel the queued directives.
    m_directiveSequencer->setDialogRequestId(SECOND_DIALOG_REQUEST_ID);

    while (params.type != TestDirectiveHandler::DirectiveParams::Type::TIMEOUT) {
        if (params.type == TestDirectiveHandler::DirectiveParams::Type::HANDLE) {
            // Unblock the queue so SS can do its work.
            params.result->setCompleted();
        }
        params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    }

    // SpeechSynthesizer is still finished.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WANTING_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);

    // Check that the test client is still in the foreground.
    ASSERT_EQ(m_testClient->waitForFocusChange(WANTING_TIMEOUT_DURATION), FocusState::FOREGROUND);

    // SpeechStarted is not sent.
    TestMessageSender::SendParams canceledSendStartedParams =
        m_avsConnectionManager->waitForNext(WANTING_TIMEOUT_DURATION);
    ASSERT_FALSE(checkSentEventName(canceledSendStartedParams, NAME_SPEECH_STARTED));

    // Media Player has not changed.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WANTING_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);

    // SpeechFinished is not sent.
    TestMessageSender::SendParams canceledSendFinishedParams =
        m_avsConnectionManager->waitForNext(WANTING_TIMEOUT_DURATION);
    ASSERT_FALSE(checkSentEventName(canceledSendFinishedParams, NAME_SPEECH_FINISHED));

    // Alerts channel regains the foreground.
    ASSERT_EQ(m_testClient->waitForFocusChange(WANTING_TIMEOUT_DURATION), FocusState::FOREGROUND);
}

/**
 * Test ability for the SpeechSynthesizer to handle a Multiturn scenario.
 *
 * This test is intended to test the SpeechSynthesizer's ability to receive one directive, play it using a MediaPlayer
 * then return to a finished state. Another recognize event is then sent to AVS is response to the ExpectSpeech
 * directive which prompts another Speak directive to be handled.
 */
TEST_F(SpeechSynthesizerTest, multiturnScenario) {
    // SpeechSynthesizerObserverInterface defaults to a FINISHED state.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);

    // Send audio of "Wikipedia" that will prompt SetMute, Speak, and ExpectSpeech.
    m_directiveSequencer->setDialogRequestId(FIRST_DIALOG_REQUEST_ID);
    std::string file = inputPath + RECOGNIZE_WIKI_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(
        CT_FIRST_RECOGNIZE_EVENT_JSON,
        file,
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS,
        SEND_EVENT_TIMEOUT_DURATION);

    TestMessageSender::SendParams sendRecognizeParams = m_avsConnectionManager->waitForNext(DIRECTIVE_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendRecognizeParams, NAME_RECOGNIZE));

    // Wait for the directive to route through to our handler.
    TestDirectiveHandler::DirectiveParams params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_NE(params.type, TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);
    while (params.type == TestDirectiveHandler::DirectiveParams::Type::HANDLE) {
        params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    }
    params.result->setCompleted();

    // SpeechSynthesizer is now speaking.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::GAINING_FOCUS);

    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING);

    // Check that SS grabs the channel focus by seeing that the test client has been backgrounded.
    ASSERT_EQ(m_testClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION), FocusState::BACKGROUND);

    // SpeechStarted is sent.
    TestMessageSender::SendParams sendStartedParams = m_avsConnectionManager->waitForNext(DIRECTIVE_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));

    // Media Player has finished.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);

    // SpeechFinished is sent here.
    TestMessageSender::SendParams sendFinishedParams = m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendFinishedParams, NAME_SPEECH_FINISHED));

    // Alerts channel regains the foreground.
    ASSERT_EQ(m_testClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION), FocusState::FOREGROUND);

    params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    bool expectSpeechFound = false;
    while (params.type != TestDirectiveHandler::DirectiveParams::Type::TIMEOUT) {
        if (params.type == TestDirectiveHandler::DirectiveParams::Type::HANDLE) {
            if (params.directive->getName() == NAME_EXPECT_SPEECH) {
                expectSpeechFound = true;
            }
            // Unblock the queue so SS can do its work.
            params.result->setCompleted();
        }
        params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    }
    ASSERT_TRUE(expectSpeechFound);

    // Clear out remaining directives.
    m_directiveSequencer->setDialogRequestId(FIRST_DIALOG_REQUEST_ID);
    std::string secondFile = inputPath + RECOGNIZE_LIONS_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(
        CT_FIRST_RECOGNIZE_EVENT_JSON,
        secondFile,
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS,
        SEND_EVENT_TIMEOUT_DURATION);

    TestMessageSender::SendParams secondSendRecognizeParams =
        m_avsConnectionManager->waitForNext(DIRECTIVE_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(secondSendRecognizeParams, NAME_RECOGNIZE));

    params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    while (params.type != TestDirectiveHandler::DirectiveParams::Type::TIMEOUT) {
        if (params.type == TestDirectiveHandler::DirectiveParams::Type::HANDLE) {
            // Unblock the queue so SS can do its work.
            params.result->setCompleted();
        }
        params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    }

    // SpeechSynthesizer is now speaking.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::GAINING_FOCUS);

    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING);

    // Check that SS grabs the channel focus by seeing that the test client has been backgrounded.
    ASSERT_EQ(m_testClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION), FocusState::BACKGROUND);

    // SpeechStarted is sent.
    TestMessageSender::SendParams secondSendStartedParams =
        m_avsConnectionManager->waitForNext(DIRECTIVE_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(secondSendStartedParams, NAME_SPEECH_STARTED));

    // Media Player has finished.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);

    // SpeechFinished is sent here.
    TestMessageSender::SendParams secondSendFinishedParams =
        m_avsConnectionManager->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(secondSendFinishedParams, NAME_SPEECH_FINISHED));

    // Alerts channel regains the foreground.
    ASSERT_EQ(m_testClient->waitForFocusChange(WAIT_FOR_TIMEOUT_DURATION), FocusState::FOREGROUND);
}

/**
 * Test ability for the SpeechSynthesizer to handle no directives.
 *
 * This test is intended to test the SpeechSynthesizer's ability to do nothing when there are no Speak directives. A
 * Recognize event with audio of "Volume up" is sent to AVS to prompt a AdjustVolume directive but no Speak directives.
 */
TEST_F(SpeechSynthesizerTest, handleNoSpeakDirectives) {
    // SpeechSynthesizerObserverInterface defaults to a FINISHED state.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);

    // Send audio of "Volume up" that will prompt an adjustVolume directive.
    m_directiveSequencer->setDialogRequestId(FIRST_DIALOG_REQUEST_ID);
    std::string file = inputPath + RECOGNIZE_VOLUME_UP_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(
        CT_FIRST_RECOGNIZE_EVENT_JSON,
        file,
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS,
        SEND_EVENT_TIMEOUT_DURATION);

    TestMessageSender::SendParams sendRecognizeParams = m_avsConnectionManager->waitForNext(DIRECTIVE_TIMEOUT_DURATION);
    ASSERT_TRUE(checkSentEventName(sendRecognizeParams, NAME_RECOGNIZE));

    // Wait for the directive to route through to our handler.
    TestDirectiveHandler::DirectiveParams params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_EQ(params.type, TestDirectiveHandler::DirectiveParams::Type::PREHANDLE);
    params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_EQ(params.type, TestDirectiveHandler::DirectiveParams::Type::HANDLE);
    ASSERT_EQ(params.directive->getName(), NAME_ADJUST_VOLUME);

    // Unblock the queue so SS can do its work.
    params.result->setCompleted();

    // SpeechSynthesizer just defaults to Playing state.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WANTING_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);

    // Check that the test client is still in the foreground.
    ASSERT_EQ(m_testClient->waitForFocusChange(WANTING_TIMEOUT_DURATION), FocusState::FOREGROUND);

    // SpeechStarted is not sent.
    TestMessageSender::SendParams sendStartedParams = m_avsConnectionManager->waitForNext(WANTING_TIMEOUT_DURATION);
    ASSERT_FALSE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));

    // Media Player has not changed.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WANTING_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);

    // SpeechFinished is not sent.
    TestMessageSender::SendParams sendFinishedParams = m_avsConnectionManager->waitForNext(WANTING_TIMEOUT_DURATION);
    ASSERT_FALSE(checkSentEventName(sendFinishedParams, NAME_SPEECH_FINISHED));

    // Alerts channel regains the foreground.
    ASSERT_EQ(m_testClient->waitForFocusChange(WANTING_TIMEOUT_DURATION), FocusState::FOREGROUND);
}

/**
 * Test ability for the test setup to handle no directives.
 *
 * This test is intended to test the SpeechSynthesizer's ability to do nothing when there are no Speak directives. No
 * Recognize events are sent to trigger any directives.
 */
TEST_F(SpeechSynthesizerTest, handleNoDirectives) {
    // SpeechSynthesizerObserverInterface defaults to a FINISHED state.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WAIT_FOR_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);

    TestMessageSender::SendParams sendRecognizeParams = m_avsConnectionManager->waitForNext(WANTING_TIMEOUT_DURATION);
    ASSERT_FALSE(checkSentEventName(sendRecognizeParams, NAME_RECOGNIZE));

    // Wait for the directive to route through to our handler.
    TestDirectiveHandler::DirectiveParams params = m_directiveHandler->waitForNext(WANTING_TIMEOUT_DURATION);
    ASSERT_EQ(params.type, TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);

    // SpeechSynthesizer defaults to Finished state.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WANTING_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);

    // Check that the test client is still in the foreground.
    ASSERT_EQ(m_testClient->waitForFocusChange(WANTING_TIMEOUT_DURATION), FocusState::FOREGROUND);

    // SpeechStarted is not sent.
    TestMessageSender::SendParams sendStartedParams = m_avsConnectionManager->waitForNext(WANTING_TIMEOUT_DURATION);
    ASSERT_FALSE(checkSentEventName(sendStartedParams, NAME_SPEECH_STARTED));

    // Media Player has not changed.
    ASSERT_EQ(
        m_speechSynthesizerObserver->waitForNext(WANTING_TIMEOUT_DURATION),
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);

    // SpeechFinished is not sent.
    TestMessageSender::SendParams sendFinishedParams = m_avsConnectionManager->waitForNext(WANTING_TIMEOUT_DURATION);
    ASSERT_FALSE(checkSentEventName(sendFinishedParams, NAME_SPEECH_FINISHED));

    // Alerts channel regains the foreground.
    ASSERT_EQ(m_testClient->waitForFocusChange(WANTING_TIMEOUT_DURATION), FocusState::FOREGROUND);
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
