/**
 * AudioInputProcessor.h
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_AIP_INCLUDE_AIP_AUDIO_INPUT_PROCESSOR_H_
#define ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_AIP_INCLUDE_AIP_AUDIO_INPUT_PROCESSOR_H_

#include <memory>
#include <unordered_set>

#include <AVSCommon/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/SDKInterfaces/DirectiveSequencerInterface.h>
#include <AVSCommon/SDKInterfaces/ChannelObserverInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSUtils/Threading/Executor.h>
#include <AVSCommon/Utils/Timing/Timer.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/AVS/DirectiveHandlerConfiguration.h>
#include <AVSCommon/AVS/Attachment/InProcessAttachmentReader.h>
#include "AudioProvider.h"
#include "Initiator.h"

namespace alexaClientSDK {
namespace capabilityAgent {
namespace aip {

/// Forward-declare @c aip::ObserverInterface class.
class ObserverInterface;

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
 *
 * @note @c AudioInputProcessor should be returned to an @c IDLE state before closing the application.  This can be
 * achieved by waiting on the @c std::future returned by a call to @c resetState().
 */
class AudioInputProcessor
    : public avsCommon::avs::CapabilityAgent, public std::enable_shared_from_this<AudioInputProcessor> {
public:
    /// The different states the @c AudioInputProcessor can be in
    enum class State {
        /// In this state, the @c AudioInputProcessor is not waiting for or transmitting speech.
        IDLE,

        /// In this state, the @c AudioInputProcessor is waiting for a call to recognize().
        EXPECTING_SPEECH,

        /// In this state, the @c AudioInputProcessor is actively streaming speech.
        RECOGNIZING,

        /**
         * In this state, the @c AudioInputProcessor has finished streaming and is waiting for completion of an Event.
         * Note that @c recognize() calls are not permitted while in the @c BUSY state.
         */
        BUSY
    };

    /**
     * This function converts the provided @c State to a string.
     *
     * @param state The @c State to convert to a string.
     * @return The string conversion of @c state.
     */
    static std::string stateToString(State state);

    /// A reserved @c Index value which is considered invalid.
    static const auto INVALID_INDEX = std::numeric_limits<avsCommon::sdkInterfaces::AudioInputStream::Index>::max();

    /**
     * Creates a new @c AudioInputProcessor instance.
     *
     * @param directiveSequencer The Directive Sequencer to register with for receiving directives.
     * @param messageSender The object to use for sending events.
     * @param contextManager The AVS Context manager used to generate system context for events.
     * @param focusManager The channel focus manager used to manage usage of the dialog channel.
     * @param exceptionSender The object to use for sending AVS Exception messages.
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
         std::shared_ptr<avsCommon::ExceptionEncounteredSenderInterface> exceptionSender,
         AudioProvider defaultAudioProvider = AudioProvider::null());

    /**
     * This function provides the configuration data for registering/deregistering this object with
     * @c DirectiveSequencerInterface.  This configuration should be used to register an @c AudioInputProvider instance
     * with a directive sequencer using @c DirectiveSequencerInterface::addDirectiveHandlers().  When an
     * @c AudioInputProvider instance is no longer needed, it can be deregistered from the directivesequencer by
     * passing this configuration data to @c DirectiveSequencerInterface::removeDirectiveHandlers().
     *
     * TODO: Review strategy for registering/deregistering @c CapabilityAgent implementations with
     *     @c DirectiveSequencerInterface - ACSDK277.
     *
     * @return The configuration data.
     */
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration();

    /**
     * Adds an observer to be notified of AudioInputProcessor state changes.
     *
     * @param observer The observer object to add.
     */
    void addObserver(std::shared_ptr<ObserverInterface> observer);

    /**
     * Removes an observer from the set of observers to be notified of AudioInputProcessor state changes.
     *
     * @param observer The observer object to remove.
     */
    void removeObserver(std::shared_ptr<ObserverInterface> observer);

    /**
     * This function asks the @c AudioInputProcessor to send a Recognize Event to AVS and start streaming from
     * @c audioProvider, which transitions it to the @c RECOGNIZING state.  This function can be called in any state
     * except @c BUSY, however the flags in @c AudioProvider will dictate whether the call is allowed to override an
     * ongoing Recognize Event.  If the flags do not allow an override, no event will be sent, no state change will
     * occur, and the function will fail.
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
     * @param begin The @c Index in @c audioProvider.stream where audio streaming should begin.  This parameter is
     *     optional, and defaults to @c INVALID_INDEX.  When this parameter is not specified, @c recognize() will
     *     stream audio starting at the time of the @c recognize() call.  If the @c initiator is @c WAKEWORD, and this
     *     and @c keywordEnd are specified, streaming will begin between 0 and 500ms prior to the @c Index specified by
     *     this parameter to attempt false wakeword validation.
     * @param keywordEnd The @c Index in @c audioProvider.stream where the wakeword ends.  This parameter is optional,
     *     and defaults to @c INVALID_INDEX.  This parameter is ignored if initiator is not @c WAKEWORD.
     * @param keyword The text of the keyword which was recognized.  This parameter is optional, and defaults to an
     *     empty string.  This parameter is ignored if initiator is not @c WAKEWORD.
     * @return A future which is @c true if the Recognize Event was started successfully, else @c false.
     */
    std::future<bool> recognize(
        AudioProvider audioProvider,
        Initiator initiator,
        avsCommon::sdkInterfaces::AudioInputStream::Index begin = INVALID_INDEX,
        avsCommon::sdkInterfaces::AudioInputStream::Index keywordEnd = INVALID_INDEX,
        std::string keyword = "");

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
     * @note This function should be called when shutting down an AIP to ensure it is in a quiescent state.
     *
     * @return A future which indicates when the @c AudioInputProcessor is back to the @c IDLE state.
     */
    std::future<void> resetState();

    /// @name StateProviderInterface Functions
    /// @{
    void provideState(unsigned int stateRequestToken) override;
    /// @}

    /// @name ContextRequesterInterface Functions
    /// @{
    void onContextAvailable(const std::string& jsonContext) override;
    void onContextFailure(const avsCommon::sdkInterfaces::ContextRequestError error) override;
    /// @}

    /// @name CapabilityAgent/DirectiveHandlerInterface Functions
    /// @{
    void handleDirectiveImmediately(const DirectiveAndResultInterface& directive) override;
    void preHandleDirective(const DirectiveAndResultInterface& directive) override;
    void handleDirective(const DirectiveAndResultInterface& directive) override;
    void cancelDirective(const DirectiveAndResultInterface& directive) override;
    void onDeregistered() override;
    /// @}

    /// @name ChannelObserverInterface Functions
    /// @{
    void onFocusChanged(avsCommon::sdkInterfaces::FocusState newFocus) override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param directiveSequencer The Directive Sequencer to register with for receiving directives.
     * @param messageSender The object to use for sending events.
     * @param contextManager The AVS Context manager used to generate system context for events.
     * @param focusManager The channel focus manager used to manage usage of the dialog channel.
     * @param exceptionSender The object to use for sending exceptions.
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
        std::shared_ptr<avsCommon::ExceptionEncounteredSenderInterface> exceptionSender,
        AudioProvider defaultAudioProvider);

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
     * @param directiveAndResult The @c DirectiveAndResultInterface containing the @c AVSDirective and the
     *     @c DirectiveHandlerResultInterface.
     */
    void handleStopCaptureDirective(const DirectiveAndResultInterface& directiveAndResult);

    /**
     * This function handles a EXPECT_SPEECH directive.
     *
     * @param directiveAndResult The @c DirectiveAndResultInterface containing the @c AVSDirective and the
     *     @c DirectiveHandlerResultInterface.
     */
    void handleExpectSpeechDirective(const DirectiveAndResultInterface& directiveAndResult);

    /**
     * @name Executor Thread Functions
     *
     * These functions (and only these functions) are called by @c m_executor on a single worker thread.  All other
     * functions in this class can be called asynchronously, and pass data to the @c Executor thread through parameters
     * to lambda functions.  No additional synchronization is needed.
     */
    /// @{

    /**
     * @copyDoc recognize()
     *
     * @note @c initiatorType is passed as a string to @c executeRecognize() so to support the @c ExpectSpeech opaque
     *     initiator string.
     */
    bool executeRecognize(
        AudioProvider provider,
        const std::string& initiatorType,
        avsCommon::sdkInterfaces::AudioInputStream::Index begin = INVALID_INDEX,
        avsCommon::sdkInterfaces::AudioInputStream::Index keywordEnd = INVALID_INDEX,
        const std::string& keyword = "");

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
    void executeOnFocusChanged(avsCommon::sdkInterfaces::FocusState newFocus);

    /**
     * This function asks the @c AudioInputProcessor to stop streaming audio and end an ongoing Recognize Event, which
     * transitions it to the @c BUSY state.  This function can only be called in the @c RECOGNIZING state; calling it
     * while in another state will fail.
     *
     * @param stopImmediately Flag indicating that streaming should stop immediatly.  If this flag is set to @c false
     *     (the default), streaming will continue until any existing data in the buffer has been streamed.  If this
     *     flag is set to @c true, existing data in the buffer which has not already been streamed will be discarded,
     *     and streaming will stop immediately.
     * @param directive The @c AVSDirective for this call.  This parameter can be @c nullptr, meaning the call did not
     *     come from a directive, and no cleanup is needed.
     * @param result The object to use for reporting the result of handling this directive.  This parameter can be
     *     @c nullptr, meaning the call did not come from a directive, and no reporting is needed.
     * @return @c true if called in the correct state and a Recognize Event's audio streaming was stopped successfully,
     *     else @c false.
     */
    bool executeStopCapture(
        bool stopImmediately = false,
        std::shared_ptr<avsCommon::AVSDirective> directive = nullptr,
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerResultInterface> resultInterface = nullptr);

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
     * @param initiator The initiator json string from the ExpectSpeech directive.
     * @param directive The @c AVSDirective for this call.
     * @param result The object to use for reporting the result of handling this directive.  This parameter can be
     *     @c nullptr, meaning the call did not come from a directive, and no reporting is needed.
     * @return @c true if called in the correct state and the state had changed to @c EXPECTING_SPEECH or
     *     @c RECOGNIZING, else @c false.
     */
    bool executeExpectSpeech(
        std::chrono::milliseconds timeout,
        std::string initiator,
        std::shared_ptr<avsCommon::AVSDirective> directive,
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerResultInterface> result = nullptr);

    /**
     * This function is called when @c m_expectingSpeechTimer expires.  It will send an ExpectSpeechTimedOut event.
     *
     * @return @c true if called in the correct state and the ExpectSpeechTimedOut event was sent, else @c false.
     */
    bool executeExpectSpeechTimedOut();

    /**
     * This function provides updated context information for SpeechRecognizer to @c ContextManager.  This function is
     * called when @c ContextManager calls @c provideState(), and is also called internally by @c setState().
     *
     * @param sendToken flag indicating whether @c stateRequestToken contains a valid token which should be passed
     *     along to @c ContextManager.  This flag defaults to @c false.
     * @param stateRequestToken The token @c ContextManager passed to the @c provideState() call, which will be passed
     *     along to the ContextManager::setState() call.  This parameter is not used if @c sendToken is @c false.
     */
    void executeProvideState(bool sendToken = false, unsigned int stateRequestToken = 0);

    /**
     * This function updates the @c AudioInputProcessor state and notifies the state observer.  Any changes to
     * @c m_state should be made through this function.
     *
     * @param state The new state to change to.
     */
    void setState(State state);

    /// @}

    /// The Directive Sequencer to register with for receiving directives.
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> m_directiveSequencer;

    /// The object to use for sending events.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c FocusManager used to manage usage of the dialog channel.
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> m_focusManager;

    /// An object to use for sending AVS Exception messages.
    std::shared_ptr<avsCommon::ExceptionEncounteredSenderInterface> m_exceptionSender;

    /// Timer which runs in the @c EXPECTING_SPEECH state.
    avsCommon::utils::timing::Timer m_expectingSpeechTimer;

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
     * The payload for a Recognize event.  This string is populated by a call to @c executeRecognize(), and later
     * consumed by a call to @c executeOnContextAvailable() when the context arrives and the full @c MessageRequest can
     * be assembled.  This string is only relevant during the @c RECOGNIZING state.
     */
    std::string m_payload;

    /**
     * The @c MessageRequest for a Recognize event.  This request is created by a call to
     * @c executeOnContextAvailable(), and either sent immediately (if `m_focusState == afml::FocusState::FOREGROUND`),
     * or later sent by a call to @c executeOnFocusChanged().  This pointer is only valid during the @c RECOGNIZING
     * state after a call to @c executeRecognize(), and is reset after it is sent.
     */
    std::shared_ptr<avsCommon::avs::MessageRequest> m_request;

    /// The current state of the @c AudioInputProcessor.
    State m_state;

    /// The current focus state of the @c AudioInputProcessor on the dialog channel.
    avsCommon::sdkInterfaces::FocusState m_focusState;

    /**
     * The most recent wakeword used.  This variable defaults to "ALEXA", and is updated whenever a wakeword-enabled
     * call to @c executeRecognize() is made.  The @c executeProvideState() function uses this variable to populate the
     * wakeword field in the RecognizerState context.
     */
    std::string m_wakeword;
    /// @}

    /**
     * @c Executor which queues up operations from asynchronous API calls.
     *
     * @note This declaration needs to come *after* the Executor Thread Variables above so that the thread shuts down
     *     before the Executor Thread Variables are destroyed.
     */
    avsUtils::threading::Executor m_executor;
};

} // namespace aip
} // namespace capabilityagent
} // namespace alexaClientSDK

#endif //ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_AIP_INCLUDE_AIP_AUDIO_INPUT_PROCESSOR_H_
