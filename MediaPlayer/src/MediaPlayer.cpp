/*
 * MediaPlayer.cpp
 *
 * Copyright 2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <cstring>

#include <AVSUtils/Logger/LogEntry.h>
#include <AVSUtils/Logging/Logger.h>
#include <AVSCommon/AVS/Attachment/AttachmentReader.h>

#include "MediaPlayer/MediaPlayer.h"

namespace alexaClientSDK {
namespace mediaPlayer {

using namespace avsUtils;
using namespace avsCommon::utils::mediaPlayer;
using namespace avsCommon::avs::attachment;

/// String to identify log entries originating from this file.
static const std::string TAG("MediaPlayer");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsUtils::logger::LogEntry(TAG, event)

// Extra detailed logging. Suppressed by default.
#ifdef ENABLE_MEDIA_PLAYER_DEBUG9_LOGGING
#define ACSDK_DEBUG9(...) ACSDK_DEBUG(__VA_ARGS__)
#else
#define ACSDK_DEBUG9(...)
#endif

// Macro used to verify that various methods are running on the worker thread as expected.
#ifdef DEBUG
#define VERIFY_ON_WORKER_THREAD()                                                                       \
    do {                                                                                                \
        auto id = std::this_thread::get_id();                                                           \
        if (id != m_workerThreadId) {                                                                   \
            ACSDK_ERROR(LX("VERIFY_ON_WORKER_THREAD_FAILED")                                            \
                    .d("line", __LINE__).d("thisThreadId", id).d("workerThreadId", m_workerThreadId));  \
        }                                                                                               \
    } while (false)
#else
#define VERIFY_ON_WORKER_THREAD() do {} while (false)
#endif

/// The number of bytes read from the attachment with each read in the read loop.
static const unsigned int CHUNK_SIZE(4096);

/// The interval to wait (in milliseconds) between successive attempts to read audio data when none is available.
static const guint RETRY_INTERVALS_MILLISECONDS[] = {0, 10, 10, 10, 20, 20, 50, 100};

/// Timeout value for calls to @c gst_element_get_state() calls.
static const unsigned int TIMEOUT_ZERO_NANOSECONDS(0);

/// Number of nanoseconds in a millisecond.
static const unsigned int NANOSECONDS_TO_MILLISECONDS(1000000);

std::shared_ptr<MediaPlayer> MediaPlayer::create() {
    ACSDK_DEBUG9(LX("createCalled"));
    std::shared_ptr<MediaPlayer> mediaPlayer(new MediaPlayer());
    if (MediaPlayerStatus::SUCCESS == mediaPlayer->initPlayer()) {
        return mediaPlayer;
    } else {
        return nullptr;
    }
};

MediaPlayer::~MediaPlayer() {
    ACSDK_DEBUG9(LX("~MediaPlayerCalled"));
    stop();
    g_main_loop_quit(m_mainLoop);
    if (m_mainLoopThread.joinable()) {
        m_mainLoopThread.join();
    }
    gst_object_unref(m_audioPipeline.pipeline);
    g_source_remove(m_busWatchId);
    g_main_loop_unref(m_mainLoop);
}

MediaPlayerStatus MediaPlayer::setSource(std::unique_ptr<AttachmentReader> reader) {
    ACSDK_DEBUG9(LX("setSourceCalled"));
    std::promise<MediaPlayerStatus> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, &promise, &reader](){
        handleSetSource(&promise, std::move(reader));
        return false;
    };
    queueCallback(&callback);
    return future.get();
}

MediaPlayerStatus MediaPlayer::play() {
    ACSDK_DEBUG9(LX("playCalled"));
    std::promise<MediaPlayerStatus> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, &promise](){
        handlePlay(&promise);
        return false;
    };
    queueCallback(&callback);
    return future.get();
}

MediaPlayerStatus MediaPlayer::stop() {
    ACSDK_DEBUG9(LX("stopCalled"));
    std::promise<MediaPlayerStatus> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, &promise](){
        handleStop(&promise);
        return false;
    };
    queueCallback(&callback);
    return future.get();
}

int64_t MediaPlayer::getOffsetInMilliseconds() {
    ACSDK_DEBUG9(LX("getOffsetInMillisecondsCalled"));
    std::promise<int64_t> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, &promise](){
        handleGetOffsetInMilliseconds(&promise);
        return false;
    };
    queueCallback(&callback);
    return future.get();
}

void MediaPlayer::setObserver(std::shared_ptr<MediaPlayerObserverInterface> observer) {
    ACSDK_DEBUG9(LX("setObserverCalled"));
    std::promise<void> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, &promise, observer](){
        handleSetObserver(&promise, observer);
        return false;
    };
    queueCallback(&callback);
    future.wait();
}

MediaPlayer::MediaPlayer() :
        m_sourceId{0},
        m_sourceRetryCount{0},
        m_playbackFinishedSent{false},
        m_playerObserver{nullptr},
        m_handleNeedDataFunction{[this]() { return handleNeedData(); }},
        m_handleEnoughDataFunction{[this]() { return handleEnoughData(); }} {
}

MediaPlayerStatus MediaPlayer::initPlayer() {
    if(false == gst_init_check (NULL, NULL, NULL)) {
        ACSDK_ERROR(LX("initPlayerFailed").d("reason", "gstInitCheckFailed"));
        return MediaPlayerStatus::FAILURE;
    }

    g_idle_add(reinterpret_cast<GSourceFunc>(&MediaPlayer::onSetWorkerThreadId), this);

    if (!(m_mainLoop = g_main_loop_new(nullptr, false))) {
        ACSDK_ERROR(LX("initPlayerFailed").d("reason", "gstMainLoopNewFailed"));
        return MediaPlayerStatus::FAILURE;
    };

    m_audioPipeline.appsrc = reinterpret_cast<GstAppSrc*>(gst_element_factory_make("appsrc", "src"));
    if (!m_audioPipeline.appsrc) {
        ACSDK_ERROR(LX("initPlayerFailed").d("reason", "createSourceElementFailed"));
        return MediaPlayerStatus::FAILURE;
    }
    gst_app_src_set_stream_type(m_audioPipeline.appsrc, GST_APP_STREAM_TYPE_STREAM);

    m_audioPipeline.decoder = gst_element_factory_make("decodebin", "decoder");
    if (!m_audioPipeline.decoder) {
        ACSDK_ERROR(LX("initPlayerFailed").d("reason", "createDecoderElementFailed"));
        return MediaPlayerStatus::FAILURE;
    }

    m_audioPipeline.converter = gst_element_factory_make("audioconvert", "converter");
    if (!m_audioPipeline.converter) {
        ACSDK_ERROR(LX("initPlayerFailed").d("reason", "createConverterElementFailed"));
        return MediaPlayerStatus::FAILURE;
    }

    m_audioPipeline.audioSink = gst_element_factory_make("pulsesink", "audio_sink");
    if (!m_audioPipeline.audioSink) {
        // Try creating an osxaudiosink if the pulsesink failed.
        m_audioPipeline.audioSink = gst_element_factory_make("osxaudiosink", "audio_sink");
        if (!m_audioPipeline.audioSink) {
            ACSDK_ERROR(LX("initPlayerFailed").d("reason", "createAudioSinkElementFailed"));
            return MediaPlayerStatus::FAILURE;
        }
    }

    m_audioPipeline.pipeline = gst_pipeline_new("audio-pipeline");
    if (!m_audioPipeline.pipeline) {
        ACSDK_ERROR(LX("initPlayerFailed").d("reason", "createPipelineElementFailed"));
        return MediaPlayerStatus::FAILURE;
    }

    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(m_audioPipeline.pipeline));
    m_busWatchId = gst_bus_add_watch(bus, &MediaPlayer::onBusMessage, this);
    gst_object_unref(bus);

    gst_bin_add_many(GST_BIN(m_audioPipeline.pipeline), reinterpret_cast<GstElement*>(m_audioPipeline.appsrc),
            m_audioPipeline.decoder, m_audioPipeline.converter, m_audioPipeline.audioSink, nullptr);
    /*
     * Link the source and decoder elements. The decoder source pad is added dynamically after it has determined the
     * stream type it is decoding. Once the pad has been added, pad-added signal is emitted, and the padAddedHandler
     * callback will link the newly created source pad of the decoder to the sink of the converter element.
     */
    if (false == gst_element_link(reinterpret_cast<GstElement*>(m_audioPipeline.appsrc), m_audioPipeline.decoder)) {
        ACSDK_ERROR(LX("initPlayerFailed").d("reason", "createSourceToDecoderLinkFailed"));
        return MediaPlayerStatus::FAILURE;
    }
    if (false == gst_element_link(m_audioPipeline.converter, m_audioPipeline.audioSink)) {
        ACSDK_ERROR(LX("initPlayerFailed").d("reason", "createConverterToSinkLinkFailed"));
        return MediaPlayerStatus::FAILURE;
    }
    /*
     * Once the source pad for the decoder has been added, the decoder emits the pad-added signal. Connect the signal
     * to the callback which performs the linking of the decoder source pad to the converter sink pad.
     */
    if (!g_signal_connect(m_audioPipeline.decoder, "pad-added", G_CALLBACK(onPadAdded), this)) {
        ACSDK_ERROR(LX("initPlayerFailed").d("reason", "connectPadAddedSignalFailed"));
        return MediaPlayerStatus::FAILURE;
    }
    /*
     * When the appsrc needs data, it emits the signal need-data. Connect the need-data signal to the onNeedData
     * callback which handles pushing data to the appsrc element.
     */
    if (!g_signal_connect(m_audioPipeline.appsrc, "need-data", G_CALLBACK(onNeedData), this)) {
        ACSDK_ERROR(LX("initPlayerFailed").d("reason", "connectNeedDataSignalFailed"));
        return MediaPlayerStatus::FAILURE;
    }
    /*
     * When the appsrc had enough data, it emits the signal enough-data. Connect the enough-data signal to the
     * onEnoughData callback which handles stopping the data push to the appsrc element.
     */
    if (!g_signal_connect(m_audioPipeline.appsrc, "enough-data", G_CALLBACK(onEnoughData), this)) {
        ACSDK_ERROR(LX("initPlayerFailed").d("reason", "connectEnoughDataSignalFailed"));
        return MediaPlayerStatus::FAILURE;
    }

    m_mainLoopThread = std::thread(g_main_loop_run, m_mainLoop);

    return MediaPlayerStatus::SUCCESS;
}

