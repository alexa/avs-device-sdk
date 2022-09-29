/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <algorithm>
#include <cctype>
#include <functional>
#include <list>
#include <sstream>

#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/FocusState.h>
#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/Metrics.h>
#include <AVSCommon/Utils/Metrics/DataPointDurationBuilder.h>
#include <AVSCommon/Utils/Metrics/DataPointCounterBuilder.h>
#include <AVSCommon/Utils/Metrics/DataPointStringBuilder.h>
#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>
#include <AVSCommon/Utils/String/StringUtils.h>
#include <AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h>
#include <AVSCommon/AVS/Attachment/AttachmentUtils.h>
#include <Settings/SettingEventMetadata.h>
#include <Settings/SharedAVSSettingProtocol.h>
#include <AVSCommon/AVS/Attachment/DefaultAttachmentReader.h>

#include "AIP/AudioInputProcessor.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace aip {
using namespace avsCommon::avs;
using namespace avsCommon::utils;
using namespace avsCommon::utils::logger;
using namespace avsCommon::utils::metrics;
using namespace avsCommon::utils::json;
using namespace avsCommon::sdkInterfaces;
using namespace std::chrono;

/// SpeechRecognizer capability constants
/// SpeechRecognizer interface type
static const std::string SPEECHRECOGNIZER_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";
/// SpeechRecognizer interface name
static const std::string SPEECHRECOGNIZER_CAPABILITY_INTERFACE_NAME = "SpeechRecognizer";

/// SpeechRecognizer interface version
#ifdef OVERRIDE_SPEECHRECOGNIZER_VERSION
static const std::string SPEECHRECOGNIZER_CAPABILITY_INTERFACE_VERSION = OVERRIDE_SPEECHRECOGNIZER_VERSION;
#else
static const std::string SPEECHRECOGNIZER_CAPABILITY_INTERFACE_VERSION = "2.3";
#endif

/// Configuration key used to give more details about the device configuration.
static const std::string CAPABILITY_INTERFACE_CONFIGURATIONS_KEY = "configurations";

/// Supported wake words key.
static const std::string CAPABILITY_INTERFACE_WAKE_WORDS_KEY = "wakeWords";

/// Supported maximum number of concurrent wakewords key.
static const std::string CAPABILITY_MAXIMUM_CONCURRENT_WAKEWORDS_KEY = "maximumConcurrentWakeWords";

/// The scope key for each wake words set.
static const std::string CAPABILITY_INTERFACE_SCOPES_KEY = "scopes";

/// The wake word values for a given scope.
static const std::string CAPABILITY_INTERFACE_VALUES_KEY = "values";

/// The scope configuration used as default locale wake words support.
static const std::string CAPABILITY_INTERFACE_DEFAULT_LOCALE = "DEFAULT";

/// String to identify log entries originating from this file.
#define TAG "AudioInputProcessor"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The name of the @c FocusManager channel used by @c AudioInputProvider.
static const std::string CHANNEL_NAME = FocusManagerInterface::DIALOG_CHANNEL_NAME;

/// The namespace for this capability agent.
static const std::string NAMESPACE = "SpeechRecognizer";

/// The StopCapture directive signature.
static const avsCommon::avs::NamespaceAndName STOP_CAPTURE{NAMESPACE, "StopCapture"};

/// The ExpectSpeech directive signature.
static const avsCommon::avs::NamespaceAndName EXPECT_SPEECH{NAMESPACE, "ExpectSpeech"};

/// The SetEndOfSpeechOffset directive signature.
static const avsCommon::avs::NamespaceAndName SET_END_OF_SPEECH_OFFSET{NAMESPACE, "SetEndOfSpeechOffset"};

/// The SetWakeWordConfirmation directive signature.
static const avsCommon::avs::NamespaceAndName SET_WAKE_WORD_CONFIRMATION{NAMESPACE, "SetWakeWordConfirmation"};

/// The SetSpeechConfirmation directive signature.
static const avsCommon::avs::NamespaceAndName SET_SPEECH_CONFIRMATION{NAMESPACE, "SetSpeechConfirmation"};

/// The SetWakeWords directive signature.
static const avsCommon::avs::NamespaceAndName SET_WAKE_WORDS{NAMESPACE, "SetWakeWords"};

/// The field identifying the initiator.
static const std::string INITIATOR_KEY = "initiator";

/// The field identifying the initiator's profile.
static const std::string PROFILE_KEY = "profile";

/// The field identifying the initiator's format.
static const std::string FORMAT_KEY = "format";

/// The field identifying the initiator's type.
static const std::string TYPE_KEY = "type";

/// The field identifying the initiator's token.
static const std::string TOKEN_KEY = "token";

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

/// The field name for the user voice attachment.
static const std::string AUDIO_ATTACHMENT_FIELD_NAME = "audio";

/// The field name for the wake word engine metadata.
static const std::string KWD_METADATA_FIELD_NAME = "wakewordEngineMetadata";

/// The field name for the start of speech timestamp, reported in milliseconds since epoch. This field is provided to
/// the Recognize event and it's sent back as part of SetEndOfSpeechOffset payload.
static const std::string START_OF_SPEECH_TIMESTAMP_FIELD_NAME = "startOfSpeechTimestamp";

/// The field name for the end of speech offset, reported in milliseconds, as part of SetEndOfSpeechOffset payload.
static const std::string END_OF_SPEECH_OFFSET_FIELD_NAME = "endOfSpeechOffsetInMilliseconds";

/// The value of the WakeWordConfirmationChanged Event name.
static const std::string WAKE_WORD_CONFIRMATION_CHANGED_EVENT_NAME = "WakeWordConfirmationChanged";
/// The value of the WakeWordConfirmationReport Event name.
static const std::string WAKE_WORD_CONFIRMATION_REPORT_EVENT_NAME = "WakeWordConfirmationReport";
/// The value of the payload key for wakeWordConfirmation.
static const std::string WAKE_WORD_CONFIRMATION_PAYLOAD_KEY = "wakeWordConfirmation";

/// The value of the SpeechConfirmationChanged Event name.
static const std::string SPEECH_CONFIRMATION_CHANGED_EVENT_NAME = "SpeechConfirmationChanged";
/// The value of the SpeechConfirmationReport Event name.
static const std::string SPEECH_CONFIRMATION_REPORT_EVENT_NAME = "SpeechConfirmationReport";
/// The value of the payload key for speechConfirmation.
static const std::string SPEECH_CONFIRMATION_PAYLOAD_KEY = "speechConfirmation";

/// The value of the WakeWordsChanged Event name.
static const std::string WAKE_WORDS_CHANGED_EVENT_NAME = "WakeWordsChanged";
/// The value of the WakeWordsReport Event name.
static const std::string WAKE_WORDS_REPORT_EVENT_NAME = "WakeWordsReport";
/// The value of the payload key for wake words.
static const std::string WAKE_WORDS_PAYLOAD_KEY = "wakeWords";

/// The component name of power management
static const std::string POWER_RESOURCE_COMPONENT_NAME = "AudioInputProcessor";

/// Metric Activity Name Prefix for AIP metric source
static const std::string METRIC_ACTIVITY_NAME_PREFIX_AIP = "AIP-";

/// Start of Utterance Activity Name for AIP metric source
static const std::string START_OF_UTTERANCE = "START_OF_UTTERANCE";
static const std::string START_OF_UTTERANCE_ACTIVITY_NAME = METRIC_ACTIVITY_NAME_PREFIX_AIP + START_OF_UTTERANCE;

/// Name of audio bytes stream metric Opus
static const std::string WAKEWORD_DETECTION_SEGMENT_UPLOADED_OPUS = "AIP_WAKEWORD_DETECTION_SEGMENT_UPLOADED_OPUS";

/// Name of audio bytes stream metric PCM
static const std::string WAKEWORD_DETECTION_SEGMENT_UPLOADED_PCM = "AIP_WAKEWORD_DETECTION_SEGMENT_UPLOADED_PCM";

/// Recognize EVENT is built for AIP metric source
static const std::string RECOGNIZE_START_SEND_MESSAGE = "RECOGNIZE_EVENT_IS_BUILT";
static const std::string RECOGNIZE_START_SEND_MESSAGE_ACTIVITY_NAME =
    METRIC_ACTIVITY_NAME_PREFIX_AIP + RECOGNIZE_START_SEND_MESSAGE;

/// Wakeword Activity Name for AIP metric source
static const std::string START_OF_STREAM_TIMESTAMP = "START_OF_STREAM_TIMESTAMP";
static const std::string WW_DURATION = "WW_DURATION";
static const std::string WW_DURATION_ACTIVITY_NAME = METRIC_ACTIVITY_NAME_PREFIX_AIP + WW_DURATION;

/// Stop Capture Received Activity Name for AIP metric source
static const std::string STOP_CAPTURE_RECEIVED = "STOP_CAPTURE";
static const std::string STOP_CAPTURE_RECEIVED_ACTIVITY_NAME = METRIC_ACTIVITY_NAME_PREFIX_AIP + STOP_CAPTURE_RECEIVED;

/// The duration metric for acquire power resource
static const std::string ACQUIRE_POWER_RESOURCE = "ACQUIRE_POWER_RESOURCE";
static const std::string ACQUIRE_POWER_RESOURCE_ACTIVITY = METRIC_ACTIVITY_NAME_PREFIX_AIP + ACQUIRE_POWER_RESOURCE;

/// The duration metric for release power resource
static const std::string RELEASE_POWER_RESOURCE = "RELEASE_POWER_RESOURCE";
static const std::string RELEASE_POWER_RESOURCE_ACTIVITY = METRIC_ACTIVITY_NAME_PREFIX_AIP + RELEASE_POWER_RESOURCE;

