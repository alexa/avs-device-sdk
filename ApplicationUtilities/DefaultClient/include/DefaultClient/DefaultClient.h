/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_DEFAULTCLIENT_H_
#define ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_DEFAULTCLIENT_H_

#include <ACL/AVSConnectionManager.h>
#include <ACL/Transport/MessageRouter.h>
#include <ACL/Transport/PostConnectSynchronizer.h>
#include <ACL/Transport/PostConnectSynchronizerFactory.h>
#include <ADSL/DirectiveSequencer.h>
#include <AFML/AudioActivityTracker.h>
#include <AFML/FocusManager.h>
#include <AFML/VisualActivityTracker.h>
#include <AIP/AudioInputProcessor.h>
#include <AIP/AudioProvider.h>
#include <Alerts/AlertsCapabilityAgent.h>
#include <Alerts/Renderer/Renderer.h>
#include <Alerts/Storage/AlertStorageInterface.h>
#include <AudioPlayer/AudioPlayer.h>
#include <AVSCommon/AVS/DialogUXStateAggregator.h>
#include <AVSCommon/AVS/ExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/Audio/AudioFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/Audio/EqualizerConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/Audio/EqualizerModeControllerInterface.h>
#include <AVSCommon/SDKInterfaces/Audio/EqualizerStorageInterface.h>
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/AudioPlayerObserverInterface.h>
#include <AVSCommon/SDKInterfaces/CallManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesObserverInterface.h>
#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/SingleSettingObserverInterface.h>
#include <AVSCommon/SDKInterfaces/TemplateRuntimeObserverInterface.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/Network/InternetConnectionMonitor.h>
#include <Bluetooth/Bluetooth.h>
#include <Bluetooth/BluetoothStorageInterface.h>
#include <CertifiedSender/CertifiedSender.h>
#include <CertifiedSender/SQLiteMessageStorage.h>
#include <DoNotDisturbCA/DoNotDisturbCapabilityAgent.h>
#include <Equalizer/EqualizerCapabilityAgent.h>
#include <EqualizerImplementations/EqualizerController.h>
#include <ExternalMediaPlayer/ExternalMediaPlayer.h>
#include <InteractionModel/InteractionModelCapabilityAgent.h>
#include <MRM/MRMCapabilityAgent.h>
#include <Notifications/NotificationsCapabilityAgent.h>
#include <Notifications/NotificationRenderer.h>

#ifdef ENABLE_PCC
#include <AVSCommon/SDKInterfaces/Phone/PhoneCallerInterface.h>
#include <PhoneCallController/PhoneCallController.h>
#endif

#include <PlaybackController/PlaybackController.h>
#include <PlaybackController/PlaybackRouter.h>
#include <RegistrationManager/RegistrationManager.h>
#include <Settings/DeviceSettingsManager.h>
#include <Settings/Storage/DeviceSettingStorageInterface.h>
#include <Settings/Settings.h>
#include <Settings/SettingsStorageInterface.h>
#include <Settings/SettingsUpdatedEventSender.h>
#include <SpeakerManager/SpeakerManager.h>
#include <SpeechSynthesizer/SpeechSynthesizer.h>
#include <System/SoftwareInfoSender.h>
#include <System/UserInactivityMonitor.h>
#include <TemplateRuntime/TemplateRuntime.h>

#ifdef ENABLE_REVOKE_AUTH
#include <System/RevokeAuthorizationHandler.h>
#endif

#include "EqualizerRuntimeSetup.h"

namespace alexaClientSDK {
namespace defaultClient {

/**
 * This class serves to instantiate each default component with of the SDK with no specializations to provide an
 * "out-of-box" component that users may utilize for AVS interaction.
 */
class DefaultClient : public avsCommon::sdkInterfaces::CapabilitiesObserverInterface {
public:
    /// A reserved index value which is considered invalid.
    static const auto INVALID_INDEX = capabilityAgents::aip::AudioInputProcessor::INVALID_INDEX;

