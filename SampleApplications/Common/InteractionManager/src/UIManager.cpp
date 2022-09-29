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

#include <iostream>
#include <sstream>

#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/SDKVersion.h>
#include <Settings/SettingStringConversion.h>
#include <Settings/SpeechConfirmationSettingType.h>
#include <Settings/WakeWordConfirmationSettingType.h>

#include "acsdk/Sample/Console/ConsolePrinter.h"
#include "acsdk/Sample/InteractionManager/UIManager.h"

/// String to identify log entries originating from this file.
#define TAG "UIManager"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
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
    "       SDK Version " + VERSION + "\n\n"
#ifdef DEBUG_BUILD_TYPE
    "       WARNING! THIS DEVICE HAS BEEN COMPILED IN DEBUG MODE.\n\n"
    "       RELEASING A PRODUCTION DEVICE IN DEBUG MODE MAY IMPACT DEVICE PERFORMANCE,\n"
    "       DOES NOT COMPLY WITH THE AVS SECURITY REQUIREMENTS,\n"
    "       AND COULD RESULT IN SUSPENSION OR TERMINATION OF THE ALEXA SERVICE ON YOUR DEVICES.\n\n"
#endif
;

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
#endif
    "| Playback Controls:                                                         |\n"
    "|       Press '1' for a 'PLAY' button press.                                 |\n"
    "|       Press '2' for a 'PAUSE' button press.                                |\n"
    "|       Press '3' for a 'NEXT' button press.                                 |\n"
    "|       Press '4' for a 'PREVIOUS' button press.                             |\n"
#ifdef ENABLE_COMMS
    "| Comms Controls:                                                            |\n"
    "|       Press 'd' followed by Enter at any time to control the call.         |\n"
#endif
    "| Settings:                                                                  |\n"
    "|       Press 'c' followed by Enter at any time to see the settings screen.  |\n"
    "| Speaker Control:                                                           |\n"
    "|       Press 'p' followed by Enter at any time to adjust speaker settings.  |\n"
#ifdef ENABLE_PCC
    "| Phone Control:                                                             |\n"
    "|       Press 'a' followed by Enter at any time to control the phone.        |\n"
#endif
#ifdef ENABLE_MCC
    "| Meeting Control:                                                           |\n"
    "|       Press 'j' followed by Enter at any time to control the meeting.      |\n"
#endif
#ifdef ENABLE_ENDPOINT_CONTROLLERS
    "| Endpoint Controller:                                                       |\n"
    "|       Press 'e' followed by Enter at any time to see the endpoint          |\n"
    "|       controller screen.                                                   |\n"
    "| Dynamic Endpoint Modification:                                             |\n"
    "|       Press 'y' followed by Enter at any time to see dynamic endpoint      |\n"
    "|       screen.                                                              |\n"
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
#ifdef DIAGNOSTICS
    "| Diagnostics:                                                               |\n"
    "|       Press 'o' followed by Enter at any time to enter the diagnostics     |\n"
    "|       screen.                                                              |\n"
#endif
    "| Device Setup Complete:                                                     |\n"
    "|       Press 'v' followed by Enter at any time to indicate that device      |\n"
    "|       setup is complete.                                                   |\n"
    "|                                                                            |\n"
    "| Quit:                                                                      |\n"
    "|       Press 'q' followed by Enter at any time to quit the application.     |\n"
    "+----------------------------------------------------------------------------+\n";

static const std::string LIMITED_HELP_HEADER =
    "+----------------------------------------------------------------------------+\n"
    "|                          In Limited Mode:                                  |\n"
    "+----------------------------------------------------------------------------+\n";

static const std::string AUDIO_INJECTION_HEADER =
    "+----------------------------------------------------------------------------+\n"
    "|Diagnostic audio injection has been ENABLED. Audio recording is UNAVAILABLE.|\n"
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
    "|  Press '1' followed by Enter to see language options.                      |\n"
    "|  Press '2' followed by Enter to see Do Not Disturb options.                |\n"
    "|  Press '3' followed by Enter to see wake word confirmation options.        |\n"
    "|  Press '4' followed by Enter to see speech confirmation options.           |\n"
    "|  Press '5' followed by Enter to see time zone options.                     |\n"
    "|  Press '6' followed by Enter to see the network options.                   |\n"
    "|  Press '7' followed by Enter to see the Alarm Volume Ramp options.         |\n"
    "|  Press 'q' followed by Enter to exit Settings Options.                     |\n"
    "+----------------------------------------------------------------------------+\n";

#ifdef ENABLE_ENDPOINT_CONTROLLERS
static const std::string ENDPOINT_MODIFICATION_MESSAGE =
    "+-------------------------------------------------------------------------------------------+\n"
    "|                       Dynamic Endpoint Modification Options:                              |\n"
    "|  Press 'a' followed by Enter to add an endpoint with friendly name 'light'.               |\n"
    "|  Press 'm' followed by Enter to toggle the endpoint's friendly name to 'light' or 'lamp'. |\n"
    "|  Press 'd' followed by Enter to delete the endpoint.                                      |\n"
    "|  Press 'q' followed by Enter to exit Dynamic Endpoint Modification Options.               |\n"
    "+-------------------------------------------------------------------------------------------+\n";