gboolean MediaPlayer::onSetWorkerThreadId(gpointer pointer) {
    static_cast<MediaPlayer*>(pointer)->m_workerThreadId = std::this_thread::get_id();
    return false;
}

void MediaPlayer::queueCallback(const std::function<gboolean()> *callback) {
    g_idle_add(reinterpret_cast<GSourceFunc>(&onCallback), const_cast<std::function<gboolean()> *>(callback));
}

gboolean MediaPlayer::onCallback(const std::function<gboolean()> *callback) {
    return (*callback)();
}

void MediaPlayer::onPadAdded(GstElement *decoder, GstPad *pad, gpointer pointer) {
    ACSDK_DEBUG9(LX("onPadAddedCalled"));
    auto mediaPlayer = static_cast<MediaPlayer*>(pointer);
    std::promise<void> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [mediaPlayer, &promise, decoder, pad](){
        mediaPlayer->handlePadAdded(&promise, decoder, pad);
        return false;
    };
    mediaPlayer->queueCallback(&callback);
    future.wait();
}

void MediaPlayer::handlePadAdded(std::promise<void>* promise, GstElement *decoder, GstPad *pad) {
    VERIFY_ON_WORKER_THREAD();
    ACSDK_DEBUG9(LX("handlePadAddedSignalCalled"));
    GstElement *converter = m_audioPipeline.converter;
    gst_element_link(decoder, converter);
    promise->set_value();
}

