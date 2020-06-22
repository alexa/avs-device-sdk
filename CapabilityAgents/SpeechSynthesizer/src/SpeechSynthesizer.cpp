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

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Metrics.h>
#include <AVSCommon/Utils/Metrics/DataPointCounterBuilder.h>
#include <AVSCommon/Utils/Metrics/DataPointStringBuilder.h>
#include <Captions/CaptionData.h>
#include <Captions/CaptionFormat.h>

#include "SpeechSynthesizer/SpeechSynthesizer.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace speechSynthesizer {

using namespace avsCommon::utils;
using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::mediaPlayer;
using namespace avsCommon::utils::metrics;
using namespace avsCommon::sdkInterfaces;
using namespace rapidjson;
using MediaPlayerState = avsCommon::utils::mediaPlayer::MediaPlayerState;

/// SpeechSynthesizer capability constants
/// SpeechSynthesizer interface type
static const std::string SPEECHSYNTHESIZER_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";
/// SpeechSynthesizer interface name
static const std::string SPEECHSYNTHESIZER_CAPABILITY_INTERFACE_NAME = "SpeechSynthesizer";
/// SpeechSynthesizer interface version
static const std::string SPEECHSYNTHESIZER_CAPABILITY_INTERFACE_VERSION = "1.3";

/// String to identify log entries originating from this file.
static const std::string TAG{"SpeechSynthesizer"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The namespace for the @c SpeechSynthesizer capability agent.
static const std::string NAMESPACE{"SpeechSynthesizer"};

/// The @c Speak directive signature.
static const NamespaceAndName SPEAK{NAMESPACE, "Speak"};

/// The state information @c NamespaceAndName to send to the @c ContextManager.
static const NamespaceAndName CONTEXT_MANAGER_SPEECH_STATE{NAMESPACE, "SpeechState"};

/// The name of the @c FocusManager channel used by the @c SpeechSynthesizer.
static const std::string CHANNEL_NAME = FocusManagerInterface::DIALOG_CHANNEL_NAME;

/// The name of the event to send to the AVS server once audio starting playing.
static std::string SPEECH_STARTED_EVENT_NAME{"SpeechStarted"};

/// The name of the event to send to the AVS server once audio finishes playing.
static std::string SPEECH_FINISHED_EVENT_NAME{"SpeechFinished"};

/// The name of the event to send to the AVS server once audio playing has been interrupted.
static std::string SPEECH_INTERRUPTED_EVENT_NAME{"SpeechInterrupted"};

/// The key used to look up the "url" property in the directive payload.
static const char KEY_URL[] = "url";

/// The key for the "token" property in the directive payload string and "SpeechState" string.
static const char KEY_TOKEN[] = "token";

/// The key used to look the "format" property in the directive payload string.
static const char KEY_FORMAT[] = "format";

/// The key for the "captionData" property in the directive payload.
static const char KEY_CAPTION[] = "caption";

/// The key under "captionData" containing the caption type
static const char KEY_CAPTION_TYPE[] = "type";

/// The key under "captionData" containing the caption content
static const char KEY_CAPTION_CONTENT[] = "content";

/// The key used to look the "playBehavior" property in the directive payload string.
static const char KEY_PLAY_BEHAVIOR[] = "playBehavior";

/// The expected format value in the directive payload.
static const std::string FORMAT{"AUDIO_MPEG"};

/// Prefix for content ID prefix in the url property of the directive payload.
static const std::string CID_PREFIX{"cid:"};

/// The key for the "offsetInMilliseconds" property in the event context.
static const char KEY_OFFSET_IN_MILLISECONDS[] = "offsetInMilliseconds";

/// The "playerActivity" key used to build "SpeechState" string.
static const char KEY_PLAYER_ACTIVITY[] = "playerActivity";

/// The "analyzers" key used to retrieve analyzer data from directive.
static const char KEY_ANALYZERS[] = "analyzers";

/// The key used to retrieve the audio analyzer name from directive.
static const char KEY_ANALYZERS_INTERFACE[] = "interface";

/// The key used to retrieve the audio analyzer enabled state from directive.
static const char KEY_ANALYZERS_ENABLED[] = "enabled";

/// The player activity value used to build the "SpeechState" string.
static const char PLAYER_STATE_PLAYING[] = "PLAYING";

/// The player activity value used to build the "SpeechState" string.
static const char PLAYER_STATE_FINISHED[] = "FINISHED";

/// The player activity value used to build the "SpeechState" string.
static const char PLAYER_STATE_INTERRUPTED[] = "INTERRUPTED";

/// The duration to wait for a state change in @c onFocusChanged before failing.
static const std::chrono::seconds STATE_CHANGE_TIMEOUT{5};

/// The component name of power resource
static const std::string POWER_RESOURCE_COMPONENT_NAME{"SpeechSynthesizer"};

/// Metric prefix for SpeechSynthesizer metric source
static const std::string SPEECH_SYNTHESIZER_METRIC_PREFIX = "SPEECH_SYNTHESIZER-";

/// Metric to emit when received first audio bytes
static const std::string FIRST_BYTES_AUDIO = "FIRST_BYTES_AUDIO";

/// Metric to emit at the start of TTS
static const std::string TTS_STARTED = "TTS_STARTED";

/// Metric to emit when TTS finishes
static const std::string TTS_FINISHED = "TTS_FINISHED";

/// Key name for the dialogRequestId metric information
static const std::string DIALOG_REQUEST_ID_KEY = "DIALOG_REQUEST_ID";

/// Metric to emit on TTS buffer underrrun
static const std::string BUFFER_UNDERRUN = "ERROR.TTS_BUFFER_UNDERRUN";

/**
 * Creates the SpeechSynthesizer capability configuration.
 *
 * @return The SpeechSynthesizer capability configuration.
 */
static std::shared_ptr<avsCommon::avs::CapabilityConfiguration> getSpeechSynthesizerCapabilityConfiguration();

std::shared_ptr<SpeechSynthesizer> SpeechSynthesizer::create(
    std::shared_ptr<MediaPlayerInterface> mediaPlayer,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<FocusManagerInterface> focusManager,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
    std::shared_ptr<avsCommon::avs::DialogUXStateAggregator> dialogUXStateAggregator,
    std::shared_ptr<captions::CaptionManagerInterface> captionManager,
    std::shared_ptr<PowerResourceManagerInterface> powerResourceManager) {
    if (!mediaPlayer) {
        ACSDK_ERROR(LX("SpeechSynthesizerCreationFailed").d("reason", "mediaPlayerNullReference"));
        return nullptr;
    }
    if (!messageSender) {
        ACSDK_ERROR(LX("SpeechSynthesizerCreationFailed").d("reason", "messageSenderNullReference"));
        return nullptr;
    }
    if (!focusManager) {
        ACSDK_ERROR(LX("SpeechSynthesizerCreationFailed").d("reason", "focusManagerNullReference"));
        return nullptr;
    }
    if (!contextManager) {
        ACSDK_ERROR(LX("SpeechSynthesizerCreationFailed").d("reason", "contextManagerNullReference"));
        return nullptr;
    }
    if (!exceptionSender) {
        ACSDK_ERROR(LX("SpeechSynthesizerCreationFailed").d("reason", "exceptionSenderNullReference"));
        return nullptr;
    }
    auto speechSynthesizer = std::shared_ptr<SpeechSynthesizer>(new SpeechSynthesizer(
        mediaPlayer,
        messageSender,
        focusManager,
        contextManager,
        metricRecorder,
        exceptionSender,
        captionManager,
        powerResourceManager));
    speechSynthesizer->init();

    dialogUXStateAggregator->addObserver(speechSynthesizer);

    return speechSynthesizer;
}

avsCommon::avs::DirectiveHandlerConfiguration SpeechSynthesizer::getConfiguration() const {
    DirectiveHandlerConfiguration configuration;
    configuration[SPEAK] = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, true);
    return configuration;
}

