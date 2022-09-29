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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_SAMPLEAPPLICATION_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_SAMPLEAPPLICATION_H_

#include <fstream>
#include <map>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <acsdkAuthorization/AuthorizationManager.h>
#include <acsdkAuthorization/LWA/LWAAuthorizationAdapter.h>
#include <acsdk/SDKClient/SDKClientRegistry.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/SDKInterfaces/ApplicationMediaInterfaces.h>
#include <AVSCommon/SDKInterfaces/ChannelVolumeInterface.h>
#include <AVSCommon/SDKInterfaces/Diagnostics/DiagnosticsInterface.h>
#include <AVSCommon/Utils/MediaPlayer/PooledMediaPlayerFactory.h>
#include <CapabilitiesDelegate/CapabilitiesDelegate.h>
#include <acsdkExternalMediaPlayer/ExternalMediaPlayer.h>

#include "ConsolePrinter.h"
#include <DefaultClient/EqualizerRuntimeSetup.h>

#include "SampleApplicationReturnCodes.h"

#ifdef KWD
#include <acsdkKWDImplementations/AbstractKeywordDetector.h>
#endif

#ifdef GSTREAMER_MEDIA_PLAYER
#include <MediaPlayer/MediaPlayer.h>
#elif defined(UWP_BUILD)
#include <SSSDKCommon/TestMediaPlayer.h>
#endif  // GSTREAMER_MEDIA_PLAYER

#include <acsdk/Sample/InteractionManager/InteractionManager.h>

#include "GUI/GUIActivityEventNotifier.h"
#include "GUI/GUIClient.h"
#include "GUI/GUIManager.h"
#include "IPC/Components/AlexaCaptionsHandler.h"
#include "IPC/Components/TemplateRuntimeHandler.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

/// Class to manage the top-level components of the AVS Client Application
class SampleApplication {
public:
    /**
     * Create a SampleApplication.
     *
     * @param configFiles The vector of configuration files.
     * @param logLevel The level of logging to enable.  If this parameter is an empty string, the SDK's default
     *     logging level will be used.
     * @param diagnostics An optional @c DiagnosticsInterface object to provide diagnostics on the SDK.
     * @return A new @c SampleApplication, or @c nullptr if the operation failed.
     */
    static std::unique_ptr<SampleApplication> create(
        const std::vector<std::string>& configFiles,
        const std::string& logLevel = "",
        std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface> diagnostics = nullptr);

    /**
     * Runs the application, blocking until the user asks the application to quit or a device reset is triggered.
     *
     * @return Returns a @c SampleAppReturnCode.
     */
    SampleAppReturnCode run();

    /// Destructor which manages the @c SampleApplication shutdown sequence.
    ~SampleApplication();
#ifdef UWP_BUILD
    std::shared_ptr<gui::GUIClient> getGUIClient() {
        return m_guiClient;
    }

    std::shared_ptr<gui::GUIManager> getGUIManager() {
        return m_guiManager;
    }
#endif

    /**
     * Method to create mediaPlayers for the optional music provider adapters plugged into the SDK.
     *
     * @param httpContentFetcherFactory The HTTPContentFetcherFactory to be used while creating the mediaPlayers.
     * @param equalizerRuntimeSetup Equalizer runtime setup to register equalizers
     * @return @c true if the mediaPlayer of all the registered adapters could be created @c false otherwise.
     */
    bool createMediaPlayersForAdapters(
        const std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface>&
            httpContentFetcherFactory,
        const std::shared_ptr<defaultClient::EqualizerRuntimeSetup> equalizerRuntimeSetup);

    /**
     * Instances of this class register ExternalMediaAdapters. Each adapter registers itself by instantiating
     * a static instance of the below class supplying their business name and creator method.
     */
    class AdapterRegistration {
    public:
        /**
         * Register an @c ExternalMediaAdapter for use by @c ExternalMediaPlayer.
         *
         * @param playerId The @c playerId identifying the @c ExternalMediaAdapter to register.
         * @param createFunction The function to use to create instances of the specified @c ExternalMediaAdapter.
         */
        AdapterRegistration(
            const std::string& playerId,
            acsdkExternalMediaPlayer::ExternalMediaPlayer::AdapterCreateFunction createFunction);
    };

    /**
     * Instances of this class register MediaPlayers to be created. Each third-party adapter registers a mediaPlayer
     * for itself by instantiating a static instance of the below class supplying their business name, speaker interface
     * type and creator method.
     */
    class MediaPlayerRegistration {
    public:
        /**
         * Register a @c MediaPlayer for use by a music provider adapter.
         *
         * @param playerId The @c playerId identifying the @c ExternalMediaAdapter to register.
         * @param speakerType The SpeakerType of the mediaPlayer to be created.
         */
        MediaPlayerRegistration(
            const std::string& playerId,
            avsCommon::sdkInterfaces::ChannelVolumeInterface::Type speakerType);
    };

private:
    /**
     * Initialize a SampleApplication.
     *
     * @param configFiles The vector of configuration files.
     * @param logLevel The level of logging to enable.  If this parameter is an empty string, the SDK's default
     *     logging level will be used.
     * @param diagnostics An optional @c DiagnosticsInterface object to provide diagnostics on the SDK.
     * @return @c true if initialization succeeded, else @c false.
     */
    bool initialize(
        const std::vector<std::string>& configFiles,
        const std::string& logLevel,
        std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface> diagnostics);

