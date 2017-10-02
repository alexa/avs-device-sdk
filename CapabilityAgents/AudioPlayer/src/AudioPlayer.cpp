/*
 * AudioPlayer.cpp
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

/// @file AudioPlayer.cpp

#include "AudioPlayer/AudioPlayer.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/error/en.h>

#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Timing/TimeUtils.h>

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

/// String to identify log entries originating from this file.
static const std::string TAG("AudioPlayer");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The name of the @c FocusManager channel used by @c AudioPlayer.
static const std::string CHANNEL_NAME = avsCommon::sdkInterfaces::FocusManagerInterface::CONTENT_CHANNEL_NAME;

/// The activityId string used with @c FocusManager by @c AudioPlayer.
static const std::string ACTIVITY_ID = "AudioPlayer.Play";

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

std::shared_ptr<AudioPlayer> AudioPlayer::create(
    std::shared_ptr<MediaPlayerInterface> mediaPlayer,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<FocusManagerInterface> focusManager,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<AttachmentManagerInterface> attachmentManager,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender) {
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
    } else if (nullptr == attachmentManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullAttachmentManager"));
        return nullptr;
    } else if (nullptr == exceptionSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionSender"));
        return nullptr;
    }

    auto audioPlayer = std::shared_ptr<AudioPlayer>(
        new AudioPlayer(mediaPlayer, messageSender, focusManager, contextManager, attachmentManager, exceptionSender));
    mediaPlayer->setObserver(audioPlayer);
    contextManager->setStateProvider(STATE, audioPlayer);
    return audioPlayer;
}

void AudioPlayer::provideState(unsigned int stateRequestToken) {
    m_executor.submit([this, stateRequestToken] { executeProvideState(true, stateRequestToken); });
}

void AudioPlayer::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void AudioPlayer::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    // TODO: Move as much processing up here as possilble (ACSDK415).
}

void AudioPlayer::handleDirective(std::shared_ptr<DirectiveInfo> info) {
    if (!info) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }
    if (info->directive->getName() == PLAY.name) {
        handlePlayDirective(info);
    } else if (info->directive->getName() == STOP.name) {
        handleStopDirective(info);
    } else if (info->directive->getName() == CLEAR_QUEUE.name) {
        handleClearQueueDirective(info);
    } else {
        m_executor.submit([this, info] {
            sendExceptionEncounteredAndReportFailed(
                info,
                "unexpected directive " + info->directive->getNamespace() + ":" + info->directive->getName(),
                ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        });
        ACSDK_ERROR(LX("handleDirectiveFailed")
                        .d("reason", "unknownDirective")
                        .d("namespace", info->directive->getNamespace())
                        .d("name", info->directive->getName()));
    }
}

void AudioPlayer::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
    removeDirective(info);
}

void AudioPlayer::onDeregistered() {
    executeStop();
    m_audioItems.clear();
}

DirectiveHandlerConfiguration AudioPlayer::getConfiguration() const {
    DirectiveHandlerConfiguration configuration;
    configuration[PLAY] = BlockingPolicy::NON_BLOCKING;
    configuration[STOP] = BlockingPolicy::NON_BLOCKING;
    configuration[CLEAR_QUEUE] = BlockingPolicy::NON_BLOCKING;
    return configuration;
}

void AudioPlayer::onFocusChanged(FocusState newFocus) {
    ACSDK_DEBUG9(LX("onFocusChanged").d("newFocus", newFocus));
    auto result = m_executor.submit([this, newFocus] { executeOnFocusChanged(newFocus); });
    if (result.wait_for(TIMEOUT) == std::future_status::timeout) {
        ACSDK_ERROR(LX("onFocusChangedFailed").d("reason", "timedout").d("cause", "executorTimeout"));
    }
}

void AudioPlayer::onPlaybackStarted() {
    ACSDK_DEBUG9(LX("onPlaybackStarted"));
    m_executor.submit([this] { executeOnPlaybackStarted(); });

    std::unique_lock<std::mutex> lock(m_playbackMutex);
    m_playbackStarted = true;
    m_playbackConditionVariable.notify_all();
}

void AudioPlayer::onPlaybackFinished() {
    ACSDK_DEBUG9(LX("onPlaybackFinished"));
    m_executor.submit([this] { executeOnPlaybackFinished(); });

    std::unique_lock<std::mutex> lock(m_playbackMutex);
    m_playbackFinished = true;
    m_playbackConditionVariable.notify_all();
}

void AudioPlayer::onPlaybackError(const ErrorType& type, std::string error) {
    ACSDK_DEBUG9(LX("onPlaybackError").d("type", type).d("error", error));
    m_executor.submit([this, type, error] { executeOnPlaybackError(type, error); });
}

void AudioPlayer::onPlaybackPaused() {
    ACSDK_DEBUG9(LX("onPlaybackPaused"));
    m_executor.submit([this] { executeOnPlaybackPaused(); });

    std::unique_lock<std::mutex> lock(m_playbackMutex);
    m_playbackPaused = true;
    m_playbackConditionVariable.notify_all();
}

void AudioPlayer::onPlaybackResumed() {
    ACSDK_DEBUG9(LX("onPlaybackResumed"));
    m_executor.submit([this] { executeOnPlaybackResumed(); });

    std::unique_lock<std::mutex> lock(m_playbackMutex);
    m_playbackResumed = true;
    m_playbackConditionVariable.notify_all();
}

void AudioPlayer::onBufferUnderrun() {
    ACSDK_DEBUG9(LX("onBufferUnderrun"));
    m_executor.submit([this] { executeOnBufferUnderrun(); });
}

void AudioPlayer::onBufferRefilled() {
    ACSDK_DEBUG9(LX("onBufferRefilled"));
    m_executor.submit([this] { executeOnBufferRefilled(); });
}

AudioPlayer::AudioPlayer(
    std::shared_ptr<MediaPlayerInterface> mediaPlayer,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<FocusManagerInterface> focusManager,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<AttachmentManagerInterface> attachmentManager,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender) :
        CapabilityAgent{NAMESPACE, exceptionSender},
        RequiresShutdown{"AudioPlayer"},
        m_mediaPlayer{mediaPlayer},
        m_messageSender{messageSender},
        m_focusManager{focusManager},
        m_contextManager{contextManager},
        m_attachmentManager{attachmentManager},
        m_playbackStarted{false},
        m_playbackPaused{false},
        m_playbackResumed{false},
        m_playbackFinished{false},
        m_currentActivity{PlayerActivity::IDLE},
        m_starting{false},
        m_focus{FocusState::NONE},
        m_offset{std::chrono::milliseconds{std::chrono::milliseconds::zero()}} {
}

void AudioPlayer::doShutdown() {
    m_executor.shutdown();
    executeStop();
    m_mediaPlayer->setObserver(nullptr);
    m_mediaPlayer.reset();
    m_messageSender.reset();
    m_focusManager.reset();
    m_contextManager->setStateProvider(STATE, nullptr);
    m_contextManager.reset();
    m_attachmentManager.reset();
    m_audioItems.clear();
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
    m_executor.submit([this, info] {
        sendExceptionEncounteredAndReportFailed(
            info, "Unable to parse payload", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
    });
    return false;
}

void AudioPlayer::handlePlayDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX("handlePlayDirective"));
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
        ACSDK_ERROR(LX("handlePlayDirectiveFailed")
                        .d("reason", "missingAudioItem")
                        .d("messageId", info->directive->getMessageId()));
        m_executor.submit([this, info] { sendExceptionEncounteredAndReportFailed(info, "missing AudioItem"); });
        return;
    }

    AudioItem audioItem;
    if (!jsonUtils::retrieveValue(audioItemJson->value, "audioItemId", &audioItem.id)) {
        audioItem.id = "anonymous";
    }

    rapidjson::Value::ConstMemberIterator stream;
    if (!jsonUtils::findNode(audioItemJson->value, "stream", &stream)) {
        ACSDK_ERROR(LX("handlePlayDirectiveFailed")
                        .d("reason", "missingStream")
                        .d("messageId", info->directive->getMessageId()));
        m_executor.submit([this, info] { sendExceptionEncounteredAndReportFailed(info, "missing stream"); });
        return;
    }

    if (!jsonUtils::retrieveValue(stream->value, "url", &audioItem.stream.url)) {
        ACSDK_ERROR(
            LX("handlePlayDirectiveFailed").d("reason", "missingUrl").d("messageId", info->directive->getMessageId()));
        m_executor.submit([this, info] { sendExceptionEncounteredAndReportFailed(info, "missing URL"); });
        return;
    }

    if (!jsonUtils::retrieveValue(stream->value, "streamFormat", &audioItem.stream.format)) {
        // Some streams with attachments are missing a streamFormat field; assume AUDIO_MPEG.
        audioItem.stream.format = StreamFormat::AUDIO_MPEG;
    }

    if (audioItem.stream.url.compare(0, CID_PREFIX.size(), CID_PREFIX) == 0) {
        std::string contentId = audioItem.stream.url.substr(CID_PREFIX.length());
        audioItem.stream.reader = info->directive->getAttachmentReader(contentId, AttachmentReader::Policy::BLOCKING);
        if (nullptr == audioItem.stream.reader) {
            ACSDK_ERROR(LX("handlePlayDirectiveFailed")
                            .d("reason", "getAttachmentReaderFailed")
                            .d("messageId", info->directive->getMessageId()));
            m_executor.submit(
                [this, info] { sendExceptionEncounteredAndReportFailed(info, "unable to obtain attachment reader"); });
            return;
        }

        // TODO: Add a method to MediaPlayer to query whether a format is supported (ACSDK-416).
        if (audioItem.stream.format != StreamFormat::AUDIO_MPEG) {
            ACSDK_ERROR(LX("handlePlayDirectiveFailed")
                            .d("reason", "unsupportedFormat")
                            .d("format", audioItem.stream.format)
                            .d("messageId", info->directive->getMessageId()));
            std::string message = "unsupported format " + streamFormatToString(audioItem.stream.format);
            m_executor.submit([this, info, message] { sendExceptionEncounteredAndReportFailed(info, message); });
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
        if (timing::convert8601TimeStringToUnix(expiryTimeString, &unixTime)) {
            int64_t currentTime;
            if (timing::getCurrentUnixTime(&currentTime)) {
                std::chrono::seconds timeToExpiry(unixTime - currentTime);
                audioItem.stream.expiryTime = std::chrono::steady_clock::now() + timeToExpiry;
            }
        }
    }

    rapidjson::Value::ConstMemberIterator progressReport;
    audioItem.stream.progressReport.delay = std::chrono::milliseconds::max();
    audioItem.stream.progressReport.interval = std::chrono::milliseconds::max();
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
        executePlay(playBehavior, audioItem);

        // Note: Unlike SpeechSynthesizer, AudioPlayer directives are instructing the client to start/stop/queue
        //     content, so directive handling is considered to be complete when we have queued the content for
        //     playback; we don't wait for playback to complete.
        setHandlingCompleted(info);
    });
}

void AudioPlayer::handleStopDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX("handleStopDirective"));
    m_executor.submit([this, info] {
        setHandlingCompleted(info);
        executeStop();
    });
}

void AudioPlayer::handleClearQueueDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX("handleClearQueue"));
    rapidjson::Document payload;
    if (!parseDirectivePayload(info, &payload)) {
        return;
    }

    ClearBehavior clearBehavior;
    if (!jsonUtils::retrieveValue(payload, "clearBehavior", &clearBehavior)) {
        clearBehavior = ClearBehavior::CLEAR_ENQUEUED;
    }

    m_executor.submit([this, info, clearBehavior] {
        setHandlingCompleted(info);
        executeClearQueue(clearBehavior);
    });
}

void AudioPlayer::removeDirective(std::shared_ptr<DirectiveInfo> info) {
    // Check result too, to catch cases where DirectiveInfo was created locally, without a nullptr result.
    // In those cases there is no messageId to remove because no result was expected.
    if (info->directive && info->result) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
    }
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
        OFFSET_KEY, std::chrono::duration_cast<std::chrono::milliseconds>(getOffset()).count(), state.GetAllocator());
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
    ACSDK_DEBUG9(LX("executeOnFocusChanged")
                     .d("from", m_focus)
                     .d("to", newFocus)
                     .d("m_starting", m_starting)
                     .d("m_currentActivity", m_currentActivity));
    if (m_focus == newFocus) {
        return;
    }
    m_focus = newFocus;

    switch (newFocus) {
        case FocusState::FOREGROUND:
            if (m_starting) {
                std::unique_lock<std::mutex> lock(m_playbackMutex);
                m_playbackStarted = false;
                ACSDK_DEBUG9(LX("executeOnFocusChanged").d("action", "playNextItem"));
                playNextItem();
                if (!m_playbackConditionVariable.wait_for(lock, TIMEOUT, [this] { return m_playbackStarted; })) {
                    ACSDK_ERROR(LX("onFocusChangedFailed").d("reason", "timedout").d("cause", "notStarted"));
                }
            } else if (PlayerActivity::PAUSED == m_currentActivity) {
                std::unique_lock<std::mutex> lock(m_playbackMutex);
                m_playbackResumed = false;
                ACSDK_DEBUG9(LX("executeOnFocusChanged").d("action", "resumeMediaPlayer"));
                if (m_mediaPlayer->resume() == MediaPlayerStatus::FAILURE) {
                    sendPlaybackFailedEvent(
                        m_token, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "failed to resume media player");
                    ACSDK_ERROR(LX("executeOnFocusChangedFailed").d("reason", "resumeFailed"));
                    m_focusManager->releaseChannel(CHANNEL_NAME, shared_from_this());
                    return;
                }
                if (!m_playbackConditionVariable.wait_for(lock, TIMEOUT, [this] { return m_playbackResumed; })) {
                    sendPlaybackFailedEvent(
                        m_token, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "resume media player timed out");
                    ACSDK_ERROR(LX("onFocusChangedFailed").d("reason", "timedOut").d("cause", "notResumed"));
                    m_focusManager->releaseChannel(CHANNEL_NAME, shared_from_this());
                }
            } else {
                ACSDK_DEBUG9(LX("executeOnFocusChanged").d("action", "none").d("m_currentActivity", m_currentActivity));
            }
            break;
        case FocusState::BACKGROUND:
            if (PlayerActivity::PLAYING == m_currentActivity) {
                std::unique_lock<std::mutex> lock(m_playbackMutex);
                m_playbackPaused = false;
                ACSDK_DEBUG9(LX("executeOnFocusChanged").d("action", "pauseMediaPlayer"));
                if (m_mediaPlayer->pause() == MediaPlayerStatus::FAILURE) {
                    sendPlaybackFailedEvent(
                        m_token, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "failed to pause media player");
                    ACSDK_ERROR(LX("executeOnFocusChangedFailed").d("reason", "pauseFailed"));
                    return;
                }
                if (!m_playbackConditionVariable.wait_for(lock, TIMEOUT, [this] { return m_playbackPaused; })) {
                    sendPlaybackFailedEvent(
                        m_token, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "pause media player timed out");
                    ACSDK_ERROR(LX("onFocusChangedFailed").d("reason", "timedOut").d("cause", "notPaused"));
                }
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
                    // If The focus change came in while we were in a 'playing' state, we need to stop because we are
                    // yielding the channel.
                    break;
            }

            m_audioItems.clear();

            std::unique_lock<std::mutex> lock(m_playbackMutex);
            m_playbackFinished = false;

            /* Note: MediaPlayer::stop() calls onPlaybackFinished() synchronously, which results in a mutex deadlock
             * here if the lock his held for the executeStop() call.  Releasing the lock temporarily avoids the
             * deadlock.  If MediaPlayer is changed in the future to asynchronously call onPlaybackFinished (and
             * documented as such in MediaPlayerInterface), the unlock/lock calls can be removed. */
            lock.unlock();
            ACSDK_DEBUG9(LX("executeOnFocusChanged").d("action", "executeStop"));
            executeStop();
            lock.lock();

            if (!m_playbackConditionVariable.wait_for(lock, TIMEOUT, [this] { return m_playbackFinished; })) {
                ACSDK_ERROR(LX("onFocusChangedFailed").d("reason", "timedout").d("cause", "notFinished"));
            }
            break;
    }
}