void SpeechSynthesizer::addObserver(std::shared_ptr<SpeechSynthesizerObserverInterface> observer) {
    ACSDK_DEBUG9(LX("addObserver").d("observer", observer.get()));
    m_executor.submit([this, observer]() { m_observers.insert(observer); });
}

void SpeechSynthesizer::removeObserver(std::shared_ptr<SpeechSynthesizerObserverInterface> observer) {
    ACSDK_DEBUG9(LX("removeObserver").d("observer", observer.get()));
    m_executor.submit([this, observer]() { m_observers.erase(observer); }).wait();
}

void SpeechSynthesizer::onDeregistered() {
    ACSDK_DEBUG9(LX("onDeregistered"));
    // default no-op
}

void SpeechSynthesizer::handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) {
    ACSDK_DEBUG9(LX("handleDirectiveImmediately").d("messageId", directive->getMessageId()));
    auto info = createDirectiveInfo(directive, nullptr);
    m_executor.submit([this, info]() { executeHandleImmediately(info); });
}

void SpeechSynthesizer::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    if (!info || !info->directive) {
        ACSDK_ERROR(LX("preHandleDirectiveFailed").d("reason", !info ? "nullInfo" : "nullDirective"));
        return;
    }

    ACSDK_DEBUG9(LX("preHandleDirective").d("messageId", info->directive->getMessageId()));
    m_executor.submit([this, info]() { executePreHandle(info); });
}

void SpeechSynthesizer::handleDirective(std::shared_ptr<DirectiveInfo> info) {
    if (!info) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullInfo"));
        return;
    }

    ACSDK_DEBUG9(LX("handleDirective").d("messageId", info->directive->getMessageId()));
    if (info->directive->getName() == "Speak") {
        ACSDK_METRIC_MSG(TAG, info->directive, Metrics::Location::SPEECH_SYNTHESIZER_RECEIVE);
    }
    m_executor.submit([this, info]() { executeHandle(info); });
}

void SpeechSynthesizer::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX("cancelDirective").d("messageId", info->directive->getMessageId()));
    m_executor.submit([this, info]() { executeCancel(info); });
}

void SpeechSynthesizer::onFocusChanged(FocusState newFocus, MixingBehavior behavior) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_currentFocus = newFocus;
    if (m_currentState == m_desiredState) {
        ACSDK_DEBUG(LX("onFocusChanged").d("newFocus", newFocus).d("result", "skip").d("state", m_currentState));
        return;
    }

    // Set intermediate state to avoid being considered idle
    ACSDK_DEBUG(LX("onFocusChanged").d("newFocus", newFocus).d("MixingBehavior", behavior));
    auto desiredState = m_desiredState;
    switch (newFocus) {
        case FocusState::FOREGROUND:
            setCurrentStateLocked(SpeechSynthesizerObserverInterface::SpeechSynthesizerState::GAINING_FOCUS);
            break;
        case FocusState::BACKGROUND:
            setCurrentStateLocked(SpeechSynthesizerObserverInterface::SpeechSynthesizerState::LOSING_FOCUS);
        case FocusState::NONE:
            if (SpeechSynthesizerObserverInterface::SpeechSynthesizerState::INTERRUPTED == m_currentState ||
                SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED == m_currentState) {
                ACSDK_DEBUG5(LX(__func__).d("result", "skip").d("state", m_currentState));
                return;
            }
            desiredState = SpeechSynthesizerObserverInterface::SpeechSynthesizerState::INTERRUPTED;
            break;
    }

    auto currentInfo = std::make_shared<std::shared_ptr<SpeakDirectiveInfo>>(nullptr);
    m_executor.submit([this, desiredState, currentInfo]() {
        *currentInfo = m_currentInfo;
        executeStateChange(desiredState);
    });
    // Block until we achieve the desired state or playback is being interrupted.
    if (m_waitOnStateChange.wait_for(
            lock, STATE_CHANGE_TIMEOUT, [this, desiredState]() { return m_currentState == desiredState; })) {
        ACSDK_DEBUG9(LX("onFocusChangedSuccess"));
    } else {
        ACSDK_ERROR(LX("onFocusChangeFailed")
                        .d("reason", "stateChangeTimeout")
                        .d("initialDesiredState", desiredState)
                        .d("desiredState", m_desiredState)
                        .d("currentState", m_currentState));
        m_executor.submit([this, currentInfo]() {
            ACSDK_DEBUG9(
                LX("onFocusChangedLambda")
                    .d("currentInfo",
                       *currentInfo && (*currentInfo)->directive ? (*currentInfo)->directive->getMessageId() : "null")
                    .d("m_currentInfo",
                       m_currentInfo && m_currentInfo->directive ? m_currentInfo->directive->getMessageId() : "null"));
            if (m_currentInfo && (m_currentInfo == *currentInfo)) {
                std::string error{"stateChangeTimeout"};
                if (m_currentInfo->directive) {
                    error += " messageId=" + m_currentInfo->directive->getMessageId();
                }
                sendExceptionEncounteredAndReportFailed(
                    m_currentInfo, avsCommon::avs::ExceptionErrorType::INTERNAL_ERROR, error);
            }
        });
    }
}

void SpeechSynthesizer::provideState(
    const avsCommon::avs::NamespaceAndName& stateProviderName,
    const unsigned int stateRequestToken) {
    ACSDK_DEBUG9(LX("provideState").d("token", stateRequestToken));
    m_executor.submit([this, stateRequestToken]() {
        std::lock_guard<std::mutex> lock(m_mutex);
        executeProvideStateLocked(stateRequestToken);
    });
}

void SpeechSynthesizer::onContextAvailable(const std::string& jsonContext) {
    ACSDK_DEBUG9(LX("onContextAvailable").d("context", jsonContext));
    // default no-op
}

void SpeechSynthesizer::onContextFailure(const ContextRequestError error) {
    ACSDK_DEBUG9(LX("onContextFailure").d("error", error));
    // default no-op
}

void SpeechSynthesizer::onFirstByteRead(SourceId id, const MediaPlayerState&) {
    ACSDK_DEBUG(LX(__func__).d("id", id));
    submitMetric(MetricEventBuilder{}
                     .setActivityName(SPEECH_SYNTHESIZER_METRIC_PREFIX + FIRST_BYTES_AUDIO)
                     .addDataPoint(DataPointCounterBuilder{}.setName(FIRST_BYTES_AUDIO).increment(1).build()));
}