    /**
     * Creates and initializes a default AVS SDK client. To connect the client to AVS, users should make a call to
     * connect() after creation.
     *
     * @param deviceInfo DeviceInfo which reflects the device setup credentials.
     * @param customerDataManager CustomerDataManager instance to be used by RegistrationManager and instances of
     * all classes extending CustomDataHandler.
     * @param externalMusicProviderMediaPlayers The map of <players, mediaPlayer> to use to play content from each
     * external music provider.
     * @param externalMusicProviderSpeakers The map of <players, speaker> to use to track volume of each
     * external music provider media player.
     * @param adapterCreationMap The map of <players, adapterCreationMethod> to use when creating the adapters for the
     * different music providers supported by ExternalMediaPlayer.
     * @param speakMediaPlayer The media player to use to play Alexa speech from.
     * @param audioMediaPlayer The media player to use to play Alexa audio content from.
     * @param alertsMediaPlayer The media player to use to play alerts from.
     * @param notificationsMediaPlayer The media player to play notification indicators.
     * @param bluetoothMediaPlayer The media player to play bluetooth content.
     * @param ringtoneMediaPlayer The media player to play Comms ringtones.
     * @param speakSpeaker The speaker to control volume of Alexa speech.
     * @param audioSpeaker The speaker to control volume of Alexa audio content.
     * @param alertsSpeaker The speaker to control volume of alerts.
     * @param notificationsSpeaker The speaker to control volume of notifications.
     * @param bluetoothSpeaker The speaker to control volume of bluetooth.
     * @param ringtoneSpeaker The speaker to control volume of Comms ringtones.
     * @param additionalSpeakers A list of additional speakers to receive volume changes.
     * @param equalizerRuntimeSetup Equalizer component runtime setup
     * @param audioFactory The audioFactory is a component that provides unique audio streams.
     * @param authDelegate The component that provides the client with valid LWA authorization.
     * @param alertStorage The storage interface that will be used to store alerts.
     * @param messageStorage The storage interface that will be used to store certified sender messages.
     * @param notificationsStorage The storage interface that will be used to store notification indicators.
     * @param settingsStorage The storage interface that will be used to store settings (DEPRECATED).
     * @param deviceSettingStorage The storage interface that will be used to store device settings.
     * @param bluetoothStorage The storage interface that will be used to store bluetooth data.
     * @param alexaDialogStateObservers Observers that can be used to be notified of Alexa dialog related UX state
     * changes.
     * @param connectionObservers Observers that can be used to be notified of connection status changes.
     * @param isGuiSupported Whether the device supports GUI.
     * @param capabilitiesDelegate The component that provides the client with the ability to send messages to the
     * Capabilities API.
     * @param contextManager The @c ContextManager which will provide the context for various components.
     * @param transportFactory The object passed in here will be used whenever a new transport object
     * for AVS communication is needed.
     * @param firmwareVersion The firmware version to report to @c AVS or @c INVALID_FIRMWARE_VERSION.
     * @param sendSoftwareInfoOnConnected Whether to send SoftwareInfo upon connecting to @c AVS.
     * @param softwareInfoSenderObserver Object to receive notifications about sending SoftwareInfo.
     * @return A @c std::unique_ptr to a DefaultClient if all went well or @c nullptr otherwise.
     *
     * TODO: Allow the user to pass in a MediaPlayer factory rather than each media player individually.
     */
    static std::unique_ptr<DefaultClient> create(
        std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo,
        std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
        const std::unordered_map<std::string, std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>>&
            externalMusicProviderMediaPlayers,
        const std::unordered_map<std::string, std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>&
            externalMusicProviderSpeakers,
        const capabilityAgents::externalMediaPlayer::ExternalMediaPlayer::AdapterCreationMap& adapterCreationMap,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> speakMediaPlayer,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> audioMediaPlayer,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> alertsMediaPlayer,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> notificationsMediaPlayer,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> bluetoothMediaPlayer,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> ringtoneMediaPlayer,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> speakSpeaker,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> audioSpeaker,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> alertsSpeaker,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> notificationsSpeaker,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> bluetoothSpeaker,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> ringtoneSpeaker,
        const std::vector<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>& additionalSpeakers,
#ifdef ENABLE_PCC
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> phoneSpeaker,
        std::shared_ptr<avsCommon::sdkInterfaces::phone::PhoneCallerInterface> phoneCaller,
#endif
        std::shared_ptr<EqualizerRuntimeSetup> equalizerRuntimeSetup,
        std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface> audioFactory,
        std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
        std::shared_ptr<capabilityAgents::alerts::storage::AlertStorageInterface> alertStorage,
        std::shared_ptr<certifiedSender::MessageStorageInterface> messageStorage,
        std::shared_ptr<capabilityAgents::notifications::NotificationsStorageInterface> notificationsStorage,
        std::shared_ptr<capabilityAgents::settings::SettingsStorageInterface> settingsStorage,
        std::unique_ptr<settings::storage::DeviceSettingStorageInterface> deviceSettingStorage,
        std::shared_ptr<capabilityAgents::bluetooth::BluetoothStorageInterface> bluetoothStorage,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface>>
            alexaDialogStateObservers,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>>
            connectionObservers,
        std::shared_ptr<avsCommon::utils::network::InternetConnectionMonitor> internetConnectionMonitor,
        bool isGuiSupported,
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<alexaClientSDK::acl::TransportFactoryInterface> transportFactory,
        avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion =
            avsCommon::sdkInterfaces::softwareInfo::INVALID_FIRMWARE_VERSION,
        bool sendSoftwareInfoOnConnected = false,
        std::shared_ptr<avsCommon::sdkInterfaces::SoftwareInfoSenderObserverInterface> softwareInfoSenderObserver =
            nullptr,
        std::unique_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceManagerInterface> bluetoothDeviceManager =
            nullptr);
    /// @name CapabilitiesObserverInterface Methods
    /// @{
    void onCapabilitiesStateChange(
        avsCommon::sdkInterfaces::CapabilitiesObserverInterface::State newState,
        avsCommon::sdkInterfaces::CapabilitiesObserverInterface::Error newError) override;
    /// }

