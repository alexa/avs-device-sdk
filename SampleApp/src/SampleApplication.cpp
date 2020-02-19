/*
 * Copyright 2017-2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <ContextManager/ContextManager.h>
#include <ACL/Transport/HTTP2TransportFactory.h>
#include <ACL/Transport/PostConnectSequencerFactory.h>
#include <AVSCommon/Utils/LibcurlUtils/LibcurlHTTP2ConnectionFactory.h>
#include <AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h>
#include <AVSGatewayManager/AVSGatewayManager.h>
#include <AVSGatewayManager/Storage/AVSGatewayManagerStorage.h>
#include <SynchronizeStateSender/SynchronizeStateSenderFactory.h>

#include "SampleApp/ConnectionObserver.h"
#include "SampleApp/KeywordObserver.h"
#include "SampleApp/LocaleAssetsManager.h"
#include "SampleApp/SampleApplication.h"

#ifdef ENABLE_REVOKE_AUTH
#include "SampleApp/RevokeAuthorizationObserver.h"
#endif

#ifdef ENABLE_PCC
#include "SampleApp/PhoneCaller.h"
#endif

#ifdef ENABLE_MCC
#include "SampleApp/CalendarClient.h"
#include "SampleApp/MeetingClient.h"
#endif

#ifdef KWD
#include <KWDProvider/KeywordDetectorProvider.h>
#endif

#ifdef PORTAUDIO
#include <SampleApp/PortAudioMicrophoneWrapper.h>
#endif

#ifdef GSTREAMER_MEDIA_PLAYER
#include <MediaPlayer/MediaPlayer.h>
#endif

#ifdef ANDROID
#if defined(ANDROID_MEDIA_PLAYER) || defined(ANDROID_MICROPHONE)
#include <AndroidUtilities/AndroidSLESEngine.h>
#endif

#ifdef ANDROID_MEDIA_PLAYER
#include <AndroidSLESMediaPlayer/AndroidSLESMediaPlayer.h>
#include <AndroidSLESMediaPlayer/AndroidSLESSpeaker.h>
#endif

#ifdef ANDROID_MICROPHONE
#include <AndroidUtilities/AndroidSLESMicrophone.h>
#endif

#ifdef ANDROID_LOGGER
#include <AndroidUtilities/AndroidLogger.h>
#endif

#endif

#ifdef BLUETOOTH_BLUEZ
#include <BlueZ/BlueZBluetoothDeviceManager.h>
#endif

#ifdef TOGGLE_CONTROLLER
#include <ToggleController/ToggleControllerAttributeBuilder.h>
#endif

#ifdef RANGE_CONTROLLER
#include <RangeController/RangeControllerAttributeBuilder.h>
#endif

#ifdef MODE_CONTROLLER
#include <ModeController/ModeControllerAttributeBuilder.h>
#include "SampleApp/ModeControllerHandler.h"
#endif

#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceConnectionRuleInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceManagerInterface.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Logger/LoggerSinkManager.h>
#include <AVSCommon/Utils/Network/InternetConnectionMonitor.h>
#include <Alerts/Storage/SQLiteAlertStorage.h>
#include <Audio/AudioFactory.h>
#include <Bluetooth/BasicDeviceConnectionRule.h>
#include <Bluetooth/SQLiteBluetoothStorage.h>
#include <CBLAuthDelegate/CBLAuthDelegate.h>
#include <CBLAuthDelegate/SQLiteCBLAuthDelegateStorage.h>
#include <CapabilitiesDelegate/CapabilitiesDelegate.h>
#include <CapabilitiesDelegate/Storage/SQLiteCapabilitiesDelegateStorage.h>
#include <Notifications/SQLiteNotificationsStorage.h>
#include <SampleApp/CaptionPresenter.h>
#include <SampleApp/SampleEqualizerModeController.h>
#include <SQLiteStorage/SQLiteMiscStorage.h>
#include <Settings/Storage/SQLiteDeviceSettingStorage.h>

#include <EqualizerImplementations/EqualizerController.h>
#include <EqualizerImplementations/InMemoryEqualizerConfiguration.h>
#include <EqualizerImplementations/MiscDBEqualizerStorage.h>
#include <EqualizerImplementations/SDKConfigEqualizerConfiguration.h>

#include <InterruptModelConfiguration.h>

#include <algorithm>
#include <cctype>
#include <csignal>
#include <fstream>

namespace alexaClientSDK {
namespace sampleApp {

/// The sample rate of microphone audio data.
static const unsigned int SAMPLE_RATE_HZ = 16000;

/// The number of audio channels.
static const unsigned int NUM_CHANNELS = 1;

/// The size of each word within the stream.
static const size_t WORD_SIZE = 2;

/// The maximum number of readers of the stream.
static const size_t MAX_READERS = 10;

/// The default number of MediaPlayers used by AudioPlayer CA/
/// Can be overridden in the Configuration using @c AUDIO_MEDIAPLAYER_POOL_SIZE_KEY
static const unsigned int AUDIO_MEDIAPLAYER_POOL_SIZE_DEFAULT = 2;

/// The amount of audio data to keep in the ring buffer.
static const std::chrono::seconds AMOUNT_OF_AUDIO_DATA_IN_BUFFER = std::chrono::seconds(15);

/// The size of the ring buffer.
static const size_t BUFFER_SIZE_IN_SAMPLES = (SAMPLE_RATE_HZ)*AMOUNT_OF_AUDIO_DATA_IN_BUFFER.count();

/// Key for the root node value containing configuration values for SampleApp.
static const std::string SAMPLE_APP_CONFIG_KEY("sampleApp");

/// Key for the root node value containing configuration values for Equalizer.
static const std::string EQUALIZER_CONFIG_KEY("equalizer");

/// Key for the @c firmwareVersion value under the @c SAMPLE_APP_CONFIG_KEY configuration node.
static const std::string FIRMWARE_VERSION_KEY("firmwareVersion");

/// Key for the @c endpoint value under the @c SAMPLE_APP_CONFIG_KEY configuration node.
static const std::string ENDPOINT_KEY("endpoint");

/// Key for setting if display cards are supported or not under the @c SAMPLE_APP_CONFIG_KEY configuration node.
static const std::string DISPLAY_CARD_KEY("displayCardsSupported");

/// Key for the Audio MediaPlayer pool size.
static const std::string AUDIO_MEDIAPLAYER_POOL_SIZE_KEY("audioMediaPlayerPoolSize");

using namespace capabilityAgents::externalMediaPlayer;

/// The @c m_playerToMediaPlayerMap Map of the adapter to their speaker-type and MediaPlayer creation methods.
std::unordered_map<std::string, avsCommon::sdkInterfaces::SpeakerInterface::Type>
    SampleApplication::m_playerToSpeakerTypeMap;

/// The singleton map from @c playerId to @c ExternalMediaAdapter creation functions.
std::unordered_map<std::string, ExternalMediaPlayer::AdapterCreateFunction> SampleApplication::m_adapterToCreateFuncMap;

/// String to identify log entries originating from this file.
static const std::string TAG("SampleApplication");

#ifdef ENABLE_ENDPOINT_CONTROLLERS_MENU
// Note: Discoball is an imaginary endpoint where you can control its power, its light (toggle)
// its rotation speed (range) and the color of its light (mode).

/// The derived endpoint Id used in endpoint creation.
static const std::string SAMPLE_ENDPOINT_DERIVED_ENDPOINT_ID("Discoball");

/// The description of the endpoint.
static const std::string SAMPLE_ENDPOINT_DESCRIPTION("Sample Discoball Description");

/// The friendly name of the Endpoint. This is used in utterance.
static const std::string SAMPLE_ENDPOINT_FRIENDLYNAME("Discoball");

/// The manufacturer of endpoint.
static const std::string SAMPLE_ENDPOINT_MANUFACTURER_NAME("Sample Manufacturer");

/// The display category of the endpoint.
static const std::vector<std::string> SAMPLE_ENDPOINT_DISPLAYCATEGORY({"OTHER"});

/// The instance name for the toggle controller
static const std::string SAMPLE_ENDPOINT_TOGGLE_CONTROLLER_INSTANCE_NAME("Discoball.Light");

/// The instance name for the range controller.
static const std::string SAMPLE_ENDPOINT_RANGE_CONTROLLER_INSTANCE_NAME("Discoball.Speed");

/// The instance name for the mode controller.
static const std::string SAMPLE_ENDPOINT_MODE_CONTROLLER_INSTANCE_NAME("Discoball.Mode");

/// The model of the endpoint.
static const std::string SAMPLE_ENDPOINT_ADDITIONAL_ATTRIBUTE_MODEL("Model1");

/// Serial number of the endpoint.
static const std::string SAMPLE_ENDPOINT_ADDITIONAL_ATTRIBUTE_SERIAL_NUMBER("123456789");

/// Firmware version number
static const std::string SAMPLE_ENDPOINT_ADDITIONAL_ATTRIBUTE_FIRMWARE_VERSION("1.0");

/// Software Version number.
static const std::string SAMPLE_ENDPOINT_ADDITIONAL_ATTRIBUTE_SOFTWARE_VERSION("1.0");

/// The custom identifier.
static const std::string SAMPLE_ENDPOINT_ADDITIONAL_ATTRIBUTE_CUSTOM_IDENTIFIER("SampleApp");

/// The range controller preset 'high'.
static const double SAMPLE_ENDPOINT_RANGE_CONTROLLER_PRESET_HIGH = 10;

/// The range controller preset 'medium'.
static const double SAMPLE_ENDPOINT_RANGE_CONTROLLER_PRESET_MEDIUM = 5;

/// The range controller preset 'low'.
static const double SAMPLE_ENDPOINT_RANGE_CONTROLLER_PRESET_LOW = 1;

/// US English locale string.
static const std::string EN_US("en-US");
#endif

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// A set of all log levels.
static const std::set<alexaClientSDK::avsCommon::utils::logger::Level> allLevels = {
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG9,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG8,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG7,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG6,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG5,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG4,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG3,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG2,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG1,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG0,
    alexaClientSDK::avsCommon::utils::logger::Level::INFO,
    alexaClientSDK::avsCommon::utils::logger::Level::WARN,
    alexaClientSDK::avsCommon::utils::logger::Level::ERROR,
    alexaClientSDK::avsCommon::utils::logger::Level::CRITICAL,
    alexaClientSDK::avsCommon::utils::logger::Level::NONE};

/**
 * Gets a log level consumable by the SDK based on the user input string for log level.
 *
 * @param userInputLogLevel The string to be parsed into a log level.
 * @return The log level. This will default to NONE if the input string is not properly parsable.
 */
