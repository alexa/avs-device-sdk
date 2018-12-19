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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AIP_INCLUDE_AIP_AUDIOINPUTPROCESSOR_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AIP_INCLUDE_AIP_AUDIOINPUTPROCESSOR_H_

#include <chrono>
#include <memory>
#include <unordered_set>
#include <vector>

#include <AVSCommon/AVS/Attachment/InProcessAttachmentReader.h>
#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/AVS/DirectiveHandlerConfiguration.h>
#include <AVSCommon/AVS/DialogUXStateAggregator.h>
#include <AVSCommon/SDKInterfaces/AudioInputProcessorObserverInterface.h>
#include <AVSCommon/SDKInterfaces/ChannelObserverInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveSequencerInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageRequestObserverInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/UserInactivityMonitorInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/Timing/Timer.h>
#include <SpeechEncoder/SpeechEncoder.h>

#include "AudioProvider.h"
#include "ESPData.h"
#include "Initiator.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace aip {

/**
 * This class implements a @c SpeechRecognizer capability agent.
 *
 * @see https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/speechrecognizer
 *
 * @c AudioInputProcessor is a top-level component which should be instantiated in application code, and connected up
 * to other interfaces in the Alexa Client SDK.  Interfaces which are used directly by the AudioInputProcessor are
 * passed directly to its @c create() function.  To start sending an audio event, application code should call one of
 * the @c recognize() functions.  To stop sending audio, application code should call the @c stopCapture() function.
 * Application code can also register objects which implement the @c ObserverInterface to receive notifications when
 * the @c AudioInputProcessor state changes.
 */
