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

#include "acsdk/AlexaLiveViewController/AlexaLiveViewControllerFactory.h"
#include "acsdk/AlexaLiveViewController/private/AlexaLiveViewControllerCapabilityAgent.h"

namespace alexaClientSDK {
namespace alexaLiveViewController {

avsCommon::utils::Optional<AlexaLiveViewControllerFactory::LiveViewControllerCapabilityAgentData>
AlexaLiveViewControllerFactory::create(
    const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
    const std::shared_ptr<alexaLiveViewControllerInterfaces::LiveViewControllerInterface>& liveViewController,
    const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender,
    const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
    const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
    const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender) {
    AlexaLiveViewControllerFactory::LiveViewControllerCapabilityAgentData liveViewControllerCapabilityAgentData;
    auto liveViewControllerCA = alexaLiveViewController::AlexaLiveViewControllerCapabilityAgent::create(
        endpointId, liveViewController, messageSender, contextManager, responseSender, exceptionSender);
    if (liveViewControllerCA) {
        liveViewControllerCapabilityAgentData.capabilityConfiguration = liveViewControllerCA;
        liveViewControllerCapabilityAgentData.liveViewControllerObserver = liveViewControllerCA;
        liveViewControllerCapabilityAgentData.directiveHandler = liveViewControllerCA;
        liveViewControllerCapabilityAgentData.requiresShutdown = liveViewControllerCA;
        return avsCommon::utils::Optional<AlexaLiveViewControllerFactory::LiveViewControllerCapabilityAgentData>(
            liveViewControllerCapabilityAgentData);
    }

    return avsCommon::utils::Optional<AlexaLiveViewControllerFactory::LiveViewControllerCapabilityAgentData>();
}

}  // namespace alexaLiveViewController
}  // namespace alexaClientSDK
