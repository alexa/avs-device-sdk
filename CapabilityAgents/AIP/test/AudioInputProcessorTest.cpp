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
#include <AVSCommon/SDKInterfaces/MockUserActivityNotifier.h>
#include <AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h>
#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Memory/Memory.h>

#include "AIP/AudioInputProcessor.h"
#include "MockObserver.h"

using namespace testing;
using namespace alexaClientSDK::avsCommon::utils::json;

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

/// JSON key for the wakeword field in SpeechRecognizer context state.
static const std::string STATE_WAKEWORD_KEY = "wakeword";

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

/// Value used in tests for a state request token by the context manager.
static const unsigned int STATE_REQUEST_TOKEN = 12345;

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
        const ESPData& espData = ESPData::EMPTY_ESP_DATA);

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

    /// The attachment reader saved by a call to @c verifyMessage().
    std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> m_reader;
};

RecognizeEvent::RecognizeEvent(
    AudioProvider audioProvider,
    Initiator initiator,
    avsCommon::avs::AudioInputStream::Index begin,
    avsCommon::avs::AudioInputStream::Index keywordEnd,
    std::string keyword,
    std::shared_ptr<std::string> avsInitiator,
    const ESPData& espData) :
        m_audioProvider{audioProvider},
        m_initiator{initiator},
        m_begin{begin},
        m_keywordEnd{keywordEnd},
        m_keyword{keyword},
        m_avsInitiator{avsInitiator},
        m_espData{espData} {
}

std::future<bool> RecognizeEvent::send(std::shared_ptr<AudioInputProcessor> audioInputProcessor) {
    auto result =
        audioInputProcessor->recognize(m_audioProvider, m_initiator, m_begin, m_keywordEnd, m_keyword, m_espData);
    EXPECT_TRUE(result.valid());
    return result;
}