static const std::string ENDPOINT_CONTROLLER_MESSAGE =
    "+----------------------------------------------------------------------------+\n"
    "|                 Peripheral Endpoint Controller Options:                        |\n"
#ifdef POWER_CONTROLLER
    "|  Press '1' followed by Enter to see Power Controller Options.              |\n"
#endif
#ifdef TOGGLE_CONTROLLER
    "|  Press '2' followed by Enter to see Toggle Controller Options.             |\n"
#endif
#ifdef MODE_CONTROLLER
    "|  Press '3' followed by Enter to see Mode Controller Options.               |\n"
#endif
#ifdef RANGE_CONTROLLER
    "|  Press '4' followed by Enter to see Range Controller Options.              |\n"
#endif
    "|  Press 'q' followed by Enter to exit Endpoint Controller Options.          |\n"
    "+----------------------------------------------------------------------------+\n";
#ifdef POWER_CONTROLLER
static const std::string POWER_CONTROLLER_OPTIONS =
    "+----------------------------------------------------------------------------+\n"
    "|                        Power Controller Options :                          |\n"
    "|                                                                            |\n"
    "| Press '1' followed by Enter to set power state to ON.                      |\n"
    "| Press '2' followed by Enter to set power state to OFF.                     |\n"
    "| Press 'q' to exit Power Controller Options.                                |\n"
    "+----------------------------------------------------------------------------+\n";
#endif
#ifdef TOGGLE_CONTROLLER
static const std::string TOGGLE_CONTROLLER_OPTIONS =
    "+----------------------------------------------------------------------------+\n"
    "|                        Toggle Controller Options :                         |\n"
    "|                                                                            |\n"
    "| Press '1' followed by Enter to set toggle state to ON.                     |\n"
    "| Press '2' followed by Enter to set toggle state to OFF.                    |\n"
    "| Press 'q' to exit Toggle Controller Options.                               |\n"
    "+----------------------------------------------------------------------------+\n";
#endif
#ifdef MODE_CONTROLLER
static const std::string MODE_CONTROLLER_OPTIONS =
    "+----------------------------------------------------------------------------+\n"
    "|                        Mode Controller Options :                           |\n"
    "|                                                                            |\n"
    "| Press '1' followed by Enter to set mode to \"Red\".                          |\n"
    "| Press '2' followed by Enter to set mode to \"Green\".                        |\n"
    "| Press '3' followed by Enter to set mode to \"Blue\".                         |\n"
    "| Press 'q' to exit Mode Controller Options.                                 |\n"
    "+----------------------------------------------------------------------------+\n";
#endif
#ifdef RANGE_CONTROLLER
static const std::string RANGE_CONTROLLER_OPTIONS =
    "+----------------------------------------------------------------------------+\n"
    "|                        Range Controller Options :                          |\n"
    "|                                                                            |\n"
    "| Enter Range between 1 to 10 followed by Enter.                             |\n"
    "+----------------------------------------------------------------------------+\n";
#endif
#endif

static const std::string LOCALE_MESSAGE_HEADER =
    "+----------------------------------------------------------------------------+\n"
    "|                          Language Options:                                  \n"
    "|\n";

static const std::string LOCALE_MESSAGE_FOOTER =
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

#ifdef ENABLE_MCC
static const std::string MEETING_CONTROL_MESSAGE =
    "+----------------------------------------------------------------------------+\n"
    "|                   Meeting Control Options:                                 |\n"
    "|                                                                            |\n"
    "| Press '1' followed by Enter to send MeetingJoined event                    |\n"
    "| Press '2' followed by Enter to send MeetingEnded event                     |\n"
    "| Press '3' followed by Enter to send CalendarItems event                    |\n"
    "| Press '4' followed by Enter to send SetCurrentMeetingSession event         |\n"
    "| Press '5' followed by Enter to send ClearCurrentMeetingSession event       |\n"
    "| Press '6' followed by Enter to send ConferenceConfigurationChanged event   |\n"
    "| Press '7' followed by Enter to send MeetingClientErrorOccured event        |\n"
    "| Press '8' followed by Enter to send CalendarClientErrorOccured event       |\n"
    "| Press 'i' to display this help screen.                                     |\n"
    "| Press 'q' to exit Meeting Control Mode.                                    |\n"
    "+----------------------------------------------------------------------------+\n";

static const std::string ENTER_SESSION_ID_MESSAGE =
    "+----------------------------------------------------------------------------+\n"
    "|                              Session ID:                                   |\n"
    "|                                                                            |\n"
    "| Enter session ID followed by Enter                                         |\n"
    "|                                                                            |\n"
    "+----------------------------------------------------------------------------+\n";

static const std::string ENTER_CALENDAR_ITEMS_FILE_PATH_MESSAGE =
    "+----------------------------------------------------------------------------+\n"
    "|                              Calendar Items:                               |\n"
    "|                                                                            |\n"
    "| Enter path of calendar items json file followed by Enter                   |\n"
    "|                                                                            |\n"
    "+----------------------------------------------------------------------------+\n";
