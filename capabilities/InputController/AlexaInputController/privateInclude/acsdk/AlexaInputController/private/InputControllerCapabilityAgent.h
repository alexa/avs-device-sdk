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

#ifndef ACSDK_ALEXAINPUTCONTROLLER_PRIVATE_INPUTCONTROLLERCAPABILITYAGENT_H_
#define ACSDK_ALEXAINPUTCONTROLLER_PRIVATE_INPUTCONTROLLERCAPABILITYAGENT_H_

#include <memory>
#include <set>
#include <string>
#include <unordered_set>

#include <Alexa/AlexaInterfaceMessageSender.h>
#include <Alexa/AlexaInterfaceMessageSenderInternalInterface.h>
#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>

#include <acsdk/AlexaInputControllerInterfaces/InputControllerInterface.h>

namespace alexaClientSDK {
namespace alexaInputController {

/**
 * The Input Controller Capability Agent provides an implementation for a client to interface with the
 * Alexa.InputController API.
 *
 * @see https://developer.amazon.com/en-US/docs/alexa/device-apis/alexa-inputcontroller.html
 *
 * AVS sends SelectInput directives to the device to switch the input.
 *
 */
class AlexaInputControllerCapabilityAgent
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public avsCommon::utils::RequiresShutdown
        , public alexaInputControllerInterfaces::InputControllerObserverInterface
        , public std::enable_shared_from_this<AlexaInputControllerCapabilityAgent> {
public:
    /// Alias to improve readability.
    using EndpointIdentifier = avsCommon::sdkInterfaces::endpoints::EndpointIdentifier;

    /**
     * Creates an instance of the @c InputControllerCapabilityAgent.
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param inputController An interface that this object will use to perform the input controller operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the response to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @param isProactivelyReported Whether the input state properties change is proactively reported to AVS in a
     * change report.
     * @param isRetrievable Whether the input state properties can be retrieved when AVS sends a state report
     * request to the endpoint.
     * @return @c nullptr if the inputs are invalid, else a new instance of @c AlexaInputControllerCapabilityAgent.
     */
    static std::shared_ptr<AlexaInputControllerCapabilityAgent> create(
        avsCommon::sdkInterfaces::endpoints::EndpointIdentifier endpointId,
        std::shared_ptr<alexaInputControllerInterfaces::InputControllerInterface> inputController,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> responseSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        bool isProactivelyReported,
        bool isRetrievable);

    /**
     * Destructor.
     */
    ~AlexaInputControllerCapabilityAgent();

    /// @name CapabilityAgent Functions
    /// @{
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
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

    /// @name InputControllerObserverInterface Functions
    /// @{
    void onInputChanged(alexaInputControllerInterfaces::Input input) override;
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
     * @param inputController An interface that this object will use to perform the input controller operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the response to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @param isProactivelyReported Whether the channel state properties change is proactively reported to AVS in a
     * change report.
     * @param isRetrievable Whether the channel state properties can be retrieved when AVS sends a state report
     * request to the endpoint.
     */
    AlexaInputControllerCapabilityAgent(
        EndpointIdentifier endpointId,
        std::shared_ptr<alexaInputControllerInterfaces::InputControllerInterface> inputController,
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
     * Helper function to process the incoming directive
     *
     * @param info Directive to be processed.
     * @param[out] errMessage Error message associated to failure to process the directive
     * @param[out] type Error type associated to failure to process the directive
     * @return A bool indicating the success of processing the directive
     */
    bool executeHandleDirectiveHelper(
        std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
        std::string* errMessage,
        avsCommon::avs::ExceptionErrorType* type);

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
     * Send event based the endpoint's response for a controller method call.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param result This contains the endpoint's response of a controller call.
     */
    void sendResponseEvent(
        const std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo>& info,
        const alexaInputControllerInterfaces::InputControllerInterface::Response& result);

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
     * Helper method to build the @c CapabilityState
     *
     * @param input The input name
     */
    avsCommon::avs::CapabilityState buildCapabilityState(alexaInputControllerInterfaces::Input input);

    /// Endpoint the capability agent associated to.
    EndpointIdentifier m_endpointId;

    /// Whether the power state property change is proactively reported to Alexa in a change report.
    bool m_isProactivelyReported;

    /// Whether the power state property can be retrieved when Alexa sends a state report request to the endpoint.
    bool m_isRetrievable;

    /// Set of capability configurations that will get published using Capabilities API
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> m_capabilityConfigurations;

    /// The object to handle input change events.
    std::shared_ptr<alexaInputControllerInterfaces::InputControllerInterface> m_inputController;

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c AlexaInterfaceMessageSenderInterface used to send event messages.
    std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> m_responseSender;

    /// The configuration of the inputs.
    alexaInputControllerInterfaces::InputControllerInterface::SupportedInputs m_supportedInputs;

    /**
     * @c Executor which queues up operations from asynchronous API calls.
     *
     * @note This declaration needs to come *after* the Executor Thread Variables above so that the thread shuts down
     * before the Executor Thread Variables are destroyed.
     */
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace alexaInputController
}  // namespace alexaClientSDK

#endif  // ACSDK_ALEXAINPUTCONTROLLER_PRIVATE_INPUTCONTROLLERCAPABILITYAGENT_H_
