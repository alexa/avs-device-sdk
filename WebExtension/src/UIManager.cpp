/*
 * UIManager.cpp
 *
 * Copyright (c) 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <sstream>

#include "WebExtension/UIManager.h"

#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>

#include "WebExtension/ConsolePrinter.h"

namespace alexaClientSDK {
namespace webExtension {

using namespace avsCommon::sdkInterfaces;

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
    "       #####  #    # #    # #      ###### ######    #     # #      #          \n";

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
    "| Settings:                                                                  |\n"
    "|       Press 'c' followed by Enter at any time to see the settings screen.  |\n"
    "| Speaker Control:                                                           |\n"
    "|       Press 'p' followed by Enter at any time to adjust speaker settings.  |\n"
    "| Info:                                                                      |\n"
    "|       Press 'i' followed by Enter at any time to see the help screen.      |\n"
    "| Quit:                                                                      |\n"
    "|       Press 'q' followed by Enter at any time to quit the application.     |\n"
    "+----------------------------------------------------------------------------+\n";

static const std::string SETTINGS_MESSAGE =
    "+----------------------------------------------------------------------------+\n"
    "|                          Setting Options:                                  |\n"
    "| Change Language:                                                           |\n"
    "|       Press '1' followed by Enter to see language options.                 |\n"
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
    "+----------------------------------------------------------------------------+\n";

static const std::string SPEAKER_CONTROL_MESSAGE =
    "+----------------------------------------------------------------------------+\n"
    "|                          Speaker Options:                                  |\n"
    "|                                                                            |\n"
    "| Press '1' followed by Enter to modify AVS_SYNCED typed speakers.           |\n"
    "|       AVS_SYNCED Speakers Control Volume For: Speech, Content.             |\n"
    "| Press '2' followed by Enter to modify LOCAL typed speakers.                |\n"
    "|       LOCAL Speakers Control Volume For: Alerts.                           |\n"
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

void UIManager::printWelcomeScreen() {
    m_executor.submit([]() { ConsolePrinter::simplePrint(ALEXA_WELCOME_MESSAGE); });
}

void UIManager::printHelpScreen() {
    m_executor.submit([]() { ConsolePrinter::simplePrint(HELP_MESSAGE); });
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

void UIManager::printVolumeControlScreen() {
    m_executor.submit([]() { ConsolePrinter::simplePrint(VOLUME_CONTROL_MESSAGE); });
}

void UIManager::printErrorScreen() {
    m_executor.submit([]() { ConsolePrinter::prettyPrint("Invalid Option"); });
}

void UIManager::microphoneOff() {
    m_executor.submit([]() { ConsolePrinter::prettyPrint("Microphone Off!"); });
}

void UIManager::microphoneOn() {
    m_executor.submit([this]() { printState(); });
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
            case DialogUXState::THINKING:
                ConsolePrinter::prettyPrint("Thinking...");
                return;
                ;
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

}  // namespace webExtension
}  // namespace alexaClientSDK
