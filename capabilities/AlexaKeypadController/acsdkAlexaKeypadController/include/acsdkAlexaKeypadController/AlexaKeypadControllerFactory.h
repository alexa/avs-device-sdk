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

#ifndef ACSDKALEXAKEYPADCONTROLLER_ALEXAKEYPADCONTROLLERFACTORY_H_
#define ACSDKALEXAKEYPADCONTROLLER_ALEXAKEYPADCONTROLLERFACTORY_H_

#include "acsdkAlexaKeypadControllerInterfaces/AlexaKeypadControllerInterface.h"
#include <AVSCommon/SDKInterfaces/AlexaInterfaceMessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace acsdkAlexaKeypadController {

/**
 * This factory can be used to create a new @c AlexaKeypadControllerCapabilityAgent and return a generic object that
 * contains certain interfaces which can be used for registering this capability agent to an endpoint.
 */
class AlexaKeypadControllerFactory {
public:
    /// This object used to register the capability agent to an endpoint.
    struct AlexaKeypadControllerCapabilityAgentData {
        /// The interface that this object will use to perform the keypad controller operations.
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> directiveHandler;
        /// The interface that provides the configurations of the capabilities being implemented by this capability
        /// agent.
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface> capabilityConfigurationInterface;
        /// The method for cleaning up this capability agent objects during shutdown.
        std::shared_ptr<avsCommon::utils::RequiresShutdown> requiresShutdown;
    };

    /**
     * Creates a new AlexaKeypadController capability agent configuration
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param keypadController An interface that this object will use to perform the keypad controller operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the reponse to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @return An @c Optional object with no valid value if the inputs are invalid, else a new
     * @c AlexaKeypadControllerCapabilityAgentData object.
     */
    static avsCommon::utils::Optional<AlexaKeypadControllerCapabilityAgentData> create(
        const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
        std::shared_ptr<acsdkAlexaKeypadControllerInterfaces::AlexaKeypadControllerInterface> keypadController,
        const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
        const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender);
};

}  // namespace acsdkAlexaKeypadController
}  // namespace alexaClientSDK

#endif  // ACSDKALEXAKEYPADCONTROLLER_ALEXAKEYPADCONTROLLERFACTORY_H_
