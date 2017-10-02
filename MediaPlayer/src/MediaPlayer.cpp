/*
 * MediaPlayer.cpp
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

#include <cstring>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/AVS/Attachment/AttachmentReader.h>
#include <PlaylistParser/PlaylistParser.h>

#include "MediaPlayer/AttachmentReaderSource.h"
#include "MediaPlayer/ErrorTypeConversion.h"
#include "MediaPlayer/IStreamSource.h"
#include "MediaPlayer/UrlSource.h"

#include "MediaPlayer/MediaPlayer.h"

namespace alexaClientSDK {
namespace mediaPlayer {

using namespace avsCommon::utils;
using namespace avsCommon::utils::mediaPlayer;
using namespace avsCommon::avs::attachment;

/// String to identify log entries originating from this file.
static const std::string TAG("MediaPlayer");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Timeout value for calls to @c gst_element_get_state() calls.
static const unsigned int TIMEOUT_ZERO_NANOSECONDS(0);

std::shared_ptr<MediaPlayer> MediaPlayer::create(
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory) {
    ACSDK_DEBUG9(LX("createCalled"));
    std::shared_ptr<MediaPlayer> mediaPlayer(new MediaPlayer(contentFetcherFactory));
    if (mediaPlayer->init()) {
        return mediaPlayer;
    } else {
        return nullptr;
    }
};

MediaPlayer::~MediaPlayer() {
    ACSDK_DEBUG9(LX("~MediaPlayerCalled"));
    stop();
    if (m_source) {
        m_source->terminate();
    }
    m_source.reset();
    // Destroy before g_main_loop.
    if (m_setSourceThread.joinable()) {
        m_setSourceThread.join();
    }
    g_main_loop_quit(m_mainLoop);
    if (m_mainLoopThread.joinable()) {
        m_mainLoopThread.join();
    }
    gst_object_unref(m_pipeline.pipeline);
    resetPipeline();
    g_source_remove(m_busWatchId);
    g_main_loop_unref(m_mainLoop);
}

MediaPlayerStatus MediaPlayer::setSource(std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> reader) {
    ACSDK_DEBUG9(LX("setSourceCalled").d("sourceType", "AttachmentReader"));
    std::promise<MediaPlayerStatus> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, &promise, &reader]() {
        handleSetAttachmentReaderSource(&promise, std::move(reader));
        return false;
    };
    queueCallback(&callback);
    return future.get();
}

MediaPlayerStatus MediaPlayer::setSource(std::shared_ptr<std::istream> stream, bool repeat) {
    ACSDK_DEBUG9(LX("setSourceCalled").d("sourceType", "istream"));
    std::promise<MediaPlayerStatus> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, &promise, &stream, repeat]() {
        handleSetIStreamSource(&promise, stream, repeat);
        return false;
    };
    queueCallback(&callback);
    return future.get();
}

MediaPlayerStatus MediaPlayer::setSource(const std::string& url) {
    ACSDK_DEBUG9(LX("setSourceForUrlCalled").sensitive("url", url));
    std::promise<MediaPlayerStatus> promise;
    std::future<MediaPlayerStatus> future = promise.get_future();

    if (m_setSourceThread.joinable()) {
        m_setSourceThread.join();
    }

    std::function<gboolean()> callback = [this, &promise, url]() {

        /*
         * We call the tearDown here instead of inside MediaPlayer::handleSetSource to ensure
         * serialization of the tearDowns in the g_main_loop
         */
        tearDownTransientPipelineElements();

        /*
         * A separate thread is needed because the UrlSource needs block and wait for callbacks
         * from the main event loop (g_main_loop). Deadlock will occur if UrlSource is created
         * on the main event loop.
         *
         * TODO: This thread is only needed for the Totem PlaylistParser.  Need to investigate if
         * it's still needed after a new PlaylistParser is added.
         */
        m_setSourceThread = std::thread(&MediaPlayer::handleSetSource, this, std::move(promise), url);
        return false;
    };
    queueCallback(&callback);
    return future.get();
}

MediaPlayerStatus MediaPlayer::play() {
    ACSDK_DEBUG9(LX("playCalled"));
    if (!m_source) {
        ACSDK_ERROR(LX("playFailed").d("reason", "sourceNotSet"));
        return MediaPlayerStatus::FAILURE;
    }
    m_source->preprocess();

    std::promise<MediaPlayerStatus> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, &promise]() {
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
    std::function<gboolean()> callback = [this, &promise]() {
        handleStop(&promise);
        return false;
    };
    queueCallback(&callback);
    return future.get();
}

