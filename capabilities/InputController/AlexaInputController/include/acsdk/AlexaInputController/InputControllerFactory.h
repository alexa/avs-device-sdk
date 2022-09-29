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

#ifndef ACSDK_ALEXAINPUTCONTROLLER_INPUTCONTROLLERFACTORY_H_
#define ACSDK_ALEXAINPUTCONTROLLER_INPUTCONTROLLERFACTORY_H_

#include <memory>

#include <acsdk/AlexaInputControllerInterfaces/InputControllerInterface.h>

#include <AVSCommon/SDKInterfaces/AlexaInterfaceMessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveSequencerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace alexaInputController {

/**
 * This factory can be used to create an AlexaInputControllerFactory object which could be a parameter for Capability
 * Agent construction.
 */
class AlexaInputControllerFactory {
private:
    /**
     * This object contains the interfaces to interact with the AlexaInputController Capability Agent.
     */
    struct InputControllerCapabilityAgentData {
        /// An interface used to handle Alexa.InputController directives.
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::DirectiveHandlerInterface> directiveHandler;
        /// An interface used to provide configurations of the capabilities being implemented by this capability agent.
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::CapabilityConfigurationInterface>
            capabilityConfigurationInterface;
        /// The object responsible for cleaning up this capability agent's objects during shutdown.
        std::shared_ptr<alexaClientSDK::avsCommon::utils::RequiresShutdown> requiresShutdown;
    };

public:
    /**
     * Creates a new InputController capability agent configuration.
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param inputController An interface that this object will use to perform the input controller operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the response to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @param isProactivelyReported Whether the input properties change is proactively reported to AVS in a
     * change report.
     * @param isRetrievable Whether the input properties can be retrieved when AVS sends a state report
     * request to the endpoint.
     * @return An @c Optional object containing an instance of @c InputControllerCapabilityAgentData object if
     * successful.
     */
    static avsCommon::utils::Optional<InputControllerCapabilityAgentData> create(
        const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
        const std::shared_ptr<alexaInputControllerInterfaces::InputControllerInterface>& inputController,
        const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
        const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender,
        bool isProactivelyReported,
        bool isRetrievable);
};

}  // namespace alexaInputController
}  // namespace alexaClientSDK

#endif  // ACSDK_ALEXAINPUTCONTROLLER_INPUTCONTROLLERFACTORY_H_