void AudioPlayer::executeOnPlaybackStarted() {
    changeActivity(PlayerActivity::PLAYING);

    sendPlaybackStartedEvent();

    // TODO: Once MediaPlayer can notify of nearly finished, send there instead (ACSDK-417).
    sendPlaybackNearlyFinishedEvent();
}

void AudioPlayer::executeOnPlaybackFinished() {
    ACSDK_DEBUG9(LX("executeOnPlaybackFinished"));
    if (m_currentActivity != PlayerActivity::PLAYING) {
        ACSDK_ERROR(
            LX("executeOnPlaybackFinishedError").d("reason", "notPlaying").d("m_currentActivity", m_currentActivity));
        return;
    }

    if (m_audioItems.empty()) {
        changeActivity(PlayerActivity::FINISHED);
        sendPlaybackFinishedEvent();
        m_focusManager->releaseChannel(CHANNEL_NAME, shared_from_this());
        return;
    }
    sendPlaybackFinishedEvent();
    playNextItem();
}

void AudioPlayer::executeOnPlaybackError(const ErrorType& type, std::string error) {
    ACSDK_ERROR(LX("executeOnPlaybackError").d("type", type).d("error", error));
    sendPlaybackFailedEvent(m_token, type, error);
    executeStop();
}

void AudioPlayer::executeOnPlaybackPaused() {
    ACSDK_DEBUG9(LX("executeOnPlaybackPaused"));
    // TODO: AVS recommends sending this after a recognize event to reduce latency (ACSDK-371).
    sendPlaybackPausedEvent();
    changeActivity(PlayerActivity::PAUSED);
}

