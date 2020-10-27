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

#ifndef ACSDKEXTERNALMEDIAPLAYER_EXTERNALMEDIAPLAYER_H_
#define ACSDKEXTERNALMEDIAPLAYER_EXTERNALMEDIAPLAYER_H_

#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/DirectiveHandlerConfiguration.h>
#include <AVSCommon/AVS/NamespaceAndName.h>
#include <AVSCommon/SDKInterfaces/AudioFocusAnnotation.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/SDKInterfaces/LocalPlaybackHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/PlaybackHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/PlaybackRouterInterface.h>
#include <AVSCommon/SDKInterfaces/RenderPlayerInfoCardsProviderInterface.h>
#include <AVSCommon/SDKInterfaces/RenderPlayerInfoCardsProviderRegistrarInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/DefaultEndpointAnnotation.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointBuilderInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <CertifiedSender/CertifiedSender.h>
#include <acsdkApplicationAudioPipelineFactoryInterfaces/ApplicationAudioPipelineFactoryInterface.h>
#include <acsdkExternalMediaPlayerInterfaces/ExternalMediaAdapterInterface.h>
#include <acsdkExternalMediaPlayerInterfaces/ExternalMediaAdapterHandlerInterface.h>
#include <acsdkExternalMediaPlayerInterfaces/ExternalMediaPlayerInterface.h>
#include <acsdkExternalMediaPlayerInterfaces/ExternalMediaPlayerObserverInterface.h>
#include <acsdkManufactory/Annotated.h>
#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>
#include <acsdkStartupManagerInterfaces/RequiresStartupInterface.h>
#include <acsdkStartupManagerInterfaces/StartupNotifierInterface.h>

#include "acsdkExternalMediaPlayer/AuthorizedSender.h"
#include "acsdkExternalMediaPlayer/StaticExternalMediaPlayerAdapterHandler.h"