void SpeechSynthesizer::onPlaybackStarted(SourceId id, const MediaPlayerState&) {
    ACSDK_DEBUG9(LX("onPlaybackStarted").d("callbackSourceId", id));
    ACSDK_METRIC_IDS(TAG, "SpeechStarted", "", "", Metrics::Location::SPEECH_SYNTHESIZER_RECEIVE);

    m_executor.submit([this, id] {
        if (id != m_mediaSourceId) {
            ACSDK_ERROR(LX("queueingExecutePlaybackStartedFailed")
                            .d("reason", "mismatchSourceId")
                            .d("callbackSourceId", id)
                            .d("sourceId", m_mediaSourceId));
            executePlaybackError(ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "executePlaybackStartedFailed");
        } else {
            submitMetric(MetricEventBuilder{}
                             .setActivityName(SPEECH_SYNTHESIZER_METRIC_PREFIX + TTS_STARTED)
                             .addDataPoint(DataPointCounterBuilder{}.setName(TTS_STARTED).increment(1).build())
                             .addDataPoint(DataPointStringBuilder{}
                                               .setName(DIALOG_REQUEST_ID_KEY)
                                               .setValue(m_currentInfo->directive->getDialogRequestId())
                                               .build()));
            executePlaybackStarted();
        }
    });
}

void SpeechSynthesizer::onPlaybackFinished(SourceId id, const MediaPlayerState&) {
    ACSDK_DEBUG9(LX("onPlaybackFinished").d("callbackSourceId", id));
    ACSDK_METRIC_IDS(TAG, "SpeechFinished", "", "", Metrics::Location::SPEECH_SYNTHESIZER_RECEIVE);

    m_executor.submit([this, id] {
        if (id != m_mediaSourceId) {
            ACSDK_ERROR(LX("queueingExecutePlaybackFinishedFailed")
                            .d("reason", "mismatchSourceId")
                            .d("callbackSourceId", id)
                            .d("sourceId", m_mediaSourceId));
            executePlaybackError(ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "executePlaybackFinishedFailed");
        } else {
            submitMetric(MetricEventBuilder{}
                             .setActivityName(SPEECH_SYNTHESIZER_METRIC_PREFIX + TTS_FINISHED)
                             .addDataPoint(DataPointCounterBuilder{}.setName(TTS_FINISHED).increment(1).build())
                             .addDataPoint(DataPointStringBuilder{}
                                               .setName(DIALOG_REQUEST_ID_KEY)
                                               .setValue(m_currentInfo->directive->getDialogRequestId())
                                               .build()));
            executePlaybackFinished();
        }
    });
}

void SpeechSynthesizer::onPlaybackError(
    SourceId id,
    const avsCommon::utils::mediaPlayer::ErrorType& type,
    std::string error,
    const MediaPlayerState&) {
    ACSDK_DEBUG9(LX("onPlaybackError").d("callbackSourceId", id));
    m_executor.submit([this, type, error]() { executePlaybackError(type, error); });
}

void SpeechSynthesizer::onPlaybackStopped(SourceId id, const MediaPlayerState&) {
    ACSDK_DEBUG9(LX("onPlaybackStopped").d("callbackSourceId", id));

    // MediaPlayer is for some reason stopping the playback of the speech.  Call setFailed if isSetFailedCalled flag is
    // not set yet.
    m_executor.submit([this, id]() {
        if (m_currentInfo && m_mediaSourceId == id) {
            m_currentInfo->sendCompletedMessage = false;
            if (m_currentInfo->result && !m_currentInfo->isSetFailedCalled) {
                m_currentInfo->result->setFailed("Stopped due to MediaPlayer stopping.");
                m_currentInfo->isSetFailedCalled = true;
            }
            executePlaybackFinished();
        } else {
            executePlaybackError(ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "UnexpectedId");
        }
    });
}

void SpeechSynthesizer::onBufferUnderrun(SourceId id, const MediaPlayerState&) {
    ACSDK_WARN(LX("onBufferUnderrun").d("callbackSourceId", id));
    submitMetric(MetricEventBuilder{}
                     .setActivityName(SPEECH_SYNTHESIZER_METRIC_PREFIX + BUFFER_UNDERRUN)
                     .addDataPoint(DataPointCounterBuilder{}.setName(BUFFER_UNDERRUN).increment(1).build()));
}

SpeechSynthesizer::SpeakDirectiveInfo::SpeakDirectiveInfo(std::shared_ptr<DirectiveInfo> directiveInfo) :
        directive{directiveInfo->directive},
        result{directiveInfo->result},
        sendPlaybackStartedMessage{false},
        sendPlaybackFinishedMessage{false},
        sendCompletedMessage{false},
        isSetFailedCalled{false},
        isPlaybackInitiated{false},
        isHandled{false},
        playBehavior{PlayBehavior::REPLACE_ALL} {
}

void SpeechSynthesizer::SpeakDirectiveInfo::clear() {
    attachmentReader.reset();
    sendPlaybackStartedMessage = false;
    sendPlaybackFinishedMessage = false;
    sendCompletedMessage = false;
    isSetFailedCalled = false;
    isPlaybackInitiated = false;
}

SpeechSynthesizer::SpeechSynthesizer(
    std::shared_ptr<MediaPlayerInterface> mediaPlayer,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<FocusManagerInterface> focusManager,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<MetricRecorderInterface> metricRecorder,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<captions::CaptionManagerInterface> captionManager,
    std::shared_ptr<PowerResourceManagerInterface> powerResourceManager) :
        CapabilityAgent{NAMESPACE, exceptionSender},
        RequiresShutdown{"SpeechSynthesizer"},
        m_mediaSourceId{MediaPlayerInterface::ERROR},
        m_offsetInMilliseconds{0},
        m_speechPlayer{mediaPlayer},
        m_metricRecorder{metricRecorder},
        m_messageSender{messageSender},
        m_focusManager{focusManager},
        m_contextManager{contextManager},
        m_captionManager{captionManager},
        m_currentState{SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED},
        m_desiredState{SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED},
        m_currentFocus{FocusState::NONE},
        m_isShuttingDown{false},
        m_initialDialogUXStateReceived{false},
        m_powerResourceManager{powerResourceManager} {
    m_capabilityConfigurations.insert(getSpeechSynthesizerCapabilityConfiguration());
}

std::shared_ptr<CapabilityConfiguration> getSpeechSynthesizerCapabilityConfiguration() {
    std::unordered_map<std::string, std::string> configMap;
    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, SPEECHSYNTHESIZER_CAPABILITY_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, SPEECHSYNTHESIZER_CAPABILITY_INTERFACE_NAME});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, SPEECHSYNTHESIZER_CAPABILITY_INTERFACE_VERSION});

    return std::make_shared<CapabilityConfiguration>(configMap);
}

