/*
 * Copyright 2017-2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AUDIOPLAYER_INCLUDE_AUDIOPLAYER_AUDIOPLAYER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AUDIOPLAYER_INCLUDE_AUDIOPLAYER_AUDIOPLAYER_H_

#include <deque>
#include <list>
#include <memory>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/PlayBehavior.h>
#include <AVSCommon/AVS/PlayerActivity.h>
#include <AVSCommon/SDKInterfaces/Audio/MixingBehavior.h>
#include <AVSCommon/SDKInterfaces/AudioPlayerInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/PlaybackRouterInterface.h>
#include <AVSCommon/SDKInterfaces/RenderPlayerInfoCardsProviderInterface.h>
#include <AVSCommon/Utils/MediaPlayer/ErrorTypes.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerFactoryInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerFactoryObserverInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/Timing/Timer.h>
#include <AVSCommon/Utils/Timing/TimeUtils.h>
#include <Captions/CaptionManagerInterface.h>

#include "AudioItem.h"
#include "ClearBehavior.h"
#include "ProgressTimer.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace audioPlayer {

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
        , public avsCommon::sdkInterfaces::AudioPlayerInterface
        , public avsCommon::sdkInterfaces::MediaPropertiesInterface
        , public avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderInterface
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface
        , public avsCommon::utils::mediaPlayer::MediaPlayerFactoryObserverInterface
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<AudioPlayer> {
public:
    /**
     * Destructor.
     */
    virtual ~AudioPlayer() = default;

    /**
     * Creates a new @c AudioPlayer instance.
     *
     * @param mediaPlayerFactory The instance of the @c MediaPlayerFactoryInterface used to manage players for playing
     * audio.
     * @param messageSender The object to use for sending events.
     * @param focusManager The channel focus manager used to manage usage of the dialog channel.
     * @param contextManager The AVS Context manager used to generate system context for events.
     * @param exceptionSender The object to use for sending AVS Exception messages.
     * @param playbackRouter The @c PlaybackRouterInterface instance to use when @c AudioPlayer becomes active.
     * @param captionManager The optional @c CaptionManagerInterface instance to use for handling captions.
     * @return A @c std::shared_ptr to the new @c AudioPlayer instance.
     */
    static std::shared_ptr<AudioPlayer> create(
        std::unique_ptr<avsCommon::utils::mediaPlayer::MediaPlayerFactoryInterface> mediaPlayerFactory,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> playbackRouter,
        std::shared_ptr<captions::CaptionManagerInterface> captionManager = nullptr);

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
    void requestProgress() override;
    /// @}

    /// @name AudioPlayerInterface Functions
    /// @{
    void addObserver(std::shared_ptr<avsCommon::sdkInterfaces::AudioPlayerObserverInterface> observer) override;
    void removeObserver(std::shared_ptr<avsCommon::sdkInterfaces::AudioPlayerObserverInterface> observer) override;
    /// @}

    /// @name RenderPlayerInfoCardsProviderInterface Functions
    /// @{
    void setObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsObserverInterface> observer) override;
    /// @}

    /// @name MediaPropertiesInterface Functions
    /// @{
    std::chrono::milliseconds getAudioItemOffset() override;
    /// @}

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;
    /// @}

