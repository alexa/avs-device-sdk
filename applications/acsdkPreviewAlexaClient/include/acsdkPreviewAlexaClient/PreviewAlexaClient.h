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

#ifndef ACSDKPREVIEWALEXACLIENT_PREVIEWALEXACLIENT_H_
#define ACSDKPREVIEWALEXACLIENT_PREVIEWALEXACLIENT_H_

#include <fstream>
#include <map>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <acsdkExternalMediaPlayer/ExternalMediaPlayer.h>
#include <acsdkShutdownManagerInterfaces/ShutdownManagerInterface.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/SDKInterfaces/ApplicationMediaInterfaces.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/ChannelVolumeInterface.h>
#include <AVSCommon/SDKInterfaces/Diagnostics/DiagnosticsInterface.h>
#include <SampleApp/ConsolePrinter.h>
#include <SampleApp/ConsoleReader.h>
#include <SampleApp/GuiRenderer.h>
#include <SampleApp/SampleApplicationReturnCodes.h>
#include <SampleApp/UserInputManager.h>

#ifdef AUTH_MANAGER
#include <acsdkAuthorization/AuthorizationManager.h>
#include <acsdkAuthorization/LWA/LWAAuthorizationAdapter.h>
#endif

#ifdef KWD
#include <acsdkKWDImplementations/AbstractKeywordDetector.h>
#endif

#ifdef GSTREAMER_MEDIA_PLAYER
#include <MediaPlayer/MediaPlayer.h>
#elif defined(ANDROID_MEDIA_PLAYER)
#include <AndroidSLESMediaPlayer/AndroidSLESMediaPlayer.h>
#endif

namespace alexaClientSDK {
namespace acsdkPreviewAlexaClient {

/// Key for the root node value containing configuration values for SampleApp.
static constexpr const char* SAMPLE_APP_CONFIG_KEY("sampleApp");

/**
 * Class to manage the top-level components of the AVS Client Application.
 *
 * This PreviewAlexaClient is provided as a preview of the ongoing manufactory work being integrated into the SDK.
 * It provides identical functionality to the SampleApplication, but the source code is different.
 *
 * This class and the components it uses are expected to change over the next several releases as the SDK team
 * incrementally integrates the manufactory to the SDK.
 *
 */
class PreviewAlexaClient {
public:
    /**
     * Create a PreviewAlexaClient.
     *
     * @param consoleReader The @c ConsoleReader to read inputs from console.
     * @param configFiles The vector of configuration files.
     * @param logLevel The level of logging to enable.  If this parameter is an empty string, the SDK's default
     *     logging level will be used.
     * @param diagnostics An optional @c DiagnosticsInterface object to provide diagnostics on the SDK.
     * @return A new @c PreviewAlexaClient, or @c nullptr if the operation failed.
     */
    static std::unique_ptr<PreviewAlexaClient> create(
        std::shared_ptr<alexaClientSDK::sampleApp::ConsoleReader> consoleReader,
        const std::vector<std::string>& configFiles,
        const std::string& logLevel = "",
        std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface> diagnostics = nullptr);

    /**
     * Runs the application, blocking until the user asks the application to quit or a device reset is triggered.
     *
     * @return Returns a @c SampleAppReturnCode.
     */
    sampleApp::SampleAppReturnCode run();

#ifdef DIAGNOSTICS
    /**
     * Initiates application stop for restart sequence. This method notifies event loop that the application
     * should be terminated with subsequent restart, in other words, if the application is running, it should
     * return SampleAppReturnCode::RESTART code.
     *
     * @return True if restart has been successfully initiated, false on error or if operation is not supported.
     */
    bool initiateRestart();
#endif

