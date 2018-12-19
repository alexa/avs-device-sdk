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

/// @file AudioInputProcessorTest.cpp

#include <cstring>
#include <climits>
#include <numeric>
#include <sstream>
#include <vector>
#include <gtest/gtest.h>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <AVSCommon/SDKInterfaces/MockDirectiveSequencer.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockFocusManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockUserInactivityMonitor.h>
#include <AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h>
#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Memory/Memory.h>

#include "AIP/AudioInputProcessor.h"
#include "MockObserver.h"

using namespace alexaClientSDK::avsCommon::avs;
using namespace alexaClientSDK::avsCommon::utils::json;
using namespace testing;

namespace alexaClientSDK {
namespace capabilityAgents {
namespace aip {
namespace test {

using avsCommon::sdkInterfaces::AudioInputProcessorObserverInterface;

/// The name of the @c FocusManager channel used by @c AudioInputProvider.
static const std::string CHANNEL_NAME = avsCommon::sdkInterfaces::FocusManagerInterface::DIALOG_CHANNEL_NAME;

/// The namespace for this capability agent.
static const std::string NAMESPACE = "SpeechRecognizer";

/// The StopCapture directive signature.
static const avsCommon::avs::NamespaceAndName STOP_CAPTURE{NAMESPACE, "StopCapture"};

/// The ExpectSpeech directive signature.
static const avsCommon::avs::NamespaceAndName EXPECT_SPEECH{NAMESPACE, "ExpectSpeech"};

/// The SetEndOfSpeechOffset directive signature.
static const avsCommon::avs::NamespaceAndName SET_END_OF_SPEECH_OFFSET{NAMESPACE, "SetEndOfSpeechOffset"};

/// The directives @c AudioInputProcessor should handle.
avsCommon::avs::NamespaceAndName DIRECTIVES[] = {STOP_CAPTURE, EXPECT_SPEECH, SET_END_OF_SPEECH_OFFSET};

/// The SpeechRecognizer context state signature.
static const avsCommon::avs::NamespaceAndName RECOGNIZER_STATE{NAMESPACE, "RecognizerState"};

/// Sample rate for audio input stream.
static const unsigned int SAMPLE_RATE_HZ = 16000;

/// Integral type of a sample.
using Sample = uint16_t;

/// Number of bytes per word in the SDS circular buffer.
static const size_t SDS_WORDSIZE = sizeof(Sample);

/// Sample size for audio input stream.
static const unsigned int SAMPLE_SIZE_IN_BITS = SDS_WORDSIZE * CHAR_BIT;

/// Number of channels in audio input stream.
static const unsigned int NUM_CHANNELS = 1;

/// Number of milliseconds of preroll applied for cloud-based wakeword verification.
static const std::chrono::milliseconds PREROLL_MS = std::chrono::milliseconds(500);

/// Number of words of preroll.
static const size_t PREROLL_WORDS = (SAMPLE_RATE_HZ / 1000) * PREROLL_MS.count();

/// Number of words in wakeword.
static const size_t WAKEWORD_WORDS = PREROLL_WORDS;

/// Number of words to hold in the SDS circular buffer.
static const size_t SDS_WORDS = PREROLL_WORDS + WAKEWORD_WORDS + 1000;

/// Number of test pattern words to write to the SDS circular buffer.
static const size_t PATTERN_WORDS = SDS_WORDS / 2;

/// Maximum number of readers to support in the SDS circular buffer.
static const size_t SDS_MAXREADERS = 3;

/// Boolean value to indicate an AudioProvider is always readable.
static const bool ALWAYS_READABLE = true;

/// Boolean value to indicate an AudioProvider can override another AudioProvider.
static const bool CAN_OVERRIDE = true;

/// Boolean value to indicate an AudioProvider can be overridden by another AudioProvider.
static const bool CAN_BE_OVERRIDDEN = true;

/// JSON key for the context section of a message.
static const std::string MESSAGE_CONTEXT_KEY = "context";

/// JSON key for the event section of a message.
static const std::string MESSAGE_EVENT_KEY = "event";

/// JSON key for the directive section of a message.
static const std::string MESSAGE_DIRECTIVE_KEY = "directive";

/// JSON key for the header section of a message.
static const std::string MESSAGE_HEADER_KEY = "header";

/// JSON key for the payload section of an message.
static const std::string MESSAGE_PAYLOAD_KEY = "payload";

/// JSON key for the namespace field of a message header.
static const std::string MESSAGE_NAMESPACE_KEY = "namespace";

/// JSON key for the name field of a message header.
static const std::string MESSAGE_NAME_KEY = "name";

/// JSON key for the message ID field of a message header.
static const std::string MESSAGE_MESSAGE_ID_KEY = "messageId";

/// JSON key for the dialog request ID field of a message header.
static const std::string MESSAGE_DIALOG_REQUEST_ID_KEY = "dialogRequestId";

/// JSON value for a recognize event's name.
static const std::string RECOGNIZE_EVENT_NAME = "Recognize";

/// JSON key for the ASR profile field of a recognize event.
static const std::string ASR_PROFILE_KEY = "profile";

/// JSON key for the audio format field of a recognize event.
static const std::string AUDIO_FORMAT_KEY = "format";

/// Accepted JSON values for a recognize event's audio format.
static const std::unordered_set<std::string> AUDIO_FORMAT_VALUES = {"AUDIO_L16_RATE_16000_CHANNELS_1", "OPUS"};

/// JSON key for the initiator field of a recognize event.
static const std::string RECOGNIZE_INITIATOR_KEY = "initiator";

/// JSON key for the type field of a recognize event's initiator.
static const std::string INITIATOR_TYPE_KEY = "type";

/// JSON key for the type field of a recognize event's payload.
static const std::string INITIATOR_PAYLOAD_KEY = "payload";

/// JSON key for the wakeword indices field of a wakeword recognize event's payload.
static const std::string WAKE_WORD_INDICES_KEY = "wakeWordIndices";

/// JSON key for the start index field of a wakeword recognize event's payload.
static const std::string START_INDEX_KEY = "startIndexInSamples";

/// JSON key for the end index field of a wakeword recognize event's payload.
static const std::string END_INDEX_KEY = "endIndexInSamples";

/// Value used in the tests for an expect speech initiator.
static const std::string EXPECT_SPEECH_INITIATOR = R"({"opaque":"expectSpeechInitiator"})";

/// JSON key for the timeout field of an expect speech directive.
static const std::string EXPECT_SPEECH_TIMEOUT_KEY = "timeoutInMilliseconds";

/// Value used in the tests for an expect speech timeout.
static const int64_t EXPECT_SPEECH_TIMEOUT_IN_MILLISECONDS = 100;

/// JSON key for the initiator field of an expect speech directive.
static const std::string EXPECT_SPEECH_INITIATOR_KEY = "initiator";

/// JSON value for a expect speech timed out event's name.
static const std::string EXPECT_SPEECH_TIMED_OUT_EVENT_NAME = "ExpectSpeechTimedOut";

/// Value used in the tests for a wakeword recognize event.
static const std::string KEYWORD_TEXT = "ALEXA";

/// Boolean value to indicate a directive being tested has a dialog request ID.
static const bool WITH_DIALOG_REQUEST_ID = true;

/// Boolean value to indicate an expect speech directive should time out.
static const bool VERIFY_TIMEOUT = true;

/// General timeout for tests to fail.
static const std::chrono::seconds TEST_TIMEOUT(10);

/// JSON value for a ReportEchoSpatialPerceptionData event's name.
static const std::string ESP_EVENT_NAME = "ReportEchoSpatialPerceptionData";

/// JSON key for the voice energy field of a ReportEchoSpatialPerceptionData event.
static const std::string ESP_VOICE_ENERGY_KEY = "voiceEnergy";

/// JSON key for the ambient energy field of a ReportEchoSpatialPerceptionData event.
static const std::string ESP_AMBIENT_ENERGY_KEY = "ambientEnergy";

/// The field name for the user voice attachment.
static const std::string AUDIO_ATTACHMENT_FIELD_NAME = "audio";

/// The field name for the wake word engine metadata.
static const std::string KWD_METADATA_FIELD_NAME = "wakewordEngineMetadata";

/// The field name for the wake word detected.
static const std::string WAKEWORD_FIELD_NAME = "wakeWord";

/// The field name for the end of speech offset, reported in milliseconds.
/// This field comes in the payload of the SetEndOfSpeechOffset directive.
static const std::string END_OF_SPEECH_OFFSET_FIELD_NAME = "endOfSpeechOffsetInMilliseconds";

/// Value used in the tests for an end of speech offset
static const int64_t END_OF_SPEECH_OFFSET_IN_MILLISECONDS = 1526;

/// The field name for the start of speech timestamp. It is sent during Recognize event and received as part of
/// SetEndOfSpeechOffset directive.
static const std::string START_OF_SPEECH_TIMESTAMP_FIELD_NAME = "startOfSpeechTimestamp";

/// Value used in the tests for an start of speech timestamp.
static const auto START_OF_SPEECH_TIMESTAMP = std::chrono::steady_clock::now();

/// String value used for start of speech timestamp string representation.
static const auto START_OF_SPEECH_TIMESTAMP_STR = std::to_string(START_OF_SPEECH_TIMESTAMP.time_since_epoch().count());

/// The index of the Wakeword engine metadata in the @c MessageRequest.
static const size_t MESSAGE_ATTACHMENT_KWD_METADATA_INDEX = 0;

/// Sample Wakeword engine metadata to compare with the @ AttachmentReader
static const std::string KWD_METADATA_EXAMPLE = "Wakeword engine metadata example";

/// Utility function to parse a JSON document.
static rapidjson::Document parseJson(const std::string& json) {
    rapidjson::Document document;
    document.Parse(json);
    EXPECT_FALSE(document.HasParseError())
        << "rapidjson detected a parsing error at offset:" + std::to_string(document.GetErrorOffset()) +
               ", error message: " + GetParseError_En(document.GetParseError());
    return document;
}

/// Utility function to look up a JSON string in a container.
static std::string getJsonString(const rapidjson::Value& container, const std::string& key) {
    auto member = container.FindMember(key);
    EXPECT_TRUE(member->value.IsString());
    if (!member->value.IsString()) {
        return "";
    }
    return member->value.GetString();
}

/// Utility function to look up a JSON 'long' in a container.
static int64_t getJsonInt64(const rapidjson::Value& container, const std::string& key) {
    auto member = container.FindMember(key);
    EXPECT_TRUE(member->value.IsInt64());
    if (!member->value.IsInt64()) {
        return 0;
    }
    return member->value.GetInt64();
}

/**
 * Utility class which captures parameters to a Recognize event, and provides functions to send and verify the event
 * using those parameters.
 */
class RecognizeEvent {
public:
    /**
     * Constructs an object which captures the parameters to send in a Recognize Event.  Parameters are passed through
     * directly to @c AudioInputProcessor::recognize().
     */
    RecognizeEvent(
        AudioProvider audioProvider,
        Initiator initiator,
        avsCommon::avs::AudioInputStream::Index begin = AudioInputProcessor::INVALID_INDEX,
        avsCommon::avs::AudioInputStream::Index keywordEnd = AudioInputProcessor::INVALID_INDEX,
        std::string keyword = "",
        std::shared_ptr<std::string> avsInitiator = nullptr,
        const ESPData espData = ESPData::getEmptyESPData(),
        const std::shared_ptr<std::vector<char>> KWDMetadata = nullptr);

    /**
     * This function sends a recognize event using the provided @c AudioInputProcessor and the recognize parameters
     * captured by this instance.
     *
     * @param audioInputProcessor The @c AudioInputProcessor to call @c AudioInputProcessor::recognize() on.
     * @return A future which is @c true if the call succeeded, else @c false.
     */
    std::future<bool> send(std::shared_ptr<AudioInputProcessor> audioInputProcessor);

    /**
     * This function verifies that the JSON state string is correct and matches the captured parameters.  This function
     * signature matches that of @c ContextManager::setState() so that an @c EXPECT_CALL() can @c Invoke() this
     * function directly, but only the jsonState parameter is verified by this function.
     *
     * @param jsonState The JSON context state string to verify.
     */
    void verifyJsonState(
        const avsCommon::avs::NamespaceAndName&,
        const std::string& jsonState,
        const avsCommon::avs::StateRefreshPolicy&,
        const unsigned int);

    /*
     * This function verifies that JSON content of a ReportEchoSpatialPerceptionData @c MessageRequest is correct.
     *
     * @param request The @c MessageRequest to verify.
     * @param dialogRequestId The expected dialogRequestId in the @c MessageRequest.
     */
    void verifyEspMessage(std::shared_ptr<avsCommon::avs::MessageRequest> request, const std::string& dialogRequestId);

    /**
     * This function verifies the metadata @c AttachmentReader created is correct
     *
     * @param request The @c MessageRequest to verify
     * @param KWDMetadata The Wakeword engine metadata recevied by @c AudioInputProcessor::recognize
     */
    void verifyMetadata(
        const std::shared_ptr<avsCommon::avs::MessageRequest> request,
        const std::shared_ptr<std::vector<char>> KWDMetadata);

    /**
     * This function verifies that JSON content of a recognize @c MessageRequest is correct, and that it has an
     * attachment.
     *
     * @param request The @c MessageRequest to verify.
     * @param pattern Vector of samples holding a test pattern expected from the @c AudioInputStream.
     * @param dialogRequestId The expected dialogRequestId in the @c MessageRequest.
     */
    void verifyMessage(
        std::shared_ptr<avsCommon::avs::MessageRequest> request,
        const std::vector<Sample>& pattern,
        const std::string& dialogRequestId);

    /**
     * Accessor function to get the attachment reader for a verified message.
     *
     * @return the attachment reader for a verified message.
     */
    std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> getReader();

private:
    /// The audio provider to use for this recognize event.
    AudioProvider m_audioProvider;

    /// The initiator to use for this recognize event.
    Initiator m_initiator;

    /// The begin index to use for this recognize event.
    avsCommon::avs::AudioInputStream::Index m_begin;

    /// The keyword end index to use for this recognize event.
    avsCommon::avs::AudioInputStream::Index m_keywordEnd;

    /// The keyword string to use for this recognize event.
    std::string m_keyword;

    /// The initiator that is passed from AVS in a preceding ExpectSpeech.
    std::shared_ptr<std::string> m_avsInitiator;

    /// The ESP data for this ReportEchoSpatialPerceptionData event.
    const ESPData m_espData;