MediaPlayerStatus MediaPlayer::pause() {
    ACSDK_DEBUG9(LX("pausedCalled"));
    std::promise<MediaPlayerStatus> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, &promise]() {
        handlePause(&promise);
        return false;
    };
    queueCallback(&callback);
    return future.get();
}

MediaPlayerStatus MediaPlayer::resume() {
    ACSDK_DEBUG9(LX("resumeCalled"));
    std::promise<MediaPlayerStatus> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, &promise]() {
        handleResume(&promise);
        return false;
    };
    queueCallback(&callback);
    return future.get();
}

std::chrono::milliseconds MediaPlayer::getOffset() {
    ACSDK_DEBUG9(LX("getOffsetCalled"));
    std::promise<std::chrono::milliseconds> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, &promise]() {
        handleGetOffset(&promise);
        return false;
    };
    queueCallback(&callback);
    return future.get();
}

MediaPlayerStatus MediaPlayer::setOffset(std::chrono::milliseconds offset) {
    ACSDK_DEBUG9(LX("setOffsetCalled"));
    std::promise<MediaPlayerStatus> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, &promise, offset]() {
        handleSetOffset(&promise, offset);
        return false;
    };
    queueCallback(&callback);
    return future.get();
}

void MediaPlayer::setObserver(std::shared_ptr<MediaPlayerObserverInterface> observer) {
    ACSDK_DEBUG9(LX("setObserverCalled"));
    std::promise<void> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, &promise, observer]() {
        handleSetObserver(&promise, observer);
        return false;
    };
    queueCallback(&callback);
    future.wait();
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
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory) :
        m_contentFetcherFactory{contentFetcherFactory},
        m_playbackStartedSent{false},
        m_playbackFinishedSent{false},
        m_isPaused{false},
        m_isBufferUnderrun{false},
        m_playerObserver{nullptr} {
}

bool MediaPlayer::init() {
    if (false == gst_init_check(NULL, NULL, NULL)) {
        ACSDK_ERROR(LX("initPlayerFailed").d("reason", "gstInitCheckFailed"));
        return false;
    }

    if (!(m_mainLoop = g_main_loop_new(nullptr, false))) {
        ACSDK_ERROR(LX("initPlayerFailed").d("reason", "gstMainLoopNewFailed"));
        return false;
    };

    m_mainLoopThread = std::thread(g_main_loop_run, m_mainLoop);

    if (!setupPipeline()) {
        ACSDK_ERROR(LX("initPlayerFailed").d("reason", "setupPipelineFailed"));
        return false;
    }

    return true;
}

bool MediaPlayer::setupPipeline() {
    m_pipeline.converter = gst_element_factory_make("audioconvert", "converter");
    if (!m_pipeline.converter) {
        ACSDK_ERROR(LX("setupPipelineFailed").d("reason", "createConverterElementFailed"));
        return false;
    }

    m_pipeline.audioSink = gst_element_factory_make("autoaudiosink", "audio_sink");
    if (!m_pipeline.audioSink) {
        ACSDK_ERROR(LX("setupPipelineFailed").d("reason", "createAudioSinkElementFailed"));
        return false;
    }

    m_pipeline.pipeline = gst_pipeline_new("audio-pipeline");
    if (!m_pipeline.pipeline) {
        ACSDK_ERROR(LX("setupPipelineFailed").d("reason", "createPipelineElementFailed"));
        return false;
    }

    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline.pipeline));
    m_busWatchId = gst_bus_add_watch(bus, &MediaPlayer::onBusMessage, this);
    gst_object_unref(bus);

    // Link only the converter and sink here. Src will be linked in respective source files.
    gst_bin_add_many(GST_BIN(m_pipeline.pipeline), m_pipeline.converter, m_pipeline.audioSink, nullptr);

    if (!gst_element_link(m_pipeline.converter, m_pipeline.audioSink)) {
        ACSDK_ERROR(LX("setupPipelineFailed").d("reason", "createConverterToSinkLinkFailed"));
        return false;
    }

    return true;
}

