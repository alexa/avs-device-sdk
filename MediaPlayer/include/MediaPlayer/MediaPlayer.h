/*
 * MediaPlayer.h
 *
 * Copyright (c) 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_MEDIA_PLAYER_INCLUDE_MEDIA_PLAYER_MEDIA_PLAYER_H_
#define ALEXA_CLIENT_SDK_MEDIA_PLAYER_INCLUDE_MEDIA_PLAYER_MEDIA_PLAYER_H_

#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <thread>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

#include <AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>

namespace alexaClientSDK {
namespace mediaPlayer {

/**
 * Class that handles creation of audio pipeline and playing of audio data.
 */
class MediaPlayer: public avsCommon::utils::mediaPlayer::MediaPlayerInterface {
public:
    /**
     * Creates an instance of the @c MediaPlayer.
     *
     * @return An instance of the @c MediaPlayer if successful else a @c nullptr.
     */
    static std::shared_ptr<MediaPlayer> create();

    /**
     * Destructor.
     */
    ~MediaPlayer();

    avsCommon::utils::mediaPlayer::MediaPlayerStatus setSource(
            std::unique_ptr<avsCommon::avs::attachment::AttachmentReader> reader) override;

    avsCommon::utils::mediaPlayer::MediaPlayerStatus play() override;

    avsCommon::utils::mediaPlayer::MediaPlayerStatus stop() override;

    int64_t getOffsetInMilliseconds() override;

    void setObserver(std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface> observer) override;

private:
    /**
     * The @c AudioPipeline consists of the following elements:
     * @li @c appsrc The appsrc element is used as the source to which audio data is provided.
     * @li @c decoder Decodebin is used as the decoder element to decode audio.
     * @li @c converter An audio-converter is used to convert between audio formats.
     * @li @c audioSink Sink for the audio.
     * @li @c pipeline The pipeline is a bin consisting of the @c appsrc, the @c decoder, the @c converter, and the
     * @c audioSink.
     *
     * The data flow through the elements is appsrc -> decoder -> converter -> audioSink.
     */
    struct AudioPipeline {
        /// The source element.
        GstAppSrc* appsrc;

        /// The decoder element.
        GstElement* decoder;

        /// The converter element.
        GstElement* converter;

        /// The sink element.
        GstElement* audioSink;

        /// Pipeline element.
        GstElement* pipeline;
    };

    /**
     * Constructor.
     */
    MediaPlayer();

    /**
     * Initializes Gstreamer. Creates the different elements of the pipeline and links the elements. Sets up the
     * signals and their callbacks.
     *
     * @return @c SUCCESS if initialization was successful. Else @c FAILURE.
     */
    avsCommon::utils::mediaPlayer::MediaPlayerStatus initPlayer();

    /**
     * Worker thread handler for setting m_workerThreadId.
     *
     * @return Whether the callback should be called back when worker thread is once again idle (always @c false).
     */
    static gboolean onSetWorkerThreadId(gpointer pointer);

    /**
     * Queue the specified callback for execution on the worker thread.
     *
     * @param callback The callback to queue.
     */
    void queueCallback(const std::function<gboolean()>* callback);

    /**
     * Notification of a callback to execute on the worker thread.
     *
     * @param callback The callback to execute.
     * @return Whether the callback should be called back when worker thread is once again idle.
     */
    static gboolean onCallback(const std::function<gboolean()>* callback);

    /**
     * This handles linking the source pad of the decoder to the sink pad of the converter once the pad-added signal
     * has been emitted by the decoder element.
     *
     * @note Pads are the element's interface. Data streams from one element's source pad to another element's sink pad.
     *
     * @param src The element for which the pad has been added.
     * @param pad The pad added to the @c src element.
     * @param mediaPlayer The instance of the mediaPlayer that the @c src is part of.
     */
    static void onPadAdded(GstElement* src, GstPad* pad, gpointer mediaPlayer);

    /**
     * Performs the linking of the decoder and converter elements once the pads have been added to the decoder element.
     *
     * @param promise A void promise to fulfill once this method completes.
     * @param src The element for which the pad has been added.
     * @param pad The pad added to the @c src element.
     */
    void handlePadAdded(std::promise<void>* promise, GstElement* src, GstPad* pad);

    /**
     * The callback for processing messages posted on the bus.
     *
     * @param bus Gstreamer bus.
     * @param msg The message posted to the bus.
     * @param mediaPlayer The instance of the mediaPlayer that the @c bus is a part of.
     *
     * @return @c true. GStreamer needs this functions to return true always. On @c false, the main-loop thread on which
     * this function is executed exits.
     */
    static gboolean onBusMessage(GstBus* bus, GstMessage* msg, gpointer mediaPlayer);

    /**
     * Performs actions based on the message.
     *
     * @param message The message posted on the bus.
     *
     * @return Always @c true.
     */
    gboolean handleBusMessage(GstMessage* message);

    /**
     * Worker thread handler for setting the source of audio to play.
     *
     * @param promise A promise to fulfill with a @ MediaPlayerStatus value once the source has been set
     * (or the operation failed).
     * @param reader The AttachmentReader with which to receive the audio to play.
     */
    void handleSetSource(
            std::promise<avsCommon::utils::mediaPlayer::MediaPlayerStatus>* promise,
            std::unique_ptr<avsCommon::avs::attachment::AttachmentReader> reader);