#endif

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

static const std::string ALARM_VOLUME_RAMP_HEADER =
    "+----------------------------------------------------------------------------+\n"
    "|                 Alarm Volume Ramp Configuration:                           |";

static const std::string SPEECH_CONFIRMATION_HEADER =
    "+----------------------------------------------------------------------------+\n"
    "|                 Speech Confirmation Configuration:                         |";

static const std::string WAKEWORD_CONFIRMATION_HEADER =
    "+----------------------------------------------------------------------------+\n"
    "|                 Wake Word Confirmation Configuration:                      |";

static const std::string DONOTDISTURB_CONFIRMATION_HEADER =
    "+----------------------------------------------------------------------------+\n"
    "|                 Do Not Disturb Mode Configuration:                         |";

static const std::string ENABLE_SETTING_MENU =
    "|                                                                            |\n"
    "| Press 'E' followed by Enter to enable this configuration.                  |\n"
    "| Press 'D' followed by Enter to disable this configuration.                 |\n"
    "| Press 'q' followed by Enter to quit this configuration menu.               |\n"
    "+----------------------------------------------------------------------------+\n";

static const std::string TIMEZONE_SETTING_MENU =
    "+----------------------------------------------------------------------------+\n"
    "|                          TimeZone Configuration:                           |\n"
    "|                                                                            |\n"
    "| Press '1' followed by Enter to set the time zone to America/Vancouver.     |\n"
    "| Press '2' followed by Enter to set the time zone to America/Edmonton.      |\n"
    "| Press '3' followed by Enter to set the time zone to America/Winnipeg.      |\n"
    "| Press '4' followed by Enter to set the time zone to America/Toronto.       |\n"
    "| Press '5' followed by Enter to set the time zone to America/Halifax.       |\n"
    "| Press '6' followed by Enter to set the time zone to America/St_Johns.      |\n"
    "| Press 'q' followed by Enter to quit this configuration menu.               |\n"
    "+----------------------------------------------------------------------------+\n";

static const std::string NETWORK_INFO_SETTING_MENU =
    "+----------------------------------------------------------------------------+\n"
    "|                      Network Info Configuration:                           |\n"
    "|                                                                            |\n"
    "| Press '1' followed by Enter to see the current network info                |\n"
    "| Press '2' followed by Enter to set the connection type                     |\n"
    "| Press '3' followed by Enter to set the network name (ESSID)                |\n"
    "| Press '4' followed by Enter to set the physical access point name (BSSID)  |\n"
    "| Press '5' followed by Enter to set the ip address                          |\n"
    "| Press '6' followed by Enter to set the subnet mask                         |\n"
    "| Press '7' followed by Enter to set the mac address                         |\n"
    "| Press '8' followed by Enter to set the dhcp server address                 |\n"
    "| Press '9' followed by Enter to set the static ip settings                  |\n"
    "| Press 'q' followed by Enter to quit this configuration menu.               |\n"
    "+----------------------------------------------------------------------------+\n";

static const std::initializer_list<std::string> NETWORK_INFO_CONNECTION_TYPE_PROMPT = {
    "Press '1' followed by Enter to set connection type to Ethernet",
    "Press '2' followed by Enter to set connection type to Wifi",
    "Press '3' followed by Enter to reset the connection type."};

static const std::initializer_list<std::string> NETWORK_INFO_ESSID_PROMPT = {
    "Type in the name of the network (ESSID) and press enter.",
    "Leave empty to reset the ESSID."};

static const std::initializer_list<std::string> NETWORK_INFO_BSSID_PROMPT = {
    "Type in the name of the physical access point (BSSID)",
    "and press Enter. Leave empty to reset the BSSID."};

static const std::initializer_list<std::string> NETWORK_INFO_IP_PROMPT = {
    "Type in the ip address (eg. 192.168.0.1) and press Enter.",
    "Leave empty to reset the ip address."};

static const std::initializer_list<std::string> NETWORK_INFO_SUBNET_MASK_PROMPT = {
    "Type in the subnet mask and press Enter.",
    "Leave empty to reset the subnet mask."};

static const std::initializer_list<std::string> NETWORK_INFO_MAC_ADDRESS_PROMPT = {
    "Type in the device mac address and press Enter.",
    "Leave empty to reset the mac address."};

static const std::initializer_list<std::string> NETWORK_INFO_DHCP_ADDRESS_PROMPT = {
    "Type in the dhcp server address and press Enter.",
    "Leave empty to reset the dhcp server address."};

static const std::initializer_list<std::string> NETWORK_INFO_STATIC_IP_PROMPT = {
    "Press '1' followed by Enter to set the ip type to static",
    "Press '2' followed by Enter to set the ip type to dynamic",
    "Press '3' followed by Enter to reset the ip type."};

static const std::string DIAGNOSTICS_SCREEN =
    "+----------------------------------------------------------------------------+\n"
    "|                 Diagnostics Options:                                       |\n"
    "|                                                                            |\n"
#ifdef DEVICE_PROPERTIES
    "| Press 'p' followed by Enter to go to the device properties screen.         |\n"