void RecognizeEvent::verifyJsonState(
    const avsCommon::avs::NamespaceAndName&,
    const std::string& jsonState,
    const avsCommon::avs::StateRefreshPolicy&,
    const unsigned int) {
    rapidjson::Document document = parseJson(jsonState);
    EXPECT_EQ(getJsonString(document, STATE_WAKEWORD_KEY), m_keyword);
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

        if (m_initiator == Initiator::WAKEWORD && m_begin != AudioInputProcessor::INVALID_INDEX &&
            m_keywordEnd != AudioInputProcessor::INVALID_INDEX) {
            auto wakeWordIndices = initiatorPayload->value.FindMember(WAKE_WORD_INDICES_KEY);
            EXPECT_NE(wakeWordIndices, initiatorPayload->value.MemberEnd());

            if (wakeWordIndices != initiatorPayload->value.MemberEnd()) {
                EXPECT_EQ(getJsonInt64(wakeWordIndices->value, START_INDEX_KEY), static_cast<int64_t>(m_begin));
                EXPECT_EQ(getJsonInt64(wakeWordIndices->value, END_INDEX_KEY), static_cast<int64_t>(m_keywordEnd));
            }
        }
    }

    m_reader = request->getAttachmentReader();
    EXPECT_NE(m_reader, nullptr);

    std::vector<Sample> samples(PATTERN_WORDS);
    size_t samplesRead = 0;
    auto t0 = std::chrono::steady_clock::now();
    do {
        avsCommon::avs::attachment::AttachmentReader::ReadStatus status;
        auto bytesRead =
            m_reader->read(samples.data() + samplesRead, (samples.size() - samplesRead) * SDS_WORDSIZE, &status);
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
    return m_reader;
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
        const ESPData& espData = ESPData::EMPTY_ESP_DATA);

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

    /// The mock @c UserActivityNotifierInterface.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockUserActivityNotifier> m_mockUserActivityNotifier;

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
    m_mockUserActivityNotifier = std::make_shared<avsCommon::sdkInterfaces::test::MockUserActivityNotifier>();
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
    EXPECT_CALL(*m_mockContextManager, setStateProvider(RECOGNIZER_STATE, Ne(nullptr)));
    m_audioInputProcessor = AudioInputProcessor::create(
        m_mockDirectiveSequencer,
        m_mockMessageSender,
        m_mockContextManager,
        m_mockFocusManager,
        m_dialogUXStateAggregator,
        m_mockExceptionEncounteredSender,
        m_mockUserActivityNotifier,
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
    const ESPData& espData) {
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
    m_recognizeEvent =
        std::make_shared<RecognizeEvent>(audioProvider, initiator, begin, keywordEnd, keyword, avsInitiator, espData);
    if (keyword.empty()) {
        EXPECT_CALL(*m_mockContextManager, getContext(_)).WillOnce(InvokeWithoutArgs([this] {
            m_audioInputProcessor->provideState(STOP_CAPTURE, STATE_REQUEST_TOKEN);
        }));
        EXPECT_CALL(
            *m_mockContextManager,
            setState(RECOGNIZER_STATE, _, avsCommon::avs::StateRefreshPolicy::NEVER, STATE_REQUEST_TOKEN))
            .WillOnce(DoAll(
                Invoke(m_recognizeEvent.get(), &RecognizeEvent::verifyJsonState),
                InvokeWithoutArgs([this, contextJson, stopPoint] {
                    m_audioInputProcessor->onContextAvailable(contextJson);
                    if (RecognizeStopPoint::AFTER_CONTEXT == stopPoint) {
                        EXPECT_TRUE(m_audioInputProcessor->stopCapture().valid());
                    }
                    return avsCommon::sdkInterfaces::SetStateResult::SUCCESS;
                })));
    } else {
        // Enforce the sequence; setState needs to be called before getContext, otherwise ContextManager will not
        // include the new state in the context for this recognize.
        InSequence dummy;

        EXPECT_CALL(*m_mockContextManager, setState(RECOGNIZER_STATE, _, avsCommon::avs::StateRefreshPolicy::NEVER, 0))
            .WillOnce(DoAll(Invoke(m_recognizeEvent.get(), &RecognizeEvent::verifyJsonState), InvokeWithoutArgs([] {
                                return avsCommon::sdkInterfaces::SetStateResult::SUCCESS;
                            })));
        EXPECT_CALL(*m_mockContextManager, getContext(_)).WillOnce(InvokeWithoutArgs([this, contextJson, stopPoint] {
            m_audioInputProcessor->onContextAvailable(contextJson);
            if (RecognizeStopPoint::AFTER_CONTEXT == stopPoint) {
                EXPECT_TRUE(m_audioInputProcessor->stopCapture().valid());
            }
        }));
    }

    if (!bargeIn) {
        EXPECT_CALL(*m_mockUserActivityNotifier, onUserActive()).Times(2);
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
                    m_audioInputProcessor->onSendCompleted(
                        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS);
                }));
        }
        EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
            .WillOnce(DoAll(
                Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
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
    EXPECT_CALL(*m_mockUserActivityNotifier, onUserActive()).Times(2);
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
    EXPECT_CALL(*m_mockUserActivityNotifier, onUserActive()).Times(2);
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

    auto avsDirective = createAVSDirective(EXPECT_SPEECH, WITH_DIALOG_REQUEST_ID);
    auto result = avsCommon::utils::memory::make_unique<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>();
    if (WITH_DIALOG_REQUEST_ID) {
        EXPECT_CALL(*result, setFailed(_)).WillOnce(InvokeWithoutArgs([&] {
            std::lock_guard<std::mutex> lock(mutex);
            done = true;
            conditionVariable.notify_one();
        }));
    }
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> directiveHandler = m_audioInputProcessor;

    if (!WITH_DIALOG_REQUEST_ID) {
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
    EXPECT_CALL(*m_mockUserActivityNotifier, onUserActive()).Times(2);
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
    bool WITH_DIALOG_REQUEST_ID,
    bool withInitiator) {
    auto header = std::make_shared<avsCommon::avs::AVSMessageHeader>(
        directive.nameSpace,
        directive.name,
        avsCommon::utils::uuidGeneration::generateUUID(),
        avsCommon::utils::uuidGeneration::generateUUID());

    rapidjson::Document document(rapidjson::kObjectType);
    rapidjson::Value directiveJson(rapidjson::kObjectType);
    rapidjson::Value headerJson(rapidjson::kObjectType);
    rapidjson::Value payloadJson(rapidjson::kObjectType);
    rapidjson::Value namespaceJson(header->getNamespace(), document.GetAllocator());
    rapidjson::Value nameJson(header->getName(), document.GetAllocator());
    rapidjson::Value messageIdJson(header->getMessageId(), document.GetAllocator());
    rapidjson::Value dialogRequestIdJson(header->getDialogRequestId(), document.GetAllocator());

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

    EXPECT_EQ(request->getAttachmentReader(), nullptr);
}

void AudioInputProcessorTest::removeDefaultAudioProvider() {
    EXPECT_CALL(*m_mockContextManager, setStateProvider(RECOGNIZER_STATE, Ne(nullptr)));
    m_audioInputProcessor->removeObserver(m_dialogUXStateAggregator);
    m_audioInputProcessor = AudioInputProcessor::create(
        m_mockDirectiveSequencer,
        m_mockMessageSender,
        m_mockContextManager,
        m_mockFocusManager,
        m_dialogUXStateAggregator,
        m_mockExceptionEncounteredSender,
        m_mockUserActivityNotifier);
    EXPECT_NE(m_audioInputProcessor, nullptr);
    m_audioInputProcessor->addObserver(m_mockObserver);
    m_audioInputProcessor->addObserver(m_dialogUXStateAggregator);
}

void AudioInputProcessorTest::makeDefaultAudioProviderNotAlwaysReadable() {
    m_audioProvider->alwaysReadable = false;
    EXPECT_CALL(*m_mockContextManager, setStateProvider(RECOGNIZER_STATE, Ne(nullptr)));
    m_audioInputProcessor->removeObserver(m_dialogUXStateAggregator);
    m_audioInputProcessor = AudioInputProcessor::create(
        m_mockDirectiveSequencer,
        m_mockMessageSender,
        m_mockContextManager,
        m_mockFocusManager,
        m_dialogUXStateAggregator,
        m_mockExceptionEncounteredSender,
        m_mockUserActivityNotifier,
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
        m_mockUserActivityNotifier,
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
        m_mockUserActivityNotifier,
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
        m_mockUserActivityNotifier,
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
        m_mockUserActivityNotifier,
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
        m_mockUserActivityNotifier,
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
        m_mockUserActivityNotifier,
        *m_audioProvider);
    EXPECT_EQ(m_audioInputProcessor, nullptr);
}

/**
 * Function to verify that @c AudioInputProcessor::create() errors out with an invalid
 * @c UserActivityNotifierInterface.
 */
TEST_F(AudioInputProcessorTest, createWithoutUserActivityNotifier) {
    m_audioInputProcessor->removeObserver(m_dialogUXStateAggregator);
    m_audioInputProcessor = AudioInputProcessor::create(
        m_mockDirectiveSequencer,
        m_mockMessageSender,
        m_mockContextManager,
        m_mockFocusManager,
        m_dialogUXStateAggregator,
        m_mockExceptionEncounteredSender,
        nullptr,
        *m_audioProvider);
    EXPECT_EQ(m_audioInputProcessor, nullptr);
}

/// Function to verify that @c AudioInputProcessor::create() succeeds with a null @c AudioProvider.
TEST_F(AudioInputProcessorTest, createWithoutAudioProvider) {
    EXPECT_CALL(*m_mockContextManager, setStateProvider(RECOGNIZER_STATE, Ne(nullptr)));
    m_audioInputProcessor->removeObserver(m_dialogUXStateAggregator);
    m_audioInputProcessor = AudioInputProcessor::create(
        m_mockDirectiveSequencer,
        m_mockMessageSender,
        m_mockContextManager,
        m_mockFocusManager,
        m_dialogUXStateAggregator,
        m_mockExceptionEncounteredSender,
        m_mockUserActivityNotifier,
        AudioProvider::null());
    EXPECT_NE(m_audioInputProcessor, nullptr);
}

/// Function to verify that @c AudioInputProcessor::getconfiguration() returns the expected configuration data.
TEST_F(AudioInputProcessorTest, getConfiguration) {
    auto configuration = m_audioInputProcessor->getConfiguration();
    EXPECT_EQ(configuration.size(), NUM_DIRECTIVES);
    for (auto namespaceAndName : DIRECTIVES) {
        auto directive = configuration.find(namespaceAndName);
        EXPECT_NE(directive, configuration.end());
        if (configuration.end() == directive) {
            continue;
        }
        EXPECT_EQ(directive->second, BLOCKING_POLICY);
    }
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
 * This function verifies that @c AudioInputProcessor::recognize() fails in @c State::RECOGNIZING when the previous
 * recognize used the NEAR_FIELD profile.
 */
TEST_F(AudioInputProcessorTest, recognizeBargeInWhileRecognizingNearField) {
    auto audioProvider = *m_audioProvider;
    audioProvider.profile = ASRProfile::NEAR_FIELD;
    ASSERT_TRUE(testRecognizeSucceeds(audioProvider, Initiator::TAP));
    ASSERT_TRUE(testRecognizeFails(*m_audioProvider, Initiator::TAP));
}

/**
 * This function verifies that @c AudioInputProcessor::recognize() fails in @c State::RECOGNIZING when the previous
 * recognize used the FAR_FIELD profile.
 */
TEST_F(AudioInputProcessorTest, recognizeBargeInWhileRecognizingFarField) {
    auto audioProvider = *m_audioProvider;
    audioProvider.profile = ASRProfile::FAR_FIELD;
    ASSERT_TRUE(testRecognizeSucceeds(audioProvider, Initiator::TAP));
    ASSERT_TRUE(testRecognizeFails(*m_audioProvider, Initiator::TAP));
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
}  // namespace test
}  // namespace aip
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
