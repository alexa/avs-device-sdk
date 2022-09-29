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

#include <acsdk/AlexaRecordController/AlexaRecordControllerFactory.h>
#include <acsdk/AlexaRecordController/private/AlexaRecordControllerCapabilityAgent.h>

namespace alexaClientSDK {
namespace alexaRecordController {

avsCommon::utils::Optional<AlexaRecordControllerFactory::RecordControllerCapabilityAgentData>
AlexaRecordControllerFactory::create(
    const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
    const std::shared_ptr<alexaRecordControllerInterfaces::RecordControllerInterface>& recordController,
    const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
    const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
    const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender,
    bool isRetrievable) {
    AlexaRecordControllerFactory::RecordControllerCapabilityAgentData recordControllerCapabilityAgentData;
    auto recordControllerCA = alexaRecordController::AlexaRecordControllerCapabilityAgent::create(
        endpointId, recordController, contextManager, responseSender, exceptionSender, isRetrievable);
    if (recordControllerCA) {
        recordControllerCapabilityAgentData.capabilityConfigurationInterface = recordControllerCA;
        recordControllerCapabilityAgentData.directiveHandler = recordControllerCA;
        recordControllerCapabilityAgentData.requiresShutdown = recordControllerCA;
        return avsCommon::utils::Optional<AlexaRecordControllerFactory::RecordControllerCapabilityAgentData>(
            recordControllerCapabilityAgentData);
    }

    return avsCommon::utils::Optional<AlexaRecordControllerFactory::RecordControllerCapabilityAgentData>();
}
}  // namespace alexaRecordController
}  // namespace alexaClientSDK