namespace alexaClientSDK {
namespace acsdkExternalMediaPlayer {

/**
 * This class implements the @c ExternalMediaPlayer capability agent. This agent
 * is responsible for handling music service providers which manage their PLAY
 * queue.
 *
 * @note For instances of this class to be cleaned up correctly, @c shutdown()
 * must be called.
 */
class ExternalMediaPlayer
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::utils::RequiresShutdown
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerInterface
        , public avsCommon::sdkInterfaces::MediaPropertiesInterface
        , public avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderInterface
        , public avsCommon::sdkInterfaces::PlaybackHandlerInterface
        , public avsCommon::sdkInterfaces::LocalPlaybackHandlerInterface
        , public acsdkStartupManagerInterfaces::RequiresStartupInterface
        , public std::enable_shared_from_this<ExternalMediaPlayer> {
public:
    // Map of adapter business names to their mediaPlayers.
    using AdapterMediaPlayerMap =
        std::unordered_map<std::string, std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>>;

    // Map of adapter business names to their speakers.
    using AdapterSpeakerMap =
        std::unordered_map<std::string, std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>>;

    // Signature of functions to create an ExternalMediaAdapter.
    using AdapterCreateFunction =
        std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaAdapterInterface> (*)(
            std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface>,
            std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer,
            std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface> speaker,
            std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager,
            std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
            std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
            std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
            std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerInterface> externalMediaPlayer);

    // Map of adapter business names to their creation method.
    using AdapterCreationMap = std::unordered_map<std::string, AdapterCreateFunction>;

    /// The spiVersion of this implementation of ExternalMediaPlayer.
    static constexpr const char* SPI_VERSION = "1.0";

    /**
     * Forwards an @c ExternalMediaPlayer as an @c ExternalMediaPlayerInterface.
     *
     * @param externalMediaPlayer The object to forward.
     * @return A shared_ptr to @c ExternalMediaPlayerInterface.
     */
    static std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerInterface>
    createExternalMediaPlayerInterface(std::shared_ptr<ExternalMediaPlayer> externalMediaPlayer);

    /**
     * Factory method to create a new @c ExternalMediaPlayer.
     *
     * @param messageSender The object to use for sending events.
     * @param certifiedMessageSender Used to send messages that must be guaranteed.
     * @param contextManager The AVS Context Manager used to generate system context for events.
     * @param exceptionSender The object to use for sending AVS Exception messages.
     * @param playbackRouter The @c PlaybackRouterInterface instance to use when the EMP becomes active.
     * @param endpointCapabilitiesRegistrar The @c EndpointCapabilitiesRegistratInterface to use to register this
     * capability with the default endpoint.
     * @param shutdownNotifier The object that will notify this EMP should shut down.
     * @param startupNotifier The object that will notify this EMP to startup.
     * @param renderPlayerInfoCardsProviderRegistrar The registrar for this PlayerInfoCardsProvider.
     * @param metricRecorder The object to record metrics.
     * @return A shared pointer to an @c ExternalMediaPlayer.
     */
    static std::shared_ptr<ExternalMediaPlayer> createExternalMediaPlayer(
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<certifiedSender::CertifiedSender> certifiedMessageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> playbackRouter,
        acsdkManufactory::Annotated<
            avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
            avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface> endpointCapabilitiesRegistrar,
        std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface> shutdownNotifier,
        std::shared_ptr<acsdkStartupManagerInterfaces::StartupNotifierInterface> startupNotifier,
        std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderRegistrarInterface>
            renderPlayerInfoCardsProviderRegistrar,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder);

    /**
     * Factory method to create a new @c ExternalMediaPlayer.
     *
     * @deprecated This is for backwards compatibility only; prefer createExternalMediaPlayer which does not create
     * adapters as part of EMP initialization, and instead adapters should register themselves with EMP.
     *
     * @param adapterCreationMap A map of <playerId: AdapterCreateFunction> that this EMP will use to instantiate
     * adapters.
     * @param audioPipelineFactory The object to use for creating the media players and related interfaces for
     * the adapters.
     * @param messageSender The object to use for sending events.
     * @param certifiedMessageSender Used to send messages that must be guaranteed.
     * @param audioFocusManager The object used to manage audio focus for the adapter managed by the EMP.
     * @param contextManager The AVS Context Manager used to generate system context for events.
     * @param exceptionSender The object to use for sending AVS Exception messages.
     * @param playbackRouter The @c PlaybackRouterInterface instance to use when the EMP becomes active.
     * @param endpointCapabilitiesRegistrar The @c EndpointCapabilitiesRegistratInterface to use to register this
     * capability with the default endpoint.
     * @param shutdownNotifier The object that will notify this EMP should shut down.
     * @param startupNotifier The object that will notify this EMP to startup.
     * @param renderPlayerInfoCardsProviderRegistrar The registrar for this PlayerInfoCardsProvider.
     * @param metricRecorder The object to record metrics.
     * @return A shared pointer to an @c ExternalMediaPlayer.
     */
    static std::shared_ptr<ExternalMediaPlayer> createExternalMediaPlayerWithAdapters(
        const acsdkExternalMediaPlayer::ExternalMediaPlayer::AdapterCreationMap& adapterCreationMap,
        std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>
            audioPipelineFactory,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<certifiedSender::CertifiedSender> certifiedMessageSender,
        acsdkManufactory::Annotated<
            avsCommon::sdkInterfaces::AudioFocusAnnotation,
            avsCommon::sdkInterfaces::FocusManagerInterface> audioFocusManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> playbackRouter,
        acsdkManufactory::Annotated<
            avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
            avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface> endpointCapabilitiesRegistrar,
        std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface> shutdownNotifier,
        std::shared_ptr<acsdkStartupManagerInterfaces::StartupNotifierInterface> startupNotifier,
        std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderRegistrarInterface>
            renderPlayerInfoCardsProviderRegistrar,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager);

    /// @name ContextRequesterInterface Functions
    /// @{
    void onContextAvailable(const std::string& jsonContext) override;
    void onContextFailure(const avsCommon::sdkInterfaces::ContextRequestError error) override;
    /// @}

    /// @name StateProviderInterface Functions
    /// @{
    void provideState(const avsCommon::avs::NamespaceAndName& stateProviderName, unsigned int stateRequestToken)
        override;
    /// @}

    /// @name CapabilityAgent/DirectiveHandlerInterface Functions
    /// @{
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<DirectiveInfo> info) override;
    void onDeregistered() override;
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    /// @}

    /// @name Overridden PlaybackHandlerInterface methods.
    /// @{
    virtual void onButtonPressed(avsCommon::avs::PlaybackButton button) override;

    virtual void onTogglePressed(avsCommon::avs::PlaybackToggle toggle, bool action) override;
    /// @}

