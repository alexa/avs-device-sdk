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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TOGGLECONTROLLER_INCLUDE_TOGGLECONTROLLER_TOGGLECONTROLLERCAPABILITYAGENT_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TOGGLECONTROLLER_INCLUDE_TOGGLECONTROLLER_TOGGLECONTROLLERCAPABILITYAGENT_H_

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/CapabilityState.h>
#include <AVSCommon/SDKInterfaces/AlexaInterfaceMessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ToggleController/ToggleControllerAttributes.h>
#include <AVSCommon/SDKInterfaces/ToggleController/ToggleControllerInterface.h>
#include <AVSCommon/SDKInterfaces/ToggleController/ToggleControllerObserverInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/Timing/Timer.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace toggleController {

/**
 * The @c ToggleControllerCapabilityAgent is responsible for handling Alexa.ToggleController directives and
 * calls the @c ToggleControllerInterface APIs.
 *
 * This class implements a @c CapabilityAgent that handles the @c Alexa.ToggleContoller Interface.
 */
class ToggleControllerCapabilityAgent
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::sdkInterfaces::toggleController::ToggleControllerObserverInterface
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<ToggleControllerCapabilityAgent> {
public:
    /// Alias to improve readability.
    using EndpointIdentifier = avsCommon::sdkInterfaces::endpoints::EndpointIdentifier;
    using ToggleState = avsCommon::sdkInterfaces::toggleController::ToggleControllerInterface::ToggleState;

    /**
     * Create an instance of @c ToggleControllerCapabilityAgent.
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param instance A unique non-empty instance name identifying a toggle controller in endpoint.
     * @param toggleControllerAttributes Used to populate the discovery message.
     * @param toggleController An interface that this object will use to perform the toggle operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the reponse to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @param isProactivelyReported Whether the toggle state property change is proactively reported to Alexa in a
     * change report.
     * @param isRetrievable Whether the toggle state property can be retrieved when Alexa sends a state report request
     * to the endpoint.
     * @param isNonControllable Whether the toggle state can be controlled or not. This must be @c false for the
     * toggle state to be controllable. Default is @c false.
     * @return @c nullptr if the inputs are invalid, else a new instance of @c ToggleControllerCapabilityAgent.
     */
    static std::shared_ptr<ToggleControllerCapabilityAgent> create(
        const EndpointIdentifier& endpointId,
        const std::string& instance,
        const avsCommon::sdkInterfaces::toggleController::ToggleControllerAttributes& toggleControllerAttributes,
        std::shared_ptr<avsCommon::sdkInterfaces::toggleController::ToggleControllerInterface> toggleController,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> responseSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        bool isProactivelyReported,
        bool isRetrievable,
        bool isNonControllable = false);

    /// @name CapabilityAgent Functions
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

    /// @name ToggleControllerObserverInterface Function
    /// @{
    void onToggleStateChanged(const ToggleState& toggleState, avsCommon::sdkInterfaces::AlexaStateChangeCauseType cause)
        override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param instance A unique non-empty instance name identifying a toggle controller in endpoint.
     * @param toggleControllerAttributes Used to populate the discovery message.
     * @param toggleController An interface that this object will use to perform the toggle operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the event reponse to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @param isProactivelyReported Whether the toggle state property change is proactively reported to Alexa in a
     * change report.
     * @param isRetrievable Whether the toggle state property can be retrieved when Alexa sends a state report request
     * to the endpoint.
     * @param isNonControllable Whether the toggle state can be controlled or not. This must be @c false for the
     * toggle state to be controllable. Default is @c false.
     */
    ToggleControllerCapabilityAgent(
        const EndpointIdentifier& endpointId,
        const std::string& instance,
        const avsCommon::sdkInterfaces::toggleController::ToggleControllerAttributes& toggleControllerAttributes,
        std::shared_ptr<avsCommon::sdkInterfaces::toggleController::ToggleControllerInterface> toggleController,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> responseSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        bool isProactivelyReported,
        bool isRetrievable,
        bool isNonControllable = false);

    // @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

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
     * Gets the current toggle state from endpoint and notifies @c ContextManager
     *
     * @param stateProviderName Provides the property name and used in the @c ContextManager methods.
     * @param contextRequestToken The token to be used in when providing the response to context manager.
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
        std::pair<avsCommon::avs::AlexaResponseType, std::string> result);

    /**
     * Helper method to build the @c CapabilityState
     *
     * @param toggleState The toggle state defined using @c ToggleState
     */
    avsCommon::avs::CapabilityState buildCapabilityState(const ToggleState& toggleState);

    /// Endpoint the capability agent associated to.
    EndpointIdentifier m_endpointId;

    /// The instance name of the toggle endpoint.
    std::string m_instance;

    /// Whether the toggle state property change is proactively reported to Alexa in a change report.
    bool m_isProactivelyReported;

    /// Whether the toggle state property can be retrieved when Alexa sends a state report request to the endpoint.
    bool m_isRetrievable;

    /// Whether the toggle state property can be controlled or not.
    bool m_isNonControllable;

    /// The toggle controller attributes used in discovery.
    avsCommon::sdkInterfaces::toggleController::ToggleControllerAttributes m_toggleControllerAttributes;

    /// Reference to @c ToggleControllerInterface
    std::shared_ptr<avsCommon::sdkInterfaces::toggleController::ToggleControllerInterface> m_toggleController;

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c AlexaInterfaceMessageSenderInterface used to send event messages.
    std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> m_responseSender;

    /// This is the worker thread for the @c ToggleController CA.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace toggleController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TOGGLECONTROLLER_INCLUDE_TOGGLECONTROLLER_TOGGLECONTROLLERCAPABILITYAGENT_H_