/// The timestamp for the start of execute on context available
static const std::string FETCH_CONTEXT_DURATION = "FETCH_CONTEXT_DURATION";
static const std::string FETCH_CONTEXT_DURATION_ACTIVITY_NAME =
    METRIC_ACTIVITY_NAME_PREFIX_AIP + FETCH_CONTEXT_DURATION;

/// End of Speech Offset Received Activity Name for AIP metric source
static const std::string END_OF_SPEECH_OFFSET_RECEIVED = "END_OF_SPEECH_OFFSET";
static const std::string END_OF_SPEECH_OFFSET_RECEIVED_ACTIVITY_NAME =
    METRIC_ACTIVITY_NAME_PREFIX_AIP + END_OF_SPEECH_OFFSET_RECEIVED;

/// The duration metric for short time out
static const std::string STOP_CAPTURE_TO_END_OF_SPEECH_METRIC_NAME = "STOP_CAPTURE_TO_END_OF_SPEECH";
static const std::string STOP_CAPTURE_TO_END_OF_SPEECH_ACTIVITY_NAME =
    METRIC_ACTIVITY_NAME_PREFIX_AIP + STOP_CAPTURE_TO_END_OF_SPEECH_METRIC_NAME;

/// The recognize request initiator metric.
static const std::string INITIATOR_PREFIX = "INITIATOR_";
static const std::string INITIATOR_ACTIVITY_NAME_PREFIX = METRIC_ACTIVITY_NAME_PREFIX_AIP + INITIATOR_PREFIX;

/// AudioEncodingFormat Activity Name for AIP metric source
static const std::string AUDIO_ENCODING_FORMAT_METRIC_NAME = "AUDIO_ENCODING_FORMAT";
static const std::string AUDIO_ENCODING_FORMAT_ACTIVITY_NAME =
    METRIC_ACTIVITY_NAME_PREFIX_AIP + AUDIO_ENCODING_FORMAT_METRIC_NAME;
static const std::string AUDIO_ENCODING_FORMAT_OPUS = "OPUSAudioEncoding";
static const std::string AUDIO_ENCODING_FORMAT_LPCM = "LPCMAudioEncoding";

/// The default resolveKey used as a placeholder when only one encoding format is configured for @c AudioInputProcessor
static const std::string DEFAULT_RESOLVE_KEY = "DEFAULT_RESOLVE_KEY";

/// Keys for instance entry metric specific fields
static const std::string ENTRY_METRIC_ACTOR_NAME = "AudioInputProcessor";
static const std::string ENTRY_METRIC_ACTIVITY_NAME = METRIC_ACTIVITY_NAME_PREFIX_AIP + ENTRY_METRIC_ACTOR_NAME;
static const std::string ENTRY_METRIC_KEY_SEGMENT_ID = "segment_id";
static const std::string ENTRY_METRIC_KEY_ACTOR = "actor";
static const std::string ENTRY_METRIC_KEY_ENTRY_TYPE = "entry_type";
static const std::string ENTRY_METRIC_KEY_ENTRY_NAME = "entry_name";
static const std::string ENTRY_METRIC_NAME_STATE_CHANGE = "StateChange";

/// Preroll duration is a fixed 500ms.
static const std::chrono::milliseconds PREROLL_DURATION = std::chrono::milliseconds(500);

static const int MILLISECONDS_PER_SECOND = 1000;

/// Threshold number of bytes for OPUS Encoded Wakeword detection
static const int WAKEWORD_DETECTION_SEGMENT_SIZE_BYTES_OPUS = 5209;
/// Threshold number of bytes for PCM Encoded Wakeword detection
static const int WAKEWORD_DETECTION_SEGMENT_SIZE_BYTES_PCM = 40480;

/// Cloud resolve key constant. Used to determine the encoding sent to the cloud.
static const std::string CLOUD_RESOLVE_KEY = "CLOUD";

/**
 * Helper function to get string values of encoding audio format, which are used in Recognize event.
 * @param encoding Target encoding format
 * @return String name of the encoding format
 */
static std::string encodingFormatToString(avsCommon::utils::AudioFormat::Encoding encoding) {
    switch (encoding) {
        case avsCommon::utils::AudioFormat::Encoding::OPUS:
            return "OPUS";
        case avsCommon::utils::AudioFormat::Encoding::LPCM:
            return "AUDIO_L16_RATE_16000_CHANNELS_1";
    }
    return "UNKNOWN";
}

/**
 * Handles a Metric event by creating and recording it. Failure to create or record the event results
 * in an early return.
 *
 * @param metricRecorder The @c MetricRecorderInterface which records Metric events.
 * @param metricEventBuilder The @c MetricEventBuilder.
 * @param dialogRequestId The dialogRequestId associated with this Metric event; default is empty string.
 */
static void submitMetric(
    const std::shared_ptr<MetricRecorderInterface>& metricRecorder,
    MetricEventBuilder& metricEventBuilder,
    const std::string& dialogRequestId = "") {
    metricEventBuilder.addDataPoint(
        DataPointStringBuilder{}.setName("DIALOG_REQUEST_ID").setValue(dialogRequestId).build());

    auto metricEvent = metricEventBuilder.build();

    if (metricEvent == nullptr) {
        ACSDK_ERROR(LX("Error creating metric with explicit dialogRequestId"));
        return;
    }
    recordMetric(metricRecorder, metricEvent);
}

/**
 * Handles a Metric event by creating and recording it. Failure to create or record the event results
 * in an early return.
 *
 * @param metricRecorder The @c MetricRecorderInterface which records Metric events.
 * @param activityName The activityName of the Metric event.
 * @param dataPoint The @c DataPoint of this Metric event (e.g. duration).
 * @param directive The @c AVSDirective associated with this Metric event; default is nullptr.
 */
static void submitMetric(
    const std::shared_ptr<MetricRecorderInterface> metricRecorder,
    const std::string& activityName,
    const DataPoint& dataPoint,
    const std::shared_ptr<AVSDirective>& directive = nullptr) {
    auto metricEventBuilder = MetricEventBuilder{}.setActivityName(activityName).addDataPoint(dataPoint);

    if (directive != nullptr) {
        metricEventBuilder.addDataPoint(
            DataPointStringBuilder{}.setName("HTTP2_STREAM").setValue(directive->getAttachmentContextId()).build());
        metricEventBuilder.addDataPoint(
            DataPointStringBuilder{}.setName("DIRECTIVE_MESSAGE_ID").setValue(directive->getMessageId()).build());
        metricEventBuilder.addDataPoint(
            DataPointStringBuilder{}.setName("DIALOG_REQUEST_ID").setValue(directive->getDialogRequestId()).build());
    }

    auto metricEvent = metricEventBuilder.build();

    if (metricEvent == nullptr) {
        ACSDK_ERROR(LX("Error creating metric from directive"));
        return;
    }
    recordMetric(metricRecorder, metricEvent);
}

/**
 * Creates and records an instance entry metric with the given identifiers and metadata.
 * @param metricRecorder The @c MetricRecorderInterface which records Metric events.
 * @param segmentId The segmentId corresponding to this metric event.
 * @param name The name of this metric
 * @param metadata Any metadata to be associated with this metric; default is empty
 */
static void submitInstanceEntryMetric(
    const std::shared_ptr<MetricRecorderInterface>& metricRecorder,
    const std::string& segmentId,
    const std::string& name,
    const std::map<std::string, std::string>& metadata = {}) {
    if (segmentId.empty() || name.empty()) {
        ACSDK_ERROR(LX(__FUNCTION__).m("Unable to create instance metric").d("segmentId", segmentId).d("name", name));
        return;
    }

    auto metricBuilder = MetricEventBuilder{}.setActivityName(ENTRY_METRIC_ACTIVITY_NAME);
    metricBuilder.addDataPoint(
        DataPointStringBuilder{}.setName(ENTRY_METRIC_KEY_SEGMENT_ID).setValue(segmentId).build());
    metricBuilder.addDataPoint(
        DataPointStringBuilder{}.setName(ENTRY_METRIC_KEY_ACTOR).setValue(ENTRY_METRIC_ACTOR_NAME).build());
    metricBuilder.addDataPoint(DataPointStringBuilder{}.setName(ENTRY_METRIC_KEY_ENTRY_NAME).setValue(name).build());
    metricBuilder.addDataPoint(
        DataPointStringBuilder{}.setName(ENTRY_METRIC_KEY_ENTRY_TYPE).setValue("INSTANCE").build());
    for (auto const& pair : metadata) {
        metricBuilder.addDataPoint(DataPointStringBuilder{}.setName(pair.first).setValue(pair.second).build());
    }
    auto metric = metricBuilder.build();
    if (metric == nullptr) {
        ACSDK_ERROR(LX(__FUNCTION__).m("Error creating instance entry metric."));
        return;
    }
    recordMetric(metricRecorder, metric);
}

/**
 * Creates the SpeechRecognizer capability configuration.
 *
 * @param assetsManager Responsible for retrieving and changing the wake words and locale.
 * @return The SpeechRecognizer capability configuration.
 */
static std::shared_ptr<avsCommon::avs::CapabilityConfiguration> getSpeechRecognizerCapabilityConfiguration(
    const LocaleAssetsManagerInterface& assetsManager);