void MediaPlayer::tearDownTransientPipelineElements() {
    ACSDK_DEBUG9(LX("tearDownTransientPipelineElements"));
    if (m_pipeline.pipeline) {
        doStop();
        if (m_pipeline.appsrc) {
            gst_bin_remove(GST_BIN(m_pipeline.pipeline), GST_ELEMENT(m_pipeline.appsrc));
        }
        m_pipeline.appsrc = nullptr;

        if (m_pipeline.decoder) {
            gst_bin_remove(GST_BIN(m_pipeline.pipeline), GST_ELEMENT(m_pipeline.decoder));
        }
        m_pipeline.decoder = nullptr;
    }
    m_offsetManager.clear();
}

void MediaPlayer::resetPipeline() {
    ACSDK_DEBUG9(LX("resetPipeline"));
    m_pipeline.pipeline = nullptr;
    m_pipeline.appsrc = nullptr;
    m_pipeline.decoder = nullptr;
    m_pipeline.converter = nullptr;
    m_pipeline.audioSink = nullptr;
}

bool MediaPlayer::queryBufferingStatus(bool* buffering) {
    ACSDK_DEBUG9(LX("queryBufferingStatus"));
    GstQuery* query = gst_query_new_buffering(GST_FORMAT_TIME);
    if (gst_element_query(m_pipeline.pipeline, query)) {
        gboolean busy;
        gst_query_parse_buffering_percent(query, &busy, nullptr);
        *buffering = busy;
        ACSDK_INFO(LX("queryBufferingStatus").d("buffering", *buffering));
        gst_query_unref(query);
        return true;
    } else {
        ACSDK_ERROR(LX("queryBufferingStatusFailed").d("reason", "bufferyQueryFailed"));
        gst_query_unref(query);
        return false;
    }
}

bool MediaPlayer::queryIsSeekable(bool* isSeekable) {
    ACSDK_DEBUG9(LX("queryIsSeekable"));
    gboolean seekable;
    GstQuery* query;
    query = gst_query_new_seeking(GST_FORMAT_TIME);
    if (gst_element_query(m_pipeline.pipeline, query)) {
        gst_query_parse_seeking(query, NULL, &seekable, NULL, NULL);
        *isSeekable = (seekable == TRUE);
        ACSDK_DEBUG(LX("queryIsSeekable").d("isSeekable", *isSeekable));
        gst_query_unref(query);
        return true;
    } else {
        ACSDK_ERROR(LX("queryIsSeekableFailed").d("reason", "seekQueryFailed"));
        gst_query_unref(query);
        return false;
    }
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
    return g_idle_add(reinterpret_cast<GSourceFunc>(&onCallback), const_cast<std::function<gboolean()>*>(callback));
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
    mediaPlayer->queueCallback(&callback);
    future.wait();
}

void MediaPlayer::handlePadAdded(std::promise<void>* promise, GstElement* decoder, GstPad* pad) {
    ACSDK_DEBUG9(LX("handlePadAddedSignalCalled"));
    GstElement* converter = m_pipeline.converter;
    gst_element_link(decoder, converter);
    promise->set_value();
}

gboolean MediaPlayer::onBusMessage(GstBus* bus, GstMessage* message, gpointer mediaPlayer) {
    return static_cast<MediaPlayer*>(mediaPlayer)->handleBusMessage(message);
}

