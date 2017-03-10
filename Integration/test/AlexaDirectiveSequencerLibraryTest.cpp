/*
 * AlexaDirectiveSequencerLibraryTest.cpp
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

/// @file AlexaDirectiveSequencerLibraryTest.cpp
 #include <gtest/gtest.h>
#include <string>
#include <future>
#include <fstream>
#include <chrono>
#include <deque>
#include <mutex>
#include <unordered_map>

#include "ACL/AVSConnectionManager.h"
#include "ACL/Message.h"
#include "ACL/Transport/HTTP2MessageRouter.h"
#include "ACL/Values.h"
#include "ACL/AttachmentManager.h"
#include "AuthDelegate/AuthDelegate.h"
#include "AuthDelegate/Config.h"
#include "AVSUtils/Initialization/AlexaClientSDKInit.h"
#include "Integration/AuthObserver.h"
#include "Integration/ClientMessageHandler.h"
#include "Integration/ConnectionStatusObserver.h"
#include "Integration/FileConfig.h"
#include "Integration/ObservableMessageRequest.h"
#include "ADSL/BlockingPolicy.h"
#include "ADSL/DirectiveHandlerInterface.h"
#include "ADSL/DirectiveSequencer.h"
#include "ADSL/MessageInterpreter.h"
#include "ADSL/DirectiveHandlerResultInterface.h"
#include "AVSCommon/ExceptionEncounteredSenderInterface.h"
#include "AVSCommon/JSON/JSONUtils.h"

namespace alexaClientSDK {
namespace integration {

using namespace alexaClientSDK::acl;
using namespace alexaClientSDK::adsl;
using namespace alexaClientSDK::authDelegate;
using namespace alexaClientSDK::avsUtils::initialization;
using namespace alexaClientSDK::avsCommon;

static const std::string USERINACTIVITYREPORT_EVENT_JSON = 
"{"
    "\"event\":{"
        "\"header\":{" 
            "\"namespace\":\"System\","
            "\"name\":\"UserInactivityReport\","
            "\"messageId\":\"messageId123\""  
        "},"
        "\"payload\":{"
            "\"inactiveTimeInSeconds\":360"
        "}"

    "}"

"}";

/**
 * This string specifies a Recognize event using the specified profile.
 *
 * CLOSE_TALK performs end-of-speech detection on the client, so no directive is sent from AVS to stop recording.
 * NEAR_FIELD performs end-of-speech detection in AVS, so a directive is sent from AVS to stop recording.
 */
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


/// This is a 16 bit 16 kHz little endian linear PCM audio file of "Joke" to be recognized.
static const std::string RECOGNIZE_JOKE_AUDIO_FILE_NAME = "/recognize_joke_test.wav";
/// This is a 16 bit 16 kHz little endian linear PCM audio file of "Wikipedia" to be recognized.
static const std::string RECOGNIZE_WIKI_AUDIO_FILE_NAME = "/recognize_wiki_test.wav";
/// This is a 16 bit 16 kHz little endian linear PCM audio file of "Lions" to be recognized.
static const std::string RECOGNIZE_LIONS_AUDIO_FILE_NAME = "/recognize_lions_test.wav";
/// This is a 16 bit 16 kHz little endian linear PCM audio file of "Flashbriefing" to be recognized.
static const std::string RECOGNIZE_FLASHBRIEFING_AUDIO_FILE_NAME = "/recognize_flashbriefing_test.wav";
//String to be used as a basic DialogRequestID.
#define FIRST_DIALOG_REQUEST_ID "DialogRequestID123"
//String to be used as a DialogRequestID when the first has already been used.
#define SECOND_DIALOG_REQUEST_ID "DialogRequestID456"
/// This string specifies a Recognize event using the CLOSE_TALK profile and uses the first DialogRequestID.
static const std::string CT_FIRST_RECOGNIZE_EVENT_JSON = RECOGNIZE_EVENT_JSON(CLOSE_TALK, FIRST_DIALOG_REQUEST_ID);
/// This string specifies a Recognize event using the CLOSE_TALK profile and uses the first DialogRequestID.
static const std::string CT_FIRST_RECOGNIZE_EVENT_JSON_NEAR = RECOGNIZE_EVENT_JSON(NEAR_FIELD, FIRST_DIALOG_REQUEST_ID);
/// This string specifies a Recognize event using the CLOSE_TALK profile and uses the second DialogRequestID.
static const std::string CT_SECOND_RECOGNIZE_EVENT_JSON = RECOGNIZE_EVENT_JSON(CLOSE_TALK, SECOND_DIALOG_REQUEST_ID);
// This string to be used for Speak Directives which use the NAMESPACE_SPEECH_SYNTHESIZER namespace.
static const std::string NAME_SPEAK = "Speak";
// This string to be used for ExpectSpeech Directives which use the NAMESPACE_SPEECH_RECOGNIZER namespace.
static const std::string NAME_EXPECT_SPEECH = "ExpectSpeech";
// This string to be used for SetMute Directives which use the NAMESPACE_SPEAKER namespace.
static const std::string NAME_SET_MUTE = "SetMute";
// This string to be used for Play Directives which use the NAMESPACE_AUDIO_PLAYER namespace.
static const std::string NAME_PLAY = "Play";
// This string to be used for StopCapture Directives which use the NAMESPACE_SPEECH_RECOGNIZER namespace.
static const std::string NAME_STOP_CAPTURE = "StopCapture";
// This String to be used to register the SpeechRecognizer namespace to a DirectiveHandler.
static const std::string NAMESPACE_SPEECH_RECOGNIZER = "SpeechRecognizer";
// This String to be used to register the SpeechSynthesizer namespace to a DirectiveHandler.
static const std::string NAMESPACE_SPEECH_SYNTHESIZER = "SpeechSynthesizer";
// This String to be used to register the AudioPlayer namespace to a DirectiveHandler. 
static const std::string NAMESPACE_AUDIO_PLAYER = "AudioPlayer";
// This String to be used to register the Speaker namespace to a DirectiveHandler. 
static const std::string NAMESPACE_SPEAKER = "Speaker";

