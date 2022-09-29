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

#include <acsdkAudioInputStream/AudioInputStreamFactory.h>
#include <acsdkAudioInputStream/CompatibleAudioFormat.h>
#include <acsdkManufactory/Manufactory.h>
#include <ACL/Transport/HTTP2TransportFactory.h>
#include <ACL/Transport/PostConnectSequencerFactory.h>
#include <AVSCommon/AVS/CapabilitySemantics/CapabilitySemantics.h>
#include <AVSCommon/AVS/Initialization/InitializationParametersBuilder.h>
#include <AVSCommon/SDKInterfaces/PowerResourceManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPost.h>
#include <AVSCommon/Utils/LibcurlUtils/LibcurlHTTP2ConnectionFactory.h>
#include <AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h>
#include <AVSGatewayManager/AVSGatewayManager.h>
#include <AVSGatewayManager/Storage/AVSGatewayManagerStorage.h>
#include <SynchronizeStateSender/SynchronizeStateSenderFactory.h>

#include <acsdk/AlexaPresentationFeatureClient/AlexaPresentationFeatureClientBuilder.h>
#include <acsdk/PresentationOrchestratorFeatureClient/PresentationOrchestratorFeatureClientBuilder.h>
#include <acsdk/Sample/Endpoint/EndpointFocusAdapter.h>
#include <acsdk/Sample/InteractionManager/UIManager.h>
#include <acsdk/Sample/TemplateRuntime/TemplateRuntimePresentationAdapter.h>
#include <acsdk/SDKClient/SDKClientBuilder.h>
#include <acsdk/VisualCharacteristicsFeatureClient/VisualCharacteristicsFeatureClientBuilder.h>
#include <acsdk/VisualStateTrackerFeatureClient/VisualStateTrackerFeatureClientBuilder.h>
#include <acsdkAuthorization/AuthorizationManager.h>
#include <acsdkAuthorization/LWA/LWAAuthorizationAdapter.h>
#include <acsdkAuthorization/LWA/LWAAuthorizationStorage.h>
#include <acsdkAuthorizationInterfaces/AuthorizationManagerInterface.h>
#include <acsdkSampleApplicationCBLAuthRequester/SampleApplicationCBLAuthRequester.h>

#include <DefaultClient/DefaultClientBuilder.h>

#ifdef ENABLE_VIDEO_CONTROLLERS
#include <acsdk/Sample/Endpoint/EndpointCapabilitiesBuilder.h>
#endif

#ifdef INPUT_CONTROLLER
#include <acsdk/Sample/Endpoint/InputControllerEndpointCapabilitiesBuilder.h>
#endif

#ifdef ENABLE_REVOKE_AUTH
#include "IPCServerSampleApp/RevokeAuthorizationObserver.h"
#endif

#ifdef KWD
#include <acsdkKWDProvider/KWDProvider/KeywordDetectorProvider.h>
#endif

#ifdef PORTAUDIO
#include "IPCServerSampleApp/PortAudioMicrophoneWrapper.h"
#elif defined(UWP_BUILD)
#include "SSSDKCommon/NullMicrophone.h"
#endif

#ifdef GSTREAMER_MEDIA_PLAYER
#include <MediaPlayer/MediaPlayer.h>
#elif defined(UWP_BUILD)
#include <SSSDKCommon/NullMediaSpeaker.h>
#include <SSSDKCommon/NullEqualizer.h>
#endif

#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceConnectionRuleInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Diagnostics/ProtocolTracerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointBuilderInterface.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Logger/LoggerSinkManager.h>
#include <AVSCommon/Utils/Network/InternetConnectionMonitor.h>
#include <acsdkAlerts/Storage/SQLiteAlertStorage.h>
#include <Audio/AudioFactory.h>
#include <Audio/MicrophoneInterface.h>
#include <acsdkBluetooth/BasicDeviceConnectionRule.h>
#include <acsdkBluetooth/SQLiteBluetoothStorage.h>
#include <acsdkNotifications/SQLiteNotificationsStorage.h>
#include <CapabilitiesDelegate/CapabilitiesDelegate.h>
#include <CapabilitiesDelegate/Storage/SQLiteCapabilitiesDelegateStorage.h>
#include <SQLiteStorage/SQLiteMiscStorage.h>
#include <Settings/Storage/SQLiteDeviceSettingStorage.h>
#include "IPCServerSampleApp/SampleEqualizerModeController.h"

#include <acsdkEqualizerImplementations/InMemoryEqualizerConfiguration.h>
#include <acsdkEqualizerImplementations/MiscDBEqualizerStorage.h>
#include <acsdkEqualizerImplementations/SDKConfigEqualizerConfiguration.h>
#include <acsdkEqualizerInterfaces/EqualizerInterface.h>
#include <InterruptModel/config/InterruptModelConfiguration.h>

#include <algorithm>
#include <cctype>
#include <csignal>

#ifndef UWP_BUILD
#include <Communication/WebSocketServer.h>
#else
#include "UWPSampleApp/UWPSampleApp/include/UWPSampleApp/NullSocketServer.h"
#endif

#ifdef CUSTOM_MEDIA_PLAYER
namespace alexaClientSDK {

std::shared_ptr<avsCommon::sdkInterfaces::ApplicationMediaInterfaces> createCustomMediaPlayer(
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory,
    bool enableEqualizer,
    const std::string& name,
    bool enableLiveMode);

}  // namespace alexaClientSDK
#endif

#ifdef ENABLE_RTCSC
#include <acsdk/LiveViewControllerFeatureClient/LiveViewControllerFeatureClientBuilder.h>
#include "IPCServerSampleApp/LiveViewController/LiveViewControllerPresentationAdapter.h"
#endif

#include "IPCServerSampleApp/AlexaPresentation/APLRuntimeInterfaceImpl.h"
#include "IPCServerSampleApp/AlexaPresentation/APLRuntimePresentationAdapter.h"
#include "IPCServerSampleApp/GUI/TemplateRuntimePresentationAdapterBridge.h"
#include "IPCServerSampleApp/ExternalCapabilitiesBuilder.h"
#include "IPCServerSampleApp/KeywordObserver.h"
#include "IPCServerSampleApp/LocaleAssetsManager.h"
#include "IPCServerSampleApp/SampleApplication.h"
#include "IPCServerSampleApp/SampleApplicationComponent.h"
#include "IPCServerSampleApp/SmartScreenCaptionPresenter.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

using namespace avsCommon::sdkInterfaces::storage;

/**
 * WebSocket interface to listen on.
 * WARNING: If this is changed to listen on a publicly accessible interface then additional
 * security precautions will need to be taken to secure client connections and authenticate
 * connecting clients.
 */
static const std::string DEFAULT_WEBSOCKET_INTERFACE = "127.0.0.1";

/// WebSocket port to listen on.
static const int DEFAULT_WEBSOCKET_PORT = 8933;

/// The size of each word within the stream.
static const size_t WORD_SIZE = 2;

/// The maximum number of readers of the stream.
static const size_t MAX_READERS = 10;

/// The default number of MediaPlayers used by AudioPlayer CA
static const unsigned int AUDIO_MEDIAPLAYER_POOL_SIZE_DEFAULT = 2;

#ifdef ENABLE_VIDEO_CONTROLLERS
// Note: The video endpoint is a virtual endpoint for enabling Video capabilities like AlexaLauncher,
// AlexaKeypadController, AlexaPlaybackController, AlexaSeekController, AlexaVideoRecorder, AlexaChannelController,
// AlexaRecordController and AlexaRemoteVideoPlayer.

/// The derived endpoint Id used in endpoint creation.
static const char VIDEO_ENDPOINT_DERIVED_ENDPOINT_ID[] = "Video";

/// The description of the endpoint.
static const char VIDEO_ENDPOINT_DESCRIPTION[] = "Sample Video Description";

/// The friendly name of the video endpoint. This is used in utterance.
static const char VIDEO_ENDPOINT_FRIENDLY_NAME[] = "Video";

/// The manufacturer of video endpoint.
static const char VIDEO_ENDPOINT_MANUFACTURER_NAME[] = "Sample Manufacturer";

/// The display category of the video endpoint.
static const std::vector<std::string> VIDEO_ENDPOINT_DISPLAYCATEGORY({"OTHER"});

/// The model of the video endpoint.
static const char VIDEO_ENDPOINT_ADDITIONAL_ATTRIBUTE_MODEL[] = "Model1";

/// Serial number of the video endpoint.
static const char VIDEO_ENDPOINT_ADDITIONAL_ATTRIBUTE_SERIAL_NUMBER[] = "123456789";

