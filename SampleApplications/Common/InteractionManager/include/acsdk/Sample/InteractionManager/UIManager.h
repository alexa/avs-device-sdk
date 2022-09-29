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

#ifndef ACSDK_SAMPLE_INTERACTIONMANAGER_UIMANAGER_H_
#define ACSDK_SAMPLE_INTERACTIONMANAGER_UIMANAGER_H_

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

#include <acsdkSampleApplicationInterfaces/UIStateAggregatorInterface.h>
#include <acsdkSampleApplicationInterfaces/UIAuthNotifierInterface.h>
#include <acsdkSampleApplicationInterfaces/UIManagerInterface.h>
#include <AVSCommon/SDKInterfaces/AuthObserverInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesDelegateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>
#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/LocaleAssetsManagerInterface.h>
#include <AVSCommon/SDKInterfaces/SingleSettingObserverInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerObserverInterface.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <Settings/DeviceSettingsManager.h>
#include <Settings/SettingCallbacks.h>
#include <Settings/SpeechConfirmationSettingType.h>
#include <Settings/WakeWordConfirmationSettingType.h>
#include <acsdkAlertsInterfaces/AlertObserverInterface.h>
#include <acsdkBluetoothInterfaces/BluetoothDeviceObserverInterface.h>
#include <acsdkNotificationsInterfaces/NotificationsObserverInterface.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

/**
 * This class manages the states that the user will see when interacting with the Sample Application. For now, it simply
 * prints states to the screen.
 */
