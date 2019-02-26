/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <sstream>

#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/FocusState.h>
#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/Metrics.h>
#include <AVSCommon/Utils/String/StringUtils.h>
#include <AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h>
#include <AVSCommon/AVS/Attachment/AttachmentUtils.h>
#include <SpeechEncoder/SpeechEncoder.h>

#include "AIP/AudioInputProcessor.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace aip {
using namespace avsCommon::avs;
using namespace avsCommon::utils;
using namespace avsCommon::utils::logger;

/// SpeechRecognizer capability constants
/// SpeechRecognizer interface type
static const std::string SPEECHRECOGNIZER_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";
/// SpeechRecognizer interface name
static const std::string SPEECHRECOGNIZER_CAPABILITY_INTERFACE_NAME = "SpeechRecognizer";
/// SpeechRecognizer interface version
static const std::string SPEECHRECOGNIZER_CAPABILITY_INTERFACE_VERSION = "2.1";

/// String to identify log entries originating from this file.
static const std::string TAG("AudioInputProcessor");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

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

/// The SpeechRecognizer context state signature.
static const avsCommon::avs::NamespaceAndName RECOGNIZER_STATE{NAMESPACE, "RecognizerState"};

/// The field identifying the initiator.
static const std::string INITIATOR_KEY = "initiator";

/// The field identifying the initiator's profile.
static const std::string PROFILE_KEY = "profile";

/// The field identifying the initiator's format.
static const std::string FORMAT_KEY = "format";

/// The field identifying the initiator's type.
static const std::string TYPE_KEY = "type";

/// The field identifying the initiator's payload.
static const std::string PAYLOAD_KEY = "payload";

/// The field identifying the initiator's wakeword indices.
static const std::string WAKEWORD_INDICES_KEY = "wakeWordIndices";

/// The field identifying the initiator's wakeword start index.
static const std::string START_INDEX_KEY = "startIndexInSamples";

/// The field identifying the initiator's wakeword end index.
static const std::string END_INDEX_KEY = "endIndexInSamples";

/// The field identifying the initiator's wake word.
static const std::string WAKE_WORD_KEY = "wakeWord";

/// The field identifying the ESP voice energy.
static const std::string VOICE_ENERGY_KEY = "voiceEnergy";

/// The field identifying the ESP ambient energy.
static const std::string AMBIENT_ENERGY_KEY = "ambientEnergy";

/// The field name for the user voice attachment.
static const std::string AUDIO_ATTACHMENT_FIELD_NAME = "audio";

/// The field name for the wake word engine metadata.
static const std::string KWD_METADATA_FIELD_NAME = "wakewordEngineMetadata";

/// The field name for the start of speech timestamp, reported in milliseconds since epoch. This field is provided to
/// the Recognize event and it's sent back as part of SetEndOfSpeechOffset payload.
static const std::string START_OF_SPEECH_TIMESTAMP_FIELD_NAME = "startOfSpeechTimestamp";

/// The field name for the end of speech offset, reported in milliseconds, as part of SetEndOfSpeechOffset payload.
static const std::string END_OF_SPEECH_OFFSET_FIELD_NAME = "endOfSpeechOffsetInMilliseconds";

/**
 * Creates the SpeechRecognizer capability configuration.
 *
 * @return The SpeechRecognizer capability configuration.
 */
static std::shared_ptr<avsCommon::avs::CapabilityConfiguration> getSpeechRecognizerCapabilityConfiguration();

std::shared_ptr<AudioInputProcessor> AudioInputProcessor::create(
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> directiveSequencer,
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
    std::shared_ptr<avsCommon::avs::DialogUXStateAggregator> dialogUXStateAggregator,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<avsCommon::sdkInterfaces::UserInactivityMonitorInterface> userInactivityMonitor,
    std::shared_ptr<speechencoder::SpeechEncoder> speechEncoder,
    AudioProvider defaultAudioProvider) {
    if (!directiveSequencer) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullDirectiveSequencer"));
        return nullptr;
    } else if (!messageSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMessageSender"));
        return nullptr;
    } else if (!contextManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullContextManager"));
        return nullptr;
    } else if (!focusManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullFocusManager"));
        return nullptr;
    } else if (!dialogUXStateAggregator) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullDialogUXStateAggregator"));
        return nullptr;
    } else if (!exceptionEncounteredSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionEncounteredSender"));
        return nullptr;
    } else if (!userInactivityMonitor) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullUserInctivityMonitor"));
        return nullptr;
    }

    auto aip = std::shared_ptr<AudioInputProcessor>(new AudioInputProcessor(
        directiveSequencer,
        messageSender,
        contextManager,
        focusManager,
        exceptionEncounteredSender,
        userInactivityMonitor,
        speechEncoder,
        defaultAudioProvider));

    if (aip) {
        dialogUXStateAggregator->addObserver(aip);
    }

    return aip;
}

avsCommon::avs::DirectiveHandlerConfiguration AudioInputProcessor::getConfiguration() const {
    avsCommon::avs::DirectiveHandlerConfiguration configuration;
    configuration[STOP_CAPTURE] = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    configuration[EXPECT_SPEECH] = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, true);
    configuration[SET_END_OF_SPEECH_OFFSET] = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    return configuration;
}