    /// @name Overridden ExternalMediaPlayerInterface methods.
    /// @{
    virtual void setPlayerInFocus(const std::string& playerInFocus) override;
    virtual void updateDiscoveredPlayers(
        const std::vector<acsdkExternalMediaPlayerInterfaces::DiscoveredPlayerInfo>& addedPlayers,
        const std::unordered_set<std::string>& removedLocalPlayerIds) override;
    virtual void addAdapterHandler(
        std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaAdapterHandlerInterface> adapterHandler)
        override;
    virtual void removeAdapterHandler(
        std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaAdapterHandlerInterface> adapterHandler)
        override;
    virtual void addObserver(
        const std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerObserverInterface> observer)
        override;
    virtual void removeObserver(
        const std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerObserverInterface> observer)
        override;
    /// @}

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;
    /// @}

    /// @name RenderPlayerInfoCardsProviderInterface Functions
    /// @{
    void setObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsObserverInterface> observer) override;
    /// @}

    /// @name LocalPlaybackHandlerInterface Functions
    /// @{
    bool localOperation(PlaybackOperation op) override;
    bool localSeekTo(std::chrono::milliseconds location, bool fromStart) override;
    /// @}

    /// @name MediaPropertiesInterface Functions
    /// @{
    std::chrono::milliseconds getAudioItemOffset() override;
    std::chrono::milliseconds getAudioItemDuration() override;
    /// @}

    /**
     * Getter for statically configured adapters - note this only returns adapters provided through the
     * adapterCreationMap during @c init
     *
     * @return The Map of @c localPlayerId (business names) to adapters.
     */
    std::map<std::string, std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaAdapterInterface>>
    getAdaptersMap();

    /// @name RequiresStartupInterface methods
    /// @{
    bool startup() override;
    /// @}

private:
    /**
     * A struct containing the localPlayerId and corresponding handler
     */
    struct LocalPlayerIdHandler {
        std::string localPlayerId;
        std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaAdapterHandlerInterface> adapterHandler;
    };

    /**
     * Creates an @c ExternalMediaPlayer. This method contains the shared logic between the new and deprecated public
     * code paths for creating an @c ExternalMediaPlayer.
     *
     * @param messageSender The object to use for sending events.
     * @param certifiedMessageSender Used to send messages that must be guaranteed.
     * @param contextManager The AVS Context Manager used to generate system context for events.
     * @param exceptionSender The object to use for sending AVS Exception messages.
     * @param playbackRouter The @c PlaybackRouterInterface instance to use when the EMP becomes active.
     * @param endpointCapabilitiesRegistrar The @c EndpointCapabilitiesRegistratInterface to use to register this
     * capability with the default endpoint.
     * @param shutdownNotifier The object that will notify this EMP should shut down.
     * @param startupNotifier The object that will notify this EMP to startup.
     * @param renderPlayerInfoCardsProviderRegistrar The registrar for this PlayerInfoCardsProvider.
     * @param metricRecorder The object to record metrics.
     * @return A shared pointer to an @c ExternalMediaPlayer.
     */
    static std::shared_ptr<ExternalMediaPlayer> create(
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<certifiedSender::CertifiedSender> certifiedMessageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> playbackRouter,
        acsdkManufactory::Annotated<
            avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
            avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface> endpointCapabilitiesRegistrar,
        std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface> shutdownNotifier,
        std::shared_ptr<acsdkStartupManagerInterfaces::StartupNotifierInterface> startupNotifier,
        std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderRegistrarInterface>
            renderPlayerInfoCardsProviderRegistrar,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder);

    /**
     * Constructor.
     *
     * @param messageSender The messager sender of the adapter.
     * @param certifiedMessageSender Used to send messages that must be
     * guaranteed.
     * @param contextManager The AVS Context manager used to generate system
     * context for events.
     * @param exceptionSender The object to use for sending AVS Exception
     * messages.
     * @param playbackRouter The @c PlaybackRouterInterface instance to use when
     * @c ExternalMediaPlayer becomes active.
     * @param metricRecorder The metric recorder.
     * @return A @c std::shared_ptr to the new @c ExternalMediaPlayer instance.
     */
    ExternalMediaPlayer(
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<certifiedSender::CertifiedSender> certifiedMessageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> playbackRouter,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder);

    /**
     * Initialize the ExternalMediaPlayer.
     *
     * @return true if successful, otherwise false.
     */
    bool init();

