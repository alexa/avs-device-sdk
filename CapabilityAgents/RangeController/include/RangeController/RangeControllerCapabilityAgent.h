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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_RANGECONTROLLER_INCLUDE_RANGECONTROLLER_RANGECONTROLLERCAPABILITYAGENT_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_RANGECONTROLLER_INCLUDE_RANGECONTROLLER_RANGECONTROLLERCAPABILITYAGENT_H_

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/CapabilityState.h>
#include <AVSCommon/AVS/CapabilityResources.h>
#include <AVSCommon/SDKInterfaces/AlexaInterfaceMessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/RangeController/RangeControllerAttributeBuilderInterface.h>
#include <AVSCommon/SDKInterfaces/RangeController/RangeControllerInterface.h>
#include <AVSCommon/SDKInterfaces/RangeController/RangeControllerObserverInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/Timing/Timer.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace rangeController {

/**
 * The @c RangeControllerCapabilityAgent is responsible for handling Alexa.RangeController directives and
 * calls the @c RangeControllerInterface APIs.
 *
 * This class implements a @c CapabilityAgent that handles the @c Alexa.RangeContoller Interface.
 */
class RangeControllerCapabilityAgent
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::sdkInterfaces::rangeController::RangeControllerObserverInterface
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<RangeControllerCapabilityAgent> {
public:
    /// Alias to improve readability.
    using EndpointIdentifier = avsCommon::sdkInterfaces::endpoints::EndpointIdentifier;
    using RangeState = avsCommon::sdkInterfaces::rangeController::RangeControllerInterface::RangeState;

    /**
     * Create an instance of @c RangeControllerCapabilityAgent.
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param instance A unique non-empty instance name identifying a range controller in endpoint.
     * @param rangeControllerAttributes Used to populate the discovery message.
     * @param rangeController An interface that this object will use to perform the range related operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the event reponse to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @param isProactivelyReported Whether the range property change is proactively reported to Alexa in a change
     * report.
     * @param isRetrievable Whether the range property can be retrieved when Alexa sends a state report request to the
     * endpoint.
     * @param isNonControllable Whether the range property can be controlled or not. This must be @c false for the
     * range property to be controllable. Default is @c false.
     * @return @c nullptr if the inputs are invalid, else a new instance of @c RangeControllerCapabilityAgent.
     */
    static std::shared_ptr<RangeControllerCapabilityAgent> create(
        const EndpointIdentifier& endpointId,
        const std::string& instance,
        const avsCommon::sdkInterfaces::rangeController::RangeControllerAttributes& rangeControllerAttributes,
        std::shared_ptr<avsCommon::sdkInterfaces::rangeController::RangeControllerInterface> rangeController,
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

    /// @name RangeControllerObserverInterface Function
    /// @{
    void onRangeChanged(const RangeState& rangeState, avsCommon::sdkInterfaces::AlexaStateChangeCauseType causeType)
        override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param instance A unique non-empty instance name identifying a range controller in endpoint.
     * @param rangeControllerAttributes Used to populate the discovery message.
     * @param rangeController An interface that this object will use to perform the range related operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the reponse to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @param isProactivelyReported Whether the range property change is proactively reported to Alexa in a change
     * report.
     * @param isRetrievable Whether the range property can be retrieved when Alexa sends a state report request to the
     * endpoint.
     * @param isNonControllable Whether the range property can be controlled or not. This must be @c false for the
     * range property to be controllable. Default is @c false.
     */
    RangeControllerCapabilityAgent(
        const EndpointIdentifier& endpointId,
        const std::string& instance,
        const avsCommon::sdkInterfaces::rangeController::RangeControllerAttributes& rangeControllerAttributes,
        std::shared_ptr<avsCommon::sdkInterfaces::rangeController::RangeControllerInterface> rangeController,
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
     * Builds the configuration json required for the discovery.
     *
     * @return The Json string.
     */
    std::string buildRangeConfigurationJson();

    /**
     * Called on the executor thread to handle the @c SetRangeState directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param payload A @c rapidjson::Document holding the parsed directive payload.
     */
    void executeSetRangeValueDirective(std::shared_ptr<DirectiveInfo> info, rapidjson::Document& payload);

    /**
     * Called on the executor thread to handle the @c AdjustRangeState directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param payload A @c rapidjson::Document holding the parsed directive payload.
     */
    void executeAdjustRangeValueDirective(std::shared_ptr<DirectiveInfo> info, rapidjson::Document& payload);

    /**
     * Called on executor to handle any unknown directives received.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param type The @c ExceptionErrorType.
     */
    void executeUnknownDirective(std::shared_ptr<DirectiveInfo> info, avsCommon::avs::ExceptionErrorType type);

    /**
     * Function validates the specified range is between the maximum and mimimum value provided in the
     * configuration.
     *
     * @param rangeValue The range to be validated.
     */

    bool validateRangeValue(double rangeValue);

    /**
     * Gets the current range value from endpoint and notifies @c ContextManager
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
     * @param result This contains the endpoints response of a controller call.
     */
    void executeSendResponseEvent(
        const std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
        std::pair<avsCommon::avs::AlexaResponseType, std::string> result);

    /**
     * Helper method to build the @c CapabilityState
     *
     * @param rangeState The range state defined using @c RangeState
     */
    avsCommon::avs::CapabilityState buildCapabilityState(const RangeState& rangeState);

    /// Endpoint the capability agent associated to.
    EndpointIdentifier m_endpointId;

    /// The instance name of the range endpoint.
    std::string m_instance;

    /// Whether the range property change is proactively reported to Alexa in a change report.
    bool m_isProactivelyReported;

    /// Whether the range property can be retrieved when Alexa sends a state report request to the endpoint.
    bool m_isRetrievable;

    /// Whether the range property can be controlled or not.
    bool m_isNonControllable;

    /// The range controller attributes used in discovery.
    avsCommon::sdkInterfaces::rangeController::RangeControllerAttributes m_rangeControllerAttributes;

    /// Reference to Range Controller
    std::shared_ptr<avsCommon::sdkInterfaces::rangeController::RangeControllerInterface> m_rangeController;

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c AlexaInterfaceMessageSenderInterface used to send event messages.
    std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> m_responseSender;

    /// The range controller configuration used in discovery.
    avsCommon::sdkInterfaces::rangeController::RangeControllerInterface::RangeControllerConfiguration
        m_rangeControllerConfiguration;

    /// This is the worker thread for the @c RangeController CA.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace rangeController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_RANGECONTROLLER_INCLUDE_RANGECONTROLLER_RANGECONTROLLERCAPABILITYAGENT_H_