// This Integer to be used to specify a timeout in seconds for a directive to reach the DirectiveHandler.
static const std::chrono::seconds WAIT_FOR_TIMEOUT_DURATION(5);
// This Integer to be used to specify a timeout in seconds for AuthDelegate to wait for LWA response.
static const std::chrono::seconds SEND_EVENT_TIMEOUT_DURATION(20); 


std::string configPath;
std::string inputPath;

class TestDirectiveHandler : public DirectiveHandlerInterface, public ExceptionEncounteredSenderInterface {
public:
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::AVSDirective> directive) override;

    void preHandleDirective(
            std::shared_ptr<avsCommon::AVSDirective> directive,
            std::shared_ptr<DirectiveHandlerResultInterface> result) override;

    void handleDirective(const std::string& messageId) override;

    void cancelDirective(const std::string& messageId) override;
    
    void sendExceptionEncountered(
        const std::string& unparsedDirective,
        ExceptionErrorType error,
        const std::string& message) override;

    void shutdown() override;

    // Utility struct to pass directive parameters to the test.
    struct DirectiveParams {
        // Enum for the way the directive was passed to DirectiveHandler.
        enum class Type {
            // Set when handleDirectiveImediately is called.
            HANDLE_IMMEDIATELY,
            // Set when preHandleDirective is called.
            PREHANDLE,
            // Set when handleDirective is called.
            HANDLE,
            //Set when cancelDirective is called.
            CANCEL,
            // Set when sendExceptionEncountered is called.
            EXCEPTION,
            // Set when waitForNext times out waiting for a directive.
            TIMEOUT
        };
        // Type of how the directive was passed to DirectiveHandler.
        Type type;
        // AVSDirective passed from the Directive Sequencer to the DirectiveHandler. 
        std::shared_ptr<avsCommon::AVSDirective> directive;
        // DirectiveHandlerResult to inform the Directive Sequencer a directive has either successfully or
        // unsuccessfully handled.
        std::shared_ptr<DirectiveHandlerResultInterface> result;
        // Unparsed directive string passed to sendExceptionEncountered.
        std::string exceptionUnparsedDirective;
        // Error type passed to sendExceptionEncountered.
        ExceptionErrorType exceptionError;
        // Additional information passed to sendExceptionEncountered.
        std::string exceptionMessage;
    };

    /**
     * Function to retrieve the next DirectiveParams in the test queue or time out if the queue is empty. Takes a duration in seconds 
     * to wait before timing out.
     */
    DirectiveParams waitForNext(const std::chrono::seconds duration);

private:
    /// Mutex to protect m_queue.
    std::mutex m_mutex;
    /// Trigger to wake up waitForNext calls.
    std::condition_variable m_wakeTrigger;
    /// Queue of received directives that have not been waited on.
    std::deque<DirectiveParams> m_queue;
    /// map of message IDs to result handlers.
    std::unordered_map<std::string, std::shared_ptr<DirectiveHandlerResultInterface>> results;
    /// map of message IDs to result handlers.
    std::unordered_map<std::string, std::shared_ptr<avsCommon::AVSDirective>> directives;
};

class AlexaDirectiveSequencerLibraryTest : public ::testing::Test {
protected:

    virtual void SetUp() {
        ASSERT_TRUE(AlexaClientSDKInit::initialize());
        m_config = std::make_shared<FileConfig>(configPath);
        m_authObserver = std::make_shared<AuthObserver>();
        m_authDelegate = AuthDelegate::create(m_config);
        m_authDelegate->setAuthObserver(m_authObserver);
        m_connectionStatusObserver = std::make_shared<ConnectionStatusObserver>();
        m_clientMessageHandler = std::make_shared<ClientMessageHandler>();
        bool isEnable = false;
        m_messageRouter = std::make_shared<HTTP2MessageRouter>(m_authDelegate);
        m_directiveHandler = std::make_shared<TestDirectiveHandler>();
        m_directiveSequencer = DirectiveSequencer::create(m_directiveHandler);
        m_messageInterpreter = std::make_shared<MessageInterpreter>(
            m_directiveHandler,
            m_directiveSequencer);

        //note: No DirectiveHandlers have been registered with the DirectiveSequencer yet.  Registration of handlers is
        //  deferred to individual test implementations.

        m_avsConnectionManager = AVSConnectionManager::create(
                m_messageRouter,
                isEnable,
                m_connectionStatusObserver,
                m_messageInterpreter);
        connect();
    }

    virtual void TearDown() {
        disconnect();
        m_directiveSequencer->shutdown();
        AlexaClientSDKInit::uninitialize();
    }

    virtual void connect() {
        ASSERT_TRUE(m_authObserver->waitFor(AuthObserver::State::REFRESHED))
            << "Retrieving the auth token timed out.";
        m_avsConnectionManager->enable();
        ASSERT_TRUE(m_connectionStatusObserver->waitFor(ConnectionStatus::CONNECTED))
            << "Connecting timed out.";
    }

