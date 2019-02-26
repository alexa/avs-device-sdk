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

#include <cmath>
#include <cstring>
#include <unordered_map>

#include <AVSCommon/AVS/Attachment/AttachmentReader.h>
#include <AVSCommon/AVS/SpeakerConstants/SpeakerConstants.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <PlaylistParser/PlaylistParser.h>
#include <PlaylistParser/UrlContentToAttachmentConverter.h>

#include "MediaPlayer/AttachmentReaderSource.h"
#include "MediaPlayer/ErrorTypeConversion.h"
#include "MediaPlayer/IStreamSource.h"
#include "MediaPlayer/Normalizer.h"

#include "MediaPlayer/MediaPlayer.h"

namespace alexaClientSDK {
namespace mediaPlayer {

using namespace avsCommon::avs::attachment;
using namespace avsCommon::avs::speakerConstants;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::mediaPlayer;
using namespace avsCommon::utils::memory;
using namespace avsCommon::utils::configuration;

/// String to identify log entries originating from this file.
static const std::string TAG("MediaPlayer");

static const std::string MEDIAPLAYER_CONFIGURATION_ROOT_KEY = "gstreamerMediaPlayer";
/// The key in our config file to set the audioSink.
static const std::string MEDIAPLAYER_AUDIO_SINK_KEY = "audioSink";
/// The key in our config file to find the output conversion type.
static const std::string MEDIAPLAYER_OUTPUT_CONVERSION_ROOT_KEY = "outputConversion";
/// The acceptable conversion keys to find in the config file
/// Key strings are mapped to gstreamer capabilities documented here:
/// https://gstreamer.freedesktop.org/documentation/design/mediatype-audio-raw.html
static const std::unordered_map<std::string, int> MEDIAPLAYER_ACCEPTED_KEYS = {{"rate", G_TYPE_INT},
                                                                               {"format", G_TYPE_STRING},
                                                                               {"channels", G_TYPE_INT}};

/// A counter used to increment the source id when a new source is set.
static MediaPlayer::SourceId g_id{0};

/// A link to @c MediaPlayerInterface::ERROR.
static const MediaPlayer::SourceId ERROR_SOURCE_ID = MediaPlayer::ERROR;

/// A value to indicate an unqueued callback. g_idle_add() only returns ids >= 0.
static const guint UNQUEUED_CALLBACK = guint(0);

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Timeout value for calls to @c gst_element_get_state() calls.
static const unsigned int TIMEOUT_ZERO_NANOSECONDS(0);

/// GStreamer Volume Element Minimum.
static const int8_t GST_SET_VOLUME_MIN = 0;

/// GStreamer Volume Element Maximum.
static const int8_t GST_SET_VOLUME_MAX = 1;

/// GStreamer Volume Adjust Minimum.
static const int8_t GST_ADJUST_VOLUME_MIN = -1;

/// GStreamer Volume Adjust Maximum.
static const int8_t GST_ADJUST_VOLUME_MAX = 1;

/// Represents the zero volume to avoid the actual 0.0 value. Used as a fix for GStreamer crashing on 0 volume for PCM.
static const gdouble VOLUME_ZERO = 0.0000001;

/// Mimimum level for equalizer bands
static const int MIN_EQUALIZER_LEVEL = -24;

/// Maximum level for equalizer bands
static const int MAX_EQUALIZER_LEVEL = 12;

/// The GStreamer property name for the frequency band 100 Hz.
static char GSTREAMER_BASS_BAND_NAME[] = "band0";

/// The GStreamer property name for the frequency band 1.1 kHz.
static char GSTREAMER_MIDRANGE_BAND_NAME[] = "band1";

/// The GStreamer property name for the frequency band 11 kHz.
static char GSTREAMER_TREBLE_BAND_NAME[] = "band2";

/**
 * Processes tags found in the tagList.
 * Called through gst_tag_list_foreach.
 *
 * @param tagList List of tags to iterate over.
 * @param tag A specific tag from the tag list.
 * @param pointerToMutableVectorOfTags Pointer to VectorOfTags. Use push_back to preserve order.
 *
 */
static void collectOneTag(const GstTagList* tagList, const gchar* tag, gpointer pointerToMutableVectorOfTags) {
    auto vectorOfTags = static_cast<VectorOfTags*>(pointerToMutableVectorOfTags);
    int num = gst_tag_list_get_tag_size(tagList, tag);
    for (int index = 0; index < num; ++index) {
        const GValue* val = gst_tag_list_get_value_index(tagList, tag, index);
        MediaPlayerObserverInterface::TagKeyValueType tagKeyValueType;
        tagKeyValueType.key = std::string(tag);
        if (G_VALUE_HOLDS_STRING(val)) {
            tagKeyValueType.value = std::string(g_value_get_string(val));
            tagKeyValueType.type = MediaPlayerObserverInterface::TagType::STRING;
        } else if (G_VALUE_HOLDS_UINT(val)) {
            tagKeyValueType.value = std::to_string(g_value_get_uint(val));
            tagKeyValueType.type = MediaPlayerObserverInterface::TagType::UINT;
        } else if (G_VALUE_HOLDS_INT(val)) {
            tagKeyValueType.value = std::to_string(g_value_get_int(val));
            tagKeyValueType.type = MediaPlayerObserverInterface::TagType::INT;
        } else if (G_VALUE_HOLDS_BOOLEAN(val)) {
            tagKeyValueType.value = std::string(g_value_get_boolean(val) ? "true" : "false");
            tagKeyValueType.type = MediaPlayerObserverInterface::TagType::BOOLEAN;
        } else if (GST_VALUE_HOLDS_DATE_TIME(val)) {
            GstDateTime* dt = static_cast<GstDateTime*>(g_value_get_boxed(val));
            gchar* dt_str = gst_date_time_to_iso8601_string(dt);
            if (!dt_str) {
                continue;
            }
            tagKeyValueType.value = std::string(dt_str);
            tagKeyValueType.type = MediaPlayerObserverInterface::TagType::STRING;
            g_free(dt_str);
        } else if (G_VALUE_HOLDS_DOUBLE(val)) {
            tagKeyValueType.value = std::to_string(g_value_get_double(val));
            tagKeyValueType.type = MediaPlayerObserverInterface::TagType::DOUBLE;
        } else {
            /*
             * Ignore GST_VALUE_HOLDS_BUFFER and other types.
             */
            continue;
        }
        vectorOfTags->push_back(tagKeyValueType);
    }
}

std::shared_ptr<MediaPlayer> MediaPlayer::create(
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory,
    bool enableEqualizer,
    SpeakerInterface::Type type,
    std::string name) {
    ACSDK_DEBUG9(LX("createCalled"));
    std::shared_ptr<MediaPlayer> mediaPlayer(new MediaPlayer(contentFetcherFactory, enableEqualizer, type, name));
    if (mediaPlayer->init()) {
        return mediaPlayer;
    } else {
        return nullptr;
    }
};

MediaPlayer::~MediaPlayer() {
    ACSDK_DEBUG9(LX("~MediaPlayerCalled"));
    cleanUpSource();
    g_main_loop_quit(m_mainLoop);
    if (m_mainLoopThread.joinable()) {
        m_mainLoopThread.join();
    }
    gst_object_unref(m_pipeline.pipeline);
    resetPipeline();

    removeSource(m_busWatchId);
    g_main_loop_unref(m_mainLoop);

    g_main_context_unref(m_workerContext);
}

MediaPlayer::SourceId MediaPlayer::setSource(
    std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> reader,
    const avsCommon::utils::AudioFormat* audioFormat) {
    ACSDK_DEBUG9(LX("setSourceCalled").d("sourceType", "AttachmentReader"));
    std::promise<MediaPlayer::SourceId> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, &reader, &promise, audioFormat]() {
        handleSetAttachmentReaderSource(std::move(reader), &promise, audioFormat);
        return false;
    };
    if (queueCallback(&callback) != UNQUEUED_CALLBACK) {
        return future.get();
    }
    return ERROR_SOURCE_ID;
}

MediaPlayer::SourceId MediaPlayer::setSource(std::shared_ptr<std::istream> stream, bool repeat) {
    ACSDK_DEBUG9(LX("setSourceCalled").d("sourceType", "istream"));
    std::promise<MediaPlayer::SourceId> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, &stream, repeat, &promise]() {
        handleSetIStreamSource(stream, repeat, &promise);
        return false;
    };
    if (queueCallback(&callback) != UNQUEUED_CALLBACK) {
        return future.get();
    }
    return ERROR_SOURCE_ID;
}

