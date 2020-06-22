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

#include <cctype>
#include <limits>

#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/String/StringUtils.h>
#include "SampleApp/UserInputManager.h"
#include "SampleApp/ConsolePrinter.h"

#ifdef MODE_CONTROLLER
#include "SampleApp/PeripheralEndpoint/PeripheralEndpointModeControllerHandler.h"
#endif

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

#ifdef ENABLE_ENDPOINT_CONTROLLERS
static const char ENDPOINT_CONTROLLER = 'e';
static const char DYNAMIC_ENDPOINT_SCREEN = 'y';
#endif

#ifdef DIAGNOSTICS
static const char DIAGNOSTICS_CONTROL = 'o';
#endif

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
    TIME_ZONE = '5',
    NETWORK_INFO = '6',
    ALARM_VOLUME_RAMP = '7',
    QUIT = 'q'
};

#ifdef ENABLE_COMMS
static const std::unordered_map<char, CallManagerInterface::DTMFTone> dtmfToneMaps = {
    {'0', CallManagerInterface::DTMFTone::DTMF_ZERO},
    {'1', CallManagerInterface::DTMFTone::DTMF_ONE},
    {'2', CallManagerInterface::DTMFTone::DTMF_TWO},
    {'3', CallManagerInterface::DTMFTone::DTMF_THREE},
    {'4', CallManagerInterface::DTMFTone::DTMF_FOUR},
    {'5', CallManagerInterface::DTMFTone::DTMF_FIVE},
    {'6', CallManagerInterface::DTMFTone::DTMF_SIX},
    {'7', CallManagerInterface::DTMFTone::DTMF_SEVEN},
    {'8', CallManagerInterface::DTMFTone::DTMF_EIGHT},
    {'9', CallManagerInterface::DTMFTone::DTMF_NINE},
    {'*', CallManagerInterface::DTMFTone::DTMF_STAR},
    {'#', CallManagerInterface::DTMFTone::DTMF_POUND},
};
#endif

#ifdef ENABLE_ENDPOINT_CONTROLLERS
enum class DynamicEndpointModificationMenuChoice : char {
    ADD_ENDPOINT = 'a',
    MODIFY_ENDPOINT = 'm',
    DELETE_ENDPOINT = 'd',
    QUIT = 'q'
};

enum class EndpointControllerMenuChoice : char {
#ifdef POWER_CONTROLLER
    POWER_CONTROLLER_OPTION = '1',
#endif

#ifdef TOGGLE_CONTROLLER
    TOGGLE_CONTROLLER_OPTION = '2',
#endif

#ifdef MODE_CONTROLLER
    MODE_CONTROLLER_OPTION = '3',
#endif

#ifdef RANGE_CONTROLLER
    RANGE_CONTROLLER_OPTION = '4',
#endif
    QUIT = 'q'
};

#ifdef POWER_CONTROLLER
static const char POWER_CONTROLLER_ON = '1';
static const char POWER_CONTROLLER_OFF = '2';
#endif

#ifdef TOGGLE_CONTROLLER
static const char TOGGLE_CONTROLLER_ON = '1';
static const char TOGGLE_CONTROLLER_OFF = '2';
#endif

#ifdef MODE_CONTROLLER
static const char MODE_RED = '1';
static const char MODE_GREEN = '2';
static const char MODE_BLUE = '3';
#endif

#endif

/// The index of the first option in displaying a list of options.
static const unsigned OPTION_ENUM_START = 1;

/// The number used to quit a numeric options menu
static const unsigned OPTION_ENUM_QUIT = 0;

static const std::unordered_map<char, std::string> TZ_VALUES({{'1', "America/Vancouver"},
                                                              {'2', "America/Edmonton"},
                                                              {'3', "America/Winnipeg"},
                                                              {'4', "America/Toronto"},
                                                              {'5', "America/Halifax"},
                                                              {'6', "America/St_Johns"}});

static const std::unordered_map<char, ChannelVolumeInterface::Type> SPEAKER_TYPES(
    {{'1', ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME}, {'2', ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME}});

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
    std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface> localeAssetsManager,
    const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& defaultEndpointId) {
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
        new UserInputManager(interactionManager, consoleReader, localeAssetsManager, defaultEndpointId));
}

