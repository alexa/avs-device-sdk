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

#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <ratio>
#include <string>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include <AVSCommon/AVS/SpeakerConstants/SpeakerConstants.h>
#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <acsdkEqualizerImplementations/EqualizerLinearBandMapper.h>
#include <PlaylistParser/IterativePlaylistParser.h>

#include "AndroidSLESMediaPlayer/AndroidSLESMediaPlayer.h"
#include "AndroidSLESMediaPlayer/AndroidSLESSpeaker.h"
#include "AndroidSLESMediaPlayer/FFmpegAttachmentInputController.h"
#include "AndroidSLESMediaPlayer/FFmpegDecoder.h"
#include "AndroidSLESMediaPlayer/FFmpegStreamInputController.h"
#include "AndroidSLESMediaPlayer/FFmpegUrlInputController.h"

/// String to identify log entries originating from this file.
#define TAG "AndroidSLESMediaPlayer"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace mediaPlayer {
namespace android {

using namespace applicationUtilities::androidUtilities;
using namespace avsCommon::utils::mediaPlayer;
using namespace alexaClientSDK::acsdkEqualizerInterfaces;
using namespace alexaClientSDK::acsdkEqualizer;
using MediaPlayerState = avsCommon::utils::mediaPlayer::MediaPlayerState;

/// The playback audio sample rate in HZ.
static constexpr size_t SAMPLE_RATE_HZ = 48000;

/// The number of channels.
static constexpr size_t NUMBER_OF_CHANNELS = 2;

/// The playback audio sample size in bits.
static constexpr size_t SAMPLE_SIZE_BITS = 16;

/// The playback audio sample size in bytes.
static constexpr size_t SAMPLE_SIZE_BYTES = SAMPLE_SIZE_BITS / 8;

/// Multiplier to convert dB to mB.
static constexpr int DECIBELL_TO_MILLIBELL_MULT = 100;

/// A counter used to increment the source id when a new source is set.
static std::atomic<MediaPlayerInterface::SourceId> g_id{1};

/// The data locator used to configure the android media player to use a buffer queue.
static SLDataLocator_AndroidSimpleBufferQueue DATA_LOCATOR = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                              AndroidSLESMediaQueue::NUMBER_OF_BUFFERS};

static SLuint32 convertSampleSize(PlaybackConfiguration::SampleFormat format) {
    switch (format) {
        case PlaybackConfiguration::SampleFormat::UNSIGNED_8:
            return SL_PCMSAMPLEFORMAT_FIXED_8;
        case PlaybackConfiguration::SampleFormat::SIGNED_16:
            return SL_PCMSAMPLEFORMAT_FIXED_16;
        case PlaybackConfiguration::SampleFormat::SIGNED_32:
            return SL_PCMSAMPLEFORMAT_FIXED_32;
    }
    ACSDK_ERROR(LX("invalidFormat").d("format", static_cast<int>(format)));
    return SL_PCMSAMPLEFORMAT_FIXED_16;
}

static SLuint32 convertLayout(PlaybackConfiguration::ChannelLayout layout) {
    switch (layout) {
        case PlaybackConfiguration::ChannelLayout::LAYOUT_MONO:
            return SL_SPEAKER_FRONT_CENTER;
        case PlaybackConfiguration::ChannelLayout::LAYOUT_STEREO:
            return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
        case PlaybackConfiguration::ChannelLayout::LAYOUT_SURROUND:
            return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT | SL_SPEAKER_FRONT_CENTER;
        case PlaybackConfiguration::ChannelLayout::LAYOUT_5POINT1:
            return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT | SL_SPEAKER_FRONT_CENTER | SL_SPEAKER_BACK_LEFT |
                   SL_SPEAKER_BACK_RIGHT | SL_SPEAKER_LOW_FREQUENCY;
    }
    ACSDK_ERROR(LX("invalidLayout").d("layout", static_cast<int>(layout)));
    return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
}