MediaPlayer::SourceId MediaPlayer::setSource(const std::string& url, std::chrono::milliseconds offset, bool repeat) {
    ACSDK_DEBUG9(LX("setSourceForUrlCalled").sensitive("url", url));
    std::promise<MediaPlayer::SourceId> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, url, offset, &promise, repeat]() {
        handleSetUrlSource(url, offset, &promise, repeat);
        return false;
    };
    if (queueCallback(&callback) != UNQUEUED_CALLBACK) {
        return future.get();
    }
    return ERROR_SOURCE_ID;
}

uint64_t MediaPlayer::getNumBytesBuffered() {
    ACSDK_DEBUG9(LX("getNumBytesBuffered"));
    guint64 bytesBuffered = 0;
    if (m_pipeline.appsrc) {
        bytesBuffered = gst_app_src_get_current_level_bytes(GST_APP_SRC(m_pipeline.appsrc));
    }
    if (m_pipeline.decodedQueue) {
        guint bytesInQueue = 0;
        g_object_get(m_pipeline.decodedQueue, "current-level-bytes", &bytesInQueue, NULL);
        bytesBuffered += bytesInQueue;
    }
    return bytesBuffered;
}

bool MediaPlayer::play(MediaPlayer::SourceId id) {
    ACSDK_DEBUG9(LX("playCalled"));
    if (!m_source) {
        ACSDK_ERROR(LX("playFailed").d("reason", "sourceNotSet"));
        return ERROR;
    }

    m_source->preprocess();

    std::promise<bool> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, id, &promise]() {
        handlePlay(id, &promise);
        return false;
    };

    if (queueCallback(&callback) != UNQUEUED_CALLBACK) {
        return future.get();
    }
    return false;
}

bool MediaPlayer::stop(MediaPlayer::SourceId id) {
    ACSDK_DEBUG9(LX("stopCalled"));
    std::promise<bool> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, id, &promise]() {
        handleStop(id, &promise);
        return false;
    };
    if (queueCallback(&callback) != UNQUEUED_CALLBACK) {
        return future.get();
    }
    return false;
}

bool MediaPlayer::pause(MediaPlayer::SourceId id) {
    ACSDK_DEBUG9(LX("pausedCalled"));
    std::promise<bool> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, id, &promise]() {
        handlePause(id, &promise);
        return false;
    };
    if (queueCallback(&callback) != UNQUEUED_CALLBACK) {
        return future.get();
    }
    return false;
}

bool MediaPlayer::resume(MediaPlayer::SourceId id) {
    ACSDK_DEBUG9(LX("resumeCalled"));
    std::promise<bool> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, id, &promise]() {
        handleResume(id, &promise);
        return false;
    };
    if (queueCallback(&callback) != UNQUEUED_CALLBACK) {
        return future.get();
    }
    return false;
}

std::chrono::milliseconds MediaPlayer::getOffset(MediaPlayer::SourceId id) {
    ACSDK_DEBUG9(LX("getOffsetCalled"));
    std::promise<std::chrono::milliseconds> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, id, &promise]() {
        handleGetOffset(id, &promise);
        return false;
    };

    if (queueCallback(&callback) != UNQUEUED_CALLBACK) {
        return future.get();
    }
    return MEDIA_PLAYER_INVALID_OFFSET;
}

void MediaPlayer::setObserver(std::shared_ptr<MediaPlayerObserverInterface> observer) {
    ACSDK_DEBUG9(LX("setObserverCalled"));
    std::promise<void> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, &promise, &observer]() {
        handleSetObserver(&promise, observer);
        return false;
    };

    if (queueCallback(&callback) != UNQUEUED_CALLBACK) {
        future.wait();
    }
}

bool MediaPlayer::setVolume(int8_t volume) {
    ACSDK_DEBUG9(LX("setVolumeCalled"));
    std::promise<bool> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, &promise, volume]() {
        handleSetVolume(&promise, volume);
        return false;
    };
    if (queueCallback(&callback) != UNQUEUED_CALLBACK) {
        return future.get();
    }
    return false;
}

void MediaPlayer::handleSetVolumeInternal(gdouble gstVolume) {
    if (gstVolume == 0) {
        g_object_set(m_pipeline.volume, "volume", VOLUME_ZERO, NULL);
    } else {
        g_object_set(m_pipeline.volume, "volume", gstVolume, NULL);
    }
    m_lastVolume = gstVolume;
}

void MediaPlayer::handleSetVolume(std::promise<bool>* promise, int8_t volume) {
    ACSDK_DEBUG9(LX("handleSetVolumeCalled"));
    auto toGstVolume =
        Normalizer::create(AVS_SET_VOLUME_MIN, AVS_SET_VOLUME_MAX, GST_SET_VOLUME_MIN, GST_SET_VOLUME_MAX);
    if (!toGstVolume) {
        ACSDK_ERROR(LX("handleSetVolumeFailed").d("reason", "createNormalizerFailed"));
        promise->set_value(false);
        return;
    }

    gdouble gstVolume;
    if (!m_pipeline.volume) {
        ACSDK_ERROR(LX("handleSetVolumeFailed").d("reason", "volumeElementNull"));
        promise->set_value(false);
        return;
    }

    if (!toGstVolume->normalize(volume, &gstVolume)) {
        ACSDK_ERROR(LX("handleSetVolumeFailed").d("reason", "normalizeVolumeFailed"));
        promise->set_value(false);
        return;
    }

    handleSetVolumeInternal(gstVolume);
    promise->set_value(true);
}

bool MediaPlayer::adjustVolume(int8_t delta) {
    ACSDK_DEBUG9(LX("adjustVolumeCalled"));
    std::promise<bool> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, &promise, delta]() {
        handleAdjustVolume(&promise, delta);
        return false;
    };
    if (queueCallback(&callback) != UNQUEUED_CALLBACK) {
        return future.get();
    }
    return false;
}

void MediaPlayer::handleAdjustVolume(std::promise<bool>* promise, int8_t delta) {
    ACSDK_DEBUG9(LX("handleAdjustVolumeCalled"));
    auto toGstDeltaVolume =
        Normalizer::create(AVS_ADJUST_VOLUME_MIN, AVS_ADJUST_VOLUME_MAX, GST_ADJUST_VOLUME_MIN, GST_ADJUST_VOLUME_MAX);

    if (!toGstDeltaVolume) {
        ACSDK_ERROR(LX("handleAdjustVolumeFailed").d("reason", "createNormalizerFailed"));
        promise->set_value(false);
        return;
    }

    if (!m_pipeline.volume) {
        ACSDK_ERROR(LX("adjustVolumeFailed").d("reason", "volumeElementNull"));
        promise->set_value(false);
        return;
    }

    gdouble gstVolume;
    g_object_get(m_pipeline.volume, "volume", &gstVolume, NULL);

    gdouble gstDelta;
    if (!toGstDeltaVolume->normalize(delta, &gstDelta)) {
        ACSDK_ERROR(LX("adjustVolumeFailed").d("reason", "normalizeVolumeFailed"));
        promise->set_value(false);
        return;
    }

    gstVolume += gstDelta;

    // If adjustment exceeds bounds, cap at max/min.
    gstVolume = std::min(gstVolume, static_cast<gdouble>(GST_SET_VOLUME_MAX));
    gstVolume = std::max(gstVolume, static_cast<gdouble>(GST_SET_VOLUME_MIN));

    handleSetVolumeInternal(gstVolume);
    promise->set_value(true);
}

bool MediaPlayer::setMute(bool mute) {
    ACSDK_DEBUG9(LX("setMuteCalled"));
    std::promise<bool> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, &promise, mute]() {
        handleSetMute(&promise, mute);
        return false;
    };
    if (queueCallback(&callback) != UNQUEUED_CALLBACK) {
        return future.get();
    }
    return false;
}

void MediaPlayer::handleSetMute(std::promise<bool>* promise, bool mute) {
    ACSDK_DEBUG9(LX("handleSetMuteCalled"));
    if (!m_pipeline.volume) {
        ACSDK_ERROR(LX("setMuteFailed").d("reason", "volumeElementNull"));
        promise->set_value(false);
        return;
    }

    // A fix for GStreamer crashing for zero volume on PCM data
    g_object_set(m_pipeline.volume, "volume", mute || m_lastVolume == 0 ? VOLUME_ZERO : m_lastVolume, NULL);
    m_isMuted = mute;
    promise->set_value(true);
}