    /**
     * Connects the client to AVS. After this call, users can observe the state of the connection asynchronously by
     * using a connectionObserver that was passed in to the create() function.
     *
     * @param capabilitiesDelegate The component that provides the client with the ability to send messages to the
     * Capabilities API.
     * @param avsEndpoint An optional parameter to the AVS URL to connect to. If empty the "endpoint" value of the
     * "acl" configuration will be used.  If there no such configuration value a default value will be used instead.
     */
    void connect(
        const std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface>& capabilitiesDelegate,
        const std::string& avsEndpoint = "");

    /**
     * Disconnects the client from AVS if it is connected. After the call, users can observer the state of the
     * connection asynchronously by using a connectionObserver that was passed in to the create() function.
     */
    void disconnect();

    /**
     * Stops the foreground activity if there is one. This acts as a "stop" button that can be used to stop an
     * ongoing activity. This call will block until the foreground activity has stopped all user-observable activities.
     */
    void stopForegroundActivity();

    /**
     * This function provides a way for application code to request this object stop any active alert as the result
     * of a user action, such as pressing a physical 'stop' button on the device.
     */
    void localStopActiveAlert();

    /**
     * Adds an observer to be notified of Alexa dialog related UX state.
     *
     * @param observer The observer to add.
     */
    void addAlexaDialogStateObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface> observer);

    /**
     * Removes an observer to be notified of Alexa dialog related UX state.
     *
     * @note This is a synchronous call which can not be made by an observer callback.  Attempting to call
     *     @c removeObserver() from @c DialogUXStateObserverInterface::onDialogUXStateChanged() will result in a
     *     deadlock.
     *
     * @param observer The observer to remove.
     */
    void removeAlexaDialogStateObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface> observer);

    /**
     * Adds an observer to be notified when a message arrives from AVS.
     *
     * @param observer The observer to add.
     */
    void addMessageObserver(std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer);

    /**
     * Removes an observer to be notified when a message arrives from AVS.
     *
     * @param observer The observer to remove.
     */
    void removeMessageObserver(std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer);

    /**
     * Adds an observer to be notified of connection status changes.
     *
     * @param observer The observer to add.
     */
    void addConnectionObserver(std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> observer);