SLDataFormat_PCM convertFormat(const PlaybackConfiguration& playbackConfiguration) {
    auto endianness = playbackConfiguration.isLittleEndian() ? SL_BYTEORDER_LITTLEENDIAN : SL_BYTEORDER_BIGENDIAN;
    auto rate = playbackConfiguration.sampleRate() * 1000;
    auto containerSize = convertSampleSize(playbackConfiguration.sampleFormat());
    auto layout = convertLayout(playbackConfiguration.channelLayout());
    return SLDataFormat_PCM{SL_DATAFORMAT_PCM,                                              // Output format
                            static_cast<SLuint32>(playbackConfiguration.numberChannels()),  // Number of channels
                            static_cast<SLuint32>(rate),                                    // Sample rate in mHZ
                            containerSize,                                                  // Bits per sample
                            containerSize,                                                  // Container size
                            layout,                                                         // Speaker layout
                            endianness};                                                    // Audio endianness
}

AndroidSLESMediaPlayer::SourceId AndroidSLESMediaPlayer::setSource(
    std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader,
    const avsCommon::utils::AudioFormat* format,
    const avsCommon::utils::mediaPlayer::SourceConfig& config) {
    auto input = FFmpegAttachmentInputController::create(attachmentReader, format);
    auto newId = configureNewRequest(std::move(input), config);
    if (ERROR == newId) {
        ACSDK_ERROR(
            LX("setSourceFailed").d("name", RequiresShutdown::name()).d("type", "attachment").d("format", format));
    }
    return newId;
}

AndroidSLESMediaPlayer::SourceId AndroidSLESMediaPlayer::setSource(
    std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader,
    std::chrono::milliseconds offsetAdjustment,
    const avsCommon::utils::AudioFormat* format,
    const avsCommon::utils::mediaPlayer::SourceConfig& config) {
    auto input = FFmpegAttachmentInputController::create(attachmentReader, format);
    auto newId = configureNewRequest(std::move(input), config, nullptr, offsetAdjustment);
    if (ERROR == newId) {
        ACSDK_ERROR(
            LX("setSourceFailed").d("name", RequiresShutdown::name()).d("type", "attachment").d("format", format));
    }
    return newId;
}

AndroidSLESMediaPlayer::SourceId AndroidSLESMediaPlayer::setSource(
    const std::string& url,
    std::chrono::milliseconds offset,
    const avsCommon::utils::mediaPlayer::SourceConfig& config,
    bool repeat,
    const avsCommon::utils::mediaPlayer::PlaybackContext& playbackContext) {
    std::shared_ptr<playlistParser::IterativePlaylistParser> playlistParser =
        playlistParser::IterativePlaylistParser::create(m_contentFetcherFactory);
    auto input = FFmpegUrlInputController::create(playlistParser, url, offset, repeat);
    auto newId = configureNewRequest(std::move(input), config, playlistParser, offset);
    if (ERROR == newId) {
        ACSDK_ERROR(LX("setSourceFailed")
                        .d("name", RequiresShutdown::name())
                        .d("type", "url")
                        .d("offset(ms)", offset.count())
                        .sensitive("url", url));
    }

    return newId;
}

AndroidSLESMediaPlayer::SourceId AndroidSLESMediaPlayer::setSource(
    std::shared_ptr<std::istream> stream,
    bool repeat,
    const avsCommon::utils::mediaPlayer::SourceConfig& config,
    avsCommon::utils::MediaType format) {
    auto input = FFmpegStreamInputController::create(stream, repeat);
    auto newId = configureNewRequest(std::move(input), config);
    if (ERROR == newId) {
        ACSDK_ERROR(LX("setSourceFailed")
                        .d("name", RequiresShutdown::name())
                        .d("type", "istream")
                        .d("repeat", repeat)
                        .d("format", format));
    }
    return newId;
}

bool AndroidSLESMediaPlayer::play(SourceId id) {
    ACSDK_DEBUG7(LX(__func__).d("requestId", id));

    std::lock_guard<std::mutex> lock{m_operationMutex};
    if (id == m_sourceId) {
        SLuint32 state;
        auto result = (*m_player)->GetPlayState(m_player, &state);
        if (result != SL_RESULT_SUCCESS) {
            ACSDK_ERROR(LX("playFailed")
                            .d("name", RequiresShutdown::name())
                            .d("reason", "getPlayStateFailed")
                            .d("result", result));
            return false;
        }

        if (SL_PLAYSTATE_STOPPED == state) {
            // set the player's state to playing
            result = (*m_player)->SetPlayState(m_player, SL_PLAYSTATE_PLAYING);
            if (result != SL_RESULT_SUCCESS) {
                ACSDK_ERROR(LX("playFailed")
                                .d("name", RequiresShutdown::name())
                                .d("reason", "setPlayStateFailed")
                                .d("result", result)
                                .d("id", id));
                return false;
            }
            auto observers = m_observers;
            for (const auto& observer : observers) {
                observer->onPlaybackStarted(id, createMediaPlayerState(id));
            }
            return true;
        }
        ACSDK_ERROR(LX("playFailed")
                        .d("name", RequiresShutdown::name())
                        .d("reason", "invalidState")
                        .d("requestId", id)
                        .d("state", state));
    } else {
        ACSDK_ERROR(LX("playFailed")
                        .d("name", RequiresShutdown::name())
                        .d("reason", "invalidId")
                        .d("requestId", id)
                        .d("currentId", m_sourceId));
    }
    return false;
}