/// Firmware version number of the video endpoint.
static const char VIDEO_ENDPOINT_ADDITIONAL_ATTRIBUTE_FIRMWARE_VERSION[] = "1.0";

/// Software Version number of the video endpoint.
static const char VIDEO_ENDPOINT_ADDITIONAL_ATTRIBUTE_SOFTWARE_VERSION[] = "1.0";

/// The custom identifier of the video endpoint.
static const char VIDEO_ENDPOINT_ADDITIONAL_ATTRIBUTE_CUSTOM_IDENTIFIER[] = "IPCServerSampleApp";
#endif  // ENABLE_VIDEO_CONTROLLERS

/// The amount of audio data to keep in the ring buffer.
static const std::chrono::seconds AMOUNT_OF_AUDIO_DATA_IN_BUFFER = std::chrono::seconds(15);

/// Key for the root node value containing configuration values for SampleApp.
static const std::string SAMPLE_APP_CONFIG_KEY("sampleApp");

/// Key for the root node value containing configuration values for Equalizer.
static const std::string EQUALIZER_CONFIG_KEY("equalizer");

/// Key for the @c firmwareVersion value under the @c SAMPLE_APP_CONFIG_KEY configuration node.
static const std::string FIRMWARE_VERSION_KEY("firmwareVersion");

/// Key for the @c websocket config under the @c SAMPLE_APP_CONFIG_KEY configuration node.
static const std::string WEBSOCKET_CONFIG_KEY("websocket");

/// Key for setting the interface which websockets will bind to @c WEBSOCKET_CONFIG_KEY configuration node.
static const std::string WEBSOCKET_INTERFACE_KEY("websocketInterface");

/// Key for setting the port number which websockets will listen on @c WEBSOCKET_CONFIG_KEY configuration node.
static const std::string WEBSOCKET_PORT_KEY("websocketPort");

/// Key for setting the certificate file for use by websockets when SSL is enabled, @c SAMPLE_APP_CONFIG configuration
/// node.
static const std::string WEBSOCKET_CERTIFICATE("websocketCertificate");

/// Key for setting the private key file for use by websockets when SSL is enabled, @c SAMPLE_APP_CONFIG configuration
/// node.
static const std::string WEBSOCKET_PRIVATE_KEY("websocketPrivateKey");

/// Key for setting the certificate authority file for use by websockets when SSL is enabled, @c SAMPLE_APP_CONFIG
/// configuration node.
static const std::string WEBSOCKET_CERTIFICATE_AUTHORITY("websocketCertificateAuthority");

/// Key for the Audio MediaPlayer pool size.
static const std::string AUDIO_MEDIAPLAYER_POOL_SIZE_KEY("audioMediaPlayerPoolSize");

/// Key for the root node value containing configuration values for APL Content Cache.
static const std::string APL_CONTENT_CACHE_CONFIG_KEY("aplContentCache");

/// Key for cache reuse period for imported packages in seconds.
static const std::string CONTENT_CACHE_REUSE_PERIOD_IN_SECONDS_KEY("contentCacheReusePeriodInSeconds");

/// Default value for cache reuse period.
static const std::string DEFAULT_CONTENT_CACHE_REUSE_PERIOD_IN_SECONDS("600");

/// Key for max number of cache entries for imported packages.
static const std::string CONTENT_CACHE_MAX_SIZE_KEY("contentCacheMaxSize");

/// Default value for max number of cache entries for imported packages.
static const std::string DEFAULT_CONTENT_CACHE_MAX_SIZE("50");

/// The key in our config file to find the maxNumberOfConcurrentDownloads configuration.
static const std::string MAX_NUMBER_OF_CONCURRENT_DOWNLOAD_CONFIGURATION_KEY = "maxNumberOfConcurrentDownloads";

/// The default value for the maximum number of concurrent downloads.
static const int DEFAULT_MAX_NUMBER_OF_CONCURRENT_DOWNLOAD = 5;

/// Misc Storage app component name.
static const std::string MISC_STORAGE_APP_COMPONENT_NAME("IPCServerSampleApp");

/// Misc Storage APL table name.
static const std::string MISC_STORAGE_APL_TABLE_NAME("APL");

/// Storage key name for APLMaxVersion entry.
static const std::string APL_MAX_VERSION_DB_KEY("APLMaxVersion");

using namespace alexaClientSDK;
using namespace acsdkExternalMediaPlayer;
using namespace acsdkManufactory;
using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using MediaPlayerInterface = avsCommon::utils::mediaPlayer::MediaPlayerInterface;
using ApplicationMediaInterfaces = avsCommon::sdkInterfaces::ApplicationMediaInterfaces;

/// The @c m_playerToMediaPlayerMap Map of the adapter to their speaker-type and MediaPlayer creation methods.
std::unordered_map<std::string, avsCommon::sdkInterfaces::ChannelVolumeInterface::Type>
    SampleApplication::m_playerToSpeakerTypeMap;

/// The singleton map from @c playerId to @c ExternalMediaAdapter creation functions.
std::unordered_map<std::string, ExternalMediaPlayer::AdapterCreateFunction> SampleApplication::m_adapterToCreateFuncMap;

/// String to identify log entries originating from this file.
static const std::string TAG("SampleApplication");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

/// A set of all log levels.
static const std::set<avsCommon::utils::logger::Level> allLevels = {avsCommon::utils::logger::Level::DEBUG9,
                                                                    avsCommon::utils::logger::Level::DEBUG8,
                                                                    avsCommon::utils::logger::Level::DEBUG7,
                                                                    avsCommon::utils::logger::Level::DEBUG6,
                                                                    avsCommon::utils::logger::Level::DEBUG5,
                                                                    avsCommon::utils::logger::Level::DEBUG4,
                                                                    avsCommon::utils::logger::Level::DEBUG3,
                                                                    avsCommon::utils::logger::Level::DEBUG2,
                                                                    avsCommon::utils::logger::Level::DEBUG1,
                                                                    avsCommon::utils::logger::Level::DEBUG0,
                                                                    avsCommon::utils::logger::Level::INFO,
                                                                    avsCommon::utils::logger::Level::WARN,
                                                                    avsCommon::utils::logger::Level::ERROR,
                                                                    avsCommon::utils::logger::Level::CRITICAL,
                                                                    avsCommon::utils::logger::Level::NONE};

/**
 * Gets a log level consumable by the SDK based on the user input string for log level.
 *
 * @param userInputLogLevel The string to be parsed into a log level.
 * @return The log level. This will default to NONE if the input string is not properly parsable.
 */