static alexaClientSDK::avsCommon::utils::logger::Level getLogLevelFromUserInput(std::string userInputLogLevel) {
    std::transform(userInputLogLevel.begin(), userInputLogLevel.end(), userInputLogLevel.begin(), ::toupper);
    return alexaClientSDK::avsCommon::utils::logger::convertNameToLevel(userInputLogLevel);
}

/**
 * Allows the process to ignore the SIGPIPE signal.
 * The SIGPIPE signal may be received when the application performs a write to a closed socket.
 * This is a case that arises in the use of certain networking libraries.
 *
 * @return true if the action for handling SIGPIPEs was correctly set to ignore, else false.
 */
static bool ignoreSigpipeSignals() {
#ifndef NO_SIGPIPE
    if (std::signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        return false;
    }
#endif
    return true;
}

std::unique_ptr<SampleApplication> SampleApplication::create(
    std::shared_ptr<alexaClientSDK::sampleApp::ConsoleReader> consoleReader,
    const std::vector<std::string>& configFiles,
    const std::string& pathToInputFolder,
    const std::string& logLevel) {
    auto clientApplication = std::unique_ptr<SampleApplication>(new SampleApplication);
    if (!clientApplication->initialize(consoleReader, configFiles, pathToInputFolder, logLevel)) {
        ACSDK_CRITICAL(LX("Failed to initialize SampleApplication"));
        return nullptr;
    }
    if (!ignoreSigpipeSignals()) {
        ACSDK_CRITICAL(LX("Failed to set a signal handler for SIGPIPE"));
        return nullptr;
    }

    return clientApplication;
}

SampleApplication::AdapterRegistration::AdapterRegistration(
    const std::string& playerId,
    ExternalMediaPlayer::AdapterCreateFunction createFunction) {
    if (m_adapterToCreateFuncMap.find(playerId) != m_adapterToCreateFuncMap.end()) {
        ACSDK_WARN(LX("Adapter already exists").d("playerID", playerId));
    }

    m_adapterToCreateFuncMap[playerId] = createFunction;
}

SampleApplication::MediaPlayerRegistration::MediaPlayerRegistration(
    const std::string& playerId,
    avsCommon::sdkInterfaces::SpeakerInterface::Type speakerType) {
    if (m_playerToSpeakerTypeMap.find(playerId) != m_playerToSpeakerTypeMap.end()) {
        ACSDK_WARN(LX("MediaPlayer already exists").d("playerId", playerId));
    }

    m_playerToSpeakerTypeMap[playerId] = speakerType;
}

SampleAppReturnCode SampleApplication::run() {
    return m_userInputManager->run();
}

SampleApplication::~SampleApplication() {
    if (m_capabilitiesDelegate) {
        m_capabilitiesDelegate->shutdown();
    }

    // First clean up anything that depends on the the MediaPlayers.
    m_userInputManager.reset();
    m_externalMusicProviderMediaPlayersMap.clear();

    if (m_interactionManager) {
        m_interactionManager->shutdown();
    }

    // Now it's safe to shut down the MediaPlayers.
    for (auto& mediaPlayer : m_audioMediaPlayerPool) {
        mediaPlayer->shutdown();
    }
    for (auto& mediaPlayer : m_adapterMediaPlayers) {
        mediaPlayer->shutdown();
    }
    if (m_speakMediaPlayer) {
        m_speakMediaPlayer->shutdown();
    }
    if (m_alertsMediaPlayer) {
        m_alertsMediaPlayer->shutdown();
    }
    if (m_notificationsMediaPlayer) {
        m_notificationsMediaPlayer->shutdown();
    }
    if (m_bluetoothMediaPlayer) {
        m_bluetoothMediaPlayer->shutdown();
    }
    if (m_systemSoundMediaPlayer) {
        m_systemSoundMediaPlayer->shutdown();
    }

    if (m_ringtoneMediaPlayer) {
        m_ringtoneMediaPlayer->shutdown();
    }

#ifdef ENABLE_COMMS_AUDIO_PROXY
    if (m_commsMediaPlayer) {
        m_commsMediaPlayer->shutdown();
    }
#endif

#ifdef ENABLE_PCC
    if (m_phoneMediaPlayer) {
        m_phoneMediaPlayer->shutdown();
    }
#endif

    avsCommon::avs::initialization::AlexaClientSDKInit::uninitialize();
}

