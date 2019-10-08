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

#include <cctype>
#include <limits>

#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/String/StringUtils.h>
#include "SampleApp/UserInputManager.h"
#include "SampleApp/ConsolePrinter.h"

namespace alexaClientSDK {
namespace sampleApp {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::softwareInfo;

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
static const char SKIP_FORWARD = '5';
static const char SKIP_BACKWARD = '6';
static const char SHUFFLE = '7';
static const char LOOP = '8';
static const char REPEAT = '9';
static const char THUMBS_UP = '+';
static const char THUMBS_DOWN = '-';
static const char SETTINGS = 'c';
static const char SPEAKER_CONTROL = 'p';
static const char FIRMWARE_VERSION = 'f';
static const char RESET = 'k';
static const char REAUTHORIZE = 'z';
#ifdef ENABLE_COMMS
static const char COMMS_CONTROL = 'd';
#endif

#ifdef ENABLE_PCC
static const char PHONE_CONTROL = 'a';
#endif

#ifdef ENABLE_MCC
static const char MEETING_CONTROL = 'j';
#endif

static constexpr char ENABLE = 'E';
static constexpr char DISABLE = 'D';

enum class SettingsValues : char {
    LOCALE = '1',
    DO_NOT_DISTURB = '2',
    WAKEWORD_CONFIRMATION = '3',
    SPEECH_CONFIRMATION = '4',
    TIME_ZONE = '5'
};

/// The index of the first option in displaying a list of options.
static const unsigned OPTION_ENUM_START = 1;

static const std::unordered_map<char, std::string> TZ_VALUES({{'1', "America/Vancouver"},
                                                              {'2', "America/Edmonton"},
                                                              {'3', "America/Winnipeg"},
                                                              {'4', "America/Toronto"},
                                                              {'5', "America/Halifax"},
                                                              {'6', "America/St_Johns"}});

static const std::unordered_map<char, SpeakerInterface::Type> SPEAKER_TYPES(
    {{'1', SpeakerInterface::Type::AVS_SPEAKER_VOLUME}, {'2', SpeakerInterface::Type::AVS_ALERTS_VOLUME}});

static const int8_t INCREASE_VOLUME = 10;

static const int8_t DECREASE_VOLUME = -10;

/// Time to wait for console input
static const auto READ_CONSOLE_TIMEOUT = std::chrono::milliseconds(100);

/// String to identify log entries originating from this file.
static const std::string TAG("UserInputManager");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<UserInputManager> UserInputManager::create(
    std::shared_ptr<InteractionManager> interactionManager,
    std::shared_ptr<ConsoleReader> consoleReader,
    std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface> localeAssetsManager) {
    if (!interactionManager) {
        ACSDK_CRITICAL(LX("Invalid InteractionManager passed to UserInputManager"));
        return nullptr;
    }

    if (!consoleReader) {
        ACSDK_CRITICAL(LX("Invalid ConsoleReader passed to UserInputManager"));
        return nullptr;
    }

    if (!localeAssetsManager) {
        ACSDK_CRITICAL(LX("Invalid LocaleAssetsManager passed to UserInputManager"));
        return nullptr;
    }

    return std::unique_ptr<UserInputManager>(
        new UserInputManager(interactionManager, consoleReader, localeAssetsManager));
}

UserInputManager::UserInputManager(
    std::shared_ptr<InteractionManager> interactionManager,
    std::shared_ptr<ConsoleReader> consoleReader,
    std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface> localeAssetsManager) :
        m_interactionManager{interactionManager},
        m_consoleReader{consoleReader},
        m_localeAssetsManager{localeAssetsManager},
        m_limitedInteraction{false},
        m_restart{false} {
}

bool UserInputManager::readConsoleInput(char* input) {
    while (input && !m_restart) {
        if (m_consoleReader->read(READ_CONSOLE_TIMEOUT, input)) {
            return true;
        }
    }
    return false;
}

SampleAppReturnCode UserInputManager::run() {
    bool userTriggeredLogout = false;
    m_interactionManager->begin();
    while (true) {
        char x;
        if (!readConsoleInput(&x)) {
            break;
        }
        x = ::tolower(x);
        if (x == QUIT) {
            break;
        } else if (x == RESET) {
            if (confirmReset()) {
                userTriggeredLogout = true;
            }
        } else if (x == REAUTHORIZE) {
            confirmReauthorizeDevice();
        } else if (x == MIC_TOGGLE) {
            m_interactionManager->microphoneToggle();
        } else if (x == STOP) {
            m_interactionManager->stopForegroundActivity();
        } else if (x == SPEAKER_CONTROL) {
            controlSpeaker();
#ifdef ENABLE_PCC
        } else if (x == PHONE_CONTROL) {
            controlPhone();
#endif
#ifdef ENABLE_MCC
        } else if (x == MEETING_CONTROL) {
            controlMeeting();
#endif
        } else if (x == SETTINGS) {
            settingsMenu();
        } else if (x == INFO) {
            if (m_limitedInteraction) {
                m_interactionManager->limitedHelp();
            } else {
                m_interactionManager->help();
            }
        } else if (m_limitedInteraction) {
            m_interactionManager->errorValue();
            // ----- Add a new interaction bellow if the action is available only in 'unlimited interaction mode'.
        } else if (x == HOLD) {
            m_interactionManager->holdToggled();
        } else if (x == TAP) {
            m_interactionManager->tap();
        } else if (x == PLAY) {
            m_interactionManager->playbackPlay();
        } else if (x == PAUSE) {
            m_interactionManager->playbackPause();
        } else if (x == NEXT) {
            m_interactionManager->playbackNext();
        } else if (x == PREVIOUS) {
            m_interactionManager->playbackPrevious();
        } else if (x == SKIP_FORWARD) {
            m_interactionManager->playbackSkipForward();
        } else if (x == SKIP_BACKWARD) {
            m_interactionManager->playbackSkipBackward();
        } else if (x == SHUFFLE) {
            m_interactionManager->playbackShuffle();
        } else if (x == LOOP) {
            m_interactionManager->playbackLoop();
        } else if (x == REPEAT) {
            m_interactionManager->playbackRepeat();
        } else if (x == THUMBS_UP) {
            m_interactionManager->playbackThumbsUp();
        } else if (x == THUMBS_DOWN) {
            m_interactionManager->playbackThumbsDown();
        } else if (x == FIRMWARE_VERSION) {
            std::string text;
            std::getline(std::cin, text);
            // If text is empty the user entered newline right after the command key.
            // Prompt for the version number and get the version from the next line.
            if (text.empty()) {
                m_interactionManager->firmwareVersionControl();
                std::getline(std::cin, text);
            }
            int version;
            const long long int maxValue = MAX_FIRMWARE_VERSION;
            if (avsCommon::utils::string::stringToInt(text, &version) && version > 0 && version <= maxValue) {
                m_interactionManager->setFirmwareVersion(static_cast<FirmwareVersion>(version));
            } else {
                m_interactionManager->errorValue();
            }
#ifdef ENABLE_COMMS
        } else if (x == COMMS_CONTROL) {
            m_interactionManager->commsControl();
            char commsChoice;
            bool continueWhileLoop = true;
            while (continueWhileLoop) {
                std::cin >> commsChoice;
                switch (commsChoice) {
                    case 'a':
                    case 'A':
                        m_interactionManager->acceptCall();
                        break;
                    case 's':
                    case 'S':
                        m_interactionManager->stopCall();
                        break;
                    case 'q':
                        m_interactionManager->help();
                        continueWhileLoop = false;
                        break;
                    default:
                        m_interactionManager->errorValue();
                        continueWhileLoop = false;
                        break;
                }
            }
#endif
        } else {
            m_interactionManager->errorValue();
        }
    }
    if (!userTriggeredLogout && m_restart) {
        return SampleAppReturnCode::RESTART;
    }
    return SampleAppReturnCode::OK;
}

void UserInputManager::onLogout() {
    m_restart = true;
}

void UserInputManager::onAuthStateChange(AuthObserverInterface::State newState, AuthObserverInterface::Error newError) {
    m_limitedInteraction = m_limitedInteraction || (newState == AuthObserverInterface::State::UNRECOVERABLE_ERROR);
}

void UserInputManager::onCapabilitiesStateChange(
    CapabilitiesObserverInterface::State newState,
    CapabilitiesObserverInterface::Error newError) {
    m_limitedInteraction = m_limitedInteraction || (newState == CapabilitiesObserverInterface::State::FATAL_ERROR);
}

void UserInputManager::controlSpeaker() {
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
}

#ifdef ENABLE_PCC
void UserInputManager::controlPhone() {
    std::string callerId;

    m_interactionManager->callId();
    std::string callId;
    std::cin >> callId;
    m_interactionManager->phoneControl();
    char phoneChoice;
    std::cin >> phoneChoice;
    switch (phoneChoice) {
        case '1':
            m_interactionManager->sendCallActivated(callId);
            break;
        case '2':
            m_interactionManager->sendCallTerminated(callId);
            break;
        case '3':
            m_interactionManager->sendCallFailed(callId);
            break;
        case '4':
            m_interactionManager->callerId();
            std::cin >> callerId;
            m_interactionManager->sendCallReceived(callId, callerId);
            break;
        case '5':
            m_interactionManager->callerId();
            std::cin >> callerId;
            m_interactionManager->sendCallerIdReceived(callId, callerId);
            break;
        case '6':
            m_interactionManager->sendInboundRingingStarted(callId);
            break;
        case '7':
            m_interactionManager->sendOutboundCallRequested(callId);
            break;
        case '8':
            m_interactionManager->sendOutboundRingingStarted(callId);
            break;
        case '9':
            m_interactionManager->sendSendDtmfSucceeded(callId);
            break;
        case '0':
            m_interactionManager->sendSendDtmfFailed(callId);
            break;
        case 'q':
            m_interactionManager->help();
            break;
        case 'i':
            m_interactionManager->phoneControl();
            break;
        default:
            m_interactionManager->errorValue();
            break;
    }
}

#endif

#ifdef ENABLE_MCC
void UserInputManager::controlMeeting() {
    std::string sessionId;
    std::string calendarItemsFile;

    m_interactionManager->meetingControl();
    char meetingChoice;
    std::cin >> meetingChoice;
    switch (meetingChoice) {
        case '1':
            m_interactionManager->sessionId();
            std::cin >> sessionId;
            m_interactionManager->sendMeetingJoined(sessionId);
            break;
        case '2':
            m_interactionManager->sessionId();
            std::cin >> sessionId;
            m_interactionManager->sendMeetingEnded(sessionId);
            break;
        case '3':
            m_interactionManager->calendarItemsFile();
            std::cin >> calendarItemsFile;
            m_interactionManager->sendCalendarItemsRetrieved(calendarItemsFile);
            break;
        case '4':
            m_interactionManager->sessionId();
            std::cin >> sessionId;
            m_interactionManager->sendSetCurrentMeetingSession(sessionId);
            break;
        case '5':
            m_interactionManager->sendClearCurrentMeetingSession();
            break;
        case '6':
            m_interactionManager->sendConferenceConfigurationChanged();
            break;
        case '7':
            m_interactionManager->sessionId();
            std::cin >> sessionId;
            m_interactionManager->sendMeetingClientErrorOccured(sessionId);
            break;
        case '8':
            m_interactionManager->sendCalendarClientErrorOccured();
            break;
        case 'q':
            m_interactionManager->help();
            break;
        case 'i':
            m_interactionManager->meetingControl();
            break;
        default:
            m_interactionManager->errorValue();
            break;
    }
}
#endif

bool UserInputManager::confirmReset() {
    m_interactionManager->confirmResetDevice();
    char y;
    do {
        std::cin >> y;
        // Check the Setting which has to be changed.
        switch (y) {
            case 'Y':
            case 'y':
                m_interactionManager->resetDevice();
                return true;
            case 'N':
            case 'n':
                return false;
            default:
                m_interactionManager->errorValue();
                m_interactionManager->confirmResetDevice();
                break;
        }
    } while (true);

    return false;
}

bool UserInputManager::confirmReauthorizeDevice() {
    m_interactionManager->confirmReauthorizeDevice();
    char y;
    do {
        std::cin >> y;
        // Check the Setting which has to be changed.
        switch (y) {
            case 'Y':
            case 'y':
                m_interactionManager->resetDevice();
                return true;
            case 'N':
            case 'n':
                return false;
            default:
                m_interactionManager->errorValue();
                m_interactionManager->confirmReauthorizeDevice();
                break;
        }
    } while (true);

    return false;
}

void boolSettingMenu(std::function<void(bool)> setFunction) {
    char y;
    std::cin >> y;
    switch (y) {
        case ENABLE:
            setFunction(true);
            return;
        case DISABLE:
            setFunction(false);
            return;
    }
}

void UserInputManager::settingsMenu() {
    m_interactionManager->settings();
    char y;
    std::cin >> y;
    // Check the Setting which has to be changed.
    switch (y) {
        case (char)SettingsValues::LOCALE: {
            unsigned int optionSelected;
            m_interactionManager->locale();
            std::cin >> optionSelected;
            auto numOfSupportedLocales = m_localeAssetsManager->getSupportedLocales().size();
            auto numOfSupportedLocaleCombinations = m_localeAssetsManager->getSupportedLocaleCombinations().size();
            if (optionSelected <= numOfSupportedLocales && optionSelected >= OPTION_ENUM_START) {
                std::string locale = *std::next(
                    m_localeAssetsManager->getSupportedLocales().begin(), optionSelected - OPTION_ENUM_START);
                m_interactionManager->setLocale({locale});
            } else if (
                optionSelected > numOfSupportedLocales &&
                optionSelected <= (numOfSupportedLocales + numOfSupportedLocaleCombinations)) {
                auto index = optionSelected - numOfSupportedLocales - OPTION_ENUM_START;
                auto locales = *std::next(m_localeAssetsManager->getSupportedLocaleCombinations().begin(), index);
                m_interactionManager->setLocale(locales);
            } else {
                m_interactionManager->errorValue();
            }
            // Clear error flag on cin (so that future I/O operations will work correctly) in case of incorrect input.
            std::cin.clear();
            // Ignore anything else on the same line as the non-number so that it does not cause another parse failure.
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return;
        }
        case (char)SettingsValues::DO_NOT_DISTURB: {
            m_interactionManager->doNotDisturb();
            boolSettingMenu([this](bool enable) { m_interactionManager->setDoNotDisturbMode(enable); });
            return;
        }
        case (char)SettingsValues::WAKEWORD_CONFIRMATION: {
            m_interactionManager->wakewordConfirmation();
            boolSettingMenu([this](bool enable) {
                m_interactionManager->setWakewordConfirmation(
                    enable ? settings::WakeWordConfirmationSettingType::TONE
                           : settings::WakeWordConfirmationSettingType::NONE);
            });
            return;
        }
        case (char)SettingsValues::SPEECH_CONFIRMATION: {
            m_interactionManager->speechConfirmation();
            boolSettingMenu([this](bool enable) {
                m_interactionManager->setSpeechConfirmation(
                    enable ? settings::SpeechConfirmationSettingType::TONE
                           : settings::SpeechConfirmationSettingType::NONE);
            });
            return;
        }
        case (char)SettingsValues::TIME_ZONE: {
            m_interactionManager->timeZone();
            char tzValue;
            std::cin >> tzValue;
            auto searchTz = TZ_VALUES.find(tzValue);
            if (searchTz != TZ_VALUES.end()) {
                m_interactionManager->setTimeZone(searchTz->second);
            } else {
                m_interactionManager->errorValue();
            }
            return;
        }
    }
    m_interactionManager->help();
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
