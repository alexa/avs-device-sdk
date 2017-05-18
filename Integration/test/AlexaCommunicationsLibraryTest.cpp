/*
 * AlexaCommunicationsLibraryTest.cpp
 *
 * Copyright 2016-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

/// @file AlexaCommunicationsLibraryTest.cpp

#include <future>
#include <fstream>
#include <chrono>

#include <gtest/gtest.h>

#include <ACL/AVSConnectionManager.h>
#include <AVSCommon/AVS/Message.h>
#include <ACL/Transport/HTTP2MessageRouter.h>
#include <ACL/Values.h>
#include "AVSCommon/AVS/Attachment/InProcessAttachment.h"
#include "AVSCommon/Utils/SDS/InProcessSDS.h"
#include <AuthDelegate/AuthDelegate.h>
#include <AVSUtils/Initialization/AlexaClientSDKInit.h>
#include <AVSUtils/Logger/LogEntry.h>
#include <AVSUtils/Logging/Logger.h>

#include "Integration/AuthObserver.h"
#include "Integration/ClientMessageHandler.h"
#include "Integration/ConnectionStatusObserver.h"
#include "Integration/ObservableMessageRequest.h"

namespace alexaClientSDK {
namespace integration {

using namespace alexaClientSDK::acl;
using namespace alexaClientSDK::authDelegate;
using namespace alexaClientSDK::avsUtils::initialization;
using namespace alexaClientSDK::avsCommon::avs::attachment;
using namespace alexaClientSDK::avsCommon::utils::sds;

static const std::string SYNCHRONIZE_STATE_JSON =
    "{"
        "\"context\":[{"
            "\"header\":{"
                "\"name\":\"SpeechState\","
                "\"namespace\":\"SpeechSynthesizer\""
            "},"
            "\"payload\":{"
                "\"playerActivity\":\"FINISHED\","
                "\"offsetInMilliseconds\":0,"
                "\"token\":\"\""
            "}"
        "}],"
        "\"event\":{"
            "\"header\":{"
                "\"messageId\":\"00000000-0000-0000-0000-000000000000\","
                "\"name\":\"SynchronizeState\","
                "\"namespace\":\"System\""
            "},"
            "\"payload\":{"
            "}"
        "}"
    "}";

/// This is a partial JSON string that should not be parseable.
static const std::string BAD_SYNCHRONIZE_STATE_JSON = "{";

/**
 * This string specifies a Recognize event using the specified profile.
 *
 * CLOSE_TALK performs end-of-speech detection on the client, so no directive is sent from AVS to stop recording.
 * NEAR_FIELD performs end-of-speech detection in AVS, so a directive is sent from AVS to stop recording.
 */
#define RECOGNIZE_EVENT_JSON(PROFILE)                             \
    "{"                                                           \
        "\"event\":{"                                             \
            "\"payload\":{"                                       \
                "\"format\":\"AUDIO_L16_RATE_16000_CHANNELS_1\"," \
                "\"profile\":\"" #PROFILE "\""                    \
            "},"                                                  \
            "\"header\":{"                                        \
                "\"dialogRequestId\":\"dialogRequestId123\","     \
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

/// This string specifies a Recognize event using the CLOSE_TALK profile
static const std::string CT_RECOGNIZE_EVENT_JSON = RECOGNIZE_EVENT_JSON(CLOSE_TALK);
/// This string specifies a Recognize event using the NEAR_FIELD profile
static const std::string NF_RECOGNIZE_EVENT_JSON = RECOGNIZE_EVENT_JSON(NEAR_FIELD);

/// This string specifies an ExpectSpeechTimedOut event
static const std::string EXPECT_SPEECH_TIMED_OUT_EVENT_JSON =
    "{"
        "\"event\": {"
            "\"header\": {"
                "\"namespace\": \"SpeechRecognizer\","
                "\"name\": \"ExpectSpeechTimedOut\","
                "\"messageId\": \"messageId123\","
            "},"
            "\"payload\": {"
            "}"
        "}"
    "}";