    virtual void disconnect() {
        m_avsConnectionManager->disable();
        ASSERT_TRUE(m_connectionStatusObserver->waitFor(ConnectionStatus::DISCONNECTED)) 
            << "Connecting timed out.";
    }

    /*
     * Function to send a message.
     */
    virtual void sendEvent(std::shared_ptr<Message> message, SendMessageStatus expectedStatus,
            std::chrono::seconds timeout) {
        auto messageRequest = std::make_shared<ObservableMessageRequest>(message);
        m_avsConnectionManager->send(messageRequest);
        ASSERT_TRUE(messageRequest->waitFor(expectedStatus, timeout));
    }

    /**
     * Function to setup a message and send it.
     */
    virtual void setupMessageAndSend(const std::string& json, SendMessageStatus expectedStatus,
            std::chrono::seconds timeout) {
        auto message = std::make_shared<Message>(json);
        sendEvent(message, expectedStatus, timeout);
    }

    /**
     * Function accepts a json string and file name. It creates an attachment from the file and sets up a message
     * with the attachment and sends it.
     */

    virtual void setupMessageWithAttachmentAndSend(const std::string& json, std::string& file,
            SendMessageStatus expectedStatus, std::chrono::seconds timeout) {
        auto attachmentStream = std::make_shared<std::ifstream>(file, std::ios::binary);
        ASSERT_TRUE(attachmentStream->is_open());

        auto message = std::make_shared<Message>(json, attachmentStream);
        sendEvent(message, expectedStatus, std::chrono::seconds(timeout));
    }

    std::shared_ptr<Config> m_config;
    std::shared_ptr<AuthObserver> m_authObserver;
    std::shared_ptr<AuthDelegate> m_authDelegate;
    std::shared_ptr<ConnectionStatusObserver> m_connectionStatusObserver;
    std::shared_ptr<MessageRouter> m_messageRouter;
    std::shared_ptr<AVSConnectionManager> m_avsConnectionManager;
    std::shared_ptr<TestDirectiveHandler> m_directiveHandler;
    std::shared_ptr<DirectiveSequencer> m_directiveSequencer;
    std::shared_ptr<AttachmentManager> m_attachmentManager;
    std::shared_ptr<MessageInterpreter> m_messageInterpreter;
    std::shared_ptr<ClientMessageHandler> m_clientMessageHandler;

};

void TestDirectiveHandler::handleDirectiveImmediately(std::shared_ptr<avsCommon::AVSDirective> directive) {
    std::unique_lock<std::mutex> lock(m_mutex);
    TestDirectiveHandler::DirectiveParams dp;
    dp.type = DirectiveParams::Type::HANDLE_IMMEDIATELY;
    dp.directive = directive;
    m_queue.push_back(dp);
}

void TestDirectiveHandler::preHandleDirective(
            std::shared_ptr<avsCommon::AVSDirective> directive,
            std::shared_ptr<DirectiveHandlerResultInterface> result)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    TestDirectiveHandler::DirectiveParams dp;
    dp.type = TestDirectiveHandler::DirectiveParams::Type::PREHANDLE;
    dp.directive = directive;
    dp.result = result;
    ASSERT_EQ(results.find(directive->getMessageId()), results.end());
    results[directive->getMessageId()] = result;
    ASSERT_EQ(directives.find(directive->getMessageId()), directives.end());
    directives[directive->getMessageId()] = directive;
    m_queue.push_back(dp);
}

void TestDirectiveHandler::handleDirective(const std::string& messageId) {
    std::unique_lock<std::mutex> lock(m_mutex);
    TestDirectiveHandler::DirectiveParams dp;
    dp.type = DirectiveParams::Type::HANDLE;
    auto result = results.find(messageId);
    ASSERT_NE(result, results.end());
    dp.result = result->second;
    auto directive = directives.find(messageId);
    ASSERT_NE(directive, directives.end());
    dp.directive = directive->second;
    m_queue.push_back(dp);
}

void TestDirectiveHandler::cancelDirective(const std::string& messageId) {
    std::unique_lock<std::mutex> lock(m_mutex);
    TestDirectiveHandler::DirectiveParams dp;
    dp.type = DirectiveParams::Type::CANCEL;
    auto result = results.find(messageId);
    ASSERT_NE(result, results.end());
    dp.result = result->second;
    results.erase(result);
    auto directive = directives.find(messageId);
    ASSERT_NE(directive, directives.end());
    dp.directive = directive->second;
    directives.erase(directive);
    m_queue.push_back(dp);
}

void TestDirectiveHandler::sendExceptionEncountered(
    const std::string& unparsedDirective,
    ExceptionErrorType error,
    const std::string& message) {

    std::cout
        << __FUNCTION__ << "(\"" << unparsedDirective << "\", " << error << ", \"" << message << "\")" << std::endl;
    
    std::unique_lock<std::mutex> lock(m_mutex);
    TestDirectiveHandler::DirectiveParams dp;
    dp.type = TestDirectiveHandler::DirectiveParams::Type::EXCEPTION;
    dp.exceptionUnparsedDirective = unparsedDirective;
    dp.exceptionError = error;
    dp.exceptionMessage = message;
    m_queue.push_back(dp);

}

void TestDirectiveHandler::shutdown() {
    // Clear the queue so that we don't retain any pointers to DirectiveSequencer.
    m_queue.clear();
}

