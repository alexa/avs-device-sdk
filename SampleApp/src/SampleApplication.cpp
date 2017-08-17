/*
 * SampleApplication.cpp
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

#include "SampleApp/KeywordObserver.h"
#include "SampleApp/ConnectionObserver.h"
#include "SampleApp/SampleApplication.h"

#ifdef KWD_KITTAI
#include <KittAi/KittAiKeyWordDetector.h>
#elif KWD_SENSORY
#include <Sensory/SensoryKeywordDetector.h>
#endif
#include <Alerts/Storage/SQLiteAlertStorage.h>
#include <AuthDelegate/AuthDelegate.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/Utils/Logger/LoggerSinkManager.h>
#include <MediaPlayer/MediaPlayer.h>

#include <algorithm>
#include <cctype>
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

/// The amount of audio data to keep in the ring buffer.
static const std::chrono::seconds AMOUNT_OF_AUDIO_DATA_IN_BUFFER = std::chrono::seconds(15);

/// The size of the ring buffer.
static const size_t BUFFER_SIZE_IN_SAMPLES = (SAMPLE_RATE_HZ) * AMOUNT_OF_AUDIO_DATA_IN_BUFFER.count();

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
        alexaClientSDK::avsCommon::utils::logger::Level::NONE
};

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
 * The interface used to display messages in the console.
 *
 * TODO: g_consolePrinter is a static/global because it is passed by reference to changeSinkLogger() below,
 * which keeps a reference to it for the lifetime of the logging system.  If the logging system is refactoroed to
 * use shared_ptrs (ACSDK-445), the ConsolePrinter can be instantiated as shared_ptr class member and passed to
 * LoggerSinkManager.
 */
static alexaClientSDK::sampleApp::ConsolePrinter g_consolePrinter;

std::unique_ptr<SampleApplication> SampleApplication::create(
        const std::string& pathToConfig,
        const std::string& pathToInputFolder,
        const std::string& logLevel) {
    auto clientApplication = std::unique_ptr<SampleApplication>(new SampleApplication);
    if (!clientApplication->initialize(pathToConfig, pathToInputFolder, logLevel)) {
        ConsolePrinter::simplePrint("Failed to initialize SampleApplication");
        return nullptr;
    }
    return clientApplication;
}

void SampleApplication::run() {
    m_userInputManager->run();
}