gboolean MediaPlayer::handleBusMessage(GstMessage* message) {
    ACSDK_DEBUG9(LX("messageReceived").d("messageType", gst_message_type_get_name(GST_MESSAGE_TYPE(message))));
    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_EOS:
            if (GST_MESSAGE_SRC(message) == GST_OBJECT_CAST(m_pipeline.pipeline)) {
                if (!m_source->handleEndOfStream()) {
                    alexaClientSDK::avsCommon::utils::logger::LogEntry* errorDescription =
                        &(LX("handleBusMessageFailed").d("reason", "sourceHandleEndOfStreamFailed"));
                    ACSDK_ERROR(*errorDescription);
                    sendPlaybackError(ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, errorDescription->c_str());
                }

                // Continue playback if there is additional data.
                if (m_source->hasAdditionalData()) {
                    if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(m_pipeline.pipeline, GST_STATE_NULL)) {
                        alexaClientSDK::avsCommon::utils::logger::LogEntry* errorDescription =
                            &(LX("continuingPlaybackFailed").d("reason", "setPiplineToNullFailed"));

                        ACSDK_ERROR(*errorDescription);
                        sendPlaybackError(ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, errorDescription->c_str());
                    }

                    if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(m_pipeline.pipeline, GST_STATE_PLAYING)) {
                        alexaClientSDK::avsCommon::utils::logger::LogEntry* errorDescription =
                            &(LX("continuingPlaybackFailed").d("reason", "setPiplineToPlayingFailed"));

                        ACSDK_ERROR(*errorDescription);
                        sendPlaybackError(ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, errorDescription->c_str());
                    }
                } else {
                    tearDownTransientPipelineElements();
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
            sendPlaybackError(gerrorToErrorType(error, m_source->isPlaybackRemote()), error->message);
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

                if (newState == GST_STATE_PLAYING) {
                    if (!m_playbackStartedSent) {
                        sendPlaybackStarted();
                    } else {
                        if (m_isBufferUnderrun) {
                            sendBufferRefilled();
                            m_isBufferUnderrun = false;
                        } else if (m_isPaused) {
                            sendPlaybackResumed();
                            m_isPaused = false;
                        }
                    }
                } else if (
                    newState == GST_STATE_PAUSED && oldState == GST_STATE_READY &&
                    pendingState == GST_STATE_VOID_PENDING) {
                    /*
                     * Currently the hls/hlsdemux/hlssink plugins are needed to handle HLS sources.
                     * No BUFFERING message are sent, and instead the pipeline goes
                     * straight into a PAUSED state with the buffer query failing.
                     *
                     * This behavior has also been observed in a small percentage of unit tests.
                     *
                     * For use case of buffer query failing (ie not supporting buffering) or not currently
                     * buffering, start the playback immediately.
                     */
                    bool buffering = false;
                    if (!queryBufferingStatus(&buffering) || !buffering) {
                        gst_element_set_state(m_pipeline.pipeline, GST_STATE_PLAYING);
                    }
                } else if (newState == GST_STATE_PAUSED && oldState == GST_STATE_PLAYING) {
                    if (m_isBufferUnderrun) {
                        sendBufferUnderrun();
                    } else if (!m_isPaused) {
                        sendPlaybackPaused();
                        m_isPaused = true;
                    }
                } else if (newState == GST_STATE_NULL && oldState == GST_STATE_READY) {
                    sendPlaybackFinished();
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
        default:
            break;
    }

    return true;
}

void MediaPlayer::handleSetAttachmentReaderSource(
    std::promise<MediaPlayerStatus>* promise,
    std::shared_ptr<AttachmentReader> reader) {
    ACSDK_DEBUG(LX("handleSetSourceCalled"));

    tearDownTransientPipelineElements();

    m_source = AttachmentReaderSource::create(this, reader);

    if (!m_source) {
        ACSDK_ERROR(LX("handleSetAttachmentReaderSourceFailed").d("reason", "sourceIsNullptr"));
        promise->set_value(MediaPlayerStatus::FAILURE);
        return;
    }

    /*
     * Once the source pad for the decoder has been added, the decoder emits the pad-added signal. Connect the signal
     * to the callback which performs the linking of the decoder source pad to the converter sink pad.
     */
    if (!g_signal_connect(m_pipeline.decoder, "pad-added", G_CALLBACK(onPadAdded), this)) {
        ACSDK_ERROR(LX("handleSetAttachmentReaderSourceFailed").d("reason", "connectPadAddedSignalFailed"));
        promise->set_value(MediaPlayerStatus::FAILURE);
        return;
    }

    promise->set_value(MediaPlayerStatus::SUCCESS);
}

void MediaPlayer::handleSetIStreamSource(
    std::promise<MediaPlayerStatus>* promise,
    std::shared_ptr<std::istream> stream,
    bool repeat) {
    ACSDK_DEBUG(LX("handleSetSourceCalled"));

    tearDownTransientPipelineElements();

    m_source = IStreamSource::create(this, stream, repeat);

    if (!m_source) {
        ACSDK_ERROR(LX("handleSetIStreamSourceFailed").d("reason", "sourceIsNullptr"));
        promise->set_value(MediaPlayerStatus::FAILURE);
        return;
    }

    /*
     * Once the source pad for the decoder has been added, the decoder emits the pad-added signal. Connect the signal
     * to the callback which performs the linking of the decoder source pad to the converter sink pad.
     */
    if (!g_signal_connect(m_pipeline.decoder, "pad-added", G_CALLBACK(onPadAdded), this)) {
        ACSDK_ERROR(LX("handleSetIStreamSourceFailed").d("reason", "connectPadAddedSignalFailed"));
        promise->set_value(MediaPlayerStatus::FAILURE);
        return;
    }

    promise->set_value(MediaPlayerStatus::SUCCESS);
}