    /**
     * Create an application media player.
     *
     * @param contentFetcherFactory Used to create objects that can fetch remote HTTP content.
     * @param enableEqualizer Flag indicating if equalizer should be enabled for this media player.
     * @param name The media player instance name used for logging purpose.
     * @param enableLiveMode Flag, indicating if the player is in live mode.
     * @return Application Media interface if it succeeds; otherwise, return @c nullptr.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::ApplicationMediaInterfaces> createApplicationMediaPlayer(
        const std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface>&
            httpContentFetcherFactory,
        bool enableEqualizer,
        const std::string& name,
        bool enableLiveMode = false);

    /// Object with which to trigger shutdown operations.
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface> m_shutdownManager;

    /// Object to manage lifecycle of Alexa Client SDK initialization.
    std::shared_ptr<avsCommon::avs::initialization::AlexaClientSDKInit> m_sdkInit;

    /// The @c SDKClientRegistry which contains all feature clients
    std::shared_ptr<sdkClient::SDKClientRegistry> m_clientRegistry;

    /// The @c DefaultClient which "glues" together all other modules.
    std::shared_ptr<defaultClient::DefaultClient> m_defaultClient;

    /// The @c InteractionManager which perform user interaction.
    std::shared_ptr<common::InteractionManager> m_interactionManager;

    /// The @c GUIClient
    std::shared_ptr<gui::GUIClient> m_guiClient;

    /// The @c GUIManager
    std::shared_ptr<gui::GUIManager> m_guiManager;

    /// The @c AplClientBridge
    std::shared_ptr<AplClientBridge> m_aplClientBridge;

    /// The @c GUIActivityEventNotifier
    std::shared_ptr<gui::GUIActivityEventNotifier> m_activityEventNotifier;

    /// The @c TemplateRuntimeHandler.
    std::shared_ptr<ipc::TemplateRuntimeHandler> m_templateRuntimeIPCHandler;

    /// The AlexaCaptions IPC Handler.
    std::shared_ptr<ipc::AlexaCaptionsHandler> m_alexaCaptionIPCHandler;

    /// The map of the adapters and their mediaPlayers.
    std::unordered_map<std::string, std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>>
        m_externalMusicProviderMediaPlayersMap;

    /// The map of the adapters and their mediaPlayers.
    std::unordered_map<std::string, std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>
        m_externalMusicProviderSpeakersMap;

    /// The vector of components requiring shutdown
    std::vector<std::shared_ptr<avsCommon::utils::RequiresShutdown>> m_shutdownRequiredList;

    /// The @c MediaPlayer used by @c SpeechSynthesizer.
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_speakMediaPlayer;

    /// The Pool of @c MediaPlayers used by @c AudioPlayer (via @c PooledMediaPlayerFactory)
    std::vector<std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>> m_audioMediaPlayerPool;

    /// The @c MediaPlayer used by @c Alerts.
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_alertsMediaPlayer;

    /// The @c MediaPlayer used by @c NotificationsCapabilityAgent.
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_notificationsMediaPlayer;

    /// The @c MediaPlayer used by @c Bluetooth.
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_bluetoothMediaPlayer;

    /// The @c MediaPlayer used by @c SystemSoundPlayer.
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_systemSoundMediaPlayer;

#ifdef ENABLE_COMMS_AUDIO_PROXY
    /// The @c MediaPlayer used by @c Comms.
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_commsMediaPlayer;
#endif

    /// The @c CapabilitiesDelegate used by the client.
    std::shared_ptr<capabilitiesDelegate::CapabilitiesDelegate> m_capabilitiesDelegate;

    /// The @c MediaPlayer used by @c NotificationsCapabilityAgent.
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_ringtoneMediaPlayer;

    /// The singleton map from @c playerId to @c ChannelVolumeInterface::Type.
    static std::unordered_map<std::string, avsCommon::sdkInterfaces::ChannelVolumeInterface::Type>
        m_playerToSpeakerTypeMap;

    /// The singleton map from @c playerId to @c ExternalMediaAdapter creation functions.
    static acsdkExternalMediaPlayer::ExternalMediaPlayer::AdapterCreationMap m_adapterToCreateFuncMap;

#ifdef KWD
    /// The Wakeword Detector which can wake up the client using audio input.
    std::shared_ptr<acsdkKWDImplementations::AbstractKeywordDetector> m_keywordDetector;
#endif

    /// The @c AuthorizationManager instance that can be used to dynamically authorize with different methods.
    std::shared_ptr<acsdkAuthorization::AuthorizationManager> m_authManager;

    /// The adapter that supports authorizing with LWA based methods.
    std::shared_ptr<acsdkAuthorization::lwa::LWAAuthorizationAdapter> m_lwaAdapter;

    std::string decodeHexToAscii(const std::string hexString);
};

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_SAMPLEAPPLICATION_H_
