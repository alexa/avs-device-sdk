/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_MEDIAPLAYER_INCLUDE_MEDIAPLAYER_MEDIAPLAYER_H_
#define ALEXA_CLIENT_SDK_MEDIAPLAYER_INCLUDE_MEDIAPLAYER_MEDIAPLAYER_H_

#include <atomic>
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
#include <gst/base/gstbasesink.h>

#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterfaceFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h>
#include <AVSCommon/Utils/PlaylistParser/PlaylistParserInterface.h>
#include <PlaylistParser/UrlContentToAttachmentConverter.h>

#include "MediaPlayer/OffsetManager.h"
#include "MediaPlayer/PipelineInterface.h"
#include "MediaPlayer/SourceInterface.h"

namespace alexaClientSDK {
namespace mediaPlayer {

typedef std::vector<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface::TagKeyValueType> VectorOfTags;

/**
 * Class that handles creation of audio pipeline and playing of audio data.
 */
class MediaPlayer
        : public avsCommon::utils::RequiresShutdown
        , public avsCommon::utils::mediaPlayer::MediaPlayerInterface
        , public avsCommon::sdkInterfaces::SpeakerInterface
        , private PipelineInterface
        , public playlistParser::UrlContentToAttachmentConverter::ErrorObserverInterface
        , public std::enable_shared_from_this<MediaPlayer> {
public:
    /**
     * Creates an instance of the @c MediaPlayer.
     *
     * @param contentFetcherFactory Used to create objects that can fetch remote HTTP content.
     * @param type The type used to categorize the speaker for volume control.
     * @return An instance of the @c MediaPlayer if successful else a @c nullptr.
     */
    static std::shared_ptr<MediaPlayer> create(
        std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory =
            nullptr,
        avsCommon::sdkInterfaces::SpeakerInterface::Type type =
            avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SYNCED,
        std::string name = "");

    /**
     * Destructor.
     */
    ~MediaPlayer();

    /// @name Overridden MediaPlayerInterface methods.
    /// @{
    SourceId setSource(
        std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader,
        const avsCommon::utils::AudioFormat* format = nullptr) override;
    SourceId setSource(std::shared_ptr<std::istream> stream, bool repeat) override;
    SourceId setSource(const std::string& url, std::chrono::milliseconds offset = std::chrono::milliseconds::zero())
        override;

    bool play(SourceId id) override;
    bool stop(SourceId id) override;
    bool pause(SourceId id) override;
    /**
     * To resume playback after a pause, call @c resume. Calling @c play
     * will reset the pipeline and source, and will not resume playback.
     */
    bool resume(SourceId id) override;
    uint64_t getNumBytesBuffered() override;
    std::chrono::milliseconds getOffset(SourceId id) override;
    void setObserver(std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface> observer) override;
    /// @}

    /// @name Overridden SpeakerInterface methods.
    /// @{
    bool setVolume(int8_t volume) override;
    bool adjustVolume(int8_t volume) override;
    bool setMute(bool mute) override;
    bool getSpeakerSettings(avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings* settings) override;
    avsCommon::sdkInterfaces::SpeakerInterface::Type getSpeakerType() override;
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

    /// @name Overriden UrlContentToAttachmentConverter::ErrorObserverInterface methods.
    /// @{
    void onError() override;
    /// @}

    void doShutdown() override;

private:
    /**
     * The @c AudioPipeline consists of the following elements:
     * @li @c appsrc The appsrc element is used as the source to which audio data is provided.
     * @li @c decoder Decodebin is used as the decoder element to decode audio.
     * @li @c decodedQueue A queue is used to store the decoded data.
     * @li @c converter An audio-converter is used to convert between audio formats.
     * @li @c volume The volume element is used as a volume control.
     * @li @c resampler The optional resampler element is used to convert to a specified format
     * @li @c caps The optional caps element is used to specify the resampler format
     * @li @c audioSink Sink for the audio.
     * @li @c pipeline The pipeline is a bin consisting of the @c appsrc, the @c decoder, the @c converter, and the
     * @c audioSink.
     *
     * The data flow through the elements is appsrc -> decoder -> decodedQueue -> converter -> volume -> audioSink.
     * Ideally we would want to use playsink or playbin directly to automate as much as possible. However, this
     * causes problems with multiple pipelines and volume settings in pulse audio. Pending further investigation.
     */
    struct AudioPipeline {
        /// The source element.
        GstAppSrc* appsrc;

        /// The decoder element.
        GstElement* decoder;

        /// A queue for decoded elements.
        GstElement* decodedQueue;

        /// The converter element.
        GstElement* converter;

        /// The volume element.
        GstElement* volume;

        /// The resampler element.
        GstElement* resample;

        /// The capabilities element.
        GstElement* caps;

        /// The sink element.
        GstElement* audioSink;

        /// Pipeline element.
        GstElement* pipeline;

        /// Constructor.
        AudioPipeline() :
                appsrc{nullptr},
                decoder{nullptr},
                decodedQueue{nullptr},
                converter{nullptr},
                volume{nullptr},
                audioSink{nullptr},
                pipeline{nullptr} {};
    };

    /**
     * Constructor.
     *
     * @param contentFetcherFactory Used to create objects that can fetch remote HTTP content.
     * @param type The type used to categorize the speaker for volume control.
     */
    MediaPlayer(
        std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory,
        avsCommon::sdkInterfaces::SpeakerInterface::Type type,
        std::string name);

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

    /**
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
     * Gather all stream tags found into a vector of tags.
     *
     * @param message The message posted on the bus.
     * @return Vector of stream tags.
     */
    std::unique_ptr<const VectorOfTags> collectTags(GstMessage* message);

    /**
     * Send tags that are found in the stream to the observer.
     *
     * @param vectorOfTags Vector containing tags that are found in the stream.
     */
    void sendStreamTagsToObserver(std::unique_ptr<const VectorOfTags> vectorOfTags);

    /**
     * Worker thread handler for setting the source of audio to play.
     *
     * @param reader The @c AttachmentReader with which to receive the audio to play.
     * @param promise A promise to fulfill with a @c SourceId value once the source has been set.
     * @param audioFormat The audioFormat to be used to interpret raw audio data.
     */
    void handleSetAttachmentReaderSource(
        std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> reader,
        std::promise<SourceId>* promise,
        const avsCommon::utils::AudioFormat* audioFormat = nullptr);

    /**
     * Worker thread handler for setting the source of audio to play.
     *
     * @param url The url to set as the source.
     * @param offset The offset from which to start streaming from.
     * @param promise A promise to fulfill with a @c SourceId value once the source has been set.
     */
    void handleSetUrlSource(const std::string& url, std::chrono::milliseconds offset, std::promise<SourceId>* promise);

    /**
     * Worker thread handler for setting the source of audio to play.
     *
     * @param stream The source from which to receive the audio to play.
     * @param repeat Whether the audio stream should be played in a loop until stopped.
     * @param promise A promise to fulfill with a @ SourceId value once the source has been set.
     */
    void handleSetIStreamSource(std::shared_ptr<std::istream> stream, bool repeat, std::promise<SourceId>* promise);

    /**
     * Internal method to update the volume according to a gstreamer bug fix
     * https://bugzilla.gnome.org/show_bug.cgi?id=793081
     * @param gstVolume a volume to be set to GStreamer
     */
    void handleSetVolumeInternal(gdouble gstVolume);

    /**
     * Worker thread handler for setting the volume.
     *
     * @param promise The promise to be set indicating the success of the operation.
     * @param volume The absolute volume.
     */
    void handleSetVolume(std::promise<bool>* promise, int8_t volume);

    /**
     * Worker thread handler for adjusting the volume.
     *
     * @param promise The promise to be set indicating the success of the operation.
     * @param delta The volume change.
     */
    void handleAdjustVolume(std::promise<bool>* promise, int8_t delta);

    /**
     * Worker thread handler for setting the mute.
     *
     * @param promise The promise to be set indicating the success of the operation.
     * @param mute The mute setting. True for mute and false for unmute.
     */
    void handleSetMute(std::promise<bool>* promise, bool mute);

    /**
     * Worker thread handler for getting the @c SpeakerSettings.
     *
     * @param promise The promise to be set indicating the success of the operation.
     * @param settings The current @c SpeakerSettings.
     */
    void handleGetSpeakerSettings(
        std::promise<bool>* promise,
        avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings* settings);

    /**
     * Worker thread handler for starting playback of the current audio source.
     *
     * @param id The @c SourceId that the caller is expecting to be handled.
     * @param promise A promise to fulfill with a @c bool value once playback has been initiated
     * (or the operation has failed).
     */
    void handlePlay(SourceId id, std::promise<bool>* promise);

    /**
     * Worker thread handler for stopping audio playback.
     *
     * @param id The @c SourceId that the caller is expecting to be handled.
     * @param promise A promise to fulfill with a @c bool once playback stop has been initiated
     * (or the operation has failed).
     */
    void handleStop(SourceId id, std::promise<bool>* promise);

    /**
     * Worker thread handler for pausing playback of the current audio source.
     *
     * @param id The @c SourceId that the caller is expecting to be handled.
     * @param promise A promise to fulfill with a @c bool value once playback has been paused
     * (or the operation has failed).
     */
    void handlePause(SourceId id, std::promise<bool>* promise);

    /**
     * Worker thread handler for resume playback of the current audio source.
     *
     * @param id The @c SourceId that the caller is expecting to be handled.
     * @param promise A promise to fulfill with a @c bool value once playback has been resumed
     * (or the operation has failed).
     */
    void handleResume(SourceId id, std::promise<bool>* promise);

    /**
     * Worker thread handler for getting the current playback position.
     *
     * @param id The @c SourceId that the caller is expecting to be handled.
     * @param promise A promise to fulfill with the offset once the value has been determined.
     */
    void handleGetOffset(SourceId id, std::promise<std::chrono::milliseconds>* promise);

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
     * Sends the playback stopped notification to the observer.
     */
    void sendPlaybackStopped();

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
     * Performs a seek to the @c seekPoint.
     *
     * @return A boolean indicating whether the seek operation was successful.
     */
    bool seek();

    /**
     * Validates that the given id matches the current source id being operated on.
     *
     * @param id The id to validate
     * @return @c true if the id matches the source being operated on and @c false otherwise.
     */
    bool validateSourceAndId(SourceId id);

    /**
     * The callback to be added to the event loop to process upon onError() callback.
     *
     * @param pointer The instance to this @c MediaPlayer.
     * @return @c false if there is no error with this callback, else @c true.
     */
    static gboolean onErrorCallback(gpointer pointer);

    /**
     * Save offset of stream before we teardown the pipeline.
     */
    void saveOffsetBeforeTeardown();

    /**
     * Destructs the @c m_source with proper steps.
     */
    void cleanUpSource();

    /// The volume to restore to when exiting muted state. Used in GStreamer crash fix for zero volume on PCM data.
    gdouble m_lastVolume;

    /// The muted state of the player. Used in GStreamer crash fix for zero volume on PCM data.
    bool m_isMuted;

    /// Used to stream urls into attachments
    std::shared_ptr<playlistParser::UrlContentToAttachmentConverter> m_urlConverter;

    /// An instance of the @c OffsetManager.
    OffsetManager m_offsetManager;

    /// Used to create objects that can fetch remote HTTP content.
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> m_contentFetcherFactory;

    /// An instance of the @c AudioPipeline.
    AudioPipeline m_pipeline;

    /// The Speaker type.
    avsCommon::sdkInterfaces::SpeakerInterface::Type m_speakerType;

    /// Main event loop.
    GMainLoop* m_mainLoop;

    // Main loop thread
    std::thread m_mainLoopThread;

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

    /// The current source id.
    SourceId m_currentId;

    /// Flag to indicate whether a play is currently pending a callback.
    bool m_playPending;

    /// Flag to indicate whether a pause is currently pending a callback.
    bool m_pausePending;

    /// Flag to indicate whether a resume is currently pending a callback.
    bool m_resumePending;

    /// Flag to indicate whether a pause should happen immediately.
    bool m_pauseImmediately;

    /// Stream offset before we teardown the pipeline
    std::chrono::milliseconds m_offsetBeforeTeardown;
};

}  // namespace mediaPlayer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_MEDIAPLAYER_INCLUDE_MEDIAPLAYER_MEDIAPLAYER_H_