    /**
     * Removes an observer to be notified of connection status changes.
     *
     * @param observer The observer to remove.
     */
    void removeConnectionObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> observer);

    /**
     * Adds an observer to be notified of internet connection status changes.
     *
     * @param observer The observer to add.
     */
    void addInternetConnectionObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionObserverInterface> observer);

    /**
     * Removes an observer to be notified of internet connection status changes.
     *
     * @param observer The observer to remove.
     */
    void removeInternetConnectionObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionObserverInterface> observer);

    /**
     * Adds an observer to be notified of alert state changes.
     *
     * @param observer The observer to add.
     */
    void addAlertsObserver(std::shared_ptr<capabilityAgents::alerts::AlertObserverInterface> observer);

    /**
     * Removes an observer to be notified of alert state changes.
     *
     * @param observer The observer to remove.
     */
    void removeAlertsObserver(std::shared_ptr<capabilityAgents::alerts::AlertObserverInterface> observer);

    /**
     * Adds an observer to be notified of @c AudioPlayer state changes. This can be used to be be notified of the @c
     * PlayerActivity of the @c AudioPlayer.
     *
     * @param observer The observer to add.
     */
    void addAudioPlayerObserver(std::shared_ptr<avsCommon::sdkInterfaces::AudioPlayerObserverInterface> observer);

    /**
     * Removes an observer to be notified of @c AudioPlayer state changes.
     *
     * @param observer The observer to remove.
     */
    void removeAudioPlayerObserver(std::shared_ptr<avsCommon::sdkInterfaces::AudioPlayerObserverInterface> observer);

    /**
     * Adds an observer to be notified when a TemplateRuntime directive is received.
     *
     * @param observer The observer to add.
     */
    void addTemplateRuntimeObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::TemplateRuntimeObserverInterface> observer);

    /**
     * Removes an observer to be notified when a TemplateRuntime directive is received.
     *
     * @param observer The observer to remove.
     */
    void removeTemplateRuntimeObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::TemplateRuntimeObserverInterface> observer);

    /**
     * Notify the TemplateRuntime Capability Agent that the display card is cleared from the screen.
     */
    void TemplateRuntimeDisplayCardCleared();

    /**
     * Adds an observer to a single setting to be notified of that setting change.
     *
     * @param key The name of the setting.
     * @param observer The settings observer to be added.
     */
    void addSettingObserver(
        const std::string& key,
        std::shared_ptr<avsCommon::sdkInterfaces::SingleSettingObserverInterface> observer);

    /**
     * Removes an observer to a single setting to be notified of that setting change.
     *
     * @param key The name of the setting.
     * @param observer The settings observer to remove.
     */
    void removeSettingObserver(
        const std::string& key,
        std::shared_ptr<avsCommon::sdkInterfaces::SingleSettingObserverInterface> observer);

    /**
     * Adds an observer to be notified of IndicatorState changes.
     *
     * @param observer The observer to add.
     */
    void addNotificationsObserver(std::shared_ptr<avsCommon::sdkInterfaces::NotificationsObserverInterface> observer);

    /**
     * Removes an observer to be notified of IndicatorState changes.
     *
     * @param observer The observer to remove.
     */
    void removeNotificationsObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::NotificationsObserverInterface> observer);

    /**
     * Adds an observer to be notified of ExternalMediaPlayer changes
     *
     * @param observer The observer to add.
     */
    void addExternalMediaPlayerObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::externalMediaPlayer::ExternalMediaPlayerObserverInterface> observer);

    /**
     * Removes an observer to be notified of ExternalMediaPlayer changes.
     *
     * @param observer The observer to remove.
     */
    void removeExternalMediaPlayerObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::externalMediaPlayer::ExternalMediaPlayerObserverInterface> observer);

    /**
     * Adds an observer to be notified of bluetooth device changes.
     *
     * @param observer The @c BluetoothDeviceObserverInterface to add.
     */
    void addBluetoothDeviceObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceObserverInterface> observer);

    /**
     * Removes an observer to be notified of bluetooth device changes.
     *
     * @param observer The @c BluetoothDeviceObserverInterface to remove.
     */
    void removeBluetoothDeviceObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceObserverInterface> observer);

    /**
     * Calls the changeSetting function of Settings object.
     *
     * @param key The name of the setting to be changed.
     * @param value The value of the setting to be set.
     */
    void changeSetting(const std::string& key, const std::string& value);

    /**
     * Sends the default settings set by the user to the AVS after connection is established.
     */
    void sendDefaultSettings();

    /**
     * Get a reference to the PlaybackRouter
     *
     * @return shared_ptr to the PlaybackRouter.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> getPlaybackRouter() const;

    /**
     * Adds a SpeakerManagerObserver to be alerted when the volume and mute changes.
     *
     * @param observer The observer to be notified upon volume and mute changes.
     */
    void addSpeakerManagerObserver(std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerObserverInterface> observer);

    /**
     * Removes a SpeakerManagerObserver from being alerted when the volume and mute changes.
     *
     * @param observer The observer to be notified upon volume and mute changes.
     */
    void removeSpeakerManagerObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerObserverInterface> observer);

    /**
     * Get a shared_ptr to the SpeakerManager.
     *
     * @return shared_ptr to the SpeakerManager.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> getSpeakerManager();

    /**
     * Get a shared_ptr to the RegistrationManager.
     *
     * @return shared_ptr to the RegistrationManager.
     */
    std::shared_ptr<registrationManager::RegistrationManager> getRegistrationManager();