static avsCommon::utils::logger::Level getLogLevelFromUserInput(std::string userInputLogLevel) {
    std::transform(userInputLogLevel.begin(), userInputLogLevel.end(), userInputLogLevel.begin(), ::toupper);
    return avsCommon::utils::logger::convertNameToLevel(userInputLogLevel);
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
    const std::vector<std::string>& configFiles,
    const std::string& logLevel,
    std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface> diagnostics) {
    auto clientApplication = std::unique_ptr<SampleApplication>(new SampleApplication);
    if (!clientApplication->initialize(configFiles, logLevel, diagnostics)) {
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
    ACSDK_DEBUG0(LX(__func__).d("id", playerId));
    if (m_adapterToCreateFuncMap.find(playerId) != m_adapterToCreateFuncMap.end()) {
        ACSDK_WARN(LX("Adapter already exists").d("playerID", playerId));
    }

    m_adapterToCreateFuncMap[playerId] = createFunction;
}

SampleApplication::MediaPlayerRegistration::MediaPlayerRegistration(
    const std::string& playerId,
    avsCommon::sdkInterfaces::ChannelVolumeInterface::Type speakerType) {
    ACSDK_DEBUG0(LX(__func__).d("id", playerId));
    if (m_playerToSpeakerTypeMap.find(playerId) != m_playerToSpeakerTypeMap.end()) {
        ACSDK_WARN(LX("MediaPlayer already exists").d("playerId", playerId));
    }

    m_playerToSpeakerTypeMap[playerId] = speakerType;
}

SampleAppReturnCode SampleApplication::run() {
    return m_guiClient->run();
}

SampleApplication::~SampleApplication() {
    if (m_aplClientBridge) {
        m_aplClientBridge->shutdown();
    }

    if (m_guiManager) {
        m_guiManager->shutdown();
    }

    if (m_guiClient) {
        m_guiClient->shutdown();
    }
    // Clean up anything that depends on the the MediaPlayers.
    m_externalMusicProviderMediaPlayersMap.clear();

    for (auto& shutdownable : m_shutdownRequiredList) {
        if (!shutdownable) {
            ACSDK_ERROR(LX("shutdownFailed").m("Component requiring shutdown was null"));
        }
        shutdownable->shutdown();
    }

    m_sdkInit.reset();
}

bool SampleApplication::createMediaPlayersForAdapters(
    const std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface>&
        httpContentFetcherFactory,
    const std::shared_ptr<defaultClient::EqualizerRuntimeSetup> equalizerRuntimeSetup) {
    bool equalizerEnabled = equalizerRuntimeSetup->isEnabled();

    for (auto& entry : m_playerToSpeakerTypeMap) {
        auto applicationMediaInterfaces = createApplicationMediaPlayer(
            httpContentFetcherFactory, equalizerEnabled, entry.first + "MediaPlayer", false);
        if (applicationMediaInterfaces) {
            m_externalMusicProviderMediaPlayersMap[entry.first] = applicationMediaInterfaces->mediaPlayer;
            m_externalMusicProviderSpeakersMap[entry.first] = applicationMediaInterfaces->speaker;
            if (equalizerEnabled) {
                equalizerRuntimeSetup->addEqualizer(applicationMediaInterfaces->equalizer);
            }
        } else {
            ACSDK_CRITICAL(LX("Failed to create application media interface").d("playerId", entry.first));
            return false;
        }
    }

    return true;
}

bool SampleApplication::initialize(
    const std::vector<std::string>& configFiles,
    const std::string& logLevel,
    std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface> diagnostics) {
    avsCommon::utils::logger::Level logLevelValue = avsCommon::utils::logger::Level::UNKNOWN;

    if (!logLevel.empty()) {
        logLevelValue = getLogLevelFromUserInput(logLevel);
        if (avsCommon::utils::logger::Level::UNKNOWN == logLevelValue) {
            ConsolePrinter::simplePrint("Unknown log level input!");
            ConsolePrinter::simplePrint("Possible log level options are: ");
            for (auto it = allLevels.begin(); it != allLevels.end(); ++it) {
                ConsolePrinter::simplePrint(avsCommon::utils::logger::convertLevelToName(*it));
            }
            return false;
        }

        ConsolePrinter::simplePrint(
            "Running app with log level: " + avsCommon::utils::logger::convertLevelToName(logLevelValue));
    }

    avsCommon::utils::logger::LoggerSinkManager::instance().setLevel(logLevelValue);

    auto configJsonStreams = std::make_shared<std::vector<std::shared_ptr<std::istream>>>();

    for (auto& configFile : configFiles) {
        if (configFile.empty()) {
            ConsolePrinter::simplePrint("Config filename is empty!");
            return false;
        }

        auto configInFile = std::shared_ptr<std::ifstream>(new std::ifstream(configFile));
        if (!configInFile->good()) {
            ACSDK_CRITICAL(LX("Failed to read config file").d("filename", configFile));
            ConsolePrinter::simplePrint("Failed to read config file " + configFile);
            return false;
        }

        configJsonStreams->push_back(configInFile);
    }

    bool enableDucking = true;

#ifdef DISABLE_DUCKING
    enableDucking = false;
#endif

    // Add the InterruptModel Configuration.
    configJsonStreams->push_back(afml::interruptModel::InterruptModelConfiguration::getConfig(enableDucking));

    auto builder = initialization::InitializationParametersBuilder::create();
    if (!builder) {
        ACSDK_ERROR(LX("createInitializeParamsFailed").d("reason", "nullBuilder"));
        return false;
    }

    builder->withJsonStreams(configJsonStreams);

    auto initParams = builder->build();
    auto sampleAppComponent = getComponent(std::move(initParams), m_shutdownRequiredList);

    auto manufactory = Manufactory<
        std::shared_ptr<avsCommon::avs::initialization::AlexaClientSDKInit>,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>,
        std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>,
        std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>,
        std::shared_ptr<avsCommon::utils::DeviceInfo>,
        std::shared_ptr<registrationManager::CustomerDataManagerInterface>,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>,
        std::shared_ptr<cryptoInterfaces::CryptoFactoryInterface>,
        std::shared_ptr<cryptoInterfaces::KeyStoreInterface>>::create(sampleAppComponent);

    auto metricRecorder = manufactory->get<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>();

    m_sdkInit = manufactory->get<std::shared_ptr<avsCommon::avs::initialization::AlexaClientSDKInit>>();
    if (!m_sdkInit) {
        ACSDK_CRITICAL(LX("Failed to get SDKInit!"));
        return false;
    }

    auto configPtr = manufactory->get<std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>>();
    if (!configPtr) {
        ACSDK_CRITICAL(LX("Failed to acquire the configuration"));
        return false;
    }
    auto& config = *configPtr;

#ifdef ENABLE_CONFIG_VALIDATION

    /*
     * Creating config validator
     */
    auto configValidator = ConfigValidator::create();
    if (!configValidator) {
        ACSDK_CRITICAL(LX("Failed to create config validator!"));
        return false;
    }

    auto schemaStringInAscii = decodeHexToAscii(SCHEMA_HEX);
    std::stringstream schemaStream;
    schemaStream << schemaStringInAscii;

    if (schemaStream.good()) {
        rapidjson::IStreamWrapper isw(schemaStream);
        rapidjson::Document jsonSchema;
        if (!jsonSchema.ParseStream(isw).HasParseError()) {
            // Validating config parameters
            if (!configValidator->validate(config, jsonSchema)) {
                ACSDK_ERROR(LX("Configuration validation failed!"));
                return false;
            }
        } else {
            ACSDK_ERROR(LX("Configuration file could not be validated!").d("reason", "invalid json schema"));
            return false;
        }
    } else {
        ACSDK_ERROR(LX("Configuration file could not be validated!").d("reason", "failed to read schema string"));
        return false;
    }

#endif

    auto sampleAppConfig = config[SAMPLE_APP_CONFIG_KEY];

    auto httpContentFetcherFactory = std::make_shared<avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory>();

    // Creating the misc DB object to be used by various components.
    std::shared_ptr<alexaClientSDK::storage::sqliteStorage::SQLiteMiscStorage> miscStorage =
        alexaClientSDK::storage::sqliteStorage::SQLiteMiscStorage::create(config);

    if (!miscStorage->isOpened() && !miscStorage->open()) {
        ACSDK_DEBUG3(LX("openStorage").m("Couldn't open misc database. Creating."));

        if (!miscStorage->createDatabase()) {
            ACSDK_ERROR(LX("openStorageFailed").m("Could not create misc database."));
            return false;
        }
    }

    std::string APLVersion;
    std::string websocketInterface;
    auto websocketConfig = sampleAppConfig[WEBSOCKET_CONFIG_KEY];
    websocketConfig.getString(WEBSOCKET_INTERFACE_KEY, &websocketInterface, DEFAULT_WEBSOCKET_INTERFACE);

    int websocketPortNumber = DEFAULT_WEBSOCKET_PORT;
    websocketConfig.getInt(WEBSOCKET_PORT_KEY, &websocketPortNumber, DEFAULT_WEBSOCKET_PORT);

    // Create the websocket server that handles communications with websocket clients

#ifdef UWP_BUILD
    auto webSocketServer = std::make_shared<NullSocketServer>();
#else
    auto webSocketServer = std::make_shared<communication::WebSocketServer>(websocketInterface, websocketPortNumber);

#ifdef ENABLE_WEBSOCKET_SSL
    std::string sslCaFile;
    websocketConfig.getString(WEBSOCKET_CERTIFICATE_AUTHORITY, &sslCaFile);

    std::string sslCertificateFile;
    websocketConfig.getString(WEBSOCKET_CERTIFICATE, &sslCertificateFile);

    std::string sslPrivateKeyFile;
    websocketConfig.getString(WEBSOCKET_PRIVATE_KEY, &sslPrivateKeyFile);

    webSocketServer->setCertificateFile(sslCaFile, sslCertificateFile, sslPrivateKeyFile);
#endif  // ENABLE_WEBSOCKET_SSL

#endif  // UWP_BUILD

    /*
     * Creating customerDataManager which will be used by the registrationManager and all classes that extend
     * CustomerDataHandler
     */
    auto customerDataManager = manufactory->get<std::shared_ptr<registrationManager::CustomerDataManagerInterface>>();
    if (!customerDataManager) {
        ACSDK_CRITICAL(LX("Failed to get CustomerDataManager!"));
        return false;
    }

    m_activityEventNotifier = gui::GUIActivityEventNotifier::create();
    auto ipcVersionManager = std::make_shared<ipc::IPCVersionManager>();

    /*
     * Creating the deviceInfo object
     */
    auto deviceInfo = manufactory->get<std::shared_ptr<avsCommon::utils::DeviceInfo>>();
    if (!deviceInfo) {
        ACSDK_CRITICAL(LX("Creation of DeviceInfo failed!"));
        return false;
    }

    m_guiClient = gui::GUIClient::create(webSocketServer, customerDataManager, ipcVersionManager, deviceInfo);

    if (!m_guiClient) {
        ACSDK_CRITICAL(LX("Creation of GUIClient failed!"));
        return false;
    }

    // Initialize the IPC router
    m_guiClient->initIPCRouter();

    /*
     * Creating Equalizer specific implementations
     */
    auto equalizerConfigBranch = config[EQUALIZER_CONFIG_KEY];
    auto equalizerConfiguration = acsdkEqualizer::SDKConfigEqualizerConfiguration::create(equalizerConfigBranch);
    std::shared_ptr<defaultClient::EqualizerRuntimeSetup> equalizerRuntimeSetup =
        std::make_shared<defaultClient::EqualizerRuntimeSetup>(false);

    bool equalizerEnabled = false;
    if (equalizerConfiguration && equalizerConfiguration->isEnabled()) {
        equalizerEnabled = true;
        equalizerRuntimeSetup = std::make_shared<defaultClient::EqualizerRuntimeSetup>();
        auto equalizerStorage = acsdkEqualizer::MiscDBEqualizerStorage::create(miscStorage);
        auto equalizerModeController = SampleEqualizerModeController::create();

        equalizerRuntimeSetup->setStorage(equalizerStorage);
        equalizerRuntimeSetup->setConfiguration(equalizerConfiguration);
        equalizerRuntimeSetup->setModeController(equalizerModeController);
    }

    auto speakerMediaInterfaces = createApplicationMediaPlayer(httpContentFetcherFactory, false, "SpeakMediaPlayer");
    if (!speakerMediaInterfaces) {
        ACSDK_CRITICAL(LX("Failed to create application media interfaces for speech!"));
        return false;
    }
    m_speakMediaPlayer = speakerMediaInterfaces->mediaPlayer;

    int poolSize = AUDIO_MEDIAPLAYER_POOL_SIZE_DEFAULT;
    sampleAppConfig.getInt(AUDIO_MEDIAPLAYER_POOL_SIZE_KEY, &poolSize, AUDIO_MEDIAPLAYER_POOL_SIZE_DEFAULT);
    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>> audioSpeakers;

    for (int index = 0; index < poolSize; index++) {
        std::shared_ptr<MediaPlayerInterface> mediaPlayer;
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> speaker;
        std::shared_ptr<acsdkEqualizerInterfaces::EqualizerInterface> equalizer;

        auto audioMediaInterfaces =
            createApplicationMediaPlayer(httpContentFetcherFactory, equalizerEnabled, "AudioMediaPlayer");
        if (!audioMediaInterfaces) {
            ACSDK_CRITICAL(LX("Failed to create application media interfaces for audio!"));
            return false;
        }
        m_audioMediaPlayerPool.push_back(audioMediaInterfaces->mediaPlayer);
        audioSpeakers.push_back(audioMediaInterfaces->speaker);
        // Creating equalizers
        if (equalizerRuntimeSetup->isEnabled()) {
            equalizerRuntimeSetup->addEqualizer(audioMediaInterfaces->equalizer);
        }
    }

    avsCommon::utils::Optional<avsCommon::utils::mediaPlayer::Fingerprint> fingerprint =
        (*(m_audioMediaPlayerPool.begin()))->getFingerprint();
    auto audioMediaPlayerFactory = std::unique_ptr<mediaPlayer::PooledMediaPlayerFactory>();
    if (fingerprint.hasValue()) {
        audioMediaPlayerFactory =
            mediaPlayer::PooledMediaPlayerFactory::create(m_audioMediaPlayerPool, fingerprint.value());
    } else {
        audioMediaPlayerFactory = mediaPlayer::PooledMediaPlayerFactory::create(m_audioMediaPlayerPool);
    }
    if (!audioMediaPlayerFactory) {
        ACSDK_CRITICAL(LX("Failed to create media player factory for content!"));
        return false;
    }

    auto notificationMediaInterfaces =
        createApplicationMediaPlayer(httpContentFetcherFactory, false, "NotificationsMediaPlayer");
    if (!notificationMediaInterfaces) {
        ACSDK_CRITICAL(LX("Failed to create application media interfaces for notifications!"));
        return false;
    }
    m_notificationsMediaPlayer = notificationMediaInterfaces->mediaPlayer;

    auto bluetoothMediaInterfaces =
        createApplicationMediaPlayer(httpContentFetcherFactory, false, "BluetoothMediaPlayer");
    if (!bluetoothMediaInterfaces) {
        ACSDK_CRITICAL(LX("Failed to create application media interfaces for bluetooth!"));
        return false;
    }
    m_bluetoothMediaPlayer = bluetoothMediaInterfaces->mediaPlayer;

    auto ringtoneMediaInterfaces =
        createApplicationMediaPlayer(httpContentFetcherFactory, false, "RingtoneMediaPlayer");
    if (!ringtoneMediaInterfaces) {
        ConsolePrinter::simplePrint("Failed to create application media interfaces for ringtones!");
        return false;
    }
    m_ringtoneMediaPlayer = ringtoneMediaInterfaces->mediaPlayer;

#ifdef ENABLE_COMMS_AUDIO_PROXY
    auto commsMediaInterfaces =
        createApplicationMediaPlayer(httpContentFetcherFactory, false, "CommsMediaPlayer", true);
    if (!commsMediaInterfaces) {
        ACSDK_CRITICAL(LX("Failed to create application media interfaces for comms!"));
        return false;
    }
    m_commsMediaPlayer = commsMediaInterfaces->mediaPlayer;
    auto commsSpeaker = commsMediaInterfaces->speaker;
#endif

    auto alertsMediaInterfaces = createApplicationMediaPlayer(httpContentFetcherFactory, false, "AlertsMediaPlayer");
    if (!alertsMediaInterfaces) {
        ACSDK_CRITICAL(LX("Failed to create application media interfaces for alerts!"));
        return false;
    }

    m_alertsMediaPlayer = alertsMediaInterfaces->mediaPlayer;
    auto systemSoundMediaInterfaces =
        createApplicationMediaPlayer(httpContentFetcherFactory, false, "SystemSoundMediaPlayer");
    if (!systemSoundMediaInterfaces) {
        ACSDK_CRITICAL(LX("Failed to create application media interfaces for system sound player!"));
        return false;
    }
    m_systemSoundMediaPlayer = systemSoundMediaInterfaces->mediaPlayer;

    if (!createMediaPlayersForAdapters(httpContentFetcherFactory, equalizerRuntimeSetup)) {
        ACSDK_CRITICAL(LX("Could not create mediaPlayers for adapters"));
        return false;
    }

    auto audioFactory = std::make_shared<applicationUtilities::resources::audio::AudioFactory>();

    // Creating the alert storage object to be used for rendering and storing alerts.
    auto alertStorage =
        acsdkAlerts::storage::SQLiteAlertStorage::create(config, audioFactory->alerts(), metricRecorder);

    // Creating the message storage object to be used for storing message to be sent later.
    auto messageStorage = certifiedSender::SQLiteMessageStorage::create(config);

    /*
     * Creating notifications storage object to be used for storing notification indicators.
     */
    auto notificationsStorage = acsdkNotifications::SQLiteNotificationsStorage::create(config);

    /*
     * Creating new device settings storage object to be used for storing AVS Settings.
     */
    auto deviceSettingsStorage = settings::storage::SQLiteDeviceSettingStorage::create(config);

    /*
     * Creating bluetooth storage object to be used for storing uuid to mac mappings for devices.
     */
    auto bluetoothStorage = acsdkBluetooth::SQLiteBluetoothStorage::create(config);

    /*
     * Create sample locale asset manager.
     */
    auto localeAssetsManager = manufactory->get<std::shared_ptr<LocaleAssetsManagerInterface>>();
    if (!localeAssetsManager) {
        ACSDK_CRITICAL(LX("Failed to create Locale Assets Manager!"));
        return false;
    }

    std::string cachePeriodInSeconds;
    std::string maxCacheSize;

    auto aplContentCacheConfig = sampleAppConfig[APL_CONTENT_CACHE_CONFIG_KEY];

    aplContentCacheConfig.getString(
        CONTENT_CACHE_REUSE_PERIOD_IN_SECONDS_KEY,
        &cachePeriodInSeconds,
        DEFAULT_CONTENT_CACHE_REUSE_PERIOD_IN_SECONDS);
    aplContentCacheConfig.getString(CONTENT_CACHE_MAX_SIZE_KEY, &maxCacheSize, DEFAULT_CONTENT_CACHE_MAX_SIZE);

    auto contentDownloadManager = std::make_shared<CachingDownloadManager>(
        httpContentFetcherFactory,
        std::stol(cachePeriodInSeconds),
        std::stol(maxCacheSize),
        miscStorage,
        customerDataManager);

    int maxNumberOfConcurrentDownloads = DEFAULT_MAX_NUMBER_OF_CONCURRENT_DOWNLOAD;
    aplContentCacheConfig.getInt(
        MAX_NUMBER_OF_CONCURRENT_DOWNLOAD_CONFIGURATION_KEY,
        &maxNumberOfConcurrentDownloads,
        DEFAULT_MAX_NUMBER_OF_CONCURRENT_DOWNLOAD);

    if (1 > maxNumberOfConcurrentDownloads) {
        maxNumberOfConcurrentDownloads = DEFAULT_MAX_NUMBER_OF_CONCURRENT_DOWNLOAD;
        ACSDK_ERROR(LX("Invalid values for maxNumberOfConcurrentDownloads"));
    }

    auto parameters = AplClientBridgeParameter{maxNumberOfConcurrentDownloads};
    m_aplClientBridge = AplClientBridge::create(contentDownloadManager, m_guiClient, parameters);
    APLVersion = m_aplClientBridge->getMaxAPLVersion();

    bool aplTableExists = false;
    if (!miscStorage->tableExists(MISC_STORAGE_APP_COMPONENT_NAME, MISC_STORAGE_APL_TABLE_NAME, &aplTableExists)) {
        ACSDK_ERROR(LX("openStorageFailed").m("Could not get apl table information from misc database."));
        return false;
    }

    if (!aplTableExists) {
        ACSDK_DEBUG3(LX("openStorage").d("apl table doesn't exist", MISC_STORAGE_APL_TABLE_NAME));
        if (!miscStorage->createTable(
                MISC_STORAGE_APP_COMPONENT_NAME,
                MISC_STORAGE_APL_TABLE_NAME,
                MiscStorageInterface::KeyType::STRING_KEY,
                MiscStorageInterface::ValueType::STRING_VALUE)) {
            ACSDK_ERROR(LX("openStorageFailed")
                            .d("reason", "Could not create table")
                            .d("table", MISC_STORAGE_APL_TABLE_NAME)
                            .d("component", MISC_STORAGE_APP_COMPONENT_NAME));
            return false;
        }
    }

    std::string APLDBVersion;
    if (!miscStorage->get(
            MISC_STORAGE_APP_COMPONENT_NAME, MISC_STORAGE_APL_TABLE_NAME, APL_MAX_VERSION_DB_KEY, &APLDBVersion)) {
        ACSDK_ERROR(LX("getAPLMaxVersionFromStorageFailed").d("reason", "storage failure"));
    }
    ACSDK_DEBUG3(LX(__func__).d("APLDBVersion", APLDBVersion));
    if (APLDBVersion.empty()) {
        ACSDK_INFO(LX("no APL version saved").d("reason", "couldn't find saved APLDBVersion"));
    }

    bool aplVersionChanged = false;
    if (APLVersion != APLDBVersion) {
        // Empty value does not constitute version change, and should not result in app reset.
        if (!APLDBVersion.empty()) {
            aplVersionChanged = true;
        }
        if (!miscStorage->put(
                MISC_STORAGE_APP_COMPONENT_NAME, MISC_STORAGE_APL_TABLE_NAME, APL_MAX_VERSION_DB_KEY, APLVersion)) {
            ACSDK_ERROR(LX("saveAPLMaxVersionInStorage").m("Could not set new value"));
        } else {
            ACSDK_DEBUG1(LX("saveAPLMaxVersionInStorage").m("succeeded"));
        }
    }

    m_guiClient->setAplClientBridge(m_aplClientBridge, aplVersionChanged);

    m_alexaCaptionIPCHandler = ipc::AlexaCaptionsHandler::create(m_guiClient->getIPCRouter(), miscStorage);
    /*
     * Create the presentation layer for the captions.
     */
    auto captionPresenter = std::make_shared<SmartScreenCaptionPresenter>(m_alexaCaptionIPCHandler);

    // Creating the UI component that observes various components and prints to the console accordingly.
    auto uiManager = common::UIManager::create(localeAssetsManager, deviceInfo);
    if (!uiManager) {
        ACSDK_CRITICAL(LX("Failed to get UIManager!"));
        return false;
    }

    uiManager->setUIAuthNotifier(m_guiClient);
    uiManager->setUIStateAggregator(m_guiClient);

    auto cblAuthRequester = acsdkSampleApplicationCBLAuthRequester::SampleApplicationCBLAuthRequester::
        createCBLAuthorizationObserverInterface(uiManager);
    if (!cblAuthRequester) {
        ACSDK_CRITICAL(LX("Failed to initialize CBLAuthRequester"));
        return false;
    }

    std::static_pointer_cast<acsdkSampleApplicationCBLAuthRequester::SampleApplicationCBLAuthRequester>(
        cblAuthRequester)
        ->setUIAuthNotifier(m_guiClient);

    /*
     * Supply a SALT for UUID generation, this should be as unique to each individual device as possible
     */
    avsCommon::utils::uuidGeneration::setSalt(deviceInfo->getClientId() + deviceInfo->getDeviceSerialNumber());

    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate;

    m_authManager = acsdkAuthorization::AuthorizationManager::create(miscStorage, customerDataManager);
    if (!m_authManager) {
        ACSDK_CRITICAL(LX("Failed to create AuthorizationManager!"));
        return false;
    }

    m_shutdownRequiredList.push_back(m_authManager);
    authDelegate = m_authManager;

    if (!authDelegate) {
        ACSDK_CRITICAL(LX("Creation of AuthDelegate failed!"));
        return false;
    }

    /*
     * Creating the CapabilitiesDelegate - This component provides the client with the ability to send messages to the
     * Capabilities API.
     */
    auto capabilitiesDelegateStorage = capabilitiesDelegate::storage::SQLiteCapabilitiesDelegateStorage::create(config);

    m_capabilitiesDelegate = capabilitiesDelegate::CapabilitiesDelegate::create(
        authDelegate, std::move(capabilitiesDelegateStorage), customerDataManager);

    if (!m_capabilitiesDelegate) {
        ACSDK_CRITICAL(LX("Creation of CapabilitiesDelegate failed!"));
        return false;
    }

    m_shutdownRequiredList.push_back(m_capabilitiesDelegate);
    authDelegate->addAuthObserver(uiManager);
    m_capabilitiesDelegate->addCapabilitiesObserver(uiManager);

    // INVALID_FIRMWARE_VERSION is passed to @c getInt() as a default in case FIRMWARE_VERSION_KEY is not found.
    int firmwareVersion = static_cast<int>(avsCommon::sdkInterfaces::softwareInfo::INVALID_FIRMWARE_VERSION);
    sampleAppConfig.getInt(FIRMWARE_VERSION_KEY, &firmwareVersion, firmwareVersion);

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
    auto contextManager = manufactory->get<std::shared_ptr<ContextManagerInterface>>();
    if (!contextManager) {
        ACSDK_CRITICAL(LX("Creation of ContextManager failed."));
        return false;
    }

    auto avsGatewayManagerStorage = avsGatewayManager::storage::AVSGatewayManagerStorage::create(miscStorage);
    if (!avsGatewayManagerStorage) {
        ACSDK_CRITICAL(LX("Creation of AVSGatewayManagerStorage failed"));
        return false;
    }
    auto avsGatewayManager = avsGatewayManager::AVSGatewayManager::create(
        std::move(avsGatewayManagerStorage), customerDataManager, config, authDelegate);
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

    std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::ProtocolTracerInterface> deviceProtocolTracer;
    if (diagnostics) {
        /*
         * Create the deviceProtocolTracer to trace events and directives.
         */
        deviceProtocolTracer = diagnostics->getProtocolTracer();

        if (deviceProtocolTracer) {
            const std::string DIAGNOSTICS_KEY = "diagnostics";
            const std::string MAX_TRACED_MESSAGES_KEY = "maxTracedMessages";
            const std::string TRACE_FROM_STARTUP = "protocolTraceFromStartup";

            int configMaxValue = -1;
            bool configProtocolTraceEnabled = false;

            if (avsCommon::utils::configuration::ConfigurationNode::getRoot()[DIAGNOSTICS_KEY].getInt(
                    MAX_TRACED_MESSAGES_KEY, &configMaxValue)) {
                if (configMaxValue < 0) {
                    ACSDK_WARN(LX("ignoringMaxTracedMessages")
                                   .d("reason", "negativeValue")
                                   .d("maxTracedMessages", configMaxValue));
                } else {
                    deviceProtocolTracer->setMaxMessages(static_cast<unsigned int>(configMaxValue));
                }
            }

            if (avsCommon::utils::configuration::ConfigurationNode::getRoot()[DIAGNOSTICS_KEY].getBool(
                    TRACE_FROM_STARTUP, &configProtocolTraceEnabled)) {
                if (configProtocolTraceEnabled) {
                    deviceProtocolTracer->setProtocolTraceFlag(configProtocolTraceEnabled);
                    ACSDK_DEBUG9(LX("Protocol Trace has been enabled at startup"));
                }
            }
        }
    }

    /*
     * Create a factory to create objects that establish a connection with AVS.
     */
    auto transportFactory = std::make_shared<acl::HTTP2TransportFactory>(
        std::make_shared<avsCommon::utils::libcurlUtils::LibcurlHTTP2ConnectionFactory>(),
        postConnectSequencerFactory,
        nullptr,
        deviceProtocolTracer);

    /*
     * Creating the buffer (Shared Data Stream) that will hold user audio data. This is the main input into the SDK.
     */
    std::shared_ptr<avsCommon::utils::AudioFormat> compatibleAudioFormat =
        acsdkAudioInputStream::CompatibleAudioFormat::getCompatibleAudioFormat();

    std::shared_ptr<avsCommon::avs::AudioInputStream> sharedDataStream =
        acsdkAudioInputStream::AudioInputStreamFactory::createAudioInputStream(
            compatibleAudioFormat, WORD_SIZE, MAX_READERS, AMOUNT_OF_AUDIO_DATA_IN_BUFFER);

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
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceConnectionRuleInterface>>
        enabledConnectionRules;
    enabledConnectionRules.insert(acsdkBluetooth::BasicDeviceConnectionRule::create());

    /*
     * Creating each of the audio providers. An audio provider is a simple package of data consisting of the stream
     * of audio data, as well as metadata about the stream. For each of the three audio providers created here, the same
     * stream is used since this sample application will only have one microphone.
     */

    // Creating tap to talk audio provider
    capabilityAgents::aip::AudioProvider tapToTalkAudioProvider =
        capabilityAgents::aip::AudioProvider::AudioProvider::TapAudioProvider(sharedDataStream, *compatibleAudioFormat);

    // Creating hold to talk audio provider
    capabilityAgents::aip::AudioProvider holdToTalkAudioProvider =
        capabilityAgents::aip::AudioProvider::AudioProvider::HoldAudioProvider(
            sharedDataStream, *compatibleAudioFormat);

    // Creating wake word audio provider, if necessary
#ifdef KWD
    capabilityAgents::aip::AudioProvider wakeWordAudioProvider =
        capabilityAgents::aip::AudioProvider::WakeAudioProvider(sharedDataStream, *compatibleAudioFormat);
#endif  // KWD

#ifdef PORTAUDIO
    std::shared_ptr<PortAudioMicrophoneWrapper> micWrapper = PortAudioMicrophoneWrapper::create(sharedDataStream);
#elif AUDIO_INJECTION
    std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::AudioInjectorInterface> audioInjector;
    if (diagnostics) {
        audioInjector = diagnostics->getAudioInjector();
    }

    if (!audioInjector) {
        ACSDK_CRITICAL(LX("No audio injector provided!"));
        return false;
    }
    std::shared_ptr<applicationUtilities::resources::audio::MicrophoneInterface> micWrapper =
        audioInjector->getMicrophone(sharedDataStream, *compatibleAudioFormat);
#elif defined(UWP_BUILD)
    std::shared_ptr<alexaSmartScreenSDK::sssdkCommon::NullMicrophone> micWrapper =
        std::make_shared<alexaSmartScreenSDK::sssdkCommon::NullMicrophone>(sharedDataStream);
#else
    ACSDK_CRITICAL(LX("No microphone module provided!"));
    return false;
#endif
    if (!micWrapper) {
        ACSDK_CRITICAL(LX("Failed to create microphone wrapper!"));
        return false;
    }

    auto aplRuntimeInterface = APLRuntimeInterfaceImpl::create(m_aplClientBridge);
    if (!aplRuntimeInterface) {
        ACSDK_CRITICAL(LX("Failed to create APLRuntimeInterface!"));
        return false;
    }

    auto aplRuntimePresentationAdapter =
        APLRuntimePresentationAdapter::create(aplRuntimeInterface, m_activityEventNotifier);

    if (!aplRuntimePresentationAdapter) {
        ACSDK_CRITICAL(LX("Failed to create APLRuntimePresentationAdapter!"));
        return false;
    }

    auto templateRuntimePresentationAdapter = sampleApplications::common::TemplateRuntimePresentationAdapter::create();

    if (!templateRuntimePresentationAdapter) {
        ACSDK_CRITICAL(LX("Failed to create TemplateRuntimePresentationAdapter!"));
        return false;
    }

    auto templateRuntimePresentationAdapterBridge =
        gui::TemplateRuntimePresentationAdapterBridge::create(templateRuntimePresentationAdapter);

    if (!templateRuntimePresentationAdapterBridge) {
        ACSDK_CRITICAL(LX("Failed to create TemplateRuntimePresentationAdapterBridge!"));
        return false;
    }

    m_templateRuntimeIPCHandler =
        ipc::TemplateRuntimeHandler::create(m_guiClient->getIPCRouter(), templateRuntimePresentationAdapterBridge);

    templateRuntimePresentationAdapter->addTemplateRuntimePresentationAdapterObserver(m_templateRuntimeIPCHandler);
    templateRuntimePresentationAdapter->addTemplateRuntimePresentationAdapterObserver(m_aplClientBridge);

    /*
     * Retrieve the cryptoFactory - This component provides the encryption facilities
     */
    auto cryptoFactory = manufactory->get<std::shared_ptr<cryptoInterfaces::CryptoFactoryInterface>>();

    /*
     * Creating the various feature clients which will add all the features that are used by the application.
     * The FeatureClientBuilder will glue them all together and build a single client which can access them
     */
    sdkClient::SDKClientBuilder sdkClientBuilder;

    /*
     * Creating the DefaultClient builder - this client builds most of the core functionality required by the
     * application
     */
    auto defaultClientBuilder = defaultClient::DefaultClientBuilder::create(
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
        speakerMediaInterfaces->speaker,
        audioSpeakers,
        alertsMediaInterfaces->speaker,
        notificationMediaInterfaces->speaker,
        bluetoothMediaInterfaces->speaker,
        ringtoneMediaInterfaces->speaker,
        systemSoundMediaInterfaces->speaker,
        {},
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
        {uiManager},
        {uiManager},
        std::move(internetConnectionMonitor),
        true,
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
        std::move(bluetoothDeviceManager),
        metricRecorder,
        nullptr,
        diagnostics,
        std::make_shared<ExternalCapabilitiesBuilder>(deviceInfo),
        speakerManager::createChannelVolumeFactory(),
        true,
        std::make_shared<acl::MessageRouterFactory>(),
        nullptr,
        capabilityAgents::aip::AudioProvider::null(),
        cryptoFactory);

    if (!defaultClientBuilder) {
        ACSDK_CRITICAL(LX("Failed to create default client builder!"));
        return false;
    }
    sdkClientBuilder.withFeature(std::move(defaultClientBuilder));

    /**
     * Creating the VisualCharacteristicsFeatureClientBuilder, this will add visual characteristics features to the
     * application
     */
    auto visualCharacteristicsFeatureBuilder = featureClient::VisualCharacteristicsFeatureClientBuilder::create();
    if (!visualCharacteristicsFeatureBuilder) {
        ACSDK_CRITICAL(LX("Failed to create visual characteristics client builder!"));
        return false;
    }
    sdkClientBuilder.withFeature(std::move(visualCharacteristicsFeatureBuilder));

    /**
     * Creating the VisualStateTrackerFeatureClientBuilder, this will add visual state tracking features to the
     * application
     */
    auto visualStateTrackerFeatureBuilder = featureClient::VisualStateTrackerFeatureClientBuilder::create();
    if (!visualStateTrackerFeatureBuilder) {
        ACSDK_CRITICAL(LX("Failed to create visual state tracker client builder!"));
        return false;
    }
    sdkClientBuilder.withFeature(std::move(visualStateTrackerFeatureBuilder));

    /**
     * Creating the PresentationOrchestratorFeatureClientBuilder, this will add presentation orchestrator features to
     * the application
     */
    auto presentationOrchestratorFeatureBuilder = featureClient::PresentationOrchestratorFeatureClientBuilder::create();
    if (!presentationOrchestratorFeatureBuilder) {
        ACSDK_CRITICAL(LX("Failed to create presentation orchestrator client builder!"));
        return false;
    }
    sdkClientBuilder.withFeature(std::move(presentationOrchestratorFeatureBuilder));

    /**
     * Creating the AlexaPresentationFeatureClientBuilder, this will add APL features to the application
     */
    auto alexaPresentationFeatureBuilder =
        featureClient::AlexaPresentationFeatureClientBuilder::create(APLVersion, aplRuntimePresentationAdapter);
    if (!alexaPresentationFeatureBuilder) {
        ACSDK_CRITICAL(LX("Failed to create alexa presentation client builder!"));
        return false;
    }
    sdkClientBuilder.withFeature(std::move(alexaPresentationFeatureBuilder));

#ifdef ENABLE_RTCSC
    /**
     * Creating the LiveViewControllerFeatureClientBuilder, this will add LiveViewController features to the
     * application.
     */
    auto liveViewControllerPresentationAdapter = liveViewController::LiveViewControllerPresentationAdapter::create(
        m_guiClient->getIPCRouter(), m_aplClientBridge);

    if (!liveViewControllerPresentationAdapter) {
        ACSDK_CRITICAL(LX("Failed to create LiveViewControllerPresentationAdapter!"));
        return false;
    }

    auto liveViewControllerFeatureBuilder = featureClient::LiveViewControllerFeatureClientBuilder::create(
        deviceInfo->getDeviceSerialNumber(), liveViewControllerPresentationAdapter);
    if (!liveViewControllerFeatureBuilder) {
        ACSDK_CRITICAL(LX("Failed to create live view controller client builder!"));
        return false;
    }
    sdkClientBuilder.withFeature(std::move(liveViewControllerFeatureBuilder));
#endif

    m_clientRegistry = sdkClientBuilder.build();
    if (!m_clientRegistry) {
        ACSDK_CRITICAL(LX("Failed to build SDK Client!"));
        return false;
    }

    m_defaultClient = m_clientRegistry->get<defaultClient::DefaultClient>();
    if (!m_defaultClient) {
        ACSDK_CRITICAL(LX("Failed to retrieve default SDK client!"));
        return false;
    }

    auto poFeatureClient = m_clientRegistry->get<featureClient::PresentationOrchestratorFeatureClient>();
    if (!poFeatureClient) {
        ACSDK_CRITICAL(LX("Failed to retrieve PresentationOrchestratorFeatureClient!"));
        return false;
    }

    auto aplFeatureClient = m_clientRegistry->get<featureClient::AlexaPresentationFeatureClient>();
    if (!aplFeatureClient) {
        ACSDK_CRITICAL(LX("Failed to retrieve AlexaPresentationFeatureClient!"));
        return false;
    }

#ifdef ENABLE_RTCSC
    auto liveViewControllerFeatureClient = m_clientRegistry->get<featureClient::LiveViewControllerFeatureClient>();
    if (!liveViewControllerFeatureClient) {
        ACSDK_CRITICAL(LX("Failed to retrieve LiveViewControllerFeatureClient!"));
        return false;
    }

    liveViewControllerPresentationAdapter->setPresentationOrchestrator(
        poFeatureClient->getPresentationOrchestratorClient());

    m_defaultClient->addAudioInputProcessorObserver(liveViewControllerPresentationAdapter);
#endif

    aplRuntimeInterface->setPresentationOrchestrator(poFeatureClient->getPresentationOrchestratorClient());
    templateRuntimePresentationAdapter->setPresentationOrchestrator(
        poFeatureClient->getPresentationOrchestratorClient());
    aplRuntimePresentationAdapter->setAlexaPresentationCA(aplFeatureClient->getAlexaPresentationCapabilityAgent());

    m_defaultClient->addSpeakerManagerObserver(uiManager);
    m_defaultClient->addNotificationsObserver(uiManager);
    m_defaultClient->addTemplateRuntimeObserver(templateRuntimePresentationAdapter);
    aplFeatureClient->addAPLCapabilityAgentObserver(aplRuntimePresentationAdapter);

    m_defaultClient->addAudioPlayerObserver(m_aplClientBridge);
    m_defaultClient->addAudioPlayerObserver(templateRuntimePresentationAdapter);
    m_defaultClient->addExternalMediaPlayerObserver(m_aplClientBridge);

    m_defaultClient->addAlexaDialogStateObserver(poFeatureClient->getVisualTimeoutManager());
    m_defaultClient->addAlexaDialogStateObserver(aplRuntimePresentationAdapter);
    m_activityEventNotifier->addObserver(poFeatureClient->getVisualTimeoutManager());
    m_activityEventNotifier->addObserver(aplRuntimePresentationAdapter);
    m_guiClient->setAPLRuntimePresentationAdapter(aplRuntimePresentationAdapter);
#ifdef KWD
    m_keywordDetector = kwd::KeywordDetectorProvider::create(
        sharedDataStream,
        *compatibleAudioFormat,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordObserverInterface>>(),
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>>());
    if (!m_keywordDetector) {
        ACSDK_CRITICAL(LX("Failed to create keyword detector!"));
    }

    // This observer is notified any time a keyword is detected and notifies the DefaultClient to start recognizing.
    auto keywordObserver = KeywordObserver::create(m_defaultClient, wakeWordAudioProvider, m_keywordDetector);
#endif

#ifdef ENABLE_CAPTIONS
    std::vector<std::shared_ptr<MediaPlayerInterface>> captionableMediaSources = m_audioMediaPlayerPool;
    captionableMediaSources.emplace_back(m_speakMediaPlayer);
    m_defaultClient->addCaptionPresenter(captionPresenter);
#ifndef UWP_BUILD
    m_defaultClient->setCaptionMediaPlayers(captionableMediaSources);
#endif
#endif

    std::shared_ptr<common::EndpointAlexaLauncherHandler> launcherHandler;

#ifdef ENABLE_VIDEO_CONTROLLERS
    auto visualStateTrackerFeatureClient = m_clientRegistry->get<featureClient::VisualStateTrackerFeatureClient>();
    if (!visualStateTrackerFeatureClient) {
        ACSDK_CRITICAL(LX("setClientFailed").d("reason", "null state tracker client"));
        return false;
    }

    // Create the endpoint focus adapter.
    auto endpointFocusAdapter = sampleApplications::common::EndpointFocusAdapter::create(
        m_defaultClient->getAudioFocusManager(),
        poFeatureClient->getPresentationOrchestrator(),
        visualStateTrackerFeatureClient->getPresentationOrchestratorStateTracker());

    if (!endpointFocusAdapter) {
        ACSDK_CRITICAL(LX("Failed to create EndpointFocusAdapter!"));
        return false;
    }

    auto videoEndpointBuilder = m_defaultClient->createEndpointBuilder();
    if (!videoEndpointBuilder) {
        ACSDK_CRITICAL(LX("Failed to create Video endpoint Builder!"));
        return false;
    }

    videoEndpointBuilder->withDerivedEndpointId(VIDEO_ENDPOINT_DERIVED_ENDPOINT_ID)
        .withDeviceRegistration()
        .withDescription(VIDEO_ENDPOINT_DESCRIPTION)
        .withFriendlyName(VIDEO_ENDPOINT_FRIENDLY_NAME)
        .withManufacturerName(VIDEO_ENDPOINT_MANUFACTURER_NAME)
        .withAdditionalAttributes(
            VIDEO_ENDPOINT_MANUFACTURER_NAME,
            VIDEO_ENDPOINT_ADDITIONAL_ATTRIBUTE_MODEL,
            VIDEO_ENDPOINT_ADDITIONAL_ATTRIBUTE_SERIAL_NUMBER,
            VIDEO_ENDPOINT_ADDITIONAL_ATTRIBUTE_FIRMWARE_VERSION,
            VIDEO_ENDPOINT_ADDITIONAL_ATTRIBUTE_SOFTWARE_VERSION,
            VIDEO_ENDPOINT_ADDITIONAL_ATTRIBUTE_CUSTOM_IDENTIFIER)
        .withDisplayCategory(VIDEO_ENDPOINT_DISPLAYCATEGORY);

    auto videoEndpointCapabilitiesBuilder =
        std::make_shared<alexaClientSDK::sampleApplications::common::EndpointCapabilitiesBuilder>(endpointFocusAdapter);
    if (!videoEndpointCapabilitiesBuilder) {
        ACSDK_CRITICAL(LX("addCapabilitiesBuilderToVideoEndpointFailed").m("invalidVideoEndpointCapabilitiesBuilder"));
        return false;
    }
    videoEndpointBuilder->withEndpointCapabilitiesBuilder(videoEndpointCapabilitiesBuilder);

#ifdef INPUT_CONTROLLER
    // Enable the InputController on the Video Endpoint.
    auto inputControllerEndpointCapabilitiesBuilder =
        std::make_shared<sampleApplications::common::InputControllerEndpointCapabilitiesBuilder>();
    if (!inputControllerEndpointCapabilitiesBuilder) {
        ACSDK_CRITICAL(
            LX("addCapabilitiesBuilderToVideoEndpointFailed").m("invalidInputControllerEndpointCapabilitiesBuilder"));
        return false;
    }
    videoEndpointBuilder->withEndpointCapabilitiesBuilder(inputControllerEndpointCapabilitiesBuilder);
#endif  // INPUT_CONTROLLER

    auto videoEndpoint = videoEndpointBuilder->build();
    if (!videoEndpoint) {
        ACSDK_CRITICAL(LX("Failed to create Video Endpoint!"));
        return false;
    }

    launcherHandler = videoEndpointCapabilitiesBuilder->getAlexaLauncherHandler();
    m_defaultClient->registerEndpoint(std::move(videoEndpoint));
#endif  // ENABLE_VIDEO_CONTROLLERS

#if defined(INPUT_CONTROLLER) && !defined(ENABLE_VIDEO_CONTROLLERS)
    // Enable the InputController on the default endpoint
    auto defaultEndpointInputControllerBuilder =
        std::make_shared<alexaClientSDK::sampleApplications::common::InputControllerEndpointCapabilitiesBuilder>();
    if (!defaultEndpointInputControllerBuilder) {
        ACSDK_CRITICAL(
            LX("addCapabilitiesBuilderToDefaultEndpointFailed").m("invalidInputControllerEndpointCapabilitiesBuilder"));
        return false;
    }

    m_defaultClient->getDefaultEndpointBuilder()->withEndpointCapabilitiesBuilder(
        defaultEndpointInputControllerBuilder);
#endif

    // clang-format off
    // If wake word is not enabled, then creating the interaction manager without a wake word audio provider.
    m_interactionManager = std::make_shared<common::InteractionManager>(
        m_defaultClient,
        micWrapper,
        uiManager,
        holdToTalkAudioProvider,
        tapToTalkAudioProvider,
#ifdef KWD
        wakeWordAudioProvider,
#else
        capabilityAgents::aip::AudioProvider::null(),
#endif
        nullptr,
        diagnostics);

    if (m_interactionManager) {
        m_shutdownRequiredList.push_back(m_interactionManager);
    }

#ifdef ENABLE_RTCSC
    liveViewControllerPresentationAdapter->setInteractionManager(m_interactionManager);
#endif

#ifdef KWD
    // If wake word is enabled, then creating the GUI manager with a wake word audio provider.
    m_guiManager = gui::GUIManager::create(
        m_guiClient,
        m_activityEventNotifier,
        micWrapper,
        m_interactionManager,
        launcherHandler);
#else
    // If wake word is not enabled, then creating the gui manager without a wake word audio provider.
    m_guiManager = gui::GUIManager::create(
        m_guiClient,
        m_activityEventNotifier,
        micWrapper,
        m_interactionManager,
        launcherHandler);
#endif  // KWD

    m_guiManager->setClient(m_clientRegistry);
    m_guiClient->setGUIManager(m_guiManager);

    m_defaultClient->addAlexaDialogStateObserver(m_guiManager);
    m_defaultClient->addAlexaDialogStateObserver(m_interactionManager);
#ifdef ENABLE_COMMS
    m_defaultClient->addCallStateObserver(m_interactionManager);
#endif
    m_defaultClient->addAudioPlayerObserver(m_guiManager);
    m_defaultClient->getAudioFocusManager()->addObserver(m_guiManager);
    m_defaultClient->addAudioInputProcessorObserver(m_guiManager);

#ifdef ENABLE_REVOKE_AUTH
    // Creating the revoke authorization observer.
    auto revokeObserver = std::make_shared<RevokeAuthorizationObserver>(
        m_defaultClient->getRegistrationManager());
    m_defaultClient->addRevokeAuthorizationObserver(revokeObserver);
#endif  // ENABLE_REVOKE_AUTH

    authDelegate->addAuthObserver(m_guiClient);
    m_defaultClient->addRegistrationObserver(m_guiClient);
    m_capabilitiesDelegate->addCapabilitiesObserver(m_guiClient);

    m_authManager->setRegistrationManager(m_defaultClient->getRegistrationManager());

    auto httpPost = avsCommon::utils::libcurlUtils::HttpPost::createHttpPostInterface();
    auto keyStore = manufactory->get<std::shared_ptr<cryptoInterfaces::KeyStoreInterface>>();

    m_lwaAdapter = acsdkAuthorization::lwa::LWAAuthorizationAdapter::create(
        configPtr,
        std::move(httpPost),
        deviceInfo,
        acsdkAuthorization::lwa::LWAAuthorizationStorage::createLWAAuthorizationStorageInterface(
            configPtr, "", cryptoFactory, keyStore));

    if (!m_lwaAdapter) {
        ACSDK_CRITICAL(LX("Failed to create LWA Adapter!"));
        return false;
    }

    m_authManager->add(m_lwaAdapter);

    m_lwaAdapter->authorizeUsingCBL(cblAuthRequester);

    m_guiManager->setIpcVersionManager(ipcVersionManager);
    m_guiManager->configureSettingsNotifications();

    m_guiManager->setAPLRuntimePresentationAdapter(aplRuntimePresentationAdapter);
    m_guiManager->setTemplateRuntimePresentationAdapterBridge(templateRuntimePresentationAdapterBridge);

    if (!m_guiClient->start()) {
        return false;
    }

    m_defaultClient->connect();

    return true;
}

std::shared_ptr<ApplicationMediaInterfaces> SampleApplication::createApplicationMediaPlayer(
    const std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface>&
        httpContentFetcherFactory,
    bool enableEqualizer,
    const std::string& name,
    bool enableLiveMode) {
#ifdef GSTREAMER_MEDIA_PLAYER
    /*
     * For the SDK, the MediaPlayer happens to also provide volume control functionality.
     * Note the externalMusicProviderMediaPlayer is not added to the set of SpeakerInterfaces as there would be
     * more actions needed for these beyond setting the volume control on the MediaPlayer.
     */
    auto mediaPlayer = mediaPlayer::MediaPlayer::create(
        httpContentFetcherFactory, enableEqualizer, name, enableLiveMode);
    if (!mediaPlayer) {
        return nullptr;
    }
    auto speaker = std::static_pointer_cast<avsCommon::sdkInterfaces::SpeakerInterface>(mediaPlayer);
    auto equalizer =
        std::static_pointer_cast<acsdkEqualizerInterfaces::EqualizerInterface>(mediaPlayer);
    auto requiresShutdown = std::static_pointer_cast<avsCommon::utils::RequiresShutdown>(mediaPlayer);
    auto applicationMediaInterfaces =
        std::make_shared<ApplicationMediaInterfaces>(mediaPlayer, speaker, equalizer, requiresShutdown);
#elif defined(CUSTOM_MEDIA_PLAYER)
    // Custom media players must implement the createCustomMediaPlayer function
    auto applicationMediaInterfaces =
        createCustomMediaPlayer(httpContentFetcherFactory, enableEqualizer, name, enableLiveMode);
    if (!applicationMediaInterfaces) {
        return nullptr;
    }

#elif defined(UWP_BUILD)
    auto mediaPlayer = std::make_shared<alexaSmartScreenSDK::sssdkCommon::TestMediaPlayer>();
    auto nullEqualizer = std::make_shared<alexaSmartScreenSDK::sssdkCommon::NullEqualizer>();

    auto speaker = std::make_shared<alexaSmartScreenSDK::sssdkCommon::NullMediaSpeaker>();
    auto requiresShutdown = std::static_pointer_cast<avsCommon::utils::RequiresShutdown>(mediaPlayer);

    return std::make_shared<ApplicationMediaInterfaces>(mediaPlayer, speaker, nullEqualizer, requiresShutdown);
#endif

#ifndef UWP_BUILD
    if (applicationMediaInterfaces->requiresShutdown) {
        m_shutdownRequiredList.push_back(applicationMediaInterfaces->requiresShutdown);
    }
    return applicationMediaInterfaces;
#endif
}

std::string SampleApplication::decodeHexToAscii(const std::string hexString) {
    std::string asciiString(hexString.size() / 2, '\0');
    std::string byte(2, '\0');
    for (size_t i = 0; i < hexString.size() - 1; i += 2) {
        byte[0] = hexString[i];
        byte[1] = hexString[i + 1];
        asciiString[i / 2] = static_cast<char>(std::stoi(byte, nullptr, 16));
    }

    return asciiString;
}

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