gboolean MediaPlayer::onBusMessage(GstBus *bus, GstMessage *message, gpointer mediaPlayer) {
    return static_cast<MediaPlayer*>(mediaPlayer)->handleBusMessage(message);
}

gboolean MediaPlayer::handleBusMessage(GstMessage *message) {
    VERIFY_ON_WORKER_THREAD();
    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_EOS:
            if (GST_MESSAGE_SRC(message) == GST_OBJECT_CAST(m_audioPipeline.pipeline)) {
                sendPlaybackFinished();
            }
            break;

        case GST_MESSAGE_ERROR: {
            GError *error;
            gst_message_parse_error(message, &error, nullptr);
            std::string messageSrcName = GST_MESSAGE_SRC_NAME(message);
            ACSDK_ERROR(LX("handleBusMessageError").d("source", messageSrcName).d("error", error->message));
            sendPlaybackError(error->message);
            g_error_free(error);
            break;
        }
        case GST_MESSAGE_STATE_CHANGED: {
            // Check that the state change is for the pipeline.
            if (GST_MESSAGE_SRC(message) == GST_OBJECT_CAST(m_audioPipeline.pipeline)) {
                GstState oldState;
                GstState newState;
                GstState pendingState;
                gst_message_parse_state_changed(message, &oldState, &newState, &pendingState);
                if (newState == GST_STATE_PLAYING) {
                    sendPlaybackStarted();
                }
                /*
                 * If the previous state was PLAYING and the new state is PAUSED, ie, the audio has stopped playing,
                 * indicate to the observer that playback has finished.
                 */
                if (newState == GST_STATE_PAUSED && oldState == GST_STATE_PLAYING) {
                    sendPlaybackFinished();
                }
            }
            break;
        }
        default:
            break;
    }

    return true;
}