bool AndroidSLESMediaPlayer::stop(SourceId id) {
    ACSDK_DEBUG7(LX(__func__).d("requestId", id));

    std::lock_guard<std::mutex> lock{m_operationMutex};
    if (id == m_sourceId) {
        return stopLocked();
    }
    ACSDK_ERROR(LX("stopFailed")
                    .d("name", RequiresShutdown::name())
                    .d("reason", "Invalid Id")
                    .d("requestId", id)
                    .d("currentId", id));
    return false;
}

bool AndroidSLESMediaPlayer::stopLocked() {
    SLuint32 state;
    auto result = (*m_player)->GetPlayState(m_player, &state);
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_ERROR(LX("stopFailed")
                        .d("name", RequiresShutdown::name())
                        .d("reason", "getPlayStateFailed")
                        .d("result", result)
                        .d("id", m_sourceId));
        return false;
    }

    if (state != SL_PLAYSTATE_STOPPED) {
        auto result = (*m_player)->SetPlayState(m_player, SL_PLAYSTATE_STOPPED);
        if (result != SL_RESULT_SUCCESS) {
            ACSDK_ERROR(LX("stopFailed")
                            .d("name", RequiresShutdown::name())
                            .d("reason", "setPlayStateFailed")
                            .d("result", result)
                            .d("id", m_sourceId));
            return false;
        }
        auto observers = m_observers;
        for (const auto& observer : observers) {
            observer->onPlaybackStopped(m_sourceId, createMediaPlayerState(m_sourceId));
        }
    }
    return true;
}

bool AndroidSLESMediaPlayer::pause(SourceId id) {
    ACSDK_DEBUG7(LX(__func__).d("requestId", id));

    std::lock_guard<std::mutex> lock{m_operationMutex};
    if (id == m_sourceId) {
        SLuint32 state;
        auto result = (*m_player)->GetPlayState(m_player, &state);
        if (result != SL_RESULT_SUCCESS) {
            ACSDK_ERROR(LX("pauseFailed")
                            .d("name", RequiresShutdown::name())
                            .d("reason", "getPlayStateFailed")
                            .d("result", result));
            return false;
        }

        if (SL_PLAYSTATE_PLAYING == state) {
            result = (*m_player)->SetPlayState(m_player, SL_PLAYSTATE_PAUSED);
            if (result != SL_RESULT_SUCCESS) {
                ACSDK_ERROR(LX("pauseFailed")
                                .d("name", RequiresShutdown::name())
                                .d("reason", "setPlayStateFailed")
                                .d("result", result)
                                .d("id", id));
                return false;
            }
            auto observers = m_observers;
            for (const auto& observer : observers) {
                observer->onPlaybackPaused(id, createMediaPlayerState(id));
            }
            return true;
        }
        ACSDK_ERROR(LX("pauseFailed")
                        .d("name", RequiresShutdown::name())
                        .d("reason", "invalidState")
                        .d("requestId", id)
                        .d("state", state));
    } else {
        ACSDK_ERROR(LX("pauseFailed")
                        .d("name", RequiresShutdown::name())
                        .d("reason", "invalidId")
                        .d("requestId", id)
                        .d("currentId", m_sourceId));
    }
    return false;
}