bool SampleApplication::createMediaPlayersForAdapters(
    std::shared_ptr<avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory> httpContentFetcherFactory,
    std::shared_ptr<defaultClient::EqualizerRuntimeSetup> equalizerRuntimeSetup,
    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>& additionalSpeakers) {
    bool equalizerEnabled = nullptr != equalizerRuntimeSetup;
    for (auto& entry : m_playerToSpeakerTypeMap) {
        auto mediaPlayerSpeakerPair = createApplicationMediaPlayer(
            httpContentFetcherFactory, equalizerEnabled, entry.second, entry.first + "MediaPlayer", false);
        auto mediaPlayer = mediaPlayerSpeakerPair.first;
        auto speaker = mediaPlayerSpeakerPair.second;
        if (mediaPlayer) {
            m_externalMusicProviderMediaPlayersMap[entry.first] = mediaPlayer;
            m_externalMusicProviderSpeakersMap[entry.first] = speaker;
            additionalSpeakers.push_back(speaker);
            m_adapterMediaPlayers.push_back(mediaPlayer);
            if (equalizerEnabled) {
                equalizerRuntimeSetup->addEqualizer(mediaPlayer);
            }
        } else {
            ACSDK_CRITICAL(LX("Failed to create mediaPlayer").d("playerId", entry.first));
            return false;
        }
    }

    return true;
}