void AudioInputProcessor::addObserver(std::shared_ptr<ObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("addObserverFailed").d("reason", "nullObserver"));
        return;
    }
    m_executor.submit([this, observer]() { m_observers.insert(observer); });
}

void AudioInputProcessor::removeObserver(std::shared_ptr<ObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("removeObserverFailed").d("reason", "nullObserver"));
        return;
    }
    m_executor.submit([this, observer]() { m_observers.erase(observer); }).wait();
}

std::future<bool> AudioInputProcessor::recognize(
    AudioProvider audioProvider,
    Initiator initiator,
    std::chrono::steady_clock::time_point startOfSpeechTimestamp,
    avsCommon::avs::AudioInputStream::Index begin,
    avsCommon::avs::AudioInputStream::Index keywordEnd,
    std::string keyword,
    const ESPData espData,
    std::shared_ptr<const std::vector<char>> KWDMetadata) {
    ACSDK_METRIC_IDS(TAG, "Recognize", "", "", Metrics::Location::AIP_RECEIVE);

    // If no begin index was provided, grab the current index ASAP so that we can start streaming from the time this
    // call was made.
    if (audioProvider.stream && INVALID_INDEX == begin) {
        static const bool startWithNewData = true;
        auto reader = audioProvider.stream->createReader(
            avsCommon::avs::AudioInputStream::Reader::Policy::NONBLOCKING, startWithNewData);
        if (!reader) {
            ACSDK_ERROR(LX("recognizeFailed").d("reason", "createReaderFailed"));
            std::promise<bool> ret;
            ret.set_value(false);
            return ret.get_future();
        }
        begin = reader->tell();
    }

    if (!espData.isEmpty()) {
        m_executor.submit([this, espData]() { executePrepareEspPayload(espData); });
    }

    return m_executor.submit(
        [this, audioProvider, initiator, startOfSpeechTimestamp, begin, keywordEnd, keyword, KWDMetadata]() {
            return executeRecognize(
                audioProvider, initiator, startOfSpeechTimestamp, begin, keywordEnd, keyword, KWDMetadata);
        });
}

std::future<bool> AudioInputProcessor::stopCapture() {
    return m_executor.submit([this]() { return executeStopCapture(); });
}

std::future<void> AudioInputProcessor::resetState() {
    return m_executor.submit([this]() { executeResetState(); });
}

void AudioInputProcessor::onContextAvailable(const std::string& jsonContext) {
    m_executor.submit([this, jsonContext]() { executeOnContextAvailable(jsonContext); });
}

void AudioInputProcessor::onContextFailure(const avsCommon::sdkInterfaces::ContextRequestError error) {
    m_executor.submit([this, error]() { executeOnContextFailure(error); });
}

void AudioInputProcessor::handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) {
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void AudioInputProcessor::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
}

void AudioInputProcessor::handleDirective(std::shared_ptr<DirectiveInfo> info) {
    if (!info) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }
    if (info->directive->getName() == STOP_CAPTURE.name) {
        ACSDK_METRIC_MSG(TAG, info->directive, Metrics::Location::AIP_RECEIVE);
        handleStopCaptureDirective(info);
    } else if (info->directive->getName() == EXPECT_SPEECH.name) {
        handleExpectSpeechDirective(info);
    } else if (info->directive->getName() == SET_END_OF_SPEECH_OFFSET.name) {
        handleSetEndOfSpeechOffsetDirective(info);
    } else {
        std::string errorMessage =
            "unexpected directive " + info->directive->getNamespace() + ":" + info->directive->getName();
        m_exceptionEncounteredSender->sendExceptionEncountered(
            info->directive->getUnparsedDirective(),
            avsCommon::avs::ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED,
            errorMessage);
        if (info->result) {
            info->result->setFailed(errorMessage);
        }
        ACSDK_ERROR(LX("handleDirectiveFailed")
                        .d("reason", "unknownDirective")
                        .d("namespace", info->directive->getNamespace())
                        .d("name", info->directive->getName()));
        removeDirective(info);
    }
}

void AudioInputProcessor::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
    removeDirective(info);
}

void AudioInputProcessor::onDeregistered() {
    resetState();
}

void AudioInputProcessor::onFocusChanged(avsCommon::avs::FocusState newFocus) {
    ACSDK_DEBUG9(LX("onFocusChanged").d("newFocus", newFocus));
    m_executor.submit([this, newFocus]() { executeOnFocusChanged(newFocus); });
}

void AudioInputProcessor::onDialogUXStateChanged(
    avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState newState) {
    m_executor.submit([this, newState]() { executeOnDialogUXStateChanged(newState); });
}