bool AndroidSLESMediaPlayer::resume(SourceId id) {
    ACSDK_DEBUG7(LX(__func__).d("requestId", id));

    std::lock_guard<std::mutex> lock{m_operationMutex};
    if (id == m_sourceId) {
        SLuint32 state;
        auto result = (*m_player)->GetPlayState(m_player, &state);
        if (result != SL_RESULT_SUCCESS) {
            ACSDK_ERROR(LX("resumeFailed")
                            .d("name", RequiresShutdown::name())
                            .d("reason", "getPlayStateFailed")
                            .d("result", result));
            return false;
        }

        if (SL_PLAYSTATE_PAUSED == state) {
            result = (*m_player)->SetPlayState(m_player, SL_PLAYSTATE_PLAYING);
            if (result != SL_RESULT_SUCCESS) {
                ACSDK_ERROR(LX("resumeFailed")
                                .d("name", RequiresShutdown::name())
                                .d("reason", "setPlayStateFailed")
                                .d("result", result)
                                .d("id", id));
                return false;
            }
            auto observers = m_observers;
            for (const auto& observer : observers) {
                observer->onPlaybackResumed(id, createMediaPlayerState(id));
            }
            return true;
        }
        ACSDK_ERROR(LX("resumeFailed")
                        .d("name", RequiresShutdown::name())
                        .d("reason", "invalidState")
                        .d("requestId", id)
                        .d("state", state));
    } else {
        ACSDK_ERROR(LX("resumeFailed")
                        .d("name", RequiresShutdown::name())
                        .d("reason", "invalidId")
                        .d("requestId", id)
                        .d("currentId", m_sourceId));
    }
    return false;
}

std::chrono::milliseconds AndroidSLESMediaPlayer::getOffset(SourceId id) {
    std::lock_guard<std::mutex> lock{m_requestMutex};
    return getOffsetLocked(id);
}

std::chrono::milliseconds AndroidSLESMediaPlayer::getOffsetLocked(SourceId id) {
    if (!m_mediaQueue) {
        ACSDK_ERROR(LX("getOffsetLockedFailed").d("reason", "nullMediaQueue"));
        return MEDIA_PLAYER_INVALID_OFFSET;
    }
    return m_initialOffset + computeDuration(m_mediaQueue->getNumBytesPlayed());
}

MediaPlayerState AndroidSLESMediaPlayer::createMediaPlayerState(SourceId id) {
    MediaPlayerState state = {getOffset(id)};
    return state;
}

uint64_t AndroidSLESMediaPlayer::getNumBytesBuffered() {
    std::lock_guard<std::mutex> lock{m_requestMutex};
    return m_mediaQueue->getNumBytesBuffered();
}

avsCommon::utils::Optional<MediaPlayerState> AndroidSLESMediaPlayer::getMediaPlayerState(SourceId id) {
    std::lock_guard<std::mutex> lock{m_requestMutex};
    auto offset = getOffsetLocked(id);
    if (offset == MEDIA_PLAYER_INVALID_OFFSET) {
        ACSDK_ERROR(LX("getMediaPlayerStateFailed").d("reason", "invalidOffset"));
        return avsCommon::utils::Optional<MediaPlayerState>();
    }
    auto state = MediaPlayerState(offset);
    return avsCommon::utils::Optional<MediaPlayerState>(state);
}

void AndroidSLESMediaPlayer::addObserver(std::shared_ptr<MediaPlayerObserverInterface> playerObserver) {
    if (m_hasShutdown) {
        ACSDK_ERROR(LX("addObserverFailed").d("name", RequiresShutdown::name()).d("reason", "playerHasShutdown"));
        return;
    }

    if (!playerObserver) {
        ACSDK_ERROR(LX("addObserverFailed").d("reason", "observer is null"));
        return;
    }
    std::lock_guard<std::mutex> lock{m_operationMutex};
    m_observers.insert(playerObserver);
}

void AndroidSLESMediaPlayer::removeObserver(std::shared_ptr<MediaPlayerObserverInterface> playerObserver) {
    if (!playerObserver) {
        ACSDK_ERROR(LX("removeObserverFailed").d("reason", "observer is null"));
        return;
    }
    std::lock_guard<std::mutex> lock{m_operationMutex};
    m_observers.erase(playerObserver);
}

void AndroidSLESMediaPlayer::doShutdown() {
    ACSDK_DEBUG9(LX(__func__).d("name", RequiresShutdown::name()));
    std::lock_guard<std::mutex> lock{m_operationMutex};
    stopLocked();
    m_observers.clear();
    m_sourceId = ERROR;
    m_hasShutdown = true;

    if (m_playlistParser) {
        m_playlistParser->abort();
    }

    if (m_prefetchStatus) {
        (*m_prefetchStatus)->RegisterCallback(m_prefetchStatus, nullptr, nullptr);
    }
}