class UIManager
        : public avsCommon::sdkInterfaces::DialogUXStateObserverInterface
        , public avsCommon::sdkInterfaces::AuthObserverInterface
        , public avsCommon::sdkInterfaces::CapabilitiesDelegateObserverInterface
        , public avsCommon::sdkInterfaces::ConnectionStatusObserverInterface
        , public avsCommon::sdkInterfaces::SingleSettingObserverInterface
        , public avsCommon::sdkInterfaces::SpeakerManagerObserverInterface
        , public acsdkNotificationsInterfaces::NotificationsObserverInterface
        , public acsdkBluetoothInterfaces::BluetoothDeviceObserverInterface
        , public acsdkSampleApplicationInterfaces::UIManagerInterface {
public:
    using DeviceAttributes = acsdkBluetoothInterfaces::BluetoothDeviceObserverInterface::DeviceAttributes;

    /**
     * Create a UIManager.
     *
     * @param localeAssetsManager The @c LocaleAssetsManagerInterface that provides the supported locales.
     * @param deviceInfo Information about the device.  For example, the default endpoint.
     * @return a new instance of UIManager.
     */
    static std::shared_ptr<UIManager> create(
        const std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>& localeAssetsManager,
        const std::shared_ptr<avsCommon::utils::DeviceInfo>& deviceInfo);

    /// @name DialogUxStateObserverInetrface methods.
    /// @{
    void onDialogUXStateChanged(DialogUXState state) override;
    /// @}

    /// @name ConnectionStatusObserverInterface methods.
    /// @{
    void onConnectionStatusChanged(const Status status, const ChangedReason reason) override;
    /// @}

    /// @name SingleSettingObserverInterface methods
    /// @{
    void onSettingChanged(const std::string& key, const std::string& value) override;
    /// @}

    /// @name SpeakerManagerObserverInterface Functions
    /// @{
    void onSpeakerSettingsChanged(
        const avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source& source,
        const avsCommon::sdkInterfaces::ChannelVolumeInterface::Type& type,
        const avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings& settings) override;
    /// }

    /// @name NotificationsObserverInterface Functions
    /// @{
    void onSetIndicator(avsCommon::avs::IndicatorState state) override;
    void onNotificationReceived() override{};
    /// }

    /// @name UIManagerInterface Functions
    /// @{
    void printMessage(const std::string& message) override;
    /// }

    /// @name AuthObserverInterface Methods
    /// @{
    void onAuthStateChange(
        avsCommon::sdkInterfaces::AuthObserverInterface::State newState,
        avsCommon::sdkInterfaces::AuthObserverInterface::Error newError) override;
    /// }

    /// @name CapabilitiesDelegateObserverInterface Methods
    /// @{
    void onCapabilitiesStateChange(
        avsCommon::sdkInterfaces::CapabilitiesDelegateObserverInterface::State newState,
        avsCommon::sdkInterfaces::CapabilitiesDelegateObserverInterface::Error newError,
        const std::vector<avsCommon::sdkInterfaces::endpoints::EndpointIdentifier>& addedOrUpdatedEndpoints,
        const std::vector<avsCommon::sdkInterfaces::endpoints::EndpointIdentifier>& deletedEndpoints) override;
    /// }

    /// @name BluetoothDeviceObserverInterface Methods
    /// @{
    void onActiveDeviceConnected(const DeviceAttributes& deviceAttributes) override;
    void onActiveDeviceDisconnected(const DeviceAttributes& deviceAttributes) override;
    /// }

    /**
     * Prints the welcome screen.
     */
    void printWelcomeScreen();

    /**
     * Prints the help screen.
     */
    void printHelpScreen();

    /**
     * Prints the help screen with limited options. This is used when not connected to AVS.
     */
    void printLimitedHelp();

    /**
     * Prints the audio injection header to warn that audio recording is unavailable when audio injection
     * is enabled.
     */
    void printAudioInjectionHeader();

    /**
     * Prints the Settings Options screen.
     */
    void printSettingsScreen();

    /**
     * Prints the endpoint modification screen.
     */
    void printEndpointModificationScreen();

    /**
     * Prints an error message.
     * @param message The error message to print.
     */
    void printEndpointModificationError(const std::string& message);

    /**
     * Prints the Endpoint Controller Options screen.
     */
    void printEndpointControllerScreen();

    /**
     * Prints the Locale Options screen.
     */
    void printLocaleScreen();

    /**
     * Prints the Power Controller Options screen.
     */
    void printPowerControllerScreen();

    /**
     * Prints the Toggle Controller Options screen.
     */
    void printToggleControllerScreen();

    /**
     * Prints the Mode Controller Options screen.
     */
    void printModeControllerScreen();

    /**
     * Prints the Range Controller Options screen.
     */
    void printRangeControllerScreen();

    /**
     * Prints the Speaker Control Options screen. This prompts the user to select a @c ChannelVolumeInterface::Type to
     * modify.
     */
    void printSpeakerControlScreen();

    /**
     * Prints the Firmware Version Control screen. This prompts the user to enter a positive decimal integer.
     */
    void printFirmwareVersionControlScreen();

    /**
     * Prints the Volume Control Options screen. This gives the user the possible volume control options.
     */
    void printVolumeControlScreen();

#ifdef ENABLE_PCC
    /**
     * Prints the Phone Control screen. This gives the user the possible phone control options
     */
    void printPhoneControlScreen();

    /**
     * Prints the Call Id screen. This prompts the user to enter a caller Id.
     */
    void printCallerIdScreen();

    /**
     * Prints the Caller Id screen. This prompts the user to enter a caller Id.
     */
    void printCallIdScreen();
#endif

#ifdef ENABLE_MCC
    /**
     * Prints the Meeting Control screen. This gives the user the possible meeting control options
     */
    void printMeetingControlScreen();

    /**
     * Prints the Session Id screen. This prompts the user to enter a session Id.
     */
    void printSessionIdScreen();

    /**
     * Prints the Calendar Items screen. This prompts the user to enter a path to Calendar Json file.
     */
    void printCalendarItemsScreen();
#endif

#ifdef ENABLE_COMMS
    /**
     * Prints the Comms Control Options screen. This gives the user the possible Comms control options.
     */
    void printCommsControlScreen();

    /**
     *  Prints the dtmf screen. This prompts the user to enter dtmf tones.
     */
    void printDtmfScreen();

    /**
     * Prints the Error Message for Invalid dtmf input.
     */
    void printDtmfErrorScreen();

    /**
     * Notifies the user that the call is muted.
     */
    void printMuteCallScreen();

    /**
     * Notifies the user that the call is unmuted.
     */
    void printUnmuteCallScreen();
#endif

    /**
     * Prints the Error Message for Wrong Input.
     */
    void printErrorScreen();

    /**
     * Notifies the user that the microphone is off.
     */
    void microphoneOff();

    /*
     * Prints the state that Alexa is currenty in.
     */
    void microphoneOn();

    /**
     * Prints a warning that the customer still has to manually deregister the device.
     */
    void printResetWarning();

    /**
     * Prints a confirmation message prompting the user to confirm their intent.
     */
    void printResetConfirmation();

    /**
     * Prints a confirmation message prompting the user to confirm their intent to reauthorize the device.
     */
    void printReauthorizeConfirmation();

    /**
     * Prints an error message when trying to access Comms controls if Comms is not supported.
     */
    void printCommsNotSupported();

    /**
     * Configure settings notifications.
     *
     * @param Pointer to the settings manager
     * @return @true if it succeeds to configure the settings notifications; @c false otherwise.
     */
    bool configureSettingsNotifications(std::shared_ptr<settings::DeviceSettingsManager> settingsManager);

    /**
     * Prints menu for alarm volume ramp.
     */
    void printAlarmVolumeRampScreen();

    /**
     * Prints menu for wake word confirmation.
     */
    void printWakeWordConfirmationScreen();

    /**
     * Prints menu for speech confirmation.
     */
    void printSpeechConfirmationScreen();

    /**
     * Prints menu for device time zone.
     */
    void printTimeZoneScreen();

    /**
     * Prints menu for device network info.
     */
    void printNetworkInfoScreen();

    /// @name Print network info menu options
    /// @{
    void printNetworkInfoConnectionTypePrompt();
    void printNetworkInfoESSIDPrompt();
    void printNetworkInfoBSSIDPrompt();
    void printNetworkInfoIpPrompt();
    void printNetworkInfoSubnetPrompt();
    void printNetworkInfoMacPrompt();
    void printNetworkInfoDHCPPrompt();
    void printNetworkInfoStaticIpPrompt();
    /// @}

    /**
     * Prints menu for do not disturb mode.
     */
    void printDoNotDisturbScreen();

    /**
     * Prints menu for diagnostics screen.
     */
    void printDiagnosticsScreen();

    /**
     * Prints the menu for the device properties screen.
     */
    void printDevicePropertiesScreen();

    /**
     * Print all the device properties from @c DevicePropertyAggregator on screen.
     * @param deviceProperties The deviceProperties map.
     */
    void printAllDeviceProperties(const std::unordered_map<std::string, std::string>& deviceProperties);

    /**
     * Prints the Device Protocol Tracer screen.
     */
    void printDeviceProtocolTracerScreen();

    /**
     * Prints the captured protocol trace.
     *
     * @param The protocol trace string.
     */
    void printProtocolTrace(const std::string& protocolTrace);

    /**
     * Prints the protocol trace flag.
     *
     * @param enabled the boolean indicating if protocol trace is enabled/disabled.
     */
    void printProtocolTraceFlag(bool enabled);

    /**
     *  Prints the Audio Injection screen.
     */
    void printAudioInjectionScreen();

    /**
     *  Prints audio injection failure message.
     */
    void printAudioInjectionFailureMessage();

    /**
     * Set a notifier that bridges Authorization state from UI manager to the user interface.
     * @param uiAuthNotifier An instance of @c UIAuthNotifierInterface.
     */
    void setUIAuthNotifier(std::shared_ptr<acsdkSampleApplicationInterfaces::UIAuthNotifierInterface> uiAuthNotifier);

    /**
     * Set a notifier that bridges Alexa and connection state from UI manager to the user interface.
     * @param uiStateAggregator An instance of @c UIStateAggregatorInterface.
     */
    void setUIStateAggregator(
        std::shared_ptr<acsdkSampleApplicationInterfaces::UIStateAggregatorInterface> uiStateAggregator);

private:
    /**
     * Constructor
     *
     * @param localeAssetsManager The @c LocaleAssetsManagerInterface that provides the supported locales.
     * @param deviceInfo Information about the device.  For example, the default endpoint.
     */
    UIManager(
        const std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>& localeAssetsManager,
        const std::shared_ptr<avsCommon::utils::DeviceInfo>& deviceInfo);

    /**
     * Prints the current state of Alexa after checking what the appropriate message to display is based on the current
     * component states. This should only be used within the internal executor.
     */
    void printState();

    /**
     * Callback function triggered when there is a notification available regarding a boolean setting.
     *
     * @param name The setting name that was affected.
     * @param enable Whether the configuration is currently enabled or disabled.
     * @param notification The type of notification.
     */
    void onBooleanSettingNotification(
        const std::string& name,
        bool enable,
        settings::SettingNotifications notification);

    /**
     * Callback function triggered when there is a notification available regarding a string setting.
     *
     * @param name The setting name that was affected.
     * @param value The string value.
     * @param notification The type of notification.
     */
    void onStringSettingNotification(const std::string& name, bool enable, settings::SettingNotifications notification);

    /**
     * Callback function triggered when there is a notification available regarding a setting.
     * This will print out the new value of the setting.
     *
     * @tparam SettingType The data type of the setting.
     * @param name The setting name that was affected.
     * @param value The value of the setting.
     * @param notification The type of notification.
     */
    template <typename SettingType>
    void onSettingNotification(const std::string& name, SettingType value, settings::SettingNotifications notification);

    /**
     * Sets the failure status. If status is new and not empty, we'll print the limited mode help.
     *
     * @param failureStatus Status message with the failure reason.
     * @warning Only call this function from inside the executor thread.
     */
    void setFailureStatus(const std::string& status);

    /// The current dialog UX state of the SDK
    DialogUXState m_dialogState;

    /// The current authorization state of the SDK.
    avsCommon::sdkInterfaces::AuthObserverInterface::State m_authState;

    /// The current connection state of the SDK.
    avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status m_connectionStatus;

    /// The @c LocaleAssetsManagerInterface that provides the supported locales.
    std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface> m_localeAssetsManager;

    /// An internal executor that performs execution of callable objects passed to it sequentially but asynchronously.
    avsCommon::utils::threading::Executor m_executor;

    // String that holds a failure status message to be displayed when we are in limited mode.
    std::string m_failureStatus;

    // Object that manages settings notifications.
    std::shared_ptr<settings::SettingCallbacks<settings::DeviceSettingsManager>> m_callbacks;

    // The @c EndpointIdentifier of the default endpoint.
    avsCommon::sdkInterfaces::endpoints::EndpointIdentifier m_defaultEndpointId;

    /// An instance of @c UIStateAggregatorInterface to be used to notify user interface about Alexa State.
    std::shared_ptr<acsdkSampleApplicationInterfaces::UIStateAggregatorInterface> m_uiStateAggregator;

    /// An instance of @c UIAuthNotifierInterface to be used to notify authorization State.
    std::shared_ptr<acsdkSampleApplicationInterfaces::UIAuthNotifierInterface> m_uiAuthNotifier;
};

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ACSDK_SAMPLE_INTERACTIONMANAGER_UIMANAGER_H_