#ifdef ENABLE_REVOKE_AUTH
    /**
     * Adds a RevokeAuthorizationObserver to be alerted when a revoke authorization request occurs.
     *
     * @param observer The observer to be notified of revoke authorization requests.
     */
    void addRevokeAuthorizationObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::RevokeAuthorizationObserverInterface> observer);

    /**
     * Removes a RevokeAuthorizationObserver from being alerted when a revoke authorization request occurs.
     *
     * @param observer The observer to no longer be notified of revoke authorization requests.
     */
    void removeRevokeAuthorizationObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::RevokeAuthorizationObserverInterface> observer);
#endif

    /**
     * Get a shared_ptr to the EqualizerController.
     *
     * @note Be sure to release all references to the returned @c EqualizerController before releasing the last
     * reference to the @c DefaultClient.
     * @return shared_ptr to the EqualizerController.
     */
    std::shared_ptr<equalizer::EqualizerController> getEqualizerController();

    /**
     * Update the firmware version.
     *
     * @param firmwareVersion The new firmware version.
     * @return Whether the setting was accepted.
     */
    bool setFirmwareVersion(avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion);

    /**
     * Begins a wake word initiated Alexa interaction.
     *
     * @param wakeWordAudioProvider The audio provider containing the audio data stream along with its metadata.
     * @param beginIndex The begin index of the keyword found within the stream.
     * @param endIndex The end index of the keyword found within the stream.
     * @param keyword The keyword that was detected.
     * @param espData The ESP measurement data.
     * @param KWDMetadata Wake word engine metadata.
     * @param startOfSpeechTimestamp Moment in time when user started talking to Alexa.
     * @return A future indicating whether the interaction was successfully started.
     */
    std::future<bool> notifyOfWakeWord(
        capabilityAgents::aip::AudioProvider wakeWordAudioProvider,
        avsCommon::avs::AudioInputStream::Index beginIndex,
        avsCommon::avs::AudioInputStream::Index endIndex,
        std::string keyword,
        std::chrono::steady_clock::time_point startOfSpeechTimestamp,
        const capabilityAgents::aip::ESPData espData = capabilityAgents::aip::ESPData::getEmptyESPData(),
        std::shared_ptr<const std::vector<char>> KWDMetadata = nullptr);

    /**
     * Begins a tap to talk initiated Alexa interaction. Note that this can also be used for wake word engines that
     * don't support providing both a begin and end index.
     *
     * @param tapToTalkAudioProvider The audio provider containing the audio data stream along with its metadata.
     * @param beginIndex An optional parameter indicating where in the stream to start reading from.
     * @param startOfSpeechTimestamp Moment in time when user started talking to Alexa. This parameter is optional
     * and it is used to measure user perceived latency.
     * @return A future indicating whether the interaction was successfully started.
     */
    std::future<bool> notifyOfTapToTalk(
        capabilityAgents::aip::AudioProvider tapToTalkAudioProvider,
        avsCommon::avs::AudioInputStream::Index beginIndex = INVALID_INDEX,
        std::chrono::steady_clock::time_point startOfSpeechTimestamp = std::chrono::steady_clock::now());

    /**
     * Begins a hold to talk initiated Alexa interaction.
     *
     * @param holdToTalkAudioProvider The audio provider containing the audio data stream along with its metadata.
     * @param startOfSpeechTimestamp Moment in time when user started talking to Alexa. This parameter is optional
     * and it is used to measure user perceived latency.
     * @return A future indicating whether the interaction was successfully started.
     */
    std::future<bool> notifyOfHoldToTalkStart(
        capabilityAgents::aip::AudioProvider holdToTalkAudioProvider,
        std::chrono::steady_clock::time_point startOfSpeechTimestamp = std::chrono::steady_clock::now());

    /**
     * Ends a hold to talk interaction by forcing the client to stop streaming audio data to the cloud and ending any
     * currently ongoing recognize interactions.
     *
     * @return A future indicating whether audio streaming was successfully stopped. This can be false if this was
     * called in the wrong state.
     */
    std::future<bool> notifyOfHoldToTalkEnd();

    /**
     * Ends a tap to talk interaction by forcing the client to stop streaming audio data to the cloud and ending any
     * currently ongoing recognize interactions.
     *
     * @return A future indicating whether audio streaming was successfully stopped. This can be false if this was
     * called in the wrong state.
     */
    std::future<bool> notifyOfTapToTalkEnd();

    /**
     * Retrieves the device settings manager which can be used to access device settings.
     *
     * @return Pointer to the device settings manager.
     */
    std::shared_ptr<settings::DeviceSettingsManager> getSettingsManager();

    /**
     * Adds an observer to be notified when the call state has changed.
     *
     * @param observer The observer to add.
     */
    void addCallStateObserver(std::shared_ptr<avsCommon::sdkInterfaces::CallStateObserverInterface> observer);

    /**
     * Removes an observer to be notified when the call state has changed.
     *
     * @param observer The observer to remove.
     */
    void removeCallStateObserver(std::shared_ptr<avsCommon::sdkInterfaces::CallStateObserverInterface> observer);

    /**
     * Lets the caller know if Comms is enabled.
     *
     * @return True if comms is enabled.
     */
    bool isCommsEnabled();

    /**
     * Accepts an incoming phone-call.
     */
    void acceptCommsCall();

    /**
     * Stops a phone-call.
     */
    void stopCommsCall();

    /**
     * Destructor.
     */
    ~DefaultClient();

