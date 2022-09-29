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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AIP_INCLUDE_AIP_AUDIOINPUTPROCESSOR_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AIP_INCLUDE_AIP_AUDIOINPUTPROCESSOR_H_

#include <chrono>
#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <acsdk/AudioEncoderInterfaces/AudioEncoderInterface.h>
#include <AVSCommon/AVS/Attachment/InProcessAttachmentReader.h>
#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/CapabilityChangeNotifierInterface.h>
#include <AVSCommon/AVS/DialogUXStateAggregator.h>
#include <AVSCommon/AVS/DirectiveHandlerConfiguration.h>
#include <AVSCommon/AVS/ExceptionErrorType.h>
#include <AVSCommon/AVS/EditableMessageRequest.h>
#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/SDKInterfaces/AudioInputProcessorObserverInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ChannelObserverInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveSequencerInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/SDKInterfaces/ExpectSpeechTimeoutHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/SDKInterfaces/InternetConnectionObserverInterface.h>
#include <AVSCommon/SDKInterfaces/LocaleAssetsManagerInterface.h>
#include <AVSCommon/SDKInterfaces/LocaleAssetsObserverInterface.h>
#include <AVSCommon/SDKInterfaces/MessageRequestObserverInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/PowerResourceManagerInterface.h>
#include <AVSCommon/SDKInterfaces/SystemSoundPlayerInterface.h>
#include <AVSCommon/SDKInterfaces/UserInactivityMonitorInterface.h>
#include <AVSCommon/Utils/AudioFormat.h>
#include <AVSCommon/Utils/Metrics/DataPointDurationBuilder.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/Timing/Timer.h>
#include <Settings/DeviceSettingsManager.h>
#include <Settings/SettingEventMetadata.h>

