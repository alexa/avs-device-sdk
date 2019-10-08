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

/// @file AudioPlayer.cpp
#include <algorithm>

#include "AudioPlayer/AudioPlayer.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/error/en.h>

#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace audioPlayer {

using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::json;
using namespace avsCommon::utils::logger;
using namespace avsCommon::utils::mediaPlayer;

/// AudioPlayer capability constants
/// AudioPlayer interface type
static const std::string AUDIOPLAYER_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";
/// AudioPlayer interface name
static const std::string AUDIOPLAYER_CAPABILITY_INTERFACE_NAME = "AudioPlayer";
/// AudioPlayer interface version
static const std::string AUDIOPLAYER_CAPABILITY_INTERFACE_VERSION = "1.0";

/// String to identify log entries originating from this file.
static const std::string TAG("AudioPlayer");

/// A link to @c MediaPlayerInterface::ERROR.
static const AudioPlayer::SourceId ERROR_SOURCE_ID = MediaPlayerInterface::ERROR;

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The name of the @c FocusManager channel used by @c AudioPlayer.
static const std::string CHANNEL_NAME = avsCommon::sdkInterfaces::FocusManagerInterface::CONTENT_CHANNEL_NAME;

/// The namespace for this capability agent.
static const std::string NAMESPACE = "AudioPlayer";

/// The @c Play directive signature.
static const NamespaceAndName PLAY{NAMESPACE, "Play"};

/// The @c Stop directive signature.
static const NamespaceAndName STOP{NAMESPACE, "Stop"};

/// The @c ClearQueue directive signature.
static const NamespaceAndName CLEAR_QUEUE{NAMESPACE, "ClearQueue"};

/// The @c AudioPlayer context state signature.
static const NamespaceAndName STATE{NAMESPACE, "PlaybackState"};

/// Prefix for content ID prefix in the url property of the directive payload.
static const std::string CID_PREFIX{"cid:"};

/// The token key used in @c AudioPlayer events.
static const char TOKEN_KEY[] = "token";

/// The offset key used in @c AudioPlayer events.
static const char OFFSET_KEY[] = "offsetInMilliseconds";

/// The activity key used in @c AudioPlayer events.
static const char ACTIVITY_KEY[] = "playerActivity";

/// The stutter key used in @c AudioPlayer events.
static const char STUTTER_DURATION_KEY[] = "stutterDurationInMilliseconds";

/// The duration to wait for a state change in @c onFocusChanged before failing.
static const std::chrono::seconds TIMEOUT{2};

/**
 * Creates the AudioPlayer capability configuration.
 *
 * @return The AudioPlayer capability configuration.
 */
static std::shared_ptr<avsCommon::avs::CapabilityConfiguration> getAudioPlayerCapabilityConfiguration();

AudioPlayer::PlayDirectiveInfo::PlayDirectiveInfo(const std::string& messageId) :
        messageId{messageId},
        playBehavior{PlayBehavior::ENQUEUE},
        sourceId{ERROR_SOURCE_ID} {
}

std::shared_ptr<AudioPlayer> AudioPlayer::create(
    std::shared_ptr<MediaPlayerInterface> mediaPlayer,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<FocusManagerInterface> focusManager,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<PlaybackRouterInterface> playbackRouter) {
    if (nullptr == mediaPlayer) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMediaPlayer"));
        return nullptr;
    } else if (nullptr == messageSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMessageSender"));
        return nullptr;
    } else if (nullptr == focusManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullFocusManager"));
        return nullptr;
    } else if (nullptr == contextManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullContextManager"));
        return nullptr;
    } else if (nullptr == exceptionSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionSender"));
        return nullptr;
    } else if (nullptr == playbackRouter) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullPlaybackRouter"));
        return nullptr;
    }

    auto audioPlayer = std::shared_ptr<AudioPlayer>(
        new AudioPlayer(mediaPlayer, messageSender, focusManager, contextManager, exceptionSender, playbackRouter));
    mediaPlayer->setObserver(audioPlayer);
    contextManager->setStateProvider(STATE, audioPlayer);
    return audioPlayer;
}

void AudioPlayer::provideState(
    const avsCommon::avs::NamespaceAndName& stateProviderName,
    unsigned int stateRequestToken) {
    ACSDK_DEBUG(LX("provideState").d("stateRequestToken", stateRequestToken));
    m_executor.submit([this, stateRequestToken] { executeProvideState(true, stateRequestToken); });
}

void AudioPlayer::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    auto info = std::make_shared<DirectiveInfo>(directive, nullptr);
    preHandleDirective(info);
    handleDirective(info);
}

void AudioPlayer::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    if (!info || !info->directive) {
        ACSDK_ERROR(LX("preHandleDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }
    ACSDK_DEBUG5(LX(__func__).d("name", info->directive->getName()).d("messageId", info->directive->getMessageId()));

    if (info->directive->getName() == PLAY.name) {
        preHandlePlayDirective(info);
    }
}

void AudioPlayer::handleDirective(std::shared_ptr<DirectiveInfo> info) {
    if (!info || !info->directive) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }
    ACSDK_DEBUG5(
        LX("handleDirective").d("name", info->directive->getName()).d("messageId", info->directive->getMessageId()));
    if (info->directive->getName() == PLAY.name) {
        handlePlayDirective(info);
    } else if (info->directive->getName() == STOP.name) {
        handleStopDirective(info);
    } else if (info->directive->getName() == CLEAR_QUEUE.name) {
        handleClearQueueDirective(info);
    } else {
        sendExceptionEncounteredAndReportFailed(
            info,
            "unexpected directive " + info->directive->getNamespace() + ":" + info->directive->getName(),
            ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        ACSDK_ERROR(LX("handleDirectiveFailed")
                        .d("reason", "unknownDirective")
                        .d("namespace", info->directive->getNamespace())
                        .d("name", info->directive->getName()));
    }
}

void AudioPlayer::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
    removeDirective(info);
    if (info && info->directive) {
        ACSDK_DEBUG(LX("cancelDirective").d("name", info->directive->getName()));
        auto messageId = info->directive->getMessageId();
        m_executor.submit([this, messageId] {
            m_preHandlePlayInfoList.erase(
                std::remove_if(
                    m_preHandlePlayInfoList.begin(),
                    m_preHandlePlayInfoList.end(),
                    [messageId](std::shared_ptr<PlayDirectiveInfo> info) { return messageId == info->messageId; }),
                m_preHandlePlayInfoList.end());
        });
    }
}

void AudioPlayer::onDeregistered() {
    ACSDK_DEBUG(LX("onDeregistered"));
    m_executor.submit([this] {
        executeStop();
        m_audioPlayQueue.clear();
    });
}

DirectiveHandlerConfiguration AudioPlayer::getConfiguration() const {
    DirectiveHandlerConfiguration configuration;
    auto audioNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, false);

    configuration[PLAY] = audioNonBlockingPolicy;
    configuration[STOP] = audioNonBlockingPolicy;
    configuration[CLEAR_QUEUE] = audioNonBlockingPolicy;
    return configuration;
}

