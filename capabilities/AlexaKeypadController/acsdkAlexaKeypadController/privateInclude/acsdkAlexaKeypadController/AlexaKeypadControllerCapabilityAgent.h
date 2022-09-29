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

#ifndef ACSDKALEXAKEYPADCONTROLLER_PRIVATEINCLUDE_ACSDKALEXAKEYPADCONTROLLER_ALEXAKEYPADCONTROLLERCAPABILITYAGENT_H_
#define ACSDKALEXAKEYPADCONTROLLER_PRIVATEINCLUDE_ACSDKALEXAKEYPADCONTROLLER_ALEXAKEYPADCONTROLLERCAPABILITYAGENT_H_

#include <string>

#include <acsdkAlexaKeypadControllerInterfaces/AlexaKeypadControllerInterface.h>
#include <acsdkAlexaKeypadControllerInterfaces/Keystroke.h>
#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/AlexaInterfaceMessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace acsdkAlexaKeypadController {

/**
 * The @c AlexaKeypadControllerCapabilityAgent is responsible for handling Alexa.KeypadController directives and calls
 * the @c AlexaKeypadControllerInterface APIs.
 *
 * This class implements a @c CapabilityAgent that handles the @c Alexa.KeypadController Interface.
 */
class AlexaKeypadControllerCapabilityAgent
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<AlexaKeypadControllerCapabilityAgent> {
public:
    /**
     * Create an instance of @c AlexaKeypadControllerCapabilityAgent.
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param keypadController An interface that this object will use to perform the keypad controller operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the reponse to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @return @c nullptr if the inputs are invalid, else a new instance of @c AlexaKeypadControllerCapabilityAgent.
     */
    static std::shared_ptr<AlexaKeypadControllerCapabilityAgent> create(
        const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
        const std::shared_ptr<acsdkAlexaKeypadControllerInterfaces::AlexaKeypadControllerInterface>& keypadController,
        const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
        const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender);

    /// @name CapabilityAgent/DirectiveHandlerInterface Functions
    /// @{
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<DirectiveInfo> info) override;
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
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
     * @param endpointId A endpoint to which this capability is associated.
     * @param keypadController An interface that this object will use to perform the keypad controller operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the reponse to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     */
    AlexaKeypadControllerCapabilityAgent(
        const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
        const std::shared_ptr<acsdkAlexaKeypadControllerInterfaces::AlexaKeypadControllerInterface>& keypadController,
        const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
        const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender);

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
        const acsdkAlexaKeypadControllerInterfaces::AlexaKeypadControllerInterface::Response& result);

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
     * Send Alexa.Video interface error response.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param alexaVideoErrorResponseType Alexa.Video interface error response type.
     * @param responseMessage Alexa.Video interface error response message.
     */
    void sendAlexaVideoErrorResponse(
        const std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
        const avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType
            alexaVideoErrorResponseType,
        const std::string& responseMessage);

    /// Endpoint the capability agent associated to.
    avsCommon::sdkInterfaces::endpoints::EndpointIdentifier m_endpointId;

    /// Reference to @c AlexaKeypadControllerInterface
    std::shared_ptr<acsdkAlexaKeypadControllerInterfaces::AlexaKeypadControllerInterface> m_keypadController;

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c AlexaInterfaceMessageSenderInterface used to send event messages.
    std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> m_responseSender;

    /// This is the worker thread for the @c AlexaKeypadControllerCapabilityAgent.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace acsdkAlexaKeypadController
}  // namespace alexaClientSDK
#endif  // ACSDKALEXAKEYPADCONTROLLER_PRIVATEINCLUDE_ACSDKALEXAKEYPADCONTROLLER_ALEXAKEYPADCONTROLLERCAPABILITYAGENT_H_