#include "AudioProvider.h"
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
        , public avsCommon::sdkInterfaces::LocaleAssetsObserverInterface
        , public avsCommon::sdkInterfaces::DialogUXStateObserverInterface
        , public avsCommon::sdkInterfaces::MessageRequestObserverInterface
        , public avsCommon::sdkInterfaces::InternetConnectionObserverInterface
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<AudioInputProcessor> {
public:
    /// Alias to the @c AudioInputProcessorObserverInterface for brevity.
    using ObserverInterface = avsCommon::sdkInterfaces::AudioInputProcessorObserverInterface;

    /**
     * Request to configure @c AudioInputProcessor to provide multiple audio streams for a single Recognize event. Key
     * in the map is resolve key, which will be used by caller to resolve the unresolved @c MessageRequest. Each resolve
     * key correlates to a pair of encoding formats. The first format is the preferred format, and the second one is the
     * fallback format which is used if the preferred one is not supported.
     */
    using EncodingFormatRequest = std::
        map<std::string, std::pair<avsCommon::utils::AudioFormat::Encoding, avsCommon::utils::AudioFormat::Encoding>>;

    /**
     * Response to caller when AIP receives EncodingFormatRequest. Key in the map is resolve key from the request, and
     * corresponding value is the confirmed encoding format that AIP will provide for this resolve key.
     */
    using EncodingFormatResponse = std::map<std::string, avsCommon::utils::AudioFormat::Encoding>;

    /**
     * This function allows applications to tell the @c AudioInputProcessor that the @c ExpectSpeech directive's timeout
     * will be handled externally and stops the @c AudioInputProcessor from starting an internal timer to handle it.
     *
     * @param timeout The timeout of the @c ExpectSpeech directive.
     * @param expectSpeechTimedOut An @c std::function that applications may call if the timeout expires. This results
     * in an @c ExpectSpeechTimedOut event being sent to AVS if no @c recognize() call is made prior to the timeout
     * expiring. This function will return a future which is @c true if called in the correct state and an
     * @c ExpectSpeechTimeout Event was sent successfully, or @c false otherwise.
     * @return @c true if the @c ExpectSpeech directive's timeout will be handled externally and should not be handled
     * via an internal timer owned by the @c AudioInputProcessor.
     *
     * @note This function will be called after any calls to the @c AudioInputProcessorObserverInterface's
     * onStateChanged() method to notify of a state change to @c EXPECTING_SPEECH.
     * @note Implementations are not required to be thread-safe.
     * @note If the @cAudioInputProcessor object that this call came from is destroyed, the @c expectSpeechTimedOut
     * parameter is no longer valid.
     */
    using ExpectSpeechTimeoutHandler = avsCommon::sdkInterfaces::ExpectSpeechTimeoutHandlerInterface;

    /// A special keyword sent by supported wakeword engines for "Alexa, Stop".
    static constexpr const char* KEYWORD_TEXT_STOP = "STOP";

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
     * @param systemSoundPlayer The instance of the system sound player.
     * @param assetsManager Responsible for retrieving and changing the wake words and locale.
     * @param wakeWordConfirmation The wake word confirmation setting.
     * @param speechConfirmation The end of speech confirmation setting.
     * @param capabilityChangeNotifier The object with which to notify observers of @c AudioInputProcessor capability
     *     configurations change.
     * @param wakeWordsSetting The setting that represents the enabled wake words. This parameter is required if this
     * device supports wake words.
     * @param audioEncoder The Audio Encoder used to encode audio inputs. This parameter is optional and
     *     defaults to nullptr, which disable the encoding feature.
     * @param defaultAudioProvider A default @c avsCommon::AudioProvider to use for ExpectSpeech if the previous
     *     provider is not readable (@c avsCommon::AudioProvider::alwaysReadable).  This parameter is optional and
     *     defaults to an invalid @c avsCommon::AudioProvider.
     * @param powerResourceManager Power Resource Manager.
     * @param metricRecorder The metric recorder.
     * @param expectSpeechTimeoutHandler An optional interface that applications may provide to specify external
     * handling of the @c ExpectSpeech directive's timeout. If provided, this function must remain valid for the
     * lifetime of the @c AudioInputProcessor.
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
        std::shared_ptr<avsCommon::sdkInterfaces::SystemSoundPlayerInterface> systemSoundPlayer,
        const std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>& assetsManager,
        std::shared_ptr<settings::WakeWordConfirmationSetting> wakeWordConfirmation,
        std::shared_ptr<settings::SpeechConfirmationSetting> speechConfirmation,
        const std::shared_ptr<avsCommon::avs::CapabilityChangeNotifierInterface>& capabilityChangeNotifier,
        std::shared_ptr<settings::WakeWordsSetting> wakeWordsSetting = nullptr,
        std::shared_ptr<audioEncoderInterfaces::AudioEncoderInterface> audioEncoder = nullptr,
        AudioProvider defaultAudioProvider = AudioProvider::null(),
        std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface> powerResourceManager = nullptr,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder = nullptr,
        const std::shared_ptr<ExpectSpeechTimeoutHandler>& expectSpeechTimeoutHandler = nullptr);

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
     * audioProvider, which transitions it to the @c RECOGNIZING state. This function can be
     * called in any state except @c BUSY, however the flags in @c AudioProvider will dictate whether the call is
     * allowed to override an ongoing Recognize Event. If the flags do not allow an override, no event will be sent, no
     * state change will occur, and the function will fail.
     *
     * A special case is that the function will also fail if the keyword passed in is equal
     * to @c KEYWORD_TEXT_STOP. This check is case insensitive.
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
     * @attention User perceived latency metrics will only be accurate if the startOfSpeechTimestamp is correct. Some
     * keyword detectors determine start of speech at different times, and in some cases exclude the wakeword. This
     * leads to a later timestamp and excludes the wakeword duration from the user perceived latency calculation, thus
     * underestimating the latency. Verify that the startOfSpeechTimestamp is including the wakeword duration if the
     * audio stream is initiated by wakeword detection. (Tap-To-Talk remains unaffected)
     *
     * @param audioProvider The @c AudioProvider to stream audio from.
     * @param initiator The type of interface that initiated this recognize event.
     * @param startOfSpeechTimestamp Moment in time when user started talking to Alexa. This parameter is optional
     *     and it is used to measure user perceived latency. The startOfSpeechTimestamp must include the wakeword
     *     duration if the audio stream is initiated by a wakeword, otherwise the latency calculation will not be
     *     correct.
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
     * @param KWDMetadata Wake word engine metadata.
     * @param initiatorToken An optional opaque string associated with the interaction.
     * @return A future which is @c true if the Recognize Event was started successfully, else @c false.
     */
    std::future<bool> recognize(
        AudioProvider audioProvider,
        Initiator initiator,
        std::chrono::steady_clock::time_point startOfSpeechTimestamp = std::chrono::steady_clock::now(),
        avsCommon::avs::AudioInputStream::Index begin = INVALID_INDEX,
        avsCommon::avs::AudioInputStream::Index keywordEnd = INVALID_INDEX,
        std::string keyword = "",
        std::shared_ptr<const std::vector<char>> KWDMetadata = nullptr,
        const std::string& initiatorToken = "");

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
    void onResponseStatusReceived(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status status) override;
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
    void onFocusChanged(avsCommon::avs::FocusState newFocus, avsCommon::avs::MixingBehavior behavior) override;
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

    /// @name LocaleAssetsObserverInterface Functions
    /// @{
    void onLocaleAssetsChanged() override;
    /// @}

    /// @name InternetConnectionObserverInterface Functions
    /// @{
    void onConnectionStatusChanged(bool connected) override;
    /// @}

    /**
     * Gets the wake words events metadata.
     *
     * @return The wake words events metadata.
     */
    static settings::SettingEventMetadata getWakeWordsEventsMetadata();

    /**
     * Gets the wake word confirmation events metadata.
     *
     * @return The wake word confirmation event metadata.
     */
    static settings::SettingEventMetadata getWakeWordConfirmationMetadata();

    /**
     * Gets the speech confirmation events metadata.
     *
     * @return The speech confirmation event metadata.
     */
    static settings::SettingEventMetadata getSpeechConfirmationMetadata();

    /**
     * Set encoding for the audio format. The new encoding will be used for future utterances. Any audio stream already
     * in progress will not be affected. This is an alternative API to @c requestEncodingAudioFormats(), but will
     * configure @c AudioInputProcessor to only produce one audio stream.
     *
     * @param encoding The encoding format to use.
     * @return @true on success, @c false on failure to set the encoding.
     */
    bool setEncodingAudioFormat(avsCommon::utils::AudioFormat::Encoding encoding);

    /**
     * Function to request multiple audio streams from @c AudioInputProcessor in a single Recognize event. This is an
     * alternative API to @c setEncodingAudioFormat().
     * @param encodings A map of resolveKey to a pair of encoding formats. Each resolveKey stands for an audio stream.
     * The first encoding format is the requested format, and the second one is backup format if the requested one isn't
     * supported by @c AudioInputProcessor.
     * @return A map of resolveKey to result encoding format. If the requested format is supported, the result will be
     * the requested one. If not, @c AudioInputProcessor will check backup format. If neither of them is supported, the
     * corresponding resolve key will be removed from result.
     */
    EncodingFormatResponse requestEncodingAudioFormats(const EncodingFormatRequest& encodings);

    /**
     * Function to get encoding audio formats.
     * @return Encoding formats requested for multiple audio streams.
     */
    EncodingFormatResponse getEncodingAudioFormats() const;