AudioInputProcessor::AudioInputProcessor(
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> directiveSequencer,
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<avsCommon::sdkInterfaces::UserInactivityMonitorInterface> userInactivityMonitor,
    std::shared_ptr<speechencoder::SpeechEncoder> speechEncoder,
    AudioProvider defaultAudioProvider) :
        CapabilityAgent{NAMESPACE, exceptionEncounteredSender},
        RequiresShutdown{"AudioInputProcessor"},
        m_directiveSequencer{directiveSequencer},
        m_messageSender{messageSender},
        m_contextManager{contextManager},
        m_focusManager{focusManager},
        m_userInactivityMonitor{userInactivityMonitor},
        m_encoder{speechEncoder},
        m_defaultAudioProvider{defaultAudioProvider},
        m_lastAudioProvider{AudioProvider::null()},
        m_KWDMetadataReader{nullptr},
        m_state{ObserverInterface::State::IDLE},
        m_focusState{avsCommon::avs::FocusState::NONE},
        m_preparingToSend{false},
        m_initialDialogUXStateReceived{false},
        m_localStopCapturePerformed{false},
        m_precedingExpectSpeechInitiator{nullptr} {
    m_capabilityConfigurations.insert(getSpeechRecognizerCapabilityConfiguration());
}

std::shared_ptr<avsCommon::avs::CapabilityConfiguration> getSpeechRecognizerCapabilityConfiguration() {
    std::unordered_map<std::string, std::string> configMap;
    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, SPEECHRECOGNIZER_CAPABILITY_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, SPEECHRECOGNIZER_CAPABILITY_INTERFACE_NAME});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, SPEECHRECOGNIZER_CAPABILITY_INTERFACE_VERSION});

    return std::make_shared<avsCommon::avs::CapabilityConfiguration>(configMap);
}

void AudioInputProcessor::doShutdown() {
    m_executor.shutdown();
    executeResetState();
    m_directiveSequencer.reset();
    m_messageSender.reset();
    m_contextManager.reset();
    m_focusManager.reset();
    m_userInactivityMonitor.reset();
    m_observers.clear();
}

std::future<bool> AudioInputProcessor::expectSpeechTimedOut() {
    return m_executor.submit([this]() { return executeExpectSpeechTimedOut(); });
}

void AudioInputProcessor::handleStopCaptureDirective(std::shared_ptr<DirectiveInfo> info) {
    m_executor.submit([this, info]() {
        bool stopImmediately = true;
        executeStopCapture(stopImmediately, info);
    });
}

void AudioInputProcessor::handleExpectSpeechDirective(std::shared_ptr<DirectiveInfo> info) {
    int64_t timeout;
    bool found = avsCommon::utils::json::jsonUtils::retrieveValue(
        info->directive->getPayload(), "timeoutInMilliseconds", &timeout);

    if (!found) {
        static const char* errorMessage = "missing/invalid timeoutInMilliseconds";
        m_exceptionEncounteredSender->sendExceptionEncountered(
            info->directive->getUnparsedDirective(),
            avsCommon::avs::ExceptionErrorType::UNSUPPORTED_OPERATION,
            errorMessage);
        if (info->result) {
            info->result->setFailed(errorMessage);
        }
        ACSDK_ERROR(LX("handleExpectSpeechDirectiveFailed")
                        .d("reason", "missingJsonField")
                        .d("field", "timeoutInMilliseconds"));
        removeDirective(info);
        return;
    }

    m_executor.submit([this, timeout, info]() { executeExpectSpeech(std::chrono::milliseconds{timeout}, info); });
}

void AudioInputProcessor::handleSetEndOfSpeechOffsetDirective(std::shared_ptr<DirectiveInfo> info) {
    auto payload = info->directive->getPayload();
    int64_t endOfSpeechOffset = 0;
    int64_t startOfSpeechTimestamp = 0;
    std::string startOfSpeechTimeStampInString;
    bool foundEnd = json::jsonUtils::retrieveValue(payload, END_OF_SPEECH_OFFSET_FIELD_NAME, &endOfSpeechOffset);
    bool foundStart =
        json::jsonUtils::retrieveValue(payload, START_OF_SPEECH_TIMESTAMP_FIELD_NAME, &startOfSpeechTimeStampInString);

    if (foundEnd && foundStart) {
        std::istringstream iss{startOfSpeechTimeStampInString};
        iss >> startOfSpeechTimestamp;

        ACSDK_DEBUG0(LX("handleSetEndOfSpeechOffsetDirective")
                         .d("startTimeSpeech(ms)", startOfSpeechTimestamp)
                         .d("endTimeSpeech(ms)", startOfSpeechTimestamp + endOfSpeechOffset));
        info->result->setCompleted();
    } else {
        std::string missing;
        if (!foundEnd && !foundStart) {
            missing = END_OF_SPEECH_OFFSET_FIELD_NAME + " and " + START_OF_SPEECH_TIMESTAMP_FIELD_NAME;
        } else if (!foundEnd) {
            missing = END_OF_SPEECH_OFFSET_FIELD_NAME;
        } else {
            missing = START_OF_SPEECH_TIMESTAMP_FIELD_NAME;
        }

        ACSDK_ERROR(LX("handleSetEndOfSpeechOffsetDirective").d("missing", missing));
        info->result->setFailed("Missing parameter(s): " + missing);
    }
    removeDirective(info);
}