bool MediaPlayer::getSpeakerSettings(SpeakerInterface::SpeakerSettings* settings) {
    ACSDK_DEBUG9(LX("getSpeakerSettingsCalled"));
    std::promise<bool> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, &promise, settings]() {
        handleGetSpeakerSettings(&promise, settings);
        return false;
    };
    if (queueCallback(&callback) != UNQUEUED_CALLBACK) {
        return future.get();
    }
    return false;
}

void MediaPlayer::handleGetSpeakerSettings(
    std::promise<bool>* promise,
    avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings* settings) {
    ACSDK_DEBUG9(LX("handleGetSpeakerSettingsCalled"));
    if (!settings) {
        ACSDK_ERROR(LX("getSpeakerSettingsFailed").d("reason", "nullSettings"));
        promise->set_value(false);
        return;
    } else if (!m_pipeline.volume) {
        ACSDK_ERROR(LX("getSpeakerSettingsFailed").d("reason", "volumeElementNull"));
        promise->set_value(false);
        return;
    }

    auto toAVSVolume =
        Normalizer::create(GST_SET_VOLUME_MIN, GST_SET_VOLUME_MAX, AVS_SET_VOLUME_MIN, AVS_SET_VOLUME_MAX);
    if (!toAVSVolume) {
        ACSDK_ERROR(LX("handleGetSpeakerSettingsFailed").d("reason", "createNormalizerFailed"));
        promise->set_value(false);
        return;
    }

    gdouble avsVolume;
    gdouble gstVolume;
    gboolean mute;
    g_object_get(m_pipeline.volume, "volume", &gstVolume, "mute", &mute, NULL);

    /// A part of GStreamer crash fix for zero volume on PCM data
    mute = m_isMuted;
    if (mute) {
        gstVolume = m_lastVolume;
    }

    if (!toAVSVolume->normalize(gstVolume, &avsVolume)) {
        ACSDK_ERROR(LX("handleGetSpeakerSettingsFailed").d("reason", "normalizeVolumeFailed"));
        promise->set_value(false);
        return;
    }

    // AVS Volume will be between 0 and 100.
    settings->volume = static_cast<int8_t>(std::round(avsVolume));
    settings->mute = mute;

    promise->set_value(true);
}

SpeakerInterface::Type MediaPlayer::getSpeakerType() {
    ACSDK_DEBUG9(LX("getSpeakerTypeCalled"));
    return m_speakerType;
}

void MediaPlayer::setAppSrc(GstAppSrc* appSrc) {
    m_pipeline.appsrc = appSrc;
}

GstAppSrc* MediaPlayer::getAppSrc() const {
    return m_pipeline.appsrc;
}

void MediaPlayer::setDecoder(GstElement* decoder) {
    m_pipeline.decoder = decoder;
}

GstElement* MediaPlayer::getDecoder() const {
    return m_pipeline.decoder;
}

GstElement* MediaPlayer::getPipeline() const {
    return m_pipeline.pipeline;
}

MediaPlayer::MediaPlayer(
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory,
    bool enableEqualizer,
    SpeakerInterface::Type type,
    std::string name) :
        RequiresShutdown{name},
        m_lastVolume{GST_SET_VOLUME_MAX},
        m_isMuted{false},
        m_contentFetcherFactory{contentFetcherFactory},
        m_equalizerEnabled{enableEqualizer},
        m_speakerType{type},
        m_playbackStartedSent{false},
        m_playbackFinishedSent{false},
        m_isPaused{false},
        m_isBufferUnderrun{false},
        m_playerObserver{nullptr},
        m_currentId{ERROR},
        m_playPending{false},
        m_pausePending{false},
        m_resumePending{false},
        m_pauseImmediately{false} {
}

void MediaPlayer::workerLoop() {
    g_main_context_push_thread_default(m_workerContext);

    // Add bus watch only after calling g_main_context_push_thread_default.
    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline.pipeline));
    m_busWatchId = gst_bus_add_watch(bus, &MediaPlayer::onBusMessage, this);
    gst_object_unref(bus);

    g_main_loop_run(m_mainLoop);

    g_main_context_pop_thread_default(m_workerContext);
}

bool MediaPlayer::init() {
    m_workerContext = g_main_context_new();
    if (!m_workerContext) {
        ACSDK_ERROR(LX("initPlayerFailed").d("reason", "nullWorkerContext"));
        return false;
    }

    if (!(m_mainLoop = g_main_loop_new(m_workerContext, false))) {
        ACSDK_ERROR(LX("initPlayerFailed").d("reason", "gstMainLoopNewFailed"));
        return false;
    };

    if (false == gst_init_check(NULL, NULL, NULL)) {
        ACSDK_ERROR(LX("initPlayerFailed").d("reason", "gstInitCheckFailed"));
        return false;
    }

    if (!setupPipeline()) {
        ACSDK_ERROR(LX("initPlayerFailed").d("reason", "setupPipelineFailed"));
        return false;
    }

    m_mainLoopThread = std::thread(&MediaPlayer::workerLoop, this);

    return true;
}

bool MediaPlayer::setupPipeline() {
    m_pipeline.decodedQueue = gst_element_factory_make("queue2", "decodedQueue");
    if (!m_pipeline.decodedQueue) {
        ACSDK_ERROR(LX("setupPipelineFailed").d("reason", "createQueueElementFailed"));
        return false;
    }
    g_object_set(m_pipeline.decodedQueue, "use-buffering", TRUE, NULL);
    m_pipeline.converter = gst_element_factory_make("audioconvert", "converter");
    if (!m_pipeline.converter) {
        ACSDK_ERROR(LX("setupPipelineFailed").d("reason", "createConverterElementFailed"));
        return false;
    }

    m_pipeline.volume = gst_element_factory_make("volume", "volume");
    if (!m_pipeline.volume) {
        ACSDK_ERROR(LX("setupPipelineFailed").d("reason", "createVolumeElementFailed"));
        return false;
    }

    if (m_equalizerEnabled) {
        m_pipeline.equalizer = gst_element_factory_make("equalizer-3bands", "equalizer");
        if (!m_pipeline.equalizer) {
            ACSDK_ERROR(LX("setupPipelineFailed").d("reason", "createEqualizerElementFailed"));
            return false;
        }
    }

    std::string audioSinkElement;
    ConfigurationNode::getRoot()[MEDIAPLAYER_CONFIGURATION_ROOT_KEY].getString(
        MEDIAPLAYER_AUDIO_SINK_KEY, &audioSinkElement, "autoaudiosink");
    m_pipeline.audioSink = gst_element_factory_make(audioSinkElement.c_str(), "audio_sink");
    if (!m_pipeline.audioSink) {
        ACSDK_ERROR(LX("setupPipelineFailed")
                        .d("reason", "createAudioSinkElementFailed")
                        .d("audioSinkElement", audioSinkElement));
        return false;
    }

    GstCaps* caps = gst_caps_new_empty_simple("audio/x-raw");
    if (!caps) {
        ACSDK_ERROR(LX("setupPipelineFailed").d("reason", "createCapabilityStructFailed"));
        return false;
    }

    m_pipeline.resample = nullptr;
    m_pipeline.caps = nullptr;

    // Check to see if user has specified an output configuration
    auto configurationRoot =
        ConfigurationNode::getRoot()[MEDIAPLAYER_CONFIGURATION_ROOT_KEY][MEDIAPLAYER_OUTPUT_CONVERSION_ROOT_KEY];
    if (configurationRoot) {
        std::string value;

        // Search for output configuration keys
        for (auto& it : MEDIAPLAYER_ACCEPTED_KEYS) {
            if (!configurationRoot.getString(it.first, &value) || value.empty()) {
                continue;
            }

            // Found key, add it to capability struct
            switch (it.second) {
                case G_TYPE_INT:
                    gst_caps_set_simple(caps, it.first.c_str(), it.second, std::stoi(value), NULL);
                    break;
                case G_TYPE_STRING:
                    gst_caps_set_simple(caps, it.first.c_str(), it.second, value.c_str(), NULL);
                    break;
            }
        }

        // Add resample logic if configuration found
        if (!gst_caps_is_empty(caps)) {
            ACSDK_INFO(LX("outputConversion").d("string", gst_caps_to_string(caps)));

            m_pipeline.resample = gst_element_factory_make("audioresample", "resample");
            if (!m_pipeline.resample) {
                ACSDK_ERROR(LX("setupPipelineFailed").d("reason", "createResampleElementFailed"));
                return false;
            }

            m_pipeline.caps = gst_element_factory_make("capsfilter", "caps");
            if (!m_pipeline.caps) {
                ACSDK_ERROR(LX("setupPipelineFailed").d("reason", "createCapabilityElementFailed"));
                return false;
            }

            g_object_set(G_OBJECT(m_pipeline.caps), "caps", caps, NULL);
        } else {
            ACSDK_INFO(LX("invalidOutputConversion").d("string", gst_caps_to_string(caps)));
        }
    } else {
        ACSDK_DEBUG9(LX("noOutputConversion"));
    }

    // clean up caps object
    gst_caps_unref(caps);

    m_pipeline.pipeline = gst_pipeline_new("audio-pipeline");
    if (!m_pipeline.pipeline) {
        ACSDK_ERROR(LX("setupPipelineFailed").d("reason", "createPipelineElementFailed"));
        return false;
    }

    // Link only the queue, converter, volume, and sink here. Src will be linked in respective source files.
    gst_bin_add_many(
        GST_BIN(m_pipeline.pipeline),
        m_pipeline.decodedQueue,
        m_pipeline.converter,
        m_pipeline.volume,
        m_pipeline.audioSink,
        nullptr);

    GstElement* pipelineTailElement = m_pipeline.audioSink;

    if (m_equalizerEnabled) {
        // Add equalizer to a pipeline tail
        gst_bin_add(GST_BIN(m_pipeline.pipeline), m_pipeline.equalizer);
        pipelineTailElement = m_pipeline.equalizer;
        if (!gst_element_link(m_pipeline.equalizer, m_pipeline.audioSink)) {
            ACSDK_ERROR(LX("setupPipelineFailed").d("reason", "failed to linke equalizer to audiosink."));
            return false;
        }
    }

    if (m_pipeline.resample != nullptr && m_pipeline.caps != nullptr) {
        // Add resampler to the pipeline tail
        gst_bin_add_many(GST_BIN(m_pipeline.pipeline), m_pipeline.resample, m_pipeline.caps, nullptr);

        if (!gst_element_link_many(m_pipeline.resample, m_pipeline.caps, pipelineTailElement, nullptr)) {
            ACSDK_ERROR(LX("setupPipelineFailed").d("reason", "Failed to link converter."));
            return false;
        }
        pipelineTailElement = m_pipeline.resample;
    }

    // Complete the pipeline linking
    if (!gst_element_link_many(
            m_pipeline.decodedQueue, m_pipeline.converter, m_pipeline.volume, pipelineTailElement, nullptr)) {
        ACSDK_ERROR(LX("setupPipelineFailed").d("reason", "Failed to link pipeline."));
        return false;
    }

    return true;
}