void AudioPlayer::onFocusChanged(FocusState newFocus) {
    ACSDK_DEBUG(LX("onFocusChanged").d("newFocus", newFocus));
    m_executor.submit([this, newFocus] { executeOnFocusChanged(newFocus); });

    switch (newFocus) {
        case FocusState::FOREGROUND:
            // Could wait for playback to actually start, but there's no real benefit to waiting, and long delays in
            // buffering could result in timeouts, so returning immediately for this case.
            return;
        case FocusState::BACKGROUND: {
            // Ideally expecting to see a transition to PAUSED, but in terms of user-observable changes, a move to any
            // of PAUSED/STOPPED/FINISHED will indicate that it's safe for another channel to move to the foreground.
            auto predicate = [this] {
                switch (m_currentActivity) {
                    case PlayerActivity::IDLE:
                    case PlayerActivity::PAUSED:
                    case PlayerActivity::STOPPED:
                    case PlayerActivity::FINISHED:
                        return true;
                    case PlayerActivity::PLAYING:
                    case PlayerActivity::BUFFER_UNDERRUN:
                        return false;
                }
                ACSDK_ERROR(LX("onFocusChangedFailed")
                                .d("reason", "unexpectedActivity")
                                .d("m_currentActivity", m_currentActivity));
                return false;
            };
            std::unique_lock<std::mutex> lock(m_currentActivityMutex);
            if (!m_currentActivityConditionVariable.wait_for(lock, TIMEOUT, predicate)) {
                ACSDK_ERROR(
                    LX("onFocusChangedTimedOut").d("newFocus", newFocus).d("m_currentActivity", m_currentActivity));
            }
        }
            return;
        case FocusState::NONE: {
            // Need to wait for STOPPED or FINISHED, indicating that we have completely ended playback.
            auto predicate = [this] {
                switch (m_currentActivity) {
                    case PlayerActivity::IDLE:
                    case PlayerActivity::STOPPED:
                    case PlayerActivity::FINISHED:
                        return true;
                    case PlayerActivity::PLAYING:
                    case PlayerActivity::PAUSED:
                    case PlayerActivity::BUFFER_UNDERRUN:
                        return false;
                }
                ACSDK_ERROR(LX("onFocusChangedFailed")
                                .d("reason", "unexpectedActivity")
                                .d("m_currentActivity", m_currentActivity));
                return false;
            };
            std::unique_lock<std::mutex> lock(m_currentActivityMutex);
            if (!m_currentActivityConditionVariable.wait_for(lock, TIMEOUT, predicate)) {
                ACSDK_ERROR(LX("onFocusChangedFailed")
                                .d("reason", "activityChangeTimedOut")
                                .d("newFocus", newFocus)
                                .d("m_currentActivity", m_currentActivity));
            }
        }
            return;
    }
    ACSDK_ERROR(LX("onFocusChangedFailed").d("reason", "unexpectedFocusState").d("newFocus", newFocus));
}

void AudioPlayer::onPlaybackStarted(SourceId id) {
    ACSDK_DEBUG(LX("onPlaybackStarted").d("id", id));
    m_executor.submit([this, id] { executeOnPlaybackStarted(id); });
}

void AudioPlayer::onPlaybackStopped(SourceId id) {
    ACSDK_DEBUG(LX("onPlaybackStopped").d("id", id));
    m_executor.submit([this, id] { executeOnPlaybackStopped(id); });
}

void AudioPlayer::onPlaybackFinished(SourceId id) {
    ACSDK_DEBUG(LX("onPlaybackFinished").d("id", id));
    m_executor.submit([this, id] { executeOnPlaybackFinished(id); });
}

void AudioPlayer::onPlaybackError(SourceId id, const ErrorType& type, std::string error) {
    ACSDK_DEBUG(LX("onPlaybackError").d("type", type).d("error", error).d("id", id));
    m_executor.submit([this, id, type, error] { executeOnPlaybackError(id, type, error); });
}

void AudioPlayer::onPlaybackPaused(SourceId id) {
    ACSDK_DEBUG(LX("onPlaybackPaused").d("id", id));
    m_executor.submit([this, id] { executeOnPlaybackPaused(id); });
}

void AudioPlayer::onPlaybackResumed(SourceId id) {
    ACSDK_DEBUG(LX("onPlaybackResumed").d("id", id));
    m_executor.submit([this, id] { executeOnPlaybackResumed(id); });
}

void AudioPlayer::onBufferUnderrun(SourceId id) {
    ACSDK_DEBUG(LX("onBufferUnderrun").d("id", id));
    m_executor.submit([this, id] { executeOnBufferUnderrun(id); });
}

void AudioPlayer::onBufferRefilled(SourceId id) {
    ACSDK_DEBUG(LX("onBufferRefilled").d("id", id));
    m_executor.submit([this, id] { executeOnBufferRefilled(id); });
}

void AudioPlayer::onTags(SourceId id, std::unique_ptr<const VectorOfTags> vectorOfTags) {
    ACSDK_DEBUG(LX("onTags").d("id", id));
    if (nullptr == vectorOfTags || vectorOfTags->empty()) {
        ACSDK_ERROR(LX("onTagsFailed").d("reason", "noTags"));
        return;
    }
    std::shared_ptr<const VectorOfTags> sharedVectorOfTags(std::move(vectorOfTags));
    m_executor.submit([this, id, sharedVectorOfTags] { executeOnTags(id, sharedVectorOfTags); });
}

void AudioPlayer::onProgressReportDelayElapsed() {
    ACSDK_DEBUG5(LX(__func__));
    m_executor.submit([this] { sendEventWithTokenAndOffset("ProgressReportDelayElapsed"); });
}

void AudioPlayer::onProgressReportIntervalElapsed() {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this] { sendEventWithTokenAndOffset("ProgressReportIntervalElapsed"); });
}

void AudioPlayer::requestProgress() {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this] {
        auto progress = getOffset();
        m_progressTimer.onProgress(progress);
    });
}

void AudioPlayer::addObserver(std::shared_ptr<avsCommon::sdkInterfaces::AudioPlayerObserverInterface> observer) {
    ACSDK_DEBUG1(LX("addObserver"));
    if (!observer) {
        ACSDK_ERROR(LX("addObserver").m("Observer is null."));
        return;
    }
    m_executor.submit([this, observer] {
        if (!m_observers.insert(observer).second) {
            ACSDK_ERROR(LX("addObserver").m("Duplicate observer."));
        }
    });
}

void AudioPlayer::removeObserver(std::shared_ptr<avsCommon::sdkInterfaces::AudioPlayerObserverInterface> observer) {
    ACSDK_DEBUG1(LX("removeObserver"));
    if (!observer) {
        ACSDK_ERROR(LX("removeObserver").m("Observer is null."));
        return;
    }
    m_executor.submit([this, observer] {
        if (m_observers.erase(observer) == 0) {
            ACSDK_WARN(LX("removeObserver").m("Nonexistent observer."));
        }
    });
}

void AudioPlayer::setObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsObserverInterface> observer) {
    ACSDK_DEBUG1(LX(__func__));
    m_executor.submit([this, observer] { m_renderPlayerObserver = observer; });
}

std::chrono::milliseconds AudioPlayer::getAudioItemOffset() {
    ACSDK_DEBUG1(LX(__func__));
    auto offset = m_executor.submit([this] { return getOffset(); });
    return offset.get();
}

AudioPlayer::AudioPlayer(
    std::shared_ptr<MediaPlayerInterface> mediaPlayer,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<FocusManagerInterface> focusManager,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<PlaybackRouterInterface> playbackRouter) :
        CapabilityAgent{NAMESPACE, exceptionSender},
        RequiresShutdown{"AudioPlayer"},
        m_mediaPlayer{mediaPlayer},
        m_messageSender{messageSender},
        m_focusManager{focusManager},
        m_contextManager{contextManager},
        m_playbackRouter{playbackRouter},
        m_currentActivity{PlayerActivity::IDLE},
        m_focus{FocusState::NONE},
        m_initialOffset{0},
        m_sourceId{MediaPlayerInterface::ERROR},
        m_isPreBuffering{false},
        m_offset{std::chrono::milliseconds{std::chrono::milliseconds::zero()}},
        m_isStopCalled{false} {
    m_capabilityConfigurations.insert(getAudioPlayerCapabilityConfiguration());
}

