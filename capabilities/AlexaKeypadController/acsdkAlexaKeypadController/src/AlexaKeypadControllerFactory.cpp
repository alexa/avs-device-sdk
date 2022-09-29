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

#include "acsdkAlexaKeypadController/AlexaKeypadControllerFactory.h"
#include "acsdkAlexaKeypadController/AlexaKeypadControllerCapabilityAgent.h"

namespace alexaClientSDK {
namespace acsdkAlexaKeypadController {

avsCommon::utils::Optional<AlexaKeypadControllerFactory::AlexaKeypadControllerCapabilityAgentData>
AlexaKeypadControllerFactory::create(
    const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
    std::shared_ptr<acsdkAlexaKeypadControllerInterfaces::AlexaKeypadControllerInterface> keypadController,
    const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
    const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
    const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender) {
    AlexaKeypadControllerFactory::AlexaKeypadControllerCapabilityAgentData alexaKeypadControllerCapabilityAgentData;
    auto keypadControllerCA = acsdkAlexaKeypadController::AlexaKeypadControllerCapabilityAgent::create(
        endpointId, keypadController, contextManager, responseSender, exceptionSender);
    if (keypadControllerCA) {
        alexaKeypadControllerCapabilityAgentData.capabilityConfigurationInterface = keypadControllerCA;
        alexaKeypadControllerCapabilityAgentData.directiveHandler = keypadControllerCA;
        alexaKeypadControllerCapabilityAgentData.requiresShutdown = keypadControllerCA;
        return avsCommon::utils::Optional<AlexaKeypadControllerFactory::AlexaKeypadControllerCapabilityAgentData>(
            alexaKeypadControllerCapabilityAgentData);
    }

    return avsCommon::utils::Optional<AlexaKeypadControllerFactory::AlexaKeypadControllerCapabilityAgentData>();
}

}  // namespace acsdkAlexaKeypadController
}  // namespace alexaClientSDK