/**
 * Resolving method to be called in lambda function that will be passed to @c MessageRequest as @c
 * MessageRequestResolveFunction
 * @param[in,out] request @c EditableMessageRequest to resolve
 * @param resolveKey Provided resolve key
 * @param encodingFormats Mappings from resolveKey to encoding format
 * @param attachmentReaders Mappings from resolveKey to attachment readers
 * @param metricRecorder @c MetricRecorderInterface object to emit metrics
 * @return @c true if the message is resolved successfully, else @c false
 */
static bool resolveMessageRequest(
    const std::shared_ptr<avsCommon::avs::EditableMessageRequest>& request,
    const std::string& resolveKey,
    AudioInputProcessor::EncodingFormatResponse encodingFormats,
    const std::unordered_map<std::string, std::vector<std::shared_ptr<avsCommon::avs::MessageRequest::NamedReader>>>&
        attachmentReaders,
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder);

std::shared_ptr<AudioInputProcessor> AudioInputProcessor::create(
    std::shared_ptr<DirectiveSequencerInterface> directiveSequencer,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<FocusManagerInterface> focusManager,
    std::shared_ptr<avsCommon::avs::DialogUXStateAggregator> dialogUXStateAggregator,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<UserInactivityMonitorInterface> userInactivityMonitor,
    std::shared_ptr<SystemSoundPlayerInterface> systemSoundPlayer,
    const std::shared_ptr<LocaleAssetsManagerInterface>& assetsManager,
    std::shared_ptr<settings::WakeWordConfirmationSetting> wakeWordConfirmation,
    std::shared_ptr<settings::SpeechConfirmationSetting> speechConfirmation,
    const std::shared_ptr<avsCommon::avs::CapabilityChangeNotifierInterface>& capabilityChangeNotifier,
    std::shared_ptr<settings::WakeWordsSetting> wakeWordsSetting,
    std::shared_ptr<audioEncoderInterfaces::AudioEncoderInterface> audioEncoder,
    AudioProvider defaultAudioProvider,
    std::shared_ptr<PowerResourceManagerInterface> powerResourceManager,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
    const std::shared_ptr<ExpectSpeechTimeoutHandler>& expectSpeechTimeoutHandler) {
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
        ACSDK_ERROR(LX("createFailed").d("reason", "nullUserInactivityMonitor"));
        return nullptr;
    } else if (!systemSoundPlayer) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullSystemSoundPlayer"));
        return nullptr;
    } else if (!wakeWordConfirmation) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullWakeWordsConfirmation"));
        return nullptr;
    } else if (!speechConfirmation) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullSpeechConfirmation"));
        return nullptr;
    } else if (!assetsManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullAssetsManager"));
        return nullptr;
    } else if (!assetsManager->getDefaultSupportedWakeWords().empty() && !wakeWordsSetting) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullWakeWordsSetting"));
        return nullptr;
    } else if (!capabilityChangeNotifier) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullCapabilityChangeNotifier"));
        return nullptr;
    }

    auto capabilitiesConfiguration = getSpeechRecognizerCapabilityConfiguration(*assetsManager);
    if (!capabilitiesConfiguration) {
        ACSDK_ERROR(LX("createFailed").d("reason", "unableToCreateCapabilitiesConfiguration"));
        return nullptr;
    }
    auto aip = std::shared_ptr<AudioInputProcessor>(new AudioInputProcessor(
        directiveSequencer,
        messageSender,
        contextManager,
        focusManager,
        exceptionEncounteredSender,
        userInactivityMonitor,
        systemSoundPlayer,
        assetsManager,
        audioEncoder,
        defaultAudioProvider,
        wakeWordConfirmation,
        speechConfirmation,
        capabilityChangeNotifier,
        wakeWordsSetting,
        capabilitiesConfiguration,
        powerResourceManager,
        std::move(metricRecorder),
        expectSpeechTimeoutHandler));

    if (!aip->initialize()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "unableToInitialize"));
        return nullptr;
    }

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
    configuration[SET_WAKE_WORD_CONFIRMATION] = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    configuration[SET_SPEECH_CONFIRMATION] = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    configuration[SET_WAKE_WORDS] = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    return configuration;
}

void AudioInputProcessor::addObserver(std::shared_ptr<ObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("addObserverFailed").d("reason", "nullObserver"));
        return;
    }
    m_executor.execute([this, observer]() { m_observers.insert(observer); });
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
    steady_clock::time_point startOfSpeechTimestamp,
    avsCommon::avs::AudioInputStream::Index begin,
    avsCommon::avs::AudioInputStream::Index keywordEnd,
    std::string keyword,
    std::shared_ptr<const std::vector<char>> KWDMetadata,
    const std::string& initiatorToken) {
    ACSDK_METRIC_IDS(TAG, "Recognize", "", "", Metrics::Location::AIP_RECEIVE);
    ACSDK_DEBUG5(LX(__func__));

    std::string upperCaseKeyword = keyword;
    std::transform(upperCaseKeyword.begin(), upperCaseKeyword.end(), upperCaseKeyword.begin(), ::toupper);
    if (KEYWORD_TEXT_STOP == upperCaseKeyword) {
        ACSDK_DEBUG(LX("skippingRecognizeEvent").d("reason", "invalidKeyword").d("keyword", keyword));
        std::promise<bool> ret;
        ret.set_value(false);
        return ret.get_future();
    }

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

    return m_executor.submit([this,
                              audioProvider,
                              initiator,
                              startOfSpeechTimestamp,
                              begin,
                              keywordEnd,
                              keyword,
                              KWDMetadata,
                              initiatorToken]() {
        return executeRecognize(
            audioProvider, initiator, startOfSpeechTimestamp, begin, keywordEnd, keyword, KWDMetadata, initiatorToken);
    });
}

std::future<bool> AudioInputProcessor::stopCapture() {
    return m_executor.submit([this]() { return executeStopCapture(); });
}

std::future<void> AudioInputProcessor::resetState() {
    ACSDK_DEBUG0(LX(__func__));
    return m_executor.submit([this]() { executeResetState(); });
}

void AudioInputProcessor::onContextAvailable(const std::string& jsonContext) {
    ACSDK_DEBUG0(LX(__func__));
    m_executor.execute([this, jsonContext]() { executeOnContextAvailable(jsonContext); });
}