void AudioInputProcessor::executePrepareEspPayload(const ESPData espData) {
    m_espPayload.clear();
    if (!espData.verify()) {
        // Log an error as the values are invalid, but we should continue to send the recognize event.
        ACSDK_ERROR(LX("executeRecognizeFailed")
                        .d("reason", "invalidEspData")
                        .d("voiceEnergy", espData.getVoiceEnergy())
                        .d("ambientEnergy", espData.getAmbientEnergy()));
    } else {
        int64_t voiceEnergy = 0;
        int64_t ambientEnergy = 0;
        if (!string::stringToInt64(espData.getAmbientEnergy(), &ambientEnergy) ||
            !string::stringToInt64(espData.getVoiceEnergy(), &voiceEnergy)) {
            ACSDK_ERROR(LX("executePrepareEspPayloadFailed")
                            .d("reason", "invalidESPData")
                            .d("ambientEnergy", espData.getAmbientEnergy())
                            .d("voiceEnergy", espData.getVoiceEnergy()));
        }
        // Assemble the event payload.
        json::JsonGenerator jsonGenerator;
        jsonGenerator.addMember(VOICE_ENERGY_KEY, voiceEnergy);
        jsonGenerator.addMember(AMBIENT_ENERGY_KEY, ambientEnergy);

        // Record the ReportEchoSpatialPerceptionData event payload for later use by executeContextAvailable().
        m_espPayload = jsonGenerator.toString();
    }
    m_espRequest.reset();
}

bool AudioInputProcessor::executeRecognize(
    AudioProvider provider,
    Initiator initiator,
    std::chrono::steady_clock::time_point startOfSpeechTimestamp,
    avsCommon::avs::AudioInputStream::Index begin,
    avsCommon::avs::AudioInputStream::Index end,
    const std::string& keyword,
    std::shared_ptr<const std::vector<char>> KWDMetadata) {
    // Make sure we have a keyword if this is a wakeword initiator.
    if (Initiator::WAKEWORD == initiator && keyword.empty()) {
        ACSDK_ERROR(LX("executeRecognizeFailed").d("reason", "emptyKeywordWithWakewordInitiator"));
        return false;
    }

    // 500ms preroll.
    avsCommon::avs::AudioInputStream::Index preroll = provider.format.sampleRateHz / 2;

    // Check if we have everything we need to enable false wakeword detection.
    // TODO: Consider relaxing the hard requirement for a full 500ms preroll - ACSDK-276.
    bool falseWakewordDetection =
        Initiator::WAKEWORD == initiator && begin != INVALID_INDEX && begin >= preroll && end != INVALID_INDEX;

    // If we will be enabling false wakeword detection, add preroll and build the initiator payload.
    json::JsonGenerator generator;
    generator.addMember(TYPE_KEY, initiatorToString(initiator));
    generator.startObject(PAYLOAD_KEY);
    // If we will be enabling false wakeword detection, add preroll and build the initiator payload.
    if (falseWakewordDetection) {
        generator.startObject(WAKEWORD_INDICES_KEY);
        generator.addMember(START_INDEX_KEY, preroll);
        generator.addMember(END_INDEX_KEY, preroll + (end - begin));
        generator.finishObject();

        begin -= preroll;
    }

    if (!keyword.empty()) {
        generator.addMember(WAKE_WORD_KEY, string::stringToUpperCase(keyword));
    }

    return executeRecognize(provider, generator.toString(), startOfSpeechTimestamp, begin, keyword, KWDMetadata);
}