class AudioInputProcessor
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public avsCommon::sdkInterfaces::DialogUXStateObserverInterface
        , public avsCommon::sdkInterfaces::MessageRequestObserverInterface
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<AudioInputProcessor> {
public:
    /// Alias to the @c AudioInputProcessorObserverInterface for brevity.
    using ObserverInterface = avsCommon::sdkInterfaces::AudioInputProcessorObserverInterface;

    /// A reserved @c Index value which is considered invalid.
    static const auto INVALID_INDEX = std::numeric_limits<avsCommon::avs::AudioInputStream::Index>::max();

    /**
     * Creates a new @c AudioInputProcessor instance.
     *
     * @param directiveSequencer The Directive Sequencer to register with for receiving directives.
     * @param messageSender The object to use for sending events.
     * @param contextManager The AVS Context manager used to generate system context for events.
     * @param focusManager The channel focus manager used to manage usage of the dialog channel.
     * @param dialogUXStateAggregator The dialog state aggregator which tracks UX states related to dialog.
     * @param exceptionEncounteredSender The object to use for sending AVS Exception messages.
     * @param userInactivityNotifier The object to use for resetting user inactivity.
     * @param speechEncoder The Speech Encoder used to encode audio inputs. This parameter is optional and
     *     defaults to nullptr, which disable the encoding feature.
     * @param defaultAudioProvider A default @c avsCommon::AudioProvider to use for ExpectSpeech if the previous
     *     provider is not readable (@c avsCommon::AudioProvider::alwaysReadable).  This parameter is optional and
     *     defaults to an invalid @c avsCommon::AudioProvider.
     * @return A @c std::shared_ptr to the new @c AudioInputProcessor instance.
     */
    static std::shared_ptr<AudioInputProcessor> create(
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> directiveSequencer,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<avsCommon::avs::DialogUXStateAggregator> dialogUXStateAggregator,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
        std::shared_ptr<avsCommon::sdkInterfaces::UserInactivityMonitorInterface> userInactivityNotifier,
        std::shared_ptr<speechencoder::SpeechEncoder> speechEncoder = nullptr,
        AudioProvider defaultAudioProvider = AudioProvider::null());

    /**
     * Adds an observer to be notified of AudioInputProcessor state changes.
     *
     * @param observer The observer object to add.
     */
    void addObserver(std::shared_ptr<ObserverInterface> observer);

    /**
     * Removes an observer from the set of observers to be notified of AudioInputProcessor state changes.
     *
     * @note This is a synchronous call which can not be made by an observer callback.  Attempting to call
     *     @c removeObserver() from @c ObserverInterface::onStateChanged() will result in a deadlock.
     *
     * @param observer The observer object to remove.
     */
    void removeObserver(std::shared_ptr<ObserverInterface> observer);

    /**
     * This function asks the @c AudioInputProcessor to send a Recognize Event to AVS and start streaming from @c
     * audioProvider, which transitions it to the @c RECOGNIZING state.  A ReportEchoSpatialPerceptionData Event will
     * also be sent before the Recognize event if ESP measurements is passed into @c espData.  This function can be
     * called in any state except @c BUSY, however the flags in @c AudioProvider will dictate whether the call is
     * allowed to override an ongoing Recognize Event. If the flags do not allow an override, no event will be sent, no
     * state change will occur, and the function will fail.
     *
     * @note This function will not pass the audio stream to @c MessageSenderInterface to start streaming if the the
     *     start index or any subsequent data has already expired from the buffer.  In addition, it is assumed that
     *     @c MessageSenderInterface will stop streaming immediately if it detects an overrun, and notify AIP of this
     *     condition (through a @c MessageRequest callback). These requirements ensure that the @c begin and
     *     @c keywordEnd indices will remain accurate for the actual audio samples sent to AVS.
     *
     * False-wakeword detection in the cloud will be used when all of the following requirements are met:
     *
     * @li `initiator == Initiator::WAKEWORD`
     * @li `begin != INVALID_INDEX`
     * @li `end != INVALID_INDEX`
     * @li `!keyword.empty()`
     *
     * If all of the above requirements are met, audio steaming will start between 0 and 500ms before @c begin, and the
     * cloud will perform additional verification of the wakeword audio before proceeding to recognize the subsequent
     * audio.
     *
     * @param audioProvider The @c AudioProvider to stream audio from.
     * @param initiator The type of interface that initiated this recognize event.
     * @param startOfSpeechTimestamp Moment in time when user started talking to Alexa. This parameter is optional
     * and it is used to measure user perceived latency.
     * @param begin The @c Index in @c audioProvider.stream where audio streaming should begin.  This parameter is
     *     optional, and defaults to @c INVALID_INDEX.  When this parameter is not specified, @c recognize() will
     *     stream audio starting at the time of the @c recognize() call.  If the @c initiator is @c WAKEWORD, and this
     *     and @c keywordEnd are specified, streaming will begin between 0 and 500ms prior to the @c Index specified by
     *     this parameter to attempt false wakeword validation.
     * @param keywordEnd The @c Index in @c audioProvider.stream where the wakeword ends.  This parameter is optional,
     *     and defaults to @c INVALID_INDEX.  This parameter is ignored if initiator is not @c WAKEWORD.
     * @param keyword The text of the keyword which was recognized.  This parameter is optional, and defaults to an
     *     empty string.  This parameter is ignored if initiator is not @c WAKEWORD.  The only value currently
     *     accepted by AVS for keyword is "ALEXA".  See
     *     https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/context#recognizerstate
     * @param espData The ESP measurements to be sent in the ReportEchoSpatialPerceptionData event.
     * @param KWDMetadata Wake word engine metadata.
     * @return A future which is @c true if the Recognize Event was started successfully, else @c false.
     */
    std::future<bool> recognize(
        AudioProvider audioProvider,
        Initiator initiator,
        std::chrono::steady_clock::time_point startOfSpeechTimestamp = std::chrono::steady_clock::now(),
        avsCommon::avs::AudioInputStream::Index begin = INVALID_INDEX,
        avsCommon::avs::AudioInputStream::Index keywordEnd = INVALID_INDEX,
        std::string keyword = "",
        const ESPData espData = ESPData::getEmptyESPData(),
        std::shared_ptr<const std::vector<char>> KWDMetadata = nullptr);

    /**
     * This function asks the @c AudioInputProcessor to stop streaming audio and end an ongoing Recognize Event, which
     * transitions it to the @c BUSY state.  This function can only be called in the @c RECOGNIZING state; calling it
     * while in another state will fail.
     *
     * @return A future which is @c true if called in the correct state and a Recognize Event's audio streaming was
     *     stopped successfully, else @c false.
     */
    std::future<bool> stopCapture();

    /**
     * This function forces the @c AudioInputProcessor back to the @c IDLE state.  This function can be called in any
     * state, and will end any Event which is currently in progress.
     *
     * @return A future which indicates when the @c AudioInputProcessor is back to the @c IDLE state.
     */
    std::future<void> resetState();

    /// @name ContextRequesterInterface Functions
    /// @{
    void onContextAvailable(const std::string& jsonContext) override;
    void onContextFailure(const avsCommon::sdkInterfaces::ContextRequestError error) override;
    /// @}

    /// @name MessageRequestObserverInterface Functions
    /// @{
    void onSendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status status) override;
    void onExceptionReceived(const std::string& exceptionMessage) override;
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

    /// @name DialogUXStateObserverInterface Functions
    /// @{
    void onDialogUXStateChanged(
        avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState newState) override;
    /// @}

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param directiveSequencer The Directive Sequencer to register with for receiving directives.
     * @param messageSender The object to use for sending events.
     * @param contextManager The AVS Context manager used to generate system context for events.
     * @param focusManager The channel focus manager used to manage usage of the dialog channel.
     * @param exceptionEncounteredSender The object to use for sending ExceptionEncountered messages.
     * @param userInactivityMonitor The object to use for resetting user inactivity.
     * @param speechEncoder The Speech Encoder used to encode audio inputs. This parameter is optional and
     *     will disable the encoding feature if set to nullptr.
     * @param defaultAudioProvider A default @c avsCommon::AudioProvider to use for ExpectSpeech if the previous
     *     provider is not readable (@c AudioProvider::alwaysReadable).  This parameter is optional, and ignored if set
     *     to @c AudioProvider::null().
     *
     * @note This constructor is private so that users are forced to use the @c create() factory function.  The primary
     *     reason for this is to ensure that a @c std::shared_ptr to the instance exists, which is a requirement for
     *     using @c std::enable_shared_from_this.
     */
    AudioInputProcessor(
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> directiveSequencer,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
        std::shared_ptr<avsCommon::sdkInterfaces::UserInactivityMonitorInterface> userInactivityMonitor,
        std::shared_ptr<speechencoder::SpeechEncoder> speechEncoder,
        AudioProvider defaultAudioProvider);

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /**
     * This is a helper function which submits an @c Executor call to @c executeExpectSpeechTimedOut() when
     * @c m_expectingSpeechTimer expires.  This function should only be called from the @c EXPECTING_SPEECH state.
     *
     * @return A future which is @c true if called in the correct state and an ExpectSpeechTimeout Event was sent
     *     successfully, else @c false.
     */
    std::future<bool> expectSpeechTimedOut();

    /**
     * This function handles a STOP_CAPTURE directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void handleStopCaptureDirective(std::shared_ptr<DirectiveInfo> info);

    /**
     * This function handles a EXPECT_SPEECH directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void handleExpectSpeechDirective(std::shared_ptr<DirectiveInfo> info);

    /**
     * This function handles a SET_END_OF_SPEECH_OFFSET directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void handleSetEndOfSpeechOffsetDirective(std::shared_ptr<DirectiveInfo> info);

    /**
     * @name Executor Thread Functions
     *
     * These functions (and only these functions) are called by @c m_executor on a single worker thread.  All other
     * functions in this class can be called asynchronously, and pass data to the @c Executor thread through parameters
     * to lambda functions.  No additional synchronization is needed.
     */
    /// @{

    /**
     * This function builds a @c ReportEchoSpatialPerceptionData event.  The event will not be sent out until context is
     * available so that the @c ReportEchoSpatialPerceptionData event will be sent just before the @c Recognize event.
     * @param espData The ESP measurements to be sent in the ReportEchoSpatialPerceptionData event.
     */
    void executePrepareEspPayload(const ESPData espData);

    /**
     * This function builds @c a Recognize event and will request context so the events will be sent upon @c
     * onContextAvailable().  This version of the function expects an enumerated @c Initiator, and will build up the
     * initiator json content for the event, before calling the @c executeRecognize() function below which takes an
     * initiator string.
     *
     * @see @c recognize() for a detailed explanation of the Recognize Event.
     *
     * @param audioProvider The @c AudioProvider to stream audio from.
     * @param initiator The type of interface that initiated this recognize event.
     * @param startOfSpeechTimestamp Moment in time when user started talking to Alexa.
     * @param begin The @c Index in @c audioProvider.stream where audio streaming should begin. When this parameter is
     *     @c INVALID_INDEX, @c recognize() will stream audio starting at the time of the @c recognize() call.
     * @param keywordEnd The @c Index in @c audioProvider.stream where the wakeword ends.
     * @param keyword The text of the keyword which was recognized.
     * @param KWDMetadata Wake word engine metadata.
     * @return @c true if the Recognize Event was started successfully, else @c false.
     */
    bool executeRecognize(
        AudioProvider provider,
        Initiator initiator,
        std::chrono::steady_clock::time_point startOfSpeechTimestamp,
        avsCommon::avs::AudioInputStream::Index begin,
        avsCommon::avs::AudioInputStream::Index keywordEnd,
        const std::string& keyword,
        std::shared_ptr<const std::vector<char>> KWDMetadata);

    /**
     * This function builds and sends a @c Recognize event.  This version of the function expects a pre-built string
     * containing the initiator json content for the event.  This initiator string is either built by the
     * @c executeRecognize() function above which takes an enumerated @c Initiator, or is an opaque object provided by
     * an @c ExpectSpeech directive.
     *
     * @see @c recognize() for a detailed explanation of the Recognize Event.
     *
     * @param audioProvider The @c AudioProvider to stream audio from.
     * @param initiatorJson A JSON string describing the type of interface that initiated this recognize event.
     * @param startOfSpeechTimestamp Moment in time when user started talking to Alexa.
     * @param begin The @c Index in @c audioProvider.stream where audio streaming should begin.  This parameter is
     *     optional, and defaults to @c INVALID_INDEX.  When this parameter is not specified, @c recognize() will
     *     stream audio starting at the time of the @c recognize() call.  If the @c initiator is @c WAKEWORD, and this
     *     and @c keywordEnd are specified, streaming will begin between 0 and 500ms prior to the @c Index specified by
     *     this parameter to attempt false wakeword validation.
     * @param keyword The text of the keyword which was recognized.  This parameter is optional, and defaults to an
     *     empty string.  This parameter is ignored if initiator is not @c WAKEWORD.  The only value currently
     *     accepted by AVS for keyword is "ALEXA".  See
     *     https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/context#recognizerstate
     * @param KWDMetadata Wake word engine metadata.
     * @return @c true if the Recognize Event was started successfully, else @c false.
     */
    bool executeRecognize(
        AudioProvider provider,
        const std::string& initiatorJson,
        std::chrono::steady_clock::time_point startOfSpeechTimestamp = std::chrono::steady_clock::now(),
        avsCommon::avs::AudioInputStream::Index begin = INVALID_INDEX,
        const std::string& keyword = "",
        std::shared_ptr<const std::vector<char>> KWDMetadata = nullptr);

    /**
     * This function receives the full system context from @c ContextManager.  Context requests are initiated by
     * @c executeRecognize() calls, and provide the final piece of information needed to assemble a @c MessageRequest.
     * If focus has already changed to @c FOREGROUND by the time this function is called, this function will send the
     * @c MessageRequest.  If focus has not changed to @c FOREGROUND, this function will assemble the MessageRequest,
     * but will defer sending it to @c executeOnFocusChanged().
     *
     * @param jsonContext The full system context to send with the event.
     */
    void executeOnContextAvailable(const std::string jsonContext);

    /**
     * This function is called when a context request fails.  Context requests are initiated by @c executeRecognize()
     * calls, and failure to complete the context request results in failure to send the recognize event.
     *
     * @param error The reason the context request failed to complete.
     */
    void executeOnContextFailure(const avsCommon::sdkInterfaces::ContextRequestError error);

    /**
     * This function is called when the @c FocusManager focus changes.  This might occur when another component
     * acquires focus on the dialog channel, in which case the @c AudioInputProcessor will end any activity and return
     * to @c IDLE. This function is also called after a call to @c executeRecognize() tries to acquire the channel.
     * If this function changes the focus to @c FOREGROUND in the @c RECOGNIZING state and the @c MessageRequest has
     * already been generated, this function will send the @c MessageRequest.  If the @c MessageRequest has not been
     * generated yet, this function will defer sending to @c executeOnContextAvailable().
     *
     * @param newFocus The focus state to change to.
     */
    void executeOnFocusChanged(avsCommon::avs::FocusState newFocus);

    /**
     * This function asks the @c AudioInputProcessor to stop streaming audio and end an ongoing Recognize Event, which
     * transitions it to the @c BUSY state.  This function can only be called in the @c RECOGNIZING state; calling it
     * while in another state will fail.
     *
     * @param stopImmediately Flag indicating that streaming should stop immediatly.  If this flag is set to @c false
     *     (the default), streaming will continue until any existing data in the buffer has been streamed.  If this
     *     flag is set to @c true, existing data in the buffer which has not already been streamed will be discarded,
     *     and streaming will stop immediately.
     * @param info The @c DirectiveInfo for this call.  This parameter can be @c nullptr, meaning the call did not
     *     come from a directive, and no cleanup is needed.
     * @return @c true if called in the correct state and a Recognize Event's audio streaming was stopped successfully,
     *     else @c false.
     */
    bool executeStopCapture(bool stopImmediately = false, std::shared_ptr<DirectiveInfo> info = nullptr);

    /**
     * This function forces the @c AudioInputProcessor back to the @c IDLE state.  This function can be called in any
     * state, and will end any Event which is currently in progress.
     */
    void executeResetState();

    /**
     * This function tells the @c AudioInputProcessor to expect a Recognize event within the specified timeout.  If the
     * previous or default @c AudioProvider is capable of streaming immediately, this function will start the Recognize
     * right away.  If neither @c AudioProvider can start streaming immediately, this function will transition to the
     * @c EXPECTING_SPEECH state for the specified timeout.  If a call to @c executeRecognize() occurs before the
     * timeout, it will stop the timer and send the event.  If the timer expires before a call to @c executeRecognize()
     * occurs, the timer will call @c executeExpectSpeechTimedOut(), which will send an ExpectSpeechTimedOut event.
     *
     * @param timeout The number of milliseconds to wait for a Recognize event.
     * @param info The @c DirectiveInfo for this call.
     * @return @c true if called in the correct state and the state had changed to @c EXPECTING_SPEECH or
     *     @c RECOGNIZING, else @c false.
     */
    bool executeExpectSpeech(std::chrono::milliseconds timeout, std::shared_ptr<DirectiveInfo> info);

    /**
     * This function is called when @c m_expectingSpeechTimer expires.  It will send an ExpectSpeechTimedOut event.
     *
     * @return @c true if called in the correct state and the ExpectSpeechTimedOut event was sent, else @c false.
     */
    bool executeExpectSpeechTimedOut();

    /**
     * This function is called whenever the AVS UX dialog state of the system changes. This function will block
     * processing of other state changes, so any implementation of this should return quickly.
     *
     * @param newState The new dialog specific AVS UX state.
     */
    void executeOnDialogUXStateChanged(
        avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState newState);

    /**
     * This function updates the @c AudioInputProcessor state and notifies the state observer.  Any changes to
     * @c m_state should be made through this function.
     *
     * @param state The new state to change to.
     */
    void setState(ObserverInterface::State state);

    /**
     * Remove a directive from the map of message IDs to DirectiveInfo instances.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective whose message ID is to be removed.
     */
    void removeDirective(std::shared_ptr<DirectiveInfo> info);

    /// This function sends @c m_request, updates state, and calls @c m_deferredStopCapture if pending.
    void sendRequestNow();

    /// @}

    /// The Directive Sequencer to register with for receiving directives.
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> m_directiveSequencer;

    /// The object to use for sending events.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c FocusManager used to manage usage of the dialog channel.
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> m_focusManager;

    /// The @c UserInactivityMonitorInterface used to reset the inactivity timer of the user.
    std::shared_ptr<avsCommon::sdkInterfaces::UserInactivityMonitorInterface> m_userInactivityMonitor;

    /// Timer which runs in the @c EXPECTING_SPEECH state.
    avsCommon::utils::timing::Timer m_expectingSpeechTimer;

    /// The Speech Encoder to encode input stream.
    std::shared_ptr<speechencoder::SpeechEncoder> m_encoder;

    /**
     * @name Executor Thread Variables
     *
     * These member variables are only accessed by functions in the @c m_executor worker thread, and do not require any
     * synchronization.
     */
    /// @{

    /// Observer objects to notify when the @c AudioInputProvder changes states.
    std::unordered_set<std::shared_ptr<ObserverInterface>> m_observers;

    /**
     * Default @c AudioProvider which was passed to the constructor; used for ExpectSpeech directives when
     * @c m_lastAudioProvider is not capable of streaming on demand (@c AudioProvider::alwaysReadable).
     */
    AudioProvider m_defaultAudioProvider;

    /**
     * The last @c AudioProvider used in an @c executeRecognize(); will be used for ExpectSpeech directives
     * if it is capable of streaming on demand (@c AudioProvider::alwaysReadable).
     */
    AudioProvider m_lastAudioProvider;

    /**
     * The attachment reader which is currently being used to stream audio for a Recognize event.  This pointer is
     * valid during the @c RECOGNIZING state, and is retained by @c AudioInputProcessor so that it can close the
     * stream from @c executeStopCapture().
     */
    std::shared_ptr<avsCommon::avs::attachment::InProcessAttachmentReader> m_reader;

    /**
     * The attachment reader used for the wake word engine metadata. It's is populated by a call to @c
     * executeRecognize(), and later consumed by a call to @c executeOnContextAvailable() when the context arrives and
     * the full @c MessageRequest can be assembled.  This reader is only relevant during the @c RECOGNIZING state.
     */
    std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> m_KWDMetadataReader;

    /**
     * The payload for a ReportEchoSpatialPerceptionData event.  This string is populated by a call to @c
     * executeRecognize(), and later consumed by a call to @c executeOnContextAvailable() when the context arrives and
     * the full @c MessageRequest can be assembled.  This string is only relevant during the @c RECOGNIZING state.
     */
    std::string m_espPayload;

    /**
     * The payload for a Recognize event.  This string is populated by a call to @c executeRecognize(), and later
     * consumed by a call to @c executeOnContextAvailable() when the context arrives and the full @c MessageRequest can
     * be assembled.  This string is only relevant during the @c RECOGNIZING state.
     */
    std::string m_recognizePayload;

    /**
     * The @c MessageRequest for a ReportEchoSpatialPerceptionData event.  This request is created by a call to
     * @c executeOnContextAvailable(), and either sent immediately (if `m_focusState == afml::FocusState::FOREGROUND`),
     * or later sent by a call to @c executeOnFocusChanged().  This pointer is only valid during the @c RECOGNIZING
     * state after a call to @c executeRecognize(), and is reset after it is sent.
     */
    std::shared_ptr<avsCommon::avs::MessageRequest> m_espRequest;

    /**
     * The @c MessageRequest for a Recognize event.  This request is created by a call to
     * @c executeOnContextAvailable(), and either sent immediately (if `m_focusState == afml::FocusState::FOREGROUND`),
     * or later sent by a call to @c executeOnFocusChanged().  This pointer is only valid during the @c RECOGNIZING
     * state after a call to @c executeRecognize(), and is reset after it is sent.
     *
     * @note If ESP measurement is provided in @c recognize(), the @c ReportEchoSpatialPerceptionData event is sent
     * before the @c Recognize event.
     */
    std::shared_ptr<avsCommon::avs::MessageRequest> m_recognizeRequest;

    /// The current state of the @c AudioInputProcessor.
    ObserverInterface::State m_state;

    /// The current focus state of the @c AudioInputProcessor on the dialog channel.
    avsCommon::avs::FocusState m_focusState;

    /**
     * This flag is set to @c true upon entering the @c RECOGNIZING state, and remains true until the @c Recognize
     * event is sent.
     */
    bool m_preparingToSend;

    /**
     * If @c stopCapture() is called during @c RECOGNIZING before the event is sent, the stop operation is stored in
     * this variable so that it can be called after the @c Recognize event is sent.
     */
    std::function<void()> m_deferredStopCapture;

    /// This flag indicates whether the initial dialog UX State has been received.
    bool m_initialDialogUXStateReceived;

    /**
     * This flag indicates if a stop has been done locally on the device and that it's safe to ignore the StopCapture
     * directive.
     */
    bool m_localStopCapturePerformed;

    /**
     * The initiator value from the preceding ExpectSpeech directive. The ExpectSpeech directive's initiator
     * will need to be consumed and sent back in a subsequent Recognize event. This should be cleared if the
     * ExpectSpeech times out. An empty initiator is possible, in which case an empty initiator should be sent back to
     * AVS. This must override the standard user initiated Recognize initiator.
     *
     * A value of nullptr indicates that there is no pending preceding initiator to be consumed, and the Recognize's
     * initiator should conform to the standard user initiated format.
     */
    std::unique_ptr<std::string> m_precedingExpectSpeechInitiator;
    /// @}

    /// Set of capability configurations that will get published using the Capabilities API
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> m_capabilityConfigurations;

    /**
     * @c Executor which queues up operations from asynchronous API calls.
     *
     * @note This declaration needs to come *after* the Executor Thread Variables above so that the thread shuts down
     *     before the Executor Thread Variables are destroyed.
     */
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace aip
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AIP_INCLUDE_AIP_AUDIOINPUTPROCESSOR_H_
