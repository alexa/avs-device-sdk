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

#ifndef ACSDKALEXAPLAYBACKCONTROLLER_ALEXAPLAYBACKCONTROLLERFACTORY_H_
#define ACSDKALEXAPLAYBACKCONTROLLER_ALEXAPLAYBACKCONTROLLERFACTORY_H_

#include <acsdkAlexaPlaybackControllerInterfaces/AlexaPlaybackControllerInterface.h>
#include <AVSCommon/SDKInterfaces/AlexaInterfaceMessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace acsdkAlexaPlaybackController {

/**
 * This factory can be used to create a new @c AlexaPlaybackControllerCapabilityAgent and return a generic object that
 * contains certain interfaces which can be used for registering this capability agent to an endpoint.
 */
class AlexaPlaybackControllerFactory {
public:
    /**
     * This structure contains the interfaces to interact with the AlexaPlaybackController Capability Agent.
     */
    struct AlexaPlaybackControllerCapabilityAgentData {
        /// The interface that this object will use to perform the playback controller operations.
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> directiveHandler;
        /// The interface that provides the configurations of the capabilities being implemented by this capability
        /// agent.
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface> capabilityConfigurationInterface;
        /// The object responsible for cleaning up this capability agent objects during shutdown.
        std::shared_ptr<avsCommon::utils::RequiresShutdown> requiresShutdown;
    };

    /**
     * Creates a new AlexaPlaybackController capability agent instance and exposes the @c AlexaPlaybackController
     * related interfaces to the caller, including the handler for performing playback operations @c directiveHandler,
     * the capability configurations associated to the capability agent instance @c capbilityConfigurationInterface, and
     * the object for cleaning up the capability agent instance during shutdown @c requiresShutdown.
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param playbackController An interface that this object will use to perform the playback controller operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the reponse to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @param isProactivelyReported Whether the playback state properties change is proactively reported to Alexa in a
     * change report.
     * @param isRetrievable Whether the playback state properties can be retrieved when Alexa sends a state report
     * request to the endpoint.
     * @return An @c Optional object with no valid value if the inputs are invalid, else a new
     *  @c AlexaPlaybackControllerCapabilityAgentData object.
     */
    static avsCommon::utils::Optional<AlexaPlaybackControllerCapabilityAgentData> create(
        const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
        std::shared_ptr<acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface> playbackController,
        const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
        const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender,
        bool isProactivelyReported,
        bool isRetrievable);
};

}  // namespace acsdkAlexaPlaybackController
}  // namespace alexaClientSDK

#endif  // ACSDKALEXAPLAYBACKCONTROLLER_ALEXAPLAYBACKCONTROLLERFACTORY_H_