#endif
#ifdef AUDIO_INJECTION
    "| Press 'a' followed by Enter to go to the audio injection screen.           |\n"
#endif
#ifdef PROTOCOL_TRACE
    "| Press 't' followed by Enter to go to the device protocol trace screen.     |\n"
#endif
    "|                                                                            |\n"
    "| Press 'i' followed by Enter for help.                                      |\n"
    "| Press 'q' followed by Enter to go to the previous screen.                  |\n"
    "+----------------------------------------------------------------------------+\n";

static const std::string AUDIO_INJECTION_SCREEN =
    "+------------------------------------------------------------------------------+\n"
    "|                            Audio Injection Screen:                           |\n"
    "|                                                                              |\n"
    "| This diagnostic allows for injecting audio from wav files directly into the  |\n"
    "| microphone. Note input wav files should conform to the following:            |\n"
    "|                                                                              |\n"
    "| Sample Size : 16 bytes                                                       |\n"
    "| Sample Rate : 16Khz                                                          |\n"
    "| Number of channels : 1                                                       |\n"
    "| Endianness : Little                                                          |\n"
    "| Encoding Format : LPCM                                                       |\n"
    "|                                                                              |\n"
    "| Press '1' followed by Enter to go into input mode. Once inside input mode,   |\n"
    "| enter the absolute path of the wav file to inject audio from wav file.       |\n"
    "|                                                                              |\n"
    "| Press 'i' followed by Enter for help.                                        |\n"
    "| Press 'q' followed by Enter to go to the previous screen.                    |\n"
    "+------------------------------------------------------------------------------+\n";

static const std::string DEVICE_PROTOCOL_TRACE_SCREEN =
    "+----------------------------------------------------------------------------+\n"
    "|                 Device Protocol Trace Screen:                              |\n"
    "|                                                                            |\n"
    "| Press 'e' followed by Enter to enable device protocol trace.               |\n"
    "| Press 'd' followed by Enter to disable device protocol trace.              |\n"
    "| Press 'c' followed by Enter to clear the protocol trace.                   |\n"
    "| Press 't' followed by Enter to show the protocol trace.                    |\n"
    "|                                                                            |\n"
    "| Press 'i' followed by Enter for help.                                      |\n"
    "| Press 'q' followed by Enter to go to the previous screen.                  |\n"
    "|                                                                            |\n"
    "| Note: After the limit is reached, protocol tracing halts.                  |\n"
    "| The default is 100, and it is a configurable value in the JSON config.     |\n"
    "+----------------------------------------------------------------------------+\n";

static const std::string DEVICE_PROPERTIES_SCREEN =
    "+----------------------------------------------------------------------------+\n"
    "|                 Device Properties Screen:                                  |\n"
    "|                                                                            |\n"
    "| Press 'p' followed by Enter to show the current device properties          |\n"
    "|                                                                            |\n"
    "| Press 'i' followed by Enter for help.                                      |\n"
    "| Press 'q' followed by Enter to go to the previous screen.                  |\n"
    "+----------------------------------------------------------------------------+\n";

static const std::string RESET_WARNING =
    "Device was reset! Please don't forget to deregister it. For more details "
    "visit https://www.amazon.com/gp/help/customer/display.html?nodeId=201357520";

static const std::string ENTER_LIMITED = "Entering limited interaction mode.";

/// The name of the alarm volume ramp setting.
static const std::string ALARM_VOLUME_RAMP_NAME = "AlarmVolumeRamp";

/// The name of the speech confirmation setting.
static const std::string SPEECH_CONFIRMATION_NAME = "SpeechConfirmation";

/// The name of the wake word confirmation setting.
static const std::string WAKEWORD_CONFIRMATION_NAME = "WakeWordConfirmation";

/// The name of the time zone setting.
static const std::string TIMEZONE_NAME = "TimeZone";

/// The name of the locale setting.
static const std::string LOCALE_NAME = "Locale";

/// The name of the wake words setting.
static const std::string WAKE_WORDS_NAME = "WakeWords";

/// The name of the do not disturb confirmation setting.
static const std::string DO_NOT_DISTURB_NAME = "DoNotDisturb";

/// The name of the network info setting.
static const std::string NETWORK_INFO_NAME = "NetworkInfo";

/// The index of the first option in displaying a list of options.
static const unsigned int OPTION_ENUM_START = 1;

std::shared_ptr<UIManager> UIManager::create(
    const std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>& localeAssetsManager,
    const std::shared_ptr<avsCommon::utils::DeviceInfo>& deviceInfo) {
    if (!localeAssetsManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullLocaleAssetsManager"));
        return nullptr;
    }
    if (!deviceInfo) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullDeviceInfo"));
        return nullptr;
    }

    return std::shared_ptr<UIManager>(new UIManager(localeAssetsManager, deviceInfo));
}

UIManager::UIManager(
    const std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>& localeAssetsManager,
    const std::shared_ptr<avsCommon::utils::DeviceInfo>& deviceInfo) :
        m_dialogState{DialogUXState::IDLE},
        m_authState{AuthObserverInterface::State::UNINITIALIZED},
        m_connectionStatus{avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status::DISCONNECTED},
        m_localeAssetsManager{localeAssetsManager},
        m_defaultEndpointId{deviceInfo->getDefaultEndpointId()} {
}

