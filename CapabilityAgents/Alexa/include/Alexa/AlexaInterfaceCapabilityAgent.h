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

#include <Alexa/AlexaInterfaceMessageSenderInternalInterface.h>
#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/AlexaEventProcessedObserverInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
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
     * Creates an instance of @c AlexaInterfaceCapabilityAgent.
     *
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

    /**
     * Adds an observer which implements @c AlexaEventProcessedObserverInterface.
     *
     * @param observer The @c AlexaEventProcessedObserverInterface.
     */
    void addEventProcessedObserver(
        const std::shared_ptr<avsCommon::sdkInterfaces::AlexaEventProcessedObserverInterface>& observer);

    /**
     * Removes the observer which implements the @c AlexaEventProcessedObserverInterface.
     *
     * @param observer The @c AlexaEventProcessedObserverInterface.
     */
    void removeEventProcessedObserver(
        const std::shared_ptr<avsCommon::sdkInterfaces::AlexaEventProcessedObserverInterface>& observer);

private:
    /**
     * Constructor.
     *
     * @param deviceInfo Structure with information about the Alexa client device.
     * @param endpointId The endpoint to which this capability is associated.
     * @param exceptionEncounteredSender The @c ExceptionEncounteredSender to send exceptions.
     * @param alexaMessageSender The @c AlexaInterfaceMessageSender to send AlexaInterface events.
     */
    AlexaInterfaceCapabilityAgent(
        const avsCommon::utils::DeviceInfo& deviceInfo,
        const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
        std::shared_ptr<AlexaInterfaceMessageSenderInternalInterface> alexaMessageSender);

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
    const avsCommon::utils::DeviceInfo m_deviceInfo;

    /// The endpoint to which this capability instance is associated.
    avsCommon::sdkInterfaces::endpoints::EndpointIdentifier m_endpointId;

    /// The instance of @c AlexaInterfaceMessageSender to send AlexaInterface events.
    std::shared_ptr<AlexaInterfaceMessageSenderInternalInterface> m_alexaMessageSender;

    /// Mutex to protect access to the observer list.
    std::mutex m_observerMutex;

    /// Set of observers that get notified when an EventProcessed directive is received.
    std::set<std::shared_ptr<avsCommon::sdkInterfaces::AlexaEventProcessedObserverInterface>> m_observers;

    /// An executor used for serializing requests on a standalone thread of execution.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace alexa
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALEXA_INCLUDE_ALEXA_ALEXAINTERFACECAPABILITYAGENT_H_