void MediaPlayer::tearDownTransientPipelineElements(bool notifyStop) {
    ACSDK_DEBUG9(LX("tearDownTransientPipelineElements"));
    m_offsetBeforeTeardown = getCurrentStreamOffset();
    if (notifyStop) {
        sendPlaybackStopped();
    }
    m_currentId = ERROR_SOURCE_ID;
    cleanUpSource();
    m_offsetManager.clear();
    m_playPending = false;
    m_pausePending = false;
    m_resumePending = false;
    m_pauseImmediately = false;
    m_playbackStartedSent = false;
    m_playbackFinishedSent = false;
    m_isPaused = false;
    m_isBufferUnderrun = false;
    if (m_pipeline.audioSink) {
        // Set audioSink's sink option back to TRUE
        g_object_set(m_pipeline.audioSink, "sync", TRUE, NULL);
    }
}

void MediaPlayer::resetPipeline() {
    ACSDK_DEBUG9(LX("resetPipeline"));
    m_pipeline.pipeline = nullptr;
    m_pipeline.appsrc = nullptr;
    m_pipeline.decoder = nullptr;
    m_pipeline.decodedQueue = nullptr;
    m_pipeline.converter = nullptr;
    m_pipeline.volume = nullptr;
    m_pipeline.resample = nullptr;
    m_pipeline.caps = nullptr;
    m_pipeline.equalizer = nullptr;
    m_pipeline.audioSink = nullptr;
}

bool MediaPlayer::queryBufferPercent(gint* percent) {
    ACSDK_DEBUG5(LX("queryBufferPercent"));
    if (!percent) {
        ACSDK_ERROR(LX("queryBufferPercentFailed").d("reason", "nullPercent"));
        return false;
    }
    GstQuery* query;

    query = gst_query_new_buffering(GST_FORMAT_TIME);
    if (gst_element_query(m_pipeline.pipeline, query)) {
        gst_query_parse_buffering_percent(query, NULL, percent);
        gst_query_unref(query);
        return true;
    } else {
        ACSDK_ERROR(LX("queryBufferPercentFailed").d("reason", "bufferingQueryFailed"));
        gst_query_unref(query);
        return true;
    }
}

bool MediaPlayer::queryIsSeekable(bool* isSeekable) {
    GstState curState;
    auto stateChange = gst_element_get_state(m_pipeline.pipeline, &curState, NULL, TIMEOUT_ZERO_NANOSECONDS);
    if (stateChange == GST_STATE_CHANGE_FAILURE) {
        ACSDK_ERROR(LX("queryIsSeekableFailed").d("reason", "gstElementGetStateFailed"));
        return false;
    }
    if (stateChange == GST_STATE_CHANGE_ASYNC) {
        ACSDK_DEBUG(LX("pipelineNotSeekable").d("reason", "stateChangeAsync"));
        return false;
    }
    if ((GST_STATE_PLAYING != curState) && (GST_STATE_PAUSED != curState)) {
        ACSDK_DEBUG(LX("pipelineNotSeekable").d("reason", "notPlayingOrPaused"));
        *isSeekable = false;
        return true;
    }
    // TODO: ACSDK-1778: Investigate why gst_query_parse_seeking() is not working.
    // If it's usable again, use gst_query_parse_seeking() instead of gst_app_src_get_stream_type()

    *isSeekable = (GST_APP_STREAM_TYPE_SEEKABLE == gst_app_src_get_stream_type(m_pipeline.appsrc));
    return true;
}

bool MediaPlayer::seek() {
    bool seekSuccessful = true;
    ACSDK_DEBUG9(LX("seekCalled"));
    if (!m_offsetManager.isSeekable() || !m_offsetManager.isSeekPointSet()) {
        ACSDK_ERROR(LX("seekFailed")
                        .d("reason", "invalidState")
                        .d("isSeekable", m_offsetManager.isSeekable())
                        .d("seekPointSet", m_offsetManager.isSeekPointSet()));
        seekSuccessful = false;
    } else if (!gst_element_seek_simple(
                   m_pipeline.pipeline,
                   GST_FORMAT_TIME,  // ns
                   static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
                   std::chrono::duration_cast<std::chrono::nanoseconds>(m_offsetManager.getSeekPoint()).count())) {
        ACSDK_ERROR(LX("seekFailed").d("reason", "gstElementSeekSimpleFailed"));
        seekSuccessful = false;
    } else {
        ACSDK_DEBUG(LX("seekSuccessful").d("offsetInMs", m_offsetManager.getSeekPoint().count()));
    }

    m_offsetManager.clear();
    return seekSuccessful;
}

guint MediaPlayer::queueCallback(const std::function<gboolean()>* callback) {
    if (isShutdown()) {
        return UNQUEUED_CALLBACK;
    }
    auto source = g_idle_source_new();
    g_source_set_callback(
        source, reinterpret_cast<GSourceFunc>(&onCallback), const_cast<std::function<gboolean()>*>(callback), nullptr);
    auto sourceId = g_source_attach(source, m_workerContext);
    g_source_unref(source);
    return sourceId;
}

guint MediaPlayer::attachSource(GSource* source) {
    if (source) {
        return g_source_attach(source, m_workerContext);
    }
    return UNQUEUED_CALLBACK;
}

gboolean MediaPlayer::removeSource(guint tag) {
    auto source = g_main_context_find_source_by_id(m_workerContext, tag);
    if (source) {
        g_source_destroy(source);
    }
    return true;
}