void MediaPlayer::handleSetSource(std::promise<MediaPlayerStatus> promise, std::string url) {
    ACSDK_DEBUG(LX("handleSetSourceForUrlCalled"));
    m_source =
        UrlSource::create(this, alexaClientSDK::playlistParser::PlaylistParser::create(m_contentFetcherFactory), url);
    if (!m_source) {
        ACSDK_ERROR(LX("handleSetSourceForUrlFailed").d("reason", "sourceIsNullptr"));
        promise.set_value(MediaPlayerStatus::FAILURE);
        return;
    }

    /*
     * This works with audio only sources. This does not work for any source that has more than one stream.
     * The first pad that is added may not be the correct stream (ie may be a video stream), and will fail.
     *
     * Once the source pad for the decoder has been added, the decoder emits the pad-added signal. Connect the signal
     * to the callback which performs the linking of the decoder source pad to the converter sink pad.
     */
    if (!g_signal_connect(m_pipeline.decoder, "pad-added", G_CALLBACK(onPadAdded), this)) {
        ACSDK_ERROR(LX("handleSetSourceForUrlFailed").d("reason", "connectPadAddedSignalFailed"));
        promise.set_value(MediaPlayerStatus::FAILURE);
        return;
    }

    promise.set_value(MediaPlayerStatus::SUCCESS);
}

