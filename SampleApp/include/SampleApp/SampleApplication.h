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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_SAMPLEAPPLICATION_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_SAMPLEAPPLICATION_H_

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ConsolePrinter.h"
#include "ConsoleReader.h"
#include "DefaultClient/EqualizerRuntimeSetup.h"
#include "SampleApp/GuiRenderer.h"
#include "SampleApplicationReturnCodes.h"
#include "UserInputManager.h"

#ifdef KWD
#include <KWD/AbstractKeywordDetector.h>
#endif

#ifdef GSTREAMER_MEDIA_PLAYER
#include <MediaPlayer/MediaPlayer.h>
#elif defined(ANDROID_MEDIA_PLAYER)
#include <AndroidSLESMediaPlayer/AndroidSLESMediaPlayer.h>
#endif

#ifdef BLUETOOTH_BLUEZ_PULSEAUDIO_OVERRIDE_ENDPOINTS
#include <BlueZ/PulseAudioBluetoothInitializer.h>
#endif

#include <CapabilitiesDelegate/CapabilitiesDelegate.h>
#include <ExternalMediaPlayer/ExternalMediaPlayer.h>

namespace alexaClientSDK {
namespace sampleApp {

#ifdef GSTREAMER_MEDIA_PLAYER
using ApplicationMediaPlayer = mediaPlayer::MediaPlayer;
#elif defined(ANDROID_MEDIA_PLAYER)
using ApplicationMediaPlayer = mediaPlayer::android::AndroidSLESMediaPlayer;
#endif

/// Class to manage the top-level components of the AVS Client Application
class SampleApplication {
public:
    /**
     * Create a SampleApplication.
     *
     * @param consoleReader The @c ConsoleReader to read inputs from console.
     * @param configFiles The vector of configuration files.
     * @param pathToInputFolder The path to the inputs folder containing data files needed by this application.
     * @param logLevel The level of logging to enable.  If this parameter is an empty string, the SDK's default
     *     logging level will be used.
     * @return A new @c SampleApplication, or @c nullptr if the operation failed.
     */
    static std::unique_ptr<SampleApplication> create(
        std::shared_ptr<alexaClientSDK::sampleApp::ConsoleReader> consoleReader,
        const std::vector<std::string>& configFiles,
        const std::string& pathToInputFolder,
        const std::string& logLevel = "");

    /**
     * Runs the application, blocking until the user asks the application to quit or a device reset is triggered.
     *
     * @return Returns a @c SampleAppReturnCode.
     */
    SampleAppReturnCode run();

    /// Destructor which manages the @c SampleApplication shutdown sequence.
    ~SampleApplication();

    /**
     * Method to create mediaPlayers for the optional music provider adapters plugged into the SDK.
     *
     * @param httpContentFetcherFactory The HTTPContentFetcherFactory to be used while creating the mediaPlayers.
     * @param equalizerRuntimeSetup Equalizer runtime setup to register equalizers
     * @param additionalSpeakers The speakerInterface to add the created mediaPlayer.
     * @return @c true if the mediaPlayer of all the registered adapters could be created @c false otherwise.
     */
    bool createMediaPlayersForAdapters(
        std::shared_ptr<avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory> httpContentFetcherFactory,
        std::shared_ptr<defaultClient::EqualizerRuntimeSetup> equalizerRuntimeSetup,
        std::vector<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>& additionalSpeakers);

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
            capabilityAgents::externalMediaPlayer::ExternalMediaPlayer::AdapterCreateFunction createFunction);
    };

    /**
     * Signature of functions to create a MediaPlayer.
     *
     * @param httpContentFetcherFactory The HTTPContentFetcherFactory to be used while creating the mediaPlayers.
     * @param type The type of the SpeakerInterface.
     * @param name The name of the MediaPlayer instance.
     * @return Return shared pointer to the created MediaPlayer instance.
     */
    using MediaPlayerCreateFunction = std::shared_ptr<ApplicationMediaPlayer> (*)(
        std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory,
        bool enableEqualizer,
        avsCommon::sdkInterfaces::SpeakerInterface::Type type,
        std::string name);

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
         * @speakerType The SpeakerType of the mediaPlayer to be created.
         * @param createFunction The function to use to create instances of the mediaPlayer to use for the player.
         */
        MediaPlayerRegistration(
            const std::string& playerId,
            avsCommon::sdkInterfaces::SpeakerInterface::Type speakerType,
            MediaPlayerCreateFunction createFunction);
    };