bool AudioInputProcessor::executeRecognize(
    AudioProvider provider,
    const std::string& initiatorJson,
    std::chrono::steady_clock::time_point startOfSpeechTimestamp,
    avsCommon::avs::AudioInputStream::Index begin,
    const std::string& keyword,
    std::shared_ptr<const std::vector<char>> KWDMetadata) {
    if (!provider.stream) {
        ACSDK_ERROR(LX("executeRecognizeFailed").d("reason", "nullAudioInputStream"));
        return false;
    }

    std::unordered_map<int, std::string> mapSampleRatesAVSEncoding = {{32000, "OPUS"}};
    std::string avsEncodingFormat;
    std::unordered_map<int, std::string>::iterator itSampleRateAVSEncoding;

    switch (provider.format.encoding) {
        case avsCommon::utils::AudioFormat::Encoding::LPCM:
            if (provider.format.sampleRateHz != 16000) {
                ACSDK_ERROR(LX("executeRecognizeFailed")
                                .d("reason", "unsupportedSampleRateForPCM")
                                .d("sampleRate", provider.format.sampleRateHz));
                return false;
            } else if (provider.format.sampleSizeInBits != 16) {
                ACSDK_ERROR(LX("executeRecognizeFailed")
                                .d("reason", "unsupportedSampleSize")
                                .d("sampleSize", provider.format.sampleSizeInBits));
                return false;
            }

            avsEncodingFormat = "AUDIO_L16_RATE_16000_CHANNELS_1";
            break;
        case avsCommon::utils::AudioFormat::Encoding::OPUS:
            itSampleRateAVSEncoding = mapSampleRatesAVSEncoding.find(provider.format.sampleRateHz);
            if (itSampleRateAVSEncoding == mapSampleRatesAVSEncoding.end()) {
                ACSDK_ERROR(LX("executeRecognizeFailed")
                                .d("reason", "unsupportedSampleRateForOPUS")
                                .d("sampleRate", provider.format.sampleRateHz));
                return false;
            }

            avsEncodingFormat = itSampleRateAVSEncoding->second;
            break;
        default:
            ACSDK_ERROR(LX("executeRecognizeFailed")
                            .d("reason", "unsupportedEncoding")
                            .d("encoding", provider.format.encoding));
            return false;
    }

    if (provider.format.endianness != avsCommon::utils::AudioFormat::Endianness::LITTLE) {
        ACSDK_ERROR(LX("executeRecognizeFailed")
                        .d("reason", "unsupportedEndianness")
                        .d("endianness", provider.format.endianness));
        return false;
    } else if (provider.format.numChannels != 1) {
        ACSDK_ERROR(LX("executeRecognizeFailed")
                        .d("reason", "unsupportedNumChannels")
                        .d("channels", provider.format.numChannels));
        return false;
    }

    // If this is a barge-in, verify that it is permitted.
    switch (m_state) {
        case ObserverInterface::State::IDLE:
        case ObserverInterface::State::EXPECTING_SPEECH:
            break;
        case ObserverInterface::State::RECOGNIZING:
            // Barge-in is only permitted if the audio providers have compatible policies.
            if (!m_lastAudioProvider.canBeOverridden) {
                ACSDK_ERROR(LX("executeRecognizeFailed").d("reason", "Active audio provider can not be overridden"));
                return false;
            }
            if (!provider.canOverride) {
                ACSDK_ERROR(LX("executeRecognizeFailed").d("reason", "New audio provider can not override"));
                return false;
            }
            // For barge-in, we should close the previous reader before creating another one.
            if (m_reader) {
                m_reader->close(
                    avsCommon::avs::attachment::AttachmentReader::ClosePoint::AFTER_DRAINING_CURRENT_BUFFER);
                m_reader.reset();
            }
            break;
        case ObserverInterface::State::BUSY:
            ACSDK_ERROR(LX("executeRecognizeFailed").d("reason", "Barge-in is not permitted while busy"));
            return false;
    }

    // Check if SpeechEncoder is available
    const bool shouldBeEncoded = (m_encoder != nullptr);
    // Set up format
    if (shouldBeEncoded && m_encoder->getContext()) {
        avsEncodingFormat = m_encoder->getContext()->getAVSFormatName();
    }

    // Assemble the event payload.
    json::JsonGenerator payloadGenerator;
    payloadGenerator.addMember(PROFILE_KEY, asrProfileToString(provider.profile));
    payloadGenerator.addMember(FORMAT_KEY, avsEncodingFormat);

    // The initiator (or lack thereof) from a previous ExpectSpeech has precedence.
    if (m_precedingExpectSpeechInitiator) {
        if (!m_precedingExpectSpeechInitiator->empty()) {
            payloadGenerator.addRawJsonMember(INITIATOR_KEY, *m_precedingExpectSpeechInitiator);
        }
        m_precedingExpectSpeechInitiator.reset();
    } else if (!initiatorJson.empty()) {
        payloadGenerator.addRawJsonMember(INITIATOR_KEY, initiatorJson);
    }

    payloadGenerator.addMember(
        START_OF_SPEECH_TIMESTAMP_FIELD_NAME, std::to_string(startOfSpeechTimestamp.time_since_epoch().count()));

    // Set up an attachment reader for the event.
    avsCommon::avs::attachment::InProcessAttachmentReader::SDSTypeIndex offset = 0;
    avsCommon::avs::attachment::InProcessAttachmentReader::SDSTypeReader::Reference reference =
        avsCommon::avs::attachment::InProcessAttachmentReader::SDSTypeReader::Reference::BEFORE_WRITER;
    if (INVALID_INDEX != begin) {
        offset = begin;
        reference = avsCommon::avs::attachment::InProcessAttachmentReader::SDSTypeReader::Reference::ABSOLUTE;
    }

    // Set up the speech encoder
    if (shouldBeEncoded) {
        ACSDK_DEBUG(LX("encodingAudio").d("format", avsEncodingFormat));
        if (!m_encoder->startEncoding(provider.stream, provider.format, offset, reference)) {
            ACSDK_ERROR(LX("executeRecognizeFailed").d("reason", "Failed to start encoder"));
            return false;
        }
        offset = 0;
        reference = avsCommon::avs::attachment::InProcessAttachmentReader::SDSTypeReader::Reference::BEFORE_WRITER;
    } else {
        ACSDK_DEBUG(LX("notEncodingAudio"));
    }

    m_reader = avsCommon::avs::attachment::InProcessAttachmentReader::create(
        sds::ReaderPolicy::NONBLOCKING,
        shouldBeEncoded ? m_encoder->getEncodedStream() : provider.stream,
        offset,
        reference);
    if (!m_reader) {
        ACSDK_ERROR(LX("executeRecognizeFailed").d("reason", "Failed to create attachment reader"));
        return false;
    }

    if (KWDMetadata) {
        m_KWDMetadataReader = avsCommon::avs::attachment::AttachmentUtils::createAttachmentReader(*KWDMetadata);
        if (!m_KWDMetadataReader) {
            ACSDK_ERROR(LX("sendingKWDMetadataFailed").d("reason", "Failed to create attachment reader"));
        }
    }

    // Code below this point changes the state of AIP.  Formally update state now, and don't error out without calling
    // executeResetState() after this point.
    setState(ObserverInterface::State::RECOGNIZING);

    // Note that we're preparing to send a Recognize event.
    m_preparingToSend = true;

    // Reset flag when we send a new recognize event.
    m_localStopCapturePerformed = false;

    //  Start assembling the context; we'll service the callback after assembling our Recognize event.
    m_contextManager->getContext(shared_from_this());

    // Stop the ExpectSpeech timer so we don't get a timeout.
    m_expectingSpeechTimer.stop();

    // Record provider as the last-used AudioProvider so it can be used in the event of an ExpectSpeech directive.
    m_lastAudioProvider = provider;

    // Record the Recognize event payload for later use by executeContextAvailable().
    m_recognizePayload = payloadGenerator.toString();

    // We can't assemble the MessageRequest until we receive the context.
    m_recognizeRequest.reset();

    return true;
}