void MediaPlayer::handlePlay(std::promise<MediaPlayerStatus>* promise) {
    ACSDK_DEBUG(LX("handlePlayCalled"));

    // If the player was in PLAYING state or was pending transition to PLAYING state, stop playing audio.
    if (MediaPlayerStatus::SUCCESS != doStop()) {
        ACSDK_ERROR(LX("handlePlayFailed").d("reason", "doStopFailed"));
        promise->set_value(MediaPlayerStatus::FAILURE);
        return;
    }

    m_playbackFinishedSent = false;

    gboolean attemptBuffering;
    g_object_get(m_pipeline.decoder, "use-buffering", &attemptBuffering, NULL);
    ACSDK_DEBUG(LX("handlePlay").d("attemptBuffering", attemptBuffering));

    GstState startingState = GST_STATE_PLAYING;
    if (attemptBuffering) {
        /*
         * Set pipeline to PAUSED state to attempt buffering.
         * The pipeline will be set to PLAY in two ways:
         * i) If buffering is supported, then upon receiving buffer percent = 100.
         * ii) If buffering is not supported, then the pipeline will be set to PLAY immediately.
         */
        startingState = GST_STATE_PAUSED;
    }

    auto stateChangeRet = gst_element_set_state(m_pipeline.pipeline, startingState);
    ACSDK_DEBUG(LX("handlePlay")
                    .d("startingState", gst_element_state_get_name(startingState))
                    .d("stateReturn", gst_element_state_change_return_get_name(stateChangeRet)));
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

void MediaPlayer::handleStop(std::promise<MediaPlayerStatus>* promise) {
    ACSDK_DEBUG(LX("handleStopCalled"));
    promise->set_value(doStop());
}

MediaPlayerStatus MediaPlayer::doStop() {
    GstState state;
    GstState pending;

    auto stateChangeRet = gst_element_get_state(m_pipeline.pipeline, &state, &pending, TIMEOUT_ZERO_NANOSECONDS);
    if (GST_STATE_CHANGE_FAILURE == stateChangeRet) {
        ACSDK_ERROR(LX("doStopFailed").d("reason", "gstElementGetStateFailed"));
        return MediaPlayerStatus::FAILURE;
    }

    if (GST_STATE_CHANGE_SUCCESS == stateChangeRet && GST_STATE_NULL == state) {
        ACSDK_DEBUG(LX("doStopSuccess").d("reason", "alreadyStopped"));
    } else if (GST_STATE_CHANGE_ASYNC == stateChangeRet && GST_STATE_NULL == pending) {
        ACSDK_DEBUG(LX("doStopSuccess").d("reason", "alreadyStopping"));
    } else {
        stateChangeRet = gst_element_set_state(m_pipeline.pipeline, GST_STATE_NULL);
        if (GST_STATE_CHANGE_FAILURE == stateChangeRet) {
            ACSDK_ERROR(LX("doStopFailed").d("reason", "gstElementSetStateFailed"));
            if (m_source) {
                m_source->terminate();
            }
            m_source.reset();
            return MediaPlayerStatus::FAILURE;
        } else if (GST_STATE_CHANGE_ASYNC == stateChangeRet) {
            ACSDK_DEBUG9(LX("doStopPending"));
            return MediaPlayerStatus::PENDING;
        } else {
            sendPlaybackFinished();
        }
    }
    ACSDK_DEBUG9(LX("doStopSuccess"));
    return MediaPlayerStatus::SUCCESS;
}

void MediaPlayer::handlePause(std::promise<MediaPlayerStatus>* promise) {
    ACSDK_DEBUG(LX("handlePauseCalled"));
    if (!m_source) {
        ACSDK_ERROR(LX("handlePauseFailed").d("reason", "sourceNotSet"));
        promise->set_value(MediaPlayerStatus::FAILURE);
        return;
    }

    GstState curState;
    // If previous set state return was GST_STATE_CHANGE_ASYNC, this will block infinitely
    // until that state has been set.
    auto stateChangeRet = gst_element_get_state(m_pipeline.pipeline, &curState, NULL, GST_CLOCK_TIME_NONE);
    if (GST_STATE_CHANGE_FAILURE == stateChangeRet) {
        ACSDK_ERROR(LX("handlePauseFailed").d("reason", "gstElementGetStateFailure"));
        promise->set_value(MediaPlayerStatus::FAILURE);
        return;
    }

    // Error if attempting to pause in any other state.
    if (curState != GST_STATE_PLAYING) {
        ACSDK_ERROR(LX("handlePauseFailed").d("reason", "noAudioPlaying"));
        promise->set_value(MediaPlayerStatus::FAILURE);
        return;
    }

    stateChangeRet = gst_element_set_state(m_pipeline.pipeline, GST_STATE_PAUSED);
    if (GST_STATE_CHANGE_FAILURE == stateChangeRet) {
        ACSDK_ERROR(LX("handlePauseFailed").d("reason", "gstElementSetStateFailure"));
        promise->set_value(MediaPlayerStatus::FAILURE);
    } else if (GST_STATE_CHANGE_ASYNC == stateChangeRet) {
        promise->set_value(MediaPlayerStatus::PENDING);
    } else {
        promise->set_value(MediaPlayerStatus::SUCCESS);
    }
    return;
}

void MediaPlayer::handleResume(std::promise<MediaPlayerStatus>* promise) {
    ACSDK_DEBUG(LX("handleResumeCalled"));
    if (!m_source) {
        ACSDK_ERROR(LX("handleResumeFailed").d("reason", "sourceNotSet"));
        promise->set_value(MediaPlayerStatus::FAILURE);
        return;
    }

    GstState curState;
    // If previous set state return was GST_STATE_CHANGE_ASYNC, this will block infinitely
    // until that state has been set.
    auto stateChangeRet = gst_element_get_state(m_pipeline.pipeline, &curState, NULL, GST_CLOCK_TIME_NONE);

    if (GST_STATE_CHANGE_FAILURE == stateChangeRet) {
        ACSDK_ERROR(LX("handleResumeFailed").d("reason", "gstElementGetStateFailure"));
        promise->set_value(MediaPlayerStatus::FAILURE);
    }

    // Only unpause if currently paused.
    if (curState != GST_STATE_PAUSED) {
        ACSDK_ERROR(LX("handleResumeFailed").d("reason", "notCurrentlyPaused"));
        promise->set_value(MediaPlayerStatus::FAILURE);
        return;
    }

    stateChangeRet = gst_element_set_state(m_pipeline.pipeline, GST_STATE_PLAYING);
    if (GST_STATE_CHANGE_FAILURE == stateChangeRet) {
        ACSDK_ERROR(LX("handleResumeFailed").d("reason", "gstElementSetStateFailure"));
        promise->set_value(MediaPlayerStatus::FAILURE);
    } else if (GST_STATE_CHANGE_ASYNC == stateChangeRet) {
        promise->set_value(MediaPlayerStatus::PENDING);
    } else {
        promise->set_value(MediaPlayerStatus::SUCCESS);
    }

    return;
}

void MediaPlayer::handleGetOffset(std::promise<std::chrono::milliseconds>* promise) {
    ACSDK_DEBUG(LX("handleGetOffsetCalled"));
    gint64 position = -1;
    GstState state;

    // Check if pipeline is set.
    if (!m_pipeline.pipeline) {
        ACSDK_INFO(LX("handleGetOffsetStopped").m("pipelineNotSet"));
        promise->set_value(MEDIA_PLAYER_INVALID_OFFSET);
        return;
    }

    auto stateChangeRet = gst_element_get_state(m_pipeline.pipeline, &state, NULL, TIMEOUT_ZERO_NANOSECONDS);

    if (GST_STATE_CHANGE_FAILURE == stateChangeRet) {
        // Getting the state failed.
        ACSDK_ERROR(LX("handleGetOffsetFailed").d("reason", "getElementGetStateFailure"));
    } else if (GST_STATE_CHANGE_SUCCESS != stateChangeRet) {
        // Getting the state was not successful (GST_STATE_CHANGE_ASYNC or GST_STATE_CHANGE_NO_PREROLL).
        ACSDK_INFO(LX("handleGetOffset")
                       .d("reason", "getElementGetStateUnsuccessful")
                       .d("stateChangeReturn", gst_element_state_change_return_get_name(stateChangeRet)));
    } else if (GST_STATE_PAUSED != state && GST_STATE_PLAYING != state) {
        // Invalid State.
        std::ostringstream expectedStates;
        expectedStates << gst_element_state_get_name(GST_STATE_PAUSED) << "/"
                       << gst_element_state_get_name(GST_STATE_PLAYING);
        ACSDK_ERROR(LX("handleGetOffsetFailed")
                        .d("reason", "invalidPipelineState")
                        .d("state", gst_element_state_get_name(state))
                        .d("expectedStates", expectedStates.str()));
    } else if (!gst_element_query_position(m_pipeline.pipeline, GST_FORMAT_TIME, &position)) {
        // Query Failed.
        ACSDK_ERROR(LX("handleGetOffsetInMillisecondsFailed").d("reason", "gstElementQueryPositionError"));
    } else {
        // Query succeeded.
        promise->set_value(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds(position)));
        return;
    }

    promise->set_value(MEDIA_PLAYER_INVALID_OFFSET);
}

