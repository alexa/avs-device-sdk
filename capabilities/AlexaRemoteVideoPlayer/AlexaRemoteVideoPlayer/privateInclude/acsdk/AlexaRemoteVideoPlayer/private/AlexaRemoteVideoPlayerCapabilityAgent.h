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

#ifndef ACSDK_ALEXAREMOTEVIDEOPLAYER_PRIVATE_ALEXAREMOTEVIDEOPLAYERCAPABILITYAGENT_H_
#define ACSDK_ALEXAREMOTEVIDEOPLAYER_PRIVATE_ALEXAREMOTEVIDEOPLAYERCAPABILITYAGENT_H_

#include <memory>
#include <string>
#include <unordered_set>

#include <rapidjson/document.h>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/CapabilityState.h>
#include <AVSCommon/SDKInterfaces/AlexaInterfaceMessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>

#include <acsdk/AlexaRemoteVideoPlayerInterfaces/RemoteVideoPlayerInterface.h>

namespace alexaClientSDK {
namespace alexaRemoteVideoPlayer {

/**
 * The @c AlexaRemoteVideoPlayerCapabilityAgent is responsible for handling Alexa.RemoteVideoPlayer directives and
 * calls the @c RemoteVideoPlayerInterface APIs.
 *
 * This class implements a @c CapabilityAgent that handles the @c Alexa.RemoteVideoPlayer Interface.
 */
class AlexaRemoteVideoPlayerCapabilityAgent
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<AlexaRemoteVideoPlayerCapabilityAgent> {
public:
    /// Alias to improve readability.
    using EndpointIdentifier = alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointIdentifier;

    /**
     * Create an instance of @c AlexaRemoteVideoPlayerCapabilityAgent.
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param remoteVideoPlayer An interface that this object will use to perform the remote video player operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the reponse to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @return @c nullptr if the inputs are invalid, else a new instance of @c AlexaRemoteVideoPlayerCapabilityAgent.
     */
    static std::shared_ptr<AlexaRemoteVideoPlayerCapabilityAgent> create(
        EndpointIdentifier endpointId,
        std::shared_ptr<alexaRemoteVideoPlayerInterfaces::RemoteVideoPlayerInterface> remoteVideoPlayer,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> responseSender,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender);

    /// @name CapabilityAgent/DirectiveHandlerInterface Functions
    /// @{
    void handleDirectiveImmediately(std::shared_ptr<alexaClientSDK::avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<DirectiveInfo> info) override;
    alexaClientSDK::avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    /// @}

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityConfiguration>>
    getCapabilityConfigurations() override;
    /// @}

    // @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param remoteVideoPlayer An interface that this object will use to perform the remote video player operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the reponse to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     */
    AlexaRemoteVideoPlayerCapabilityAgent(
        EndpointIdentifier endpointId,
        std::shared_ptr<alexaRemoteVideoPlayerInterfaces::RemoteVideoPlayerInterface> remoteVideoPlayer,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> responseSender,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender);

    /**
     * Remove a directive from the map of message IDs to DirectiveInfo instances.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective whose message ID is to be removed.
     */
    void removeDirective(std::shared_ptr<DirectiveInfo> info);

    /**
     * Send the handling completed notification and clean up the resources.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void executeSetHandlingCompleted(std::shared_ptr<DirectiveInfo> info);

    /**
     * Called on executor to handle any unknown directives received.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param type The @c ExceptionErrorType.
     */
    void executeUnknownDirective(
        std::shared_ptr<DirectiveInfo> info,
        alexaClientSDK::avsCommon::avs::ExceptionErrorType type);

    /**
     * Call methods of @c AlexaInterfaceMessageSenderInterface based the endpoint's response for a remote video player
     * method call.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param result This contains the endpoint's response for a remote video player call.
     * @param playbackState The current playback state to be reported in the Playback State Reporter response.
     */
    void executeSendResponseEvent(
        const std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
        const alexaRemoteVideoPlayerInterfaces::RemoteVideoPlayerInterface::Response& result,
        const std::string& playbackState);

    /**
     * Send Alexa interface error response.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param alexaErrorResponseType Alexa interface error response type.
     * @param responseMessage Alexa interface error response message.
     */
    void sendAlexaErrorResponse(
        const std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
        const avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface::ErrorResponseType alexaErrorResponseType,
        const std::string& responseMessage);

    /**
     * Send Alexa.Video interface error response.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param alexaVideoErrorResponseType Alexa.Video interface error response type.
     * @param responseMessage Alexa.Video interface error response message.
     */
    void sendAlexaVideoErrorResponse(
        const std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
        const avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType
            alexaVideoErrorResponseType,
        const std::string& responseMessage);

    /**
     * Builds the configuration json required for the discovery.
     *
     * @param configuration The remote video player configuration object containing the supported directive types,
     * entity types and catalog information.
     * @return The json string corresponding to the configuration.
     */
    std::string buildRemoteVideoPlayerConfigurationJson(
        const alexaRemoteVideoPlayerInterfaces::Configuration& configuration);

    /*
     * Parse the payload received with Alexa.RemoteVideoPlayer directives.
     *
     * @param payload String json payload to be parsed.
     * @return unique pointer to @c RemoteVideoPlayerRequest if the payload is parseable, else nullptr
     */
    std::unique_ptr<alexaRemoteVideoPlayerInterfaces::RemoteVideoPlayerRequest> parseDirectivePayload(
        const std::string& payload);

    /*
     * Parse a video entity object.
     * @param entityJson String json entity to be parsed
     * @param [out] remoteVideoPlayerRequest Context to be loaded with the video entity object.
     * @return true if successful.
     */
    bool parseEntityJson(
        const rapidjson::Value& entityJson,
        alexaRemoteVideoPlayerInterfaces::RemoteVideoPlayerRequest& remoteVideoPlayerRequest);

    /// Endpoint the capability agent associated to.
    EndpointIdentifier m_endpointId;

    /// Reference to @c RemoteVideoPlayerInterface
    std::shared_ptr<alexaRemoteVideoPlayerInterfaces::RemoteVideoPlayerInterface> m_remoteVideoPlayer;

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c AlexaInterfaceMessageSenderInterface used to send event messages.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> m_responseSender;

    /// This is the worker thread for the @c AlexaRemoteVideoPlayerCapabilityAgent.
    alexaClientSDK::avsCommon::utils::threading::Executor m_executor;
};

}  // namespace alexaRemoteVideoPlayer
}  // namespace alexaClientSDK

#endif  // ACSDK_ALEXAREMOTEVIDEOPLAYER_PRIVATE_ALEXAREMOTEVIDEOPLAYERCAPABILITYAGENT_H_