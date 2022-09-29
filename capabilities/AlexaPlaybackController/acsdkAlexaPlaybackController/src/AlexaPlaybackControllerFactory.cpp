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

#include "acsdkAlexaPlaybackController/AlexaPlaybackControllerFactory.h"

#include "acsdkAlexaPlaybackController/AlexaPlaybackControllerCapabilityAgent.h"

namespace alexaClientSDK {
namespace acsdkAlexaPlaybackController {

avsCommon::utils::Optional<AlexaPlaybackControllerFactory::AlexaPlaybackControllerCapabilityAgentData>
AlexaPlaybackControllerFactory::create(
    const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
    std::shared_ptr<acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface> playbackController,
    const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
    const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
    const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender,
    bool isProactivelyReported,
    bool isRetrievable) {
    AlexaPlaybackControllerFactory::AlexaPlaybackControllerCapabilityAgentData playbackControllerCapabilityAgentData;
    auto playbackControllerCA = acsdkAlexaPlaybackController::AlexaPlaybackControllerCapabilityAgent::create(
        endpointId,
        playbackController,
        contextManager,
        responseSender,
        exceptionSender,
        isProactivelyReported,
        isRetrievable);
    if (playbackControllerCA) {
        playbackControllerCapabilityAgentData.capabilityConfigurationInterface = playbackControllerCA;
        playbackControllerCapabilityAgentData.directiveHandler = playbackControllerCA;
        playbackControllerCapabilityAgentData.requiresShutdown = playbackControllerCA;
        return avsCommon::utils::Optional<AlexaPlaybackControllerFactory::AlexaPlaybackControllerCapabilityAgentData>(
            playbackControllerCapabilityAgentData);
    }

    return avsCommon::utils::Optional<AlexaPlaybackControllerFactory::AlexaPlaybackControllerCapabilityAgentData>();
}

}  // namespace acsdkAlexaPlaybackController
}  // namespace alexaClientSDK
