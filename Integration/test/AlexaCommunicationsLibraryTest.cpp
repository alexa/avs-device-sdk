/*
 * Copyright 2016-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/AVS/Attachment/InProcessAttachment.h>
#include <CBLAuthDelegate/SQLiteCBLAuthDelegateStorage.h>
#include <ContextManager/ContextManager.h>

#include "Integration/ACLTestContext.h"
#include "Integration/ClientMessageHandler.h"
#include "Integration/ConnectionStatusObserver.h"
#include "Integration/ObservableMessageRequest.h"

namespace alexaClientSDK {
namespace integration {
namespace test {

using namespace acl;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::sds;
using namespace registrationManager;

/// This is a basic synchronize JSON message which may be used to initiate a connection with AVS.
// clang-format off
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
// clang-format on

/// This is a partial JSON string that should not be parseable.
static const std::string BAD_SYNCHRONIZE_STATE_JSON = "{";

/**
 * This string specifies a Recognize event using the specified profile.
 *
 * CLOSE_TALK performs end-of-speech detection on the client, so a stop-capture directive will not be received from AVS.
 * NEAR_FIELD performs end-of-speech detection in AVS, so a stop-capture directive will be received from AVS.
 */
// clang-format off
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
// clang-format on

/// This string specifies a Recognize event using the CLOSE_TALK profile.
static const std::string CT_RECOGNIZE_EVENT_JSON = RECOGNIZE_EVENT_JSON(CLOSE_TALK);
/// This string specifies a Recognize event using the NEAR_FIELD profile.
static const std::string NF_RECOGNIZE_EVENT_JSON = RECOGNIZE_EVENT_JSON(NEAR_FIELD);

/// This string specifies an ExpectSpeechTimedOut event.
// clang-format off
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
// clang-format on

/// This is a 16 bit 16 kHz little endian linear PCM audio file containing a recognized message for AVS
static const std::string RECOGNIZE_AUDIO_FILE_NAME = "recognize_test.wav";
/// This is a 16 bit 16 kHz little endian linear PCM audio file containing several seconds of silence
static const std::string SILENCE_AUDIO_FILE_NAME = "silence_test.wav";

/**
 * The value of MAX_THREADS is determined by the maximum number of streams we can have active at once which is defined
 * in HTTP2Connection.cpp as a constant MAX_STREAMS=10. Streams include events, the downchannel and ping.
 * Since we establish a downchannel when we connect, we can only have (MAX_STREAMS - 1) events sent at once.
 * Therefore, the value of MAX_THREADS = MAX_STREAMS -1.
 */
static const int MAX_CONCURRENT_STREAMS = 9;

/// Path to the AlexaClientSDKConfig.json file (from command line arguments).
static std::string g_configPath;
/// Path to resources (e.g. audio files) for tests (from command line arguments).
static std::string g_inputPath;

class AlexaCommunicationsLibraryTest : public ::testing::Test {
public:
    void SetUp() override {
        m_context = ACLTestContext::create(g_configPath);
        ASSERT_TRUE(m_context);

        m_clientMessageHandler = std::make_shared<ClientMessageHandler>(m_context->getAttachmentManager());
        m_avsConnectionManager = AVSConnectionManager::create(
            m_context->getMessageRouter(), false, {m_context->getConnectionStatusObserver()}, {m_clientMessageHandler});
        ASSERT_TRUE(m_avsConnectionManager);

        connect();
    }

    void TearDown() override {
        // Note nullptr checks needed to avoid segaults if @c SetUp() failed.
        if (m_avsConnectionManager) {
            disconnect();
            m_avsConnectionManager->shutdown();
        }
        m_context.reset();
    }

    void connect() {
        m_avsConnectionManager->enable();
        m_context->waitForConnected();
    }

    void disconnect() {
        if (m_avsConnectionManager) {
            m_avsConnectionManager->disable();
            m_context->waitForDisconnected();
        }
    }

    /*
     * Function to send an Event to AVS.
     */
    void sendEvent(
        const std::string& jsonContent,
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status expectedStatus,
        std::chrono::seconds timeout,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader = nullptr) {
        auto messageRequest = std::make_shared<ObservableMessageRequest>(jsonContent, attachmentReader);

        m_avsConnectionManager->sendMessage(messageRequest);
        ASSERT_TRUE(messageRequest->waitFor(expectedStatus, timeout));
        ASSERT_TRUE(messageRequest->hasSendCompleted() || messageRequest->wasExceptionReceived());
    }

