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

#include <acsdk/AlexaChannelController/AlexaChannelControllerFactory.h>

#include <acsdk/AlexaChannelController/private/AlexaChannelControllerCapabilityAgent.h>

namespace alexaClientSDK {
namespace alexaChannelController {

avsCommon::utils::Optional<AlexaChannelControllerFactory::ChannelControllerCapabilityAgentData>
AlexaChannelControllerFactory::create(
    const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
    const std::shared_ptr<alexaChannelControllerInterfaces::ChannelControllerInterface>& channelController,
    const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
    const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
    const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender,
    bool isProactivelyReported,
    bool isRetrievable) {
    AlexaChannelControllerFactory::ChannelControllerCapabilityAgentData channelControllerCapabilityAgentData;
    auto channelControllerCA = AlexaChannelControllerCapabilityAgent::create(
        endpointId,
        channelController,
        contextManager,
        responseSender,
        exceptionSender,
        isProactivelyReported,
        isRetrievable);
    if (channelControllerCA) {
        channelControllerCapabilityAgentData.capabilityConfigurationInterface = channelControllerCA;
        channelControllerCapabilityAgentData.directiveHandler = channelControllerCA;
        channelControllerCapabilityAgentData.requiresShutdown = channelControllerCA;
        return avsCommon::utils::Optional<AlexaChannelControllerFactory::ChannelControllerCapabilityAgentData>(
            channelControllerCapabilityAgentData);
    }

    return avsCommon::utils::Optional<AlexaChannelControllerFactory::ChannelControllerCapabilityAgentData>();
}

}  // namespace alexaChannelController
}  // namespace alexaClientSDK