/// This is a 16 bit 16 kHz little endian linear PCM audio file containing a recognized message for AVS
static const std::string RECOGNIZE_AUDIO_FILE_NAME = "/recognize_test.wav";
/// This is a 16 bit 16 kHz little endian linear PCM audio file containing several seconds of silence
static const std::string SILENCE_AUDIO_FILE_NAME = "/silence_test.wav";

/**
 * The value of MAX_THREADS is determined by the maximum number of streams we can have active at once which is defined
 * in HTTP2Connection.cpp as a constant MAX_STREAMS=10. Streams include events, the downchannel and ping.
 * Since we establish a downchannel when we connect, we can only have (MAX_STREAMS - 1) events sent at once.
 * Therefore, the value of MAX_THREADS = MAX_STREAMS -1.
 */
static const int MAX_TEST_THREADS = 9;

std::string configPath;
std::string inputPath;

class AlexaCommunicationsLibraryTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        std::ifstream infile(configPath);
        ASSERT_TRUE(infile.good());
        ASSERT_TRUE(AlexaClientSDKInit::initialize({&infile}));
        m_authObserver = std::make_shared<AuthObserver>();
        ASSERT_TRUE(m_authDelegate = AuthDelegate::create());
        m_authDelegate->setAuthObserver(m_authObserver);
        m_connectionStatusObserver = std::make_shared<ConnectionStatusObserver>();
        m_clientMessageHandler = std::make_shared<ClientMessageHandler>();
        bool isEnable = false;
        m_messageRouter = std::make_shared<HTTP2MessageRouter>(m_authDelegate);
        m_avsConnectionManager = AVSConnectionManager::create(
                m_messageRouter,
                isEnable,
                m_connectionStatusObserver,
                m_clientMessageHandler);
        connect();
    }

    virtual void TearDown() {
        disconnect();
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
        if (m_avsConnectionManager) {
            m_avsConnectionManager->disable();
            ASSERT_TRUE(m_connectionStatusObserver->waitFor(ConnectionStatus::DISCONNECTED)) << "Connecting timed out.";
        }
    }

    /*
     * Function to send a message.
     */
    virtual void sendEvent(
            const std::string & jsonContent,
            std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader,
            avsCommon::avs::MessageRequest::Status expectedStatus,
            std::chrono::seconds timeout) {
        auto messageRequest = std::make_shared<ObservableMessageRequest>(jsonContent, attachmentReader);
        m_avsConnectionManager->sendMessage(messageRequest);
        ASSERT_TRUE(messageRequest->waitFor(expectedStatus, timeout));
    }

    /**
     * Function to setup a message and send it.
     */
    virtual void setupMessageAndSend(const std::string& json, avsCommon::avs::MessageRequest::Status expectedStatus,
            std::chrono::seconds timeout) {
        sendEvent(json, nullptr, expectedStatus, timeout);
    }

    /**
     * Function accepts a json string and file name. It creates an attachment from the file and sets up a message
     * with the attachment and sends it.
     */

    virtual void setupMessageWithAttachmentAndSend(const std::string& json, std::string& file,
            avsCommon::avs::MessageRequest::Status expectedStatus, std::chrono::seconds timeout) {
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
            bool writeStatusOk = (AttachmentWriter::WriteStatus::OK == writeStatus ||
                    AttachmentWriter::WriteStatus::CLOSED == writeStatus);
            ASSERT_TRUE(writeStatusOk);
        }

        attachmentWriter->close();

        std::shared_ptr<InProcessAttachmentReader> attachmentReader =
                InProcessAttachmentReader::create(AttachmentReader::Policy::NON_BLOCKING, sds);
        ASSERT_NE(attachmentReader, nullptr);

        sendEvent(json, attachmentReader, expectedStatus, std::chrono::seconds(timeout));
    }

    /**
     * This functions send and event or an event with an attachment based on the random number generated.
     */
    virtual void sendEventRandom() {
        int i = rand() % 100;

        if (i % 2) {
            setupMessageAndSend(
                SYNCHRONIZE_STATE_JSON,
                avsCommon::avs::MessageRequest::Status::SUCCESS,
                std::chrono::seconds(40));
        } else {
            std::string file = inputPath + RECOGNIZE_AUDIO_FILE_NAME;
            setupMessageWithAttachmentAndSend(
                CT_RECOGNIZE_EVENT_JSON,
                file,
                avsCommon::avs::MessageRequest::Status::SUCCESS,
                std::chrono::seconds(40));
        }
    }

    std::shared_ptr<AuthObserver> m_authObserver;
    std::shared_ptr<AuthDelegate> m_authDelegate;
    std::shared_ptr<ConnectionStatusObserver> m_connectionStatusObserver;
    std::shared_ptr<ClientMessageHandler> m_clientMessageHandler;
    std::shared_ptr<MessageRouter> m_messageRouter;
    std::shared_ptr<AVSConnectionManager> m_avsConnectionManager;
};

