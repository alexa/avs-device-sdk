/*
 * MediaPlayer.h
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

#ifndef ALEXA_CLIENT_SDK_MEDIA_PLAYER_INCLUDE_MEDIA_PLAYER_MEDIA_PLAYER_H_
#define ALEXA_CLIENT_SDK_MEDIA_PLAYER_INCLUDE_MEDIA_PLAYER_MEDIA_PLAYER_H_

#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <queue>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterfaceFactoryInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/PlaylistParser/PlaylistParserInterface.h>

#include "MediaPlayer/OffsetManager.h"
#include "MediaPlayer/PipelineInterface.h"
#include "MediaPlayer/SourceInterface.h"

namespace alexaClientSDK {
namespace mediaPlayer {

/**
 * Class that handles creation of audio pipeline and playing of audio data.
 */
class MediaPlayer
        : public avsCommon::utils::mediaPlayer::MediaPlayerInterface
        , private PipelineInterface {
public:
    /**
     * Creates an instance of the @c MediaPlayer.
     *
     * @param contentFetcherFactory Used to create objects that can fetch remote HTTP content.
     * @return An instance of the @c MediaPlayer if successful else a @c nullptr.
     */
    static std::shared_ptr<MediaPlayer> create(
        std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory =
            nullptr);

    /**
     * Destructor.
     */
    ~MediaPlayer();

    /// @name Overridden MediaPlayerInterface methods.
    /// @{
    avsCommon::utils::mediaPlayer::MediaPlayerStatus setSource(
        std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader) override;
    avsCommon::utils::mediaPlayer::MediaPlayerStatus setSource(std::shared_ptr<std::istream> stream, bool repeat)
        override;
    avsCommon::utils::mediaPlayer::MediaPlayerStatus setSource(const std::string& url) override;

    avsCommon::utils::mediaPlayer::MediaPlayerStatus play() override;
    avsCommon::utils::mediaPlayer::MediaPlayerStatus stop() override;
    avsCommon::utils::mediaPlayer::MediaPlayerStatus pause() override;
    /**
     * To resume playback after a pause, call @c resume. Calling @c play
     * will reset the pipeline and source, and will not resume playback.
     */
    avsCommon::utils::mediaPlayer::MediaPlayerStatus resume() override;
    std::chrono::milliseconds getOffset() override;
    /**
     * This function is a setter, storing @c offset to be consumed internally by @c play().
     * The function will always return MediaPlayerStatus::SUCCESS.
     */
    avsCommon::utils::mediaPlayer::MediaPlayerStatus setOffset(std::chrono::milliseconds offset) override;
    void setObserver(std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface> observer) override;
    /// @}

    /// @name Overridden PipelineInterface methods.
    /// @{
    void setAppSrc(GstAppSrc* appSrc) override;
    GstAppSrc* getAppSrc() const override;
    void setDecoder(GstElement* decoder) override;
    GstElement* getDecoder() const override;
    GstElement* getPipeline() const override;
    guint queueCallback(const std::function<gboolean()>* callback) override;
    /// @}

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

        /// Constructor.
        AudioPipeline() :
                appsrc{nullptr},
                decoder{nullptr},
                converter{nullptr},
                audioSink{nullptr},
                pipeline{nullptr} {};
    };

    /**
     * Constructor.
     *
     * @param contentFetcherFactory Used to create objects that can fetch remote HTTP content.
     */
    MediaPlayer(
        std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory);

    /**
     * Initializes GStreamer and starts a main event loop on a new thread.
     *
     * @return @c SUCCESS if initialization was successful. Else @c FAILURE.
     */
    bool init();

    /**
     * Worker thread handler for setting m_workerThreadId.
     *
     * @return Whether the callback should be called back when worker thread is once again idle (always @c false).
     */
    static gboolean onSetWorkerThreadId(gpointer pointer);

    /**
     * Notification of a callback to execute on the worker thread.
     *
     * @param callback The callback to execute.
     * @return Whether the callback should be called back when worker thread is once again idle.
     */
    static gboolean onCallback(const std::function<gboolean()>* callback);

    /**
     * Creates the @c AudioPipeline with the permanent elements and links them together.  The permanent elements
     * are converter and audioSink.
     *
     * @return @c true if all the elements were created and linked successfully else @c false.
     */
    bool setupPipeline();

    /**
     * Stops the currently playing audio and removes the transient elements.  The transient elements
     * are appsrc and decoder.
     */
    void tearDownTransientPipelineElements();

    /*
     * Resets the @c AudioPipeline.
     */
    void resetPipeline();

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
     * @param bus GStreamer bus.
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
     * @param reader The @c AttachmentReader with which to receive the audio to play.
     */
    void handleSetAttachmentReaderSource(
        std::promise<avsCommon::utils::mediaPlayer::MediaPlayerStatus>* promise,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> reader);

    void handleSetSource(std::promise<avsCommon::utils::mediaPlayer::MediaPlayerStatus> promise, std::string url);

    /**
     * Worker thread handler for setting the source of audio to play.
     *
     * @param promise A promise to fulfill with a @ MediaPlayerStatus value once the source has been set
     * (or the operation failed).
     * @param stream The source from which to receive the audio to play.
     */
    void handleSetIStreamSource(
        std::promise<avsCommon::utils::mediaPlayer::MediaPlayerStatus>* promise,
        std::shared_ptr<std::istream> stream,
        bool repeat);

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
     * Worker thread handler for pausing playback of the current audio source.
     *
     * @param promise A promise to fulfill with a @c MediaPlayerStatus value once playback has been paused
     * (or the operation has failed).
     */
    void handlePause(std::promise<avsCommon::utils::mediaPlayer::MediaPlayerStatus>* promise);

    /**
     * Worker thread handler for resume playback of the current audio source.
     *
     * @param promise A promise to fulfill with a @c MediaPlayerStatus value once playback has been resumed
     * (or the operation has failed).
     */
    void handleResume(std::promise<avsCommon::utils::mediaPlayer::MediaPlayerStatus>* promise);

    /**
     * Worker thread handler for getting the current playback position.
     *
     * @param promise A promise to fulfill with the offset once the value has been determined.
     */
    void handleGetOffset(std::promise<std::chrono::milliseconds>* promise);

    /**
     * Worker thread handler for setting the playback position.
     *
     * @param promise A promise to fulfill with a @c MediaPlayerStatus value once the offset has been set.
     * @param offset The offset to start playing from.
     */
    void handleSetOffset(
        std::promise<avsCommon::utils::mediaPlayer::MediaPlayerStatus>* promise,
        std::chrono::milliseconds offset);

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
     * Sends the playback paused notification to the observer.
     */
    void sendPlaybackPaused();

    /**
     * Sends the playback resumed notification to the observer.
     */
    void sendPlaybackResumed();

    /**
     * Sends the playback error notification to the observer.
     *
     * @param type The error type.
     * @param error The error details.
     */
    void sendPlaybackError(
        const alexaClientSDK::avsCommon::utils::mediaPlayer::ErrorType& type,
        const std::string& error);

    /**
     * Sends the buffer underrun notification to the observer.
     */
    void sendBufferUnderrun();

    /**
     * Sends the buffer refilled notification to the observer.
     */
    void sendBufferRefilled();

    /**
     * Used to obtain seeking information about the pipeline.
     *
     * @param isSeekable A boolean indicating whether the stream is seekable.
     * @return A boolean indicating whether the operation was successful.
     */
    bool queryIsSeekable(bool* isSeekable);

    /**
     * Used to obtain the current buffering status of the pipeline.
     *
     * @param[out] buffering Whether the pipeline is currently buffering.
     * @return A boolean indicating whether the operation was successful.
     */
    bool queryBufferingStatus(bool* buffering);

    /**
     * Performs a seek to the @c seekPoint.
     *
     * @return A boolean indicating whether the seek operation was successful.
     */
    bool seek();

    /// An instance of the @c OffsetManager.
    OffsetManager m_offsetManager;

    /// Used to create objects that can fetch remote HTTP content.
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> m_contentFetcherFactory;

    /// An instance of the @c AudioPipeline.
    AudioPipeline m_pipeline;

    /// Main event loop.
    GMainLoop* m_mainLoop;

    // Main loop thread
    std::thread m_mainLoopThread;

    // Set Source thread.
    std::thread m_setSourceThread;

    /// Bus Id to track the bus.
    guint m_busWatchId;

    /// Flag to indicate when a playback started notification has been sent to the observer.
    bool m_playbackStartedSent;

    /// Flag to indicate when a playback finished notification has been sent to the observer.
    bool m_playbackFinishedSent;

    /// Flag to indicate whether a playback is paused.
    bool m_isPaused;

    /// Flag to indicate whether a buffer underrun is occurring.
    bool m_isBufferUnderrun;

    /// @c MediaPlayerObserverInterface instance to notify when the playback state changes.
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface> m_playerObserver;

    /// @c SourceInterface instance set to the appropriate source.
    std::shared_ptr<SourceInterface> m_source;
};

}  // namespace mediaPlayer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_MEDIA_PLAYER_INCLUDE_MEDIA_PLAYER_MEDIA_PLAYER_H_
