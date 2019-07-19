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

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Metrics.h>

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
using namespace rapidjson;

/// SpeechSynthesizer capability constants
/// SpeechSynthesizer interface type
static const std::string SPEECHSYNTHESIZER_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";
/// SpeechSynthesizer interface name
static const std::string SPEECHSYNTHESIZER_CAPABILITY_INTERFACE_NAME = "SpeechSynthesizer";
/// SpeechSynthesizer interface version
static const std::string SPEECHSYNTHESIZER_CAPABILITY_INTERFACE_VERSION = "1.0";

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

/// The key used to look up the "url" property in the directive payload.
static const char KEY_URL[] = "url";

/// The key for the "token" property in the directive payload string and "SpeechState" string.
static const char KEY_TOKEN[] = "token";

/// The key used to look the "format" property in the directive payload string.
static const char KEY_FORMAT[] = "format";

/// The expected format value in the directive payload.
static const std::string FORMAT{"AUDIO_MPEG"};

/// Prefix for content ID prefix in the url property of the directive payload.
static const std::string CID_PREFIX{"cid:"};

/// The key for the "offsetInMilliseconds" property in the event context.
static const char KEY_OFFSET_IN_MILLISECONDS[] = "offsetInMilliseconds";

/// The "playerActivity" key used to build "SpeechState" string.
static const char KEY_PLAYER_ACTIVITY[] = "playerActivity";

/// The player activity value used to build the "SpeechState" string.
static const char PLAYER_STATE_PLAYING[] = "PLAYING";

/// The player activity value used to build the "SpeechState" string.
static const char PLAYER_STATE_FINISHED[] = "FINISHED";

/// The duration to wait for a state change in @c onFocusChanged before failing.
static const std::chrono::seconds STATE_CHANGE_TIMEOUT{5};

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
    std::shared_ptr<avsCommon::avs::DialogUXStateAggregator> dialogUXStateAggregator) {
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
    auto speechSynthesizer = std::shared_ptr<SpeechSynthesizer>(
        new SpeechSynthesizer(mediaPlayer, messageSender, focusManager, contextManager, exceptionSender));
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
    ACSDK_DEBUG9(LX("preHandleDirective").d("messageId", info->directive->getMessageId()));
    m_executor.submit([this, info]() { executePreHandle(info); });
}