void MediaPlayer::handleSetSource(
        std::promise<MediaPlayerStatus> *promise, std::unique_ptr<AttachmentReader> reader) {
    VERIFY_ON_WORKER_THREAD();
    ACSDK_DEBUG(LX("handleSetSourceCalled"));
    if (MediaPlayerStatus::SUCCESS != doStop()) {
        ACSDK_ERROR(LX("handleSetSourceFailed").d("reason", "doStopFailed"));
        promise->set_value(MediaPlayerStatus::FAILURE);
        return;
    };
    uninstallOnReadDataHandler();
    resetAttachmentReader(std::move(reader));
    m_playbackFinishedSent = false;
    promise->set_value(MediaPlayerStatus::SUCCESS);
}

void MediaPlayer::handlePlay(std::promise<MediaPlayerStatus> *promise) {
    VERIFY_ON_WORKER_THREAD();
    ACSDK_DEBUG(LX("handlePlayCalled"));
    if (!m_attachmentReader) {
        ACSDK_ERROR(LX("handlePlayFailed").d("reason", "sourceNotSet"));
        promise->set_value(MediaPlayerStatus::FAILURE);
        return;
    }

    // If the player was in PLAYING state or was pending transition to PLAYING state, stop playing audio.
    if (MediaPlayerStatus::SUCCESS != doStop()) {
        ACSDK_ERROR(LX("handlePlayFailed").d("reason", "doStopFailed"));
        promise->set_value(MediaPlayerStatus::FAILURE);
        return;
    }

    m_playbackFinishedSent = false;

    auto stateChangeRet = gst_element_set_state(m_audioPipeline.pipeline, GST_STATE_PLAYING);
    if (GST_STATE_CHANGE_FAILURE == stateChangeRet) {
        ACSDK_ERROR(LX("handlePlayFailed").d("reason", "gstElementSetStateFailure"));
        promise->set_value(MediaPlayerStatus::FAILURE);
    } else if (GST_STATE_CHANGE_ASYNC == stateChangeRet) {
        promise->set_value(MediaPlayerStatus::PENDING);
    } else {
        promise->set_value(MediaPlayerStatus::SUCCESS);
    }
    return;
}

