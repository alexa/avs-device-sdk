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

#ifndef ACSDK_ALEXACHANNELCONTROLLER_PRIVATE_ALEXACHANNELCONTROLLERCAPABILITYAGENT_H_
#define ACSDK_ALEXACHANNELCONTROLLER_PRIVATE_ALEXACHANNELCONTROLLERCAPABILITYAGENT_H_

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

#include <acsdk/AlexaChannelControllerInterfaces/ChannelControllerInterface.h>
#include <acsdk/AlexaChannelControllerInterfaces/ChannelType.h>

namespace alexaClientSDK {
namespace alexaChannelController {

/**
 * The @c AlexaChannelControllerCapabilityAgent is responsible for handling Alexa.ChannelController directives and
 * calls the @c AlexaChannelControllerInterface APIs.
 *
 * This class implements a @c CapabilityAgent that handles the @c Alexa.ChannelController Interface.
 */
class AlexaChannelControllerCapabilityAgent
        : public avsCommon::avs::CapabilityAgent
        , public alexaChannelControllerInterfaces::ChannelControllerObserverInterface
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<AlexaChannelControllerCapabilityAgent> {
public:
    /**
     * Create an instance of @c AlexaChannelControllerCapabilityAgent.
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param channelController An interface that this object will use to perform the channel controller operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the response to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @param isProactivelyReported Whether the channel state properties change is proactively reported to AVS in a
     * change report.
     * @param isRetrievable Whether the channel state properties can be retrieved when AVS sends a state report
     * request to the endpoint.
     * @return @c nullptr if the inputs are invalid, else a new instance of @c AlexaChannelControllerCapabilityAgent.
     */
    static std::shared_ptr<AlexaChannelControllerCapabilityAgent> create(
        avsCommon::sdkInterfaces::endpoints::EndpointIdentifier endpointId,
        std::shared_ptr<alexaChannelControllerInterfaces::ChannelControllerInterface> channelController,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> responseSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        bool isProactivelyReported,
        bool isRetrievable);

    /// @name CapabilityAgent/DirectiveHandlerInterface Functions
    /// @{
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<DirectiveInfo> info) override;
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    /// @}

    /// @name StateProviderInterface Functions
    /// @{
    void provideState(
        const avsCommon::avs::CapabilityTag& stateProviderName,
        const avsCommon::sdkInterfaces::ContextRequestToken contextRequestToken) override;
    bool canStateBeRetrieved() override;
    bool hasReportableStateProperties() override;
    /// @}

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;
    /// @}

    /// @name ChannelControllerObserverInterface Function
    /// @{
    void onChannelChanged(std::unique_ptr<alexaChannelControllerTypes::Channel> channel) override;
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
     * @param channelController An interface that this object will use to perform the channel controller operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the response to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @param isProactivelyReported Whether the channel state properties change is proactively reported to AVS in a
     * change report.
     * @param isRetrievable Whether the channel state properties can be retrieved when AVS sends a state report
     * request to the endpoint.
     */
    AlexaChannelControllerCapabilityAgent(
        avsCommon::sdkInterfaces::endpoints::EndpointIdentifier endpointId,
        std::shared_ptr<alexaChannelControllerInterfaces::ChannelControllerInterface> channelController,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> responseSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        bool isProactivelyReported,
        bool isRetrievable);

    /**
     * Initializes the object.
     */
    bool initialize();

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
    void executeUnknownDirective(std::shared_ptr<DirectiveInfo> info, avsCommon::avs::ExceptionErrorType type);

    /**
     * Gets the current channel state from endpoint and notifies @c ContextManager
     *
     * @param stateProviderName Provides the property name and used in the @c ContextManager methods.
     * @param contextRequestToken The token to be used when providing the response to @c ContextManager
     */
    void executeProvideState(
        const avsCommon::avs::CapabilityTag& stateProviderName,
        const avsCommon::sdkInterfaces::ContextRequestToken contextRequestToken);

    /**
     * Call methods of @c AlexaInterfaceMessageSenderInterface based the endpoint's response for a controller method
     * call.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param result This contains the endpoint's response to a controller call.
     * @param currentChannel The current channel at the endpoint.
     */
    void sendResponseEvent(
        const std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
        const alexaChannelControllerInterfaces::ChannelControllerInterface::Response& result,
        std::unique_ptr<alexaChannelControllerTypes::Channel> currentChannel);

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
     * Helper method to build the @c JSON string for CapabilityState
     *
     * @param channelState The channel state defined using @c ChannelState
     */
    std::string buildCapabilityStateString(const alexaChannelControllerTypes::Channel& channelState);

    /**
     * parse ChangeChannel payload and return channel information
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective for changing channel.
     * @param payloadDocument JSON document, contains ChangeChannel directive payload.
     * @return nullptr if the payload is invalid, a std::unique_ptr to a channel object containing the information
     * otherwise.
     */
    std::unique_ptr<alexaChannelControllerTypes::Channel> parseChannel(
        const std::shared_ptr<DirectiveInfo> info,
        const rapidjson::Document& payloadDocument);

    /**
     * parse ChangeChannel payload and return channel count information
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective for changing channel.
     * @param payloadDocument JSON document, contains ChangeChannel directive payload.
     * @param channelCount This stores the channel count value extracted from the payload, if the payload is valid.
     * @return false if the payload is invalid, true otherwise.
     */
    bool parseChannelCount(
        const std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
        const rapidjson::Document& payloadDocument,
        int64_t& channelCount);

    /**
     * Builds the lineup configuration json, that is optionally passed during discovery.
     *
     * @return The Json string.
     */
    std::string buildLineupConfigurationJson();

    /// Endpoint the capability agent associated to.
    avsCommon::sdkInterfaces::endpoints::EndpointIdentifier m_endpointId;

    /// Whether the channel state properties change is proactively reported to Alexa in a change report.
    bool m_isProactivelyReported;

    /// Whether the channel state properties can be retrieved when Alexa sends a state report request to the endpoint.
    bool m_isRetrievable;

    /// Reference to @c ChannelControllerInterface
    std::shared_ptr<alexaChannelControllerInterfaces::ChannelControllerInterface> m_channelController;

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c AlexaInterfaceMessageSenderInterface used to send event messages.
    std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> m_responseSender;

    /// This is the worker thread for the @c AlexaChannelControllerCapabilityAgent.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace alexaChannelController
}  // namespace alexaClientSDK

#endif  // ACSDK_ALEXACHANNELCONTROLLER_PRIVATE_ALEXACHANNELCONTROLLERCAPABILITYAGENT_H_