    /**
     * Create the adapters to be managed by ExternalMediaPlayer.
     *
     * @deprecated Adapters should be created in components and register their own handlers with EMP CA, rather than
     * rely on EMP to create the adapters.
     *
     * @param audioPipelineFactory The factory to create media players and related interfaces for the adapters.
     * @param focusManager The audio focus manager.
     * @param speakerManager The speaker manager to control volume.
     */
    void createAdapters(
        const AdapterCreationMap& adapterCreationMap,
        std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>
            audioPipelineFactory,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager);

    /**
     * This method returns the ExternalMediaPlayer session state registered in the ExternalMediaPlayer namespace.
     *
     * @param adapterStates The list of adapter states to use when generating the session state
     * @return The session state
     */
    std::string provideSessionState(std::vector<acsdkExternalMediaPlayerInterfaces::AdapterState> adapterStates);

    /**
     * This method returns the Playback state registered in the Alexa.PlaybackStateReporter state.
     *
     * @param adapterStates The list of adapter states to use when generating the playback state
     * @return The playback state
     */
    std::string providePlaybackState(std::vector<acsdkExternalMediaPlayerInterfaces::AdapterState> adapterStates);

    /**
     * This function deserializes a @c Directive's payload into a @c
     * rapidjson::Document.
     *
     * @param info The @c DirectiveInfo to read the payload string from.
     * @param[out] document The @c rapidjson::Document to parse the payload into.
     * @return @c true if parsing was successful, else @c false.
     */
    bool parseDirectivePayload(std::shared_ptr<DirectiveInfo> info, rapidjson::Document* document);

    /**
     * Remove a directive from the map of message IDs to DirectiveInfo instances.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective whose
     * message ID is to be removed.
     */
    void removeDirective(std::shared_ptr<DirectiveInfo> info);

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /**
     * Sends an event which reveals the given discovered players.
     *
     * @param discoveredPlayers The list of discovered players
     */
    void sendReportDiscoveredPlayersEvent(
        const std::vector<acsdkExternalMediaPlayerInterfaces::DiscoveredPlayerInfo>& discoveredPlayers);

    /**
     * Sends an event indicating that the authorization workflow has completed.
     *
     * @param authorized A map of playerId to skillToken for authorized adapters.
     * The attributes should be the one in the corresponding @c AuthorizedPlayers
     * directive
     * @param deauthorized A set of deauthorized localPlayerId.
     */
    void sendAuthorizationCompleteEvent(
        const std::unordered_map<std::string, std::string>& authorized,
        const std::unordered_set<std::string>& deauthorized);

    /**
     * Send the handling completed notification and clean up the resources the
     * specified @c DirectiveInfo.
     *
     * @param info The @c DirectiveInfo to complete and clean up.
     */
    void setHandlingCompleted(std::shared_ptr<DirectiveInfo> info);

    /**
     * Send ExceptionEncountered and report a failure to handle the @c
     * AVSDirective.
     *
     * @param info The @c AVSDirective that encountered the error and ancillary
     * information.
     * @param type The type of Exception that was encountered.
     * @param message The error message to include in the ExceptionEncountered
     * message.
     */
    void sendExceptionEncounteredAndReportFailed(
        std::shared_ptr<DirectiveInfo> info,
        const std::string& message,
        avsCommon::avs::ExceptionErrorType type = avsCommon::avs::ExceptionErrorType::INTERNAL_ERROR);

    /**
     * @name Executor Thread Functions
     *
     * These functions (and only these functions) are called by @c m_executor on a
     * single worker thread.  All other functions in this class can be called
     * asynchronously, and pass data to the @c Executor thread through parameters
     * to lambda functions.  No additional synchronization is needed.
     */
    /// @{
    /**
     * This function provides updated context information for @c
     * ExternalMediaPlayer to @c ContextManager.  This function is called when @c
     * ContextManager calls @c provideState(), and is also called internally by @c
     * changeActivity().
     *
     * @param stateProviderName The name of the stateProvider.
     * @param sendToken flag indicating whether @c stateRequestToken contains a
     * valid token which should be passed along to @c ContextManager.  This flag
     * defaults to @c false.
     * @param stateRequestToken The token @c ContextManager passed to the @c
     * provideState() call, which will be passed along to the
     * ContextManager::setState() call.  This parameter is not used if @c
     * sendToken is @c false.
     */
    void executeProvideState(
        const avsCommon::avs::NamespaceAndName& stateProviderName,
        bool sendToken = false,
        unsigned int stateRequestToken = 0);
    /// @}