UserInputManager::UserInputManager(
    std::shared_ptr<InteractionManager> interactionManager,
    std::shared_ptr<ConsoleReader> consoleReader,
    std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface> localeAssetsManager,
    const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& defaultEndpointId) :
        m_interactionManager{interactionManager},
        m_consoleReader{consoleReader},
        m_localeAssetsManager{localeAssetsManager},
        m_defaultEndpointId{defaultEndpointId},
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

#ifdef ENABLE_COMMS
static bool isValidDtmf(const std::string& dtmfTones) {
    int dtmfLength = dtmfTones.length();
    for (int i = 0; i < dtmfLength; i++) {
        auto dtmfIterator = dtmfToneMaps.find(dtmfTones[i]);
        if (dtmfIterator == dtmfToneMaps.end()) {
            return false;
        }
    }
    return true;
}

bool UserInputManager::sendDtmf(const std::string& dtmfTones) {
    int dtmfLength = dtmfTones.length();
    if (!isValidDtmf(dtmfTones)) {
        return false;
    }

    for (int i = 0; i < dtmfLength; i++) {
        auto dtmfIterator = dtmfToneMaps.find(dtmfTones[i]);
        m_interactionManager->sendDtmf(dtmfIterator->second);
    }
    return true;
}
#endif

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
#ifdef ENABLE_ENDPOINT_CONTROLLERS
        } else if (x == ENDPOINT_CONTROLLER) {
            endpointControllerMenu();
        } else if (x == DYNAMIC_ENDPOINT_SCREEN) {
            dynamicEndpointModificationMenu();
#endif
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
            std::string dtmfTones;
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
                    case 'd':
                    case 'D':
                        m_interactionManager->dtmfControl();
                        std::cin >> dtmfTones;
                        if (!sendDtmf(dtmfTones)) {
                            m_interactionManager->errorDtmf();
                        }
                        break;
                    case 'm':
                    case 'M':
                        m_interactionManager->muteCallToggle();
                        break;
                    case 'q':
                        m_interactionManager->help();
                        continueWhileLoop = false;
                        break;
                    default:
                        m_interactionManager->errorValue();
                        m_interactionManager->commsControl();
                        break;
                }
            }
#endif
#ifdef DIAGNOSTICS
        } else if (x == DIAGNOSTICS_CONTROL) {
            diagnosticsMenu();
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
    CapabilitiesObserverInterface::Error newError,
    const std::vector<avsCommon::sdkInterfaces::endpoints::EndpointIdentifier>& addedOrUpdatedEndpoints,
    const std::vector<avsCommon::sdkInterfaces::endpoints::EndpointIdentifier>& deletedEndpoints) {
    // If one of the added or updated endpointIds is the default endpoint, and the
    // add/update failed, go into limited mode.
    // Limited mode is unnecessary if the failure is for non-default endpoints.
    bool shouldLimitInteration = false;
    if (CapabilitiesObserverInterface::State::FATAL_ERROR == newState) {
        auto it = std::find(addedOrUpdatedEndpoints.begin(), addedOrUpdatedEndpoints.end(), m_defaultEndpointId);
        if (addedOrUpdatedEndpoints.end() != it) {
            shouldLimitInteration = true;
        }
    }

    m_interactionManager->clearCachedEndpointIdentifiers(deletedEndpoints);
    m_limitedInteraction = m_limitedInteraction || shouldLimitInteration;
}