void SpeechSynthesizer::doShutdown() {
    ACSDK_DEBUG9(LX("doShutdown"));
    {
        std::lock_guard<std::mutex> lock(m_speakInfoQueueMutex);
        m_isShuttingDown = true;
    }
    m_contextManager->removeStateProvider(CONTEXT_MANAGER_SPEECH_STATE);
    m_executor.shutdown();  // Wait for any ongoing job and avoid new jobs being enqueued.
    m_speechPlayer->removeObserver(shared_from_this());
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING == m_currentState) {
            m_desiredState = SpeechSynthesizerObserverInterface::SpeechSynthesizerState::INTERRUPTED;
            if (m_currentInfo) {
                m_currentInfo->sendPlaybackFinishedMessage = false;
            }
            lock.unlock();
            stopPlaying();
            releaseForegroundFocus();
            lock.lock();
            m_currentState = SpeechSynthesizerObserverInterface::SpeechSynthesizerState::INTERRUPTED;
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_speakInfoQueueMutex);
        if (m_currentInfo) {
            // Add to the queue so it is canceled.
            m_speakInfoQueue.push_front(m_currentInfo);
        }
        for (auto& info : m_speakInfoQueue) {
            if (info->result && !info->isSetFailedCalled) {
                info->result->setFailed("SpeechSynthesizerShuttingDown");
            }
            removeSpeakDirectiveInfo(info->directive->getMessageId());
            removeDirective(info->directive->getMessageId());
        }
    }

    m_speechPlayer.reset();
    m_waitOnStateChange.notify_one();
    m_messageSender.reset();
    m_focusManager.reset();
    m_contextManager.reset();
    m_observers.clear();
}

void SpeechSynthesizer::init() {
    m_speechPlayer->addObserver(shared_from_this());
    m_contextManager->setStateProvider(CONTEXT_MANAGER_SPEECH_STATE, shared_from_this());
}

void SpeechSynthesizer::executeHandleImmediately(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG(LX("executeHandleImmediately").d("messageId", info->directive->getMessageId()));
    auto speakInfo = validateInfo("executeHandleImmediately", info, false);
    if (!speakInfo) {
        ACSDK_ERROR(LX("executeHandleImmediatelyFailed").d("reason", "invalidDirective"));
        return;
    }
    executePreHandleAfterValidation(speakInfo);
    executeHandleAfterValidation(speakInfo);
}

void SpeechSynthesizer::executePreHandleAfterValidation(std::shared_ptr<SpeakDirectiveInfo> speakInfo) {
    if (speakInfo->directive->getName() != SPEAK.name) {
        const std::string message("unexpectedDirective " + speakInfo->directive->getName());
        ACSDK_ERROR(LX("executePreHandleFailed")
                        .d("reason", "unexpectedDirective")
                        .d("directiveName", speakInfo->directive->getName()));
        sendExceptionEncounteredAndReportFailed(
            speakInfo, avsCommon::avs::ExceptionErrorType::UNSUPPORTED_OPERATION, message);
        return;
    }

    Document payload;
    if (payload.Parse(speakInfo->directive->getPayload()).HasParseError()) {
        const std::string message("unableToParsePayload" + speakInfo->directive->getMessageId());
        ACSDK_ERROR(
            LX("executePreHandleFailed").d("reason", message).d("messageId", speakInfo->directive->getMessageId()));
        sendExceptionEncounteredAndReportFailed(
            speakInfo, avsCommon::avs::ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED, message);
        return;
    }

    Value::ConstMemberIterator it = payload.FindMember(KEY_TOKEN);
    if (payload.MemberEnd() == it) {
        sendExceptionEncounteredAndReportMissingProperty(speakInfo, KEY_TOKEN);
        return;
    } else if (!(it->value.IsString())) {
        sendExceptionEncounteredAndReportUnexpectedPropertyType(speakInfo, KEY_TOKEN);
        return;
    }
    speakInfo->token = it->value.GetString();

    it = payload.FindMember(KEY_FORMAT);
    if (payload.MemberEnd() == it) {
        sendExceptionEncounteredAndReportMissingProperty(speakInfo, KEY_FORMAT);
        return;
    } else if (!(it->value.IsString())) {
        sendExceptionEncounteredAndReportUnexpectedPropertyType(speakInfo, KEY_FORMAT);
        return;
    }

    std::string format = it->value.GetString();
    if (format != FORMAT) {
        const std::string message("unknownFormat " + speakInfo->directive->getMessageId() + " format " + format);
        ACSDK_ERROR(LX("executePreHandleFailed")
                        .d("reason", "unknownFormat")
                        .d("messageId", speakInfo->directive->getMessageId())
                        .d("format", format));
        sendExceptionEncounteredAndReportFailed(
            speakInfo, avsCommon::avs::ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED, message);
    }

    it = payload.FindMember(KEY_URL);
    if (payload.MemberEnd() == it) {
        sendExceptionEncounteredAndReportMissingProperty(speakInfo, KEY_URL);
        return;
    } else if (!(it->value.IsString())) {
        sendExceptionEncounteredAndReportUnexpectedPropertyType(speakInfo, KEY_URL);
        return;
    }

    std::string urlValue = it->value.GetString();
    auto contentIdPosition = urlValue.find(CID_PREFIX);
    if (contentIdPosition != 0) {
        const std::string message("expectedCIDUrlPrefixNotFound");
        ACSDK_ERROR(LX("executePreHandleFailed").d("reason", message).sensitive("url", urlValue));
        sendExceptionEncounteredAndReportFailed(
            speakInfo, avsCommon::avs::ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED, message);
        return;
    }
    std::string contentId = urlValue.substr(contentIdPosition + CID_PREFIX.length());
    speakInfo->attachmentReader = speakInfo->directive->getAttachmentReader(contentId, sds::ReaderPolicy::NONBLOCKING);
    if (!speakInfo->attachmentReader) {
        const std::string message("getAttachmentReaderFailed");
        ACSDK_ERROR(LX("executePreHandleFailed").d("reason", message));
        sendExceptionEncounteredAndReportFailed(speakInfo, avsCommon::avs::ExceptionErrorType::INTERNAL_ERROR, message);
    }

    it = payload.FindMember(KEY_PLAY_BEHAVIOR);
    if (payload.MemberEnd() != it) {
        if (!(it->value.IsString())) {
            sendExceptionEncounteredAndReportUnexpectedPropertyType(speakInfo, KEY_PLAY_BEHAVIOR);
            return;
        }
        std::string behavior = it->value.GetString();
        if (!stringToPlayBehavior(behavior, &(speakInfo->playBehavior))) {
            const std::string message = "failedToParsePlayBehavior";
            ACSDK_ERROR(LX("executePreHandleFailed").d("reason", message).d("behavior", behavior));
            sendExceptionEncounteredAndReportFailed(
                speakInfo, avsCommon::avs::ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED, message);
            return;
        }
    } else {
        speakInfo->playBehavior = PlayBehavior::REPLACE_ALL;
    }

    if (!m_captionManager) {
        ACSDK_DEBUG5(LX("captionsNotParsed").d("reason", "captionManagerIsNull"));
    } else {
        auto captionIterator = payload.FindMember(KEY_CAPTION);
        if (payload.MemberEnd() != captionIterator) {
            if (captionIterator->value.IsObject()) {
                rapidjson::Value& captionsPayload = payload[KEY_CAPTION];

                auto captionFormat = captions::CaptionFormat::UNKNOWN;
                captionIterator = captionsPayload.FindMember(KEY_CAPTION_TYPE);
                if ((payload.MemberEnd() != captionIterator) && (captionIterator->value.IsString())) {
                    captionFormat = captions::avsStringToCaptionFormat(captionIterator->value.GetString());
                } else {
                    ACSDK_WARN(LX("captionParsingIncomplete").d("reason", "failedToParseField").d("field", "type"));
                }

                std::string captionContent;
                captionIterator = captionsPayload.FindMember(KEY_CAPTION_CONTENT);
                if ((payload.MemberEnd() != captionIterator) && (captionIterator->value.IsString())) {
                    captionContent = captionIterator->value.GetString();
                } else {
                    ACSDK_WARN(LX("captionParsingIncomplete").d("reason", "failedToParseField").d("field", "content"));
                }

                ACSDK_DEBUG3(LX("captionPayloadParsed").d("type", captionFormat));
                speakInfo->captionData = captions::CaptionData(captionFormat, captionContent);
            } else {
                ACSDK_WARN(LX("captionsNotParsed").d("reason", "keyNotAnObject"));
            }
        } else {
            ACSDK_DEBUG3(LX("captionsNotParsed").d("reason", "keyNotFoundInPayload"));
        }
    }

    // Populate audio analyzers state from directive to speak info
    auto analyzerIterator = payload.FindMember(KEY_ANALYZERS);
    if (payload.MemberEnd() != analyzerIterator) {
        const Value& analyzersPayload = payload[KEY_ANALYZERS];
        if (!analyzersPayload.IsArray()) {
            ACSDK_WARN(LX("audioAnalyzerParsingIncomplete").d("reason", "NotAnArray").d("field", "analyzers"));
        } else {
            std::vector<avsCommon::utils::audioAnalyzer::AudioAnalyzerState> analyzersData;
            for (auto itr = analyzersPayload.Begin(); itr != analyzersPayload.End(); ++itr) {
                const Value& analyzerStatePayload = *itr;
                auto nameIterator = analyzerStatePayload.FindMember(KEY_ANALYZERS_INTERFACE);
                auto stateIterator = analyzerStatePayload.FindMember(KEY_ANALYZERS_ENABLED);
                if (analyzerStatePayload.MemberEnd() != nameIterator &&
                    analyzerStatePayload.MemberEnd() != stateIterator) {
                    audioAnalyzer::AudioAnalyzerState state(
                        nameIterator->value.GetString(), stateIterator->value.GetString());
                    analyzersData.push_back(state);
                }
            }
            speakInfo->analyzersData = analyzersData;
        }
    }

    // If everything checks out, add the speakInfo to the map.
    if (!setSpeakDirectiveInfo(speakInfo->directive->getMessageId(), speakInfo)) {
        ACSDK_ERROR(LX("executePreHandleFailed")
                        .d("reason", "prehandleCalledTwiceOnSameDirective")
                        .d("messageId", speakInfo->directive->getMessageId()));
        return;
    }

    addToDirectiveQueue(speakInfo);
}