void MediaPlayer::onError() {
    ACSDK_DEBUG9(LX("onError"));
    /*
     * Instead of calling the queueCallback, we are calling g_idle_add here directly here because we want this callback
     * to be non-blocking.  To do this, we are creating a static callback function with the this pointer passed in as
     * a parameter.
     */
    auto source = g_idle_source_new();
    g_source_set_callback(source, reinterpret_cast<GSourceFunc>(&onErrorCallback), this, nullptr);
    g_source_attach(source, m_workerContext);
    g_source_unref(source);
}

void MediaPlayer::doShutdown() {
    if (m_urlConverter) {
        m_urlConverter->shutdown();
    }
    m_urlConverter.reset();
    m_playerObserver.reset();
}

gboolean MediaPlayer::onCallback(const std::function<gboolean()>* callback) {
    return (*callback)();
}

void MediaPlayer::onPadAdded(GstElement* decoder, GstPad* pad, gpointer pointer) {
    ACSDK_DEBUG9(LX("onPadAddedCalled"));
    auto mediaPlayer = static_cast<MediaPlayer*>(pointer);
    std::promise<void> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [mediaPlayer, &promise, decoder, pad]() {
        mediaPlayer->handlePadAdded(&promise, decoder, pad);
        return false;
    };
    if (mediaPlayer->queueCallback(&callback) != UNQUEUED_CALLBACK) {
        future.wait();
    }
}

void MediaPlayer::handlePadAdded(std::promise<void>* promise, GstElement* decoder, GstPad* pad) {
    ACSDK_DEBUG9(LX("handlePadAddedSignalCalled"));
    gst_element_link(decoder, m_pipeline.decodedQueue);
    promise->set_value();
}

gboolean MediaPlayer::onBusMessage(GstBus* bus, GstMessage* message, gpointer mediaPlayer) {
    return static_cast<MediaPlayer*>(mediaPlayer)->handleBusMessage(message);
}

gboolean MediaPlayer::handleBusMessage(GstMessage* message) {
    ACSDK_DEBUG9(
        LX("messageReceived").d("type", GST_MESSAGE_TYPE_NAME(message)).d("source", GST_MESSAGE_SRC_NAME(message)));
    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_EOS:
            if (GST_MESSAGE_SRC(message) == GST_OBJECT_CAST(m_pipeline.pipeline)) {
                if (!m_source->handleEndOfStream()) {
                    const std::string errorMessage{"reason=sourceHandleEndOfStreamFailed"};
                    ACSDK_ERROR(LX("handleBusMessageFailed").m(errorMessage));
                    sendPlaybackError(ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, errorMessage);
                    break;
                }

                // Continue playback if there is additional data.
                if (m_source->hasAdditionalData()) {
                    if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(m_pipeline.pipeline, GST_STATE_NULL)) {
                        const std::string errorMessage{"reason=setPipelineToNullFailed"};
                        ACSDK_ERROR(LX("continuingPlaybackFailed").m(errorMessage));
                        sendPlaybackError(ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, errorMessage);
                        break;
                    }

                    if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(m_pipeline.pipeline, GST_STATE_PLAYING)) {
                        const std::string errorMessage{"reason=setPipelineToPlayingFailed"};
                        ACSDK_ERROR(LX("continuingPlaybackFailed").m(errorMessage));
                        sendPlaybackError(ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, errorMessage);
                        break;
                    }
                } else {
                    sendPlaybackFinished();
                }
            }
            break;

        case GST_MESSAGE_ERROR: {
            GError* error;
            gchar* debug;
            gst_message_parse_error(message, &error, &debug);

            std::string messageSrcName = GST_MESSAGE_SRC_NAME(message);
            ACSDK_ERROR(LX("handleBusMessageError")
                            .d("source", messageSrcName)
                            .d("error", error->message)
                            .d("debug", debug ? debug : "noInfo"));
            bool isPlaybackRemote = m_source ? m_source->isPlaybackRemote() : false;
            sendPlaybackError(gerrorToErrorType(error, isPlaybackRemote), error->message);
            g_error_free(error);
            g_free(debug);
            break;
        }
        case GST_MESSAGE_STATE_CHANGED: {
            // Check that the state change is for the pipeline.
            if (GST_MESSAGE_SRC(message) == GST_OBJECT_CAST(m_pipeline.pipeline)) {
                GstState oldState;
                GstState newState;
                GstState pendingState;
                gst_message_parse_state_changed(message, &oldState, &newState, &pendingState);
                ACSDK_DEBUG9(LX("State Change")
                                 .d("oldState", gst_element_state_get_name(oldState))
                                 .d("newState", gst_element_state_get_name(newState))
                                 .d("pendingState", gst_element_state_get_name(pendingState)));
                if (GST_STATE_PAUSED == newState) {
                    /*
                     * Pause occurred immediately after a play/resume, so it's possible that the play/resume
                     * was never enacted by MediaPlayer. If there are pending play/resume at the time of the pause,
                     * notify the observers that the calls were still received.
                     */
                    if (m_pauseImmediately) {
                        if (m_playPending) {
                            sendPlaybackStarted();
                        } else if (m_resumePending) {
                            sendPlaybackResumed();
                        }
                        sendPlaybackPaused();
                    } else if (GST_STATE_PLAYING == pendingState) {
                        // GStreamer seeks should be performed when the pipeline is in the PAUSED or PLAYING state only,
                        // so just before we make our first upwards state change from PAUSED to PLAYING, we perform the
                        // seek.
                        if (m_offsetManager.isSeekable() && m_offsetManager.isSeekPointSet()) {
                            if (!seek()) {
                                std::string error = "seekFailed";
                                ACSDK_ERROR(LX(error));
                                sendPlaybackError(ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, error);
                            };
                        }
                    } else if (GST_STATE_PLAYING == oldState && GST_STATE_VOID_PENDING == pendingState) {
                        // State change from PLAYING -> PAUSED.
                        if (m_isBufferUnderrun) {
                            sendBufferUnderrun();
                        } else if (!m_isPaused) {
                            sendPlaybackPaused();
                        }
                    }
                } else if (newState == GST_STATE_PLAYING) {
                    if (!m_playbackStartedSent) {
                        sendPlaybackStarted();
                    } else {
                        if (m_isBufferUnderrun) {
                            sendBufferRefilled();
                            m_isBufferUnderrun = false;
                        }
                        if (m_isPaused) {
                            sendPlaybackResumed();
                        }
                    }
                } else if (newState == GST_STATE_NULL && oldState == GST_STATE_READY) {
                    sendPlaybackStopped();
                }
            } else if (g_str_has_prefix(GST_MESSAGE_SRC_NAME(message), "tsdemux")) {
                /*
                 * tsdemux element can be used to determine if the music sources are MPEG-TS.
                 */
                GstState oldState;
                GstState newState;
                GstState pendingState;
                gst_message_parse_state_changed(message, &oldState, &newState, &pendingState);
                ACSDK_DEBUG5(LX("tsdemux State Change")
                                 .d("oldState", gst_element_state_get_name(oldState))
                                 .d("newState", gst_element_state_get_name(newState))
                                 .d("pendingState", gst_element_state_get_name(pendingState)));

                if (GST_STATE_READY == newState) {
                    /*
                     * Certain music sources, specifically Audible, were unable to play properly. With Audible, frames
                     * were getting dropped and the audio would play very choppily. For example, in a 10 second chunk,
                     * seconds 1-5 would play followed immediately by seconds 6.5-7.5, followed by 8.5-10. Setting this
                     * property to false prevents the sink from dropping frames because they arrive too late.
                     * TODO: (ACSDK-828) Investigate why frames are arriving late to the sink causing MPEG-TS files to
                     * play choppily
                     */
                    ACSDK_DEBUG5(LX("audioSink").m("Sync option set to false."));
                    g_object_set(m_pipeline.audioSink, "sync", FALSE, NULL);
                } else if (GST_STATE_NULL == newState) {
                    // Reset sync state back to true if tsdemux changes to NULL state
                    ACSDK_DEBUG5(LX("audioSink").m("Sync option set to true."));
                    g_object_set(m_pipeline.audioSink, "sync", TRUE, NULL);
                }
            }
            break;
        }

        case GST_MESSAGE_BUFFERING: {
            gint bufferPercent = 0;
            gst_message_parse_buffering(message, &bufferPercent);
            ACSDK_DEBUG9(LX("handleBusMessage").d("message", "GST_MESSAGE_BUFFERING").d("percent", bufferPercent));

            if (bufferPercent < 100) {
                if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(m_pipeline.pipeline, GST_STATE_PAUSED)) {
                    std::string error = "pausingOnBufferUnderrunFailed";
                    ACSDK_ERROR(LX(error));
                    sendPlaybackError(ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, error);
                    break;
                }
                // Only enter bufferUnderrun after playback has started.
                if (m_playbackStartedSent) {
                    m_isBufferUnderrun = true;
                }
            } else {
                if (m_pauseImmediately) {
                    // To avoid starting to play if a pause() was called immediately after calling a play()
                    break;
                }
                bool isSeekable = false;
                if (queryIsSeekable(&isSeekable)) {
                    m_offsetManager.setIsSeekable(isSeekable);
                }

                ACSDK_DEBUG9(LX("offsetState")
                                 .d("isSeekable", m_offsetManager.isSeekable())
                                 .d("isSeekPointSet", m_offsetManager.isSeekPointSet()));

                if (m_offsetManager.isSeekable() && m_offsetManager.isSeekPointSet()) {
                    seek();
                } else if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(m_pipeline.pipeline, GST_STATE_PLAYING)) {
                    std::string error = "resumingOnBufferRefilledFailed";
                    ACSDK_ERROR(LX(error));
                    sendPlaybackError(ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, error);
                }
            }
            break;
        }
        case GST_MESSAGE_TAG: {
            auto vectorOfTags = collectTags(message);
            sendStreamTagsToObserver(std::move(vectorOfTags));
            break;
        }
        default:
            break;
    }
    return true;
}