    /// Destructor which manages the @c SampleApplication shutdown sequence.
    ~PreviewAlexaClient();

private:
    /**
     * Initialize a PreviewAlexaClient.
     *
     * @param consoleReader The @c ConsoleReader to read inputs from console.
     * @param configFiles The vector of configuration files.
     * @param logLevel The level of logging to enable.  If this parameter is an empty string, the SDK's default
     *     logging level will be used.
     * @param diagnostics An optional @c DiagnosticsInterface object to provide diagnostics on the SDK.
     * @return @c true if initialization succeeded, else @c false.
     */
    bool initialize(
        std::shared_ptr<alexaClientSDK::sampleApp::ConsoleReader> consoleReader,
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

#ifdef ENABLE_ENDPOINT_CONTROLLERS
    /**
     * Function to add Toggle, Range and Mode handlers to the Default endpoint.
     *
     * @param defaultEndpointBuilder The reference to default endpoint's @c EndpointBuilderInterface used for adding
     * controllers.
     * @return Returns @c true if successful in adding all the three controllers otherwise @c false.
     */

    bool addControllersToDefaultEndpoint(
        std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface> defaultEndpointBuilder);

    /**
     * Function to add Power, Toggle, Range and Mode handlers to the Peripheral endpoint.
     *
     * @param peripheralEndpointBuilder The reference to peripheral endpoint's @c EndpointBuilder used for adding
     * controllers.
     * @return Returns @c true if successful in adding all the four controllers otherwise @c false.
     */
    bool addControllersToPeripheralEndpoint(
        std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface> peripheralEndpointBuilder);
#endif
    /// Object with which to trigger shutdown operations.
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface> m_shutdownManager;

    /// Object to manage lifecycle of Alexa Client SDK initialization.
    std::shared_ptr<avsCommon::avs::initialization::AlexaClientSDKInit> m_sdkInit;

    /// The @c InteractionManager which perform user requests.
    std::shared_ptr<sampleApp::InteractionManager> m_interactionManager;

    /// The @c UserInputManager which controls the client.
    std::shared_ptr<sampleApp::UserInputManager> m_userInputManager;

    /// The @c GuiRender which provides an abstraction to visual rendering
    std::shared_ptr<sampleApp::GuiRenderer> m_guiRenderer;

    /// The vector of components requiring shutdown
    std::vector<std::shared_ptr<avsCommon::utils::RequiresShutdown>> m_shutdownRequiredList;

#ifdef ENABLE_COMMS_AUDIO_PROXY
    /// The @c MediaPlayer used by @c Comms.
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_commsMediaPlayer;
#endif

#ifdef ENABLE_PCC
    /// The @c MediaPlayer used by PhoneCallController.
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_phoneMediaPlayer;
#endif

    /// The @c CapabilitiesDelegate used by the client.
    std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> m_capabilitiesDelegate;

    /// The @c MediaPlayer used by @c NotificationsCapabilityAgent.
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_ringtoneMediaPlayer;

    /// The Wakeword Detector which can wake up the client using audio input.
    std::shared_ptr<acsdkKWDImplementations::AbstractKeywordDetector> m_keywordDetector;

#if defined(ANDROID_MEDIA_PLAYER) || defined(ANDROID_MICROPHONE)
    /// The android OpenSL ES engine used to create media players and microphone.
    std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESEngine> m_openSlEngine;
#endif

#ifdef POWER_CONTROLLER
    /// The @c PeripheralEndpointPowerControllerHandler used by @c InteractionManager
    std::shared_ptr<sampleApp::PeripheralEndpointPowerControllerHandler> m_peripheralEndpointPowerHandler;
#endif

#ifdef TOGGLE_CONTROLLER
    /// The @c PeripheralEndpointToggleControllerHandler used by @c InteractionManager
    std::shared_ptr<sampleApp::PeripheralEndpointToggleControllerHandler> m_peripheralEndpointToggleHandler;
#endif

#ifdef RANGE_CONTROLLER
    /// The @c PeripheralEndpointRangeControllerHandler used by @c InteractionManager
    std::shared_ptr<sampleApp::PeripheralEndpointRangeControllerHandler> m_peripheralEndpointRangeHandler;
#endif

#ifdef MODE_CONTROLLER
    /// The @c PeripheralEndpointModeControllerHandler used by @c InteractionManager
    std::shared_ptr<sampleApp::PeripheralEndpointModeControllerHandler> m_peripheralEndpointModeHandler;
#endif

#ifdef AUTH_MANAGER
    /// The @c AuthorizationManager instance that can be used to dynamically authorize with different methods.
    std::shared_ptr<acsdkAuthorization::AuthorizationManager> m_authManager;

    /// The adapter that supports authorizing with LWA based methods.
    std::shared_ptr<acsdkAuthorization::lwa::LWAAuthorizationAdapter> m_lwaAdapter;
#endif
};

}  // namespace acsdkPreviewAlexaClient
}  // namespace alexaClientSDK

#endif  // ACSDKPREVIEWALEXACLIENT_PREVIEWALEXACLIENT_H_