void SpeechSynthesizer::executeHandleAfterValidation(std::shared_ptr<SpeakDirectiveInfo> speakInfo) {
    speakInfo->isHandled = true;
    if (m_currentInfo) {
        ACSDK_DEBUG3(LX(__func__).d("result", "skip").d("reason", "cancellationInProgress"));
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_speakInfoQueueMutex);
        if (m_speakInfoQueue.empty() ||
            (speakInfo->directive->getMessageId() != m_speakInfoQueue.front()->directive->getMessageId())) {
            ACSDK_ERROR(LX("executeHandleFailed")
                            .d("reason", "unexpectedDirective")
                            .d("messageId", speakInfo->directive->getMessageId())
                            .d("expected",
                               m_speakInfoQueue.empty() ? std::string{"empty"}
                                                        : m_speakInfoQueue.front()->directive->getMessageId()));
            sendExceptionEncounteredAndReportFailed(
                speakInfo,
                avsCommon::avs::ExceptionErrorType::INTERNAL_ERROR,
                "UnexpectedId " + speakInfo->directive->getMessageId());
            return;
        }
        m_speakInfoQueue.pop_front();
    }

    m_currentInfo = speakInfo;
    setDesiredState(SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING);

    auto activity = FocusManagerInterface::Activity::create(
        NAMESPACE, shared_from_this(), std::chrono::milliseconds::zero(), avsCommon::avs::ContentType::MIXABLE);
    if (!m_focusManager->acquireChannel(CHANNEL_NAME, activity)) {
        static const std::string message = std::string("Could not acquire ") + CHANNEL_NAME + " for " + NAMESPACE;
        ACSDK_ERROR(LX("executeHandleFailed")
                        .d("reason", "CouldNotAcquireChannel")
                        .d("messageId", m_currentInfo->directive->getMessageId()));
        sendExceptionEncounteredAndReportFailed(speakInfo, avsCommon::avs::ExceptionErrorType::INTERNAL_ERROR, message);
        // Undo desired state.
        std::lock_guard<std::mutex> lock{m_mutex};
        m_desiredState = m_currentState;
    }
}

void SpeechSynthesizer::executePreHandle(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG(LX("executePreHandle").d("messageId", info->directive->getMessageId()));
    auto speakInfo = validateInfo("executePreHandle", info);
    if (!speakInfo) {
        ACSDK_ERROR(LX("executePreHandleFailed").d("reason", "invalidDirectiveInfo"));
        return;
    }
    executePreHandleAfterValidation(speakInfo);
}

void SpeechSynthesizer::executeHandle(std::shared_ptr<DirectiveInfo> info) {
    auto speakInfo = validateInfo("executeHandle", info);
    if (!speakInfo) {
        ACSDK_ERROR(LX("executeHandleFailed").d("reason", "invalidDirectiveInfo"));
        return;
    }
    ACSDK_DEBUG(LX("executeHandle").d("messageId", info->directive->getMessageId()));
    executeHandleAfterValidation(speakInfo);
}

void SpeechSynthesizer::executeCancel(std::shared_ptr<DirectiveInfo> info) {
    auto speakInfo = validateInfo("executeCancel", info);
    if (!speakInfo) {
        ACSDK_ERROR(LX("executeCancel").d("reason", "invalidDirectiveInfo"));
        return;
    }
    executeCancel(speakInfo);
}

void SpeechSynthesizer::executeCancel(std::shared_ptr<SpeakDirectiveInfo> speakInfo) {
    if (!speakInfo) {
        ACSDK_ERROR(LX("executeCancelFailed").d("reason", "invalidDirectiveInfo"));
        return;
    }
    ACSDK_DEBUG(LX(__func__).d("messageId", speakInfo->directive->getMessageId()));
    if (!m_currentInfo || (speakInfo->directive->getMessageId() != m_currentInfo->directive->getMessageId())) {
        ACSDK_DEBUG3(LX(__func__).d("result", "cancelPendingDirective"));
        speakInfo->clear();
        removeSpeakDirectiveInfo(speakInfo->directive->getMessageId());
        {
            std::lock_guard<std::mutex> lock(m_speakInfoQueueMutex);
            for (auto it = m_speakInfoQueue.begin(); it != m_speakInfoQueue.end(); it++) {
                if (speakInfo->directive->getMessageId() == it->get()->directive->getMessageId()) {
                    it = m_speakInfoQueue.erase(it);
                    break;
                }
            }
        }
        removeDirective(speakInfo->directive->getMessageId());
        return;
    }

    ACSDK_DEBUG3(LX(__func__)
                     .d("result", "cancelCurrentDirective")
                     .d("state", m_currentState)
                     .d("desiredState", m_desiredState)
                     .d("isPlaybackInitiated", m_currentInfo->isPlaybackInitiated));
    m_currentInfo->sendPlaybackStartedMessage = false;
    m_currentInfo->sendCompletedMessage = false;
    if (m_currentInfo->isPlaybackInitiated) {
        stopPlaying();
    } else {
        // Playback has not started yet. Cancel desired playing state.
        {
            std::lock_guard<std::mutex> lock{m_mutex};
            if (SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED == m_currentState) {
                m_desiredState = SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED;
            } else {
                m_desiredState = SpeechSynthesizerObserverInterface::SpeechSynthesizerState::INTERRUPTED;
            }
        }
        executePlaybackFinished();
        if (!m_currentInfo) {
            // There is no next item to play.
            releaseForegroundFocus();
        }
    }
}