    /**
     * Method that checks the preconditions for all directives.
     *
     * @param info The DirectiveInfo to be preprocessed
     * @param document The rapidjson document resulting from parsing the directive in directiveInfo.
     * @return if the directive was pre-processed successfully
     */
    bool preprocessDirective(std::shared_ptr<DirectiveInfo> info, rapidjson::Document* document);

    /**
     * Helper method to get the local player id from playerId.
     *
     * @param playerId The cloud assigned playerId.
     *
     * @return The local player id and handler, if one exists
     */
    avsCommon::utils::Optional<LocalPlayerIdHandler> getHandlerFromPlayerId(const std::string& playerId);

    /**
     * Handler for AuthorizeDiscoveredPlayers directive.
     *
     * @param info The DirectiveInfo to be processed.
     * @param The type of the request. Will be NONE for the
     *        handleAuthorizeDiscoveredPlayers case.
     */
    void handleAuthorizeDiscoveredPlayers(
        std::shared_ptr<DirectiveInfo> info,
        acsdkExternalMediaPlayerInterfaces::RequestType request);

    /**
     * Handler for login directive.
     *
     * @param info The DirectiveInfo to be processed.
     * @param The type of the request. Will be LOGIN for the handleLogin case.
     */
    void handleLogin(std::shared_ptr<DirectiveInfo> info, acsdkExternalMediaPlayerInterfaces::RequestType request);

    /**
     * Handler for logout directive.
     *
     * @param info The DirectiveInfo to be processed.
     * @param The type of the request. Will be LOGOUT for the handleLogout case.
     */
    void handleLogout(std::shared_ptr<DirectiveInfo> info, acsdkExternalMediaPlayerInterfaces::RequestType request);

    /**
     * Handler for play directive.
     *
     * @param info The DirectiveInfo to be processed.
     * @param The type of the request. Will be PLAY for the handlePlay case.
     */
    void handlePlay(std::shared_ptr<DirectiveInfo> info, acsdkExternalMediaPlayerInterfaces::RequestType request);

    /**
     * Handler for play control directive.
     *
     * @param info The DirectiveInfo to be processed.
     * @param The type of the request. Can be NEXT/PREVIOUS/PAUSE/RESUME... for
     * the handlePlayControl case.
     */
    void handlePlayControl(
        std::shared_ptr<DirectiveInfo> info,
        acsdkExternalMediaPlayerInterfaces::RequestType request);

    /**
     * Handler for SetSeekControl  directive.
     *
     * @param info The DirectiveInfo to be processed.
     * @param The type of the request. RequestType will be SEEK.
     */
    void handleSeek(std::shared_ptr<DirectiveInfo> info, acsdkExternalMediaPlayerInterfaces::RequestType request);

    /**
     * Handler for AdjustSeekControl  directive.
     *
     * @param info The DirectiveInfo to be processed.
     * @param The type of the request. RequestType will be ADJUST_SEEK.
     */
    void handleAdjustSeek(std::shared_ptr<DirectiveInfo> info, acsdkExternalMediaPlayerInterfaces::RequestType request);

    /**
     * Calls each observer and provides the ObservableSessionProperties for this
     * adapter
     *
     * @param playerId the ExternalMediaAdapter being reported on
     * @param sessionProperties  the observable session properties being reported
     */
    void notifyObservers(
        const std::string& playerId,
        const acsdkExternalMediaPlayerInterfaces::ObservableSessionProperties* sessionProperties);

    /**
     * Calls each observer and provides the ObservablePlaybackStateProperties for
     * this adapter
     *
     * @param playerId the ExternalMediaAdapter being reported on
     * @param playbackProperties  the observable playback state properties being
     * reported
     */
    void notifyObservers(
        const std::string& playerId,
        const acsdkExternalMediaPlayerInterfaces::ObservablePlaybackStateProperties* playbackProperties);

    /**
     * Calls each observer and provides the supplied ObservableProperties for this
     * adapter
     *
     * @param adapter the ExternalMediaAdapter being reported on
     * @param sessionProperties  the observable session properties being reported
     * @param playbackProperties  the observable playback state properties being
     * reported
     */
    void notifyObservers(
        const std::string& playerId,
        const acsdkExternalMediaPlayerInterfaces::ObservableSessionProperties* sessionProperties,
        const acsdkExternalMediaPlayerInterfaces::ObservablePlaybackStateProperties* playbackProperties);