bool SampleApplication::initialize(
        const std::string& pathToConfig,
        const std::string& pathToInputFolder,
        const std::string& logLevel) {
    /*
     * Set up the SDK logging system to write to the SampleApp's ConsolePrinter.  Also adjust the logging level
     * if requested.
     */
    if (!logLevel.empty()) {
        auto logLevelValue = getLogLevelFromUserInput(logLevel);
        if (alexaClientSDK::avsCommon::utils::logger::Level::UNKNOWN == logLevelValue) {
            alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Unknown log level input!");
            alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Possible log level options are: ");
            for (auto it = allLevels.begin(); 
                 it != allLevels.end(); 
                 ++it) {
                alexaClientSDK::sampleApp::ConsolePrinter::simplePrint(
                    alexaClientSDK::avsCommon::utils::logger::convertLevelToName(*it)
                    );
            }
            return false;
        }

        alexaClientSDK::sampleApp::ConsolePrinter::simplePrint(
            "Running app with log level: " +
            alexaClientSDK::avsCommon::utils::logger::convertLevelToName(logLevelValue));
        g_consolePrinter.setLevel(logLevelValue);
    }
    alexaClientSDK::avsCommon::utils::logger::LoggerSinkManager::instance().changeSinkLogger(g_consolePrinter);

    /* 
     * This is a required step upon startup of the SDK before any modules are created. For that reason, it is being
     * called here, before creating the MediaPlayer, audio streams, DefaultClient, etc.
     */
    std::ifstream configInfile(pathToConfig);
    if (!configInfile.good()) {
        alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Failed to read config file!");
        return false;
    }
    if (!avsCommon::avs::initialization::AlexaClientSDKInit::initialize({&configInfile})) {
        alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Failed to initialize SDK!");
        return false;
    }

    /*
     * Creating the media players. Here, the default GStreamer based MediaPlayer is being created. However, any
     * MediaPlayer that follows the specified MediaPlayerInterface can work.
     */
    auto speakMediaPlayer = alexaClientSDK::mediaPlayer::MediaPlayer::create();
    if (!speakMediaPlayer) {
        alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Failed to create media player for speech!");
        return false;
    }

    auto audioMediaPlayer = alexaClientSDK::mediaPlayer::MediaPlayer::create();
    if (!audioMediaPlayer) {
        alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Failed to create media player for content!");
        return false;
    }

    auto alertsMediaPlayer = alexaClientSDK::mediaPlayer::MediaPlayer::create();
    if (!alertsMediaPlayer) {
        alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Failed to create media player for alerts!");
        return false;
    }

    // Creating the alert storage object to be used for rendering and storing alerts. 
    auto alertStorage = std::make_shared<alexaClientSDK::capabilityAgents::alerts::storage::SQLiteAlertStorage>();

    /*
     * Creating the UI component that observes various components and prints to the console accordingly.
     */
    auto userInterfaceManager = std::make_shared<alexaClientSDK::sampleApp::UIManager>();

    /*
     * Setting up a connection observer to wait for connection and authorization prior to accepting user input at
     * startup.
     */
    auto connectionObserver = std::make_shared<alexaClientSDK::sampleApp::ConnectionObserver>();

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
                    speakMediaPlayer, 
                    audioMediaPlayer, 
                    alertsMediaPlayer,
                    authDelegate, 
                    alertStorage,
                    {userInterfaceManager},
                    {connectionObserver, userInterfaceManager});

    if (!client) {
        alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Failed to create default SDK client!");
        return false;
    }

    /*
     * TODO: ACSDK-384 Remove the requirement of clients having to wait for authorization before making the connect() 
     * call.
     */
    if (!connectionObserver->waitFor(
            alexaClientSDK::avsCommon::sdkInterfaces::AuthObserverInterface::State::REFRESHED)) {
        alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Failed to authorize SDK client!");
        return false;
    }

    client->connect();

    if (!connectionObserver->waitFor(
                avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status::POST_CONNECTED)) {
        alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Failed to connect to AVS!");
        return false;
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
        alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Failed to create shared data stream!");
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

    std::shared_ptr<alexaClientSDK::sampleApp::PortAudioMicrophoneWrapper> micWrapper = 
            alexaClientSDK::sampleApp::PortAudioMicrophoneWrapper::create(sharedDataStream);
    if (!micWrapper) {
        alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Failed to create PortAudioMicrophoneWrapper!");
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
    auto keywordObserver = std::make_shared<alexaClientSDK::sampleApp::KeywordObserver>(client, wakeWordAudioProvider);

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
        alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Failed to create KittAiKeywWordDetector!");
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
        alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Failed to create SensoryKeywWordDetector!");
        return false;
    }
#endif
    
    // If wake word is enabled, then creating the interaction manager with a wake word audio provider.
    auto interactionManager = std::make_shared<alexaClientSDK::sampleApp::InteractionManager>(
            client, 
            micWrapper,
            userInterfaceManager,
            holdToTalkAudioProvider, 
            tapToTalkAudioProvider, 
            wakeWordAudioProvider);

#else
    // If wake word is not enabled, then creating the interaction manager without a wake word audio provider.
    auto interactionManager = std::make_shared<alexaClientSDK::sampleApp::InteractionManager>(
            client, 
            micWrapper,
            userInterfaceManager,
            holdToTalkAudioProvider, 
            tapToTalkAudioProvider);
#endif

    // Creating the input observer.
    m_userInputManager = alexaClientSDK::sampleApp::UserInputManager::create(interactionManager);
    if (!m_userInputManager) {
        alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Failed to create UserInputManager!");
        return false;
    }

    return true;
}

} // namespace sampleApp
} // namespace alexaClientSDK