bool SampleApplication::initialize(
    std::shared_ptr<alexaClientSDK::sampleApp::ConsoleReader> consoleReader,
    const std::vector<std::string>& configFiles,
    const std::string& pathToInputFolder,
    const std::string& logLevel) {
    /*
     * Set up the SDK logging system to write to the SampleApp's ConsolePrinter.  Also adjust the logging level
     * if requested.
     */
    std::shared_ptr<alexaClientSDK::avsCommon::utils::logger::Logger> consolePrinter =
        std::make_shared<alexaClientSDK::sampleApp::ConsolePrinter>();

    avsCommon::utils::logger::Level logLevelValue = avsCommon::utils::logger::Level::UNKNOWN;
    if (!logLevel.empty()) {
        logLevelValue = getLogLevelFromUserInput(logLevel);
        if (alexaClientSDK::avsCommon::utils::logger::Level::UNKNOWN == logLevelValue) {
            alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Unknown log level input!");
            alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Possible log level options are: ");
            for (auto it = allLevels.begin(); it != allLevels.end(); ++it) {
                alexaClientSDK::sampleApp::ConsolePrinter::simplePrint(
                    alexaClientSDK::avsCommon::utils::logger::convertLevelToName(*it));
            }
            return false;
        }

        alexaClientSDK::sampleApp::ConsolePrinter::simplePrint(
            "Running app with log level: " +
            alexaClientSDK::avsCommon::utils::logger::convertLevelToName(logLevelValue));
        consolePrinter->setLevel(logLevelValue);
    }

#ifdef ANDROID_LOGGER
    alexaClientSDK::avsCommon::utils::logger::LoggerSinkManager::instance().initialize(
        std::make_shared<applicationUtilities::androidUtilities::AndroidLogger>(logLevelValue));
#else
    alexaClientSDK::avsCommon::utils::logger::LoggerSinkManager::instance().initialize(consolePrinter);
#endif

    std::vector<std::shared_ptr<std::istream>> configJsonStreams;

    for (auto configFile : configFiles) {
        if (configFile.empty()) {
            alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Config filename is empty!");
            return false;
        }

        auto configInFile = std::shared_ptr<std::ifstream>(new std::ifstream(configFile));
        if (!configInFile->good()) {
            ACSDK_CRITICAL(LX("Failed to read config file").d("filename", configFile));
            alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Failed to read config file " + configFile);
            return false;
        }

        configJsonStreams.push_back(configInFile);
    }

    // add the InterruptModel Configuration
    configJsonStreams.push_back(alexaClientSDK::afml::interruptModel::InterruptModelConfiguration::getConfig(false));

    if (!avsCommon::avs::initialization::AlexaClientSDKInit::initialize(configJsonStreams)) {
        ACSDK_CRITICAL(LX("Failed to initialize SDK!"));
        return false;
    }

    auto config = alexaClientSDK::avsCommon::utils::configuration::ConfigurationNode::getRoot();
    auto sampleAppConfig = config[SAMPLE_APP_CONFIG_KEY];

    auto httpContentFetcherFactory = std::make_shared<avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory>();

    // Creating the misc DB object to be used by various components.
    std::shared_ptr<alexaClientSDK::storage::sqliteStorage::SQLiteMiscStorage> miscStorage =
        alexaClientSDK::storage::sqliteStorage::SQLiteMiscStorage::create(config);

    /*
     * Creating Equalizer specific implementations
     */
    auto equalizerConfigBranch = config[EQUALIZER_CONFIG_KEY];
    auto equalizerConfiguration = equalizer::SDKConfigEqualizerConfiguration::create(equalizerConfigBranch);
    std::shared_ptr<defaultClient::EqualizerRuntimeSetup> equalizerRuntimeSetup = nullptr;

    bool equalizerEnabled = false;
    if (equalizerConfiguration && equalizerConfiguration->isEnabled()) {
        equalizerEnabled = true;
        equalizerRuntimeSetup = std::make_shared<defaultClient::EqualizerRuntimeSetup>();
        auto equalizerStorage = equalizer::MiscDBEqualizerStorage::create(miscStorage);
        auto equalizerModeController = sampleApp::SampleEqualizerModeController::create();

        equalizerRuntimeSetup->setStorage(equalizerStorage);
        equalizerRuntimeSetup->setConfiguration(equalizerConfiguration);
        equalizerRuntimeSetup->setModeController(equalizerModeController);
    }

#if defined(ANDROID_MEDIA_PLAYER) || defined(ANDROID_MICROPHONE)
    m_openSlEngine = applicationUtilities::androidUtilities::AndroidSLESEngine::create();
    if (!m_openSlEngine) {
        ACSDK_ERROR(LX("createAndroidMicFailed").d("reason", "failed to create engine"));
        return false;
    }
#endif

    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> speakSpeaker;
    std::tie(m_speakMediaPlayer, speakSpeaker) = createApplicationMediaPlayer(
        httpContentFetcherFactory,
        false,
        avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
        "SpeakMediaPlayer");
    if (!m_speakMediaPlayer || !speakSpeaker) {
        ACSDK_CRITICAL(LX("Failed to create media player for speech!"));
        return false;
    }

    int poolSize;
    sampleAppConfig.getInt(AUDIO_MEDIAPLAYER_POOL_SIZE_KEY, &poolSize, AUDIO_MEDIAPLAYER_POOL_SIZE_DEFAULT);

    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>> additionalSpeakers;
    for (int index = 0; index < poolSize; index++) {
        std::shared_ptr<ApplicationMediaPlayer> mediaPlayer;
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> speaker;

        std::tie(mediaPlayer, speaker) = createApplicationMediaPlayer(
            httpContentFetcherFactory,
            equalizerEnabled,
            avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
            "AudioMediaPlayer");
        if (!mediaPlayer || !speaker) {
            ACSDK_CRITICAL(LX("Failed to create media player for audio!"));
            return false;
        }
        m_audioMediaPlayerPool.push_back(mediaPlayer);
        additionalSpeakers.push_back(speaker);
        // Creating equalizers
        if (nullptr != equalizerRuntimeSetup) {
            equalizerRuntimeSetup->addEqualizer(mediaPlayer);
        }
    }

    std::vector<std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>> pool(
        m_audioMediaPlayerPool.begin(), m_audioMediaPlayerPool.end());
    auto audioMediaPlayerFactory = mediaPlayer::PooledMediaPlayerFactory::create(pool);
    if (!audioMediaPlayerFactory) {
        ACSDK_CRITICAL(LX("Failed to create media player factory for content!"));
        return false;
    }

    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> notificationsSpeaker;
    std::tie(m_notificationsMediaPlayer, notificationsSpeaker) = createApplicationMediaPlayer(
        httpContentFetcherFactory,
        false,
        avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_ALERTS_VOLUME,
        "NotificationsMediaPlayer");
    if (!m_notificationsMediaPlayer || !notificationsSpeaker) {
        ACSDK_CRITICAL(LX("Failed to create media player for notifications!"));
        return false;
    }

    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> bluetoothSpeaker;
    std::tie(m_bluetoothMediaPlayer, bluetoothSpeaker) = createApplicationMediaPlayer(
        httpContentFetcherFactory,
        false,
        avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
        "BluetoothMediaPlayer");

    if (!m_bluetoothMediaPlayer || !bluetoothSpeaker) {
        ACSDK_CRITICAL(LX("Failed to create media player for bluetooth!"));
        return false;
    }

    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> ringtoneSpeaker;
    std::tie(m_ringtoneMediaPlayer, ringtoneSpeaker) = createApplicationMediaPlayer(
        httpContentFetcherFactory,
        false,
        avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
        "RingtoneMediaPlayer");
    if (!m_ringtoneMediaPlayer || !ringtoneSpeaker) {
        alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Failed to create media player for ringtones!");
        return false;
    }

#ifdef ENABLE_COMMS_AUDIO_PROXY
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> commsSpeaker;
    std::tie(m_commsMediaPlayer, commsSpeaker) = createApplicationMediaPlayer(
        httpContentFetcherFactory,
        false,
        avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
        "CommsMediaPlayer",
        true);
    if (!m_commsMediaPlayer || !commsSpeaker) {
        ACSDK_CRITICAL(LX("Failed to create media player for comms!"));
        return false;
    }
#endif

    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> alertsSpeaker;
    std::tie(m_alertsMediaPlayer, alertsSpeaker) = createApplicationMediaPlayer(
        httpContentFetcherFactory,
        false,
        avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_ALERTS_VOLUME,
        "AlertsMediaPlayer");
    if (!m_alertsMediaPlayer || !alertsSpeaker) {
        ACSDK_CRITICAL(LX("Failed to create media player for alerts!"));
        return false;
    }

    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> systemSoundSpeaker;
    std::tie(m_systemSoundMediaPlayer, systemSoundSpeaker) = createApplicationMediaPlayer(
        httpContentFetcherFactory,
        false,
        avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
        "SystemSoundMediaPlayer");
    if (!m_systemSoundMediaPlayer || !systemSoundSpeaker) {
        ACSDK_CRITICAL(LX("Failed to create media player for system sound player!"));
        return false;
    }

#ifdef ENABLE_PCC
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> phoneSpeaker;
    std::tie(m_phoneMediaPlayer, phoneSpeaker) = createApplicationMediaPlayer(
        httpContentFetcherFactory,
        false,
        avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
        "PhoneMediaPlayer");

    if (!m_phoneMediaPlayer || !phoneSpeaker) {
        ACSDK_CRITICAL(LX("Failed to create media player for phone!"));
        return false;
    }
#endif

#ifdef ENABLE_MCC
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> meetingSpeaker;
    std::shared_ptr<ApplicationMediaPlayer> meetingMediaPlayer;
    std::tie(meetingMediaPlayer, meetingSpeaker) = createApplicationMediaPlayer(
        httpContentFetcherFactory,
        false,
        avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
        "MeetingMediaPlayer");

    if (!meetingMediaPlayer || !meetingSpeaker) {
        ACSDK_CRITICAL(LX("Failed to create media player for meeting client!"));
        return false;
    }
#endif

    if (!createMediaPlayersForAdapters(httpContentFetcherFactory, equalizerRuntimeSetup, additionalSpeakers)) {
        ACSDK_CRITICAL(LX("Could not create mediaPlayers for adapters"));
        return false;
    }

    auto audioFactory = std::make_shared<alexaClientSDK::applicationUtilities::resources::audio::AudioFactory>();

    // Creating the alert storage object to be used for rendering and storing alerts.
    auto alertStorage =
        alexaClientSDK::capabilityAgents::alerts::storage::SQLiteAlertStorage::create(config, audioFactory->alerts());

    // Creating the message storage object to be used for storing message to be sent later.
    auto messageStorage = alexaClientSDK::certifiedSender::SQLiteMessageStorage::create(config);

    /*
     * Creating notifications storage object to be used for storing notification indicators.
     */
    auto notificationsStorage =
        alexaClientSDK::capabilityAgents::notifications::SQLiteNotificationsStorage::create(config);

    /*
     * Creating new device settings storage object to be used for storing AVS Settings.
     */
    auto deviceSettingsStorage = alexaClientSDK::settings::storage::SQLiteDeviceSettingStorage::create(config);

    /*
     * Creating bluetooth storage object to be used for storing uuid to mac mappings for devices.
     */
    auto bluetoothStorage = alexaClientSDK::capabilityAgents::bluetooth::SQLiteBluetoothStorage::create(config);

#ifdef KWD
    bool wakeWordEnabled = true;
#else
    bool wakeWordEnabled = false;
#endif

    /*
     * Create sample locale asset manager.
     */
    auto localeAssetsManager = LocaleAssetsManager::create(wakeWordEnabled);
    if (!localeAssetsManager) {
        ACSDK_CRITICAL(LX("Failed to create Locale Assets Manager!"));
        return false;
    }

    /*
     * Creating the UI component that observes various components and prints to the console accordingly.
     */
    auto userInterfaceManager = std::make_shared<alexaClientSDK::sampleApp::UIManager>(localeAssetsManager);

    /*
     * Create the presentation layer for the captions.
     */
    auto captionPresenter = std::make_shared<alexaClientSDK::sampleApp::CaptionPresenter>();

    /*
     * Creating customerDataManager which will be used by the registrationManager and all classes that extend
     * CustomerDataHandler
     */
    auto customerDataManager = std::make_shared<registrationManager::CustomerDataManager>();

#ifdef ENABLE_PCC
    auto phoneCaller = std::make_shared<alexaClientSDK::sampleApp::PhoneCaller>();
#endif

#ifdef ENABLE_MCC
    auto meetingClient = std::make_shared<alexaClientSDK::sampleApp::MeetingClient>();
    auto calendarClient = std::make_shared<alexaClientSDK::sampleApp::CalendarClient>();
#endif

    /*
     * Creating the deviceInfo object
     */
    std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo = avsCommon::utils::DeviceInfo::create(config);
    if (!deviceInfo) {
        ACSDK_CRITICAL(LX("Creation of DeviceInfo failed!"));
        return false;
    }

    /*
     * Supply a SALT for UUID generation, this should be as unique to each individual device as possible
     */
    alexaClientSDK::avsCommon::utils::uuidGeneration::setSalt(
        deviceInfo->getClientId() + deviceInfo->getDeviceSerialNumber());

    /*
     * Creating the AuthDelegate - this component takes care of LWA and authorization of the client.
     */
    auto authDelegateStorage = authorization::cblAuthDelegate::SQLiteCBLAuthDelegateStorage::create(config);
    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate =
        authorization::cblAuthDelegate::CBLAuthDelegate::create(
            config, customerDataManager, std::move(authDelegateStorage), userInterfaceManager, nullptr, deviceInfo);

    if (!authDelegate) {
        ACSDK_CRITICAL(LX("Creation of AuthDelegate failed!"));
        return false;
    }

    /*
     * Creating the CapabilitiesDelegate - This component provides the client with the ability to send messages to the
     * Capabilities API.
     */
    auto capabilitiesDelegateStorage =
        alexaClientSDK::capabilitiesDelegate::storage::SQLiteCapabilitiesDelegateStorage::create(config);

    m_capabilitiesDelegate = alexaClientSDK::capabilitiesDelegate::CapabilitiesDelegate::create(
        authDelegate, std::move(capabilitiesDelegateStorage), customerDataManager);

    if (!m_capabilitiesDelegate) {
        alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Creation of CapabilitiesDelegate failed!");
        return false;
    }

    authDelegate->addAuthObserver(userInterfaceManager);
    m_capabilitiesDelegate->addCapabilitiesObserver(userInterfaceManager);

    // INVALID_FIRMWARE_VERSION is passed to @c getInt() as a default in case FIRMWARE_VERSION_KEY is not found.
    int firmwareVersion = static_cast<int>(avsCommon::sdkInterfaces::softwareInfo::INVALID_FIRMWARE_VERSION);
    sampleAppConfig.getInt(FIRMWARE_VERSION_KEY, &firmwareVersion, firmwareVersion);

    /*
     * Check to see if displayCards is supported on the device. The default is supported unless specified otherwise in
     * the configuration.
     */
    bool displayCardsSupported;
    config[SAMPLE_APP_CONFIG_KEY].getBool(DISPLAY_CARD_KEY, &displayCardsSupported, true);

    /*
     * Creating the InternetConnectionMonitor that will notify observers of internet connection status changes.
     */
    auto internetConnectionMonitor =
        avsCommon::utils::network::InternetConnectionMonitor::create(httpContentFetcherFactory);
    if (!internetConnectionMonitor) {
        ACSDK_CRITICAL(LX("Failed to create InternetConnectionMonitor"));
        return false;
    }

    /*
     * Creating the Context Manager - This component manages the context of each of the components to update to AVS.
     * It is required for each of the capability agents so that they may provide their state just before any event is
     * fired off.
     */
    auto contextManager = contextManager::ContextManager::create(*deviceInfo);
    if (!contextManager) {
        ACSDK_CRITICAL(LX("Creation of ContextManager failed."));
        return false;
    }

    auto avsGatewayManagerStorage = avsGatewayManager::storage::AVSGatewayManagerStorage::create(miscStorage);
    if (!avsGatewayManagerStorage) {
        ACSDK_CRITICAL(LX("Creation of AVSGatewayManagerStorage failed"));
        return false;
    }
    auto avsGatewayManager =
        avsGatewayManager::AVSGatewayManager::create(std::move(avsGatewayManagerStorage), customerDataManager, config);
    if (!avsGatewayManager) {
        ACSDK_CRITICAL(LX("Creation of AVSGatewayManager failed"));
        return false;
    }

    auto synchronizeStateSenderFactory = synchronizeStateSender::SynchronizeStateSenderFactory::create(contextManager);
    if (!synchronizeStateSenderFactory) {
        ACSDK_CRITICAL(LX("Creation of SynchronizeStateSenderFactory failed"));
        return false;
    }

    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationProviderInterface>> providers;
    providers.push_back(synchronizeStateSenderFactory);
    providers.push_back(avsGatewayManager);
    providers.push_back(m_capabilitiesDelegate);

    /*
     * Create a factory for creating objects that handle tasks that need to be performed right after establishing
     * a connection to AVS.
     */
    auto postConnectSequencerFactory = acl::PostConnectSequencerFactory::create(providers);

    /*
     * Create a factory to create objects that establish a connection with AVS.
     */
    auto transportFactory = std::make_shared<acl::HTTP2TransportFactory>(
        std::make_shared<avsCommon::utils::libcurlUtils::LibcurlHTTP2ConnectionFactory>(), postConnectSequencerFactory);

    /*
     * Creating the buffer (Shared Data Stream) that will hold user audio data. This is the main input into the SDK.
     */
    size_t bufferSize = alexaClientSDK::avsCommon::avs::AudioInputStream::calculateBufferSize(
        BUFFER_SIZE_IN_SAMPLES, WORD_SIZE, MAX_READERS);
    auto buffer = std::make_shared<alexaClientSDK::avsCommon::avs::AudioInputStream::Buffer>(bufferSize);
    std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> sharedDataStream =
        alexaClientSDK::avsCommon::avs::AudioInputStream::create(buffer, WORD_SIZE, MAX_READERS);

    if (!sharedDataStream) {
        ACSDK_CRITICAL(LX("Failed to create shared data stream!"));
        return false;
    }

    /*
     * Create the BluetoothDeviceManager to communicate with the Bluetooth stack.
     */
    std::unique_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceManagerInterface> bluetoothDeviceManager;

    /*
     * Create the connectionRules to communicate with the Bluetooth stack.
     */
    std::unordered_set<
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceConnectionRuleInterface>>
        enabledConnectionRules;
    enabledConnectionRules.insert(alexaClientSDK::capabilityAgents::bluetooth::BasicDeviceConnectionRule::create());

#ifdef BLUETOOTH_BLUEZ
    auto eventBus = std::make_shared<avsCommon::utils::bluetooth::BluetoothEventBus>();

#ifdef BLUETOOTH_BLUEZ_PULSEAUDIO_OVERRIDE_ENDPOINTS
    /*
     * Create PulseAudio initializer object. Subscribe to BLUETOOTH_DEVICE_MANAGER_INITIALIZED event before we create
     * the BT Device Manager, otherwise may miss it.
     */
    m_pulseAudioInitializer = bluetoothImplementations::blueZ::PulseAudioBluetoothInitializer::create(eventBus);
#endif

    bluetoothDeviceManager = bluetoothImplementations::blueZ::BlueZBluetoothDeviceManager::create(eventBus);
#endif

    /*
     * Creating the DefaultClient - this component serves as an out-of-box default object that instantiates and "glues"
     * together all the modules.
     */
    std::shared_ptr<alexaClientSDK::defaultClient::DefaultClient> client =
        alexaClientSDK::defaultClient::DefaultClient::create(
            deviceInfo,
            customerDataManager,
            m_externalMusicProviderMediaPlayersMap,
            m_externalMusicProviderSpeakersMap,
            m_adapterToCreateFuncMap,
            m_speakMediaPlayer,
            std::move(audioMediaPlayerFactory),
            m_alertsMediaPlayer,
            m_notificationsMediaPlayer,
            m_bluetoothMediaPlayer,
            m_ringtoneMediaPlayer,
            m_systemSoundMediaPlayer,
            speakSpeaker,
            nullptr,  // added into 'additionalSpeakers
            alertsSpeaker,
            notificationsSpeaker,
            bluetoothSpeaker,
            ringtoneSpeaker,
            systemSoundSpeaker,
            additionalSpeakers,
#ifdef ENABLE_PCC
            phoneSpeaker,
            phoneCaller,
#endif
#ifdef ENABLE_MCC
            meetingSpeaker,
            meetingClient,
            calendarClient,
#endif
#ifdef ENABLE_COMMS_AUDIO_PROXY
            m_commsMediaPlayer,
            commsSpeaker,
            sharedDataStream,
#endif
            equalizerRuntimeSetup,
            audioFactory,
            authDelegate,
            std::move(alertStorage),
            std::move(messageStorage),
            std::move(notificationsStorage),
            std::move(deviceSettingsStorage),
            std::move(bluetoothStorage),
            miscStorage,
            {userInterfaceManager},
            {userInterfaceManager},
            std::move(internetConnectionMonitor),
            displayCardsSupported,
            m_capabilitiesDelegate,
            contextManager,
            transportFactory,
            avsGatewayManager,
            localeAssetsManager,
            enabledConnectionRules,
            /* systemTimezone*/ nullptr,
            firmwareVersion,
            true,
            nullptr,
            std::move(bluetoothDeviceManager));

    if (!client) {
        ACSDK_CRITICAL(LX("Failed to create default SDK client!"));
        return false;
    }

    client->addSpeakerManagerObserver(userInterfaceManager);

    client->addNotificationsObserver(userInterfaceManager);

    client->addBluetoothDeviceObserver(userInterfaceManager);

#ifdef ENABLE_CAPTIONS
    std::vector<std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>> captionableMediaSources = pool;
    captionableMediaSources.emplace_back(m_speakMediaPlayer);
    client->addCaptionPresenter(captionPresenter);
    client->setCaptionMediaPlayers(captionableMediaSources);
#endif

    userInterfaceManager->configureSettingsNotifications(client->getSettingsManager());

    /*
     * Add GUI Renderer as an observer if display cards are supported.
     */
    if (displayCardsSupported) {
        m_guiRenderer = std::make_shared<GuiRenderer>();
        client->addTemplateRuntimeObserver(m_guiRenderer);
    }

    alexaClientSDK::avsCommon::utils::AudioFormat compatibleAudioFormat;
    compatibleAudioFormat.sampleRateHz = SAMPLE_RATE_HZ;
    compatibleAudioFormat.sampleSizeInBits = WORD_SIZE * CHAR_BIT;
    compatibleAudioFormat.numChannels = NUM_CHANNELS;
    compatibleAudioFormat.endianness = alexaClientSDK::avsCommon::utils::AudioFormat::Endianness::LITTLE;
    compatibleAudioFormat.encoding = alexaClientSDK::avsCommon::utils::AudioFormat::Encoding::LPCM;

    /*
     * Creating each of the audio providers. An audio provider is a simple package of data consisting of the stream
     * of audio data, as well as metadata about the stream. For each of the three audio providers created here, the same
     * stream is used since this sample application will only have one microphone.
     */

    // Creating tap to talk audio provider
    bool tapAlwaysReadable = true;
    bool tapCanOverride = true;
    bool tapCanBeOverridden = true;

    alexaClientSDK::capabilityAgents::aip::AudioProvider tapToTalkAudioProvider(
        sharedDataStream,
        compatibleAudioFormat,
        alexaClientSDK::capabilityAgents::aip::ASRProfile::NEAR_FIELD,
        tapAlwaysReadable,
        tapCanOverride,
        tapCanBeOverridden);

    // Creating hold to talk audio provider
    bool holdAlwaysReadable = false;
    bool holdCanOverride = true;
    bool holdCanBeOverridden = false;

    alexaClientSDK::capabilityAgents::aip::AudioProvider holdToTalkAudioProvider(
        sharedDataStream,
        compatibleAudioFormat,
        alexaClientSDK::capabilityAgents::aip::ASRProfile::CLOSE_TALK,
        holdAlwaysReadable,
        holdCanOverride,
        holdCanBeOverridden);

#ifdef PORTAUDIO
    std::shared_ptr<PortAudioMicrophoneWrapper> micWrapper = PortAudioMicrophoneWrapper::create(sharedDataStream);
#elif defined(ANDROID_MICROPHONE)
    std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESMicrophone> micWrapper =
        m_openSlEngine->createAndroidMicrophone(sharedDataStream);
#else
#error "No audio input provided"
#endif
    if (!micWrapper) {
        ACSDK_CRITICAL(LX("Failed to create PortAudioMicrophoneWrapper!"));
        return false;
    }

#ifdef ENABLE_ENDPOINT_CONTROLLERS_MENU
    auto discoballEndpointBuilder = client->createEndpointBuilder();
    if (!discoballEndpointBuilder) {
        ACSDK_CRITICAL(LX("Failed to create Endpoint Builder!"));
        return false;
    }

    discoballEndpointBuilder->withDerivedEndpointId(SAMPLE_ENDPOINT_DERIVED_ENDPOINT_ID)
        .withDescription(SAMPLE_ENDPOINT_DESCRIPTION)
        .withFriendlyName(SAMPLE_ENDPOINT_FRIENDLYNAME)
        .withManufacturerName(SAMPLE_ENDPOINT_MANUFACTURER_NAME)
        .withAdditionalAttributes(
            SAMPLE_ENDPOINT_MANUFACTURER_NAME,
            SAMPLE_ENDPOINT_ADDITIONAL_ATTRIBUTE_MODEL,
            SAMPLE_ENDPOINT_ADDITIONAL_ATTRIBUTE_SERIAL_NUMBER,
            SAMPLE_ENDPOINT_ADDITIONAL_ATTRIBUTE_FIRMWARE_VERSION,
            SAMPLE_ENDPOINT_ADDITIONAL_ATTRIBUTE_SOFTWARE_VERSION,
            SAMPLE_ENDPOINT_ADDITIONAL_ATTRIBUTE_CUSTOM_IDENTIFIER)
        .withDisplayCategory(SAMPLE_ENDPOINT_DISPLAYCATEGORY);

#ifdef POWER_CONTROLLER
    auto discoballPowerHandler = PowerControllerHandler::create();
    if (!discoballPowerHandler) {
        ACSDK_CRITICAL(LX("Failed to create power controller handler!"));
        return false;
    }
    discoballEndpointBuilder->withPowerController(discoballPowerHandler, true, true);
#endif

#ifdef TOGGLE_CONTROLLER
    auto discoballToggleHandler = ToggleControllerHandler::create();
    if (!discoballToggleHandler) {
        ACSDK_CRITICAL(LX("Failed to create toggle controller handler!"));
        return false;
    }

    auto toggleControllerAttributeBuilder =
        capabilityAgents::toggleController::ToggleControllerAttributeBuilder::create();
    if (!toggleControllerAttributeBuilder) {
        ACSDK_CRITICAL(LX("Failed to create toggle controller attribute builder!"));
        return false;
    }

    auto toggleCapabilityResources = avsCommon::avs::CapabilityResources();
    if (!toggleCapabilityResources.addFriendlyNameWithText("Light", EN_US)) {
        ACSDK_CRITICAL(LX("Failed to create Toggle Controller capability resources!"));
        return false;
    }

    toggleControllerAttributeBuilder->withCapabilityResources(toggleCapabilityResources);
    auto toggleControllerAttributes = toggleControllerAttributeBuilder->build();
    if (!toggleControllerAttributes.hasValue()) {
        ACSDK_CRITICAL(LX("Failed to create Toggle Controller attributes!"));
        return false;
    }

    discoballEndpointBuilder->withToggleController(
        discoballToggleHandler,
        SAMPLE_ENDPOINT_TOGGLE_CONTROLLER_INSTANCE_NAME,
        toggleControllerAttributes.value(),
        true,
        true,
        false);
#endif

#ifdef RANGE_CONTROLLER
    auto discoballRangeHandler = RangeControllerHandler::create();
    if (!discoballRangeHandler) {
        ACSDK_CRITICAL(LX("Failed to create range controller handler!"));
        return false;
    }

    auto rangeControllerAttributeBuilder = capabilityAgents::rangeController::RangeControllerAttributeBuilder::create();
    if (!rangeControllerAttributeBuilder) {
        ACSDK_CRITICAL(LX("Failed to create range controller attribute builder!"));
        return false;
    }

    auto rangeCapabilityResources = avsCommon::avs::CapabilityResources();
    if (!rangeCapabilityResources.addFriendlyNameWithText("Speed", EN_US)) {
        ACSDK_CRITICAL(LX("Failed to create Range Controller capability resources!"));
        return false;
    }

    auto highPresetResources = avsCommon::avs::CapabilityResources();
    if (!highPresetResources.addFriendlyNameWithAssetId(avsCommon::avs::resources::ASSET_ALEXA_VALUE_MAXIMUM) ||
        !highPresetResources.addFriendlyNameWithAssetId(avsCommon::avs::resources::ASSET_ALEXA_VALUE_HIGH)) {
        ACSDK_CRITICAL(LX("Failed to create Range Controller HIGH preset resources!"));
        return false;
    }

    auto mediumPresetResources = avsCommon::avs::CapabilityResources();
    if (!mediumPresetResources.addFriendlyNameWithText("mid", EN_US) ||
        !mediumPresetResources.addFriendlyNameWithAssetId(avsCommon::avs::resources::ASSET_ALEXA_VALUE_MEDIUM)) {
        ACSDK_CRITICAL(LX("Failed to create Range Controller MEDIUM preset resources!"));
        return false;
    }

    auto lowPresetResources = avsCommon::avs::CapabilityResources();
    if (!lowPresetResources.addFriendlyNameWithAssetId(avsCommon::avs::resources::ASSET_ALEXA_VALUE_MINIMUM) ||
        !lowPresetResources.addFriendlyNameWithAssetId(avsCommon::avs::resources::ASSET_ALEXA_VALUE_LOW)) {
        ACSDK_CRITICAL(LX("Failed to create Range Controller LOW preset resources!"));
        return false;
    }

    rangeControllerAttributeBuilder->withCapabilityResources(rangeCapabilityResources)
        .addPreset(std::make_pair(SAMPLE_ENDPOINT_RANGE_CONTROLLER_PRESET_HIGH, highPresetResources))
        .addPreset(std::make_pair(SAMPLE_ENDPOINT_RANGE_CONTROLLER_PRESET_MEDIUM, mediumPresetResources))
        .addPreset(std::make_pair(SAMPLE_ENDPOINT_RANGE_CONTROLLER_PRESET_LOW, lowPresetResources));
    auto rangeControllerAttributes = rangeControllerAttributeBuilder->build();
    if (!rangeControllerAttributes.hasValue()) {
        ACSDK_CRITICAL(LX("Failed to create Range Controller attributes!"));
        return false;
    }

    discoballEndpointBuilder->withRangeController(
        discoballRangeHandler,
        SAMPLE_ENDPOINT_RANGE_CONTROLLER_INSTANCE_NAME,
        rangeControllerAttributes.value(),
        true,
        true,
        false);
#endif

#ifdef MODE_CONTROLLER
    auto discoballModeHandler = ModeControllerHandler::create();
    if (!discoballModeHandler) {
        ACSDK_CRITICAL(LX("Failed to create mode controller handler!"));
        return false;
    }
    auto modeControllerAttributeBuilder = capabilityAgents::modeController::ModeControllerAttributeBuilder::create();
    if (!modeControllerAttributeBuilder) {
        ACSDK_CRITICAL(LX("Failed to create mode controller attribute builder!"));
        return false;
    }

    auto modeCapabilityResources = avsCommon::avs::CapabilityResources();
    if (!modeCapabilityResources.addFriendlyNameWithText("Light", EN_US) ||
        !modeCapabilityResources.addFriendlyNameWithAssetId(avsCommon::avs::resources::ASSET_ALEXA_SETTING_MODE)) {
        ACSDK_CRITICAL(LX("Failed to create Mode Controller capability resources!"));
        return false;
    }

    auto modeRedResources = avsCommon::avs::CapabilityResources();
    if (!modeRedResources.addFriendlyNameWithText("Red", EN_US)) {
        ACSDK_CRITICAL(LX("Failed to create Mode Controller 'Red' mode resources!"));
        return false;
    }

    auto modeGreenResources = avsCommon::avs::CapabilityResources();
    if (!modeGreenResources.addFriendlyNameWithText("Green", EN_US)) {
        ACSDK_CRITICAL(LX("Failed to create Mode Controller 'Green' mode resources!"));
        return false;
    }

    auto modeBlueResources = avsCommon::avs::CapabilityResources();
    if (!modeBlueResources.addFriendlyNameWithText("Blue", EN_US)) {
        ACSDK_CRITICAL(LX("Failed to create Mode Controller 'Blue' mode resources!"));
        return false;
    }

    modeControllerAttributeBuilder->withCapabilityResources(modeCapabilityResources)
        .addMode(ModeControllerHandler::MODE_CONTROLLER_MODE_RED, modeRedResources)
        .addMode(ModeControllerHandler::MODE_CONTROLLER_MODE_GREEN, modeGreenResources)
        .addMode(ModeControllerHandler::MODE_CONTROLLER_MODE_BLUE, modeBlueResources)
        .setOrdered(true);
    auto modeControllerAttributes = modeControllerAttributeBuilder->build();
    if (!modeControllerAttributes.hasValue()) {
        ACSDK_CRITICAL(LX("Failed to create Mode Controller attributes!"));
        return false;
    }

    discoballEndpointBuilder->withModeController(
        discoballModeHandler,
        SAMPLE_ENDPOINT_MODE_CONTROLLER_INSTANCE_NAME,
        modeControllerAttributes.value(),
        true,
        true,
        false);
#endif

    auto discoballEndpointId = discoballEndpointBuilder->build();
    if (!discoballEndpointId.hasValue()) {
        ACSDK_CRITICAL(LX("Failed to create Smart Home Endpoint!"));
        return false;
    }
#endif

// Creating wake word audio provider, if necessary
#ifdef KWD
    bool wakeAlwaysReadable = true;
    bool wakeCanOverride = false;
    bool wakeCanBeOverridden = true;

    alexaClientSDK::capabilityAgents::aip::AudioProvider wakeWordAudioProvider(
        sharedDataStream,
        compatibleAudioFormat,
        alexaClientSDK::capabilityAgents::aip::ASRProfile::NEAR_FIELD,
        wakeAlwaysReadable,
        wakeCanOverride,
        wakeCanBeOverridden);

    // This observer is notified any time a keyword is detected and notifies the DefaultClient to start recognizing.
    auto keywordObserver = std::make_shared<alexaClientSDK::sampleApp::KeywordObserver>(client, wakeWordAudioProvider);

    m_keywordDetector = alexaClientSDK::kwd::KeywordDetectorProvider::create(
        sharedDataStream,
        compatibleAudioFormat,
        {keywordObserver},
        std::unordered_set<
            std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>>(),
        pathToInputFolder);
    if (!m_keywordDetector) {
        ACSDK_CRITICAL(LX("Failed to create keyword detector!"));
    }

    // If wake word is enabled, then creating the interaction manager with a wake word audio provider.
    m_interactionManager = std::make_shared<alexaClientSDK::sampleApp::InteractionManager>(
        client,
        micWrapper,
        userInterfaceManager,
#ifdef ENABLE_PCC
        phoneCaller,
#endif
#ifdef ENABLE_MCC
        meetingClient,
        calendarClient,
#endif
        holdToTalkAudioProvider,
        tapToTalkAudioProvider,
        m_guiRenderer,
        wakeWordAudioProvider
#ifdef POWER_CONTROLLER
        ,
        discoballPowerHandler
#endif
#ifdef TOGGLE_CONTROLLER
        ,
        discoballToggleHandler
#endif
#ifdef RANGE_CONTROLLER
        ,
        discoballRangeHandler
#endif
#ifdef MODE_CONTROLLER
        ,
        discoballModeHandler
#endif
    );

#else
    // clang-format off
    // If wake word is not enabled, then creating the interaction manager without a wake word audio provider.
    m_interactionManager = std::make_shared<alexaClientSDK::sampleApp::InteractionManager>(
        client,
        micWrapper,
        userInterfaceManager,
#ifdef ENABLE_PCC
        phoneCaller,
#endif
#ifdef ENABLE_MCC
        meetingClient,
        calendarClient,
#endif
        holdToTalkAudioProvider,
        tapToTalkAudioProvider,
        m_guiRenderer,
        capabilityAgents::aip::AudioProvider::null()
#ifdef POWER_CONTROLLER
        ,
        discoballPowerHandler
#endif
#ifdef TOGGLE_CONTROLLER
        ,
        discoballToggleHandler
#endif
#ifdef RANGE_CONTROLLER
        ,
        discoballRangeHandler
#endif
#ifdef MODE_CONTROLLER
        ,
        discoballModeHandler
#endif
    );
    // clang-format on
#endif

    client->addAlexaDialogStateObserver(m_interactionManager);
    client->addCallStateObserver(m_interactionManager);

#ifdef ENABLE_REVOKE_AUTH
    // Creating the revoke authorization observer.
    auto revokeObserver =
        std::make_shared<alexaClientSDK::sampleApp::RevokeAuthorizationObserver>(client->getRegistrationManager());
    client->addRevokeAuthorizationObserver(revokeObserver);
#endif

    // Creating the input observer.
    m_userInputManager =
        alexaClientSDK::sampleApp::UserInputManager::create(m_interactionManager, consoleReader, localeAssetsManager);
    if (!m_userInputManager) {
        ACSDK_CRITICAL(LX("Failed to create UserInputManager!"));
        return false;
    }

    authDelegate->addAuthObserver(m_userInputManager);
    client->getRegistrationManager()->addObserver(m_userInputManager);
    m_capabilitiesDelegate->addCapabilitiesObserver(m_userInputManager);

    client->connect();

    return true;
}