TestDirectiveHandler::DirectiveParams TestDirectiveHandler::waitForNext(const std::chrono::seconds duration) {
    DirectiveParams ret;
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakeTrigger.wait_for(lock, duration, [this]() { return !m_queue.empty(); })) {
        ret.type = DirectiveParams::Type::TIMEOUT;
        return ret;
    }
    ret = m_queue.front();
    m_queue.pop_front();
    return ret;
}

/**
 * Test ability to pass a directive from the sequencer to a DirectiveHandler.
 *
 * This test is intended to test the Directive Sequencer's ability to tell a DirectiveHandler to handle a directive that has been registered to the Handler. 
 *
 */
TEST_F(AlexaDirectiveSequencerLibraryTest, sendEventWithDirective) {
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEAKER,  NAME_SET_MUTE}, { m_directiveHandler,  BlockingPolicy::NON_BLOCKING}}}).get());

    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEECH_SYNTHESIZER,  NAME_SPEAK}, { m_directiveHandler,  BlockingPolicy::BLOCKING}}}).get());

    //Send a recgonize event, which should trigger a directive from AVS
    m_directiveSequencer->setDialogRequestId(FIRST_DIALOG_REQUEST_ID); 
    std::string file = inputPath + RECOGNIZE_JOKE_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(
        CT_FIRST_RECOGNIZE_EVENT_JSON, file, SendMessageStatus::SUCCESS, SEND_EVENT_TIMEOUT_DURATION);

    //Wait for the directive to route through to our handler
    TestDirectiveHandler::DirectiveParams params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_NE(params.type, TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);

}

/**
 * Test ability to dequeue a directive group with no blocking directives.
 *
 * This test is intended to test the Directive Sequencer's ability to handle a directive group that has no blocking 
 * directives in it without blocking any of the directives from being passed to the DirectiveHandler.
 *
 */
TEST_F(AlexaDirectiveSequencerLibraryTest, sendDirectiveGroupWithoutBlocking) {
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEAKER,  NAME_SET_MUTE}, { m_directiveHandler,  BlockingPolicy::NON_BLOCKING}}}).get());
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEECH_SYNTHESIZER,  NAME_SPEAK}, { m_directiveHandler,  BlockingPolicy::NON_BLOCKING}}}).get());

    m_directiveSequencer->setDialogRequestId(FIRST_DIALOG_REQUEST_ID); 
    std::string file = inputPath + RECOGNIZE_JOKE_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(
        CT_FIRST_RECOGNIZE_EVENT_JSON, file, SendMessageStatus::SUCCESS, SEND_EVENT_TIMEOUT_DURATION);

    TestDirectiveHandler::DirectiveParams params;

    // Look for SetMute.
    do {
        params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        ASSERT_NE(params.type, TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);
    } while (params.type != TestDirectiveHandler::DirectiveParams::Type::HANDLE);

    // Look for Speak.
    do {
        params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        ASSERT_NE(params.type, TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);
    } while (params.type != TestDirectiveHandler::DirectiveParams::Type::HANDLE);

    // Test queue should now be empty.
    params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_EQ(params.type, TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);
}

/**
 * Test ability to drop directives that do not match the group's DialogRequestID.
 *
 * This test is intended to test the Directive Sequencer's ability to handle a directive that has a different DialogRequestID than the 
 * currently set DialogRequestID. The DirectiveHandler should have no preHandles or onHandles for the dropped directive.
 *
 */
TEST_F(AlexaDirectiveSequencerLibraryTest, sendDirectiveWithDifferentDialogRequestID) {
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEAKER,  NAME_SET_MUTE}, { m_directiveHandler,  BlockingPolicy::NON_BLOCKING}}}).get());
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEECH_SYNTHESIZER,  NAME_SPEAK}, { m_directiveHandler,  BlockingPolicy::BLOCKING}}}).get());
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_AUDIO_PLAYER,  NAME_PLAY}, { m_directiveHandler,  BlockingPolicy::NON_BLOCKING}}}).get());

    m_directiveSequencer->setDialogRequestId(FIRST_DIALOG_REQUEST_ID); 
    std::string file = inputPath + RECOGNIZE_FLASHBRIEFING_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(
        CT_FIRST_RECOGNIZE_EVENT_JSON, file, SendMessageStatus::SUCCESS, SEND_EVENT_TIMEOUT_DURATION);

    // Get a directive with a different dialogRequestID
    file = inputPath + RECOGNIZE_JOKE_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(
        CT_SECOND_RECOGNIZE_EVENT_JSON, file, SendMessageStatus::SUCCESS, SEND_EVENT_TIMEOUT_DURATION);

    // Making sure they all have the same DialogRequestId
    TestDirectiveHandler::DirectiveParams params;

    // 5 for SetMute, Speak, SetMute, Play, Play from the flashbriefing
    int numDirectivesForFlashBriefing = 5;
    // 2 for SetMute and Speak
    int numExpectedExceptions = 2; 

    int handleCounter = 0;
    int exceptionCount = 0;

    params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    while (params.type != TestDirectiveHandler::DirectiveParams::Type::TIMEOUT) {
        ASSERT_NE (params.type, TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);
        if (params.type == TestDirectiveHandler::DirectiveParams::Type::EXCEPTION) {
            ASSERT_EQ(params.exceptionError, ExceptionErrorType::INTERNAL_ERROR);
            ++exceptionCount;
        }
        if (params.type == TestDirectiveHandler::DirectiveParams::Type::HANDLE) {
            ++handleCounter;
            params.result->setCompleted();
            ASSERT_EQ (params.directive->getDialogRequestId(), FIRST_DIALOG_REQUEST_ID);
        }
        params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    }

    ASSERT_EQ (exceptionCount, numExpectedExceptions);
    ASSERT_EQ (handleCounter, numDirectivesForFlashBriefing);
    ASSERT_EQ (params.type, TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);
}