/// Test basic ability to connect and disconnect from Alexa Voice Service
TEST_F(AlexaCommunicationsLibraryTest, connectAndDisconnect) {
    /*
     * Connect is called in SetUp and disconnect is called in TearDown. This test is a sanity check that connect and
     * disconnect are working.
     */
}

/**
 * Test ability to send an event to Alexa Voice Service.
 *
 * This test sends a SynchronizeState event, which does not require attachments or respond with directives.
 *
 * @see https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/system#synchronizestate
 */
TEST_F(AlexaCommunicationsLibraryTest, sendEvent) {
    setupMessageAndSend(
        SYNCHRONIZE_STATE_JSON,
        avsCommon::avs::MessageRequest::Status::SUCCESS,
        std::chrono::seconds(10));
}

/**
 * Test ability to send an event with attachments to Alexa Voice Service.
 *
 * This test sends a RecognizeAudio event, which requires an attachment of the audio stream.  In this case,
 * we send a pre-recorded audio file which asks Alexa to "tell me a joke".
 *
 * @see https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/speechrecognizer#recognize
 */
TEST_F(AlexaCommunicationsLibraryTest, sendEventWithAttachments) {
    std::string file = inputPath + RECOGNIZE_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(
        CT_RECOGNIZE_EVENT_JSON,
        file, avsCommon::avs::MessageRequest::Status::SUCCESS,
        std::chrono::seconds(10));
}

/**
 * Function that tests the behavior when of the ACL when an improperly formatted message is sent, expecting the server
 * to return an internal error
 */
TEST_F(AlexaCommunicationsLibraryTest, sendEventThatWillFail) {
    setupMessageAndSend(BAD_SYNCHRONIZE_STATE_JSON, avsCommon::avs::MessageRequest::Status::SERVER_INTERNAL_ERROR,
            std::chrono::seconds(10));
}

/**
 * Function tests for disconnection between events. After sending the first event, the test monitors for
 * change in the connection in the next 20s. We are expecting no disconnects between events.
 */
TEST_F(AlexaCommunicationsLibraryTest, testPersistentConnection) {
    std::string file = inputPath + RECOGNIZE_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(CT_RECOGNIZE_EVENT_JSON, file, avsCommon::avs::MessageRequest::Status::SUCCESS,
            std::chrono::seconds(10));
    ASSERT_FALSE(m_connectionStatusObserver->waitFor(ConnectionStatus::DISCONNECTED, std::chrono::seconds(20)))
        << "Connection changed after a response was received";
    setupMessageWithAttachmentAndSend(CT_RECOGNIZE_EVENT_JSON, file, avsCommon::avs::MessageRequest::Status::SUCCESS,
            std::chrono::seconds(10));
}