private:
    /**
     * Initializes the SDK and "glues" all the components together.
     *
     * @param deviceInfo DeviceInfo which reflects the device setup credentials.
     * @param customerDataManager CustomerDataManager instance to be used by RegistrationManager and instances of
     * all classes extending CustomDataHandler.
     * @param externalMusicProviderMediaPlayers The map of <PlayerId, mediaPlayer> to use to play content from each
     * external music provider.
     * @param externalMusicProviderSpeakers The map of <players, speaker> to use to track volume of each
     * external music provider media player.
     * @param adapterCreationMap The map of <players, adapterCreationMethod> to use when creating the adapters for the
     * different music providers supported by ExternalMediaPlayer.
     * @param speakMediaPlayer The media player to use to play Alexa speech from.
     * @param audioMediaPlayer The media player to use to play Alexa audio content from.
     * @param alertsMediaPlayer The media player to use to play alerts from.
     * @param notificationsMediaPlayer The media player to play notification indicators.
     * @param bluetoothMediaPlayer The media player to play bluetooth content.
     * @param ringtoneMediaPlayer The media player to play Comms ringtones.
     * @param speakSpeaker The speaker to control volume of Alexa speech.
     * @param audioSpeaker The speaker to control volume of Alexa audio content.
     * @param alertsSpeaker The speaker to control volume of alerts.
     * @param notificationsSpeaker The speaker to control volume of notifications.
     * @param bluetoothSpeaker The speaker to control bluetooth volume.
     * @param ringtoneSpeaker The speaker to control volume of Comms ringtones.
     * @param additionalSpeakers A list of additional speakers to receive volume changes.
     * @param equalizerRuntimeSetup Equalizer component runtime setup
     * @param audioFactory The audioFactory is a component the provides unique audio streams.
     * @param authDelegate The component that provides the client with valid LWA authorization.
     * @param alertStorage The storage interface that will be used to store alerts.
     * @param messageStorage The storage interface that will be used to store certified sender messages.
     * @param notificationsStorage The storage interface that will be used to store notification indicators.
     * @param settingsStorage The storage interface that will be used to store settings (DEPRECATED).
     * @param deviceSettingsStorage The storage interface that will be used to store device settings.
     * @param bluetoothStorage The storage interface that will be used to store bluetooth data.
     * @param alexaDialogStateObservers Observers that can be used to be notified of Alexa dialog related UX state
     * changes.
     * @param connectionObservers Observers that can be used to be notified of connection status changes.
     * @param isGuiSupported Whether the device supports GUI.
     * @param capabilitiesDelegate The component that provides the client with the ability to send messages to the
     * Capabilities API.
     * @param contextManager The @c ContextManager which will provide the context for various components.
     * @param transportFactory The object passed in here will be used whenever a new transport object
     * for AVS communication is needed.
     * @param firmwareVersion The firmware version to report to @c AVS or @c INVALID_FIRMWARE_VERSION.
     * @param sendSoftwareInfoOnConnected Whether to send SoftwareInfo upon connecting to @c AVS.
     * @param softwareInfoSenderObserver Object to receive notifications about sending SoftwareInfo.
     * @return Whether the SDK was initialized properly.
     */
    bool initialize(
        std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo,
        std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
        const std::unordered_map<std::string, std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>>&
            externalMusicProviderMediaPlayers,
        const std::unordered_map<std::string, std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>&
            externalMusicProviderSpeakers,
        const capabilityAgents::externalMediaPlayer::ExternalMediaPlayer::AdapterCreationMap& adapterCreationMap,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> speakMediaPlayer,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> audioMediaPlayer,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> alertsMediaPlayer,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> notificationsMediaPlayer,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> bluetoothMediaPlayer,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> ringtoneMediaPlayer,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> speakSpeaker,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> audioSpeaker,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> alertsSpeaker,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> notificationsSpeaker,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> bluetoothSpeaker,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> ringtoneSpeaker,
        const std::vector<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>& additionalSpeakers,
#ifdef ENABLE_PCC
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> phoneSpeaker,
        std::shared_ptr<avsCommon::sdkInterfaces::phone::PhoneCallerInterface> phoneCaller,
#endif
        std::shared_ptr<EqualizerRuntimeSetup> equalizerRuntimeSetup,
        std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface> audioFactory,
        std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
        std::shared_ptr<capabilityAgents::alerts::storage::AlertStorageInterface> alertStorage,
        std::shared_ptr<certifiedSender::MessageStorageInterface> messageStorage,
        std::shared_ptr<capabilityAgents::notifications::NotificationsStorageInterface> notificationsStorage,
        std::shared_ptr<capabilityAgents::settings::SettingsStorageInterface> settingsStorage,
        std::shared_ptr<settings::storage::DeviceSettingStorageInterface> deviceSettingStorage,
        std::shared_ptr<capabilityAgents::bluetooth::BluetoothStorageInterface> bluetoothStorage,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface>>
            alexaDialogStateObservers,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>>
            connectionObservers,
        std::shared_ptr<avsCommon::utils::network::InternetConnectionMonitor> internetConnectionMonitor,
        bool isGuiSupported,
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<alexaClientSDK::acl::TransportFactoryInterface> transportFactory,
        avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion,
        bool sendSoftwareInfoOnConnected,
        std::shared_ptr<avsCommon::sdkInterfaces::SoftwareInfoSenderObserverInterface> softwareInfoSenderObserver,
        std::unique_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceManagerInterface> bluetoothDeviceManager);

    /// The directive sequencer.
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> m_directiveSequencer;

    /// The focus manager for audio channels.
    std::shared_ptr<afml::FocusManager> m_audioFocusManager;

    /// The focus manager for visual channels.
    std::shared_ptr<afml::FocusManager> m_visualFocusManager;

    /// The audio activity tracker.
    std::shared_ptr<afml::AudioActivityTracker> m_audioActivityTracker;

    /// The visual activity tracker.
    std::shared_ptr<afml::VisualActivityTracker> m_visualActivityTracker;

    /// The message router.
    std::shared_ptr<acl::MessageRouter> m_messageRouter;

    /// The connection manager.
    std::shared_ptr<acl::AVSConnectionManager> m_connectionManager;

    std::shared_ptr<avsCommon::utils::network::InternetConnectionMonitor> m_internetConnectionMonitor;

    /// The exception sender.
    std::shared_ptr<avsCommon::avs::ExceptionEncounteredSender> m_exceptionSender;

    /// The certified sender.
    std::shared_ptr<certifiedSender::CertifiedSender> m_certifiedSender;

    /// The audio input processor.
    std::shared_ptr<capabilityAgents::aip::AudioInputProcessor> m_audioInputProcessor;

    /// The speech synthesizer.
    std::shared_ptr<capabilityAgents::speechSynthesizer::SpeechSynthesizer> m_speechSynthesizer;

    /// The audio player.
    std::shared_ptr<capabilityAgents::audioPlayer::AudioPlayer> m_audioPlayer;

    /// The external media player.
    std::shared_ptr<capabilityAgents::externalMediaPlayer::ExternalMediaPlayer> m_externalMediaPlayer;

    /// The alerts capability agent.
    std::shared_ptr<capabilityAgents::alerts::AlertsCapabilityAgent> m_alertsCapabilityAgent;

    /// The bluetooth capability agent.
    std::shared_ptr<capabilityAgents::bluetooth::Bluetooth> m_bluetooth;

    /// The interaction model capability agent.
    std::shared_ptr<capabilityAgents::interactionModel::InteractionModelCapabilityAgent> m_interactionCapabilityAgent;

    /// The notifications capability agent.
    std::shared_ptr<capabilityAgents::notifications::NotificationsCapabilityAgent> m_notificationsCapabilityAgent;

    /// The user inactivity monitor.
    std::shared_ptr<capabilityAgents::system::UserInactivityMonitor> m_userInactivityMonitor;

