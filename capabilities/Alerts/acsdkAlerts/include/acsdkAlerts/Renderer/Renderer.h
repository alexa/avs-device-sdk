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

#ifndef ACSDKALERTS_RENDERER_RENDERER_H_
#define ACSDKALERTS_RENDERER_RENDERER_H_

#include "acsdkAlerts/Renderer/RendererInterface.h"
#include "acsdkAlerts/Renderer/RendererObserverInterface.h"

#include <AVSCommon/SDKInterfaces/SpeakerManagerInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h>
#include <AVSCommon/Utils/MediaPlayer/SourceConfig.h>
#include <AVSCommon/Utils/MediaType.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>

namespace alexaClientSDK {
namespace acsdkAlerts {
namespace renderer {

/**
 * An implementation of an alert renderer.  This class is thread-safe.
 */
class Renderer
        : public RendererInterface
        , public avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface {
public:
    /**
     * Creates a @c Renderer.
     *
     * @param mediaPlayer the @c MediaPlayerInterface that the @c Renderer object will interact with.
     * @param metricRecorder the metric recorder.
     * @return The @c Renderer object.
     */
    static std::shared_ptr<Renderer> create(
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder = nullptr);

    void start(
        std::shared_ptr<RendererObserverInterface> observer,
        std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()> audioFactory,
        bool volumeRampEnabled,
        const std::vector<std::string>& urls = std::vector<std::string>(),
        int loopCount = 0,
        std::chrono::milliseconds loopPause = std::chrono::milliseconds{0},
        bool startWithPause = false) override;

    void stop() override;

    void onFirstByteRead(SourceId sourceId, const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;

    void onPlaybackStarted(SourceId sourceId, const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;

    void onPlaybackStopped(SourceId sourceId, const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;

    void onPlaybackFinished(SourceId sourceId, const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;

    void onPlaybackError(
        SourceId sourceId,
        const avsCommon::utils::mediaPlayer::ErrorType& type,
        std::string error,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;

private:
    /// A type that identifies which source is currently being operated on.
    using SourceId = avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId;

    /**
     * Constructor.
     *
     * @param mediaPlayer The @c MediaPlayerInterface, which will render audio for an alert.
     * @param metricRecorder the metric recorder.
     */
    Renderer(
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder);

    /**
     * @name Executor Thread Functions
     *
     * These functions (and only these functions) are called by @c m_executor on a single worker thread.  All other
     * functions in this class can be called asynchronously, and pass data to the @c Executor thread through parameters
     * to lambda functions.  No additional synchronization is needed.
     */
    /// @{

    /**
     * This function will start rendering audio for the currently active alert.
     *
     * @param observer The observer that will receive renderer events.
     * @param audioFactory A function that produces a pair of stream of audio and audio format that is used for the
     * default if nothing else is available.
     * @param volumeRampEnabled whether this media should ramp
     * @param urls A container of urls to be rendered per the above description.
     * @param loopCount The number of times the urls should be rendered.
     * @param loopPause The duration which must expire between the beginning of rendering of any loop of audio.
     * Therefore if the audio (either urls or local audio file) finishes before this duration, then the renderer will
     * wait for the remainder of this time before starting the next loop.
     * @param startWithPause Indicates if the renderer should begin with an initial pause before rendering audio.
     * This initial pause will be for the same duration as @c loopPause, and will not be considered part of the
     * @c loopCount.
     */
    void executeStart(
        std::shared_ptr<RendererObserverInterface> observer,
        std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()> audioFactory,
        bool volumeRampEnabled,
        const std::vector<std::string>& urls,
        int loopCount,
        std::chrono::milliseconds loopPause,
        bool startWithPause);

    /**
     * This function will stop rendering the currently active alert audio.
     */
    void executeStop();

    /**
     * This function will handle the playbackStarted callback.
     *
     * @param sourceId The sourceId of the media that has started playing.
     */
    void executeOnPlaybackStarted(SourceId sourceId);

    /**
     * This function will handle the playbackStopped callback.
     *
     * @param sourceId The sourceId of the media that has stopped playing.
     */
    void executeOnPlaybackStopped(SourceId sourceId);

    /**
     * This function will handle the playbackFinished callback.
     *
     * @param sourceId The sourceId of the media that has finished playing.
     */
    void executeOnPlaybackFinished(SourceId sourceId);

    /**
     * This function will handle the playbackError callback.
     * @param sourceId The sourceId of the media that encountered the error.
     * @param type The error type.
     * @param error The error string generated by the @c MediaPlayer.
     */
    void executeOnPlaybackError(
        SourceId sourceId,
        const avsCommon::utils::mediaPlayer::ErrorType& type,
        const std::string& error);

    /**
     * Utility function to handle notifying our observer, if there is one.
     *
     * @param state The state to notify the observer of.
     * @param message The optional message to pass to the observer.
     */
    void notifyObserver(RendererObserverInterface::State state, const std::string& message = "");

    /**
     * Utility function to set our internal sourceId to a non-assigned state, which is best encapsulated in
     * a function since it is a bit non-intuitive per MediaPlayer's interface.
     */
    void resetSourceId();

    /**
     * Utility function that checks whether the default audio will be rendered or will be taken from the URL sources
     *
     * @return @c true if default audio will be rendered.  Returns @c false otherwise.
     */
    bool shouldPlayDefault();

    /**
     * Utility function that checks whether the audio source will be played repeatedly by the media player
     *
     * @return @c true if media player will loop the audio source .  Returns @c false otherwise.
     */
    bool shouldMediaPlayerRepeat();

    /**
     * Utility function that checks whether there is an audio source that will be played next
     *
     * @return @c true if there is an audio source to play next.  Returns @c false otherwise.
     */
    bool shouldRenderNext();

    /**
     * Utility function that checks whether to pause after a playback
     *
     * @return @c true if pause is needed after a plaback.  Returns @c false otherwise.
     */
    bool shouldPause();

    /**
     * Utility function that checks whether the last played source is the last one in the loop
     *
     * @return @c true if  last played source is the last one in the loop.  Returns @c false otherwise.
     */
    bool isLastSourceInLoop();

    /**
     * Utility function that checks whether the last played source is the last one to be played
     *
     * @return @c true if  last played source is the last one to be polayed.  Returns @c false otherwise.
     */
    bool isLastSource();

    /**
     * Implements the pause between playback loops
     *
     * @param duration The duration the pause should be for.  Must be a positive value.
     * @return @c true if the pause completed succesfully. Returns @c false if the pause
     * was interrupted by a user action or the duration was less than or equal to 0.
     */
    bool pause(std::chrono::milliseconds duration);

    /**
     * Implements the playback of the audio source
     */
    void play();

    /**
     * Generate the media player configuration for the current media rendering including volume gain.
     *
     * @return The media player configuration for the current media rendering.
     */
    avsCommon::utils::mediaPlayer::SourceConfig generateMediaConfiguration();

    /**
     * Utility function to handle the rendering of the next audio asset, with respect to @c m_remainingLoopCount and @c
     * m_nextUrlIndexToRender.  If all urls within a loop have completed, and there are further loops to render, this
     * function will also perform a sleep for the @c m_loopPause duration.
     *
     * @param pauseInterruptedOut A return value parameter. If there are no more audio assets to render because
     * a pause during the render loop was interrupted, the boolean pointed to will be set to true. Otherwise it
     * will be set to false.
     * @return @c true if there are more audio assets to render, and the next one has been successfully sent to the @c
     * m_mediaPlayer to be played.  Returns @c false otherwise.
     */
    bool renderNextAudioAsset(bool* pauseInterruptedOut = nullptr);

    /**
     * Utility function to handle all aspects of an error occurring.  The sourceId is reset, the observer is notified
     * and the observer is reset.
     *
     * @param error The error string.
     */
    void handlePlaybackError(const std::string& error);

    /// @}

    /**
     * @name Executor Thread Variables
     *
     * These member variables are only accessed by functions in the @c m_executor worker thread, and do not require any
     * synchronization.
     */
    /// @{

    /// The @cMediaPlayerInterface which renders the audio files.
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_mediaPlayer;

    /// The metric recorder.
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> m_metricRecorder;

    /// Our observer.
    std::shared_ptr<RendererObserverInterface> m_observer;

    /// An optional sequence of urls to be rendered.  If the vector is empty, m_localAudioFilePath will be
    /// rendered instead.
    std::vector<std::string> m_urls;

    /// The number of streams that have been rendered during the processing of the current loop.
    int m_numberOfStreamsRenderedThisLoop;

    /// The number of times remaining @c m_urls should be rendered.
    int m_remainingLoopCount;

    /// The number of times @c m_urls should be rendered as specified in the directive
    int m_directiveLoopCount;

    /// The time to pause between the rendering of the @c m_urls sequence.
    std::chrono::milliseconds m_loopPause;

    /// A variable to capture if the renderer should perform an initial pause before rendering audio.
    bool m_shouldPauseBeforeRender;

    /// The timestamp when the current loop began rendering.
    std::chrono::time_point<std::chrono::steady_clock> m_loopStartTime;

    /// A pointer to a pair of stream and stream format factory to use as the default audio to use when the audio assets
    /// aren't available.
    std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()> m_defaultAudioFactory;

    /// A flag to capture if the renderer has been asked to stop by its owner. @c m_waitMutex must be locked when
    /// modifying this variable.
    bool m_isStopping;

    /// A flag to indicate that renderer is going to start playing a new asset once the old one is stopped.
    bool m_isStartPending;

    /// The condition variable used to wait for @c stop() or @c m_loopPause timeouts.
    std::condition_variable m_waitCondition;

    /// The mutex for @c m_waitCondition.
    std::mutex m_waitMutex;

    /// The id associated with the media that our MediaPlayer is currently handling.
    SourceId m_currentSourceId;

    /// Whether the volume ramp property was enabled when the media started playing.
    bool m_volumeRampEnabled;

    /// @}

    /**
     * The @c Executor which queues up operations from asynchronous API calls.
     *
     * @note This declaration needs to come *after* the Executor Thread Variables above so that the thread shuts down
     *     before the Executor Thread Variables are destroyed.
     */
    avsCommon::utils::threading::Executor m_executor;

    /// The time that the alert started rendering.
    std::chrono::steady_clock::time_point m_renderStartTime;
};

}  // namespace renderer
}  // namespace acsdkAlerts
}  // namespace alexaClientSDK

#endif  // ACSDKALERTS_RENDERER_RENDERER_H_