    /**
     * Function to create an attachmentReader given a filename.
     */
    std::shared_ptr<InProcessAttachmentReader> createAttachmentReader(const std::string& fileName) {
        // 1MB is large enough for our test audio samples.
        const int mbBytes = 1024 * 1024;

        // create an SDS with 1MB size, so we can write the entire audio file into it.
        auto sdsBufferSize = InProcessSDS::calculateBufferSize(mbBytes);
        auto sdsBuffer = std::make_shared<InProcessSDSTraits::Buffer>(sdsBufferSize);
        std::shared_ptr<InProcessSDS> sds = InProcessSDS::create(sdsBuffer);

        // open the file.
        auto is = std::make_shared<std::ifstream>(fileName, std::ios::binary);
        EXPECT_TRUE(is->is_open());

        // read the data from the file into the SDS, via the local buffer.
        std::vector<char> localBuffer(mbBytes);
        auto attachmentWriter = InProcessAttachmentWriter::create(sds);
        while (*is) {
            // data -> local buffer
            is->read(localBuffer.data(), mbBytes);
            size_t numBytesRead = static_cast<size_t>(is->gcount());

            // local buffer -> sds
            AttachmentWriter::WriteStatus writeStatus = AttachmentWriter::WriteStatus::OK;
            attachmentWriter->write(localBuffer.data(), numBytesRead, &writeStatus);

            // confirm the SDS write operation went ok
            bool writeStatusOk =
                (AttachmentWriter::WriteStatus::OK == writeStatus ||
                 AttachmentWriter::WriteStatus::CLOSED == writeStatus);
            EXPECT_TRUE(writeStatusOk);
        }

        // closing the writer indicates to readers of the SDS that there is no more data that will be written.
        attachmentWriter->close();

        // create and return the reader.
        std::shared_ptr<InProcessAttachmentReader> attachmentReader =
            InProcessAttachmentReader::create(ReaderPolicy::NONBLOCKING, sds);
        EXPECT_NE(attachmentReader, nullptr);

        return attachmentReader;
    }

    /**
     * This function sends an Event to AVS, deciding to include an audio attachment based upon a random factor.
     */
    void sendRandomEvent() {
        std::shared_ptr<InProcessAttachmentReader> attachmentReader;
        std::string eventJson = SYNCHRONIZE_STATE_JSON;
        auto status = avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT;

        if (rand() % 2) {
            eventJson = CT_RECOGNIZE_EVENT_JSON;
            attachmentReader = createAttachmentReader(g_inputPath + "/" + RECOGNIZE_AUDIO_FILE_NAME);
            status = avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS;
        }

        sendEvent(eventJson, status, std::chrono::seconds(40), attachmentReader);
    }

    /// Context for running ACL based tests.
    std::unique_ptr<ACLTestContext> m_context;

    std::shared_ptr<ClientMessageHandler> m_clientMessageHandler;
    std::shared_ptr<AVSConnectionManager> m_avsConnectionManager;
};

/// Test connecting and disconnecting from AVS.
TEST_F(AlexaCommunicationsLibraryTest, testConnectAndDisconnect) {
    // Connect is called in SetUp and disconnect is called in TearDown. Simply check that we are connected.
    ASSERT_TRUE(m_avsConnectionManager->isConnected());
}

/**
 * Test sending an Event to AVS.
 *
 * This test sends a SynchronizeState Event, which does not require an attachment, or receive a Directive in response.
 *
 * @see https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/system#synchronizestate
 */
TEST_F(AlexaCommunicationsLibraryTest, testSendEvent) {
    sendEvent(
        SYNCHRONIZE_STATE_JSON,
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT,
        std::chrono::seconds(10));
}

/**
 * Function that tests the behavior of the ACL when an improperly formatted message is sent, expecting the server
 * to return a bad request status.
 */
