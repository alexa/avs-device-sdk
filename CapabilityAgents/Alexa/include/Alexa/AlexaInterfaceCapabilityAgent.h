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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALEXA_INCLUDE_ALEXA_ALEXAINTERFACECAPABILITYAGENT_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALEXA_INCLUDE_ALEXA_ALEXAINTERFACECAPABILITYAGENT_H_

#include <memory>
#include <mutex>
#include <set>

#include <acsdkAlexaEventProcessedNotifierInterfaces/AlexaEventProcessedNotifierInterface.h>
#include <acsdkManufactory/Annotated.h>
#include <Alexa/AlexaInterfaceMessageSenderInternalInterface.h>
#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/AlexaEventProcessedObserverInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointBuilderInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointCapabilitiesRegistrarInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/SDKInterfaces/Endpoints/DefaultEndpointAnnotation.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alexa {

/**
 * Capability Agent to handle directives under the Alexa Namespace.
 */
class AlexaInterfaceCapabilityAgent : public avsCommon::avs::CapabilityAgent {
public:
    /**
     * Creates an instance of @c AlexaInterfaceCapabilityAgent for the default endpoint.
     *
     * @param deviceInfo Structure with information about the Alexa client device.
     * @param exceptionEncounteredSender The @c ExceptionEncounteredSender to send exceptions.
     * @param alexaMessageSender The @c AlexaInterfaceMessageSender to send AlexaInterface events.
     * @param alexaEventProcessedNotifier The @c AlexaEventProcessedNotifierInterface to notify observers of
     * AlexaEventProcessed events.
     * @param endpointCapabilitiesRegistrar The @c EndpointCapabilitiesRegistrarInterface for the default endpoint
     * (annotated with DefaultEndpointAnnotation), so that the SpeakerManager can register itself as a capability
     * with the default endpoint.
     * @return an instance of @c AlexaInterfaceCapabilityAgent on success, @c nullptr otherwise.
     */
    static std::shared_ptr<AlexaInterfaceCapabilityAgent> createDefaultAlexaInterfaceCapabilityAgent(
        const std::shared_ptr<avsCommon::utils::DeviceInfo>& deviceInfo,
        const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>&
            exceptionEncounteredSender,
        const std::shared_ptr<AlexaInterfaceMessageSenderInternalInterface>& alexaMessageSender,
        const std::shared_ptr<acsdkAlexaEventProcessedNotifierInterfaces::AlexaEventProcessedNotifierInterface>&
            alexaEventProcessedNotifier,
        const acsdkManufactory::Annotated<
            avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
            avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>&
            endpointCapabilitiesRegistrar);

    /**
     * Creates an instance of @c AlexaInterfaceCapabilityAgent.
     *
     * @param deviceInfo Structure with information about the Alexa client device.
     * @param endpointId The @c EndpointIdentifier for the endpoint that will own this capability.
     * @param exceptionEncounteredSender The @c ExceptionEncounteredSender to send exceptions.
     * @param alexaMessageSender The @c AlexaInterfaceMessageSender to send AlexaInterface events.
     * @return an instance of @c AlexaInterfaceCapabilityAgent on success, @c nullptr otherwise.
     */
    static std::shared_ptr<AlexaInterfaceCapabilityAgent> create(
        const std::shared_ptr<avsCommon::utils::DeviceInfo>& deviceInfo,
        const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
        const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>&
            exceptionEncounteredSender,
        const std::shared_ptr<AlexaInterfaceMessageSenderInternalInterface>& alexaMessageSender);

