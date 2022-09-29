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

#ifndef ACSDK_ALEXACHANNELCONTROLLER_ALEXACHANNELCONTROLLERFACTORY_H_
#define ACSDK_ALEXACHANNELCONTROLLER_ALEXACHANNELCONTROLLERFACTORY_H_

#include <memory>

#include <acsdk/AlexaChannelControllerInterfaces/ChannelControllerInterface.h>

#include <AVSCommon/SDKInterfaces/AlexaInterfaceMessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveSequencerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace alexaChannelController {

/**
 * This factory can be used to create a AlexaChannelControllerFactory object which could be a parameter for Capability
 * Agent construction.
 */
class AlexaChannelControllerFactory {
private:
    /**
     * This object contains the interfaces to interact with the AlexaChannelController Capability Agent.
     */
    struct ChannelControllerCapabilityAgentData {
        /// An interface used to handle Alexa.ChannelController directives.
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> directiveHandler;
        /// An interface used to provide configurations of the capabilities being implemented by this capability agent.
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface> capabilityConfigurationInterface;
        /// Instance of @c RequiresShutdown used for cleaning up the capability agent during shutdown.
        std::shared_ptr<avsCommon::utils::RequiresShutdown> requiresShutdown;
    };

public:
    /**
     * Creates a new ChannelController capability agent configuration.
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param channelController An interface that this object will use to perform the channel controller operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the response to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @param isProactivelyReported Whether the channel properties change is proactively reported to AVS in a
     * change report.
     * @param isRetrievable Whether the channel properties can be retrieved when AVS sends a state report
     * request to the endpoint.
     * @return An @c Optional object containing an instance of @c ChannelControllerCapabilityAgentData object if
     * successful, otherwise an @c Optional object with no valid value.
     */
    static avsCommon::utils::Optional<ChannelControllerCapabilityAgentData> create(
        const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
        const std::shared_ptr<alexaChannelControllerInterfaces::ChannelControllerInterface>& channelController,
        const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
        const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender,
        bool isProactivelyReported,
        bool isRetrievable);
};

}  // namespace alexaChannelController
}  // namespace alexaClientSDK

#endif  // ACSDK_ALEXACHANNELCONTROLLER_ALEXACHANNELCONTROLLERFACTORY_H_