std::pair<std::shared_ptr<ApplicationMediaPlayer>, std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>
SampleApplication::createApplicationMediaPlayer(
    std::shared_ptr<avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory> httpContentFetcherFactory,
    bool enableEqualizer,
    avsCommon::sdkInterfaces::SpeakerInterface::Type type,
    const std::string& name,
    bool enableLiveMode) {
#ifdef GSTREAMER_MEDIA_PLAYER
    /*
     * For the SDK, the MediaPlayer happens to also provide volume control functionality.
     * Note the externalMusicProviderMediaPlayer is not added to the set of SpeakerInterfaces as there would be
     * more actions needed for these beyond setting the volume control on the MediaPlayer.
     */
    auto mediaPlayer = alexaClientSDK::mediaPlayer::MediaPlayer::create(
        httpContentFetcherFactory, enableEqualizer, type, name, enableLiveMode);
    return {mediaPlayer,
            std::static_pointer_cast<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface>(mediaPlayer)};
#elif defined(ANDROID_MEDIA_PLAYER)
    // TODO - Add support of live mode to AndroidSLESMediaPlayer (ACSDK-2530).
    auto mediaPlayer = mediaPlayer::android::AndroidSLESMediaPlayer::create(
        httpContentFetcherFactory,
        m_openSlEngine,
        type,
        enableEqualizer,
        mediaPlayer::android::PlaybackConfiguration(),
        name);
    if (!mediaPlayer) {
        return {nullptr, nullptr};
    }
    auto speaker = mediaPlayer->getSpeaker();
    return {std::move(mediaPlayer), speaker};
#endif
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