void SpeechSynthesizer::handleDirective(std::shared_ptr<DirectiveInfo> info) {
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

void SpeechSynthesizer::onFocusChanged(FocusState newFocus) {
    ACSDK_DEBUG(LX("onFocusChanged").d("newFocus", newFocus));

    std::unique_lock<std::mutex> lock(m_mutex);
    m_currentFocus = newFocus;
    setDesiredStateLocked(newFocus);
    if (m_currentState == m_desiredState) {
        return;
    }

    // Set intermediate state to avoid being considered idle
    switch (newFocus) {
        case FocusState::FOREGROUND:
            setCurrentStateLocked(SpeechSynthesizerObserverInterface::SpeechSynthesizerState::GAINING_FOCUS);
            break;
        case FocusState::BACKGROUND:
            setCurrentStateLocked(SpeechSynthesizerObserverInterface::SpeechSynthesizerState::LOSING_FOCUS);
            break;
        case FocusState::NONE:
            // We do not care of other focus states yet
            break;
    }

    auto messageId = (m_currentInfo && m_currentInfo->directive) ? m_currentInfo->directive->getMessageId() : "";
    m_executor.submit([this]() { executeStateChange(); });
    // Block until we achieve the desired state.
    if (m_waitOnStateChange.wait_for(
            lock, STATE_CHANGE_TIMEOUT, [this]() { return m_currentState == m_desiredState; })) {
        ACSDK_DEBUG9(LX("onFocusChangedSuccess"));
    } else {
        ACSDK_ERROR(LX("onFocusChangeFailed").d("reason", "stateChangeTimeout").d("messageId", messageId));
        if (m_currentInfo) {
            lock.unlock();
            sendExceptionEncounteredAndReportFailed(
                m_currentInfo, avsCommon::avs::ExceptionErrorType::INTERNAL_ERROR, "stateChangeTimeout");
        }
    }

    m_executor.submit([this, messageId]() {
        auto speakInfo = getSpeakDirectiveInfo(messageId);
        if (speakInfo && speakInfo->isDelayedCancel) {
            executeCancel(speakInfo);
        }
    });
}

void SpeechSynthesizer::provideState(
    const avsCommon::avs::NamespaceAndName& stateProviderName,
    const unsigned int stateRequestToken) {
    ACSDK_DEBUG9(LX("provideState").d("token", stateRequestToken));
    m_executor.submit([this, stateRequestToken]() {
        std::lock_guard<std::mutex> lock(m_mutex);
        executeProvideState(stateRequestToken);
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

void SpeechSynthesizer::onPlaybackStarted(SourceId id) {
    ACSDK_DEBUG9(LX("onPlaybackStarted").d("callbackSourceId", id));
    ACSDK_METRIC_IDS(TAG, "SpeechStarted", "", "", Metrics::Location::SPEECH_SYNTHESIZER_RECEIVE);
    if (id != m_mediaSourceId) {
        ACSDK_ERROR(LX("queueingExecutePlaybackStartedFailed")
                        .d("reason", "mismatchSourceId")
                        .d("callbackSourceId", id)
                        .d("sourceId", m_mediaSourceId));
        m_executor.submit([this] {
            executePlaybackError(ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "executePlaybackStartedFailed");
        });
    } else {
        m_executor.submit([this]() { executePlaybackStarted(); });
    }
}

void SpeechSynthesizer::onPlaybackFinished(SourceId id) {
    ACSDK_DEBUG9(LX("onPlaybackFinished").d("callbackSourceId", id));
    ACSDK_METRIC_IDS(TAG, "SpeechFinished", "", "", Metrics::Location::SPEECH_SYNTHESIZER_RECEIVE);

    if (id != m_mediaSourceId) {
        ACSDK_ERROR(LX("queueingExecutePlaybackFinishedFailed")
                        .d("reason", "mismatchSourceId")
                        .d("callbackSourceId", id)
                        .d("sourceId", m_mediaSourceId));
        m_executor.submit([this] {
            executePlaybackError(ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "executePlaybackFinishedFailed");
        });
    } else {
        m_executor.submit([this]() { executePlaybackFinished(); });
    }
}

void SpeechSynthesizer::onPlaybackError(
    SourceId id,
    const avsCommon::utils::mediaPlayer::ErrorType& type,
    std::string error) {
    ACSDK_DEBUG9(LX("onPlaybackError").d("callbackSourceId", id));
    m_executor.submit([this, type, error]() { executePlaybackError(type, error); });
}

void SpeechSynthesizer::onPlaybackStopped(SourceId id) {
    ACSDK_DEBUG9(LX("onPlaybackStopped").d("callbackSourceId", id));

    // MediaPlayer is for some reason stopping the playback of the speech.  Call setFailed if isSetFailedCalled flag is
    // not set yet.
    m_executor.submit([this]() {
        if (m_currentInfo) {
            m_currentInfo->sendPlaybackFinishedMessage = false;
            m_currentInfo->sendCompletedMessage = false;
            if (m_currentInfo->result && !m_currentInfo->isSetFailedCalled) {
                m_currentInfo->result->setFailed("Stopped due to MediaPlayer stopping.");
                m_currentInfo->isSetFailedCalled = true;
            }
        }
    });
    onPlaybackFinished(id);
}

SpeechSynthesizer::SpeakDirectiveInfo::SpeakDirectiveInfo(std::shared_ptr<DirectiveInfo> directiveInfo) :
        directive{directiveInfo->directive},
        result{directiveInfo->result},
        sendPlaybackStartedMessage{false},
        sendPlaybackFinishedMessage{false},
        sendCompletedMessage{false},
        isSetFailedCalled{false},
        isPlaybackInitiated{false},
        isDelayedCancel{false} {
}

void SpeechSynthesizer::SpeakDirectiveInfo::clear() {
    attachmentReader.reset();
    sendPlaybackStartedMessage = false;
    sendPlaybackFinishedMessage = false;
    sendCompletedMessage = false;
    isSetFailedCalled = false;
    isPlaybackInitiated = false;
    isDelayedCancel = false;
}

SpeechSynthesizer::SpeechSynthesizer(
    std::shared_ptr<MediaPlayerInterface> mediaPlayer,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<FocusManagerInterface> focusManager,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender) :
        CapabilityAgent{NAMESPACE, exceptionSender},
        RequiresShutdown{"SpeechSynthesizer"},
        m_mediaSourceId{MediaPlayerInterface::ERROR},
        m_speechPlayer{mediaPlayer},
        m_messageSender{messageSender},
        m_focusManager{focusManager},
        m_contextManager{contextManager},
        m_currentState{SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED},
        m_desiredState{SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED},
        m_currentFocus{FocusState::NONE},
        m_isAlreadyStopping{false},
        m_initialDialogUXStateReceived{false} {
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
    m_speechPlayer->setObserver(nullptr);
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING == m_currentState ||
            SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING == m_desiredState) {
            m_desiredState = SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED;
            if (m_currentInfo) {
                m_currentInfo->sendPlaybackFinishedMessage = false;
            }
            lock.unlock();
            stopPlaying();
            releaseForegroundFocus();

            lock.lock();
            m_currentState = SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED;
        }
    }
    {
        std::lock_guard<std::mutex> lock(m_speakInfoQueueMutex);
        for (auto& info : m_speakInfoQueue) {
            if (info.get()->result) {
                info.get()->result->setFailed("SpeechSynthesizerShuttingDown");
            }
            removeSpeakDirectiveInfo(info.get()->directive->getMessageId());
            removeDirective(info.get()->directive->getMessageId());
        }
    }
    m_executor.shutdown();
    m_speechPlayer.reset();
    m_waitOnStateChange.notify_one();
    m_messageSender.reset();
    m_focusManager.reset();
    m_contextManager.reset();
    m_observers.clear();
}

void SpeechSynthesizer::init() {
    m_speechPlayer->setObserver(shared_from_this());
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
    addToDirectiveQueue(speakInfo);
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
    }
    speakInfo->token = it->value.GetString();

    it = payload.FindMember(KEY_FORMAT);
    if (payload.MemberEnd() == it) {
        sendExceptionEncounteredAndReportMissingProperty(speakInfo, KEY_FORMAT);
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

    // If everything checks out, add the speakInfo to the map.
    if (!setSpeakDirectiveInfo(speakInfo->directive->getMessageId(), speakInfo)) {
        ACSDK_ERROR(LX("executePreHandleFailed")
                        .d("reason", "prehandleCalledTwiceOnSameDirective")
                        .d("messageId", speakInfo->directive->getMessageId()));
    }
}

void SpeechSynthesizer::executeHandleAfterValidation(std::shared_ptr<SpeakDirectiveInfo> speakInfo) {
    m_currentInfo = speakInfo;
    if (!m_focusManager->acquireChannel(CHANNEL_NAME, shared_from_this(), NAMESPACE)) {
        static const std::string message = std::string("Could not acquire ") + CHANNEL_NAME + " for " + NAMESPACE;
        ACSDK_ERROR(LX("executeHandleFailed")
                        .d("reason", "CouldNotAcquireChannel")
                        .d("messageId", m_currentInfo->directive->getMessageId()));
        sendExceptionEncounteredAndReportFailed(speakInfo, avsCommon::avs::ExceptionErrorType::INTERNAL_ERROR, message);
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
    ACSDK_DEBUG(LX("executeHandle").d("messageId", info->directive->getMessageId()));
    auto speakInfo = validateInfo("executeHandle", info);
    if (!speakInfo) {
        ACSDK_ERROR(LX("executeHandleFailed").d("reason", "invalidDirectiveInfo"));
        return;
    }
    addToDirectiveQueue(speakInfo);
}

void SpeechSynthesizer::executeCancel(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG(LX(__func__).d("messageId", info->directive->getMessageId()));
    auto speakInfo = validateInfo("executeCancel", info);
    executeCancel(speakInfo);
}

void SpeechSynthesizer::executeCancel(std::shared_ptr<SpeakDirectiveInfo> speakInfo) {
    if (!speakInfo) {
        ACSDK_ERROR(LX("executeCancelFailed").d("reason", "invalidDirectiveInfo"));
        return;
    }
    ACSDK_DEBUG(LX(__func__).d("messageId", speakInfo->directive->getMessageId()));
    if (speakInfo != m_currentInfo) {
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

    if (m_currentInfo) {
        if (m_currentInfo->isPlaybackInitiated) {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_desiredState = SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED;
            lock.unlock();

            m_currentInfo->sendPlaybackStartedMessage = false;
            m_currentInfo->sendCompletedMessage = false;
            stopPlaying();

        } else {
            // Playback has not been initiated yet. Setting the flag to delay the cancel call.
            m_currentInfo->isDelayedCancel = true;
        }
    }
}

void SpeechSynthesizer::executeStateChange() {
    SpeechSynthesizerObserverInterface::SpeechSynthesizerState newState;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        newState = m_desiredState;
    }
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
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED:
            // This happens when focus state is changed to BACKGROUND or NONE, requiring the @c SpeechSynthesizer to
            // trigger termination of playing the audio playback.
            if (m_currentInfo) {
                m_currentInfo->sendPlaybackFinishedMessage = false;
                m_currentInfo->sendCompletedMessage = false;
                if (m_currentInfo->result) {
                    m_currentInfo->result->setFailed("Stopped due to SpeechSynthesizer going into FINISHED state.");
                    m_currentInfo->isSetFailedCalled = true;
                }

                if (m_currentInfo->isPlaybackInitiated) {
                    stopPlaying();
                }
            }
            break;
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::GAINING_FOCUS:
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::LOSING_FOCUS:
            // Nothing to do so far
            break;
    }
}

void SpeechSynthesizer::executeProvideState(const unsigned int& stateRequestToken) {
    ACSDK_DEBUG(LX("executeProvideState").d("stateRequestToken", stateRequestToken));
    int64_t offsetInMilliseconds = 0;
    StateRefreshPolicy refreshPolicy = StateRefreshPolicy::NEVER;
    std::string speakDirectiveToken;

    if (SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING == m_currentState) {
        if (!m_currentInfo) {
            ACSDK_ERROR(LX("executeProvideStateFailed").d("reason", "currentInfoIsNull"));
            return;
        }

        // We should never be in the PLAYING state without a valid m_mediaSourceId.
        if (MediaPlayerInterface::ERROR == m_mediaSourceId) {
            ACSDK_ERROR(LX("executeProvideStateFailed")
                            .d("reason", "invalidMediaSourceId")
                            .d("mediaSourceId", m_mediaSourceId));
            return;
        }

        offsetInMilliseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(m_speechPlayer->getOffset(m_mediaSourceId)).count();
        refreshPolicy = StateRefreshPolicy::ALWAYS;
        speakDirectiveToken = m_currentInfo->token;
    } else if (m_currentInfo) {
        speakDirectiveToken = m_currentInfo->token;
    }

    auto jsonState = buildState(speakDirectiveToken, offsetInMilliseconds);
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
    m_waitOnStateChange.notify_one();
    if (m_currentInfo->sendPlaybackStartedMessage) {
        auto payload = buildPayload(m_currentInfo->token);
        if (payload.empty()) {
            ACSDK_ERROR(
                LX("executePlaybackStartedFailed").d("reason", "buildPayloadFailed").d("token", m_currentInfo->token));
            return;
        }
        auto msgIdAndJsonEvent = buildJsonEventString(SPEECH_STARTED_EVENT_NAME, "", payload);

        auto request = std::make_shared<MessageRequest>(msgIdAndJsonEvent.second);
        m_messageSender->sendMessage(request);
    }
}

void SpeechSynthesizer::executePlaybackFinished() {
    ACSDK_DEBUG(LX("executePlaybackFinished"));
    if (!m_currentInfo) {
        ACSDK_ERROR(LX("executePlaybackFinishedIgnored").d("reason", "nullptrDirectiveInfo"));
        return;
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        setCurrentStateLocked(SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);
    }
    m_waitOnStateChange.notify_one();
    if (m_currentInfo->sendPlaybackFinishedMessage) {
        auto payload = buildPayload(m_currentInfo->token);
        if (payload.empty()) {
            ACSDK_ERROR(LX("executePlaybackFinishedFailed")
                            .d("reason", "buildPayloadFailed")
                            .d("messageId", m_currentInfo->directive->getMessageId()));
        } else {
            auto msgIdAndJsonEvent = buildJsonEventString(SPEECH_FINISHED_EVENT_NAME, "", payload);

            auto request = std::make_shared<MessageRequest>(msgIdAndJsonEvent.second);
            m_messageSender->sendMessage(request);
        }
    }
    if (m_currentInfo->sendCompletedMessage) {
        setHandlingCompleted();
    }
    resetCurrentInfo();
    {
        std::lock_guard<std::mutex> lock_guard(m_speakInfoQueueMutex);
        if (!m_speakInfoQueue.empty()) {
            m_speakInfoQueue.pop_front();
            if (!m_speakInfoQueue.empty()) {
                executeHandleAfterValidation(m_speakInfoQueue.front());
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
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        setCurrentStateLocked(SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED);
    }
    m_waitOnStateChange.notify_one();
    releaseForegroundFocus();
    resetCurrentInfo();
    resetMediaSourceId();
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (!m_speakInfoQueue.empty()) {
            auto speakInfo = m_speakInfoQueue.front();
            m_speakInfoQueue.pop_front();
            lock.unlock();
            sendExceptionEncounteredAndReportFailed(
                speakInfo, avsCommon::avs::ExceptionErrorType::INTERNAL_ERROR, error);
            lock.lock();
        }
    }
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
    }

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    if (!state.Accept(writer)) {
        ACSDK_ERROR(LX("buildStateFailed").d("reason", "writerRefusedJsonObject"));
        return "";
    }

    return buffer.GetString();
}

std::string SpeechSynthesizer::buildPayload(std::string& token) {
    Document payload(kObjectType);
    Document::AllocatorType& alloc = payload.GetAllocator();

    payload.AddMember(KEY_TOKEN, token, alloc);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX("buildPayloadFailed").d("reason", "writerRefusedJsonObject"));
        return "";
    }

    return buffer.GetString();
}

