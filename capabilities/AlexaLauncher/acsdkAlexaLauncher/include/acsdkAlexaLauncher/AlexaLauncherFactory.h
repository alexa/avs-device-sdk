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

#ifndef ACSDKALEXALAUNCHER_ALEXALAUNCHERFACTORY_H_
#define ACSDKALEXALAUNCHER_ALEXALAUNCHERFACTORY_H_

#include <acsdkAlexaLauncherInterfaces/AlexaLauncherInterface.h>
#include <AVSCommon/SDKInterfaces/AlexaInterfaceMessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace acsdkAlexaLauncher {

/**
 * This factory can be used to create a new @c AlexaLauncherCapabilityAgent
 * and return a generic object that contains certain interfaces which can be used
 * for registering this capability agent to an endpoint.
 */
class AlexaLauncherFactory {
public:
    /**
     * This structure contains the interfaces to interact with the AlexaLauncher Capability Agent.
     */
    struct AlexaLauncherCapabilityAgentData {
        /// Interface for handling @c AVSDirectives
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> directiveHandler;
        /// Interface providing CapabilitiesDelegate access to the version and configurations of the capabilities.
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface> capabilityConfigurationInterface;
        std::shared_ptr<avsCommon::utils::RequiresShutdown> requiresShutdown;
    };

    /**
     * Creates a new AlexaLauncher capability agent configuration.
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param launcher An interface that this object will use to perform the launcher operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the response to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @param isProactivelyReported Whether property changes are proactively reported to Alexa in a change
     * report.
     * @param isRetrievable Whether properties can be retrieved when Alexa sends a state report request to the
     * endpoint.
     * @return An @c Optional object with no valid value if the inputs are invalid, else a new
     *  @c AlexaLauncherCapabilityAgentData object.
     */
    static avsCommon::utils::Optional<AlexaLauncherCapabilityAgentData> create(
        const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
        std::shared_ptr<acsdkAlexaLauncherInterfaces::AlexaLauncherInterface> launcher,
        const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
        const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender,
        bool isProactivelyReported,
        bool isRetrievable);
};

}  // namespace acsdkAlexaLauncher
}  // namespace alexaClientSDK

#endif  // ACSDKALEXALAUNCHER_ALEXALAUNCHERFACTORY_H_