void SpeechSynthesizer::executeStateChange(SpeechSynthesizerObserverInterface::SpeechSynthesizerState newState) {
    ACSDK_DEBUG(LX("executeStateChange").d("newState", newState));
    switch (newState) {
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING:
            if (m_currentInfo) {
                m_currentInfo->sendPlaybackStartedMessage = true;
                m_currentInfo->sendPlaybackFinishedMessage = true;
                m_currentInfo->sendCompletedMessage = true;
                m_currentInfo->isPlaybackInitiated = true;
                startPlaying();
            }
            break;
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::INTERRUPTED:
            // This happens when focus state is changed to BACKGROUND or NONE, requiring the @c SpeechSynthesizer to
            // trigger termination of playing the audio playback.
            if (m_currentInfo) {
                m_currentInfo->sendCompletedMessage = false;
                if (m_currentInfo->result) {
                    m_currentInfo->result->setFailed("Stopped due to SpeechSynthesizer going into INTERRUPTED state.");
                    m_currentInfo->isSetFailedCalled = true;
                }

                if (m_currentInfo->isPlaybackInitiated) {
                    stopPlaying();
                } else {
                    setDesiredState(SpeechSynthesizerObserverInterface::SpeechSynthesizerState::INTERRUPTED);
                    executePlaybackFinished();
                }
            }
            break;
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED:
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::GAINING_FOCUS:
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::LOSING_FOCUS:
            // This should not happen but there should be no user impact.
            ACSDK_WARN(LX(__func__).d("unexpectedStateChange", newState));
            break;
    }
}

void SpeechSynthesizer::executeProvideStateLocked(const unsigned int& stateRequestToken) {
    ACSDK_DEBUG(LX("executeProvideState").d("stateRequestToken", stateRequestToken).d("state", m_currentState));
    StateRefreshPolicy refreshPolicy = StateRefreshPolicy::NEVER;
    std::string speakDirectiveToken;
    if (m_currentInfo) {
        speakDirectiveToken = m_currentInfo->token;
    }

    if (SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING == m_currentState) {
        m_offsetInMilliseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(m_speechPlayer->getOffset(m_mediaSourceId)).count();
        refreshPolicy = StateRefreshPolicy::ALWAYS;
    }

    auto jsonState = buildState(speakDirectiveToken, m_offsetInMilliseconds);
    if (jsonState.empty()) {
        ACSDK_ERROR(LX("executeProvideStateFailed").d("reason", "buildStateFailed").d("token", speakDirectiveToken));
        return;
    }
    auto result = m_contextManager->setState(CONTEXT_MANAGER_SPEECH_STATE, jsonState, refreshPolicy, stateRequestToken);
    if (result != SetStateResult::SUCCESS) {
        ACSDK_ERROR(LX("executeProvideStateFailed")
                        .d("reason", "contextManagerSetStateFailed")
                        .d("token", speakDirectiveToken));
    }
}

void SpeechSynthesizer::executePlaybackStarted() {
    ACSDK_DEBUG(LX("executePlaybackStarted"));
    if (!m_currentInfo) {
        ACSDK_ERROR(LX("executePlaybackStartedIgnored").d("reason", "nullptrDirectiveInfo"));
        return;
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        setCurrentStateLocked(SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING);
    }
    setDesiredState(SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);
    m_waitOnStateChange.notify_one();
    if (m_currentInfo->sendPlaybackStartedMessage) {
        sendEvent(SPEECH_STARTED_EVENT_NAME, buildPayload(m_currentInfo->token));
    }
}

void SpeechSynthesizer::executePlaybackFinished() {
    ACSDK_DEBUG(LX("executePlaybackFinished"));
    if (!m_currentInfo) {
        ACSDK_ERROR(LX("executePlaybackFinishedIgnored").d("reason", "nullptrDirectiveInfo"));
        return;
    }

    auto newState = SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED;
    auto eventName = SPEECH_FINISHED_EVENT_NAME;
    std::string payload;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (SpeechSynthesizerObserverInterface::SpeechSynthesizerState::INTERRUPTED == m_desiredState) {
            newState = SpeechSynthesizerObserverInterface::SpeechSynthesizerState::INTERRUPTED;
            eventName = SPEECH_INTERRUPTED_EVENT_NAME;
            payload = buildPayload(m_currentInfo->token, m_offsetInMilliseconds);
        } else {
            payload = buildPayload(m_currentInfo->token);
            m_offsetInMilliseconds = 0;
        }
        setCurrentStateLocked(newState);
    }

    ACSDK_DEBUG3(LX(__func__).d("reason", eventName));
    m_waitOnStateChange.notify_one();
    if (m_currentInfo->sendPlaybackFinishedMessage) {
        sendEvent(eventName, payload);
    }
    if (m_currentInfo->sendCompletedMessage) {
        setHandlingCompleted();
    }
    resetCurrentInfo();
    {
        std::unique_lock<std::mutex> lock(m_speakInfoQueueMutex);
        if (!m_isShuttingDown && !m_speakInfoQueue.empty()) {
            if (m_speakInfoQueue.front()->isHandled) {
                auto front = m_speakInfoQueue.front();
                lock.unlock();
                executeHandleAfterValidation(front);
            }
        }
    }
    resetMediaSourceId();
}

void SpeechSynthesizer::executePlaybackError(const avsCommon::utils::mediaPlayer::ErrorType& type, std::string error) {
    ACSDK_DEBUG(LX("executePlaybackError").d("type", type).d("error", error));
    if (!m_currentInfo) {
        return;
    }
    setDesiredState(SpeechSynthesizerObserverInterface::SpeechSynthesizerState::INTERRUPTED);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        setCurrentStateLocked(SpeechSynthesizerObserverInterface::SpeechSynthesizerState::INTERRUPTED);
    }
    m_waitOnStateChange.notify_one();
    releaseForegroundFocus();
    {
        std::unique_lock<std::mutex> lock(m_speakInfoQueueMutex);
        m_speakInfoQueue.push_front(m_currentInfo);
        while (!m_speakInfoQueue.empty()) {
            auto speakInfo = m_speakInfoQueue.front();
            m_speakInfoQueue.pop_front();
            lock.unlock();
            sendExceptionEncounteredAndReportFailed(
                speakInfo, avsCommon::avs::ExceptionErrorType::INTERNAL_ERROR, error);
            lock.lock();
        }
    }
    resetCurrentInfo();
    resetMediaSourceId();
}