void AudioInputProcessor::executeOnContextAvailable(const std::string jsonContext) {
    ACSDK_DEBUG(LX("executeOnContextAvailable").sensitive("jsonContext", jsonContext));

    // Should already be RECOGNIZING if we get here.
    if (m_state != ObserverInterface::State::RECOGNIZING) {
        ACSDK_ERROR(
            LX("executeOnContextAvailableFailed").d("reason", "Not permitted in current state").d("state", m_state));
        return;
    }

    // Should already have a reader.
    if (!m_reader) {
        ACSDK_ERROR(LX("executeOnContextAvailableFailed").d("reason", "nullReader"));
        executeResetState();
        return;
    }
    // Recognize payload should not be empty.
    if (m_recognizePayload.empty()) {
        ACSDK_ERROR(LX("executeOnContextAvailableFailed").d("reason", "payloadEmpty"));
        executeResetState();
        return;
    }

    // Start acquiring the channel right away; we'll service the callback after assembling our Recognize event.
    if (m_focusState != avsCommon::avs::FocusState::FOREGROUND) {
        if (!m_focusManager->acquireChannel(CHANNEL_NAME, shared_from_this(), NAMESPACE)) {
            ACSDK_ERROR(LX("executeOnContextAvailableFailed").d("reason", "Unable to acquire channel"));
            executeResetState();
            return;
        }
    }

    // Assemble the MessageRequest.  It will be sent by executeOnFocusChanged when we acquire the channel.
    auto dialogRequestId = avsCommon::utils::uuidGeneration::generateUUID();
    m_directiveSequencer->setDialogRequestId(dialogRequestId);
    if (!m_espPayload.empty()) {
        auto msgIdAndESPJsonEvent =
            buildJsonEventString("ReportEchoSpatialPerceptionData", dialogRequestId, m_espPayload);
        m_espPayload.clear();
        m_espRequest = std::make_shared<avsCommon::avs::MessageRequest>(msgIdAndESPJsonEvent.second);
    }
    auto msgIdAndJsonEvent = buildJsonEventString("Recognize", dialogRequestId, m_recognizePayload, jsonContext);
    m_recognizeRequest = std::make_shared<avsCommon::avs::MessageRequest>(msgIdAndJsonEvent.second);

    if (m_KWDMetadataReader) {
        m_recognizeRequest->addAttachmentReader(KWD_METADATA_FIELD_NAME, m_KWDMetadataReader);
    }
    m_recognizeRequest->addAttachmentReader(AUDIO_ATTACHMENT_FIELD_NAME, m_reader);

    // Release ownership of the metadata so it can be released once ACL will finish sending the message.
    m_KWDMetadataReader.reset();

    m_recognizeRequest->addObserver(shared_from_this());

    // If we already have focus, there won't be a callback to send the message, so send it now.
    if (avsCommon::avs::FocusState::FOREGROUND == m_focusState) {
        sendRequestNow();
    }
}

void AudioInputProcessor::executeOnContextFailure(const avsCommon::sdkInterfaces::ContextRequestError error) {
    ACSDK_ERROR(LX("executeOnContextFailure").d("error", error));
    executeResetState();
}

void AudioInputProcessor::executeOnFocusChanged(avsCommon::avs::FocusState newFocus) {
    ACSDK_DEBUG(LX("executeOnFocusChanged").d("newFocus", newFocus));

    // Note new focus state.
    m_focusState = newFocus;

    // If we're losing focus, stop using the channel.
    if (newFocus != avsCommon::avs::FocusState::FOREGROUND) {
        ACSDK_DEBUG(LX("executeOnFocusChanged").d("reason", "Lost focus"));
        executeResetState();
        return;
    }

    // We're not losing the channel (m_focusState == avsCommon::avs::FocusState::FOREGROUND).  For all
    // states except RECOGNIZING, there's nothing more to do here.
    if (m_state != ObserverInterface::State::RECOGNIZING) {
        return;
    }

    // For a focus change to FOREGROUND in the Recognizing state, we may have a message queued up to send.  If we do,
    // we can safely send it now.
    sendRequestNow();
}