void MediaPlayer::handleStop(std::promise<MediaPlayerStatus> *promise) {
    VERIFY_ON_WORKER_THREAD();
    ACSDK_DEBUG(LX("handleStopCalled"));
    promise->set_value(doStop());
}

MediaPlayerStatus MediaPlayer::doStop() {
    VERIFY_ON_WORKER_THREAD();
    GstState state;
    GstState pending;

    auto stateChangeRet = gst_element_get_state(m_audioPipeline.pipeline, &state, &pending, TIMEOUT_ZERO_NANOSECONDS);
    if (GST_STATE_CHANGE_FAILURE == stateChangeRet) {
        ACSDK_ERROR(LX("doStopFailed").d("reason", "gstElementGetStateFailed"));
        return MediaPlayerStatus::FAILURE;
    }

    if (GST_STATE_CHANGE_SUCCESS == stateChangeRet && GST_STATE_NULL == state) {
        ACSDK_DEBUG(LX("doStopSuccess").d("reason", "alreadyStopped"));
    } else if (GST_STATE_CHANGE_ASYNC == stateChangeRet && GST_STATE_NULL == pending) {
        ACSDK_DEBUG(LX("doStopSuccess").d("reason", "alreadyStopping"));
    } else {
        resetAttachmentReader();
        stateChangeRet = gst_element_set_state(m_audioPipeline.pipeline, GST_STATE_NULL);
        if (GST_STATE_CHANGE_FAILURE == stateChangeRet) {
            ACSDK_ERROR(LX("doStopFailed").d("reason", "gstElementSetStateFailed"));
            return MediaPlayerStatus::FAILURE;
        } else if (GST_STATE_CHANGE_ASYNC == stateChangeRet) {
            ACSDK_DEBUG9(LX("doStopPending"));
            return MediaPlayerStatus::PENDING;
        } else {
            clearOnReadDataHandler();
            sendPlaybackFinished();
        }
    }
    ACSDK_DEBUG9(LX("doStopSuccess"));
    return MediaPlayerStatus::SUCCESS;
}

void MediaPlayer::handleGetOffsetInMilliseconds(std::promise<int64_t> *promise) {
    VERIFY_ON_WORKER_THREAD();
    ACSDK_DEBUG(LX("handleGetOffsetInMillisecondsCalled"));
    gint64 position = -1;
    GstState state;
    GstState pending;

    auto stateChangeRet = gst_element_get_state(
            m_audioPipeline.pipeline, &state, &pending, TIMEOUT_ZERO_NANOSECONDS);
    if (GST_STATE_CHANGE_FAILURE == stateChangeRet) {
        ACSDK_ERROR(LX("handleGetOffsetInMillisecondsFailed").d("reason", "getElementGetStateFailed"));
    } else if (GST_STATE_CHANGE_SUCCESS == stateChangeRet &&
           (GST_STATE_PLAYING == state || GST_STATE_PAUSED == state) &&
           gst_element_query_position(m_audioPipeline.pipeline, GST_FORMAT_TIME, &position)) {
        position /= NANOSECONDS_TO_MILLISECONDS;
    }
    promise->set_value(static_cast<int64_t>(position));
}

void MediaPlayer::handleSetObserver(
        std::promise<void>* promise,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface> observer) {
    VERIFY_ON_WORKER_THREAD();
    ACSDK_DEBUG(LX("handleSetObserverCalled"));
    m_playerObserver = observer;
    promise->set_value();
}

void MediaPlayer::sendPlaybackStarted() {
    VERIFY_ON_WORKER_THREAD();
    ACSDK_DEBUG(LX("callingOnPlaybackStarted"));
    if (m_playerObserver) {
        m_playerObserver->onPlaybackStarted();
    }
}

void MediaPlayer::sendPlaybackFinished() {
    VERIFY_ON_WORKER_THREAD();
    uninstallOnReadDataHandler();
    if (!m_playbackFinishedSent) {
        m_playbackFinishedSent = true;
        ACSDK_DEBUG(LX("callingOnPlaybackFinished"));
        if (m_playerObserver) {
            m_playerObserver->onPlaybackFinished();
        }
    }
}

