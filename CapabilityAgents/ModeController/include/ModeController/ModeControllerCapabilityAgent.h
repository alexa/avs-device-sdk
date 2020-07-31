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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_MODECONTROLLER_INCLUDE_MODECONTROLLER_MODECONTROLLERCAPABILITYAGENT_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_MODECONTROLLER_INCLUDE_MODECONTROLLER_MODECONTROLLERCAPABILITYAGENT_H_

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/CapabilityState.h>
#include <AVSCommon/AVS/CapabilityResources.h>
#include <AVSCommon/SDKInterfaces/AlexaInterfaceMessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ModeController/ModeControllerAttributes.h>
#include <AVSCommon/SDKInterfaces/ModeController/ModeControllerInterface.h>
#include <AVSCommon/SDKInterfaces/ModeController/ModeControllerObserverInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/Timing/Timer.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace modeController {

/**
 * The @c ModeControllerCapabilityAgent is responsible for handling Alexa.ModeController directives and
 * calls the @c ModeControllerInterface APIs.
 *
 * This class implements a @c CapabilityAgent that handles the @c Alexa.ModeContoller Interface.
 */
class ModeControllerCapabilityAgent
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::sdkInterfaces::modeController::ModeControllerObserverInterface
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<ModeControllerCapabilityAgent> {
public:
    /// Alias to improve readability.
    using EndpointIdentifier = avsCommon::sdkInterfaces::endpoints::EndpointIdentifier;
    using ModeState = avsCommon::sdkInterfaces::modeController::ModeControllerInterface::ModeState;

    /**
     * Create an instance of @c ModeControllerCapabilityAgent.
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param instance A unique non-empty instance name identifying a mode controller in endpoint.
     * @param modeControllerAttributes Used to populate the discovery message.
     * @param modeController An interface that this object will use to perform the mode related operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the reponse to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @param isProactivelyReported Whether the mode property change is proactively reported to Alexa in a change
     * report.
     * @param isRetrievable Whether the mode property can be retrieved when Alexa sends a state report request to the
     * endpoint.
     * @param isNonControllable Whether the mode property can be controlled or not. This must be @c false for the
     * mode property to be controllable. Default is @c false.
     * @return @c nullptr if the inputs are invalid, else a new instance of @c ModeControllerCapabilityAgent.
     */
    static std::shared_ptr<ModeControllerCapabilityAgent> create(
        const EndpointIdentifier& endpointId,
        const std::string& instance,
        const avsCommon::sdkInterfaces::modeController::ModeControllerAttributes& modeControllerAttributes,
        std::shared_ptr<avsCommon::sdkInterfaces::modeController::ModeControllerInterface> modeController,
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

    /// @name ModeControllerObserverInterface Function
    /// @{
    void onModeChanged(const ModeState& mode, avsCommon::sdkInterfaces::AlexaStateChangeCauseType cause) override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param instance A unique non-empty instance name identifying a mode controller in endpoint.
     * @param modeControllerAttributes Used to populate the discovery message.
     * @param modeController An interface that this object will use to perform the mode related operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the event reponse to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @param isProactivelyReported Whether the mode property change is proactively reported to Alexa in a change
     * report.
     * @param isRetrievable Whether the mode property can be retrieved when Alexa sends a state report request to the
     * endpoint.
     * @param isNonControllable Whether the mode property can be controlled or not. This must be @c false for the
     * mode property to be controllable. Default is @c false.
     */
    ModeControllerCapabilityAgent(
        const EndpointIdentifier& endpointId,
        const std::string& instance,
        const avsCommon::sdkInterfaces::modeController::ModeControllerAttributes& modeControllerAttributes,
        std::shared_ptr<avsCommon::sdkInterfaces::modeController::ModeControllerInterface> modeController,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> responseSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        bool isProactivelyReported,
        bool isRetrievable,
        bool isNonControllable);

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
    std::string buildModeConfigurationJson();

    /**
     * This function handles a @c SetMode directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param payload A @c rapidjson::Document holding the parsed directive payload.
     */
    void executeSetModeDirective(std::shared_ptr<DirectiveInfo> info, rapidjson::Document& payload);

    /**
     * This function handles a @c AdjustMode directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param payload A @c rapidjson::Document holding the parsed directive payload.
     */
    void executeAdjustModeDirective(std::shared_ptr<DirectiveInfo> info, rapidjson::Document& payload);

    /**
     * Called on executor to handle any unknown directives received.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param type The @c ExceptionErrorType.
     */
    void executeUnknownDirective(std::shared_ptr<DirectiveInfo> info, avsCommon::avs::ExceptionErrorType type);

    /**
     * Validates if the specified mode is in the supported modes provided as part of the
     * configuration.
     *
     * @param mode The mode to be validated.
     */

    bool validateMode(const std::string& mode);

    /**
     * Called on the executor thread to handle the @c SetMode directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param payload A @c rapidjson::Document holding the parsed directive payload.
     */
    void executeSetMode(
        const std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
        rapidjson::Document& payload);

    /**
     * Called on the executor thread to handle the @c AdjustMode directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param payload A @c rapidjson::Document holding the parsed directive payload.
     */
    void executeAdjustMode(
        const std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
        rapidjson::Document& payload);

    /**
     * Gets the current mode  from endpoint and notifies @c ContextManager
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
     * @param modeState The mode state defined using @c ModeState
     */
    avsCommon::avs::CapabilityState buildCapabilityState(const ModeState& modeState);

    /// Endpoint the capability agent associated to.
    EndpointIdentifier m_endpointId;

    /// The instance name of the mode capability.
    std::string m_instance;

    /// Whether the mode property change is proactively reported to Alexa in a change report.
    bool m_isProactivelyReported;

    /// Whether the mode property can be retrieved when Alexa sends a state report request to the endpoint.
    bool m_isRetrievable;

    /// Whether the mode property can be controlled or not.
    bool m_isNonControllable;

    /// The mode controller attributes used in discovery.
    avsCommon::sdkInterfaces::modeController::ModeControllerAttributes m_modeControllerAttributes;

    /// The mode controller configuration.
    avsCommon::sdkInterfaces::modeController::ModeControllerInterface::ModeControllerConfiguration
        m_modeControllerConfiguration;

    /// Reference to Mode Controller
    std::shared_ptr<avsCommon::sdkInterfaces::modeController::ModeControllerInterface> m_modeController;

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c AlexaInterfaceMessageSenderInterface used to send event messages.
    std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> m_responseSender;

    // The pair to hold the @c ModeState and @c AlexaStateChangeCauseType
    using ModeAndCauseTypePair = std::pair<ModeState, avsCommon::sdkInterfaces::AlexaStateChangeCauseType>;

    // Map of context request token and @c ModeAndCauseTypePair for handling the ChangeReport.
    std::map<avsCommon::sdkInterfaces::ContextRequestToken, ModeAndCauseTypePair> m_pendingChangeReports;

    /// This is the worker thread for the @c ModeController CA.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace modeController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_MODECONTROLLER_INCLUDE_MODECONTROLLER_MODECONTROLLERCAPABILITYAGENT_H_