    /// The user voice attachment reader saved by a call to @c verifyMessage().
    std::shared_ptr<avsCommon::avs::MessageRequest::NamedReader> m_reader;

    /// The wake word engine metadata attachment reader saved by a call to @c verifyMessage().
    std::shared_ptr<std::vector<char>> m_KWDMetadata;
};

RecognizeEvent::RecognizeEvent(
    AudioProvider audioProvider,
    Initiator initiator,
    avsCommon::avs::AudioInputStream::Index begin,
    avsCommon::avs::AudioInputStream::Index keywordEnd,
    std::string keyword,
    std::shared_ptr<std::string> avsInitiator,
    const ESPData espData,
    const std::shared_ptr<std::vector<char>> KWDMetadata) :
        m_audioProvider{audioProvider},
        m_initiator{initiator},
        m_begin{begin},
        m_keywordEnd{keywordEnd},
        m_keyword{keyword},
        m_avsInitiator{avsInitiator},
        m_espData{espData},
        m_KWDMetadata{KWDMetadata} {
}

std::future<bool> RecognizeEvent::send(std::shared_ptr<AudioInputProcessor> audioInputProcessor) {
    auto result = audioInputProcessor->recognize(
        m_audioProvider,
        m_initiator,
        START_OF_SPEECH_TIMESTAMP,
        m_begin,
        m_keywordEnd,
        m_keyword,
        m_espData,
        m_KWDMetadata);
    EXPECT_TRUE(result.valid());
    return result;
}

void RecognizeEvent::verifyEspMessage(
    std::shared_ptr<avsCommon::avs::MessageRequest> request,
    const std::string& dialogRequestId) {
    rapidjson::Document document;
    document.Parse(request->getJsonContent().c_str());
    EXPECT_FALSE(document.HasParseError())
        << "rapidjson detected a parsing error at offset:" + std::to_string(document.GetErrorOffset()) +
               ", error message: " + GetParseError_En(document.GetParseError());

    auto event = document.FindMember(MESSAGE_EVENT_KEY);
    EXPECT_NE(event, document.MemberEnd());

    auto header = event->value.FindMember(MESSAGE_HEADER_KEY);
    EXPECT_NE(header, event->value.MemberEnd());
    auto payload = event->value.FindMember(MESSAGE_PAYLOAD_KEY);
    EXPECT_NE(payload, event->value.MemberEnd());

    EXPECT_EQ(getJsonString(header->value, MESSAGE_NAMESPACE_KEY), NAMESPACE);
    EXPECT_EQ(getJsonString(header->value, MESSAGE_NAME_KEY), ESP_EVENT_NAME);
    EXPECT_NE(getJsonString(header->value, MESSAGE_MESSAGE_ID_KEY), "");
    EXPECT_EQ(getJsonString(header->value, MESSAGE_DIALOG_REQUEST_ID_KEY), dialogRequestId);

    EXPECT_EQ(std::to_string(getJsonInt64(payload->value, ESP_VOICE_ENERGY_KEY)), m_espData.getVoiceEnergy());
    EXPECT_EQ(std::to_string(getJsonInt64(payload->value, ESP_AMBIENT_ENERGY_KEY)), m_espData.getAmbientEnergy());
}

void RecognizeEvent::verifyMetadata(
    const std::shared_ptr<avsCommon::avs::MessageRequest> request,
    const std::shared_ptr<std::vector<char>> KWDMetadata) {
    if (!KWDMetadata) {
        EXPECT_EQ(request->attachmentReadersCount(), 1);
    } else {
        char buffer[50];
        auto readStatus = avsCommon::avs::attachment::AttachmentReader::ReadStatus::OK;

        EXPECT_EQ(request->attachmentReadersCount(), 2);
        EXPECT_NE(request->getAttachmentReader(MESSAGE_ATTACHMENT_KWD_METADATA_INDEX), nullptr);
        auto bytesRead = request->getAttachmentReader(MESSAGE_ATTACHMENT_KWD_METADATA_INDEX)
                             ->reader->read(buffer, KWD_METADATA_EXAMPLE.length(), &readStatus);

        EXPECT_EQ(bytesRead, KWD_METADATA_EXAMPLE.length());
        EXPECT_EQ(memcmp(buffer, KWD_METADATA_EXAMPLE.data(), KWD_METADATA_EXAMPLE.length()), 0);
    }
}
void RecognizeEvent::verifyMessage(
    std::shared_ptr<avsCommon::avs::MessageRequest> request,
    const std::vector<Sample>& pattern,
    const std::string& dialogRequestId) {
    rapidjson::Document document;
    document.Parse(request->getJsonContent().c_str());
    EXPECT_FALSE(document.HasParseError())
        << "rapidjson detected a parsing error at offset:" + std::to_string(document.GetErrorOffset()) +
               ", error message: " + GetParseError_En(document.GetParseError());

    auto context = document.FindMember(MESSAGE_CONTEXT_KEY);
    EXPECT_NE(context, document.MemberEnd());
    auto event = document.FindMember(MESSAGE_EVENT_KEY);
    EXPECT_NE(event, document.MemberEnd());

    auto header = event->value.FindMember(MESSAGE_HEADER_KEY);
    EXPECT_NE(header, event->value.MemberEnd());
    auto payload = event->value.FindMember(MESSAGE_PAYLOAD_KEY);
    EXPECT_NE(payload, event->value.MemberEnd());

    EXPECT_EQ(getJsonString(header->value, MESSAGE_NAMESPACE_KEY), NAMESPACE);
    EXPECT_EQ(getJsonString(header->value, MESSAGE_NAME_KEY), RECOGNIZE_EVENT_NAME);
    EXPECT_NE(getJsonString(header->value, MESSAGE_MESSAGE_ID_KEY), "");
    EXPECT_EQ(getJsonString(header->value, MESSAGE_DIALOG_REQUEST_ID_KEY), dialogRequestId);

    std::ostringstream profile;
    profile << m_audioProvider.profile;

    std::ostringstream encodingFormat;
    encodingFormat << m_audioProvider.format.encoding;

    EXPECT_EQ(getJsonString(payload->value, ASR_PROFILE_KEY), profile.str());
    EXPECT_EQ(getJsonString(payload->value, START_OF_SPEECH_TIMESTAMP_FIELD_NAME), START_OF_SPEECH_TIMESTAMP_STR);

    EXPECT_FALSE(
        AUDIO_FORMAT_VALUES.find(getJsonString(payload->value, AUDIO_FORMAT_KEY)) == AUDIO_FORMAT_VALUES.end());
    auto initiator = payload->value.FindMember(RECOGNIZE_INITIATOR_KEY);
    EXPECT_NE(initiator, payload->value.MemberEnd());

    if (m_avsInitiator) {
        std::string initiatorString;
        EXPECT_TRUE(jsonUtils::convertToValue(initiator->value, &initiatorString));
        EXPECT_EQ(initiatorString, *m_avsInitiator);
    } else {
        EXPECT_EQ(getJsonString(initiator->value, INITIATOR_TYPE_KEY), initiatorToString(m_initiator));
        auto initiatorPayload = initiator->value.FindMember(INITIATOR_PAYLOAD_KEY);
        EXPECT_NE(initiatorPayload, initiator->value.MemberEnd());

        if (m_initiator == Initiator::WAKEWORD) {
            if (m_begin != AudioInputProcessor::INVALID_INDEX && m_keywordEnd != AudioInputProcessor::INVALID_INDEX) {
                auto wakeWordIndices = initiatorPayload->value.FindMember(WAKE_WORD_INDICES_KEY);
                EXPECT_NE(wakeWordIndices, initiatorPayload->value.MemberEnd());

                if (wakeWordIndices != initiatorPayload->value.MemberEnd()) {
                    EXPECT_EQ(getJsonInt64(wakeWordIndices->value, START_INDEX_KEY), static_cast<int64_t>(m_begin));
                    EXPECT_EQ(getJsonInt64(wakeWordIndices->value, END_INDEX_KEY), static_cast<int64_t>(m_keywordEnd));
                }
            }
            EXPECT_EQ(getJsonString(initiatorPayload->value, WAKEWORD_FIELD_NAME), KEYWORD_TEXT);
        }
    }

    m_reader = request->getAttachmentReader(request->attachmentReadersCount() - 1);
    EXPECT_NE(m_reader, nullptr);
    EXPECT_EQ(m_reader->name, AUDIO_ATTACHMENT_FIELD_NAME);

    std::vector<Sample> samples(PATTERN_WORDS);
    size_t samplesRead = 0;
    auto t0 = std::chrono::steady_clock::now();
    do {
        avsCommon::avs::attachment::AttachmentReader::ReadStatus status;
        auto bytesRead = m_reader->reader->read(
            samples.data() + samplesRead, (samples.size() - samplesRead) * SDS_WORDSIZE, &status);
        if (avsCommon::avs::attachment::AttachmentReader::ReadStatus::OK_WOULDBLOCK == status) {
            std::this_thread::yield();
            continue;
        }
        EXPECT_EQ(status, avsCommon::avs::attachment::AttachmentReader::ReadStatus::OK);
        EXPECT_GT(bytesRead, 0u);
        EXPECT_EQ(bytesRead % 2, 0u);
        samplesRead += bytesRead / 2;
    } while (samplesRead < samples.size() && t0 - std::chrono::steady_clock::now() < TEST_TIMEOUT);
    EXPECT_EQ(samplesRead, samples.size());
    EXPECT_EQ(samples, pattern);
}

std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> RecognizeEvent::getReader() {
    return m_reader->reader;
}

/// Class to monitor DialogUXStateAggregator for the @c THINKING state and automatically move it to @c IDLE.
class TestDialogUXStateObserver : public avsCommon::sdkInterfaces::DialogUXStateObserverInterface {
public:
    /**
     * Constructor
     *
     * @param aggregator The @c DialogUXStateAggregator to move from @c THINKING to @c IDLE.
     */
    TestDialogUXStateObserver(std::shared_ptr<avsCommon::avs::DialogUXStateAggregator> aggregator);

    void onDialogUXStateChanged(DialogUXState newState) override;

private:
    /// The @c DialogUXStateAggregator to move from @c THINKING to @c IDLE.
    std::shared_ptr<avsCommon::avs::DialogUXStateAggregator> m_aggregator;
};

TestDialogUXStateObserver::TestDialogUXStateObserver(
    std::shared_ptr<avsCommon::avs::DialogUXStateAggregator> aggregator) :
        m_aggregator(aggregator) {
}

void TestDialogUXStateObserver::onDialogUXStateChanged(DialogUXState newState) {
    if (DialogUXState::THINKING == newState) {
        m_aggregator->receive("", "");
    }
}

/// Test harness for @c AudioInputProcessor class.
class AudioInputProcessorTest : public ::testing::Test {
public:
    /// Set up the test harness for running a test.
    void SetUp() override;

    /// Clean up the test harness after running a test.
    void TearDown() override;

protected:
    /// Enumerate different points to call @c stopCapture() during @c testRecognizeSucceeds().
    enum class RecognizeStopPoint {
        /// Call @c stopCapture() immediately after the @c recognize() call.
        AFTER_RECOGNIZE,
        /// Call @c stopCapture() immediately after the @c onContextAvailable() call.
        AFTER_CONTEXT,
        /// Call @c stopCapture() immediately after the @c onFocusChanged() call.
        AFTER_FOCUS,
        /// Call @c stopCapture() immediately after the message is sent.
        AFTER_SEND,
        /// Do not call @c stopCapture() during the test.
        NONE
    };

    /// Enumerates the different points when to pass a stop capture directive to AIP via @c
    /// AudioInputProcessor::handleDirectiveImmediately())
    enum class StopCaptureDirectiveSchedule {
        /// Pass a stop capture directive to AIP before the event stream is closed.
        BEFORE_EVENT_STREAM_CLOSE,
        /// Pass a stop capture directive after the event stream is closed.
        AFTER_EVENT_STREAM_CLOSE,
        /// Do not pass a stop capture directive.
        NONE
    };

    /**
     * Function to send a recognize event and verify that it fails.  Parameters are passed through
     * to @c RecognizeEvent::RecognizeEvent().
     *
     * @return @c true if the recognize event failed to send correctly, else @c false.
     */
    bool testRecognizeFails(
        AudioProvider audioProvider,
        Initiator initiator,
        avsCommon::avs::AudioInputStream::Index begin = AudioInputProcessor::INVALID_INDEX,
        avsCommon::avs::AudioInputStream::Index keywordEnd = AudioInputProcessor::INVALID_INDEX,
        std::string keyword = "");

    /**
     * Function to send a recognize event and verify that it succeeds.  All parameters except @c stopPoint are passed
     * through to @c RecognizeEvent::RecognizeEvent().
     *
     * @param stopPoint Where (if at all) to call @c stopCapture() during this test.
     * @return @c true if the recognize event sent correctly, else @c false.
     */
    bool testRecognizeSucceeds(
        AudioProvider audioProvider,
        Initiator initiator,
        avsCommon::avs::AudioInputStream::Index begin = AudioInputProcessor::INVALID_INDEX,
        avsCommon::avs::AudioInputStream::Index keywordEnd = AudioInputProcessor::INVALID_INDEX,
        std::string keyword = "",
        RecognizeStopPoint stopPoint = RecognizeStopPoint::NONE,
        std::shared_ptr<std::string> avsInitiator = nullptr,
        const ESPData espData = ESPData::getEmptyESPData(),
        const std::shared_ptr<std::vector<char>> KWDMetadata = nullptr);

    /**
     * Function to call @c AudioInputProcessor::stopCapture() and verify that it succeeds.
     *
     * @return @c true if the call works correctly, else @c false.
     */
    bool testStopCaptureSucceeds();

    /**
     * Function to call AudioInputProcessor::onContextFailure() and verify that @c AudioInputProcessor responds to it
     * correctly.
     *
     * @param error The failure type to test.
     * @return @c true if the call works correctly, else @c false.
     */
    bool testContextFailure(avsCommon::sdkInterfaces::ContextRequestError error);

    /**
     * Function to receive a StopCapture directive and verify that @c AudioInputProcessor responds to it
     * correctly.
     *
     * @param withDialogRequestId A flag indicating whether to send the directive with a dialog request ID.
     * @return @c true if the call works correctly, else @c false.
     */
    bool testStopCaptureDirectiveSucceeds(bool withDialogRequestId);