void UserInputManager::controlSpeaker() {
    m_interactionManager->speakerControl();
    char speakerChoice;
    std::cin >> speakerChoice;
    if (SPEAKER_TYPES.count(speakerChoice) == 0) {
        m_interactionManager->errorValue();
    } else {
        m_interactionManager->volumeControl();
        ChannelVolumeInterface::Type speaker = SPEAKER_TYPES.at(speakerChoice);
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
        case QUIT:
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
            if (!std::cin.fail()) {
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
                } else if (OPTION_ENUM_QUIT == optionSelected) {
                    return;
                } else {
                    m_interactionManager->errorValue();
                }
            } else {
                m_interactionManager->errorValue();
                // Clear error flag on cin (so that future I/O operations will work correctly) in case of incorrect
                // input.
                std::cin.clear();
                // Ignore anything else on the same line as the non-number so that it does not cause another parse
                // failure.
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
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
            } else if (QUIT == tzValue) {
                return;
            } else {
                m_interactionManager->errorValue();
            }
            return;
        }
        case (char)SettingsValues::NETWORK_INFO: {
            bool inNetworkMenu = true;
            do {
                m_interactionManager->networkInfo();
                using settings::types::NetworkInfo;
                auto setOrReset = [](std::function<void()> reset, std::function<void(const std::string&)> set) {
                    std::string input;
                    std::cin.ignore();
                    std::getline(std::cin, input);
                    if (input.empty()) {
                        reset();
                    } else {
                        set(input);
                    }
                };
                char input = 0;
                std::cin >> input;
                const auto constNetInfo = m_interactionManager->getNetworkInfo();
                auto netInfo = constNetInfo;
                switch (input) {
                    case '1': {
                        std::cout << constNetInfo << std::endl;
                        break;
                    }
                    case '2': {
                        m_interactionManager->networkInfoConnectionTypePrompt();
                        char connectionType = 0;
                        std::cin >> connectionType;
                        switch (connectionType) {
                            case '1':
                                netInfo.setConnectionType(NetworkInfo::ConnectionType::ETHERNET);
                                break;
                            case '2':
                                netInfo.setConnectionType(NetworkInfo::ConnectionType::WIFI);
                                break;
                            case '3':
                                netInfo.resetConnectionType();
                                break;
                            default:
                                m_interactionManager->errorValue();
                        }
                        break;
                    }
                    case '3': {
                        m_interactionManager->networkInfoESSIDPrompt();
                        setOrReset(
                            std::bind(&NetworkInfo::resetEssid, &netInfo),
                            std::bind(&NetworkInfo::setEssid, &netInfo, std::placeholders::_1));
                        break;
                    }
                    case '4': {
                        m_interactionManager->networkInfoBSSIDPrompt();
                        setOrReset(
                            std::bind(&NetworkInfo::resetBssid, &netInfo),
                            std::bind(&NetworkInfo::setBssid, &netInfo, std::placeholders::_1));
                        break;
                    }
                    case '5': {
                        m_interactionManager->networkInfoIpPrompt();
                        setOrReset(
                            std::bind(&NetworkInfo::resetIpAddress, &netInfo),
                            std::bind(&NetworkInfo::setIpAddress, &netInfo, std::placeholders::_1));
                        break;
                    }
                    case '6': {
                        m_interactionManager->networkInfoSubnetPrompt();
                        setOrReset(
                            std::bind(&NetworkInfo::resetSubnetMask, &netInfo),
                            std::bind(&NetworkInfo::setSubnetMask, &netInfo, std::placeholders::_1));
                        break;
                    }
                    case '7': {
                        m_interactionManager->networkInfoMacPrompt();
                        setOrReset(
                            std::bind(&NetworkInfo::resetMacAddress, &netInfo),
                            std::bind(&NetworkInfo::setMacAddress, &netInfo, std::placeholders::_1));
                        break;
                    }
                    case '8': {
                        m_interactionManager->networkInfoDHCPPrompt();
                        setOrReset(
                            std::bind(&NetworkInfo::resetDhcpServerAddress, &netInfo),
                            std::bind(&NetworkInfo::setDhcpServerAddress, &netInfo, std::placeholders::_1));
                        break;
                    }
                    case '9': {
                        m_interactionManager->networkInfoStaticIpPrompt();
                        char ipType = 0;
                        std::cin >> ipType;
                        switch (ipType) {
                            case '1':
                                netInfo.setIsStaticIP(true);
                                break;
                            case '2':
                                netInfo.setIsStaticIP(false);
                                break;
                            case '3':
                                netInfo.resetIsStaticIP();
                                break;
                            default:
                                m_interactionManager->errorValue();
                        }
                        break;
                    }
                    case QUIT: {
                        inNetworkMenu = false;
                        break;
                    }
                    default: {
                        m_interactionManager->errorValue();
                        inNetworkMenu = false;
                        break;
                    }
                }
                if (netInfo != constNetInfo) {
                    m_interactionManager->setNetworkInfo(netInfo);
                }
            } while (inNetworkMenu);
            return;
        }
        case (char)SettingsValues::ALARM_VOLUME_RAMP: {
            m_interactionManager->alarmVolumeRamp();
            boolSettingMenu([this](bool enable) { m_interactionManager->setAlarmVolumeRamp(enable); });
            return;
        }
        case (char)SettingsValues::QUIT: {
            return;
        }
        default: {
            m_interactionManager->errorValue();
            return;
        }
    }
}