    /**
     * Calls observer and provides the supplied changes related to
     * RenderPlayerInfoCards for the active adapter.
     */
    void notifyRenderPlayerInfoCardsObservers();

    /// The EMP Agent String, for server identification
    std::string m_agentString;

    /// The metric recorder.
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> m_metricRecorder;

    /// The @c MessageSenderInterface for sending events.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /// The @c CertifiedSender for guaranteeing sending of events.
    std::shared_ptr<certifiedSender::CertifiedSender> m_certifiedMessageSender;

    /// The @c ContextManager that needs to be updated of the state.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c PlaybackRouterInterface instance to use when @c ExternalMediaPlayer
    /// becomes active.
    std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> m_playbackRouter;

    /// Protects access to @c m_authorizedAdapters.
    std::mutex m_authorizedMutex;

    /// A map of cloud assigned @c playerId to local player id/handler. Unauthorized adapters will not be in this map.
    std::unordered_map<std::string, LocalPlayerIdHandler> m_authorizedAdapters;

    /// The @c m_adapters Map of @c localPlayerId (business names) to adapters.
    std::map<std::string, std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaAdapterInterface>>
        m_staticAdapters;

    /// The id of the player which currently has focus.  Access to @c m_playerInFocus is protected by @c
    /// m_inFocusAdapterMutex.
    std::string m_playerInFocus;

    /// Mutex to serialize access to the @c m_playerInFocus.
    std::mutex m_inFocusAdapterMutex;

    /// The @c AuthorizedSender that will only allow authorized players to send events.
    std::shared_ptr<AuthorizedSender> m_authorizedSender;

    /// Mutex to serialize access to the observers.
    std::mutex m_observersMutex;

    /// The set of observers watching session and playback state
    std::unordered_set<std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerObserverInterface>>
        m_observers;

    /// Observer for changes related to RenderPlayerInfoCards.
    std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsObserverInterface> m_renderPlayerObserver;

    /// An event queue used to store events which need to be sent to the cloud.
    /// The pair is <eventName, eventPayload>.
    std::queue<std::pair<std::string, std::string>> m_eventQueue;

    /// typedef of the function pointer to handle AVS directives.
    typedef void (ExternalMediaPlayer::*DirectiveHandler)(
        std::shared_ptr<DirectiveInfo> info,
        acsdkExternalMediaPlayerInterfaces::RequestType request);

    /// The singleton map from a directive to its handler.
    static std::unordered_map<
        avsCommon::avs::NamespaceAndName,
        std::pair<acsdkExternalMediaPlayerInterfaces::RequestType, ExternalMediaPlayer::DirectiveHandler>>
        m_directiveToHandlerMap;

    /// Set of capability configurations that will get published using the Capabilities API
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> m_capabilityConfigurations;

    /// The set of @name ExternalMediaAdapterHandler objects
    std::unordered_set<std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaAdapterHandlerInterface>>
        m_adapterHandlers;

    /// The set of discovered local player IDs which have already been reported to AVS
    std::unordered_set<std::string> m_reportedDiscoveredPlayers;

    /// A map of local player IDs to their DiscoveredPlayerInfo which have not been reported to AVS yet, as startup has
    /// not been called on this object yet. This allows the EMP to accumulate adapters before startup.
    std::unordered_map<std::string, acsdkExternalMediaPlayerInterfaces::DiscoveredPlayerInfo>
        m_unreportedPlayersToReportAtStartup;

    /// Mutex to serialize access to the @c m_onStartupHasBeenCalled.
    std::mutex m_onStartupHasBeenCalledMutex;

    /// Whether startup has been called on this object.
    bool m_onStartupHasBeenCalled;

    /// The @c FocusManager used to manage usage of the channel.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::FocusManagerInterface> m_focusManager;

    /**
     * @c Executor which queues up operations from asynchronous API calls.
     *
     * @note This declaration needs to come *after* the Executor Thread Variables above so that the thread shuts down
     *     before the Executor Thread Variables are destroyed.
     */
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace acsdkExternalMediaPlayer
}  // namespace alexaClientSDK

#endif  // ACSDKEXTERNALMEDIAPLAYER_EXTERNALMEDIAPLAYER_H_