void SpeechSynthesizer::startPlaying() {
    ACSDK_DEBUG9(LX("startPlaying"));
    m_mediaSourceId = m_speechPlayer->setSource(std::move(m_currentInfo->attachmentReader));
    if (MediaPlayerInterface::ERROR == m_mediaSourceId) {
        ACSDK_ERROR(LX("startPlayingFailed").d("reason", "setSourceFailed"));
        executePlaybackError(ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "playFailed");
    } else if (!m_speechPlayer->play(m_mediaSourceId)) {
        executePlaybackError(ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "playFailed");
    } else {
        // Execution of play is successful.
        m_isAlreadyStopping = false;
    }
}

void SpeechSynthesizer::stopPlaying() {
    ACSDK_DEBUG9(LX("stopPlaying"));
    if (MediaPlayerInterface::ERROR == m_mediaSourceId) {
        ACSDK_ERROR(LX("stopPlayingFailed").d("reason", "invalidMediaSourceId").d("mediaSourceId", m_mediaSourceId));
    } else if (m_isAlreadyStopping) {
        ACSDK_DEBUG9(LX("stopPlayingIgnored").d("reason", "isAlreadyStopping"));
    } else if (!m_speechPlayer->stop(m_mediaSourceId)) {
        executePlaybackError(ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "stopFailed");
    } else {
        // Execution of stop is successful.
        m_isAlreadyStopping = true;
    }
}