static const std::string COMMS_MESSAGE =
    "+----------------------------------------------------------------------------+\n"
    "|                          Comms Options:                                    |\n"
    "|                                                                            |\n"
    "| Press 'a' followed by Enter to accept an incoming call.                    |\n"
    "| Press 's' followed by Enter to stop an ongoing call.                       |\n"
    "| Press 'd' followed by Enter to input dtmf tones.                           |\n"
    "| Press 'm' followed by Enter to mute/unmte self during an active call.      |\n"
    "| Press 'q' to exit Comms Control Mode.                                      |\n"
    "+----------------------------------------------------------------------------+\n";

static const std::string DTMF_MESSAGE =
    "+----------------------------------------------------------------------------+\n"
    "|                              Dtmf Tones:                                   |\n"
    "|                                                                            |\n"
    "| Enter dtmf tones followed by Enter.                                        |\n"
    "|                                                                            |\n"
    "+----------------------------------------------------------------------------+\n";

void UIManager::onDialogUXStateChanged(DialogUXState state) {
    m_executor.execute([this, state]() {
        if (state == m_dialogState) {
            return;
        }
        m_dialogState = state;
        printState();
    });
}

void UIManager::onConnectionStatusChanged(const Status status, const ChangedReason reason) {
    m_executor.execute([this, status]() {
        if (m_connectionStatus == status) {
            return;
        }
        m_connectionStatus = status;
        printState();
    });
}

void UIManager::onSettingChanged(const std::string& key, const std::string& value) {
    m_executor.execute([key, value]() {
        std::string msg = key + " set to " + value;
        ConsolePrinter::prettyPrint(msg);
    });
}

void UIManager::onSpeakerSettingsChanged(
    const SpeakerManagerObserverInterface::Source& source,
    const ChannelVolumeInterface::Type& type,
    const SpeakerInterface::SpeakerSettings& settings) {
    m_executor.execute([source, type, settings]() {
        std::ostringstream oss;
        oss << "SOURCE:" << source << " TYPE:" << type << " VOLUME:" << static_cast<int>(settings.volume)
            << " MUTE:" << settings.mute;
        ConsolePrinter::prettyPrint(oss.str());
    });
}

void UIManager::onSetIndicator(avsCommon::avs::IndicatorState state) {
    m_executor.execute([state]() {
        std::ostringstream oss;
        oss << "NOTIFICATION INDICATOR STATE: " << state;
        ConsolePrinter::prettyPrint(oss.str());
    });
}

void UIManager::printMessage(const std::string& message) {
    m_executor.execute([message]() { ConsolePrinter::prettyPrint(message); });
}

void UIManager::onAuthStateChange(AuthObserverInterface::State newState, AuthObserverInterface::Error newError) {
    m_executor.execute([this, newState, newError]() {
        if (m_authState != newState) {
            m_authState = newState;
            switch (m_authState) {
                case AuthObserverInterface::State::AUTHORIZING:
                    break;
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

            if (m_uiAuthNotifier) {
                m_uiAuthNotifier->notifyAuthorizationStateChange(m_authState);
            }
        }
    });
}

void UIManager::onCapabilitiesStateChange(
    CapabilitiesDelegateObserverInterface::State newState,
    CapabilitiesDelegateObserverInterface::Error newError,
    const std::vector<avsCommon::sdkInterfaces::endpoints::EndpointIdentifier>& addedOrUpdatedEndpoints,
    const std::vector<avsCommon::sdkInterfaces::endpoints::EndpointIdentifier>& deletedEndpoints) {
    m_executor.execute([this, newState, newError, addedOrUpdatedEndpoints, deletedEndpoints]() {
        // If one of the added or updated endpointIds is the default endpoint, and the
        // add/update failed, go into limited mode.
        // Limited mode is unnecessary if the failure is for non-default endpoints.
        if (CapabilitiesDelegateObserverInterface::State::FATAL_ERROR == newState) {
            auto it = std::find(addedOrUpdatedEndpoints.begin(), addedOrUpdatedEndpoints.end(), m_defaultEndpointId);
            if (addedOrUpdatedEndpoints.end() != it) {
                std::ostringstream oss;
                oss << "UNRECOVERABLE CAPABILITIES API ERROR: " << newError;
                ConsolePrinter::prettyPrint({oss.str(), ENTER_LIMITED});
                setFailureStatus(CAPABILITIES_API_FAILED_STR);
                return;
            } else {
                std::ostringstream oss;
                if (!addedOrUpdatedEndpoints.empty()) {
                    oss << "Failed to register " << addedOrUpdatedEndpoints.size() << " endpoint(s): " << newError;
                    ConsolePrinter::prettyPrint(oss.str());
                }
                if (!deletedEndpoints.empty()) {
                    oss << "Failed to deregister " << deletedEndpoints.size() << " endpoint(s): " << newError;
                    ConsolePrinter::prettyPrint(oss.str());
                }
            }
        } else if (CapabilitiesDelegateObserverInterface::State::SUCCESS == newState) {
            std::ostringstream oss;
            if (!addedOrUpdatedEndpoints.empty()) {
                oss << "Successfully registered " << addedOrUpdatedEndpoints.size() << " endpoint(s). ";
                ConsolePrinter::prettyPrint(oss.str());
            }
            if (!deletedEndpoints.empty()) {
                oss << "Successfully deregistered " << deletedEndpoints.size() << " endpoint(s).";
                ConsolePrinter::prettyPrint(oss.str());
            }
        }
    });
}

void UIManager::printWelcomeScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(ALEXA_WELCOME_MESSAGE); });
}