    /**
     * Function to receive a StopCapture directive and verify that @c AudioInputProcessor rejects it.
     *
     * @param withDialogRequestId A flag indicating whether to send the directive with a dialog request ID.
     * @return @c true if the call fails as expected, else @c false.
     */
    bool testStopCaptureDirectiveFails(bool withDialogRequestId);

    /**
     * Function to send an expect speech event and verify that it succeeds.
     *
     * @param withDialogRequestId A flag indicating whether to send the directive with a dialog request ID.
     * @return @c true if the call works correctly, else @c false.
     */
    bool testExpectSpeechSucceeds(bool withDialogRequestId);

    /**
     * Function to send an expect speech event and optionally verify that it times out.
     *
     * @param withDialogRequestId A flag indicating whether to send the event with a dialog request ID.
     * @param verifyTimeout A flag indicating whether to wait for a timeout.
     * @return @c true if the call works correctly, else @c false.
     */
    bool testExpectSpeechWaits(bool withDialogRequestId, bool verifyTimeout);

    /**
     * Function to send an expect speech event and optionally verify that it fails.
     *
     * @param withDialogRequestId A flag indicating whether to send the event with a dialog request ID.
     * @return @c true if the call fails as expected, else @c false.
     */
    bool testExpectSpeechFails(bool withDialogRequestId);

    /**
     * Function to send an ExpectSpeech directive and verify the initiator is handled correctly on the
     * subsequent Recognize.
     *
     * @param withInitiator A flag indicating whether the ExpectSpeech will contain the initiator.
     * @return @c true if the call succeeds, else @c false.
     */
    bool testRecognizeWithExpectSpeechInitiator(bool withInitiator);

    /**
     * Function to construct an @c AVSDirective for the specified namespace/name.
     *
     * @param directive The namespace and name to use for this directive.
     * @param withDialogRequestId A flag indicating whether to include a dialog request ID.
     * @param withInitiator A flag indicating whether the directive should have an initiator.
     * @return the constructed @c AVSDirective.
     */
    static std::shared_ptr<avsCommon::avs::AVSDirective> createAVSDirective(
        const avsCommon::avs::NamespaceAndName& directive,
        bool withDialogRequestId,
        bool withInitiator = true);

    /**
     * Function to construct an @c AVSDirective for the specified namespace/name, with the given payload.
     *
     * @param directive The namespace and name to use for this directive.
     * @param withDialogRequestId A flag indicating whether to include a dialog request ID.
     * @param withInitiator A flag indicating whether the directive should have an initiator.
     * @param document The root document object to use.
     * @param document The payload to use.
     * @return the constructed @c AVSDirective.
     */
    static std::shared_ptr<avsCommon::avs::AVSDirective> createAVSDirective(
        const avsCommon::avs::NamespaceAndName& directive,
        bool withDialogRequestId,
        bool withInitiator,
        rapidjson::Document& document,
        rapidjson::Value& payloadJson);
    /**
     * This function verifies that JSON content of an ExpectSpeechTimedOut @c MessageRequest is correct, and that it
     * does not have an attachment.  This function signature matches that of @c MessageSenderInterface::sendMessage()
     * so that an @c EXPECT_CALL() can @c Invoke() this function directly.
     *
     * @param request The @c MessageRequest to verify.
     */
    static void verifyExpectSpeechTimedOut(std::shared_ptr<avsCommon::avs::MessageRequest> request);

    /// This function replace @c m_audioInputProcessor with a new one that does not have a default @c AudioProvider.
    void removeDefaultAudioProvider();

    /**
     * This function replace @c m_audioInputProcessor with a new one that has an @c AudioProvider that is not
     * @c alwaysReadable.
     */
    void makeDefaultAudioProviderNotAlwaysReadable();

    /**
     * Function to call @c onFocusChanged() and verify that @c AudioInputProcessor responds correctly.
     *
     * @param state The focus state to test with.
     * @return @c true if the @c AudioInputProcessor responds as expected, else @c false.
     */
    bool testFocusChange(avsCommon::avs::FocusState state);

    /**
     * Performs a test to check the AIP correctly transitions to a state after getting notified that the recognize event
     * stream has been closed and/or receiving a stop capture directive.
     *
     * @param eventStreamFinishedStatus The status of the recognize event stream when the stream closes.
     * @param stopCaptureSchedule Specify the point when to pass a stop capture directive to AIP.
     * @param expectedAIPFinalState The expected final state of the AIP.
     * @param expectFocusReleased If true, it is expected that AIP will release the channel focus in the final state,
     * and false otherwise.
     */
    void testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status eventStreamFinishedStatus,
        StopCaptureDirectiveSchedule stopCaptureSchedule,
        AudioInputProcessorObserverInterface::State expectedAIPFinalState,
        bool expectFocusReleased);

    /// The mock @c DirectiveSequencerInterface.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockDirectiveSequencer> m_mockDirectiveSequencer;

    /// The mock @c MessageSenderInterface.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockMessageSender> m_mockMessageSender;

    /// The mock @c ContextManagerInterface.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockContextManager> m_mockContextManager;

    /// The mock @c FocusManagerInterface.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockFocusManager> m_mockFocusManager;

    /// The @c DialogUXStateAggregator to test with.
    std::shared_ptr<avsCommon::avs::DialogUXStateAggregator> m_dialogUXStateAggregator;

    /// A @c TestDialogUXStateObserver track when the @c DialogUXStateAggregator is thinking.
    std::shared_ptr<TestDialogUXStateObserver> m_dialogUXStateObserver;

    /// The mock @c ExceptionEncounteredSenderInterface.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender> m_mockExceptionEncounteredSender;

    /// The mock @c UserInactivityMonitorInterface.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockUserInactivityMonitor> m_mockUserInactivityMonitor;

    /// A @c AudioInputStream::Writer to write audio data to m_audioProvider.
    std::unique_ptr<avsCommon::avs::AudioInputStream::Writer> m_writer;

    /// The @c AudioProvider to test with.
    std::unique_ptr<AudioProvider> m_audioProvider;

    /// The @c AudioInputProcessor to test.
    std::shared_ptr<AudioInputProcessor> m_audioInputProcessor;

    /// The mock @c ObserverInterface.
    std::shared_ptr<MockObserver> m_mockObserver;

    /// The @c RecognizeEvent from the last @c testRecognizeSucceeds() call.
    std::shared_ptr<RecognizeEvent> m_recognizeEvent;

    /// Vector of samples holding a test pattern to feed through the @c AudioInputStream.
    std::vector<Sample> m_pattern;

    std::string m_dialogRequestId;
};

void AudioInputProcessorTest::SetUp() {
    m_mockDirectiveSequencer = std::make_shared<avsCommon::sdkInterfaces::test::MockDirectiveSequencer>();
    m_mockMessageSender = std::make_shared<avsCommon::sdkInterfaces::test::MockMessageSender>();
    m_mockContextManager = std::make_shared<avsCommon::sdkInterfaces::test::MockContextManager>();
    m_mockFocusManager = std::make_shared<avsCommon::sdkInterfaces::test::MockFocusManager>();
    m_dialogUXStateAggregator = std::make_shared<avsCommon::avs::DialogUXStateAggregator>();
    m_dialogUXStateObserver = std::make_shared<TestDialogUXStateObserver>(m_dialogUXStateAggregator);

    m_dialogUXStateAggregator->addObserver(m_dialogUXStateObserver);

    m_mockExceptionEncounteredSender =
        std::make_shared<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender>();
    m_mockUserInactivityMonitor = std::make_shared<avsCommon::sdkInterfaces::test::MockUserInactivityMonitor>();
    size_t bufferSize = avsCommon::avs::AudioInputStream::calculateBufferSize(SDS_WORDS, SDS_WORDSIZE, SDS_MAXREADERS);
    auto buffer = std::make_shared<avsCommon::avs::AudioInputStream::Buffer>(bufferSize);
    auto stream = avsCommon::avs::AudioInputStream::create(buffer, SDS_WORDSIZE, SDS_MAXREADERS);
    ASSERT_NE(stream, nullptr);
    m_writer = stream->createWriter(avsCommon::avs::AudioInputStream::Writer::Policy::NONBLOCKABLE);
    ASSERT_NE(m_writer, nullptr);

    avsCommon::utils::AudioFormat format = {avsCommon::utils::AudioFormat::Encoding::LPCM,
                                            avsCommon::utils::AudioFormat::Endianness::LITTLE,
                                            SAMPLE_RATE_HZ,
                                            SAMPLE_SIZE_IN_BITS,
                                            NUM_CHANNELS};
    m_audioProvider = avsCommon::utils::memory::make_unique<AudioProvider>(
        std::move(stream), format, ASRProfile::NEAR_FIELD, ALWAYS_READABLE, CAN_OVERRIDE, CAN_BE_OVERRIDDEN);
    m_audioInputProcessor = AudioInputProcessor::create(
        m_mockDirectiveSequencer,
        m_mockMessageSender,
        m_mockContextManager,
        m_mockFocusManager,
        m_dialogUXStateAggregator,
        m_mockExceptionEncounteredSender,
        m_mockUserInactivityMonitor,
        nullptr,
        *m_audioProvider);
    ASSERT_NE(m_audioInputProcessor, nullptr);
    m_audioInputProcessor->addObserver(m_dialogUXStateAggregator);
    // Note: StrictMock here so that we fail on unexpected AIP state changes
    m_mockObserver = std::make_shared<StrictMock<MockObserver>>();
    ASSERT_NE(m_mockObserver, nullptr);
    m_audioInputProcessor->addObserver(m_mockObserver);

    // Populate the test pattern with values that correspond to indexes for easy verification.
    m_pattern.resize(PATTERN_WORDS);
    std::iota(m_pattern.begin(), m_pattern.end(), 0);
}

void AudioInputProcessorTest::TearDown() {
    if (m_audioInputProcessor) {
        m_audioInputProcessor->removeObserver(m_dialogUXStateAggregator);
        EXPECT_CALL(*m_mockFocusManager, releaseChannel(CHANNEL_NAME, _)).Times(AtLeast(0));
        EXPECT_CALL(*m_mockObserver, onStateChanged(AudioInputProcessorObserverInterface::State::IDLE))
            .Times(AtLeast(0));
        m_audioInputProcessor->resetState().wait();
    }
    m_dialogUXStateAggregator->removeObserver(m_dialogUXStateObserver);
}

bool AudioInputProcessorTest::testRecognizeFails(
    AudioProvider audioProvider,
    Initiator initiator,
    avsCommon::avs::AudioInputStream::Index begin,
    avsCommon::avs::AudioInputStream::Index keywordEnd,
    std::string keyword) {
    RecognizeEvent recognize(audioProvider, initiator, begin, keywordEnd, keyword);
    return !recognize.send(m_audioInputProcessor).get();
}

bool AudioInputProcessorTest::testRecognizeSucceeds(
    AudioProvider audioProvider,
    Initiator initiator,
    avsCommon::avs::AudioInputStream::Index begin,
    avsCommon::avs::AudioInputStream::Index keywordEnd,
    std::string keyword,
    RecognizeStopPoint stopPoint,
    std::shared_ptr<std::string> avsInitiator,
    const ESPData espData,
    const std::shared_ptr<std::vector<char>> KWDMetadata) {
    std::mutex mutex;
    std::condition_variable conditionVariable;

    bool done = false;
    bool bargeIn = m_recognizeEvent != nullptr;

    // If a valid begin index is provided, preload the SDS buffer with the test pattern.
    if (begin != AudioInputProcessor::INVALID_INDEX) {
        EXPECT_EQ(m_writer->write(m_pattern.data(), m_pattern.size()), static_cast<ssize_t>(m_pattern.size()));
    }

    rapidjson::Document contextDocument(rapidjson::kObjectType);
    rapidjson::Value contextArray(rapidjson::kArrayType);
    contextDocument.AddMember(rapidjson::StringRef(MESSAGE_CONTEXT_KEY), contextArray, contextDocument.GetAllocator());
    rapidjson::StringBuffer contextBuffer;
    rapidjson::Writer<rapidjson::StringBuffer> contextWriter(contextBuffer);
    contextDocument.Accept(contextWriter);
    std::string contextJson = contextBuffer.GetString();
    m_recognizeEvent = std::make_shared<RecognizeEvent>(
        audioProvider, initiator, begin, keywordEnd, keyword, avsInitiator, espData, KWDMetadata);
    if (keyword.empty()) {
        EXPECT_CALL(*m_mockContextManager, getContext(_)).WillOnce(InvokeWithoutArgs([this, contextJson, stopPoint] {
            m_audioInputProcessor->onContextAvailable(contextJson);
            if (RecognizeStopPoint::AFTER_CONTEXT == stopPoint) {
                EXPECT_TRUE(m_audioInputProcessor->stopCapture().valid());
            }
        }));
    } else {
        // Enforce the sequence; setState needs to be called before getContext, otherwise ContextManager will not
        // include the new state in the context for this recognize.
        InSequence dummy;

        EXPECT_CALL(*m_mockContextManager, getContext(_)).WillOnce(InvokeWithoutArgs([this, contextJson, stopPoint] {
            m_audioInputProcessor->onContextAvailable(contextJson);
            if (RecognizeStopPoint::AFTER_CONTEXT == stopPoint) {
                EXPECT_TRUE(m_audioInputProcessor->stopCapture().valid());
            }
        }));
    }

    if (!bargeIn) {
        EXPECT_CALL(*m_mockUserInactivityMonitor, onUserActive()).Times(2);
        EXPECT_CALL(*m_mockObserver, onStateChanged(AudioInputProcessorObserverInterface::State::RECOGNIZING));
        EXPECT_CALL(*m_mockFocusManager, acquireChannel(CHANNEL_NAME, _, NAMESPACE))
            .WillOnce(InvokeWithoutArgs([this, stopPoint] {
                m_audioInputProcessor->onFocusChanged(avsCommon::avs::FocusState::FOREGROUND);
                if (RecognizeStopPoint::AFTER_FOCUS == stopPoint) {
                    EXPECT_TRUE(m_audioInputProcessor->stopCapture().valid());
                }
                return true;
            }));
    }
    EXPECT_CALL(*m_mockDirectiveSequencer, setDialogRequestId(_))
        .WillOnce(Invoke([this](const std::string& dialogRequestId) { m_dialogRequestId = dialogRequestId; }));
    {
        // Enforce the sequence.
        InSequence dummy;
        if (espData.verify()) {
            EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
                .WillOnce(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
                    m_recognizeEvent->verifyEspMessage(request, m_dialogRequestId);
                    EXPECT_CALL(*m_mockObserver, onStateChanged(AudioInputProcessorObserverInterface::State::BUSY));
                    m_audioInputProcessor->onSendCompleted(
                        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS);
                }));
        }
        EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
            .WillOnce(DoAll(
                Invoke([this, KWDMetadata](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
                    m_recognizeEvent->verifyMetadata(request, KWDMetadata);
                    m_recognizeEvent->verifyMessage(request, m_pattern, m_dialogRequestId);
                }),
                InvokeWithoutArgs([&] {
                    if (RecognizeStopPoint::AFTER_SEND == stopPoint) {
                        EXPECT_TRUE(m_audioInputProcessor->stopCapture().valid());
                    } else if (RecognizeStopPoint::NONE == stopPoint) {
                        std::lock_guard<std::mutex> lock(mutex);
                        done = true;
                        conditionVariable.notify_one();
                    }
                })));
    }
    if (stopPoint != RecognizeStopPoint::NONE) {
        EXPECT_CALL(*m_mockObserver, onStateChanged(AudioInputProcessorObserverInterface::State::BUSY));
        EXPECT_CALL(*m_mockFocusManager, releaseChannel(CHANNEL_NAME, _));
        EXPECT_CALL(*m_mockObserver, onStateChanged(AudioInputProcessorObserverInterface::State::IDLE))
            .WillOnce(InvokeWithoutArgs([&] {
                std::lock_guard<std::mutex> lock(mutex);
                done = true;
                conditionVariable.notify_one();
            }));
    }