void SpeechSynthesizer::setCurrentStateLocked(SpeechSynthesizerObserverInterface::SpeechSynthesizerState newState) {
    ACSDK_DEBUG9(LX("setCurrentStateLocked").d("state", newState));
    m_currentState = newState;
    switch (newState) {
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING:
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED:
            executeProvideState(0);
            break;
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::LOSING_FOCUS:
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::GAINING_FOCUS:
            break;
    }
    for (auto observer : m_observers) {
        observer->onStateChanged(m_currentState);
    }
}

void SpeechSynthesizer::setDesiredStateLocked(FocusState newFocus) {
    switch (newFocus) {
        case FocusState::FOREGROUND:
            m_desiredState = SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING;
            break;
        case FocusState::BACKGROUND:
        case FocusState::NONE:
            m_desiredState = SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED;
            break;
    }
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

void SpeechSynthesizer::addToDirectiveQueue(std::shared_ptr<SpeakDirectiveInfo> speakInfo) {
    std::lock_guard<std::mutex> lock(m_speakInfoQueueMutex);
    if (m_speakInfoQueue.empty()) {
        m_speakInfoQueue.push_back(speakInfo);
        executeHandleAfterValidation(speakInfo);
    } else {
        ACSDK_DEBUG9(LX("addToDirectiveQueue").d("queueSize", m_speakInfoQueue.size()));
        m_speakInfoQueue.push_back(speakInfo);
    }
}

void SpeechSynthesizer::resetMediaSourceId() {
    m_mediaSourceId = MediaPlayerInterface::ERROR;
}

void SpeechSynthesizer::onDialogUXStateChanged(
    avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState newState) {
    m_executor.submit([this, newState]() { executeOnDialogUXStateChanged(newState); });
}

void SpeechSynthesizer::executeOnDialogUXStateChanged(
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

}  // namespace speechSynthesizer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