/**
 * Test ability to drop a queue after Barge-In
 *
 * This test is intended to test the Directive Sequencer's ability to handle a change in the set dialogRequestID while a directive group is in the 
 * Sequencer. For this, a blocking directive is passed to a DirectiveHandler to block any other directives from leaving the queue then setDirectiveHandler 
 * is called 
 *
 */
TEST_F(AlexaDirectiveSequencerLibraryTest, dropQueueAfterBargeIn) {
    TestDirectiveHandler::DirectiveParams params, blocking_params;

    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEAKER,  NAME_SET_MUTE}, { m_directiveHandler,  BlockingPolicy::NON_BLOCKING}}}).get());
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEECH_SYNTHESIZER,  NAME_SPEAK}, { m_directiveHandler,  BlockingPolicy::BLOCKING}}}).get());
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_AUDIO_PLAYER,  NAME_PLAY}, { m_directiveHandler,  BlockingPolicy::NON_BLOCKING}}}).get());

    m_directiveSequencer->setDialogRequestId(FIRST_DIALOG_REQUEST_ID); 
    std::string file = inputPath + RECOGNIZE_FLASHBRIEFING_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(
        CT_FIRST_RECOGNIZE_EVENT_JSON, file, SendMessageStatus::SUCCESS, SEND_EVENT_TIMEOUT_DURATION);

    // Expect set-mute (non-blocking)
    do {
        params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        ASSERT_NE(params.type, TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);
    } while (params.type != TestDirectiveHandler::DirectiveParams::Type::HANDLE);
    ASSERT_NE(params.type, TestDirectiveHandler::DirectiveParams::Type::EXCEPTION);
    ASSERT_EQ(params.directive->getDialogRequestId(), FIRST_DIALOG_REQUEST_ID);

    // Expect speak (blocking)
    do {
        blocking_params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        ASSERT_NE(blocking_params.type, TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);
    } while (blocking_params.type != TestDirectiveHandler::DirectiveParams::Type::HANDLE);
    
    // Send an event with a different DialogRequestId to get a directive with a different DialogRequestId 
    m_directiveSequencer->setDialogRequestId(SECOND_DIALOG_REQUEST_ID); 
    std::string differentFile = inputPath + RECOGNIZE_JOKE_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(
            CT_SECOND_RECOGNIZE_EVENT_JSON,
            differentFile,
            SendMessageStatus::SUCCESS,
            SEND_EVENT_TIMEOUT_DURATION);

    // Handle the blocking directive
    do {
        params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        if (TestDirectiveHandler::DirectiveParams::Type::CANCEL == params.type) {
            ASSERT_EQ(params.directive->getDialogRequestId(), FIRST_DIALOG_REQUEST_ID);
        } else if (TestDirectiveHandler::DirectiveParams::Type::HANDLE == params.type) {
            ASSERT_EQ(params.directive->getDialogRequestId(), SECOND_DIALOG_REQUEST_ID);
            ASSERT_TRUE(params.result);
            params.result->setCompleted();
        }
    } while (params.type != TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);
}

/**
 * Test ability to handle a Directive without a DialogRequestID
 *
 * This test is intended to test the Directive Sequencer's ability to immediately pass a directive that does not have a DialogRequestId to the registered DirectiveHandler
 *
 */
TEST_F(AlexaDirectiveSequencerLibraryTest, sendDirectiveWithoutADialogRequestID) {
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEECH_RECOGNIZER,  NAME_STOP_CAPTURE}, { m_directiveHandler,  BlockingPolicy::NON_BLOCKING}}}).get());
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEAKER,  NAME_SET_MUTE}, { m_directiveHandler,  BlockingPolicy::NON_BLOCKING}}}).get());
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEECH_SYNTHESIZER,  NAME_SPEAK}, { m_directiveHandler,  BlockingPolicy::NON_BLOCKING}}}).get());

    m_directiveSequencer->setDialogRequestId(FIRST_DIALOG_REQUEST_ID); 
    std::string file = inputPath + RECOGNIZE_JOKE_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(
        CT_FIRST_RECOGNIZE_EVENT_JSON_NEAR, file, SendMessageStatus::SUCCESS, SEND_EVENT_TIMEOUT_DURATION);


    TestDirectiveHandler::DirectiveParams params;

    // Make sure we get the handleImmediately from StopCapture
    params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_EQ(params.type, TestDirectiveHandler::DirectiveParams::Type::HANDLE_IMMEDIATELY );

    params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    while (params.type != TestDirectiveHandler::DirectiveParams::Type::TIMEOUT){
        // Make sure no other calls for StopCapture are made except for the initial handleImmediately 
        ASSERT_NE(params.type, TestDirectiveHandler::DirectiveParams::Type::EXCEPTION);
        ASSERT_NE(params.directive->getName(), NAME_STOP_CAPTURE);
        params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    }
}

/**
 * Test ability to alert a DirectiveHandler to prehandle a directive
 *
 * This test is intended to test the Directive Sequencer's ability to send preHandle to a DirectiveHandler in anticipation of a directive needing to be handled 
 *
 */
