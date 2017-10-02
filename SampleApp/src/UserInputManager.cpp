/*
 * UserInputManager.cpp
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

#include "SampleApp/UserInputManager.h"
#include "SampleApp/ConsolePrinter.h"

#include <cctype>

namespace alexaClientSDK {
namespace sampleApp {

static const char HOLD = 'h';
static const char TAP = 't';
static const char QUIT = 'q';
static const char INFO = 'i';
static const char MIC_TOGGLE = 'm';
static const char STOP = 's';
static const char PLAY = '1';
static const char PAUSE = '2';
static const char NEXT = '3';
static const char PREVIOUS = '4';
static const char SETTINGS = 'c';

enum class SettingsValues : char { LOCALE = '1' };

static const std::unordered_map<char, std::string> LOCALE_VALUES({{'1', "en-US"}, {'2', "en-GB"}, {'3', "de-DE"}});

std::unique_ptr<UserInputManager> UserInputManager::create(std::shared_ptr<InteractionManager> interactionManager) {
    if (!interactionManager) {
        ConsolePrinter::simplePrint("Invalid InteractionManager passed to UserInputManager");
        return nullptr;
    }
    return std::unique_ptr<UserInputManager>(new UserInputManager(interactionManager));
}

UserInputManager::UserInputManager(std::shared_ptr<InteractionManager> interactionManager) :
        m_interactionManager{interactionManager} {
}

void UserInputManager::run() {
    if (!m_interactionManager) {
        return;
    }
    m_interactionManager->begin();
    while (true) {
        char x;
        std::cin >> x;
        x = ::tolower(x);
        if (x == QUIT) {
            return;
        } else if (x == INFO) {
            m_interactionManager->help();
        } else if (x == MIC_TOGGLE) {
            m_interactionManager->microphoneToggle();
        } else if (x == HOLD) {
            m_interactionManager->holdToggled();
        } else if (x == TAP) {
            m_interactionManager->tap();
        } else if (x == STOP) {
            m_interactionManager->stopForegroundActivity();
        } else if (x == PLAY) {
            m_interactionManager->playbackPlay();
        } else if (x == PAUSE) {
            m_interactionManager->playbackPause();
        } else if (x == NEXT) {
            m_interactionManager->playbackNext();
        } else if (x == PREVIOUS) {
            m_interactionManager->playbackPrevious();
        } else if (x == SETTINGS) {
            m_interactionManager->settings();
            char y;
            std::cin >> y;
            // Check the Setting which has to be changed.
            switch (y) {
                case (char)SettingsValues::LOCALE: {
                    char localeValue;
                    m_interactionManager->locale();
                    std::cin >> localeValue;
                    auto searchLocale = LOCALE_VALUES.find(localeValue);
                    if (searchLocale != LOCALE_VALUES.end()) {
                        m_interactionManager->changeSetting("locale", searchLocale->second);
                    } else {
                        m_interactionManager->errorValue();
                    }
                    break;
                }
                default:
                    m_interactionManager->errorValue();
                    break;
            }
            m_interactionManager->help();
        }
    }
}

}  // namespace sampleApp
}  // namespace alexaClientSDK