    auto sentFuture = m_recognizeEvent->send(m_audioInputProcessor);

    // If a valid begin index was not provided, load the SDS buffer with the test pattern after recognize() is sent.
    if (AudioInputProcessor::INVALID_INDEX == begin) {
        EXPECT_EQ(m_writer->write(m_pattern.data(), m_pattern.size()), static_cast<ssize_t>(m_pattern.size()));
    }

    auto sent = sentFuture.get();
    EXPECT_TRUE(sent);
    if (!sent) {
        return false;
    }

    if (RecognizeStopPoint::AFTER_RECOGNIZE == stopPoint) {
        EXPECT_TRUE(m_audioInputProcessor->stopCapture().valid());
    }
    std::unique_lock<std::mutex> lock(mutex);
    return conditionVariable.wait_for(lock, TEST_TIMEOUT, [&done] { return done; });
}

bool AudioInputProcessorTest::testStopCaptureSucceeds() {
    std::mutex mutex;
    std::condition_variable conditionVariable;
    bool done = false;

    EXPECT_CALL(*m_mockObserver, onStateChanged(AudioInputProcessorObserverInterface::State::BUSY));
    EXPECT_CALL(*m_mockFocusManager, releaseChannel(CHANNEL_NAME, _));
    EXPECT_CALL(*m_mockObserver, onStateChanged(AudioInputProcessorObserverInterface::State::IDLE))
        .WillOnce(InvokeWithoutArgs([&] {
            std::lock_guard<std::mutex> lock(mutex);
            done = true;
            conditionVariable.notify_one();
        }));

    auto stopCaptureResult = m_audioInputProcessor->stopCapture();
    EXPECT_TRUE(stopCaptureResult.valid());
    if (!stopCaptureResult.valid() && stopCaptureResult.get()) {
        return false;
    }

    std::unique_lock<std::mutex> lock(mutex);
    return conditionVariable.wait_for(lock, TEST_TIMEOUT, [&done] { return done; });
}

bool AudioInputProcessorTest::testContextFailure(avsCommon::sdkInterfaces::ContextRequestError error) {
    std::mutex mutex;
    std::condition_variable conditionVariable;
    bool done = false;
    RecognizeEvent recognize(*m_audioProvider, Initiator::TAP);

    EXPECT_CALL(*m_mockContextManager, getContext(_)).WillOnce(InvokeWithoutArgs([this, error] {
        m_audioInputProcessor->onContextFailure(error);
    }));
    EXPECT_CALL(*m_mockObserver, onStateChanged(AudioInputProcessorObserverInterface::State::RECOGNIZING));
    EXPECT_CALL(*m_mockUserInactivityMonitor, onUserActive()).Times(2);
    EXPECT_CALL(*m_mockObserver, onStateChanged(AudioInputProcessorObserverInterface::State::IDLE))
        .WillOnce(InvokeWithoutArgs([&] {
            std::lock_guard<std::mutex> lock(mutex);
            done = true;
            conditionVariable.notify_one();
        }));

    if (recognize.send(m_audioInputProcessor).get()) {
        std::unique_lock<std::mutex> lock(mutex);
        return conditionVariable.wait_for(lock, TEST_TIMEOUT, [&done] { return done; });
    }
    return false;
}

bool AudioInputProcessorTest::testStopCaptureDirectiveSucceeds(bool withDialogRequestId) {
    std::mutex mutex;
    std::condition_variable conditionVariable;
    bool done = false;

    auto avsDirective = createAVSDirective(STOP_CAPTURE, withDialogRequestId);
    auto result = avsCommon::utils::memory::make_unique<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>();
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> directiveHandler = m_audioInputProcessor;

    EXPECT_CALL(*m_mockObserver, onStateChanged(AudioInputProcessorObserverInterface::State::BUSY));
    EXPECT_CALL(*m_mockFocusManager, releaseChannel(CHANNEL_NAME, _));
    if (withDialogRequestId) {
        EXPECT_CALL(*result, setCompleted());
    }
    EXPECT_CALL(*m_mockObserver, onStateChanged(AudioInputProcessorObserverInterface::State::IDLE))
        .WillOnce(InvokeWithoutArgs([&] {
            std::lock_guard<std::mutex> lock(mutex);
            done = true;
            conditionVariable.notify_one();
        }));

    if (!withDialogRequestId) {
        directiveHandler->handleDirectiveImmediately(avsDirective);
    } else {
        directiveHandler->preHandleDirective(avsDirective, std::move(result));
        EXPECT_TRUE(directiveHandler->handleDirective(avsDirective->getMessageId()));
    }

    std::unique_lock<std::mutex> lock(mutex);
    return conditionVariable.wait_for(lock, TEST_TIMEOUT, [&done] { return done; });
}

bool AudioInputProcessorTest::testStopCaptureDirectiveFails(bool withDialogRequestId) {
    std::mutex mutex;
    std::condition_variable conditionVariable;
    bool done = false;

    auto avsDirective = createAVSDirective(STOP_CAPTURE, withDialogRequestId);
    auto result = avsCommon::utils::memory::make_unique<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>();
    EXPECT_CALL(*result, setFailed(_)).WillOnce(InvokeWithoutArgs([&] {
        std::lock_guard<std::mutex> lock(mutex);
        done = true;
        conditionVariable.notify_one();
    }));
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> directiveHandler = m_audioInputProcessor;

    if (!withDialogRequestId) {
        directiveHandler->handleDirectiveImmediately(avsDirective);
    } else {
        directiveHandler->preHandleDirective(avsDirective, std::move(result));
        EXPECT_TRUE(directiveHandler->handleDirective(avsDirective->getMessageId()));
    }
    std::unique_lock<std::mutex> lock(mutex);
    return conditionVariable.wait_for(lock, TEST_TIMEOUT, [&done] { return done; });
}

bool AudioInputProcessorTest::testExpectSpeechSucceeds(bool withDialogRequestId) {
    std::mutex mutex;
    std::condition_variable conditionVariable;
    bool done = false;

    auto avsDirective = createAVSDirective(EXPECT_SPEECH, withDialogRequestId);
    auto result = avsCommon::utils::memory::make_unique<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>();
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> directiveHandler = m_audioInputProcessor;

    EXPECT_CALL(*m_mockObserver, onStateChanged(AudioInputProcessorObserverInterface::State::EXPECTING_SPEECH));
    EXPECT_CALL(*m_mockObserver, onStateChanged(AudioInputProcessorObserverInterface::State::RECOGNIZING));
    EXPECT_CALL(*m_mockUserInactivityMonitor, onUserActive()).Times(2);
    if (withDialogRequestId) {
        EXPECT_CALL(*result, setCompleted());
    }
    EXPECT_CALL(*m_mockContextManager, getContext(_)).WillOnce(InvokeWithoutArgs([&] {
        std::lock_guard<std::mutex> lock(mutex);
        done = true;
        conditionVariable.notify_one();
    }));

    if (!withDialogRequestId) {
        directiveHandler->handleDirectiveImmediately(avsDirective);
    } else {
        directiveHandler->preHandleDirective(avsDirective, std::move(result));
        EXPECT_TRUE(directiveHandler->handleDirective(avsDirective->getMessageId()));
    }

    std::unique_lock<std::mutex> lock(mutex);
    return conditionVariable.wait_for(lock, TEST_TIMEOUT, [&done] { return done; });
}

bool AudioInputProcessorTest::testExpectSpeechWaits(bool withDialogRequestId, bool verifyTimeout) {
    std::mutex mutex;
    std::condition_variable conditionVariable;
    bool done = false;

    auto avsDirective = createAVSDirective(EXPECT_SPEECH, withDialogRequestId);
    auto result = avsCommon::utils::memory::make_unique<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>();
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> directiveHandler = m_audioInputProcessor;

    if (withDialogRequestId) {
        EXPECT_CALL(*result, setCompleted());
    }
    if (verifyTimeout) {
        EXPECT_CALL(*m_mockObserver, onStateChanged(AudioInputProcessorObserverInterface::State::EXPECTING_SPEECH));
        EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).WillOnce(Invoke(&verifyExpectSpeechTimedOut));
        EXPECT_CALL(*m_mockObserver, onStateChanged(AudioInputProcessorObserverInterface::State::IDLE))
            .WillOnce(InvokeWithoutArgs([&] {
                std::lock_guard<std::mutex> lock(mutex);
                done = true;
                conditionVariable.notify_one();
            }));
    } else {
        EXPECT_CALL(*m_mockObserver, onStateChanged(AudioInputProcessorObserverInterface::State::EXPECTING_SPEECH))
            .WillOnce(InvokeWithoutArgs([&] {
                std::lock_guard<std::mutex> lock(mutex);
                done = true;
                conditionVariable.notify_one();
            }));
    }

    if (!withDialogRequestId) {
        directiveHandler->handleDirectiveImmediately(avsDirective);
    } else {
        directiveHandler->preHandleDirective(avsDirective, std::move(result));
        EXPECT_TRUE(directiveHandler->handleDirective(avsDirective->getMessageId()));
    }

    std::unique_lock<std::mutex> lock(mutex);
    return conditionVariable.wait_for(lock, TEST_TIMEOUT, [&done] { return done; });
}

bool AudioInputProcessorTest::testExpectSpeechFails(bool withDialogRequestId) {
    std::mutex mutex;
    std::condition_variable conditionVariable;
    bool done = false;

    auto avsDirective = createAVSDirective(EXPECT_SPEECH, withDialogRequestId);
    auto result = avsCommon::utils::memory::make_unique<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>();
    if (withDialogRequestId) {
        EXPECT_CALL(*result, setFailed(_)).WillOnce(InvokeWithoutArgs([&] {
            std::lock_guard<std::mutex> lock(mutex);
            done = true;
            conditionVariable.notify_one();
        }));
    }
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> directiveHandler = m_audioInputProcessor;

    if (!withDialogRequestId) {
        directiveHandler->handleDirectiveImmediately(avsDirective);
        return true;
    } else {
        directiveHandler->preHandleDirective(avsDirective, std::move(result));
        EXPECT_TRUE(directiveHandler->handleDirective(avsDirective->getMessageId()));
        std::unique_lock<std::mutex> lock(mutex);
        return conditionVariable.wait_for(lock, TEST_TIMEOUT, [&done] { return done; });
    }
}

static bool getInitiatorFromDirective(const std::string directive, std::string* initiator) {
    std::string event, payload;
    if (!jsonUtils::retrieveValue(directive, MESSAGE_EVENT_KEY, &event)) {
        return false;
    }
    if (!jsonUtils::retrieveValue(event, MESSAGE_PAYLOAD_KEY, &payload)) {
        return false;
    }
    return jsonUtils::retrieveValue(payload, EXPECT_SPEECH_INITIATOR_KEY, initiator);
}

bool AudioInputProcessorTest::testRecognizeWithExpectSpeechInitiator(bool withInitiator) {
    std::mutex mutex;
    std::condition_variable conditionVariable;
    bool done = false;

    auto avsDirective = createAVSDirective(EXPECT_SPEECH, true, withInitiator);

    auto result = avsCommon::utils::memory::make_unique<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>();
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> directiveHandler = m_audioInputProcessor;

    // Parse out message contents and set expectations based on withInitiator value.
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .WillOnce(Invoke([&](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            std::string actualInitiatorString;
            if (withInitiator) {
                ASSERT_TRUE(getInitiatorFromDirective(request->getJsonContent(), &actualInitiatorString));
                ASSERT_EQ(actualInitiatorString, EXPECT_SPEECH_INITIATOR);
            } else {
                ASSERT_FALSE(getInitiatorFromDirective(request->getJsonContent(), &actualInitiatorString));
            }
            std::lock_guard<std::mutex> lock(mutex);
            done = true;
            conditionVariable.notify_one();
        }));

    rapidjson::Document contextDocument(rapidjson::kObjectType);
    rapidjson::Value contextArray(rapidjson::kArrayType);
    contextDocument.AddMember(rapidjson::StringRef(MESSAGE_CONTEXT_KEY), contextArray, contextDocument.GetAllocator());
    rapidjson::StringBuffer contextBuffer;
    rapidjson::Writer<rapidjson::StringBuffer> contextWriter(contextBuffer);
    contextDocument.Accept(contextWriter);
    std::string contextJson = contextBuffer.GetString();

    // Check for successful Directive handling.
    EXPECT_CALL(*result, setCompleted());
    EXPECT_CALL(*m_mockObserver, onStateChanged(AudioInputProcessorObserverInterface::State::EXPECTING_SPEECH));
    EXPECT_CALL(*m_mockObserver, onStateChanged(AudioInputProcessorObserverInterface::State::RECOGNIZING));
    EXPECT_CALL(*m_mockUserInactivityMonitor, onUserActive()).Times(2);
    EXPECT_CALL(*m_mockContextManager, getContext(_));
    EXPECT_CALL(*m_mockDirectiveSequencer, setDialogRequestId(_));

    // Set AIP to a sane state.
    directiveHandler->preHandleDirective(avsDirective, std::move(result));
    EXPECT_TRUE(directiveHandler->handleDirective(avsDirective->getMessageId()));
    m_audioInputProcessor->onFocusChanged(avsCommon::avs::FocusState::FOREGROUND);
    m_audioInputProcessor->onContextAvailable(contextJson);

    std::unique_lock<std::mutex> lock(mutex);
    return conditionVariable.wait_for(lock, TEST_TIMEOUT, [&done] { return done; });
}