/**
 * Function that sends multiple events in succession and verifies that the server responds to each of them properly
 */
TEST_F(AlexaCommunicationsLibraryTest, sendEventsInSuccession) {
    const int NUMBER_OF_SUCCESSIVE_SENDS = 10;
    for (int i = 0; i < NUMBER_OF_SUCCESSIVE_SENDS; ++i) {
        std::string file = inputPath + RECOGNIZE_AUDIO_FILE_NAME;
        setupMessageWithAttachmentAndSend(
            CT_RECOGNIZE_EVENT_JSON,
            file,
            avsCommon::avs::MessageRequest::Status::SUCCESS,
            std::chrono::seconds(10));
    }
}

/**
 * Function that sends multiple events concurrently and verifies that the server responds to each of them properly
 */
TEST_F(AlexaCommunicationsLibraryTest, sendConcurrentEvents) {
    int no_of_threads = MAX_TEST_THREADS;
    std::vector<std::future<void>> concurrentTaskFutures;

    for(int i = 0; i < no_of_threads; ++i){
        auto future = std::async(std::launch::async, [this](){ sendEventRandom(); });
        concurrentTaskFutures.push_back(std::move(future));
    }

    for (auto& future : concurrentTaskFutures) {
        auto status = future.wait_for(std::chrono::seconds(20));
        ASSERT_EQ(status, std::future_status::ready);
    }
}

/**
 * Test ability to receive a directive from Alexa Voice Service in response to an event.
 *
 * This test sends a RecognizeAudio event, which requires an attachment of the audio stream.  In this case,
 * we send a pre-recorded audio file which asks Alexa to "tell me a joke".  It then verifies that we receive a
 * directive in response - in this case the aforementioned joke.
 *
 * @see https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/audioplayer#states
 */
TEST_F(AlexaCommunicationsLibraryTest, sendEventWithDirective) {
    std::string file = inputPath + RECOGNIZE_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(CT_RECOGNIZE_EVENT_JSON, file, avsCommon::avs::MessageRequest::Status::SUCCESS,
            std::chrono::seconds(10));

    //expecting to receive directives in response to the recognize event we sent; wait for the first one
    ASSERT_TRUE(m_clientMessageHandler->waitForNext(std::chrono::seconds(20)));
}

/**
 * Test ability to receive a spontaneous directive from Alexa Voice Service.
 *
 * This test is intended to test ACL's ability to receive an unsolicited directive.  To stimulate Alexa Voice Services
 * to send a directive, we send a RecognizeAudio event with silence for the attached audio.  By using the NEAR_FIELD
 * audio profile, we expect AVS to detect the end of speech and send a StopCapture directive - this is effectively an
 * unsolicited directive.
 *
 * @see https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/speechrecognizer#profiles
 */
TEST_F(AlexaCommunicationsLibraryTest, directiveFromDownchannel) {
    std::string file = inputPath + SILENCE_AUDIO_FILE_NAME;
    setupMessageWithAttachmentAndSend(NF_RECOGNIZE_EVENT_JSON, file, avsCommon::avs::MessageRequest::Status::SUCCESS,
            std::chrono::seconds(10));

    //we sent silence, so there is no event, but due to the near-field profile, we expect a stop-capture directive;
    ASSERT_TRUE(m_clientMessageHandler->waitForNext(std::chrono::seconds(20)));
}

} // namespace integration
} // namespace alexaClientSDK

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    if (argc < 3) {
        std::cerr << "USAGE: AlexaCommunicationsLibraryTest <path_to_auth_delgate_config> <path_to_inputs_folder>"
                << std::endl;
        return 1;
    } else {
        alexaClientSDK::integration::configPath = std::string(argv[1]);
        alexaClientSDK::integration::inputPath = std::string(argv[2]);
        return RUN_ALL_TESTS();
    }
}
