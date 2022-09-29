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

#include "acsdk/AlexaInputController/private/InputControllerCapabilityAgent.h"
#include "acsdk/AlexaInputController/InputControllerFactory.h"

namespace alexaClientSDK {
namespace alexaInputController {

avsCommon::utils::Optional<AlexaInputControllerFactory::InputControllerCapabilityAgentData>
AlexaInputControllerFactory::create(
    const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
    const std::shared_ptr<alexaInputControllerInterfaces::InputControllerInterface>& handler,
    const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
    const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
    const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender,
    bool isProactivelyReported,
    bool isRetrievable) {
    auto inputControllerCA = AlexaInputControllerCapabilityAgent::create(
        endpointId, handler, contextManager, responseSender, exceptionSender, isProactivelyReported, isRetrievable);
    if (inputControllerCA) {
        return avsCommon::utils::Optional<AlexaInputControllerFactory::InputControllerCapabilityAgentData>(
            {inputControllerCA, inputControllerCA, inputControllerCA});
    }
    return avsCommon::utils::Optional<AlexaInputControllerFactory::InputControllerCapabilityAgentData>();
}

}  // namespace alexaInputController
}  // namespace alexaClientSDK