void UIManager::printHelpScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(HELP_MESSAGE); });
}

void UIManager::printLimitedHelp() {
    m_executor.execute(
        [this]() { ConsolePrinter::simplePrint(LIMITED_HELP_HEADER + m_failureStatus + LIMITED_HELP_MESSAGE); });
}

void UIManager::printAudioInjectionHeader() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(AUDIO_INJECTION_HEADER); });
}

void UIManager::printSettingsScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(SETTINGS_MESSAGE); });
}

#ifdef ENABLE_ENDPOINT_CONTROLLERS
void UIManager::printEndpointModificationScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(ENDPOINT_MODIFICATION_MESSAGE); });
}

void UIManager::printEndpointModificationError(const std::string& message) {
    m_executor.execute([message]() { ConsolePrinter::prettyPrint(message); });
}

void UIManager::printEndpointControllerScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(ENDPOINT_CONTROLLER_MESSAGE); });
}
#endif

#ifdef POWER_CONTROLLER
void UIManager::printPowerControllerScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(POWER_CONTROLLER_OPTIONS); });
}
#endif

#ifdef TOGGLE_CONTROLLER
void UIManager::printToggleControllerScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(TOGGLE_CONTROLLER_OPTIONS); });
}
#endif

#ifdef MODE_CONTROLLER
void UIManager::printModeControllerScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(MODE_CONTROLLER_OPTIONS); });
}
#endif

#ifdef RANGE_CONTROLLER
void UIManager::printRangeControllerScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(RANGE_CONTROLLER_OPTIONS); });
}
#endif

void UIManager::printLocaleScreen() {
    auto supportedLocales = m_localeAssetsManager->getSupportedLocales();
    auto supportedLocaleCombinations = m_localeAssetsManager->getSupportedLocaleCombinations();
    auto printLocaleMessage = [supportedLocales, supportedLocaleCombinations]() {
        auto option = OPTION_ENUM_START;
        std::string optionString;
        for (const auto& locale : supportedLocales) {
            optionString +=
                "| Press '" + std::to_string(option) + "' followed by Enter to change the locale to " + locale + "\n";
            option++;
        }
        for (const auto& combination : supportedLocaleCombinations) {
            optionString +=
                "| Press '" + std::to_string(option) + "' followed by Enter to change the locale combinations to " +
                settings::toSettingString<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface::Locales>(combination)
                    .second +
                "\n";
            option++;
        }
        optionString += "| Press '0' followed by Enter to quit.\n";
        ConsolePrinter::simplePrint(LOCALE_MESSAGE_HEADER + optionString + LOCALE_MESSAGE_FOOTER);
    };

    m_executor.execute(printLocaleMessage);
}

void UIManager::printSpeakerControlScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(SPEAKER_CONTROL_MESSAGE); });
}

void UIManager::printFirmwareVersionControlScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(FIRMWARE_CONTROL_MESSAGE); });
}

void UIManager::printVolumeControlScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(VOLUME_CONTROL_MESSAGE); });
}

#ifdef ENABLE_PCC
void UIManager::printPhoneControlScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(PHONE_CONTROL_MESSAGE); });
}

void UIManager::printCallIdScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(ENTER_CALL_ID_MESSAGE); });
}

void UIManager::printCallerIdScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(ENTER_CALLER_ID_MESSAGE); });
}
#endif

#ifdef ENABLE_MCC
void UIManager::printMeetingControlScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(MEETING_CONTROL_MESSAGE); });
}

void UIManager::printSessionIdScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(ENTER_SESSION_ID_MESSAGE); });
}

void UIManager::printCalendarItemsScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(ENTER_CALENDAR_ITEMS_FILE_PATH_MESSAGE); });
}
#endif

#ifdef ENABLE_COMMS
void UIManager::printCommsControlScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(COMMS_MESSAGE); });
}

void UIManager::printDtmfScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(DTMF_MESSAGE); });
}

void UIManager::printDtmfErrorScreen() {
    m_executor.execute([]() { ConsolePrinter::prettyPrint("Invalid Dtmf Tones"); });
}

void UIManager::printMuteCallScreen() {
    m_executor.execute([]() { ConsolePrinter::prettyPrint("Mute the call"); });
}

void UIManager::printUnmuteCallScreen() {
    m_executor.execute([]() { ConsolePrinter::prettyPrint("Unmute the call"); });
}
#endif