bool AudioInputProcessor::executeStopCapture(bool stopImmediately, std::shared_ptr<DirectiveInfo> info) {
    if (info && info->isCancelled) {
        ACSDK_DEBUG(LX("stopCaptureIgnored").d("reason", "isCancelled"));
        return true;
    }
    if (m_state != ObserverInterface::State::RECOGNIZING) {
        static const char* errorMessage = "StopCapture only allowed in RECOGNIZING state.";
        auto returnValue = false;
        if (info) {
            if (info->result) {
                if (m_localStopCapturePerformed) {
                    // Since a local StopCapture was performed, we can safely ignore the StopCapture from AVS.
                    m_localStopCapturePerformed = false;
                    returnValue = true;
                    info->result->setCompleted();
                    ACSDK_INFO(LX("executeStopCapture")
                                   .m("StopCapture directive ignored because local StopCapture was performed."));

                } else {
                    info->result->setFailed(errorMessage);
                    ACSDK_ERROR(LX("executeStopCaptureFailed")
                                    .d("reason", "invalidState")
                                    .d("expectedState", "RECOGNIZING")
                                    .d("state", m_state));
                }
            }
            removeDirective(info);
        }
        return returnValue;
    }

    // If info is nullptr, this indicates that a local StopCapture is performed.
    if (nullptr == info) {
        m_localStopCapturePerformed = true;
    }

    // Create a lambda to do the StopCapture.
    std::function<void()> stopCapture = [=] {
        ACSDK_DEBUG(LX("stopCapture").d("stopImmediately", stopImmediately));
        if (m_encoder) {
            // If SpeechEncoder is enabled, let it finish it so the stream will be closed automatically.
            m_encoder->stopEncoding(stopImmediately);
        } else {
            // Otherwise close current reader manually.
            if (stopImmediately) {
                m_reader->close(avsCommon::avs::attachment::AttachmentReader::ClosePoint::IMMEDIATELY);
            } else {
                m_reader->close(
                    avsCommon::avs::attachment::AttachmentReader::ClosePoint::AFTER_DRAINING_CURRENT_BUFFER);
            }
        }

        m_reader.reset();
        setState(ObserverInterface::State::BUSY);

        if (info) {
            if (info->result) {
                info->result->setCompleted();
            }
            removeDirective(info);
        }
    };

    if (m_preparingToSend) {
        // If we're still preparing to send, we'll save the lambda and call it after we start sending.
        m_deferredStopCapture = stopCapture;
    } else {
        // Otherwise, we can call the lambda now.
        stopCapture();
    }

    return true;
}

void AudioInputProcessor::executeResetState() {
    // Irrespective of current state, clean up and go back to idle.
    m_expectingSpeechTimer.stop();
    m_precedingExpectSpeechInitiator.reset();
    if (m_reader) {
        m_reader->close();
    }
    if (m_encoder) {
        m_encoder->stopEncoding(true);
    }
    m_reader.reset();
    m_KWDMetadataReader.reset();
    m_recognizeRequest.reset();
    m_espRequest.reset();
    m_preparingToSend = false;
    m_deferredStopCapture = nullptr;
    if (m_focusState != avsCommon::avs::FocusState::NONE) {
        m_focusManager->releaseChannel(CHANNEL_NAME, shared_from_this());
    }
    m_focusState = avsCommon::avs::FocusState::NONE;
    setState(ObserverInterface::State::IDLE);
}

bool AudioInputProcessor::executeExpectSpeech(std::chrono::milliseconds timeout, std::shared_ptr<DirectiveInfo> info) {
    if (info && info->isCancelled) {
        ACSDK_DEBUG(LX("expectSpeechIgnored").d("reason", "isCancelled"));
        return true;
    }
    if (m_state != ObserverInterface::State::IDLE && m_state != ObserverInterface::State::BUSY) {
        static const char* errorMessage = "ExpectSpeech only allowed in IDLE or BUSY state.";
        if (info->result) {
            info->result->setFailed(errorMessage);
        }
        removeDirective(info);
        ACSDK_ERROR(LX("executeExpectSpeechFailed")
                        .d("reason", "invalidState")
                        .d("expectedState", "IDLE/BUSY")
                        .d("state", m_state));
        return false;
    }

    if (m_precedingExpectSpeechInitiator) {
        ACSDK_ERROR(LX(__func__)
                        .d("reason", "precedingExpectSpeechInitiatorUnconsumed")
                        .d(INITIATOR_KEY.c_str(), *m_precedingExpectSpeechInitiator));
    }

    m_precedingExpectSpeechInitiator = memory::make_unique<std::string>("");
    bool found = json::jsonUtils::retrieveValue(
        info->directive->getPayload(), INITIATOR_KEY, m_precedingExpectSpeechInitiator.get());
    if (found) {
        ACSDK_DEBUG(LX(__func__).d("initiatorFound", *m_precedingExpectSpeechInitiator));
    } else {
        *m_precedingExpectSpeechInitiator = "";
    }

    // Start the ExpectSpeech timer.
    if (!m_expectingSpeechTimer.start(timeout, std::bind(&AudioInputProcessor::expectSpeechTimedOut, this)).valid()) {
        ACSDK_ERROR(LX("executeExpectSpeechFailed").d("reason", "startTimerFailed"));
    }
    setState(ObserverInterface::State::EXPECTING_SPEECH);
    if (info->result) {
        info->result->setCompleted();
    }
    removeDirective(info);

    // If possible, start recognizing immediately.
    if (m_lastAudioProvider && m_lastAudioProvider.alwaysReadable) {
        return executeRecognize(m_lastAudioProvider, "");
    } else if (m_defaultAudioProvider && m_defaultAudioProvider.alwaysReadable) {
        return executeRecognize(m_defaultAudioProvider, "");
    }

    return true;
}