void AudioPlayer::executeOnPlaybackResumed() {
    ACSDK_DEBUG9(LX("executeOnPlaybackResumed"));
    if (m_currentActivity == PlayerActivity::STOPPED) {
        ACSDK_ERROR(LX("executeOnPlaybackResumedAborted").d("reason", "currentActivity:STOPPED"));
        return;
    }

    sendPlaybackResumedEvent();
    changeActivity(PlayerActivity::PLAYING);
}

void AudioPlayer::executeOnBufferUnderrun() {
    ACSDK_DEBUG9(LX("executeOnBufferUnderrun"));
    if (PlayerActivity::BUFFER_UNDERRUN == m_currentActivity) {
        ACSDK_ERROR(LX("executeOnBufferUnderrunFailed").d("reason", "alreadyInUnderrun"));
        return;
    }
    m_bufferUnderrunTimestamp = std::chrono::steady_clock::now();
    sendPlaybackStutterStartedEvent();
    changeActivity(PlayerActivity::BUFFER_UNDERRUN);
}

void AudioPlayer::executeOnBufferRefilled() {
    ACSDK_DEBUG9(LX("executeOnBufferRefilled"));
    sendPlaybackStutterFinishedEvent();
    changeActivity(PlayerActivity::PLAYING);
}

void AudioPlayer::executePlay(PlayBehavior playBehavior, const AudioItem& audioItem) {
    ACSDK_DEBUG9(LX("executePlay").d("playBehavior", playBehavior));

    switch (playBehavior) {
        case PlayBehavior::REPLACE_ALL:
            executeStop(false);
        // FALL-THROUGH
        case PlayBehavior::REPLACE_ENQUEUED:
            m_audioItems.clear();
        // FALL-THROUGH
        case PlayBehavior::ENQUEUE:
            // Per AVS docs, drop/ignore AudioItems that specify an expectedPreviousToken which does not match the
            // current/previous token
            if (!audioItem.stream.expectedPreviousToken.empty()) {
                auto previousToken = m_audioItems.empty() ? m_token : m_audioItems.back().stream.token;
                if (previousToken != audioItem.stream.expectedPreviousToken) {
                    ACSDK_INFO(LX("executePlayDropped")
                                   .d("reason", "unexpectedPreviousToken")
                                   .d("previous", previousToken)
                                   .d("expected", audioItem.stream.expectedPreviousToken));
                    return;
                }
            }
            m_audioItems.push_back(audioItem);
            break;
    }

    if (m_audioItems.empty()) {
        ACSDK_ERROR(LX("executePlayFailed").d("reason", "unhandledPlayBehavior").d("playBehavior", playBehavior));
        return;
    }

    if (m_starting || PlayerActivity::PLAYING == m_currentActivity) {
        return;
    }

    if (FocusState::FOREGROUND == m_focus) {
        playNextItem();
    } else if (!m_focusManager->acquireChannel(CHANNEL_NAME, shared_from_this(), ACTIVITY_ID)) {
        ACSDK_ERROR(LX("executePlayFailed").d("reason", "CouldNotAcquireChannel"));
        sendPlaybackFailedEvent(
            m_token,
            ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR,
            std::string("Could not acquire ") + CHANNEL_NAME + " for " + ACTIVITY_ID);
        return;
    }

    m_starting = true;
}

