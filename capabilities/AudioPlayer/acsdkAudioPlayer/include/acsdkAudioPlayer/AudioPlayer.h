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

#ifndef ALEXA_CLIENT_SDK_ACSDKAUDIOPLAYER_INCLUDE_ACSDKAUDIOPLAYER_AUDIOPLAYER_H_
#define ALEXA_CLIENT_SDK_ACSDKAUDIOPLAYER_INCLUDE_ACSDKAUDIOPLAYER_AUDIOPLAYER_H_

#include <deque>
#include <list>
#include <memory>

#include <acsdkAudioPlayerInterfaces/AudioPlayerInterface.h>
#include <acsdk/CryptoInterfaces/CryptoFactoryInterface.h>
#include <acsdkManufactory/Annotated.h>
#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>
#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/PlayBehavior.h>
#include <AVSCommon/AVS/PlayerActivity.h>
#include <AVSCommon/AVS/PlayRequestor.h>
#include <AVSCommon/SDKInterfaces/Audio/MixingBehavior.h>
#include <AVSCommon/SDKInterfaces/AudioFocusAnnotation.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ChannelVolumeInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/SDKInterfaces/LocalPlaybackHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/PlaybackRouterInterface.h>
#include <AVSCommon/SDKInterfaces/RenderPlayerInfoCardsProviderInterface.h>
#include <AVSCommon/SDKInterfaces/RenderPlayerInfoCardsProviderRegistrarInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/DefaultEndpointAnnotation.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointCapabilitiesRegistrarInterface.h>
#include <AVSCommon/Utils/MediaPlayer/ErrorTypes.h>
#include <AVSCommon/Utils/MediaPlayer/PooledMediaResourceProviderInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerFactoryObserverInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h>
#include <AVSCommon/Utils/Metrics/DataPointDurationBuilder.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/Timing/Timer.h>
#include <AVSCommon/Utils/Timing/TimeUtils.h>
#include <Captions/CaptionManagerInterface.h>

#include "AudioItem.h"
#include "ClearBehavior.h"
#include "ProgressTimer.h"

namespace alexaClientSDK {
namespace acsdkAudioPlayer {

/// Splitting AudioPlayer internal state from the external facing PlayerActivity
/// the change here is trivial, but the sematics of BUFFERING vs BUFFER_UNDERRUN are slightly different
/// so, this was a recommended path from the sdk team
enum class AudioPlayerState {
    /// Initial state, prior to acting on the first @c Play directive, or after the current queue is finished
    IDLE,
    /// Indicates that an audio stream is pre-buffering, but is not ready to play.
    BUFFERING,
    /// Indicates that an audio stream under-run has interrupted playback
    /// The difference between BUFFERING and BUFFER_UNDERRUN only affects a couple of behaviors
    BUFFER_UNDERRUN,
    /// Indicates that audio is currently playing.
    PLAYING,
    /**
     * Indicates that audio playback was stopped due to an error or a directive which stops or replaces the current
     * stream.
     */
    STOPPED,
    /// Indicates that the audio stream has been paused.
    PAUSED,
    /// Indicates that playback has finished.
    FINISHED
};

/*
 * Convert a @c AudioPlayerState to @c std::string.
 *
 * @param state The @c AudioPlayerState to convert.
 * @return The string representation of @c AudioPlayerState.
 */
inline std::string playerStateToString(AudioPlayerState state) {
    switch (state) {
        case AudioPlayerState::IDLE:
            return "IDLE";
        case AudioPlayerState::PLAYING:
            return "PLAYING";
        case AudioPlayerState::STOPPED:
            return "STOPPED";
        case AudioPlayerState::PAUSED:
            return "PAUSED";
        case AudioPlayerState::BUFFERING:
            return "BUFFERING";
        case AudioPlayerState::BUFFER_UNDERRUN:
            return "BUFFER_UNDERRUN";
        case AudioPlayerState::FINISHED:
            return "FINISHED";
    }
    return "unknown AudioPlayerState";
}

/**
 * This class implements the @c AudioPlayer capability agent.
 *
 * @see https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/audioplayer
 *
 * @note For instances of this class to be cleaned up correctly, @c shutdown() must be called.
 */
class AudioPlayer
        : public avsCommon::avs::CapabilityAgent
        , public ProgressTimer::ContextInterface
        , public acsdkAudioPlayerInterfaces::AudioPlayerInterface
        , public avsCommon::sdkInterfaces::MediaPropertiesInterface
        , public avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderInterface
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public avsCommon::sdkInterfaces::LocalPlaybackHandlerInterface
        , public avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface
        , public avsCommon::utils::mediaPlayer::MediaPlayerFactoryObserverInterface
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<AudioPlayer> {
public:
    /**
     * Destructor.
     */
    virtual ~AudioPlayer();

    /**
     * Factory method to create a new @c AudioPlayerInterface.
     *
     * @param mediaResourceProvider The instance of the @c PooledMediaResourceProviderInterface used to manage players
     * for playing audio.
     * @param messageSender The object to use for sending events.
     * @param annotatedFocusManager The annotated audio focus manager used to manage usage of the dialog channel.
     * @param contextManager The AVS Context manager used to generate system context for events.
     * @param exceptionSender The object to use for sending AVS Exception messages.
     * @param playbackRouter The @c PlaybackRouterInterface instance to use when @c AudioPlayer becomes active.
     * @param captionManager The @c CaptionManagerInterface instance to use for handling captions.
     * @param metricRecorder The metric recorder.
     * @param shutdownNotifier The object to notify this AudioPlayer when to shut down.
     * @param endpointCapabilitiesRegistrar The object with which to register this AudioPlayer's capabilities for the
     * default endpoint.
     * @param renderPlayerInfoCardsProviderRegistrar The object with which to register this AudioPlayer as a
     * @c RenderPlayerInfoCardsProviderInterface.
     * @param cryptoFactory Encryption facilities factory.
     * @return A shared_ptr to a new @c AudioPlayerInterface.
     */
    static std::shared_ptr<acsdkAudioPlayerInterfaces::AudioPlayerInterface> createAudioPlayerInterface(
        const std::shared_ptr<avsCommon::utils::mediaPlayer::PooledMediaResourceProviderInterface>&
            mediaResourceProvider,
        const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender,
        const acsdkManufactory::Annotated<
            avsCommon::sdkInterfaces::AudioFocusAnnotation,
            avsCommon::sdkInterfaces::FocusManagerInterface>& annotatedFocusManager,
        const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender,
        const std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface>& playbackRouter,
        const std::shared_ptr<captions::CaptionManagerInterface>& captionManager,
        const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder,
        const std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>& shutdownNotifier,
        const acsdkManufactory::Annotated<
            avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
            avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>& endpointCapabilitiesRegistrar,
        const std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderRegistrarInterface>&
            renderPlayerInfoCardsProviderRegistrar,
        const std::shared_ptr<cryptoInterfaces::CryptoFactoryInterface>& cryptoFactory);

    /**
     * Creates a new @c AudioPlayer instance.
     *
     * @param mediaResourceProvider The instance of the @c PooledMediaResourceProviderInterface used to manage players
     * for playing audio.
     * @param messageSender The object to use for sending events.
     * @param focusManager The channel focus manager used to manage usage of the dialog channel.
     * @param contextManager The AVS Context manager used to generate system context for events.
     * @param exceptionSender The object to use for sending AVS Exception messages.
     * @param playbackRouter The @c PlaybackRouterInterface instance to use when @c AudioPlayer becomes active.
     * @param captionManager The optional @c CaptionManagerInterface instance to use for handling captions.
     * @param metricRecorder The metric recorder.
     * @param cryptoFactory Encryption facilities factory.
     * @return A @c std::shared_ptr to the new @c AudioPlayer instance.
     */
    static std::shared_ptr<AudioPlayer> create(
        std::shared_ptr<avsCommon::utils::mediaPlayer::PooledMediaResourceProviderInterface> mediaResourceProvider,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> playbackRouter,
        std::shared_ptr<cryptoInterfaces::CryptoFactoryInterface> cryptoFactory,
        std::shared_ptr<captions::CaptionManagerInterface> captionManager = nullptr,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder = nullptr);

    /// @name StateProviderInterface Functions
    /// @{
    void provideState(const avsCommon::avs::NamespaceAndName& stateProviderName, unsigned int stateRequestToken)
        override;
    /// @}

    /// @name CapabilityAgent/DirectiveHandlerInterface Functions
    /// @{
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<DirectiveInfo> info) override;
    void onDeregistered() override;
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    /// @}

