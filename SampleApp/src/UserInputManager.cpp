/*
 * UserInputManager.cpp
 *
 * Copyright (c) 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <cctype>

#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>
#include <AVSCommon/Utils/String/StringUtils.h>
#include "SampleApp/UserInputManager.h"
#include "SampleApp/ConsolePrinter.h"

namespace alexaClientSDK {
namespace sampleApp {

using namespace avsCommon::sdkInterfaces;

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
static const char SPEAKER_CONTROL = 'p';
static const char FIRMWARE_VERSION = 'f';

enum class SettingsValues : char { LOCALE = '1' };

static const std::unordered_map<char, std::string> LOCALE_VALUES(
    {{'1', "en-US"}, {'2', "en-GB"}, {'3', "de-DE"}, {'4', "en-IN"}, {'5', "en-CA"}, {'6', "ja-JP"}});

static const std::unordered_map<char, SpeakerInterface::Type> SPEAKER_TYPES({{'1', SpeakerInterface::Type::AVS_SYNCED},
                                                                             {'2', SpeakerInterface::Type::LOCAL}});

static const int8_t INCREASE_VOLUME = 10;

static const int8_t DECREASE_VOLUME = -10;

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
            m_interactionManager->shutdown();
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
                    m_interactionManager->help();
            }
        } else if (x == SPEAKER_CONTROL) {
            m_interactionManager->speakerControl();
            char speakerChoice;
            std::cin >> speakerChoice;
            if (SPEAKER_TYPES.count(speakerChoice) == 0) {
                m_interactionManager->errorValue();
            } else {
                m_interactionManager->volumeControl();
                SpeakerInterface::Type speaker = SPEAKER_TYPES.at(speakerChoice);
                char volume;
                while (std::cin >> volume && volume != 'q') {
                    switch (volume) {
                        case '1':
                            m_interactionManager->adjustVolume(speaker, INCREASE_VOLUME);
                            break;
                        case '2':
                            m_interactionManager->adjustVolume(speaker, DECREASE_VOLUME);
                            break;
                        case '3':
                            m_interactionManager->setMute(speaker, true);
                            break;
                        case '4':
                            m_interactionManager->setMute(speaker, false);
                            break;
                        case 'i':
                            m_interactionManager->volumeControl();
                            break;
                        default:
                            m_interactionManager->errorValue();
                            break;
                    }
                }
            }
        } else if (x == FIRMWARE_VERSION) {
            m_interactionManager->firmwareVersionControl();
            std::string text;
            std::cin >> text;
            int version;
            if (avsCommon::utils::string::stringToInt(text, &version) && version > 0) {
                m_interactionManager->setFirmwareVersion(
                    static_cast<avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion>(version));
            } else {
                m_interactionManager->errorValue();
            }
        }
    }
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