bool AudioInputProcessor::executeExpectSpeechTimedOut() {
    if (m_state != ObserverInterface::State::EXPECTING_SPEECH) {
        ACSDK_ERROR(LX("executeExpectSpeechTimedOutFailure")
                        .d("reason", "invalidState")
                        .d("expectedState", "EXPECTING_SPEECH")
                        .d("state", m_state));
        return false;
    }
    m_precedingExpectSpeechInitiator.reset();
    auto msgIdAndJsonEvent = buildJsonEventString("ExpectSpeechTimedOut");
    auto request = std::make_shared<avsCommon::avs::MessageRequest>(msgIdAndJsonEvent.second);
    request->addObserver(shared_from_this());
    m_messageSender->sendMessage(request);
    setState(ObserverInterface::State::IDLE);
    ACSDK_ERROR(LX("executeExpectSpeechFailed").d("reason", "Timed Out"));
    return true;
}

void AudioInputProcessor::executeOnDialogUXStateChanged(
    avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState newState) {
    if (!m_initialDialogUXStateReceived) {
        // The initial dialog UX state change call comes from simply registering as an observer; it is not a deliberate
        // change to the dialog state which should interrupt a recognize event.
        m_initialDialogUXStateReceived = true;
        return;
    }
    if (newState != avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::IDLE) {
        return;
    }
    if (m_focusState != avsCommon::avs::FocusState::NONE) {
        m_focusManager->releaseChannel(CHANNEL_NAME, shared_from_this());
        m_focusState = avsCommon::avs::FocusState::NONE;
    }
    setState(ObserverInterface::State::IDLE);
}

void AudioInputProcessor::setState(ObserverInterface::State state) {
    if (m_state == state) {
        return;
    }

    // Reset the user inactivity if transitioning to or from `RECOGNIZING` state.
    if (ObserverInterface::State::RECOGNIZING == m_state || ObserverInterface::State::RECOGNIZING == state) {
        m_userInactivityMonitor->onUserActive();
    }

    ACSDK_DEBUG(LX("setState").d("from", m_state).d("to", state));
    m_state = state;
    for (auto observer : m_observers) {
        observer->onStateChanged(m_state);
    }
}

void AudioInputProcessor::removeDirective(std::shared_ptr<DirectiveInfo> info) {
    // Check result too, to catch cases where DirectiveInfo was created locally, without a nullptr result.
    // In those cases there is no messageId to remove because no result was expected.
    if (info->directive && info->result) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
    }
}

void AudioInputProcessor::sendRequestNow() {
    if (m_espRequest) {
        m_messageSender->sendMessage(m_espRequest);
        m_espRequest.reset();
    }

    if (m_recognizeRequest) {
        ACSDK_METRIC_IDS(TAG, "Recognize", "", "", Metrics::Location::AIP_SEND);
        m_messageSender->sendMessage(m_recognizeRequest);
        m_recognizeRequest.reset();
        m_preparingToSend = false;
        if (m_deferredStopCapture) {
            m_deferredStopCapture();
            m_deferredStopCapture = nullptr;
        }
    }
}

void AudioInputProcessor::onExceptionReceived(const std::string& exceptionMessage) {
    ACSDK_ERROR(LX("onExceptionReceived").d("exception", exceptionMessage));
    resetState();
}

void AudioInputProcessor::onSendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status status) {
    ACSDK_DEBUG(LX("onSendCompleted").d("status", status));

    if (status == avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS ||
        status == avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT ||
        status == avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::PENDING) {
        // Stop listening from audio input source when the recognize event steam is closed.
        ACSDK_DEBUG5(LX("stopCapture").d("reason", "streamClosed"));
        stopCapture();

        return;
    }
    ACSDK_DEBUG(LX("resetState").d("dueToStatus", status));
    resetState();
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> AudioInputProcessor::
    getCapabilityConfigurations() {
    return m_capabilityConfigurations;
}

}  // namespace aip
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
