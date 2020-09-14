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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_APIGATEWAY_INCLUDE_APIGATEWAY_APIGATEWAYCAPABILITYAGENT_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_APIGATEWAY_INCLUDE_APIGATEWAY_APIGATEWAYCAPABILITYAGENT_H_

#include <memory>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/SDKInterfaces/AVSGatewayManagerInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace apiGateway {

/**
 * This class handles the SetGateway Directive from AVS.
 */
class ApiGatewayCapabilityAgent
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public avsCommon::utils::RequiresShutdown {
public:
    /**
     * Creates an instance of the @c ApiGatewayCapabilityAgent.
     *
     * @param avsGatewayManager The @c AVSGatewayManager instance to update the gateway URL.
     * @param exceptionEncounteredSender The @c ExceptionEncounteredSender to notify errors processing directives.
     * @return A new instance of the @c ApiGatewayCapabilityAgent.
     */
    static std::shared_ptr<ApiGatewayCapabilityAgent> create(
        std::shared_ptr<avsCommon::sdkInterfaces::AVSGatewayManagerInterface> avsGatewayManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender);

    /// @name CapabilityAgent Functions
    /// @{
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    /// @}

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;
    /// @}

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param avsGatewayManager The @c AVSGatewayManager instance to update the gateway URL.
     * @param exceptionEncounteredSender The @c ExceptionEncounteredSender to notify errors processing directives.
     */
    ApiGatewayCapabilityAgent(
        std::shared_ptr<avsCommon::sdkInterfaces::AVSGatewayManagerInterface> avsGatewayManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender);

    /**
     * Handles the directives received on the executor thread.
     *
     * @param info The input @c DirectiveInfo.
     */
    void executeHandleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info);

    /**
     * Sets handling completed on the directive.
     *
     * @param info The pointer to @c DirectiveInfo.
     */
    void executeSetHandlingCompleted(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info);

    /**
     * Removes the directive from the @c CapabilityAgents processing queue.
     *
     * @param info The pointer to @c DirectiveInfo
     */
    void removeDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info);

    /**
     * Logs the error and sends the exception encountered message.
     *
     * @param info The pointer to @c DirectiveInfo.
     * @param errorMessage The error message string.
     * @param errorType The @c ExceptionErrorType.
     */
    void executeSendExceptionEncountered(
        std::shared_ptr<DirectiveInfo> info,
        const std::string& errorMessage,
        avsCommon::avs::ExceptionErrorType errorType);

    /// Set of capability configurations that will get published in Discovery.
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> m_capabilityConfigurations;

    /// Reference to the @c AVSGatewayManager to update the gateway URL from SetGateway directive.
    std::shared_ptr<avsCommon::sdkInterfaces::AVSGatewayManagerInterface> m_avsGatewayManager;

    /// An executor used for serializing requests on a standalone thread of execution.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace apiGateway
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_APIGATEWAY_INCLUDE_APIGATEWAY_APIGATEWAYCAPABILITYAGENT_H_