std::shared_ptr<CapabilityConfiguration> getAudioPlayerCapabilityConfiguration() {
    std::unordered_map<std::string, std::string> configMap;
    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, AUDIOPLAYER_CAPABILITY_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, AUDIOPLAYER_CAPABILITY_INTERFACE_NAME});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, AUDIOPLAYER_CAPABILITY_INTERFACE_VERSION});

    return std::make_shared<CapabilityConfiguration>(configMap);
}

void AudioPlayer::doShutdown() {
    m_progressTimer.stop();
    m_executor.shutdown();
    executeStop();
    m_mediaPlayer->setObserver(nullptr);
    m_mediaPlayer.reset();
    m_messageSender.reset();
    m_focusManager.reset();
    m_contextManager->setStateProvider(STATE, nullptr);
    m_contextManager.reset();
    m_audioPlayQueue.clear();
    m_playbackRouter.reset();
}

bool AudioPlayer::parseDirectivePayload(std::shared_ptr<DirectiveInfo> info, rapidjson::Document* document) {
    rapidjson::ParseResult result = document->Parse(info->directive->getPayload());
    if (result) {
        return true;
    }

    ACSDK_ERROR(LX("parseDirectivePayloadFailed")
                    .d("reason", rapidjson::GetParseError_En(result.Code()))
                    .d("offset", result.Offset())
                    .d("messageId", info->directive->getMessageId()));
    sendExceptionEncounteredAndReportFailed(
        info, "Unable to parse payload", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
    return false;
}

void AudioPlayer::preHandlePlayDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG1(LX("preHandlePlayDirective"));
    ACSDK_DEBUG9(LX("PLAY").d("payload", info->directive->getPayload()));
    rapidjson::Document payload;
    if (!parseDirectivePayload(info, &payload)) {
        return;
    }

    PlayBehavior playBehavior;
    if (!jsonUtils::retrieveValue(payload, "playBehavior", &playBehavior)) {
        playBehavior = PlayBehavior::ENQUEUE;
    }

    rapidjson::Value::ConstMemberIterator audioItemJson;
    if (!jsonUtils::findNode(payload, "audioItem", &audioItemJson)) {
        ACSDK_ERROR(LX("preHandlePlayDirectiveFailed")
                        .d("reason", "missingAudioItem")
                        .d("messageId", info->directive->getMessageId()));
        sendExceptionEncounteredAndReportFailed(info, "missing AudioItem");
        return;
    }

    AudioItem audioItem;
    if (!jsonUtils::retrieveValue(audioItemJson->value, "audioItemId", &audioItem.id)) {
        audioItem.id = "anonymous";
    }

    rapidjson::Value::ConstMemberIterator stream;
    if (!jsonUtils::findNode(audioItemJson->value, "stream", &stream)) {
        ACSDK_ERROR(LX("preHandlePlayDirectiveFailed")
                        .d("reason", "missingStream")
                        .d("messageId", info->directive->getMessageId()));
        sendExceptionEncounteredAndReportFailed(info, "missing stream");
        return;
    }

    if (!jsonUtils::retrieveValue(stream->value, "url", &audioItem.stream.url)) {
        ACSDK_ERROR(LX("preHandlePlayDirectiveFailed")
                        .d("reason", "missingUrl")
                        .d("messageId", info->directive->getMessageId()));
        sendExceptionEncounteredAndReportFailed(info, "missing URL");
        return;
    }

    if (!jsonUtils::retrieveValue(stream->value, "streamFormat", &audioItem.stream.format)) {
        // Some streams with attachments are missing a streamFormat field; assume AUDIO_MPEG.
        audioItem.stream.format = StreamFormat::AUDIO_MPEG;
    }

    if (audioItem.stream.url.compare(0, CID_PREFIX.size(), CID_PREFIX) == 0) {
        std::string contentId = audioItem.stream.url.substr(CID_PREFIX.length());
        audioItem.stream.reader = info->directive->getAttachmentReader(contentId, sds::ReaderPolicy::NONBLOCKING);
        if (nullptr == audioItem.stream.reader) {
            ACSDK_ERROR(LX("preHandlePlayDirectiveFailed")
                            .d("reason", "getAttachmentReaderFailed")
                            .d("messageId", info->directive->getMessageId()));
            sendExceptionEncounteredAndReportFailed(info, "unable to obtain attachment reader");
            return;
        }

        // TODO: Add a method to MediaPlayer to query whether a format is supported (ACSDK-416).
        if (audioItem.stream.format != StreamFormat::AUDIO_MPEG) {
            ACSDK_ERROR(LX("preHandlePlayDirectiveFailed")
                            .d("reason", "unsupportedFormat")
                            .d("format", audioItem.stream.format)
                            .d("messageId", info->directive->getMessageId()));
            std::string message = "unsupported format " + streamFormatToString(audioItem.stream.format);
            sendExceptionEncounteredAndReportFailed(info, message);
            return;
        }
    }

    int64_t milliseconds;
    if (jsonUtils::retrieveValue(stream->value, "offsetInMilliseconds", &milliseconds)) {
        audioItem.stream.offset = std::chrono::milliseconds(milliseconds);
    } else {
        audioItem.stream.offset = std::chrono::milliseconds::zero();
    }

    // TODO : ACSDK-387 should simplify this code
    // Note: expiryTime is provided by AVS, but no enforcement of it is required; capturing it here for completeness,
    //     but it is currently unused.
    std::string expiryTimeString;
    audioItem.stream.expiryTime = std::chrono::steady_clock::time_point::max();
    if (jsonUtils::retrieveValue(stream->value, "expiryTime", &expiryTimeString)) {
        int64_t unixTime;
        if (m_timeUtils.convert8601TimeStringToUnix(expiryTimeString, &unixTime)) {
            int64_t currentTime;
            if (m_timeUtils.getCurrentUnixTime(&currentTime)) {
                std::chrono::seconds timeToExpiry(unixTime - currentTime);
                audioItem.stream.expiryTime = std::chrono::steady_clock::now() + timeToExpiry;
            }
        }
    }

    rapidjson::Value::ConstMemberIterator progressReport;
    audioItem.stream.progressReport.delay = ProgressTimer::NO_DELAY;
    audioItem.stream.progressReport.interval = ProgressTimer::NO_INTERVAL;
    if (!jsonUtils::findNode(stream->value, "progressReport", &progressReport)) {
        progressReport = stream->value.MemberEnd();
    } else {
        if (jsonUtils::retrieveValue(progressReport->value, "progressReportDelayInMilliseconds", &milliseconds)) {
            audioItem.stream.progressReport.delay = std::chrono::milliseconds(milliseconds);
        }

        if (jsonUtils::retrieveValue(progressReport->value, "progressReportIntervalInMilliseconds", &milliseconds)) {
            audioItem.stream.progressReport.interval = std::chrono::milliseconds(milliseconds);
        }
    }

    if (!jsonUtils::retrieveValue(stream->value, "token", &audioItem.stream.token)) {
        audioItem.stream.token = "";
    }

    if (!jsonUtils::retrieveValue(stream->value, "expectedPreviousToken", &audioItem.stream.expectedPreviousToken)) {
        audioItem.stream.expectedPreviousToken = "";
    }

    m_executor.submit([this, info, playBehavior, audioItem] {
        auto messageId = info->directive->getMessageId();
        if (executeIsInPreHandlePlayInfoList(messageId)) {
            // There's is already a playInfo with the same messageId in the queue.
            ACSDK_ERROR(LX("preHandlePlayDirectiveFailed").d("reason", "messageIdAlreadyInPreHandleQueue"));
            sendExceptionEncounteredAndReportFailed(
                info, "duplicated messageId " + messageId, ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }

        auto playDirectiveInfo = std::make_shared<AudioPlayer::PlayDirectiveInfo>(messageId);
        playDirectiveInfo->audioItem = audioItem;
        playDirectiveInfo->playBehavior = playBehavior;
        executePrePlay(playDirectiveInfo);
    });
}

void AudioPlayer::handlePlayDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG1(LX("handlePlayDirective"));

    // Note: Unlike SpeechSynthesizer, AudioPlayer directives are instructing the client to start/stop/queue
    //     content, so directive handling is considered to be complete when we have queued the content for
    //     playback; we don't wait for playback to complete.
    setHandlingCompleted(info);

    auto messageId = info->directive->getMessageId();

    m_executor.submit([this, messageId] { executePlay(messageId); });
}