AndroidSLESMediaPlayer::~AndroidSLESMediaPlayer() {
    ACSDK_DEBUG9(LX(__func__).d("name", RequiresShutdown::name()));
    m_mediaQueue.reset();
    doShutdown();
}

MediaPlayerInterface::SourceId AndroidSLESMediaPlayer::configureNewRequest(
    std::unique_ptr<FFmpegInputControllerInterface> inputController,
    const SourceConfig& config,
    std::shared_ptr<avsCommon::utils::playlistParser::IterativePlaylistParserInterface> playlistParser,
    std::chrono::milliseconds offset) {
    std::lock_guard<std::mutex> requestLock{m_requestMutex};
    {
        // Use global lock to stop player and set new source id.
        std::lock_guard<std::mutex> lock{m_operationMutex};
        if (m_hasShutdown) {
            ACSDK_ERROR(
                LX("configureNewRequestFailed").d("name", RequiresShutdown::name()).d("reason", "playerHasShutdown"));
            return ERROR;
        }

        stopLocked();
        m_sourceId = g_id.fetch_add(1);
        m_almostDone = false;
        m_initialOffset = offset;
    }

    if (m_playlistParser) {
        m_playlistParser->abort();
    }
    m_playlistParser = playlistParser;

    auto requestId = m_sourceId;
    auto callback = [this, requestId](AndroidSLESMediaQueue::QueueEvent status, const std::string& reason) {
        this->onQueueEvent(status, reason, requestId);
    };

    m_mediaQueue.reset();  // Delete old queue before configuring new one.

    auto decoder = FFmpegDecoder::create(std::move(inputController), m_config, config);
    m_mediaQueue = AndroidSLESMediaQueue::create(m_playerObject, std::move(decoder), callback, m_config);
    if (!m_mediaQueue) {
        ACSDK_ERROR(LX("configureNewRequestFailed")
                        .d("name", RequiresShutdown::name())
                        .d("reason", "failedToCreateMediaQueue")
                        .d("decoder", inputController.get()));
        return ERROR;
    }
    return m_sourceId;
}

std::unique_ptr<AndroidSLESMediaPlayer> AndroidSLESMediaPlayer::create(
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory,
    std::shared_ptr<AndroidSLESEngine> engine,
    bool enableEqualizer,
    const PlaybackConfiguration& config,
    const std::string& name) {
    if (!contentFetcherFactory) {
        ACSDK_ERROR(LX("createFailed").d("name", name).d("reason", "invalidContentFetcherFactory"));
        return nullptr;
    }

    if (!engine) {
        ACSDK_ERROR(LX("createFailed").d("name", name).d("reason", "invalidEngine"));
        return nullptr;
    }

    std::shared_ptr<AndroidSLESObject> outputMix = engine->createOutputMix();
    if (!outputMix) {
        ACSDK_ERROR(LX("createFailed").d("name", name).d("reason", "invalidOutputMix"));
        return nullptr;
    }

    // configure audio source
    auto dataFormat = convertFormat(config);
    SLDataSource audioSource = {&DATA_LOCATOR, &dataFormat};

    // configure audio sink
    SLDataLocator_OutputMix outputDataLocator = {SL_DATALOCATOR_OUTPUTMIX, outputMix->get()};
    SLDataSink audioSink = {&outputDataLocator, NULL};

    std::shared_ptr<AndroidSLESObject> playerObject = engine->createPlayer(audioSource, audioSink, enableEqualizer);
    if (!playerObject) {
        ACSDK_ERROR(LX("createFailed").d("name", name).d("reason", "createPlayerFailed"));
        return nullptr;
    }

    SLPlayItf playerInterface;
    if (!playerObject->getInterface(SL_IID_PLAY, &playerInterface)) {
        ACSDK_ERROR(LX("createFailed").d("name", name).d("reason", "getPlayerInterfaceFailed"));
        return nullptr;
    }

    auto speaker = AndroidSLESSpeaker::create(engine, outputMix, playerObject);
    if (!speaker) {
        ACSDK_ERROR(LX("createFailed").d("name", name).d("reason", "createSpeakerFailed"));
        return nullptr;
    }

    // configure equalizer if needed
    SLEqualizerItf equalizerItf = nullptr;
    if (enableEqualizer) {
        if (!playerObject->getInterface(SL_IID_EQUALIZER, &equalizerItf)) {
            ACSDK_ERROR(LX("createFailed").d("name", name).d("reason", "equalizerInterfaceUnavailable"));
            return nullptr;
        }
    }

    auto player = std::unique_ptr<AndroidSLESMediaPlayer>(new AndroidSLESMediaPlayer(
        contentFetcherFactory,
        std::move(speaker),
        engine,
        outputMix,
        playerObject,
        equalizerItf,
        playerInterface,
        config,
        name));
    if (!player->registerPrefetchStatusCallback()) {
        ACSDK_WARN(LX(__func__)
                       .d("name", name)
                       .m("Media player will be unable to retrieve prefetch status information. This may cause choppy "
                          "playback when connection is poor."));
    }

    if (enableEqualizer) {
        if (!player->initializeEqualizer()) {
            ACSDK_ERROR(LX("createFailed")
                            .d("name", name)
                            .m("Equalizer does not seem to be supported in this environment. You should turn it "
                               "off in the configuration by setting 'equalizer.enabled' value to false."));
            return nullptr;
        }
    }

    return player;
}