TEST_F(AlexaDirectiveSequencerLibraryTest, sendDirectivesForPreHandling) {
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEAKER,  NAME_SET_MUTE}, { m_directiveHandler,  BlockingPolicy::NON_BLOCKING}}}).get());
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEECH_SYNTHESIZER,  NAME_SPEAK}, { m_directiveHandler,  BlockingPolicy::BLOCKING}}}).get());
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_AUDIO_PLAYER,  NAME_PLAY}, { m_directiveHandler,  BlockingPolicy::NON_BLOCKING}}}).get());

    m_directiveSequencer->setDialogRequestId(FIRST_DIALOG_REQUEST_ID); 
    std::string file = inputPath + RECOGNIZE_FLASHBRIEFING_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(
        CT_FIRST_RECOGNIZE_EVENT_JSON, file, SendMessageStatus::SUCCESS, SEND_EVENT_TIMEOUT_DURATION);

    TestDirectiveHandler::DirectiveParams params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    int preHandleCounter = 0; 
    int onHandleCounter = 0; 
    while (params.type != TestDirectiveHandler::DirectiveParams::Type::TIMEOUT) {
        if (params.type == TestDirectiveHandler::DirectiveParams::Type::PREHANDLE) {
            preHandleCounter++;
        } else if (params.type == TestDirectiveHandler::DirectiveParams::Type::HANDLE) {
            onHandleCounter++;
            ASSERT_TRUE(params.result);
            params.result->setCompleted();
        }
        ASSERT_TRUE(preHandleCounter >= onHandleCounter);
        params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    }

    // DirectiveHandler was notified of the directive for pre-handling. 
    ASSERT_EQ(preHandleCounter, onHandleCounter);
}

/**
 * Test ability to drop the head of a DialogRequestID group
 *
 * This test is intended to test the Directive Sequencer's ability to drop directives to facilitate flushing out the Senquencer's queue
 *
 */
TEST_F(AlexaDirectiveSequencerLibraryTest, cancelDirectivesWhileInQueue) {
    TestDirectiveHandler::DirectiveParams params, blocking_params;

    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEAKER,  NAME_SET_MUTE}, { m_directiveHandler,  BlockingPolicy::NON_BLOCKING}}}).get());
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEECH_SYNTHESIZER,  NAME_SPEAK}, { m_directiveHandler,  BlockingPolicy::BLOCKING}}}).get());
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_AUDIO_PLAYER,  NAME_PLAY}, { m_directiveHandler,  BlockingPolicy::NON_BLOCKING}}}).get());

    m_directiveSequencer->setDialogRequestId(FIRST_DIALOG_REQUEST_ID); 
    std::string file = inputPath + RECOGNIZE_FLASHBRIEFING_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(
        CT_FIRST_RECOGNIZE_EVENT_JSON, file, SendMessageStatus::SUCCESS, SEND_EVENT_TIMEOUT_DURATION);

    // Expect set-mute (non-blocking)
    do {
        params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        ASSERT_NE(params.type, TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);
    } while (params.type != TestDirectiveHandler::DirectiveParams::Type::HANDLE);

    // Expect speak (blocking)
    do {
        blocking_params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        ASSERT_NE(blocking_params.type, TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);
    } while (blocking_params.type != TestDirectiveHandler::DirectiveParams::Type::HANDLE);

    // Expect timeout (because we're blocked)
    do {
        params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        ASSERT_NE(params.type, TestDirectiveHandler::DirectiveParams::Type::HANDLE);
    } while (params.type != TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);
    
    // Send back error for the speak handler
    ASSERT_TRUE(blocking_params.result);
    blocking_params.result->setFailed("General Error");

    // Check that no other directives arrive for handling. 
    do {
        params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        if(TestDirectiveHandler::DirectiveParams::Type::CANCEL != params.type) {
            break;
        }
    } while (params.type != TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);
    ASSERT_EQ(params.type, TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);
}

/**
 * Test ability to senquence a group that has one Blocking Directive at the front
 *
 * This test is intended to test the Directive Sequencer's ability to handle a DialogRequestID group that has a blocking directive at the front, followed by nonblocking directives. 
 * Until the DirectiveHandler returns a onDirectiveHandled, it should not recieve any directives to handle. Once the queue is unblocked, non blocking directives should flow from 
 * the sequencer to the handler without waiting for the previous directive to be handled 
 *
 */
TEST_F(AlexaDirectiveSequencerLibraryTest, oneBlockingDirectiveAtTheFront) {
    TestDirectiveHandler::DirectiveParams params, blocking_params;

    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEAKER,  NAME_SET_MUTE}, { m_directiveHandler,  BlockingPolicy::BLOCKING}}}).get());
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEECH_SYNTHESIZER,  NAME_SPEAK}, { m_directiveHandler,  BlockingPolicy::NON_BLOCKING}}}).get());
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_AUDIO_PLAYER,  NAME_PLAY}, { m_directiveHandler,  BlockingPolicy::NON_BLOCKING}}}).get());

    m_directiveSequencer->setDialogRequestId(FIRST_DIALOG_REQUEST_ID); 
    std::string file = inputPath + RECOGNIZE_JOKE_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(
        CT_FIRST_RECOGNIZE_EVENT_JSON, file, SendMessageStatus::SUCCESS, SEND_EVENT_TIMEOUT_DURATION);

    // Expect set-mute handle (blocking) and no other handles until we time out (because we're blocked)
    int numHandles = 0;
    do {
        params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        if (params.type == TestDirectiveHandler::DirectiveParams::Type::HANDLE) {
            // Note the blocking params from handle so we can unblock below
            blocking_params = params;
            ++numHandles;
        }
    } while (params.type != TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);
    ASSERT_EQ(numHandles, 1);

    // Unblock the queue 
    ASSERT_TRUE(blocking_params.result);
    blocking_params.result->setCompleted();

    // Expect speak (non-blocking)
    do {
        params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        ASSERT_NE(params.type, TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);
    } while (params.type != TestDirectiveHandler::DirectiveParams::Type::HANDLE);
}