    /**
     * Worker thread handler for starting playback of the current audio source.
     *
     * @param promise A promise to fulfill with a @c MediaPlayerStatus value once playback has been initiated
     * (or the operation has failed).
     */
    void handlePlay(std::promise<avsCommon::utils::mediaPlayer::MediaPlayerStatus>* promise);

    /**
     * Worker thread handler for stopping audio playback.
     *
     * @param promise A promise to fulfill with a @c MediaPlayerStatus once playback stop has been initiated
     * (or the operation has failed).
     */
    void handleStop(std::promise<avsCommon::utils::mediaPlayer::MediaPlayerStatus>* promise);

    /**
     * Actually trigger a stop of audio playback.
     *
     * @return The status of the operation.
     */
    avsCommon::utils::mediaPlayer::MediaPlayerStatus doStop();

    /**
     * Worker thread handler for getting the current playback position.
     *
     * @param promise A promise to fulfill with the offset once the value has been determined.
     */
    void handleGetOffsetInMilliseconds(std::promise<int64_t>* promise);

    /**
     * Worker thread handler for setting the observer.
     *
     * @param promise A void promise to fulfill once the observer has been set.
     * @param observer The new observer.
     */
    void handleSetObserver(
            std::promise<void>* promise,
            std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface> observer);

    /**
     * Sends the playback started notification to the observer.
     */
    void sendPlaybackStarted();

    /**
     * Sends the playback finished notification to the observer.
     */
    void sendPlaybackFinished();

    /**
     * Sends the playback error notification to the observer.
     *
     * @param error The error details.
     */
    void sendPlaybackError(const std::string& error);

    /**
     * The callback for pushing data into the appsrc element.
     *
     * @param pipeline The pipeline element.
     * @param size The size of the data to push to the pipeline.
     * @param mediaPlayer The instance of the mediaPlayer that the bus is a part of.
     */
    static void onNeedData(GstElement* pipeline, guint size, gpointer mediaPlayer);

    /**
     * Pushes data into the appsrc element by starting a read data loop to run.
     *
     * @return Whether this callback should be called again on the worker thread.
     */
    gboolean handleNeedData();

    /**
     * The callback to stop pushing data into the appsrc element.
     *
     * @param pipeline The pipeline element.
     * @param mediaPlayer The instance of the mediaPlayer that the pipeline is a part of.
     */
    static void onEnoughData(GstElement* pipeline, gpointer mediaPlayer);

    /**
     * Stops pushing data to the appsrc element.
     *
     * @return Whether this callback should be called again on the worker thread.
     */
    gboolean handleEnoughData();

    /**
     * The callback for reading data from the attachment.
     *
     * @param mediaPlayer An instance of the mediaPlayer.
     *
     * @return @c false if there is an error or end of attachment else @c true.
     */
    static gboolean onReadData(gpointer mediaPlayer);

    /**
     * Reads data from the attachment and pushes it into appsrc element.
     *
     * @return @c false if there is an error or end of attachment else @c true.
     */
    gboolean handleReadData();

    /**
     * Signal gstreamer about the end of the audio stream.  @c close() and release @c m_attachmentReader.
     */
    void endDataFromAttachment();

    /**
     * Close @c m_attachmentReader (if not @c nullptr) and release it.
     */
    void resetAttachmentReader(
            std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> reader = nullptr);

    /**
     * Install the @c onReadData() handler.  If it is already installed, reset the retry count.
     */
    void installOnReadDataHandler();

    /**
     * Update when to call @c onReadData() handler based upon the number of retries since data was last read.
     */
    void updateOnReadDataHandler();

    /**
     * Uninstall the @c onReadData() handler.
     */
    void uninstallOnReadDataHandler();

    /**
     * Clear out the tracking of the @c onReadData() handler callback.  This is used when gstreamer is
     * known to have uninstalled the handler on its own.
     */
    void clearOnReadDataHandler();

    /// An instance of the @c AudioPipeline.
    AudioPipeline m_audioPipeline;

    /// Main event loop.
    GMainLoop* m_mainLoop;

    // Main loop thread
    std::thread m_mainLoopThread;

    // The thread that callbacks queued via @c g_idle_add() and @c m_timeout_add() will be called on.
    std::thread::id m_workerThreadId;

    /// Bus Id to track the bus.
    guint m_busWatchId;

    /// The sourceId used to identify the installation of the onReadData() handler.
    guint m_sourceId;

    /// Number of times reading data has been attempted since data was last successfully read.
    guint m_sourceRetryCount;

    /// The AttachmentReader to read audioData from.
    std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> m_attachmentReader;

    /// Flag to indicate when a playback finished notification has been sent to the observer.
    bool m_playbackFinishedSent;

    /// @c MediaPlayerObserverInterface instance to notify when the playback state changes.
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface> m_playerObserver;

    /// Function to invoke on the worker thread thread when more data is needed.
    const std::function<gboolean()> m_handleNeedDataFunction;

    /// Function to invoke on the worker thread thread when there is enough data.
    const std::function<gboolean()> m_handleEnoughDataFunction;
};

} // namespace mediaPlayer
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_MEDIA_PLAYER_INCLUDE_MEDIA_PLAYER_MEDIA_PLAYER_H_
