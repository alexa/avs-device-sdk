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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_INTERACTIONMANAGER_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_INTERACTIONMANAGER_H_

#include <memory>

#include <Audio/MicrophoneInterface.h>
#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <DefaultClient/DefaultClient.h>
#include <ESP/ESPDataModifierInterface.h>
#include <RegistrationManager/CustomerDataManager.h>

#include "KeywordObserver.h"
#include "UIManager.h"

#include "GuiRenderer.h"

#ifdef ENABLE_PCC
#include "PhoneCaller.h"
#endif

namespace alexaClientSDK {
namespace sampleApp {

/**
 * This class manages most of the user interaction by taking in commands and notifying the DefaultClient and the
 * userInterface (the view) accordingly.
 */
class InteractionManager
        : public avsCommon::sdkInterfaces::DialogUXStateObserverInterface
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
        capabilityAgents::aip::AudioProvider holdToTalkAudioProvider,
        capabilityAgents::aip::AudioProvider tapToTalkAudioProvider,
        std::shared_ptr<sampleApp::GuiRenderer> guiRenderer = nullptr,
        capabilityAgents::aip::AudioProvider wakeWordAudioProvider = capabilityAgents::aip::AudioProvider::null(),
        std::shared_ptr<esp::ESPDataProviderInterface> espProvider = nullptr,
        std::shared_ptr<esp::ESPDataModifierInterface> espModifier = nullptr,
        std::shared_ptr<avsCommon::sdkInterfaces::CallManagerInterface> callManager = nullptr);

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
     * Should be called whenever a user requests 'LOCALE' change.
     */
    void locale();

    /**
     * Should be called whenever a user requests 'DO_NOT_DISTURB' change.
     */
    void doNotDisturb();

    /**
     * Should be called whenever a user presses invalid option.
     */
    void errorValue();

    /**
     * Should be called when setting value is selected by the user.
     */
    void changeSetting(const std::string& key, const std::string& value);

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
    void adjustVolume(avsCommon::sdkInterfaces::SpeakerInterface::Type type, int8_t delta);

    /**
     * Should be called after a user wishes to set mute.
     */
    void setMute(avsCommon::sdkInterfaces::SpeakerInterface::Type type, bool mute);

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

    /**
     * Should be called whenever a user requests for ESP control.
     */
    void espControl();

    /**
     * Should be called whenever a user requests to toggle the ESP support.
     */
    void toggleESPSupport();

    /**
     * Should be called whenever a user requests to set the @c voiceEnergy sent in ReportEchoSpatialPerceptionData
     * event.
     *
     * @param voiceEnergy The voice energy measurement to be set as the ESP measurement.
     */
    void setESPVoiceEnergy(const std::string& voiceEnergy);

    /**
     * Should be called whenever a user requests set the @c ambientEnergy sent in ReportEchoSpatialPerceptionData
     * event.
     *
     * @param ambientEnergy The ambient energy measurement to be set as the ESP measurement.
     */
    void setESPAmbientEnergy(const std::string& ambientEnergy);

    /**
     * Grants the user access to the communications controls.
     */
    void commsControl();

    /**
     * Should be called when the user wants to accept a call.
     */
    void acceptCall();

    /**
     * Should be called when the user wants to stop a call.
     */
    void stopCall();

    /**
     * UXDialogObserverInterface methods
     */
    void onDialogUXStateChanged(DialogUXState newState) override;

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

    /**
     * Sets the do not disturb mode state.
     */
    void setDoNotDisturbMode(bool enable);

private:
    /// The default SDK client.
    std::shared_ptr<defaultClient::DefaultClient> m_client;

    /// The microphone managing object.
    std::shared_ptr<applicationUtilities::resources::audio::MicrophoneInterface> m_micWrapper;

    /// The user interface manager.
    std::shared_ptr<sampleApp::UIManager> m_userInterface;

    /// The gui renderer.
    std::shared_ptr<sampleApp::GuiRenderer> m_guiRenderer;

    /// The ESP provider.
    std::shared_ptr<esp::ESPDataProviderInterface> m_espProvider;

    /// The ESP modifier.
    std::shared_ptr<esp::ESPDataModifierInterface> m_espModifier;

    /// The call manager.
    std::shared_ptr<avsCommon::sdkInterfaces::CallManagerInterface> m_callManager;

#ifdef ENABLE_PCC
    /// The Phone Caller
    std::shared_ptr<sampleApp::PhoneCaller> m_phoneCaller;
#endif

    /// The hold to talk audio provider.
    capabilityAgents::aip::AudioProvider m_holdToTalkAudioProvider;

    /// The tap to talk audio provider.
    capabilityAgents::aip::AudioProvider m_tapToTalkAudioProvider;

    /// The wake word audio provider.
    capabilityAgents::aip::AudioProvider m_wakeWordAudioProvider;

    /// Whether a hold is currently occurring.
    bool m_isHoldOccurring;

    /// Whether a tap is currently occurring.
    bool m_isTapOccurring;

    /// Whether the microphone is currently turned on.
    bool m_isMicOn;

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