void AudioPlayer::handleStopDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG1(LX("handleStopDirective"));
    setHandlingCompleted(info);
    m_executor.submit([this] { executeStop(); });
}

void AudioPlayer::handleClearQueueDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG1(LX("handleClearQueue"));
    rapidjson::Document payload;
    if (!parseDirectivePayload(info, &payload)) {
        return;
    }

    ClearBehavior clearBehavior;
    if (!jsonUtils::retrieveValue(payload, "clearBehavior", &clearBehavior)) {
        clearBehavior = ClearBehavior::CLEAR_ENQUEUED;
    }

    setHandlingCompleted(info);
    m_executor.submit([this, clearBehavior] { executeClearQueue(clearBehavior); });
}

void AudioPlayer::removeDirective(std::shared_ptr<DirectiveInfo> info) {
    // Check result too, to catch cases where DirectiveInfo was created locally, without a nullptr result.
    // In those cases there is no messageId to remove because no result was expected.
    if (info->directive && info->result) {
        auto messageId = info->directive->getMessageId();
        CapabilityAgent::removeDirective(messageId);
    }
}

void AudioPlayer::setHandlingCompleted(std::shared_ptr<DirectiveInfo> info) {
    if (info && info->result) {
        info->result->setCompleted();
    }
    removeDirective(info);
}