void MediaPlayer::sendPlaybackError(const std::string& error) {
    VERIFY_ON_WORKER_THREAD();
    ACSDK_DEBUG(LX("callingOnPlaybackError").d("error", error));
    if (m_playerObserver) {
        m_playerObserver->onPlaybackError(error);
    }
}

void MediaPlayer::onNeedData(GstElement *pipeline, guint size, gpointer pointer) {
    ACSDK_DEBUG9(LX("onNeedDataCalled").d("size", size));
    auto mediaPlayer = static_cast<MediaPlayer*>(pointer);
    mediaPlayer->queueCallback(&mediaPlayer->m_handleNeedDataFunction);
}

gboolean MediaPlayer::handleNeedData() {
    VERIFY_ON_WORKER_THREAD();
    installOnReadDataHandler();
    return false;
}

void MediaPlayer::onEnoughData(GstElement *pipeline, gpointer pointer) {
    ACSDK_DEBUG9(LX("onEnoughDataCalled"));
    auto mediaPlayer = static_cast<MediaPlayer*>(pointer);
    mediaPlayer->queueCallback(&mediaPlayer->m_handleEnoughDataFunction);
}

gboolean MediaPlayer::handleEnoughData() {
    VERIFY_ON_WORKER_THREAD();
    uninstallOnReadDataHandler();
    return false;
}

gboolean MediaPlayer::onReadData(gpointer mediaPlayer) {
    return static_cast<MediaPlayer*>(mediaPlayer)->handleReadData();
}

gboolean MediaPlayer::handleReadData() {
    VERIFY_ON_WORKER_THREAD();
    if (!m_attachmentReader) {
        ACSDK_ERROR(LX("onReadDataFailed").d("reason", "calledAfterEndOfStreamReached"));
        return false;
    }

    auto buffer = gst_buffer_new_allocate(nullptr, CHUNK_SIZE, nullptr);

    if (!buffer) {
        ACSDK_ERROR(LX("onReadDataFailed").d("reason", "gstBufferNewAllocateFailed"));
        endDataFromAttachment();
        return false;
    }

    GstMapInfo info;
    if (!gst_buffer_map(buffer, &info, GST_MAP_WRITE)) {
        ACSDK_ERROR(LX("onReadDataFailed").d("reason", "gstBufferMapFailed"));
        gst_buffer_unref(buffer);
        endDataFromAttachment();
        return false;
    }

    auto status = AttachmentReader::ReadStatus::OK;
    auto size = m_attachmentReader->read(info.data, info.size, &status);

    ACSDK_DEBUG9(LX("read").d("size", size).d("status", static_cast<int>(status)));

    gst_buffer_unmap(buffer, &info);

    if (size > 0 && size < info.size) {
        gst_buffer_resize(buffer, 0, size);
    }

    switch (status) {
        case AttachmentReader::ReadStatus::CLOSED:
            if (0 == size) {
                break;
            }
            // Fall through if some data was read.
        case AttachmentReader::ReadStatus::OK:
        case AttachmentReader::ReadStatus::OK_WOULDBLOCK:
            if (size > 0) {
                installOnReadDataHandler();
                auto flowRet = gst_app_src_push_buffer(m_audioPipeline.appsrc, buffer);
                if (flowRet != GST_FLOW_OK) {
                    ACSDK_ERROR(LX("onReadDataFailed")
                            .d("reason", "gstAppSrcPushBufferFailed").d("error", static_cast<int>(flowRet)));
                    break;
                }
                return true;
            }
            // Fall through to retry reading later.
        case AttachmentReader::ReadStatus::OK_TIMEDOUT: {
            gst_buffer_unref(buffer);
            updateOnReadDataHandler();
            return true;
        }
        case AttachmentReader::ReadStatus::ERROR_OVERRUN:
        case AttachmentReader::ReadStatus::ERROR_BYTES_LESS_THAN_WORD_SIZE:
        case AttachmentReader::ReadStatus::ERROR_INTERNAL:
            auto error = static_cast<int>(status);
            ACSDK_ERROR(LX("onReadDataFailed").d("reason", "readFailed").d("error", error));
            break;
    }

    gst_buffer_unref(buffer);
    endDataFromAttachment();
    return false;
}