/**
 * Test ability to senquence a group that has one Blocking Directive in the middle
 *
 * This test is intended to test the Directive Sequencer's ability to handle a DialogRequestID group that has at least one nonblocking directive, followed by a blocking directive, 
 * followed by nonblocking directives. Directives should be given to the proper handler until the blocking directive is reached. Until the DirectiveHandler returns a onDirectiveHandled, 
 * it should not recieve any directives to handle. Once the queue is unblocked, nonblocking directives should flow from the sequencer to the handler without waiting for the previous 
 * directive to be handled 
 *
 */
TEST_F(AlexaDirectiveSequencerLibraryTest, oneBlockingDirectiveInTheMiddle) {
    TestDirectiveHandler::DirectiveParams params, blocking_params;

    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEAKER,  NAME_SET_MUTE}, { m_directiveHandler,  BlockingPolicy::NON_BLOCKING}}}).get());
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEECH_SYNTHESIZER,  NAME_SPEAK}, { m_directiveHandler,  BlockingPolicy::BLOCKING}}}).get());
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_AUDIO_PLAYER,  NAME_PLAY}, { m_directiveHandler,  BlockingPolicy::NON_BLOCKING}}}).get());

    m_directiveSequencer->setDialogRequestId(FIRST_DIALOG_REQUEST_ID); 
    std::string file = inputPath + RECOGNIZE_FLASHBRIEFING_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(
        CT_FIRST_RECOGNIZE_EVENT_JSON, file, SendMessageStatus::SUCCESS, SEND_EVENT_TIMEOUT_DURATION);

    // Expect set-mute (non-blocking)
    do {
        params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        ASSERT_NE(params.type, TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);
    } while (params.type != TestDirectiveHandler::DirectiveParams::Type::HANDLE);

    // Expect speak (blocking)
    do {
        blocking_params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        ASSERT_NE(blocking_params.type, TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);
    } while (blocking_params.type != TestDirectiveHandler::DirectiveParams::Type::HANDLE);

    // Expect timeout (because we're blocked)
    do {
        params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        ASSERT_NE(params.type, TestDirectiveHandler::DirectiveParams::Type::HANDLE);
    } while (params.type != TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);
    
    // Unblock the queue 
    ASSERT_TRUE(blocking_params.result);
    blocking_params.result->setCompleted();

    // See things that were previously blocked in the queue come through afterward 
    params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_NE(params.type, TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);
}

/**
 * Test ability to drop a directive when no handlers are registered 
 *
 * This test is intended to test the Directive Sequencer's ability to drop directives if no DirectiveHandlers have registered to handle the directive. To do this, no 
 * setBlockingAndDirectiveHandler to called for a directive that is known to come down consistently in response to a Recognize event 
 */
TEST_F(AlexaDirectiveSequencerLibraryTest, noDirectiveHandlerRegisteredForADirective) {
    // Don't Register a DirectiveHandler for set mute 
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEECH_SYNTHESIZER,  NAME_SPEAK}, { m_directiveHandler,  BlockingPolicy::NON_BLOCKING}}}).get());

    m_directiveSequencer->setDialogRequestId(FIRST_DIALOG_REQUEST_ID); 
    std::string file = inputPath + RECOGNIZE_JOKE_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(
        CT_FIRST_RECOGNIZE_EVENT_JSON, file, SendMessageStatus::SUCCESS, SEND_EVENT_TIMEOUT_DURATION);

    TestDirectiveHandler::DirectiveParams params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);

    while (params.type != TestDirectiveHandler::DirectiveParams::Type::TIMEOUT) {
        ASSERT_NE(params.type, TestDirectiveHandler::DirectiveParams::Type::EXCEPTION);
        ASSERT_NE(params.directive->getName(), NAME_SET_MUTE);
        params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);  
    }
}

/**
 * Test ability to sequnce a directive when two handlers are registered 
 *
 * This test is intended to test the Directive Sequencer's ability to drop directives if two DirectiveHandlers have registered to handle the directive. To do this, two 
 * different handlers call setBlockingAndDirectiveHandler for a directive that is known to come down consistently in response to a Recognize event. The Handler that
 * registers second is the handler that receives the directive 
 */
TEST_F(AlexaDirectiveSequencerLibraryTest, twoDirectiveHandlersRegisteredForADirective) {
    auto m_directiveHandlerA = std::make_shared<TestDirectiveHandler>();
    auto m_directiveHandlerB = std::make_shared<TestDirectiveHandler>();

    // Register 2 DirectiveHandlers to 1 directive
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEAKER,  NAME_SET_MUTE}, { m_directiveHandlerA,  BlockingPolicy::BLOCKING}}}).get());
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEAKER,  NAME_SET_MUTE}, { m_directiveHandlerB,  BlockingPolicy::BLOCKING}}}).get());

    m_directiveSequencer->setDialogRequestId(FIRST_DIALOG_REQUEST_ID); 
    std::string file = inputPath + RECOGNIZE_JOKE_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(
        CT_FIRST_RECOGNIZE_EVENT_JSON, file, SendMessageStatus::SUCCESS, SEND_EVENT_TIMEOUT_DURATION);

    // A is empty 
    TestDirectiveHandler::DirectiveParams paramsA = m_directiveHandlerA->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_EQ(paramsA.type, TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);

    // B received the directive 
    TestDirectiveHandler::DirectiveParams paramsB = m_directiveHandlerB->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_NE(paramsB.type, TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);
}