std::unique_ptr<const VectorOfTags> MediaPlayer::collectTags(GstMessage* message) {
    VectorOfTags vectorOfTags;
    GstTagList* tags = NULL;
    gst_message_parse_tag(message, &tags);
    int num_of_tags = gst_tag_list_n_tags(tags);
    if (!num_of_tags) {
        gst_tag_list_unref(tags);
        return nullptr;
    }
    gst_tag_list_foreach(tags, &collectOneTag, &vectorOfTags);
    gst_tag_list_unref(tags);
    return make_unique<const VectorOfTags>(vectorOfTags);
}

void MediaPlayer::sendStreamTagsToObserver(std::unique_ptr<const VectorOfTags> vectorOfTags) {
    ACSDK_DEBUG(LX("callingOnTags"));
    if (m_playerObserver) {
        m_playerObserver->onTags(m_currentId, std::move(vectorOfTags));
    }
}

void MediaPlayer::handleSetAttachmentReaderSource(
    std::shared_ptr<AttachmentReader> reader,
    std::promise<MediaPlayer::SourceId>* promise,
    const avsCommon::utils::AudioFormat* audioFormat,
    bool repeat) {
    ACSDK_DEBUG(LX("handleSetAttachmentReaderSourceCalled"));

    tearDownTransientPipelineElements(true);

    std::shared_ptr<SourceInterface> source = AttachmentReaderSource::create(this, reader, audioFormat, repeat);

    if (!source) {
        ACSDK_ERROR(LX("handleSetAttachmentReaderSourceFailed").d("reason", "sourceIsNullptr"));
        promise->set_value(ERROR_SOURCE_ID);
        return;
    }

    /*
     * Once the source pad for the decoder has been added, the decoder emits the pad-added signal. Connect the signal
     * to the callback which performs the linking of the decoder source pad to decodedQueue sink pad.
     */
    if (!g_signal_connect(m_pipeline.decoder, "pad-added", G_CALLBACK(onPadAdded), this)) {
        ACSDK_ERROR(LX("handleSetAttachmentReaderSourceFailed").d("reason", "connectPadAddedSignalFailed"));
        promise->set_value(ERROR_SOURCE_ID);
        return;
    }

    m_source = source;
    m_currentId = ++g_id;
    m_offsetManager.setIsSeekable(true);
    promise->set_value(m_currentId);
}

void MediaPlayer::handleSetIStreamSource(
    std::shared_ptr<std::istream> stream,
    bool repeat,
    std::promise<MediaPlayer::SourceId>* promise) {
    ACSDK_DEBUG(LX("handleSetSourceCalled"));

    tearDownTransientPipelineElements(true);

    std::shared_ptr<SourceInterface> source = IStreamSource::create(this, stream, repeat);

    if (!source) {
        ACSDK_ERROR(LX("handleSetIStreamSourceFailed").d("reason", "sourceIsNullptr"));
        promise->set_value(ERROR_SOURCE_ID);
        return;
    }

    /*
     * Once the source pad for the decoder has been added, the decoder emits the pad-added signal. Connect the signal
     * to the callback which performs the linking of the decoder source pad to the decodedQueue sink pad.
     */
    if (!g_signal_connect(m_pipeline.decoder, "pad-added", G_CALLBACK(onPadAdded), this)) {
        ACSDK_ERROR(LX("handleSetIStreamSourceFailed").d("reason", "connectPadAddedSignalFailed"));
        promise->set_value(ERROR_SOURCE_ID);
        return;
    }

    m_source = source;
    m_currentId = ++g_id;
    promise->set_value(m_currentId);
}

void MediaPlayer::handleSetUrlSource(
    const std::string& url,
    std::chrono::milliseconds offset,
    std::promise<SourceId>* promise,
    bool repeat) {
    ACSDK_DEBUG(LX("handleSetSourceForUrlCalled"));

    tearDownTransientPipelineElements(true);

    m_urlConverter = alexaClientSDK::playlistParser::UrlContentToAttachmentConverter::create(
        m_contentFetcherFactory, url, shared_from_this(), offset);
    if (!m_urlConverter) {
        ACSDK_ERROR(LX("setSourceUrlFailed").d("reason", "badUrlConverter"));
        promise->set_value(ERROR_SOURCE_ID);
        return;
    }
    auto attachment = m_urlConverter->getAttachment();
    if (!attachment) {
        ACSDK_ERROR(LX("setSourceUrlFailed").d("reason", "badAttachmentReceived"));
        promise->set_value(ERROR_SOURCE_ID);
        return;
    }
    std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> reader =
        attachment->createReader(sds::ReaderPolicy::NONBLOCKING);
    if (!reader) {
        ACSDK_ERROR(LX("setSourceUrlFailed").d("reason", "failedToCreateAttachmentReader"));
        promise->set_value(ERROR_SOURCE_ID);
        return;
    }
    handleSetAttachmentReaderSource(reader, promise, nullptr, repeat);
}

void MediaPlayer::handlePlay(SourceId id, std::promise<bool>* promise) {
    ACSDK_DEBUG(LX("handlePlayCalled").d("idPassed", id).d("currentId", (m_currentId)));
    if (!validateSourceAndId(id)) {
        ACSDK_ERROR(LX("handlePlayFailed"));
        promise->set_value(false);
        return;
    }

    GstState curState;
    auto stateChange = gst_element_get_state(m_pipeline.pipeline, &curState, NULL, TIMEOUT_ZERO_NANOSECONDS);
    if (stateChange == GST_STATE_CHANGE_FAILURE) {
        ACSDK_ERROR(LX("handlePlayFailed").d("reason", "gstElementGetStateFailed"));
        promise->set_value(false);
        return;
    }
    if (curState == GST_STATE_PLAYING) {
        ACSDK_DEBUG(LX("handlePlayFailed").d("reason", "alreadyPlaying"));
        promise->set_value(false);
        return;
    }
    if (m_playPending) {
        ACSDK_DEBUG(LX("handlePlayFailed").d("reason", "playCurrentlyPending"));
        promise->set_value(false);
        return;
    }

    m_playbackFinishedSent = false;
    m_playbackStartedSent = false;
    m_playPending = true;
    m_pauseImmediately = false;
    promise->set_value(true);

    /*
     * If the pipeline is completely buffered, then go straight to PLAY otherwise,
     * set pipeline to PAUSED state to attempt buffering.  The pipeline will be set to PLAY upon receiving buffer
     * percent = 100.
     */
    GstState startingState = GST_STATE_PAUSED;
    gint percent = 0;
    if ((GST_STATE_PAUSED == curState) && (queryBufferPercent(&percent))) {
        if (100 == percent) {
            startingState = GST_STATE_PLAYING;
        }
    }

    stateChange = gst_element_set_state(m_pipeline.pipeline, startingState);
    ACSDK_DEBUG(LX("handlePlay")
                    .d("startingState", gst_element_state_get_name(startingState))
                    .d("stateReturn", gst_element_state_change_return_get_name(stateChange)));

    switch (stateChange) {
        case GST_STATE_CHANGE_FAILURE: {
            const std::string errorMessage{"reason=gstElementSetStateFailure"};
            ACSDK_ERROR(LX("handlePlayFailed").m(errorMessage));
            sendPlaybackError(ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, errorMessage);
        }
            return;
        default:
            if (m_urlConverter) {
                if (m_urlConverter->getDesiredStreamingPoint() == std::chrono::milliseconds::zero()) {
                    return;
                }
                m_offsetManager.setSeekPoint(
                    m_urlConverter->getDesiredStreamingPoint() - m_urlConverter->getStartStreamingPoint());
            }
            // Allow sending callbacks to be handled on the bus message
            return;
    }
}