#ifdef ENABLE_ENDPOINT_CONTROLLERS
void UserInputManager::dynamicEndpointModificationMenu() {
    m_interactionManager->endpointModification();
    char input;
    std::cin >> input;
    switch (input) {
        case static_cast<char>(DynamicEndpointModificationMenuChoice::ADD_ENDPOINT):
            m_interactionManager->addDynamicEndpoint();
            break;
        case static_cast<char>(DynamicEndpointModificationMenuChoice::MODIFY_ENDPOINT):
            m_interactionManager->modifyDynamicEndpoint();
            break;
        case static_cast<char>(DynamicEndpointModificationMenuChoice::DELETE_ENDPOINT):
            m_interactionManager->deleteDynamicEndpoint();
            break;
        case static_cast<char>(DynamicEndpointModificationMenuChoice::QUIT):
            return;
        default:
            m_interactionManager->errorValue();
            // Clear error flag on cin (so that future I/O operations will work correctly) in case of incorrect
            // input.
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

void UserInputManager::endpointControllerMenu() {
    m_interactionManager->endpointController();
    char y;
    std::cin >> y;
    switch (y) {
#ifdef POWER_CONTROLLER
        case static_cast<char>(EndpointControllerMenuChoice::POWER_CONTROLLER_OPTION): {
            char optionSelected;
            m_interactionManager->powerController();
            std::cin >> optionSelected;
            if (!std::cin.fail()) {
                if (optionSelected == POWER_CONTROLLER_ON) {
                    m_interactionManager->setPowerState(true);
                } else if (optionSelected == POWER_CONTROLLER_OFF) {
                    m_interactionManager->setPowerState(false);
                } else if (QUIT == optionSelected) {
                    return;
                } else {
                    m_interactionManager->errorValue();
                }
            } else {
                m_interactionManager->errorValue();
                // Clear error flag on cin (so that future I/O operations will work correctly) in case of incorrect
                // input.
                std::cin.clear();
                // Ignore anything else on the same line as the non-number so that it does not cause another parse
                // failure.
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
            return;
        }
#endif
#ifdef TOGGLE_CONTROLLER
        case static_cast<char>(EndpointControllerMenuChoice::TOGGLE_CONTROLLER_OPTION): {
            char optionSelected;
            m_interactionManager->toggleController();
            std::cin >> optionSelected;
            if (!std::cin.fail()) {
                if (optionSelected == TOGGLE_CONTROLLER_ON) {
                    m_interactionManager->setToggleState(true);
                } else if (optionSelected == TOGGLE_CONTROLLER_OFF) {
                    m_interactionManager->setToggleState(false);
                } else if (QUIT == optionSelected) {
                    return;
                } else {
                    m_interactionManager->errorValue();
                }
            } else {
                m_interactionManager->errorValue();
                // Clear error flag on cin (so that future I/O operations will work correctly) in case of incorrect
                // input.
                std::cin.clear();
                // Ignore anything else on the same line as the non-number so that it does not cause another parse
                // failure.
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
            return;
        }
#endif
#ifdef MODE_CONTROLLER
        case static_cast<char>(EndpointControllerMenuChoice::MODE_CONTROLLER_OPTION): {
            char optionSelected;
            m_interactionManager->modeController();
            std::cin >> optionSelected;
            if (!std::cin.fail()) {
                if (optionSelected == MODE_RED) {
                    m_interactionManager->setMode(PeripheralEndpointModeControllerHandler::MODE_CONTROLLER_MODE_RED);
                } else if (optionSelected == MODE_GREEN) {
                    m_interactionManager->setMode(PeripheralEndpointModeControllerHandler::MODE_CONTROLLER_MODE_GREEN);
                } else if (optionSelected == MODE_BLUE) {
                    m_interactionManager->setMode(PeripheralEndpointModeControllerHandler::MODE_CONTROLLER_MODE_BLUE);
                } else if (QUIT == optionSelected) {
                    return;
                } else {
                    m_interactionManager->errorValue();
                }
            } else {
                m_interactionManager->errorValue();
                // Clear error flag on cin (so that future I/O operations will work correctly) in case of incorrect
                // input.
                std::cin.clear();
                // Ignore anything else on the same line as the non-number so that it does not cause another parse
                // failure.
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
            return;
        }
#endif
#ifdef RANGE_CONTROLLER
        case static_cast<char>(EndpointControllerMenuChoice::RANGE_CONTROLLER_OPTION): {
            double value;
            m_interactionManager->rangeController();
            std::cin >> value;
            m_interactionManager->setRangeValue(value);
            return;
        }
#endif
        case static_cast<char>(EndpointControllerMenuChoice::QUIT): {
            return;
        }
        default: {
            m_interactionManager->errorValue();
            return;
        }
    }
}
#endif

void UserInputManager::diagnosticsMenu() {
    m_interactionManager->diagnosticsControl();
    char input;
    while (std::cin >> input && input != 'q') {
        switch (input) {
#ifdef DEVICE_PROPERTIES
            case 'p':
                devicePropertiesMenu();
                break;
#endif

#ifdef AUDIO_INJECTION
            case 'a':
                audioInjectionMenu();
                break;
#endif

#ifdef PROTOCOL_TRACE
            case 't':
                deviceProtocolTracerMenu();
                break;
#endif

            case 'i':
                m_interactionManager->diagnosticsControl();
                break;

            default:
                m_interactionManager->errorValue();
                break;
        }
    }
    m_interactionManager->help();
}

void UserInputManager::devicePropertiesMenu() {
    m_interactionManager->devicePropertiesControl();
    char input;
    while (std::cin >> input && input != 'q') {
        switch (input) {
            case 'p':
                m_interactionManager->showDeviceProperties();
                break;

            case 'i':
                m_interactionManager->devicePropertiesControl();
                break;

            default:
                m_interactionManager->errorValue();
                break;
        }
    }
    m_interactionManager->diagnosticsControl();
}

void UserInputManager::deviceProtocolTracerMenu() {
    m_interactionManager->deviceProtocolTraceControl();
    char input;
    while (std::cin >> input && input != 'q') {
        switch (input) {
            case 'e':
                m_interactionManager->setProtocolTraceFlag(true);
                break;

            case 'd':
                m_interactionManager->setProtocolTraceFlag(false);
                break;

            case 'c':
                m_interactionManager->clearProtocolTrace();
                break;

            case 't':
                m_interactionManager->printProtocolTrace();
                break;

            case 'i':
                m_interactionManager->deviceProtocolTraceControl();
                break;

            default:
                m_interactionManager->errorValue();
                break;
        }
    }
    m_interactionManager->diagnosticsControl();
}

void UserInputManager::audioInjectionMenu() {
    m_interactionManager->audioInjectionControl();
    char input;
    std::string absoluteFilePath;
    while (std::cin >> input && input != 'q') {
        switch (input) {
            case '1':
                std::cin >> absoluteFilePath;
                m_interactionManager->injectWavFile(absoluteFilePath);
                break;

            case 'i':
                m_interactionManager->audioInjectionControl();
                break;

            default:
                m_interactionManager->errorValue();
                break;
        }
    }
    m_interactionManager->diagnosticsControl();
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
