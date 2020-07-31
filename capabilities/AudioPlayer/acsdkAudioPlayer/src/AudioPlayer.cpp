/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * SPDX-License-Identifier: LicenseRef-.amazon.com.-ASL-1.0
 * Licensed under the Amazon Software License (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/asl/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/// @file AudioPlayer.cpp
#include <algorithm>
#include <cctype>

#include "acsdkAudioPlayer/AudioPlayer.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/error/en.h>

#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/Audio/MixingBehavior.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>
#include <AVSCommon/Utils/Metrics/DataPointStringBuilder.h>
#include <AVSCommon/Utils/Metrics/DataPointCounterBuilder.h>

namespace alexaClientSDK {
namespace acsdkAudioPlayer {

using namespace acsdkAudioPlayerInterfaces;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::json;
using namespace avsCommon::utils::logger;
using namespace avsCommon::utils::mediaPlayer;
using namespace avsCommon::utils::metrics;
using MediaPlayerState = avsCommon::utils::mediaPlayer::MediaPlayerState;

/// AudioPlayer capability constants
/// AudioPlayer interface type
static const std::string AUDIOPLAYER_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";
/// AudioPlayer interface name
static const std::string AUDIOPLAYER_CAPABILITY_INTERFACE_NAME = "AudioPlayer";
/// AudioPlayer interface version
static const std::string AUDIOPLAYER_CAPABILITY_INTERFACE_VERSION = "1.4";

/// The fingerprint key used in @c AudioPlayer configurations.
static const std::string FINGERPRINT_KEY = "fingerprint";

/// The package key used in @c AudioPlayer fingerprint.
static const std::string PACKAGE_KEY = "package";

/// The buildType key used in @c AudioPlayer fingerprint.
static const std::string BUILD_TYPE_KEY = "buildType";

/// The versionNumber key used in @c AudioPlayer fingerprint.
static const std::string VERSION_NUMBER_KEY = "versionNumber";

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
static const std::string CHANNEL_NAME = FocusManagerInterface::CONTENT_CHANNEL_NAME;

/// The namespace for this capability agent.
static const std::string NAMESPACE = "AudioPlayer";

/// The @c Play directive signature.
static const NamespaceAndName PLAY{NAMESPACE, "Play"};

/// The @c Stop directive signature.
static const NamespaceAndName STOP{NAMESPACE, "Stop"};

/// The @c ClearQueue directive signature.
static const NamespaceAndName CLEAR_QUEUE{NAMESPACE, "ClearQueue"};

/// The @c UpdateProgressReportInterval directive signature.
static const NamespaceAndName UPDATE_PROGRESS_REPORT_INTERVAL{NAMESPACE, "UpdateProgressReportInterval"};

/// The @c AudioPlayer context state signature.
static const NamespaceAndName STATE{NAMESPACE, "PlaybackState"};

/// Prefix for content ID prefix in the url property of the directive payload.
static const std::string CID_PREFIX{"cid:"};

/// The token key used in @c AudioPlayer events.
static const char TOKEN_KEY[] = "token";

/// The offset key used in @c AudioPlayer events.
static const char OFFSET_KEY[] = "offsetInMilliseconds";

/// The seek start offset key used in @c PlaybackSeeked events.
static const char SEEK_START_OFFSET_KEY[] = "seekStartOffsetInMilliseconds";

/// The seek end offset key used in @c PlaybackSeeked events.
static const char SEEK_END_OFFSET_KEY[] = "seekEndOffsetInMilliseconds";

/// The playbackAttributes key used in @c AudioPlayer events.
static const char PLAYBACK_ATTRIBUTES_KEY[] = "playbackAttributes";

/// The name key used in @c AudioPlayer events.
static const char NAME_KEY[] = "name";

/// The codec key used in @c AudioPlayer events.
static const char CODEC_KEY[] = "codec";

/// The samplingRateInHertz key used in @c AudioPlayer events.
static const char SAMPLING_RATE_IN_HERTZ_KEY[] = "samplingRateInHertz";

/// The dateRateInBitsPerSecond key used in @c AudioPlayer events.
static const char DATA_RATE_IN_BITS_PER_SECOND_KEY[] = "dataRateInBitsPerSecond";

/// The playbackReports key used in @c AudioPlayer events.
static const char PLAYBACK_REPORTS_KEY[] = "playbackReports";

/// The startOffsetInMilliseconds key used in @c AudioPlayer events.
static const char START_OFFSET_KEY[] = "startOffsetInMilliseconds";

/// The endOffsetInMilliseconds key used in @c AudioPlayer events.
static const char END_OFFSET_KEY[] = "endOffsetInMilliseconds";

/// The activity key used in @c AudioPlayer events.
static const char ACTIVITY_KEY[] = "playerActivity";

/// The key for the "captionData" property in the directive payload.
static const char CAPTION_KEY[] = "caption";

/// The key under "captionData" containing the caption type
static const char CAPTION_TYPE_KEY[] = "type";

/// The key under "captionData" containing the caption content
static const char CAPTION_CONTENT_KEY[] = "content";

/// The stutter key used in @c AudioPlayer events.
static const char STUTTER_DURATION_KEY[] = "stutterDurationInMilliseconds";

/// The duration to wait for a state change in @c onFocusChanged before failing.
static const std::chrono::seconds TIMEOUT{2};

/// default mixing behavior
static const audio::MixingBehavior DEFAULT_MIXING_BEHAVIOR = audio::MixingBehavior::BEHAVIOR_PAUSE;

/// prefix for metrics emitted from the AudioPlayer CA
static const std::string AUDIO_PLAYER_METRIC_PREFIX = "AUDIO_PLAYER-";

/// Metric to catch unsupported/missing interruptedBehavior
static const std::string UNSUPPORTED_INTERRUPTED_BEHAVIOR = "UNSUPPORTED_INTERRUPTED_BEHAVIOR_PLAY_DIRECTIVE";
static const std::string MISSING_INTERRUPTED_BEHAVIOR = "MISSING_INTERRUPTED_BEHAVIOR_PLAY_DIRECTIVE";

/// playback metrics use for AudioPlayer UPL

/// The directive received metric is used in place of 'first bytes' for UPL because pre-buffering may distort the metric
static const std::string PREPARE_PLAY_DIRECTIVE_RECEIVED = "PREPARE_PLAY_DIRECTIVE_RECEIVED";

/// The directive received metric is used in place of 'first bytes' for UPL because pre-buffering may distort the metric
static const std::string PLAY_DIRECTIVE_RECEIVED = "PLAY_DIRECTIVE_RECEIVED";

/// The directive received metric is used in place of 'first bytes' for Stop UPL because there are no bytes involved
static const std::string STOP_DIRECTIVE_RECEIVED = "STOP_DIRECTIVE_RECEIVED";

/// Metric used to trigger UPL recording for play
static const std::string PLAYBACK_STARTED = "PLAYBACK_STARTED";

/// Metric used to trigger UPL recording for stop
static const std::string PLAYBACK_STOPPED = "PLAYBACK_STOPPED";

/// Metrics for filtered and unfiltered metadata
static const std::string METADATA_UNFILTERED_ENCOUNTERED = "UnfilteredMetadataEncountered";
static const std::string METADATA_FILTERED_ENCOUNTERED = "FilteredMetadataEncountered";

/// Total play time metric
static const std::string MEDIA_PLAYBACK_TIME = "MediaPlaybackTime";

/// Track to Track time metric
static const std::string TRACK_TO_TRACK_TIME = "AutoProgressionLatency";

/// Playback error during auto-progression
static const std::string TRACK_PROGRESSION_FATAL = "TrackProgressionFatalError";

/// Fatal error mid-playback
static const std::string MID_PLAYBACK_FATAL = "PlaybackFatalError";

/// Time spent on queue
static const std::string TRACK_TIME_ON_QUEUE = "TrackTimeOnQueue";

/// whitelisted metadata to send to server.  This is done to avoid excessive traffic
/// must be all lower-case
static const std::vector<std::string> METADATA_WHITELIST = {"title"};

/// Min time between metadata events
static const std::chrono::seconds METADATA_EVENT_RATE{1};

/// Time to keep the pipeline open after a local pause.
static const std::chrono::seconds LOCAL_STOP_DEFAULT_PIPELINE_OPEN_TIME(900);  // 15min

/**
 * Compare two URLs up to the 'query' portion, if one exists.
 *
 * @param url1 First URL to compare
 * @param url2  URL to compare to
 * @return true if the URLs are identical except for their query strings.
 */
static bool compareUrlNotQuery(const std::string& url1, const std::string& url2) {
    std::string::size_type p1 = url1.find('?');
    std::string::size_type p2 = url2.find('?');
    if (p1 != p2) {
        return false;
    }
    if (p1 == std::string::npos) {
        return url1 == url2;
    }
    std::string url1Trunc = url1.substr(0, p1);
    std::string url2Trunc = url2.substr(0, p2);

    return url1Trunc == url2Trunc;
}

/**
 * Creates the AudioPlayer capability configuration.
 *
 * @param fingerprint The @c Fingerprint of @c MediaPlayer.
 * @return The AudioPlayer capability configuration.
 */
static std::shared_ptr<avsCommon::avs::CapabilityConfiguration> getAudioPlayerCapabilityConfiguration(
    Fingerprint fingerprint);

/**
 * Handles a Metric event by creating and recording it. Failure to create or record the event results
 * in an early return.
 *
 * @param metricRecorder The @c MetricRecorderInterface which records Metric events.
 * @param activityName The activityName of the Metric event.
 * @param dataPoint The @c DataPoint of this Metric event.
 * @param msgId The message id string retrieved from the directive.
 * @param trackId The track id string retrieved from the directive.
 * @param requesterType The requester type string retrieved from the directive, defaults to empty string.
 */
static void submitMetric(
    const std::shared_ptr<MetricRecorderInterface>& metricRecorder,
    const std::string& metricActivityName,
    const DataPoint& dataPoint,
    const std::string& msgId,
    const std::string& trackId,
    const std::string& requesterType = "") {
    auto metricBuilder = MetricEventBuilder{}.setActivityName(metricActivityName).addDataPoint(dataPoint);
    if (!msgId.empty()) {
        metricBuilder.addDataPoint(DataPointStringBuilder{}.setName("DIRECTIVE_MESSAGE_ID").setValue(msgId).build());
    }
    if (!trackId.empty()) {
        metricBuilder.addDataPoint(DataPointStringBuilder{}.setName("TRACK_ID").setValue(trackId).build());
    }
    if (!requesterType.empty()) {
        metricBuilder.addDataPoint(DataPointStringBuilder{}.setName("REQUESTER_TYPE").setValue(requesterType).build());
    }

    auto metric = metricBuilder.build();
    if (metric == nullptr) {
        ACSDK_ERROR(LX("Error creating metric."));
        return;
    }
    recordMetric(metricRecorder, metric);
}

static void submitAnnotatedMetric(
    const std::shared_ptr<MetricRecorderInterface>& metricRecorder,
    const std::string& baseMetric,
    const std::string& annotation) {
    auto metricName = baseMetric + "_" + annotation;
    submitMetric(
        metricRecorder,
        AUDIO_PLAYER_METRIC_PREFIX + metricName,
        DataPointCounterBuilder{}.setName(metricName).increment(1).build(),
        "",
        "");
}

AudioPlayer::PlayDirectiveInfo::PlayDirectiveInfo(const std::string& messageId) :
        messageId{messageId},
        playBehavior{PlayBehavior::ENQUEUE},
        sourceId{ERROR_SOURCE_ID},
        isBuffered{false} {
}

std::shared_ptr<AudioPlayer> AudioPlayer::create(
    std::unique_ptr<MediaPlayerFactoryInterface> mediaPlayerFactory,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<FocusManagerInterface> focusManager,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<PlaybackRouterInterface> playbackRouter,
    std::vector<std::shared_ptr<ChannelVolumeInterface>> audioChannelVolumeInterfaces,
    std::shared_ptr<captions::CaptionManagerInterface> captionManager,
    std::shared_ptr<MetricRecorderInterface> metricRecorder) {
    if (nullptr == mediaPlayerFactory) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMediaPlayerFactory"));
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
    } else if (audioChannelVolumeInterfaces.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "emptyAudioChannelVolumeInterfaces"));
        return nullptr;
    }

    auto audioPlayer = std::shared_ptr<AudioPlayer>(new AudioPlayer(
        std::move(mediaPlayerFactory),
        messageSender,
        focusManager,
        contextManager,
        exceptionSender,
        playbackRouter,
        audioChannelVolumeInterfaces,
        captionManager,
        metricRecorder));
    contextManager->setStateProvider(STATE, audioPlayer);
    audioPlayer->m_mediaPlayerFactory->addObserver(audioPlayer);

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
    if (info && (info->directive->getName() == PLAY.name || info->directive->getName() == CLEAR_QUEUE.name)) {
        preHandleDirective(info);
    }
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
    } else if (info->directive->getName() == CLEAR_QUEUE.name) {
        // Handle ClearQueue in pre-handle, because when we get a sequence of
        // ClearQueue, Play
        // The handleClearQueue gets called between preHandle Play, and Handle Play - clearing the pre-buffered data
        handleClearQueueDirective(info);
    } else if (info->directive->getName() == STOP.name) {
        // Handle Stop in pre-handle, because when we get a sequence of
        // Stop, Play
        // The handleStopDirective gets called between preHandle Play, and Handle Play - stopping playback
        handleStopDirective(info);
    } else {
        ACSDK_DEBUG(LX("preHandleDirective NO-OP")
                        .d("reason", "NoPreHandleBehavior")
                        .d("directiveName", info->directive->getName()));
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
        // Stop handled in 'preHandle, above
    } else if (info->directive->getName() == CLEAR_QUEUE.name) {
        // clearQueue handled in 'preHandle, above
    } else if (info->directive->getName() == UPDATE_PROGRESS_REPORT_INTERVAL.name) {
        handleUpdateProgressReportIntervalDirective(info);
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
            auto okToRequest = m_okToRequestNextTrack;
            m_okToRequestNextTrack = false;
            for (auto it = m_audioPlayQueue.begin(); it != m_audioPlayQueue.end(); it++) {
                if (messageId == (*it)->messageId) {
                    if (it->get()->mediaPlayer) {
                        stopAndReleaseMediaPlayer(*it);
                    }
                    m_audioPlayQueue.erase(it);
                    break;
                }
            }
            m_okToRequestNextTrack = okToRequest;
        });
    }
}