private:
    /**
     * Inintialize audio input processor.
     *
     * @return @c true on success, @c false on failure to initialize
     */
    bool initialize();

    /**
     * Constructor.
     *
     * @param directiveSequencer The Directive Sequencer to register with for receiving directives.
     * @param messageSender The object to use for sending events.
     * @param contextManager The AVS Context manager used to generate system context for events.
     * @param focusManager The channel focus manager used to manage usage of the dialog channel.
     * @param exceptionEncounteredSender The object to use for sending ExceptionEncountered messages.
     * @param userInactivityMonitor The object to use for resetting user inactivity.
     * @param systemSoundPlayer The instance of the system sound player.
     * @param audioEncoder The Audio Encoder used to encode audio inputs. This parameter is optional and
     *     will disable the encoding feature if set to nullptr.
     * @param defaultAudioProvider A default @c avsCommon::AudioProvider to use for ExpectSpeech if the previous
     *     provider is not readable (@c AudioProvider::alwaysReadable).  This parameter is optional, and ignored if set
     *     to @c AudioProvider::null().
     * @param wakeWordConfirmation The wake word confirmation setting.
     * @param speechConfirmation The end of speech confirmation setting.
     * @param capabilityChangeNotifier The object with which to notify observers of @c AudioInputProcessor capability
     *     configurations change.
     * @param wakeWordsSetting The setting that represents the enabled wake words. This parameter is required if this
     * device supports wake words.
     * @param capabilitiesConfiguration The SpeechRecognizer capabilities configuration.
     * @param powerResourceManager Power Resource Manager.
     * @param metricRecorder The metric recorder.
     * @param expectSpeechTimeoutHandler An optional interface that applications may provide to specify external
     * handling of the @c ExpectSpeech directive's timeout. If provided, this function must remain valid for the
     * lifetime of the @c AudioInputProcessor.
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
        std::shared_ptr<avsCommon::sdkInterfaces::SystemSoundPlayerInterface> systemSoundPlayer,
        const std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>& assetsManager,
        std::shared_ptr<audioEncoderInterfaces::AudioEncoderInterface> audioEncoder,
        AudioProvider defaultAudioProvider,
        std::shared_ptr<settings::WakeWordConfirmationSetting> wakeWordConfirmation,
        std::shared_ptr<settings::SpeechConfirmationSetting> speechConfirmation,
        const std::shared_ptr<avsCommon::avs::CapabilityChangeNotifierInterface>& capabilityChangeNotifier,
        std::shared_ptr<settings::WakeWordsSetting> wakeWordsSetting,
        std::shared_ptr<avsCommon::avs::CapabilityConfiguration> capabilitiesConfiguration,
        std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface> powerResourceManager,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
        const std::shared_ptr<ExpectSpeechTimeoutHandler>& expectSpeechTimeoutHandler);

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
     * Performs operations to handle the failure of processing a directive.
     *
     * @param errorMessage A string containing the error message.
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param errorType The type of exception encountered.
     */
    void handleDirectiveFailure(
        const std::string& errorMessage,
        std::shared_ptr<DirectiveInfo> info,
        avsCommon::avs::ExceptionErrorType errorType);

    /**
     * @name Executor Thread Functions
     *
     * These functions (and only these functions) are called by @c m_executor on a single worker thread.  All other
     * functions in this class can be called asynchronously, and pass data to the @c Executor thread through parameters
     * to lambda functions.  No additional synchronization is needed.
     */
    /// @{

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
     * @param initiatorToken An optional opaque string associated with the interaction.
     * @return @c true if the Recognize Event was started successfully, else @c false.
     */
    bool executeRecognize(
        AudioProvider provider,
        Initiator initiator,
        std::chrono::steady_clock::time_point startOfSpeechTimestamp,
        avsCommon::avs::AudioInputStream::Index begin,
        avsCommon::avs::AudioInputStream::Index keywordEnd,
        const std::string& keyword,
        std::shared_ptr<const std::vector<char>> KWDMetadata,
        const std::string& initiatorToken);

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
     * @param end The @c Index in @c audioProvider.stream where the wakeword ends.
     * @param keyword The text of the keyword which was recognized.  This parameter is optional, and defaults to an
     *     empty string.  This parameter is ignored if initiator is not @c WAKEWORD.  The only value currently
     *     accepted by AVS for keyword is "ALEXA".  See
     *     https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/context#recognizerstate
     * @param KWDMetadata Wake word engine metadata.
     * @param initiatedByWakeword Whether the Initiator was Wakeword; false by default.
     * @param falseWakewordDetection Whether false Wakeword detection was enabled; false by default.
     * @param initiatorString - The @c Initiator string to be used to log a metric.
     * @return @c true if the Recognize Event was started successfully, else @c false.
     */
    bool executeRecognize(
        AudioProvider provider,
        const std::string& initiatorJson,
        std::chrono::steady_clock::time_point startOfSpeechTimestamp = std::chrono::steady_clock::now(),
        avsCommon::avs::AudioInputStream::Index begin = INVALID_INDEX,
        avsCommon::avs::AudioInputStream::Index end = INVALID_INDEX,
        const std::string& keyword = "",
        std::shared_ptr<const std::vector<char>> KWDMetadata = nullptr,
        bool initiatedByWakeword = false,
        bool falseWakewordDetection = false,
        const std::string& initiatorString = "");

    /**
     * This function receives the full system context from @c ContextManager.  Context requests are initiated by
     * @c executeRecognize() calls, and provide the final piece of information needed to assemble a @c MessageRequest.
     * If focus has already changed to @c FOREGROUND by the time this function is called, this function will send the
     * @c MessageRequest.  If focus has not changed to @c FOREGROUND, this function will assemble the MessageRequest,
     * but will defer sending it to @c executeOnFocusChanged().
     *
     * @param jsonContext The full system context to send with the event.
     */
    void executeOnContextAvailable(const std::string& jsonContext);

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
     * This function is called whenever disconnected is received with onConnectionStatusChanged event
     */
    void executeDisconnected();

    /**
     * This function is called whenever locale assets of @c LocaleAssetsManagerInterface are changed.
     */
    void executeOnLocaleAssetsChanged();

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

    /**
     * A helper function to handle the SetWakeWordConfirmation directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @return Whether the directive processing was successful.
     */
    bool handleSetWakeWordConfirmation(std::shared_ptr<DirectiveInfo> info);

    /**
     * A helper function to handle the SetSpeechConfirmation directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @return Whether the directive processing was successful.
     */
    bool handleSetSpeechConfirmation(std::shared_ptr<DirectiveInfo> info);

    /**
     * A helper function to handle the SetWakeWords directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @return Whether the directive processing was successful.
     */
    bool handleSetWakeWords(std::shared_ptr<DirectiveInfo> info);

    /**
     * Acquire power resource when listening and release power resource when finished listening.
     * @param newState The new state of AIP.
     */
    void managePowerResource(ObserverInterface::State newState);

    /**
     * Helper function to create @c MessageRequestResolveFunction for ongoing Recognize event.
     * @return MessageRequestResolver function with respect to current @c m_encodingAudioFormats, and attachment readers
     * @note This function is not thread-safe, caller should requires @c m_encodingFormatMutex for synchronization
     */
    avsCommon::avs::MessageRequest::MessageRequestResolveFunction getMessageRequestResolverLocked() const;

    /**
     * Helper function to close both KWD metadata and audio attachment readers in @c AudioInputProcessor regardless
     * how many streams it's configured to produce.
     * @param closePoint Close the reader immediately or after draining buffer
     * @note For the sake of thread safety, this helper function can only be called by @c m_executor thread
     */
    void closeAttachmentReaders(
        avsCommon::avs::attachment::AttachmentReader::ClosePoint closePoint =
            avsCommon::avs::attachment::AttachmentReader::ClosePoint::AFTER_DRAINING_CURRENT_BUFFER);

    /**
     * Helper function to check whether provided encoding audio format is supported by audio stream provider or encoder.
     * @param encodingFormat Encoding format to check.
     * @return @c true if it's supported, else @c false
     */
    bool isEncodingFormatSupported(avsCommon::utils::AudioFormat::Encoding encodingFormat) const;

    /**
     * Helper function to indicate if audio encoder is being used.
     * @return @c true if encoder is used, else @c false
     * @note This function is not thread-safe, caller should requires @c m_encodingFormatMutex for synchronization
     */
    bool isUsingEncoderLocked() const;

    /**
     * Helper function to indicate if @c AudioInputProcessor is configured to produce multiple audio streams
     * @return @c true if multiple streams are being requested, else @c false
     * @note This function is not thread-safe, caller should acquire @c m_encodingFormatMutex for synchronization
     */
    bool multiStreamsRequestedLocked() const;

    /// The metric recorder.
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> m_metricRecorder;

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
    const std::shared_ptr<audioEncoderInterfaces::AudioEncoderInterface> m_encoder;

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
     * The attachment readers which are currently being used to stream audio for a Recognize event. These readers are
     * valid during the @c RECOGNIZING state, and are retained by @c AudioInputProcessor so that they can close the
     * stream from @c executeStopCapture(). These attachment readers are grouped by resolve key, which aims to support
     * multiple audio stream use case. Attachment readers include both KWD metadata and audio stream, and they're
     * ordered properly in the vector, and ready to use to assemble @c MessageRequest.
     * @note For the sake of thread safety, this member data should only be updated by @c m_executor thread.
     */
    std::unordered_map<std::string, std::vector<std::shared_ptr<avsCommon::avs::MessageRequest::NamedReader>>>
        m_attachmentReaders;

    /**
     * The payload for a Recognize event.  This string is populated by a call to @c executeRecognize(), and later
     * consumed by a call to @c executeOnContextAvailable() when the context arrives and the full @c MessageRequest can
     * be assembled.  This string is only relevant during the @c RECOGNIZING state.
     */
    std::string m_recognizePayload;

    /**
     * The @c MessageRequest for a Recognize event.  This request is created by a call to
     * @c executeOnContextAvailable(), and either sent immediately (if `m_focusState == afml::FocusState::FOREGROUND`),
     * or later sent by a call to @c executeOnFocusChanged().  This pointer is only valid during the @c RECOGNIZING
     * state after a call to @c executeRecognize(), and is reset after it is sent.
     */
    std::shared_ptr<avsCommon::avs::MessageRequest> m_recognizeRequest;

    /// The @c MessageRequest for the most recent Recognize event sent with the @c MessageSender.
    std::shared_ptr<avsCommon::avs::MessageRequest> m_recognizeRequestSent;

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
     * This flag indicates if the event stream is closed while the device is still in @c RECOGNIZING.
     */
    bool m_streamIsClosedInRecognizingState;

    /// The system sound player.
    std::shared_ptr<avsCommon::sdkInterfaces::SystemSoundPlayerInterface> m_systemSoundPlayer;

    /// The locale assets manager.
    std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface> m_assetsManager;

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

    /// The wake word confirmation setting.
    std::shared_ptr<settings::WakeWordConfirmationSetting> m_wakeWordConfirmation;

    /// The end of speech confirmation setting.
    std::shared_ptr<settings::SpeechConfirmationSetting> m_speechConfirmation;

    /// The object to notify of @c AudioInputProcessor capability configurations change.
    std::shared_ptr<avsCommon::avs::CapabilityChangeNotifierInterface> m_capabilityChangeNotifier;

    /// The wake words setting. This field is optional and it is only used if the device supports wake words.
    std::shared_ptr<settings::WakeWordsSetting> m_wakeWordsSetting;

    /// The power resource manager
    std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface> m_powerResourceManager;

    /// StopCapture received time
    std::chrono::steady_clock::time_point m_stopCaptureReceivedTime;

    /**
     * An optional interface that applications may provide to specify external handling of the @c ExpectSpeech
     * directive's timeout.
     */
    std::shared_ptr<ExpectSpeechTimeoutHandler> m_expectSpeechTimeoutHandler;

    /**
     * Temporary value of dialogRequestId generated when onRecognize starts. This should be cleared after being
     * passed to the directive sequencer.
     */
    std::string m_preCachedDialogRequestId;

    /**
     * Value that will contain the time since last wake from suspend when AIP acquires the wakelock.
     */
    std::chrono::milliseconds m_timeSinceLastResumeMS;

    /**
     * Value that will contain the time since last partial LPM state change when AIP acquires the wakelock.
     */
    std::chrono::milliseconds m_timeSinceLastPartialMS;

    /**
     * Value that will contain the resource type since last partial LPM state change when AIP acquires the wakelock.
     */
    avsCommon::sdkInterfaces::PowerResourceManagerInterface::PartialStateBitSet m_resourceFlags;

    /**
     * Value to indicate if audio encoder is being used.
     */
    bool m_usingEncoder;

    /**
     * Mutex to serialize access to m_capabilityConfigurations.
     */
    std::mutex m_mutex;

    /**
     * Resolve function to pass to @c MessageRequest when multiple audio streams are requested.
     */
    avsCommon::avs::MessageRequest::MessageRequestResolveFunction m_messageRequestResolver;

    /**
     * Encoding formats used to encode audio streams. This is a map from resolveKey to corresponding encoding format. If
     * there is only one format in the map, @c AudioInputProcess will send resolved @c MessageRequest, and the
     * resolveKey will be ignored. If there are multiple formats in the map, @c AudioInputProcessor will send unresolved
     * @c MessageRequest with audio streams of all provided formats, and user needs to use the corresponding resolveKey
     * to resolve the @c MessageRequest to a valid one containing only the audio with the right format.
     */
    EncodingFormatResponse m_encodingAudioFormats;

    /**
     * Mutex to synchronize updates to @c m_encodingAudioFormats.
     */
    mutable std::mutex m_encodingFormatMutex;

    /**
     * The number of audio bytes sent to the cloud that we will trigger the wake word upload metric.
     */
    unsigned int m_audioBytesForMetricThreshold;

    /**
     * The name of the wake word upload metric.
     */
    std::string m_uploadMetricName;

    /**
     * The Duration Builder used to calculate the duration of the fetch context call.
     */
    avsCommon::utils::metrics::DataPointDurationBuilder m_fetchContextTimeMetricData;

    /// A @c PowerResourceId used for wakelock logic.
    std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface::PowerResourceId> m_powerResourceId;

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