std::string SpeechSynthesizer::buildState(std::string& token, int64_t offsetInMilliseconds) const {
    Document state(kObjectType);
    Document::AllocatorType& alloc = state.GetAllocator();
    state.AddMember(KEY_TOKEN, token, alloc);
    state.AddMember(KEY_OFFSET_IN_MILLISECONDS, offsetInMilliseconds, alloc);

    switch (m_currentState) {
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING:
            state.AddMember(KEY_PLAYER_ACTIVITY, PLAYER_STATE_PLAYING, alloc);
            break;
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED:
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::GAINING_FOCUS:
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::LOSING_FOCUS:
            state.AddMember(KEY_PLAYER_ACTIVITY, PLAYER_STATE_FINISHED, alloc);
            break;
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::INTERRUPTED:
            state.AddMember(KEY_PLAYER_ACTIVITY, PLAYER_STATE_INTERRUPTED, alloc);
            break;
    }

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    if (!state.Accept(writer)) {
        ACSDK_ERROR(LX("buildStateFailed").d("reason", "writerRefusedJsonObject"));
        return "";
    }

    return buffer.GetString();
}

std::string SpeechSynthesizer::buildPayload(std::string& token, int64_t offsetInMilliseconds) {
    json::JsonGenerator generator;
    generator.addMember(KEY_TOKEN, token);
    generator.addMember(KEY_OFFSET_IN_MILLISECONDS, offsetInMilliseconds);
    return generator.toString();
}

std::string SpeechSynthesizer::buildPayload(std::string& token) {
    json::JsonGenerator generator;
    generator.addMember(KEY_TOKEN, token);
    return generator.toString();
}

void SpeechSynthesizer::startPlaying() {
    ACSDK_DEBUG9(LX("startPlaying"));
    std::shared_ptr<AttachmentReader> attachmentReader = std::move(m_currentInfo->attachmentReader);
    m_mediaSourceId = m_speechPlayer->setSource(std::move(attachmentReader));
    if (m_captionManager && m_currentInfo->captionData.isValid()) {
        m_captionManager->onCaption(m_mediaSourceId, m_currentInfo->captionData);
    }
    if (MediaPlayerInterface::ERROR == m_mediaSourceId) {
        ACSDK_ERROR(LX("startPlayingFailed").d("reason", "setSourceFailed"));
        executePlaybackError(ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "playFailed");
    } else if (!m_speechPlayer->play(m_mediaSourceId)) {
        executePlaybackError(ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "playFailed");
    }
}

void SpeechSynthesizer::stopPlaying() {
    ACSDK_DEBUG9(LX("stopPlaying"));
    bool isAlreadyStopping = false;
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        isAlreadyStopping = SpeechSynthesizerObserverInterface::SpeechSynthesizerState::INTERRUPTED == m_desiredState;
    }
    if (MediaPlayerInterface::ERROR == m_mediaSourceId) {
        ACSDK_ERROR(LX("stopPlayingFailed").d("reason", "invalidMediaSourceId").d("mediaSourceId", m_mediaSourceId));
    } else if (isAlreadyStopping) {
        ACSDK_DEBUG9(LX("stopPlayingIgnored").d("reason", "isAlreadyStopping"));
    } else {
        m_offsetInMilliseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(m_speechPlayer->getOffset(m_mediaSourceId)).count();
        if (!m_speechPlayer->stop(m_mediaSourceId)) {
            executePlaybackError(ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "stopFailed");
        } else {
            // Execution of stop is successful.
            setDesiredState(SpeechSynthesizerObserverInterface::SpeechSynthesizerState::INTERRUPTED);
        }
    }
}

void SpeechSynthesizer::setCurrentStateLocked(SpeechSynthesizerObserverInterface::SpeechSynthesizerState newState) {
    ACSDK_DEBUG9(LX("setCurrentStateLocked").d("state", newState));
    if (m_currentState == newState) {
        return;
    }

    m_currentState = newState;
    managePowerResource(m_currentState);
    switch (newState) {
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING:
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED:
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::INTERRUPTED:
            executeProvideStateLocked(0);
            break;
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::LOSING_FOCUS:
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::GAINING_FOCUS:
            break;
    }

    std::vector<audioAnalyzer::AudioAnalyzerState> analyzersData;
    if (m_currentInfo) {
        analyzersData = m_currentInfo->analyzersData;
    }

    for (auto observer : m_observers) {
        observer->onStateChanged(
            m_currentState, m_mediaSourceId, m_speechPlayer->getMediaPlayerState(m_mediaSourceId), analyzersData);
    }
}

void SpeechSynthesizer::setDesiredState(SpeechSynthesizerObserverInterface::SpeechSynthesizerState desiredState) {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_desiredState = desiredState;
}

void SpeechSynthesizer::resetCurrentInfo(std::shared_ptr<SpeakDirectiveInfo> speakInfo) {
    if (m_currentInfo != speakInfo) {
        if (m_currentInfo) {
            removeSpeakDirectiveInfo(m_currentInfo->directive->getMessageId());
            removeDirective(m_currentInfo->directive->getMessageId());
            m_currentInfo->clear();
        }
        m_currentInfo = speakInfo;
    }
}

void SpeechSynthesizer::setHandlingCompleted() {
    ACSDK_DEBUG9(LX("setHandlingCompleted"));
    if (m_currentInfo && m_currentInfo->result) {
        m_currentInfo->result->setCompleted();
    }
}

void SpeechSynthesizer::sendEvent(const std::string& eventName, const std::string& payload) const {
    if (payload.empty()) {
        ACSDK_ERROR(LX("sendEventFailed").d("event", eventName).d("token", m_currentInfo->token));
        return;
    }
    auto msgIdAndJsonEvent = buildJsonEventString(eventName, "", payload);

    auto request = std::make_shared<MessageRequest>(msgIdAndJsonEvent.second);
    m_messageSender->sendMessage(request);
}

void SpeechSynthesizer::sendExceptionEncounteredAndReportFailed(
    std::shared_ptr<SpeakDirectiveInfo> speakInfo,
    avsCommon::avs::ExceptionErrorType type,
    const std::string& message) {
    if (speakInfo) {
        if (speakInfo->directive) {
            m_exceptionEncounteredSender->sendExceptionEncountered(
                speakInfo->directive->getUnparsedDirective(), type, message);
            removeDirective(speakInfo->directive->getMessageId());
        } else {
            ACSDK_ERROR(LX("sendExceptionEncounteredAndReportFailed").d("reason", "speakInfoHasNoDirective"));
        }
        if (speakInfo->result) {
            speakInfo->result->setFailed(message);

        } else {
            ACSDK_ERROR(LX("sendExceptionEncounteredAndReportFailed").d("reason", "speakInfoHasNoResult"));
        }
        speakInfo->clear();
    } else {
        ACSDK_ERROR(LX("sendExceptionEncounteredAndReportFailed").d("reason", "speakInfoNotFound"));
    }
    std::unique_lock<std::mutex> lock(m_mutex);
    if (SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING == m_currentState ||
        SpeechSynthesizerObserverInterface::SpeechSynthesizerState::GAINING_FOCUS == m_currentState) {
        lock.unlock();
        // set flag to indicate setFailed has been called.
        if (speakInfo) {
            speakInfo->isSetFailedCalled = true;
        }
        stopPlaying();
    }
}

void SpeechSynthesizer::sendExceptionEncounteredAndReportMissingProperty(
    std::shared_ptr<SpeakDirectiveInfo> speakInfo,
    const std::string& key) {
    ACSDK_ERROR(LX("executePreHandleFailed").d("reason", "missingProperty").d("key", key));
    sendExceptionEncounteredAndReportFailed(
        speakInfo,
        avsCommon::avs::ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED,
        std::string("missing property: ") + key);
}