void AudioPlayer::onDeregistered() {
    ACSDK_DEBUG(LX("onDeregistered"));
    m_executor.submit([this] {
        executeStop();
        clearPlayQueue(false);
    });
}

DirectiveHandlerConfiguration AudioPlayer::getConfiguration() const {
    DirectiveHandlerConfiguration configuration;
    auto audioNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, false);

    configuration[PLAY] = audioNonBlockingPolicy;
    configuration[STOP] = audioNonBlockingPolicy;
    configuration[CLEAR_QUEUE] = audioNonBlockingPolicy;
    configuration[UPDATE_PROGRESS_REPORT_INTERVAL] = audioNonBlockingPolicy;
    return configuration;
}

void AudioPlayer::onFocusChanged(FocusState newFocus, MixingBehavior behavior) {
    ACSDK_DEBUG(LX("onFocusChanged").d("newFocus", newFocus).d("MixingBehavior", behavior));
    m_executor.submit([this, newFocus, behavior] { executeOnFocusChanged(newFocus, behavior); });

    switch (newFocus) {
        case FocusState::FOREGROUND:
            // Could wait for playback to actually start, but there's no real benefit to waiting, and long delays in
            // buffering could result in timeouts, so returning immediately for this case.
            return;
        case FocusState::BACKGROUND: {
            // In case of MAY_DUCK MixingBehavior, exit early without waiting for state change below
            if (MixingBehavior::MAY_DUCK == behavior) {
                return;
            }

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

void AudioPlayer::onFirstByteRead(SourceId id, const MediaPlayerState& state) {
    ACSDK_DEBUG(LX(__func__).d("id", id).d("state", state));
}

void AudioPlayer::onPlaybackStarted(SourceId id, const MediaPlayerState& state) {
    ACSDK_DEBUG(LX("onPlaybackStarted").d("id", id).d("state", state));

    submitMetric(
        m_metricRecorder,
        AUDIO_PLAYER_METRIC_PREFIX + PLAYBACK_STARTED,
        DataPointCounterBuilder{}.setName(PLAYBACK_STARTED).increment(1).build(),
        m_currentlyPlaying->messageId,
        m_currentlyPlaying->audioItem.id);

    m_executor.submit([this, id, state] { executeOnPlaybackStarted(id, state); });
}

void AudioPlayer::onPlaybackStopped(SourceId id, const MediaPlayerState& state) {
    ACSDK_DEBUG(LX("onPlaybackStopped").d("id", id).d("state", state));

    submitMetric(
        m_metricRecorder,
        AUDIO_PLAYER_METRIC_PREFIX + PLAYBACK_STOPPED,
        DataPointCounterBuilder{}.setName(PLAYBACK_STOPPED).increment(1).build(),
        m_currentlyPlaying->stopMessageId,
        m_currentlyPlaying->audioItem.id);

    m_executor.submit([this, id, state] { executeOnPlaybackStopped(id, state); });
}

void AudioPlayer::onPlaybackFinished(SourceId id, const MediaPlayerState& state) {
    ACSDK_DEBUG(LX("onPlaybackFinished").d("id", id).d("state", state));
    m_executor.submit([this, id, state] { executeOnPlaybackFinished(id, state); });
}

void AudioPlayer::onPlaybackError(
    SourceId id,
    const ErrorType& type,
    std::string error,
    const MediaPlayerState& state) {
    ACSDK_DEBUG(LX("onPlaybackError").d("type", type).d("error", error).d("id", id).d("state", state));
    m_executor.submit([this, id, type, error, state] { executeOnPlaybackError(id, type, error, state); });
}

void AudioPlayer::onPlaybackPaused(SourceId id, const MediaPlayerState& state) {
    ACSDK_DEBUG(LX("onPlaybackPaused").d("id", id).d("state", state));
    m_executor.submit([this, id, state] { executeOnPlaybackPaused(id, state); });
}

void AudioPlayer::onPlaybackResumed(SourceId id, const MediaPlayerState& state) {
    ACSDK_DEBUG(LX("onPlaybackResumed").d("id", id).d("state", state));
    m_executor.submit([this, id, state] { executeOnPlaybackResumed(id, state); });
}

void AudioPlayer::onBufferUnderrun(SourceId id, const MediaPlayerState& state) {
    ACSDK_DEBUG(LX("onBufferUnderrun").d("id", id).d("state", state));
    m_executor.submit([this, id, state] { executeOnBufferUnderrun(id, state); });
}

void AudioPlayer::onBufferRefilled(SourceId id, const MediaPlayerState& state) {
    ACSDK_DEBUG(LX("onBufferRefilled").d("id", id).d("state", state));
    m_executor.submit([this, id, state] { executeOnBufferRefilled(id, state); });
}

void AudioPlayer::onSeeked(SourceId id, const MediaPlayerState& startState, const MediaPlayerState& endState) {
    ACSDK_DEBUG(LX("onSeeked").d("id", id).d("startState", startState).d("endState", endState));
    m_executor.submit([this, id, startState, endState] { executeOnSeeked(id, startState, endState); });
}

void AudioPlayer::onTags(SourceId id, std::unique_ptr<const VectorOfTags> vectorOfTags, const MediaPlayerState& state) {
    ACSDK_DEBUG(LX("onTags").d("id", id).d("state", state));
    if (nullptr == vectorOfTags || vectorOfTags->empty()) {
        ACSDK_ERROR(LX("onTagsFailed").d("reason", "noTags"));
        return;
    }
    std::shared_ptr<const VectorOfTags> sharedVectorOfTags(std::move(vectorOfTags));
    m_executor.submit([this, id, sharedVectorOfTags, state] { executeOnTags(id, sharedVectorOfTags, state); });
}

void AudioPlayer::onBufferingComplete(SourceId id, const MediaPlayerState& state) {
    ACSDK_DEBUG(LX("onBufferingComplete").d("id", id).d("state", state));
    m_executor.submit([this, id, state] { executeOnBufferingComplete(id, state); });
}

void AudioPlayer::onReadyToProvideNextPlayer() {
    ACSDK_DEBUG(LX(__func__));
    m_executor.submit([this] { executeOnReadyToProvideNextPlayer(); });
}

void AudioPlayer::onProgressReportDelayElapsed() {
    ACSDK_DEBUG5(LX(__func__));
    m_executor.submit([this] { sendEventWithTokenAndOffset("ProgressReportDelayElapsed"); });
}

void AudioPlayer::onProgressReportIntervalElapsed() {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this] { sendEventWithTokenAndOffset("ProgressReportIntervalElapsed", true); });
}

void AudioPlayer::onProgressReportIntervalUpdated() {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this] { sendEventWithTokenAndOffset("ProgressReportIntervalUpdated"); });
}

void AudioPlayer::requestProgress() {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this] {
        auto progress = getOffset();
        m_progressTimer.onProgress(progress);
    });
}

void AudioPlayer::addObserver(std::shared_ptr<AudioPlayerObserverInterface> observer) {
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

void AudioPlayer::removeObserver(std::shared_ptr<AudioPlayerObserverInterface> observer) {
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

void AudioPlayer::setObserver(std::shared_ptr<RenderPlayerInfoCardsObserverInterface> observer) {
    ACSDK_DEBUG1(LX(__func__));
    m_executor.submit([this, observer] { m_renderPlayerObserver = observer; });
}

std::chrono::milliseconds AudioPlayer::getAudioItemOffset() {
    ACSDK_DEBUG1(LX(__func__));
    auto offset = m_executor.submit([this] { return getOffset(); });
    return offset.get();
}

std::chrono::milliseconds AudioPlayer::getAudioItemDuration() {
    ACSDK_DEBUG5(LX(__func__));
    auto duration = m_executor.submit([this] { return getDuration(); });
    return duration.get();
}

void AudioPlayer::stopPlayback() {
    localOperation(PlaybackOperation::STOP_PLAYBACK);
}

bool AudioPlayer::localOperation(PlaybackOperation op) {
    ACSDK_DEBUG5(LX(__func__));
    std::promise<bool> opSuccess;
    auto successFuture = opSuccess.get_future();

    m_executor.submit([this, op, &opSuccess] { executeLocalOperation(op, std::move(opSuccess)); });

    if (successFuture.wait_for(std::chrono::milliseconds(1000)) == std::future_status::ready) {
        return successFuture.get();
    }
    return false;
}

bool AudioPlayer::localSeekTo(std::chrono::milliseconds location, bool fromStart) {
    ACSDK_DEBUG5(LX(__func__));
    auto result = m_executor.submit([this, location, fromStart] { return executeLocalSeekTo(location, fromStart); });
    return result.get();
}

AudioPlayer::AudioPlayer(
    std::unique_ptr<MediaPlayerFactoryInterface> mediaPlayerFactory,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<FocusManagerInterface> focusManager,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<PlaybackRouterInterface> playbackRouter,
    std::vector<std::shared_ptr<ChannelVolumeInterface>> audioChannelVolumeInterfaces,
    std::shared_ptr<captions::CaptionManagerInterface> captionManager,
    std::shared_ptr<MetricRecorderInterface> metricRecorder) :
        CapabilityAgent{NAMESPACE, exceptionSender},
        RequiresShutdown{"AudioPlayer"},
        m_mediaPlayerFactory{std::move(mediaPlayerFactory)},
        m_messageSender{messageSender},
        m_focusManager{focusManager},
        m_contextManager{contextManager},
        m_playbackRouter{playbackRouter},
        m_captionManager{captionManager},
        m_metricRecorder{metricRecorder},
        m_currentActivity{PlayerActivity::IDLE},
        m_focus{FocusState::NONE},
        m_currentlyPlaying(std::make_shared<PlayDirectiveInfo>("")),
        m_offset{std::chrono::milliseconds{std::chrono::milliseconds::zero()}},
        m_isStopCalled{false},
        m_okToRequestNextTrack{false},
        m_isAcquireChannelRequestPending{false},
        m_currentMixability{avsCommon::avs::ContentType::UNDEFINED},
        m_mixingBehavior{avsCommon::avs::MixingBehavior::UNDEFINED},
        m_isAutoProgressing{false},
        m_isLocalResumePending(false),
        m_isStartingPlayback{false},
        m_audioChannelVolumeInterfaces{audioChannelVolumeInterfaces} {
    Fingerprint fingerprint = m_mediaPlayerFactory->getFingerprint();
    m_capabilityConfigurations.insert(getAudioPlayerCapabilityConfiguration(fingerprint));
    m_currentlyPlaying->sourceId = ERROR_SOURCE_ID;
}

std::shared_ptr<CapabilityConfiguration> getAudioPlayerCapabilityConfiguration(Fingerprint fingerprint) {
    std::unordered_map<std::string, std::string> configMap;
    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, AUDIOPLAYER_CAPABILITY_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, AUDIOPLAYER_CAPABILITY_INTERFACE_NAME});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, AUDIOPLAYER_CAPABILITY_INTERFACE_VERSION});

    if (!fingerprint.package.empty() && !fingerprint.buildType.empty() && !fingerprint.versionNumber.empty()) {
        JsonGenerator configurations;
        configurations.startObject(FINGERPRINT_KEY);
        configurations.addMember(PACKAGE_KEY, fingerprint.package);
        configurations.addMember(BUILD_TYPE_KEY, fingerprint.buildType);
        configurations.addMember(VERSION_NUMBER_KEY, fingerprint.versionNumber);
        configMap.insert({CAPABILITY_INTERFACE_CONFIGURATIONS_KEY, configurations.toString()});
    }

    return std::make_shared<CapabilityConfiguration>(configMap);
}

