/*
 * AudioPlayer.h
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_AUDIO_PLAYER_INCLUDE_AUDIO_PLAYER_AUDIO_PLAYER_H_
#define ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_AUDIO_PLAYER_INCLUDE_AUDIO_PLAYER_AUDIO_PLAYER_H_

#include <memory>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/Timing/Timer.h>

#include "AudioItem.h"
#include "ClearBehavior.h"
#include "ErrorType.h"
#include "PlayBehavior.h"
#include "PlayerActivity.h"

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
class AudioPlayer :
        public avsCommon::avs::CapabilityAgent,
        public avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface,
        public avsCommon::utils::RequiresShutdown,
        public std::enable_shared_from_this<AudioPlayer> {
public:
    /**
     * Creates a new @c AudioPlayer instance.
     *
     * @param mediaPlayer The instance of the @c MediaPlayerInterface used to play audio.
     * @param messageSender The object to use for sending events.
     * @param focusManager The channel focus manager used to manage usage of the dialog channel.
     * @param contextManager The AVS Context manager used to generate system context for events.
     * @param attachmentManager The instance of the @c AttachmentManagerInterface to use to read the attachment.
     * @param exceptionSender The object to use for sending AVS Exception messages.
     * @return A @c std::shared_ptr to the new @c AudioPlayer instance.
     */
    static std::shared_ptr<AudioPlayer> create(
            std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer,
            std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
            std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
            std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
            std::shared_ptr<avsCommon::avs::attachment::AttachmentManagerInterface> attachmentManager,
            std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender);

    /// @name StateProviderInterface Functions
    /// @{
    void provideState(unsigned int stateRequestToken) override;
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
    void onFocusChanged(avsCommon::avs::FocusState newFocus) override;
    /// @}

    /// @name MediaPlayerObserverInterface Functions
    /// @{
    void onPlaybackStarted() override;
    void onPlaybackFinished() override;
    void onPlaybackError(std::string error) override;
    void onPlaybackPaused() override;
    void onPlaybackResumed() override;
    void onBufferUnderrun() override;
    void onBufferRefilled() override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param mediaPlayer The instance of the @c MediaPlayerInterface used to play audio.
     * @param messageSender The object to use for sending events.
     * @param focusManager The channel focus manager used to manage usage of the dialog channel.
     * @param contextManager The AVS Context manager used to generate system context for events.
     * @param attachmentManager The instance of the @c AttachmentManagerInterface to use to read the attachment.
     * @param exceptionSender The object to use for sending AVS Exception messages.
     * @return A @c std::shared_ptr to the new @c AudioPlayer instance.
     */
    AudioPlayer(
            std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer,
            std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
            std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
            std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
            std::shared_ptr<avsCommon::avs::attachment::AttachmentManagerInterface> attachmentManager,
            std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender);

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
     */
    void executeOnFocusChanged(avsCommon::avs::FocusState newFocus);

    /// Handle notification that audio playback has started.
    void executeOnPlaybackStarted();

    /// Handle notification that audio playback has finished.
    void executeOnPlaybackFinished();

    /**
     * Handle notification that audio playback encountered an error.
     *
     * @param error Text describing the nature of the error.
     */
    void executeOnPlaybackError(std::string error);

    /// Handle notification that audio playback has paused.
    void executeOnPlaybackPaused();

    /// Handle notification that audio playback has resumed after being paused.
    void executeOnPlaybackResumed();

    /// Handle notification that audio playback has run out of data in the audio buffer.
    void executeOnBufferUnderrun();

    /// Handle notification that audio playback has resumed after encountering a buffer underrun.
    void executeOnBufferRefilled();

    /**
     * This function executes a parsed @c PLAY directive.
     *
     * @param playBehavior Specifies how @c audioItem should be queued/played.
     * @param audioItem The new @c AudioItem to play.
     */
    void executePlay(PlayBehavior playBehavior, const AudioItem& audioItem);

    /// This fuction plays the next @c AudioItem in the queue.
    void playNextItem();

    /**
     * This function executes a parsed @c STOP directive.
     *
     * @param releaseFocus Indicates whether this function should release focus on the content channel.
     */
    void executeStop(bool releaseFocus = true);

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
    void changeActivity(PlayerActivity activity);

    /**
     * Send the handling completed notification and clean up the resources of @c m_currentInfo.
     */
    void setHandlingCompleted(std::shared_ptr<DirectiveInfo> info);

    /**
     * Send ExceptionEncountered and report a failure to handle the @c AVSDirective.
     *
     * @param info The @c AVSDirective that encountered the error and ancillary information.
     * @param type The type of Exception that was encountered.
     * @param message The error message to include in the ExceptionEncountered message.
     */
    void sendExceptionEncounteredAndReportFailed(
            std::shared_ptr<DirectiveInfo> info,
            const std::string& message,
            avsCommon::avs::ExceptionErrorType type = avsCommon::avs::ExceptionErrorType::INTERNAL_ERROR);

    /**
     * Most of the @c AudioPlayer events use the same payload, and only vary in their event name.  This utility
     * function constructs and sends these generic @c AudioPlayer events.
     *
     * @param name The name of the event to send.
     */
    void sendEventWithTokenAndOffset(const std::string& eventName);

    /// Send a @c PlaybackStarted event.
    void sendPlaybackStartedEvent();

    /// Send a @c PlaybackNearlyFinished event.
    void sendPlaybackNearlyFinishedEvent();

    /// Send a @c ProgressReportDelayElapsed event.
    void sendProgressReportDelayElapsedEvent();

    /// Send a @c ProgressReportIntervalElapsed event.
    void sendProgressReportIntervalElapsedEvent();

    /// Send a @c PlaybackStutterStarted event.
    void sendPlaybackStutterStartedEvent();

    /// Send a @c PlaybackStutterFinished event.
    void sendPlaybackStutterFinishedEvent();

    /// Send a @c PlaybackFinished event.
    void sendPlaybackFinishedEvent();

    /**
     * Send a @c PlaybackFailed event.
     *
     * @param failingToken The token of the playback item that failed.
     * @param errorType The cause of the failure.
     * @param message A message describing the failure.
     */
    void sendPlaybackFailedEvent(
           const std::string& failingToken,
           ErrorType errorType,
           const std::string& message);

    /// Send a @c PlaybackStopped event.
    void sendPlaybackStoppedEvent();

    /// Send a @c PlaybackPaused event.
    void sendPlaybackPausedEvent();

    /// Send a @c PlaybackResumed event.
    void sendPlaybackResumedEvent();

    /// Send a @c PlaybackQueueCleared event.
    void sendPlaybackQueueClearedEvent();

    /// Send a @c PlaybackMetadataExtracted event.
    void sendStreamMetadataExtractedEvent();

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

    /// @}

    /// MediaPlayerInterface instance to send audio attachments to.
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_mediaPlayer;

    /// The object to use for sending events.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /// The @c FocusManager used to manage usage of the dialog channel.
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> m_focusManager;

    /// The @c ContextManager that needs to be updated of the state.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c AttachmentManager used to read attachments.
    std::shared_ptr<avsCommon::avs::attachment::AttachmentManagerInterface> m_attachmentManager;

    /**
     * @name Playback Synchronization Variables
     *
     * These member variables are used during focus change events to wait for callbacks from @c MediaPlayer.  They are
     * accessed asychronously by the @c MediaPlayerObserverInterface callbacks, as well as by @c m_executor functions.
     * These accesses are synchronized by m_blaybackMutex.
     */
    /// @{    

    /// Flag which is set by @c onPlaybackStarted.
    bool m_playbackStarted;

    /// Flag which is set by @c onPlaybackPaused.
    bool m_playbackPaused;

    /// Flag which is set by @c onPlaybackResumed.
    bool m_playbackResumed;

    /// Flag which is set by @c onPlaybackFinished.
    bool m_playbackFinished;

    /// @}

    /// Mutex to synchronize access to Playback Synchronization Variables.
    std::mutex m_playbackMutex;

    /// Condition variable to signal changes to Playback Synchronization Variables.
    std::condition_variable m_playbackConditionVariable;

    /**
     * @name Executor Thread Variables
     *
     * These member variables are only accessed by functions in the @c m_executor worker thread, and do not require any
     * synchronization.
     */
    /// @{    

    /// The current state of the @c AudioPlayer.
    PlayerActivity m_currentActivity;

    /// Sub-state indicating we're transitioning to @c PLAYING from @c IDLE/STOPPED/FINISHED
    bool m_starting;

    /// The current focus state of the @c AudioPlayer on the content channel.
    avsCommon::avs::FocusState m_focus;

    /// The queue of @c AudioItems to play.
    std::deque<AudioItem> m_audioItems;

    /// The token of the currently (or most recently) playing @c AudioItem.
    std::string m_token;

    /// When in the @c BUFFER_UNDERRUN state, this records the time at which the state was entered.
    std::chrono::steady_clock::time_point m_bufferUnderrunTimestamp;

    /// This timer is used to send @c ProgressReportDelayElapsed events.
    avsCommon::utils::timing::Timer m_delayTimer;

    /// This timer is used to send @c ProgressReportIntervalElapsed events.
    avsCommon::utils::timing::Timer m_intervalTimer;

    /**
     * This keeps track of the current offset in the audio stream.  Reading the offset from @c MediaPlayer is
     * insufficient because @c MediaPlayer only returns a valid offset when it is actively playing, but @c AudioPlayer
     * must return a valid offset when @c MediaPlayer is stopped.
     */
    std::chrono::milliseconds m_offset;

    /// @}

    /**
     * @c Executor which queues up operations from asynchronous API calls.
     *
     * @note This declaration needs to come *after* the Executor Thread Variables above so that the thread shuts down
     *     before the Executor Thread Variables are destroyed.
     */
    avsCommon::utils::threading::Executor m_executor;
};

} // namespace audioPlayer
} // namespace capabilityAgents
} // namespace alexaClientSDK

#endif //ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_AUDIO_PLAYER_INCLUDE_AUDIO_PLAYER_AUDIO_PLAYER_H_
