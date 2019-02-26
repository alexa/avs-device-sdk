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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_UIMANAGER_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_UIMANAGER_H_

#include <AVSCommon/SDKInterfaces/AuthObserverInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesObserverInterface.h>
#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>
#include <AVSCommon/SDKInterfaces/AuthObserverInterface.h>
#include <AVSCommon/SDKInterfaces/NotificationsObserverInterface.h>
#include <AVSCommon/SDKInterfaces/SingleSettingObserverInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerObserverInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceObserverInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <CBLAuthDelegate/CBLAuthRequesterInterface.h>
#include <Settings/DeviceSettingsManager.h>
#include <Settings/SettingCallbacks.h>

namespace alexaClientSDK {
namespace sampleApp {

/**
 * This class manages the states that the user will see when interacting with the Sample Application. For now, it simply
 * prints states to the screen.
 */
class UIManager
        : public avsCommon::sdkInterfaces::DialogUXStateObserverInterface
        , public avsCommon::sdkInterfaces::AuthObserverInterface
        , public avsCommon::sdkInterfaces::CapabilitiesObserverInterface
        , public avsCommon::sdkInterfaces::ConnectionStatusObserverInterface
        , public avsCommon::sdkInterfaces::SingleSettingObserverInterface
        , public avsCommon::sdkInterfaces::SpeakerManagerObserverInterface
        , public avsCommon::sdkInterfaces::NotificationsObserverInterface
        , public avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceObserverInterface
        , public authorization::cblAuthDelegate::CBLAuthRequesterInterface {
public:
    using DeviceAttributes = avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceObserverInterface::DeviceAttributes;

    /**
     * Constructor.
     */
    UIManager();

    void onDialogUXStateChanged(DialogUXState state) override;

    void onConnectionStatusChanged(const Status status, const ChangedReason reason) override;

    void onSettingChanged(const std::string& key, const std::string& value) override;

    /// @name SpeakerManagerObserverInterface Functions
    /// @{
    void onSpeakerSettingsChanged(
        const avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source& source,
        const avsCommon::sdkInterfaces::SpeakerInterface::Type& type,
        const avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings& settings) override;
    /// }

    /// @name NotificationsObserverInterface Functions
    /// @{
    void onSetIndicator(avsCommon::avs::IndicatorState state) override;
    /// }

    /// @name CBLAuthRequesterInterface Functions
    /// @{
    void onRequestAuthorization(const std::string& url, const std::string& code) override;
    void onCheckingForAuthorization() override;
    /// }

    /// @name AuthObserverInterface Methods
    /// @{
    void onAuthStateChange(
        avsCommon::sdkInterfaces::AuthObserverInterface::State newState,
        avsCommon::sdkInterfaces::AuthObserverInterface::Error newError) override;
    /// }

    /// @name CapabilitiesObserverInterface Methods
    /// @{
    void onCapabilitiesStateChange(
        avsCommon::sdkInterfaces::CapabilitiesObserverInterface::State newState,
        avsCommon::sdkInterfaces::CapabilitiesObserverInterface::Error newError) override;
    /// }

    /// @name BluetoothDeviceObserverInterface Methods
    // @{
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
     * Prints the Settings Options screen.
     */
    void printSettingsScreen();

    /**
     * Prints the Locale Options screen.
     */
    void printLocaleScreen();

    /**
     * Prints the Speaker Control Options screen. This prompts the user to select a @c SpeakerInterface::Type to modify.
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

    /**
     * Prints the ESP Control Options screen. This gives the user the possible ESP control options.
     */
    void printESPControlScreen(bool support, const std::string& voiceEnergy, const std::string& ambientEnergy);

    /**
     * Prints the Comms Control Options screen. This gives the user the possible Comms control options.
     */
    void printCommsControlScreen();

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
     * Prints an error message while trying to configure ESP in a device where ESP is not supported.
     */
    void printESPNotSupported();

    /**
     * Prints an error message while trying to override ESP Data in a device that do not support manual override.
     */
    void printESPDataOverrideNotSupported();

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
     * Prints menu for do not disturb mode.
     */
    void printDoNotDisturbScreen();

private:
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
     * Sets the failure status. If status is new and not empty, we'll print the limited mode help.
     *
     * @param failureStatus Status message with the failure reason.
     * @warning Only call this function from inside the executor thread.
     */
    void setFailureStatus(const std::string& status);

    /// The current dialog UX state of the SDK
    DialogUXState m_dialogState;

    /// The current CapabilitiesDelegate state.
    avsCommon::sdkInterfaces::CapabilitiesObserverInterface::State m_capabilitiesState;

    /// The error associated with the CapabilitiesDelegate state.
    avsCommon::sdkInterfaces::CapabilitiesObserverInterface::Error m_capabilitiesError;

    /// The current authorization state of the SDK.
    avsCommon::sdkInterfaces::AuthObserverInterface::State m_authState;

    /// Counter used to make repeated messages about checking for authorization distinguishable from each other.
    int m_authCheckCounter;

    /// The current connection state of the SDK.
    avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status m_connectionStatus;

    /// An internal executor that performs execution of callable objects passed to it sequentially but asynchronously.
    avsCommon::utils::threading::Executor m_executor;

    // String that holds a failure status message to be displayed when we are in limited mode.
    std::string m_failureStatus;

    // Object that manages settings notifications.
    std::shared_ptr<settings::SettingCallbacks<settings::DeviceSettingsManager>> m_callbacks;
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_UIMANAGER_H_