void AudioPlayer::doShutdown() {
    m_progressTimer.stop();
    m_executor.shutdown();
    executeStop();
    releaseMediaPlayer(m_currentlyPlaying);
    m_mediaPlayerFactory->removeObserver(shared_from_this());
    m_mediaPlayerFactory.reset();
    m_messageSender.reset();
    m_focusManager.reset();
    m_contextManager->setStateProvider(STATE, nullptr);
    m_contextManager.reset();
    clearPlayQueue(true);
    m_playbackRouter.reset();
    m_captionManager.reset();
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

static audio::MixingBehavior getMixingBehavior(
    const std::string& messageID,
    const rapidjson::Value& payload,
    const std::shared_ptr<MetricRecorderInterface>& metricRecorder) {
    std::string interruptedBehavior;
    if (jsonUtils::retrieveValue(payload, "interruptedBehavior", &interruptedBehavior)) {
        if (interruptedBehavior == "PAUSE")
            return audio::MixingBehavior::BEHAVIOR_PAUSE;
        else if (interruptedBehavior == "ATTENUATE")
            return audio::MixingBehavior::BEHAVIOR_DUCK;
        else {
            submitMetric(
                metricRecorder,
                AUDIO_PLAYER_METRIC_PREFIX + UNSUPPORTED_INTERRUPTED_BEHAVIOR,
                DataPointCounterBuilder{}.setName(UNSUPPORTED_INTERRUPTED_BEHAVIOR).increment(1).build(),
                messageID,
                "");
        }
    } else {
        submitMetric(
            metricRecorder,
            AUDIO_PLAYER_METRIC_PREFIX + MISSING_INTERRUPTED_BEHAVIOR,
            DataPointCounterBuilder{}.setName(MISSING_INTERRUPTED_BEHAVIOR).increment(1).build(),
            messageID,
            "");
    }

    ACSDK_ERROR(LX("getMixingBehaviorFailed")
                    .d("reason", "interruptedBehavior flag missing")
                    .m("using Default Behavior (PAUSE)"));
    return DEFAULT_MIXING_BEHAVIOR;
}

void AudioPlayer::preHandlePlayDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG1(LX("preHandlePlayDirective"));
    ACSDK_DEBUG9(LX("prePLAY").d("payload", info->directive->getPayload()));
    rapidjson::Document payload;
    if (!parseDirectivePayload(info, &payload)) {
        return;
    }

    std::shared_ptr<PlayDirectiveInfo> playItem = std::make_shared<PlayDirectiveInfo>(info->directive->getMessageId());

    if (!jsonUtils::retrieveValue(payload, "playBehavior", &playItem->playBehavior)) {
        playItem->playBehavior = PlayBehavior::ENQUEUE;
    }

    rapidjson::Value::ConstMemberIterator audioItemJson;
    if (!jsonUtils::findNode(payload, "audioItem", &audioItemJson)) {
        ACSDK_ERROR(
            LX("preHandlePlayDirectiveFailed").d("reason", "missingAudioItem").d("messageId", playItem->messageId));
        sendExceptionEncounteredAndReportFailed(info, "missing AudioItem");
        return;
    }

    AudioItem audioItem;
    if (!jsonUtils::retrieveValue(audioItemJson->value, "audioItemId", &audioItem.id)) {
        audioItem.id = "anonymous";
    }

    rapidjson::Value::ConstMemberIterator stream;
    if (!jsonUtils::findNode(audioItemJson->value, "stream", &stream)) {
        ACSDK_ERROR(
            LX("preHandlePlayDirectiveFailed").d("reason", "missingStream").d("messageId", playItem->messageId));
        sendExceptionEncounteredAndReportFailed(info, "missing stream");
        return;
    }

    playItem->mixingBehavior = getMixingBehavior(info->directive->getMessageId(), stream->value, m_metricRecorder);

    if (!jsonUtils::retrieveValue(stream->value, "url", &audioItem.stream.url)) {
        ACSDK_ERROR(LX("preHandlePlayDirectiveFailed").d("reason", "missingUrl").d("messageId", playItem->messageId));
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
                            .d("messageId", playItem->messageId));
            sendExceptionEncounteredAndReportFailed(info, "unable to obtain attachment reader");
            return;
        }

        if (audioItem.stream.format != StreamFormat::AUDIO_MPEG) {
            ACSDK_ERROR(LX("preHandlePlayDirectiveFailed")
                            .d("reason", "unsupportedFormat")
                            .d("format", audioItem.stream.format)
                            .d("messageId", playItem->messageId));
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

    if (jsonUtils::retrieveValue(stream->value, "endOffsetInMilliseconds", &milliseconds)) {
        // if endOffsetInMilliseconds exists in the directive, it must be greater than offsetInMilliseconds
        auto chronoMillis = std::chrono::milliseconds(milliseconds);
        if (chronoMillis <= audioItem.stream.offset) {
            ACSDK_ERROR(LX("preHandlePlayDirectiveFailed")
                            .d("reason", "endOffsetLessThanOffset")
                            .d("offset", audioItem.stream.offset.count())
                            .d("endOffset", chronoMillis.count()));
            std::string message = "bad endOffset value";
            sendExceptionEncounteredAndReportFailed(info, message, ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }
        audioItem.stream.endOffset = chronoMillis;
    } else {
        audioItem.stream.endOffset = std::chrono::milliseconds::zero();
    }

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
    audioItem.stream.progressReport.delay = ProgressTimer::getNoDelay();
    audioItem.stream.progressReport.interval = ProgressTimer::getNoInterval();
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

    rapidjson::Value::ConstMemberIterator httpHeadersIterator;
    if (jsonUtils::findNode(stream->value, PlaybackContext::HTTP_HEADERS, &httpHeadersIterator)) {
        parseHeadersFromPlayDirective(httpHeadersIterator->value, audioItem);
    }
    // Trigger play directive received after we have the track ID
    auto messageId = info->directive->getMessageId();
    if (playItem->playBehavior != PlayBehavior::ENQUEUE && !messageId.empty()) {
        submitMetric(
            m_metricRecorder,
            AUDIO_PLAYER_METRIC_PREFIX + PREPARE_PLAY_DIRECTIVE_RECEIVED,
            DataPointCounterBuilder{}.setName(PREPARE_PLAY_DIRECTIVE_RECEIVED).increment(1).build(),
            messageId,
            audioItem.stream.token);
    }

    if (!m_captionManager) {
        ACSDK_DEBUG5(LX("captionsNotParsed").d("reason", "captionManagerIsNull"));
    } else {
        rapidjson::Value::ConstMemberIterator captionIterator;
        if (!jsonUtils::findNode(stream->value, CAPTION_KEY, &captionIterator)) {
            captionIterator = stream->value.MemberEnd();
            ACSDK_DEBUG3(LX("captionsNotParsed").d("reason", "keyNotFoundInPayload"));
        } else {
            auto captionFormat = captions::CaptionFormat::UNKNOWN;
            std::string captionFormatString;
            if (jsonUtils::retrieveValue(captionIterator->value, CAPTION_TYPE_KEY, &captionFormatString)) {
                captionFormat = captions::avsStringToCaptionFormat(captionFormatString);
            } else {
                ACSDK_WARN(LX("captionParsingIncomplete").d("reason", "failedToParseField").d("field", "type"));
            }

            std::string captionContentString;
            if (!jsonUtils::retrieveValue(captionIterator->value, CAPTION_CONTENT_KEY, &captionContentString)) {
                ACSDK_WARN(LX("captionParsingIncomplete").d("reason", "failedToParseField").d("field", "content"));
            }

            auto captionData = captions::CaptionData(captionFormat, captionContentString);
            ACSDK_DEBUG5(LX("captionPayloadParsed").d("type", captionData.format));
            audioItem.captionData = captions::CaptionData(captionFormat, captionContentString);
        }
    }
    rapidjson::Value::ConstMemberIterator playRequestorJson;
    if (jsonUtils::findNode(payload, "playRequestor", &playRequestorJson)) {
        if (!jsonUtils::retrieveValue(playRequestorJson->value, "type", &playItem->playRequestor.type)) {
            ACSDK_ERROR(LX("handlePlayDirectiveFailed")
                            .d("reason", "missingPlayRequestorType")
                            .d("messageId", info->directive->getMessageId()));
            sendExceptionEncounteredAndReportFailed(info, "missing playRequestor type field");
            return;
        }
        if (!jsonUtils::retrieveValue(playRequestorJson->value, "id", &playItem->playRequestor.id)) {
            ACSDK_ERROR(LX("handlePlayDirectiveFailed")
                            .d("reason", "missingPlayRequestorId")
                            .d("messageId", info->directive->getMessageId()));
            sendExceptionEncounteredAndReportFailed(info, "missing playRequestor id field");
            return;
        }
    }

    playItem->audioItem = audioItem;
    m_executor.submit([this, info, playItem] {
        if (isMessageInQueue(playItem->messageId)) {
            // There's is already a playInfo with the same messageId in the queue.
            ACSDK_ERROR(LX("preHandlePlayDirectiveFailed").d("reason", "messageIdAlreadyInPreHandleQueue"));
            sendExceptionEncounteredAndReportFailed(
                info,
                "duplicated messageId " + playItem->messageId,
                ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }

        executePrePlay(playItem);
    });
}

void AudioPlayer::handlePlayDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG1(LX("handlePlayDirective"));
    ACSDK_DEBUG9(LX("PLAY").d("payload", info->directive->getPayload()));

    rapidjson::Document payload;
    if (!parseDirectivePayload(info, &payload)) {
        return;
    }

    rapidjson::Value::ConstMemberIterator audioItemJson;
    if (!jsonUtils::findNode(payload, "audioItem", &audioItemJson)) {
        ACSDK_ERROR(LX("handlePlayDirectiveFailed")
                        .d("reason", "missingAudioItem")
                        .d("messageId", info->directive->getMessageId()));
        sendExceptionEncounteredAndReportFailed(info, "missing AudioItem");
        return;
    }

    rapidjson::Value::ConstMemberIterator stream;
    if (!jsonUtils::findNode(audioItemJson->value, "stream", &stream)) {
        ACSDK_ERROR(LX("handlePlayDirectiveFailed")
                        .d("reason", "missingStream")
                        .d("messageId", info->directive->getMessageId()));
        sendExceptionEncounteredAndReportFailed(info, "missing stream");
        return;
    }

    PlayBehavior playBehavior;
    if (!jsonUtils::retrieveValue(payload, "playBehavior", &playBehavior)) {
        playBehavior = PlayBehavior::ENQUEUE;
    }

    std::string token;
    if (!jsonUtils::retrieveValue(stream->value, "token", &token)) {
        token = "";
    }

    // Note: Unlike SpeechSynthesizer, AudioPlayer directives are instructing the client to start/stop/queue
    //     content, so directive handling is considered to be complete when we have queued the content for
    //     playback; we don't wait for playback to complete.
    setHandlingCompleted(info);

    auto messageId = info->directive->getMessageId();

    if (playBehavior != PlayBehavior::ENQUEUE && !messageId.empty()) {
        rapidjson::Value::ConstMemberIterator playRequestorJson;
        std::string requestorType{};
        if (jsonUtils::findNode(payload, "playRequestor", &playRequestorJson)) {
            if (!jsonUtils::retrieveValue(playRequestorJson->value, "type", &requestorType)) {
                ACSDK_WARN(LX("handlePlayDirectiveWarning")
                               .d("reason", "missingPlayRequestorType")
                               .d("messageId", info->directive->getMessageId()));
            }
        }

        submitMetric(
            m_metricRecorder,
            AUDIO_PLAYER_METRIC_PREFIX + PLAY_DIRECTIVE_RECEIVED,
            DataPointCounterBuilder{}.setName(PLAY_DIRECTIVE_RECEIVED).increment(1).build(),
            messageId,
            token,
            requestorType);
    }

    // Pass messageId so we can use it when the track is popped from the play queue
    m_executor.submit([this, messageId] { executePlay(messageId); });
}

void AudioPlayer::handleStopDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG1(LX("handleStopDirective"));

    submitMetric(
        m_metricRecorder,
        AUDIO_PLAYER_METRIC_PREFIX + STOP_DIRECTIVE_RECEIVED,
        DataPointCounterBuilder{}.setName(STOP_DIRECTIVE_RECEIVED).increment(1).build(),
        info->directive->getMessageId(),
        m_currentlyPlaying->audioItem.id);

    auto messageId = info->directive->getMessageId();

    setHandlingCompleted(info);
    m_executor.submit([this, messageId] { executeStop(messageId); });
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

void AudioPlayer::handleUpdateProgressReportIntervalDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG1(LX("handleUpdateProgressReportIntervalDirective"));
    rapidjson::Document payload;
    if (!parseDirectivePayload(info, &payload)) {
        return;
    }

    int64_t milliseconds;
    if (!jsonUtils::retrieveValue(payload, "progressReportIntervalInMilliseconds", &milliseconds)) {
        sendExceptionEncounteredAndReportFailed(info, "missing progressReportIntervalInMilliseconds");
        return;
    }

    setHandlingCompleted(info);
    m_executor.submit(
        [this, milliseconds] { executeUpdateProgressReportInterval(std::chrono::milliseconds(milliseconds)); });
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
    state.AddMember(TOKEN_KEY, m_currentlyPlaying->audioItem.stream.token, state.GetAllocator());
    state.AddMember(
        OFFSET_KEY,
        (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(getOffset()).count(),
        state.GetAllocator());
    state.AddMember(ACTIVITY_KEY, playerActivityToString(m_currentActivity), state.GetAllocator());
    attachPlaybackAttributesIfAvailable(state, state.GetAllocator());

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
        ACSDK_ERROR(LX("executeProvideState")
                        .d("reason", "contextManagerSetStateFailed")
                        .d("token", m_currentlyPlaying->audioItem.stream.token));
    }
}