void UIManager::printErrorScreen() {
    m_executor.execute([]() { ConsolePrinter::prettyPrint("Invalid Option"); });
}

void UIManager::microphoneOff() {
    m_executor.execute([]() { ConsolePrinter::prettyPrint("Microphone Off!"); });
}

void UIManager::printResetConfirmation() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(RESET_CONFIRMATION); });
}

void UIManager::printReauthorizeConfirmation() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(REAUTHORIZE_CONFIRMATION); });
}

void UIManager::printResetWarning() {
    m_executor.execute([]() { ConsolePrinter::prettyPrint(RESET_WARNING); });
}

void UIManager::printAlarmVolumeRampScreen() {
    m_executor.execute([]() {
        ConsolePrinter::simplePrint(ALARM_VOLUME_RAMP_HEADER);
        ConsolePrinter::simplePrint(ENABLE_SETTING_MENU);
    });
}

void UIManager::printDoNotDisturbScreen() {
    m_executor.execute([]() {
        ConsolePrinter::simplePrint(DONOTDISTURB_CONFIRMATION_HEADER);
        ConsolePrinter::simplePrint(ENABLE_SETTING_MENU);
    });
}

void UIManager::printWakeWordConfirmationScreen() {
    m_executor.execute([]() {
        ConsolePrinter::simplePrint(WAKEWORD_CONFIRMATION_HEADER);
        ConsolePrinter::simplePrint(ENABLE_SETTING_MENU);
    });
}

void UIManager::printSpeechConfirmationScreen() {
    m_executor.execute([]() {
        ConsolePrinter::simplePrint(SPEECH_CONFIRMATION_HEADER);
        ConsolePrinter::simplePrint(ENABLE_SETTING_MENU);
    });
}

void UIManager::printTimeZoneScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(TIMEZONE_SETTING_MENU); });
}

void UIManager::printNetworkInfoScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(NETWORK_INFO_SETTING_MENU); });
}

void UIManager::printNetworkInfoConnectionTypePrompt() {
    m_executor.execute([]() { ConsolePrinter::prettyPrint(NETWORK_INFO_CONNECTION_TYPE_PROMPT); });
}

void UIManager::printNetworkInfoESSIDPrompt() {
    m_executor.execute([]() { ConsolePrinter::prettyPrint(NETWORK_INFO_ESSID_PROMPT); });
}

void UIManager::printNetworkInfoBSSIDPrompt() {
    m_executor.execute([]() { ConsolePrinter::prettyPrint(NETWORK_INFO_BSSID_PROMPT); });
}

void UIManager::printNetworkInfoIpPrompt() {
    m_executor.execute([]() { ConsolePrinter::prettyPrint(NETWORK_INFO_IP_PROMPT); });
}

void UIManager::printNetworkInfoSubnetPrompt() {
    m_executor.execute([]() { ConsolePrinter::prettyPrint(NETWORK_INFO_SUBNET_MASK_PROMPT); });
}

void UIManager::printNetworkInfoMacPrompt() {
    m_executor.execute([]() { ConsolePrinter::prettyPrint(NETWORK_INFO_MAC_ADDRESS_PROMPT); });
}

void UIManager::printNetworkInfoDHCPPrompt() {
    m_executor.execute([]() { ConsolePrinter::prettyPrint(NETWORK_INFO_DHCP_ADDRESS_PROMPT); });
}

void UIManager::printNetworkInfoStaticIpPrompt() {
    m_executor.execute([]() { ConsolePrinter::prettyPrint(NETWORK_INFO_STATIC_IP_PROMPT); });
}

void UIManager::printDiagnosticsScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(DIAGNOSTICS_SCREEN); });
}

void UIManager::printDevicePropertiesScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(DEVICE_PROPERTIES_SCREEN); });
}

void UIManager::printAllDeviceProperties(const std::unordered_map<std::string, std::string>& deviceProperties) {
    std::stringstream ss;
    for (const auto& keyVal : deviceProperties) {
        ss << keyVal.first << ":" << keyVal.second << std::endl;
    }
    auto propertiesString = ss.str();
    m_executor.execute([propertiesString]() { ConsolePrinter::simplePrint(propertiesString); });
}

void UIManager::printDeviceProtocolTracerScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(DEVICE_PROTOCOL_TRACE_SCREEN); });
}

void UIManager::printProtocolTrace(const std::string& protocolTrace) {
    m_executor.execute([protocolTrace]() { ConsolePrinter::simplePrint(protocolTrace); });
}

void UIManager::printProtocolTraceFlag(bool enabled) {
    m_executor.execute([enabled]() {
        ConsolePrinter::simplePrint("Protocol trace : " + std::string(enabled ? "Enabled" : "Disabled"));
    });
}

void UIManager::printAudioInjectionScreen() {
    m_executor.execute([]() { ConsolePrinter::simplePrint(AUDIO_INJECTION_SCREEN); });
}

void UIManager::printAudioInjectionFailureMessage() {
    m_executor.execute([]() { ConsolePrinter::simplePrint("Failure injecting audio file."); });
}

