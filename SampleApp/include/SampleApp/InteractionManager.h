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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_INTERACTIONMANAGER_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_INTERACTIONMANAGER_H_

#include <memory>

#include <Audio/MicrophoneInterface.h>
#include <AVSCommon/SDKInterfaces/Diagnostics/AudioInjectorInterface.h>
#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/CallStateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/Diagnostics/DiagnosticsInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>
#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <DefaultClient/DefaultClient.h>
#include <RegistrationManager/CustomerDataManager.h>
#include <Settings/SpeechConfirmationSettingType.h>
#include <Settings/Types/NetworkInfo.h>
#include <Settings/WakeWordConfirmationSettingType.h>

#include "KeywordObserver.h"
#include "GuiRenderer.h"
#include "UIManager.h"

#ifdef ENABLE_PCC
#include "PhoneCaller.h"
#endif

#ifdef ENABLE_MCC
#include "CalendarClient.h"
#include "MeetingClient.h"
#endif

#ifdef POWER_CONTROLLER
#include "PeripheralEndpoint/PeripheralEndpointPowerControllerHandler.h"
#endif

#ifdef TOGGLE_CONTROLLER
#include "PeripheralEndpoint/PeripheralEndpointToggleControllerHandler.h"
#endif

#ifdef RANGE_CONTROLLER
#include "PeripheralEndpoint/PeripheralEndpointRangeControllerHandler.h"
#endif

#ifdef MODE_CONTROLLER
#include "PeripheralEndpoint/PeripheralEndpointModeControllerHandler.h"
#endif

namespace alexaClientSDK {
namespace sampleApp {

/**
 * This class manages most of the user interaction by taking in commands and notifying the DefaultClient and the
 * userInterface (the view) accordingly.
 */
class InteractionManager
        : public avsCommon::sdkInterfaces::DialogUXStateObserverInterface
        , public avsCommon::sdkInterfaces::CallStateObserverInterface
        , public avsCommon::utils::RequiresShutdown {
public:
    /**
     * Constructor.
     */
    InteractionManager(
        std::shared_ptr<defaultClient::DefaultClient> client,
        std::shared_ptr<applicationUtilities::resources::audio::MicrophoneInterface> micWrapper,
        std::shared_ptr<sampleApp::UIManager> userInterface,
#ifdef ENABLE_PCC
        std::shared_ptr<sampleApp::PhoneCaller> phoneCaller,
#endif
#ifdef ENABLE_MCC
        std::shared_ptr<sampleApp::MeetingClient> meetingClient,
        std::shared_ptr<sampleApp::CalendarClient> calendarClient,
#endif
        capabilityAgents::aip::AudioProvider holdToTalkAudioProvider,
        capabilityAgents::aip::AudioProvider tapToTalkAudioProvider,
        std::shared_ptr<sampleApp::GuiRenderer> guiRenderer = nullptr,
        capabilityAgents::aip::AudioProvider wakeWordAudioProvider = capabilityAgents::aip::AudioProvider::null(),
#ifdef POWER_CONTROLLER
        std::shared_ptr<PeripheralEndpointPowerControllerHandler> powerControllerHandler = nullptr,
#endif
#ifdef TOGGLE_CONTROLLER
        std::shared_ptr<PeripheralEndpointToggleControllerHandler> toggleControllerHandler = nullptr,
#endif
#ifdef RANGE_CONTROLLER
        std::shared_ptr<PeripheralEndpointRangeControllerHandler> rangeControllerHandler = nullptr,
#endif
#ifdef MODE_CONTROLLER
        std::shared_ptr<PeripheralEndpointModeControllerHandler> modeControllerHandler = nullptr,
#endif
        std::shared_ptr<avsCommon::sdkInterfaces::CallManagerInterface> callManager = nullptr,
        std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface> diagnostics = nullptr);

    /**
     * Begins the interaction between the Sample App and the user. This should only be called at startup.
     */
    void begin();

    /**
     * Should be called when a user requests help.
     */
    void help();

    /**
     * Should be called when a user requests help and the application failed to connect to AVS.
     */
    void limitedHelp();

    /**
     * Toggles the microphone state if the Sample App was built with wakeword. When the microphone is turned off, the
     * app enters a privacy mode in which it stops recording audio data from the microphone, thus disabling Alexa waking
     * up due to wake word. Note however that hold-to-talk and tap-to-talk modes will still work by recording
     * microphone data temporarily until a user initiated interacion is complete. If this app was built without wakeword
     * then this will do nothing as the microphone is already off.
     */
    void microphoneToggle();

    /**
     * Should be called whenever a user presses or releases the hold button.
     */
    void holdToggled();

    /**
     * Should be called whenever a user presses and releases the tap button.
     */
    void tap();

    /**
     * Acts as a "stop" button. This stops whatever has foreground focus.
     */
    void stopForegroundActivity();

    /**
     * Should be called whenever a user presses 'PLAY' for playback.
     */
    void playbackPlay();

    /**
     * Should be called whenever a user presses 'PAUSE' for playback.
     */
    void playbackPause();

    /**
     * Should be called whenever a user presses 'NEXT' for playback.
     */
    void playbackNext();

    /**
     * Should be called whenever a user presses 'PREVIOUS' for playback.
     */
    void playbackPrevious();

    /**
     * Should be called whenever a user presses 'SKIP_FORWARD' for playback.
     */
    void playbackSkipForward();

    /**
     * Should be called whenever a user presses 'SKIP_BACKWARD' for playback.
     */
    void playbackSkipBackward();

    /**
     * Should be called whenever a user presses 'SHUFFLE' for playback.
     */
    void playbackShuffle();

    /**
     * Should be called whenever a user presses 'LOOP' for playback.
     */
    void playbackLoop();

    /**
     * Should be called whenever a user presses 'REPEAT' for playback.
     */
    void playbackRepeat();

    /**
     * Should be called whenever a user presses 'THUMBS_UP' for playback.
     */
    void playbackThumbsUp();

    /**
     * Should be called whenever a user presses 'THUMBS_DOWN' for playback.
     */
    void playbackThumbsDown();

    /**
     * Should be called whenever a user presses 'SETTINGS' for settings options.
     */
    void settings();

    /**
     * Should be called whenever a user requests 'ALARM_VOLUME_RAMP' change.
     */
    void alarmVolumeRamp();

    /**
     * Should be called whenever a user requests 'WAKEWORD_CONFIRMATION' change.
     */
    void wakewordConfirmation();

    /**
     * Should be called whenever a user requests 'SPEECH_CONFIRMATION' change.
     */
    void speechConfirmation();

    /**
     * Should be called whenever a user requests 'LOCALE' change.
     */
    void locale();

    /**
     * Resets cached endpoint identifiers.
     */
    void clearCachedEndpointIdentifiers(
        const std::vector<avsCommon::sdkInterfaces::endpoints::EndpointIdentifier>& deletedEndpoints);

#ifdef ENABLE_ENDPOINT_CONTROLLERS
    /**
     * Should be called whenever a user requests dynamic endpoint modification options.
     */
    void endpointModification();

    /**
     * Builds and dynamically registers an endpoint with the given friendlyName.
     * @param friendlyName
     * @return Whether building and enqueuing the endpoint for registration succeeded.
     * The CapabilitiesDelegate observer callback will indicate whether registration with AVS succeeded.
     */
    bool addEndpoint(const std::string& friendlyName);

    /**
     * Adds an endpoint.
     */
    void addDynamicEndpoint();

    /**
     * Modifies an endpoint.
     */
    void modifyDynamicEndpoint();

    /**
     * Deletes an endpoint.
     */
    void deleteDynamicEndpoint();

    /**
     * Should be called whenever a user presses 'ENDPOINT_CONTROLLER' for endpoint point controller options.
     */
    void endpointController();
#endif

#ifdef POWER_CONTROLLER
    /**
     * Should be called whenever a user requests 'POWER CONTROLLER' options.
     */
    void powerController();

#endif
#ifdef TOGGLE_CONTROLLER
    /**
     * Should be called whenever a user requests 'TOGGLE CONTROLLER' options.
     */
    void toggleController();
#endif

#ifdef MODE_CONTROLLER
    /**
     * Should be called whenever a user requests 'MODE CONTROLLER' options.
     */
    void modeController();
#endif

#ifdef RANGE_CONTROLLER
    /**
     * Should be called whenever a user requests 'RANGE CONTROLLER' options.
     */
    void rangeController();
#endif

    /**
     * Should be called whenever a user requests 'TIMEZONE' change.
     */
    void timeZone();

    /**
     * Should be called whenever a user requests 'NETWORK_INFO' change.
     */
    void networkInfo();

    /// @name Network Info Prompt Functions
    /// Should be called whenever a user requests a specific 'NETWORK_INFO' change.
    /// @{
    void networkInfoConnectionTypePrompt();
    void networkInfoESSIDPrompt();
    void networkInfoBSSIDPrompt();
    void networkInfoIpPrompt();
    void networkInfoSubnetPrompt();
    void networkInfoMacPrompt();
    void networkInfoDHCPPrompt();
    void networkInfoStaticIpPrompt();
    /// @}

    /**
     * Should be called whenever a user requests 'DO_NOT_DISTURB' change.
     */
    void doNotDisturb();

    /**
     * Should be called whenever a user presses invalid option.
     */
    void errorValue();

    /**
     * Should be called whenever a users requests 'SPEAKER_CONTROL' for speaker control.
     */
    void speakerControl();

    /**
     * Should be called whenever a users requests to set the firmware version.
     */
    void firmwareVersionControl();

    /**
     * Update the firmware version.
     *
     * @param firmwareVersion The new firmware version.
     */
    void setFirmwareVersion(avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion);

    /**
     * Should be called after a user selects a speaker.
     */
    void volumeControl();

    /**
     * Should be called after a user wishes to modify the volume.
     */
    void adjustVolume(avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type, int8_t delta);

    /**
     * Should be called after a user wishes to set mute.
     */
    void setMute(avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type, bool mute);

    /**
     * Reset the device and remove any customer data.
     */
    void resetDevice();

    /**
     * Prompts the user to confirm the intent to reset the device.
     */
    void confirmResetDevice();

    /**
     * Prompts the user to confirm the intent to re-authorize the device.
     */
    void confirmReauthorizeDevice();

#ifdef ENABLE_COMMS
    /**
     * Grants the user access to the communications controls.
     */
    void commsControl();

    /**
     * Should be called when the user wants to accept a call.
     */
    void acceptCall();

    /**
     * Send dtmf tones during the call.
     *
     * @param dtmfTone The signal of the dtmf message.
     */
    void sendDtmf(avsCommon::sdkInterfaces::CallManagerInterface::DTMFTone dtmfTone);

    /**
     * Should be called whenever collecting a dtmf.
     */
    void dtmfControl();

    /**
     * Should be called whenever a user presses invalid dtmf.
     */
    void errorDtmf();

    /**
     * Should be called when the user wants to stop a call.
     */
    void stopCall();

    /**
     * Should be called when the user wants to mute/unmute a call.
     */
    void muteCallToggle();
#endif

    /**
     * UXDialogObserverInterface methods
     */
    void onDialogUXStateChanged(DialogUXState newState) override;

    /**
     * CallStateObserverInterface methods
     */
    void onCallStateChange(CallState newState) override;

#ifdef ENABLE_PCC
    /**
     * Should be called whenever a user selects Phone Control.
     */
    void phoneControl();

    /**
     * Should be called whenever collecting a Call Id.
     */
    void callId();

    /**
     * Should be called whenever collecting a Caller Id.
     */
    void callerId();

    /**
     * PhoneCallController commands
     */
    void sendCallActivated(const std::string& callId);
    void sendCallTerminated(const std::string& callId);
    void sendCallFailed(const std::string& callId);
    void sendCallReceived(const std::string& callId, const std::string& callerId);
    void sendCallerIdReceived(const std::string& callId, const std::string& callerId);
    void sendInboundRingingStarted(const std::string& callId);
    void sendOutboundCallRequested(const std::string& callId);
    void sendOutboundRingingStarted(const std::string& callId);
    void sendSendDtmfSucceeded(const std::string& callId);
    void sendSendDtmfFailed(const std::string& callId);

#endif

#ifdef ENABLE_MCC
    /**
     * Should be called whenever a user selects Phone Control.
     */
    void meetingControl();

    /**
     * Should be called whenever collecting a Call Id.
     */
    void sessionId();

    /**
     * Should be called whenever collecting a path to Calendar Items file.
     */
    void calendarItemsFile();

    /**
     * MeetingClientController commands.
     */
    void sendMeetingJoined(const std::string& sessionId);
    void sendMeetingEnded(const std::string& sessionId);
    void sendSetCurrentMeetingSession(const std::string& sessionId);
    void sendClearCurrentMeetingSession();
    void sendConferenceConfigurationChanged();
    void sendMeetingClientErrorOccured(const std::string& sessionId);
    void sendCalendarItemsRetrieved(const std::string& calendarItemsFile);
    void sendCalendarClientErrorOccured();
#endif

    /**
     * Sets the do not disturb mode state.
     */
    void setDoNotDisturbMode(bool enable);

    /**
     * Sets the Alarm Volume Ramp state.
     */
    void setAlarmVolumeRamp(bool enable);

    /**
     * Sets the speech confirmation state.
     */
    void setSpeechConfirmation(settings::SpeechConfirmationSettingType value);

    /**
     * Sets the wake word confirmation state.
     */
    void setWakewordConfirmation(settings::WakeWordConfirmationSettingType value);

    /**
     * Sets the time zone of the device.
     */
    void setTimeZone(const std::string& value);

    /**
     * Sets the locale of the device.
     */
    void setLocale(const settings::DeviceLocales& value);

    /**
     * Gets the network info of the device.
     * @return The network info.
     */
    settings::types::NetworkInfo getNetworkInfo();

    /**
     * Sets the network info of the device.
     * @param value The network info being set.
     */
    void setNetworkInfo(const settings::types::NetworkInfo& value);

#ifdef POWER_CONTROLLER
    /**
     * Sets the power state on power handler.
     *
     * @param powerState Power state to be set.
     */
    void setPowerState(const bool powerState);
#endif

#ifdef TOGGLE_CONTROLLER
    /**
     * Sets the toggle state on toggle handler.
     *
     * @param toggleState Toggle state to be set.
     */
    void setToggleState(const bool toggleState);
#endif

#ifdef RANGE_CONTROLLER
    /**
     * Sets the range on range handler.
     *
     * @param rangeValue A range to be set.
     */
    void setRangeValue(const int rangeValue);
#endif

#ifdef MODE_CONTROLLER
    /**
     * Sets the mode on mode handler.
     *
     * @param mode The mode to be set.
     */
    void setMode(const std::string mode);
#endif

    /**
     * Stop the microphone from streaming audio data.
     */
    void stopMicrophone();

    /**
     * Start streaming audio data from the microphone.
     */
    void startMicrophone();

    /**
     * Prints the diagnostics screen.
     */
    void diagnosticsControl();

    /**
     * Prints the device properties screen.
     */
    void devicePropertiesControl();

    /**
     * Prints a requested device property.
     */
    void showDeviceProperties();

    /**
     * Prints the audio injection screen.
     */
    void audioInjectionControl();

    /**
     * Injects wav file into audio stream.
     *
     * Note: Currently audio injection is supported for wav files with the following properties
     * Sample Size: 16 bits
     * Sample Rate: 16 KHz
     * Number of channels : 1
     * Endianness : Little
     * Encoding Format : LPCM
     *
     * @param absoluteFilePath the absolute filepath of the wav file containing the audio to be injected.
     */
    void injectWavFile(const std::string& absoluteFilePath);

    /**
     * Prints device protocol tracer screen.
     */
    void deviceProtocolTraceControl();

    /**
     * Prints the protocol trace string.
     */
    void printProtocolTrace();

    /**
     * Enables the protocol trace utility.
     *
     * @param enabled flag indicating if protocol trace is enabled/disabled.
     */
    void setProtocolTraceFlag(bool enabled);

    /**
     * Clears the protocol trace message list.
     */
    void clearProtocolTrace();

private:
    /// The default SDK client.
    std::shared_ptr<defaultClient::DefaultClient> m_client;

    /// The microphone managing object.
    std::shared_ptr<applicationUtilities::resources::audio::MicrophoneInterface> m_micWrapper;

    /// The user interface manager.
    std::shared_ptr<sampleApp::UIManager> m_userInterface;

    /// The gui renderer.
    std::shared_ptr<sampleApp::GuiRenderer> m_guiRenderer;

    /// The call manager.
    std::shared_ptr<avsCommon::sdkInterfaces::CallManagerInterface> m_callManager;

#ifdef ENABLE_PCC
    /// The Phone Caller
    std::shared_ptr<sampleApp::PhoneCaller> m_phoneCaller;
#endif

#ifdef ENABLE_MCC
    /// The Meeting Client.
    std::shared_ptr<sampleApp::MeetingClient> m_meetingClient;
    /// The Calendar Client.
    std::shared_ptr<sampleApp::CalendarClient> m_calendarClient;
#endif

    /// The hold to talk audio provider.
    capabilityAgents::aip::AudioProvider m_holdToTalkAudioProvider;

    /// The tap to talk audio provider.
    capabilityAgents::aip::AudioProvider m_tapToTalkAudioProvider;

    /// The wake word audio provider.
    capabilityAgents::aip::AudioProvider m_wakeWordAudioProvider;

#ifdef POWER_CONTROLLER
    /// The Power Controller Handler
    std::shared_ptr<PeripheralEndpointPowerControllerHandler> m_powerControllerHandler;
#endif

#ifdef TOGGLE_CONTROLLER
    /// The Toggle Controller Handler
    std::shared_ptr<PeripheralEndpointToggleControllerHandler> m_toggleControllerHandler;
#endif

#ifdef RANGE_CONTROLLER
    /// The Range Controller Handler
    std::shared_ptr<PeripheralEndpointRangeControllerHandler> m_rangeControllerHandler;
#endif

#ifdef MODE_CONTROLLER
    /// The Mode Controller Handler
    std::shared_ptr<PeripheralEndpointModeControllerHandler> m_modeControllerHandler;
#endif

    /// Whether a hold is currently occurring.
    bool m_isHoldOccurring;

    /// Whether a tap is currently occurring.
    bool m_isTapOccurring;

    /// Whether a call is currently connected.
    bool m_isCallConnected;

    /// Whether the microphone is currently on.
    bool m_isMicOn;

    /// Optional dynamic endpointId.
    avsCommon::utils::Optional<avsCommon::sdkInterfaces::endpoints::EndpointIdentifier> m_dynamicEndpointId;

#ifdef ENABLE_ENDPOINT_CONTROLLERS
    /// Whether to toggle the dynamic endpoint's friendly name.
    bool m_friendlyNameToggle;
#endif

    /// Diagnostics object.
    std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface> m_diagnostics;

    /**
     * An internal executor that performs execution of callable objects passed to it sequentially but asynchronously.
     */
    avsCommon::utils::threading::Executor m_executor;

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /// sends Gui Toggle event
    void sendGuiToggleEvent(const std::string& toggleName, avsCommon::avs::PlaybackToggle toggleType);
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_INTERACTIONMANAGER_H_
