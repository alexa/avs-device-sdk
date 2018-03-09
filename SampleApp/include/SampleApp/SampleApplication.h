/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "ConsolePrinter.h"
#include "UserInputManager.h"

#ifdef KWD
#include <KWD/AbstractKeywordDetector.h>
#endif
#include <ExternalMediaPlayer/ExternalMediaPlayer.h>
#include <MediaPlayer/MediaPlayer.h>

namespace alexaClientSDK {
namespace sampleApp {

/// Class to manage the top-level components of the AVS Client Application
class SampleApplication {
public:
    /**
     * Create a SampleApplication.
     *
     * @param pathToConfig The path to the SDK configuration file.
     * @param pathToInputFolder The path to the inputs folder containing data files needed by this application.
     * @param logLevel The level of logging to enable.  If this parameter is an empty string, the SDK's default
     *     logging level will be used.
     * @return A new @c SampleApplication, or @c nullptr if the operation failed.
     */
    static std::unique_ptr<SampleApplication> create(
        const std::string& pathToConfig,
        const std::string& pathToInputFolder,
        const std::string& logLevel = "");

    /// Runs the application, blocking until the user asks the application to quit.
    void run();

    /// Destructor which manages the @c SampleApplication shutdown sequence.
    ~SampleApplication();

    /**
     * Method to create mediaPlayers for the optional music provider adapters plugged into the SDK.
     *
     * @param httpContentFetcherFactory The HTTPContentFetcherFactory to be used while creating the mediaPlayers.
     * @param additionalSpeakers The speakerInterface to add the created mediaPlayer.
     * @return @c true if the mediaPlayer of all the registered adapters could be created @c false otherwise.
     */
    bool createMediaPlayersForAdapters(
        std::shared_ptr<avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory> httpContentFetcherFactory,
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
    using MediaPlayerCreateFunction = std::shared_ptr<mediaPlayer::MediaPlayer> (*)(
        std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory,
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
     * @param pathToConfig The path to the SDK configuration file.
     * @param pathToInputFolder The path to the inputs folder containing data files needed by this application.
     * @param logLevel The level of logging to enable.  If this parameter is an empty string, the SDK's default
     *     logging level will be used.
     * @return @c true if initialization succeeded, else @c false.
     */
    bool initialize(const std::string& pathToConfig, const std::string& pathToInputFolder, const std::string& logLevel);

    /// The @c UserInputManager which controls the client.
    std::unique_ptr<UserInputManager> m_userInputManager;

    /// The map of the adapters and their mediaPlayers.
    std::unordered_map<std::string, std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>>
        m_externalMusicProviderMediaPlayersMap;

    /// The vector of mediaPlayers for the adapters.
    std::vector<std::shared_ptr<mediaPlayer::MediaPlayer>> m_adapterMediaPlayers;

    /// The @c MediaPlayer used by @c SpeechSynthesizer.
    std::shared_ptr<mediaPlayer::MediaPlayer> m_speakMediaPlayer;

    /// The @c MediaPlayer used by @c AudioPlayer.
    std::shared_ptr<mediaPlayer::MediaPlayer> m_audioMediaPlayer;

    /// The @c MediaPlayer used by @c Alerts.
    std::shared_ptr<mediaPlayer::MediaPlayer> m_alertsMediaPlayer;

    /// The @c MediaPlayer used by @c NotificationsCapabilityAgent.
    std::shared_ptr<mediaPlayer::MediaPlayer> m_notificationsMediaPlayer;

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
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_SAMPLEAPPLICATION_H_