TEST_F(AlexaCommunicationsLibraryTest, testSendInvalidEvent) {
    sendEvent(
        BAD_SYNCHRONIZE_STATE_JSON,
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::BAD_REQUEST,
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
TEST_F(AlexaCommunicationsLibraryTest, testSendEventWithAttachment) {
    auto attachmentReader = createAttachmentReader(g_inputPath + "/" + RECOGNIZE_AUDIO_FILE_NAME);
    sendEvent(
        CT_RECOGNIZE_EVENT_JSON,
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS,
        std::chrono::seconds(10),
        attachmentReader);
}

/**
 * Test sending an Event and receiving a Directive in response.
 *
 * This test sends a Recognize event, which includes an audio attachment.  In this case, the audio file sent to AVS
 * asks Alexa to "tell me a joke".  The Speak Directive we expect in response will be the joke.
 *
 * @see https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/audioplayer#states
 */
TEST_F(AlexaCommunicationsLibraryTest, testSendEventAndReceiveDirective) {
    auto attachmentReader = createAttachmentReader(g_inputPath + "/" + RECOGNIZE_AUDIO_FILE_NAME);
    sendEvent(
        CT_RECOGNIZE_EVENT_JSON,
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS,
        std::chrono::seconds(10),
        attachmentReader);

    // We expect to receive Directives in response to the recognize Event.  Wait for the first one.
    ASSERT_TRUE(m_clientMessageHandler->waitForNext(std::chrono::seconds(20)));
}

/**
 * Test sending multiple Events in succession and verify that AVS responds to each of them.
 */
TEST_F(AlexaCommunicationsLibraryTest, testSendEventsSerially) {
    const int NUMBER_OF_SUCCESSIVE_SENDS = 10;
    for (int i = 0; i < NUMBER_OF_SUCCESSIVE_SENDS; ++i) {
        auto attachmentReader = createAttachmentReader(g_inputPath + "/" + RECOGNIZE_AUDIO_FILE_NAME);
        sendEvent(
            CT_RECOGNIZE_EVENT_JSON,
            avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS,
            std::chrono::seconds(10),
            attachmentReader);
    }
}

/**
 * Test sending multiple Events concurrently and verify that AVS responds to each of them.
 */
TEST_F(AlexaCommunicationsLibraryTest, testSendEventsConcurrently) {
    std::vector<std::future<void>> futures;

    for (int i = 0; i < MAX_CONCURRENT_STREAMS; ++i) {
        auto future = std::async(std::launch::async, [this]() { sendRandomEvent(); });
        futures.push_back(std::move(future));
    }

    for (auto& future : futures) {
        auto status = future.wait_for(std::chrono::seconds(20));
        ASSERT_EQ(status, std::future_status::ready);
    }
}

/**
 * Test receiving an AVS Directive on the DownChannel.
 *
 * DownChannel Directives are unsolicited - they may be pushed to the client by AVS at any time.
 * This test exercises this functionality by sending a RecognizeAudio Event with silence for the attached audio.
 * By using the NEAR_FIELD audio profile, we expect AVS to detect the end of speech and send a StopCapture Directive.
 *
 * @see https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/speechrecognizer#profiles
 */
TEST_F(AlexaCommunicationsLibraryTest, testReceiveDirectiveOnDownchannel) {
    auto attachmentReader = createAttachmentReader(g_inputPath + "/" + SILENCE_AUDIO_FILE_NAME);
    sendEvent(
        NF_RECOGNIZE_EVENT_JSON,
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT,
        std::chrono::seconds(10),
        attachmentReader);

    // Wait for the StopCapture Directive to be received.
    ASSERT_TRUE(m_clientMessageHandler->waitForNext(std::chrono::seconds(20)));
}

/**
 * Test that a connection to AVS persists between sending Events.
 */
TEST_F(AlexaCommunicationsLibraryTest, testPersistentConnection) {
    auto attachmentReader = createAttachmentReader(g_inputPath + "/" + RECOGNIZE_AUDIO_FILE_NAME);
    sendEvent(
        CT_RECOGNIZE_EVENT_JSON,
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS,
        std::chrono::seconds(10),
        attachmentReader);
    ASSERT_FALSE(m_context->getConnectionStatusObserver()->waitFor(
        ConnectionStatusObserverInterface::Status::DISCONNECTED, std::chrono::seconds(20)))
        << "Connection changed after a response was received";
    sendEvent(
        CT_RECOGNIZE_EVENT_JSON,
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT,
        std::chrono::seconds(10),
        attachmentReader);
}

/**
 * Test add- and removeConnectionStatuObserver, expecting the observer to be updated only when it is added.
 */
TEST_F(AlexaCommunicationsLibraryTest, testMultipleConnectionStatusObservers) {
    auto observer = std::make_shared<ConnectionStatusObserver>();
    m_avsConnectionManager->addConnectionStatusObserver(observer);

    ASSERT_TRUE(observer->waitFor(ConnectionStatusObserverInterface::Status::CONNECTED));

    // Remove the observer and disconnect, expecting the status to not change.
    m_avsConnectionManager->removeConnectionStatusObserver(observer);
    disconnect();
    ASSERT_EQ(observer->getConnectionStatus(), ConnectionStatusObserverInterface::Status::CONNECTED);
    ASSERT_TRUE(
        m_context->getConnectionStatusObserver()->waitFor(ConnectionStatusObserverInterface::Status::DISCONNECTED));
}
}  // namespace test
}  // namespace integration
}  // namespace alexaClientSDK

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    if (argc < 3) {
        std::cerr << "USAGE: " << std::string(argv[0]) << " <path_to_auth_delgate_config> <path_to_inputs_folder>"
                  << std::endl;
        return 1;
    } else {
        alexaClientSDK::integration::test::g_configPath = std::string(argv[1]);
        alexaClientSDK::integration::test::g_inputPath = std::string(argv[2]);
        return RUN_ALL_TESTS();
    }
}