private:
    /**
     * This structure contain the necessary objects from the @c PLAY directive that are used for playing the audio.
     */
    struct PlayDirectiveInfo {
        /// MessageId from the @c PLAY directive.
        const std::string messageId;

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

        /**
         * Constructor.
         *
         * @param messageId The message ID of the @c PLAY directive.
         */
        PlayDirectiveInfo(const std::string& messageId);
    };

    /**
     * Constructor.
     *
     * @param mediaPlayerFactory The instance of the @c MediaPlayerFactoryInterface used to manage players for playing
     * audio.
     * @param messageSender The object to use for sending events.
     * @param focusManager The channel focus manager used to manage usage of the dialog channel.
     * @param contextManager The AVS Context manager used to generate system context for events.
     * @param exceptionSender The object to use for sending AVS Exception messages.
     * @param playbackRouter The playback router used for switching playback buttons handler to default.
     * @param captionManager The optional @c CaptionManagerInterface instance to use for handling captions.
     * @return A @c std::shared_ptr to the new @c AudioPlayer instance.
     */
    AudioPlayer(
        std::unique_ptr<avsCommon::utils::mediaPlayer::MediaPlayerFactoryInterface> mediaPlayerFactory,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> playbackRouter,
        std::shared_ptr<captions::CaptionManagerInterface> captionManager = nullptr);

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

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
     * called when @c ContextManager calls @c provideState(), and is also called internally by @c changeActivity().
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
     * @param startNextSong Indicates whether to start playing the next song after stopping the current song.
     */
    void executeStop(bool startNextSong = false);

    /**
     * This function executes a parsed @c CLEAR_QUEUE directive.
     *
     * @param clearBehavior Specifies how the queue should be cleared.
     */
    void executeClearQueue(ClearBehavior clearBehavior);

    /**
     * This function changes the @c AudioPlayer state.  All state changes are made by calling this function.
     *
     * @param activity The state to change to.
     */
    void changeActivity(avsCommon::avs::PlayerActivity activity);

    /**
     * Most of the @c AudioPlayer events use the same payload, and only vary in their event name.  This utility
     * function constructs and sends these generic @c AudioPlayer events.
     *
     * @param eventName The name of the event to send.
     * @param offset The offset to send.  If this parameter is left with its default (invalid) value, the current
     *     offset from MediaPlayer will be sent.
     */
    void sendEventWithTokenAndOffset(
        const std::string& eventName,
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
    ;
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

    bool isMessageInQueue(const std::string& messageId);
    /// @}

    /// This is used to safely access the time utilities.
    avsCommon::utils::timing::TimeUtils m_timeUtils;

    /// MediaPlayerFactoryInterface instance is used to generate Players used to play tracks.
    std::unique_ptr<avsCommon::utils::mediaPlayer::MediaPlayerFactoryInterface> m_mediaPlayerFactory;

    /// The object to use for sending events.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /// The @c FocusManager used to manage usage of the dialog channel.
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> m_focusManager;

    /// The @c ContextManager that needs to be updated of the state.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c PlaybackRouterInterface instance to use when @c AudioPlayer becomes active.
    std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> m_playbackRouter;

    /// The @c CaptionManagerInterface used for handling captions.
    std::shared_ptr<captions::CaptionManagerInterface> m_captionManager;

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
     * @c m_currentActivityMutex, and that writes from inside the executor lock @c m_currentActivityMutex and notify
     * @c m_currentActivityConditionVariable..
     */
    avsCommon::avs::PlayerActivity m_currentActivity;

    /// Protects writes to @c m_currentActivity and waiting on @c m_currentActivityConditionVariable.
    std::mutex m_currentActivityMutex;

    /// Provides notifications of changes to @c m_currentActivity.
    std::condition_variable m_currentActivityConditionVariable;

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
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::AudioPlayerObserverInterface>> m_observers;

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
    /// @}

    /// Set of capability configurations that will get published using the Capabilities API
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> m_capabilityConfigurations;

    /// Current ContentType Rendering in the AudioPlayer
    avsCommon::avs::ContentType m_currentMixability;

    /// Current MixingBehavior for the AudioPlayer.
    avsCommon::avs::MixingBehavior m_mixingBehavior;

    /**
     * @c Executor which queues up operations from asynchronous API calls.
     *
     * @note This declaration needs to come *after* the Executor Thread Variables above so that the thread shuts down
     *     before the Executor Thread Variables are destroyed.
     */
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace audioPlayer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AUDIOPLAYER_INCLUDE_AUDIOPLAYER_AUDIOPLAYER_H_