void AudioPlayer::playNextItem() {
    ACSDK_DEBUG9(LX("playNextItem").d("m_audioItems.size", m_audioItems.size()));
    if (m_audioItems.empty()) {
        sendPlaybackFailedEvent(m_token, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "queue is empty");
        ACSDK_ERROR(LX("playNextItemFailed").d("reason", "emptyQueue"));
        executeStop();
        return;
    }

    auto item = m_audioItems.front();
    m_audioItems.pop_front();
    m_token = item.stream.token;

    if (item.stream.reader) {
        if (m_mediaPlayer->setSource(std::move(item.stream.reader)) == MediaPlayerStatus::FAILURE) {
            sendPlaybackFailedEvent(
                m_token, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "failed to set attachment media source");
            ACSDK_ERROR(LX("playNextItemFailed").d("reason", "setSourceFailed").d("type", "attachment"));
            return;
        }
    } else if (m_mediaPlayer->setSource(item.stream.url) == MediaPlayerStatus::FAILURE) {
        sendPlaybackFailedEvent(
            m_token, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "failed to set URL media source");
        ACSDK_ERROR(LX("playNextItemFailed").d("reason", "setSourceFailed").d("type", "URL"));
        return;
    }

    ACSDK_DEBUG9(LX("playNextItem").d("item.stream.offset", item.stream.offset.count()));
    if (item.stream.offset.count() && m_mediaPlayer->setOffset(item.stream.offset) == MediaPlayerStatus::FAILURE) {
        sendPlaybackFailedEvent(m_token, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "failed to set stream offset");
        ACSDK_ERROR(LX("playNextItemFailed").d("reason", "setOffsetFailed"));
        return;
    }

    if (m_mediaPlayer->play() == MediaPlayerStatus::FAILURE) {
        executeOnPlaybackError(ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "playFailed");
        return;
    }
    if (std::chrono::milliseconds::max() != item.stream.progressReport.delay) {
        m_delayTimer.start(item.stream.progressReport.delay - item.stream.offset, [this] {
            m_executor.submit([this] { sendProgressReportDelayElapsedEvent(); });
        });
    }
    if (std::chrono::milliseconds::max() != item.stream.progressReport.interval) {
        m_intervalTimer.start(
            item.stream.progressReport.interval - item.stream.offset,
            item.stream.progressReport.interval,
            timing::Timer::PeriodType::ABSOLUTE,
            timing::Timer::FOREVER,
            [this] { m_executor.submit([this] { sendProgressReportIntervalElapsedEvent(); }); });
    }
}