std::shared_ptr<avsCommon::avs::AVSDirective> AudioInputProcessorTest::createAVSDirective(
    const avsCommon::avs::NamespaceAndName& directive,
    bool withDialogRequestId,
    bool withInitiator) {
    rapidjson::Document document(rapidjson::kObjectType);
    rapidjson::Value payloadJson(rapidjson::kObjectType);

    if (EXPECT_SPEECH == directive) {
        rapidjson::Value timeoutInMillisecondsJson(EXPECT_SPEECH_TIMEOUT_IN_MILLISECONDS);
        payloadJson.AddMember(
            rapidjson::StringRef(EXPECT_SPEECH_TIMEOUT_KEY), timeoutInMillisecondsJson, document.GetAllocator());

        if (withInitiator) {
            rapidjson::Value initiatorJson(rapidjson::StringRef(EXPECT_SPEECH_INITIATOR));
            payloadJson.AddMember(
                rapidjson::StringRef(EXPECT_SPEECH_INITIATOR_KEY), initiatorJson, document.GetAllocator());
        }
    }

    return createAVSDirective(directive, withDialogRequestId, withInitiator, document, payloadJson);
}

std::shared_ptr<avsCommon::avs::AVSDirective> AudioInputProcessorTest::createAVSDirective(
    const avsCommon::avs::NamespaceAndName& directive,
    bool withDialogRequestId,
    bool withInitiator,
    rapidjson::Document& document,
    rapidjson::Value& payloadJson) {
    auto header = std::make_shared<avsCommon::avs::AVSMessageHeader>(
        directive.nameSpace,
        directive.name,
        avsCommon::utils::uuidGeneration::generateUUID(),
        avsCommon::utils::uuidGeneration::generateUUID());

    rapidjson::Value directiveJson(rapidjson::kObjectType);
    rapidjson::Value headerJson(rapidjson::kObjectType);
    rapidjson::Value namespaceJson(header->getNamespace(), document.GetAllocator());
    rapidjson::Value nameJson(header->getName(), document.GetAllocator());
    rapidjson::Value messageIdJson(header->getMessageId(), document.GetAllocator());
    rapidjson::Value dialogRequestIdJson(header->getDialogRequestId(), document.GetAllocator());

    headerJson.AddMember(rapidjson::StringRef(MESSAGE_NAMESPACE_KEY), namespaceJson, document.GetAllocator());
    headerJson.AddMember(rapidjson::StringRef(MESSAGE_NAME_KEY), nameJson, document.GetAllocator());
    headerJson.AddMember(rapidjson::StringRef(MESSAGE_MESSAGE_ID_KEY), messageIdJson, document.GetAllocator());
    headerJson.AddMember(
        rapidjson::StringRef(MESSAGE_DIALOG_REQUEST_ID_KEY), dialogRequestIdJson, document.GetAllocator());
    directiveJson.AddMember(rapidjson::StringRef(MESSAGE_HEADER_KEY), headerJson, document.GetAllocator());

    rapidjson::StringBuffer payloadBuffer;
    rapidjson::Writer<rapidjson::StringBuffer> payloadWriter(payloadBuffer);
    payloadJson.Accept(payloadWriter);

    directiveJson.AddMember(rapidjson::StringRef(MESSAGE_PAYLOAD_KEY), payloadJson, document.GetAllocator());
    document.AddMember(rapidjson::StringRef(MESSAGE_DIRECTIVE_KEY), directiveJson, document.GetAllocator());

    rapidjson::StringBuffer documentBuffer;
    rapidjson::Writer<rapidjson::StringBuffer> documentWriter(documentBuffer);
    document.Accept(documentWriter);

    auto mockAttachmentManager = std::make_shared<avsCommon::avs::attachment::test::MockAttachmentManager>();
    return avsCommon::avs::AVSDirective::create(
        documentBuffer.GetString(), header, payloadBuffer.GetString(), mockAttachmentManager, "");
}

void AudioInputProcessorTest::verifyExpectSpeechTimedOut(std::shared_ptr<avsCommon::avs::MessageRequest> request) {
    rapidjson::Document document = parseJson(request->getJsonContent());

    auto event = document.FindMember(MESSAGE_EVENT_KEY);
    EXPECT_NE(event, document.MemberEnd());

    auto header = event->value.FindMember(MESSAGE_HEADER_KEY);
    EXPECT_NE(header, event->value.MemberEnd());
    auto payload = event->value.FindMember(MESSAGE_PAYLOAD_KEY);
    EXPECT_NE(payload, event->value.MemberEnd());

    EXPECT_EQ(getJsonString(header->value, MESSAGE_NAMESPACE_KEY), NAMESPACE);
    EXPECT_EQ(getJsonString(header->value, MESSAGE_NAME_KEY), EXPECT_SPEECH_TIMED_OUT_EVENT_NAME);
    EXPECT_NE(getJsonString(header->value, MESSAGE_MESSAGE_ID_KEY), "");

    EXPECT_EQ(request->attachmentReadersCount(), 0);
}

void AudioInputProcessorTest::removeDefaultAudioProvider() {
    m_audioInputProcessor->removeObserver(m_dialogUXStateAggregator);
    m_audioInputProcessor = AudioInputProcessor::create(
        m_mockDirectiveSequencer,
        m_mockMessageSender,
        m_mockContextManager,
        m_mockFocusManager,
        m_dialogUXStateAggregator,
        m_mockExceptionEncounteredSender,
        m_mockUserInactivityMonitor);
    EXPECT_NE(m_audioInputProcessor, nullptr);
    m_audioInputProcessor->addObserver(m_mockObserver);
    m_audioInputProcessor->addObserver(m_dialogUXStateAggregator);
}

void AudioInputProcessorTest::makeDefaultAudioProviderNotAlwaysReadable() {
    m_audioProvider->alwaysReadable = false;
    m_audioInputProcessor->removeObserver(m_dialogUXStateAggregator);
    m_audioInputProcessor = AudioInputProcessor::create(
        m_mockDirectiveSequencer,
        m_mockMessageSender,
        m_mockContextManager,
        m_mockFocusManager,
        m_dialogUXStateAggregator,
        m_mockExceptionEncounteredSender,
        m_mockUserInactivityMonitor,
        nullptr,
        *m_audioProvider);
    EXPECT_NE(m_audioInputProcessor, nullptr);
    m_audioInputProcessor->addObserver(m_mockObserver);
    m_audioInputProcessor->addObserver(m_dialogUXStateAggregator);
}

bool AudioInputProcessorTest::testFocusChange(avsCommon::avs::FocusState state) {
    std::mutex mutex;
    std::condition_variable conditionVariable;
    bool done = false;

    bool recognizeSucceeded = testRecognizeSucceeds(*m_audioProvider, Initiator::TAP);
    EXPECT_TRUE(recognizeSucceeded);
    if (!recognizeSucceeded) {
        return false;
    }
    if (state != avsCommon::avs::FocusState::NONE) {
        EXPECT_CALL(*m_mockFocusManager, releaseChannel(CHANNEL_NAME, _));
    }
    EXPECT_CALL(*m_mockObserver, onStateChanged(AudioInputProcessorObserverInterface::State::IDLE))
        .WillOnce(InvokeWithoutArgs([&] {
            std::lock_guard<std::mutex> lock(mutex);
            done = true;
            conditionVariable.notify_one();
        }));
    m_audioInputProcessor->onFocusChanged(state);

    std::unique_lock<std::mutex> lock(mutex);
    return conditionVariable.wait_for(lock, TEST_TIMEOUT, [&done] { return done; });
}

void AudioInputProcessorTest::testAIPStateTransitionOnEventFinish(
    avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status eventStreamFinishedStatus,
    StopCaptureDirectiveSchedule stopCaptureSchedule,
    AudioInputProcessorObserverInterface::State expectedAIPFinalState,
    bool expectFocusReleased) {
    // Simulate tap to talk and start recognizing.
    ASSERT_TRUE(testRecognizeSucceeds(*m_audioProvider, Initiator::TAP, 0));

    // Expect some AIP transient states.
    EXPECT_CALL(*m_mockObserver, onStateChanged(_)).Times(AtLeast(0));

    // Expected final state.
    EXPECT_CALL(*m_mockObserver, onStateChanged(expectedAIPFinalState)).Times(1);

    if (expectFocusReleased) {
        EXPECT_CALL(*m_mockFocusManager, releaseChannel(CHANNEL_NAME, _));
    }

    auto avsDirective = createAVSDirective(STOP_CAPTURE, true);

    if (StopCaptureDirectiveSchedule::BEFORE_EVENT_STREAM_CLOSE == stopCaptureSchedule) {
        m_audioInputProcessor->handleDirectiveImmediately(avsDirective);
    }

    m_audioInputProcessor->onSendCompleted(eventStreamFinishedStatus);

    if (StopCaptureDirectiveSchedule::AFTER_EVENT_STREAM_CLOSE == stopCaptureSchedule) {
        m_audioInputProcessor->handleDirectiveImmediately(avsDirective);
    }
}

/// Function to verify that @c AudioInputProcessor::create() errors out with an invalid @c DirectiveSequencerInterface.
TEST_F(AudioInputProcessorTest, createWithoutDirectiveSequencer) {
    m_audioInputProcessor->removeObserver(m_dialogUXStateAggregator);
    m_audioInputProcessor = AudioInputProcessor::create(
        nullptr,
        m_mockMessageSender,
        m_mockContextManager,
        m_mockFocusManager,
        m_dialogUXStateAggregator,
        m_mockExceptionEncounteredSender,
        m_mockUserInactivityMonitor,
        nullptr,
        *m_audioProvider);
    EXPECT_EQ(m_audioInputProcessor, nullptr);
}

/// Function to verify that @c AudioInputProcessor::create() errors out with an invalid @c MessageSenderInterface.
TEST_F(AudioInputProcessorTest, createWithoutMessageSender) {
    m_audioInputProcessor->removeObserver(m_dialogUXStateAggregator);
    m_audioInputProcessor = AudioInputProcessor::create(
        m_mockDirectiveSequencer,
        nullptr,
        m_mockContextManager,
        m_mockFocusManager,
        m_dialogUXStateAggregator,
        m_mockExceptionEncounteredSender,
        m_mockUserInactivityMonitor,
        nullptr,
        *m_audioProvider);
    EXPECT_EQ(m_audioInputProcessor, nullptr);
}

/// Function to verify that @c AudioInputProcessor::create() errors out with an invalid @c ContextManagerInterface.
TEST_F(AudioInputProcessorTest, createWithoutContextManager) {
    m_audioInputProcessor->removeObserver(m_dialogUXStateAggregator);
    m_audioInputProcessor = AudioInputProcessor::create(
        m_mockDirectiveSequencer,
        m_mockMessageSender,
        nullptr,
        m_mockFocusManager,
        m_dialogUXStateAggregator,
        m_mockExceptionEncounteredSender,
        m_mockUserInactivityMonitor,
        nullptr,
        *m_audioProvider);
    EXPECT_EQ(m_audioInputProcessor, nullptr);
}

/// Function to verify that @c AudioInputProcessor::create() errors out with an invalid @c FocusManagerInterface.
TEST_F(AudioInputProcessorTest, createWithoutFocusManager) {
    m_audioInputProcessor->removeObserver(m_dialogUXStateAggregator);
    m_audioInputProcessor = AudioInputProcessor::create(
        m_mockDirectiveSequencer,
        m_mockMessageSender,
        m_mockContextManager,
        nullptr,
        m_dialogUXStateAggregator,
        m_mockExceptionEncounteredSender,
        m_mockUserInactivityMonitor,
        nullptr,
        *m_audioProvider);
    EXPECT_EQ(m_audioInputProcessor, nullptr);
}

/// Function to verify that @c AudioInputProcessor::create() errors out with an invalid @c DialogUXStateAggregator.
TEST_F(AudioInputProcessorTest, createWithoutStateAggregator) {
    m_audioInputProcessor->removeObserver(m_dialogUXStateAggregator);
    m_audioInputProcessor = AudioInputProcessor::create(
        m_mockDirectiveSequencer,
        m_mockMessageSender,
        m_mockContextManager,
        m_mockFocusManager,
        nullptr,
        m_mockExceptionEncounteredSender,
        m_mockUserInactivityMonitor,
        nullptr,
        *m_audioProvider);
    EXPECT_EQ(m_audioInputProcessor, nullptr);
}

/**
 * Function to verify that @c AudioInputProcessor::create() errors out with an invalid
 * @c ExceptionEncounteredSenderInterface.
 */
TEST_F(AudioInputProcessorTest, createWithoutExceptionSender) {
    m_audioInputProcessor->removeObserver(m_dialogUXStateAggregator);
    m_audioInputProcessor = AudioInputProcessor::create(
        m_mockDirectiveSequencer,
        m_mockMessageSender,
        m_mockContextManager,
        m_mockFocusManager,
        m_dialogUXStateAggregator,
        nullptr,
        m_mockUserInactivityMonitor,
        nullptr,
        *m_audioProvider);
    EXPECT_EQ(m_audioInputProcessor, nullptr);
}

/**
 * Function to verify that @c AudioInputProcessor::create() errors out with an invalid
 * @c UserInactivityMonitorInterface.
 */
TEST_F(AudioInputProcessorTest, createWithoutUserInactivityMonitor) {
    m_audioInputProcessor->removeObserver(m_dialogUXStateAggregator);
    m_audioInputProcessor = AudioInputProcessor::create(
        m_mockDirectiveSequencer,
        m_mockMessageSender,
        m_mockContextManager,
        m_mockFocusManager,
        m_dialogUXStateAggregator,
        m_mockExceptionEncounteredSender,
        nullptr,
        nullptr,
        *m_audioProvider);
    EXPECT_EQ(m_audioInputProcessor, nullptr);
}

