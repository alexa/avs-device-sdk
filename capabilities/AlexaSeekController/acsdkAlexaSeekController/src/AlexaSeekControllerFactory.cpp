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

#include "acsdkAlexaSeekController/AlexaSeekControllerFactory.h"

#include "acsdkAlexaSeekController/AlexaSeekControllerCapabilityAgent.h"

namespace alexaClientSDK {
namespace acsdkAlexaSeekController {

avsCommon::utils::Optional<AlexaSeekControllerFactory::AlexaSeekControllerCapabilityAgentData>
AlexaSeekControllerFactory::create(
    const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
    std::shared_ptr<acsdkAlexaSeekControllerInterfaces::AlexaSeekControllerInterface> seekController,
    const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
    const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
    const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender,
    bool isRetrievable) {
    AlexaSeekControllerFactory::AlexaSeekControllerCapabilityAgentData alexaSeekControllerCapabilityAgentData;
    auto seekControllerCA = acsdkAlexaSeekController::AlexaSeekControllerCapabilityAgent::create(
        endpointId, seekController, contextManager, responseSender, exceptionSender, isRetrievable);
    if (seekControllerCA) {
        alexaSeekControllerCapabilityAgentData.capabilityConfigurationInterface = seekControllerCA;
        alexaSeekControllerCapabilityAgentData.directiveHandler = seekControllerCA;
        alexaSeekControllerCapabilityAgentData.requiresShutdown = seekControllerCA;
        return avsCommon::utils::Optional<AlexaSeekControllerFactory::AlexaSeekControllerCapabilityAgentData>(
            alexaSeekControllerCapabilityAgentData);
    }

    return avsCommon::utils::Optional<AlexaSeekControllerFactory::AlexaSeekControllerCapabilityAgentData>();
}

}  // namespace  acsdkAlexaSeekController
}  // namespace alexaClientSDK