void AudioPlayer::executeStop(bool releaseFocus) {
    ACSDK_DEBUG9(LX("executeStop").d("m_currentActivity", m_currentActivity));
    auto stopStatus = MediaPlayerStatus::SUCCESS;
    switch (m_currentActivity) {
        case PlayerActivity::IDLE:
        case PlayerActivity::STOPPED:
            if (m_starting) {
                break;
            } else {
                return;
            }
        // FALL-THROUGH
        case PlayerActivity::PLAYING:
        case PlayerActivity::PAUSED:
        case PlayerActivity::BUFFER_UNDERRUN:
            getOffset();
            stopStatus = m_mediaPlayer->stop();
            break;
        default:
            break;
    }

    m_starting = false;
    m_delayTimer.stop();
    m_intervalTimer.stop();
    if (releaseFocus && m_focus != avsCommon::avs::FocusState::NONE) {
        m_focusManager->releaseChannel(CHANNEL_NAME, shared_from_this());
    }
    changeActivity(PlayerActivity::STOPPED);
    if (MediaPlayerStatus::FAILURE == stopStatus) {
        executeOnPlaybackError(ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "mediaPlayerStopFailed");
    }
    sendPlaybackStoppedEvent();
}

void AudioPlayer::executeClearQueue(ClearBehavior clearBehavior) {
    switch (clearBehavior) {
        case ClearBehavior::CLEAR_ALL:
            executeStop();
        // FALL-THROUGH
        case ClearBehavior::CLEAR_ENQUEUED:
            m_audioItems.clear();
            break;
    }
    sendPlaybackQueueClearedEvent();
}

