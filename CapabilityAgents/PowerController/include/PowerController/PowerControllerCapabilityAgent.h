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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_POWERCONTROLLER_INCLUDE_POWERCONTROLLER_POWERCONTROLLERCAPABILITYAGENT_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_POWERCONTROLLER_INCLUDE_POWERCONTROLLER_POWERCONTROLLERCAPABILITYAGENT_H_

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/CapabilityState.h>
#include <AVSCommon/SDKInterfaces/AlexaInterfaceMessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/PowerController/PowerControllerInterface.h>
#include <AVSCommon/SDKInterfaces/PowerController/PowerControllerObserverInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/Timing/Timer.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace powerController {

/**
 * The @c PowerControllerCapabilityAgent is responsible for handling Alexa.PowerController directives and
 * calls the @c PowerControllerInterface APIs.
 *
 * This class implements a @c CapabilityAgent that handles the @c Alexa.PowerContoller Interface.
 */
class PowerControllerCapabilityAgent
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::sdkInterfaces::powerController::PowerControllerObserverInterface
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<PowerControllerCapabilityAgent> {
public:
    /// Alias to improve readability.
    using EndpointIdentifier = avsCommon::sdkInterfaces::endpoints::EndpointIdentifier;
    using PowerState = avsCommon::sdkInterfaces::powerController::PowerControllerInterface::PowerState;

    /**
     * Create an instance of @c PowerControllerCapabilityAgent.
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param powerController An interface that this object will use to perform the power controller operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the reponse to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @param isProactivelyReported Whether the power state property change is proactively reported to Alexa in a change
     * report.
     * @param isRetrievable Whether the power state property can be retrieved when Alexa sends a state report request to
     * the endpoint.
     * @return @c nullptr if the inputs are invalid, else a new instance of @c PowerControllerCapabilityAgent.
     */
    static std::shared_ptr<PowerControllerCapabilityAgent> create(
        const EndpointIdentifier& endpointId,
        std::shared_ptr<avsCommon::sdkInterfaces::powerController::PowerControllerInterface> powerController,
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

    /**
     * Get the capability configuration for this agent.
     *
     * @return The capability configuration.
     */
    avsCommon::avs::CapabilityConfiguration getCapabilityConfiguration();

    /// @name PowerControllerObserverInterface Function
    /// @{
    void onPowerStateChanged(const PowerState& powerState, avsCommon::sdkInterfaces::AlexaStateChangeCauseType cause)
        override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param powerController An interface that this object will use to perform the power controller operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the reponse to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @param isProactivelyReported Whether the power state property change is proactively reported to Alexa in a change
     * report.
     * @param isRetrievable Whether the power state property can be retrieved when Alexa sends a state report request to
     * the endpoint.
     */
    PowerControllerCapabilityAgent(
        const EndpointIdentifier& endpointId,
        std::shared_ptr<avsCommon::sdkInterfaces::powerController::PowerControllerInterface> powerController,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> responseSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        bool isProactivelyReported,
        bool isRetrievable);

    // @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

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
     * Gets the current power state from endpoint and notifies @c ContextManager
     *
     * @param stateProviderName Provides the property name and used in the @c ContextManager methods.
     * @param contextRequestToken The token to be used when providing the response to @c ContextManager
     */
    void executeProvideState(
        const avsCommon::avs::CapabilityTag& stateProviderName,
        const avsCommon::sdkInterfaces::ContextRequestToken contextRequestToken);

    /**
     * Call methods of @c AlexaInterfaceMessageSenderInterface based the endpoint's respone for a controller method
     * call.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param result This contains the endpoint's response of a controller call.
     */
    void executeSendResponseEvent(
        const std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
        const std::pair<avsCommon::avs::AlexaResponseType, std::string> result);

    /**
     * Helper method to build the @c CapabilityState
     *
     * @param powerState The power state defined using @c PowerState
     */
    avsCommon::avs::CapabilityState buildCapabilityState(const PowerState& powerState);

    /// Endpoint the capability agent associated to.
    EndpointIdentifier m_endpointId;

    /// Whether the power state property change is proactively reported to Alexa in a change report.
    bool m_isProactivelyReported;

    /// Whether the power state property can be retrieved when Alexa sends a state report request to the endpoint.
    bool m_isRetrievable;

    /// Reference to @c PowerControllerInterface
    std::shared_ptr<avsCommon::sdkInterfaces::powerController::PowerControllerInterface> m_powerController;

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c AlexaInterfaceMessageSenderInterface used to send event messages.
    std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> m_responseSender;

    /// This is the worker thread for the @c PowerControllerCapabilityAgent.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace powerController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_POWERCONTROLLER_INCLUDE_POWERCONTROLLER_POWERCONTROLLERCAPABILITYAGENT_H_