void MediaPlayer::endDataFromAttachment() {
    VERIFY_ON_WORKER_THREAD();
    ACSDK_DEBUG9(LX("endDataFromAttachmentCalled"));
    auto flowRet = gst_app_src_end_of_stream(m_audioPipeline.appsrc);
    if (flowRet != GST_FLOW_OK) {
        ACSDK_ERROR(LX("endDataFromAttachmentFailed")
                .d("reason", "gstAppSrcEndOfStreamFailed").d("result", static_cast<int>(flowRet)));
    }
    resetAttachmentReader();
    clearOnReadDataHandler();
}

void MediaPlayer::resetAttachmentReader(std::shared_ptr<AttachmentReader> reader) {
    VERIFY_ON_WORKER_THREAD();
    if (m_attachmentReader) {
        m_attachmentReader->close();
    }
    m_attachmentReader = reader;
}

void MediaPlayer::installOnReadDataHandler() {
    VERIFY_ON_WORKER_THREAD();
    if (!m_attachmentReader) {
        return;
    }
    if (m_sourceId != 0) {
        // Remove the existing source if it was timer based.  Otherwise it is already properly installed.
        if (m_sourceRetryCount != 0) {
            ACSDK_DEBUG9(LX("installOnReadDataHandler").d("action", "removeSourceId").d("sourceId", m_sourceId));
            if (!g_source_remove(m_sourceId)) {
                ACSDK_ERROR(LX("installOnReadDataHandlerError")
                        .d("reason", "gSourceRemoveFailed").d("sourceId", m_sourceId));
            }
        } else {
            return;
        }
    }
    m_sourceRetryCount = 0;
    m_sourceId = g_idle_add(reinterpret_cast<GSourceFunc>(&onReadData), this);
    ACSDK_DEBUG9(LX("installOnReadDataHandler").d("action", "newSourceId").d("sourceId", m_sourceId));
}

void MediaPlayer::updateOnReadDataHandler() {
    VERIFY_ON_WORKER_THREAD();
    if (m_sourceRetryCount < sizeof(RETRY_INTERVALS_MILLISECONDS) / sizeof(RETRY_INTERVALS_MILLISECONDS[0])) {
        ACSDK_DEBUG9(LX("updateOnReadDataHandler").d("action", "removeSourceId").d("sourceId", m_sourceId));
        if (!g_source_remove(m_sourceId)) {
            ACSDK_ERROR(LX("updateOnReadDataHandlerError")
                    .d("reason", "gSourceRemoveFailed").d("sourceId", m_sourceId));
        }
        auto interval = RETRY_INTERVALS_MILLISECONDS[m_sourceRetryCount];
        m_sourceRetryCount++;
        m_sourceId = g_timeout_add(interval, reinterpret_cast<GSourceFunc>(&onReadData), this);
        ACSDK_DEBUG9(LX("updateOnReadDataHandlerNewSourceId")
                .d("action", "newSourceId").d("sourceId", m_sourceId).d("sourceRetryCount", m_sourceRetryCount));
    }
}

void MediaPlayer::uninstallOnReadDataHandler() {
    VERIFY_ON_WORKER_THREAD();
    if (m_sourceId != 0) {
        if (!g_source_remove(m_sourceId)) {
            ACSDK_ERROR(LX("uninstallOnReadDataHandlerError")
                    .d("reason", "gSourceRemoveFailed").d("sourceId", m_sourceId));
        }
        clearOnReadDataHandler();
    }
}

void MediaPlayer::clearOnReadDataHandler() {
    VERIFY_ON_WORKER_THREAD();
    ACSDK_DEBUG9(LX("clearOnReadDataHandlerCalled"));
    m_sourceRetryCount = 0;
    m_sourceId = 0;
}

} // namespace mediaPlayer
} // namespace alexaClientSDK