void AudioPlayer::changeActivity(PlayerActivity activity) {
    ACSDK_DEBUG(LX("changeActivity").d("from", m_currentActivity).d("to", activity));
    m_starting = false;
    m_currentActivity = activity;
    executeProvideState();
}

void AudioPlayer::setHandlingCompleted(std::shared_ptr<DirectiveInfo> info) {
    if (info && info->result) {
        info->result->setCompleted();
    }
    removeDirective(info);
}

void AudioPlayer::sendExceptionEncounteredAndReportFailed(
    std::shared_ptr<DirectiveInfo> info,
    const std::string& message,
    avsCommon::avs::ExceptionErrorType type) {
    m_exceptionEncounteredSender->sendExceptionEncountered(info->directive->getUnparsedDirective(), type, message);
    if (info && info->result) {
        info->result->setFailed(message);
    }
    removeDirective(info);
}

void AudioPlayer::sendEventWithTokenAndOffset(const std::string& eventName) {
    rapidjson::Document payload(rapidjson::kObjectType);
    payload.AddMember(TOKEN_KEY, m_token, payload.GetAllocator());
    payload.AddMember(
        OFFSET_KEY, std::chrono::duration_cast<std::chrono::milliseconds>(getOffset()).count(), payload.GetAllocator());

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
    sendEventWithTokenAndOffset("PlaybackStarted");
}

