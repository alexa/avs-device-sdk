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

#ifndef ACSDKALEXALAUNCHER_PRIVATEINCLUDE_ACSDKALEXALAUNCHER_ALEXALAUNCHERCAPABILITYAGENT_H_
#define ACSDKALEXALAUNCHER_PRIVATEINCLUDE_ACSDKALEXALAUNCHER_ALEXALAUNCHERCAPABILITYAGENT_H_

#include <string>

#include <Alexa/AlexaInterfaceMessageSender.h>
#include <Alexa/AlexaInterfaceMessageSenderInternalInterface.h>
#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/CapabilityState.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <acsdkAlexaLauncherInterfaces/AlexaLauncherTargetState.h>
#include <acsdkAlexaLauncherInterfaces/AlexaLauncherInterface.h>
#include <acsdkAlexaLauncherInterfaces/AlexaLauncherObserverInterface.h>

namespace alexaClientSDK {
namespace acsdkAlexaLauncher {

/**
 * The @c AlexaLauncherCapabilityAgent is responsible for handling Alexa.Launcher directives.
 *
 * This class implements a @c CapabilityAgent that handles the @c Alexa.Launcher Interface.
 */
class AlexaLauncherCapabilityAgent
        : public avsCommon::avs::CapabilityAgent
        , public alexaClientSDK::acsdkAlexaLauncherInterfaces::AlexaLauncherObserverInterface
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<AlexaLauncherCapabilityAgent> {
public:
    /**
     * Create an instance of @c AlexaLauncherCapabilityAgent.
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param launcher An interface that this object will use to perform the launcher operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the response to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @param isProactivelyReported Whether property changes are proactively reported to Alexa in a change
     * report.
     * @param isRetrievable Whether properties can be retrieved when Alexa sends a state report request to the
     * endpoint.
     * @return @c nullptr if the inputs are invalid, else a new instance of @c AlexaLauncherCapabilityAgent.
     */
    static std::shared_ptr<AlexaLauncherCapabilityAgent> create(
        const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
        const std::shared_ptr<acsdkAlexaLauncherInterfaces::AlexaLauncherInterface>& launcher,
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

    /// @name AlexaLauncherObserverInterface Functions
    /// @{
    void onLauncherTargetChanged(const acsdkAlexaLauncherInterfaces::TargetState& targetState) override;
    /// @}

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param launcher An interface that this object will use to perform the launcher operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the reponse to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @param isProactivelyReported Whether property changes are proactively reported to Alexa in a change
     * report.
     * @param isRetrievable Whether properties can be retrieved when Alexa sends a state report request to the
     * endpoint.
     */
    AlexaLauncherCapabilityAgent(
        const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
        const std::shared_ptr<acsdkAlexaLauncherInterfaces::AlexaLauncherInterface>& launcher,
        const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
        const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender,
        bool isProactivelyReported,
        bool isRetrievable);

    /**
     * Initialize this object.
     *
     * @return @c true if successful, @c false otherwise.
     */
    bool initialize();

    /**
     * Remove a directive from the map of message IDs to DirectiveInfo instances.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective whose message ID is to be removed.
     */
    void removeDirective(const std::shared_ptr<DirectiveInfo>& info);

    /**
     * Send the handling completed notification and clean up the resources.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void executeSetHandlingCompleted(const std::shared_ptr<DirectiveInfo>& info);

    /**
     * Called on executor to handle any unknown directives received.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param type The @c ExceptionErrorType.
     */
    void executeUnknownDirective(const std::shared_ptr<DirectiveInfo>& info, avsCommon::avs::ExceptionErrorType type);

    /**
     * Gets the current launcher target state from endpoint and notifies @c ContextManager
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
     * @param result This contains the endpoint's response of a controller call.
     */
    void executeSendResponseEvent(
        const std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
        const acsdkAlexaLauncherInterfaces::AlexaLauncherInterface::Response result);

    /**
     * Helper method to build the @c CapabilityState
     *
     * @param target The launcher target state defined using @c Target
     */
    avsCommon::avs::CapabilityState buildCapabilityState(const acsdkAlexaLauncherInterfaces::TargetState& targetState);

    /**
     * Helper method to read launcher payload and fill the Target object.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective for changing launch target.
     * @param payloadDocument JSON document, contains LaunchTarget directive payload.
     * @param[out] target Pointer to a Target object.
     */
    void readLaunchTargetPayload(
        const std::shared_ptr<DirectiveInfo> info,
        const rapidjson::Document& payloadDocument,
        acsdkAlexaLauncherInterfaces::TargetState& targetState);

    /**
     *  Evaulates whether the @c AlexaLauncherResponseType responseType is a video-specific error type.
     *
     * @param responseType the response type to evaluate
     * @return true if responseType maps to an @c AlexaVideoErrorResponseType, false otherwise
     */
    bool isVideoErrorResponseType(
        const acsdkAlexaLauncherInterfaces::AlexaLauncherInterface::Response::Type responseType);

    /**
     * Convert an @c AlexaLauncherResponseType to its corresponding @c ErrorResponseType. Note that any @c
     * AlexaLauncherResponseType that does not map to @c ErrorResponseType will return INTERNAL_ERROR.
     *
     * @param responseType The response type to convert.
     * @return The corresponding @c avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface::ErrorResponseType.
     */
    static avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface::ErrorResponseType
    alexaLauncherResponseTypeToErrorType(
        const acsdkAlexaLauncherInterfaces::AlexaLauncherInterface::Response::Type responseType);

    /**
     * Convert an @c AlexaLauncherResponseType to its corresponding @c AlexaVideoErrorResponseType. Note that any @c
     * AlexaLauncherResponseType that does not map to @c AlexaVideoErrorResponseType will return NONE.
     *
     * @param responseType The response type to convert.
     * @return The corresponding @c ErrorResponseType if it's valid, otherwise NONE.
     */
    static avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType
    alexaLauncherResponseTypeToVideoErrorType(
        const acsdkAlexaLauncherInterfaces::AlexaLauncherInterface::Response::Type responseType);

    /// Endpoint the capability agent associated to.
    avsCommon::sdkInterfaces::endpoints::EndpointIdentifier m_endpointId;

    /// Whether the mode property change is proactively reported to Alexa in a change report.
    bool m_isProactivelyReported;

    /// Whether the mode property can be retrieved when Alexa sends a state report request to the endpoint.
    bool m_isRetrievable;

    /// Reference to @c LauncherInterface
    std::shared_ptr<acsdkAlexaLauncherInterfaces::AlexaLauncherInterface> m_launcher;

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c AlexaInterfaceMessageSenderInterface used to send event messages.
    std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> m_responseSender;

    /// This is the worker thread for the @c AlexaLauncherCapabilityAgent.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace acsdkAlexaLauncher
}  // namespace alexaClientSDK

#endif  // ACSDKALEXALAUNCHER_PRIVATEINCLUDE_ACSDKALEXALAUNCHER_ALEXALAUNCHERCAPABILITYAGENT_H_
