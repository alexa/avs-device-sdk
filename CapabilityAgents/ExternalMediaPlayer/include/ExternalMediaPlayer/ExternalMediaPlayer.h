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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_EXTERNALMEDIAPLAYER_INCLUDE_EXTERNALMEDIAPLAYER_EXTERNALMEDIAPLAYER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_EXTERNALMEDIAPLAYER_INCLUDE_EXTERNALMEDIAPLAYER_EXTERNALMEDIAPLAYER_H_

#include <map>
#include <memory>
#include <string>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/DirectiveHandlerConfiguration.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ExternalMediaAdapterInterface.h>
#include <AVSCommon/SDKInterfaces/ExternalMediaPlayerInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/PlaybackHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/PlaybackRouterInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerInterface.h>
#include <AVSCommon/AVS/NamespaceAndName.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace externalMediaPlayer {

/**
 * This class implements the @c ExternalMediaPlayer capability agent. This agent is responsible for handling
 * music service providers which manage their PLAY queue.
 *
 * @note For instances of this class to be cleaned up correctly, @c shutdown() must be called.
 */
class ExternalMediaPlayer
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::utils::RequiresShutdown
        , public avsCommon::sdkInterfaces::ExternalMediaPlayerInterface
        , public avsCommon::sdkInterfaces::SpeakerInterface
        , public avsCommon::sdkInterfaces::PlaybackHandlerInterface
        , public std::enable_shared_from_this<ExternalMediaPlayer> {
public:
    // Map of adapter business names to their mediaPlayers.
    using AdapterMediaPlayerMap =
        std::unordered_map<std::string, std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>>;

    // Signature of functions to create an ExternalMediaAdapter.
    using AdapterCreateFunction =
        std::shared_ptr<avsCommon::sdkInterfaces::externalMediaPlayer::ExternalMediaAdapterInterface> (*)(
            std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer,
            std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager,
            std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
            std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
            std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
            std::shared_ptr<avsCommon::sdkInterfaces::ExternalMediaPlayerInterface> externalMediaPlayer);

    // Map of adapter business names to their creation method.
    using AdapterCreationMap = std::unordered_map<std::string, AdapterCreateFunction>;

    /**
     * Creates a new @c ExternalMediaPlayer instance.
     *
     * @param mediaPlayers The map of <PlayerId, MediaPlayer> to be used to find the mediaPlayer to use for this
     * adapter.
     * @param adapterCreationMap The map of <PlayerId, AdapterCreateFunction> to be used to create the adapters.
     * @param speakerManager A @c SpeakerManagerInterface to perform volume changes requested by adapters.
     * @param messageSender The object to use for sending events.
     * @param focusManager The object used to manage focus for the adapter managed by the EMP.
     * @param contextManager The AVS Context manager used to generate system context for events.
     * @param exceptionSender The object to use for sending AVS Exception messages.
     * @param playbackRouter The @c PlaybackRouterInterface instance to use when @c ExternalMediaPlayer becomes active.
     * @return A @c std::shared_ptr to the new @c ExternalMediaPlayer instance.
     */
    static std::shared_ptr<ExternalMediaPlayer> create(
        const AdapterMediaPlayerMap& mediaPlayers,
        const AdapterCreationMap& adapterCreationMap,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> playbackRouter);

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

    /// @name Overridden SpeakerInterface methods.
    /// @{
    bool setVolume(int8_t volume) override;
    bool adjustVolume(int8_t volume) override;
    bool setMute(bool mute) override;
    bool getSpeakerSettings(avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings* settings) override;
    avsCommon::sdkInterfaces::SpeakerInterface::Type getSpeakerType() override;
    /// @}

    /// @name Overridden PlaybackHandlerInterface methods.
    /// @{
    virtual void onButtonPressed(avsCommon::avs::PlaybackButton button) override;
    /// @}

    /// @name Overridden ExternalMediaPlayerInterface methods.
    /// @{
    virtual void setPlayerInFocus(const std::string& playerInFocus) override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param speakerManager A @c SpeakerManagerInterface to perform volume changes requested by adapters.
     * @param contextManager The AVS Context manager used to generate system context for events.
     * @param exceptionSender The object to use for sending AVS Exception messages.
     * @param playbackRouter The @c PlaybackRouterInterface instance to use when @c ExternalMediaPlayer becomes active.
     * @return A @c std::shared_ptr to the new @c ExternalMediaPlayer instance.
     */
    ExternalMediaPlayer(
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> playbackRouter);

    /**
     * This method returns the ExternalMediaPlayer session state registered in the ExternalMediaPlayer namespace.
     */
    std::string provideSessionState();

    /**
     * This method returns the Playback state registered in the Alexa.PlaybackStateReporter state.
     */
    std::string providePlaybackState();

    /**
     * This function deserializes a @c Directive's payload into a @c rapidjson::Document.
     *
     * @param info The @c DirectiveInfo to read the payload string from.
     * @param[out] document The @c rapidjson::Document to parse the payload into.
     * @return @c true if parsing was successful, else @c false.
     */
    bool parseDirectivePayload(std::shared_ptr<DirectiveInfo> info, rapidjson::Document* document);

    /**
     * Remove a directive from the map of message IDs to DirectiveInfo instances.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective whose message ID is to be removed.
     */
    void removeDirective(std::shared_ptr<DirectiveInfo> info);

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /**
     * Method to create all the adapters registered.
     *
     * @param mediaPlayers The map of <PlayerId, MediaPlayer> to be used to find the mediaPlayer to use for this
     * adapter.
     * @param adapterCreationMap The map of <PlayerId, AdapterCreateFunction> to be used to create the adapters.
     * @param messageSender The messager sender of the adapter.
     * @param focusManager The focus manager to be used by the adapter to acquire/release channel.
     * @param contextManager The context manager of the ExternalMediaPlayer and adapters.
     */
    void createAdapters(
        const AdapterMediaPlayerMap& mediaPlayers,
        const AdapterCreationMap& adapterCreationMap,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager);

    /**
     * Send the handling completed notification and clean up the resources the specified @c DirectiveInfo.
     *
     * @param info The @c DirectiveInfo to complete and clean up.
     */
    void setHandlingCompleted(std::shared_ptr<DirectiveInfo> info);

    /**
     * Send ExceptionEncountered and report a failure to handle the @c AVSDirective.
     *
     * @param info The @c AVSDirective that encountered the error and ancillary information.
     * @param type The type of Exception that was encountered.
     * @param message The error message to include in the ExceptionEncountered message.
     */
    void sendExceptionEncounteredAndReportFailed(
        std::shared_ptr<DirectiveInfo> info,
        const std::string& message,
        avsCommon::avs::ExceptionErrorType type = avsCommon::avs::ExceptionErrorType::INTERNAL_ERROR);

    /**
     * @name Executor Thread Functions
     *
     * These functions (and only these functions) are called by @c m_executor on a single worker thread.  All other
     * functions in this class can be called asynchronously, and pass data to the @c Executor thread through parameters
     * to lambda functions.  No additional synchronization is needed.
     */
    /// @{
    /**
     * This function provides updated context information for @c ExternalMediaPlayer to @c ContextManager.  This
     * function is called when @c ContextManager calls @c provideState(), and is also called internally by @c
     * changeActivity().
     *
     * @param stateProviderName The name of the stateProvider.
     * @param sendToken flag indicating whether @c stateRequestToken contains a valid token which should be passed
     *     along to @c ContextManager.  This flag defaults to @c false.
     * @param stateRequestToken The token @c ContextManager passed to the @c provideState() call, which will be passed
     *     along to the ContextManager::setState() call.  This parameter is not used if @c sendToken is @c false.
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
     * @return A shared-ptr to the ExternalMediaAdapterInterface on which the actual
     *        adapter method has to be invoked.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::externalMediaPlayer::ExternalMediaAdapterInterface> preprocessDirective(
        std::shared_ptr<DirectiveInfo> info,
        rapidjson::Document* document);

    /**
     * Handler for login directive.
     *
     * @param info The DirectiveInfo to be processed.
     * @param The type of the request. Will be LOGIN for the handleLogin case.
     */
    void handleLogin(
        std::shared_ptr<DirectiveInfo> info,
        avsCommon::sdkInterfaces::externalMediaPlayer::RequestType request);

    /**
     * Handler for logout directive.
     *
     * @param info The DirectiveInfo to be processed.
     * @param The type of the request. Will be LOGOUT for the handleLogout case.
     */
    void handleLogout(
        std::shared_ptr<DirectiveInfo> info,
        avsCommon::sdkInterfaces::externalMediaPlayer::RequestType request);

    /**
     * Handler for play directive.
     *
     * @param info The DirectiveInfo to be processed.
     * @param The type of the request. Will be PLAY for the handlePlay case.
     */
    void handlePlay(
        std::shared_ptr<DirectiveInfo> info,
        avsCommon::sdkInterfaces::externalMediaPlayer::RequestType request);

    /**
     * Handler for play control directive.
     *
     * @param info The DirectiveInfo to be processed.
     * @param The type of the request. Can be NEXT/PREVIOUS/PAUSE/RESUME... for the handlePlayControl case.
     */
    void handlePlayControl(
        std::shared_ptr<DirectiveInfo> info,
        avsCommon::sdkInterfaces::externalMediaPlayer::RequestType request);

    /**
     * Handler for SetSeekControl  directive.
     *
     * @param info The DirectiveInfo to be processed.
     * @param The type of the request. RequestType will be SEEK.
     */
    void handleSeek(
        std::shared_ptr<DirectiveInfo> info,
        avsCommon::sdkInterfaces::externalMediaPlayer::RequestType request);

    /**
     * Handler for AdjustSeekControl  directive.
     *
     * @param info The DirectiveInfo to be processed.
     * @param The type of the request. RequestType will be ADJUST_SEEK.
     */
    void handleAdjustSeek(
        std::shared_ptr<DirectiveInfo> info,
        avsCommon::sdkInterfaces::externalMediaPlayer::RequestType request);

    /// The @c SpeakerManagerInterface used to change the volume when requested by @c ExternalMediaAdapterInterface.
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> m_speakerManager;

    /// The @c ContextManager that needs to be updated of the state.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c PlaybackRouterInterface instance to use when @c ExternalMediaPlayer becomes active.
    std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> m_playbackRouter;

    /// The @ m_adapters Map of business names to the adapters.
    std::map<std::string, std::shared_ptr<avsCommon::sdkInterfaces::externalMediaPlayer::ExternalMediaAdapterInterface>>
        m_adapters;

    /// The id of the player which currently has focus.
    std::string m_playerInFocus;

    /// A holder for @c SpeakerSettings to report.
    avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings m_speakerSettings;

    /**
     * @c Executor which queues up operations from asynchronous API calls.
     *
     * @note This declaration needs to come *after* the Executor Thread Variables above so that the thread shuts down
     *     before the Executor Thread Variables are destroyed.
     */
    avsCommon::utils::threading::Executor m_executor;

    /// typedef of the function pointer to handle AVS directives.
    typedef void (ExternalMediaPlayer::*DirectiveHandler)(
        std::shared_ptr<DirectiveInfo> info,
        avsCommon::sdkInterfaces::externalMediaPlayer::RequestType request);

    /// The singleton map from a directive to its handler.
    static std::unordered_map<
        avsCommon::avs::NamespaceAndName,
        std::pair<avsCommon::sdkInterfaces::externalMediaPlayer::RequestType, ExternalMediaPlayer::DirectiveHandler>>
        m_directiveToHandlerMap;
};

}  // namespace externalMediaPlayer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_EXTERNALMEDIAPLAYER_INCLUDE_EXTERNALMEDIAPLAYER_EXTERNALMEDIAPLAYER_H_