void AudioPlayer::executeProvideState(bool sendToken, unsigned int stateRequestToken) {
    ACSDK_DEBUG(LX("executeProvideState").d("sendToken", sendToken).d("stateRequestToken", stateRequestToken));
    auto policy = StateRefreshPolicy::NEVER;
    if (PlayerActivity::PLAYING == m_currentActivity) {
        policy = StateRefreshPolicy::ALWAYS;
    }

    rapidjson::Document state(rapidjson::kObjectType);
    state.AddMember(TOKEN_KEY, m_token, state.GetAllocator());
    state.AddMember(
        OFFSET_KEY,
        (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(getOffset()).count(),
        state.GetAllocator());
    state.AddMember(ACTIVITY_KEY, playerActivityToString(m_currentActivity), state.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!state.Accept(writer)) {
        ACSDK_ERROR(LX("executeProvideState").d("reason", "writerRefusedJsonObject"));
        return;
    }

    SetStateResult result;
    if (sendToken) {
        result = m_contextManager->setState(STATE, buffer.GetString(), policy, stateRequestToken);
    } else {
        result = m_contextManager->setState(STATE, buffer.GetString(), policy);
    }
    if (result != SetStateResult::SUCCESS) {
        ACSDK_ERROR(LX("executeProvideState").d("reason", "contextManagerSetStateFailed").d("token", m_token));
    }
}

void AudioPlayer::executeOnFocusChanged(FocusState newFocus) {
    ACSDK_DEBUG1(
        LX("executeOnFocusChanged").d("from", m_focus).d("to", newFocus).d("m_currentActivity", m_currentActivity));
    if (m_focus == newFocus) {
        return;
    }
    m_focus = newFocus;

    switch (newFocus) {
        case FocusState::FOREGROUND:
            switch (m_currentActivity) {
                case PlayerActivity::IDLE:
                case PlayerActivity::STOPPED:
                case PlayerActivity::FINISHED:
                    // We see a focus change to foreground in these states if we are starting to play a new song.
                    if (!m_audioPlayQueue.empty()) {
                        ACSDK_DEBUG1(LX("executeOnFocusChanged").d("action", "playNextItem"));
                        playNextItem();
                    }

                    // If m_audioPlayQueue is empty and channel wasn't released, that means we are going to
                    // play the next item.

                    return;
                case PlayerActivity::PAUSED: {
                    // AudioPlayer is in the process of stopping.  So there's no need to resume playback for this case.
                    if (m_isStopCalled) {
                        ACSDK_DEBUG1(LX("executeOnFocusChanged").d("action", "stoppingAlreadyDoNothing"));
                        return;
                    }
                    // A focus change to foreground when paused means we should resume the current song.
                    ACSDK_DEBUG1(LX("executeOnFocusChanged").d("action", "resumeMediaPlayer"));
                    if (!m_mediaPlayer->resume(m_sourceId)) {
                        sendPlaybackFailedEvent(
                            m_token, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "failed to resume media player");
                        ACSDK_ERROR(LX("executeOnFocusChangedFailed").d("reason", "resumeFailed"));
                        m_focusManager->releaseChannel(CHANNEL_NAME, shared_from_this());
                        return;
                    }
                }
                    return;
                case PlayerActivity::PLAYING:
                case PlayerActivity::BUFFER_UNDERRUN:
                    // We should already have foreground focus in these states; break out to the warning below.
                    break;
            }
            break;
        case FocusState::BACKGROUND:
            switch (m_currentActivity) {
                case PlayerActivity::STOPPED:
                    // If we're stopping due to a new play and would have been continuing on to the next song, we want
                    // to block that.
                    if (m_playNextItemAfterStopped && !m_audioPlayQueue.empty()) {
                        m_playNextItemAfterStopped = false;
                        return;
                    }

                // We can also end up here with an empty queue if we've asked MediaPlayer to play, but playback
                // hasn't started yet, so we fall through to call @c pause() here as well.
                case PlayerActivity::FINISHED:
                case PlayerActivity::IDLE:
                // Note: can be in FINISHED or IDLE while waiting for MediaPlayer to start playing, so we fall
                // through to call @c pause() here as well.
                case PlayerActivity::PAUSED:
                // Note: can be in PAUSED while we're trying to resume, in which case we still want to pause, so we
                // fall through to call @c pause() here as well.
                case PlayerActivity::PLAYING:
                case PlayerActivity::BUFFER_UNDERRUN: {
                    // If we get pushed into the background while playing or buffering, pause the current song.
                    ACSDK_DEBUG1(LX("executeOnFocusChanged").d("action", "pauseMediaPlayer"));
                    // Note: Ignoring the return value of pause() here as we end up calling it in some cases where it is
                    //     not needed and MediaPlayer is not in a pausable state.  This is harmless, but we don't want
                    //     to be reporting errors in those cases.
                    // TODO: Consider expanding the states to track the transition to PLAYING so that we don't call
                    //       pause when we're genuinely IDLE/STOPPED/FINISHED (ACSDK-734).
                    m_mediaPlayer->pause(m_sourceId);
                }
                    return;
            }
            break;
        case FocusState::NONE:
            switch (m_currentActivity) {
                case PlayerActivity::IDLE:
                case PlayerActivity::STOPPED:
                case PlayerActivity::FINISHED:
                    // Nothing to more to do if we're already not playing; we got here because the act of stopping
                    // caused the channel to be released, which in turn caused this callback.
                    return;
                case PlayerActivity::PLAYING:
                case PlayerActivity::PAUSED:
                case PlayerActivity::BUFFER_UNDERRUN:
                    // If the focus change came in while we were in a 'playing' state, we need to stop because we are
                    // yielding the channel.
                    m_audioPlayQueue.clear();
                    ACSDK_DEBUG1(LX("executeOnFocusChanged").d("action", "executeStop"));
                    executeStop();
                    return;
            }
            break;
    }
    ACSDK_WARN(LX("unexpectedExecuteOnFocusChanged").d("newFocus", newFocus).d("m_currentActivity", m_currentActivity));
}

void AudioPlayer::executeOnPlaybackStarted(SourceId id) {
    ACSDK_DEBUG1(LX("executeOnPlaybackStarted").d("id", id));

    if (id != m_sourceId) {
        ACSDK_ERROR(LX("executeOnPlaybackStartedFailed")
                        .d("reason", "invalidSourceId")
                        .d("id", id)
                        .d("m_sourceId", m_sourceId));
        return;
    }

    m_isPreBuffering = false;

    // Race condition exists where focus can be lost before the executeOnPlaybackStarted callback.
    if (avsCommon::avs::FocusState::NONE == m_focus) {
        ACSDK_WARN(LX(__func__).d("reason", "callbackAfterFocusLost").d("action", "stopping"));
        if (!m_mediaPlayer->stop(m_sourceId)) {
            ACSDK_ERROR(LX(__func__).d("reason", "stopFailed"));
        }
    }

    /*
     * When @c AudioPlayer is the active player, @c PlaybackController which is
     * the default playback handler, should handle playback button presses.
     */
    m_playbackRouter->switchToDefaultHandler();
    changeActivity(PlayerActivity::PLAYING);

    sendPlaybackStartedEvent();
    m_progressTimer.start();
}

void AudioPlayer::executeOnPlaybackStopped(SourceId id) {
    ACSDK_DEBUG1(LX("executeOnPlaybackStopped").d("id", id));

    if (id != m_sourceId) {
        ACSDK_ERROR(LX("executeOnPlaybackStoppedFailed")
                        .d("reason", "invalidSourceId")
                        .d("id", id)
                        .d("m_sourceId", m_sourceId));
        return;
    }

    switch (m_currentActivity) {
        case PlayerActivity::PLAYING:
        case PlayerActivity::PAUSED:
        case PlayerActivity::BUFFER_UNDERRUN:
            changeActivity(PlayerActivity::STOPPED);
            m_progressTimer.stop();
            sendPlaybackStoppedEvent();
            m_isStopCalled = false;
            if (!m_playNextItemAfterStopped || (m_audioPlayQueue.empty() && m_preHandlePlayInfoList.empty())) {
                handlePlaybackCompleted();
            } else if (avsCommon::avs::FocusState::FOREGROUND == m_focus) {
                playNextItem();
            } else if (avsCommon::avs::FocusState::BACKGROUND == m_focus) {
                // Check if we have something to pre-buffer.
                if (!m_preHandlePlayInfoList.empty() && !m_isPreBuffering) {
                    auto playInfo = m_preHandlePlayInfoList.front();
                    if (playInfo) {
                        playInfo->sourceId = setSource(playInfo->audioItem);
                        if (ERROR_SOURCE_ID != playInfo->sourceId) {
                            m_isPreBuffering = true;
                            ACSDK_INFO(LX("executeOnPlaybackStoppedPreBuffering").d("id", playInfo->audioItem.id));
                        } else {
                            ACSDK_ERROR(LX("executeOnPlaybackStoppedFailed").d("reason", "SetSourceFailed"));
                        }
                    }
                }
            }
            return;
        case PlayerActivity::IDLE:
        case PlayerActivity::STOPPED:
        case PlayerActivity::FINISHED:
            /*
             * If playback failed before state changed to playing
             * this function is called and need to release focus.
             */
            if (m_focus != FocusState::NONE) {
                handlePlaybackCompleted();
                return;
            }
            ACSDK_ERROR(LX("executeOnPlaybackStoppedFailed")
                            .d("reason", "alreadyStopped")
                            .d("m_currentActivity", m_currentActivity));
            break;
    }
    ACSDK_ERROR(LX("executeOnPlaybackStoppedFailed")
                    .d("reason", "unexpectedActivity")
                    .d("m_currentActivity", m_currentActivity));
}

void AudioPlayer::executeOnPlaybackFinished(SourceId id) {
    ACSDK_DEBUG1(LX("executeOnPlaybackFinished").d("id", id));

    if (id != m_sourceId) {
        ACSDK_ERROR(LX("executeOnPlaybackFinishedFailed")
                        .d("reason", "invalidSourceId")
                        .d("id", id)
                        .d("m_sourceId", m_sourceId));
        return;
    }

    switch (m_currentActivity) {
        case PlayerActivity::PLAYING:
            changeActivity(PlayerActivity::FINISHED);
            m_progressTimer.stop();

            /*
             * We used to send PlaybackNearlyFinished right after we sent PlaybackStarted.  But we found a problem when
             * we are playing Audible such that after sending PlaybackNearlyFinished, AVS will send us the next item to
             * start buffering.  But since we don't actually access the url until we finish playing the current chapter,
             * by the time we open the url, the url has already expired so we got a 403 reponse. To address this
             * problem, we are sending the PlaybackNearlyFinished event just before we send PlaybackFinished.
             *
             * TODO: Once MediaPlayer can notify of nearly finished, send there instead (ACSDK-417).
             */
            sendPlaybackNearlyFinishedEvent();

            sendPlaybackFinishedEvent();
            if (m_audioPlayQueue.empty()) {
                handlePlaybackCompleted();
            } else {
                playNextItem();
            }
            return;
        case PlayerActivity::IDLE:
        case PlayerActivity::STOPPED:
        case PlayerActivity::PAUSED:
        case PlayerActivity::BUFFER_UNDERRUN:
        case PlayerActivity::FINISHED:
            ACSDK_ERROR(LX("executeOnPlaybackFinishedFailed")
                            .d("reason", "notPlaying")
                            .d("m_currentActivity", m_currentActivity));
            return;
    }
    ACSDK_ERROR(LX("executeOnPlaybackFinishedFailed")
                    .d("reason", "unexpectedActivity")
                    .d("m_currentActivity", m_currentActivity));
}

void AudioPlayer::handlePlaybackCompleted() {
    m_progressTimer.stop();
    if (m_focus != avsCommon::avs::FocusState::NONE) {
        m_focusManager->releaseChannel(CHANNEL_NAME, shared_from_this());
    }
}

void AudioPlayer::executeOnPlaybackError(SourceId id, const ErrorType& type, std::string error) {
    ACSDK_ERROR(LX("executeOnPlaybackError").d("id", id).d("type", type).d("error", error));

    if (id != m_sourceId) {
        ACSDK_ERROR(
            LX("executeOnPlaybackErrorFailed").d("reason", "invalidSourceId").d("id", id).d("m_sourceId", m_sourceId));
        return;
    }

    m_isPreBuffering = false;
    m_progressTimer.stop();
    sendPlaybackFailedEvent(m_token, type, error);

    /*
     * There's no need to call stop() here as the MediaPlayer has already stopped due to the playback error.  Instead,
     * call executeOnPlaybackStopped() so that the states in AudioPlayer are reset properly.
     */
    executeOnPlaybackStopped(m_sourceId);
}

void AudioPlayer::executeOnPlaybackPaused(SourceId id) {
    ACSDK_DEBUG1(LX("executeOnPlaybackPaused").d("id", id));

    if (id != m_sourceId) {
        ACSDK_ERROR(
            LX("executeOnPlaybackPausedFailed").d("reason", "invalidSourceId").d("id", id).d("m_sourceId", m_sourceId));
        return;
    }

    m_progressTimer.pause();
    // TODO: AVS recommends sending this after a recognize event to reduce latency (ACSDK-371).
    sendPlaybackPausedEvent();
    changeActivity(PlayerActivity::PAUSED);
}

void AudioPlayer::executeOnPlaybackResumed(SourceId id) {
    ACSDK_DEBUG1(LX("executeOnPlaybackResumed").d("id", id));

    if (id != m_sourceId) {
        ACSDK_ERROR(LX("executeOnPlaybackResumedFailed")
                        .d("reason", "invalidSourceId")
                        .d("id", id)
                        .d("m_sourceId", m_sourceId));
        return;
    }

    if (m_currentActivity == PlayerActivity::STOPPED) {
        ACSDK_ERROR(LX("executeOnPlaybackResumedAborted").d("reason", "currentActivity:STOPPED"));
        return;
    }

    sendPlaybackResumedEvent();
    m_progressTimer.resume();
    changeActivity(PlayerActivity::PLAYING);
}

void AudioPlayer::executeOnBufferUnderrun(SourceId id) {
    ACSDK_DEBUG1(LX("executeOnBufferUnderrun").d("id", id));

    if (id != m_sourceId) {
        ACSDK_ERROR(
            LX("executeOnBufferUnderrunFailed").d("reason", "invalidSourceId").d("id", id).d("m_sourceId", m_sourceId));
        return;
    }

    if (PlayerActivity::BUFFER_UNDERRUN == m_currentActivity) {
        ACSDK_ERROR(LX("executeOnBufferUnderrunFailed").d("reason", "alreadyInUnderrun"));
        return;
    }
    m_bufferUnderrunTimestamp = std::chrono::steady_clock::now();
    sendPlaybackStutterStartedEvent();
    changeActivity(PlayerActivity::BUFFER_UNDERRUN);
}

void AudioPlayer::executeOnBufferRefilled(SourceId id) {
    ACSDK_DEBUG1(LX("executeOnBufferRefilled").d("id", id));

    if (id != m_sourceId) {
        ACSDK_ERROR(
            LX("executeOnBufferRefilledFailed").d("reason", "invalidSourceId").d("id", id).d("m_sourceId", m_sourceId));
        return;
    }

    sendPlaybackStutterFinishedEvent();
    changeActivity(PlayerActivity::PLAYING);
}

void AudioPlayer::executeOnTags(SourceId id, std::shared_ptr<const VectorOfTags> vectorOfTags) {
    ACSDK_DEBUG1(LX("executeOnTags").d("id", id));

    if (id != m_sourceId) {
        ACSDK_ERROR(LX("executeOnTags").d("reason", "invalidSourceId").d("id", id).d("m_sourceId", m_sourceId));
        return;
    }

    sendStreamMetadataExtractedEvent(vectorOfTags);
}

bool AudioPlayer::executeShouldPreBufferInPreHandle(PlayBehavior playBehavior) {
    bool shouldPreBuffer = false;
    switch (m_currentActivity) {
        case PlayerActivity::PLAYING:
            break;

        case PlayerActivity::IDLE:
        case PlayerActivity::STOPPED:
        case PlayerActivity::FINISHED:
            shouldPreBuffer = !m_isPreBuffering || (PlayBehavior::REPLACE_ALL == playBehavior);
            break;

        case PlayerActivity::PAUSED:
            if (PlayBehavior::REPLACE_ALL == playBehavior) {
                // We shouldn't pre-buffer now for this case, but we should call executeStop now and play the next
                // item.  And in executeOnPlaybackStopped, if the focus is still in BACKGROUND, then we should start
                // buffering at that point.
                executeStop(true);
            }
            break;

        case PlayerActivity::BUFFER_UNDERRUN:
            break;
    }
    return shouldPreBuffer;
}

AudioPlayer::SourceId AudioPlayer::setSource(const AudioItem& audioItem) {
    ACSDK_DEBUG1(LX(__func__));
    auto sourceId = ERROR_SOURCE_ID;
    if (audioItem.stream.reader) {
        sourceId = m_mediaPlayer->setSource(audioItem.stream.reader);
        if (MediaPlayerInterface::ERROR == sourceId) {
            sendPlaybackFailedEvent(
                audioItem.stream.token,
                ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR,
                "failed to set attachment media source");
            ACSDK_ERROR(LX("setSourceFailed").d("reason", "setSourceFailed").d("type", "attachment"));
        }
    } else {
        ACSDK_DEBUG9(LX("settingUrlSource").d("offset", audioItem.stream.offset.count()));
        sourceId = m_mediaPlayer->setSource(audioItem.stream.url, audioItem.stream.offset);
        if (MediaPlayerInterface::ERROR == sourceId) {
            sendPlaybackFailedEvent(
                audioItem.stream.token, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "failed to set URL media source");
            ACSDK_ERROR(LX("setSourceFailed").d("reason", "setSourceFailed").d("type", "URL"));
        }
    }
    return sourceId;
}

bool AudioPlayer::executeIsInPreHandlePlayInfoList(const std::string& messageId) {
    for (const auto& info : m_preHandlePlayInfoList) {
        if (info && info->messageId == messageId) {
            return true;
        }
    }
    return false;
}

void AudioPlayer::executePrePlay(std::shared_ptr<AudioPlayer::PlayDirectiveInfo> info) {
    if (!info) {
        ACSDK_ERROR(LX("executePrePlayFailed").d("reason", "nullInfo"));
        return;
    }
    ACSDK_DEBUG1(LX(__func__).d("playBehavior", info->playBehavior).d("state", m_currentActivity));

    // Per AVS docs, drop/ignore AudioItems that specify an expectedPreviousToken which does not match the
    // current/previous token.
    if (!info->audioItem.stream.expectedPreviousToken.empty()) {
        std::string previousToken;
        if (!m_preHandlePlayInfoList.empty()) {
            const auto& lastPlayInfo = m_preHandlePlayInfoList.back();
            if (lastPlayInfo) {
                previousToken = lastPlayInfo->audioItem.stream.token;
            } else {
                ACSDK_ERROR(
                    LX("executePrePlayPreviousTokenNotFound").d("reason", "nullPlayInfoInPreHandlePlayInfoQueue"));
            }
        } else if (!m_audioPlayQueue.empty()) {
            const auto& lastPlayInfo = m_audioPlayQueue.back();
            if (lastPlayInfo) {
                previousToken = lastPlayInfo->audioItem.stream.token;
            } else {
                ACSDK_ERROR(LX("executePrePlayPreviousTokenNotFound").d("reason", "nullPlayInfoInAudioPlayQueue"));
            }
        } else {
            previousToken = m_token;
        }
        if (previousToken != info->audioItem.stream.expectedPreviousToken) {
            ACSDK_INFO(LX("executePrePlayDropped")
                           .d("reason", "unexpectedPreviousToken")
                           .d("previous", previousToken)
                           .d("expected", info->audioItem.stream.expectedPreviousToken));
            return;
        }
    }

    if (executeShouldPreBufferInPreHandle(info->playBehavior)) {
        info->sourceId = setSource(info->audioItem);
        if (info->sourceId != ERROR_SOURCE_ID) {
            ACSDK_INFO(LX("executePrePlayPreBuffering").d("id", info->audioItem.id));
            m_isPreBuffering = true;
        }
    }

    m_preHandlePlayInfoList.push_back(info);
}

void AudioPlayer::executePlay(const std::string& messageId) {
    ACSDK_DEBUG1(LX(__func__));
    if (m_preHandlePlayInfoList.empty()) {
        ACSDK_ERROR(LX("executePlayFailed").d("reason", "emptyPlayQueue"));
        return;
    }

    // If messageId is not found in set, just return
    if (!executeIsInPreHandlePlayInfoList(messageId)) {
        ACSDK_ERROR(LX("executePlayFailed").d("reason", "messageIdNotFoundInMap"));
        return;
    }

    std::shared_ptr<PlayDirectiveInfo> playInfo;

    // pop m_preHandlePlayInfoList until the playInfo with the matching messageId
    while (!m_preHandlePlayInfoList.empty()) {
        playInfo = m_preHandlePlayInfoList.front();
        m_preHandlePlayInfoList.pop_front();
        if (!playInfo) {
            ACSDK_ERROR(LX("executePlayError").d("reason", "NullPlayInfo"));
            continue;
        }

        if (playInfo->messageId == messageId) {
            break;
        }
        ACSDK_WARN(LX("executePlay").d("reason", "TrackNotHeadOfQueue"));
    }

    if (!playInfo || playInfo->messageId != messageId) {
        ACSDK_ERROR(LX("executePlayFailed").d("reason", "playInfoNotFound"));
        return;
    }

    // Do any playback/queue maintenance per playBehavior.
    switch (playInfo->playBehavior) {
        case PlayBehavior::REPLACE_ALL:
            // Note: this will change m_currentActivity to STOPPED.
            if (!m_isStopCalled) {
                executeStop(true);
            }
        // FALL-THROUGH
        case PlayBehavior::REPLACE_ENQUEUED:
            m_audioPlayQueue.clear();
        // FALL-THROUGH
        case PlayBehavior::ENQUEUE:
            m_audioPlayQueue.push_back(playInfo);
            break;
    }
    if (m_audioPlayQueue.empty()) {
        ACSDK_ERROR(
            LX("executePlayFailed").d("reason", "unhandledPlayBehavior").d("playBehavior", playInfo->playBehavior));
        return;
    }

    // Initiate playback if not already playing.
    switch (m_currentActivity) {
        case PlayerActivity::IDLE:
        case PlayerActivity::STOPPED:
        case PlayerActivity::FINISHED:
            if (FocusState::NONE == m_focus) {
                // If we don't currently have focus, acquire it now; playback will start when focus changes to
                // FOREGROUND.
                if (!m_focusManager->acquireChannel(CHANNEL_NAME, shared_from_this(), NAMESPACE)) {
                    ACSDK_ERROR(LX("executePlayFailed").d("reason", "CouldNotAcquireChannel"));
                    m_progressTimer.stop();
                    sendPlaybackFailedEvent(
                        m_token,
                        ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR,
                        std::string("Could not acquire ") + CHANNEL_NAME + " for " + NAMESPACE);
                    return;
                }
            }
            return;
        case PlayerActivity::PLAYING:
        case PlayerActivity::PAUSED:
        case PlayerActivity::BUFFER_UNDERRUN:
            // If we're already 'playing', the new song should have been enqueued above and there's nothing more to do
            // here.
            return;
    }
    ACSDK_ERROR(LX("executePlayFailed").d("reason", "unexpectedActivity").d("m_currentActivity", m_currentActivity));
}

void AudioPlayer::playNextItem() {
    ACSDK_DEBUG1(LX("playNextItem").d("m_audioPlayQueue.size", m_audioPlayQueue.size()));
    // Cancel any existing progress timer.  The new timer will start when playback starts.
    m_progressTimer.stop();
    if (m_audioPlayQueue.empty()) {
        sendPlaybackFailedEvent(m_token, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "queue is empty");
        ACSDK_ERROR(LX("playNextItemFailed").d("reason", "emptyQueue"));
        executeStop();
        return;
    }

    std::shared_ptr<PlayDirectiveInfo> info;
    do {
        info = m_audioPlayQueue.front();
        m_audioPlayQueue.pop_front();
    } while (!info && !m_audioPlayQueue.empty());

    if (!info) {
        ACSDK_ERROR(LX("playNextItemFailed").d("reason", "nullPlayInfo"));
        return;
    }

    auto item = info->audioItem;
    m_audioItemId = item.id;
    m_token = item.stream.token;
    m_initialOffset = item.stream.offset;
    m_sourceId = info->sourceId;

    if (MediaPlayerInterface::ERROR == m_sourceId) {
        m_sourceId = setSource(item);
        if (MediaPlayerInterface::ERROR == m_sourceId) {
            ACSDK_ERROR(LX("playNextItemFailed").d("reason", "setSourceFailed"));
            return;
        }
    }

    if (!m_mediaPlayer->play(m_sourceId)) {
        executeOnPlaybackError(m_sourceId, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "playFailed");
        return;
    }

    m_progressTimer.init(
        shared_from_this(), item.stream.progressReport.delay, item.stream.progressReport.interval, item.stream.offset);
}

void AudioPlayer::executeStop(bool playNextItem) {
    ACSDK_DEBUG1(LX("executeStop").d("playNextItem", playNextItem).d("m_currentActivity", m_currentActivity));
    switch (m_currentActivity) {
        case PlayerActivity::IDLE:
        case PlayerActivity::STOPPED:
        case PlayerActivity::FINISHED:
            // If we're already stopped, there's nothing more to do.
            return;
        case PlayerActivity::PLAYING:
        case PlayerActivity::PAUSED:
        case PlayerActivity::BUFFER_UNDERRUN:
            // Make sure we have the offset cached before stopping.
            getOffset();
            // Set a flag indicating what we want to do in the onPlaybackStopped() call.
            m_playNextItemAfterStopped = playNextItem;
            // Request to stop.
            if (!m_mediaPlayer->stop(m_sourceId)) {
                ACSDK_ERROR(LX("executeStopFailed").d("reason", "stopFailed"));
            } else {
                m_isStopCalled = true;
            }
            return;
    }
    ACSDK_ERROR(LX("executeStopFailed").d("reason", "unexpectedActivity").d("m_currentActivity", m_currentActivity));
}

void AudioPlayer::executeClearQueue(ClearBehavior clearBehavior) {
    ACSDK_DEBUG1(LX("executeClearQueue").d("clearBehavior", clearBehavior));
    switch (clearBehavior) {
        case ClearBehavior::CLEAR_ALL:
            executeStop();
        // FALL-THROUGH
        case ClearBehavior::CLEAR_ENQUEUED:
            m_audioPlayQueue.clear();
            sendPlaybackQueueClearedEvent();
            return;
    }
    ACSDK_ERROR(LX("executeClearQueueFailed").d("reason", "unexpectedClearBehavior").d("clearBehavior", clearBehavior));
}

void AudioPlayer::changeActivity(PlayerActivity activity) {
    ACSDK_DEBUG(LX("changeActivity").d("from", m_currentActivity).d("to", activity));
    {
        std::lock_guard<std::mutex> lock(m_currentActivityMutex);
        m_currentActivity = activity;
    }
    m_currentActivityConditionVariable.notify_all();
    executeProvideState();
    notifyObserver();
}

void AudioPlayer::sendEventWithTokenAndOffset(const std::string& eventName, std::chrono::milliseconds offset) {
    rapidjson::Document payload(rapidjson::kObjectType);
    payload.AddMember(TOKEN_KEY, m_token, payload.GetAllocator());
    // Note: offset is an optional parameter, which defaults to MEDIA_PLAYER_INVALID_OFFSET.  Per documentation, this
    //     function will use the current MediaPlayer offset is a valid offset was not provided.
    if (MEDIA_PLAYER_INVALID_OFFSET == offset) {
        offset = getOffset();
    }
    ACSDK_DEBUG1(LX("sendEventWithTokenAndOffset").d("eventName", eventName).d("offset", offset.count()));
    payload.AddMember(
        OFFSET_KEY,
        (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(offset).count(),
        payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX("sendEventWithTokenAndOffsetFailed").d("reason", "writerRefusedJsonObject"));
        return;
    }

    auto event = buildJsonEventString(eventName, "", buffer.GetString());
    auto request = std::make_shared<MessageRequest>(event.second);
    m_messageSender->sendMessage(request);
}

void AudioPlayer::sendPlaybackStartedEvent() {
    sendEventWithTokenAndOffset("PlaybackStarted", m_initialOffset);
}

void AudioPlayer::sendPlaybackNearlyFinishedEvent() {
    sendEventWithTokenAndOffset("PlaybackNearlyFinished");
}

void AudioPlayer::sendPlaybackStutterStartedEvent() {
    sendEventWithTokenAndOffset("PlaybackStutterStarted");
}

void AudioPlayer::sendPlaybackStutterFinishedEvent() {
    rapidjson::Document payload(rapidjson::kObjectType);
    payload.AddMember(TOKEN_KEY, m_token, payload.GetAllocator());
    payload.AddMember(
        OFFSET_KEY,
        (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(getOffset()).count(),
        payload.GetAllocator());
    auto stutterDuration = std::chrono::steady_clock::now() - m_bufferUnderrunTimestamp;
    payload.AddMember(
        STUTTER_DURATION_KEY,
        (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(stutterDuration).count(),
        payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX("sendPlaybackStutterFinishedEventFailed").d("reason", "writerRefusedJsonObject"));
        return;
    }

    auto event = buildJsonEventString("PlaybackStutterFinished", "", buffer.GetString());
    auto request = std::make_shared<MessageRequest>(event.second);
    m_messageSender->sendMessage(request);
}

void AudioPlayer::sendPlaybackFinishedEvent() {
    sendEventWithTokenAndOffset("PlaybackFinished");
}

void AudioPlayer::sendPlaybackFailedEvent(
    const std::string& failingToken,
    ErrorType errorType,
    const std::string& message) {
    rapidjson::Document payload(rapidjson::kObjectType);
    payload.AddMember(TOKEN_KEY, failingToken, payload.GetAllocator());

    rapidjson::Value currentPlaybackState(rapidjson::kObjectType);
    currentPlaybackState.AddMember(TOKEN_KEY, m_token, payload.GetAllocator());
    currentPlaybackState.AddMember(
        OFFSET_KEY,
        (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(getOffset()).count(),
        payload.GetAllocator());
    currentPlaybackState.AddMember(ACTIVITY_KEY, playerActivityToString(m_currentActivity), payload.GetAllocator());

    payload.AddMember("currentPlaybackState", currentPlaybackState, payload.GetAllocator());

    rapidjson::Value error(rapidjson::kObjectType);
    error.AddMember("type", errorTypeToString(errorType), payload.GetAllocator());
    error.AddMember("message", message, payload.GetAllocator());

    payload.AddMember("error", error, payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX("sendPlaybackStartedEventFailed").d("reason", "writerRefusedJsonObject"));
        return;
    }

    auto event = buildJsonEventString("PlaybackFailed", "", buffer.GetString());
    auto request = std::make_shared<MessageRequest>(event.second);
    m_messageSender->sendMessage(request);
}

void AudioPlayer::sendPlaybackStoppedEvent() {
    sendEventWithTokenAndOffset("PlaybackStopped");
}

void AudioPlayer::sendPlaybackPausedEvent() {
    sendEventWithTokenAndOffset("PlaybackPaused");
}

void AudioPlayer::sendPlaybackResumedEvent() {
    sendEventWithTokenAndOffset("PlaybackResumed");
}

void AudioPlayer::sendPlaybackQueueClearedEvent() {
    auto event = buildJsonEventString("PlaybackQueueCleared");
    auto request = std::make_shared<MessageRequest>(event.second);
    m_messageSender->sendMessage(request);
}

void AudioPlayer::sendStreamMetadataExtractedEvent(std::shared_ptr<const VectorOfTags> vectorOfTags) {
    rapidjson::Document payload(rapidjson::kObjectType);
    payload.AddMember(TOKEN_KEY, m_token, payload.GetAllocator());

    rapidjson::Value metadata(rapidjson::kObjectType);
    for (auto& tag : *vectorOfTags) {
        rapidjson::Value tagKey(tag.key.c_str(), payload.GetAllocator());
        if (TagType::BOOLEAN == tag.type) {
            if (!tag.value.compare("true")) {
                metadata.AddMember(tagKey, true, payload.GetAllocator());
            } else {
                metadata.AddMember(tagKey, false, payload.GetAllocator());
            }
        } else {
            rapidjson::Value tagValue(tag.value.c_str(), payload.GetAllocator());
            metadata.AddMember(tagKey, tagValue, payload.GetAllocator());
        }
    }
    payload.AddMember("metadata", metadata, payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX("sendStreamMetadataExtractedEvent").d("reason", "writerRefusedJsonObject"));
        return;
    }

    auto event = buildJsonEventString("StreamMetadataExtracted", "", buffer.GetString());
    auto request = std::make_shared<MessageRequest>(event.second);
    m_messageSender->sendMessage(request);
}

void AudioPlayer::notifyObserver() {
    avsCommon::sdkInterfaces::AudioPlayerObserverInterface::Context context;
    context.audioItemId = m_audioItemId;
    context.offset = getOffset();

    ACSDK_DEBUG1(LX("notifyObserver").d("playerActivity", playerActivityToString(m_currentActivity)));

    for (auto& observer : m_observers) {
        observer->onPlayerActivityChanged(m_currentActivity, context);
    }

    if (m_renderPlayerObserver) {
        avsCommon::sdkInterfaces::RenderPlayerInfoCardsObserverInterface::Context renderPlayerInfoContext;
        renderPlayerInfoContext.audioItemId = m_audioItemId;
        renderPlayerInfoContext.offset = getOffset();
        renderPlayerInfoContext.mediaProperties = shared_from_this();
        m_renderPlayerObserver->onRenderPlayerCardsInfoChanged(m_currentActivity, renderPlayerInfoContext);
    }
}

std::chrono::milliseconds AudioPlayer::getOffset() {
    // If the source id is not set, do not ask MediaPlayer for the offset.
    if (m_sourceId != ERROR_SOURCE_ID) {
        auto offset = m_mediaPlayer->getOffset(m_sourceId);
        if (offset != MEDIA_PLAYER_INVALID_OFFSET) {
            m_offset = offset;
        }
    }
    return m_offset;
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> AudioPlayer::
    getCapabilityConfigurations() {
    return m_capabilityConfigurations;
}

}  // namespace audioPlayer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