void AudioInputProcessor::onContextFailure(const ContextRequestError error) {
    ACSDK_DEBUG0(LX(__func__));
    m_executor.execute([this, error]() { executeOnContextFailure(error); });
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

    if (!info->directive) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirective"));
        return;
    }

    if (info->directive->getName() == STOP_CAPTURE.name) {
        ACSDK_METRIC_MSG(TAG, info->directive, Metrics::Location::AIP_RECEIVE);
        submitMetric(
            m_metricRecorder,
            STOP_CAPTURE_RECEIVED_ACTIVITY_NAME,
            DataPointCounterBuilder{}.setName(STOP_CAPTURE_RECEIVED).increment(1).build(),
            info->directive);
        handleStopCaptureDirective(info);
    } else if (info->directive->getName() == EXPECT_SPEECH.name) {
        handleExpectSpeechDirective(info);
    } else if (info->directive->getName() == SET_END_OF_SPEECH_OFFSET.name) {
        handleSetEndOfSpeechOffsetDirective(info);
    } else if (info->directive->getName() == SET_WAKE_WORD_CONFIRMATION.name) {
        handleSetWakeWordConfirmation(info);
    } else if (info->directive->getName() == SET_SPEECH_CONFIRMATION.name) {
        handleSetSpeechConfirmation(info);
    } else if (info->directive->getName() == SET_WAKE_WORDS.name) {
        handleSetWakeWords(info);
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

void AudioInputProcessor::onFocusChanged(avsCommon::avs::FocusState newFocus, avsCommon::avs::MixingBehavior behavior) {
    ACSDK_DEBUG9(LX("onFocusChanged").d("newFocus", newFocus).d("MixingBehavior", behavior));
    m_executor.execute([this, newFocus]() { executeOnFocusChanged(newFocus); });
}

void AudioInputProcessor::onDialogUXStateChanged(DialogUXStateObserverInterface::DialogUXState newState) {
    m_executor.execute([this, newState]() { executeOnDialogUXStateChanged(newState); });
}

AudioInputProcessor::AudioInputProcessor(
    std::shared_ptr<DirectiveSequencerInterface> directiveSequencer,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<FocusManagerInterface> focusManager,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<UserInactivityMonitorInterface> userInactivityMonitor,
    std::shared_ptr<SystemSoundPlayerInterface> systemSoundPlayer,
    const std::shared_ptr<LocaleAssetsManagerInterface>& assetsManager,
    std::shared_ptr<audioEncoderInterfaces::AudioEncoderInterface> audioEncoder,
    AudioProvider defaultAudioProvider,
    std::shared_ptr<settings::WakeWordConfirmationSetting> wakeWordConfirmation,
    std::shared_ptr<settings::SpeechConfirmationSetting> speechConfirmation,
    const std::shared_ptr<avsCommon::avs::CapabilityChangeNotifierInterface>& capabilityChangeNotifier,
    std::shared_ptr<settings::WakeWordsSetting> wakeWordsSetting,
    std::shared_ptr<avsCommon::avs::CapabilityConfiguration> capabilitiesConfiguration,
    std::shared_ptr<PowerResourceManagerInterface> powerResourceManager,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
    const std::shared_ptr<ExpectSpeechTimeoutHandler>& expectSpeechTimeoutHandler) :
        CapabilityAgent{NAMESPACE, exceptionEncounteredSender},
        RequiresShutdown{"AudioInputProcessor"},
        m_metricRecorder{metricRecorder},
        m_directiveSequencer{directiveSequencer},
        m_messageSender{messageSender},
        m_contextManager{contextManager},
        m_focusManager{focusManager},
        m_userInactivityMonitor{userInactivityMonitor},
        m_encoder{audioEncoder},
        m_defaultAudioProvider{defaultAudioProvider},
        m_lastAudioProvider{AudioProvider::null()},
        m_state{ObserverInterface::State::IDLE},
        m_focusState{avsCommon::avs::FocusState::NONE},
        m_preparingToSend{false},
        m_initialDialogUXStateReceived{false},
        m_localStopCapturePerformed{false},
        m_streamIsClosedInRecognizingState{false},
        m_systemSoundPlayer{systemSoundPlayer},
        m_assetsManager{assetsManager},
        m_precedingExpectSpeechInitiator{nullptr},
        m_wakeWordConfirmation{wakeWordConfirmation},
        m_speechConfirmation{speechConfirmation},
        m_capabilityChangeNotifier{capabilityChangeNotifier},
        m_wakeWordsSetting{wakeWordsSetting},
        m_powerResourceManager{powerResourceManager},
        m_expectSpeechTimeoutHandler{expectSpeechTimeoutHandler},
        m_timeSinceLastResumeMS{std::chrono::milliseconds(0)},
        m_timeSinceLastPartialMS{std::chrono::milliseconds(0)},
        m_resourceFlags{0},
        m_usingEncoder{false},
        m_messageRequestResolver{nullptr},
        m_encodingAudioFormats{{DEFAULT_RESOLVE_KEY, AudioFormat::Encoding::LPCM}} {
    m_capabilityConfigurations.insert(capabilitiesConfiguration);
    m_fetchContextTimeMetricData.setName(FETCH_CONTEXT_DURATION);

    if (m_powerResourceManager) {
        m_powerResourceId = m_powerResourceManager->create(
            POWER_RESOURCE_COMPONENT_NAME, false, PowerResourceManagerInterface::PowerResourceLevel::ACTIVE_HIGH);
        if (!m_powerResourceId) {
            ACSDK_ERROR(
                LX(__func__).d("reason", "createRawPowerResourceFailed").d("name", POWER_RESOURCE_COMPONENT_NAME));
        }
    }
}

/**
 * Generate supported wake words json capability configuration for a given scope (default, language or locale).
 *
 * @param scope The scope being reported.
 * @param wakeWordsCombination The set of a combination of wake words supported in the given scope.
 * @return A json representation of the scope configuration.
 */
std::string generateSupportedWakeWordsJson(
    const std::string& scope,
    const LocaleAssetsManagerInterface::WakeWordsSets& wakeWordsCombination) {
    json::JsonGenerator generator;
    generator.addStringArray(CAPABILITY_INTERFACE_SCOPES_KEY, std::list<std::string>({scope}));
    generator.addCollectionOfStringArray(CAPABILITY_INTERFACE_VALUES_KEY, wakeWordsCombination);
    return generator.toString();
}

std::shared_ptr<avsCommon::avs::CapabilityConfiguration> getSpeechRecognizerCapabilityConfiguration(
    const LocaleAssetsManagerInterface& assetsManager) {
    std::unordered_map<std::string, std::string> configMap;
    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, SPEECHRECOGNIZER_CAPABILITY_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, SPEECHRECOGNIZER_CAPABILITY_INTERFACE_NAME});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, SPEECHRECOGNIZER_CAPABILITY_INTERFACE_VERSION});

    // Generate wake words capability configuration if supportedWakeWords is not empty.
    auto defaultWakeWords = assetsManager.getDefaultSupportedWakeWords();
    if (!defaultWakeWords.empty()) {
        std::set<std::string> wakeWords;
        wakeWords.insert(generateSupportedWakeWordsJson(CAPABILITY_INTERFACE_DEFAULT_LOCALE, defaultWakeWords));
        for (const auto& entry : assetsManager.getLanguageSpecificWakeWords()) {
            wakeWords.insert(generateSupportedWakeWordsJson(entry.first, entry.second));
        }

        for (const auto& entry : assetsManager.getLocaleSpecificWakeWords()) {
            wakeWords.insert(generateSupportedWakeWordsJson(entry.first, entry.second));
        }

        json::JsonGenerator generator;
        generator.addMembersArray(CAPABILITY_INTERFACE_WAKE_WORDS_KEY, wakeWords);
        configMap.insert({CAPABILITY_INTERFACE_CONFIGURATIONS_KEY, generator.toString()});
        ACSDK_DEBUG5(LX(__func__).d("wakeWords", generator.toString()));
    }

    return std::make_shared<avsCommon::avs::CapabilityConfiguration>(configMap);
}

settings::SettingEventMetadata AudioInputProcessor::getWakeWordsEventsMetadata() {
    return settings::SettingEventMetadata{
        NAMESPACE, WAKE_WORDS_CHANGED_EVENT_NAME, WAKE_WORDS_REPORT_EVENT_NAME, WAKE_WORDS_PAYLOAD_KEY};
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
    m_expectSpeechTimeoutHandler.reset();
    if (m_powerResourceManager && m_powerResourceId) {
        m_powerResourceManager->close(m_powerResourceId);
    }
}

bool resolveMessageRequest(
    const std::shared_ptr<EditableMessageRequest>& request,
    const std::string& resolveKey,
    AudioInputProcessor::EncodingFormatResponse encodingFormats,
    const std::unordered_map<std::string, std::vector<std::shared_ptr<MessageRequest::NamedReader>>>& attachmentReaders,
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder) {
    if (!request) {
        ACSDK_ERROR(LX("Failed to resolve MessageRequest")
                        .d("reason", "Null pointer to MessageRequest")
                        .d("value", resolveKey));
        return false;
    }

    if (encodingFormats.find(resolveKey) == encodingFormats.end()) {
        ACSDK_ERROR(LX("Failed to resolve MessageRequest").d("reason", "Invalid resolveKey").d("value", resolveKey));
        return false;
    }

    rapidjson::Document eventWithContext;
    jsonUtils::parseJSON(request->getJsonContent(), &eventWithContext);
    auto event = eventWithContext.FindMember("event");
    if (event == eventWithContext.MemberEnd()) {
        ACSDK_ERROR(LX("Failed to resolve MessageRequest").d("reason", "No event found in json"));
        return false;
    }
    auto payload = event->value.FindMember("payload");
    if (payload == event->value.MemberEnd()) {
        ACSDK_ERROR(LX("Failed to resolve MessageRequest").d("reason", "No payload in json"));
        return false;
    }

    rapidjson::Value formatValue;
    auto encodingFormat = encodingFormats.at(resolveKey);
    auto formatString = encodingFormatToString(encodingFormat);
    formatValue.SetString(formatString.c_str(), static_cast<rapidjson::SizeType>(formatString.length()));

    if (payload->value.FindMember(FORMAT_KEY) != payload->value.MemberEnd()) {
        ACSDK_WARN(LX("Format already exists in Json payload. Replace it with").d("format", formatString));
        payload->value.FindMember(FORMAT_KEY)->value.SetString(formatString.c_str(), eventWithContext.GetAllocator());
    } else {
        payload->value.AddMember(
            rapidjson::StringRef(FORMAT_KEY.c_str()), formatValue, eventWithContext.GetAllocator());
    }

    // emit metric
    auto metricEvent = MetricEventBuilder{}
                           .setActivityName(AUDIO_ENCODING_FORMAT_ACTIVITY_NAME)
                           .addDataPoint(DataPointStringBuilder{}.setName("resolveKey").setValue(resolveKey).build());
    switch (encodingFormat) {
        case AudioFormat::Encoding::LPCM:
            submitMetric(
                metricRecorder,
                metricEvent.addDataPoint(
                    DataPointCounterBuilder{}.setName(AUDIO_ENCODING_FORMAT_LPCM).increment(1).build()));
            break;
        case AudioFormat::Encoding::OPUS:
            submitMetric(
                metricRecorder,
                metricEvent.addDataPoint(
                    DataPointCounterBuilder{}.setName(AUDIO_ENCODING_FORMAT_OPUS).increment(1).build()));
            break;
        default:
            break;
    }

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    eventWithContext.Accept(writer);

    std::string jsonContent(buffer.GetString());
    request->setJsonContent(jsonContent);
    if (attachmentReaders.find(resolveKey) != attachmentReaders.end()) {
        request->setAttachmentReaders(attachmentReaders.at(resolveKey));
    }
    return true;
}

std::future<bool> AudioInputProcessor::expectSpeechTimedOut() {
    return m_executor.submit([this]() { return executeExpectSpeechTimedOut(); });
}

