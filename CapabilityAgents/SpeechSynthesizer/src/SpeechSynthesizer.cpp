/*
 * SpeechSynthesizer.cpp
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

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <AVSUtils/Logger/LogEntry.h>
#include <AVSUtils/Logging/Logger.h>

#include "SpeechSynthesizer/SpeechSynthesizer.h"

namespace alexaClientSDK {
namespace capabilityAgent {
namespace speechSynthesizer {

using namespace avsUtils;
using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::mediaPlayer;
using namespace rapidjson;

/// String to identify log entries originating from this file.
static const std::string TAG{"SpeechSynthesizer"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsUtils::logger::LogEntry(TAG, event)

/// The namespace for the @c SpeechSynthesizer capability agent.
static const std::string NAMESPACE{"SpeechSynthesizer"};

/// The @c Speak directive signature.
static const NamespaceAndName SPEAK{NAMESPACE, "Speak"};

/// The state information @c NamespaceAndName to send to the @c ContextManager.
static const NamespaceAndName CONTEXT_MANAGER_SPEECH_STATE{NAMESPACE, "SpeechState"};

/// The name of the @c FocusManager channel used by the @c SpeechSynthesizer.
static const std::string CHANNEL_NAME{"Dialog"};

/// The activity Id used with the @c FocusManager by @c SpeechSynthesizer.
static const std::string FOCUS_MANAGER_ACTIVITY_ID{"SpeechSynthesizer.Speak"};

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
static const std::chrono::seconds STATE_CHANGE_TIMEOUT{2};

std::unique_ptr<SpeechSynthesizer> SpeechSynthesizer::create(
        std::shared_ptr<MediaPlayerInterface> mediaPlayer,
        std::shared_ptr<MessageSenderInterface> messageSender,
        std::shared_ptr<FocusManagerInterface> focusManager,
        std::shared_ptr<ContextManagerInterface> contextManager,
        std::shared_ptr<attachment::AttachmentManagerInterface> attachmentManager,
        std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender) {
        std::unique_ptr<SpeechSynthesizer> speechSynthesizer(new SpeechSynthesizer(
                mediaPlayer, messageSender, focusManager, contextManager, attachmentManager, exceptionSender));
    if (speechSynthesizer) {
        speechSynthesizer->init();
        return speechSynthesizer;
    } else {
        return nullptr;
    }
}

SpeechSynthesizer::~SpeechSynthesizer() {
    m_speechPlayer->setObserver(nullptr);
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (SpeechSynthesizerState::PLAYING == m_currentState || SpeechSynthesizerState::PLAYING == m_desiredState) {
            m_desiredState = SpeechSynthesizerState::FINISHED;
            m_sendPlaybackFinishedMessage = false;
            stopPlaying();
            m_currentState = SpeechSynthesizerState::FINISHED;
            lock.unlock();
            releaseForegroundFocus();
        }
    }
    m_speechPlayer.reset();
    m_waitOnStateChange.notify_one();
}

void SpeechSynthesizer::addObserver(std::shared_ptr<SpeechSynthesizerObserver> observer) {
    m_executor.submit([this, observer] () { m_observers.insert(observer); });
}

void SpeechSynthesizer::onDeregistered() {
    // default no-op
}

void SpeechSynthesizer::handleDirectiveImmediately(std::shared_ptr <avsCommon::AVSDirective> directive) {
    std::string message = "speakDirectiveNotInvokedAsPartOfDialog" + directive->getMessageId();
    ACSDK_ERROR(LX("handleDirectiveImmediatelyFailed").d("reason", "speakDirectiveNotInvokedAsPartOfDialog")
            .d("messageId", directive->getMessageId()));
    m_exceptionSender->sendExceptionEncountered(
            directive->getUnparsedDirective(),
            ExceptionErrorType::INTERNAL_ERROR,
            message);
}

void SpeechSynthesizer::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    m_executor.submit([this, info] () { executePreHandle(info); });
}

void SpeechSynthesizer::handleDirective(std::shared_ptr<DirectiveInfo> info) {
    m_executor.submit([this, info] () { executeHandle(info); });
}

void SpeechSynthesizer::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
    m_executor.submit([this, info] () { executeCancel(info); });
}

void SpeechSynthesizer::onFocusChanged(FocusState newFocus) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_currentFocus = newFocus;
    setDesiredStateLocked(newFocus);
    if (m_currentState == m_desiredState) {
        return;
    }
    auto messageId = (m_currentInfo && m_currentInfo->directive) ? m_currentInfo->directive->getMessageId() : "";
    m_executor.submit([this] () { executeStateChange(); });
    // Block until we achieve the desired state.
    if (!m_waitOnStateChange.wait_for(lock, STATE_CHANGE_TIMEOUT, [this] () {
            return m_currentState == m_desiredState; })) {
        ACSDK_ERROR(LX("onFocusChangeFailed").d("reason", "stateChangeTimeout")
                .d("messageId", messageId));
        if (m_currentInfo) {
            sendExceptionEncounteredAndReportFailed(
                    m_currentInfo,
                    avsCommon::ExceptionErrorType::INTERNAL_ERROR,
                    "stateChangeTimeout");
        }
    };
}

void SpeechSynthesizer::provideState(const unsigned int stateRequestToken) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto state = m_currentState;
    m_executor.submit([this, state, stateRequestToken] () { executeProvideState(state, stateRequestToken); });
}

void SpeechSynthesizer::onContextAvailable(const std::string& jsonContext) {
    // default no-op
}

void SpeechSynthesizer::onContextFailure(const ContextRequestError error) {
    // default no-op
}

void SpeechSynthesizer::onPlaybackStarted() {
    m_executor.submit([this] () { executePlaybackStarted(); });
}

void SpeechSynthesizer::onPlaybackFinished() {
    m_executor.submit([this] () { executePlaybackFinished(); });
}

void SpeechSynthesizer::onPlaybackError(std::string error) {
    m_executor.submit([this, error] () { executePlaybackError(error); });
}

SpeechSynthesizer::SpeakDirectiveInfo::SpeakDirectiveInfo(
        std::shared_ptr<avsCommon::AVSDirective> directive,
        std::unique_ptr<avsCommon::sdkInterfaces::DirectiveHandlerResultInterface> result) :
        DirectiveInfo(directive, std::move(result)) {
}

void SpeechSynthesizer::SpeakDirectiveInfo::clear() {
    token.clear();
    attachmentReader.reset();
}

SpeechSynthesizer::SpeechSynthesizer(
        std::shared_ptr<MediaPlayerInterface> mediaPlayer,
        std::shared_ptr<MessageSenderInterface> messageSender,
        std::shared_ptr<FocusManagerInterface> focusManager,
        std::shared_ptr<ContextManagerInterface> contextManager,
        std::shared_ptr<attachment::AttachmentManagerInterface> attachmentManager,
        std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender) :
        CapabilityAgent{NAMESPACE, exceptionSender},
        m_speechPlayer{mediaPlayer},
        m_messageSender{messageSender},
        m_focusManager{focusManager},
        m_contextManager{contextManager},
        m_attachmentManager{attachmentManager},
        m_exceptionSender{exceptionSender},
        m_currentState{SpeechSynthesizerState::FINISHED},
        m_desiredState{SpeechSynthesizerState::FINISHED},
        m_currentFocus{FocusState::NONE},
        m_sendPlaybackFinishedMessage{true} {
    // The shared_ptr m_focusManager and calls made through it passing a shared_ptr to this SpeechSynthesizer pose
    // a shared_ptr cycle hazard. To eliminate this hazard a separate shared_ptr to the ChannelObserver portion
    // of this (with a disabled deleter) is created and passed to FocusManager instead of a shared_from_this().
    m_thisAsChannelObserver = std::shared_ptr<ChannelObserverInterface>(this, [](MediaPlayerObserverInterface*){});
}

void SpeechSynthesizer::init() {
    // SpeechSynthesizer holds a shared_ptr to MediaPlayer and MediaPlayer holds a shared_ptr to a
    // MediaPlayerObserverInterface, which SpeechSynthesizer is derived from. This would result in shared_ptr
    // circular reference that would prevent both SpeechSynthesizer and its MediaPlayer from ever being deleted.
    // This problem is prevented by passing MediaPlayer a separate shared_ptr to this SpeechSynthesizer with a
    // disabled deleter. The separate shared_ptr prevents MediaPlayer's reference to SpeechSynthesizer from
    // preventing SpeechSynthesizer's deletion. The disabled deleter prevents MediaPlayer's eventual release of
    // its shared_ptr to SpeechSynthesizer from causing a duplicate deletion of SpeechSynthesizer.
    std::shared_ptr<MediaPlayerObserverInterface> thisAsMediaPlayerObserver(this, [](MediaPlayerObserverInterface*){});
    m_speechPlayer->setObserver(thisAsMediaPlayerObserver);
    std::shared_ptr<StateProviderInterface> thisAsStateProvider(this, [](StateProviderInterface*){});
    m_contextManager->setStateProvider(CONTEXT_MANAGER_SPEECH_STATE, thisAsStateProvider);
}

std::shared_ptr<CapabilityAgent::DirectiveInfo> SpeechSynthesizer::createDirectiveInfo(
        std::shared_ptr<avsCommon::AVSDirective> directive,
        std::unique_ptr<avsCommon::sdkInterfaces::DirectiveHandlerResultInterface> result) {
    return std::make_shared<SpeakDirectiveInfo>(directive, std::move(result));
}

void SpeechSynthesizer::executePreHandle(std::shared_ptr<DirectiveInfo> info) {
    auto speakInfo = validateInfo("executePreHandle", info);
    if (!speakInfo) {
        ACSDK_ERROR(LX("executePreHandleFailed").d("reason", "invalidDirectiveInfo"));
        return;
    }

    if (speakInfo->directive->getName() != SPEAK.name) {
        const std::string message("unexpectedDirective " + speakInfo->directive->getName());
        ACSDK_ERROR(LX("executePreHandleFailed").d("reason", "unexpectedDirective")
                .d("directiveName", speakInfo->directive->getName()));
        sendExceptionEncounteredAndReportFailed(
                speakInfo, avsCommon::ExceptionErrorType::UNSUPPORTED_OPERATION, message);
        return;
    }

    Document payload;
    if (payload.Parse(speakInfo->directive->getPayload()).HasParseError()) {
        const std::string message("unableToParsePayload" + speakInfo->directive->getMessageId());
        ACSDK_ERROR(LX("executePreHandleFailed").d("reason", message)
                .d("messageId", speakInfo->directive->getMessageId()));
        sendExceptionEncounteredAndReportFailed(
                speakInfo, avsCommon::ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED, message);
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
        const std::string message("unknownFormat " + speakInfo->directive->getMessageId() +
                "format " + format);
        ACSDK_ERROR(LX("executePreHandleFailed").d("reason", "unknownFormat")
                .d("messageId", speakInfo->directive->getMessageId()));
        sendExceptionEncounteredAndReportFailed(
                speakInfo, avsCommon::ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED, message);
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
        ACSDK_ERROR(LX("executePreHandleFailed").d("reason", message).d("url", urlValue));
        sendExceptionEncounteredAndReportFailed(
                speakInfo, avsCommon::ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED, message);
        return;
    }
    std::string contentId = urlValue.substr(contentIdPosition + CID_PREFIX.length());
    speakInfo->attachmentReader = speakInfo->directive->getAttachmentReader(
            contentId, AttachmentReader::Policy::BLOCKING);
    if (!speakInfo->attachmentReader) {
        const std::string message("getAttachmentReaderFailed");
        ACSDK_ERROR(LX("executePreHandleFailed").d("reason", message));
        sendExceptionEncounteredAndReportFailed(speakInfo, avsCommon::ExceptionErrorType::INTERNAL_ERROR, message);
    }
}

void SpeechSynthesizer::executeHandle(std::shared_ptr<DirectiveInfo> info) {
    auto speakInfo = validateInfo("executeHandle", info);
    if (!speakInfo) {
        ACSDK_ERROR(LX("executeHandleFailed").d("reason", "invalidDirectiveInfo"));
        return;
    }
    if (m_currentInfo) {
        std::string message = "speakAlreadyBeingHandled for messageId " + m_currentInfo->directive->getMessageId();
        ACSDK_ERROR(LX("executeHandleFailed").d("reason", "speakAlreadyBeingHandled")
                .d("messageId", m_currentInfo->directive->getMessageId()));
        m_currentInfo->result->setFailed("stoppedByRequestToHandleNewSpeakDirective");
    }
    m_currentInfo = speakInfo;
    if (!m_focusManager->acquireChannel(CHANNEL_NAME, m_thisAsChannelObserver, FOCUS_MANAGER_ACTIVITY_ID)) {
        static const std::string message = std::string("Could not acquire ") + CHANNEL_NAME + " for " +
                FOCUS_MANAGER_ACTIVITY_ID;
        ACSDK_ERROR(LX("executeHandleFailed").d("reason", "CouldNotAcquireChannel")
                .d("messageId", m_currentInfo->directive->getMessageId()));
        sendExceptionEncounteredAndReportFailed(speakInfo, avsCommon::ExceptionErrorType::INTERNAL_ERROR, message);
    }
}

void SpeechSynthesizer::executeCancel(std::shared_ptr<DirectiveInfo> info) {
    auto speakInfo = validateInfo("executeCancel", info);
    if (!speakInfo) {
        ACSDK_ERROR(LX("executeCancelFailed").d("reason", "invalidDirectiveInfo"));
        return;
    }
    if (speakInfo != m_currentInfo) {
        speakInfo->clear();
        removeDirective(speakInfo->directive->getMessageId());
        return;
    }
    std::unique_lock<std::mutex> lock(m_mutex);
    m_desiredState = SpeechSynthesizerState::FINISHED;
    if (SpeechSynthesizerState::PLAYING == m_currentState) {
        lock.unlock();
        m_sendPlaybackFinishedMessage = false;
        stopPlaying();
    }
}

void SpeechSynthesizer::executeStateChange() {
    SpeechSynthesizerState newState;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        newState = m_desiredState;
    }
    switch (newState) {
        case SpeechSynthesizerState::PLAYING:
            m_sendPlaybackFinishedMessage = true;
            startPlaying();
            break;
        case SpeechSynthesizerState::FINISHED:
            stopPlaying();
            break;
    }
}

void SpeechSynthesizer::executeProvideState(const SpeechSynthesizerState &state, const unsigned int& stateRequestToken) {
    int64_t offsetInMilliseconds = 0;
    StateRefreshPolicy refreshPolicy = StateRefreshPolicy::NEVER;
    std::string speakDirectiveToken;

    if (!m_currentInfo && SpeechSynthesizerState::PLAYING == state) {
        ACSDK_ERROR(LX("executeProvideStateFailed").d("reason", "currentInfoIsNull"));
        return;
    } else if (SpeechSynthesizerState::PLAYING == state) {
        offsetInMilliseconds = m_speechPlayer->getOffsetInMilliseconds();
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
    auto result = m_contextManager->setState(CONTEXT_MANAGER_SPEECH_STATE, jsonState, refreshPolicy,
            stateRequestToken);
    if (result != SetStateResult::SUCCESS) {
        ACSDK_ERROR(LX("executeProvideState").d("reason", "contextManagerSetStateFailed").
                d("token", speakDirectiveToken));
    }
}

void SpeechSynthesizer::executePlaybackStarted() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        setCurrentStateLocked(SpeechSynthesizerState::PLAYING);
    }
    m_waitOnStateChange.notify_one();
    auto payload = buildPayload(m_currentInfo->token);
    if (payload.empty()) {
        ACSDK_ERROR(LX("executePlaybackStartedFailed").d("reason", "buildPayloadFailed")
                .d("token", m_currentInfo->token));
        return;
    }
    auto jsonEventString = buildJsonEventString(SPEECH_STARTED_EVENT_NAME, "", payload);
    auto request = std::make_shared<MessageRequest>(jsonEventString);
    m_messageSender->sendMessage(request);
}

void SpeechSynthesizer::executePlaybackFinished() {
    if (!m_currentInfo) {
        ACSDK_DEBUG(LX("executePlaybackFinsihedIgnored").d("reason", "nullptrDirectiveInfo"));
        return;
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        setCurrentStateLocked(SpeechSynthesizerState::FINISHED);
    }
    m_waitOnStateChange.notify_one();
    releaseForegroundFocus();
    if (m_sendPlaybackFinishedMessage) {
        auto payload = buildPayload(m_currentInfo->token);
        if (payload.empty()) {
            ACSDK_ERROR(LX("executePlaybackFinishedFailed").d("reason", "buildPayloadFailed")
                    .d("messageId", m_currentInfo->directive->getMessageId()));
        } else {
            auto jsonEventString = buildJsonEventString(SPEECH_FINISHED_EVENT_NAME, "", payload);
            auto request = std::make_shared<MessageRequest>(jsonEventString);
            m_messageSender->sendMessage(request);
        }
    }
    setHandlingCompleted();
}

void SpeechSynthesizer::executePlaybackError(std::string error) {
    if (!m_currentInfo) {
        ACSDK_DEBUG(LX("executePlaybackErrorIgnored").d("reason", "nullptrDirectiveInfo"));
        return;
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        setCurrentStateLocked(SpeechSynthesizerState::FINISHED);
    }
    m_waitOnStateChange.notify_one();
    releaseForegroundFocus();
    sendExceptionEncounteredAndReportFailed(m_currentInfo, avsCommon::ExceptionErrorType::INTERNAL_ERROR, error);
    resetCurrentInfo();
}

std::string SpeechSynthesizer::buildState(std::string &token, int64_t offsetInMilliseconds) const {
    Document state(kObjectType);
    Document::AllocatorType& alloc = state.GetAllocator();
    state.AddMember(KEY_TOKEN, token, alloc);
    state.AddMember(KEY_OFFSET_IN_MILLISECONDS, offsetInMilliseconds, alloc);
    if (SpeechSynthesizerState::PLAYING == m_currentState) {
        state.AddMember(KEY_PLAYER_ACTIVITY, PLAYER_STATE_PLAYING, alloc);
    } else {
        state.AddMember(KEY_PLAYER_ACTIVITY, PLAYER_STATE_FINISHED, alloc);
    }

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    if (!state.Accept(writer)) {
        ACSDK_ERROR(LX("buildStateFailed").d("reason", "writerRefusedJsonObject"));
        return "";
    }

    return buffer.GetString();
}

std::string SpeechSynthesizer::buildPayload(std::string &token) {
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
    m_speechPlayer->setSource(std::move(m_currentInfo->attachmentReader));
    auto mediaPlayerStatus = m_speechPlayer->play();
    switch (mediaPlayerStatus) {
        case MediaPlayerStatus::SUCCESS:
        case MediaPlayerStatus::PENDING:
            break;
        case MediaPlayerStatus::FAILURE:
            executePlaybackError("playFailed");
            break;
    }
}

void SpeechSynthesizer::stopPlaying() {
    auto mediaPlayerStatus = m_speechPlayer->stop();
    switch (mediaPlayerStatus) {
        case MediaPlayerStatus::SUCCESS:
        case MediaPlayerStatus::PENDING:
            break;
        case MediaPlayerStatus::FAILURE:
            executePlaybackError("stopFailed");
            break;
    }
}

void SpeechSynthesizer::setCurrentStateLocked(SpeechSynthesizerState newState) {
    m_currentState = newState;
    executeProvideState(m_currentState, 0);
    // Don't allow any slow observers to block our caller.
    m_executor.submit([this] () {
        for (auto observer : m_observers) {
            observer->onStateChanged(m_currentState);
        }
    });
}

void SpeechSynthesizer::setDesiredStateLocked(FocusState newFocus) {
     switch (newFocus) {
        case FocusState::FOREGROUND:
            m_desiredState = SpeechSynthesizerState::PLAYING;
            break;
        case FocusState::BACKGROUND:
        case FocusState::NONE:
            m_desiredState = SpeechSynthesizerState::FINISHED;
            break;
    }
}

void SpeechSynthesizer::resetCurrentInfo(std::shared_ptr<SpeakDirectiveInfo> speakInfo) {
    if (m_currentInfo != speakInfo) {
        if (m_currentInfo) {
            removeDirective(m_currentInfo->directive->getMessageId());
            m_currentInfo->clear();
        }
        m_currentInfo = speakInfo;
    }
}

void SpeechSynthesizer::setHandlingCompleted() {
    if (m_currentInfo) {
        m_currentInfo->result->setCompleted();
    }
    resetCurrentInfo();
}

void SpeechSynthesizer::setHandlingFailed(const std::string& description) {
    if (m_currentInfo) {
        m_currentInfo->result->setFailed(description);
    }
    resetCurrentInfo();
}

void SpeechSynthesizer::sendExceptionEncounteredAndReportFailed(
        std::shared_ptr<SpeakDirectiveInfo> speakInfo,
        avsCommon::ExceptionErrorType type,
        const std::string& message) {
    m_exceptionSender->sendExceptionEncountered(speakInfo->directive->getUnparsedDirective(), type, message);
    speakInfo->result->setFailed(message);
    speakInfo->clear();
    removeDirective(speakInfo->directive->getMessageId());
}

void SpeechSynthesizer::sendExceptionEncounteredAndReportMissingProperty(
        std::shared_ptr<SpeakDirectiveInfo> speakInfo, const std::string& key) {
    ACSDK_ERROR(LX("executePreHandleFailed").d("reason", "missingProperty").d("key", key));
    sendExceptionEncounteredAndReportFailed(
            speakInfo,
            avsCommon::ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED,
            std::string("missing property: ") + key);
}

void SpeechSynthesizer::releaseForegroundFocus() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_currentFocus = FocusState::NONE;
    }
    m_focusManager->releaseChannel(CHANNEL_NAME, m_thisAsChannelObserver);
}

std::shared_ptr<SpeechSynthesizer::SpeakDirectiveInfo> SpeechSynthesizer::validateInfo(
        const std::string& caller, std::shared_ptr<DirectiveInfo> info) {
    if (!info) {
        ACSDK_ERROR(LX(caller + "Failed").d("reason", "nullptrInfo"));
        return nullptr;
    }
    if (!info->directive) {
        ACSDK_ERROR(LX(caller + "Failed").d("reason", "nullptrDirective"));
        return nullptr;
    }
    if (!info->result) {
        ACSDK_ERROR(LX(caller + "Failed").d("reason", "nullptrResult"));
        return nullptr;
    }
    auto result = std::dynamic_pointer_cast<SpeakDirectiveInfo>(info);
    if (!result) {
        ACSDK_ERROR(LX(caller + "Failed").d("reason", "castToSpeakDirectiveInfoFailed"));
    }
    return result;
}

} // namespace speechSynthesizer
} // namespace capabilityAgent
} // namespace alexaClientSDK
