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

#include <iostream>
#include <sstream>

#include "SampleApp/UIManager.h"

#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/SDKVersion.h>

#include "SampleApp/ConsolePrinter.h"
#include "Settings/SettingStringConversion.h"

/// String to identify log entries originating from this file.
static const std::string TAG("UIManager");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace sampleApp {

using namespace avsCommon::sdkInterfaces;
using namespace settings;

static const std::string VERSION = avsCommon::utils::sdkVersion::getCurrentVersion();

// clang-format off
static const std::string ALEXA_WELCOME_MESSAGE =
    "                  #    #     #  #####      #####  ######  #    #              \n"
    "                 # #   #     # #     #    #     # #     # #   #               \n"
    "                #   #  #     # #          #       #     # #  #                \n"
    "               #     # #     #  #####      #####  #     # ###                 \n"
    "               #######  #   #        #          # #     # #  #                \n"
    "               #     #   # #   #     #    #     # #     # #   #               \n"
    "               #     #    #     #####      #####  ######  #    #              \n"
    "                                                                              \n"
    "       #####                                           #                      \n"
    "      #     #   ##   #    # #####  #      ######      # #   #####  #####      \n"
    "      #        #  #  ##  ## #    # #      #          #   #  #    # #    #     \n"
    "       #####  #    # # ## # #    # #      #####     #     # #    # #    #     \n"
    "            # ###### #    # #####  #      #         ####### #####  #####      \n"
    "      #     # #    # #    # #      #      #         #     # #      #          \n"
    "       #####  #    # #    # #      ###### ######    #     # #      #          \n\n"
    "       SDK Version " + VERSION + "\n";
// clang-format on
static const std::string HELP_MESSAGE =
    "+----------------------------------------------------------------------------+\n"
    "|                                  Options:                                  |\n"
#ifdef KWD
    "| Wake word:                                                                 |\n"
    "|       Simply say Alexa and begin your query.                               |\n"
#endif
    "| Tap to talk:                                                               |\n"
    "|       Press 't' and Enter followed by your query (no need for the 'Alexa').|\n"
    "| Hold to talk:                                                              |\n"
    "|       Press 'h' followed by Enter to simulate holding a button.            |\n"
    "|       Then say your query (no need for the 'Alexa').                       |\n"
    "|       Press 'h' followed by Enter to simulate releasing a button.          |\n"
    "| Stop an interaction:                                                       |\n"
    "|       Press 's' and Enter to stop an ongoing interaction.                  |\n"
#ifdef KWD
    "| Privacy mode (microphone off):                                             |\n"
    "|       Press 'm' and Enter to turn on and off the microphone.               |\n"
    "| Echo Spatial Perception (ESP): This is for testing purpose only!           |\n"
    "|       Press 'e' followed by Enter at any time to adjust ESP settings.      |\n"
#endif
    "| Playback Controls:                                                         |\n"
    "|       Press '1' for a 'PLAY' button press.                                 |\n"
    "|       Press '2' for a 'PAUSE' button press.                                |\n"
    "|       Press '3' for a 'NEXT' button press.                                 |\n"
    "|       Press '4' for a 'PREVIOUS' button press.                             |\n"
#ifdef ENABLE_COMMS
    "| Comms Controls:                                                            |\n"
    "|       Press 'd' followed by Enter at any time to accept or stop calls.     |\n"
#endif
    "| Settings:                                                                  |\n"
    "|       Press 'c' followed by Enter at any time to see the settings screen.  |\n"
    "| Speaker Control:                                                           |\n"
    "|       Press 'p' followed by Enter at any time to adjust speaker settings.  |\n"
#ifdef ENABLE_PCC
    "| Phone Control:                                                             |\n"
    "|       Press 'a' followed by Enter at any time to control the phone.        |\n"
#endif
    "| Firmware Version:                                                          |\n"
    "|       Press 'f' followed by Enter at any time to report a different        |\n"
    "|       firmware version.                                                    |\n"
    "| Info:                                                                      |\n"
    "|       Press 'i' followed by Enter at any time to see the help screen.      |\n"
    "| Reset device:                                                              |\n"
    "|       Press 'k' followed by Enter at any time to reset your device. This   |\n"
    "|       will erase any data stored in the device and you will have to        |\n"
    "|       re-register your device.                                             |\n"
    "|       This option will also exit the application.                          |\n"
    "| Reauthorize device:                                                        |\n"
    "|       Press 'z' followed by Enter at any time to re-authorize your device. |\n"
    "|       This will erase any data stored in the device and initiate           |\n"
    "|       re-authorization.                                                    |\n"
    "| Quit:                                                                      |\n"
    "|       Press 'q' followed by Enter at any time to quit the application.     |\n"
    "+----------------------------------------------------------------------------+\n";

static const std::string LIMITED_HELP_HEADER =
    "+----------------------------------------------------------------------------+\n"
    "|                          In Limited Mode:                                  |\n"
    "+----------------------------------------------------------------------------+\n";

static const std::string AUTH_FAILED_STR =
    "| Status : Unrecoverable authorization failure.                              |\n";

static const std::string REAUTH_REQUIRED_STR =
    "| Status : Re-authorization required.                                        |\n";

static const std::string CAPABILITIES_API_FAILED_STR =
    "| Status : Unrecoverable Capabilities API call failure.                      |\n";

static const std::string LIMITED_HELP_MESSAGE =
    "+----------------------------------------------------------------------------+\n"
    "| Info:                                                                      |\n"
    "|       Press 'i' followed by Enter at any time to see the help screen.      |\n"
    "| Stop an interaction:                                                       |\n"
    "|       Press 's' and Enter to stop an ongoing interaction.                  |\n"
#ifdef KWD
    "| Privacy mode (microphone off):                                             |\n"
    "|       Press 'm' and Enter to turn on and off the microphone.               |\n"
#endif
    "| Speaker Control:                                                           |\n"
    "|       Press 'p' followed by Enter at any time to adjust speaker settings.  |\n"
    "| Reset device:                                                              |\n"
    "|       Press 'k' followed by Enter at any time to reset your device. This   |\n"
    "|       will erase any data stored in the device and you will have to        |\n"
    "|       re-register your device.                                             |\n"
    "|       This option will also exit the application.                          |\n"
    "| Reauthorize device:                                                        |\n"
    "|       Press 'z' followed by Enter at any time to re-authorize your device. |\n"
    "|       This will erase any data stored in the device and initiate           |\n"
    "|       re-authorization.                                                    |\n"
    "| Quit:                                                                      |\n"
    "|       Press 'q' followed by Enter at any time to quit the application.     |\n"
    "+----------------------------------------------------------------------------+\n";

static const std::string SETTINGS_MESSAGE =
    "+----------------------------------------------------------------------------+\n"
    "|                          Setting Options:                                  |\n"
    "| Change Language:                                                           |\n"
    "|       Press '1' followed by Enter to see language options.                 |\n"
    "| Change Do Not Disturb mode:                                                |\n"
    "|       Press '2' followed by Enter to see Do Not Disturb options.           |\n"
    "+----------------------------------------------------------------------------+\n";

static const std::string LOCALE_MESSAGE =
    "+----------------------------------------------------------------------------+\n"
    "|                          Language Options:                                 |\n"
    "|                                                                            |\n"
    "| Press '1' followed by Enter to change the language to US English.          |\n"
    "| Press '2' followed by Enter to change the language to UK English.          |\n"
    "| Press '3' followed by Enter to change the language to German.              |\n"
    "| Press '4' followed by Enter to change the language to Indian English.      |\n"
    "| Press '5' followed by Enter to change the language to Canadian English.    |\n"
    "| Press '6' followed by Enter to change the language to Japanese.            |\n"
    "| Press '7' followed by Enter to change the language to Australian English.  |\n"
    "| Press '8' followed by Enter to change the language to French.              |\n"
    "| Press '9' followed by Enter to change the language to Italian.             |\n"
    "| Press 'a' followed by Enter to change the language to Spanish.             |\n"
    "| Press 'b' followed by Enter to change the language to Mexican Spanish.     |\n"
    "| Press 'c' followed by Enter to change the language to Canadian French.     |\n"
    "+----------------------------------------------------------------------------+\n";

static const std::string SPEAKER_CONTROL_MESSAGE =
    "+----------------------------------------------------------------------------+\n"
    "|                          Speaker Options:                                  |\n"
    "|                                                                            |\n"
    "| Press '1' followed by Enter to modify AVS_SPEAKER_VOLUME typed speakers.   |\n"
    "|       AVS_SPEAKER_VOLUME Speakers Control Volume For:                      |\n"
    "|             Speech, Content, Notification, Bluetooth.                      |\n"
    "| Press '2' followed by Enter to modify AVS_ALERTS_VOLUME typed speakers.    |\n"
    "|       AVS_ALERTS_VOLUME Speakers Control Volume For:                       |\n"
    "|             Alerts.                                                        |\n"
    "+----------------------------------------------------------------------------+\n";

static const std::string FIRMWARE_CONTROL_MESSAGE =
    "+----------------------------------------------------------------------------+\n"
    "|                          Firmware Version:                                 |\n"
    "|                                                                            |\n"
    "| Enter a decimal integer value between 1 and 2147483647.                    |\n"
    "+----------------------------------------------------------------------------+\n";

static const std::string VOLUME_CONTROL_MESSAGE =
    "+----------------------------------------------------------------------------+\n"
    "|                          Volume Options:                                   |\n"
    "|                                                                            |\n"
    "| Press '1' followed by Enter to increase the volume.                        |\n"
    "| Press '2' followed by Enter to decrease the volume.                        |\n"
    "| Press '3' followed by Enter to mute the volume.                            |\n"
    "| Press '4' followed by Enter to unmute the volume.                          |\n"
    "| Press 'i' to display this help screen.                                     |\n"
    "| Press 'q' to exit Volume Control Mode.                                     |\n"
    "+----------------------------------------------------------------------------+\n";

#ifdef ENABLE_PCC
static const std::string PHONE_CONTROL_MESSAGE =
    "+----------------------------------------------------------------------------+\n"
    "|                   Phone Control Options:                                   |\n"
    "|                                                                            |\n"
    "| Press '1' followed by Enter to send CallActivated event                    |\n"
    "| Press '2' followed by Enter to send CallTerminated event                   |\n"
    "| Press '3' followed by Enter to send CallFailed event                       |\n"
    "| Press '4' followed by Enter to send CallReceived event                     |\n"
    "| Press '5' followed by Enter to send CallerIdReceived event                 |\n"
    "| Press '6' followed by Enter to send InboundRingingStarted event            |\n"
    "| Press '7' followed by Enter to send DialStarted event                      |\n"
    "| Press '8' followed by Enter to send OutboundRingingStarted event           |\n"
    "| Press '9' followed by Enter to send SendDtmfSucceeded event                |\n"
    "| Press '0' followed by Enter to send SendDtmfFailed event                   |\n"
    "| Press 'i' to display this help screen.                                     |\n"
    "| Press 'q' to exit Phone Control Mode.                                      |\n"
    "+----------------------------------------------------------------------------+\n";

static const std::string ENTER_CALL_ID_MESSAGE =
    "+----------------------------------------------------------------------------+\n"
    "|                              Call ID:                                      |\n"
    "|                                                                            |\n"
    "| Enter call ID followed by Enter                                            |\n"
    "|                                                                            |\n"
    "+----------------------------------------------------------------------------+\n";

static const std::string ENTER_CALLER_ID_MESSAGE =
    "+----------------------------------------------------------------------------+\n"
    "|                              Caller ID:                                    |\n"
    "|                                                                            |\n"
    "| Enter caller ID followed by Enter                                          |\n"
    "|                                                                            |\n"
    "+----------------------------------------------------------------------------+\n";
#endif

static const std::string ESP_CONTROL_MESSAGE =
    "+----------------------------------------------------------------------------+\n"
    "|                          ESP Options:                                      |\n"
    "|                                                                            |\n"
    "| By Default ESP support is off and the implementation in the SampleApp is   |\n"
    "| for testing purpose only!                                                  |\n"
    "|                                                                            |\n"
    "| Press '1' followed by Enter to toggle ESP support.                         |\n"
    "| Press '2' followed by Enter to enter the voice energy.                     |\n"
    "| Press '3' followed by Enter to enter the ambient energy.                   |\n"
    "| Press 'q' to exit ESP Control Mode.                                        |\n";

static const std::string RESET_CONFIRMATION =
    "+----------------------------------------------------------------------------+\n"
    "|                    Device Reset Confirmation:                              |\n"
    "|                                                                            |\n"
    "| This operation will remove all your personal information, device settings, |\n"
    "| and downloaded content. Are you sure you want to reset your device?        |\n"
    "|                                                                            |\n"
    "| Press 'Y' followed by Enter to reset the device.                           |\n"
    "| Press 'N' followed by Enter to cancel the device reset operation.          |\n"
    "+----------------------------------------------------------------------------+\n";

static const std::string REAUTHORIZE_CONFIRMATION =
    "+----------------------------------------------------------------------------+\n"
    "|                 Device Re-authorization Confirmation:                      |\n"
    "|                                                                            |\n"
    "| This operation will remove all your personal information, device settings, |\n"
    "| and downloaded content. Are you sure you want to reauthorize your device?  |\n"
    "|                                                                            |\n"
    "| Press 'Y' followed by Enter to reset the device.                           |\n"
    "| Press 'N' followed by Enter to cancel re-authorization.                    |\n"
    "+----------------------------------------------------------------------------+\n";

static const std::string DONOTDISTURB_CONFIRMATION_HEADER =
    "+----------------------------------------------------------------------------+\n"
    "|                 Do Not Disturb Mode Configuration:                         |";

static const std::string ENABLE_SETTING_MENU =
    "|                                                                            |\n"
    "| Press 'E' followed by Enter to enable this configuration.                  |\n"
    "| Press 'D' followed by Enter to disable this configuration.                 |\n"
    "+----------------------------------------------------------------------------+\n";

static const std::string RESET_WARNING =
    "Device was reset! Please don't forget to deregister it. For more details "
    "visit https://www.amazon.com/gp/help/customer/display.html?nodeId=201357520";

static const std::string ENTER_LIMITED = "Entering limited interaction mode.";

/// The name of the do not disturb confirmation setting.
static const std::string DO_NOT_DISTURB_NAME = "DoNotDisturb";

UIManager::UIManager() :
        m_dialogState{DialogUXState::IDLE},
        m_capabilitiesState{CapabilitiesObserverInterface::State::UNINITIALIZED},
        m_capabilitiesError{CapabilitiesObserverInterface::Error::UNINITIALIZED},
        m_authState{AuthObserverInterface::State::UNINITIALIZED},
        m_authCheckCounter{0},
        m_connectionStatus{avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status::DISCONNECTED} {
}

static const std::string COMMS_MESSAGE =
    "+----------------------------------------------------------------------------+\n"
    "|                          Comms Options:                                    |\n"
    "|                                                                            |\n"
    "| Press 'a' followed by Enter to accept an incoming call.                    |\n"
    "| Press 's' followed by Enter to stop an ongoing call.                       |\n"
    "+----------------------------------------------------------------------------+\n";

void UIManager::onDialogUXStateChanged(DialogUXState state) {
    m_executor.submit([this, state]() {
        if (state == m_dialogState) {
            return;
        }
        m_dialogState = state;
        printState();
    });
}

void UIManager::onConnectionStatusChanged(const Status status, const ChangedReason reason) {
    m_executor.submit([this, status]() {
        if (m_connectionStatus == status) {
            return;
        }
        m_connectionStatus = status;
        printState();
    });
}

void UIManager::onSettingChanged(const std::string& key, const std::string& value) {
    m_executor.submit([key, value]() {
        std::string msg = key + " set to " + value;
        ConsolePrinter::prettyPrint(msg);
    });
}

void UIManager::onSpeakerSettingsChanged(
    const SpeakerManagerObserverInterface::Source& source,
    const SpeakerInterface::Type& type,
    const SpeakerInterface::SpeakerSettings& settings) {
    m_executor.submit([source, type, settings]() {
        std::ostringstream oss;
        oss << "SOURCE:" << source << " TYPE:" << type << " VOLUME:" << static_cast<int>(settings.volume)
            << " MUTE:" << settings.mute;
        ConsolePrinter::prettyPrint(oss.str());
    });
}

void UIManager::onSetIndicator(avsCommon::avs::IndicatorState state) {
    m_executor.submit([state]() {
        std::ostringstream oss;
        oss << "NOTIFICATION INDICATOR STATE: " << state;
        ConsolePrinter::prettyPrint(oss.str());
    });
}

void UIManager::onRequestAuthorization(const std::string& url, const std::string& code) {
    m_executor.submit([this, url, code]() {
        m_authCheckCounter = 0;
        ConsolePrinter::prettyPrint("NOT YET AUTHORIZED");
        std::ostringstream oss;
        oss << "To authorize, browse to: '" << url << "' and enter the code: " << code;
        ConsolePrinter::prettyPrint(oss.str());
    });
}

void UIManager::onCheckingForAuthorization() {
    m_executor.submit([this]() {
        std::ostringstream oss;
        oss << "Checking for authorization (" << ++m_authCheckCounter << ")...";
        ConsolePrinter::prettyPrint(oss.str());
    });
}

void UIManager::onAuthStateChange(AuthObserverInterface::State newState, AuthObserverInterface::Error newError) {
    m_executor.submit([this, newState, newError]() {
        if (m_authState != newState) {
            m_authState = newState;
            switch (m_authState) {
                case AuthObserverInterface::State::UNINITIALIZED:
                    break;
                case AuthObserverInterface::State::REFRESHED:
                    ConsolePrinter::prettyPrint("Authorized!");
                    break;
                case AuthObserverInterface::State::EXPIRED:
                    ConsolePrinter::prettyPrint("AUTHORIZATION EXPIRED. RETRYING...");
                    break;
                case AuthObserverInterface::State::UNRECOVERABLE_ERROR:
                    switch (newError) {
                        case AuthObserverInterface::Error::SUCCESS:
                        case AuthObserverInterface::Error::UNKNOWN_ERROR:
                        case AuthObserverInterface::Error::AUTHORIZATION_FAILED:
                        case AuthObserverInterface::Error::UNAUTHORIZED_CLIENT:
                        case AuthObserverInterface::Error::SERVER_ERROR:
                        case AuthObserverInterface::Error::INVALID_REQUEST:
                        case AuthObserverInterface::Error::INVALID_VALUE:
                        case AuthObserverInterface::Error::UNSUPPORTED_GRANT_TYPE:
                        case AuthObserverInterface::Error::AUTHORIZATION_PENDING:
                        case AuthObserverInterface::Error::SLOW_DOWN:
                        case AuthObserverInterface::Error::INTERNAL_ERROR:
                        case AuthObserverInterface::Error::INVALID_CBL_CLIENT_ID: {
                            std::ostringstream oss;
                            oss << "UNRECOVERABLE AUTHORIZATION ERROR: " << newError;
                            ConsolePrinter::prettyPrint({oss.str(), ENTER_LIMITED});
                            setFailureStatus(AUTH_FAILED_STR);
                            break;
                        }
                        case AuthObserverInterface::Error::AUTHORIZATION_EXPIRED:
                            ConsolePrinter::prettyPrint(
                                {"AUTHORIZATION FAILED", "RE-AUTHORIZATION REQUIRED", ENTER_LIMITED});
                            setFailureStatus(REAUTH_REQUIRED_STR);
                            break;
                        case AuthObserverInterface::Error::INVALID_CODE_PAIR:
                            ConsolePrinter::prettyPrint(
                                {"AUTHORIZATION CODE EXPIRED", "(RE)-AUTHORIZATION REQUIRED", ENTER_LIMITED});
                            setFailureStatus(REAUTH_REQUIRED_STR);
                            break;
                    }
                    break;
            }
        }
    });
}

void UIManager::onCapabilitiesStateChange(
    CapabilitiesObserverInterface::State newState,
    CapabilitiesObserverInterface::Error newError) {
    m_executor.submit([this, newState, newError]() {
        if ((m_capabilitiesState != newState) && (m_capabilitiesError != newError)) {
            m_capabilitiesState = newState;
            m_capabilitiesError = newError;
            if (CapabilitiesObserverInterface::State::FATAL_ERROR == m_capabilitiesState) {
                std::ostringstream oss;
                oss << "UNRECOVERABLE CAPABILITIES API ERROR: " << m_capabilitiesError;
                ConsolePrinter::prettyPrint({oss.str(), ENTER_LIMITED});
                setFailureStatus(CAPABILITIES_API_FAILED_STR);
            }
        }
    });
}

void UIManager::printWelcomeScreen() {
    m_executor.submit([]() { ConsolePrinter::simplePrint(ALEXA_WELCOME_MESSAGE); });
}

void UIManager::printHelpScreen() {
    m_executor.submit([]() { ConsolePrinter::simplePrint(HELP_MESSAGE); });
}

void UIManager::printLimitedHelp() {
    m_executor.submit(
        [this]() { ConsolePrinter::simplePrint(LIMITED_HELP_HEADER + m_failureStatus + LIMITED_HELP_MESSAGE); });
}

void UIManager::printSettingsScreen() {
    m_executor.submit([]() { ConsolePrinter::simplePrint(SETTINGS_MESSAGE); });
}

void UIManager::printLocaleScreen() {
    m_executor.submit([]() { ConsolePrinter::simplePrint(LOCALE_MESSAGE); });
}

void UIManager::printSpeakerControlScreen() {
    m_executor.submit([]() { ConsolePrinter::simplePrint(SPEAKER_CONTROL_MESSAGE); });
}

void UIManager::printFirmwareVersionControlScreen() {
    m_executor.submit([]() { ConsolePrinter::simplePrint(FIRMWARE_CONTROL_MESSAGE); });
}

void UIManager::printVolumeControlScreen() {
    m_executor.submit([]() { ConsolePrinter::simplePrint(VOLUME_CONTROL_MESSAGE); });
}

#ifdef ENABLE_PCC
void UIManager::printPhoneControlScreen() {
    m_executor.submit([]() { ConsolePrinter::simplePrint(PHONE_CONTROL_MESSAGE); });
}

void UIManager::printCallIdScreen() {
    m_executor.submit([]() { ConsolePrinter::simplePrint(ENTER_CALL_ID_MESSAGE); });
}

void UIManager::printCallerIdScreen() {
    m_executor.submit([]() { ConsolePrinter::simplePrint(ENTER_CALLER_ID_MESSAGE); });
}
#endif

void UIManager::printESPControlScreen(bool support, const std::string& voiceEnergy, const std::string& ambientEnergy) {
    m_executor.submit([support, voiceEnergy, ambientEnergy]() {
        std::string screen = ESP_CONTROL_MESSAGE;
        screen += "|\n";
        screen += "| support       = ";
        screen += support ? "true\n" : "false\n";
        screen += "| voiceEnergy   = " + voiceEnergy + "\n";
        screen += "| ambientEnergy = " + ambientEnergy + "\n";
        screen += "+----------------------------------------------------------------------------+\n";
        ConsolePrinter::simplePrint(screen);
    });
}

void UIManager::printCommsControlScreen() {
    m_executor.submit([]() { ConsolePrinter::simplePrint(COMMS_MESSAGE); });
}

void UIManager::printErrorScreen() {
    m_executor.submit([]() { ConsolePrinter::prettyPrint("Invalid Option"); });
}

void UIManager::microphoneOff() {
    m_executor.submit([]() { ConsolePrinter::prettyPrint("Microphone Off!"); });
}

void UIManager::printResetConfirmation() {
    m_executor.submit([]() { ConsolePrinter::simplePrint(RESET_CONFIRMATION); });
}

void UIManager::printReauthorizeConfirmation() {
    m_executor.submit([]() { ConsolePrinter::simplePrint(REAUTHORIZE_CONFIRMATION); });
}

void UIManager::printResetWarning() {
    m_executor.submit([]() { ConsolePrinter::prettyPrint(RESET_WARNING); });
}

void UIManager::printDoNotDisturbScreen() {
    m_executor.submit([]() {
        ConsolePrinter::simplePrint(DONOTDISTURB_CONFIRMATION_HEADER);
        ConsolePrinter::simplePrint(ENABLE_SETTING_MENU);
    });
}

void UIManager::microphoneOn() {
    m_executor.submit([this]() { printState(); });
}

void UIManager::onBooleanSettingNotification(
    const std::string& name,
    bool state,
    settings::SettingNotifications notification) {
    std::string msg;
    if (settings::SettingNotifications::LOCAL_CHANGE_FAILED == notification ||
        settings::SettingNotifications::AVS_CHANGE_FAILED == notification) {
        msg = "ERROR: Failed to set " + name + ". ";
    }
    msg += name + " is " + std::string(state ? "ON" : "OFF");
    m_executor.submit([msg]() { ConsolePrinter::prettyPrint(msg); });
}

void UIManager::printState() {
    if (m_connectionStatus == avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status::DISCONNECTED) {
        ConsolePrinter::prettyPrint("Client not connected!");
    } else if (m_connectionStatus == avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status::PENDING) {
        ConsolePrinter::prettyPrint("Connecting...");
    } else if (m_connectionStatus == avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status::CONNECTED) {
        switch (m_dialogState) {
            case DialogUXState::IDLE:
                ConsolePrinter::prettyPrint("Alexa is currently idle!");
                return;
            case DialogUXState::LISTENING:
                ConsolePrinter::prettyPrint("Listening...");
                return;
            case DialogUXState::EXPECTING:
                ConsolePrinter::prettyPrint("Expecting...");
                return;
            case DialogUXState::THINKING:
                ConsolePrinter::prettyPrint("Thinking...");
                return;
            case DialogUXState::SPEAKING:
                ConsolePrinter::prettyPrint("Speaking...");
                return;
            /*
             * This is an intermediate state after a SPEAK directive is completed. In the case of a speech burst the
             * next SPEAK could kick in or if its the last SPEAK directive ALEXA moves to the IDLE state. So we do
             * nothing for this state.
             */
            case DialogUXState::FINISHED:
                return;
        }
    }
}

void UIManager::printESPDataOverrideNotSupported() {
    m_executor.submit([]() { ConsolePrinter::simplePrint("Cannot override ESP Value in this device."); });
}

void UIManager::printESPNotSupported() {
    m_executor.submit([]() { ConsolePrinter::simplePrint("ESP is not supported in this device."); });
}

void UIManager::printCommsNotSupported() {
    m_executor.submit([]() { ConsolePrinter::simplePrint("Comms is not supported in this device."); });
}

void UIManager::setFailureStatus(const std::string& status) {
    if (!status.empty() && status != m_failureStatus) {
        m_failureStatus = status;
        printLimitedHelp();
    }
}

bool UIManager::configureSettingsNotifications(std::shared_ptr<settings::DeviceSettingsManager> settingsManager) {
    m_callbacks = SettingCallbacks<DeviceSettingsManager>::create(settingsManager);
    if (!m_callbacks) {
        ACSDK_ERROR(LX("configureSettingsNotificationsFailed").d("reason", "createCallbacksFailed"));
        return false;
    }
    bool ok =
        m_callbacks->add<DeviceSettingsIndex::DO_NOT_DISTURB>([this](bool enable, SettingNotifications notifications) {
            onBooleanSettingNotification(DO_NOT_DISTURB_NAME, enable, notifications);
        });
    return ok;
}

void UIManager::onActiveDeviceConnected(const DeviceAttributes& deviceAttributes) {
    m_executor.submit([deviceAttributes]() {
        std::ostringstream oss;
        oss << "SUPPORTED SERVICES: ";
        std::string separator = "";
        for (const auto& supportService : deviceAttributes.supportedServices) {
            oss << separator << supportService;
            separator = ", ";
        }
        ConsolePrinter::prettyPrint({"BLUETOOTH DEVICE CONNECTED", "Name: " + deviceAttributes.name, oss.str()});
    });
}

void UIManager::onActiveDeviceDisconnected(const DeviceAttributes& deviceAttributes) {
    m_executor.submit([deviceAttributes]() {
        std::ostringstream oss;
        oss << "SUPPORTED SERVICES: ";
        std::string separator = "";
        for (const auto& supportedService : deviceAttributes.supportedServices) {
            oss << separator << supportedService;
            separator = ", ";
        }
        ConsolePrinter::prettyPrint({"BLUETOOTH DEVICE DISCONNECTED", "Name: " + deviceAttributes.name, oss.str()});
    });
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