void MediaPlayer::handleStop(MediaPlayer::SourceId id, std::promise<bool>* promise) {
    ACSDK_DEBUG(LX("handleStopCalled").d("idPassed", id).d("currentId", (m_currentId)));
    if (!validateSourceAndId(id)) {
        ACSDK_ERROR(LX("handleStopFailed"));
        promise->set_value(false);
        return;
    }

    GstState curState;
    GstState pending;
    auto stateChangeRet = gst_element_get_state(m_pipeline.pipeline, &curState, &pending, TIMEOUT_ZERO_NANOSECONDS);
    if (GST_STATE_CHANGE_FAILURE == stateChangeRet) {
        ACSDK_ERROR(LX("handleStopFailed").d("reason", "gstElementGetStateFailure"));
        promise->set_value(false);
        return;
    }

    // Only stop if currently not stopped.
    if (curState == GST_STATE_NULL) {
        ACSDK_ERROR(LX("handleStopFailed").d("reason", "alreadyStopped"));
        promise->set_value(false);
        return;
    }

    if (pending == GST_STATE_NULL) {
        ACSDK_ERROR(LX("handleStopFailed").d("reason", "alreadyStopping"));
        promise->set_value(false);
        return;
    }

    stateChangeRet = gst_element_set_state(m_pipeline.pipeline, GST_STATE_NULL);
    if (GST_STATE_CHANGE_FAILURE == stateChangeRet) {
        ACSDK_ERROR(LX("handleStopFailed").d("reason", "gstElementSetStateFailure"));
        promise->set_value(false);
    } else {
        /*
         * Based on GStreamer docs, a gst_element_set_state call to change the state to GST_STATE_NULL will never
         * return GST_STATE_CHANGE_ASYNC.
         */
        promise->set_value(true);
        if (m_playPending) {
            sendPlaybackStarted();
        } else if (m_resumePending) {
            sendPlaybackResumed();
        }
        sendPlaybackStopped();
    }
}

void MediaPlayer::handlePause(MediaPlayer::SourceId id, std::promise<bool>* promise) {
    ACSDK_DEBUG(LX("handlePauseCalled").d("idPassed", id).d("currentId", (m_currentId)));
    if (!validateSourceAndId(id)) {
        ACSDK_ERROR(LX("handlePauseFailed"));
        promise->set_value(false);
        return;
    }

    GstState curState;
    auto stateChangeRet = gst_element_get_state(m_pipeline.pipeline, &curState, NULL, TIMEOUT_ZERO_NANOSECONDS);
    if (GST_STATE_CHANGE_FAILURE == stateChangeRet) {
        ACSDK_ERROR(LX("handlePauseFailed").d("reason", "gstElementGetStateFailure"));
        promise->set_value(false);
        return;
    }

    /*
     * If a play() or resume() call is pending, we want to try pausing immediately to avoid blips in audio.
     */
    if (m_playPending || m_resumePending) {
        ACSDK_DEBUG9(LX("handlePauseCalled").d("info", "playOrResumePending"));
        if (m_pausePending) {
            ACSDK_DEBUG(LX("handlePauseFailed").d("reason", "pauseCurrentlyPending"));
            promise->set_value(false);
            return;
        }
        stateChangeRet = gst_element_set_state(m_pipeline.pipeline, GST_STATE_PAUSED);
        if (GST_STATE_CHANGE_FAILURE == stateChangeRet) {
            ACSDK_ERROR(LX("handlePauseFailed").d("reason", "gstElementSetStateFailure"));
            promise->set_value(false);
        } else {
            m_pauseImmediately = true;
            ACSDK_DEBUG(LX("handlePause").d("pauseImmediately", "true"));
            promise->set_value(true);
        }
        return;
    }

    if (curState != GST_STATE_PLAYING) {
        ACSDK_ERROR(LX("handlePauseFailed").d("reason", "noAudioPlaying"));
        promise->set_value(false);
        return;
    }
    if (m_pausePending) {
        ACSDK_DEBUG(LX("handlePauseFailed").d("reason", "pauseCurrentlyPending"));
        promise->set_value(false);
        return;
    }

    stateChangeRet = gst_element_set_state(m_pipeline.pipeline, GST_STATE_PAUSED);
    if (GST_STATE_CHANGE_FAILURE == stateChangeRet) {
        ACSDK_ERROR(LX("handlePauseFailed").d("reason", "gstElementSetStateFailure"));
        promise->set_value(false);
    } else {
        m_pausePending = true;
        promise->set_value(true);
    }
}

void MediaPlayer::handleResume(MediaPlayer::SourceId id, std::promise<bool>* promise) {
    ACSDK_DEBUG(LX("handleResumeCalled").d("idPassed", id).d("currentId", (m_currentId)));
    if (!validateSourceAndId(id)) {
        ACSDK_ERROR(LX("handleResumeFailed"));
        promise->set_value(false);
        return;
    }

    GstState curState;
    auto stateChangeRet = gst_element_get_state(m_pipeline.pipeline, &curState, NULL, TIMEOUT_ZERO_NANOSECONDS);

    if (GST_STATE_CHANGE_FAILURE == stateChangeRet) {
        ACSDK_ERROR(LX("handleResumeFailed").d("reason", "gstElementGetStateFailure"));
        promise->set_value(false);
        return;
    }

    if (GST_STATE_PLAYING == curState) {
        ACSDK_ERROR(LX("handleResumeFailed").d("reason", "alreadyPlaying"));
        promise->set_value(false);
        return;
    }

    // Only unpause if currently paused.
    if (GST_STATE_PAUSED != curState) {
        ACSDK_ERROR(LX("handleResumeFailed").d("reason", "notCurrentlyPaused"));
        promise->set_value(false);
        return;
    }

    if (m_resumePending) {
        ACSDK_DEBUG(LX("handleResumeFailed").d("reason", "resumeCurrentlyPending"));
        promise->set_value(false);
        return;
    }

    // TODO: ACSDK-1778: Verify if we need to check percent buffer here and decide whether to continue pausing to refill
    // buffer or go straight to play
    stateChangeRet = gst_element_set_state(m_pipeline.pipeline, GST_STATE_PLAYING);
    if (GST_STATE_CHANGE_FAILURE == stateChangeRet) {
        ACSDK_ERROR(LX("handleResumeFailed").d("reason", "gstElementSetStateFailure"));
        promise->set_value(false);
    } else {
        m_resumePending = true;
        m_pauseImmediately = false;
        promise->set_value(true);
    }
}

void MediaPlayer::handleGetOffset(SourceId id, std::promise<std::chrono::milliseconds>* promise) {
    ACSDK_DEBUG9(LX("handleGetOffsetCalled").d("idPassed", id).d("currentId", (m_currentId)));

    // Check if pipeline is set.
    if (!m_pipeline.pipeline) {
        ACSDK_INFO(LX("handleGetOffsetStopped").m("pipelineNotSet"));
        promise->set_value(MEDIA_PLAYER_INVALID_OFFSET);
        return;
    }

    if (!validateSourceAndId(id)) {
        promise->set_value(m_offsetBeforeTeardown);
        return;
    }

    promise->set_value(getCurrentStreamOffset());
}