bool AudioPlayer::executeStopDucking() {
    for (auto& it : m_audioChannelVolumeInterfaces) {
        if (!it->stopDucking()) {
            ACSDK_WARN(LX("executeOnFocusChanged").m("Failed to Restore Audio Channel Volume"));
            return false;
        } else {
            ACSDK_DEBUG9(LX("executeOnFocusChanged").m("Restored Audio Channel Volume"));
        }
    }
    return true;
}

bool AudioPlayer::executeStartDucking() {
    for (auto& it : m_audioChannelVolumeInterfaces) {
        if (it->startDucking()) {
            ACSDK_DEBUG9(LX("executeOnFocusChanged").m("Attenuated Audio Channel Volume"));
        } else {
            ACSDK_WARN(LX("executeOnFocusChanged").m("Failed to Attenuate Audio Channel Volume"));
            return false;
        }
    }
    return true;
}

void AudioPlayer::executeOnFocusChanged(FocusState newFocus, MixingBehavior behavior) {
    ACSDK_DEBUG1(LX("executeOnFocusChanged")
                     .d("from", m_focus)
                     .d("to", newFocus)
                     .d("m_currentActivity", m_currentActivity)
                     .d("MixingBehavior", behavior));

    if ((m_focus == newFocus) && (m_mixingBehavior == behavior)) {
        return;
    }
    auto previousFocus = m_focus;
    m_focus = newFocus;
    m_mixingBehavior = behavior;

    switch (newFocus) {
        case FocusState::FOREGROUND: {
            // mark any pending request as false
            if (m_isAcquireChannelRequestPending) {
                m_isAcquireChannelRequestPending = false;
            }

            // Restore Channel Volume (in case channel was attenuated earlier)
            executeStopDucking();

            if (m_isLocalResumePending) {
                m_isLocalResumePending = false;
                bool success = false;

                if (m_currentlyPlaying->mediaPlayer && m_currentlyPlaying->sourceId != ERROR_SOURCE_ID) {
                    success = m_currentlyPlaying->mediaPlayer->play(m_currentlyPlaying->sourceId);
                }
                m_localResumeSuccess.set_value(success);
                if (!success) {
                    ACSDK_ERROR(LX("executeOnFocusChangedFailed").d("reason", "localResumeFailed"));
                    m_focusManager->releaseChannel(CHANNEL_NAME, shared_from_this());
                }
                return;
            }

            switch (m_currentActivity) {
                case PlayerActivity::IDLE:
                case PlayerActivity::STOPPED:
                case PlayerActivity::FINISHED:
                    // We see a focus change to foreground in these states if we are starting to play a new song.
                    if (!m_audioPlayQueue.empty() && !m_isStartingPlayback) {
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
                    if (!m_currentlyPlaying->mediaPlayer->resume(m_currentlyPlaying->sourceId)) {
                        sendPlaybackFailedEvent(
                            m_currentlyPlaying->audioItem.stream.token,
                            ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR,
                            "failed to resume media player",
                            getMediaPlayerState());
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
        } break;
        case FocusState::BACKGROUND:
            // mark any pending request as false
            if (m_isAcquireChannelRequestPending) {
                m_isAcquireChannelRequestPending = false;
            }

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
                    if (previousFocus == FocusState::NONE && !m_audioPlayQueue.empty()) {
                        // If a higher priority CA (like Alarm) is in the foreground, AudioPlayer state will be
                        // background We need also to start to play, the mixing behavior will decide to pause or play in
                        // duck
                        if (m_audioPlayQueue.front()->mixingBehavior == audio::MixingBehavior::BEHAVIOR_DUCK &&
                            !m_isStartingPlayback) {
                            ACSDK_DEBUG1(LX("transitionToBackgroundWhileNotPlayingAndNextItemDuckable")
                                             .d("result", "playNextItemInDuck"));
                            playNextItem();
                        }

                        // if in FINISHED/IDLE/STOPPED state , NONE -> BACKGROUND transition and mixingBehavior is PAUSE
                        // do not call pause on mediaplayer : this will cause PAUSE when no music is playing, messing up
                        // audioplayer states we should rather wait for audioplayer to become FOREGROUND in the future
                        // to play an example of this is Flash Briefing:
                        // 1. Ask for Flash briefing
                        // 2. AudioPlayer receives Play directive : but is in background (since SpeechSynthesizer has
                        // Foreground : "here's your flash briefing" 2a. AudioPlayer should not call PAUSE through the
                        // behaviorDelegate below : instead wait for FOREGROUND
                        else if (m_audioPlayQueue.front()->mixingBehavior == audio::MixingBehavior::BEHAVIOR_PAUSE) {
                            ACSDK_DEBUG1(LX("transitionToBackgroundWhileNotPlayingAndNextItemNotDuckable")
                                             .d("result", "delayNextItemUntilForegroundAcquired"));
                            return;
                        }
                    } else if (m_audioPlayQueue.empty()) {
                        // if in FINISHED/IDLE/STOPPED state and no audioItems in queue : no need to Duck/Pause
                        ACSDK_DEBUG1(LX("transitionToBackGroundWhileNotPlaying"));
                        return;
                    }
                case PlayerActivity::PAUSED:
                    // Note: can be in PAUSED while we're trying to resume, in which case we still want to pause, so we
                    // fall through to call @c pause() here as well.
                case PlayerActivity::PLAYING:
                case PlayerActivity::BUFFER_UNDERRUN: {
                    if (behavior == MixingBehavior::MAY_DUCK) {
                        if (executeStartDucking()) {
                            // return early if ducking succeeded
                            return;
                        }
                        // else fall through to Pause by default
                    } else if (MixingBehavior::MUST_PAUSE != behavior) {
                        ACSDK_WARN(LX("executeOnFocusChanged").d("Unhandled MixingBehavior", behavior));
                    }

                    // If we get pushed into the background while playing or buffering, pause the current song.
                    ACSDK_DEBUG1(LX("executeOnFocusChanged").d("action", "pauseMediaPlayer"));
                    // Note: Ignoring the return value of pause() here as we end up calling it in some cases where it is
                    //     not needed and MediaPlayer is not in a pausable state.  This is harmless, but we don't want
                    //     to be reporting errors in those cases.

                    // Pause By Default Upon Receiving Background Focus
                    if (m_currentlyPlaying->mediaPlayer) {
                        m_currentlyPlaying->mediaPlayer->pause(m_currentlyPlaying->sourceId);
                    }
                }
                    return;
            }
            break;
        case FocusState::NONE:
            // Restore Channel Volume (in case channel was attenuated earlier)
            executeStopDucking();

            switch (m_currentActivity) {
                case PlayerActivity::IDLE:
                case PlayerActivity::STOPPED:
                case PlayerActivity::FINISHED:
                    // Nothing to more to do if we're already not playing; we got here because the act of
                    // stopping caused the channel to be released, which in turn caused this callback.
                    return;
                case PlayerActivity::PLAYING:
                case PlayerActivity::PAUSED:
                case PlayerActivity::BUFFER_UNDERRUN:
                    if (m_isAcquireChannelRequestPending) {
                        // Transient state, ignore - we don't want to STOP
                        ACSDK_DEBUG1(LX("executeOnFocusChanged").d("action", "Ignore-Transient"));
                        return;
                    }
                    // If the focus change came in while we were in a 'playing' state, we need to stop because we are
                    // yielding the channel.
                    clearPlayQueue(false);
                    ACSDK_DEBUG1(LX("executeOnFocusChanged").d("action", "executeStop"));
                    executeStop();
                    return;
            }
            break;
    }
    ACSDK_WARN(LX("unexpectedExecuteOnFocusChanged").d("newFocus", newFocus).d("m_currentActivity", m_currentActivity));
}

void AudioPlayer::executeOnPlaybackStarted(SourceId id, const MediaPlayerState& state) {
    ACSDK_DEBUG1(LX("executeOnPlaybackStarted").d("id", id).d("state", state));

    m_isStartingPlayback = false;

    if (id != m_currentlyPlaying->sourceId) {
        ACSDK_ERROR(LX("executeOnPlaybackStartedFailed")
                        .d("reason", "invalidSourceId")
                        .d("id", id)
                        .d("state", state)
                        .d("m_sourceId", m_currentlyPlaying->sourceId));
        return;
    }

    // Race condition exists where focus can be lost before the executeOnPlaybackStarted callback.
    if (avsCommon::avs::FocusState::NONE == m_focus) {
        ACSDK_WARN(LX(__func__).d("reason", "callbackAfterFocusLost").d("action", "stopping"));
        if (!m_currentlyPlaying->mediaPlayer->stop(m_currentlyPlaying->sourceId)) {
            ACSDK_ERROR(LX(__func__).d("reason", "stopFailed"));
        }
        return;
    }

    // Note: Send playback started event before changeActivity below to send the offset closer to when onPlaybackStarted
    // was actually emitted
    sendPlaybackStartedEvent(state);

    /*
     * When @c AudioPlayer is the active player, @c PlaybackController which is
     * the default playback handler, should handle playback button presses.
     */
    m_playbackRouter->useDefaultHandlerWith(shared_from_this());
    changeActivity(PlayerActivity::PLAYING);
    if (m_isAutoProgressing) {
        m_isAutoProgressing = false;
        submitMetric(
            m_metricRecorder,
            AUDIO_PLAYER_METRIC_PREFIX + TRACK_TO_TRACK_TIME,
            m_autoProgressTimeMetricData.setName(TRACK_TO_TRACK_TIME).stopDurationTimer().build(),
            "",
            "");
    }
    m_progressTimer.start();

    if (m_mediaPlayerFactory->isMediaPlayerAvailable() && m_currentlyPlaying->isBuffered) {
        sendPlaybackNearlyFinishedEvent(state);
    } else {
        m_okToRequestNextTrack = true;
    }
}

void AudioPlayer::executeOnBufferingComplete(SourceId id, const MediaPlayerState& state) {
    ACSDK_DEBUG(LX("executeOnBufferingComplete").d("id", id).d("state", state));

    if (id != m_currentlyPlaying->sourceId) {
        // See if SourceId identifies a loading track
        if (!m_audioPlayQueue.empty()) {
            for (auto& it : m_audioPlayQueue) {
                if (it->sourceId == id) {
                    // Mark as buffered, so nearly finished can be send when started.
                    it->isBuffered = true;
                    ACSDK_DEBUG2(LX(__func__).m("FullyBufferedBeforeStarted").d("id", id).d("state", state));
                    return;
                }
            }
        }

        ACSDK_DEBUG(LX("executeOnBufferingComplete")
                        .d("reason", "sourceIdDoesNotMatchCurrentTrack")
                        .d("id", id)
                        .d("state", state)
                        .d("m_sourceId", m_currentlyPlaying->sourceId));
        return;
    }

    if (m_mediaPlayerFactory->isMediaPlayerAvailable() && m_okToRequestNextTrack) {
        sendPlaybackNearlyFinishedEvent(state);
        m_okToRequestNextTrack = false;
    } else {
        m_currentlyPlaying->isBuffered = true;
        ACSDK_DEBUG2(LX(__func__).m("FullyBufferedBeforeStarted").d("id", id).d("state", state));
    }
}

void AudioPlayer::executeOnPlaybackStopped(SourceId id, const MediaPlayerState& state) {
    ACSDK_DEBUG1(LX("executeOnPlaybackStopped").d("id", id).d("state", state));

    if (id != m_currentlyPlaying->sourceId) {
        ACSDK_DEBUG(LX("executeOnPlaybackStopped")
                        .d("reason", "sourceIdDoesNotMatchCurrentTrack")
                        .d("id", id)
                        .d("state", state)
                        .d("m_sourceId", m_currentlyPlaying->sourceId));
        return;
    }

    m_isAutoProgressing = false;
    switch (m_currentActivity) {
        case PlayerActivity::PLAYING:
        case PlayerActivity::PAUSED:
        case PlayerActivity::BUFFER_UNDERRUN:
            changeActivity(PlayerActivity::STOPPED);
            m_progressTimer.stop();
            sendPlaybackStoppedEvent(state);
            m_okToRequestNextTrack = false;
            m_isStopCalled = false;
            if (!m_playNextItemAfterStopped || m_audioPlayQueue.empty()) {
                handlePlaybackCompleted();
            } else if (avsCommon::avs::FocusState::FOREGROUND == m_focus) {
                playNextItem();
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

void AudioPlayer::executeOnPlaybackFinished(SourceId id, const MediaPlayerState& state) {
    ACSDK_DEBUG1(LX("executeOnPlaybackFinished").d("id", id).d("state", state));

    if (id != m_currentlyPlaying->sourceId) {
        ACSDK_ERROR(LX("executeOnPlaybackFinishedFailed")
                        .d("reason", "invalidSourceId")
                        .d("id", id)
                        .d("state", state)
                        .d("m_sourceId", m_currentlyPlaying->sourceId));
        return;
    }
    m_isAutoProgressing = false;

    switch (m_currentActivity) {
        case PlayerActivity::PLAYING:
            changeActivity(PlayerActivity::FINISHED);
            m_progressTimer.stop();

            {
                if (m_audioPlayQueue.empty()) {
                    releaseMediaPlayer(m_currentlyPlaying);
                }

                m_executor.submit([this, state] {
                    // In some corner cases, usually in testing only, the 'NearlyFinished' event may not have been sent
                    // yet, check here, and send before 'Finished'
                    if (m_okToRequestNextTrack) {
                        sendPlaybackNearlyFinishedEvent(state);
                        m_okToRequestNextTrack = false;
                    }
                    // Sending this in an executor in case the 'releaseMediaPlayer()' call above
                    // resulted in a 'playbackNearlyFinished' event (which would be done on the executor, not
                    // synchronously). If it did, this needs to be sent afterward.
                    sendPlaybackFinishedEvent(state);
                    m_okToRequestNextTrack = false;
                    if (m_audioPlayQueue.empty()) {
                        handlePlaybackCompleted();
                    } else {
                        m_autoProgressTimeMetricData.startDurationTimer();
                        m_isAutoProgressing = true;
                        playNextItem();
                    }
                });
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

void AudioPlayer::executeOnPlaybackError(
    SourceId id,
    const ErrorType& type,
    std::string error,
    const MediaPlayerState& state) {
    ACSDK_DEBUG1(LX(__func__).d("id", id).d("state", state).d("type", type).d("error", error));

    m_isStartingPlayback = false;

    if (id != m_currentlyPlaying->sourceId) {
        // See if SourceId identifies a loading track
        if (!m_audioPlayQueue.empty()) {
            for (auto& it : m_audioPlayQueue) {
                if (it->sourceId == id) {
                    it->errorMsg = std::move(error);
                    it->errorType = type;
                    ACSDK_WARN(LX(__func__).m("ErrorWhileBuffering").d("id", id).d("state", state));
                    return;
                }
            }
        }

        ACSDK_ERROR(LX(__func__)
                        .d("reason", "invalidSourceId")
                        .d("id", id)
                        .d("state", state)
                        .d("m_sourceId", m_currentlyPlaying->sourceId));
        return;
    }

    if (m_isAutoProgressing) {
        submitMetric(
            m_metricRecorder,
            AUDIO_PLAYER_METRIC_PREFIX + TRACK_PROGRESSION_FATAL,
            DataPointCounterBuilder{}.setName(TRACK_PROGRESSION_FATAL).increment(1).build(),
            m_currentlyPlaying->messageId,
            m_currentlyPlaying->audioItem.id);
    } else {
        submitMetric(
            m_metricRecorder,
            AUDIO_PLAYER_METRIC_PREFIX + MID_PLAYBACK_FATAL,
            DataPointCounterBuilder{}.setName(MID_PLAYBACK_FATAL).increment(1).build(),
            m_currentlyPlaying->messageId,
            m_currentlyPlaying->audioItem.id);
    }
    m_isAutoProgressing = false;
    m_okToRequestNextTrack = false;
    m_progressTimer.stop();
    // Only send the event to the server if playing or buffering (or variations - PAUSED is OK too)
    // if we send an error after stopped, the server will send a PLAY for the next track - not what we want
    // but we want to send if initially bufferingCap
    if (m_currentActivity != PlayerActivity::IDLE) {
        auto offsetDelta = getOffset() - m_currentlyPlaying->initialOffset;
        if (offsetDelta < std::chrono::milliseconds(500) ||
            (m_currentActivity != PlayerActivity::STOPPED && m_currentActivity != PlayerActivity::FINISHED)) {
            sendPlaybackFailedEvent(m_currentlyPlaying->audioItem.stream.token, type, error, state);
        }
    }
    submitAnnotatedMetric(m_metricRecorder, "MediaError", error);

    /*
     * There's no need to call stop() here as the MediaPlayer has already stopped due to the playback error.
     * Instead, call executeOnPlaybackStopped() so that the states in AudioPlayer are reset properly.
     */
    executeOnPlaybackStopped(m_currentlyPlaying->sourceId, state);
}

void AudioPlayer::executeOnPlaybackPaused(SourceId id, const MediaPlayerState& state) {
    ACSDK_DEBUG1(LX("executeOnPlaybackPaused").d("id", id).d("state", state));

    if (id != m_currentlyPlaying->sourceId) {
        ACSDK_ERROR(LX("executeOnPlaybackPausedFailed")
                        .d("reason", "invalidSourceId")
                        .d("id", id)
                        .d("state", state)
                        .d("m_sourceId", m_currentlyPlaying->sourceId));
        return;
    }

    m_progressTimer.pause();
    sendPlaybackPausedEvent(state);
    changeActivity(PlayerActivity::PAUSED);
}

void AudioPlayer::executeOnPlaybackResumed(SourceId id, const MediaPlayerState& state) {
    ACSDK_DEBUG1(LX("executeOnPlaybackResumed").d("id", id).d("state", state));

    if (id != m_currentlyPlaying->sourceId) {
        ACSDK_ERROR(LX("executeOnPlaybackResumedFailed")
                        .d("reason", "invalidSourceId")
                        .d("id", id)
                        .d("state", state)
                        .d("m_sourceId", m_currentlyPlaying->sourceId));
        return;
    }

    if (m_currentActivity == PlayerActivity::STOPPED) {
        ACSDK_ERROR(LX("executeOnPlaybackResumedAborted").d("reason", "currentActivity:STOPPED"));
        return;
    }

    sendPlaybackResumedEvent(state);
    m_progressTimer.resume();
    changeActivity(PlayerActivity::PLAYING);
}

void AudioPlayer::executeOnBufferUnderrun(SourceId id, const MediaPlayerState& state) {
    ACSDK_DEBUG1(LX("executeOnBufferUnderrun").d("id", id).d("state", state));

    if (id != m_currentlyPlaying->sourceId) {
        ACSDK_ERROR(LX("executeOnBufferUnderrunFailed")
                        .d("reason", "invalidSourceId")
                        .d("id", id)
                        .d("state", state)
                        .d("m_sourceId", m_currentlyPlaying->sourceId));
        return;
    }

    if (PlayerActivity::BUFFER_UNDERRUN == m_currentActivity) {
        ACSDK_ERROR(LX("executeOnBufferUnderrunFailed").d("reason", "alreadyInUnderrun"));
        return;
    }

    m_bufferUnderrunTimestamp = std::chrono::steady_clock::now();
    sendPlaybackStutterStartedEvent(state);
    m_progressTimer.pause();
    changeActivity(PlayerActivity::BUFFER_UNDERRUN);
}

void AudioPlayer::executeOnBufferRefilled(SourceId id, const MediaPlayerState& state) {
    ACSDK_DEBUG1(LX("executeOnBufferRefilled").d("id", id).d("state", state));

    if (id != m_currentlyPlaying->sourceId) {
        ACSDK_ERROR(LX("executeOnBufferRefilledFailed")
                        .d("reason", "invalidSourceId")
                        .d("id", id)
                        .d("state", state)
                        .d("m_sourceId", m_currentlyPlaying->sourceId));
        return;
    }

    sendPlaybackStutterFinishedEvent(state);
    m_progressTimer.resume();
    changeActivity(PlayerActivity::PLAYING);
}

void AudioPlayer::executeOnSeeked(SourceId id, const MediaPlayerState& startState, const MediaPlayerState& endState) {
    ACSDK_DEBUG1(LX(__func__).d("id", id).d("startState", startState).d("endState", endState));

    if (id != m_currentlyPlaying->sourceId) {
        ACSDK_ERROR(LX("executeOnSeekStartFailed")
                        .d("reason", "invalidSourceId")
                        .d("id", id)
                        .d("startState", startState)
                        .d("endState", endState)
                        .d("m_sourceId", m_currentlyPlaying->sourceId));
        return;
    }

    if (m_currentActivity == PlayerActivity::BUFFER_UNDERRUN) {
        sendPlaybackStutterFinishedEvent(endState);
        m_progressTimer.resume();
        changeActivity(PlayerActivity::PLAYING);
    }
    sendPlaybackSeekedEvent(startState, endState);
}

void AudioPlayer::executeOnTags(
    SourceId id,
    std::shared_ptr<const VectorOfTags> vectorOfTags,
    const MediaPlayerState& state) {
    ACSDK_DEBUG1(LX("executeOnTags").d("id", id).d("state", state));

    if (id != m_currentlyPlaying->sourceId) {
        // See if SourceId identifies a loading track
        for (const auto& it : m_audioPlayQueue) {
            if (it->sourceId == id) {
                sendStreamMetadataExtractedEvent(it->audioItem, vectorOfTags, state);
                return;
            }
        }

        ACSDK_ERROR(LX("executeOnTags")
                        .d("reason", "invalidSourceId")
                        .d("id", id)
                        .d("state", state)
                        .d("m_sourceId", m_currentlyPlaying->sourceId));
        return;
    }

    sendStreamMetadataExtractedEvent(m_currentlyPlaying->audioItem, vectorOfTags, state);
}

void AudioPlayer::clearPlayQueue(const bool stopCurrentPlayer) {
    // release all MediaPlayers on the play queue
    for (auto& it : m_audioPlayQueue) {
        if (it->mediaPlayer) {
            if (!stopCurrentPlayer && it->sourceId == m_currentlyPlaying->sourceId) {
                releaseMediaPlayer(it);
                continue;
            }
            stopAndReleaseMediaPlayer(it);
        }
    }
    m_audioPlayQueue.clear();
}

void AudioPlayer::stopAndReleaseMediaPlayer(std::shared_ptr<PlayDirectiveInfo> playbackItem) {
    m_isStartingPlayback = false;
    if (playbackItem->mediaPlayer) {
        playbackItem->mediaPlayer->stop(playbackItem->sourceId);
    }
    releaseMediaPlayer(playbackItem);
}

void AudioPlayer::releaseMediaPlayer(std::shared_ptr<PlayDirectiveInfo> playbackItem) {
    if (playbackItem->mediaPlayer) {
        ACSDK_DEBUG5(LX(__func__).d("sourceId", playbackItem->sourceId));
        playbackItem->mediaPlayer->removeObserver(shared_from_this());
        if (!m_mediaPlayerFactory->releaseMediaPlayer(playbackItem->mediaPlayer)) {
            ACSDK_ERROR(
                LX(__func__).m("releaseMediaPlayerFailed").d("reason", "Factory Release Failed - Invalid MediaPlayer"));
            // This should never happen, but even if it does, the pointer should eb reset, so continue.
        }
        playbackItem->mediaPlayer.reset();
        playbackItem->sourceId = ERROR_SOURCE_ID;
    }
}

bool AudioPlayer::isMessageInQueue(const std::string& messageId) {
    for (const auto& it : m_audioPlayQueue) {
        if (messageId == it->messageId) {
            return true;
        }
    }
    return false;
}

bool AudioPlayer::configureMediaPlayer(std::shared_ptr<PlayDirectiveInfo>& playbackItem) {
    if (!playbackItem->mediaPlayer) {
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer =
            m_mediaPlayerFactory->acquireMediaPlayer();
        AudioPlayer::SourceId sourceId = ERROR_SOURCE_ID;
        if (mediaPlayer == nullptr) {
            ACSDK_ERROR(LX("configureMediaPlayerFailed").d("reason", "nullMediaPlayer"));
            return false;
        }
        if (playbackItem->audioItem.stream.reader) {
            ACSDK_DEBUG9(LX("configureMediaPlayer"));
            sourceId = mediaPlayer->setSource(std::move(playbackItem->audioItem.stream.reader));
            if (ERROR_SOURCE_ID == sourceId) {
                sendPlaybackFailedEvent(
                    playbackItem->audioItem.stream.token,
                    ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR,
                    "failed to set attachment media source",
                    getMediaPlayerState());
                ACSDK_ERROR(LX("configureMediaPlayerFailed").d("reason", "setSourceFailed").d("type", "attachment"));
                return false;
            }
        } else {
            ACSDK_DEBUG9(LX("configureMediaPlayer").d("offset", playbackItem->audioItem.stream.offset.count()));
            SourceConfig cfg = emptySourceConfig();
            cfg.endOffset = playbackItem->audioItem.stream.endOffset;
            sourceId = mediaPlayer->setSource(
                playbackItem->audioItem.stream.url,
                playbackItem->audioItem.stream.offset,
                cfg,
                false,
                playbackItem->audioItem.playbackContext);
            if (ERROR_SOURCE_ID == sourceId) {
                sendPlaybackFailedEvent(
                    playbackItem->audioItem.stream.token,
                    ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR,
                    "failed to set URL media source",
                    getMediaPlayerState());
                ACSDK_ERROR(LX("configureMediaPlayerFailed").d("reason", "setSourceFailed").d("type", "URL"));
                return false;
            }
        }
        ACSDK_DEBUG5(LX(__func__).d("sourceId", sourceId).d("audioItemId", playbackItem->audioItem.id));
        playbackItem->mediaPlayer = mediaPlayer;
        playbackItem->sourceId = sourceId;
        playbackItem->mediaPlayer->addObserver(shared_from_this());
    }
    return true;
}

void AudioPlayer::executeOnReadyToProvideNextPlayer() {
    ACSDK_DEBUG1(LX(__func__).d("queueSize", m_audioPlayQueue.size()));

    if (!m_mediaPlayerFactory->isMediaPlayerAvailable()) {
        ACSDK_DEBUG1(LX(__func__).m("AvailablePlayerInUse"));
        return;
    }

    if (m_audioPlayQueue.empty() && m_okToRequestNextTrack) {
        sendPlaybackNearlyFinishedEvent(getMediaPlayerState());
        m_okToRequestNextTrack = false;
    } else {
        // If something is on the queue, and doesn't have a player,
        // then satisfy that now
        for (auto& it : m_audioPlayQueue) {
            if (!it->mediaPlayer) {
                ACSDK_INFO(LX(__func__).m("providing available MediaPlayer to queued item"));
                configureMediaPlayer(it);
                break;
            }
        }
    }
}

void AudioPlayer::executePrePlay(std::shared_ptr<PlayDirectiveInfo> info) {
    ACSDK_DEBUG1(LX(__func__).d("messageId", info->messageId).d("preBufferBehavior", info->playBehavior));

    // Per AVS docs, drop/ignore AudioItems that specify an expectedPreviousToken which does not match the
    // current/previous token.
    if (!info->audioItem.stream.expectedPreviousToken.empty()) {
        auto previousToken = m_audioPlayQueue.empty() ? m_currentlyPlaying->audioItem.stream.token
                                                      : m_audioPlayQueue.back()->audioItem.stream.token;
        if (previousToken != info->audioItem.stream.expectedPreviousToken) {
            ACSDK_INFO(LX("executePrePlayDropped")
                           .d("reason", "unexpectedPreviousToken")
                           .d("previous", previousToken)
                           .d("expected", info->audioItem.stream.expectedPreviousToken));
            return;
        }
    }

    // check for 'next' case by comparing AudioItemId with next queued track
    bool isNextItem = false;

    if (!isNextItem && !m_audioPlayQueue.empty() && info->playBehavior != PlayBehavior::ENQUEUE) {
        auto& existingItem = m_audioPlayQueue.front();

        // When using song id, the server will send a tts-as-track & music directive pair, both with
        // the same audioItemId / token.  So to make this work, we need to compare urls as well as id's
        // This is temperary until the Cloud issue DEEAPP-919865 is fixed, at which point AudioItemIds
        // are all that need comparing
        isNextItem =
            (m_audioPlayQueue.size() == 1 && existingItem->audioItem.id == info->audioItem.id &&
             compareUrlNotQuery(existingItem->audioItem.stream.url, info->audioItem.stream.url));

        if (!isNextItem && m_audioPlayQueue.size() > 1) {
            for (auto& it : m_audioPlayQueue) {
                if (it->audioItem.id == info->audioItem.id &&
                    compareUrlNotQuery(it->audioItem.stream.url, info->audioItem.stream.url)) {
                    isNextItem = true;
                    existingItem = it;
                    break;
                }
            }
        }
        if (isNextItem) {
            ACSDK_DEBUG(
                LX(__func__).d("usingExistingBufferedItem", info->audioItem.id).d("sourceId", existingItem->sourceId));
            // replace the passed item with the already queued & buffered item
            info->mediaPlayer = existingItem->mediaPlayer;
            info->sourceId = existingItem->sourceId;
            // Clear the player instance, so it is not released
            existingItem->mediaPlayer.reset();
        }
    }

    if (!isNextItem) {
        // Only configure the new MediaPlayer if a player is available, and the head of the queue already has one.
        if (m_mediaPlayerFactory->isMediaPlayerAvailable() &&
            (m_audioPlayQueue.empty() || m_audioPlayQueue.front()->mediaPlayer)) {
            ACSDK_DEBUG(LX(__func__).m("acquiringPlayerSetSource"));
            if (!configureMediaPlayer(info)) {
                // Error already logged
                return;
            }
        } else {
            ACSDK_DEBUG5(LX(__func__).m("enqueueWithoutPlayer"));
            // if not available, enqueue the item without a player
        }
    }

    // Establish the link between message, audioItem, and sourceId in the log.
    ACSDK_INFO(
        LX(__func__).d("enqueuing", info->audioItem.id).d("sourceId", info->sourceId).d("messageId", info->messageId));

    // Do any playback/queue maintenance per playBehavior.
    switch (info->playBehavior) {
        case PlayBehavior::REPLACE_ALL:
            // handled in executePlay()
            // FALL-THROUGH
        case PlayBehavior::REPLACE_ENQUEUED:
            clearPlayQueue(false);
            // FALL-THROUGH
        case PlayBehavior::ENQUEUE:
            info->queueTimeMetricData.startDurationTimer();
            m_audioPlayQueue.push_back(info);
            break;
    }
}

void AudioPlayer::executePlay(const std::string& messageId) {
    ACSDK_DEBUG1(LX(__func__));

    if (m_audioPlayQueue.empty()) {
        ACSDK_ERROR(LX("executePlayFailed").d("reason", "emptyPlayQueue"));
        return;
    }

    auto playItem = m_audioPlayQueue.front();
    if (playItem->playBehavior != PlayBehavior::ENQUEUE && playItem->messageId != messageId) {
        ACSDK_ERROR(LX("executePlayFailed").d("reason", "TrackNotHeadOfQueue"));
        return;
    }

    if (playItem->playBehavior == PlayBehavior::REPLACE_ALL) {
        executeStop("", true);
    }

    // Determine Mixability of the item we are about to play.
    auto mixability = playItem->mixingBehavior == audio::MixingBehavior::BEHAVIOR_PAUSE
                          ? avsCommon::avs::ContentType::NONMIXABLE
                          : avsCommon::avs::ContentType::MIXABLE;

    // Initiate playback if not already playing.
    switch (m_currentActivity) {
        case PlayerActivity::IDLE:
        case PlayerActivity::STOPPED:
        case PlayerActivity::FINISHED:
            if (FocusState::NONE == m_focus) {
                auto activity = FocusManagerInterface::Activity::create(
                    NAMESPACE, shared_from_this(), std::chrono::milliseconds::zero(), mixability);
                // If we don't currently have focus, acquire it now; playback will start when focus changes to
                // FOREGROUND.
                if (m_isAcquireChannelRequestPending) {
                    ACSDK_INFO(LX("executePlay")
                                   .d("No-op m_isAcquireChannelRequestPending", m_isAcquireChannelRequestPending));
                } else if (m_focusManager->acquireChannel(CHANNEL_NAME, activity)) {
                    ACSDK_INFO(LX("executePlay").d("acquiring Channel", CHANNEL_NAME));
                    m_currentMixability = mixability;
                } else {
                    ACSDK_ERROR(LX("executePlayFailed").d("reason", "CouldNotAcquireChannel"));
                    m_progressTimer.stop();
                    sendPlaybackFailedEvent(
                        playItem->audioItem.stream.token,
                        ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR,
                        std::string("Could not acquire ") + CHANNEL_NAME + " for " + NAMESPACE,
                        getMediaPlayerState());
                }
            }
            return;
        case PlayerActivity::PAUSED:
            if (m_focus == FocusState::BACKGROUND) {
                auto activity = FocusManagerInterface::Activity::create(
                    NAMESPACE, shared_from_this(), std::chrono::milliseconds::zero(), mixability);
                if (m_focusManager->acquireChannel(CHANNEL_NAME, activity)) {
                    ACSDK_INFO(LX("executePlay").d("acquiring Channel", CHANNEL_NAME));
                    m_isAcquireChannelRequestPending = true;
                }
            }
            return;

        case PlayerActivity::PLAYING:
        case PlayerActivity::BUFFER_UNDERRUN:
            // If we're already 'playing', the new song should have been enqueued above and there's nothing more to
            // do here.
            return;
    }
    ACSDK_ERROR(LX("executePlayFailed").d("reason", "unexpectedActivity").d("m_currentActivity", m_currentActivity));
}

void AudioPlayer::playNextItem() {
    ACSDK_DEBUG1(LX(__func__).d("m_audioPlayQueue.size", m_audioPlayQueue.size()));
    // Cancel any existing progress timer.  The new timer will start when playback starts.
    m_progressTimer.stop();
    if (m_audioPlayQueue.empty()) {
        sendPlaybackFailedEvent(
            m_currentlyPlaying->audioItem.stream.token,
            ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR,
            "queue is empty",
            getMediaPlayerState());
        ACSDK_ERROR(LX("playNextItemFailed").d("reason", "emptyQueue"));
        executeStop();
        return;
    }

    auto playItem = m_audioPlayQueue.front();
    m_audioPlayQueue.pop_front();

    submitMetric(
        m_metricRecorder,
        AUDIO_PLAYER_METRIC_PREFIX + TRACK_TIME_ON_QUEUE,
        playItem->queueTimeMetricData.setName(TRACK_TIME_ON_QUEUE).stopDurationTimer().build(),
        playItem->messageId,
        playItem->audioItem.id);

    releaseMediaPlayer(m_currentlyPlaying);
    m_currentlyPlaying = playItem;
    m_currentlyPlaying->initialOffset = m_currentlyPlaying->audioItem.stream.offset;
    m_offset = m_currentlyPlaying->initialOffset;

    // AudioPlayer must notify of its mixability as it is based on content.
    auto mixability = m_currentlyPlaying->mixingBehavior == audio::MixingBehavior::BEHAVIOR_PAUSE
                          ? avsCommon::avs::ContentType::NONMIXABLE
                          : avsCommon::avs::ContentType::MIXABLE;

    if (m_currentMixability != mixability) {
        m_currentMixability = mixability;
        m_focusManager->modifyContentType(CHANNEL_NAME, NAMESPACE, mixability);
    }

    if (!m_currentlyPlaying->mediaPlayer) {
        if (m_mediaPlayerFactory->isMediaPlayerAvailable()) {
            if (!configureMediaPlayer(m_currentlyPlaying)) {
                // error log already generated
                return;
            }
        }
    }
    if (!m_currentlyPlaying->mediaPlayer) {
        ACSDK_ERROR(LX("playNextItemFailed").m("playerNotConfigured"));
        sendPlaybackFailedEvent(
            m_currentlyPlaying->audioItem.stream.token,
            ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR,
            "player not configured",
            getMediaPlayerState());
        return;
    }

    if (!m_currentlyPlaying->errorMsg.empty()) {
        ACSDK_ERROR(LX("playNextItemFailed").m("reportingPrebufferedError"));
        executeOnPlaybackError(
            m_currentlyPlaying->sourceId,
            m_currentlyPlaying->errorType,
            m_currentlyPlaying->errorMsg,
            getMediaPlayerState());
        return;
    }

    ACSDK_DEBUG1(LX(__func__).d("playingSourceId", m_currentlyPlaying->sourceId));
    if (!m_currentlyPlaying->mediaPlayer->play(m_currentlyPlaying->sourceId)) {
        executeOnPlaybackError(
            m_currentlyPlaying->sourceId,
            ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR,
            "playFailed",
            getMediaPlayerState());
        return;
    }
    m_isStartingPlayback = true;

    if (m_captionManager && m_currentlyPlaying->audioItem.captionData.isValid()) {
        m_captionManager->onCaption(m_currentlyPlaying->sourceId, m_currentlyPlaying->audioItem.captionData);
    }

    m_progressTimer.init(
        shared_from_this(),
        m_currentlyPlaying->audioItem.stream.progressReport.delay,
        m_currentlyPlaying->audioItem.stream.progressReport.interval,
        m_currentlyPlaying->initialOffset);
}

void AudioPlayer::executeStop(const std::string& messageId, bool playNextItem) {
    ACSDK_DEBUG1(LX("executeStop")
                     .d("playNextItem", playNextItem)
                     .d("m_currentActivity", m_currentActivity)
                     .d("sourceId", m_currentlyPlaying->sourceId));
    m_currentlyPlaying->stopMessageId = messageId;
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
            if (m_currentlyPlaying->mediaPlayer &&
                !m_currentlyPlaying->mediaPlayer->stop(m_currentlyPlaying->sourceId)) {
                ACSDK_ERROR(
                    LX("executeStopFailed").d("reason", "stopFailed").d("sourceId", m_currentlyPlaying->sourceId));
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
            clearPlayQueue(false);
            break;
        case ClearBehavior::CLEAR_ENQUEUED:
            clearPlayQueue(true);
            break;
    }
    sendPlaybackQueueClearedEvent();
}

void AudioPlayer::executeUpdateProgressReportInterval(std::chrono::milliseconds progressReportInterval) {
    ACSDK_DEBUG1(LX("executeUpdateProgressReportInterval").d("progressReportInterval", progressReportInterval.count()));
    m_progressTimer.updateInterval(progressReportInterval);
}

void AudioPlayer::executeLocalOperation(PlaybackOperation op, std::promise<bool> success) {
    ACSDK_DEBUG1(
        LX(__func__).d("m_currentActivity", m_currentActivity).d("sourceId", m_currentlyPlaying->sourceId).d("op", op));

    if (m_currentlyPlaying->mediaPlayer && m_currentlyPlaying->sourceId != ERROR_SOURCE_ID) {
        switch (m_currentActivity) {
            case PlayerActivity::STOPPED:
                if (op == PlaybackOperation::RESUME_PLAYBACK) {
                    if (FocusState::FOREGROUND != m_focus) {
                        auto activity = FocusManagerInterface::Activity::create(
                            NAMESPACE, shared_from_this(), std::chrono::milliseconds::zero(), m_currentMixability);
                        // If we don't currently have focus, acquire it now; playback will start when focus changes to
                        // FOREGROUND.
                        if (m_isAcquireChannelRequestPending) {
                            ACSDK_INFO(
                                LX("executeLocalResume")
                                    .d("No-op m_isAcquireChannelRequestPending", m_isAcquireChannelRequestPending));
                            m_isLocalResumePending = true;
                            m_localResumeSuccess.swap(success);
                        } else if (m_focusManager->acquireChannel(CHANNEL_NAME, activity)) {
                            ACSDK_INFO(LX("executeLocalResume").d("acquiring Channel", CHANNEL_NAME));
                            m_isAcquireChannelRequestPending = true;
                            m_isLocalResumePending = true;
                            m_localResumeSuccess.swap(success);
                        } else {
                            ACSDK_ERROR(LX("executeLocalResumeFailed").d("reason", "CouldNotAcquireChannel"));
                            m_progressTimer.stop();
                            success.set_value(false);
                            sendPlaybackFailedEvent(
                                m_currentlyPlaying->audioItem.stream.token,
                                ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR,
                                std::string("Could not acquire ") + CHANNEL_NAME + " for " + NAMESPACE,
                                getMediaPlayerState());
                        }
                    } else {
                        m_currentlyPlaying->initialOffset = getOffset();
                        success.set_value(m_currentlyPlaying->mediaPlayer->play(m_currentlyPlaying->sourceId));
                    }
                } else {
                    // we are stopped, so ok...
                    success.set_value(true);
                }
                break;

            case PlayerActivity::IDLE:
            case PlayerActivity::FINISHED:
                // cannot resume from this
                success.set_value(false);
                break;

            case PlayerActivity::PLAYING:
            case PlayerActivity::PAUSED:
            case PlayerActivity::BUFFER_UNDERRUN:
                if (op != PlaybackOperation::RESUME_PLAYBACK) {
                    // Make sure we have the offset cached before stopping.
                    m_currentlyPlaying->initialOffset = getOffset();
                    // Request to stop.
                    if (op == PlaybackOperation::STOP_PLAYBACK) {
                        m_isStopCalled = m_currentlyPlaying->mediaPlayer->stop(m_currentlyPlaying->sourceId);
                    } else {
                        m_isStopCalled = m_currentlyPlaying->mediaPlayer->stop(
                            m_currentlyPlaying->sourceId, LOCAL_STOP_DEFAULT_PIPELINE_OPEN_TIME);
                    }
                    success.set_value(m_isStopCalled);
                } else {
                    success.set_value(true);
                }
        }
    }
}

bool AudioPlayer::executeLocalSeekTo(std::chrono::milliseconds location, bool fromStart) {
    ACSDK_DEBUG1(LX(__func__)
                     .d("m_currentActivity", m_currentActivity)
                     .d("sourceId", m_currentlyPlaying->sourceId)
                     .d("seekTo", location.count())
                     .d("fromStart", fromStart));

    switch (m_currentActivity) {
        case PlayerActivity::IDLE:
        case PlayerActivity::FINISHED:
            // cannot seek
            return false;

        case PlayerActivity::STOPPED:
        case PlayerActivity::PLAYING:
        case PlayerActivity::PAUSED:
        case PlayerActivity::BUFFER_UNDERRUN:
            if (m_currentlyPlaying->mediaPlayer && m_currentlyPlaying->sourceId != ERROR_SOURCE_ID) {
                return m_currentlyPlaying->mediaPlayer->seekTo(m_currentlyPlaying->sourceId, location, fromStart);
            }
    }
    return false;
}

std::chrono::milliseconds AudioPlayer::getDuration() {
    // If the mediaplayer / source id is not set, do not ask MediaPlayer for the state.
    if (m_currentlyPlaying->mediaPlayer && m_currentlyPlaying->sourceId != ERROR_SOURCE_ID) {
        auto state = m_currentlyPlaying->mediaPlayer->getMediaPlayerState(m_currentlyPlaying->sourceId);
        if (state.hasValue()) {
            return state.value().duration;
        }
    }

    ACSDK_ERROR(LX(__func__).m("getDurationFailed"));

    return std::chrono::milliseconds::zero();
}

void AudioPlayer::changeActivity(PlayerActivity activity) {
    ACSDK_DEBUG(LX("changeActivity").d("from", m_currentActivity).d("to", activity));
    std::unique_lock<std::mutex> lock(m_currentActivityMutex);
    auto oldActivity = m_currentActivity;
    m_currentActivity = activity;
    lock.unlock();
    m_currentActivityConditionVariable.notify_all();
    if (oldActivity != m_currentActivity && oldActivity == PlayerActivity::PLAYING) {
        // stop playing, update time metric
        submitMetric(
            m_metricRecorder,
            AUDIO_PLAYER_METRIC_PREFIX + MEDIA_PLAYBACK_TIME,
            m_playbackTimeMetricData.setName(MEDIA_PLAYBACK_TIME).stopDurationTimer().build(),
            "",
            "");
    } else if (oldActivity != m_currentActivity && m_currentActivity == PlayerActivity::PLAYING) {
        m_playbackTimeMetricData.startDurationTimer();
    }
    executeProvideState();
    notifyObserver();
}

void AudioPlayer::sendEventWithTokenAndOffset(
    const std::string& eventName,
    bool includePlaybackReports,
    std::chrono::milliseconds offset) {
    rapidjson::Document payload(rapidjson::kObjectType);
    payload.AddMember(TOKEN_KEY, m_currentlyPlaying->audioItem.stream.token, payload.GetAllocator());
    // Note: offset is an optional parameter, which defaults to MEDIA_PLAYER_INVALID_OFFSET.  Per documentation,
    // this function will use the current MediaPlayer offset is a valid offset was not provided.
    if (MEDIA_PLAYER_INVALID_OFFSET == offset) {
        offset = getOffset();
    }
    ACSDK_DEBUG1(LX("sendEventWithTokenAndOffset")
                     .d("eventName", eventName)
                     .d("offset", offset.count())
                     .d("report", includePlaybackReports));
    payload.AddMember(
        OFFSET_KEY,
        (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(offset).count(),
        payload.GetAllocator());
    attachPlaybackAttributesIfAvailable(payload, payload.GetAllocator());
    if (includePlaybackReports) {
        attachPlaybackReportsIfAvailable(payload, payload.GetAllocator());
    }

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX("sendEventWithTokenAndOffsetFailed").d("reason", "writerRefusedJsonObject"));
        return;
    }

    sendEvent(eventName, "", buffer.GetString());
}

void AudioPlayer::sendPlaybackStartedEvent(const MediaPlayerState& state) {
    sendEventWithTokenAndOffset("PlaybackStarted", false, m_currentlyPlaying->initialOffset);
}

void AudioPlayer::sendPlaybackNearlyFinishedEvent(const MediaPlayerState& state) {
    sendEventWithTokenAndOffset("PlaybackNearlyFinished", false, state.offset);
}

void AudioPlayer::sendPlaybackStutterStartedEvent(const MediaPlayerState& state) {
    sendEventWithTokenAndOffset("PlaybackStutterStarted", false, state.offset);
}

void AudioPlayer::sendPlaybackStutterFinishedEvent(const MediaPlayerState& state) {
    rapidjson::Document payload(rapidjson::kObjectType);
    payload.AddMember(TOKEN_KEY, m_currentlyPlaying->audioItem.stream.token, payload.GetAllocator());
    payload.AddMember(
        OFFSET_KEY,
        (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(state.offset).count(),
        payload.GetAllocator());
    auto stutterDuration = std::chrono::steady_clock::now() - m_bufferUnderrunTimestamp;
    payload.AddMember(
        STUTTER_DURATION_KEY,
        (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(stutterDuration).count(),
        payload.GetAllocator());
    attachPlaybackAttributesIfAvailable(payload, payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX("sendPlaybackStutterFinishedEventFailed").d("reason", "writerRefusedJsonObject"));
        return;
    }

    sendEvent("PlaybackStutterFinished", "", buffer.GetString());
}

void AudioPlayer::sendPlaybackFinishedEvent(const MediaPlayerState& state) {
    sendEventWithTokenAndOffset("PlaybackFinished", true, state.offset);
}

void AudioPlayer::sendPlaybackFailedEvent(
    const std::string& failingToken,
    ErrorType errorType,
    const std::string& message,
    const MediaPlayerState& state) {
    rapidjson::Document payload(rapidjson::kObjectType);
    payload.AddMember(TOKEN_KEY, failingToken, payload.GetAllocator());

    rapidjson::Value currentPlaybackState(rapidjson::kObjectType);
    currentPlaybackState.AddMember(TOKEN_KEY, m_currentlyPlaying->audioItem.stream.token, payload.GetAllocator());
    currentPlaybackState.AddMember(
        OFFSET_KEY,
        (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(state.offset).count(),
        payload.GetAllocator());
    currentPlaybackState.AddMember(ACTIVITY_KEY, playerActivityToString(m_currentActivity), payload.GetAllocator());
    attachPlaybackAttributesIfAvailable(currentPlaybackState, payload.GetAllocator());

    payload.AddMember("currentPlaybackState", currentPlaybackState, payload.GetAllocator());

    rapidjson::Value error(rapidjson::kObjectType);
    error.AddMember("type", errorTypeToString(errorType), payload.GetAllocator());
    error.AddMember("message", message, payload.GetAllocator());

    payload.AddMember("error", error, payload.GetAllocator());
    attachPlaybackReportsIfAvailable(payload, payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX("sendPlaybackStartedEventFailed").d("reason", "writerRefusedJsonObject"));
        return;
    }

    sendEvent("PlaybackFailed", "", buffer.GetString());
}

void AudioPlayer::sendPlaybackStoppedEvent(const MediaPlayerState& state) {
    sendEventWithTokenAndOffset("PlaybackStopped", true, state.offset);
}

void AudioPlayer::sendPlaybackPausedEvent(const MediaPlayerState& state) {
    sendEventWithTokenAndOffset("PlaybackPaused", false, state.offset);
}

void AudioPlayer::sendPlaybackResumedEvent(const MediaPlayerState& state) {
    sendEventWithTokenAndOffset("PlaybackResumed", false, state.offset);
}

void AudioPlayer::sendPlaybackSeekedEvent(const MediaPlayerState& startState, const MediaPlayerState& endState) {
    rapidjson::Document payload(rapidjson::kObjectType);
    payload.AddMember(TOKEN_KEY, m_currentlyPlaying->audioItem.stream.token, payload.GetAllocator());
    payload.AddMember(SEEK_START_OFFSET_KEY, (int64_t)startState.offset.count(), payload.GetAllocator());
    payload.AddMember(SEEK_END_OFFSET_KEY, (int64_t)endState.offset.count(), payload.GetAllocator());
    payload.AddMember(ACTIVITY_KEY, playerActivityToString(m_currentActivity), payload.GetAllocator());
    attachPlaybackAttributesIfAvailable(payload, payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX("sendPlaybackSeekedEventFailed").d("reason", "writerRefusedJsonObject"));
        return;
    }

    /* This is how it is supposed to work as of AudioPlayer 1.6.
     * For now, we send playbackStarted or Stopped
    sendEvent("PlaybackSeeked", "", buffer.GetString());
     */
    // Until AudioPlayer 1.6 only.  Yes, it violates the documented 'one started event per track' policy, but it works.
    if (m_currentActivity == PlayerActivity::PLAYING) {
        m_currentlyPlaying->initialOffset = endState.offset;
        sendPlaybackStartedEvent(endState);
    } else {
        sendPlaybackStoppedEvent(endState);
        // Reset 'StartOffset', so the following PlaybackStarted will be correct
        m_currentlyPlaying->initialOffset = endState.offset;
    }
}

void AudioPlayer::sendPlaybackQueueClearedEvent() {
    sendEvent("PlaybackQueueCleared");
}

void AudioPlayer::sendStreamMetadataExtractedEvent(
    AudioItem& audioItem,
    std::shared_ptr<const VectorOfTags> vectorOfTags,
    const MediaPlayerState&) {
    const std::string& token = audioItem.stream.token;
    rapidjson::Document payload(rapidjson::kObjectType);
    payload.AddMember(TOKEN_KEY, token, payload.GetAllocator());

    rapidjson::Value metadata(rapidjson::kObjectType);
    bool passFilter = false;
    // Make a copy of the duplicate cache
    VectorOfTags newCache(audioItem.cachedMetadata.begin(), audioItem.cachedMetadata.end());
    for (auto& tag : *vectorOfTags) {
        // Filter metadata against whitelist
        std::string lowerTag = tag.key;
        std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), ::tolower);
        if (std::find(std::begin(METADATA_WHITELIST), std::end(METADATA_WHITELIST), lowerTag) !=
                std::end(METADATA_WHITELIST) &&
            !tag.value.empty()) {
            // Check for duplicates
            bool dupFound = false;
            for (auto iter = newCache.begin(); iter != newCache.end(); ++iter) {
                if (iter->key == tag.key) {
                    if (iter->value == tag.value && iter->type == tag.type) {
                        // duplicate, don't send
                        dupFound = true;
                    } else {
                        newCache.erase(iter);
                    }
                    break;
                }
            }
            if (dupFound) {
                continue;
            }
            passFilter = true;
            newCache.push_back(tag);
            rapidjson::Value tagKey(tag.key.c_str(), payload.GetAllocator());
            if (TagType::BOOLEAN == tag.type) {
                std::string value = tag.value;
                std::transform(value.begin(), value.end(), value.begin(), ::tolower);
                if (value == "true") {
                    metadata.AddMember(tagKey, true, payload.GetAllocator());
                } else {
                    metadata.AddMember(tagKey, false, payload.GetAllocator());
                }
            } else {
                rapidjson::Value tagValue(tag.value.c_str(), payload.GetAllocator());
                metadata.AddMember(tagKey, tagValue, payload.GetAllocator());
            }
        }
    }

    if (!passFilter) {
        submitMetric(
            m_metricRecorder,
            AUDIO_PLAYER_METRIC_PREFIX + METADATA_UNFILTERED_ENCOUNTERED,
            DataPointCounterBuilder{}.setName(METADATA_UNFILTERED_ENCOUNTERED).increment(1).build(),
            "",
            token);
        ACSDK_DEBUG(LX("sendStreamMetadataExtractedEvent").d("eventNotSent", "noWhitelistedData"));
        return;
    } else {
        submitMetric(
            m_metricRecorder,
            AUDIO_PLAYER_METRIC_PREFIX + METADATA_FILTERED_ENCOUNTERED,
            DataPointCounterBuilder{}.setName(METADATA_FILTERED_ENCOUNTERED).increment(1).build(),
            "",
            token);
    }

    std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
    if (audioItem.lastMetadataEvent.time_since_epoch().count() == 0 ||
        now - audioItem.lastMetadataEvent > METADATA_EVENT_RATE) {
        audioItem.lastMetadataEvent = now;
        // Only keep the updated cache if we actually send the event.
        audioItem.cachedMetadata = newCache;
    } else {
        ACSDK_DEBUG(LX("sendStreamMetadataExtractedEvent").d("eventNotSent", "tooFrequent"));
        return;
    }

    payload.AddMember("metadata", metadata, payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX("sendStreamMetadataExtractedEvent").d("reason", "writerRefusedJsonObject"));
        return;
    }
    sendEvent("StreamMetadataExtracted", "", buffer.GetString());
}

void AudioPlayer::notifyObserver() {
    AudioPlayerObserverInterface::Context context;
    context.audioItemId = m_currentlyPlaying->audioItem.id;
    context.offset = getOffset();
    context.playRequestor = m_currentlyPlaying->playRequestor;

    ACSDK_DEBUG1(LX("notifyObserver").d("playerActivity", playerActivityToString(m_currentActivity)));

    for (const auto& observer : m_observers) {
        observer->onPlayerActivityChanged(m_currentActivity, context);
    }

    if (m_renderPlayerObserver) {
        RenderPlayerInfoCardsObserverInterface::Context renderPlayerInfoContext;
        renderPlayerInfoContext.audioItemId = m_currentlyPlaying->audioItem.id;
        renderPlayerInfoContext.offset = getOffset();
        renderPlayerInfoContext.mediaProperties = shared_from_this();
        m_renderPlayerObserver->onRenderPlayerCardsInfoChanged(m_currentActivity, renderPlayerInfoContext);
    }
}

std::chrono::milliseconds AudioPlayer::getOffset() {
    // If the mediaplayer / source id is not set, do not ask MediaPlayer for the offset.
    if (m_currentlyPlaying->mediaPlayer && m_currentlyPlaying->sourceId != ERROR_SOURCE_ID) {
        auto offset = m_currentlyPlaying->mediaPlayer->getOffset(m_currentlyPlaying->sourceId);
        if (offset != MEDIA_PLAYER_INVALID_OFFSET) {
            m_offset = offset;
        }
    }

    return m_offset;
}

void AudioPlayer::sendEvent(
    const std::string& eventName,
    const std::string& dialogRequestIdString,
    const std::string& payload,
    const std::string& context) {
    auto event = buildJsonEventString(eventName, dialogRequestIdString, payload, context);
    auto request = std::make_shared<MessageRequest>(event.second, "");
    m_messageSender->sendMessage(request);
}

MediaPlayerState AudioPlayer::getMediaPlayerState() {
    MediaPlayerState state = MediaPlayerState(getOffset(), getDuration());
    return state;
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> AudioPlayer::
    getCapabilityConfigurations() {
    return m_capabilityConfigurations;
}

void AudioPlayer::attachPlaybackAttributesIfAvailable(
    rapidjson::Value& parent,
    rapidjson::Document::AllocatorType& allocator) {
    if (!m_currentlyPlaying->mediaPlayer) {
        return;
    }

    Optional<PlaybackAttributes> playbackAttributes = m_currentlyPlaying->mediaPlayer->getPlaybackAttributes();
    if (!playbackAttributes.hasValue()) {
        return;
    }

    rapidjson::Value jsonPlaybackAttributes(rapidjson::kObjectType);
    jsonPlaybackAttributes.AddMember(NAME_KEY, playbackAttributes.value().name, allocator);
    jsonPlaybackAttributes.AddMember(CODEC_KEY, playbackAttributes.value().codec, allocator);
    jsonPlaybackAttributes.AddMember(
        SAMPLING_RATE_IN_HERTZ_KEY, static_cast<int64_t>(playbackAttributes.value().samplingRateInHertz), allocator);
    jsonPlaybackAttributes.AddMember(
        DATA_RATE_IN_BITS_PER_SECOND_KEY,
        static_cast<int64_t>(playbackAttributes.value().dataRateInBitsPerSecond),
        allocator);

    parent.AddMember(PLAYBACK_ATTRIBUTES_KEY, jsonPlaybackAttributes, allocator);
}

void AudioPlayer::attachPlaybackReportsIfAvailable(
    rapidjson::Value& parent,
    rapidjson::Document::AllocatorType& allocator) {
    if (!m_currentlyPlaying->mediaPlayer) {
        return;
    }

    std::vector<PlaybackReport> playbackReports = m_currentlyPlaying->mediaPlayer->getPlaybackReports();
    if (playbackReports.empty()) {
        return;
    }

    rapidjson::Value jsonPlaybackReports(rapidjson::kArrayType);
    for (auto& report : playbackReports) {
        rapidjson::Value jsonReport(rapidjson::kObjectType);
        jsonReport.AddMember(START_OFFSET_KEY, static_cast<int64_t>(report.startOffset.count()), allocator);
        jsonReport.AddMember(END_OFFSET_KEY, static_cast<int64_t>(report.endOffset.count()), allocator);

        rapidjson::Value jsonPlaybackAttributes(rapidjson::kObjectType);
        jsonPlaybackAttributes.AddMember(NAME_KEY, report.playbackAttributes.name, allocator);
        jsonPlaybackAttributes.AddMember(CODEC_KEY, report.playbackAttributes.codec, allocator);
        jsonPlaybackAttributes.AddMember(
            SAMPLING_RATE_IN_HERTZ_KEY, static_cast<int64_t>(report.playbackAttributes.samplingRateInHertz), allocator);
        jsonPlaybackAttributes.AddMember(
            DATA_RATE_IN_BITS_PER_SECOND_KEY,
            static_cast<int64_t>(report.playbackAttributes.dataRateInBitsPerSecond),
            allocator);

        jsonReport.AddMember(PLAYBACK_ATTRIBUTES_KEY, jsonPlaybackAttributes, allocator);
        jsonPlaybackReports.PushBack(jsonReport, allocator);
    }
    parent.AddMember(PLAYBACK_REPORTS_KEY, jsonPlaybackReports, allocator);
}

void AudioPlayer::parseHeadersFromPlayDirective(const rapidjson::Value& httpHeaders, AudioItem& audioItem) {
    jsonUtils::retrieveStringMapFromArray(
        httpHeaders, PlaybackContext::HTTP_KEY_HEADERS, audioItem.playbackContext.keyConfig);
    jsonUtils::retrieveStringMapFromArray(
        httpHeaders, PlaybackContext::HTTP_MANIFEST_HEADERS, audioItem.playbackContext.manifestConfig);
    jsonUtils::retrieveStringMapFromArray(
        httpHeaders, PlaybackContext::HTTP_AUDIOSEGMENT_HEADERS, audioItem.playbackContext.audioSegmentConfig);
    jsonUtils::retrieveStringMapFromArray(
        httpHeaders, PlaybackContext::HTTP_ALL_HEADERS, audioItem.playbackContext.allConfig);
    if (validatePlaybackContextHeaders(&(audioItem.playbackContext))) {
        ACSDK_WARN(
            LX("validatePlaybackContext").m("Error in validation of headers-attempt to playback with valid data"));
    }
}

}  // namespace acsdkAudioPlayer
}  // namespace alexaClientSDK