void AudioPlayer::sendPlaybackNearlyFinishedEvent() {
    sendEventWithTokenAndOffset("PlaybackNearlyFinished");
}

void AudioPlayer::sendProgressReportDelayElapsedEvent() {
    sendEventWithTokenAndOffset("ProgressReportDelayElapsed");
}

void AudioPlayer::sendProgressReportIntervalElapsedEvent() {
    sendEventWithTokenAndOffset("ProgressReportIntervalElapsed");
}

void AudioPlayer::sendPlaybackStutterStartedEvent() {
    sendEventWithTokenAndOffset("PlaybackStutterStarted");
}

void AudioPlayer::sendPlaybackStutterFinishedEvent() {
    rapidjson::Document payload(rapidjson::kObjectType);
    payload.AddMember(TOKEN_KEY, m_token, payload.GetAllocator());
    payload.AddMember(
        OFFSET_KEY, std::chrono::duration_cast<std::chrono::milliseconds>(getOffset()).count(), payload.GetAllocator());
    auto stutterDuration = std::chrono::steady_clock::now() - m_bufferUnderrunTimestamp;
    payload.AddMember(
        STUTTER_DURATION_KEY,
        std::chrono::duration_cast<std::chrono::milliseconds>(stutterDuration).count(),
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
        OFFSET_KEY, std::chrono::duration_cast<std::chrono::milliseconds>(getOffset()).count(), payload.GetAllocator());
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

void AudioPlayer::sendStreamMetadataExtractedEvent() {
    // TODO: Implement/call this once MediaPlayer exports metadata info (ACSDK-414).
}

std::chrono::milliseconds AudioPlayer::getOffset() {
    if (PlayerActivity::PLAYING != m_currentActivity) {
        return m_offset;
    }
    m_offset = std::chrono::milliseconds(m_mediaPlayer->getOffset());

    if (m_offset == MEDIA_PLAYER_INVALID_OFFSET) {
        m_offset = std::chrono::milliseconds::zero();
    }
    return m_offset;
}

}  // namespace audioPlayer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