AndroidSLESMediaPlayer::AndroidSLESMediaPlayer(
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory,
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> speaker,
    std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESEngine> engine,
    std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESObject> outputMixObject,
    std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESObject> playerObject,
    SLEqualizerItf equalizer,
    SLPlayItf playInterface,
    const PlaybackConfiguration& config,
    const std::string& name) :
        RequiresShutdown{name},
        m_contentFetcherFactory{contentFetcherFactory},
        m_sourceId{1},
        m_speaker{speaker},
        m_engine{engine},
        m_outputMixObject{outputMixObject},
        m_playerObject(playerObject),
        m_equalizer{equalizer},
        m_numberOfEqualizerBands{0},
        m_player{playInterface},
        m_config{config},
        m_almostDone{false},
        m_hasShutdown{false} {
}

void AndroidSLESMediaPlayer::onQueueEvent(
    AndroidSLESMediaQueue::QueueEvent status,
    const std::string& reason,
    const SourceId& eventId) {
    std::lock_guard<std::mutex> lock{m_operationMutex};
    if (m_sourceId == eventId) {
        SLint32 result;
        SLuint32 state;
        switch (status) {
            case AndroidSLESMediaQueue::QueueEvent::ERROR: {
                result = (*m_player)->GetPlayState(m_player, &state);
                if (result != SL_RESULT_SUCCESS) {
                    ACSDK_ERROR(LX("onQueueEventFailed")
                                    .d("name", RequiresShutdown::name())
                                    .d("reason", "getPlayStateFailed")
                                    .d("result", result));
                    break;
                }

                if (state != SL_PLAYSTATE_STOPPED) {
                    result = (*m_player)->SetPlayState(m_player, SL_PLAYSTATE_STOPPED);
                    if (result != SL_RESULT_SUCCESS) {
                        ACSDK_ERROR(LX("onQueueEventFailed")
                                        .d("name", RequiresShutdown::name())
                                        .d("result", result)
                                        .d("id", m_sourceId));
                        break;
                    }
                    auto observers = m_observers;
                    for (const auto& observer : observers) {
                        observer->onPlaybackError(
                            m_sourceId,
                            ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR,
                            reason,
                            createMediaPlayerState(m_sourceId));
                    }
                }
                break;
            }
            case AndroidSLESMediaQueue::QueueEvent::FINISHED_PLAYING: {
                result = (*m_player)->SetPlayState(m_player, SL_PLAYSTATE_STOPPED);
                if (result != SL_RESULT_SUCCESS) {
                    ACSDK_ERROR(LX("onQueueEventFailed")
                                    .d("name", RequiresShutdown::name())
                                    .d("result", result)
                                    .d("id", m_sourceId));
                    break;
                }
                auto observers = m_observers;
                for (const auto& observer : observers) {
                    observer->onPlaybackFinished(m_sourceId, createMediaPlayerState(m_sourceId));
                }
                break;
            }
            case AndroidSLESMediaQueue::QueueEvent::FINISHED_READING: {
                m_almostDone = true;
                auto observers = m_observers;
                for (const auto& observer : observers) {
                    observer->onBufferingComplete(m_sourceId, createMediaPlayerState(m_sourceId));
                }
                break;
            }
        }
    } else {
        ACSDK_DEBUG9(LX("eventIgnored")
                         .d("name", RequiresShutdown::name())
                         .d("status", static_cast<int>(status))
                         .d("requestId", eventId)
                         .d("currentId", m_sourceId));
    }
}

