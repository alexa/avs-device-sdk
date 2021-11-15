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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEECHSYNTHESIZER_INCLUDE_SPEECHSYNTHESIZER_SPEECHSYNTHESIZER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEECHSYNTHESIZER_INCLUDE_SPEECHSYNTHESIZER_SPEECHSYNTHESIZER_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <deque>

#include <acsdkApplicationAudioPipelineFactoryInterfaces/ApplicationAudioPipelineFactoryInterface.h>
#include <AVSCommon/AVS/AVSDirective.h>
#include <AVSCommon/AVS/PlayBehavior.h>
#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/AVS/DialogUXStateAggregator.h>
#include <AVSCommon/SDKInterfaces/SpeechSynthesizerObserverInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveSequencerInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/PowerResourceManagerInterface.h>
#include <AVSCommon/Utils/AudioAnalyzer/AudioAnalyzerState.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h>
#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <Captions/CaptionData.h>
#include <Captions/CaptionManagerInterface.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace speechSynthesizer {

/// String to identify the speak media player to render audio.
static const constexpr char* SPEAK_MEDIA_PLAYER_NAME = "SpeakMediaPlayer";

/**
 * This class implements the SpeechSynthesizer capability agent.
 * @see https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/speechsynthesizer
 */
class SpeechSynthesizer
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::sdkInterfaces::DialogUXStateObserverInterface
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<SpeechSynthesizer> {
public:
    /// Alias to the @c SpeechSynthesizerObserverInterface for brevity.
    using SpeechSynthesizerObserverInterface = avsCommon::sdkInterfaces::SpeechSynthesizerObserverInterface;

    /**
     * Create a new @c SpeechSynthesizer instance.
     *
     * @param audioPipelineFactory The instance of @c ApplicationAudioPipelineFactoryInterface to create the speak
     * media player and related interfaces.
     * @param messageSender The instance of the @c MessageSenderInterface used to send events to AVS.
     * @param focusManager The instance of the @c FocusManagerInterface used to acquire focus of a channel.
     * @param contextManager The instance of the @c ContextObserverInterface to use to set the context
     * of the @c SpeechSynthesizer.
     * @param metricRecorder The instance of the @c MetricRecorderInterface used to record metrics
     * @param exceptionSender The instance of the @c ExceptionEncounteredSenderInterface to use to notify AVS
     * when a directive cannot be processed.
     * @param captionManager The optional @c CaptionManagerInterface instance to use for handling captions.
     * @param powerResourceManager Power Resource Manager.
     *
     * @return Returns a new @c SpeechSynthesizer, or @c nullptr if the operation failed.
     */
    static std::shared_ptr<SpeechSynthesizer> createSpeechSynthesizer(
        std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>
            audioPipelineFactory,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
        std::shared_ptr<avsCommon::avs::DialogUXStateAggregator> dialogUXStateAggregator,
        std::shared_ptr<captions::CaptionManagerInterface> captionManager = nullptr,
        std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface> powerResourceManager = nullptr);

    /**
     * Create a new @c SpeechSynthesizer instance.
     *
     * @param mediaPlayer The instance of the @c MediaPlayerInterface used to play audio.
     * @param messageSender The instance of the @c MessageSenderInterface used to send events to AVS.
     * @param focusManager The instance of the @c FocusManagerInterface used to acquire focus of a channel.
     * @param contextManager The instance of the @c ContextObserverInterface to use to set the context
     * of the @c SpeechSynthesizer.
     * @param metricRecorder The instance of the @c MetricRecorderInterface used to record metrics
     * @param exceptionSender The instance of the @c ExceptionEncounteredSenderInterface to use to notify AVS
     * when a directive cannot be processed.
     * @param captionManager The optional @c CaptionManagerInterface instance to use for handling captions.
     * @param powerResourceManager Power Resource Manager.
     *
     * @return Returns a new @c SpeechSynthesizer, or @c nullptr if the operation failed.
     */
    static std::shared_ptr<SpeechSynthesizer> create(
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
        std::shared_ptr<avsCommon::avs::DialogUXStateAggregator> dialogUXStateAggregator,
        std::shared_ptr<captions::CaptionManagerInterface> captionManager = nullptr,
        std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface> powerResourceManager = nullptr);

    void onDialogUXStateChanged(DialogUXState newState) override;

    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;

    /**
     * Add an observer to the SpeechSynthesizer.
     *
     * @param observer The observer to add.
     */
    void addObserver(std::shared_ptr<SpeechSynthesizerObserverInterface> observer);

    /**
     * Remove an observer from the SpeechSynthesizer.
     *
     * @note This is a synchronous call which can not be made by an observer callback.  Attempting to call
     *     @c removeObserver() from @c SpeechSynthesizerObserverInterface::onStateChanged() will result in a deadlock.
     *
     * @param observer The observer to remove.
     */
    void removeObserver(std::shared_ptr<SpeechSynthesizerObserverInterface> observer);

    void onDeregistered() override;

    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;

    void preHandleDirective(std::shared_ptr<DirectiveInfo> info) override;

    void handleDirective(std::shared_ptr<DirectiveInfo> info) override;

    void cancelDirective(std::shared_ptr<DirectiveInfo> info) override;

    /// @name Overridden ChannelObserverInterface methods.
    /// @{
    void onFocusChanged(avsCommon::avs::FocusState newFocus, avsCommon::avs::MixingBehavior behavior) override;
    /// @}

    void provideState(const avsCommon::avs::NamespaceAndName& stateProviderName, const unsigned int stateRequestToken)
        override;

    void onContextAvailable(const std::string& jsonContext) override;

    void onContextFailure(const avsCommon::sdkInterfaces::ContextRequestError error) override;

    /// @name Overridden MediaPlayerObserverInterface methods.
    /// @{
    void onFirstByteRead(SourceId id, const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onPlaybackStarted(SourceId id, const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onPlaybackFinished(SourceId id, const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onPlaybackError(
        SourceId id,
        const avsCommon::utils::mediaPlayer::ErrorType& type,
        std::string error,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onPlaybackStopped(SourceId id, const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onBufferUnderrun(SourceId id, const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    /// @}

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;
    /// @}

private:
    /**
     * This class has all the data that is needed to process @c Speak directives.
     */
    struct SpeakDirectiveInfo {
        /**
         * Constructor.
         *
         * @param directiveInfo The @c DirectiveInfo.
         */
        SpeakDirectiveInfo(std::shared_ptr<DirectiveInfo> directiveInfo);

        /// Release Speak specific resources.
        void clear();

        /// @c AVSDirective that is passed during preHandle.
        std::shared_ptr<avsCommon::avs::AVSDirective> directive;

        /// @c DirectiveHandlerResultInterface.
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerResultInterface> result;

        /// The token for this Speak directive.
        std::string token;

        /// The @c AttachmentReader from which to read speech audio.
        std::unique_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader;

        /// A flag to indicate if an event needs to be sent to AVS on playback started.
        bool sendPlaybackStartedMessage;

        /// A flag to indicate if an event needs to be sent to AVS on playback finished / interrupted.
        bool sendPlaybackFinishedMessage;

        /// A flag to indicate if the directive complete message has to be sent to the @c DirectiveSequencer.
        bool sendCompletedMessage;

        /// A flag to indicate if setFailed() has been sent to the @c DirectiveSequencer.
        bool isSetFailedCalled;

        /// A flag to indicate if playback has been initiated.
        bool isPlaybackInitiated;

        /// A flag to indicate the directive has been handled.
        bool isHandled;

        /// A flag to indicate cancel has been initiated.
        bool isCancelInitiated;

        /// The play behavior of this directive.
        avsCommon::avs::PlayBehavior playBehavior;

        /// The caption content that goes with the speech.
        captions::CaptionData captionData;

        /// The audio analyzer states for the speech.
        std::vector<avsCommon::utils::audioAnalyzer::AudioAnalyzerState> analyzersData;
    };

    /**
     * Constructor
     *
     * @param mediaPlayer The instance of the @c MediaPlayerInterface used to play audio.
     * @param messageSender The instance of the @c MessageSenderInterface used to send events to AVS.
     * @param focusManager The instance of the @c FocusManagerInterface used to acquire focus of a channel.
     * @param contextManager The instance of the @c ContextObserverInterface to use to set the context
     * of the SpeechSynthesizer.
     * @param metricRecorder The instance of the @c MetricRecorderInterface used to record metrics
     * @param exceptionSender The instance of the @c ExceptionEncounteredSenderInterface to use to notify AVS
     * when a directive cannot be processed.
     * @param captionManager The optional @c CaptionManagerInterface instance to use for handling captions.
     * @param powerResourceManager Power Resource Manager.
     */
    SpeechSynthesizer(
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<captions::CaptionManagerInterface> captionManager = nullptr,
        std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface> powerResourceManager = nullptr);

    void doShutdown() override;

    /**
     * Initializes the @c SpeechSynthesizer.
     * Adds the @c SpeechSynthesizer as an observer of the speech player.
     * Adds the @c SpeechSynthesizer as a state provider with the @c ContextManager.
     */
    void init();

    /**
     * Handle a SpeechSynthesizer.Speak directive (on the @c m_executor thread) immediately. Starts playing the speech
     * associated with a directive.
     *
     * @param info The directive to handle immediately. The result object is a @c nullptr.
     */
    void executeHandleImmediately(std::shared_ptr<DirectiveInfo> info);

    /**
     * Pre-handle a SpeechSynthesizer.Speak directive (on the @c m_executor thread). Starts any caching of
     * attachment data for playing the speech associated with the directive. This needs to be called after
     * @c DirectiveInfo, the @c AVSDirective and @c DirectiveHandlerResultInterface have been checked to be valid.
     * @c DirectiveHandlerResultInterface is not needed for @c executeHandleImmediately.
     *
     * @param speakInfo The directive to pre-handle and the associated data.
     */
    void executePreHandleAfterValidation(std::shared_ptr<SpeakDirectiveInfo> speakInfo);

    /**
     * Handle a SpeechSynthesizer.Speak directive (on the @c m_executor thread).  This starts a request for
     * the foreground focus. This needs to be called after @c DirectiveInfo, the @c AVSDirective and
     * @c DirectiveHandlerResultInterface have been checked to be valid. @c DirectiveHandlerResultInterface is not
     * needed for @c executeHandleImmediately.
     *
     * @param speakInfo The directive to handle and the associated data.
     */
    void executeHandleAfterValidation(std::shared_ptr<SpeakDirectiveInfo> speakInfo);

    /**
     * Pre-handle a SpeechSynthesizer.Speak directive (on the @c m_executor thread). Starts any caching of
     * attachment data for playing the speech associated with the directive.
     *
     * @param info The directive to preHandle and the result object with which to communicate the result.
     */
    void executePreHandle(std::shared_ptr<DirectiveInfo> info);

    /**
     * Handle a SpeechSynthesizer.Speak directive (on the @c m_executor thread).  This starts a request for
     * the foreground focus.
     *
     * @param info The directive to handle and the result object with which to communicate the result.
     */
    void executeHandle(std::shared_ptr<DirectiveInfo> info);

    /**
     * Cancel execution of a SpeechSynthesizer @c Speak directive (on the @c m_executor thread).
     *
     * @param info The directive to cancel.
     */
    void executeCancel(std::shared_ptr<DirectiveInfo> info);

    /**
     * Cancel execution of a SpeechSynthesize @c Speak directive (on the @c m_executor thread).
     * @param speakInfo The speakInfoDirective to cancel.
     * @param internalCancel Flag if cancelled active directive shall be reported as completed.
     */
    void executeCancel(std::shared_ptr<SpeakDirectiveInfo> speakInfo, bool internalCancel);

    /**
     * Execute a change of state (on the @c m_executor thread). If the @c m_desiredState is @c PLAYING, playing the
     * audio of the current directive is started. If the @c m_desiredState is @c FINISHED this method triggers
     * termination of playing the audio.
     *
     * @param newState The target state.
     */
    void executeStateChange(SpeechSynthesizerObserverInterface::SpeechSynthesizerState newState);

    /**
     * Request to provide an update of the SpeechSynthesizer's state to the ContextManager (on the @c m_executor
     * thread).
     *
     * @param stateRequestToken The token to pass through when setting the state.
     */
    void executeProvideStateLocked(const unsigned int& stateRequestToken);

    /**
     * Handle (on the @c m_executor thread) notification that speech playback has started.
     */
    void executePlaybackStarted();

    /**
     * Handle (on the @c m_executor thread) notification that speech playback has finished.
     */
    void executePlaybackFinished();

    /**
     * Handle (on the @c m_executor thread) notification that speech playback has stopped.
     *
     * @param id The id of the source to which this event corresponds to.
     */
    void executePlaybackStopped(SourceId id);

    /**
     * Handle (on the @c m_executor thread) notification that speech playback encountered an error.
     *
     * @param error Text describing the nature of the error.
     */
    void executePlaybackError(const avsCommon::utils::mediaPlayer::ErrorType& type, std::string error);

    /**
     * Submits a metric built using MetricEventBuilder
     * @param metricEventBuilder The @c MetricEventBuilder to build the metric activity name and datapoint.
     */
    void submitMetric(avsCommon::utils::metrics::MetricEventBuilder& metricEventBuilder);

    /**
     * Creates and records an instance entry metric with the given identifiers and metadata.
     * @param segmentId The segmentId corresponding to this metric event.
     * @param name The name of this metric
     * @param metadata Any metadata to be associated with this metric; default is empty
     */
    void submitInstanceEntryMetric(
        const std::string& segmentId,
        const std::string& name,
        const std::map<std::string, std::string>& metadata = {});

    /**
     * This function is called whenever the AVS UX dialog state of the system changes. This function will block
     * processing of other state changes, so any implementation of this should return quickly.
     *
     * @param newState The new dialog specific AVS UX state.
     */
    void executeOnDialogUXStateChanged(
        avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState newState);

    /**
     * Builds the JSON state to be updated in the @c ContextManager.
     *
     * @param token The text to speech token sent in the @c Speak directive.
     * @param offsetInMilliseconds The current offset of text to speech in milliseconds.
     * @return The JSON state string.
     */
    std::string buildState(std::string& token, int64_t offsetInMilliseconds) const;

    /**
     * Notify AVS of a state change.
     *
     * @param eventName The name of the event to send to AVS.
     */
    void sendEvent(const std::string& eventName, const std::string& payload) const;

    /**
     * Builds a JSON payload string part of the event to be sent to AVS.
     *
     * @param token the token sent in the message from AVS.
     * @return The JSON payload string.
     */
    static std::string buildPayload(std::string& token);

    /**
     * Builds a JSON payload string part of the event to be sent to AVS including the playback offset.
     *
     * @param token the token sent in the message from AVS.
     * @param offsetInMilliseconds The current offset of text to speech in milliseconds.
     * @return The JSON payload string.
     */
    static std::string buildPayload(std::string& token, int64_t offsetInMilliseconds);

    /**
     * Start playing Speak directive audio.
     */
    void startPlaying();

    /**
     * Stop playing Speak directive audio.
     */
    void stopPlaying();

    /**
     * Set the current state of the @c SpeechSynthesizer. The method updates the
     * @c ContextManager with the new state and send an event with the updated state to AVS where applicable.
     * @c m_mutex must be acquired before calling this function. All observers will be notified of a new state
     * regardless of @c provideState value.
     *
     * @param newState The new state of the @c SpeechSynthesizer.
     * @param provideState If true ContextManager will be updated with the new state.
     */
    void setCurrentStateLocked(SpeechSynthesizerObserverInterface::SpeechSynthesizerState newState);

    /**
     * Set the desired state the @c SpeechSynthesizer needs to transition to.
     *
     * @param desiredState The target state.
     * @note This method will acquire m_mutex.
     */
    void setDesiredState(SpeechSynthesizerObserverInterface::SpeechSynthesizerState desiredState);

    /**
     * Reset @c m_currentInfo, cleaning up any @c SpeechSynthesizer resources and removing from CapabilityAgent's
     * map of active @c AVSDirectives.
     */
    void resetCurrentInfo();

    /**
     * Send the handling completed notification and clean up the resources of @c m_currentInfo.
     */
    void setHandlingCompleted();

    /**
     * Send the handling failed notification and clean up the resources of @c m_currentInfo.
     *
     * @param description
     */
    void setHandlingFailed(const std::string& description);

    /**
     * Send ExceptionEncountered and report a failure to handle the @c AVSDirective.
     *
     * @param info The @c AVSDirective that encountered the error and ancillary information.
     * @param type The type of Exception that was encountered.
     * @param message The error message to include in the ExceptionEncountered message.
     */
    void sendExceptionEncounteredAndReportFailed(
        std::shared_ptr<SpeakDirectiveInfo> info,
        avsCommon::avs::ExceptionErrorType type,
        const std::string& message);

    /**
     * Send ExceptionEncountered because a required property was not in the @c AVSDirective's payload.
     *
     * @param info The @c AVSDirective that was missing the property.
     * @param missingProperty The name of the missing property.
     */
    void sendExceptionEncounteredAndReportMissingProperty(
        std::shared_ptr<SpeakDirectiveInfo> info,
        const std::string& missingProperty);

    /**
     * Send ExceptionEncountered because a required property was an unexpected type in the @c AVSDirective's payload.
     *
     * @param info The @c AVSDirective that has the unexpected property type.
     * @param unexpectedProperty The name of the property with the unexpected type.
     */
    void sendExceptionEncounteredAndReportUnexpectedPropertyType(
        std::shared_ptr<SpeakDirectiveInfo> info,
        const std::string& unexpectedProperty);

    /**
     * Release the @c FOREGROUND focus (if we have it).
     */
    void releaseForegroundFocus();

    /**
     * Verify a pointer to a well formed @c SpeakDirectiveInfo.
     *
     * @param caller Name of the method making the call, for logging.
     * @param info The @c DirectiveInfo to test.
     * @param checkResult Check if the @c DirectiveHandlerResultInterface is not a @c nullptr in the @c DirectiveInfo.
     * @return A @c SpeakDirectiveInfo if it is well formed, otherwise @c nullptr.
     */
    std::shared_ptr<SpeakDirectiveInfo> validateInfo(
        const std::string& caller,
        std::shared_ptr<DirectiveInfo> info,
        bool checkResult = true);

    /**
     * Find the @c SpeakDirectiveInfo instance (if any) for the specified @c messageId.
     *
     * @param messageId The messageId value to find @c SpeakDirectiveInfo for.
     * @return The @c SpeakDirectiveInfo instance for @c messageId.
     */
    std::shared_ptr<SpeakDirectiveInfo> getSpeakDirectiveInfo(const std::string& messageId);

    /**
     * Checks if the @c messageId is already present in the map. If its not present, adds an entry to the map.
     * @param messageId The @c messageId value to add to the @c m_speakDirectiveInfoMap
     * @param speakDirectiveInfo The @c SpeakDirectiveInfo corresponding to the @c messageId to add.
     * @return @c true if @c messageId to @c SpeakDirectiveInfo mapping was added. @c false if entry already exists
     * for the @c messageId.
     */
    bool setSpeakDirectiveInfo(
        const std::string& messageId,
        std::shared_ptr<SpeechSynthesizer::SpeakDirectiveInfo> speakDirectiveInfo);

    /**
     * Adds the @c SpeakDirectiveInfo to the @c m_speakInfoQueue. If the entry is being added to an empty queue,
     * it initiates @c executeHandle on the @c SpeakDirectiveInfo.
     * @param speakInfo The @c SpeakDirectiveInfo to add to the @c m_speakInfoQueue.
     */
    void addToDirectiveQueue(std::shared_ptr<SpeakDirectiveInfo> speakInfo);

    /**
     * Removes the @c SpeakDirectiveInfo corresponding to the @c messageId from the @c m_speakDirectiveInfoMap.
     *
     * @param messageId The @c messageId to remove.
     */
    void removeSpeakDirectiveInfo(const std::string& messageId);

    /**
     * Reset the @c m_mediaSourceId once the @c MediaPlayer finishes with playback or
     * when @c MediaPlayer returns an error.
     */
    void resetMediaSourceId();

    /**
     * Clear the enqueued directives.
     *
     * @note Make sure you acquired @c m_speakInfoQueueMutex.
     */
    void clearPendingDirectivesLocked();

    /**
     * Acquire power resource when speaking and release power resource when finished speaking.
     * @param newState The new state of SpeechSynthesizer.
     */
    void managePowerResource(SpeechSynthesizerObserverInterface::SpeechSynthesizerState newState);

    /**
     * Id to identify the specific source when making calls to MediaPlayerInterface.
     * If this is modified or retrieved from methods that are not protected by the executor
     * then additional locking will be needed.
     */
    avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId m_mediaSourceId;

    /// The last media player offset reportted. This is used to provide the interrupted state information.
    int64_t m_offsetInMilliseconds;

    /// MediaPlayerInterface instance to send audio attachments to
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_speechPlayer;

    /// MetricRecorder instance to record metrics with
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> m_metricRecorder;

    /// Object used to send events.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /// The @c FocusManager used to acquire the channel.
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> m_focusManager;

    /// The @c ContextManager that needs to be updated of the state.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c CaptionManagerInterface used for handling captions.
    std::shared_ptr<captions::CaptionManagerInterface> m_captionManager;

    /// The set of @c SpeechSynthesizerObserverInterface instances to notify of state changes.
    std::unordered_set<std::shared_ptr<SpeechSynthesizerObserverInterface>> m_observers;

    /**
     * The current state of the @c SpeechSynthesizer. @c m_mutex must be acquired before reading or writing the
     * @c m_currentState.
     */
    SpeechSynthesizerObserverInterface::SpeechSynthesizerState m_currentState;

    /**
     * The state the @c SpeechSynthesizer must transition to. @c m_mutex must be acquired before reading or writing
     * the @c m_desiredState.
     */
    SpeechSynthesizerObserverInterface::SpeechSynthesizerState m_desiredState;

    /// The current focus acquired by the @c SpeechSynthesizer.
    avsCommon::avs::FocusState m_currentFocus;

    /// @c SpeakDirectiveInfo instance for the @c AVSDirective currently being handled.
    /// Serialized by only accessing it from tasks running under m_executor.
    std::shared_ptr<SpeakDirectiveInfo> m_currentInfo;

    /// Mutex to serialize access to m_currentState, m_desiredState, and m_waitOnStateChange.
    std::mutex m_mutex;

    /// Condition variable to wake @c onFocusChanged() once the state transition to desired state is complete.
    std::condition_variable m_waitOnStateChange;

    /// Map of message Id to @c SpeakDirectiveInfo.
    std::unordered_map<std::string, std::shared_ptr<SpeakDirectiveInfo>> m_speakDirectiveInfoMap;

    /**
     * Mutex to protect @c messageId to @c SpeakDirectiveInfo mapping. This mutex must be acquired before accessing
     * or modifying the m_speakDirectiveInfoMap
     */
    std::mutex m_speakDirectiveInfoMutex;

    /// Queue which holds the directives to be processed.  @c m_speakInfoQueueMutex must he acquired when
    /// accessing or modifying this member.
    std::deque<std::shared_ptr<SpeakDirectiveInfo>> m_speakInfoQueue;

    /// Flag indicating if doShutdown() has been called.  @c m_speakInfoQueueMutex must he acquired when
    /// accessing or modifying this member.
    bool m_isShuttingDown;

    /// Serializes access to @c m_speakInfoQueue.
    std::mutex m_speakInfoQueueMutex;

    /// This flag indicates whether the initial dialog UX State has been received.
    bool m_initialDialogUXStateReceived;

    /// Set of capability configurations that will get published using the Capabilities API
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> m_capabilityConfigurations;

    /// A @c PowerResourceId used for wakelock logic.
    std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface::PowerResourceId> m_powerResourceId;

    /// The power resource manager
    std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface> m_powerResourceManager;

    /**
     * @c Executor which queues up operations from asynchronous API calls.
     *
     * @note This declaration needs to come *after* any variables used by the executor thread so that the thread shuts
     *     down before the variables are destroyed.
     */
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace speechSynthesizer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEECHSYNTHESIZER_INCLUDE_SPEECHSYNTHESIZER_SPEECHSYNTHESIZER_H_