std::chrono::milliseconds MediaPlayer::getCurrentStreamOffset() {
    ACSDK_DEBUG9(LX("getCurrentStreamOffsetCalled"));

    auto offsetInMilliseconds = MEDIA_PLAYER_INVALID_OFFSET;
    gint64 position = -1;
    GstState state = GST_STATE_NULL;
    auto stateChangeRet = gst_element_get_state(m_pipeline.pipeline, &state, NULL, TIMEOUT_ZERO_NANOSECONDS);
    if (GST_STATE_CHANGE_FAILURE == stateChangeRet) {
        // Getting the state failed.
        ACSDK_ERROR(LX("getCurrentStreamOffsetFailed").d("reason", "getElementGetStateFailure"));
    } else if (GST_STATE_CHANGE_SUCCESS != stateChangeRet) {
        // Getting the state was not successful (GST_STATE_CHANGE_ASYNC or GST_STATE_CHANGE_NO_PREROLL).
        ACSDK_WARN(LX("getCurrentStreamOffsetError")
                       .d("reason", "getElementGetStateUnsuccessful")
                       .d("stateChangeReturn", gst_element_state_change_return_get_name(stateChangeRet)));
    } else if (GST_STATE_PAUSED != state && GST_STATE_PLAYING != state) {
        // Invalid State.
        ACSDK_DEBUG9(LX("getCurrentStreamOffsetInvalid")
                         .d("reason", "invalidPipelineState")
                         .d("state", gst_element_state_get_name(state))
                         .d("expectedStates", "PAUSED/PLAYING"));
    } else if (!gst_element_query_position(m_pipeline.pipeline, GST_FORMAT_TIME, &position)) {
        // Query Failed.
        ACSDK_ERROR(LX("getCurrentStreamOffsetFailed").d("reason", "gstElementQueryPositionError"));
    } else {
        // Query succeeded.
        std::chrono::milliseconds startStreamingPoint = std::chrono::milliseconds::zero();
        if (m_urlConverter) {
            startStreamingPoint = m_urlConverter->getStartStreamingPoint();
        }
        offsetInMilliseconds =
            (startStreamingPoint +
             std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds(position)));

        ACSDK_DEBUG9(LX("getCurrentStreamOffset").d("offsetInMilliseconds", offsetInMilliseconds.count()));
    }

    return offsetInMilliseconds;
}

void MediaPlayer::handleSetObserver(
    std::promise<void>* promise,
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface> observer) {
    ACSDK_DEBUG(LX("handleSetObserverCalled"));
    m_playerObserver = observer;
    promise->set_value();
}

void MediaPlayer::sendPlaybackStarted() {
    if (!m_playbackStartedSent) {
        ACSDK_DEBUG(LX("callingOnPlaybackStarted").d("currentId", m_currentId));
        m_playbackStartedSent = true;
        m_playPending = false;
        if (m_playerObserver) {
            m_playerObserver->onPlaybackStarted(m_currentId);
        }
    }
}

void MediaPlayer::sendPlaybackFinished() {
    if (m_currentId == ERROR_SOURCE_ID) {
        return;
    }
    m_isPaused = false;
    m_playbackStartedSent = false;
    if (!m_playbackFinishedSent) {
        m_playbackFinishedSent = true;
        ACSDK_DEBUG(LX("callingOnPlaybackFinished").d("currentId", m_currentId));
        if (m_playerObserver) {
            m_playerObserver->onPlaybackFinished(m_currentId);
        }
    }

    tearDownTransientPipelineElements(false);
    if (m_urlConverter) {
        m_urlConverter->shutdown();
    }
    m_urlConverter.reset();
}

void MediaPlayer::sendPlaybackPaused() {
    ACSDK_DEBUG(LX("callingOnPlaybackPaused").d("currentId", m_currentId));
    m_pausePending = false;
    m_isPaused = true;
    if (m_playerObserver) {
        m_playerObserver->onPlaybackPaused(m_currentId);
    }
}

void MediaPlayer::sendPlaybackResumed() {
    ACSDK_DEBUG(LX("callingOnPlaybackResumed").d("currentId", m_currentId));
    m_resumePending = false;
    m_isPaused = false;
    if (m_playerObserver) {
        m_playerObserver->onPlaybackResumed(m_currentId);
    }
}

void MediaPlayer::sendPlaybackStopped() {
    if (m_currentId == ERROR_SOURCE_ID) {
        return;
    }
    ACSDK_DEBUG(LX("callingOnPlaybackStopped").d("currentId", m_currentId));
    if (m_playerObserver && ERROR_SOURCE_ID != m_currentId) {
        m_playerObserver->onPlaybackStopped(m_currentId);
    }

    tearDownTransientPipelineElements(false);
    if (m_urlConverter) {
        m_urlConverter->shutdown();
    }
    m_urlConverter.reset();
}

void MediaPlayer::sendPlaybackError(const ErrorType& type, const std::string& error) {
    if (m_currentId == ERROR_SOURCE_ID) {
        return;
    }
    ACSDK_DEBUG(LX("callingOnPlaybackError").d("type", type).d("error", error).d("currentId", m_currentId));
    m_playPending = false;
    m_pausePending = false;
    m_resumePending = false;
    m_pauseImmediately = false;
    if (m_playerObserver) {
        m_playerObserver->onPlaybackError(m_currentId, type, error);
    }

    tearDownTransientPipelineElements(false);
    if (m_urlConverter) {
        m_urlConverter->shutdown();
    }
    m_urlConverter.reset();
}

void MediaPlayer::sendBufferUnderrun() {
    ACSDK_DEBUG(LX("callingOnBufferUnderrun").d("currentId", m_currentId));
    if (m_playerObserver) {
        m_playerObserver->onBufferUnderrun(m_currentId);
    }
}

void MediaPlayer::sendBufferRefilled() {
    ACSDK_DEBUG(LX("callingOnBufferRefilled").d("currentId", m_currentId));
    if (m_playerObserver) {
        m_playerObserver->onBufferRefilled(m_currentId);
    }
}

bool MediaPlayer::validateSourceAndId(SourceId id) {
    if (!m_source) {
        ACSDK_ERROR(LX("validateSourceAndIdFailed").d("reason", "sourceNotSet"));
        return false;
    }
    if (id != m_currentId) {
        ACSDK_ERROR(LX("validateSourceAndIdFailed").d("reason", "sourceIdMismatch"));
        return false;
    }
    return true;
}

gboolean MediaPlayer::onErrorCallback(gpointer pointer) {
    ACSDK_DEBUG9(LX("onErrorCallback"));
    auto mediaPlayer = static_cast<MediaPlayer*>(pointer);
    mediaPlayer->sendPlaybackError(ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "streamingError");
    return false;
}

void MediaPlayer::cleanUpSource() {
    if (m_pipeline.pipeline) {
        gst_element_set_state(m_pipeline.pipeline, GST_STATE_NULL);
    }
    if (m_source) {
        m_source->shutdown();
    }
    m_source.reset();
}

int MediaPlayer::clampEqualizerLevel(int level) {
    return std::min(std::max(level, MIN_EQUALIZER_LEVEL), MAX_EQUALIZER_LEVEL);
}

void MediaPlayer::setEqualizerBandLevels(audio::EqualizerBandLevelMap bandLevelMap) {
    if (!m_equalizerEnabled) {
        return;
    }
    std::promise<void> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, &promise, bandLevelMap]() {
        auto it = bandLevelMap.find(audio::EqualizerBand::BASS);
        if (bandLevelMap.end() != it) {
            g_object_set(
                G_OBJECT(m_pipeline.equalizer),
                GSTREAMER_BASS_BAND_NAME,
                static_cast<gdouble>(clampEqualizerLevel(it->second)),
                NULL);
        }
        it = bandLevelMap.find(audio::EqualizerBand::MIDRANGE);
        if (bandLevelMap.end() != it) {
            g_object_set(
                G_OBJECT(m_pipeline.equalizer),
                GSTREAMER_MIDRANGE_BAND_NAME,
                static_cast<gdouble>(clampEqualizerLevel(it->second)),
                NULL);
        }
        it = bandLevelMap.find(audio::EqualizerBand::TREBLE);
        if (bandLevelMap.end() != it) {
            g_object_set(
                G_OBJECT(m_pipeline.equalizer),
                GSTREAMER_TREBLE_BAND_NAME,
                static_cast<gdouble>(clampEqualizerLevel(it->second)),
                NULL);
        }
        promise.set_value();
        return false;
    };
    if (queueCallback(&callback) != UNQUEUED_CALLBACK) {
        future.get();
    }
}

int MediaPlayer::getMinimumBandLevel() {
    return MIN_EQUALIZER_LEVEL;
}

int MediaPlayer::getMaximumBandLevel() {
    return MAX_EQUALIZER_LEVEL;
}

}  // namespace mediaPlayer
}  // namespace alexaClientSDK