void UIManager::setUIStateAggregator(
    std::shared_ptr<acsdkSampleApplicationInterfaces::UIStateAggregatorInterface> uiStateAggregator) {
    m_executor.execute([this, uiStateAggregator]() { m_uiStateAggregator = std::move(uiStateAggregator); });
}

void UIManager::setUIAuthNotifier(
    std::shared_ptr<acsdkSampleApplicationInterfaces::UIAuthNotifierInterface> uiAuthNotifier) {
    m_executor.execute([this, uiAuthNotifier]() { m_uiAuthNotifier = std::move(uiAuthNotifier); });
}

void UIManager::microphoneOn() {
    m_executor.execute([this]() { printState(); });
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
    m_executor.execute([msg]() { ConsolePrinter::prettyPrint(msg); });
}

template <typename SettingType>
void UIManager::onSettingNotification(
    const std::string& name,
    SettingType value,
    settings::SettingNotifications notification) {
    std::stringstream stream;
    if (settings::SettingNotifications::LOCAL_CHANGE_FAILED == notification ||
        settings::SettingNotifications::AVS_CHANGE_FAILED == notification) {
        stream << "ERROR: Failed to set " + name + ". ";
    }
    stream << name << " is " << settings::toSettingString<SettingType>(value).second;
    std::string msg = stream.str();
    m_executor.execute([msg]() { ConsolePrinter::prettyPrint(msg); });
}

void UIManager::printState() {
    std::string alexaState;
    if (m_connectionStatus == avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status::DISCONNECTED) {
        ConsolePrinter::prettyPrint("Client not connected!");
        alexaState = "DISCONNECTED";
    } else if (m_connectionStatus == avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status::PENDING) {
        ConsolePrinter::prettyPrint("Connecting...");
        alexaState = "CONNECTING";
    } else if (m_connectionStatus == avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status::CONNECTED) {
        switch (m_dialogState) {
            case DialogUXState::IDLE:
                ConsolePrinter::prettyPrint("Alexa is currently idle!");
                alexaState = "IDLE";
                break;
            case DialogUXState::LISTENING:
                ConsolePrinter::prettyPrint("Listening...");
                alexaState = "LISTENING";
                break;
            case DialogUXState::EXPECTING:
                ConsolePrinter::prettyPrint("Expecting...");
                alexaState = "EXPECTING";
                break;
            case DialogUXState::THINKING:
                ConsolePrinter::prettyPrint("Thinking...");
                alexaState = "THINKING";
                break;
            case DialogUXState::SPEAKING:
                ConsolePrinter::prettyPrint("Speaking...");
                alexaState = "SPEAKING";
                break;
                /*
                 * This is an intermediate state after a SPEAK directive is completed. In the case of a speech burst the
                 * next SPEAK could kick in or if its the last SPEAK directive ALEXA moves to the IDLE state. So we do
                 * nothing for this state.
                 */
            case DialogUXState::FINISHED:
                return;
        }
    }
    if (m_uiStateAggregator && !alexaState.empty()) {
        m_uiStateAggregator->notifyAlexaState(alexaState);
    }
}

void UIManager::printCommsNotSupported() {
    m_executor.execute([]() { ConsolePrinter::simplePrint("Comms is not supported in this device."); });
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
    ok &= m_callbacks->add<DeviceSettingsIndex::ALARM_VOLUME_RAMP>(
        [this](types::AlarmVolumeRampTypes volumeRamp, SettingNotifications notifications) {
            onSettingNotification(ALARM_VOLUME_RAMP_NAME, volumeRamp, notifications);
        });
    ok &= m_callbacks->add<DeviceSettingsIndex::SPEECH_CONFIRMATION>(
        [this](settings::SpeechConfirmationSettingType value, SettingNotifications notifications) {
            onSettingNotification(SPEECH_CONFIRMATION_NAME, value, notifications);
        });
    ok &= m_callbacks->add<DeviceSettingsIndex::WAKEWORD_CONFIRMATION>(
        [this](settings::WakeWordConfirmationSettingType value, SettingNotifications notifications) {
            onSettingNotification(WAKEWORD_CONFIRMATION_NAME, value, notifications);
        });
    ok &= m_callbacks->add<DeviceSettingsIndex::TIMEZONE>(
        [this](const std::string& value, SettingNotifications notifications) {
            onSettingNotification(TIMEZONE_NAME, value, notifications);
        });
    ok &= m_callbacks->add<DeviceSettingsIndex::LOCALE>(
        [this](const settings::DeviceLocales& value, SettingNotifications notifications) {
            onSettingNotification(LOCALE_NAME, value, notifications);
        });
#ifdef KWD
    ok &= m_callbacks->add<DeviceSettingsIndex::WAKE_WORDS>(
        [this](const settings::WakeWords& wakeWords, SettingNotifications notifications) {
            onSettingNotification(WAKE_WORDS_NAME, wakeWords, notifications);
        });
#endif
    return ok;
}

void UIManager::onActiveDeviceConnected(const DeviceAttributes& deviceAttributes) {
    m_executor.execute([deviceAttributes]() {
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
    m_executor.execute([deviceAttributes]() {
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

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK
