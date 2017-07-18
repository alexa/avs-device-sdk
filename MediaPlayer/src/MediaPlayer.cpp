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

#include "MediaPlayer/AttachmentReaderSource.h"
#include "MediaPlayer/IStreamSource.h"
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

/// Number of nanoseconds in a millisecond.
static const unsigned int NANOSECONDS_TO_MILLISECONDS(1000000);

std::shared_ptr<MediaPlayer> MediaPlayer::create() {
    ACSDK_DEBUG9(LX("createCalled"));
    std::shared_ptr<MediaPlayer> mediaPlayer(new MediaPlayer());
    if (mediaPlayer->init()) {
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
    gst_object_unref(m_pipeline.pipeline);
    g_source_remove(m_busWatchId);
    g_main_loop_unref(m_mainLoop);
}

MediaPlayerStatus MediaPlayer::setSource(
        std::unique_ptr<avsCommon::avs::attachment::AttachmentReader> reader) {
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

MediaPlayerStatus MediaPlayer::setSource(std::unique_ptr<std::istream> stream, bool repeat) {
    ACSDK_DEBUG9(LX("setSourceCalled").d("sourceType", "istream"));
    std::promise<MediaPlayerStatus> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, &promise, &stream, repeat]() {
        handleSetIStreamSource(&promise, std::move(stream), repeat);
        return false;
    };
    queueCallback(&callback);
    return future.get();
}

MediaPlayerStatus MediaPlayer::play() {
    ACSDK_DEBUG9(LX("playCalled"));
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

int64_t MediaPlayer::getOffsetInMilliseconds() {
    ACSDK_DEBUG9(LX("getOffsetInMillisecondsCalled"));
    std::promise<int64_t> promise;
    auto future = promise.get_future();
    std::function<gboolean()> callback = [this, &promise]() {
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

MediaPlayer::MediaPlayer() :
        m_pipeline{nullptr, nullptr, nullptr, nullptr, nullptr},
        m_playbackFinishedSent{false},
        m_playerObserver{nullptr} {
}

bool MediaPlayer::init() {
    if (false == gst_init_check (NULL, NULL, NULL)) {
        ACSDK_ERROR(LX("initPlayerFailed").d("reason", "gstInitCheckFailed"));
        return false;
    }

    if (!(m_mainLoop = g_main_loop_new(nullptr, false))) {
        ACSDK_ERROR(LX("initPlayerFailed").d("reason", "gstMainLoopNewFailed"));
        return false;
    };

    m_mainLoopThread = std::thread(g_main_loop_run, m_mainLoop);

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

void MediaPlayer::tearDownPipeline() {
    if (m_pipeline.pipeline) {
        doStop();
        gst_object_unref(m_pipeline.pipeline);
        m_pipeline.pipeline = nullptr;
        g_source_remove(m_busWatchId);
    }
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
    std::function<gboolean()> callback = [mediaPlayer, &promise, decoder, pad]() {
        mediaPlayer->handlePadAdded(&promise, decoder, pad);
        return false;
    };
    mediaPlayer->queueCallback(&callback);
    future.wait();
}

void MediaPlayer::handlePadAdded(std::promise<void>* promise, GstElement *decoder, GstPad *pad) {
    ACSDK_DEBUG9(LX("handlePadAddedSignalCalled"));
    GstElement *converter = m_pipeline.converter;
    gst_element_link(decoder, converter);
    promise->set_value();
}

gboolean MediaPlayer::onBusMessage(GstBus *bus, GstMessage *message, gpointer mediaPlayer) {
    return static_cast<MediaPlayer*>(mediaPlayer)->handleBusMessage(message);
}

gboolean MediaPlayer::handleBusMessage(GstMessage *message) {
    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_EOS:
            if (GST_MESSAGE_SRC(message) == GST_OBJECT_CAST(m_pipeline.pipeline)) {
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
            if (GST_MESSAGE_SRC(message) == GST_OBJECT_CAST(m_pipeline.pipeline)) {
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

void MediaPlayer::handleSetAttachmentReaderSource(
        std::promise<MediaPlayerStatus> *promise, std::unique_ptr<AttachmentReader> reader) {
    ACSDK_DEBUG(LX("handleSetSourceCalled"));

    tearDownPipeline();

    if (!setupPipeline()) {
        ACSDK_ERROR(LX("handleSetAttachmentReaderSourceFailed").d("reason", "setupPipelineFailed"));
        promise->set_value(MediaPlayerStatus::FAILURE);
        return;
    }

    m_source = AttachmentReaderSource::create(this, std::move(reader));

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
        std::promise<MediaPlayerStatus> *promise, std::unique_ptr<std::istream> stream, bool repeat) {
    ACSDK_DEBUG(LX("handleSetSourceCalled"));

    tearDownPipeline();

    if (!setupPipeline()) {
        ACSDK_ERROR(LX("handleSetIStreamSourceFailed").d("reason", "setupPipelineFailed"));
        promise->set_value(MediaPlayerStatus::FAILURE);
        return;
    }

    m_source = IStreamSource::create(this, std::move(stream), repeat);

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

void MediaPlayer::handlePlay(std::promise<MediaPlayerStatus> *promise) {
    ACSDK_DEBUG(LX("handlePlayCalled"));
    if (!m_source) {
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

    auto stateChangeRet = gst_element_set_state(m_pipeline.pipeline, GST_STATE_PLAYING);
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
            m_source.reset();
            return MediaPlayerStatus::FAILURE;
        } else if (GST_STATE_CHANGE_ASYNC == stateChangeRet) {
            ACSDK_DEBUG9(LX("doStopPending"));
            return MediaPlayerStatus::PENDING;
        } else {
            m_source.reset();
            sendPlaybackFinished();
        }
    }
    ACSDK_DEBUG9(LX("doStopSuccess"));
    return MediaPlayerStatus::SUCCESS;
}

void MediaPlayer::handleGetOffsetInMilliseconds(std::promise<int64_t> *promise) {
    ACSDK_DEBUG(LX("handleGetOffsetInMillisecondsCalled"));
    gint64 position = -1;
    GstState state;
    GstState pending;

    auto stateChangeRet = gst_element_get_state(
            m_pipeline.pipeline, &state, &pending, TIMEOUT_ZERO_NANOSECONDS);
    if (GST_STATE_CHANGE_FAILURE == stateChangeRet) {
        ACSDK_ERROR(LX("handleGetOffsetInMillisecondsFailed").d("reason", "getElementGetStateFailed"));
    } else if (GST_STATE_CHANGE_SUCCESS == stateChangeRet &&
           (GST_STATE_PLAYING == state || GST_STATE_PAUSED == state) &&
           gst_element_query_position(m_pipeline.pipeline, GST_FORMAT_TIME, &position)) {
        position /= NANOSECONDS_TO_MILLISECONDS;
    }
    promise->set_value(static_cast<int64_t>(position));
}

void MediaPlayer::handleSetObserver(
        std::promise<void>* promise,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface> observer) {
    ACSDK_DEBUG(LX("handleSetObserverCalled"));
    m_playerObserver = observer;
    promise->set_value();
}

void MediaPlayer::sendPlaybackStarted() {
    ACSDK_DEBUG(LX("callingOnPlaybackStarted"));
    if (m_playerObserver) {
        m_playerObserver->onPlaybackStarted();
    }
}

void MediaPlayer::sendPlaybackFinished() {
    m_source.reset();
    if (!m_playbackFinishedSent) {
        m_playbackFinishedSent = true;
        ACSDK_DEBUG(LX("callingOnPlaybackFinished"));
        if (m_playerObserver) {
            m_playerObserver->onPlaybackFinished();
        }
    }
}

void MediaPlayer::sendPlaybackError(const std::string& error) {
    ACSDK_DEBUG(LX("callingOnPlaybackError").d("error", error));
    if (m_playerObserver) {
        m_playerObserver->onPlaybackError(error);
    }
}

} // namespace mediaPlayer
} // namespace alexaClientSDK
