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

#ifndef ACSDKALEXASEEKCONTROLLER_ALEXASEEKCONTROLLERFACTORY_H_
#define ACSDKALEXASEEKCONTROLLER_ALEXASEEKCONTROLLERFACTORY_H_

#include <memory>

#include <acsdkAlexaSeekControllerInterfaces/AlexaSeekControllerInterface.h>
#include <AVSCommon/SDKInterfaces/AlexaInterfaceMessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace acsdkAlexaSeekController {

/**
 * This factory can be used to create a new @c AlexaSeekControllerCapabilityAgent and return a generic object that
 * contains certain interfaces which can be used for registering this capability agent to an endpoint.
 */
class AlexaSeekControllerFactory {
public:
    /// This structure contains the interfaces used to interact with the AlexaSeekController Capability Agent.
    struct AlexaSeekControllerCapabilityAgentData {
        /// The interface that this object will use to perform the seek controller operations.
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> directiveHandler;
        /// The interface that provides the configurations of the capabilities being implemented by this capability.
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface> capabilityConfigurationInterface;
        /// The object responsible for cleaning up this capability agent's objects during shutdown.
        std::shared_ptr<avsCommon::utils::RequiresShutdown> requiresShutdown;
    };

    /**
     * Creates a new AlexaSeekController capability agent instance and exposes the @c AlexaSeekController
     * related interfaces to the caller, including the handler for performing seek operations @c directiveHandler,
     * the capability configurations associated to the capability agent instance @c capabilityConfigurationInterface,
     * and the object for cleaning up the capability agent instance during shutdown @c requiresShutdown.
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param seekController An interface that this object will use to perform the seek controller operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the reponse to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @param isRetrievable Whether properties can be retrieved when Alexa sends a state report request or the endpoint
     *  responds with an Alexa response to a directive.
     * @return An @c Optional object with no valid value if the inputs are invalid, else a new
     *  @c AlexaSeekControllerCapabilityAgentData object.
     */
    static avsCommon::utils::Optional<AlexaSeekControllerCapabilityAgentData> create(
        const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
        std::shared_ptr<acsdkAlexaSeekControllerInterfaces::AlexaSeekControllerInterface> seekController,
        const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
        const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender,
        bool isRetrievable);
};

}  // namespace acsdkAlexaSeekController
}  // namespace alexaClientSDK

#endif  // ACSDKALEXASEEKCONTROLLER_ALEXASEEKCONTROLLERFACTORY_H_