#ifdef ENABLE_PCC
    /// The phoneCallController capability agent.
    std::shared_ptr<capabilityAgents::phoneCallController::PhoneCallController> m_phoneCallControllerCapabilityAgent;
#endif

    /// The call manager capability agent.
    std::shared_ptr<avsCommon::sdkInterfaces::CallManagerInterface> m_callManager;

    /// The Alexa dialog UX aggregator.
    std::shared_ptr<avsCommon::avs::DialogUXStateAggregator> m_dialogUXStateAggregator;

    /// The playbackRouter.
    std::shared_ptr<capabilityAgents::playbackController::PlaybackRouter> m_playbackRouter;

    /// The playbackController capability agent.
    std::shared_ptr<capabilityAgents::playbackController::PlaybackController> m_playbackController;

    /// The settings object.
    std::shared_ptr<capabilityAgents::settings::Settings> m_settings;

    /// The speakerManager. Used for controlling the volume and mute settings of @c SpeakerInterface objects.
    std::shared_ptr<capabilityAgents::speakerManager::SpeakerManager> m_speakerManager;

    /// The TemplateRuntime capability agent.
    std::shared_ptr<capabilityAgents::templateRuntime::TemplateRuntime> m_templateRuntime;

    /// The MRM capability agent.
    std::shared_ptr<capabilityAgents::mrm::MRMCapabilityAgent> m_mrmCapabilityAgent;

    /// The DoNotDisturb capability agent.
    std::shared_ptr<capabilityAgents::doNotDisturb::DoNotDisturbCapabilityAgent> m_dndCapabilityAgent;

    /// The Equalizer capability agent.
    std::shared_ptr<capabilityAgents::equalizer::EqualizerCapabilityAgent> m_equalizerCapabilityAgent;

    /// The @c EqualizerController instance.
    std::shared_ptr<equalizer::EqualizerController> m_equalizerController;

    /// Equalizer runtime setup to be used in the SDK.
    std::shared_ptr<EqualizerRuntimeSetup> m_equalizerRuntimeSetup;

    /// Mutex to serialize access to m_softwareInfoSender.
    std::mutex m_softwareInfoSenderMutex;

    /// The System.SoftwareInfoSender capability agent.
    std::shared_ptr<capabilityAgents::system::SoftwareInfoSender> m_softwareInfoSender;

#ifdef ENABLE_REVOKE_AUTH
    /// The System.RevokeAuthorizationHandler directive handler.
    std::shared_ptr<capabilityAgents::system::RevokeAuthorizationHandler> m_revokeAuthorizationHandler;
#endif

    /// The RegistrationManager used to control customer registration.
    std::shared_ptr<registrationManager::RegistrationManager> m_registrationManager;

    /// Module responsible for managing device settings.
    std::shared_ptr<settings::DeviceSettingsManager> m_deviceSettingsManager;

    /// Settings storage. This storage needs to be closed during default client destruction.
    std::shared_ptr<alexaClientSDK::settings::storage::DeviceSettingStorageInterface> m_deviceSettingStorage;
};

}  // namespace defaultClient
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_DEFAULTCLIENT_H_