    /// @name ChannelObserverInterface Functions
    /// @{
    void onFocusChanged(avsCommon::avs::FocusState newFocus, avsCommon::avs::MixingBehavior behavior) override;
    /// @}

    /// @name MediaPlayerObserverInterface Functions
    /// @{
    void onFirstByteRead(SourceId id, const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onPlaybackStarted(SourceId id, const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onPlaybackStopped(SourceId id, const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onPlaybackFinished(SourceId id, const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onPlaybackError(
        SourceId id,
        const avsCommon::utils::mediaPlayer::ErrorType& type,
        std::string error,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onPlaybackPaused(SourceId id, const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onPlaybackResumed(SourceId id, const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onBufferUnderrun(SourceId id, const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onBufferRefilled(SourceId id, const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onBufferingComplete(SourceId id, const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onSeeked(
        SourceId id,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& startState,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& endState) override;
    void onTags(
        SourceId id,
        std::unique_ptr<const VectorOfTags> vectorOfTags,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    /// @}

    /// @name MediaPlayerFactoryObserverInterface Functions
    /// @{
    void onReadyToProvideNextPlayer() override;
    /// @}

    /// @name ProgressTimer::ContextInterface methods
    /// @{
    void onProgressReportDelayElapsed() override;
    void onProgressReportIntervalElapsed() override;
    void onProgressReportIntervalUpdated() override;
    void requestProgress() override;
    /// @}

    /// @name AudioPlayerInterface Functions
    /// @{
    void addObserver(std::shared_ptr<acsdkAudioPlayerInterfaces::AudioPlayerObserverInterface> observer) override;
    void removeObserver(std::shared_ptr<acsdkAudioPlayerInterfaces::AudioPlayerObserverInterface> observer) override;
    void stopPlayback() override;
    /// @}

    /// @name RenderPlayerInfoCardsProviderInterface Functions
    /// @{
    void setObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsObserverInterface> observer) override;
    /// @}

    /// @name LocalPlaybackHandlerInterface Functions
    /// @{
    bool localOperation(PlaybackOperation op) override;
    bool localSeekTo(std::chrono::milliseconds location, bool fromStart) override;
    /// @}

    /// @name MediaPropertiesInterface Functions
    /// @{
    std::chrono::milliseconds getAudioItemOffset() override;
    std::chrono::milliseconds getAudioItemDuration() override;
    /// @}

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;
    /// @}

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

private:
    /**
     * A utility class to manage interaction with the MessageSender.
     */
    class MessageRequestObserver : public avsCommon::avs::MessageRequest {
    public:
        /**
         * Constructor.
         *
         * @param metricRecorder The metric recorder.
         * @param jsonContent The JSON text to be sent to AVS.
         * @param uriPathExtension An optional URI path extension of the message to be appended to the base url of the
         * AVS endpoint. If not specified, the default AVS path extension will be used.
         */
        MessageRequestObserver(
            std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
            const std::string& jsonContent,
            const std::string& uriPathExtension = "");

        /// @name MessageRequest functions.
        /// @{
        void sendCompleted(
            avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status sendMessageStatus) override;
        /// @}
    private:
        /// The metric recorder.
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> mMetricRecorder;
    };
    /**
     * This structure contain the necessary objects from the @c PLAY directive that are used for playing the audio.
     */
    struct PlayDirectiveInfo {
        /// MessageId from the @c PLAY directive.
        const std::string messageId;

        /// DialogRequestId from the PLAY directive.
        const std::string dialogRequestId;

        /// MessageId from the @c STOP directive for this item.
        std::string stopMessageId;

        /// This is the @c PlayRequestor object from the @c PLAY directive.
        avsCommon::avs::PlayRequestor playRequestor;

        /// This is the @c AudioItem object from the @c PLAY directive.
        AudioItem audioItem;

        /// This is the @c PlayBehavior from the @c PLAY directive.
        avsCommon::avs::PlayBehavior playBehavior;

        /// Mixing behavior.
        avsCommon::sdkInterfaces::audio::MixingBehavior mixingBehavior;

        /// The @c SourceId from setSource API call.  If the SourceId is not equal to ERROR_SOURCE_ID, it means that
        /// this audioItem has buffered by the @c MediaPlayer.
        SourceId sourceId;

        /// MediaPlayerInterface instance for buffered source
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer;

        /// The initial offset for the currently (or most recent) playing @c AudioItem.
        std::chrono::milliseconds initialOffset;

        /// When buffering items, cache the error message until play request
        std::string errorMsg;

        /// When buffering items, cache the error type until play request
        avsCommon::utils::mediaPlayer::ErrorType errorType;

        /// True if buffered.  We don't want to send a 'nearlyFinished' until after 'started',
        /// so if we get the 'buffer complete' notification before the track is playing, cache the info here
        bool isBuffered;

        /// True if PlaybackNearlyFinished has been sent for this track
        bool isPNFSent;

        /// True if audio normalization should be enabled for this track
        bool normalizationEnabled;

        /// Duration builder for queue time metric
        avsCommon::utils::metrics::DataPointDurationBuilder queueTimeMetricData;

        /// Cached metadata.
        std::shared_ptr<const VectorOfTags> cachedMetadata;

        /// Analyzers data.
        std::vector<avsCommon::utils::audioAnalyzer::AudioAnalyzerState> analyzersData;

        /**
         * Constructor.
         *
         * @param messageId The message Id of the @c PLAY directive.
         * @param dialogRequestId The dialog request Id of the @c PLAY directive.
         */
        PlayDirectiveInfo(const std::string& messageId, const std::string& dialogRequestId);
    };

    /**
     * Constructor.
     *
     * @param mediaResourceProvider The instance of the @c PooledMediaResourceProviderInterface used to manage players
     * for playing audio.
     * @param messageSender The object to use for sending events.
     * @param focusManager The channel focus manager used to manage usage of the dialog channel.
     * @param contextManager The AVS Context manager used to generate system context for events.
     * @param exceptionSender The object to use for sending AVS Exception messages.
     * @param playbackRouter The playback router used for switching playback buttons handler to default.
     * @param audioChannelVolumeInterfaces A list of @c ChannelVolumeInterface instances to use to control/attenuate
     * channel volume. These instances are required for controlling volume for the @c MediaPlayerInterface instances
     * provided by @param mediaResourceProvider.
     * @param captionManager The optional @c CaptionManagerInterface instance to use for handling captions.
     * @param metricRecorder The metric recorder.
     * @param cryptoFactory The optional @c Encryption facilities factory.
     * @return A @c std::shared_ptr to the new @c AudioPlayer instance.
     */
    AudioPlayer(
        std::shared_ptr<avsCommon::utils::mediaPlayer::PooledMediaResourceProviderInterface> mediaResourceProvider,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> playbackRouter,
        std::vector<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>> audioChannelVolumeInterfaces,
        std::shared_ptr<cryptoInterfaces::CryptoFactoryInterface> cryptoFactory,
        std::shared_ptr<captions::CaptionManagerInterface> captionManager = nullptr,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder = nullptr);

    /**
     * This function deserializes a @c Directive's payload into a @c rapidjson::Document.
     *
     * @param info The @c DirectiveInfo to read the payload string from.
     * @param[out] document The @c rapidjson::Document to parse the payload into.
     * @return @c true if parsing was successful, else @c false.
     */
    bool parseDirectivePayload(std::shared_ptr<DirectiveInfo> info, rapidjson::Document* document);

    /**
     * This function pre-handles a @c PLAY directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void preHandlePlayDirective(std::shared_ptr<DirectiveInfo> info);

    /**
     * This function handles a @c PLAY directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void handlePlayDirective(std::shared_ptr<DirectiveInfo> info);

    /**
     * This function handles a @c STOP directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void handleStopDirective(std::shared_ptr<DirectiveInfo> info);

    /**
     * This function handles a @c CLEAR_QUEUE directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void handleClearQueueDirective(std::shared_ptr<DirectiveInfo> info);

    /**
     * This function handles a @c UPDATE_PROGRESS_REPORT_INTERVAL directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void handleUpdateProgressReportIntervalDirective(std::shared_ptr<DirectiveInfo> info);

    /**
     * Remove a directive from the map of message IDs to DirectiveInfo instances.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective whose message ID is to be removed.
     */
    void removeDirective(std::shared_ptr<DirectiveInfo> info);

    /**
     * Send the handling completed notification and clean up the resources the specified @c DirectiveInfo.
     *
     * @param info The @c DirectiveInfo to complete and clean up.
     */
    void setHandlingCompleted(std::shared_ptr<DirectiveInfo> info);

    /**
     * @name Executor Thread Functions
     *
     * These functions (and only these functions) are called by @c m_executor on a single worker thread.  All other
     * functions in this class can be called asynchronously, and pass data to the @c Executor thread through parameters
     * to lambda functions.  No additional synchronization is needed.
     */
    /// @{

    /**
     * This function provides updated context information for @c AudioPlayer to @c ContextManager.  This function is
     * called when @c ContextManager calls @c provideState(), and is also called internally by @c changeState().
     *
     * @param sendToken flag indicating whether @c stateRequestToken contains a valid token which should be passed
     *     along to @c ContextManager.  This flag defaults to @c false.
     * @param stateRequestToken The token @c ContextManager passed to the @c provideState() call, which will be passed
     *     along to the ContextManager::setState() call.  This parameter is not used if @c sendToken is @c false.
     */
    void executeProvideState(bool sendToken = false, unsigned int stateRequestToken = 0);

    /**
     * This function is called when the @c FocusManager focus changes.
     *
     * @li If focus changes to @c FOREGROUND after a @c Play directive requested focus, @c AudioPlayer will start
     *     playing.
     * @li If focus changes to @c BACKGROUND while playing (when another component acquires focus on a higher-priority
     *     channel), @c AudioPlayer will pause playback until it regains @c FOREGROUND focus.
     * @li If focus changes to @c FOREGROUND while paused, @c AudioPlayer will resume playing.
     * @li If focus changes to @c NONE, all playback will be stopped.
     * @li If focus changes to @c NONE, all playback will be stopped.
     *
     * @param newFocus The focus state to change to.
     * @param behavior The mixing behavior to change to.
     */
    void executeOnFocusChanged(avsCommon::avs::FocusState newFocus, avsCommon::avs::MixingBehavior behavior);

    /**
     * Executes onPlaybackStarted callback function
     *
     * @param id The id of the source to which this executed callback corresponds to.
     * @param state Metadata about the media player state
     */
    void executeOnPlaybackStarted(SourceId id, const avsCommon::utils::mediaPlayer::MediaPlayerState& state);

    /**
     * Executes onPlaybackStopped callback function
     *
     * @param id The id of the source to which this executed callback corresponds to.
     * @param state Metadata about the media player state
     */
    void executeOnPlaybackStopped(SourceId id, const avsCommon::utils::mediaPlayer::MediaPlayerState& state);

    /**
     * Executes onPlaybackFinished callback function
     *
     * @param id The id of the source to which this executed callback corresponds to.
     * @param state Metadata about the media player state
     */
    void executeOnPlaybackFinished(SourceId id, const avsCommon::utils::mediaPlayer::MediaPlayerState& state);

    /**
     * Executes onBufferingComplete callback function
     *
     * @param id The id of the source to which this executed callback corresponds to.
     * @param state Metadata about the media player state
     */
    void executeOnBufferingComplete(SourceId id, const avsCommon::utils::mediaPlayer::MediaPlayerState& state);

    /**
     * Executes onSeeked callback function
     *
     * @param id The id of the source to which this executed callback corresponds to.
     * @param startState Metadata about the media player state at the point seek started
     * @param endState Metadata about the media player state at the point the seek completed, or, if stopped / paused,
     * the point playback will be resumed.
     */
    void executeOnSeeked(
        SourceId id,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& startState,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& endState);

    /// Performs necessary cleanup when playback has finished/stopped.
    void handlePlaybackCompleted();

    /**
     * Call an @c m_progressTimer method, keeping track of how many calls to m_progressTimer are in progress.
     *
     * @param call A function that performs the actual call.
     */
    void callProgressTimer(std::function<void()> call);

    /**
     * Record whether @c m_progressTimer is between @c start() and @c stop().
     *
     * @param Whether Whether @c m_progressTimer is between @c start() and @c stop().
     */
    void setIsInProgress(bool isInProgress);

    /**
     * Executes onPlaybackError callback function
     *
     * @param id The id of the source to which this executed callback corresponds to.
     * @param type Error type
     * @param error Error in string format
     * @param state Metadata about the media player state
     */
    void executeOnPlaybackError(
        SourceId id,
        const avsCommon::utils::mediaPlayer::ErrorType& type,
        std::string error,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& state);

    /**
     * Executes onPlaybackPaused callback function
     *
     * @param id The id of the source to which this executed callback corresponds to.
     * @param state Metadata about the media player state
     */
    void executeOnPlaybackPaused(SourceId id, const avsCommon::utils::mediaPlayer::MediaPlayerState& state);

    /**
     * Executes onPlaybackResumed callback function
     *
     * @param id The id of the source to which this executed callback corresponds to.
     * @param state Metadata about the media player state
     */
    void executeOnPlaybackResumed(SourceId id, const avsCommon::utils::mediaPlayer::MediaPlayerState& state);

    /**
     * Executes onBufferUnderrun callback function
     *
     * @param id The id of the source to which this executed callback corresponds to.
     * @param state Metadata about the media player state
     */
    void executeOnBufferUnderrun(SourceId id, const avsCommon::utils::mediaPlayer::MediaPlayerState& state);

    /**
     * Executes onBufferRefilled callback function
     *
     * @param id The id of the source to which this executed callback corresponds to.
     * @param state Metadata about the media player state
     */
    void executeOnBufferRefilled(SourceId id, const avsCommon::utils::mediaPlayer::MediaPlayerState& state);

    /**
     * Executes onTags callback function
     *
     * @param id The id of the source to which this callback corresponds to.
     * @param vectorOfTags The vector containing stream tags.
     * @param state Metadata about the media player state
     */
    void executeOnTags(
        SourceId id,
        std::shared_ptr<const VectorOfTags> vectorOfTags,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& state);

    /**
     * Executes onReadyToProvideNextPlayer callback function
     */
    void executeOnReadyToProvideNextPlayer();

    /**
     * This function executes a parsed @c PLAY directive in pre-handle stage.
     *
     * @param info The @c PlayDirectiveInfo specifying the information from the @c PLAY directive.
     */
    void executePrePlay(std::shared_ptr<PlayDirectiveInfo> info);

    /**
     * This function executes a parsed @c PLAY directive in handle stage.
     *
     * @param messageId The message ID of the @C PLAY directive.
     */
    void executePlay(const std::string& messageId);

    /// This function plays the next @c AudioItem in the queue.
    void playNextItem();

    /**
     * This function stops playback of the current song, and optionally starts the next queued song.
     *
     * @param messageId Set to the directive messageId, If call comes from a Stop Directive.  Used for metrics.
     * @param startNextSong Indicates whether to start playing the next song after stopping the current song.
     */
    void executeStop(const std::string& messageId = "", bool startNextSong = false);

    /**
     * This function executes a parsed @c CLEAR_QUEUE directive.
     *
     * @param clearBehavior Specifies how the queue should be cleared.
     */
    void executeClearQueue(ClearBehavior clearBehavior);

    /**
     * This function executes a parsed @c UPDATE_PROGRESS_REPORT_INTERVAL directive.
     *
     * @param progressReportInterval New progress report interval in milliseconds.
     */
    void executeUpdateProgressReportInterval(std::chrono::milliseconds progressReportInterval);

    /**
     * This function pauses/resumes/stops  playback of the current song via local control
     */
    void executeLocalOperation(PlaybackOperation op, std::promise<bool> success);

    /**
     * This function processes timeout of a local operation
     */
    void executeLocalOperationTimedout();

    /**
     * This function seeks into the current song.
     *
     * @param location Location to seek to
     * @param fromStart true if the location is reletive to the song start, false if reletive to current location
     */
    bool executeLocalSeekTo(std::chrono::milliseconds location, bool fromStart);

    /**
     * Returns the duration of the current song.
     *
     * @return current song duration
     */
    std::chrono::milliseconds getDuration();

    /**
     * This function changes the @c AudioPlayer state.  All state changes are made by calling this function.
     *
     * @param state The state to change to.
     */
    void changeState(AudioPlayerState state);

    /**
     * Most of the @c AudioPlayer events use the same payload, and only vary in their event name.  This utility
     * function constructs and sends these generic @c AudioPlayer events.
     *
     * @param eventName The name of the event to send.
     * @param includePlaybackReports If true, playbackReports are attached, default value is false.
     * @param offset The offset to send.  If this parameter is left with its default (invalid) value, the current
     *     offset from MediaPlayer will be sent.
     */
    void sendEventWithTokenAndOffset(
        const std::string& eventName,
        bool includePlaybackReports = false,
        std::chrono::milliseconds offset = avsCommon::utils::mediaPlayer::MEDIA_PLAYER_INVALID_OFFSET);

    /**
     * Send a @c PlaybackStarted event with given state
     *
     * @param state Metadata about the media player state
     */
    void sendPlaybackStartedEvent(const avsCommon::utils::mediaPlayer::MediaPlayerState& state);

    /**
     * Send a @c PlaybackNearlyFinished event with given state
     *
     * @param state Metadata about the media player state
     */
    void sendPlaybackNearlyFinishedEvent(const avsCommon::utils::mediaPlayer::MediaPlayerState& state);

    /**
     * Send a @c PlaybackStutterStarted event with given state
     *
     * @param state Metadata about the media player state
     */
    void sendPlaybackStutterStartedEvent(const avsCommon::utils::mediaPlayer::MediaPlayerState& state);

    /**
     * Send a @c PlaybackStutterFinished event with given state
     *
     * @param state Metadata about the media player state
     */
    void sendPlaybackStutterFinishedEvent(const avsCommon::utils::mediaPlayer::MediaPlayerState& state);

    /**
     * Send a @c PlaybackFinished event with given state
     *
     * @param state Metadata about the media player state
     */
    void sendPlaybackFinishedEvent(const avsCommon::utils::mediaPlayer::MediaPlayerState& state);

    /**
     * Send a @c PlaybackSeeked event with given state
     *
     * @param startState Metadata about the media player state at the point seek started
     * @param endState Metadata about the media player state at the point the seek completed, or, if stopped / paused,
     * the point playback will be resumed.
     */
    void sendPlaybackSeekedEvent(
        const avsCommon::utils::mediaPlayer::MediaPlayerState& startState,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& endState);

    /**
     * Send a @c PlaybackFailed event with given state
     *
     * @param failingToken The token of the playback item that failed.
     * @param errorType The cause of the failure.
     * @param message A message describing the failure.
     * @param state Metadata about the media player state
     */
    void sendPlaybackFailedEvent(
        const std::string& failingToken,
        avsCommon::utils::mediaPlayer::ErrorType errorType,
        const std::string& message,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& state);

    /**
     * Send a @c PlaybackStop event with given state
     *
     * @param state Metadata about the media player state
     */
    void sendPlaybackStoppedEvent(const avsCommon::utils::mediaPlayer::MediaPlayerState& state);

    /**
     * Send a @c PlaybackPaused event with given state
     *
     * @param state Metadata about the media player state
     */
    void sendPlaybackPausedEvent(const avsCommon::utils::mediaPlayer::MediaPlayerState& state);

    /**
     * Send a @c PlaybackResumed event with given state
     *
     * @param state Metadata about the media player state
     */
    void sendPlaybackResumedEvent(const avsCommon::utils::mediaPlayer::MediaPlayerState& state);

    /// Send a @c PlaybackQueueCleared event.
    void sendPlaybackQueueClearedEvent();

    /**
     * Helper method to send events to AVS.
     *
     * @param eventName The name of the event to be include in the header.
     * @param dialogRequestIdString The value associated with the "dialogRequestId" key.
     * @param payload The payload value associated with the "payload" key.
     * @param context Optional @c context to be sent with the event message.
     */
    void sendEvent(
        const std::string& eventName,
        const std::string& dialogRequestIdString = "",
        const std::string& payload = "{}",
        const std::string& context = "");

    /**
     * Send a @c StreamMetadataExtracted event with given state
     *
     * @param audioItem item associated with the metadata
     * @param vectorOfTags Pointer to vector of tags that should be sent to AVS.
     * @param state Metadata about the media player state
     */
    void sendStreamMetadataExtractedEvent(
        AudioItem& audioItem,
        std::shared_ptr<const VectorOfTags> vectorOfTags,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& state);

    /// Notify AudioPlayerObservers of state changes.
    void notifyObserver();

    /**
     * send AudioPlayerObservers an AudioPlayerObserverInterface::Notification
     *
     * @param notification the type of Notification to send
     */
    void notifySeekActivity(
        const acsdkAudioPlayerInterfaces::AudioPlayerObserverInterface::SeekStatus& seekStatus,
        std::chrono::milliseconds offset);

    /// get an AudioPlayerObserverInterface::Context describing the current player state
    acsdkAudioPlayerInterfaces::AudioPlayerObserverInterface::Context getObserverContext(
        std::chrono::milliseconds offset = std::chrono::milliseconds(-1));

    /**
     * Get the current offset in the audio stream.
     *
     * @note @c MediaPlayer has a getOffset function which only works while actively playing, but AudioPlayer needs to
     *     be able to report its offset at any time, even when paused or stopped.  To address the gap, this function
     *     reports the live offset from @c MediaPlayer when it is playing, and reports a cached offset when
     *     @c MediaPlayer is not playing.
     *
     * @return The current offset in the stream.
     */
    std::chrono::milliseconds getOffset();

    /**
     * Attaches playbackAttributes to payload for AudioPlayer events if available.
     *
     * @param parent The parent to which playbackAttributes to be attached.
     * @param allocator The allocator for document.
     */
    void attachPlaybackAttributesIfAvailable(rapidjson::Value& parent, rapidjson::Document::AllocatorType& allocator);

    /**
     * Attaches playbackReports to payload for AudioPlayer events if available.
     *
     * @param parent The parent to which playbackReports to be attached.
     * @param allocator The allocator for document.
     */
    void attachPlaybackReportsIfAvailable(rapidjson::Value& parent, rapidjson::Document::AllocatorType& allocator);

    /**
     * Get a media player state with the given offset
     *
     * @return Media player state with current offset
     */
    avsCommon::utils::mediaPlayer::MediaPlayerState getMediaPlayerState();

    /**
     * Clears the Play Queue, releasing all players first.
     *
     * @param stopCurrentPlayer Whether or not to stop the current media player
     */
    void clearPlayQueue(const bool stopCurrentPlayer);

    /**
     * Stop and clean-up MediaPlayer information in a PlayDirectiveInfo, and return it to the
     * Factory
     *
     * @param playbackItem PlayDirectiveInfo holding the MediaPlayer
     */
    void stopAndReleaseMediaPlayer(std::shared_ptr<PlayDirectiveInfo> playbackItem);

    /**
     * Clean-up MediaPlayer information in a PlayDirectiveInfo, and return it to the Factory
     *
     * @param playbackItem PlayDirectiveInfo holding the MediaPlayer
     */
    void releaseMediaPlayer(std::shared_ptr<PlayDirectiveInfo> playbackItem);

    /**
     * Acquire player, and set source.  Player and source data stored in PlayDirectiveInfo
     *
     * @param playbackItem PlayDirectiveInfo to contain the MediaPlayer
     * @returns true if successful, false for error
     */
    bool configureMediaPlayer(std::shared_ptr<PlayDirectiveInfo>& playbackItem);

    /**
     * Returns true if the message is in the play queue.
     *
     * @param messageId id of message to search for
     * @return true if in play queue.
     */
    bool isMessageInQueue(const std::string& messageId);

    /**
     * Unduck the channel volume of the underlying @c ChannelVolumeInterface and restore the volume
     * to the earlier unducked volume.
     *
     * @return true if the operation succeeded, else false.
     */
    bool executeStopDucking();

    /**
     * Duck the channel volume of the underlying @c ChannelVolumeInterface and attenuate the channel
     * volume as per the configured volume curve in @c ChannelVolumeManager
     *
     * @return true if the operation succeeded, else false.
     */
    bool executeStartDucking();

    /**
     * Parse Http Headers from the play directive.
     *
     * @param httpHeaders JSON content with httpHeaders
     * @param[out] audioItem audioItem in which the headers will be saved
     */
    void parseHeadersFromPlayDirective(const rapidjson::Value& httpHeaders, AudioItem* audioItem);

    /**
     * Return track protection name.
     *
     * @param mediaPlayerProtection media player protection
     *
     * @return protection name
     */
    std::string getTrackProtectionName(
        const avsCommon::utils::mediaPlayer::MediaPlayerState::MediaPlayerProtection& mediaPlayerProtection) const;

    /**
     * Return track protection name.
     *
     * @param mediaPlayaerState media player state
     *
     * @return protection name
     */
    std::string getTrackProtectionName(const avsCommon::utils::mediaPlayer::MediaPlayerState& mediaPlayerState) const;

    /**
     * Return displayable track playlist type from the media player state
     *
     * @param state media player state
     *
     * @return playlist type
     */
    std::string getPlaylistType(const avsCommon::utils::mediaPlayer::MediaPlayerState& mediaPlayerState) const;

    /**
     * Return displayable track playlist type
     *
     * @param mediaPlayaerState media player state
     *
     * @return playlist type
     */
    std::string getPlaylistType(const std::string& playlistType) const;

    /**
     * Re-package the cached device context for AudioPlayer to a format compatible with
     * events.
     *
     * @param offsetOverride if valid override the offset in the context with this value
     * @return json string of the context
     */
    std::string packageContextForEvent(
        std::chrono::milliseconds offsetOverride = avsCommon::utils::mediaPlayer::MEDIA_PLAYER_INVALID_OFFSET) const;

    /**
     * Convert from internal state to external activity
     *
     * @param state Internal state value
     * @returns External Activity equivelent
     */
    avsCommon::avs::PlayerActivity activityFromState(AudioPlayerState state) const;

    /**
     * Get hash of the domain name of the passed url
     *
     * @param url url to parse
     * @returns hash of the domain if succeeds, empty string otherwise.
     */
    std::string getDomainNameHash(const std::string& url) const;

    /**
     * @brief Check if active media playback is happening currently.
     *
     * @return true  When we have active playback item and current state is
     *                'PLAYING', 'BUFFERING', 'BUFFER_UNDERRUN' or 'PAUSED'.
     */
    bool isPlaybackActive() const;

    /// This is used to safely access the time utilities.
    avsCommon::utils::timing::TimeUtils m_timeUtils;

    /// PooledMediaResourceProviderInterface instance is used to generate Players used to play tracks as well as
    /// access the other media resources associated with those players.
    std::shared_ptr<avsCommon::utils::mediaPlayer::PooledMediaResourceProviderInterface> m_mediaResourceProvider;

    /// The object to use for sending events.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /// The @c FocusManager used to manage usage of the dialog channel.
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> m_focusManager;

    /// The @c ContextManager that needs to be updated of the state.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c PlaybackRouterInterface instance to use when @c AudioPlayer becomes active.
    std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> m_playbackRouter;

    /// The @c CryptoFactoryInterface instance to be used for crypto facilities.
    std::shared_ptr<cryptoInterfaces::CryptoFactoryInterface> m_cryptoFactory;

    /// The @c CaptionManagerInterface used for handling captions.
    std::shared_ptr<captions::CaptionManagerInterface> m_captionManager;

    /// The metric recorder.
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> m_metricRecorder;

    /**
     * The current state of the @c AudioPlayer.
     *
     * This variable is primarily an Executor Thread Variable (see below) in that it is always written from the
     * executor thread and can be safely read without synchronization from the executor thread.  However, this is not
     * listed as an Executor Thread Variable with the others below because there is one non-executor function which
     * reads from this variable: @c onFocusChanged().
     *
     * Focus change notifications are required to block until the focus change completes, so @c onFocusChanged() blocks
     * waiting for a state change.  This is a read-only operation from outside the executor thread, so it doesn't break
     * thread-safety for reads inside the executor, but it does require that these reads from outside the executor lock
     * @c m_currentStateMutex, and that writes from inside the executor lock @c m_currentStateutex and notify
     * @c m_currentStateConditionVariable..
     */
    AudioPlayerState m_currentState;

    /// Protects writes to @c m_currentState and waiting on @c m_currentStateConditionVariable.
    mutable std::mutex m_currentStateMutex;

    /// Provides notifications of changes to @c m_currentState.
    std::condition_variable m_currentStateConditionVariable;

    /**
     * @name Executor Thread Variables
     *
     * These member variables are only accessed by functions in the @c m_executor worker thread, and do not require any
     * synchronization.
     */
    /// @{

    /// The current focus state of the @c AudioPlayer on the content channel.
    avsCommon::avs::FocusState m_focus;

    /*
     * The queue of @c PlayDirectiveInfo to play.  The @c PlayBehavior is already resolved when items are
     * added to the queue.  This queue is used to find the next @c AudioItem to play when @c playNextItem() is called.
     */
    std::deque<std::shared_ptr<PlayDirectiveInfo>> m_audioPlayQueue;

    /**
     * The PlayDirectiveInfo object containing information about the currently playing audioItem
     */
    std::shared_ptr<PlayDirectiveInfo> m_currentlyPlaying;

    /**
     * The PlayDirectiveInfo object containing information about an item being prepared to play next. Only present
     * between preHandle and playback.
     */
    std::shared_ptr<PlayDirectiveInfo> m_itemPendingPlaybackStart;

    /// When in the @c BUFFER_UNDERRUN state, this records the time at which the state was entered.
    std::chrono::steady_clock::time_point m_bufferUnderrunTimestamp;

    /// Drives periodically reporting playback progress.
    ProgressTimer m_progressTimer;

    /**
     * This keeps track of the current offset in the audio stream.  Reading the offset from @c MediaPlayer is
     * insufficient because @c MediaPlayer only returns a valid offset when it is actively playing, but @c AudioPlayer
     * must return a valid offset when @c MediaPlayer is stopped.
     */
    std::chrono::milliseconds m_offset;

    /// A set of observers to be notified when there's a change in the audio state.
    std::unordered_set<std::shared_ptr<acsdkAudioPlayerInterfaces::AudioPlayerObserverInterface>> m_observers;

    /// Observer for changes related to RenderPlayerInfoCards.
    std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsObserverInterface> m_renderPlayerObserver;

    /**
     * A flag which is set when calling @c MediaPlayerInterface::stop(), and used by @c onPlaybackStopped() to decide
     * whether to continue on to the next queued item.
     */
    bool m_playNextItemAfterStopped;

    /**
     * A flag which is set when calling @c MediaPlayerInterface::stop(), and cleared in @c executeOnPlaybackStopped().
     * This flag is used to tell if the @c AudioPlayer is in the process of stopping playback.
     */
    bool m_isStopCalled;

    /**
     * A flag used to indicated the window in which it is OK to send a playbackNearlyFinished event
     */
    bool m_okToRequestNextTrack;

    /**
     * Current Track's protection information. Track Protection information is available at the time of playback
     * start or playback error. It doesn't change once we have it. Therefore, we pick up the
     * mediaprotection on playback started or on error.
     */
    avsCommon::utils::mediaPlayer::MediaPlayerState::MediaPlayerProtection m_currentMediaPlayerProtection;

    /**
     * Current PlaylistType information fetched from MediaPlayerState
     */
    std::string m_currentPlaylistType;
    /// @}

    /**
     * A flag which is set when calling @c FocusManager::acquireChannel() in executePlay() and cleared in
     * executeOnFocusChanged().
     * This flag is used to tell if the @c AudioPlayer is in the process of requesting a Channel already
     * if so : no duplicate request to acquire the channel should be made , while the above request is still pending
     */
    bool m_isAcquireChannelRequestPending;

    /// @}

    /// Set of capability configurations that will get published using the Capabilities API
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> m_capabilityConfigurations;

    /// Current ContentType Rendering in the AudioPlayer
    avsCommon::avs::ContentType m_currentMixability;

    /// Current MixingBehavior for the AudioPlayer.
    avsCommon::avs::MixingBehavior m_mixingBehavior;

    /// Duration builder for Playback Time metric
    avsCommon::utils::metrics::DataPointDurationBuilder m_playbackTimeMetricData;

    /// Duration builder for Autoprogress metric
    avsCommon::utils::metrics::DataPointDurationBuilder m_autoProgressTimeMetricData;

    /// Duration builder for "directiveReceiveToPlaying" metric
    avsCommon::utils::metrics::DataPointDurationBuilder m_playCommandToPlayingTimeMetricData;

    /// Flag that we are recording a timer for directiveReceiveToPlaying
    bool m_isRecordingTimeToPlayback;

    /// Flag Autoprogression started
    bool m_isAutoProgressing;

    /// Local resume, waiting for focus
    bool m_isLocalResumePending;

    /// Promise for local resume success
    std::promise<bool> m_localResumeSuccess;

    /// Set to true if next item mediaPlayer->play() called, but response not received yet
    bool m_isStartingPlayback;

    /// Set to true if next item mediaPlayer->pause() called, but response not received yet
    bool m_isPausingPlayback;

    /// @c ChannelVolumeInterface instance to do volume adjustments with.
    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>> m_audioChannelVolumeInterfaces;

    /// Cached vopy of the device context set
    std::string m_cachedContext;

    /**
     * @c Executor which queues up operations from asynchronous API calls.
     *
     * @note This declaration needs to come *after* the Executor Thread Variables above so that the thread shuts down
     *     before the Executor Thread Variables are destroyed.
     */
    avsCommon::utils::threading::Executor m_executor;

    /**
     * The last dialogRequestId received. If a directive does not have a new DialogRequestId, assume it was sent as part
     * of the same flow that triggered the previous dialog request id.
     */
    std::string m_lastDialogRequestId;
};

}  // namespace acsdkAudioPlayer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACSDKAUDIOPLAYER_INCLUDE_ACSDKAUDIOPLAYER_AUDIOPLAYER_H_
