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

#include "SampleApp/UIManager.h"

#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>

#include "SampleApp/ConsolePrinter.h"

namespace alexaClientSDK {
namespace sampleApp {

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
"| Info:                                                                      |\n"
"|       Press 'i' followed by Enter at any time to see the help screen.      |\n"
"| Quit:                                                                      |\n"
"|       Press 'q' followed by Enter at any time to quit the application.     |\n"
"+----------------------------------------------------------------------------+\n";

void UIManager::onDialogUXStateChanged(DialogUXState state) {
    m_executor.submit(
        [this, state] () {
            if (state == m_dialogState) {
                return;
            }
            m_dialogState = state;
            printState();
        }
    );
}

void UIManager::onConnectionStatusChanged(const Status status, const ChangedReason reason) {
    m_executor.submit(
        [this, status] () {
            if (m_connectionStatus == status) {
                return;
            }
            m_connectionStatus = status;
            printState();
        }
    );
}

void UIManager::printWelcomeScreen() {
    m_executor.submit(
        [] () {
            ConsolePrinter::simplePrint(ALEXA_WELCOME_MESSAGE);
        }
    );
}

void UIManager::printHelpScreen() {
    m_executor.submit(
        [] () {
            ConsolePrinter::simplePrint(HELP_MESSAGE);
        }
    );
}

void UIManager::microphoneOff() {
    m_executor.submit(
        [] () {
            ConsolePrinter::prettyPrint("Microphone Off!");
        }
    );
}

void UIManager::microphoneOn() {
    m_executor.submit(
        [this] () {
            printState();
        }
    );
}

void UIManager::printState() {
    if (m_connectionStatus == avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status::DISCONNECTED) {
        ConsolePrinter::prettyPrint("Client not connected!");
    } else if (m_connectionStatus == avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status::PENDING) {
        ConsolePrinter::prettyPrint("Connecting...");
    } else if (m_connectionStatus == avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status::CONNECTED) {
        ConsolePrinter::prettyPrint("Performing post connect actions...");
    } else {
        switch (m_dialogState) {
            case DialogUXState::IDLE:
                ConsolePrinter::prettyPrint("Alexa is currently idle!");
                return;
            case DialogUXState::LISTENING:
                ConsolePrinter::prettyPrint("Listening...");
                return;
            case DialogUXState::THINKING:
                ConsolePrinter::prettyPrint("Thinking...");
                return;;
            case DialogUXState::SPEAKING:
                ConsolePrinter::prettyPrint("Speaking...");
 		return;
            /*
             * This is an intermediate state after a SPEAK directive is completed. In the case of a speech burst the next SPEAK 
             * could kick in or if its the last SPEAK directive ALEXA moves to the IDLE state. So we do nothing for this state.
             */
            case DialogUXState::FINISHED:
                return;
        }
    }
}

} // namespace sampleApp
} // namespace alexaClientSDK\