/// Function to verify that @c AudioInputProcessor::create() succeeds with a null @c AudioProvider.
TEST_F(AudioInputProcessorTest, createWithoutAudioProvider) {
    m_audioInputProcessor->removeObserver(m_dialogUXStateAggregator);
    m_audioInputProcessor = AudioInputProcessor::create(
        m_mockDirectiveSequencer,
        m_mockMessageSender,
        m_mockContextManager,
        m_mockFocusManager,
        m_dialogUXStateAggregator,
        m_mockExceptionEncounteredSender,
        m_mockUserInactivityMonitor);
    EXPECT_NE(m_audioInputProcessor, nullptr);
}

/// Function to verify that @c AudioInputProcessor::getconfiguration() returns the expected configuration data.
TEST_F(AudioInputProcessorTest, getConfiguration) {
    DirectiveHandlerConfiguration expectedConfiguration{
        {STOP_CAPTURE, BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false)},
        {EXPECT_SPEECH, BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, true)},
        {SET_END_OF_SPEECH_OFFSET, BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false)}};

    auto configuration = m_audioInputProcessor->getConfiguration();
    EXPECT_EQ(configuration, expectedConfiguration);
}

/**
 * Function to verify that observers can be added/removed.  Nothing is directly asserted here, but this test verifies
 * that these functions work without crashing.
 */
TEST_F(AudioInputProcessorTest, addRemoveObserver) {
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

/// This function verifies that @c AudioInputProcessor::recognize() fails when given a null @c AudioProvider.
TEST_F(AudioInputProcessorTest, recognizeNullStream) {
    auto result = m_audioInputProcessor->recognize(AudioProvider::null(), Initiator::PRESS_AND_HOLD);
    ASSERT_TRUE(result.valid());
    ASSERT_FALSE(result.get());
}

/// This function verifies that @c AudioInputProcessor::recognize() fails when given invalid @c AudioFormats.
TEST_F(AudioInputProcessorTest, recognizeInvalidAudioFormat) {
    AudioProvider audioProvider = *m_audioProvider;
    audioProvider.format.endianness = avsCommon::utils::AudioFormat::Endianness::BIG;
    EXPECT_FALSE(m_audioInputProcessor->recognize(audioProvider, Initiator::PRESS_AND_HOLD).get());

    audioProvider = *m_audioProvider;
    audioProvider.format.sampleRateHz = 0;
    EXPECT_FALSE(m_audioInputProcessor->recognize(audioProvider, Initiator::PRESS_AND_HOLD).get());

    audioProvider = *m_audioProvider;
    audioProvider.format.sampleSizeInBits = 0;
    EXPECT_FALSE(m_audioInputProcessor->recognize(audioProvider, Initiator::PRESS_AND_HOLD).get());

    audioProvider = *m_audioProvider;
    audioProvider.format.numChannels = 0;
    EXPECT_FALSE(m_audioInputProcessor->recognize(audioProvider, Initiator::PRESS_AND_HOLD).get());
}

/// This function verifies that @c AudioInputProcessor::recognize() works with @c Initiator::PRESS_AND_HOLD.
TEST_F(AudioInputProcessorTest, recognizePressAndHold) {
    ASSERT_TRUE(testRecognizeSucceeds(*m_audioProvider, Initiator::PRESS_AND_HOLD));
}

/// This function verifies that @c AudioInputProcessor::recognize() works with @c Initiator::TAP.
TEST_F(AudioInputProcessorTest, recognizeTap) {
    ASSERT_TRUE(testRecognizeSucceeds(*m_audioProvider, Initiator::TAP));
}

/// This function verifies that @c AudioInputProcessor::recognize() fails with @c Initiator::WAKEWORD and no keyword.
TEST_F(AudioInputProcessorTest, recognizeWakewordWithoutKeyword) {
    EXPECT_TRUE(testRecognizeFails(*m_audioProvider, Initiator::WAKEWORD));
}

/**
 * This function verifies that @c AudioInputProcessor::recognize() fails with @c Initiator::WAKEWORD and invalid begin
 * index.
 */
TEST_F(AudioInputProcessorTest, recognizeWakewordWithBadBegin) {
    // Write data until the sds wraps, which will make 0 an invalid index.
    for (size_t written = 0; written <= SDS_WORDS; written += PATTERN_WORDS) {
        EXPECT_EQ(m_writer->write(m_pattern.data(), m_pattern.size()), static_cast<ssize_t>(m_pattern.size()));
    }
    avsCommon::avs::AudioInputStream::Index begin = 0;
    auto end = AudioInputProcessor::INVALID_INDEX;
    EXPECT_TRUE(testRecognizeFails(*m_audioProvider, Initiator::WAKEWORD, begin, end, KEYWORD_TEXT));
}

/// This function verifies that @c AudioInputProcessor::recognize() works with @c Initiator::WAKEWORD and keyword.
TEST_F(AudioInputProcessorTest, recognizeWakewordWithKeyword) {
    auto begin = AudioInputProcessor::INVALID_INDEX;
    auto end = AudioInputProcessor::INVALID_INDEX;
    EXPECT_TRUE(testRecognizeSucceeds(*m_audioProvider, Initiator::WAKEWORD, begin, end, KEYWORD_TEXT));
}

/// This function verifies that @c AudioInputProcessor::recognize() works with @c Initiator::WAKEWORD valid begin.
TEST_F(AudioInputProcessorTest, recognizeWakewordWithGoodBegin) {
    avsCommon::avs::AudioInputStream::Index begin = 0;
    auto end = AudioInputProcessor::INVALID_INDEX;
    EXPECT_TRUE(testRecognizeSucceeds(*m_audioProvider, Initiator::WAKEWORD, begin, end, KEYWORD_TEXT));
}

/**
 * This function verifies that @c AudioInputProcessor::recognize() works with @c Initiator::WAKEWORD valid begin and
 * end indices.
 */
TEST_F(AudioInputProcessorTest, recognizeWakewordWithGoodBeginAndEnd) {
    avsCommon::avs::AudioInputStream::Index begin = PREROLL_WORDS;
    avsCommon::avs::AudioInputStream::Index end = PREROLL_WORDS + WAKEWORD_WORDS;
    EXPECT_TRUE(testRecognizeSucceeds(*m_audioProvider, Initiator::WAKEWORD, begin, end, KEYWORD_TEXT));
}

/// This function verifies that @c AudioInputProcessor::recognize() works with @c ASRProfile::CLOSE_TALK.
TEST_F(AudioInputProcessorTest, recognizeCloseTalk) {
    auto audioProvider = *m_audioProvider;
    audioProvider.profile = ASRProfile::CLOSE_TALK;
    ASSERT_TRUE(testRecognizeSucceeds(audioProvider, Initiator::PRESS_AND_HOLD));
}

/// This function verifies that @c AudioInputProcessor::recognize() works with @c ASRProfile::NEAR_FIELD.
TEST_F(AudioInputProcessorTest, recognizeNearField) {
    auto audioProvider = *m_audioProvider;
    audioProvider.profile = ASRProfile::NEAR_FIELD;
    ASSERT_TRUE(testRecognizeSucceeds(audioProvider, Initiator::TAP));
}

/// This function verifies that @c AudioInputProcessor::recognize() works with @c ASRProfile::FAR_FIELD.
TEST_F(AudioInputProcessorTest, recognizeFarField) {
    auto audioProvider = *m_audioProvider;
    audioProvider.profile = ASRProfile::FAR_FIELD;
    ASSERT_TRUE(testRecognizeSucceeds(audioProvider, Initiator::TAP));
}

/// This function verifies that @c AudioInputProcessor::recognize() works in @c State::EXPECTING_SPEECH.
TEST_F(AudioInputProcessorTest, recognizeWhileExpectingSpeech) {
    removeDefaultAudioProvider();
    ASSERT_TRUE(testExpectSpeechWaits(WITH_DIALOG_REQUEST_ID, !VERIFY_TIMEOUT));
    // Recognize event after an ExpectSpeech results in the ExpectSpeech's initiator being passed back to AVS.
    ASSERT_TRUE(testRecognizeSucceeds(
        *m_audioProvider,
        Initiator::PRESS_AND_HOLD,
        AudioInputProcessor::INVALID_INDEX,
        AudioInputProcessor::INVALID_INDEX,
        "",
        RecognizeStopPoint::NONE,
        std::make_shared<std::string>(EXPECT_SPEECH_INITIATOR)));
}

/**
 * This function verifies that @c AudioInputProcessor::recognize() works with a call to @c stopCapture() immediately
 * after the @c recognize() call.
 */
TEST_F(AudioInputProcessorTest, recognizeStopAfterRecognize) {
    auto audioProvider = *m_audioProvider;
    audioProvider.profile = ASRProfile::CLOSE_TALK;
    ASSERT_TRUE(testRecognizeSucceeds(
        audioProvider,
        Initiator::PRESS_AND_HOLD,
        AudioInputProcessor::INVALID_INDEX,
        AudioInputProcessor::INVALID_INDEX,
        "",
        RecognizeStopPoint::AFTER_RECOGNIZE));
}

/**
 * This function verifies that @c AudioInputProcessor::recognize() works with a call to @c stopCapture() immediately
 * after the @c onContextAvailable() call.
 */
TEST_F(AudioInputProcessorTest, recognizeStopAfterContext) {
    auto audioProvider = *m_audioProvider;
    audioProvider.profile = ASRProfile::CLOSE_TALK;
    ASSERT_TRUE(testRecognizeSucceeds(
        audioProvider,
        Initiator::PRESS_AND_HOLD,
        AudioInputProcessor::INVALID_INDEX,
        AudioInputProcessor::INVALID_INDEX,
        "",
        RecognizeStopPoint::AFTER_CONTEXT));
}

/**
 * This function verifies that @c AudioInputProcessor::recognize() works with a call to @c stopCapture() immediately
 * after the @c onFocusChanged() call.
 */
TEST_F(AudioInputProcessorTest, recognizeStopAfterFocus) {
    auto audioProvider = *m_audioProvider;
    audioProvider.profile = ASRProfile::CLOSE_TALK;
    ASSERT_TRUE(testRecognizeSucceeds(
        audioProvider,
        Initiator::PRESS_AND_HOLD,
        AudioInputProcessor::INVALID_INDEX,
        AudioInputProcessor::INVALID_INDEX,
        "",
        RecognizeStopPoint::AFTER_FOCUS));
}

/**
 * This function verifies that @c AudioInputProcessor::recognize() works with a call to @c stopCapture() immediately
 * after the message is sent.
 */
TEST_F(AudioInputProcessorTest, recognizeStopAfterSend) {
    auto audioProvider = *m_audioProvider;
    audioProvider.profile = ASRProfile::CLOSE_TALK;
    ASSERT_TRUE(testRecognizeSucceeds(
        audioProvider,
        Initiator::PRESS_AND_HOLD,
        AudioInputProcessor::INVALID_INDEX,
        AudioInputProcessor::INVALID_INDEX,
        "",
        RecognizeStopPoint::AFTER_SEND));
}

/**
 * This function verifies that @c AudioInputProcessor::recognize() works in @c State::RECOGNIZING when the previous
 * recognize used the CLOSE_TALK profile.
 */
TEST_F(AudioInputProcessorTest, recognizeBargeInWhileRecognizingCloseTalk) {
    auto audioProvider = *m_audioProvider;
    audioProvider.profile = ASRProfile::CLOSE_TALK;
    ASSERT_TRUE(testRecognizeSucceeds(audioProvider, Initiator::TAP));
    ASSERT_TRUE(testRecognizeSucceeds(*m_audioProvider, Initiator::TAP));
}

/**
 * This function verifies that @c AudioInputProcessor::recognize() works in @c State::RECOGNIZING when the previous
 * recognize used the NEAR_FIELD profile.
 */
TEST_F(AudioInputProcessorTest, recognizeBargeInWhileRecognizingNearField) {
    auto audioProvider = *m_audioProvider;
    audioProvider.profile = ASRProfile::NEAR_FIELD;
    ASSERT_TRUE(testRecognizeSucceeds(audioProvider, Initiator::TAP));
    ASSERT_TRUE(testRecognizeSucceeds(*m_audioProvider, Initiator::TAP));
}

/**
 * This function verifies that @c AudioInputProcessor::recognize() works in @c State::RECOGNIZING when the previous
 * recognize used the FAR_FIELD profile.
 */
TEST_F(AudioInputProcessorTest, recognizeBargeInWhileRecognizingFarField) {
    auto audioProvider = *m_audioProvider;
    audioProvider.profile = ASRProfile::FAR_FIELD;
    ASSERT_TRUE(testRecognizeSucceeds(audioProvider, Initiator::TAP));
    ASSERT_TRUE(testRecognizeSucceeds(*m_audioProvider, Initiator::TAP));
}

/**
 * This function verifies that @c AudioInputProcessor::recognize() fails in @c State::RECOGNIZING when the second
 * @c AudioProvider can't override.
 */
TEST_F(AudioInputProcessorTest, recognizeBargeInWhileRecognizingCantOverride) {
    ASSERT_TRUE(testRecognizeSucceeds(*m_audioProvider, Initiator::TAP));
    auto audioProvider = *m_audioProvider;
    audioProvider.canOverride = false;
    ASSERT_TRUE(testRecognizeFails(audioProvider, Initiator::TAP));
}

/**
 * This function verifies that @c AudioInputProcessor::recognize() fails in @c State::RECOGNIZING when the
 * first @c AudioProvider can't be overridden.
 */
TEST_F(AudioInputProcessorTest, recognizeBargeInWhileRecognizingCantBeOverridden) {
    auto audioProvider = *m_audioProvider;
    audioProvider.canBeOverridden = false;
    ASSERT_TRUE(testRecognizeSucceeds(audioProvider, Initiator::TAP));
    ASSERT_TRUE(testRecognizeFails(*m_audioProvider, Initiator::TAP));
}

/// This function verifies that @c AudioInputProcessor::stopCapture() fails in @c State::IDLE.
TEST_F(AudioInputProcessorTest, stopCaptureWhenIdle) {
    ASSERT_FALSE(m_audioInputProcessor->stopCapture().get());
}

/// This function verifies that @c AudioInputProcessor::stopCapture() fails in @c State::EXPECTING_SPEECH.
TEST_F(AudioInputProcessorTest, stopCaptureWhenExpectingSpeech) {
    removeDefaultAudioProvider();
    ASSERT_TRUE(testExpectSpeechWaits(WITH_DIALOG_REQUEST_ID, !VERIFY_TIMEOUT));
    ASSERT_FALSE(m_audioInputProcessor->stopCapture().get());
}

/// This function verifies that @c AudioInputProcessor::stopCapture() works in @c State::RECOGNIZING.
TEST_F(AudioInputProcessorTest, stopCaptureWhenRecognizing) {
    ASSERT_TRUE(testRecognizeSucceeds(*m_audioProvider, Initiator::TAP, 0));
    ASSERT_TRUE(testStopCaptureSucceeds());

    auto readStatus = avsCommon::avs::attachment::AttachmentReader::ReadStatus::OK;
    std::vector<uint8_t> buf(SDS_WORDS * SDS_WORDSIZE);
    EXPECT_EQ(m_recognizeEvent->getReader()->read(buf.data(), buf.size(), &readStatus), 0U);
    ASSERT_EQ(readStatus, avsCommon::avs::attachment::AttachmentReader::ReadStatus::CLOSED);
}

/*
 * This function verifies that @c AudioInputProcessor::stopCapture() works in @c State::RECOGNIZING and check if
 * subsequent StopCapture directive will be ignored.
 */
TEST_F(AudioInputProcessorTest, stopCaptureWhenRecognizingFollowByStopCaptureDirective) {
    ASSERT_TRUE(testRecognizeSucceeds(*m_audioProvider, Initiator::TAP, 0));
    ASSERT_TRUE(testStopCaptureSucceeds());

    auto readStatus = avsCommon::avs::attachment::AttachmentReader::ReadStatus::OK;
    std::vector<uint8_t> buf(SDS_WORDS * SDS_WORDSIZE);
    EXPECT_EQ(m_recognizeEvent->getReader()->read(buf.data(), buf.size(), &readStatus), 0U);
    ASSERT_EQ(readStatus, avsCommon::avs::attachment::AttachmentReader::ReadStatus::CLOSED);

    std::mutex mutex;
    std::condition_variable conditionVariable;
    bool done = false;

    auto avsDirective = createAVSDirective(STOP_CAPTURE, true);
    auto result = avsCommon::utils::memory::make_unique<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>();
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> directiveHandler = m_audioInputProcessor;

    EXPECT_CALL(*result, setCompleted()).WillOnce(InvokeWithoutArgs([&] {
        std::lock_guard<std::mutex> lock(mutex);
        done = true;
        conditionVariable.notify_one();
    }));
    directiveHandler->preHandleDirective(avsDirective, std::move(result));
    EXPECT_TRUE(directiveHandler->handleDirective(avsDirective->getMessageId()));

    std::unique_lock<std::mutex> lock(mutex);
    auto setCompletedResult = conditionVariable.wait_for(lock, TEST_TIMEOUT, [&done] { return done; });
    EXPECT_TRUE(setCompletedResult);
}

/// This function verifies that @c AudioInputProcessor::resetState() works in @c State::IDLE.
TEST_F(AudioInputProcessorTest, resetStateWhenIdle) {
    m_audioInputProcessor->resetState().get();
}

/// This function verifies that @c AudioInputProcessor::resetState() works in @c State::EXPECTING_SPEECH.
TEST_F(AudioInputProcessorTest, resetStateWhenExpectingSpeech) {
    removeDefaultAudioProvider();
    ASSERT_TRUE(testExpectSpeechWaits(WITH_DIALOG_REQUEST_ID, !VERIFY_TIMEOUT));
    EXPECT_CALL(*m_mockObserver, onStateChanged(AudioInputProcessorObserverInterface::State::IDLE));
    m_audioInputProcessor->resetState().get();
}

/// This function verifies that @c AudioInputProcessor::resetState() works in @c State::RECOGNIZING.
TEST_F(AudioInputProcessorTest, resetStateWhenRecognizing) {
    ASSERT_TRUE(testRecognizeSucceeds(*m_audioProvider, Initiator::TAP, 0));

    EXPECT_CALL(*m_mockFocusManager, releaseChannel(CHANNEL_NAME, _));
    EXPECT_CALL(*m_mockObserver, onStateChanged(AudioInputProcessorObserverInterface::State::IDLE));
    m_audioInputProcessor->resetState().get();
}

/**
 * This function verifies that @c AudioInputProcessor responds correctly to
 * @c ContextRequestError::STATE_PROVIDER_TIMEDOUT.
 */
TEST_F(AudioInputProcessorTest, contextFailureStateProviderTimedout) {
    ASSERT_TRUE(testContextFailure(avsCommon::sdkInterfaces::ContextRequestError::STATE_PROVIDER_TIMEDOUT));
}

/**
 * This function verifies that @c AudioInputProcessor responds correctly to
 * @c ContextRequestError::BUILD_CONTEXT_ERROR.
 */
TEST_F(AudioInputProcessorTest, contextFailureBuildContextError) {
    ASSERT_TRUE(testContextFailure(avsCommon::sdkInterfaces::ContextRequestError::BUILD_CONTEXT_ERROR));
}

/// This function verifies that StopCapture directives fail in @c State::IDLE.
TEST_F(AudioInputProcessorTest, preHandleAndHandleDirectiveStopCaptureWhenIdle) {
    ASSERT_TRUE(testStopCaptureDirectiveFails(WITH_DIALOG_REQUEST_ID));
}

/// This function verifies that StopCapture directives with dialog request ID work in @c State::RECOGNIZING.
TEST_F(AudioInputProcessorTest, preHandleAndHandleDirectiveStopCaptureWhenRecognizing) {
    ASSERT_TRUE(testRecognizeSucceeds(*m_audioProvider, Initiator::TAP, 0));
    ASSERT_TRUE(testStopCaptureDirectiveSucceeds(WITH_DIALOG_REQUEST_ID));
}

/// This function verifies that StopCapture directives fail in @c State::EXPECTING_SPEECH.
TEST_F(AudioInputProcessorTest, preHandleAndHandleDirectiveStopCaptureWhenExpectingSpeech) {
    removeDefaultAudioProvider();
    ASSERT_TRUE(testExpectSpeechWaits(WITH_DIALOG_REQUEST_ID, !VERIFY_TIMEOUT));
    ASSERT_TRUE(testStopCaptureDirectiveFails(WITH_DIALOG_REQUEST_ID));
}

/// This function verifies that StopCapture directives without dialog request ID work in @c State::RECOGNIZING.
TEST_F(AudioInputProcessorTest, handleDirectiveImmediatelyStopCaptureWhenRecognizing) {
    ASSERT_TRUE(testRecognizeSucceeds(*m_audioProvider, Initiator::TAP, 0));
    ASSERT_TRUE(testStopCaptureDirectiveSucceeds(!WITH_DIALOG_REQUEST_ID));
}

/// This function verifies that ExpectSpeech directives with dialog request ID work in @c State::IDLE.
TEST_F(AudioInputProcessorTest, preHandleAndHandleDirectiveExpectSpeechWhenIdle) {
    ASSERT_TRUE(testExpectSpeechSucceeds(WITH_DIALOG_REQUEST_ID));
}

/// This function verifies that ExpectSpeech directives without dialog request ID work in @c State::IDLE.
TEST_F(AudioInputProcessorTest, handleDirectiveImmediatelyExpectSpeechWhenIdle) {
    ASSERT_TRUE(testExpectSpeechSucceeds(!WITH_DIALOG_REQUEST_ID));
}

/// This function verifies that ExpectSpeech directives fail in @c State::RECOGNIZING.
TEST_F(AudioInputProcessorTest, preHandleAndHandleDirectiveExpectSpeechWhenRecognizing) {
    ASSERT_TRUE(testRecognizeSucceeds(*m_audioProvider, Initiator::TAP, 0));
    ASSERT_TRUE(testExpectSpeechFails(WITH_DIALOG_REQUEST_ID));
}

/// This function verifies that ExpectSpeech directives fail in @c State::EXPECTING_SPEECH.
TEST_F(AudioInputProcessorTest, preHandleAndHandleDirectiveExpectSpeechWhenExpectingSpeech) {
    removeDefaultAudioProvider();
    ASSERT_TRUE(testExpectSpeechWaits(WITH_DIALOG_REQUEST_ID, !VERIFY_TIMEOUT));
    ASSERT_TRUE(testExpectSpeechFails(WITH_DIALOG_REQUEST_ID));
}

/// This function verifies that ExpectSpeech directives wait with no default and no previous @c AudioProvider.
TEST_F(AudioInputProcessorTest, expectSpeechNoDefaultNoPrevious) {
    removeDefaultAudioProvider();
    ASSERT_TRUE(testExpectSpeechWaits(WITH_DIALOG_REQUEST_ID, VERIFY_TIMEOUT));
}

/// This function verifies that ExpectSpeech directives wait with unreadable default and no previous @c AudioProvider.
TEST_F(AudioInputProcessorTest, expectSpeechUnreadableDefaultNoPrevious) {
    makeDefaultAudioProviderNotAlwaysReadable();
    ASSERT_TRUE(testExpectSpeechWaits(WITH_DIALOG_REQUEST_ID, VERIFY_TIMEOUT));
}

/**
 * This function verifies that ExpectSpeech directives wait with unreadable default and unreadable previous
 * @c AudioProvider.
 */
TEST_F(AudioInputProcessorTest, expectSpeechUnreadableDefaultUnreadablePrevious) {
    makeDefaultAudioProviderNotAlwaysReadable();
    ASSERT_TRUE(testRecognizeSucceeds(*m_audioProvider, Initiator::PRESS_AND_HOLD, 0));
    ASSERT_TRUE(testStopCaptureSucceeds());
    ASSERT_TRUE(testExpectSpeechWaits(WITH_DIALOG_REQUEST_ID, VERIFY_TIMEOUT));
}

/// This function verifies that ExpectSpeech directives work with no default and readable previous @c AudioProvider.
TEST_F(AudioInputProcessorTest, expectSpeechNoDefaultReadablePrevious) {
    removeDefaultAudioProvider();
    ASSERT_TRUE(testRecognizeSucceeds(*m_audioProvider, Initiator::PRESS_AND_HOLD, 0));
    ASSERT_TRUE(testStopCaptureSucceeds());
    ASSERT_TRUE(testExpectSpeechSucceeds(WITH_DIALOG_REQUEST_ID));
}

/// This function verifies that the initiator from an ExpectSpeech is passed to a subsequent Recognize.
TEST_F(AudioInputProcessorTest, expectSpeechWithInitiator) {
    ASSERT_TRUE(testRecognizeWithExpectSpeechInitiator(true));
}

/**
 * This function verifies that if the ExpectSpeech does not have an initiator, no initiator is present in the
 * subsequent Recognize.
 */
TEST_F(AudioInputProcessorTest, expectSpeechWithNoInitiator) {
    ASSERT_TRUE(testRecognizeWithExpectSpeechInitiator(false));
}

/**
 * This function verifies that if the ExpectSpeech times out, the next user initiated Recognize will send the standard
 * initiator and not the one passed from AVS.
 */
TEST_F(AudioInputProcessorTest, expectSpeechWithInitiatorTimedOut) {
    removeDefaultAudioProvider();
    ASSERT_TRUE(testExpectSpeechWaits(WITH_DIALOG_REQUEST_ID, VERIFY_TIMEOUT));
    ASSERT_TRUE(testRecognizeSucceeds(*m_audioProvider, Initiator::TAP));
}

/// This function verifies that a focus change to @c FocusState::BACKGROUND causes the @c AudioInputProcessor to
/// release the channel and go back to @c State::IDLE.
TEST_F(AudioInputProcessorTest, focusChangedBackground) {
    ASSERT_TRUE(testFocusChange(avsCommon::avs::FocusState::BACKGROUND));
}

/// This function verifies that a focus change to @c FocusState::NONE causes the @c AudioInputProcessor to
/// release the channel and go back to @c State::IDLE.
TEST_F(AudioInputProcessorTest, focusChangedNone) {
    ASSERT_TRUE(testFocusChange(avsCommon::avs::FocusState::NONE));
}

/// Test that the @c AudioInputProcessor correctly transitions to @c State::IDLE
/// if @c Status::TIMEDOUT is received
TEST_F(AudioInputProcessorTest, resetStateOnTimeOut) {
    ASSERT_TRUE(testRecognizeSucceeds(*m_audioProvider, Initiator::TAP, 0));

    EXPECT_CALL(*m_mockFocusManager, releaseChannel(CHANNEL_NAME, _));
    EXPECT_CALL(*m_mockObserver, onStateChanged(AudioInputProcessorObserverInterface::State::IDLE));
    m_audioInputProcessor->onSendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::TIMEDOUT);
}

/*
 * This function verifies that @c AudioInputProcessor::recognize() works with @c Initiator::WAKEWORD, keyword and
 * valid espData.
 */
TEST_F(AudioInputProcessorTest, recognizeWakewordWithESPWithKeyword) {
    auto begin = AudioInputProcessor::INVALID_INDEX;
    auto end = AudioInputProcessor::INVALID_INDEX;
    // note that we are just using a integer instead of a float number, this is to help with JSON verification.
    ESPData espData("123456789", "987654321");
    EXPECT_TRUE(testRecognizeSucceeds(
        *m_audioProvider, Initiator::WAKEWORD, begin, end, KEYWORD_TEXT, RecognizeStopPoint::NONE, nullptr, espData));
}

/*
 * This function verifies that @c AudioInputProcessor::recognize() works with @c Initiator::WAKEWORD, keyword and
 * invalid espData.  The ReportEchoSpatialPerceptionData event will not be sent but the Recognize event should still be
 * sent.
 */
TEST_F(AudioInputProcessorTest, recognizeWakewordWithInvalidESPWithKeyword) {
    auto begin = AudioInputProcessor::INVALID_INDEX;
    auto end = AudioInputProcessor::INVALID_INDEX;
    ESPData espData("@#\"", "@#\"");
    EXPECT_TRUE(testRecognizeSucceeds(
        *m_audioProvider, Initiator::WAKEWORD, begin, end, KEYWORD_TEXT, RecognizeStopPoint::NONE, nullptr, espData));
}

/*
 * This function verifies that @c AudioInputProcessor::recognize() works with OPUS encoding used with
 * @c Initiator::TAP.
 */
TEST_F(AudioInputProcessorTest, recognizeOPUSWithTap) {
    m_audioProvider->format.encoding = avsCommon::utils::AudioFormat::Encoding::OPUS;
    m_audioProvider->format.sampleRateHz = 32000;
    ASSERT_TRUE(testRecognizeSucceeds(*m_audioProvider, Initiator::TAP));
}