void AudioInputProcessor::handleStopCaptureDirective(std::shared_ptr<DirectiveInfo> info) {
    m_stopCaptureReceivedTime = steady_clock::now();
    m_executor.execute([this, info]() {
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

    m_executor.execute([this, timeout, info]() { executeExpectSpeech(milliseconds{timeout}, info); });
}

void AudioInputProcessor::handleSetEndOfSpeechOffsetDirective(std::shared_ptr<DirectiveInfo> info) {
    // The duration from StopCapture to SetEndOfSpeechOffset. END_OF_SPEECH_OFFSET_RECEIVED starts from Recognize,
    // it is not helpful for figuring out the best SHORT timeout.
    submitMetric(
        m_metricRecorder,
        STOP_CAPTURE_TO_END_OF_SPEECH_ACTIVITY_NAME,
        DataPointDurationBuilder{duration_cast<milliseconds>(steady_clock::now() - m_stopCaptureReceivedTime)}
            .setName(STOP_CAPTURE_TO_END_OF_SPEECH_METRIC_NAME)
            .build(),
        info->directive);

    auto payload = info->directive->getPayload();
    int64_t endOfSpeechOffset = 0;

    std::string startOfSpeechTimeStampInString;
    bool foundEnd = json::jsonUtils::retrieveValue(payload, END_OF_SPEECH_OFFSET_FIELD_NAME, &endOfSpeechOffset);
    bool foundStart =
        json::jsonUtils::retrieveValue(payload, START_OF_SPEECH_TIMESTAMP_FIELD_NAME, &startOfSpeechTimeStampInString);

    if (foundEnd && foundStart) {
        int64_t startOfSpeechTimestamp = 0;
        auto offset = milliseconds(endOfSpeechOffset);
        submitMetric(
            m_metricRecorder,
            END_OF_SPEECH_OFFSET_RECEIVED_ACTIVITY_NAME,
            DataPointDurationBuilder{offset}.setName(END_OF_SPEECH_OFFSET_RECEIVED).build(),
            info->directive);
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

bool AudioInputProcessor::executeRecognize(
    AudioProvider provider,
    Initiator initiator,
    steady_clock::time_point startOfSpeechTimestamp,
    avsCommon::avs::AudioInputStream::Index begin,
    avsCommon::avs::AudioInputStream::Index end,
    const std::string& keyword,
    std::shared_ptr<const std::vector<char>> KWDMetadata,
    const std::string& initiatorToken) {
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
    std::string initiatorString = initiatorToString(initiator);
    generator.addMember(TYPE_KEY, initiatorString);
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

    if (!initiatorToken.empty()) {
        generator.addMember(TOKEN_KEY, initiatorToken);
    }

    bool initiatedByWakeword = (Initiator::WAKEWORD == initiator) ? true : false;

    return executeRecognize(
        provider,
        generator.toString(),
        startOfSpeechTimestamp,
        begin,
        end,
        keyword,
        KWDMetadata,
        initiatedByWakeword,
        falseWakewordDetection,
        initiatorString);
}

bool AudioInputProcessor::executeRecognize(
    AudioProvider provider,
    const std::string& initiatorJson,
    steady_clock::time_point startOfSpeechTimestamp,
    avsCommon::avs::AudioInputStream::Index begin,
    avsCommon::avs::AudioInputStream::Index end,
    const std::string& keyword,
    std::shared_ptr<const std::vector<char>> KWDMetadata,
    bool initiatedByWakeword,
    bool falseWakewordDetection,
    const std::string& initiatorString) {
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
            avsEncodingFormat = encodingFormatToString(provider.format.encoding);
            m_audioBytesForMetricThreshold = WAKEWORD_DETECTION_SEGMENT_SIZE_BYTES_PCM;
            m_uploadMetricName = WAKEWORD_DETECTION_SEGMENT_UPLOADED_PCM;
            break;
        case avsCommon::utils::AudioFormat::Encoding::OPUS:
            itSampleRateAVSEncoding = mapSampleRatesAVSEncoding.find(provider.format.sampleRateHz);
            if (itSampleRateAVSEncoding == mapSampleRatesAVSEncoding.end()) {
                ACSDK_ERROR(LX("executeRecognizeFailed")
                                .d("reason", "unsupportedSampleRateForOPUS")
                                .d("sampleRate", provider.format.sampleRateHz));
                return false;
            }

            avsEncodingFormat = encodingFormatToString(provider.format.encoding);
            m_audioBytesForMetricThreshold = WAKEWORD_DETECTION_SEGMENT_SIZE_BYTES_OPUS;
            m_uploadMetricName = WAKEWORD_DETECTION_SEGMENT_UPLOADED_OPUS;
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
            if (m_usingEncoder) {
                m_encoder->stopEncoding(true);
            }
            closeAttachmentReaders();
            break;
        case ObserverInterface::State::BUSY:
            ACSDK_ERROR(LX("executeRecognizeFailed").d("reason", "Barge-in is not permitted while busy"));
            return false;
    }

    if (settings::WakeWordConfirmationSettingType::TONE == m_wakeWordConfirmation->get()) {
        m_executor.execute(
            [this]() { m_systemSoundPlayer->playTone(SystemSoundPlayerInterface::Tone::WAKEWORD_NOTIFICATION); });
    }

    // Mutex required since following operations depend on value of @c m_encodingAudioFormats
    std::lock_guard<std::mutex> lock(m_encodingFormatMutex);

    // Assemble the event payload.
    json::JsonGenerator payloadGenerator;
    payloadGenerator.addMember(PROFILE_KEY, asrProfileToString(provider.profile));
    m_usingEncoder = isUsingEncoderLocked();

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
    auto reference = AudioInputStream::Reader::Reference::BEFORE_WRITER;
    if (INVALID_INDEX != begin) {
        offset = begin;
        reference = AudioInputStream::Reader::Reference::ABSOLUTE;
    }

    avsCommon::avs::AudioInputStream::Index encodingOffset = 0;
    AudioInputStream::Reader::Reference encodingReference = AudioInputStream::Reader::Reference::ABSOLUTE;

    // Set up the audio encoder
    std::shared_ptr<AudioInputStream> encodedStream;
    if (m_usingEncoder) {
        ACSDK_DEBUG(LX("encodingAudio").d("format", avsEncodingFormat));
        encodedStream = m_encoder->startEncoding(provider.stream, provider.format, offset, reference);
        if (!encodedStream) {
            ACSDK_ERROR(LX("executeRecognizeFailed").d("reason", "Failed to start encoder"));
            return false;
        }
    } else {
        ACSDK_DEBUG(LX("notEncodingAudio"));
    }

    if (KWDMetadata) {
        for (auto& it : m_encodingAudioFormats) {
            std::shared_ptr<attachment::AttachmentReader> metadataReader =
                attachment::AttachmentUtils::createAttachmentReader(*KWDMetadata);
            if (!metadataReader) {
                ACSDK_ERROR(LX("sendingKWDMetadataFailed").d("reason", "Failed to create attachment reader"));
                // If fail to create any metadata reader, clear all existing metadata for consistency
                closeAttachmentReaders(attachment::AttachmentReader::ClosePoint::IMMEDIATELY);
                break;
            }
            auto resolveKey = it.first;
            m_attachmentReaders[resolveKey] = {
                std::make_shared<MessageRequest::NamedReader>(KWD_METADATA_FIELD_NAME, metadataReader)};
        }
    }

    // N audio streams of different encodings
    for (auto& it : m_encodingAudioFormats) {
        ACSDK_DEBUG(
            LX("Create audio reader").d("format", it.second).d("offset", offset).d("encodingOffset", encodingOffset));

        const auto audioFormat = it.second;
        auto isLPCMEncodingAudioFormat = (AudioFormat::Encoding::LPCM == audioFormat);

        std::shared_ptr<attachment::AttachmentReader> audioReader =
            attachment::DefaultAttachmentReader<AudioInputStream>::create(
                sds::ReaderPolicy::NONBLOCKING,
                isLPCMEncodingAudioFormat ? provider.stream : encodedStream,
                isLPCMEncodingAudioFormat ? offset : encodingOffset,
                isLPCMEncodingAudioFormat ? reference : encodingReference);
        if (!audioReader) {
            ACSDK_ERROR(LX("executeRecognizeFailed").d("reason", "Failed to create attachment reader"));
            closeAttachmentReaders();
            return false;
        }
        auto resolveKey = it.first;
        m_attachmentReaders[resolveKey].push_back(
            std::make_shared<MessageRequest::NamedReader>(AUDIO_ATTACHMENT_FIELD_NAME, audioReader));
        ACSDK_INFO(LX("Create audio attachment reader success")
                       .d("resolveKey", resolveKey)
                       .d("format", encodingFormatToString(it.second)));

        if (resolveKey == CLOUD_RESOLVE_KEY) {
            if (isLPCMEncodingAudioFormat) {
                m_audioBytesForMetricThreshold = WAKEWORD_DETECTION_SEGMENT_SIZE_BYTES_PCM;
                m_uploadMetricName = WAKEWORD_DETECTION_SEGMENT_UPLOADED_PCM;
            } else {
                m_audioBytesForMetricThreshold = WAKEWORD_DETECTION_SEGMENT_SIZE_BYTES_OPUS;
                m_uploadMetricName = WAKEWORD_DETECTION_SEGMENT_UPLOADED_OPUS;
            }
        }
    }

    if (!multiStreamsRequestedLocked()) {
        m_messageRequestResolver = nullptr;

        // Set up format for single audio stream request
        if (m_usingEncoder) {
            avsEncodingFormat = m_encoder->getAVSFormatName();
        }
        payloadGenerator.addMember(FORMAT_KEY, avsEncodingFormat);

        // emit audio encoding format metric
        if (!m_encodingAudioFormats.empty()) {
            auto metricEvent = MetricEventBuilder{}.setActivityName(AUDIO_ENCODING_FORMAT_ACTIVITY_NAME);
            auto encodingAudioFormat = m_encodingAudioFormats.begin()->second;
            switch (encodingAudioFormat) {
                case avsCommon::utils::AudioFormat::Encoding::OPUS:
                    submitMetric(
                        m_metricRecorder,
                        metricEvent.addDataPoint(
                            DataPointCounterBuilder{}.setName(AUDIO_ENCODING_FORMAT_OPUS).increment(1).build()));
                    m_audioBytesForMetricThreshold = WAKEWORD_DETECTION_SEGMENT_SIZE_BYTES_OPUS;
                    m_uploadMetricName = WAKEWORD_DETECTION_SEGMENT_UPLOADED_OPUS;
                    break;
                case avsCommon::utils::AudioFormat::Encoding::LPCM:
                    submitMetric(
                        m_metricRecorder,
                        metricEvent.addDataPoint(
                            DataPointCounterBuilder{}.setName(AUDIO_ENCODING_FORMAT_LPCM).increment(1).build()));
                    m_audioBytesForMetricThreshold = WAKEWORD_DETECTION_SEGMENT_SIZE_BYTES_PCM;
                    m_uploadMetricName = WAKEWORD_DETECTION_SEGMENT_UPLOADED_PCM;
                    break;
                default:
                    ACSDK_WARN(LX("executeRecognize")
                                   .m("failed to emit audio encoding format metric")
                                   .d("reason", "unsupportedEncoding")
                                   .d("encoding", encodingFormatToString(encodingAudioFormat)));
                    break;
            }
        }
    } else {
        m_messageRequestResolver = getMessageRequestResolverLocked();
    }

    // Code below this point changes the state of AIP.  Formally update state now, and don't error out without calling
    // executeResetState() after this point.
    m_preCachedDialogRequestId = uuidGeneration::generateUUID();

    setState(ObserverInterface::State::RECOGNIZING);

    // Notify observers of current ASRProfile
    for (auto observer : m_observers) {
        observer->onASRProfileChanged(asrProfileToString(provider.profile));
    }

    // Note that we're preparing to send a Recognize event.
    m_preparingToSend = true;

    // Reset flag when we send a new recognize event.
    m_localStopCapturePerformed = false;
    m_streamIsClosedInRecognizingState = false;

    //  Start assembling the context; we'll service the callback after assembling our Recognize event.
    m_fetchContextTimeMetricData.startDurationTimer();
    m_contextManager->getContextWithoutReportableStateProperties(shared_from_this());

    // Stop the ExpectSpeech timer so we don't get a timeout.
    m_expectingSpeechTimer.stop();

    // Record provider as the last-used AudioProvider so it can be used in the event of an ExpectSpeech directive.
    m_lastAudioProvider = provider;

    // Record the Recognize event payload for later use by executeContextAvailable().
    m_recognizePayload = payloadGenerator.toString();

    // We can't assemble the MessageRequest until we receive the context.
    m_recognizeRequest.reset();

    // Handle metrics for this event.
    if (m_usingEncoder) {
        avsEncodingFormat = m_encoder->getAVSFormatName();
    }
    submitMetric(
        m_metricRecorder,
        MetricEventBuilder{}
            .setActivityName(START_OF_UTTERANCE_ACTIVITY_NAME)
            .addDataPoint(DataPointStringBuilder{}
                              .setName("TIME_SINCE_RESUME_ID")
                              .setValue(std::to_string(m_timeSinceLastResumeMS.count()))
                              .build())
            .addDataPoint(DataPointStringBuilder{}
                              .setName("TIME_SINCE_PARTIAL_ID")
                              .setValue(std::to_string(m_timeSinceLastPartialMS.count()))
                              .build())
            .addDataPoint(DataPointStringBuilder{}
                              .setName("RESOURCE_TYPE_ID")
                              .setValue(std::to_string(m_resourceFlags.to_ulong()))
                              .build())
            .addDataPoint(DataPointStringBuilder{}.setName("ENCODING_FORMAT").setValue(avsEncodingFormat).build())
            .addDataPoint(DataPointCounterBuilder{}.setName(START_OF_UTTERANCE).increment(1).build()),
        m_preCachedDialogRequestId);

    /// Submit initiator metric if available.
    if (!initiatorString.empty()) {
        submitMetric(
            m_metricRecorder,
            MetricEventBuilder{}
                .setActivityName(INITIATOR_ACTIVITY_NAME_PREFIX + initiatorString)
                .addDataPoint(
                    DataPointCounterBuilder{}.setName(INITIATOR_PREFIX + initiatorString).increment(1).build()),
            m_preCachedDialogRequestId);
    }

    if (initiatedByWakeword) {
        auto duration = milliseconds((end - begin) * MILLISECONDS_PER_SECOND / provider.format.sampleRateHz);

        auto startOfStreamTimestamp = startOfSpeechTimestamp;
        if (falseWakewordDetection) {
            startOfStreamTimestamp -= PREROLL_DURATION;
        }

        submitMetric(
            m_metricRecorder,
            MetricEventBuilder{}
                .setActivityName(WW_DURATION_ACTIVITY_NAME)
                .addDataPoint(DataPointDurationBuilder{duration}.setName(WW_DURATION).build())
                .addDataPoint(
                    DataPointDurationBuilder{duration_cast<milliseconds>(startOfStreamTimestamp.time_since_epoch())}
                        .setName(START_OF_STREAM_TIMESTAMP)
                        .build()),
            m_preCachedDialogRequestId);

        submitMetric(
            m_metricRecorder,
            MetricEventBuilder{}
                .setActivityName(RECOGNIZE_START_SEND_MESSAGE_ACTIVITY_NAME)
                .addDataPoint(DataPointCounterBuilder{}.setName(RECOGNIZE_START_SEND_MESSAGE).increment(1).build()),
            m_preCachedDialogRequestId);

        ACSDK_DEBUG(LX(__func__).d("WW_DURATION(ms)", duration.count()));
    }

    submitInstanceEntryMetric(
        m_metricRecorder,
        m_preCachedDialogRequestId,
        START_OF_UTTERANCE,
        std::map<std::string, std::string>{{"initiator", !initiatorString.empty() ? initiatorString : "unknown"}});

    return true;
}

void AudioInputProcessor::executeOnContextAvailable(const std::string& jsonContext) {
    ACSDK_DEBUG(LX("executeOnContextAvailable").sensitive("jsonContext", jsonContext));

    /// Submit execute context start metric
    submitMetric(
        m_metricRecorder,
        MetricEventBuilder{}
            .setActivityName(FETCH_CONTEXT_DURATION_ACTIVITY_NAME)
            .addDataPoint(m_fetchContextTimeMetricData.stopDurationTimer().build()),
        m_preCachedDialogRequestId);
    // Should already be RECOGNIZING if we get here.
    if (m_state != ObserverInterface::State::RECOGNIZING) {
        ACSDK_ERROR(
            LX("executeOnContextAvailableFailed").d("reason", "Not permitted in current state").d("state", m_state));
        return;
    }

    // Should already have a reader.
    if (m_attachmentReaders.empty()) {
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
        auto activity = FocusManagerInterface::Activity::create(
            NAMESPACE, shared_from_this(), std::chrono::milliseconds::zero(), avsCommon::avs::ContentType::MIXABLE);
        if (!m_focusManager->acquireChannel(CHANNEL_NAME, activity)) {
            ACSDK_ERROR(LX("executeOnContextAvailableFailed").d("reason", "Unable to acquire channel"));
            executeResetState();
            return;
        }
    }

    if (!m_preCachedDialogRequestId.empty()) {
        m_directiveSequencer->setDialogRequestId(m_preCachedDialogRequestId);
        m_preCachedDialogRequestId.clear();
    }

    // Assemble the MessageRequest.  It will be sent by executeOnFocusChanged when we acquire the channel.
    auto msgIdAndJsonEvent =
        buildJsonEventString("Recognize", m_directiveSequencer->getDialogRequestId(), m_recognizePayload, jsonContext);

    if (m_messageRequestResolver) {
        // Create unresolved MessageRequest
        m_recognizeRequest = std::make_shared<MessageRequest>(
            msgIdAndJsonEvent.second,
            true,
            "",
            std::vector<std::pair<std::string, std::string>>{},
            m_messageRequestResolver,
            m_audioBytesForMetricThreshold,
            m_uploadMetricName);
    } else {
        m_recognizeRequest = std::make_shared<MessageRequest>(
            msgIdAndJsonEvent.second, m_audioBytesForMetricThreshold, m_uploadMetricName);
        // Only add attachment readers for resolved MessageRequest. For unresolved one, attachment readers will be
        // passed to the resolver function.
        if (!m_attachmentReaders.empty() && !m_attachmentReaders.begin()->second.empty()) {
            for (auto& namedReader : m_attachmentReaders.begin()->second) {
                m_recognizeRequest->addAttachmentReader(namedReader->name, namedReader->reader);
            }
        }
    }
    // If we already have focus, there won't be a callback to send the message, so send it now.
    if (avsCommon::avs::FocusState::FOREGROUND == m_focusState) {
        sendRequestNow();
    }
}

void AudioInputProcessor::executeOnContextFailure(const ContextRequestError error) {
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
        auto returnValue = false;
        if (info) {
            if (info->result) {
                if (m_localStopCapturePerformed || m_streamIsClosedInRecognizingState) {
                    // Since a local StopCapture was performed or the event stream is closed while in RECOGNIZING state,
                    // we can safely ignore the StopCapture from AVS.
                    ACSDK_INFO(LX("executeStopCapture")
                                   .d("localStopCapturePerformed", m_localStopCapturePerformed)
                                   .d("streamIsClosedInRecognizingState", m_streamIsClosedInRecognizingState)
                                   .m("StopCapture directive ignored because local StopCapture was performed or event"
                                      " stream is closed while in recognizing state."));
                    m_localStopCapturePerformed = false;
                    m_streamIsClosedInRecognizingState = false;
                    returnValue = true;
                    info->result->setCompleted();
                } else {
                    static const char* errorMessage = "StopCapture only allowed in RECOGNIZING state.";
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
        if (m_usingEncoder) {
            // If Audio Encoder is enabled, let it finish it so the stream will be closed automatically.
            m_encoder->stopEncoding(stopImmediately);
            m_usingEncoder = false;
        }
        auto closePoint = stopImmediately ? attachment::AttachmentReader::ClosePoint::IMMEDIATELY
                                          : attachment::AttachmentReader::ClosePoint::AFTER_DRAINING_CURRENT_BUFFER;
        closeAttachmentReaders(closePoint);

        setState(ObserverInterface::State::BUSY);

        if (info) {
            if (info->result) {
                info->result->setCompleted();
            }
            removeDirective(info);
        }

        if (m_speechConfirmation->get() == settings::SpeechConfirmationSettingType::TONE) {
            m_systemSoundPlayer->playTone(SystemSoundPlayerInterface::Tone::END_SPEECH);
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
    ACSDK_DEBUG(LX(__func__));
    m_expectingSpeechTimer.stop();
    m_precedingExpectSpeechInitiator.reset();
    closeAttachmentReaders();
    if (m_encoder) {
        m_encoder->stopEncoding(true);
    }
    if (m_recognizeRequestSent) {
        m_recognizeRequestSent->removeObserver(shared_from_this());
        m_recognizeRequestSent.reset();
    }
    m_recognizeRequest.reset();
    m_preparingToSend = false;
    m_deferredStopCapture = nullptr;
    if (m_focusState != avsCommon::avs::FocusState::NONE) {
        m_focusManager->releaseChannel(CHANNEL_NAME, shared_from_this());
    }
    m_focusState = avsCommon::avs::FocusState::NONE;
    setState(ObserverInterface::State::IDLE);
    m_audioBytesForMetricThreshold = 0;
}

bool AudioInputProcessor::executeExpectSpeech(milliseconds timeout, std::shared_ptr<DirectiveInfo> info) {
    if (info && info->isCancelled) {
        ACSDK_DEBUG(LX("expectSpeechIgnored").d("reason", "isCancelled"));
        return true;
    }
    if (m_state != ObserverInterface::State::IDLE && m_state != ObserverInterface::State::BUSY) {
        if (info->result) {
            static const char* errorMessage = "ExpectSpeech only allowed in IDLE or BUSY state.";
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

    setState(ObserverInterface::State::EXPECTING_SPEECH);
    if (info->result) {
        info->result->setCompleted();
    }
    removeDirective(info);

    if (m_expectSpeechTimeoutHandler &&
        m_expectSpeechTimeoutHandler->handleExpectSpeechTimeout(timeout, [this]() { return expectSpeechTimedOut(); })) {
        ACSDK_DEBUG(LX(__func__).m("Expect speech timeout being handled externally"));
        // Let the ExpectSpeech timeout will be handled externally.
    } else {
        ACSDK_DEBUG(LX(__func__).m("Expect speech timeout being handled internally"));
        // Start the ExpectSpeech timer.
        if (!m_expectingSpeechTimer.start(timeout, std::bind(&AudioInputProcessor::expectSpeechTimedOut, this))
                 .valid()) {
            ACSDK_ERROR(LX("executeExpectSpeechFailed").d("reason", "startTimerFailed"));
        }
    }

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
    auto request = std::make_shared<MessageRequest>(msgIdAndJsonEvent.second);
    request->addObserver(shared_from_this());
    m_messageSender->sendMessage(request);
    setState(ObserverInterface::State::IDLE);
    ACSDK_ERROR(LX("executeExpectSpeechFailed").d("reason", "Timed Out"));
    return true;
}

void AudioInputProcessor::executeOnDialogUXStateChanged(DialogUXStateObserverInterface::DialogUXState newState) {
    ACSDK_DEBUG0(LX(__func__).d("newState", newState));

    if (!m_initialDialogUXStateReceived) {
        // The initial dialog UX state change call comes from simply registering as an observer; it is not a deliberate
        // change to the dialog state which should interrupt a recognize event.
        m_initialDialogUXStateReceived = true;
        return;
    }

    if (newState != DialogUXStateObserverInterface::DialogUXState::IDLE) {
        return;
    }

    executeResetState();
}

void AudioInputProcessor::setState(ObserverInterface::State state) {
    if (m_state == state) {
        return;
    }

    // Reset the user inactivity if transitioning to or from `RECOGNIZING` state.
    if (ObserverInterface::State::RECOGNIZING == m_state || ObserverInterface::State::RECOGNIZING == state) {
        m_executor.execute([this]() { m_userInactivityMonitor->onUserActive(); });
    }

    auto currentDialogRequestId =
        m_preCachedDialogRequestId.empty() ? m_directiveSequencer->getDialogRequestId() : m_preCachedDialogRequestId;
    ACSDK_DEBUG5(LX(__func__).d("currentDialogRequestId", currentDialogRequestId));
    if (!currentDialogRequestId.empty()) {
        submitInstanceEntryMetric(
            m_metricRecorder,
            currentDialogRequestId,
            ENTRY_METRIC_NAME_STATE_CHANGE,
            std::map<std::string, std::string>{{"from", ObserverInterface::stateToString(m_state)},
                                               {"to", ObserverInterface::stateToString(state)}});
    }

    ACSDK_DEBUG(LX("setState").d("from", m_state).d("to", state));
    m_state = state;
    managePowerResource(m_state);

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
    ACSDK_DEBUG(LX(__func__));

    if (m_recognizeRequest) {
        ACSDK_METRIC_IDS(TAG, "Recognize", "", "", Metrics::Location::AIP_SEND);
        if (m_recognizeRequestSent && (m_recognizeRequestSent != m_recognizeRequest)) {
            m_recognizeRequestSent->removeObserver(shared_from_this());
        }
        m_recognizeRequestSent = m_recognizeRequest;
        m_recognizeRequestSent->addObserver(shared_from_this());
        m_messageSender->sendMessage(m_recognizeRequestSent);

        m_recognizeRequest.reset();
        m_preparingToSend = false;
        if (m_deferredStopCapture) {
            m_deferredStopCapture();
            m_deferredStopCapture = nullptr;
        }
    }
}

void AudioInputProcessor::onResponseStatusReceived(
    avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status status) {
    ACSDK_DEBUG(LX("onResponseStatusReceived").d("status", status));
    if (status != MessageRequestObserverInterface::Status::SUCCESS) {
        resetState();
    }
}

void AudioInputProcessor::onExceptionReceived(const std::string& exceptionMessage) {
    ACSDK_ERROR(LX("onExceptionReceived").d("exception", exceptionMessage));
    resetState();
}

void AudioInputProcessor::onSendCompleted(MessageRequestObserverInterface::Status status) {
    ACSDK_DEBUG(LX("onSendCompleted").d("status", status));
    m_executor.execute([this, status]() {
        if (MessageRequestObserverInterface::Status::SUCCESS == status &&
            ObserverInterface::State::RECOGNIZING == m_state) {
            // This is to take care of the edge case where the event stream is closed before a stop capture is received.
            m_streamIsClosedInRecognizingState = true;
        }
        executeResetState();
    });
}
std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> AudioInputProcessor::
    getCapabilityConfigurations() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_capabilityConfigurations;
}

void AudioInputProcessor::onLocaleAssetsChanged() {
    m_executor.execute([this]() { executeOnLocaleAssetsChanged(); });
}

void AudioInputProcessor::executeOnLocaleAssetsChanged() {
    ACSDK_DEBUG(LX(__func__));
    std::unique_lock<std::mutex> lock(m_mutex);
    m_capabilityConfigurations.clear();
    auto newWakeWordsConfiguration = getSpeechRecognizerCapabilityConfiguration(*m_assetsManager);
    m_capabilityConfigurations.insert(newWakeWordsConfiguration);
    lock.unlock();
    m_capabilityChangeNotifier->notifyObservers(
        [newWakeWordsConfiguration](
            std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationChangeObserverInterface> observer) {
            observer->onConfigurationChanged(*newWakeWordsConfiguration);
        });
}

void AudioInputProcessor::handleDirectiveFailure(
    const std::string& errorMessage,
    std::shared_ptr<DirectiveInfo> info,
    avsCommon::avs::ExceptionErrorType errorType) {
    m_exceptionEncounteredSender->sendExceptionEncountered(
        info->directive->getUnparsedDirective(), errorType, errorMessage);

    if (info->result) {
        info->result->setFailed(errorMessage);
    }

    removeDirective(info);

    ACSDK_ERROR(LX("handleDirectiveFailed").d("error", errorMessage));
}

settings::SettingEventMetadata AudioInputProcessor::getWakeWordConfirmationMetadata() {
    return settings::SettingEventMetadata{NAMESPACE,
                                          WAKE_WORD_CONFIRMATION_CHANGED_EVENT_NAME,
                                          WAKE_WORD_CONFIRMATION_REPORT_EVENT_NAME,
                                          WAKE_WORD_CONFIRMATION_PAYLOAD_KEY};
}

settings::SettingEventMetadata AudioInputProcessor::getSpeechConfirmationMetadata() {
    return settings::SettingEventMetadata{NAMESPACE,
                                          SPEECH_CONFIRMATION_CHANGED_EVENT_NAME,
                                          SPEECH_CONFIRMATION_REPORT_EVENT_NAME,
                                          SPEECH_CONFIRMATION_PAYLOAD_KEY};
}

bool AudioInputProcessor::handleSetWakeWordConfirmation(std::shared_ptr<DirectiveInfo> info) {
    std::string jsonValue;
    bool found = avsCommon::utils::json::jsonUtils::retrieveValue(
        info->directive->getPayload(), WAKE_WORD_CONFIRMATION_PAYLOAD_KEY, &jsonValue);

    if (!found) {
        std::string errorMessage = "missing " + WAKE_WORD_CONFIRMATION_PAYLOAD_KEY;
        sendExceptionEncounteredAndReportFailed(
            info, errorMessage, avsCommon::avs::ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return false;
    }

    settings::WakeWordConfirmationSettingType value = settings::getWakeWordConfirmationDefault();
    std::stringstream ss{jsonValue};
    ss >> value;

    if (ss.fail()) {
        std::string errorMessage = "invalid " + WAKE_WORD_CONFIRMATION_PAYLOAD_KEY;
        sendExceptionEncounteredAndReportFailed(
            info, errorMessage, avsCommon::avs::ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return false;
    }

    auto executeChange = [this, value]() { m_wakeWordConfirmation->setAvsChange(value); };

    m_executor.execute(executeChange);

    if (info->result) {
        info->result->setCompleted();
    }

    removeDirective(info);

    return true;
}

bool AudioInputProcessor::handleSetSpeechConfirmation(std::shared_ptr<DirectiveInfo> info) {
    std::string jsonValue;
    bool found = avsCommon::utils::json::jsonUtils::retrieveValue(
        info->directive->getPayload(), SPEECH_CONFIRMATION_PAYLOAD_KEY, &jsonValue);

    if (!found) {
        std::string errorMessage = "missing/invalid " + SPEECH_CONFIRMATION_PAYLOAD_KEY;
        sendExceptionEncounteredAndReportFailed(
            info, errorMessage, avsCommon::avs::ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return false;
    }

    settings::SpeechConfirmationSettingType value = settings::getSpeechConfirmationDefault();
    std::stringstream ss{jsonValue};
    ss >> value;

    if (ss.fail()) {
        std::string errorMessage = "invalid " + SPEECH_CONFIRMATION_PAYLOAD_KEY;
        sendExceptionEncounteredAndReportFailed(
            info, errorMessage, avsCommon::avs::ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return false;
    }

    auto executeChange = [this, value]() { m_speechConfirmation->setAvsChange(value); };

    m_executor.execute(executeChange);

    if (info->result) {
        info->result->setCompleted();
    }

    removeDirective(info);

    return true;
}

bool AudioInputProcessor::handleSetWakeWords(std::shared_ptr<DirectiveInfo> info) {
    auto wakeWords = json::jsonUtils::retrieveStringArray<settings::WakeWords>(
        info->directive->getPayload(), WAKE_WORDS_PAYLOAD_KEY);

    if (wakeWords.empty()) {
        std::string errorMessage = "missing/invalid " + WAKE_WORDS_PAYLOAD_KEY;
        sendExceptionEncounteredAndReportFailed(
            info, errorMessage, avsCommon::avs::ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return false;
    }

    if (!m_wakeWordsSetting) {
        std::string errorMessage = "Wake words are not supported in this device";
        sendExceptionEncounteredAndReportFailed(
            info, errorMessage, avsCommon::avs::ExceptionErrorType::UNSUPPORTED_OPERATION);
        return false;
    }

    m_executor.execute([this, wakeWords, info]() { m_wakeWordsSetting->setAvsChange(wakeWords); });

    if (info->result) {
        info->result->setCompleted();
    }

    removeDirective(info);

    return true;
}

void AudioInputProcessor::managePowerResource(ObserverInterface::State newState) {
    if (!m_powerResourceId) {
        return;
    }
    auto startTime = steady_clock::now();
    ACSDK_DEBUG5(LX(__func__).d("state", newState));
    switch (newState) {
        case ObserverInterface::State::RECOGNIZING:
        case ObserverInterface::State::EXPECTING_SPEECH:
            m_powerResourceManager->acquire(m_powerResourceId);
            m_timeSinceLastResumeMS = m_powerResourceManager->getTimeSinceLastResumeMS();
            m_timeSinceLastPartialMS =
                m_powerResourceManager->getTimeSinceLastPartialMS(POWER_RESOURCE_COMPONENT_NAME, m_resourceFlags);
            submitMetric(
                m_metricRecorder,
                MetricEventBuilder{}
                    .setActivityName(ACQUIRE_POWER_RESOURCE_ACTIVITY)
                    .addDataPoint(DataPointDurationBuilder{duration_cast<milliseconds>(steady_clock::now() - startTime)}
                                      .setName(ACQUIRE_POWER_RESOURCE)
                                      .build()),
                m_preCachedDialogRequestId);
            break;
        case ObserverInterface::State::BUSY:
        case ObserverInterface::State::IDLE:
            m_powerResourceManager->release(m_powerResourceId);
            submitMetric(
                m_metricRecorder,
                MetricEventBuilder{}
                    .setActivityName(RELEASE_POWER_RESOURCE_ACTIVITY)
                    .addDataPoint(DataPointDurationBuilder{duration_cast<milliseconds>(steady_clock::now() - startTime)}
                                      .setName(RELEASE_POWER_RESOURCE)
                                      .build()),
                m_preCachedDialogRequestId);
            break;
    }
}

void AudioInputProcessor::onConnectionStatusChanged(bool connected) {
    if (!connected) {
        m_executor.execute([this]() { return executeDisconnected(); });
    }
}

void AudioInputProcessor::executeDisconnected() {
    if (ObserverInterface::State::IDLE != m_state) {
        ACSDK_WARN(LX(__func__).d("state", AudioInputProcessorObserverInterface::stateToString(m_state)));
        executeResetState();
    }
}

bool AudioInputProcessor::setEncodingAudioFormat(AudioFormat::Encoding encoding) {
    if (encoding == AudioFormat::Encoding::LPCM || (m_encoder && encoding == m_encoder->getEncoding())) {
        std::lock_guard<std::mutex> lock(m_encodingFormatMutex);
        m_encodingAudioFormats.clear();
        // Only one format is configured, and AIP will send resolved RequestMessage, and this resolveKey is simply a
        // placeholder
        m_encodingAudioFormats.emplace(DEFAULT_RESOLVE_KEY, encoding);
        return true;
    }
    return false;
}

bool AudioInputProcessor::initialize() {
    if (m_encoder) {
        std::lock_guard<std::mutex> lock(m_encodingFormatMutex);
        m_encodingAudioFormats.clear();
        m_encodingAudioFormats.emplace(DEFAULT_RESOLVE_KEY, m_encoder->getEncoding());
    }
    m_assetsManager->addLocaleAssetsObserver(shared_from_this());
    return true;
}

MessageRequest::MessageRequestResolveFunction AudioInputProcessor::getMessageRequestResolverLocked() const {
    auto encodingAudioFormats = m_encodingAudioFormats;
    auto attachmentReaders = m_attachmentReaders;
    auto metricRecorder = m_metricRecorder;
    MessageRequest::MessageRequestResolveFunction resolver = [encodingAudioFormats, attachmentReaders, metricRecorder](
                                                                 const std::shared_ptr<EditableMessageRequest>& req,
                                                                 std::string resolveKey) {
        return resolveMessageRequest(req, resolveKey, encodingAudioFormats, attachmentReaders, metricRecorder);
    };
    return resolver;
}

AudioInputProcessor::EncodingFormatResponse AudioInputProcessor::requestEncodingAudioFormats(
    const EncodingFormatRequest& encodings) {
    EncodingFormatResponse result;
    for (auto it = encodings.begin(); it != encodings.end(); it++) {
        auto requestedEncoding = it->second.first;
        auto fallbackEncoding = it->second.second;
        if (isEncodingFormatSupported(requestedEncoding)) {
            result[it->first] = requestedEncoding;
        } else if (isEncodingFormatSupported(fallbackEncoding)) {
            result[it->first] = fallbackEncoding;
        } else {
            ACSDK_WARN(LX("Both provided requested and fallback encoding formats are not supported.")
                           .d("resolveKey", it->first)
                           .d("requestedEncoding", encodingFormatToString(requestedEncoding))
                           .d("fallbackEncoding", encodingFormatToString(fallbackEncoding)));
        }
    }
    if (!result.empty()) {
        std::lock_guard<std::mutex> lock(m_encodingFormatMutex);
        m_encodingAudioFormats = result;
    } else {
        ACSDK_ERROR(LX("None of requested encoding audio formats are supported."));
    }
    return result;
}

void AudioInputProcessor::closeAttachmentReaders(attachment::AttachmentReader::ClosePoint closePoint) {
    for (auto& it : m_attachmentReaders) {
        for (auto& namedReader : it.second) {
            if (namedReader && namedReader->reader) {
                namedReader->reader->close(closePoint);
            }
        }
    }
    m_attachmentReaders.clear();
}

bool AudioInputProcessor::isEncodingFormatSupported(avsCommon::utils::AudioFormat::Encoding encodingFormat) const {
    if (encodingFormat == AudioFormat::Encoding::LPCM || (m_encoder && encodingFormat == m_encoder->getEncoding())) {
        return true;
    }
    return false;
}

bool AudioInputProcessor::isUsingEncoderLocked() const {
    return std::any_of(
        m_encodingAudioFormats.begin(),
        m_encodingAudioFormats.end(),
        [](EncodingFormatResponse::const_reference entry) { return entry.second != AudioFormat::Encoding::LPCM; });
}

bool AudioInputProcessor::multiStreamsRequestedLocked() const {
    return m_encodingAudioFormats.size() > 1;
}

AudioInputProcessor::EncodingFormatResponse AudioInputProcessor::getEncodingAudioFormats() const {
    std::lock_guard<std::mutex> lock(m_encodingFormatMutex);
    return m_encodingAudioFormats;
}

}  // namespace aip
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