void SpeechSynthesizer::sendExceptionEncounteredAndReportUnexpectedPropertyType(
    std::shared_ptr<SpeakDirectiveInfo> speakInfo,
    const std::string& key) {
    ACSDK_ERROR(LX("executePreHandleFailed").d("reason", "invalidProperty").d("key", key));
    sendExceptionEncounteredAndReportFailed(
        speakInfo,
        avsCommon::avs::ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED,
        std::string("invalid property: ") + key);
}

void SpeechSynthesizer::releaseForegroundFocus() {
    ACSDK_DEBUG9(LX("releaseForegroundFocus"));
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_currentFocus = FocusState::NONE;
    }
    m_focusManager->releaseChannel(CHANNEL_NAME, shared_from_this());
}

std::shared_ptr<SpeechSynthesizer::SpeakDirectiveInfo> SpeechSynthesizer::validateInfo(
    const std::string& caller,
    std::shared_ptr<DirectiveInfo> info,
    bool checkResult) {
    if (!info) {
        ACSDK_ERROR(LX(caller + "Failed").d("reason", "nullptrInfo"));
        return nullptr;
    }
    if (!info->directive) {
        ACSDK_ERROR(LX(caller + "Failed").d("reason", "nullptrDirective"));
        return nullptr;
    }
    if (checkResult && !info->result) {
        ACSDK_ERROR(LX(caller + "Failed").d("reason", "nullptrResult"));
        return nullptr;
    }

    auto speakDirInfo = getSpeakDirectiveInfo(info->directive->getMessageId());
    if (speakDirInfo) {
        return speakDirInfo;
    }

    speakDirInfo = std::make_shared<SpeakDirectiveInfo>(info);

    return speakDirInfo;
}

std::shared_ptr<SpeechSynthesizer::SpeakDirectiveInfo> SpeechSynthesizer::getSpeakDirectiveInfo(
    const std::string& messageId) {
    std::lock_guard<std::mutex> lock(m_speakDirectiveInfoMutex);
    auto it = m_speakDirectiveInfoMap.find(messageId);
    if (it != m_speakDirectiveInfoMap.end()) {
        return it->second;
    }
    return nullptr;
}

bool SpeechSynthesizer::setSpeakDirectiveInfo(
    const std::string& messageId,
    std::shared_ptr<SpeechSynthesizer::SpeakDirectiveInfo> speakDirectiveInfo) {
    std::lock_guard<std::mutex> lock(m_speakDirectiveInfoMutex);
    auto it = m_speakDirectiveInfoMap.find(messageId);
    if (it != m_speakDirectiveInfoMap.end()) {
        return false;
    }
    m_speakDirectiveInfoMap[messageId] = speakDirectiveInfo;
    return true;
}

void SpeechSynthesizer::removeSpeakDirectiveInfo(const std::string& messageId) {
    std::lock_guard<std::mutex> lock(m_speakDirectiveInfoMutex);
    m_speakDirectiveInfoMap.erase(messageId);
}

void SpeechSynthesizer::clearPendingDirectivesLocked() {
    while (!m_speakInfoQueue.empty()) {
        auto info = m_speakInfoQueue.front();
        if (info->result) {
            info->result->setCompleted();
        }
        removeSpeakDirectiveInfo(info->directive->getMessageId());
        removeDirective(info->directive->getMessageId());
        info->clear();
        m_speakInfoQueue.pop_front();
    }
}

void SpeechSynthesizer::addToDirectiveQueue(std::shared_ptr<SpeakDirectiveInfo> speakInfo) {
    std::unique_lock<std::mutex> lock(m_speakInfoQueueMutex);
    ACSDK_DEBUG5(LX(__func__).d("queueSize", m_speakInfoQueue.size()).d("playBehavior", speakInfo->playBehavior));
    switch (speakInfo->playBehavior) {
        case PlayBehavior::ENQUEUE:
            m_speakInfoQueue.push_back(speakInfo);
            break;
        case PlayBehavior::REPLACE_ENQUEUED:
            clearPendingDirectivesLocked();
            m_speakInfoQueue.push_back(speakInfo);
            break;
        case PlayBehavior::REPLACE_ALL:
            clearPendingDirectivesLocked();
            m_speakInfoQueue.push_back(speakInfo);
            if (m_currentInfo) {
                lock.unlock();
                executeCancel(m_currentInfo);
                lock.lock();
            }
            break;
    }
}

void SpeechSynthesizer::resetMediaSourceId() {
    m_mediaSourceId = MediaPlayerInterface::ERROR;
}

void SpeechSynthesizer::onDialogUXStateChanged(DialogUXStateObserverInterface::DialogUXState newState) {
    m_executor.submit([this, newState]() { executeOnDialogUXStateChanged(newState); });
}

void SpeechSynthesizer::executeOnDialogUXStateChanged(DialogUXStateObserverInterface::DialogUXState newState) {
    if (!m_initialDialogUXStateReceived) {
        // The initial dialog UX state change call comes from simply registering as an observer; it is not a deliberate
        // change to the dialog state which should interrupt a recognize event.
        m_initialDialogUXStateReceived = true;
        return;
    }
    if (newState != DialogUXStateObserverInterface::DialogUXState::IDLE) {
        return;
    }
    if (m_currentFocus != avsCommon::avs::FocusState::NONE &&
        m_currentState != SpeechSynthesizerObserverInterface::SpeechSynthesizerState::GAINING_FOCUS) {
        m_focusManager->releaseChannel(CHANNEL_NAME, shared_from_this());
        m_currentFocus = avsCommon::avs::FocusState::NONE;
    }
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> SpeechSynthesizer::
    getCapabilityConfigurations() {
    return m_capabilityConfigurations;
}

void SpeechSynthesizer::submitMetric(MetricEventBuilder& metricEventBuilder) {
    if (!m_metricRecorder) {
        return;
    }

    if (m_currentInfo) {
        auto metricEvent = metricEventBuilder
                               .addDataPoint(DataPointStringBuilder{}
                                                 .setName("HTTP2_STREAM")
                                                 .setValue(m_currentInfo->directive->getAttachmentContextId())
                                                 .build())
                               .addDataPoint(DataPointStringBuilder{}
                                                 .setName("DIRECTIVE_MESSAGE_ID")
                                                 .setValue(m_currentInfo->directive->getMessageId())
                                                 .build())
                               .build();

        if (metricEvent == nullptr) {
            ACSDK_ERROR(LX("Error creating metric."));
            return;
        }
        recordMetric(m_metricRecorder, metricEvent);
    }
}

void SpeechSynthesizer::managePowerResource(SpeechSynthesizerObserverInterface::SpeechSynthesizerState newState) {
    if (!m_powerResourceManager) {
        return;
    }

    ACSDK_DEBUG5(LX(__func__).d("state", newState));
    switch (newState) {
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING:
            m_powerResourceManager->acquirePowerResource(POWER_RESOURCE_COMPONENT_NAME);
            break;
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED:
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::INTERRUPTED:
            m_powerResourceManager->releasePowerResource(POWER_RESOURCE_COMPONENT_NAME);
            break;
        default:
            // no-op for focus change
            break;
    }
}

}  // namespace speechSynthesizer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