private:
    /**
     * Initialize a SampleApplication.
     *
     * @param consoleReader The @c ConsoleReader to read inputs from console.
     * @param configFiles The vector of configuration files.
     * @param pathToInputFolder The path to the inputs folder containing data files needed by this application.
     * @param logLevel The level of logging to enable.  If this parameter is an empty string, the SDK's default
     *     logging level will be used.
     * @return @c true if initialization succeeded, else @c false.
     */
    bool initialize(
        std::shared_ptr<alexaClientSDK::sampleApp::ConsoleReader> consoleReader,
        const std::vector<std::string>& configFiles,
        const std::string& pathToInputFolder,
        const std::string& logLevel);

    /**
     * Create an application media player.
     *
     * @param contentFetcherFactory Used to create objects that can fetch remote HTTP content.
     * @param enableEqualizer Flag indicating if equalizer should be enabled for this media player.
     * @param type The type used to categorize the speaker for volume control.
     * @param name The media player instance name used for logging purpose.
     * @return A pointer to the @c ApplicationMediaPlayer and to its speaker if it succeeds; otherwise, return @c
     * nullptr.
     */
    std::pair<std::shared_ptr<ApplicationMediaPlayer>, std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>
    createApplicationMediaPlayer(
        std::shared_ptr<avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory> httpContentFetcherFactory,
        bool enableEqualizer,
        avsCommon::sdkInterfaces::SpeakerInterface::Type type,
        const std::string& name);

    /// The @c InteractionManager which perform user requests.
    std::shared_ptr<InteractionManager> m_interactionManager;

    /// The @c UserInputManager which controls the client.
    std::shared_ptr<UserInputManager> m_userInputManager;

    /// The @c GuiRender which provides an abstraction to visual rendering
    std::shared_ptr<GuiRenderer> m_guiRenderer;

    /// The map of the adapters and their mediaPlayers.
    std::unordered_map<std::string, std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>>
        m_externalMusicProviderMediaPlayersMap;

    /// The map of the adapters and their mediaPlayers.
    std::unordered_map<std::string, std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>
        m_externalMusicProviderSpeakersMap;

    /// The vector of mediaPlayers for the adapters.
    std::vector<std::shared_ptr<ApplicationMediaPlayer>> m_adapterMediaPlayers;

    /// The @c MediaPlayer used by @c SpeechSynthesizer.
    std::shared_ptr<ApplicationMediaPlayer> m_speakMediaPlayer;

    /// The @c MediaPlayer used by @c AudioPlayer.
    std::shared_ptr<ApplicationMediaPlayer> m_audioMediaPlayer;

    /// The @c MediaPlayer used by @c Alerts.
    std::shared_ptr<ApplicationMediaPlayer> m_alertsMediaPlayer;

    /// The @c MediaPlayer used by @c NotificationsCapabilityAgent.
    std::shared_ptr<ApplicationMediaPlayer> m_notificationsMediaPlayer;

    /// The @c MediaPlayer used by @c Bluetooth.
    std::shared_ptr<ApplicationMediaPlayer> m_bluetoothMediaPlayer;

    /// The @c CapabilitiesDelegate used by the client.
    std::shared_ptr<alexaClientSDK::capabilitiesDelegate::CapabilitiesDelegate> m_capabilitiesDelegate;

    /// The @c MediaPlayer used by @c NotificationsCapabilityAgent.
    std::shared_ptr<ApplicationMediaPlayer> m_ringtoneMediaPlayer;

    using SpeakerTypeAndCreateFunc =
        std::pair<avsCommon::sdkInterfaces::SpeakerInterface::Type, MediaPlayerCreateFunction>;

    /// The singleton map from @c playerId to @c MediaPlayerCreateFunction.
    static std::unordered_map<std::string, SpeakerTypeAndCreateFunc> m_playerToMediaPlayerMap;

    /// The singleton map from @c playerId to @c ExternalMediaAdapter creation functions.
    static capabilityAgents::externalMediaPlayer::ExternalMediaPlayer::AdapterCreationMap m_adapterToCreateFuncMap;

#ifdef KWD
    /// The Wakeword Detector which can wake up the client using audio input.
    std::unique_ptr<kwd::AbstractKeywordDetector> m_keywordDetector;
#endif

#if defined(ANDROID_MEDIA_PLAYER) || defined(ANDROID_MICROPHONE)
    /// The android OpenSL ES engine used to create media players and microphone.
    std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESEngine> m_openSlEngine;
#endif

#ifdef BLUETOOTH_BLUEZ_PULSEAUDIO_OVERRIDE_ENDPOINTS
    /// Iniitalizer object to reload PulseAudio Bluetooth modules.
    std::shared_ptr<bluetoothImplementations::blueZ::PulseAudioBluetoothInitializer> m_pulseAudioInitializer;
#endif
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_SAMPLEAPPLICATION_H_