void AndroidSLESMediaPlayer::onPrefetchStatusChange(SLuint32 event) {
    std::lock_guard<std::mutex> lock{m_operationMutex};
    if (!m_observers.empty() && m_prefetchStatus && (event & SL_PREFETCHEVENT_STATUSCHANGE)) {
        uint32_t status;
        auto result = (*m_prefetchStatus)->GetPrefetchStatus(m_prefetchStatus, &status);
        if (result != SL_RESULT_SUCCESS) {
            ACSDK_WARN(LX("prefetchStatusFailed").d("name", RequiresShutdown::name()).d("result", result));
            return;
        }

        ACSDK_DEBUG9(
            LX("onPrefetchStatusChange").d("name", RequiresShutdown::name()).d("event", event).d("status", status));
        if ((SL_PREFETCHSTATUS_UNDERFLOW == status)) {
            if (!m_almostDone) {
                auto observers = m_observers;
                for (const auto& observer : observers) {
                    observer->onBufferUnderrun(m_sourceId, createMediaPlayerState(m_sourceId));
                }
            }
        } else if (SL_PREFETCHSTATUS_SUFFICIENTDATA == status) {
            auto observers = m_observers;
            for (const auto& observer : observers) {
                observer->onBufferRefilled(m_sourceId, createMediaPlayerState(m_sourceId));
            }
        }
    }
}

void prefetchStatusCallback(SLPrefetchStatusItf caller, void* pContext, SLuint32 event) {
    static_cast<AndroidSLESMediaPlayer*>(pContext)->onPrefetchStatusChange(event);
}

bool AndroidSLESMediaPlayer::registerPrefetchStatusCallback() {
    SLPrefetchStatusItf prefetchStatusInterface;
    if (!m_playerObject->getInterface(SL_IID_PREFETCHSTATUS, &prefetchStatusInterface)) {
        ACSDK_ERROR(LX("unavailablePrefetchInformation.")
                        .d("name", RequiresShutdown::name())
                        .d("reason", "interfaceUnavailable"));
        return false;
    }

    auto result =
        (*prefetchStatusInterface)->SetCallbackEventsMask(prefetchStatusInterface, SL_PREFETCHEVENT_STATUSCHANGE);
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_ERROR(LX("unavailablePrefetchInformation.")
                        .d("name", RequiresShutdown::name())
                        .d("reason", "setEventMaskFailed")
                        .d("result", result));
        return false;
    }

    result = (*prefetchStatusInterface)->RegisterCallback(prefetchStatusInterface, prefetchStatusCallback, this);
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_ERROR(LX("unavailablePrefetchInformation.")
                        .d("name", RequiresShutdown::name())
                        .d("reason", "registerCallbackFailed")
                        .d("result", result));
        return false;
    }

    m_prefetchStatus = prefetchStatusInterface;
    return true;
}

std::chrono::milliseconds AndroidSLESMediaPlayer::computeDuration(size_t sizeBytes) {
    float sampleKHz = static_cast<float>(SAMPLE_RATE_HZ) / std::kilo::num;
    size_t durationMs = sizeBytes / (sampleKHz * NUMBER_OF_CHANNELS * SAMPLE_SIZE_BYTES);
    return std::chrono::milliseconds{durationMs};
}

std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> AndroidSLESMediaPlayer::getSpeaker() {
    return m_speaker;
}