    /**
     * Creates an instance of @c AlexaInterfaceCapabilityAgent.
     *
     * @deprecated
     * @param deviceInfo Structure with information about the Alexa client device.
     * @param endpointId The endpoint to which this capability is associated.
     * @param exceptionEncounteredSender The @c ExceptionEncounteredSender to send exceptions.
     * @param alexaMessageSender The @c AlexaInterfaceMessageSender to send AlexaInterface events.
     * @return an instance of @c AlexaInterfaceCapabilityAgent on success, @c nullptr otherwise.
     */
    static std::shared_ptr<AlexaInterfaceCapabilityAgent> create(
        const avsCommon::utils::DeviceInfo& deviceInfo,
        const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
        std::shared_ptr<AlexaInterfaceMessageSenderInternalInterface> alexaMessageSender);

    /// @name DirectiveHandlerInterface and CapabilityAgent Functions
    /// @{
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    /// @}

    /**
     * Get the capability configuration for this agent.
     *
     * @return The capability configuration.
     */
    avsCommon::avs::CapabilityConfiguration getCapabilityConfiguration();

private:
    /**
     * Constructor.
     *
     * @param deviceInfo Structure with information about the Alexa client device.
     * @param endpointId The endpoint to which this capability is associated.
     * @param exceptionEncounteredSender The @c ExceptionEncounteredSender to send exceptions.
     * @param alexaMessageSender The @c AlexaInterfaceMessageSender to send AlexaInterface events.
     * @param alexaEventProcessedNotifier The @c AlexaEventProcessedNotifierInterface used for notifying observers of
     * AlexaEventProcessed directives.
     */
    AlexaInterfaceCapabilityAgent(
        const std::shared_ptr<avsCommon::utils::DeviceInfo>& deviceInfo,
        const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
        std::shared_ptr<AlexaInterfaceMessageSenderInternalInterface> alexaMessageSender,
        const std::shared_ptr<acsdkAlexaEventProcessedNotifierInterfaces::AlexaEventProcessedNotifierInterface>&
            alexaEventProcessedNotifier);

    /**
     * Handles the @c EventProcessed directive.
     *
     * @param directive The @c AVSDirective for the @c EventProcessedDirective.
     * @return True if the directive is successfully handled, else false.
     */
    bool executeHandleEventProcessed(const std::shared_ptr<avsCommon::avs::AVSDirective>& directive);

    /**
     * Removes the directive from the @c CapabilityAgent's processing queue.
     *
     * @param info The pointer to the @c DirectiveInfo.
     */
    void removeDirective(const std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo>& info);

    /**
     * Log an error, send an exception encountered message, report the directive as failed, and remove it.
     *
     * @param info The pointer to the @c DirectiveInfo.
     * @param errorType The @c ExceptionErrorType.
     * @param errorMessage The error message string.
     */
    void executeSendExceptionEncounteredAndReportFailed(
        const std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo>& info,
        avsCommon::avs::ExceptionErrorType errorType,
        const std::string& errorMessage);

    /**
     * Log an error and send an Alexa.ErrorReponse event.
     *
     * @param info The pointer to the @c DirectiveInfo.
     * @param errorType The @c ErrorResponseType.
     * @param errorMessage The error message string.
     */
    void executeSendErrorResponse(
        const std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo>& info,
        avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface::ErrorResponseType errorType,
        const std::string& errorMessage);

    /// The device information which contains the default endpoint ID for the device.
    std::shared_ptr<avsCommon::utils::DeviceInfo> m_deviceInfo;

    /// The endpoint to which this capability instance is associated.
    avsCommon::sdkInterfaces::endpoints::EndpointIdentifier m_endpointId;

    /// The instance of @c AlexaInterfaceMessageSender to send AlexaInterface events.
    std::shared_ptr<AlexaInterfaceMessageSenderInternalInterface> m_alexaMessageSender;

    /// The instance of @c AlexaEventProcessedNotifierInterface to send AlexaInterface events.
    std::shared_ptr<acsdkAlexaEventProcessedNotifierInterfaces::AlexaEventProcessedNotifierInterface>
        m_alexaEventProcessedNotifier;

    /// An executor used for serializing requests on a standalone thread of execution.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace alexa
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALEXA_INCLUDE_ALEXA_ALEXAINTERFACECAPABILITYAGENT_H_