/*
 * This function verifies that @c AudioInputProcessor::recognize() works with OPUS encoding used with
 * @c Initiator::PRESS_AND_HOLD.
 */
TEST_F(AudioInputProcessorTest, recognizeOPUSWithPressAndHold) {
    m_audioProvider->format.encoding = avsCommon::utils::AudioFormat::Encoding::OPUS;
    m_audioProvider->format.sampleRateHz = 32000;
    ASSERT_TRUE(testRecognizeSucceeds(*m_audioProvider, Initiator::PRESS_AND_HOLD));
}

/**
 * This function verifies that @c AudioInputProcessor::recognize() works with OPUS encoding used with
 * @c Initiator::WAKEWORD valid begin and end indices.
 */
TEST_F(AudioInputProcessorTest, recognizeOPUSWithWakeWord) {
    avsCommon::avs::AudioInputStream::Index begin = 0;
    avsCommon::avs::AudioInputStream::Index end = AudioInputProcessor::INVALID_INDEX;
    m_audioProvider->format.encoding = avsCommon::utils::AudioFormat::Encoding::OPUS;
    m_audioProvider->format.sampleRateHz = 32000;
    EXPECT_TRUE(testRecognizeSucceeds(*m_audioProvider, Initiator::WAKEWORD, begin, end, KEYWORD_TEXT));
}

/**
 * This function verifies that @c AudioInputProcessor::recognize() creates a @c MessageRequest with KWDMetadata
 * When metadata has been received
 */
TEST_F(AudioInputProcessorTest, recognizeWakewordWithKWDMetadata) {
    auto begin = AudioInputProcessor::INVALID_INDEX;
    auto end = AudioInputProcessor::INVALID_INDEX;

    auto metadata = std::make_shared<std::vector<char>>();
    metadata->assign(KWD_METADATA_EXAMPLE.data(), KWD_METADATA_EXAMPLE.data() + KWD_METADATA_EXAMPLE.length());

    EXPECT_TRUE(testRecognizeSucceeds(
        *m_audioProvider,
        Initiator::WAKEWORD,
        begin,
        end,
        KEYWORD_TEXT,
        RecognizeStopPoint::NONE,
        nullptr,
        ESPData::getEmptyESPData(),
        metadata));
}

/**
 * This function verifies that @c AudioInputProcessor state will stop listening when the recognize event stream has been
 * successfully sent.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamSuccess) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS,
        StopCaptureDirectiveSchedule::NONE,
        AudioInputProcessorObserverInterface::State::BUSY,
        false);
}

/**
 * This function verifies that @c AudioInputProcessor state will stop listening when the recognize event stream has been
 * successfully sent but received no HTTP/2 content.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamSuccessNoContent) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT,
        StopCaptureDirectiveSchedule::NONE,
        AudioInputProcessorObserverInterface::State::BUSY,
        false);
}

/**
 * This function verifies that @c AudioInputProcessor state will stop listening when the recognize event stream has not
 * been sent due to connection to AVS has been severed.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamSuccessNotConnected) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::NOT_CONNECTED,
        StopCaptureDirectiveSchedule::NONE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state will stop listening when the recognize event stream has not
 * been sent due to AVS is not synchronized.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamNotSynchronized) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::NOT_SYNCHRONIZED,
        StopCaptureDirectiveSchedule::NONE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state will stop listening when the recognize event stream has not
 * been sent due to an internal error within ACL.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamInternalrror) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::INTERNAL_ERROR,
        StopCaptureDirectiveSchedule::NONE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state will stop listening when the recognize event stream has not
 * been sent due to an underlying protocol error.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamProtocolError) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::PROTOCOL_ERROR,
        StopCaptureDirectiveSchedule::NONE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state will stop listening when the recognize event stream has not
 * been sent due to an internal error on the server which sends code 500.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamServerInternalError) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2,
        StopCaptureDirectiveSchedule::NONE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state will stop listening when the recognize event stream has not
 * been sent due to server refusing the request.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamRefused) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::REFUSED,
        StopCaptureDirectiveSchedule::NONE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state will stop listening when the recognize event stream has not
 * been sent due to server canceling it before the transmission completed.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamCanceled) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::CANCELED,
        StopCaptureDirectiveSchedule::NONE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state will stop listening when the recognize event stream has not
 * been sent due to excessive load on the server.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamThrottled) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::THROTTLED,
        StopCaptureDirectiveSchedule::NONE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state will stop listening when the recognize event stream has not
 * been sent due to the access credentials provided to ACL were invalid.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamInvalidAuth) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::INVALID_AUTH,
        StopCaptureDirectiveSchedule::NONE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state will stop listening when the recognize event stream has not
 * been sent due to invalid request sent by the user.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamBadRequest) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::BAD_REQUEST,
        StopCaptureDirectiveSchedule::NONE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state will stop listening when the recognize event stream has not
 * been sent due to unknown server error.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamUnknownServerError) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SERVER_OTHER_ERROR,
        StopCaptureDirectiveSchedule::NONE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after receiving a stop capture directive and the
 * recognize event stream has been successfully sent.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnDirectiveAndStreamSuccess) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS,
        StopCaptureDirectiveSchedule::BEFORE_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::BUSY,
        false);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after receiving a stop capture directive and the
 * recognize event stream has been successfully sent but received no HTTP/2 content.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnDirectiveAndStreamSuccessNoContent) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT,
        StopCaptureDirectiveSchedule::BEFORE_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::BUSY,
        false);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after receiving a stop capture directive and the
 * recognize event stream has not been sent due to connection to AVS has been severed.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnDirectiveAndStreamSuccessNotConnected) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::NOT_CONNECTED,
        StopCaptureDirectiveSchedule::BEFORE_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after receiving a stop capture directive and the
 * recognize event stream has not been sent due to AVS is not synchronized.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnDirectiveAndStreamNotSynchronized) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::NOT_SYNCHRONIZED,
        StopCaptureDirectiveSchedule::BEFORE_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after receiving a stop capture directive and the
 * recognize event stream has not been sent due to an internal error within ACL.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnDirectiveAndStreamInternalrror) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::INTERNAL_ERROR,
        StopCaptureDirectiveSchedule::BEFORE_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after receiving a stop capture directive and the
 * recognize event stream has not been sent due to an underlying protocol error.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnDirectiveAndStreamProtocolError) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::PROTOCOL_ERROR,
        StopCaptureDirectiveSchedule::BEFORE_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after receiving a stop capture directive and the
 * recognize event stream has not been sent due to an internal error on the server which sends code 500.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnDirectiveAndStreamServerInternalError) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2,
        StopCaptureDirectiveSchedule::BEFORE_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after receiving a stop capture directive and the
 * recognize event stream has not been sent due to server refusing the request.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnDirectiveAndStreamRefused) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::REFUSED,
        StopCaptureDirectiveSchedule::BEFORE_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after receiving a stop capture directive and the
 * recognize event stream has not been sent due to server canceling it before the transmission completed.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnDirectiveAndStreamCanceled) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::CANCELED,
        StopCaptureDirectiveSchedule::BEFORE_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after receiving a stop capture directive and the
 * recognize event stream has not been sent due to excessive load on the server.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnDirectiveAndStreamThrottled) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::THROTTLED,
        StopCaptureDirectiveSchedule::BEFORE_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after receiving a stop capture directive and the
 * recognize event stream has not been sent due to the access credentials provided to ACL were invalid.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnDirectiveAndStreamInvalidAuth) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::INVALID_AUTH,
        StopCaptureDirectiveSchedule::BEFORE_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after receiving a stop capture directive and the
 * recognize event stream has not been sent due to invalid request sent by the user.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnDirectiveAndStreamBadRequest) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::BAD_REQUEST,
        StopCaptureDirectiveSchedule::BEFORE_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after receiving a stop capture directive and the
 * recognize event stream has not been sent due to unknown server error.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnDirectiveAndStreamUnknownServerError) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SERVER_OTHER_ERROR,
        StopCaptureDirectiveSchedule::BEFORE_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after the recognize event stream has been
 * successfully sent and a stop capture directive is received.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamSuccessAndDirective) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS,
        StopCaptureDirectiveSchedule::AFTER_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::BUSY,
        false);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after the recognize event stream has been
 * successfully sent but received no HTTP/2 content and a stop capture directive is received.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamSuccessNoContentAndDirective) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT,
        StopCaptureDirectiveSchedule::AFTER_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::BUSY,
        false);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after the recognize event stream has not
 * been sent due to connection to AVS has been severed and a stop capture directive is received.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamSuccessNotConnectedAndDirective) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::NOT_CONNECTED,
        StopCaptureDirectiveSchedule::AFTER_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after the recognize event stream has not
 * been sent due to AVS is not synchronized and a stop capture directive is received.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamNotSynchronizedAndDirective) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::NOT_SYNCHRONIZED,
        StopCaptureDirectiveSchedule::AFTER_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after the recognize event stream has not
 * been sent due to an internal error within ACL and a stop capture directive is received.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamInternalrrorAndDirective) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::INTERNAL_ERROR,
        StopCaptureDirectiveSchedule::AFTER_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after the recognize event stream has not
 * been sent due to an underlying protocol error and a stop capture directive is received.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamProtocolErrorAndDirective) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::PROTOCOL_ERROR,
        StopCaptureDirectiveSchedule::AFTER_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after the recognize event stream has not
 * been sent due to an internal error on the server which sends code 500  and a stop capture directive is received.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamServerInternalErrorAndDirective) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2,
        StopCaptureDirectiveSchedule::AFTER_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after the recognize event stream has not
 * been sent due to server refusing the request and a stop capture directive is received.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamRefusedAndDirective) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::REFUSED,
        StopCaptureDirectiveSchedule::AFTER_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after the recognize event stream has not
 * been sent due to server canceling it before the transmission completed and a stop capture directive is received.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamCanceledAndDirective) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::CANCELED,
        StopCaptureDirectiveSchedule::AFTER_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after the recognize event stream has not
 * been sent due to excessive load on the server and a stop capture directive is received.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamThrottledAndDirective) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::THROTTLED,
        StopCaptureDirectiveSchedule::AFTER_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after the recognize event stream has not
 * been sent due to the access credentials provided to ACL were invalid and a stop capture directive is received.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamInvalidAuthAndDirective) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::INVALID_AUTH,
        StopCaptureDirectiveSchedule::AFTER_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after the recognize event stream has not
 * been sent due to invalid request sent by the user and a stop capture directive is received.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamBadRequestAndDirective) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::BAD_REQUEST,
        StopCaptureDirectiveSchedule::AFTER_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/**
 * This function verifies that @c AudioInputProcessor state is correct after the recognize event stream has not
 * been sent due to unknown server error and a stop capture directive is received.
 */
TEST_F(AudioInputProcessorTest, stopCaptureOnStreamUnknownServerErrorAndDirective) {
    testAIPStateTransitionOnEventFinish(
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SERVER_OTHER_ERROR,
        StopCaptureDirectiveSchedule::AFTER_EVENT_STREAM_CLOSE,
        AudioInputProcessorObserverInterface::State::IDLE,
        true);
}

/*
 * This function verifies that the SET_END_OF_SPEECH_OFFSET directive is handled properly in the successful case
 */
TEST_F(AudioInputProcessorTest, handleSetEndOfSpeechOffsetSuccess) {
    rapidjson::Document document(rapidjson::kObjectType);
    rapidjson::Value payloadJson(rapidjson::kObjectType);
    rapidjson::Value endOfSpeechOffsetMilliseconds(END_OF_SPEECH_OFFSET_IN_MILLISECONDS);
    rapidjson::Value startOfSpeechTimestamp(rapidjson::StringRef(START_OF_SPEECH_TIMESTAMP_STR));
    payloadJson.AddMember(
        rapidjson::StringRef(START_OF_SPEECH_TIMESTAMP_FIELD_NAME), startOfSpeechTimestamp, document.GetAllocator());
    payloadJson.AddMember(
        rapidjson::StringRef(END_OF_SPEECH_OFFSET_FIELD_NAME), endOfSpeechOffsetMilliseconds, document.GetAllocator());
    auto avsDirective = createAVSDirective(SET_END_OF_SPEECH_OFFSET, true, true, document, payloadJson);
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> directiveHandler = m_audioInputProcessor;
    auto result = avsCommon::utils::memory::make_unique<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>();
    EXPECT_CALL(*result, setCompleted());
    directiveHandler->preHandleDirective(avsDirective, std::move(result));
    EXPECT_TRUE(directiveHandler->handleDirective(avsDirective->getMessageId()));
}

/**
 * This function verifies that the SET_END_OF_SPEECH_OFFSET directive gracefully handles invalid offset values
 */
TEST_F(AudioInputProcessorTest, handleSetEndOfSpeechOffsetFailureInvalid) {
    rapidjson::Document document(rapidjson::kObjectType);
    rapidjson::Value payloadJson(rapidjson::kObjectType);
    rapidjson::Value badValue("foobar");
    payloadJson.AddMember(rapidjson::StringRef(END_OF_SPEECH_OFFSET_FIELD_NAME), badValue, document.GetAllocator());
    auto avsDirective = createAVSDirective(SET_END_OF_SPEECH_OFFSET, true, true, document, payloadJson);
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> directiveHandler = m_audioInputProcessor;
    auto result = avsCommon::utils::memory::make_unique<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>();
    EXPECT_CALL(*result, setFailed(_));
    directiveHandler->preHandleDirective(avsDirective, std::move(result));
    EXPECT_TRUE(directiveHandler->handleDirective(avsDirective->getMessageId()));
}

/**
 * This function verifies that the SET_END_OF_SPEECH_OFFSET directive gracefully handles missing offset values
 */
TEST_F(AudioInputProcessorTest, handleSetEndOfSpeechOffsetFailureMissing) {
    rapidjson::Document document(rapidjson::kObjectType);
    rapidjson::Value payloadJson(rapidjson::kObjectType);
    auto avsDirective = createAVSDirective(SET_END_OF_SPEECH_OFFSET, true, true, document, payloadJson);
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> directiveHandler = m_audioInputProcessor;
    auto result = avsCommon::utils::memory::make_unique<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>();
    EXPECT_CALL(*result, setFailed(_));
    directiveHandler->preHandleDirective(avsDirective, std::move(result));
    EXPECT_TRUE(directiveHandler->handleDirective(avsDirective->getMessageId()));
}

}  // namespace test
}  // namespace aip
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