/**
 * Test ability to handle a multi-turn scenario
 *
 * This test is intended to test the Directive Sequencer's ability to go through a full loop of sending a recognize event that will prompt a multiturn directive, receiving a directive 
 * group that contains ExecptSpeech, sending a recognize event to respond to Alexa's question, and receiving the final directive group 
 */
TEST_F(AlexaDirectiveSequencerLibraryTest, multiturnScenario) {
    TestDirectiveHandler::DirectiveParams params;

    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEAKER,  NAME_SET_MUTE}, { m_directiveHandler,  BlockingPolicy::NON_BLOCKING}}}).get());
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEECH_SYNTHESIZER,  NAME_SPEAK}, { m_directiveHandler,  BlockingPolicy::BLOCKING}}}).get());
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEECH_RECOGNIZER,  NAME_EXPECT_SPEECH}, { m_directiveHandler,  BlockingPolicy::NON_BLOCKING}}}).get());

    m_directiveSequencer->setDialogRequestId(FIRST_DIALOG_REQUEST_ID); 
    std::string file = inputPath + RECOGNIZE_WIKI_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(
        CT_FIRST_RECOGNIZE_EVENT_JSON, file, SendMessageStatus::SUCCESS, SEND_EVENT_TIMEOUT_DURATION);

    // Check we're being told to expect speech
    do {
        params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        ASSERT_NE(params.type, TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);
        ASSERT_NE(params.type, TestDirectiveHandler::DirectiveParams::Type::EXCEPTION);
        ASSERT_EQ(params.directive->getDialogRequestId(), FIRST_DIALOG_REQUEST_ID);
        if (params.type == TestDirectiveHandler::DirectiveParams::Type::HANDLE) {
            params.result->setCompleted();
        }
    } while (params.directive->getName() != NAME_EXPECT_SPEECH ||
            params.type != TestDirectiveHandler::DirectiveParams::Type::HANDLE);

    // Send back a recognize event  
    m_directiveSequencer->setDialogRequestId(SECOND_DIALOG_REQUEST_ID);  
    std::string differentFile = inputPath + RECOGNIZE_LIONS_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(
        CT_SECOND_RECOGNIZE_EVENT_JSON, differentFile, SendMessageStatus::SUCCESS, SEND_EVENT_TIMEOUT_DURATION);

    // Just the wikipedia directive group in response
    do {
        params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
        if (params.type == TestDirectiveHandler::DirectiveParams::Type::HANDLE) {
            ASSERT_EQ(params.directive->getDialogRequestId(), SECOND_DIALOG_REQUEST_ID);
            params.result->setCompleted();
        }
    } while (params.type != TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);
    
}

/**
 * Test ability to get an attachment from attachment manager.
 */
TEST_F(AlexaDirectiveSequencerLibraryTest, getAttachmentWithContentId) {
    ASSERT_TRUE(m_directiveSequencer->setDirectiveHandlers({{{NAMESPACE_SPEECH_SYNTHESIZER,  NAME_SPEAK}, { m_directiveHandler,  BlockingPolicy::BLOCKING}}}).get());

    // Send a recgonize event, which should trigger a directive from AVS.
    m_directiveSequencer->setDialogRequestId(FIRST_DIALOG_REQUEST_ID);
    std::string file = inputPath + RECOGNIZE_JOKE_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(
            CT_FIRST_RECOGNIZE_EVENT_JSON, file, SendMessageStatus::SUCCESS, SEND_EVENT_TIMEOUT_DURATION);

    // Wait for the directive to route through to our handler.
    TestDirectiveHandler::DirectiveParams params = m_directiveHandler->waitForNext(WAIT_FOR_TIMEOUT_DURATION);
    ASSERT_NE(params.type, TestDirectiveHandler::DirectiveParams::Type::TIMEOUT);

    auto directive = params.directive;

    std::string payloadUrl;
    jsonUtils::lookupStringValue(directive->getPayload(), "url", &payloadUrl);

    ASSERT_TRUE(!payloadUrl.empty());
    auto stringIndex = payloadUrl.find(":");
    ASSERT_TRUE(stringIndex != std::string::npos);
    ASSERT_TRUE(stringIndex != payloadUrl.size() - 1);

    auto contentId = payloadUrl.substr(payloadUrl.find(':') + 1);
    auto attachmentReader = directive->getAttachmentReader(contentId);
    auto status = attachmentReader.wait_for(WAIT_FOR_TIMEOUT_DURATION);

    ASSERT_EQ(status, std::future_status::ready);
}

} // namespace integration
} // namespace alexaClientSDK

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    if (argc < 3) {
        std::cerr << "USAGE: AlexaDirectiveSequencerLibraryTest <path_to_auth_delgate_config> <path_to_inputs_folder>"
                << std::endl;
        return 1;

    } else {
        alexaClientSDK::integration::configPath = std::string(argv[1]);
        alexaClientSDK::integration::inputPath = std::string(argv[2]);
        return RUN_ALL_TESTS();
    }
}