void MediaPlayer::handleSetOffset(std::promise<MediaPlayerStatus>* promise, std::chrono::milliseconds offset) {
    ACSDK_DEBUG(LX("handleSetOffsetCalled"));
    m_offsetManager.setSeekPoint(offset);
    promise->set_value(MediaPlayerStatus::SUCCESS);
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
        ACSDK_DEBUG(LX("callingOnPlaybackStarted"));
        m_playbackStartedSent = true;
        if (m_playerObserver) {
            m_playerObserver->onPlaybackStarted();
        }
    }
}

void MediaPlayer::sendPlaybackFinished() {
    if (m_source) {
        m_source->terminate();
    }
    m_source.reset();
    m_isPaused = false;
    m_playbackStartedSent = false;
    if (!m_playbackFinishedSent) {
        m_playbackFinishedSent = true;
        ACSDK_DEBUG(LX("callingOnPlaybackFinished"));
        if (m_playerObserver) {
            m_playerObserver->onPlaybackFinished();
        }
    }
}

void MediaPlayer::sendPlaybackPaused() {
    ACSDK_DEBUG(LX("callingOnPlaybackPaused"));
    if (m_playerObserver) {
        m_playerObserver->onPlaybackPaused();
    }
}

void MediaPlayer::sendPlaybackResumed() {
    ACSDK_DEBUG(LX("callingOnPlaybackResumed"));
    if (m_playerObserver) {
        m_playerObserver->onPlaybackResumed();
    }
}

void MediaPlayer::sendPlaybackError(const ErrorType& type, const std::string& error) {
    ACSDK_DEBUG(LX("callingOnPlaybackError").d("type", type).d("error", error));
    if (m_playerObserver) {
        m_playerObserver->onPlaybackError(type, error);
    }
}

void MediaPlayer::sendBufferUnderrun() {
    ACSDK_DEBUG(LX("callingOnBufferUnderrun"));
    if (m_playerObserver) {
        m_playerObserver->onBufferUnderrun();
    }
}

void MediaPlayer::sendBufferRefilled() {
    ACSDK_DEBUG(LX("callingOnBufferRefilled"));
    if (m_playerObserver) {
        m_playerObserver->onBufferRefilled();
    }
}

}  // namespace mediaPlayer
}  // namespace alexaClientSDK
