/*
 * WebApplication.cpp
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

#include "WebExtension/ConnectionObserver.h"
#include "WebExtension/GuiRenderer.h"
#include "WebExtension/KeywordObserver.h"
#include "WebExtension/WebApplication.h"

#ifdef KWD_KITTAI
#include <KittAi/KittAiKeyWordDetector.h>
#elif KWD_SENSORY
#include <Sensory/SensoryKeywordDetector.h>
#endif

#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>
#include <AVSCommon/Utils/Logger/LoggerSinkManager.h>
#include <Alerts/Storage/SQLiteAlertStorage.h>
#include <Audio/AudioFactory.h>
#include <AuthDelegate/AuthDelegate.h>
#include <MediaPlayer/MediaPlayer.h>
#include <Settings/SQLiteSettingStorage.h>

#include <algorithm>
#include <cctype>
#include <fstream>

namespace alexaClientSDK {
namespace webExtension {

/// The sample rate of microphone audio data.
static const unsigned int SAMPLE_RATE_HZ = 16000;

/// The number of audio channels.
static const unsigned int NUM_CHANNELS = 1;

/// The size of each word within the stream.
static const size_t WORD_SIZE = 2;

/// The maximum number of readers of the stream.
static const size_t MAX_READERS = 10;

/// The amount of audio data to keep in the ring buffer.
static const std::chrono::seconds AMOUNT_OF_AUDIO_DATA_IN_BUFFER = std::chrono::seconds(15);

/// The size of the ring buffer.
static const size_t BUFFER_SIZE_IN_SAMPLES = (SAMPLE_RATE_HZ)*AMOUNT_OF_AUDIO_DATA_IN_BUFFER.count();

/// Key for the root node value containing configuration values for WebExtension.
static const std::string SAMPLE_APP_CONFIG_KEY("webExtension");

/// Key for the endpoint value under the @c SAMPLE_APP_CONFIG_KEY configuration node.
static const std::string ENDPOINT_KEY("endpoint");

/// Key for setting if display cards are supported or not under the @c SAMPLE_APP_CONFIG_KEY configuration node.
static const std::string DISPLAY_CARD_KEY("displayCardsSupported");

#ifdef KWD_KITTAI
/// The sensitivity of the Kitt.ai engine.
static const double KITT_AI_SENSITIVITY = 0.6;

/// The audio amplifier level of the Kitt.ai engine.
static const float KITT_AI_AUDIO_GAIN = 2.0;

/// Whether Kitt.ai should apply front end audio processing.
static const bool KITT_AI_APPLY_FRONT_END_PROCESSING = true;
#endif

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

std::unique_ptr<WebApplication> WebApplication::create(
    const std::string& pathToConfig,
    const std::string& pathToInputFolder,
    const std::string& logLevel) {
    auto clientApplication = std::unique_ptr<WebApplication>(new WebApplication);
    if (!clientApplication->initialize(pathToConfig, pathToInputFolder, logLevel)) {
        ConsolePrinter::simplePrint("Failed to initialize WebApplication");
        return nullptr;
    }
    return clientApplication;
}

void WebApplication::run() {
    m_userInputManager->run();
}

WebApplication::~WebApplication() {
    // First clean up anything that depends on the the MediaPlayers.
    m_userInputManager.reset();

    // Now it's safe to shut down the MediaPlayers.
    m_speakMediaPlayer->shutdown();
    m_audioMediaPlayer->shutdown();
    m_alertsMediaPlayer->shutdown();
}

bool WebApplication::initialize(
    const std::string& pathToConfig,
    const std::string& pathToInputFolder,
    const std::string& logLevel) {
    /*
     * Set up the SDK logging system to write to the WebExtension's ConsolePrinter.  Also adjust the logging level
     * if requested.
     */
    std::shared_ptr<alexaClientSDK::avsCommon::utils::logger::Logger> consolePrinter =
        std::make_shared<alexaClientSDK::webExtension::ConsolePrinter>();

    if (!logLevel.empty()) {
        auto logLevelValue = getLogLevelFromUserInput(logLevel);
        if (alexaClientSDK::avsCommon::utils::logger::Level::UNKNOWN == logLevelValue) {
            alexaClientSDK::webExtension::ConsolePrinter::simplePrint("Unknown log level input!");
            alexaClientSDK::webExtension::ConsolePrinter::simplePrint("Possible log level options are: ");
            for (auto it = allLevels.begin(); it != allLevels.end(); ++it) {
                alexaClientSDK::webExtension::ConsolePrinter::simplePrint(
                    alexaClientSDK::avsCommon::utils::logger::convertLevelToName(*it));
            }
            return false;
        }

        alexaClientSDK::webExtension::ConsolePrinter::simplePrint(
            "Running app with log level: " +
            alexaClientSDK::avsCommon::utils::logger::convertLevelToName(logLevelValue));
        consolePrinter->setLevel(logLevelValue);
    }
    alexaClientSDK::avsCommon::utils::logger::LoggerSinkManager::instance().initialize(consolePrinter);

    /*
     * This is a required step upon startup of the SDK before any modules are created. For that reason, it is being
     * called here, before creating the MediaPlayer, audio streams, DefaultClient, etc.
     */
    std::ifstream configInfile(pathToConfig);
    if (!configInfile.good()) {
        alexaClientSDK::webExtension::ConsolePrinter::simplePrint("Failed to read config file!");
        return false;
    }
    if (!avsCommon::avs::initialization::AlexaClientSDKInit::initialize({&configInfile})) {
        alexaClientSDK::webExtension::ConsolePrinter::simplePrint("Failed to initialize SDK!");
        return false;
    }

    auto config = alexaClientSDK::avsCommon::utils::configuration::ConfigurationNode::getRoot();

    auto httpContentFetcherFactory = std::make_shared<avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory>();

    /*
     * Creating the media players. Here, the default GStreamer based MediaPlayer is being created. However, any
     * MediaPlayer that follows the specified MediaPlayerInterface can work.
     */
    m_speakMediaPlayer = alexaClientSDK::mediaPlayer::MediaPlayer::create(
        httpContentFetcherFactory, avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SYNCED, "SpeakMediaPlayer");
    if (!m_speakMediaPlayer) {
        alexaClientSDK::webExtension::ConsolePrinter::simplePrint("Failed to create media player for speech!");
        return false;
    }

    m_audioMediaPlayer = alexaClientSDK::mediaPlayer::MediaPlayer::create(
        httpContentFetcherFactory, avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SYNCED, "AudioMediaPlayer");
    if (!m_audioMediaPlayer) {
        alexaClientSDK::webExtension::ConsolePrinter::simplePrint("Failed to create media player for content!");
        return false;
    }

    /*
     * The ALERTS speaker type will cause volume control to be independent and localized. By assigning this type,
     * Alerts volume/mute changes will not be in sync with AVS. No directives or events will be associated with volume
     * control.
     */
    m_alertsMediaPlayer = alexaClientSDK::mediaPlayer::MediaPlayer::create(
        httpContentFetcherFactory, avsCommon::sdkInterfaces::SpeakerInterface::Type::LOCAL, "AlertsMediaPlayer");
    if (!m_alertsMediaPlayer) {
        alexaClientSDK::webExtension::ConsolePrinter::simplePrint("Failed to create media player for alerts!");
        return false;
    }

    /*
     * Create Speaker interfaces to control the volume. For the SDK, the MediaPlayer happens to also provide
     * volume control functionality, but this does not have to be case.
     */
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> speakSpeaker =
        std::static_pointer_cast<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface>(m_speakMediaPlayer);
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> audioSpeaker =
        std::static_pointer_cast<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface>(m_audioMediaPlayer);
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> alertsSpeaker =
        std::static_pointer_cast<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface>(m_alertsMediaPlayer);

    auto audioFactory = std::make_shared<alexaClientSDK::applicationUtilities::resources::audio::AudioFactory>();

    // Creating the alert storage object to be used for rendering and storing alerts.
    auto alertStorage =
        std::make_shared<alexaClientSDK::capabilityAgents::alerts::storage::SQLiteAlertStorage>(audioFactory->alerts());

    /*
     * Creating settings storage object to be used for storing <key, value> pairs of AVS Settings.
     */
    auto settingsStorage = std::make_shared<alexaClientSDK::capabilityAgents::settings::SQLiteSettingStorage>();

    /*
     * Creating the UI component that observes various components and prints to the console accordingly.
     */
    auto userInterfaceManager = std::make_shared<alexaClientSDK::webExtension::UIManager>();

    /*
     * Setting up a connection observer to wait for connection and authorization prior to accepting user input at
     * startup.
     */
    auto connectionObserver = std::make_shared<alexaClientSDK::webExtension::ConnectionObserver>();

    /*
     * Creating the AuthDelegate - this component takes care of LWA and authorization of the client. At the moment,
     * this must be done and authorization must be achieved prior to making the call to connect().
     */
    std::shared_ptr<alexaClientSDK::authDelegate::AuthDelegate> authDelegate =
        alexaClientSDK::authDelegate::AuthDelegate::create();

    authDelegate->addAuthObserver(connectionObserver);

    /*
     * Creating the DefaultClient - this component serves as an out-of-box default object that instantiates and "glues"
     * together all the modules.
     */
    std::shared_ptr<alexaClientSDK::defaultClient::DefaultClient> client =
        alexaClientSDK::defaultClient::DefaultClient::create(
            m_speakMediaPlayer,
            m_audioMediaPlayer,
            m_alertsMediaPlayer,
            speakSpeaker,
            audioSpeaker,
            alertsSpeaker,
            audioFactory,
            authDelegate,
            alertStorage,
            settingsStorage,
            {userInterfaceManager},
            {connectionObserver, userInterfaceManager});

    if (!client) {
        alexaClientSDK::webExtension::ConsolePrinter::simplePrint("Failed to create default SDK client!");
        return false;
    }

    /*
     * TODO: ACSDK-384 Remove the requirement of clients having to wait for authorization before making the connect()
     * call.
     */
    if (!connectionObserver->waitFor(
            alexaClientSDK::avsCommon::sdkInterfaces::AuthObserverInterface::State::REFRESHED)) {
        alexaClientSDK::webExtension::ConsolePrinter::simplePrint("Failed to authorize SDK client!");
        return false;
    }

    std::string endpoint;
    config[SAMPLE_APP_CONFIG_KEY].getString(ENDPOINT_KEY, &endpoint);

    client->connect(endpoint);

    if (!connectionObserver->waitFor(avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status::CONNECTED)) {
        alexaClientSDK::webExtension::ConsolePrinter::simplePrint("Failed to connect to AVS!");
        return false;
    }
    // Add userInterfaceManager as observer of locale setting.
    client->addSettingObserver("locale", userInterfaceManager);
    // Send default settings set by the user to AVS.
    client->sendDefaultSettings();

    client->addSpeakerManagerObserver(userInterfaceManager);

    /*
     * Add GUI Renderer as an observer if display cards are supported.  The default is supported unless specified
     * otherwise in the configuration.
     */
    bool displayCardsSupported;
    config[SAMPLE_APP_CONFIG_KEY].getBool(DISPLAY_CARD_KEY, &displayCardsSupported, true);
    if (displayCardsSupported) {
        auto guiRenderer = std::make_shared<GuiRenderer>();
        client->addTemplateRuntimeObserver(guiRenderer);
    }

    /*
     * Creating the buffer (Shared Data Stream) that will hold user audio data. This is the main input into the SDK.
     */
    size_t bufferSize = alexaClientSDK::avsCommon::avs::AudioInputStream::calculateBufferSize(
        BUFFER_SIZE_IN_SAMPLES, WORD_SIZE, MAX_READERS);
    auto buffer = std::make_shared<alexaClientSDK::avsCommon::avs::AudioInputStream::Buffer>(bufferSize);
    std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> sharedDataStream =
        alexaClientSDK::avsCommon::avs::AudioInputStream::create(buffer, WORD_SIZE, MAX_READERS);

    if (!sharedDataStream) {
        alexaClientSDK::webExtension::ConsolePrinter::simplePrint("Failed to create shared data stream!");
        return false;
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

    std::shared_ptr<alexaClientSDK::webExtension::PortAudioMicrophoneWrapper> micWrapper =
        alexaClientSDK::webExtension::PortAudioMicrophoneWrapper::create(sharedDataStream);
    if (!micWrapper) {
        alexaClientSDK::webExtension::ConsolePrinter::simplePrint("Failed to create PortAudioMicrophoneWrapper!");
        return false;
    }

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
    auto keywordObserver = std::make_shared<alexaClientSDK::webExtension::KeywordObserver>(client, wakeWordAudioProvider);

#if defined(KWD_KITTAI)
    m_keywordDetector = alexaClientSDK::kwd::KittAiKeyWordDetector::create(
        sharedDataStream,
        compatibleAudioFormat,
        {keywordObserver},
        std::unordered_set<
            std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>>(),
        pathToInputFolder + "/common.res",
        {{pathToInputFolder + "/alexa.umdl", "ALEXA", KITT_AI_SENSITIVITY}},
        KITT_AI_AUDIO_GAIN,
        KITT_AI_APPLY_FRONT_END_PROCESSING);
    if (!m_keywordDetector) {
        alexaClientSDK::webExtension::ConsolePrinter::simplePrint("Failed to create KittAiKeyWordDetector!");
        return false;
    }
#elif defined(KWD_SENSORY)
    m_keywordDetector = kwd::SensoryKeywordDetector::create(
        sharedDataStream,
        compatibleAudioFormat,
        {keywordObserver},
        std::unordered_set<
            std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>>(),
        pathToInputFolder + "/spot-alexa-rpi-31000.snsr");
    if (!m_keywordDetector) {
        alexaClientSDK::webExtension::ConsolePrinter::simplePrint("Failed to create SensoryKeyWordDetector!");
        return false;
    }
#endif

    // If wake word is enabled, then creating the interaction manager with a wake word audio provider.
    auto interactionManager = std::make_shared<alexaClientSDK::webExtension::InteractionManager>(
        client,
        micWrapper,
        userInterfaceManager,
        holdToTalkAudioProvider,
        tapToTalkAudioProvider,
        wakeWordAudioProvider);

#else
    // If wake word is not enabled, then creating the interaction manager without a wake word audio provider.
    auto interactionManager = std::make_shared<alexaClientSDK::webExtension::InteractionManager>(
        client, micWrapper, userInterfaceManager, holdToTalkAudioProvider, tapToTalkAudioProvider);
#endif

    client->addAlexaDialogStateObserver(interactionManager);

    // Creating the input observer.
    m_userInputManager = alexaClientSDK::webExtension::UserInputManager::create(interactionManager);
    if (!m_userInputManager) {
        alexaClientSDK::webExtension::ConsolePrinter::simplePrint("Failed to create UserInputManager!");
        return false;
    }

    return true;
}

}  // namespace webExtension
}  // namespace alexaClientSDK
