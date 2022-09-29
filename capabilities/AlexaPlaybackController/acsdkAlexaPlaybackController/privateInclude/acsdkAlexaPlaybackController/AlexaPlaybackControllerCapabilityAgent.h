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

#ifndef ACSDKALEXAPLAYBACKCONTROLLER_PRIVATEINCLUDE_ACSDKALEXAPLAYBACKCONTROLLER_ALEXAPLAYBACKCONTROLLERCAPABILITYAGENT_H_
#define ACSDKALEXAPLAYBACKCONTROLLER_PRIVATEINCLUDE_ACSDKALEXAPLAYBACKCONTROLLER_ALEXAPLAYBACKCONTROLLERCAPABILITYAGENT_H_

#include <string>

#include <acsdkAlexaPlaybackControllerInterfaces/AlexaPlaybackControllerInterface.h>
#include <acsdkAlexaPlaybackControllerInterfaces/AlexaPlaybackControllerObserverInterface.h>
#include <acsdkAlexaPlaybackControllerInterfaces/PlaybackState.h>
#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/CapabilityState.h>
#include <AVSCommon/SDKInterfaces/AlexaInterfaceMessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace acsdkAlexaPlaybackController {

/**
 * The @c AlexaPlaybackControllerCapabilityAgent is responsible for handling Alexa.PlaybackController directives and
 * calls the @c AlexaPlaybackControllerInterface APIs.
 *
 * Note: This is a different API than
 * https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/playbackcontroller.html
 *
 * This class implements a @c CapabilityAgent that handles the @c Alexa.PlaybackController Interface.
 */
class AlexaPlaybackControllerCapabilityAgent
        : public avsCommon::avs::CapabilityAgent
        , public acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerObserverInterface
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<AlexaPlaybackControllerCapabilityAgent> {
public:
    /**
     * Create an instance of @c AlexaPlaybackControllerCapabilityAgent.
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param playbackController An interface that this object will use to perform the playback controller operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the reponse to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @param isProactivelyReported Whether the playback state reporter properties change is proactively reported to
     * Alexa in a change report.
     * @param isRetrievable Whether the playback state reporter properties can be retrieved when Alexa sends a state
     * report request to the endpoint.
     * @return @c nullptr if the inputs are invalid, else a new instance of @c AlexaPlaybackControllerCapabilityAgent.
     */
    static std::shared_ptr<AlexaPlaybackControllerCapabilityAgent> create(
        const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
        std::shared_ptr<acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface> playbackController,
        const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
        const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender,
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

    /// @name AlexaPlaybackControllerObserverInterface Function
    /// @{
    void onPlaybackStateChanged(const acsdkAlexaPlaybackControllerInterfaces::PlaybackState& playbackState) override;
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
     * @param playbackController An interface that this object will use to perform the playback controller operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the reponse to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @param isProactivelyReported Whether the playback state reporter properties change is proactively reported to
     *Alexa in a change report.
     * @param isRetrievable Whether the playback state reporter properties can be retrieved when Alexa sends a state
     *report request to the endpoint.
     **/
    AlexaPlaybackControllerCapabilityAgent(
        const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
        const std::shared_ptr<acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface>&
            playbackController,
        const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
        const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender,
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
     * Call methods of @c AlexaInterfaceMessageSenderInterface based the endpoint's response for a controller method
     * call.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param result This contains the endpoint's response of a controller call.
     */
    void executeSendResponseEvent(
        const std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
        const acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response& result);

    /**
     * Gets the current playback state from endpoint and notifies @c ContextManager
     *
     * @param stateProviderName Provides the property name and used in the @c ContextManager methods.
     * @param contextRequestToken The token to be used when providing the response to @c ContextManager
     */
    void executeProvideState(
        const avsCommon::avs::CapabilityTag& stateProviderName,
        const avsCommon::sdkInterfaces::ContextRequestToken contextRequestToken);

    /**
     * Helper method to build the @c CapabilityState
     *
     * @param playbackState The playback state
     */
    avsCommon::avs::CapabilityState buildCapabilityState(const std::string& playbackState);

    /// Endpoint the capability agent associated to.
    avsCommon::sdkInterfaces::endpoints::EndpointIdentifier m_endpointId;

    /// Whether the playback state controller properties change is proactively reported to Alexa in a change report.
    bool m_isProactivelyReported;

    /// Whether the playback state reporter properties can be retrieved when Alexa sends a state report request to the
    /// endpoint.
    bool m_isRetrievable;

    /// Reference to @c AlexaPlaybackControllerInterface
    std::shared_ptr<acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface> m_playbackController;

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c AlexaInterfaceMessageSenderInterface used to send event messages.
    std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> m_responseSender;

    /// Set of capability configurations that will get published during discovery
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> m_capabilityConfigurations;

    /// This is the worker thread for the @c AlexaPlaybackControllerCapabilityAgent.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace acsdkAlexaPlaybackController
}  // namespace alexaClientSDK

#endif  // ACSDKALEXAPLAYBACKCONTROLLER_PRIVATEINCLUDE_ACSDKALEXAPLAYBACKCONTROLLER_ALEXAPLAYBACKCONTROLLERCAPABILITYAGENT_H_