bool AndroidSLESMediaPlayer::initializeEqualizer() {
    ACSDK_DEBUG5(LX(__func__));

    auto result = (*m_equalizer)->SetEnabled(m_equalizer, SL_BOOLEAN_TRUE);
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_ERROR(LX("initializeEqualizerFailed")
                        .d("name", RequiresShutdown::name())
                        .d("reason", "SetEnabled failed")
                        .d("result", result));
        return false;
    }

    SLuint16 nbBands = 0;
    result = (*m_equalizer)->GetNumberOfBands(m_equalizer, &nbBands);
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_ERROR(LX("initializeEqualizerFailed")
                        .d("name", RequiresShutdown::name())
                        .d("reason", "GetNumberOfBands failed")
                        .d("result", result));
        return false;
    }

    ACSDK_DEBUG7(LX(__func__).d("Bands: ", nbBands));
    m_numberOfEqualizerBands = nbBands;

    if (0 == m_numberOfEqualizerBands) {
        ACSDK_ERROR(LX("initializeEqualizerFailed")
                        .d("name", RequiresShutdown::name())
                        .d("reason", "No equalizer bands supported")
                        .d("result", result));
        return false;
    }

    // Create band mapper
    m_bandMapper = EqualizerLinearBandMapper::create(m_numberOfEqualizerBands);
    if (nullptr == m_bandMapper) {
        ACSDK_ERROR(LX("initializeEqualizerFailed")
                        .d("name", RequiresShutdown::name())
                        .d("reason", "Failed to create band mapper"));
        return false;
    }

    // Sort bands by their frequencies in ascending order.
    std::map<SLmilliHertz, int> frequencyToBandMap;
    for (int bandIndex = 0; bandIndex < m_numberOfEqualizerBands; bandIndex++) {
        SLmilliHertz bandFrequency;
        result = (*m_equalizer)->GetCenterFreq(m_equalizer, bandIndex, &bandFrequency);
        if (result != SL_RESULT_SUCCESS) {
            ACSDK_ERROR(LX("initializeEqualizerFailed")
                            .d("name", RequiresShutdown::name())
                            .d("reason", "GetBandLevelRange failed")
                            .d("result", result));
            return false;
        }
        frequencyToBandMap[bandFrequency] = bandIndex;
    }

    for (auto& it : frequencyToBandMap) {
        m_growingFrequenceBandMap.push_back(it.second);
    }

    SLmillibel minLevel = 0;
    SLmillibel maxLevel = 0;

    result = (*m_equalizer)->GetBandLevelRange(m_equalizer, &minLevel, &maxLevel);
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_ERROR(LX("initializeEqualizerFailed")
                        .d("name", RequiresShutdown::name())
                        .d("reason", "GetBandLevelRange failed")
                        .d("result", result));
        return false;
    }

    m_minBandLevel = minLevel / DECIBELL_TO_MILLIBELL_MULT;
    m_maxBandLevel = maxLevel / DECIBELL_TO_MILLIBELL_MULT;
    ACSDK_DEBUG7(LX(__func__).d("Min", m_minBandLevel).d("Max", m_maxBandLevel));

    return true;
}

void AndroidSLESMediaPlayer::setEqualizerBandLevels(acsdkEqualizerInterfaces::EqualizerBandLevelMap bandLevelMap) {
    ACSDK_DEBUG5(LX(__func__));

    if (nullptr == m_bandMapper) {
        ACSDK_ERROR(LX("setEqualizerBandLevelsFailed")
                        .d("name", RequiresShutdown::name())
                        .d("reason", "Equalizer is not enabled for this instance"));
        return;
    }

    if (bandLevelMap.empty()) {
        // Nothing to do here.
        ACSDK_WARN(LX(__func__).m("Empty band level map provided, ignoring."));
        return;
    }

    m_bandMapper->mapEqualizerBands(bandLevelMap, [this](int index, int level) {
        ACSDK_DEBUG7(LX("setEqualizerBandLevels")
                         .d("name", RequiresShutdown::name())
                         .d("band index", index)
                         .d("band level", level));
        auto result =
            (*m_equalizer)
                ->SetBandLevel(m_equalizer, m_growingFrequenceBandMap[index], level * DECIBELL_TO_MILLIBELL_MULT);
        if (result != SL_RESULT_SUCCESS) {
            ACSDK_WARN(LX(__func__).m("Failed to set equalizer band").d("Band", m_growingFrequenceBandMap[index]));
        }
    });
}

int AndroidSLESMediaPlayer::getMinimumBandLevel() {
    return m_minBandLevel;
}

int AndroidSLESMediaPlayer::getMaximumBandLevel() {
    return m_maxBandLevel;
}

}  // namespace android
}  // namespace mediaPlayer
}  // namespace alexaClientSDK
